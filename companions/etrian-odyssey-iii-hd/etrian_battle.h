#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <utility>

namespace etrian {
enum class BattleMode { unknown, map, loading, battle };
struct Combatant {
    std::string name;
    int32_t hp=-1, max_hp=-1, tp=-1, max_tp=-1;
    int war_cry=-1;
    bool operator==(const Combatant&) const = default;
};
struct BattleSnapshot {
    std::vector<Combatant> allies, enemies;
    bool operator==(const BattleSnapshot&) const = default;
};

template<class Reader> BattleMode read_battle(const Reader& memory,uint64_t base,BattleSnapshot& out) {
    out={};
    auto get=[&](uint64_t a,auto& v) { return a && memory(a,&v,sizeof(v)); };
    auto add=[](uint64_t a,uint64_t b) { return a && a<=UINT64_MAX-b ? a+b : uint64_t(0); };
    auto pointer=[&](uint64_t a,uint64_t& p) { return get(a,p) && p; };
    auto singleton=[&](uint64_t slot,uint64_t& p) {
        uint64_t x=0;
        return pointer(add(base,slot),x) && pointer(add(x,0x58),x) && pointer(add(x,0xb8),x) && pointer(x,p);
    };
    uint64_t scene=0; int32_t kind=-1;
    if(!singleton(0x618e708,scene) || !get(add(scene,0x44),kind)) return BattleMode::unknown;
    if(kind==4) return BattleMode::map;
    if(kind!=5) return BattleMode::unknown; // Sea and other scenes are not dungeon maps.
    uint64_t manager=0,packet=0,sides=0; int32_t n=0;
    if(!singleton(0x618ace8,manager) || !pointer(add(manager,0xb8),packet) ||
       !pointer(add(packet,0x88),sides) || !get(add(sides,0x18),n) || n!=2) return BattleMode::loading;
    BattleSnapshot next;
    for(int side=0;side<2;++side) {
        uint64_t party=0,array=0;
        if(!pointer(add(sides,0x20+side*8),party) || !pointer(add(party,0x18),array) ||
           !get(add(array,0x18),n) || n<1 || n>8) return BattleMode::loading;
        auto& list=side?next.enemies:next.allies;
        for(int i=0;i<n;++i) {
            uint64_t unit=0,data=0,ability=0,name=0;
            if(!get(add(array,0x20+i*8),unit)) return BattleMode::loading;
            if(!unit) continue;
            int16_t hp=0,tp=0; int32_t len=0; Combatant c;
            if(!pointer(add(unit,0x340),data) || !pointer(add(data,0x48),ability) ||
               !get(add(data,0x50),hp) || !get(add(ability,0x10),c.max_hp) ||
               hp<0 || c.max_hp<=0 || hp>c.max_hp || c.max_hp>999999) return BattleMode::loading;
            c.hp=hp;
            if(!side && get(add(data,0x52),tp) && get(add(ability,0x14),c.max_tp) &&
               tp>=0 && c.max_tp>0 && tp<=c.max_tp && c.max_tp<=999999) c.tp=tp;
            else c.max_tp=-1;
            if(pointer(add(data,0x58),name) && get(add(name,0x10),len) && len>0 && len<=40) {
                for(int j=0;j<len;++j) {
                    uint16_t ch=0; if(!get(add(name,0x14+j*2),ch)) return BattleMode::loading;
                    c.name+=ch>=32 && ch<127?char(ch):'?';
                }
            }
            if(c.name.empty()) {
                if(!side) return BattleMode::loading;
                c.name="ENEMY "+std::to_string(i+1);
            }
            list.push_back(std::move(c));
        }
    }
    if(next.allies.empty() || next.enemies.empty()) return BattleMode::loading;
    out=std::move(next); return BattleMode::battle;
}
}
