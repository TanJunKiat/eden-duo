#include "etrian_battle.h"
#include "etrian_exploration.h"
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
    auto put_string=[&](uint64_t a,std::u16string_view s) {
        m.put(a+0x10,int32_t(s.size()));
        for(size_t i=0;i<s.size();++i) m.put(a+0x14+i*2,uint16_t(s[i]));
    };
    // A stable unit ID chooses the label. A same-hash entry with the wrong key precedes it.
    m.put(0x410026,int16_t(37));
    ptr(base+0x61a6208,0x600000); ptr(0x600018,0x610000); ptr(0x6100c0,0x620000);
    ptr(0x620000,0x630000); ptr(0x6300b8,0x640000); ptr(0x640000,0x650000);
    ptr(0x650020,0x660000); ptr(0x650048,0x670000); m.put(0x670010,int32_t(0));
    ptr(0x660010,0x680000); ptr(0x660018,0x690000); m.put(0x660020,int32_t(2)); m.put(0x660024,int32_t(1));
    m.put(0x680018,uint64_t(5)); m.put(0x690018,uint64_t(5));
    m.put(0x680030,int32_t(1)); // hash 0x636c154c maps to bucket 4, zero-based index 1
    m.put(0x690090,uint32_t(0x636c154c)); m.put(0x690094,int32_t(0)); ptr(0x690098,0x700000);
    m.put(0x690020,uint32_t(0x636c154c)); m.put(0x690024,int32_t(-1)); ptr(0x690028,0x710000);
    put_string(0x700000,u"enemyname_73"); put_string(0x710000,u"enemyname_37");
    ptr(0x690068,0x720000); ptr(0x690078,0x730000);
    put_string(0x720000,u"Moon Beetle"); put_string(0x730000,u"Scarabe\u00e9");
    const auto named=[&](std::string_view expected) {
        assert(etrian::read_battle(m,base,b)==etrian::BattleMode::battle && b.enemies[0].name==expected);
    };
    named("Moon Beetle");
    m.put(0x670010,int32_t(2)); named("Scarabe\xe9");
    m.put(0x670010,int32_t(5)); named("ENEMY 1"); // no silent language substitution
    m.put(0x670010,int32_t(0)); m.put(0x690094,int32_t(1)); named("ENEMY 1"); // cyclic chain
    m.put(0x690094,int32_t(0)); named("Moon Beetle");
    // The guide walks standard formations, not gated alternates or zero-weight groups.
    etrian::FloorReference ref;
    ref.ready=true;
    ref.screen[0].chip=1; ref.screen[0].encounter=12;
    ref.screen[1]=ref.screen[0];
    uint64_t obj=base;
    for(uint64_t offset:{0x61a60f0,0x18,0xc0,0,0xb8,0}) {
        const uint64_t next=obj+0xb00000;
        ptr(obj+offset,next); obj=next;
    }
    ptr(obj+0xa8,0xb80000); ptr(obj+0xb0,0xb90000);
    ptr(0xb80010,0xba0000); ptr(0xb90010,0xbb0000);
    m.put(0xba0018,uint64_t(13)); m.put(0xbb0018,uint64_t(2));
    ptr(0xba0020+12*0x58,0xbc0000); m.put(0xbc0018,uint64_t(3));
    m.put(0xbc0020,uint16_t(0x20)); m.put(0xbc0040,uint16_t(0x10));
    ptr(0xbc0048,0xbd0000); m.put(0xbd0018,uint64_t(5));
    for(int i=0;i<5;++i) m.put(0xbd0020+i*4,std::array<uint16_t,2>{uint16_t(i>0),uint16_t(i?0:100)});
    ptr(0xbb0028,0xbe0000); m.put(0xbe0018,uint64_t(8));
    m.put(0xbe0020,std::array<uint8_t,8>{37,37,0,0,0,0,0,0});
    const auto guide=etrian::read_floor_monsters(m,base,ref);
    assert(guide.size()==1 && guide.front().id==37 && guide.front().name=="Moon Beetle");
    m.put(0xbd0022,uint16_t(99)); assert(etrian::read_floor_monsters(m,base,ref).empty());
    ptr(obj+0x20,0xbf0000); ptr(0xbf0010,0xc20000); m.put(0xc20018,uint64_t(38));
    ptr(0xc20078+37*0x68,0xc10000); m.put(0xc10018,uint64_t(3));
    for(unsigned i=0;i<3;++i) m.put(0xc10020+6*i,std::array<uint16_t,3>{0,0,0});
    const auto empty_drops=etrian::read_monster_drops(m,base,37);
    assert(empty_drops && empty_drops->empty());
    const std::string item_key="useitemname_1";
    std::array<uint32_t,2> hash{5381,5381};
    for(size_t i=0;i<item_key.size();++i) hash[i%2]=(hash[i%2]*33)^uint8_t(item_key[i]);
    const uint32_t key_hash=(hash[0]+hash[1]*0x5d588b65u)&0x7fffffffu;
    m.put(0x680020+4*(key_hash%5),int32_t(0));
    m.put(0x690020,key_hash); put_string(0x710000,u"useitemname_1");
    put_string(0x720000,u"Test Scale");
    m.put(0xc10020,std::array<uint16_t,3>{925,42,0});
    m.put(0xc10026,std::array<uint16_t,3>{925,90,0x200});
    const auto drops=etrian::read_monster_drops(m,base,37);
    assert(drops && drops->size()==2 && (*drops)[0].name=="Test Scale" && (*drops)[0].base_chance==42);
    assert(!(*drops)[0].conditional && (*drops)[1].conditional && !(*drops)[1].base_chance);
    m.put(0xc10022,uint16_t(101)); assert(!etrian::read_monster_drops(m,base,37));
    m.put(0xc10018,uint64_t(4)); assert(!etrian::read_monster_drops(m,base,37));
    m.put(0x390018,int32_t(99)); assert(etrian::read_battle(m,base,b)==etrian::BattleMode::loading && b.allies.empty());

    // The live town array is distinct from the battle roster and guild storage.
    m.put(0x230044,int32_t(3));
    ptr(base+0x639a810,0x500000); ptr(0x500000,0x510000); ptr(0x5100b8,0x520000);
    ptr(0x520000,0x530000); ptr(0x530010,0x540000); ptr(0x540020,0x550000);
    ptr(0x550028,0x560000); m.put(0x560018,int32_t(3));
    ptr(0x560020,0x570000); ptr(0x560028,0); ptr(0x560030,0x580000);
    const auto member=[&](uint64_t data,char letter,int16_t hp,int16_t tp,uint8_t order) {
        const auto name=data+0x1000;
        m.put(data+0x50,hp); m.put(data+0x52,tp); m.put(data+0x7a,order); ptr(data+0x58,name);
        m.put(name+0x10,int32_t(1)); m.put(name+0x14,uint16_t(letter));
    };
    member(0x570000,'A',44,18,2); member(0x580000,'B',0,9,0);
    const auto town=etrian::read_battle(m,base,b);
    assert(b.allies.size()==2 && b.enemies.empty());
    assert(b.allies[0].name=="B" && b.allies[0].hp==0 && b.allies[0].tp==9);
    assert(b.allies[1].name=="A" && b.allies[1].hp==44 && b.allies[1].tp==18);
    assert(b.allies[1].max_hp==-1 && b.allies[1].max_tp==-1);
    m.put(0x570052,int16_t(5)); assert(etrian::read_battle(m,base,b)==town && b.allies[1].tp==5);
    m.put(0x560018,int32_t(0)); assert(etrian::read_battle(m,base,b)==town && b.allies.empty());
    m.put(0x560018,int32_t(6));
    const auto unreadable=etrian::read_battle(m,base,b);
    assert(unreadable!=town && unreadable!=etrian::BattleMode::unknown && b.allies.empty());
    m.put(0x560018,int32_t(3)); m.bytes.erase(0x580050);
    assert(etrian::read_battle(m,base,b)==unreadable && b.allies.empty());
    for (int32_t scene : {2,6,7}) {
        m.put(0x230044,scene);
        assert(etrian::read_battle(m,base,b)==etrian::BattleMode::unknown && b.allies.empty());
    }
}
