// SPDX-License-Identifier: GPL-3.0-or-later
#undef NDEBUG
#include "etrian_bistro_bounty.h"
#include <array>
#include <cassert>
#include <vector>

int main() {
    using namespace etrian::bistro_bounty;
    constexpr std::array<uint16_t,4> three{504,256,504,504};
    constexpr std::array<uint16_t,2> two{504,504};
    constexpr std::array<uint16_t,2> others{248,256};
    constexpr std::array<uint16_t,2> unseen{999,998};
    const std::array<FloorCandidates,3> floors{{{1,three},{2,unseen},{3,others}}};
    const auto pool=eligible_species(floors,0b101);
    assert((pool==std::vector<uint16_t>{248,256,504}));
    assert(eligible_species(floors,0).empty());
    const std::array<FloorCandidates,2> invalid{{{0,unseen},{26,unseen}}};
    assert(eligible_species(invalid,0xFFFFFFFFu).empty());

    constexpr uint64_t contract_id=90210;
    constexpr uint32_t epoch=4;
    constexpr uint64_t save=77;
    ExploredFloors explored;
    explored.refresh(save,epoch);
    assert(explored.pool(floors).empty());
    assert(explored.visit(save,epoch,1) && explored.visit(save,epoch,3));
    assert(!explored.visit(save,epoch,26) && !explored.visit(save+1,epoch,2));
    assert(!explored.visit(save,epoch-1,2));
    assert(explored.pool(floors)==pool);
    explored.refresh(save,epoch);
    assert(explored.pool(floors)==pool);
    explored.refresh(save,epoch+1);
    assert(explored.pool(floors).empty() && !explored.visit(save,epoch,1));
    assert(explored.visit(save,epoch+1,1));
    explored.refresh(save+1,epoch+1);
    assert(explored.pool(floors).empty());
    explored.refresh(0,epoch+1);
    assert(!explored.visit(0,epoch+1,1));
    Contract c{contract_id,epoch,504};
    assert(!c.claimable(contract_id,epoch));
    assert(!c.record_victory(contract_id,epoch-1,1,three));
    assert(!c.record_victory(contract_id-1,epoch,1,three));
    assert(!c.record_victory(contract_id,epoch,0,three));
    assert(c.record_victory(contract_id,epoch,1,three));
    assert(c.progress()==3);
    assert(!c.record_victory(contract_id,epoch,1,three));
    assert(c.record_victory(contract_id,epoch,2,others));
    assert(c.progress()==3);
    assert(c.record_victory(contract_id,epoch,4,two));
    assert(c.progress()==5);
    assert(!c.claimable(contract_id,epoch+1));
    assert(!c.claimable(contract_id-1,epoch));
    assert(c.claimable(contract_id,epoch));
    assert(c.mark_claimed(contract_id,epoch));
    assert(!c.mark_claimed(contract_id,epoch));
    assert(!c.record_victory(contract_id,epoch,5,three));

    Contract unavailable{0,epoch,504};
    assert(!unavailable.record_victory(0,epoch,1,three));
    Contract saturated{contract_id,epoch,504};
    assert(saturated.record_victory(contract_id,epoch,1,three));
    assert(saturated.record_victory(contract_id,epoch,2,three));
    assert(saturated.progress()==5);

    assert(regular_reward(100,451)==35);
    assert(regular_reward(0,120)==12);
    assert(regular_reward(90,99)==0);
    assert(regular_reward(120,120)==0);
    assert(regular_reward(120,0)==0);
    assert(regular_reward(-1,120)==0);

    // Thresholds are native values indexed by each active member's own level.
    const std::array<RewardMember,3> roster{{{2,70,120,350},{3,70,350,870},{70,70,700000,-1}}};
    const auto awards=plan_regular_awards(roster);
    assert((awards && *awards==std::array<int32_t,5>{23,52,0,0,0}));
    const std::array<RewardMember,2> unavailable_threshold{{roster[0],{3,70,350,-1}}};
    assert(!plan_regular_awards(unavailable_threshold)); // no partial payout
    const std::array<RewardMember,1> capped{{roster[2]}};
    assert(plan_regular_awards(capped) && (*plan_regular_awards(capped))[0]==0);
    assert(!plan_regular_awards({}));

    constexpr uint64_t active=(uint64_t(3)<<32)|24;
    constexpr uint64_t complete=active|(5u<<16);
    assert(valid_word(active,3) && !ready_to_request(active));
    assert(valid_word(complete,3) && ready_to_request(complete));
    assert(valid_word(complete|report_requested,3) && !ready_to_request(complete|report_requested));
    assert(valid_word(complete|reported,3) && !ready_to_request(complete|reported));
    assert(!valid_word(active,4) && !valid_word(active|report_requested,3));
    assert(!valid_word(active|(6u<<16),3));
    assert(!valid_word(complete|reported|report_requested,3));
    assert(valid_word(complete|paused,3) && !ready_to_request(complete|paused));
    assert(!valid_word(complete|paused|report_requested,3));

    Offers board{};
    const auto first=[](size_t) {return size_t(0);};
    fill_offers(board,{24,24,90,0},first);
    assert(board[0].species==24 && board[1].species==90 && !board[2].species);
    board[0].completed=true;
    fill_offers(board,{4,4,24,72,90,100},first);
    assert(board[0].species==24 && board[0].completed && board[1].species==90);
    assert(board[2].species==4 && board[3].species==72);
    const auto unchanged=board;
    fill_offers(board,{101,103},first);
    assert(board==unchanged);
}
