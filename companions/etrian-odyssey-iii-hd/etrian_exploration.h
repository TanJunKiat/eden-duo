// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "etrian_enemy_names.h"
#include "etrian_map.h"
#include "etrian_quests.h"
#include <array>
#include <cstdint>
#include <initializer_list>
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

enum class HintIcon : uint8_t {
    none, pit, north, south, west, east, camp, door, save, stairs_up, stairs_down,
    treasure, shortcut, switch_wall, flag_door, warp, switch_plate, harvest, mine, take, event
};

struct FloorReference {
    std::array<ReferenceCell,cells> screen{};
    std::array<HintIcon,cells> markers{};
    bool ready=false;
    std::optional<size_t> primary_stair;
    bool operator==(const FloorReference&) const = default;
    // Special passages and doors may have markers but do not become hint routes.
    bool open(int x,int y) const {
        if (!ready || x<0 || x>=width || y<0 || y>=height) return false;
        const auto chip=screen[index(x,y)].chip;
        return (chip>=1 && chip<=13) || chip==15 || (chip>=17 && chip<=19) || chip==29;
    }
};

// Bounty candidates are limited to the primary entrance's ordinary walking
// component. Do not cross gates, secret passages, warps, forced movement or
// even unverified door orientations. This intentionally can omit offers.
inline std::array<bool,cells> main_bounty_area(const FloorReference& ref) {
    std::array<bool,cells> seen{};
    if(!ref.ready || !ref.primary_stair || *ref.primary_stair>=cells || ref.screen[*ref.primary_stair].chip!=18) return seen;
    std::array<size_t,cells> queue{};
    size_t last=0;
    const auto push=[&](size_t i) {seen[i]=true; queue[last++]=i;};
    push(*ref.primary_stair);
    for(size_t first=0;first<last;++first) {
        const auto i=queue[first];
        for(const auto& [dx,dy]:std::array<std::array<int,2>,4>{{{1,0},{-1,0},{0,1},{0,-1}}}) {
            const int x=int(i%width)+dx,y=int(i/width)+dy;
            if(x<0 || x>=width || y<0 || y>=height) continue;
            const auto n=index(x,y);
            if(!seen[n] && ref.screen[n].chip==1) push(n);
        }
    }
    return seen;
}

inline HintIcon chip_marker(uint8_t chip) {
    using enum HintIcon;
    switch(chip) {
    case 4: return pit;
    case 5: return north;
    case 6: return south;
    case 7: return west;
    case 8: return east;
    case 13: return camp;
    case 16: return door;
    case 17: return save;
    case 19: return treasure;
    case 20: case 21: return shortcut;
    case 22: return switch_wall;
    case 23: return flag_door;
    case 24: return warp;
    case 25: return switch_plate;
    default: return none;
    }
}

