#include "etrian_battle.h"
#include <cassert>
#include <map>
#include <cstring>
struct Memory {
    std::map<uint64_t, uint8_t> bytes;
    template<class T> void put(uint64_t a, T value) {
        const auto* p = reinterpret_cast<const uint8_t*>(&value);
        for (size_t i = 0; i < sizeof value; ++i) bytes[a+i] = p[i];
    }
    bool operator()(uint64_t a, void* p, size_t n) const {
        auto* out = static_cast<uint8_t*>(p);
        for (size_t i=0;i<n;++i) { auto it=bytes.find(a+i); if(it==bytes.end()) return false; out[i]=it->second; }
        return true;
    }
};
int main() {
    Memory m;
    auto ptr=[&](uint64_t a,uint64_t b){m.put(a,b);};
    const uint64_t base=0x100000;
    ptr(base+0x5392908,0x200000); ptr(0x200058,0x210000); ptr(0x2100b8,0x220000); ptr(0x220000,0x230000);
    m.put(0x23001c,int32_t(4));
    etrian::BattleSnapshot b;
    assert(etrian::read_battle(m,base,b)==etrian::BattleMode::map);
    m.put(0x23001c,int32_t(2));
    assert(etrian::read_battle(m,base,b)==etrian::BattleMode::loading);
    ptr(base+0x538f060,0x300000); ptr(0x300058,0x310000); ptr(0x3100b8,0x320000); ptr(0x320000,0x330000); ptr(0x330088,0x340000); ptr(0x340030,0x350000);
    m.put(0x350018,int32_t(2)); ptr(0x350020,0x360000); ptr(0x350028,0x370000);
    m.put(0x360018,int32_t(1)); m.put(0x370018,int32_t(1));
    ptr(0x360020,0x380000); ptr(0x370020,0x390000);
    for (auto c : {uint64_t(0x380000),uint64_t(0x390000)}) {
        auto param=c+0x1000, arr=c+0x2000, hp=c+0x3000, tp=c+0x4000, name=c+0x5000;
        ptr(c+0x28,name); m.put(name+0x10,int32_t(3)); m.put(name+0x14,uint16_t('R'));m.put(name+0x16,uint16_t('A'));m.put(name+0x18,uint16_t('T'));
        ptr(c+0x60,param); ptr(param+0x10,arr); m.put(arr+0x18,int32_t(2)); ptr(arr+0x20,hp); ptr(arr+0x28,tp);
        m.put(hp+0x10,int32_t(12));m.put(hp+0x14,int32_t(19));m.put(tp+0x10,int32_t(7));m.put(tp+0x14,int32_t(7));
    }
    assert(etrian::read_battle(m,base,b)==etrian::BattleMode::battle);
    assert(b.allies.size()==1 && b.enemies.size()==1 && b.enemies[0].hp==12 && b.enemies[0].max_hp==19);
    assert(b.allies[0].name=="RAT" && b.allies[0].tp==7);
    // Preserve occupied array order, even with a gap and repeated combatants.
    m.put(0x395014,uint16_t('C')); // second record is CAT
    m.put(0x360018,int32_t(4)); m.put(0x370018,int32_t(3));
    ptr(0x360020,0x390000); ptr(0x360028,0); ptr(0x360030,0x380000); ptr(0x360038,0x390000);
    ptr(0x370020,0x380000); ptr(0x370028,0); ptr(0x370030,0x390000);
    assert(etrian::read_battle(m,base,b)==etrian::BattleMode::battle);
    assert(b.allies.size()==3 && b.allies[0].name=="CAT" && b.allies[1].name=="RAT" && b.allies[2].name=="CAT");
    assert(b.enemies.size()==2 && b.enemies[0].name=="RAT" && b.enemies[1].name=="CAT");
    m.put(0x360018,int32_t(1)); m.put(0x370018,int32_t(1));
    ptr(0x360020,0x380000); ptr(0x370020,0x390000);
    ptr(0x381028,0x3a0000); m.put(0x3a0018,int32_t(1)); ptr(0x3a0020,0x3b0000);
    m.put(0x3b0010,int32_t(9)); m.put(0x3b0018,int32_t(14)); m.put(0x3b001c,float(3));
    assert(etrian::read_battle(m,base,b)==etrian::BattleMode::battle && b.allies[0].war_cry==3);
    m.put(0x3b001c,float(0));
    assert(etrian::read_battle(m,base,b)==etrian::BattleMode::battle && b.allies[0].war_cry==-1);
    m.put(0x370018,int32_t(10000));
    assert(etrian::read_battle(m,base,b)==etrian::BattleMode::loading && b.enemies.empty());
    m.put(0x23001c,int32_t(4));
    assert(etrian::read_battle(m,base,b)==etrian::BattleMode::map && b.allies.empty());
}
