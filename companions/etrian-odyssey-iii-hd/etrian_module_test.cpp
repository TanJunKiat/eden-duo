#undef NDEBUG
#include <cassert>
#include "010084A0176AA000.cpp"
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
 cards.mode=etrian::BattleMode::town; cards.town.party=std::vector<etrian::Combatant>(5);
 cards.town.formation=std::vector<uint8_t>{0,1,2,3,5};
 for(auto& m:*cards.town.party) { m.name="TEST"; m.hp=20; m.tp=10; }
 cards.town.party->back().hp=0; assert(dashboard_image(cards,&f,sink));
 assert(rgb(480,238)==0xff9eab); // fifth card border, Back slot 3
 assert(rgb(480,244)==0x121f3a); // fifth card body
 assert(rgb(295,244)!=0x121f3a); // empty Back slot 2 is not a card
 const auto formation_pixels=f.pixels;
 cards.town.formation.reset(); assert(dashboard_image(cards,&f,sink) && f.pixels!=formation_pixels);
 EdenDsmodHostApi host{}; host.title_id=Title; host.main_base=0x100000; host.userdata=&f; host.is_mapped=mapped; host.read_memory=memory; host.publish_i64=integer; host.publish_f64=number; host.publish_text=text;
 store(f,host.main_base+MethodSlot,uint64_t(0x200000)); store(f,0x200018,uint64_t(0x300000)); store(f,0x3000c0,uint64_t(0x400000)); store(f,0x400000,uint64_t(0x300000)); store(f,0x3000b8,uint64_t(0x500000)); store(f,0x500000,uint64_t(0x600000));
 store(f,0x6000dc,int32_t(1)); store(f,0x600070,int32_t(6)); store(f,0x600074,int32_t(10)); store(f,0x600080,int32_t(0)); store(f,0x6000e8,uint64_t(0x700000)); store(f,0x700018,int32_t(30));
 store(f,host.main_base+0x618e708,uint64_t(0xb00000)); store(f,0xb00058,uint64_t(0xb10000)); store(f,0xb100b8,uint64_t(0xb20000)); store(f,0xb20000,uint64_t(0xb30000)); store(f,0xb30044,int32_t(4));
 store(f,0x700040,uint64_t(0x800000)); store(f,0x700028,uint64_t(0xa00000)); store(f,0x800018,int32_t(2100)); store(f,0x800010,uint64_t(0x900000)); store(f,0x900018,int32_t(2100)); store(f,0xa00018,int32_t(1050));
 for(int i=0;i<1050;++i) { store(f,0x900020+i*4,uint32_t(i==321?46:0)); store(f,0xa00020+i*4,int32_t(i==321?2:-1)); }
 void* state=create(&host,nullptr); assert(state); sample(state,&host); assert(f.ints["map.ready"]==1); assert(f.ints["map.floor"]==1); assert(!f.key.empty()); assert(image(state,&host,f.key.c_str(),&f,sink));
 api.tick(state,&host);
 assert(!f.ints.contains("map.native_editor")); // runtime 25 package has no native map relay
 auto pixel=[&](int x,int y,int c){return f.pixels[(y*592+x)*4+c];};
 assert(pixel(68+6*13+7,107+20*13+7,0)>180); // marked stair
 // Check the rendered arrow tip for each raw HD heading, before loading artwork.
 constexpr std::array<std::array<int,2>,4> tips{{{0,-4},{0,4},{4,0},{-4,0}}};
 for(int32_t raw:{0,1,2,3,0}) {
  store(f,0x600080,raw); sample(state,&host);
  assert(image(state,&host,f.key.c_str(),&f,sink));
  const auto [dx,dy]=tips[raw];
  const int cx=68+6*13+6,cy=107+19*13+6;
  assert(rgb(cx+dx,cy+dy)==etrian::glass::hp_color);
  assert(rgb(cx-dx,cy-dy)!=etrian::glass::hp_color);
 }
 assert(rgb(68+6*13+12,107+20*13+3)==0x85bce3); // recorded right wall
 host.read_romfs=asset; assert(image(state,&host,f.key.c_str(),&f,sink));
 assert(pixel(68+6*13+7,107+20*13+7,0)>180); // unsupported artwork keeps the fallback
 assert(!action(state,"zoom_in",0) && !action(state,"zoom_out",0) && !action(state,"overview",0));
 assert(!f.floats.count("map.zoom") && !f.floats.count("map.cx"));
 assert(rgb(68+34*13+7,107+29*13+7)==0x0b1429); // last column and row remain visible
 assert(rgb(68,107)==0x314762); // sector grid origin
 // DungeonCtrl holds the loaded HD map north-first, unlike the saved tile array.
 store(f,host.main_base+0x61a6040,uint64_t(0xd00000)); store(f,0xd00018,uint64_t(0xd10000));
 store(f,0xd100c0,uint64_t(0xd20000)); store(f,0xd20000,uint64_t(0xd30000));
 store(f,0xd300b8,uint64_t(0xd40000)); store(f,0xd40000,uint64_t(0xd50000));
 store(f,0xd50078,uint64_t(0xd60000)); store(f,0xd60010,uint64_t(0xd70000));
 store(f,0xd70010,int32_t(0)); store(f,0xd70014,uint32_t(0x10010));
 store(f,0xd70018,int32_t(35)); store(f,0xd7001c,int32_t(30));
 store(f,0xd70140,uint64_t(0xd80000)); store(f,0xd80010,uint64_t(0xd90000));
 store(f,0xd80018,int32_t(1050)); store(f,0xd90018,uint64_t(1050));
 store(f,host.main_base+0x61a6018,uint64_t(0xf00000)); store(f,0xf00018,uint64_t(0xf10000));
 store(f,0xf100c0,uint64_t(0xf20000)); store(f,0xf20000,uint64_t(0xf30000));
 store(f,0xf300b8,uint64_t(0xf40000)); store(f,0xf40000,uint64_t(0xf50000));
 store(f,0xf50018,uint64_t(0xf60000)); store(f,0xf60010,uint64_t(0xf70000));
 store(f,0xf60018,int32_t(26)); store(f,0xf70018,uint64_t(26));
 const auto floor_list=[](int floor) -> uint64_t { return floor==1?0xd80000:0xfa0000+floor*0x100; };
 for(int floor=1;floor<=26;++floor) {
  const uint64_t entry=0x700020+(floor-1)*0x28, asset=0xf80000+floor*0x100;
  store(f,entry+0x20,uint64_t(0x800000)); store(f,entry+0x08,uint64_t(0xa00000));
  store(f,0xf70020+(floor-1)*8,asset); store(f,asset+0x20,floor_list(floor));
  if(floor>1) { store(f,floor_list(floor)+0x10,uint64_t(0xdb0000)); store(f,floor_list(floor)+0x18,int32_t(1050)); }
 }
 store(f,0xdb0018,uint64_t(1050));
 for(int i=0;i<1050;++i) for(int j=0;j<12;++j) {
  store(f,0xd90020+i*12+j,uint8_t(j==0?(i==671 || i==672 || i==706?1:14):0));
  // The middle square is a special warp: no outline at that square, even on deeper floors.
  store(f,0xdb0020+i*12+j,uint8_t(j==0?(i==671 || i==673?1:i==672?24:14):0));
 }
 for(const auto i : {10+25*35,23+14*35,33+19*35}) store(f,0xd90020+i*12,uint8_t(18));
 const auto chip=[&](int x,int sy,uint8_t type) { store(f,0xd90020+12*(x+35*sy),type); };
 chip(8,18,19); chip(9,18,21); chip(10,18,17); chip(11,18,1); chip(12,18,1);
 chip(13,18,1); chip(14,18,1); chip(15,18,30);
 store(f,0xd70150,int32_t(1)); store(f,0xd70158,uint64_t(0xdc0000));
 store(f,0xdc0010,uint64_t(0xdc0100)); store(f,0xdc0018,int32_t(1));
 store(f,0xdc0118,uint64_t(1)); store(f,0xdc0120,std::array<uint8_t,3>{2,22,14});
 store(f,host.main_base+0x61a60f0,uint64_t(0x1100000)); store(f,0x1100018,uint64_t(0x1101000));
 store(f,0x11010c0,uint64_t(0x1102000)); store(f,0x1102000,uint64_t(0x1103000));
 store(f,0x11030b8,uint64_t(0x1104000)); store(f,0x1104000,uint64_t(0x1105000));
 store(f,0x11050a0,uint64_t(0x1106000)); store(f,0x1106010,uint64_t(0x1107000));
 store(f,0x1107018,uint64_t(57));
 for(int i=0;i<57;++i) store(f,0x1107020+i*24,std::array<uint8_t,24>{});
 store(f,0x1107020,std::array<uint8_t,4>{1,11,18,2}); // Take (north-first)
 store(f,0x1107038,std::array<uint8_t,4>{2,8,19,0}); // Harvest on B2F only
 store(f,0x1107050,std::array<uint8_t,4>{1,14,18,3}); // unknown gathering kind
 store(f,0x1105110,uint64_t(0x1108000)); store(f,0x1108018,uint64_t(27));
 store(f,0x1108028,uint64_t(0x1109000)); store(f,0x1109010,uint64_t(0x110a000));
 store(f,0x110a018,uint64_t(4));
 for(int i=0;i<4;++i) store(f,0x110a020+i*48,std::array<int8_t,48>{});
 store(f,0x110a020,std::array<int8_t,6>{0,43,0,0,12,18}); // floor-specific event
 store(f,0x110a050,std::array<int8_t,6>{1,44,0,0,13,18}); // wrong place for this slot
 store(f,0x110a080,std::array<int8_t,6>{2,43,0,0,-1,18}); // no exact coordinate
 store(f,0x110a0b0,std::array<int8_t,6>{3,43,0,0,35,18}); // off grid
 sample(state,&host); assert(f.ints["map.exploring"]==1 && f.ints["map.hints_available"]==1);
 const auto normal_key=f.key; assert(image(state,&host,f.key.c_str(),&f,sink)); const auto normal_pixels=f.pixels;
 const auto hint_at=[&](int x,int y) { return static_cast<State*>(state)->reference.markers[etrian::index(x,y)]; };
 assert(hint_at(8,18)==etrian::HintIcon::none);
 assert(action(state,"hints",0)); sample(state,&host); assert(f.key!=normal_key);
 using etrian::HintIcon;
 assert(hint_at(8,18)==HintIcon::treasure && hint_at(9,18)==HintIcon::shortcut && hint_at(10,18)==HintIcon::save);
 assert(hint_at(11,18)==HintIcon::take && hint_at(12,18)==HintIcon::event);
 for(int x:{13,14,15}) assert(hint_at(x,18)==HintIcon::none);
 assert(hint_at(12,11)==HintIcon::none); // must not reflect the event vertically
 assert(hint_at(23,14)==HintIcon::stairs_down);
 store(f,0x110a018,uint64_t(99999)); sample(state,&host);
 assert(hint_at(12,18)==HintIcon::none && hint_at(11,18)==HintIcon::take && f.ints["map.hints_available"]==1);
 store(f,0x110a018,uint64_t(4)); f.mem.erase(0x110a0b0); sample(state,&host);
 assert(hint_at(12,18)==HintIcon::none && hint_at(11,18)==HintIcon::take);
 store(f,0x110a0b0,uint8_t(3)); sample(state,&host);
 assert(hint_at(12,18)==HintIcon::event);
 assert(!image(state,&host,normal_key.c_str(),&f,sink)); assert(image(state,&host,f.key.c_str(),&f,sink));
 assert(rgb(68+7*13+7,107+19*13+7)==0x192b40); // unrecorded hint, northeast of the saved stair
 assert(rgb(68+6*13+12,107+20*13+3)==0x85bce3); // live walls still win
 const uint64_t chest_tile=etrian::index(8,etrian::screen_y(18));
 const auto chest_pixels=[&] {
  std::vector<uint8_t> result;
  for(int row=107+18*13;row<107+19*13;++row) {
   const auto p=f.pixels.begin()+(row*592+68+8*13)*4;
   result.insert(result.end(),p,p+13*4);
  }
  return result;
 };
 const auto chest_hint=chest_pixels();
 store(f,0x900020+chest_tile*4,uint32_t(6)); sample(state,&host);
 assert(image(state,&host,f.key.c_str(),&f,sink));
 assert(rgb(68+8*13+12,107+18*13+6)==0x85bce3 && rgb(68+8*13+6,107+18*13+12)==0x85bce3);
 store(f,0x900020+chest_tile*4,uint32_t(0));
 chip(8,18,1); sample(state,&host); assert(image(state,&host,f.key.c_str(),&f,sink));
 assert(chest_pixels()!=chest_hint); // icon, not just the open geometry
 for(bool recorded_marker:{false,true}) {
  if(recorded_marker) store(f,0xa00020+chest_tile*4,int32_t(2));
  else store(f,0x900020+chest_tile*4,uint32_t(8));
  sample(state,&host); assert(image(state,&host,f.key.c_str(),&f,sink)); const auto recorded=chest_pixels();
  chip(8,18,19); sample(state,&host); assert(image(state,&host,f.key.c_str(),&f,sink));
  assert(chest_pixels()==recorded); // never add a marker on a recorded tile or over a user icon
  store(f,0x900020+chest_tile*4,uint32_t(0)); store(f,0xa00020+chest_tile*4,int32_t(-1)); chip(8,18,1);
 }
 store(f,0x600070,int32_t(8)); store(f,0x600074,int32_t(11));
 sample(state,&host); assert(image(state,&host,f.key.c_str(),&f,sink)); const auto player_tile=chest_pixels();
 chip(8,18,19); sample(state,&host); assert(image(state,&host,f.key.c_str(),&f,sink));
 assert(chest_pixels()==player_tile);
 store(f,0x600070,int32_t(6)); store(f,0x600074,int32_t(10));
 sample(state,&host);
 assert(action(state,"hints",0)); sample(state,&host); assert(image(state,&host,f.key.c_str(),&f,sink));
 assert(f.pixels==normal_pixels);
 assert(action(state,"hints",0)); sample(state,&host);
 assert(action(state,"guide",0)); sample(state,&host);
 assert(f.ints["map.guide"]==1 && f.ints["map.hints_available"]==0);
 assert(image(state,&host,f.key.c_str(),&f,sink) && f.pixels!=normal_pixels);
 assert(!action(state,"hints",0));
 assert(!action(state,"monster",0)); // no published roster
 auto& guide_state=*static_cast<State*>(state);
 guide_state.monsters=std::vector<etrian::FloorMonster>{{37,"Test Monster"}};
 assert(!action(state,"monster",-1) && !action(state,"monster",1));
 for(uint16_t id=38;id<=49;++id) guide_state.monsters->push_back({id,"More Monsters"});
 const auto roster_key=f.key;
 assert(action(state,"guide_next",0) && guide_state.guide_page==1);
 assert(!action(state,"monster",0)); // next page has not been drawn
 guide_state.redraw=false; // retain the injected roster for this rendering-only fixture
 assert(image(state,&host,roster_key.c_str(),&f,sink));
 assert(!action(state,"monster",1));
 assert(action(state,"monster",0) && guide_state.selected_monster==49);
 assert(!image(state,&host,roster_key.c_str(),&f,sink));
 assert(!action(state,"map",0) && action(state,"roster",0) && !guide_state.selected_monster);
 assert(action(state,"map",0)); sample(state,&host);
 assert(image(state,&host,f.key.c_str(),&f,sink));
 // While crossing stairs, the HUD floor can change before geometry does.
 store(f,0x6000dc,int32_t(2)); sample(state,&host);
 assert(f.ints["map.hints_available"]==0 && !action(state,"hints",0));
 assert(hint_at(8,18)==HintIcon::none && hint_at(12,18)==HintIcon::none);
 assert(image(state,&host,f.key.c_str(),&f,sink)); assert(rgb(68+7*13+7,107+19*13+7)==0x0b1429);
 for(int floor=2;floor<=25;++floor) {
  store(f,0x6000dc,int32_t(floor)); store(f,0xd70140,floor_list(floor));
  sample(state,&host);
  assert(f.ints["map.floor"]==floor && f.ints["map.hints_available"]==1);
  assert(hint_at(8,19)==(floor==2?HintIcon::harvest:HintIcon::none));
  assert(hint_at(12,18)==HintIcon::none);
  if(floor==2) {
   store(f,0x11050a8,uint64_t(0x110b000)); store(f,0x110b010,uint64_t(0x110c000));
   store(f,0x11050b0,uint64_t(0x110d000)); store(f,0x110d010,uint64_t(0x110e000));
   store(f,0x110c018,uint64_t(1)); store(f,0x110e018,uint64_t(1));
   assert(action(state,"guide",0)); sample(state,&host);
   assert(guide_state.monsters && guide_state.monsters->empty()); // B2F guide read, not unavailable
   assert(guide_state.guide_page==0 && !action(state,"monster",0));
   assert(image(state,&host,f.key.c_str(),&f,sink));
   assert(action(state,"map",0)); sample(state,&host);
  }
 }
 assert(image(state,&host,f.key.c_str(),&f,sink));
 assert(hint_at(7,19)==HintIcon::warp); // verified door warp gets a marker, not a route
 assert(rgb(68+8*13+7,107+19*13+7)==0x192b40); // B25F layout, same north-first coordinates
 assert(action(state,"hints",0)); sample(state,&host);
 assert(image(state,&host,f.key.c_str(),&f,sink) && rgb(68+8*13+7,107+19*13+7)==0x0b1429);
 assert(action(state,"hints",0)); sample(state,&host);
 assert(action(state,"guide",0)); sample(state,&host);
 assert(f.ints["map.monster_count"]==0 && action(state,"map",0)); sample(state,&host);
 assert(image(state,&host,f.key.c_str(),&f,sink) && rgb(68+8*13+7,107+19*13+7)==0x192b40);
 const auto fail_closed=[&] {
  sample(state,&host); assert(f.ints["map.ready"]==1 && f.ints["map.hints_available"]==0);
  assert(!action(state,"hints",0) && image(state,&host,f.key.c_str(),&f,sink));
 };
 store(f,0xd70010,int32_t(1)); fail_closed(); store(f,0xd70010,int32_t(0)); // not labyrinth mode
 store(f,0xd7001c,int32_t(60)); fail_closed(); store(f,0xd7001c,int32_t(30));
 store(f,0x600070,int32_t(1)); fail_closed(); store(f,0x600070,int32_t(6)); // player inside a wall in the reference
 store(f,0x6000dc,int32_t(26)); store(f,0xd70140,floor_list(26)); fail_closed(); // never hint sea entry
 store(f,0x6000dc,int32_t(1)); store(f,0xd70140,floor_list(2)); fail_closed();
 auto old_key=f.key; store(f,0x600080,int32_t(1)); sample(state,&host); assert(f.key!=old_key); // turning redraws
 // A missing journal is not an empty journal; opening it must not shrink the map.
 assert(action(state,"quests",0)); sample(state,&host);
 assert(f.ints["quest.open"]==1 && f.ints["quest.count"]==0);
 assert(image(state,&host,f.key.c_str(),&f,sink));
 const auto unavailable=f.pixels;
 assert(!action(state,"quest",0));
 // Installed-build-shaped definition and event flag arrays.
 uint64_t address=host.main_base;
 for (const auto off : {0x61a60f0,0x18,0xc0,0,0xb8,0}) {
  const auto next=0xe00000+(address==host.main_base?0:address-0xe00000+0x1000);
  store(f,address+off,next); address=next;
 }
 for (int type=0;type<2;++type) {
  const uint64_t table=0xe10000+type*0x1000, records=table+0x100;
  store(f,address+0xc0+type*8,table); store(f,table+0x10,records);
  const int n=type==0?52:13;
  store(f,records+0x18,uint64_t(n));
  for(int i=0;i<n;++i) for(int j=0;j<8;++j)
   store(f,records+0x20+16*i+2*j,uint16_t(j>=3 && j<=5?64+type*256+3*i+j-3:0));
 }
 store(f,host.main_base+0x639a810,uint64_t(0xc00000)); store(f,0xc00000,uint64_t(0xc10000));
 store(f,0xc100b8,uint64_t(0xc20000)); store(f,0xc20000,uint64_t(0xc30000));
 store(f,0xc30010,uint64_t(0xc40000)); store(f,0xc40048,uint64_t(0xef0000));
 store(f,0xef0018,uint64_t(2048));
 for(int i=0;i<2048;++i) store(f,0xef0020+i,uint8_t(0));
 sample(state,&host); assert(f.ints["quest.count"]==0);
 assert(image(state,&host,f.key.c_str(),&f,sink) && f.pixels!=unavailable);
 // Mission 0 accepted; quest 1 ready to report even when its acceptance bit is off.
 store(f,0xef0020+40,uint8_t(1)); store(f,0xef0020+8,uint8_t(16));
 sample(state,&host); assert(f.ints["quest.count"]==2);
 auto& qs=*static_cast<State*>(state);
 assert(qs.journal.entries[0].key==etrian::QuestKey(true,0));
 assert(qs.journal.entries[1].status==etrian::QuestStatus::report);
 assert(!qs.journal.entries[1].items[0].in_bag); // no inventory is not zero items
 // Carry slots, not report flags, drive the independently labeled bag total.
 store(f,0xc40020,uint64_t(0xc50000)); store(f,0xc50058,uint64_t(0xee0000));
 store(f,0xee0018,uint64_t(60));
 for(int i=0;i<60;++i) {
  const uint64_t slot=0xee1000+0x40*i;
  store(f,0xee0020+8*i,slot); store(f,slot+0x10,int16_t(i<2?928:0));
 }
 store(f,0xef0020+8,uint8_t(8)); sample(state,&host);
 assert(qs.journal.entries[1].status==etrian::QuestStatus::active);
 assert(qs.journal.entries[1].items[0].in_bag==2 && qs.journal.entries[1].items[1].in_bag==0);
 assert(!action(state,"quest",1)); // the newly changed rows have not been drawn
 assert(image(state,&host,f.key.c_str(),&f,sink));
 assert(!action(state,"quest",-1) && !action(state,"quest",2));
 assert(action(state,"quest",1)); sample(state,&host); assert(f.ints["quest.detail"]==1);
 // Completion takes priority over report; a stale selection never opens a different quest.
 store(f,0xef0020+8,uint8_t(48)); sample(state,&host);
 assert(f.ints["quest.detail"]==0 && f.ints["quest.count"]==1);
 assert(image(state,&host,f.key.c_str(),&f,sink));
 assert(action(state,"quest",0)); sample(state,&host);
 store(f,0xef0018,uint64_t(65535)); sample(state,&host);
 assert(f.ints["quest.detail"]==0 && f.ints["quest.count"]==0);
 const auto map_key=f.key; // Leaving exploration also dismisses Quests.
 assert(!action(state,"expedition",0)); // not from a different subpanel
 assert(action(state,"map",0)); sample(state,&host);
 assert(action(state,"expedition",0)); sample(state,&host);
 assert(f.ints["expedition.open"]==1 && f.ints["map.hints_available"]==0);
 assert(qs.expedition.bag_free==58 && qs.expedition.threads==0 && !qs.expedition.party);
 store(f,0xee1010,int16_t(1355)); sample(state,&host);
 assert(qs.expedition.threads==1 && qs.expedition.bag_free==58);
 f.mem.erase(0xee1010); sample(state,&host);
 assert(!qs.expedition.threads && !qs.expedition.bag_free);
 store(f,0xee1010,int16_t(1355));
 assert(image(state,&host,f.key.c_str(),&f,sink));
 // Entering town replaces the map. The same stats reuse the image; a rest updates it.
 constexpr uint64_t manager=0x84001000;
 store(f,host.main_base+0x61a6258,manager-0x1000);
 store(f,manager-0x1000+0x18,manager+0x1000);
 store(f,manager+0x1000+0xc0,manager+0x2000);
 store(f,manager+0x2000,manager+0x1000);
 store(f,manager+0x1000+0xb8,manager+0x3000);
 store(f,manager+0x3000,manager);
 store(f,manager+0x18,manager+0x4000);
 store(f,manager+0x20,int32_t(1));
 store(f,0xb30044,int32_t(3));
 store(f,host.main_base+0x639a810,uint64_t(0xc00000)); store(f,0xc00000,uint64_t(0xc10000));
 store(f,0xc100b8,uint64_t(0xc20000)); store(f,0xc20000,uint64_t(0xc30000));
 store(f,0xc30010,uint64_t(0xc40000)); store(f,0xc40020,uint64_t(0xc50000));
 store(f,0xc50028,uint64_t(0xc60000)); store(f,0xc60018,int32_t(1));
 store(f,0xc60020,uint64_t(0xc70000)); store(f,0xc70050,int16_t(30)); store(f,0xc70052,int16_t(12));
 store(f,0xc7007a,uint8_t(0)); store(f,0xc70058,uint64_t(0xc80000));
 store(f,0xc80010,int32_t(1)); store(f,0xc80014,uint16_t('A'));
 const auto town_reader=[&](uint64_t a,void* out,size_t n) { return memory(&f,a,out,n); };
 store(f,0xc60018,int32_t(2));
 store(f,0xc60028,uint64_t(0xc90000)); store(f,0xc90050,int16_t(17)); store(f,0xc90052,int16_t(4));
 store(f,0xc90058,uint64_t(0xc80000)); store(f,0xc9007a,uint8_t(1));
 store(f,0xc7007a,uint8_t(4));
 store(f,0xef0018,uint64_t(2048)); store(f,0xef0020+8,uint8_t(50)); // quest 0 ready; quest 1 complete
 for(int i=0;i<42;++i) store(f,0xee1010+0x40*i,int16_t(i<2?1355:928));
 auto town=etrian::read_town(town_reader,host.main_base);
 assert(town.source==0xc40000 && town.formation && (*town.formation==std::vector<uint8_t>{1,4}));
 assert(town.party->at(0).hp==17 && town.party->at(1).hp==30);
 store(f,0xc70040,uint64_t(0xc75000)); store(f,0xc75010,int32_t(50)); store(f,0xc75014,int32_t(24));
 store(f,0xc70048,uint64_t(0xc76000)); store(f,0xc76010,int32_t(0)); store(f,0xc76014,int32_t(0));
 store(f,0xc70050,int16_t(44)); store(f,0xc70052,int16_t(18));
 town=etrian::read_town(town_reader,host.main_base);
 assert(town.party->at(1).max_hp==50 && town.party->at(1).max_tp==24);
 f.mem.erase(0xc75014); town=etrian::read_town(town_reader,host.main_base);
 assert(town.party->at(1).max_hp==50 && town.party->at(1).tp==18 && town.party->at(1).max_tp==-1);
 store(f,0xc75010,int32_t(43)); store(f,0xc75014,int32_t(0)); town=etrian::read_town(town_reader,host.main_base);
 assert(town.party->at(1).hp==44 && town.party->at(1).max_hp==-1 && town.party->at(1).max_tp==-1);
 store(f,0xc70050,int16_t(30)); store(f,0xc70052,int16_t(12));
 assert(town.bag_free==18 && town.threads==2);
 assert(town.quests.report && town.quests.report->size()==1 && town.quests.report->front().key==etrian::QuestKey(false,0));
 // Bistro's posted-state gate is separate from acceptance and completion.
 for(int i=0;i<52;++i) for(int j=0;j<3;++j) store(f,0xe10120+16*i+2*j,uint16_t(j==0?4096+i:65535));
 store(f,0xef0020+512,uint8_t(7)); // posted quests 0,1,2; first ready and second complete
 town=etrian::read_town(town_reader,host.main_base);
 assert(town.quests.available && town.quests.available->size()==1 && town.quests.available->front().key==etrian::QuestKey(false,2));
 assert(town.quests.accepted && town.quests.accepted->size()==1 && town.quests.accepted->front().key==etrian::QuestKey(true,0));
 store(f,0xe10120+2*16+12,uint16_t(2)); // only posted in the other town
 assert(etrian::read_town(town_reader,host.main_base).quests.available->empty());
 store(f,0xe10120+2*16+12,uint16_t(1));
 store(f,0xe10120+2*16+2,uint16_t(4160)); // second prerequisite not yet met
 assert(etrian::read_town(town_reader,host.main_base).quests.available->empty());
 store(f,0xef0020+520,uint8_t(1));
 assert(etrian::read_town(town_reader,host.main_base).quests.available->size()==1);
 store(f,0xef0020+8,uint8_t(50|64)); // take quest 2 (bit 70)
 town=etrian::read_town(town_reader,host.main_base);
 assert(town.quests.available && town.quests.available->empty() && town.quests.accepted->size()==2);
 store(f,0xef0020+8,uint8_t(50|64|128)); // ready, bit 71
 town=etrian::read_town(town_reader,host.main_base);
 assert(town.quests.accepted->size()==1 && town.quests.report->size()==2);
 store(f,0xef0020+9,uint8_t(1)); // turned in, bit 72
 town=etrian::read_town(town_reader,host.main_base);
 assert(town.quests.report->size()==1 && town.quests.available->empty());
 f.mem.erase(0xef0020+512); town=etrian::read_town(town_reader,host.main_base);
 assert(!town.quests.available && town.quests.accepted->size()==1 && town.quests.report->size()==1);
 store(f,0xef0020+512,uint8_t(0)); store(f,0xef0020+8,uint8_t(50)); store(f,0xef0020+9,uint8_t(0));
 store(f,0xc7007a,uint8_t(0)); town=etrian::read_town(town_reader,host.main_base);
 assert(town.formation && (*town.formation==std::vector<uint8_t>{0,1}) && town.party->front().hp==30);
 store(f,0xc7007a,uint8_t(2)); town=etrian::read_town(town_reader,host.main_base);
 assert(town.formation && (*town.formation==std::vector<uint8_t>{1,2}) && town.party->front().hp==17);
 store(f,0xc7007a,uint8_t(4));
 store(f,0xc9007a,uint8_t(4)); town=etrian::read_town(town_reader,host.main_base);
 assert(!town.formation && town.party->size()==2 && town.threads==2);
 f.mem.erase(0xc9007a); town=etrian::read_town(town_reader,host.main_base);
 assert(!town.formation && town.party->size()==2);
 store(f,0xc9007a,uint8_t(255)); assert(!etrian::read_town(town_reader,host.main_base).formation);
 store(f,0xc9007a,uint8_t(1)); f.mem.erase(0xee1010);
 town=etrian::read_town(town_reader,host.main_base);
 assert(!town.threads && !town.bag_free && town.formation && town.quests.report);
 for(int i=0;i<60;++i) store(f,0xee1010+0x40*i,int16_t(0));
 town=etrian::read_town(town_reader,host.main_base);
 assert(town.threads==0 && town.bag_free==60);
 store(f,0xef0018,uint64_t(32768)); town=etrian::read_town(town_reader,host.main_base);
 assert(!town.quests.report && town.threads==0 && town.formation);
 store(f,0xef0018,uint64_t(2048)); store(f,0xef0020+8,uint8_t(48));
 town=etrian::read_town(town_reader,host.main_base);
 assert(town.quests.report && town.quests.report->empty());
 auto switch_save=[&](uint64_t a,void* out,size_t n) {
  const bool ok=town_reader(a,out,n);
  if(a==0xee1010) store(f,0xc30010,uint64_t(0xc41000));
  return ok;
 };
 assert(!etrian::read_town(switch_save,host.main_base).source);
 store(f,0xc30010,uint64_t(0xc40000));
 store(f,0xc60018,int32_t(1)); store(f,0xc7007a,uint8_t(0));
 sample(state,&host); const auto town_key=f.key;
 assert(f.ints["map.ready"]==1 && town_key!=map_key);
 assert(f.ints["map.exploring"]==0 && !action(state,"guide",0) && !action(state,"hints",0));
 assert(f.ints["quest.open"]==0 && !action(state,"quests",0) && !action(state,"quest",0));
 assert(f.ints["expedition.open"]==0 && !action(state,"expedition",0) && !qs.expedition.threads);
 assert(!image(state,&host,map_key.c_str(),&f,sink));
 assert(image(state,&host,town_key.c_str(),&f,sink)); const auto town_pixels=f.pixels;
 sample(state,&host); assert(f.key==town_key);
 assert(f.ints["town.board_open"]==1 && action(state,"town_board",0)); sample(state,&host);
 assert(qs.town_tab==etrian::TownQuestTab::available && image(state,&host,f.key.c_str(),&f,sink));
 const auto empty_offers=f.pixels;
 f.mem.erase(0xef0020+512); sample(state,&host);
 assert(image(state,&host,f.key.c_str(),&f,sink) && f.pixels!=empty_offers);
 assert(action(state,"town_accepted",0)); sample(state,&host);
 assert(qs.town_tab==etrian::TownQuestTab::accepted && image(state,&host,f.key.c_str(),&f,sink));
 store(f,0xef0020+512,uint8_t(0)); sample(state,&host);
 assert(image(state,&host,f.key.c_str(),&f,sink)); assert(action(state,"town_back",0)); sample(state,&host);
 assert(image(state,&host,f.key.c_str(),&f,sink) && f.pixels==town_pixels);
 store(f,0xef0020+8,uint8_t(2)); sample(state,&host);
 assert(image(state,&host,f.key.c_str(),&f,sink) && action(state,"town_board",0)); sample(state,&host);
 assert(qs.town_tab==etrian::TownQuestTab::report && image(state,&host,f.key.c_str(),&f,sink));
 assert(action(state,"town_back",0)); sample(state,&host);
 for(unsigned bit:{65u,68u,71u,74u,77u,80u,83u,86u}) f.mem[0xef0020+bit/8]|=uint8_t(1u<<(bit%8));
 sample(state,&host); assert(!action(state,"town_board",0));
 assert(image(state,&host,f.key.c_str(),&f,sink)); const auto checklist=f.pixels;
 assert(action(state,"town_board",0)); sample(state,&host);
 assert(f.ints["town.board"]==1 && f.ints["town.board_next"]==1 && !action(state,"town_board_next",0));
 assert(image(state,&host,f.key.c_str(),&f,sink) && f.pixels!=checklist);
 assert(action(state,"town_board_next",0)); sample(state,&host);
 assert(f.ints["town.board_prev"]==1 && f.ints["town.board_next"]==0);
 assert(image(state,&host,f.key.c_str(),&f,sink));
 assert(action(state,"town_back",0)); sample(state,&host);
 assert(image(state,&host,f.key.c_str(),&f,sink) && f.pixels==checklist);
 assert(action(state,"town_board",0)); sample(state,&host);
 assert(image(state,&host,f.key.c_str(),&f,sink) && action(state,"town_board_next",0)); sample(state,&host);
 store(f,0xef0020+9,uint8_t(0)); sample(state,&host); // selected page no longer exists
 assert(f.ints["town.board"]==1 && qs.town_page==0 && !action(state,"town_board_next",0));
 store(f,0xc30010,uint64_t(0xc41000)); sample(state,&host);
 assert(f.ints["town.board"]==0 && !action(state,"town_board",0) && !action(state,"town_board_next",0));
 store(f,0xc30010,uint64_t(0xc40000)); sample(state,&host);
 assert(!action(state,"town_board",0));
 assert(image(state,&host,f.key.c_str(),&f,sink));
 assert(action(state,"town_board",0)); sample(state,&host);
 store(f,0xb30044,int32_t(4)); sample(state,&host);
 assert(f.ints["town.board"]==0 && !action(state,"town_board_next",0));
 store(f,0xb30044,int32_t(3)); sample(state,&host);
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
 store(f,0xc60018,int32_t(1)); store(f,0xc70050,int16_t(0)); store(f,0xc70052,int16_t(4));
 sample(state,&host);
 assert(qs.expedition.party && qs.expedition.party->size()==1);
 assert(qs.expedition.party->front().hp==0 && qs.expedition.party->front().tp==4);
 // A battle scene without a readable roster must replace the guide with a loading image.
 assert(action(state,"guide",0)); sample(state,&host); assert(f.ints["map.guide"]==1);
 store(f,0xb30044,int32_t(5));
 old_key=f.key; sample(state,&host); assert(f.ints["map.ready"]==1 && f.key!=old_key); assert(image(state,&host,f.key.c_str(),&f,sink));
 store(f,0xb30044,int32_t(4)); sample(state,&host);
 assert(f.ints["map.ready"]==1 && f.ints["map.guide"]==0 && f.key!=old_key);
 store(f,0x6000dc,int32_t(0)); sample(state,&host); assert(f.ints["map.ready"]==0);
 // Walking from a verified entrance cannot cross hidden/special links.
 etrian::FloorReference area_reference{};
 area_reference.ready=true; area_reference.primary_stair=etrian::index(3,3);
 area_reference.screen[etrian::index(3,3)].chip=18;
 area_reference.screen[etrian::index(3,2)].chip=1;
 area_reference.screen[etrian::index(3,1)].chip=20;
 area_reference.screen[etrian::index(3,0)].chip=1;
 area_reference.screen[etrian::index(4,2)].chip=23;
 area_reference.screen[etrian::index(5,2)].chip=1;
 const auto main_area=etrian::main_bounty_area(area_reference);
 assert(main_area[etrian::index(3,2)] && !main_area[etrian::index(3,0)] && !main_area[etrian::index(5,2)]);
 // Facility data is rejected outside town or while the destination changes.
 store(f,manager+0x20,int32_t(4));
 const auto facility_memory=[&](uint64_t a,void* out,size_t n) {return memory(&f,a,out,n)!=0;};
 assert(!etrian::read_town_facility(facility_memory,host.main_base));
 store(f,0xb30044,int32_t(3));
 assert(etrian::read_town_facility(facility_memory,host.main_base)==etrian::TownFacility::bistro);
 store(f,manager+0x20,int32_t(5));
 store(f,0xc50020,uint64_t(0xca0000)); store(f,0xca0018,uint64_t(3));
 store(f,0xca0020,uint64_t(0xc70000)); store(f,0xca0028,uint64_t(0)); store(f,0xca0030,uint64_t(0xc90000));
 store(f,0xc70010,uint8_t(3)); store(f,0xc70013,uint8_t(12)); store(f,0xc70014,int32_t(0));
 store(f,0xc90010,uint8_t(1)); store(f,0xc90013,uint8_t(7)); store(f,0xc90014,int32_t(2));
 const auto members=etrian::read_guild_roster(facility_memory,host.main_base,0xc40000);
 assert(members && members->size()==2 && (*members)[0].active && !(*members)[1].active);
 assert((*members)[0].name=="A" && (*members)[0].level==12 && (*members)[1].job=="Hoplite");
 store(f,0xca0018,uint64_t(31)); assert(!etrian::read_guild_roster(facility_memory,host.main_base,0xc40000));
 store(f,0xca0018,uint64_t(3));
 assert(!etrian::read_armoroad_day(facility_memory,host.main_base,0xc40000));
 store(f,0xc5001c,int32_t(0));
 assert(etrian::read_armoroad_day(facility_memory,host.main_base,0xc40000)==0);
 store(f,0xc5001c,int32_t(7));
 assert(etrian::read_armoroad_day(facility_memory,host.main_base,0xc40000)==7);
 assert(!etrian::read_armoroad_day(facility_memory,host.main_base,0xc40008));
 store(f,0xc5001c,int32_t(-1));
 assert(!etrian::read_armoroad_day(facility_memory,host.main_base,0xc40000));
 store(f,manager+0x20,int32_t(9)); // alternate town not verified
 assert(!etrian::read_town_facility(facility_memory,host.main_base));
