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
    int x = -1, y = -1, floor = -1, direction = -1;
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
    int32_t floor=0,x=0,y=0,direction=-1,count=0,icon_count=0;
    bool ok=ptr(h,h->main_base+MethodSlot,a) && ptr(h,a+0x18,a) && ptr(h,a+0xc0,a) &&
        ptr(h,a,a) && ptr(h,a+0xb8,a) && ptr(h,a,manager) &&
        read(h,manager+0xd0,floor) && floor>=1 && floor<=30 &&
        read(h,manager+0x68,x) && read(h,manager+0x6c,y) &&
        read(h,manager+0x78,direction) && direction>=0 && direction<=3 &&
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
EdenDsmodBool image(void* p,const EdenDsmodHostApi*,const char* key,void* receiver,EdenDsmodImageSink sink) {
    if(!p || !key || !sink) return EDEN_DSMOD_FALSE;
    auto& s=*static_cast<State*>(p);
    std::lock_guard<std::mutex> lock(s.mutex);
    const std::string expected="module:etrian:map/"+std::to_string(s.generation);
    if (!s.generation || key!=expected) return EDEN_DSMOD_FALSE;
    constexpr int scale=16,left=32,top=48,w=left+etrian::width*scale,h=top+etrian::height*scale;
    std::vector<uint8_t> pixels(w*h*4);
    auto put=[&](int x,int y,uint32_t c) { if(x<0||x>=w||y<0||y>=h)return; auto i=(y*w+x)*4; pixels[i]=c>>16; pixels[i+1]=c>>8; pixels[i+2]=c; pixels[i+3]=c>>24; };
    auto box=[&](int x,int y,int bw,int bh,uint32_t c) { for(int yy=0;yy<bh;++yy) for(int xx=0;xx<bw;++xx) put(x+xx,y+yy,c); };
    // Compact 3x5 glyphs; no game assets are distributed with the package.
    auto glyph=[&](int x,int y,char ch,uint32_t color,int z=2) {
        const char* bits=nullptr;
        switch(ch) {
        case 'A':bits="010101111101101";break; case 'B':bits="110101110101110";break;
        case 'C':bits="011100100100011";break; case 'D':bits="110101101101110";break;
        case 'E':bits="111100110100111";break; case 'F':bits="111100110100100";break;
        case '0':bits="111101101101111";break; case '1':bits="010110010010111";break;
        case '2':bits="110001010100111";break; case '3':bits="110001010001110";break;
        case '4':bits="101101111001001";break; case '5':bits="111100110001110";break;
        case '6':bits="011100110101010";break; case '7':bits="111001010010010";break;
        case '8':bits="010101010101010";break; case '9':bits="010101011001110";break;
        case '!':bits="010010010000010";break; case '?':bits="110001010000010";break;
        case 'M':bits="101111111101101";break; case 'T':bits="111010010010010";break;
        case 'X':bits="101101010101101";break; case 'O':bits="010101101101010";break;
        case '^':bits="010111010010010";break; case 'v':bits="010010010111010";break;
        case '<':bits="001010111010001";break; case '>':bits="100010111010100";break;
        }
        if(bits) for(int yy=0;yy<5;++yy) for(int xx=0;xx<3;++xx) if(bits[yy*3+xx]=='1') box(x+xx*z,y+yy*z,z,z,color);
    };
    box(0,0,w,h,0xff173c49);
    box(left,top,etrian::width*scale,etrian::height*scale,0xff082744);
    glyph(10,5,'B',0xffffe8b2); if(s.floor>=10) glyph(20,5,char('0'+s.floor/10),0xffffe8b2); glyph(s.floor>=10?28:20,5,char('0'+s.floor%10),0xffffe8b2); glyph(s.floor>=10?36:28,5,'F',0xffffe8b2);
    for(int col=0;col<7;++col) glyph(left+col*5*scale+37,27,char('1'+col),0xffe5e2b3);
    for(int row=0;row<6;++row) glyph(13,top+row*5*scale+35,char('A'+row),0xffe5e2b3);
    for(int y=0;y<etrian::height;++y) for(int x=0;x<etrian::width;++x) {
        int i=etrian::index(x,y), sy=etrian::screen_y(y), ox=left+x*scale, oy=top+sy*scale; uint32_t f=s.tiles[i], c=etrian::floor_color(f);
        for(int py=0;py<scale;++py) for(int px=0;px<scale;++px) put(ox+px,oy+py,(px==0||py==0)?0xff16465c:c);
        if(x%5==0) box(ox,oy,1,scale,0xff447c84);
        if(sy%5==0) box(ox,oy,scale,1,0xff447c84);
        if(etrian::has_right_wall(f)) box(ox+scale-2,oy,2,scale,0xffc6f4ec);
        if(etrian::has_lower_wall(f)) box(ox,oy+scale-2,scale,2,0xffc6f4ec);
        int icon=s.icons[i]; char symbol=0; uint32_t color=0xffffdd77;
        switch(icon) {
        case 0:case 1:symbol='D';break; case 2:symbol='^';break; case 3:symbol='v';break;
        case 4:case 27:symbol='M';break; case 5:case 6:symbol='!';color=0xffffa261;break;
        case 7:symbol='?';break; case 8:case 9:case 10:symbol='T';color=0xff96ef9b;break;
        case 11:symbol='^';break; case 12:symbol='v';break; case 13:symbol='<';break; case 14:symbol='>';break;
        case 15:symbol='^';break; case 16:symbol='>';break; case 17:case 20:case 24:symbol='T';break;
        case 18:symbol='F';color=0xffff725d;break; case 19:case 21:symbol='O';break; case 22:symbol='X';break; case 23:symbol='A';break;
        case 25:symbol='^';break; case 26:symbol='>';break; default:if(icon>=0)symbol='?';break;
        }
        if(icon==2 || icon==3) {
            // Stair symbols use opposing stepped diagonals, not the generic arrow markers.
            box(ox+2,oy+2,12,12,0xff273b35);
            for(int n=0;n<4;++n) {
                int step=icon==2 ? 3-n : n;
                box(ox+3+n*2,oy+4+step*2,4,2,color);
            }
        } else if(icon==17 || icon==20 || icon==24) {
            box(ox+2,oy+3,12,10,0xff273b35); box(ox+3,oy+4,10,7,icon==24?0xffb3a684:0xffd9a343);
            box(ox+3,oy+7,10,1,0xff513a24); box(ox+7,oy+6,2,3,0xffffe7a2);
        } else if(symbol) { box(ox+3,oy+2,10,12,0xff273b35); glyph(ox+5,oy+3,symbol,color); }
    }
    const int py=top+etrian::screen_y(s.y)*scale, px=left+s.x*scale;
    // Draw an outlined arrow; the game direction enum is north/east/south/west (0..3).
    for(int yy=1;yy<15;++yy) for(int xx=1;xx<15;++xx) {
        int dx=xx-8,dy=yy-8,rx=dx,ry=dy;
        if(s.direction==1){rx=dy;ry=-dx;} else if(s.direction==2){rx=-dx;ry=-dy;} else if(s.direction==3){rx=-dy;ry=dx;}
        int arx=rx<0?-rx:rx;
        if(ry>=-6&&ry<=5&&arx<=(ry+6)/2) put(px+xx,py+yy,0xff62ffb4);
    }
    sink(receiver,w,h,pixels.data(),pixels.size());
    return EDEN_DSMOD_TRUE;
}
const EdenDsmodModuleApi api{EDEN_DSMOD_MODULE_ABI_VERSION,sizeof(EdenDsmodModuleApi),0,EDEN_DSMOD_MODULE_ABI_HASH,Title,"Etrian Odyssey HD map",EDEN_DSMOD_CAP_EXTENSIONS,&supports,&create,&destroy,&sample,&tick};
const EdenDsmodModuleExtensions extensions{EDEN_DSMOD_EXT_VERSION,sizeof(EdenDsmodModuleExtensions),EDEN_DSMOD_EXT_HASH,&configure,nullptr,&image};
}
extern "C" __attribute__((visibility("default"))) const EdenDsmodModuleApi* eden_dsmod_get_module(uint32_t v,uint64_t h){return v==EDEN_DSMOD_MODULE_ABI_VERSION&&h==EDEN_DSMOD_MODULE_ABI_HASH?&api:nullptr;}
extern "C" __attribute__((visibility("default"))) const EdenDsmodModuleExtensions* eden_dsmod_get_extensions(uint32_t v,uint64_t h){return v==EDEN_DSMOD_EXT_VERSION&&h==EDEN_DSMOD_EXT_HASH?&extensions:nullptr;}
