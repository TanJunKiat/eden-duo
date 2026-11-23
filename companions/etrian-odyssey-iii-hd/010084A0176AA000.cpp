// SPDX-License-Identifier: GPL-3.0-or-later
// Experimental read-only Etrian Odyssey III HD companion.
#include "etrian_render.h"
#include "../../src/core/mods/dsmod_module_abi.h"
#include "../../src/core/mods/dsmod_module_extensions.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <new>
#include <string>

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
    etrian::FloorReference reference;
    std::vector<etrian::FloorMonster> monsters;
    uint16_t selected_monster=0;
    etrian::DropList drops;
    bool exploring=false, guide=false, hints=false, redraw=false, quests=false;
    etrian::QuestJournal journal;
    std::optional<etrian::QuestKey> selected_quest;
    size_t quest_page=0;
    bool quest_list_drawn=false;
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
void publish_quests(const EdenDsmodHostApi* h,const State& s) {
    const auto first=s.quest_page*etrian::quest_page_size;
    const bool list=s.quests && !s.selected_quest;
    h->publish_i64(h->userdata,"map.subpanel",s.quests || s.guide);
    h->publish_i64(h->userdata,"quest.open",s.quests);
    h->publish_i64(h->userdata,"quest.detail",s.selected_quest.has_value());
    h->publish_i64(h->userdata,"quest.count",list && first<s.journal.entries.size()?
        std::min<size_t>(etrian::quest_page_size,s.journal.entries.size()-first):0);
    h->publish_i64(h->userdata,"quest.prev",list && s.quest_page>0);
    h->publish_i64(h->userdata,"quest.next",list && first+etrian::quest_page_size<s.journal.entries.size());
}
void clear_quests(State& s) {
    s.quests=false; s.journal={}; s.selected_quest.reset(); s.quest_page=0; s.quest_list_drawn=false;
}
void sample(void* p,const EdenDsmodHostApi* h) {
    if (!p || !h) return;
    auto& s=*static_cast<State*>(p);
    etrian::BattleSnapshot battle;
    const auto reader=[&](uint64_t address,void* data,size_t size) {
        return address && h->is_mapped(h->userdata,address,size) && h->read_memory(h->userdata,address,data,size);
    };
    const auto mode=etrian::read_battle(reader,h->main_base,battle);
    {
        std::lock_guard<std::mutex> lock(s.mutex);
        if (mode!=etrian::BattleMode::map) {
            if (s.mode!=mode || !(s.battle==battle)) ++s.generation;
            s.mode=mode; s.battle=std::move(battle);
            s.exploring=s.guide=s.redraw=false;
            clear_quests(s); publish_quests(h,s);
            s.reference={}; s.monsters.clear(); s.selected_monster=0; s.drops.reset();
            h->publish_i64(h->userdata,"map.detail",0);
            h->publish_i64(h->userdata,"map.monster_count",0);
            h->publish_i64(h->userdata,"map.exploring",0);
            h->publish_i64(h->userdata,"map.guide",0);
            h->publish_i64(h->userdata,"map.hints_available",0);
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
    if (!ok) {
        std::lock_guard<std::mutex> lock(s.mutex);
        if(s.exploring) ++s.generation;
        s.exploring=s.guide=s.redraw=false;
        clear_quests(s); publish_quests(h,s);
        s.reference={}; s.monsters.clear(); s.selected_monster=0; s.drops.reset();
        h->publish_i64(h->userdata,"map.detail",0);
        h->publish_i64(h->userdata,"map.monster_count",0);
        h->publish_i64(h->userdata,"map.ready",0);
        h->publish_i64(h->userdata,"map.exploring",0);
        h->publish_i64(h->userdata,"map.guide",0);
        h->publish_i64(h->userdata,"map.hints_available",0);
        return;
    }
    auto reference=etrian::read_floor_reference(reader,h->main_base,floor,x,y);
    unsigned generation;
    bool guide, hints_available, detail;
    size_t monster_count;
    {
        std::lock_guard<std::mutex> lock(s.mutex);
        auto monsters=s.guide?etrian::read_floor_monsters(reader,h->main_base,reference):std::vector<etrian::FloorMonster>{};
        const auto selected=(s.floor==floor && s.selected_monster && std::ranges::any_of(monsters,[&](const auto& m) {
            return m.id==s.selected_monster;
        }))?s.selected_monster:0;
        auto drops=selected?etrian::read_monster_drops(reader,h->main_base,selected):etrian::DropList{};
        auto journal=s.quests?etrian::read_quest_journal(reader,h->main_base,floor):etrian::QuestJournal{};
        const bool new_context=s.floor!=floor || s.journal.source!=journal.source;
        const bool list_changed=s.journal.entries.size()!=journal.entries.size() ||
            !std::equal(s.journal.entries.begin(),s.journal.entries.end(),journal.entries.begin(),journal.entries.end(),
                [](const auto& a,const auto& b) { return a.key==b.key; });
        if(new_context || list_changed) s.quest_page=0;
        if(new_context || s.journal!=journal) s.quest_list_drawn=false;
        if(new_context || (s.selected_quest && std::ranges::find(journal.entries,*s.selected_quest,&etrian::QuestEntry::key)==journal.entries.end()))
            s.selected_quest.reset();
        if (!s.exploring || s.redraw || s.tiles!=tiles || s.icons!=icons || s.reference!=reference || s.monsters!=monsters ||
            s.journal!=journal ||
            s.x!=x || s.y!=y || s.floor!=floor || s.direction!=direction ||
            s.selected_monster!=selected || s.drops!=drops) {
            s.selected_monster=selected; s.drops=std::move(drops);
            s.tiles=tiles; s.icons=icons; s.x=x; s.y=y; s.floor=floor; s.direction=direction;
            s.reference=std::move(reference); s.monsters=std::move(monsters); s.journal=std::move(journal); ++s.generation;
        }
        s.exploring=true;
        s.redraw=false;
        generation=s.generation;
        guide=s.guide;
        detail=s.selected_monster!=0;
        monster_count=detail?0:s.monsters.size();
        hints_available=s.reference.ready && !guide && !s.quests;
        publish_quests(h,s);
    }
    const std::string key="module:etrian:map/"+std::to_string(generation);
    h->publish_text(h->userdata,"map.key",key.c_str());
    h->publish_i64(h->userdata,"map.floor",floor);
    h->publish_i64(h->userdata,"map.exploring",1);
    h->publish_i64(h->userdata,"map.guide",guide);
    h->publish_i64(h->userdata,"map.detail",detail);
    h->publish_i64(h->userdata,"map.monster_count",monster_count);
    h->publish_i64(h->userdata,"map.hints_available",hints_available);
    h->publish_i64(h->userdata,"map.ready",1);
}
void tick(void*,const EdenDsmodHostApi*) {}
void configure(void*,const EdenDsmodHostExtensions*) {}
EdenDsmodBool action(void* p,const char* name,int64_t argument) {
    if(!p || !name) return EDEN_DSMOD_FALSE;
    auto& s=*static_cast<State*>(p);
    std::lock_guard<std::mutex> lock(s.mutex);
    if(!s.exploring) return EDEN_DSMOD_FALSE;
    if(!std::strcmp(name,"hints") && s.reference.ready && !s.guide && !s.quests) s.hints=!s.hints;
    else if(!std::strcmp(name,"guide") && !s.guide && !s.quests) s.guide=true;
    else if(!std::strcmp(name,"map") && s.guide && !s.selected_monster) s.guide=false;
    else if(!std::strcmp(name,"map") && s.quests && !s.selected_quest) clear_quests(s);
    else if(!std::strcmp(name,"quests") && !s.quests && !s.guide) s.quests=true;
    else if(!std::strcmp(name,"quest_list") && s.quests && s.selected_quest) s.selected_quest.reset();
    else if(!std::strcmp(name,"quest_prev") && s.quest_list_drawn && !s.redraw && s.quest_page>0) --s.quest_page;
    else if(!std::strcmp(name,"quest_next") && s.quest_list_drawn && !s.redraw &&
            (s.quest_page+1)*etrian::quest_page_size<s.journal.entries.size()) ++s.quest_page;
    else if(!std::strcmp(name,"quest") && s.quest_list_drawn && !s.redraw &&
            argument>=0 && argument<etrian::quest_page_size &&
            s.quest_page*etrian::quest_page_size+argument<s.journal.entries.size())
        s.selected_quest=s.journal.entries[s.quest_page*etrian::quest_page_size+argument].key;
    else if(!std::strcmp(name,"roster") && s.guide && s.selected_monster) s.selected_monster=0;
    else if(!std::strcmp(name,"monster") && s.guide && !s.redraw && !s.selected_monster &&
            argument>=0 && uint64_t(argument)<s.monsters.size()) s.selected_monster=s.monsters[argument].id;
    else return EDEN_DSMOD_FALSE;
    s.redraw=true;
    s.quest_list_drawn=false;
    return EDEN_DSMOD_TRUE;
}
EdenDsmodBool dashboard_image(const State& state,void* receiver,EdenDsmodImageSink sink) {
    etrian::glass::Canvas canvas;
    if(state.mode==etrian::BattleMode::town || state.mode==etrian::BattleMode::town_loading)
        etrian::glass::town(canvas,state.mode,state.battle.allies);
    else etrian::glass::battle(canvas,state.mode,state.battle);
    sink(receiver,canvas.width,canvas.height,canvas.pixels.data(),canvas.pixels.size());
    return EDEN_DSMOD_TRUE;
}
EdenDsmodBool image(void* p,const EdenDsmodHostApi* host,const char* key,void* receiver,EdenDsmodImageSink sink) {
    if(!p || !key || !sink) return EDEN_DSMOD_FALSE;
    auto& s=*static_cast<State*>(p);
    std::lock_guard<std::mutex> lock(s.mutex);
    const std::string expected="module:etrian:map/"+std::to_string(s.generation);
    if (!s.generation || key!=expected) return EDEN_DSMOD_FALSE;
    if (s.mode!=etrian::BattleMode::map) return dashboard_image(s,receiver,sink);
    if (!s.exploring || s.redraw) return EDEN_DSMOD_FALSE;
    if (s.quests) {
        etrian::glass::Canvas canvas;
        const auto it=s.selected_quest?std::ranges::find(s.journal.entries,*s.selected_quest,&etrian::QuestEntry::key):s.journal.entries.end();
        if(it!=s.journal.entries.end()) etrian::glass::quest_details(canvas,s.floor,*it);
        else {
            etrian::glass::quest_list(canvas,s.floor,s.journal,s.quest_page);
            s.quest_list_drawn=true;
        }
        sink(receiver,canvas.width,canvas.height,canvas.pixels.data(),canvas.pixels.size());
        return EDEN_DSMOD_TRUE;
    }
    if (s.guide) {
        etrian::glass::Canvas canvas;
        const auto it=std::ranges::find(s.monsters,s.selected_monster,&etrian::FloorMonster::id);
        if(it!=s.monsters.end()) etrian::glass::monster_details(canvas,s.floor,*it,s.drops);
        else etrian::glass::floor_guide(canvas,s.floor,s.monsters);
        sink(receiver,canvas.width,canvas.height,canvas.pixels.data(),canvas.pixels.size());
        return EDEN_DSMOD_TRUE;
    }
    if (!s.artwork_loaded && host && host->read_romfs) {
        for (int n=1;n<=41;++n) s.artwork_ok[n]=etrian::load_game_icon(*host,n,s.artwork[n]);
        for (bool loaded : s.artwork_ok) s.artwork_loaded |= loaded;
    }
    etrian::glass::Canvas canvas;
    etrian::glass::map(canvas,s.tiles,s.icons,s.artwork,s.artwork_ok,s.x,s.y,s.floor,s.direction,&s.reference,s.hints);
    sink(receiver,canvas.width,canvas.height,canvas.pixels.data(),canvas.pixels.size());
    return EDEN_DSMOD_TRUE;
}
const EdenDsmodModuleApi api{EDEN_DSMOD_MODULE_ABI_VERSION,sizeof(EdenDsmodModuleApi),0,EDEN_DSMOD_MODULE_ABI_HASH,Title,"Etrian Odyssey III HD map",EDEN_DSMOD_CAP_EXTENSIONS,&supports,&create,&destroy,&sample,&tick};
const EdenDsmodModuleExtensions extensions{EDEN_DSMOD_EXT_VERSION,sizeof(EdenDsmodModuleExtensions),EDEN_DSMOD_EXT_HASH,&configure,&action,&image};
}
extern "C" __attribute__((visibility("default"))) const EdenDsmodModuleApi* eden_dsmod_get_module(uint32_t v,uint64_t h){return v==EDEN_DSMOD_MODULE_ABI_VERSION&&h==EDEN_DSMOD_MODULE_ABI_HASH?&api:nullptr;}
extern "C" __attribute__((visibility("default"))) const EdenDsmodModuleExtensions* eden_dsmod_get_extensions(uint32_t v,uint64_t h){return v==EDEN_DSMOD_EXT_VERSION&&h==EDEN_DSMOD_EXT_HASH?&extensions:nullptr;}
