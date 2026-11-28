// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "etrian_battle.h"
#include "etrian_quests.h"

namespace etrian {
using ExpeditionParty=std::optional<std::vector<Combatant>>;
// MasterItemNoEnum.ITEM_CONSUME_ARIADONE in the supported HD build.
inline constexpr int16_t ariadne_thread=1355;
struct ExpeditionSnapshot {
    uint64_t source=0; // Identity only; never rendered.
    std::optional<unsigned> threads,bag_free;
    ExpeditionParty party;
    bool operator==(const ExpeditionSnapshot&) const = default;
};

template<class Reader> ExpeditionSnapshot read_expedition(const Reader& memory,uint64_t base) {
    using quest_data::follow;
    const auto save_slot=[&] { return follow(memory,base,{0x639a810,0,0xb8,0,0x10}); };
    const auto save=save_slot();
    if(!save) return {};
    ExpeditionSnapshot snapshot;
    snapshot.source=save;
    if(const auto items=quest_data::bag(memory,save)) {
        snapshot.threads=std::ranges::count(*items,ariadne_thread);
        snapshot.bag_free=std::ranges::count(*items,int16_t(0));
    }
    snapshot.party=read_active_party(memory,save);
    return save_slot()==save?std::move(snapshot):ExpeditionSnapshot{};
}
}