// DgLoadFloor copies the YGMapData for floor-1 from the installed asset list;
// that copy shares the very same List<DGMapData>. Chip lookup indexes y*35+x.
// Asset rows are north-first, unlike the recorded exploration grid.
template<class Reader>
FloorReference read_floor_reference(const Reader& memory,uint64_t base,int floor,int px,int py,bool icons=false,
                                    bool bounty_entrance=false) {
    constexpr int labyrinth_floors=25;
    if(floor<1 || floor>labyrinth_floors || px<0 || px>=width || py<0 || py>=height) return {};
    const auto read=[&](uint64_t object,uint64_t offset,auto& value) {
        return object && object<=UINT64_MAX-offset-sizeof(value) && memory(object+offset,&value,sizeof(value));
    };
    const auto follow=[&](uint64_t address,std::initializer_list<uint64_t> offsets) {
        for(auto offset:offsets) if(!read(address,offset,address) || !address) return uint64_t(0);
        return address;
    };
    const auto controller=[&] { return follow(base,{0x61a6040,0x18,0xc0,0,0xb8,0,0x78,0x10}); };
    const auto asset_geometry=[&] {
        const auto assets=follow(base,{0x61a6018,0x18,0xc0,0,0xb8,0,0x18});
        const auto entries=follow(assets,{0x10});
        int32_t count=0;
        uint64_t capacity=0;
        // The last asset (entry 26) belongs to the sea, not the labyrinth.
        if(!read(assets,0x18,count) || count!=labyrinth_floors+1 ||
           !read(entries,0x18,capacity) || capacity<uint64_t(count) || capacity>64) return uint64_t(0);
        return follow(entries,{0x20+uint64_t(floor-1)*8,0x20});
    };
    const auto ctrl=controller();
    const auto list=follow(ctrl,{0x140});
    const auto data=follow(list,{0x10});
    int32_t count=0;
    uint64_t capacity=0;
    struct Header {
        int32_t mode;
        uint32_t id;
        int32_t w,h;
        bool operator==(const Header&) const = default;
    } header{}, after{};
    if(!list || list!=asset_geometry() || !read(ctrl,0x10,header) || header.mode!=0 ||
       header.w!=width || header.h!=height || !read(list,0x18,count) || count!=cells ||
       !read(data,0x18,capacity) || capacity<cells || capacity>2*cells) return {};
    FloorReference ref;
    if(!read(data,0x20,ref.screen)) return {};
    for(const auto& c:ref.screen) if(c.chip>30 || (c.chip>=26 && c.chip<=28)) return {};
    const auto player_chip=ref.screen[index(px,screen_y(py))].chip;
    // A dynamic door or other special chip can contain the live player, but
    // those chips do not become hints. Wall and padding never contain them.
    if(!player_chip || player_chip==14) return {};
    if(icons || bounty_entrance) {
        using enum HintIcon;
        // DgMapGetStairData uses DGMapData.paramId to index ChipParamStair.
        // goFloor 30 is the Armoroad exit. Same-floor stairs have no verified direction.
        const auto stair_list=follow(ctrl,{0x158});
        const auto stairs=follow(stair_list,{0x10});
        int32_t stair_count=0, list_size=0;
        uint64_t stair_capacity=0;
        const bool stairs_ok=read(ctrl,0x150,stair_count) && stair_count>0 && stair_count<=256 &&
            read(stair_list,0x18,list_size) && list_size==stair_count &&
            read(stairs,0x18,stair_capacity) && stair_capacity>=uint64_t(list_size) && stair_capacity<=256;
        bool entrance_ambiguous=false;
        for(size_t i=0;i<ref.screen.size();++i) {
            const auto cell=ref.screen[i];
            if(icons) ref.markers[i]=chip_marker(cell.chip);
            if(cell.chip!=18) continue;
            if(!stairs_ok || cell.parameter>=stair_count) { entrance_ambiguous=true; continue; }
            std::array<uint8_t,3> target{};
            if(!read(stairs,0x20+16*cell.parameter,target)) { entrance_ambiguous=true; continue; }
            const auto [destination,x,y]=target;
            if(bounty_entrance && ((floor==1 && destination==30) ||
               (floor>1 && destination==floor-1 && x<width && y<height))) {
                if(ref.primary_stair) entrance_ambiguous=true;
                ref.primary_stair=i;
            }
            if(!icons) continue;
            if(destination==30 && floor==1) ref.markers[i]=stairs_up;
            else if(destination>=1 && destination<=labyrinth_floors && x<width && y<height && destination!=floor)
                ref.markers[i]=destination>floor?stairs_down:stairs_up;
        }
        if(entrance_ambiguous) ref.primary_stair.reset();
    }
    if(icons) {
        using enum HintIcon;
        const auto place_marker=[&](int x,int y,HintIcon icon) {
            if(x<0 || x>=width || y<0 || y>=height) return;
            const auto i=index(x,y);
            const auto chip=ref.screen[i].chip;
            // Prefer the explicit feature to a generic event sharing its square.
            if(chip && chip!=14 && ref.markers[i]==none) ref.markers[i]=icon;
        };
        const auto master=follow(base,{0x61a60f0,0x18,0xc0,0,0xb8,0});
        // DgItemPointGetXY scans exactly 57 DG_ITEM_POINT records in this build.
        const auto gather=follow(master,{0xa0,0x10});
        std::array<std::array<uint8_t,24>,57> gather_rows{};
        uint64_t gather_count=0;
        if(read(gather,0x18,gather_count) && gather_count==gather_rows.size() && read(gather,0x20,gather_rows)) {
            constexpr HintIcon kinds[]{harvest,mine,take};
            for(const auto& row:gather_rows)
                if(row[0]==floor && row[3]<std::size(kinds)) place_marker(row[1],row[2],kinds[row[3]]);
        }
        // GetMasterDungeonEventIndexTable indexes by floor, then record. Only exact
        // floor/X/Y entries qualify; scripts and event eligibility are not evaluated.
        const auto tables=follow(master,{0x110});
        uint64_t table_count=0, event_count=0;
        if(read(tables,0x18,table_count) && table_count==27) {
            const auto events=follow(tables,{0x20+uint64_t(floor)*8,0x10});
            std::array<std::array<int8_t,48>,256> rows{};
            // Read only the populated records, atomically with respect to read failure.
            if(read(events,0x18,event_count) && event_count && event_count<=rows.size() &&
               events<=UINT64_MAX-0x20-event_count*sizeof(rows[0]) &&
               memory(events+0x20,rows.data(),event_count*sizeof(rows[0]))) {
                for(size_t i=0;i<event_count;++i) {
                    const auto& row=rows[i];
                    if(row[1]==42+floor) place_marker(row[4],row[5],event);
                }
            }
        }
    }
    if(controller()!=ctrl || asset_geometry()!=list || follow(ctrl,{0x140,0x10})!=data ||
       follow(ctrl,{0x140})!=list || !read(ctrl,0x10,after) || after!=header) return {};
    ref.ready=true;
    return ref;
}
// DungeonItemPointState selects one of three signed item IDs at +0x10/+0x12/+0x14.
template<class Reader>
std::optional<std::vector<uint16_t>> read_gather_materials(const Reader& memory,uint64_t base,
        int floor,const std::array<bool,cells>& area) {
    using quest_data::follow;
    using quest_data::read;
    const auto data=follow(memory,base,{0x61a60f0,0x18,0xc0,0,0xb8,0,0xa0,0x10});
    std::array<std::array<uint8_t,24>,57> rows{};
    uint64_t count=0;
    if(!read(memory,data,0x18,count) || count!=rows.size() || !read(memory,data,0x20,rows)) return {};
    std::vector<uint16_t> result;
    for(const auto& row:rows) {
        const int x=row[1], y=row[2];
        if(row[0]!=floor || row[3]>=3 || x>=width || y>=height || !area[index(x,y)]) continue;
        for(unsigned offset:{16,18,20}) {
            const uint16_t id=row[offset]|(uint16_t(row[offset+1])<<8);
            if(id) result.push_back(id);
        }
    }
    if(data!=follow(memory,base,{0x61a60f0,0x18,0xc0,0,0xb8,0,0xa0,0x10})) return {};
    return result;
}

