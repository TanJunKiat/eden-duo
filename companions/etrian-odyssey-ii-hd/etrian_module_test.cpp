#include "0100B810176A8000.cpp"
#include <cassert>
#include <map>
struct Fake { std::map<uint64_t,uint8_t> mem; std::map<std::string,int64_t> ints; std::map<std::string,double> floats; std::string key; std::vector<uint8_t> pixels; };
template<class T> void store(Fake& f,uint64_t a,T v) { auto p=reinterpret_cast<const uint8_t*>(&v); for(size_t i=0;i<sizeof(v);++i)f.mem[a+i]=p[i]; }
EdenDsmodBool mapped(void* p,uint64_t a,uint64_t n) { auto& m=static_cast<Fake*>(p)->mem; for(uint64_t i=0;i<n;++i)if(!m.count(a+i))return false; return true; }
EdenDsmodBool memory(void* p,uint64_t a,void* out,size_t n) { if(!mapped(p,a,n))return false; auto& m=static_cast<Fake*>(p)->mem; for(size_t i=0;i<n;++i)static_cast<uint8_t*>(out)[i]=m[a+i]; return true; }
void integer(void* p,const char* name,int64_t v) { static_cast<Fake*>(p)->ints[name]=v; }
void number(void* p,const char* name,double v) { static_cast<Fake*>(p)->floats[name]=v; }
void text(void* p,const char*,const char* v) { static_cast<Fake*>(p)->key=v; }
size_t asset(void*,const char*,uint64_t off,void* out,size_t n) {
 if(off!=0xf7018 || n!=11768)return 0;
 auto* b=static_cast<uint8_t*>(out); std::memset(b,0,n);
 b[0]=10; std::memcpy(b+4,"icon_map01",10); b[24]=54; b[28]=54; b[40]=4; b[100]=0x90; b[101]=0x2d;
 for(size_t i=104;i<n;i+=4) { b[i]=255; b[i+3]=255; }
 return n;
}
void sink(void* p,uint32_t w,uint32_t h,const uint8_t* pixels,size_t n) { assert(w==592&&h==528); static_cast<Fake*>(p)->pixels.assign(pixels,pixels+n); }
int main() {
 Fake f;
 assert(Title==UINT64_C(0x0100B810176A8000));
 assert(supports("8883310FF46D85D17C09882B36A8205D00000000000000000000000000000000"));
 assert(!supports("4CBA9B7CE88603D196F4C63639586B8F00000000000000000000000000000000"));
 // Eight enemies above eight allies, with three columns and a centered last row.
 State cards; cards.mode=etrian::BattleMode::battle;
 for(int i=0;i<8;++i) { etrian::Combatant c; c.name="TEST"; c.hp=10; c.max_hp=20; c.war_cry=3; cards.battle.allies.push_back(c); cards.battle.enemies.push_back(c); }
 assert(battle_image(cards,&f,sink));
 auto cardpixel=[&](int x,int y,int channel){return f.pixels[(y*592+x)*4+channel];};
 for(int y : {82,328}) for(int x : {180,374,568}) assert(cardpixel(x,y,1)==106);
 for(int y : {218,464}) for(int x : {277,471}) assert(cardpixel(x,y,1)==106);
 // A two-member row is centered and keeps vector order from left to right.
 cards.battle.enemies.resize(2); cards.battle.enemies[0].name="A"; cards.battle.enemies[1].name="B";
 assert(battle_image(cards,&f,sink)); auto before=f.pixels;
 std::swap(cards.battle.enemies[0],cards.battle.enemies[1]); assert(battle_image(cards,&f,sink));
 bool different=false;
 for(int y=61;y<71;++y) for(int x=0;x<6;++x) for(int channel=0;channel<3;++channel) {
  auto left=(y*592+112+x)*4+channel, right=(y*592+306+x)*4+channel;
  different |= before[left]!=f.pixels[left];
  assert(before[left]==f.pixels[right] && before[right]==f.pixels[left]);
 }
 assert(different);
 EdenDsmodHostApi host{}; host.title_id=Title; host.main_base=0x100000; host.userdata=&f; host.is_mapped=mapped; host.read_memory=memory; host.publish_i64=integer; host.publish_f64=number; host.publish_text=text;
 store(f,host.main_base+MethodSlot,uint64_t(0x200000)); store(f,0x200018,uint64_t(0x300000)); store(f,0x3000c0,uint64_t(0x400000)); store(f,0x400000,uint64_t(0x300000)); store(f,0x3000b8,uint64_t(0x500000)); store(f,0x500000,uint64_t(0x600000));
 store(f,0x6000d0,int32_t(1)); store(f,0x600068,int32_t(6)); store(f,0x60006c,int32_t(10)); store(f,0x600078,int32_t(0)); store(f,0x6000d8,uint64_t(0x700000)); store(f,0x700018,int32_t(30));
 store(f,0x700040,uint64_t(0x800000)); store(f,0x700028,uint64_t(0xa00000)); store(f,0x800018,int32_t(2100)); store(f,0x800010,uint64_t(0x900000)); store(f,0x900018,int32_t(2100)); store(f,0xa00018,int32_t(1050));
 for(int i=0;i<1050;++i) { store(f,0x900020+i*4,uint32_t(i==321?46:0)); store(f,0xa00020+i*4,int32_t(i==321?2:-1)); }
 void* state=create(&host,nullptr); assert(state); sample(state,&host); assert(f.ints["map.ready"]==1); assert(f.ints["map.floor"]==1); assert(!f.key.empty()); assert(image(state,&host,f.key.c_str(),&f,sink));
 auto pixel=[&](int x,int y,int c){return f.pixels[(y*592+x)*4+c];};
 assert(pixel(32+6*16+7,48+20*16+7,0)>180); // marked stair
 assert(pixel(32+6*16+8,48+19*16+4,1)>180); // north-facing player
 host.read_romfs=asset; assert(image(state,&host,f.key.c_str(),&f,sink));
 assert(pixel(32+6*16+7,48+20*16+7,0)==255 && pixel(32+6*16+7,48+20*16+7,1)==0); // game art replaces fallback
 assert(!action(state,"zoom_in",0) && !action(state,"zoom_out",0) && !action(state,"overview",0));
 assert(!f.floats.count("map.zoom") && !f.floats.count("map.cx"));
 assert(pixel(40,25,1)>150 && pixel(40,25,0)<30); // green coordinate tabs
 assert(pixel(5,55,1)>150 && pixel(5,55,0)<30); // green row tabs
 auto old_key=f.key; store(f,0x600078,int32_t(1)); sample(state,&host); assert(f.key!=old_key); // turning redraws
 // A battle scene without a readable roster must replace the map with a loading image.
 store(f,host.main_base+0x55c07f0,uint64_t(0xb00000)); store(f,0xb00058,uint64_t(0xb10000)); store(f,0xb100b8,uint64_t(0xb20000)); store(f,0xb20000,uint64_t(0xb30000)); store(f,0xb3001c,int32_t(2));
 old_key=f.key; sample(state,&host); assert(f.ints["map.ready"]==1 && f.key!=old_key); assert(image(state,&host,f.key.c_str(),&f,sink));
 store(f,0xb3001c,int32_t(4)); sample(state,&host); assert(f.ints["map.ready"]==1 && f.key!=old_key);
 store(f,0x6000d0,int32_t(0)); sample(state,&host); assert(f.ints["map.ready"]==0);
 destroy(state);
}