#ifdef ETRIAN_EXP_TRIAL
 struct Box {
     uint64_t command=0,weekly=0,contract=0,party=0; uint32_t words[1024]{};
     uint64_t* at(uint32_t o) {
         switch(o) {case 0:return &command; case 144:return &contract; case 152:return &party;
                    case 176:return &weekly; default:return nullptr;}
     }
 } mailbox;
 EdenDsmodHostExtensions services{};
 services.userdata=&mailbox; services.mailbox_address=1; services.mailbox_size=4096;
 services.load_u64=[](void* p,uint32_t o,uint64_t* v)->EdenDsmodBool {const auto src=static_cast<Box*>(p)->at(o);if(!src)return false; *v=*src;return true;};
 services.store_u64=[](void* p,uint32_t o,uint64_t v)->EdenDsmodBool {const auto dst=static_cast<Box*>(p)->at(o);if(!dst)return false; *dst=v;return true;};
 services.load_u32=[](void* p,uint32_t o,uint32_t* v)->EdenDsmodBool {*v=static_cast<Box*>(p)->words[o/4];return true;};
 services.store_u32=[](void* p,uint32_t o,uint32_t v)->EdenDsmodBool {static_cast<Box*>(p)->words[o/4]=v;return true;};
 host.get_i64=[](void*,const char*,int64_t)->int64_t {return 42;};
 configure(state,&services); sample(state,&host);
 assert(qs.exp_available);
 qs.exp_rate=5; qs.deep_selected=true; qs.rules={50,true,true};
 mailbox.command=etrian::exp_trial::command(qs.exp_rate,42,false,qs.rules);
 const auto selected=mailbox.command;
 store(f,0xb30044,int32_t(6)); sample(state,&host); // transient scene during inn fade
 assert(!qs.exp_available && qs.exp_rate==5 && qs.deep_selected && mailbox.command==selected);
 store(f,0xb30044,int32_t(3)); sample(state,&host);
 assert(qs.exp_available && qs.exp_rate==5 && qs.rules.training==50 && qs.rules.rotation && qs.rules.favor);
 store(f,0xb30044,int32_t(2)); sample(state,&host);
 assert(!qs.exp_available && !qs.exp_rate && !qs.deep_selected && !qs.rules.training && !mailbox.command);
