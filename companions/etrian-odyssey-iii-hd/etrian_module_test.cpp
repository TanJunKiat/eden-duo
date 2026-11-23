#include "010084A0176AA000.cpp"
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
 if(off!=0xf6178 || n!=11768)return 0;
 auto* b=static_cast<uint8_t*>(out); std::memset(b,0,n);
 b[0]=10; std::memcpy(b+4,"icon_map01",10); b[24]=54; b[28]=54; b[40]=4; b[100]=0x90; b[101]=0x2d;
 for(size_t i=104;i<n;i+=4) { b[i]=255; b[i+3]=255; }
 return n;
}
void sink(void* p,uint32_t w,uint32_t h,const uint8_t* pixels,size_t n) { assert(w==592&&h==528); static_cast<Fake*>(p)->pixels.assign(pixels,pixels+n); }
int main() {
 Fake f;
 assert(Title==UINT64_C(0x010084A0176AA000));
 assert(supports("3D0322EDA242DEC85321974C126F43AC00000000000000000000000000000000"));
 assert(!supports("4CBA9B7CE88603D196F4C63639586B8F00000000000000000000000000000000"));
 // Eight enemies above eight allies, with three columns and a centered last row.
 State cards; cards.mode=etrian::BattleMode::battle;
 for(int i=0;i<8;++i) { etrian::Combatant c; c.name="TEST"; c.hp=10; c.max_hp=20; cards.battle.allies.push_back(c); cards.battle.enemies.push_back(c); }
 assert(dashboard_image(cards,&f,sink));
 auto rgb=[&](int x,int y) { auto i=(y*592+x)*4; return (f.pixels[i]<<16)|(f.pixels[i+1]<<8)|f.pixels[i+2]; };
 // First and last rows on both sides; the partial row is centered, not clipped.
 for(int y : {155,334}) for(int x : {35,215,395}) assert(rgb(x,y)==0x121f3a);
 for(int y : {245,454}) for(int x : {125,305}) assert(rgb(x,y)==0x121f3a);
 // In a two-enemy lineup, changing the first name only changes the left card.
 cards.battle.enemies.resize(2); cards.battle.enemies[0].name="A"; cards.battle.enemies[1].name="B";
 assert(dashboard_image(cards,&f,sink)); const auto before=f.pixels;
 cards.battle.enemies[0].name="Z"; assert(dashboard_image(cards,&f,sink));
 bool changed_left=false;
 for(int y=132;y<254;++y) for(int x=120;x<472;++x) for(int c=0;c<3;++c) {
  auto i=(y*592+x)*4+c;
  if(x<296) changed_left |= before[i]!=f.pixels[i];
  else assert(before[i]==f.pixels[i]);
 }
 assert(changed_left);
 // The fifth town member stays visible and KO has its own border and label.
 cards.mode=etrian::BattleMode::town; cards.battle.allies.resize(5);
 cards.battle.allies.back().hp=0; assert(dashboard_image(cards,&f,sink));
 assert(rgb(100,361)==0xff9eab); // fifth card border
 assert(rgb(100,367)==0x121f3a); // fifth card body
 assert(rgb(428,367)==0x090f21); // no invented sixth card
 EdenDsmodHostApi host{}; host.title_id=Title; host.main_base=0x100000; host.userdata=&f; host.is_mapped=mapped; host.read_memory=memory; host.publish_i64=integer; host.publish_f64=number; host.publish_text=text;
 store(f,host.main_base+MethodSlot,uint64_t(0x200000)); store(f,0x200018,uint64_t(0x300000)); store(f,0x3000c0,uint64_t(0x400000)); store(f,0x400000,uint64_t(0x300000)); store(f,0x3000b8,uint64_t(0x500000)); store(f,0x500000,uint64_t(0x600000));
 store(f,0x6000dc,int32_t(1)); store(f,0x600070,int32_t(6)); store(f,0x600074,int32_t(10)); store(f,0x600080,int32_t(0)); store(f,0x6000e8,uint64_t(0x700000)); store(f,0x700018,int32_t(30));
 store(f,host.main_base+0x618e708,uint64_t(0xb00000)); store(f,0xb00058,uint64_t(0xb10000)); store(f,0xb100b8,uint64_t(0xb20000)); store(f,0xb20000,uint64_t(0xb30000)); store(f,0xb30044,int32_t(4));
 store(f,0x700040,uint64_t(0x800000)); store(f,0x700028,uint64_t(0xa00000)); store(f,0x800018,int32_t(2100)); store(f,0x800010,uint64_t(0x900000)); store(f,0x900018,int32_t(2100)); store(f,0xa00018,int32_t(1050));
 for(int i=0;i<1050;++i) { store(f,0x900020+i*4,uint32_t(i==321?46:0)); store(f,0xa00020+i*4,int32_t(i==321?2:-1)); }
 void* state=create(&host,nullptr); assert(state); sample(state,&host); assert(f.ints["map.ready"]==1); assert(f.ints["map.floor"]==1); assert(!f.key.empty()); assert(image(state,&host,f.key.c_str(),&f,sink));
 auto pixel=[&](int x,int y,int c){return f.pixels[(y*592+x)*4+c];};
 assert(pixel(68+6*13+7,107+20*13+7,0)>180); // marked stair
 assert(pixel(68+6*13+6,107+19*13+4,1)>180); // north-facing player
 assert(rgb(68+6*13+12,107+20*13+3)==0x85bce3); // recorded right wall
 host.read_romfs=asset; assert(image(state,&host,f.key.c_str(),&f,sink));
 assert(pixel(68+6*13+7,107+20*13+7,0)>180); // unsupported artwork keeps the fallback
 assert(!action(state,"zoom_in",0) && !action(state,"zoom_out",0) && !action(state,"overview",0));
 assert(!f.floats.count("map.zoom") && !f.floats.count("map.cx"));
 assert(rgb(68+34*13+7,107+29*13+7)==0x0b1429); // last column and row remain visible
 assert(rgb(68,107)==0x314762); // sector grid origin
 auto old_key=f.key; store(f,0x600080,int32_t(1)); sample(state,&host); assert(f.key!=old_key); // turning redraws
 // Entering town replaces the map. The same stats reuse the image; a rest updates it.
 const auto map_key=f.key;
 store(f,0xb30044,int32_t(3));
 store(f,host.main_base+0x639a810,uint64_t(0xc00000)); store(f,0xc00000,uint64_t(0xc10000));
 store(f,0xc100b8,uint64_t(0xc20000)); store(f,0xc20000,uint64_t(0xc30000));
 store(f,0xc30010,uint64_t(0xc40000)); store(f,0xc40020,uint64_t(0xc50000));
 store(f,0xc50028,uint64_t(0xc60000)); store(f,0xc60018,int32_t(1));
 store(f,0xc60020,uint64_t(0xc70000)); store(f,0xc70050,int16_t(30)); store(f,0xc70052,int16_t(12));
 store(f,0xc7007a,uint8_t(0)); store(f,0xc70058,uint64_t(0xc80000));
 store(f,0xc80010,int32_t(1)); store(f,0xc80014,uint16_t('A'));
 sample(state,&host); const auto town_key=f.key;
 assert(f.ints["map.ready"]==1 && town_key!=map_key);
 assert(!image(state,&host,map_key.c_str(),&f,sink));
 assert(image(state,&host,town_key.c_str(),&f,sink)); const auto town_pixels=f.pixels;
 sample(state,&host); assert(f.key==town_key);
 store(f,0xc70050,int16_t(44)); store(f,0xc70052,int16_t(18)); sample(state,&host);
 assert(f.key!=town_key && image(state,&host,f.key.c_str(),&f,sink) && f.pixels!=town_pixels);
 const auto rested_key=f.key;
 f.mem.erase(0xc70052); sample(state,&host);
 assert(f.key!=rested_key && !image(state,&host,rested_key.c_str(),&f,sink));
 assert(image(state,&host,f.key.c_str(),&f,sink)); const auto town_unavailable=f.pixels;
 store(f,0xc60018,int32_t(0)); sample(state,&host);
 assert(image(state,&host,f.key.c_str(),&f,sink) && f.pixels!=town_unavailable);
 store(f,0xb30044,int32_t(6)); sample(state,&host);
 assert(image(state,&host,f.key.c_str(),&f,sink) && f.pixels!=town_unavailable);
 store(f,0xb30044,int32_t(4)); sample(state,&host);
 assert(f.ints["map.ready"]==1 && image(state,&host,f.key.c_str(),&f,sink));
 // A battle scene without a readable roster must replace the map with a loading image.
 store(f,0xb30044,int32_t(5));
 old_key=f.key; sample(state,&host); assert(f.ints["map.ready"]==1 && f.key!=old_key); assert(image(state,&host,f.key.c_str(),&f,sink));
 store(f,0xb30044,int32_t(4)); sample(state,&host); assert(f.ints["map.ready"]==1 && f.key!=old_key);
 store(f,0x6000dc,int32_t(0)); sample(state,&host); assert(f.ints["map.ready"]==0);
 destroy(state);
}
