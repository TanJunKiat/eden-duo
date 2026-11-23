// SPDX-License-Identifier: GPL-3.0-or-later
// Experimental read-only Etrian Odyssey III HD companion.
#include "etrian_render.h"
#include "../../src/core/mods/dsmod_module_abi.h"
#include "../../src/core/mods/dsmod_module_extensions.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <new>
#include <string>

namespace {
constexpr uint64_t Title = UINT64_C(0x010084A0176AA000);
constexpr char Build[] = "3D0322EDA242DEC85321974C126F43AC00000000000000000000000000000000";
constexpr uint64_t MethodSlot = 0x61a60b8;
struct State {
    std::mutex mutex;
    std::array<uint32_t, etrian::cells> tiles{};
    std::array<int32_t, etrian::cells> icons{};
    int x = -1, y = -1, floor = -1, direction = -1;
    unsigned generation = 0;
    etrian::BattleMode mode = etrian::BattleMode::map;
    etrian::BattleSnapshot battle;
    std::array<etrian::IconPixels,42> artwork{};
    std::array<bool,42> artwork_ok{};
    bool artwork_loaded = false;
};
template<class T> bool read(const EdenDsmodHostApi* h, uint64_t p, T& v) {
    return p && h->is_mapped(h->userdata,p,sizeof(T)) && h->read_memory(h->userdata,p,&v,sizeof(T));
}
bool ptr(const EdenDsmodHostApi* h, uint64_t address, uint64_t& out) { return read(h,address,out) && out; }
EdenDsmodBool supports(const char* id) { return id && !std::strcmp(id,Build); }
void* create(const EdenDsmodHostApi* h,const char*) {
    if (!h || h->title_id != Title || !h->read_memory || !h->is_mapped || !h->publish_text || !h->publish_i64) return nullptr;
    return new (std::nothrow) State;
}
void destroy(void* p) { delete static_cast<State*>(p); }
void sample(void* p,const EdenDsmodHostApi* h) {
    if (!p || !h) return;
    auto& s=*static_cast<State*>(p);
    etrian::BattleSnapshot battle;
    const auto reader=[&](uint64_t address,void* data,size_t size) {
        return address && h->is_mapped(h->userdata,address,size) && h->read_memory(h->userdata,address,data,size);
    };
    const auto mode=etrian::read_battle(reader,h->main_base,battle);
    {
        std::lock_guard<std::mutex> lock(s.mutex);
        if (mode!=etrian::BattleMode::map) {
            if (s.mode!=mode || !(s.battle==battle)) ++s.generation;
            s.mode=mode; s.battle=std::move(battle);
            const std::string key="module:etrian:map/"+std::to_string(s.generation);
            h->publish_text(h->userdata,"map.key",key.c_str());
            h->publish_i64(h->userdata,"map.ready",1);
            return;
        }
        if (s.mode!=etrian::BattleMode::map) { s.mode=etrian::BattleMode::map; s.battle={}; ++s.generation; }
    }
    uint64_t a=0, manager=0, floors=0, list=0, array=0, icon_array=0;
    int32_t floor=0,x=0,y=0,direction=-1,count=0,icon_count=0;
    bool ok=ptr(h,h->main_base+MethodSlot,a) && ptr(h,a+0x18,a) && ptr(h,a+0xc0,a) &&
        ptr(h,a,a) && ptr(h,a+0xb8,a) && ptr(h,a,manager) &&
        read(h,manager+0xdc,floor) && floor>=1 && floor<=30 &&
        read(h,manager+0x70,x) && read(h,manager+0x74,y) &&
        read(h,manager+0x80,direction) && direction>=0 && direction<=3 &&
        x>=0 && x<etrian::width && y>=0 && y<etrian::height &&
        ptr(h,manager+0xe8,floors) && read(h,floors+0x18,count) && count>=floor && count<=30;
    const uint64_t entry = floors + 0x20 + uint64_t(floor-1)*0x28;
    ok=ok && ptr(h,entry+0x20,list) && read(h,list+0x18,count) && count>=etrian::cells && count<=2100 &&
        ptr(h,list+0x10,array) && read(h,array+0x18,count) && count>=etrian::cells && count<=2100 &&
        ptr(h,entry+0x08,icon_array) && read(h,icon_array+0x18,icon_count) && icon_count>=etrian::cells && icon_count<=2100;
    std::array<uint32_t,etrian::cells> tiles{};
    std::array<int32_t,etrian::cells> icons{};
    ok=ok && h->read_memory(h->userdata,array+0x20,tiles.data(),sizeof(tiles)) &&
        h->read_memory(h->userdata,icon_array+0x20,icons.data(),sizeof(icons));
    if (!ok) { h->publish_i64(h->userdata,"map.ready",0); return; }
    unsigned generation;
    {
        std::lock_guard<std::mutex> lock(s.mutex);
        if (!s.generation || s.tiles!=tiles || s.icons!=icons || s.x!=x || s.y!=y || s.floor!=floor || s.direction!=direction) {
            s.tiles=tiles; s.icons=icons; s.x=x; s.y=y; s.floor=floor; s.direction=direction; ++s.generation;
        }
        generation=s.generation;
    }
    const std::string key="module:etrian:map/"+std::to_string(generation);
    h->publish_text(h->userdata,"map.key",key.c_str());
    h->publish_i64(h->userdata,"map.floor",floor);
    h->publish_i64(h->userdata,"map.ready",1);
}
void tick(void*,const EdenDsmodHostApi*) {}
void configure(void*,const EdenDsmodHostExtensions*) {}
EdenDsmodBool action(void*,const char*,int64_t) { return EDEN_DSMOD_FALSE; }
EdenDsmodBool dashboard_image(const State& state,void* receiver,EdenDsmodImageSink sink) {
    etrian::glass::Canvas canvas;
    if(state.mode==etrian::BattleMode::town || state.mode==etrian::BattleMode::town_loading)
        etrian::glass::town(canvas,state.mode,state.battle.allies);
    else etrian::glass::battle(canvas,state.mode,state.battle);
    sink(receiver,canvas.width,canvas.height,canvas.pixels.data(),canvas.pixels.size());
    return EDEN_DSMOD_TRUE;
}
EdenDsmodBool image(void* p,const EdenDsmodHostApi* host,const char* key,void* receiver,EdenDsmodImageSink sink) {
    if(!p || !key || !sink) return EDEN_DSMOD_FALSE;
    auto& s=*static_cast<State*>(p);
    std::lock_guard<std::mutex> lock(s.mutex);
    const std::string expected="module:etrian:map/"+std::to_string(s.generation);
    if (!s.generation || key!=expected) return EDEN_DSMOD_FALSE;
    if (s.mode!=etrian::BattleMode::map) return dashboard_image(s,receiver,sink);
    if (!s.artwork_loaded && host && host->read_romfs) {
        for (int n=1;n<=41;++n) s.artwork_ok[n]=etrian::load_game_icon(*host,n,s.artwork[n]);
        for (bool loaded : s.artwork_ok) s.artwork_loaded |= loaded;
    }
    etrian::glass::Canvas canvas;
    etrian::glass::map(canvas,s.tiles,s.icons,s.artwork,s.artwork_ok,s.x,s.y,s.floor,s.direction);
    sink(receiver,canvas.width,canvas.height,canvas.pixels.data(),canvas.pixels.size());
    return EDEN_DSMOD_TRUE;
}
const EdenDsmodModuleApi api{EDEN_DSMOD_MODULE_ABI_VERSION,sizeof(EdenDsmodModuleApi),0,EDEN_DSMOD_MODULE_ABI_HASH,Title,"Etrian Odyssey III HD map",EDEN_DSMOD_CAP_EXTENSIONS,&supports,&create,&destroy,&sample,&tick};
const EdenDsmodModuleExtensions extensions{EDEN_DSMOD_EXT_VERSION,sizeof(EdenDsmodModuleExtensions),EDEN_DSMOD_EXT_HASH,&configure,&action,&image};
}
extern "C" __attribute__((visibility("default"))) const EdenDsmodModuleApi* eden_dsmod_get_module(uint32_t v,uint64_t h){return v==EDEN_DSMOD_MODULE_ABI_VERSION&&h==EDEN_DSMOD_MODULE_ABI_HASH?&api:nullptr;}
extern "C" __attribute__((visibility("default"))) const EdenDsmodModuleExtensions* eden_dsmod_get_extensions(uint32_t v,uint64_t h){return v==EDEN_DSMOD_EXT_VERSION&&h==EDEN_DSMOD_EXT_HASH?&extensions:nullptr;}
