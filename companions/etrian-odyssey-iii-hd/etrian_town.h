// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "etrian_battle.h"
#include "etrian_expedition.h"

namespace etrian {
using TownFormation = std::vector<uint8_t>;
struct TownSnapshot {
    uint64_t source=0;
    ExpeditionParty party;
    std::optional<TownFormation> formation;
    std::optional<unsigned> bag_free,threads;
    TownQuestBoard quests;
    bool operator==(const TownSnapshot&) const = default;
};
inline constexpr size_t town_report_preview=2, town_quest_page_size=7;

// TownManager's logical destination. Its presence does not by itself prove
// that the destination's menu has finished opening.
enum class TownFacility : int32_t { plaza=1, inn, shop, bistro, guild, port, senate, entrance };
template<class Reader> std::optional<TownFacility> read_town_facility(const Reader& memory,uint64_t base) {
    using quest_data::follow;
    using quest_data::read;
    const auto scene_slot=[&] { return follow(memory,base,{0x618e708,0x58,0xb8,0}); };
    const auto manager_slot=[&] { return follow(memory,base,{0x61a6258,0x18,0xc0,0,0xb8,0}); };
    const auto scene=scene_slot();
    int32_t kind=0,id=0,after=0;
    if(!read(memory,scene,0x44,kind) || kind!=3) return {};
    const auto manager=manager_slot();
    uint64_t root=0,root_after=0;
    if(!read(memory,manager,0x18,root) || !root || !read(memory,manager,0x20,id) || id<1 || id>8) return {};
    if(scene_slot()!=scene || manager_slot()!=manager || !read(memory,scene,0x44,kind) || kind!=3 ||
       !read(memory,manager,0x18,root_after) || root_after!=root ||
       !read(memory,manager,0x20,after) || after!=id) return {};
    return static_cast<TownFacility>(id);
}

// GameTimeAddDay updates the signed, saturating counter at PARTY_DATA+0x1c.
// Unavailable is distinct from day zero, and save identity is rechecked.
template<class Reader> std::optional<uint32_t> read_armoroad_day(const Reader& memory,uint64_t base,uint64_t save) {
    using quest_data::follow;
    using quest_data::read;
    const auto save_slot=[&] { return follow(memory,base,{0x639a810,0,0xb8,0,0x10}); };
    if(!save || save_slot()!=save) return {};
    const auto party=follow(memory,save,{0x20});
    int32_t day=-1;
    if(!read(memory,party,0x1c,day) || day<0 || save_slot()!=save || follow(memory,save,{0x20})!=party) return {};
    return static_cast<uint32_t>(day);
}

struct GuildMember {
    std::string name,job;
    uint8_t level=0;
    bool active=false;
    bool operator==(const GuildMember&) const = default;
};
using GuildRoster=std::optional<std::vector<GuildMember>>;
inline constexpr unsigned guild_page_size=8;

template<class Reader> GuildRoster read_guild_roster(const Reader& memory,uint64_t base,uint64_t save) {
    using quest_data::follow;
    using quest_data::read;
    const auto save_slot=[&] { return follow(memory,base,{0x639a810,0,0xb8,0,0x10}); };
    if(!save || save_slot()!=save || read_town_facility(memory,base)!=TownFacility::guild) return {};
    const auto roster_slot=[&] { return follow(memory,save,{0x20,0x20}); };
    const auto roster=roster_slot();
    uint64_t count=0,after=0;
    if(!read(memory,roster,0x18,count) || count>30) return {};
    // Native eE_CHAR_CLASS values; use English class names until the localized
    // class lookup (including its sex/variant selector) has been verified.
    constexpr std::array jobs{"Sovereign","Gladiator","Hoplite","Buccaneer","Ninja","Monk",
                              "Zodiac","Wildling","Arbalist","Farmer","Shogun","Yggdroid"};
    std::vector<GuildMember> members;
    for(uint64_t i=0;i<count;++i) {
        uint64_t character=0;
        uint8_t flags=0,level=0;
        int32_t job=-1;
        std::string name;
        if(!read(memory,roster,0x20+8*i,character)) return {};
        if(!character) continue;
        if(!read(memory,character,0x10,flags)) return {};
        if(!(flags&1)) continue;
        if(!read(memory,character,0x13,level) || !level || level>99 ||
           !read(memory,character,0x14,job) || !read_character_name(memory,character,name) || name.empty()) return {};
        members.push_back({std::move(name),job>=0 && size_t(job)<jobs.size()?jobs[job]:"Job unavailable",level,bool(flags&2)});
    }
    if(save_slot()!=save || roster_slot()!=roster || !read(memory,roster,0x18,after) || after!=count ||
       read_town_facility(memory,base)!=TownFacility::guild) return {};
    return members;
}

template<class Reader> TownSnapshot read_town(const Reader& memory,uint64_t base) {
    using quest_data::follow;
    using quest_data::read;
    const auto scene_slot=[&] { return follow(memory,base,{0x618e708,0x58,0xb8,0}); };
    const auto save_slot=[&] { return follow(memory,base,{0x639a810,0,0xb8,0,0x10}); };
    const auto scene=scene_slot();
    int32_t kind=0;
    if(!read(memory,scene,0x44,kind) || kind!=3) return {};
    const auto save=save_slot();
    if(!save) return {};
    TownSnapshot result;
    result.source=save;
    result.party=read_active_party(memory,save,&result.formation,true);
    if(const auto items=quest_data::bag(memory,save)) {
        result.bag_free=std::ranges::count(*items,int16_t(0));
        result.threads=std::ranges::count(*items,ariadne_thread);
    }
    result.quests=read_town_quest_board(memory,base,save);
    if(scene_slot()!=scene || !read(memory,scene,0x44,kind) || kind!=3 || save_slot()!=save) return {};
    return result;
}
} // namespace etrian
