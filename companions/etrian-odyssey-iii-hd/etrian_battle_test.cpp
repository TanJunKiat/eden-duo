#include "etrian_battle.h"
#include <cassert>
#include <map>
struct Memory {
    std::map<uint64_t,uint8_t> bytes;
    template<class T> void put(uint64_t a,T value) {
        auto p=reinterpret_cast<const uint8_t*>(&value);
        for(size_t i=0;i<sizeof(value);++i) bytes[a+i]=p[i];
    }
    bool operator()(uint64_t a,void* p,size_t n) const {
        auto out=static_cast<uint8_t*>(p);
        for(size_t i=0;i<n;++i) { auto it=bytes.find(a+i); if(it==bytes.end()) return false; out[i]=it->second; }
        return true;
    }
};
int main() {
    Memory m; auto ptr=[&](uint64_t a,uint64_t b){m.put(a,b);}; const uint64_t base=0x100000;
    ptr(base+0x618e708,0x200000); ptr(0x200058,0x210000); ptr(0x2100b8,0x220000); ptr(0x220000,0x230000);
    etrian::BattleSnapshot b;
    m.put(0x230044,int32_t(4)); assert(etrian::read_battle(m,base,b)==etrian::BattleMode::map);
    m.put(0x230044,int32_t(6)); assert(etrian::read_battle(m,base,b)==etrian::BattleMode::unknown);
    m.put(0x230044,int32_t(5)); assert(etrian::read_battle(m,base,b)==etrian::BattleMode::loading);
    ptr(base+0x618ace8,0x300000); ptr(0x300058,0x310000); ptr(0x3100b8,0x320000); ptr(0x320000,0x330000);
    ptr(0x3300b8,0x340000); ptr(0x340088,0x350000); m.put(0x350018,int32_t(2));
    ptr(0x350020,0x360000); ptr(0x350028,0x370000); ptr(0x360018,0x380000); ptr(0x370018,0x390000);
    m.put(0x380018,int32_t(1)); m.put(0x390018,int32_t(1)); ptr(0x380020,0x400000); ptr(0x390020,0x410000);
    for(auto u:{uint64_t(0x400000),uint64_t(0x410000)}) {
        auto data=u+0x1000, ability=u+0x2000, name=u+0x3000;
        ptr(u+0x340,data); ptr(data+0x48,ability); m.put(data+0x50,int16_t(44)); m.put(data+0x52,int16_t(18));
        m.put(ability+0x10,int32_t(44)); m.put(ability+0x14,int32_t(18)); ptr(data+0x58,name);
        m.put(name+0x10,int32_t(u==0x400000?4:0));
        if(u==0x400000) { m.put(name+0x14,uint16_t('E'));m.put(name+0x16,uint16_t('d'));m.put(name+0x18,uint16_t('e'));m.put(name+0x1a,uint16_t('n')); }
    }
    assert(etrian::read_battle(m,base,b)==etrian::BattleMode::battle);
    assert(b.allies.size()==1 && b.enemies.size()==1);
    assert(b.allies[0].name=="Eden" && b.allies[0].hp==44 && b.allies[0].tp==18);
    assert(b.enemies[0].name=="ENEMY 1" && b.enemies[0].war_cry==-1);
    m.put(0x390018,int32_t(99)); assert(etrian::read_battle(m,base,b)==etrian::BattleMode::loading && b.allies.empty());
}
