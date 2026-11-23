// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "etrian_enemy_names.h"
#include "etrian_map.h"
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace etrian {
struct ReferenceCell {
    uint8_t chip=0, parameter=0;
    uint16_t encounter=0;
    std::array<uint8_t,8> rest{};
    bool operator==(const ReferenceCell&) const = default;
};
static_assert(sizeof(ReferenceCell)==12);

struct FloorReference {
    std::array<ReferenceCell,cells> screen{};
    bool ready=false;
    bool operator==(const FloorReference&) const = default;
    // Special passages and doors do not become hint routes. No reference icons are read.
    bool open(int x,int y) const {
        if (!ready || x<0 || x>=width || y<0 || y>=height) return false;
        const auto chip=screen[index(x,y)].chip;
        return (chip>=1 && chip<=13) || chip==15 || (chip>=17 && chip<=19) || chip==29;
    }
};

// Only the observed B1F runtime layout is enabled. The loaded map's north-first
// rows already match screen coordinates, unlike the recorded exploration array.
template<class Reader>
FloorReference read_floor_reference(const Reader& memory,uint64_t base,int floor,int px,int py) {
    if (floor!=1) return {};
    const auto read=[&](uint64_t address,auto& value) {
        return address && memory(address,&value,sizeof(value));
    };
    uint64_t a=base,ctrl=0,list=0,data=0,capacity=0;
    for (uint64_t offset:{0x61a6040,0x18,0xc0,0,0xb8,0,0x78,0x10}) {
        if (a>UINT64_MAX-offset || !read(a+offset,a) || !a) return {};
    }
    ctrl=a;
    int32_t mode=-1,w=0,h=0,count=0;
    uint32_t map_id=0;
    if (ctrl>UINT64_MAX-0x148 || !read(ctrl+0x10,mode) || mode!=0 ||
        !read(ctrl+0x14,map_id) || map_id!=0x10010 ||
        !read(ctrl+0x18,w) || w!=width || !read(ctrl+0x1c,h) || h!=height ||
        !read(ctrl+0x140,list) || !list || list>UINT64_MAX-0x20 ||
        !read(list+0x18,count) || count!=cells || !read(list+0x10,data) || !data ||
        data>UINT64_MAX-0x20-sizeof(FloorReference::screen) ||
        !read(data+0x18,capacity) || capacity<cells || capacity>2*cells) return {};
    FloorReference ref;
    if (!memory(data+0x20,ref.screen.data(),sizeof(ref.screen))) return {};
    for (const auto& c:ref.screen) if (c.chip>30) return {};
    // B1F orientation anchors: the entrance and both stair cells on the far side.
    if (ref.screen[index(10,25)].chip!=18 || ref.screen[index(23,14)].chip!=18 ||
        ref.screen[index(33,19)].chip!=18) return {};
    uint32_t id_after=0;
    uint64_t list_after=0;
    if (!read(ctrl+0x14,id_after) || id_after!=map_id ||
        !read(ctrl+0x140,list_after) || list_after!=list) return {};
    ref.ready=true;
    // Require the live location to agree with the reference geometry. Walkable
    // doors are allowed for this check, but not shown as hints.
    if (px<0 || px>=width || py<0 || py>=height) return {};
    const auto player_chip=ref.screen[index(px,screen_y(py))].chip;
    if (!ref.open(px,screen_y(py)) && player_chip!=16 && player_chip!=21 && player_chip!=23) return {};
    return ref;
}
struct FloorMonster {
    uint16_t id=0;
    std::string name;
    bool operator==(const FloorMonster&) const = default;
};
struct MonsterDrop {
    std::string name;
    std::optional<unsigned> base_chance;
    bool conditional=false;
    bool operator==(const MonsterDrop&) const = default;
};
using DropList=std::optional<std::vector<MonsterDrop>>;

template<class Reader>
DropList read_monster_drops(const Reader& memory,uint64_t base,uint16_t id) {
    if(!id) return {};
    const auto read=[&](uint64_t address,uint64_t offset,auto& value) {
        return address && address<=UINT64_MAX-offset-sizeof(value) && memory(address+offset,&value,sizeof(value));
    };
    uint64_t master=base;
    for(uint64_t offset:{0x61a60f0,0x18,0xc0,0,0xb8,0})
        if(!read(master,offset,master) || !master) return {};
    uint64_t table=0,array=0,count=0,slots=0,size=0;
    if(!read(master,0x20,table) || !read(table,0x10,array) || !read(array,0x18,count) ||
       id>=count || count>4096 || !read(array,0x78+0x68*id,slots) || !read(slots,0x18,size) || size!=3) return {};
    std::vector<MonsterDrop> drops;
    EnemyLabels labels(memory,base);
    for(unsigned i=0;i<size;++i) {
        std::array<uint16_t,3> row{};
        if(!read(slots,0x20+6*i,row)) return {};
        const auto [item,threshold,condition]=row;
        if(!item) { if(threshold || condition) return {}; continue; }
        if(threshold>100) return {};
        auto name=labels.material(memory,item);
        if(name.empty()) return {};
        drops.push_back({std::move(name),condition?std::nullopt:std::optional<unsigned>(threshold),condition!=0});
    }
    return drops;
}

// Follow the verified ordinary HD formation subtables. Skip flag-/time-gated
// variants rather than presenting them as ordinary monsters or assigning odds.
template<class Reader>
std::vector<FloorMonster> read_floor_monsters(const Reader& memory,uint64_t base,const FloorReference& ref) {
    if (!ref.ready) return {};
    const auto read=[&](uint64_t address,uint64_t offset,auto& value) {
        return address && address<=UINT64_MAX-offset-sizeof(value) && memory(address+offset,&value,sizeof(value));
    };
    uint64_t master=base;
    for (uint64_t offset:{0x61a60f0,0x18,0xc0,0,0xb8,0})
        if (!read(master,offset,master) || !master) return {};
    uint64_t a=0,entries=0,groups=0,ne=0,ng=0;
    if (!read(master,0xa8,a) || !read(a,0x10,entries) || !read(entries,0x18,ne) || ne>4096 ||
        !read(master,0xb0,a) || !read(a,0x10,groups) || !read(groups,0x18,ng) || ng>4096) return {};
    std::array<bool,4096> seen_table{};
    std::array<bool,256> monsters{};
    for (const auto& cell:ref.screen) {
        const auto id=cell.encounter;
        if (!id || cell.chip==0 || cell.chip==14) continue;
        if (id>=ne) return {};
        if (seen_table[id]) continue;
        seen_table[id]=true;
        uint64_t subs=0,n=0;
        if (!read(entries,0x20+0x58*id,subs) || !read(subs,0x18,n) || n!=3) return {};
        bool ordinary=false;
        for (uint64_t i=0;i<n;++i) {
            uint16_t condition=0;
            if (!read(subs,0x20+32*i,condition)) return {};
            if (condition!=0x10) continue; // unconditional branch, after any locked alternates
            uint64_t rates=0,count=0;
            if (!read(subs,0x28+32*i,rates) || !read(rates,0x18,count) || count!=5) return {};
            unsigned weight_sum=0;
            for (uint64_t j=0;j<count;++j) {
                std::array<uint16_t,2> rate{};
                if (!read(rates,0x20+4*j,rate) || rate[0]>=ng || rate[1]>100) return {};
                if (!rate[1]) continue;
                weight_sum+=rate[1];
                uint64_t lineup=0,size=0;
                std::array<uint8_t,8> ids{};
                if (!read(groups,0x28+16*rate[0],lineup) || !read(lineup,0x18,size) ||
                    size!=ids.size() || !read(lineup,0x20,ids)) return {};
                for (auto enemy:ids) if (enemy) monsters[enemy]=true;
            }
            if (weight_sum!=100) return {};
            ordinary=true;
            break;
        }
        if (!ordinary) return {};
    }
    EnemyLabels labels(memory,base);
    std::vector<FloorMonster> names;
    for (unsigned id=1;id<monsters.size();++id) if (monsters[id]) {
        auto name=labels.lookup(memory,id);
        if (name.empty() || names.size()==12) return {};
        names.push_back({static_cast<uint16_t>(id),std::move(name)});
    }
    return names;
}
} // namespace etrian
