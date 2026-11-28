// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace etrian::weekly {
constexpr std::array<unsigned,4> percent{115,120,130,75};
constexpr unsigned mailbox_offset=176;
constexpr bool material_id(uint16_t id) {return id>=924 && id<=1474;}
constexpr int price(int base,std::size_t slot) {
    if(base<=0 || slot>=percent.size()) return base;
    return static_cast<int>(std::clamp<int64_t>(int64_t(base)*percent[slot]/100,1,std::numeric_limits<int>::max()));
}

struct List {
    unsigned deepest=0, floor=0;
    int week=-1;
    std::array<uint16_t,4> items{};
    std::vector<uint16_t> pool;
    bool pool_ready=false;

    void visit(unsigned visited) {
        if(visited<=deepest || visited>25) return;
        deepest=visited;
        pool.clear();
        pool_ready=false;
    }
    void observe(unsigned visited,std::vector<uint16_t> candidates) {
        if(!visited || visited>25 || visited<deepest) return;
        visit(visited);
        std::erase_if(candidates,[](uint16_t id){return !material_id(id);});
        std::ranges::sort(candidates);
        candidates.erase(std::unique(candidates.begin(),candidates.end()),candidates.end());
        pool=std::move(candidates);
        pool_ready=true;
    }
    bool refresh(uint32_t day) {
        const auto current=static_cast<int>(day/7);
        if(week>=current || !pool_ready) return false;
        week=current;
        floor=deepest;
        items={};
        auto choices=pool;
        uint32_t draw=0x85ebca6bu ^ (uint32_t(week)*0x9e3779b9u) ^ (floor*0xc2b2ae35u);
        for(auto& item:items) {
            if(choices.empty()) break;
            draw^=draw<<13; draw^=draw>>17; draw^=draw<<5;
            const auto slot=draw%choices.size();
            item=choices[slot];
            choices.erase(choices.begin()+slot);
        }
        return true;
    }
    uint64_t command(uint32_t epoch) const {
        if(week<0 || epoch>0xffffffu) return 0;
        uint64_t word=uint64_t(epoch)<<40;
        for(std::size_t i=0;i<items.size();++i)
            if(items[i]) word|=uint64_t(items[i]-923)<<(10*i);
        return word;
    }
};
}
