#pragma once
#include "core/mods/dsmod_module_abi.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
namespace etrian {
using IconPixels = std::array<uint8_t,54*54*4>;
inline int art_for_map_icon(int type) {
    static constexpr int art[]={4,30,1,2,10,7,31,32,6,9,12,13,14,16,17,15,18,33,41,8,33,35,36,11,34,38,39,40};
    return type>=0 && type<28 ? art[type] : 0;
}
// The observed update moves these base-game objects by 0x30 bytes. Validate either layout.
inline bool load_streamed_icon(const EdenDsmodHostApi& h,int number,IconPixels& out) {
    struct Texture { int number; const char* name; uint64_t object; uint32_t stream; int format; };
    static constexpr Texture textures[] = {
        {30,"icon_map30",0xfe8d0,0x163dd68,4}, {31,"icon_map31",0x3d1330,0x6d873a8,4},
        {32,"icon_map32",0x3bd048,0x4b401a8,4}, {33,"icon_map33",0x3d0e50,0x6d69ba8,4},
        {34,"icon_map34",0x2754b8,0x24a2fa8,4}, {35,"icon_map35",0x3a98e8,0x32317a8,4},
        {36,"icon_map36",0x3c50b8,0x537a3a8,4}, {38,"icon_map38",0x3d1520,0x6d8d1a8,4},
        {39,"icon_map39",0x3c7ff8,0x5a3e7a8,4}, {40,"icon_map10_inactive",0x2752d0,0x249cfa8,4},
        {41,"icon_FOE_Combined",0xf9070,0xb7b368,3},
    };
    const Texture* t=nullptr; for(const auto& entry:textures) if(entry.number==number) t=&entry;
    if(!t || !h.read_romfs) return false;
    const size_t name_len=std::strlen(t->name), header=4+((name_len+3)&~size_t{3});
    const size_t metadata_size=header+224;
    std::array<uint8_t,248> metadata{};
    auto u32=[&](size_t p){return uint32_t(metadata[p]) | uint32_t(metadata[p+1])<<8 | uint32_t(metadata[p+2])<<16 | uint32_t(metadata[p+3])<<24;};
    bool found=false;
    for(uint64_t shift:{uint64_t{0x30},uint64_t{0}}) {
        if(h.read_romfs(h.userdata,"romfs:/Data/sharedassets1.assets",t->object+shift,metadata.data(),metadata_size)!=metadata_size) continue;
        if(u32(0)!=name_len || std::memcmp(metadata.data()+4,t->name,name_len)!=0 || u32(header+8)!=54 || u32(header+12)!=54 || u32(header+16)!=16384 || u32(header+24)!=uint32_t(t->format) || u32(header+80)!=92 || u32(header+92)!=3 || u32(metadata_size-44)!=t->stream || u32(metadata_size-40)!=0 || u32(metadata_size-36)!=16384 || u32(metadata_size-32)!=25 || std::memcmp(metadata.data()+metadata_size-28,"sharedassets1.assets.resS",25)!=0) continue;
        found=true; break;
    }
    if(!found) return false;
    std::array<uint8_t,16384> bytes{};
    if(h.read_romfs(h.userdata,"romfs:/Data/sharedassets1.assets.resS",t->stream,bytes.data(),bytes.size())!=bytes.size()) return false;
    // 64x64 RGBA texture, NVIDIA block-linear: eight GOBs per block, 16-byte texels.
    for(int y=0;y<54;++y) for(int x=0;x<54;++x) {
        const int gx=(x/4)%4, gy=y%8;
        const int index=(gx/2)*16+(gy/2)*4+(gx%2)*2+gy%2;
        const int source=((x/16*8+y/8)*32+index)*16+(x%4)*4;
        std::memcpy(out.data()+(y*54+x)*4,bytes.data()+source,4);
    }
    return true;
}
inline bool load_game_icon(const EdenDsmodHostApi& h,int number,IconPixels& out) {
    static constexpr uint64_t offsets[] = {0,0xf6178,0x26ea60,0x26b780,0x3cdc60,0x3b4f00,0x3be908,0x3ae440,0x3a3598,0xfb210,0x268198,0x3c8c18,0x278da8,0x3c22b0,0x264bc0,0x3b1348,0x3a68f0,0xf1d88,0x3aad70,0x3b8db0};
    if(number>=30) return load_streamed_icon(h,number,out);
    if (!h.read_romfs || number<1 || number>19) return false;
    std::array<uint8_t,104+54*54*4> bytes{};
    char name[12]; if(number==19) std::memcpy(name,"icon_player",12); else std::snprintf(name,sizeof(name),"icon_map%02d",number);
    auto u32=[&](int p){return uint32_t(bytes[p]) | uint32_t(bytes[p+1])<<8 | uint32_t(bytes[p+2])<<16 | uint32_t(bytes[p+3])<<24;};
    for(uint64_t shift : {uint64_t{0x30},uint64_t{0}}) {
        if(h.read_romfs(h.userdata,"romfs:/Data/sharedassets1.assets",offsets[number]+shift,bytes.data(),bytes.size())!=bytes.size()) continue;
        if(u32(0)!=(number==19?11u:10u) || std::memcmp(bytes.data()+4,name,number==19?11:10)!=0 || u32(24)!=54 || u32(28)!=54 || u32(40)!=4 || u32(100)!=out.size()) continue;
        std::memcpy(out.data(),bytes.data()+104,out.size());
        return true;
    }
    return false;
}
}
