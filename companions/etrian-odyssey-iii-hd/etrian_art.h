#pragma once
#include "../../src/core/mods/dsmod_module_abi.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
namespace etrian {
using IconPixels = std::array<uint8_t,54*54*4>;
inline int art_for_map_icon(int type) {
    static constexpr int art[]={4,30,1,2,10,7,31,32,6,9,12,13,14,16,17,15,18,33,41,8,33,35,36,11,34,38,39,40};
    return type>=0 && type<28 ? art[type] : 0;
}
inline bool lz4_decode(const uint8_t* src,size_t n,uint8_t* dst,size_t size) {
    size_t i=0,o=0;
    auto length=[&](size_t base,size_t& result) {
        result=base; if(base!=15) return true;
        uint8_t v; do { if(i==n) return false; v=src[i++]; if(result>size || v>size-result) return false; result+=v; } while(v==255);
        return true;
    };
    while(i<n) {
        uint8_t token=src[i++]; size_t literal=0;
        if(!length(token>>4,literal) || literal>n-i || literal>size-o) return false;
        std::memcpy(dst+o,src+i,literal); i+=literal; o+=literal;
        if(i==n) return o==size;
        if(n-i<2) return false; size_t offset=src[i]|size_t(src[i+1])<<8; i+=2;
        if(!offset || offset>o) return false;
        size_t match=0; if(!length(token&15,match) || match>size-o || size-o-match<4) return false;
        match+=4; for(size_t j=0;j<match;++j) { dst[o]=dst[o-offset]; ++o; }
    }
    return o==size;
}
inline uint32_t le32(const uint8_t* p) { return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24; }
inline uint32_t be32(const uint8_t* p) { return uint32_t(p[0])<<24|uint32_t(p[1])<<16|uint32_t(p[2])<<8|uint32_t(p[3]); }
inline uint64_t be64(const uint8_t* p) { return uint64_t(be32(p))<<32|be32(p+4); }

class Bundle {
    struct Block { uint64_t raw,packed; uint32_t size,stored; uint16_t flags; };
    struct Node { uint64_t start=0,size=0; bool found=false; };
    const EdenDsmodHostApi& h;
    std::vector<Block> blocks;
    Node assets,streams;
    static constexpr const char* path="romfs:/Data/data.unity3d";
    bool read(uint64_t offset,void* p,size_t n) const { return h.read_romfs && h.read_romfs(h.userdata,path,offset,p,n)==n; }
    static bool unpack(uint16_t flags,const std::vector<uint8_t>& in,std::vector<uint8_t>& out) {
        if((flags&63)==0) { if(in.size()!=out.size()) return false; std::memcpy(out.data(),in.data(),out.size()); return true; }
        return ((flags&63)==2 || (flags&63)==3) && lz4_decode(in.data(),in.size(),out.data(),out.size());
    }
public:
    explicit Bundle(const EdenDsmodHostApi& host):h(host) {}
    bool open() {
        std::array<uint8_t,256> header{}; if(!read(0,header.data(),header.size()) || std::memcmp(header.data(),"UnityFS\0",8)!=0 || be32(header.data()+8)!=7) return false;
        size_t p=12; for(int j=0;j<2;++j) { while(p<header.size() && header[p]) ++p; if(p==header.size()) return false; ++p; }
        if(p+20>header.size()) return false;
        uint32_t compressed=be32(header.data()+p+8), uncompressed=be32(header.data()+p+12), flags=be32(header.data()+p+16);
        if((flags&0x80) || compressed==0 || compressed>2*1024*1024 || uncompressed<24 || uncompressed>2*1024*1024) return false;
        p=(p+20+15)&~size_t(15);
        std::vector<uint8_t> packed(compressed),info(uncompressed); if(!read(p,packed.data(),packed.size()) || !unpack(flags,packed,info)) return false;
        size_t at=16; auto take=[&](size_t n){ if(n>info.size()-at) return false; at+=n; return true; };
        if(!take(4)) return false; uint32_t count=be32(info.data()+at-4); if(!count || count>12000 || count>(info.size()-at)/10) return false;
        uint64_t raw=0, file=p+compressed;
        if(flags&0x200) file=(file+15)&~uint64_t(15);
        for(uint32_t i=0;i<count;++i) {
            uint32_t size=be32(info.data()+at),stored=be32(info.data()+at+4); uint16_t mode=uint16_t(info[at+8])<<8|info[at+9]; at+=10;
            if(!size || size>4*1024*1024 || !stored || stored>4*1024*1024 || raw+size>UINT64_C(0x80000000) || file+stored>UINT64_C(0x80000000)) return false;
            blocks.push_back({raw,file,size,stored,mode}); raw+=size; file+=stored;
        }
        if(!take(4)) return false; count=be32(info.data()+at-4); if(count>1024) return false;
        for(uint32_t i=0;i<count;++i) {
            if(!take(20)) return false; uint64_t start=be64(info.data()+at-20),size=be64(info.data()+at-12);
            size_t begin=at; while(at<info.size() && info[at] && at-begin<=255) ++at;
            if(at==info.size() || at-begin>255 || start>raw || size>raw-start) return false;
            std::string name(reinterpret_cast<const char*>(info.data()+begin),at-begin); ++at;
            Node* node=name=="resources.assets" ? &assets : name=="resources.assets.resS" ? &streams : nullptr;
            if(node) { if(node->found) return false; *node={start,size,true}; }
        }
        return assets.found;
    }
    bool range(bool stream,uint64_t offset,void* output,size_t n) const {
        const Node& node=stream?streams:assets; if(!node.found || offset>node.size || n>node.size-offset) return false;
        uint64_t cursor=node.start+offset; auto* dst=static_cast<uint8_t*>(output); size_t remaining=n;
        for(const auto& b:blocks) {
            if(cursor>=b.raw+b.size || cursor<b.raw) continue;
            std::vector<uint8_t> in(b.stored),out(b.size); if(!read(b.packed,in.data(),in.size()) || !unpack(b.flags,in,out)) return false;
            size_t pos=size_t(cursor-b.raw), amount=std::min(remaining,size_t(b.size)-pos); std::memcpy(dst,out.data()+pos,amount);
            dst+=amount; cursor+=amount; remaining-=amount; if(!remaining) return true;
        }
        return remaining==0;
    }
};
inline bool load_game_icon(const EdenDsmodHostApi& h,int number,IconPixels& out) {
    static constexpr uint32_t offsets[]={0,0x257450,0x579138,0x573dd0,0x82e248,0x6eeca0,0x806a58,0x6e4210,0x6d0d00,0x3f4a30,0x56ee00,0x820828,0x5a0be8,0x80c7e0,0x569810,0x6e8908,0x6d4ce0,0x24edd8,0x6de170,0x7f4e10};
    struct Texture { int id; const char* name; uint32_t offset; int format; };
    static constexpr Texture streamed[]={
        {30,"icon_map30",0x3ffc18,4},{31,"icon_map31",0x833cf0,4},{32,"icon_map32",0x7ff538,4},
        {33,"icon_map33",0x8324b0,4},{34,"icon_map34",0x58e650,4},{35,"icon_map35",0x6d8930,4},
        {36,"icon_map36",0x80f6d0,4},{38,"icon_map38",0x8350e0,4},{39,"icon_map39",0x81d508,4},
        {40,"icon_map10_inactive",0x58e0f8,4},{41,"icon_FOE_Combined",0x25a528,3}};
    if(number<1 || (number>19 && number<30) || number>41) return false;
    Bundle bundle(h); if(!bundle.open()) return false;
    if(number<=19) {
        std::array<uint8_t,104+54*54*4> data{}; char name[12];
        if(number==19) std::memcpy(name,"icon_player",12); else std::snprintf(name,sizeof(name),"icon_map%02d",number);
        if(!bundle.range(false,offsets[number],data.data(),data.size()) || le32(data.data())!=(number==19?11u:10u) || std::memcmp(data.data()+4,name,number==19?11:10) || le32(data.data()+24)!=54 || le32(data.data()+28)!=54 || le32(data.data()+40)!=4 || le32(data.data()+100)!=out.size()) return false;
        std::memcpy(out.data(),data.data()+104,out.size()); return true;
    }
    const Texture* t=nullptr; for(const auto& item:streamed) if(item.id==number) t=&item; if(!t) return false;
    size_t name_len=std::strlen(t->name), head=4+((name_len+3)&~size_t(3)), size=head+220;
    std::array<uint8_t,248> meta{}; if(!bundle.range(false,t->offset,meta.data(),size)) return false;
    if(le32(meta.data())!=name_len || std::memcmp(meta.data()+4,t->name,name_len) || le32(meta.data()+head+8)!=54 || le32(meta.data()+head+12)!=54 || le32(meta.data()+head+16)!=16384 || le32(meta.data()+head+24)!=uint32_t(t->format) || le32(meta.data()+head+80)!=92 || le32(meta.data()+head+92)!=3 || le32(meta.data()+size-40+4)!=0 || le32(meta.data()+size-32)!=16384 || le32(meta.data()+size-28)!=21 || std::memcmp(meta.data()+size-24,"resources.assets.resS",21)) return false;
    uint32_t offset=le32(meta.data()+size-40); std::array<uint8_t,16384> pixels{}; if(!bundle.range(true,offset,pixels.data(),pixels.size())) return false;
    for(int y=0;y<54;++y) for(int x=0;x<54;++x) {
        const int gx=(x/4)%4,gy=y%8,index=(gx/2)*16+(gy/2)*4+(gx%2)*2+gy%2;
        const int source=((x/16*8+y/8)*32+index)*16+(x%4)*4;
        std::memcpy(out.data()+(y*54+x)*4,pixels.data()+source,4);
    }
    return true;
}
}
