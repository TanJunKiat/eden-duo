#pragma once
#include "etrian_enemy_names.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>
#include <utility>

namespace etrian {
enum class BattleMode { unknown, map, loading, battle, town, town_loading };
struct GuardOrderTimer {
    std::string name;
    int turns=0;
    bool operator==(const GuardOrderTimer&) const = default;
};
struct Combatant {
    std::string name;
    int32_t hp=-1, max_hp=-1, tp=-1, max_tp=-1;
    int war_cry=-1;
    std::optional<GuardOrderTimer> guard_order;
    bool operator==(const Combatant&) const = default;
};
struct BattleSnapshot {
    std::vector<Combatant> allies, enemies;
    bool operator==(const BattleSnapshot&) const = default;
};

inline uint64_t add_field(uint64_t address,uint64_t offset) {
    return address && address<=UINT64_MAX-offset ? address+offset : 0;
}

template<class Reader> bool read_character_name(const Reader& memory,uint64_t data,std::string& result) {
    uint64_t name=0;
    int32_t len=0;
    const auto get=[&](uint64_t object,uint64_t offset,auto& value) {
        const auto addr=add_field(object,offset);
        return addr && memory(addr,&value,sizeof(value));
    };
    if(!get(data,0x58,name) || !get(name,0x10,len) || len<=0 || len>40) return true;
    for(int j=0;j<len;++j) {
        uint16_t ch=0;
        if(!get(name,0x14+j*2,ch)) return false;
        result+=ch>=32 && ch<127?char(ch):'?';
    }
    return true;
}

template<class Reader> std::optional<std::vector<Combatant>> read_active_party(
    const Reader& memory,uint64_t save,std::optional<std::vector<uint8_t>>* formation=nullptr,
    bool town_maxima=false) {
    if(formation) formation->reset();
    const auto get=[&](uint64_t object,uint64_t offset,auto& value) {
        const auto addr=add_field(object,offset);
        return addr && memory(addr,&value,sizeof(value));
    };
    // PARTY_DATA +0x20 is the registered roster; only +0x28 is active.
    uint64_t party=0,array=0;
    int32_t count=0;
    std::array<std::pair<uint8_t,Combatant>,5> members;
    if(!get(save,0x20,party) || !get(party,0x28,array) || !get(array,0x18,count) ||
       count<0 || count>int32_t(members.size())) return {};
    size_t occupied=0;
    unsigned taken=0;
    bool valid_formation=true;
    for(int i=0;i<count;++i) {
        uint64_t data=0;
        if(!get(array,0x20+i*8,data)) return {};
        if(!data) continue;
        auto& [order,member]=members[occupied++];
        int16_t hp=0,tp=0;
        if(!get(data,0x7a,order)) {
            if(!formation) return {};
            order=255;
        }
        // Native GetCharLine: values above 2 are Back; the order domain has six positions.
        if(order>=6 || (taken & (1u<<order))) valid_formation=false;
        else taken|=1u<<order;
        if(!get(data,0x50,hp) || !get(data,0x52,tp) ||
           hp<0 || tp<0 || !read_character_name(memory,data,member.name) || member.name.empty()) return {};
        member.hp=hp;
        member.tp=tp;
        if(town_maxima) {
            // Town.SetStatus.SetAbilData displays fortify_ability, not battle_ability.
            uint64_t fortify=0;
            if(get(data,0x40,fortify)) {
                int32_t maximum=0;
                if(get(fortify,0x10,maximum) && maximum>0 && maximum>=hp && maximum<=INT16_MAX)
                    member.max_hp=maximum;
                if(get(fortify,0x14,maximum) && maximum>0 && maximum>=tp && maximum<=INT16_MAX)
                    member.max_tp=maximum;
            }
        }
    }
    if(!formation || valid_formation)
        std::stable_sort(members.begin(),members.begin()+occupied,[](const auto& a,const auto& b) {return a.first<b.first;});
    std::vector<Combatant> result;
    result.reserve(occupied);
    if(formation && valid_formation) formation->emplace();
    for(size_t i=0;i<occupied;++i) {
        if(formation && *formation) (*formation)->push_back(members[i].first);
        result.push_back(std::move(members[i].second));
    }
    return result;
}

template<class Reader> std::optional<GuardOrderTimer> read_guard_order(const Reader& memory,uint64_t unit,
                                                                     const EnemyLabels& labels) {
    // ReinforceInfo shows three buff slots. Only the observed player-origin skill
    // is enabled in this trial. No inference from category or target's side.
    constexpr uint16_t guard_order_id=21;
    const auto get=[&](uint64_t object,uint64_t offset,auto& value) {
        const auto address=add_field(object,offset);
        return address && memory(address,&value,sizeof(value));
    };
    uint64_t slots=0,count=0;
    std::array<uint64_t,3> entries{};
    if(!get(unit,0x2d8,slots) || !get(slots,0x18,count) || count!=entries.size() || !get(slots,0x20,entries)) return {};
    int turns=0;
    for(const auto entry:entries) {
        std::array<uint8_t,8> bytes{};
        if(!get(entry,0x10,bytes)) return {};
        if(!bytes[1] || bytes[2] || (unsigned(bytes[6])|(unsigned(bytes[7])<<8))!=guard_order_id) continue;
        if(turns || bytes[0]==0 || bytes[0]>127) return {};
        turns=bytes[0];
    }
    if(!turns) return {};
    auto name=labels.player_skill(memory,guard_order_id);
    if(name.empty()) return {};
    return GuardOrderTimer{std::move(name),turns};
}

template<class Reader> BattleMode read_battle(const Reader& memory,uint64_t base,BattleSnapshot& out) {
    out={};
    auto get=[&](uint64_t a,auto& v) { return a && memory(a,&v,sizeof(v)); };
    const auto add=add_field;
    auto pointer=[&](uint64_t a,uint64_t& p) { return get(a,p) && p; };
    auto singleton=[&](uint64_t slot,uint64_t& p) {
        uint64_t x=0;
        return pointer(add(base,slot),x) && pointer(add(x,0x58),x) && pointer(add(x,0xb8),x) && pointer(x,p);
    };
    uint64_t scene=0; int32_t kind=-1;
    if(!singleton(0x618e708,scene) || !get(add(scene,0x44),kind)) return BattleMode::unknown;
    if(kind==3) {
        uint64_t save=0;
        if(!pointer(add(base,0x639a810),save) || !pointer(save,save) ||
           !pointer(add(save,0xb8),save) || !pointer(save,save) || !pointer(add(save,0x10),save))
            return BattleMode::town_loading;
        auto party=read_active_party(memory,save);
        if(!party) return BattleMode::town_loading;
        if(!get(add(scene,0x44),kind) || kind!=3) return BattleMode::unknown;
        out.allies=std::move(*party);
        return BattleMode::town;
    }
    if(kind==4) return BattleMode::map;
    if(kind!=5) return BattleMode::unknown; // Sea and other scenes are not dungeon maps.
    uint64_t manager=0,packet=0,sides=0; int32_t n=0;
    if(!singleton(0x618ace8,manager) || !pointer(add(manager,0xb8),packet) ||
       !pointer(add(packet,0x88),sides) || !get(add(sides,0x18),n) || n!=2) return BattleMode::loading;
    // Native BattleManager.Phase: opening Skill moves CommandSelect -> ListSelect;
    // choosing a skill may move to TargetSelect. No turn advances in these menus.
    // The action-phase counter can briefly include a turn that has not ended yet.
    const auto choosing_command=[](int32_t phase) {
        constexpr int32_t command_select=5,target_select=14,list_select=15;
        return phase==command_select || phase==target_select || phase==list_select;
    };
    int32_t phase=-1;
    const bool read_timers=get(add(manager,0xb4),phase) && choosing_command(phase);
    BattleSnapshot next;
    for(int side=0;side<2;++side) {
        uint64_t party=0,array=0;
        if(!pointer(add(sides,0x20+side*8),party) || !pointer(add(party,0x18),array) ||
           !get(add(array,0x18),n) || n<(side?0:1) || n>8) return BattleMode::loading;
        auto& list=side?next.enemies:next.allies;
        const EnemyLabels labels(memory,(side || read_timers)?base:0);
        for(int i=0;i<n;++i) {
            uint64_t unit=0,data=0,ability=0;
            if(!get(add(array,0x20+i*8),unit)) return BattleMode::loading;
            if(!unit) continue;
            if(side) {
                // BattleUnitCheckAlive: an occupied slot is not necessarily a living enemy.
                // Read only word zero of the native status/flag arrays, as the game does.
                const auto first_flags=[&](uint64_t field,uint32_t& bits) {
                    uint64_t flags=0,count=0;
                    return pointer(add(unit,field),flags) && get(add(flags,0x18),count) && count &&
                           get(add(flags,0x20),bits);
                };
                uint32_t flags=0;
                if(!first_flags(0x18,flags)) return BattleMode::loading;
                if(!(flags & 1u)) continue; // BTLUNIT_STATUSFLAG0_EXIST
                if(!first_flags(0x10,flags)) return BattleMode::loading;
                if(flags & 0x02000000u) continue; // BTLUNIT_FLAG_IMTBYDEATH
            }
            if(!pointer(add(unit,0x340),data)) return BattleMode::loading;
            if(side) {
                uint16_t status=0;
                if(!get(add(data,0x60),status)) return BattleMode::loading;
                if(status & 1u) continue; // CHAR_DATA.BadStatusFg: KO
            }
            int16_t hp=0,tp=0; Combatant c;
            if(!pointer(add(data,0x48),ability) ||
               !get(add(data,0x50),hp) || !get(add(ability,0x10),c.max_hp) ||
               hp<0 || c.max_hp<=0 || hp>c.max_hp || c.max_hp>999999) return BattleMode::loading;
            c.hp=hp;
            if(!side && get(add(data,0x52),tp) && get(add(ability,0x14),c.max_tp) &&
               tp>=0 && c.max_tp>0 && tp<=c.max_tp && c.max_tp<=999999) c.tp=tp;
            else c.max_tp=-1;
            int16_t enemy_id=-1;
            if (side && get(add(unit,0x26),enemy_id)) c.name=labels.lookup(memory,enemy_id);
            if (c.name.empty() && !read_character_name(memory,data,c.name)) return BattleMode::loading;
            if(c.name.empty()) {
                if(!side) return BattleMode::loading;
                c.name="ENEMY "+std::to_string(i+1);
            }
            if(!side && c.hp>0 && read_timers) c.guard_order=read_guard_order(memory,unit,labels);
            list.push_back(std::move(c));
        }
    }
    if(next.allies.empty()) return BattleMode::loading;
    uint64_t after=0;
    if(read_timers && (!get(add(scene,0x44),kind) || kind!=5 || !get(add(manager,0xb4),phase) ||
                       !choosing_command(phase) || !pointer(add(manager,0xb8),after) || after!=packet))
        for(auto& ally:next.allies) ally.guard_order.reset();
    out=std::move(next); return BattleMode::battle;
}
}
