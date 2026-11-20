#include "etrian_art.h"
#include <cassert>
#include <cstring>
struct Fake { unsigned calls=0; bool valid=true; uint64_t shift=0; };
size_t source(void* p,const char* path,uint64_t offset,void* out,size_t size) {
 auto& f=*static_cast<Fake*>(p); ++f.calls;
 if(std::strcmp(path,"romfs:/Data/sharedassets1.assets.resS")==0) {
  if(offset!=0x678bae8 || size!=16384 || !f.valid)return 0;
  auto* b=static_cast<unsigned char*>(out); std::memset(b,0,size); b[0]=77; b[3]=255; b[4628]=123; b[4631]=255; return size;
 }
 assert(std::strcmp(path,"romfs:/Data/sharedassets1.assets")==0);
 if(offset==0x3d0698+f.shift && size==240 && f.valid) {
  auto* b=static_cast<unsigned char*>(out); std::memset(b,0,size);
  b[0]=10; std::memcpy(b+4,"icon_map33",10); b[24]=54; b[28]=54; b[33]=64; b[40]=4;
  b[96]=92; b[108]=3; b[196]=0xe8; b[197]=0xba; b[198]=0x78; b[199]=6; b[205]=64; b[208]=25;
  std::memcpy(b+212,"sharedassets1.assets.resS",25); return size;
 }
 if((offset!=0xf7018+f.shift && offset!=0x3b9640+f.shift) || size!=11768 || !f.valid)return 0;
 auto* b=static_cast<unsigned char*>(out); std::memset(b,0,size);
 if(offset==0xf7018+f.shift) { b[0]=10; std::memcpy(b+4,"icon_map01",10); }
 else { b[0]=11; std::memcpy(b+4,"icon_player",11); } b[24]=54; b[28]=54; b[40]=4;
 b[100]=0x90; b[101]=0x2d; b[104]=1; b[105]=2; b[106]=3; b[107]=255;
 return size;
}
int main() {
 constexpr int expected[]={4,30,1,2,10,7,31,32,6,9,12,13,14,16,17,15,18,33,41,8,33,35,36,11,34,38,39,40};
 for(int type=0;type<28;++type) assert(etrian::art_for_map_icon(type)==expected[type]);
 assert(etrian::art_for_map_icon(-1)==0 && etrian::art_for_map_icon(28)==0);
 Fake f; EdenDsmodHostApi h{};h.userdata=&f;h.read_romfs=source;
 etrian::IconPixels pixels{};
 assert(etrian::load_game_icon(h,1,pixels));
 assert(pixels[0]==1 && pixels[3]==255);
 assert(etrian::load_game_icon(h,19,pixels));
 f.shift=0x30; assert(etrian::load_game_icon(h,1,pixels));
 assert(etrian::load_game_icon(h,19,pixels));
 assert(etrian::load_game_icon(h,33,pixels)); assert(pixels[0]==77 && pixels[3]==255);
 assert(pixels[(9*54+17)*4]==123 && pixels[(9*54+17)*4+3]==255);
 f.valid=false; assert(!etrian::load_game_icon(h,1,pixels));
 assert(!etrian::load_game_icon(h,0,pixels));
 h.read_romfs=nullptr; assert(!etrian::load_game_icon(h,1,pixels));
}