struct FloorMonster {
    uint16_t id=0;
    std::string name;
    bool operator==(const FloorMonster&) const = default;
};
using FloorMonsters=std::optional<std::vector<FloorMonster>>;
inline constexpr size_t guide_page_size=12;
struct MonsterDrop {
    std::string name;
    std::optional<unsigned> base_chance;
    bool conditional=false;
    uint16_t item=0;
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
        drops.push_back({std::move(name),condition?std::nullopt:std::optional<unsigned>(threshold),condition!=0,item});
    }
    return drops;
}

// Follow the verified ordinary HD formation subtables. Skip flag-/time-gated
// variants rather than presenting them as ordinary monsters or assigning odds.
template<class Reader>
FloorMonsters read_floor_monsters(const Reader& memory,uint64_t base,const FloorReference& ref,
                                  const std::array<bool,cells>* area=nullptr) {
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
    for (size_t i=0;i<ref.screen.size();++i) {
        if(area && !(*area)[i]) continue;
        const auto& cell=ref.screen[i];
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
            std::array<std::array<uint16_t,2>,5> group_rates{};
            if(!read(rates,0x20,group_rates)) return {};
            unsigned weight_sum=0;
            for(const auto& rate:group_rates) {
                if(rate[0]>=ng || rate[1]>100) return {};
                weight_sum+=rate[1];
            }
            if(weight_sum>100) return {};
            // EncountDecideGroup uses the first group for rolls beyond the
            // cumulative thresholds. B8F and B24F have such ordinary tables.
            for(size_t j=0;j<group_rates.size();++j) {
                const auto [group,weight]=group_rates[j];
                if(!weight && !(j==0 && weight_sum<100)) continue;
                uint64_t lineup=0,size=0;
                std::array<uint8_t,8> ids{};
                if(!read(groups,0x28+16*group,lineup) || !read(lineup,0x18,size) ||
                   size!=ids.size() || !read(lineup,0x20,ids)) return {};
                for(auto enemy:ids) if(enemy) monsters[enemy]=true;
            }
            ordinary=true;
            break;
        }
        if (!ordinary) return {};
    }
    EnemyLabels labels(memory,base);
    std::vector<FloorMonster> names;
    for (unsigned id=1;id<monsters.size();++id) if (monsters[id]) {
        auto name=labels.lookup(memory,id);
        if (name.empty()) return {};
        names.push_back({static_cast<uint16_t>(id),std::move(name)});
    }
    return names;
}
} // namespace etrian
