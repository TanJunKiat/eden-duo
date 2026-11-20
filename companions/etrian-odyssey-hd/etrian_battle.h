#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <cmath>

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

template<class Reader> BattleMode read_battle(const Reader& memory, uint64_t base, BattleSnapshot& out) {
    out = {};
    auto get=[&](uint64_t a, auto& v) { return a && memory(a,&v,sizeof(v)); };
    auto add=[](uint64_t a,uint64_t b) { return a && a<=UINT64_MAX-b ? a+b : uint64_t(0); };
    auto pointer=[&](uint64_t a,uint64_t& p) { return get(a,p) && p; };
    auto singleton=[&](uint64_t slot,uint64_t& p) {
        uint64_t x=0;
        return pointer(add(base,slot),x) && pointer(add(x,0x58),x) && pointer(add(x,0xb8),x) && pointer(x,p);
    };
    uint64_t scene=0;
    int32_t kind=-1;
    if (!singleton(0x5392908,scene) || !get(add(scene,0x1c),kind)) return BattleMode::unknown;
    if (kind!=2) return BattleMode::map;
    uint64_t manager=0, ctrl=0, sides=0;
    int32_t n=0;
    if (!singleton(0x538f060,manager) || !pointer(add(manager,0x88),ctrl) || !pointer(add(ctrl,0x30),sides) ||
        !get(add(sides,0x18),n) || n!=2) return BattleMode::loading;
    BattleSnapshot next;
    for (int side=0;side<2;++side) {
        uint64_t array=0;
        if (!pointer(add(sides,0x20+side*8),array) || !get(add(array,0x18),n) || n<1 || n>8) return BattleMode::loading;
        auto& list=side ? next.enemies : next.allies;
        for (int i=0;i<n;++i) {
            uint64_t character=0;
            if (!get(add(array,0x20+i*8),character)) return BattleMode::loading;
            if (!character) continue;
            uint64_t name=0, param=0, values=0, value=0;
            int32_t len=0, count=0;
            Combatant c;
            if (!pointer(add(character,0x28),name) || !get(add(name,0x10),len) || len<1 || len>40 ||
                !pointer(add(character,0x60),param) || !pointer(add(param,0x10),values) ||
                !get(add(values,0x18),count) || count<2 || count>64) return BattleMode::loading;
            for (int j=0;j<len;++j) {
                uint16_t ch=0;
                if (!get(add(name,0x14+j*2),ch)) return BattleMode::loading;
                c.name+=ch>=32 && ch<127 ? char(ch) : '?';
            }
            auto pair=[&](int i,int32_t& now,int32_t& max) {
                return pointer(add(values,0x20+i*8),value) && get(add(value,0x10),now) && get(add(value,0x14),max) &&
                    now>=0 && max>0 && now<=max && max<=999999;
            };
            if (!pair(0,c.hp,c.max_hp)) return BattleMode::loading;
            if (!side && !pair(1,c.tp,c.max_tp)) { c.tp=-1; c.max_tp=-1; }
            // Only War Cry's slot and countdown have been observed live.
            uint64_t effects=0, effect=0;
            int32_t slots=0;
            if (pointer(add(param,0x28),effects) && get(add(effects,0x18),slots) && slots>=0 && slots<=3) {
                for (int j=0;j<slots;++j) {
                    int32_t skill=0, type=0; float remaining=0;
                    if (pointer(add(effects,0x20+j*8),effect) && get(add(effect,0x10),skill) &&
                        get(add(effect,0x18),type) && get(add(effect,0x1c),remaining) &&
                        skill==9 && type==14 && std::isfinite(remaining) && remaining>=1 && remaining<=4 && std::floor(remaining)==remaining)
                        c.war_cry=int(remaining);
                }
            }
            list.push_back(c);
        }
    }
    if (next.allies.empty() || next.enemies.empty()) return BattleMode::loading;
    out=std::move(next);
    return BattleMode::battle;
}
}
