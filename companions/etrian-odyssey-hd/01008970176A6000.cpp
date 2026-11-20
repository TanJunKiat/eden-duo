// SPDX-License-Identifier: GPL-3.0-or-later
// Experimental read-only Etrian Odyssey HD 1.0.3 map companion.
#include "etrian_map.h"
#include "core/mods/dsmod_module_abi.h"
#include "core/mods/dsmod_module_extensions.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <new>
#include <string>
#include <vector>

namespace {
constexpr uint64_t Title = UINT64_C(0x01008970176A6000);
constexpr char Build[] = "4CBA9B7CE88603D196F4C63639586B8F00000000000000000000000000000000";
constexpr uint64_t MethodSlot = 0x53a70d0;
struct State {
    std::mutex mutex;
    std::array<uint32_t, etrian::cells> tiles{};
    std::array<int32_t, etrian::cells> icons{};
    int x = -1, y = -1, floor = -1;
    unsigned generation = 0;
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
    uint64_t a=0, manager=0, floors=0, list=0, array=0, icon_array=0;
    int32_t floor=0,x=0,y=0,count=0,icon_count=0;
    bool ok=ptr(h,h->main_base+MethodSlot,a) && ptr(h,a+0x18,a) && ptr(h,a+0xc0,a) &&
        ptr(h,a,a) && ptr(h,a+0xb8,a) && ptr(h,a,manager) &&
        read(h,manager+0xd0,floor) && floor>=1 && floor<=30 &&
        read(h,manager+0x68,x) && read(h,manager+0x6c,y) &&
        x>=0 && x<etrian::width && y>=0 && y<etrian::height &&
        ptr(h,manager+0xd8,floors) && read(h,floors+0x18,count) && count>=floor && count<=30;
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
        if (!s.generation || s.tiles!=tiles || s.icons!=icons || s.x!=x || s.y!=y || s.floor!=floor) {
            s.tiles=tiles; s.icons=icons; s.x=x; s.y=y; s.floor=floor; ++s.generation;
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
EdenDsmodBool image(void* p,const EdenDsmodHostApi*,const char* key,void* receiver,EdenDsmodImageSink sink) {
    if(!p || !key || !sink) return EDEN_DSMOD_FALSE;
    auto& s=*static_cast<State*>(p);
    std::lock_guard<std::mutex> lock(s.mutex);
    const std::string expected="module:etrian:map/"+std::to_string(s.generation);
    if (!s.generation || key!=expected) return EDEN_DSMOD_FALSE;
    constexpr int scale=16,w=etrian::width*scale,h=etrian::height*scale;
    std::vector<uint8_t> pixels(w*h*4);
    auto put=[&](int x,int y,uint32_t c) { if(x<0||x>=w||y<0||y>=h)return; auto i=(y*w+x)*4; pixels[i]=c>>16; pixels[i+1]=c>>8; pixels[i+2]=c; pixels[i+3]=c>>24; };
    for(int y=0;y<etrian::height;++y) for(int x=0;x<etrian::width;++x) {
        int i=etrian::index(x,y), sy=etrian::screen_y(y); uint32_t f=s.tiles[i], c=etrian::floor_color(f);
        for(int py=0;py<scale;++py) for(int px=0;px<scale;++px) put(x*scale+px,sy*scale+py,(px==0||py==0)?0xff134467:c);
        if(etrian::has_right_wall(f)) for(int n=0;n<scale;++n) put(x*scale+scale-1,sy*scale+n,0xffb9efff);
        if(etrian::has_lower_wall(f)) for(int n=0;n<scale;++n) put(x*scale+n,sy*scale+scale-1,0xffb9efff);
        if(s.icons[i]>=0) for(int py=5;py<11;++py) for(int px=5;px<11;++px) put(x*scale+px,sy*scale+py,0xffffdf65);
    }
    const int py=etrian::screen_y(s.y)*scale, px=s.x*scale;
    for(int yy=3;yy<13;++yy) for(int xx=3;xx<13;++xx) if((xx-8)*(xx-8)+(yy-8)*(yy-8)<25) put(px+xx,py+yy,0xff62ffb4);
    sink(receiver,w,h,pixels.data(),pixels.size());
    return EDEN_DSMOD_TRUE;
}
const EdenDsmodModuleApi api{EDEN_DSMOD_MODULE_ABI_VERSION,sizeof(EdenDsmodModuleApi),0,EDEN_DSMOD_MODULE_ABI_HASH,Title,"Etrian Odyssey HD map",EDEN_DSMOD_CAP_EXTENSIONS,&supports,&create,&destroy,&sample,&tick};
const EdenDsmodModuleExtensions extensions{EDEN_DSMOD_EXT_VERSION,sizeof(EdenDsmodModuleExtensions),EDEN_DSMOD_EXT_HASH,&configure,nullptr,&image};
}
extern "C" __attribute__((visibility("default"))) const EdenDsmodModuleApi* eden_dsmod_get_module(uint32_t v,uint64_t h){return v==EDEN_DSMOD_MODULE_ABI_VERSION&&h==EDEN_DSMOD_MODULE_ABI_HASH?&api:nullptr;}
extern "C" __attribute__((visibility("default"))) const EdenDsmodModuleExtensions* eden_dsmod_get_extensions(uint32_t v,uint64_t h){return v==EDEN_DSMOD_EXT_VERSION&&h==EDEN_DSMOD_EXT_HASH?&extensions:nullptr;}
