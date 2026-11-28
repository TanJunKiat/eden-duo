// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace etrian::bistro_bounty {
inline constexpr unsigned regular_target=5;
inline constexpr unsigned labyrinth_floors=25;

// These offsets and bits are shared with the exact-build guest helper.
inline constexpr uint32_t contract_offset=144, party_offset=152;
inline constexpr uint64_t report_requested=1u<<20, reported=1u<<21, paused=1u<<22;
inline constexpr size_t offer_count=4;
struct Offer {
    uint16_t species=0;
    bool completed=false;
    bool operator==(const Offer&) const = default;
};
using Offers=std::array<Offer,offer_count>;

// An existing offer never changes merely because the Bistro was opened again
// or an active contract was abandoned. Additional discoveries fill gaps only.
template<class Draw> void fill_offers(Offers& offers,std::vector<uint16_t> candidates,Draw draw) {
    std::ranges::sort(candidates);
    candidates.erase(std::unique(candidates.begin(),candidates.end()),candidates.end());
    std::erase_if(candidates,[&](uint16_t id) {
        return !id || std::ranges::find(offers,id,&Offer::species)!=offers.end();
    });
    for(auto& offer:offers) if(!offer.species && !candidates.empty()) {
        const size_t i=draw(candidates.size());
        if(i>=candidates.size()) return;
        offer.species=candidates[i];
        candidates.erase(candidates.begin()+i);
    }
}
constexpr uint16_t species_of(uint64_t word) { return uint16_t(word); }
constexpr unsigned kills_of(uint64_t word) { return (word>>16)&15; }
constexpr bool valid_word(uint64_t word,uint32_t epoch) {
    if(!word) return true;
    const auto flags=word&(report_requested|reported);
    return species_of(word) && (word>>32)==epoch && !(word&0xff800000ULL) &&
           kills_of(word)<=regular_target && flags!=(report_requested|reported) && !(flags && (word&paused)) &&
           (!flags || kills_of(word)==regular_target);
}
constexpr bool ready_to_request(uint64_t word) {
    return species_of(word) && kills_of(word)==regular_target && !(word&(report_requested|reported|paused));
}

struct FloorCandidates {
    unsigned floor;
    std::span<const uint16_t> regular_species;
};

// Each eligible species gets one entry regardless of how many floors it
// inhabits. A caller draws a uniform index after authenticating visited_mask.
inline std::vector<uint16_t> eligible_species(std::span<const FloorCandidates> floors,
                                              uint32_t visited_mask) {
    std::vector<uint16_t> pool;
    for(const auto& f:floors) {
        if(!f.floor || f.floor>labyrinth_floors || !(visited_mask & (1u<<(f.floor-1)))) continue;
        for(const auto id:f.regular_species) if(id) pool.push_back(id);
    }
    std::ranges::sort(pool);
    pool.erase(std::unique(pool.begin(),pool.end()),pool.end());
    return pool;
}

// A conservative session-local fallback for floors witnessed in the live map
// after validating its controller, floor and save, not floor paint or hints.
class ExploredFloors {
public:
    constexpr void refresh(uint64_t save,uint32_t epoch) {
        if(!save || save!=save_ || epoch!=epoch_) mask_=0;
        save_=save;
        epoch_=epoch;
    }

    constexpr bool visit(uint64_t save,uint32_t epoch,unsigned floor) {
        if(!save || save!=save_ || epoch!=epoch_ || !floor || floor>labyrinth_floors) return false;
        mask_|=1u<<(floor-1);
        return true;
    }

    std::vector<uint16_t> pool(std::span<const FloorCandidates> floors) const {
        return eligible_species(floors,mask_);
    }

private:
    uint64_t save_=0;
    uint32_t epoch_=0, mask_=0;
};

// Cumulative native EXP thresholds for this level and the next, not the
// character's current EXP or the remaining EXP needed for the next level.
constexpr int32_t regular_reward(int32_t current_threshold,int32_t next_threshold) {
    if(current_threshold<0 || next_threshold<=current_threshold) return 0;
    return (next_threshold-current_threshold)/10;
}

struct RewardMember {
    int level, cap;
    int32_t current_threshold, next_threshold;
};

// Match the native five-entry town award packet. Reject the entire award if
// a non-capped party member's level or table entry is missing or malformed.
constexpr std::optional<std::array<int32_t,5>> plan_regular_awards(std::span<const RewardMember> roster) {
    std::array<int32_t,5> result{};
    if(roster.empty() || roster.size()>result.size()) return {};
    for(size_t i=0;i<roster.size();++i) {
        const auto& member=roster[i];
        if(member.level<1 || member.cap>99 || member.level>member.cap) return {};
        if(member.level==member.cap) continue;
        if(member.current_threshold<0 || member.next_threshold<=member.current_threshold) return {};
        result[i]=regular_reward(member.current_threshold,member.next_threshold);
    }
    return result;
}

// Reference policy only. The guest must authenticate ordinary victories,
// individual deaths, save identity, and the native claim/payout separately.
class Contract {
public:
    constexpr Contract(uint64_t id,uint32_t epoch,uint16_t species)
        : id{id}, epoch{epoch}, species{species} {}

    constexpr unsigned progress() const { return kills; }

    constexpr bool record_victory(uint64_t event_id,uint32_t event_epoch,uint64_t sequence,
                                  std::span<const uint16_t> defeated) {
        if(!matches(event_id,event_epoch) || claimed || sequence<=last_victory) return false;
        last_victory=sequence;
        for(const auto enemy:defeated) {
            if(kills==regular_target) break;
            if(enemy==species) ++kills;
        }
        return true;
    }

    constexpr bool claimable(uint64_t request_id,uint32_t request_epoch) const {
        return matches(request_id,request_epoch) && !claimed && kills==regular_target;
    }

    constexpr bool mark_claimed(uint64_t request_id,uint32_t request_epoch) {
        if(!claimable(request_id,request_epoch)) return false;
        claimed=true;
        return true;
    }

private:
    constexpr bool matches(uint64_t other_id,uint32_t other_epoch) const {
        return species && id && other_id==id && other_epoch==epoch;
    }

    uint64_t id, last_victory=0;
    uint32_t epoch;
    uint16_t species;
    unsigned kills=0;
    bool claimed=false;
};
} // namespace etrian::bistro_bounty
