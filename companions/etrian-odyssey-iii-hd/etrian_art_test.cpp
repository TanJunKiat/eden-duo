#include "etrian_art.h"
#include <cassert>
#include <cstring>
#include <vector>

static void be(std::vector<uint8_t>& b,uint64_t n,int len) { for(int i=len-1;i>=0;--i) b.push_back(uint8_t(n>>(i*8))); }
static void le(std::vector<uint8_t>& b,size_t p,uint32_t n) { for(int i=0;i<4;++i) b[p+i]=uint8_t(n>>(i*8)); }
static void str(std::vector<uint8_t>& b,const char* s) { while(*s) b.push_back(*s++); b.push_back(0); }
static size_t read_file(void* u,const char* path,uint64_t off,void* dst,size_t n) {
    auto& b=*static_cast<std::vector<uint8_t>*>(u); if(std::strcmp(path,"romfs:/Data/data.unity3d")||off>b.size()||n>b.size()-off) return 0;
    std::memcpy(dst,b.data()+off,n); return n;
}
int main() {
    // Repeated-match LZ4 block and an invalid back-reference.
    const uint8_t compressed[]={0x32,'a','b','c',3,0}; uint8_t decoded[9]{};
    assert(etrian::lz4_decode(compressed,sizeof(compressed),decoded,sizeof(decoded)));
    assert(std::memcmp(decoded,"abcabcabc",9)==0);
    const uint8_t bad[]={0x00,0,0}; assert(!etrian::lz4_decode(bad,sizeof(bad),decoded,4));
    const uint8_t truncated[]={0xf0,255}; assert(!etrian::lz4_decode(truncated,sizeof(truncated),decoded,sizeof(decoded)));
    assert(!etrian::lz4_decode(compressed,sizeof(compressed)-1,decoded,sizeof(decoded)));

    // Minimal valid UnityFS container with one raw block and a real-layout inline texture object.
    constexpr size_t object=0x257450, block_size=0x420000, split=0x210000;
    std::vector<uint8_t> block(block_size); le(block,object,10); std::memcpy(block.data()+object+4,"icon_map01",10);
    le(block,object+24,54); le(block,object+28,54); le(block,object+40,4); le(block,object+100,54*54*4);
    for(size_t i=0;i<54*54*4;++i) block[object+104+i]=uint8_t(i);
    constexpr size_t streamed=0x3ffc18, stream_start=0x410000, stream_offset=0x200;
    le(block,streamed,10); std::memcpy(block.data()+streamed+4,"icon_map30",10);
    le(block,streamed+24,54); le(block,streamed+28,54); le(block,streamed+32,16384); le(block,streamed+40,4);
    le(block,streamed+96,92); le(block,streamed+108,3); le(block,streamed+196,stream_offset);
    le(block,streamed+204,16384); le(block,streamed+208,21); std::memcpy(block.data()+streamed+212,"resources.assets.resS",21);
    for(size_t i=0;i<16384;++i) block[stream_start+stream_offset+i]=uint8_t(i/16);
    std::vector<uint8_t> info(16); be(info,2,4);
    for(int i=0;i<2;++i) { be(info,split,4); be(info,split,4); be(info,0,2); }
    be(info,2,4); be(info,0,8); be(info,stream_start,8); be(info,4,4); str(info,"resources.assets");
    be(info,stream_start,8); be(info,block_size-stream_start,8); be(info,0,4); str(info,"resources.assets.resS");
    std::vector<uint8_t> bundle; str(bundle,"UnityFS"); be(bundle,7,4); str(bundle,"5.x.x"); str(bundle,"2020.3.33f1");
    be(bundle,0,8); be(bundle,info.size(),4); be(bundle,info.size(),4); be(bundle,0x40,4); while(bundle.size()%16) bundle.push_back(0);
    bundle.insert(bundle.end(),info.begin(),info.end()); bundle.insert(bundle.end(),block.begin(),block.end());
    EdenDsmodHostApi h{}; h.userdata=&bundle; h.read_romfs=read_file; etrian::IconPixels out{};
    assert(etrian::load_game_icon(h,1,out)); for(size_t i=0;i<out.size();++i) assert(out[i]==uint8_t(i));
    assert(etrian::load_game_icon(h,30,out));
    assert(out[0]==0 && out[4]==0 && out[16]==2 && out[54*4]==1);
    assert(!etrian::load_game_icon(h,2,out));
    // Out-of-range stream offsets and corrupt texture headers fail closed.
    size_t data_start=bundle.size()-block.size(); le(bundle,data_start+streamed+196,0xffffffff);
    assert(!etrian::load_game_icon(h,30,out));
    bundle[0]='X'; assert(!etrian::load_game_icon(h,1,out));
    bundle.resize(100); assert(!etrian::load_game_icon(h,1,out));
}
