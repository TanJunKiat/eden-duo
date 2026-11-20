// SPDX-License-Identifier: GPL-3.0-or-later
// Experimental read-only Etrian Odyssey III HD map companion.
#include "etrian_map.h"
#include "etrian_battle.h"
#include "etrian_art.h"
#include "../../src/core/mods/dsmod_module_abi.h"
#include "../../src/core/mods/dsmod_module_extensions.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <new>
#include <string>
#include <vector>

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
    auto mode=etrian::read_battle(reader,h->main_base,battle);
    {
        std::lock_guard<std::mutex> lock(s.mutex);
        if (mode==etrian::BattleMode::unknown) mode=etrian::BattleMode::loading;
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
EdenDsmodBool battle_image(const State& state,void* receiver,EdenDsmodImageSink sink) {
    constexpr int w=592,h=528;
    std::vector<uint8_t> pixels(w*h*4);
    auto box=[&](int x,int y,int bw,int bh,uint32_t color) {
        for(int yy=y;yy<y+bh && yy<h;++yy) for(int xx=x;xx<x+bw && xx<w;++xx) {
            if(xx<0 || yy<0) continue;
            auto i=(yy*w+xx)*4; pixels[i]=color>>16; pixels[i+1]=color>>8; pixels[i+2]=color; pixels[i+3]=color>>24;
        }
    };
    auto text=[&](int x,int y,const std::string& value,uint32_t color) {
        static constexpr const char* letters[]={
            "010101111101101","110101110101110","011100100100011","110101101101110","111100110100111","111100110100100",
            "011100101101011","101101111101101","111010010010111","001001001101010","101101110101101","100100100100111",
            "101111111101101","101111111111101","010101101101010","110101110100100","010101101111011","110101110101101",
            "011100010001110","111010010010010","101101101101111","101101101101010","101101111111101","101101010101101",
            "101101010010010","111001010100111"};
        static constexpr const char* numbers[]={"111101101101111","010110010010111","110001010100111","110001010001110","101101111001001","111100110001110","011100110101010","111001010010010","010101010101010","010101011001110"};
        for(char raw:value) {
            char c=raw>='a'&&raw<='z'?char(raw-'a'+'A'):raw;
            const char* bits=nullptr;
            if(c>='A'&&c<='Z') bits=letters[c-'A']; else if(c>='0'&&c<='9') bits=numbers[c-'0'];
            else if(c=='/') bits="001001010100100"; else if(c=='+') bits="000010111010000"; else if(c=='-') bits="000000111000000";
            else if(c=='?') bits="110001010000010"; else if(c=='.') bits="000000000000010"; else if(c==':') bits="000010000010000";
            if(bits) for(int yy=0;yy<5;++yy) for(int xx=0;xx<3;++xx) if(bits[yy*3+xx]=='1') box(x+xx*2,y+yy*2,2,2,color);
            x+=8;
        }
    };
    box(0,0,w,h,0xff102b29); box(0,0,w,32,0xff132c65);
    text(16,10,"BATTLE DASHBOARD",0xff9dfcf0);
    if(state.mode!=etrian::BattleMode::battle) {
        text(16,58,"BATTLE DATA UNAVAILABLE",0xfff4d78b);
    } else {
        auto group=[&](int top,const char* title,const std::vector<etrian::Combatant>& list,bool ally) {
            text(16,top-18,title,0xffd5ecff);
            for(size_t i=0;i<list.size() && i<8;++i) {
                const int row=int(i/3), col=int(i%3);
                const int remaining=int(list.size())-row*3, columns=remaining<3?remaining:3;
                const int x=(w-(columns*184+(columns-1)*10))/2+col*194, y=top+row*68;
                const auto& c=list[i];
                box(x,y,184,64,0xff001937); box(x+2,y+2,180,13,0xff003a70);
                box(x+2,y+17,180,45,0xff006ab5);
                box(x+20,y+17,2,45,0xff002c58);
                box(x+2,y+31,180,2,0xff002c58); box(x+2,y+45,180,2,0xff002c58);
                auto name=c.name; if(name.size()>21) name=name.substr(0,18)+"...";
                text(x+5,y+3,name,0xffe3f4ff);
                text(x+25,y+19,"HP "+std::to_string(c.hp)+"/"+std::to_string(c.max_hp),0xfff8f5cb);
                if(ally) text(x+25,y+33,c.tp<0?"TP ?":"TP "+std::to_string(c.tp)+"/"+std::to_string(c.max_tp),0xffe3f4ff);
                if(c.war_cry>0) text(x+25,y+48,"WAR CRY "+std::to_string(c.war_cry)+" T",0xfff9f6b2);
                else text(x+25,y+48,"EFFECTS UNKNOWN",0xffd3e4ed);
            }
        };
        group(58,"ENEMIES",state.battle.enemies,false); group(304,"ALLIES",state.battle.allies,true);
        text(14,514,"OTHER EFFECTS UNAVAILABLE",0xffc3dce6);
    }
    sink(receiver,w,h,pixels.data(),pixels.size());
    return EDEN_DSMOD_TRUE;
}
EdenDsmodBool image(void* p,const EdenDsmodHostApi* host,const char* key,void* receiver,EdenDsmodImageSink sink) {
    if(!p || !key || !sink) return EDEN_DSMOD_FALSE;
    auto& s=*static_cast<State*>(p);
    std::lock_guard<std::mutex> lock(s.mutex);
    const std::string expected="module:etrian:map/"+std::to_string(s.generation);
    if (!s.generation || key!=expected) return EDEN_DSMOD_FALSE;
    if (s.mode!=etrian::BattleMode::map) return battle_image(s,receiver,sink);
    if (!s.artwork_loaded && host && host->read_romfs) {
        for (int n=1;n<=41;++n) s.artwork_ok[n]=etrian::load_game_icon(*host,n,s.artwork[n]);
        for (bool loaded : s.artwork_ok) s.artwork_loaded |= loaded;
    }
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
    box(0,0,w,h,0xff003153);
    box(0,0,w,20,0xff00233c);
    box(left,top,etrian::width*scale,etrian::height*scale,0xff082744);
    box(4,2,50,16,0xff075a80);
    glyph(10,5,'B',0xff9dfcf0); if(s.floor>=10) glyph(20,5,char('0'+s.floor/10),0xff9dfcf0); glyph(s.floor>=10?28:20,5,char('0'+s.floor%10),0xff9dfcf0); glyph(s.floor>=10?36:28,5,'F',0xff9dfcf0);
    for(int col=0;col<7;++col) {
        box(left+col*5*scale+1,23,5*scale-2,23,0xff00c995);
        glyph(left+col*5*scale+35,27,char('1'+col),0xff003153,3);
    }
    for(int row=0;row<6;++row) {
        box(4,top+row*5*scale+1,25,5*scale-2,0xff00c995);
        glyph(12,top+row*5*scale+32,char('A'+row),0xff003153,3);
    }
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
        const int art=etrian::art_for_map_icon(icon);
        if(art && s.artwork_ok[art]) {
            const auto& texture=s.artwork[art];
            for(int yy=0;yy<16;++yy) for(int xx=0;xx<16;++xx) {
                const int source=((53-yy*54/16)*54+xx*54/16)*4;
                const int dest=((oy+yy)*w+ox+xx)*4;
                const int alpha=texture[source+3];
                for(int c=0;c<3;++c) pixels[dest+c]=uint8_t((texture[source+c]*alpha+pixels[dest+c]*(255-alpha)+127)/255);
            }
        } else if(icon==2 || icon==3) {
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
    if(s.artwork_ok[19]) {
        const auto& texture=s.artwork[19];
        for(int yy=0;yy<16;++yy) for(int xx=0;xx<16;++xx) {
            int sx=xx,sy=yy;
            if(s.direction==1){sx=yy;sy=15-xx;} else if(s.direction==2){sx=15-xx;sy=15-yy;} else if(s.direction==3){sx=15-yy;sy=xx;}
            const int source=((53-sy*54/16)*54+sx*54/16)*4;
            const int dest=((py+yy)*w+px+xx)*4, alpha=texture[source+3];
            for(int c=0;c<3;++c) pixels[dest+c]=uint8_t((texture[source+c]*alpha+pixels[dest+c]*(255-alpha)+127)/255);
        }
    } else for(int yy=1;yy<15;++yy) for(int xx=1;xx<15;++xx) {
        int dx=xx-8,dy=yy-8,rx=dx,ry=dy;
        if(s.direction==1){rx=dy;ry=-dx;} else if(s.direction==2){rx=-dx;ry=-dy;} else if(s.direction==3){rx=-dy;ry=dx;}
        int arx=rx<0?-rx:rx;
        if(ry>=-6&&ry<=5&&arx<=(ry+6)/2) put(px+xx,py+yy,0xff62ffb4);
    }
    sink(receiver,w,h,pixels.data(),pixels.size());
    return EDEN_DSMOD_TRUE;
}
const EdenDsmodModuleApi api{EDEN_DSMOD_MODULE_ABI_VERSION,sizeof(EdenDsmodModuleApi),0,EDEN_DSMOD_MODULE_ABI_HASH,Title,"Etrian Odyssey III HD map",EDEN_DSMOD_CAP_EXTENSIONS,&supports,&create,&destroy,&sample,&tick};
const EdenDsmodModuleExtensions extensions{EDEN_DSMOD_EXT_VERSION,sizeof(EdenDsmodModuleExtensions),EDEN_DSMOD_EXT_HASH,&configure,&action,&image};
}
extern "C" __attribute__((visibility("default"))) const EdenDsmodModuleApi* eden_dsmod_get_module(uint32_t v,uint64_t h){return v==EDEN_DSMOD_MODULE_ABI_VERSION&&h==EDEN_DSMOD_MODULE_ABI_HASH?&api:nullptr;}
extern "C" __attribute__((visibility("default"))) const EdenDsmodModuleExtensions* eden_dsmod_get_extensions(uint32_t v,uint64_t h){return v==EDEN_DSMOD_EXT_VERSION&&h==EDEN_DSMOD_EXT_HASH?&extensions:nullptr;}