#ifdef ETRIAN_ARMOROAD_ROOMS
 store(f,0xb30044,int32_t(3)); store(f,manager+0x20,int32_t(1)); sample(state,&host);
 assert(f.ints["town.settings_open"] && !f.ints["town.exp_controls"]);
 assert(image(state,&host,f.key.c_str(),&f,sink) && !action(state,"exp_rate",5));
 assert(action(state,"town_settings",0) && !action(state,"exp_rate",5));
 sample(state,&host);
 assert(f.ints["town.settings_back"] && f.ints["town.exp_controls"] && !f.ints["town.board_open"]);
 store(f,0xc5001c,int32_t(13));
 qs.weekly.observe(1,{924,925,926,927}); sample(state,&host);
 assert(qs.weekly.week==1 && !mailbox.weekly && f.ints["town.weekly_toggle"]);
 assert(image(state,&host,f.key.c_str(),&f,sink) && !action(state,"merchant_favor",0));
 assert(action(state,"toggle_weekly",0) && mailbox.weekly==qs.weekly.command(42));
 sample(state,&host);
 assert(image(state,&host,f.key.c_str(),&f,sink) && action(state,"exp_rate",5));
 store(f,manager+0x20,int32_t(3)); sample(state,&host);
 assert(!f.ints["town.settings_open"] && !f.ints["town.exp_controls"] && !qs.town_settings && qs.exp_rate==5);
 const auto quote_word=mailbox.weekly;
 qs.weekly.observe(2,{928,929,930,931});
 store(f,0xc5001c,int32_t(14)); sample(state,&host);
 assert(mailbox.weekly==quote_word && qs.weekly.floor==1 && qs.weekly_armed);
 store(f,manager+0x20,int32_t(1)); sample(state,&host);
 assert(qs.weekly.floor==2 && qs.weekly.week==2 && mailbox.weekly!=quote_word);
 // A successful inn save replaces the live save and party objects without a load or epoch change.
 store(f,manager+0x20,int32_t(2)); sample(state,&host);
 qs.bounty_enabled=true; qs.bounty_offers[0].species=24;
 qs.rules={50,true,false}; qs.deep_selected=true;
 mailbox.contract=(uint64_t(42)<<32)|(3u<<16)|24; mailbox.party=0xc50000;
 sample(state,&host);
 const auto expected=mailbox.contract;
 const auto weekly_before_save=mailbox.weekly;
 store(f,manager+0x20,int32_t(0)); sample(state,&host); // facility unreadable during the save transition
 store(f,0xcf0020,uint64_t(0xd00000)); store(f,0xd0001c,int32_t(14));
 store(f,0xc30010,uint64_t(0xcf0000)); sample(state,&host);
 assert(!qs.exp_available && qs.exp_rate==5 && qs.exp_save==0xc40000);
 store(f,manager+0x20,int32_t(2)); sample(state,&host);
 assert(qs.exp_rate==5 && qs.deep_selected && qs.rules.training==50 && qs.rules.rotation);
 assert(qs.weekly_enabled && mailbox.weekly==weekly_before_save);
 assert(qs.bounty_enabled && qs.bounty_offers[0].species==24 && mailbox.contract==expected && mailbox.party==0xd00000);
 store(f,0xb30044,int32_t(2)); sample(state,&host);
 assert(!qs.weekly_enabled && !mailbox.weekly && !qs.weekly.deepest);
 assert(!qs.exp_rate && !qs.bounty_enabled && !mailbox.contract);
 store(f,0xb30044,int32_t(3)); sample(state,&host); // loaded same slot
 assert(qs.exp_available && !qs.exp_rate && !qs.weekly_enabled && !qs.bounty_enabled);
#endif
#endif
 destroy(state);
}
