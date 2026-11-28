// SPDX-License-Identifier: GPL-3.0-or-later
// Experimental read-only Etrian Odyssey III HD companion.
#include "etrian_render.h"
#include "etrian_town.h"
#ifdef ETRIAN_ARMOROAD_ROOMS
#include "etrian_weekly.h" // IWYU pragma: keep; room variant only
#endif
#ifdef ETRIAN_EXP_TRIAL
#include "etrian_exp_trial.h"
#endif
#ifdef ETRIAN_BISTRO_BOUNTY
#include "etrian_bistro_bounty.h"
#include <random>
#endif
#include "../../src/core/mods/dsmod_module_abi.h"
#include "../../src/core/mods/dsmod_module_extensions.h"
#include <array>
#ifdef ETRIAN_EXPEDITION_RULES
#include <chrono>
#endif
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
    etrian::TownSnapshot town;
    size_t town_page=0;
    etrian::TownQuestTab town_tab=etrian::TownQuestTab::available;
    bool town_board=false,town_drawn=false,town_redraw=false;
#ifdef ETRIAN_ARMOROAD_ROOMS
    bool town_settings=false;
    std::optional<etrian::TownFacility> facility,last_confirmed_facility;
    etrian::GuildRoster guild_roster;
    size_t guild_page=0;
    etrian::weekly::List weekly;
    std::optional<uint32_t> weekly_day;
    std::array<std::string,4> weekly_names{};
    std::array<int,4> weekly_prices{};
    bool weekly_enabled=false,weekly_clear=true,weekly_armed=false;
#endif
#ifdef ETRIAN_EXP_TRIAL
    EdenDsmodHostExtensions exp_box{};
    uint64_t exp_save=0;
    int64_t exp_epoch=-1;
    uint32_t exp_rate=0, deep_wins=0;
    etrian::exp_trial::Rules rules{};
    bool favor_ready=false;
    bool exp_available=false, deep_selected=false, deep_running=false;
#endif
#ifdef ETRIAN_BISTRO_BOUNTY
    uint64_t bounty_word=0, bounty_party=0, bounty_save=0;
    int64_t bounty_epoch=-1;
    uint32_t bounty_floors=0;
    bool bounty_available=false, bounty_clear=true;
    std::vector<etrian::FloorMonster> bounty_pool;
#ifdef ETRIAN_ARMOROAD_ROOMS
    bool bounty_enabled=false;
    etrian::bistro_bounty::Offers bounty_offers{};
#endif
#endif
    etrian::FloorReference reference;
    etrian::FloorMonsters monsters;
    size_t guide_page=0;
    bool guide_list_drawn=false;
    uint16_t selected_monster=0;
    etrian::DropList drops;
    bool exploring=false, guide=false, hints=false, redraw=false, quests=false, expedition_open=false;
    etrian::ExpeditionSnapshot expedition;
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
    h->publish_i64(h->userdata,"map.subpanel",s.quests || s.guide || s.expedition_open);
    h->publish_i64(h->userdata,"expedition.open",s.expedition_open);
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
void publish_guide(const EdenDsmodHostApi* h,const State& s) {
    const auto first=s.guide_page*etrian::guide_page_size;
    const bool list=s.guide && !s.selected_monster && s.monsters.has_value();
    h->publish_i64(h->userdata,"map.guide",s.guide);
    h->publish_i64(h->userdata,"map.detail",s.selected_monster!=0);
    h->publish_i64(h->userdata,"map.monster_count",list && first<s.monsters->size()?
        std::min(etrian::guide_page_size,s.monsters->size()-first):0);
    h->publish_i64(h->userdata,"map.guide_prev",list && s.guide_page>0);
    h->publish_i64(h->userdata,"map.guide_next",list && first+etrian::guide_page_size<s.monsters->size());
}
void clear_guide(State& s) {
    s.guide=false; s.monsters.reset(); s.guide_page=0; s.guide_list_drawn=false;
    s.selected_monster=0; s.drops.reset();
}
bool is_town(etrian::BattleMode mode) {
    return mode==etrian::BattleMode::town || mode==etrian::BattleMode::town_loading;
}
size_t town_quest_count(const etrian::TownSnapshot& town,etrian::TownQuestTab tab) {
    const auto& list=etrian::town_quest_list(town.quests,tab);
    return list?list->size():0;
}
bool town_next(const State& s) {
    return s.town_board && (s.town_page+1)*etrian::town_quest_page_size<town_quest_count(s.town,s.town_tab);
}
void publish_town(const EdenDsmodHostApi* h,const State& s) {
#ifdef ETRIAN_ARMOROAD_ROOMS
    h->publish_i64(h->userdata,"town.settings_open",s.town.source && !s.town_settings && !s.town_board &&
        s.facility==etrian::TownFacility::plaza);
    h->publish_i64(h->userdata,"town.settings_back",s.town_settings);
    h->publish_i64(h->userdata,"town.weekly_toggle",s.town_settings && s.exp_available && !s.weekly_clear && s.exp_epoch<=0xffffff);
    h->publish_i64(h->userdata,"town.board_open",s.town.source && !s.town_board && !s.town_settings &&
        s.facility==etrian::TownFacility::plaza);
    h->publish_i64(h->userdata,"town.bistro_board",s.town.source && !s.town_board && !s.town_settings &&
        s.facility==etrian::TownFacility::bistro);
    const bool guild=s.facility==etrian::TownFacility::guild && s.guild_roster && !s.town_settings && !s.town_board;
    h->publish_i64(h->userdata,"town.guild_prev",guild && s.guild_page>0);
    h->publish_i64(h->userdata,"town.guild_next",guild && (s.guild_page+1)*etrian::guild_page_size<s.guild_roster->size());
#else
    h->publish_i64(h->userdata,"town.board_open",s.town.source && !s.town_board);
#endif
    h->publish_i64(h->userdata,"town.board",s.town_board);
    h->publish_i64(h->userdata,"town.board_prev",s.town_board && s.town_page>0);
    h->publish_i64(h->userdata,"town.board_next",town_next(s));
#if defined(ETRIAN_BISTRO_BOUNTY) && defined(ETRIAN_ARMOROAD_ROOMS)
    using namespace etrian::bistro_bounty;
    const bool bistro=s.facility==etrian::TownFacility::bistro && s.town.source && !s.town_board && s.bounty_available && s.bounty_enabled;
    const bool idle=!s.bounty_word || (s.bounty_word&reported);
    for(size_t i=0;i<s.bounty_offers.size();++i) {
        const auto name="town.bistro_offer_"+std::to_string(i);
        const auto& offer=s.bounty_offers[i];
        h->publish_i64(h->userdata,name.c_str(),bistro && idle && offer.species && !offer.completed);
    }
    h->publish_i64(h->userdata,"town.bounty_toggle",s.town_settings && s.bounty_available && !(s.bounty_word&report_requested));
    h->publish_i64(h->userdata,"town.bistro_report",bistro && ready_to_request(s.bounty_word));
    h->publish_i64(h->userdata,"town.bistro_abandon",bistro && !idle && !(s.bounty_word&report_requested));
#elif defined(ETRIAN_BISTRO_BOUNTY)
    using namespace etrian::bistro_bounty;
    h->publish_i64(h->userdata,"town.bounty_action",s.town.source && !s.town_board && s.bounty_available &&
        (ready_to_request(s.bounty_word) || ((!s.bounty_word || (s.bounty_word&reported)) && !s.bounty_pool.empty())));
#endif
}
#ifdef ETRIAN_EXP_TRIAL
bool set_deep_run(State& s,bool active) {
    if(active==s.deep_running) return true;
    s.deep_running=false;
    s.deep_wins=0;
    const auto& box=s.exp_box;
    if(!s.exp_available) return false;
    using etrian::exp_trial::command;
    using etrian::exp_trial::win_offset;
    if(!active && !box.store_u64(box.userdata,0,command(s.exp_rate,s.exp_epoch,false,s.rules))) return false;
    if(!box.store_u32(box.userdata,win_offset,0)) return false;
    if(active && !box.store_u64(box.userdata,0,command(s.exp_rate,s.exp_epoch,true,s.rules))) return false;
    s.deep_running=active;
    return true;
}
#endif
#ifdef ETRIAN_ARMOROAD_ROOMS
template<class Reader>
bool sample_weekly(State& s,const Reader& memory,uint64_t base,uint64_t save,bool reset,bool box_ok,
                   etrian::BattleMode mode) {
    using namespace etrian::weekly;
    const bool was_armed=s.weekly_armed;
    const auto prior_day=s.weekly_day;
    const auto prior_prices=s.weekly_prices;
    const auto prior_names=s.weekly_names;
    if(reset) {
        s.weekly={}; s.weekly_names={}; s.weekly_prices={}; s.weekly_day={};
        s.weekly_enabled=false; s.weekly_clear=true;
    }
    const auto& box=s.exp_box;
    if(s.weekly_clear && box_ok) s.weekly_clear=!box.store_u64(box.userdata,mailbox_offset,0);
    const auto day=s.exp_available?etrian::read_armoroad_day(memory,base,save):std::nullopt;
    if(day) s.weekly_day=day;
    const bool shop=s.facility==etrian::TownFacility::shop;
    // Change schedules only in a confirmed non-shop facility or outside town.
    const bool can_update=(s.facility && !shop) || mode==etrian::BattleMode::map;
    const bool refreshed=day && can_update && s.weekly.refresh(*day);
    if(refreshed) {s.weekly_names={}; s.weekly_prices={};}
    s.weekly_armed=false;
    if(s.exp_available && !s.weekly_clear) {
        const bool expired=can_update && s.weekly_day && int(*s.weekly_day/7)>s.weekly.week;
        const auto word=s.weekly_enabled && !expired?s.weekly.command(s.exp_epoch):0;
        uint64_t actual=0;
        if(box.load_u64(box.userdata,mailbox_offset,&actual)) {
            const bool confirmed=actual==word || (can_update && box.store_u64(box.userdata,mailbox_offset,word));
            s.weekly_armed=word && confirmed;
        }
        if(shop && s.weekly_armed) {
            etrian::EnemyLabels labels(memory,base);
            for(size_t i=0;i<s.weekly.items.size();++i) {
                const uint16_t item=s.weekly.items[i];
                if(!item) continue;
                if(s.weekly_names[i].empty()) s.weekly_names[i]=labels.material(memory,item);
                uint64_t quote=0;
                if(box.load_u64(box.userdata,184+8*i,&quote) && quote>>32==item-923u && uint32_t(quote)<=INT32_MAX)
                    s.weekly_prices[i]=static_cast<int>(quote);
            }
        }
    }
    return reset || refreshed || was_armed!=s.weekly_armed || prior_day!=s.weekly_day ||
        prior_prices!=s.weekly_prices || prior_names!=s.weekly_names;
}

template<class Reader>
void observe_weekly(State& s,const Reader& mem,uint64_t base,int floor,const etrian::FloorMonsters& monsters,
                    const std::array<bool,etrian::cells>& area) {
    using etrian::quest_data::follow;
    using etrian::quest_data::read;
    auto items=etrian::read_gather_materials(mem,base,floor,area);
    if(!monsters || !items) return;
    for(const auto& monster:*monsters) {
        const auto drops=etrian::read_monster_drops(mem,base,monster.id);
        if(!drops) return;
        for(const auto& drop:*drops)
            if(drop.base_chance.value_or(0)>0) items->push_back(drop.item);
    }
    const auto master=follow(mem,base,{0x61a60f0,0x18,0xc0,0,0xb8,0,0x40,0x10});
    uint64_t count=0;
    if(!read(mem,master,0x18,count) || count<551 || count>4096) return;
    std::vector<uint16_t> materials;
    for(uint16_t id:*items) {
        if(!etrian::weekly::material_id(id)) continue;
        uint8_t subtype=0;
        // Native material predicate takes subtype 0 or 1 from USE_ITEM_DATA+5.
        if(!read(mem,master,0x25+16ull*(id-924),subtype)) return;
        if(subtype<=1) materials.push_back(id);
    }
    s.weekly.observe(floor,std::move(materials));
}
#endif

void sample(void* p,const EdenDsmodHostApi* h) {
    if (!p || !h) return;
    auto& s=*static_cast<State*>(p);
    etrian::BattleSnapshot battle;
    const auto reader=[&](uint64_t address,void* data,size_t size) {
        return address && h->is_mapped(h->userdata,address,size) && h->read_memory(h->userdata,address,data,size);
    };
    auto mode=etrian::read_battle(reader,h->main_base,battle);
    auto town=is_town(mode)?etrian::read_town(reader,h->main_base):etrian::TownSnapshot{};
    if(is_town(mode)) { mode=town.party?etrian::BattleMode::town:etrian::BattleMode::town_loading; battle={}; }
    {
        std::lock_guard<std::mutex> lock(s.mutex);
        bool town_changed=s.town!=town;
#ifdef ETRIAN_ARMOROAD_ROOMS
        const auto facility=town.source?etrian::read_town_facility(reader,h->main_base):std::nullopt;
        if(s.facility!=facility || s.town.source!=town.source) {
            town_changed=true;
            s.facility=facility;
            s.town_settings=false;
            s.town_board=false;
            s.guild_page=0;
        }
        auto roster=facility==etrian::TownFacility::guild?etrian::read_guild_roster(reader,h->main_base,town.source):etrian::GuildRoster{};
        if(s.guild_roster!=roster) {
            s.guild_roster=std::move(roster);
            if(!s.guild_roster || s.guild_page*etrian::guild_page_size>=s.guild_roster->size()) s.guild_page=0;
            town_changed=true;
        }
#endif
#ifdef ETRIAN_EXP_TRIAL
        const bool old_available=s.exp_available, old_deep=s.deep_selected;
        const auto old_rate=s.exp_rate;
        const bool old_favor_ready=s.favor_ready;
        const auto epoch=h->get_i64?h->get_i64(h->userdata,"__mailbox_epoch",-1):-1;
        auto save=mode==etrian::BattleMode::unknown?0:
            etrian::quest_data::follow(reader,h->main_base,{0x639a810,0,0xb8,0,0x10});
        const bool epoch_ok=epoch>=0 && epoch<=UINT32_MAX;
        int scene_kind=0;
        if(mode==etrian::BattleMode::unknown) {
            const auto scene=etrian::quest_data::follow(reader,h->main_base,{0x618e708,0x58,0xb8,0});
            if(scene) read(h,scene+0x44,scene_kind);
        }
        const bool title=scene_kind==2;
        bool continuous_inn=false;
#ifdef ETRIAN_ARMOROAD_ROOMS
        const bool was_inn=s.last_confirmed_facility==etrian::TownFacility::inn;
        // Saving at the inn swaps both native objects in-place in the town scene.
        // Wait through unreadable facility samples before accepting a new identity.
        if(was_inn && is_town(mode) && (!facility || town.source!=save || !epoch_ok)) save=0;
        continuous_inn=was_inn && facility==etrian::TownFacility::inn && save && town.source==save;
        if(title || (!is_town(mode) && mode!=etrian::BattleMode::unknown)) s.last_confirmed_facility.reset();
        else if(facility) s.last_confirmed_facility=facility;
#endif
        uint64_t actual=0;
        const auto& box=s.exp_box;
        const bool exp_box_ok=box.load_u64 && box.store_u64 && box.load_u32 && box.store_u32 &&
            box.mailbox_address && box.mailbox_size==4096;
        using etrian::exp_trial::RuleLifecycle;
        const auto lifecycle=etrian::exp_trial::rule_lifecycle(s.exp_save,s.exp_epoch,save,epoch,title,continuous_inn);
        if(lifecycle==RuleLifecycle::reset) {
            s.exp_rate=0;
            s.rules={};
            s.deep_selected=s.deep_running=false;
            s.deep_wins=0;
            if(exp_box_ok) box.store_u64(box.userdata,0,0);
            if(title) s.exp_save=0;
        }
        if(save) s.exp_save=save;
        if(epoch_ok) s.exp_epoch=epoch;
        s.exp_available=exp_box_ok && !title && save && epoch_ok && box.load_u64(box.userdata,0,&actual);
        if(s.exp_available) {
            const auto expected=etrian::exp_trial::command(s.exp_rate,epoch,s.deep_running,s.rules);
            if(actual!=expected) s.exp_available=box.store_u64(box.userdata,0,expected);
        }
#ifdef ETRIAN_BISTRO_BOUNTY
        using namespace etrian::bistro_bounty;
        const auto prior_word=s.bounty_word;
        const auto prior_available=s.bounty_available;
        uint64_t party=0, current_word=0;
        const bool has_party=save && ptr(h,save+0x20,party);
        const bool box_ok=box.load_u64 && box.store_u64 && box.mailbox_address && box.mailbox_size==4096;
        const bool identity_changed=(save && save!=s.bounty_save) || (has_party && party!=s.bounty_party);
        const bool rebind=continuous_inn && epoch_ok && s.bounty_epoch==epoch && s.bounty_save && s.bounty_party;
        // Keep the contract during an inn save, but only move its authorization
        // to a new party if the mailbox still belongs to the previous one.
        if(rebind && identity_changed && has_party && box_ok && !s.bounty_clear) {
            uint64_t old_party=0, word=0;
            if(box.load_u64(box.userdata,contract_offset,&word) && valid_word(word,epoch) &&
               (!word || (box.load_u64(box.userdata,party_offset,&old_party) && old_party==s.bounty_party &&
                          box.store_u64(box.userdata,party_offset,party)))) {
                s.bounty_save=save;
                s.bounty_party=party;
            }
        }
        const bool reset=title || (epoch_ok && s.bounty_epoch!=epoch) || (identity_changed && !rebind);
        if(reset) {
            s.bounty_pool.clear(); s.bounty_floors=0; s.bounty_clear=true;
#ifdef ETRIAN_ARMOROAD_ROOMS
            s.bounty_enabled=false;
            s.bounty_offers={};
#endif
            s.bounty_save=save; s.bounty_party=party;
            s.bounty_epoch=epoch_ok?epoch:-1;
        }
        if(s.bounty_clear && box_ok) s.bounty_clear=!box.store_u64(box.userdata,contract_offset,0);
        s.bounty_available=box_ok && !s.bounty_clear && has_party && epoch_ok &&
            s.bounty_save==save && s.bounty_party==party &&
            box.load_u64(box.userdata,contract_offset,&current_word) && valid_word(current_word,epoch);
        s.bounty_word=s.bounty_available?current_word:0;
#ifdef ETRIAN_ARMOROAD_ROOMS
        if(s.bounty_word&reported) {
            const auto it=std::ranges::find(s.bounty_offers,species_of(s.bounty_word),&Offer::species);
            if(it!=s.bounty_offers.end()) it->completed=true;
        }
        const auto filled=std::ranges::count_if(s.bounty_offers,[](const Offer& offer) {return offer.species!=0;});
        if(s.facility==etrian::TownFacility::bistro && s.bounty_enabled && s.bounty_available &&
           size_t(filled)<s.bounty_offers.size() && size_t(filled)<s.bounty_pool.size()) {
            std::vector<uint16_t> candidates;
            for(const auto& monster:s.bounty_pool) candidates.push_back(monster.id);
            const auto before=s.bounty_offers;
            try {
                std::random_device random;
                fill_offers(s.bounty_offers,std::move(candidates),[&](size_t n) {
                    return std::uniform_int_distribution<size_t>(0,n-1)(random);
                });
            } catch(...) { s.bounty_offers=before; }
            town_changed=town_changed || before!=s.bounty_offers;
        }
#endif
        town_changed=town_changed || prior_word!=s.bounty_word || prior_available!=s.bounty_available;
#endif
#ifdef ETRIAN_ARMOROAD_ROOMS
        town_changed=sample_weekly(s,reader,h->main_base,save,lifecycle==RuleLifecycle::reset,exp_box_ok,mode) || town_changed;
#endif
        if(is_town(mode) && s.exp_available && !set_deep_run(s,false)) s.exp_available=false;
        s.favor_ready=false;
#ifdef ETRIAN_EXPEDITION_RULES
        if(s.exp_available && s.rules.favor && is_town(mode)) {
            using namespace etrian::exp_trial;
            uint32_t pending=0, pending_epoch=0;
            s.favor_ready=box.load_u32(box.userdata,favor_pending_offset,&pending) && pending==1 &&
                box.load_u32(box.userdata,favor_epoch_offset,&pending_epoch) && pending_epoch==epoch;
        }
#endif
        town_changed=town_changed || old_available!=s.exp_available || old_rate!=s.exp_rate ||
            old_deep!=s.deep_selected || old_favor_ready!=s.favor_ready;
#endif
        if(s.town.source!=town.source || !town.source || !is_town(mode)) {
            s.town_board=false;
            s.town_tab=etrian::TownQuestTab::available;
        }
        if(!s.town_board || s.town_page*etrian::town_quest_page_size>=town_quest_count(town,s.town_tab))
            s.town_page=0;
        if(town_changed || s.mode!=mode) s.town_drawn=false;
        s.town=std::move(town);
        publish_town(h,s);
#ifdef ETRIAN_EXP_TRIAL
        h->publish_i64(h->userdata,"town.exp_controls",s.town.source && !s.town_board && s.exp_available
#ifdef ETRIAN_ARMOROAD_ROOMS
            && s.town_settings
#endif
        );
#endif
        if (mode!=etrian::BattleMode::map) {
            if (s.mode!=mode || s.battle!=battle || town_changed || s.town_redraw) ++s.generation;
            s.town_redraw=false;
            s.mode=mode; s.battle=std::move(battle);
            s.exploring=s.redraw=s.expedition_open=false; s.expedition={};
            clear_guide(s); publish_guide(h,s);
            clear_quests(s); publish_quests(h,s);
            s.reference={};
            h->publish_i64(h->userdata,"map.exploring",0);
            h->publish_i64(h->userdata,"map.hints_available",0);
            const std::string key="module:etrian:map/"+std::to_string(s.generation);
            h->publish_text(h->userdata,"map.key",key.c_str());
            h->publish_i64(h->userdata,"map.ready",1);
            return;
        }
        if (s.mode!=etrian::BattleMode::map) { s.mode=etrian::BattleMode::map; s.battle={}; ++s.generation; }
        s.town_redraw=false;
    }
    uint64_t a=0, manager=0, floors=0, list=0, array=0, icon_array=0;
    int32_t floor=0,x=0,y=0,game_direction=-1,count=0,icon_count=0;
    // HD reports N/S/E/W; the renderer rotates clockwise in N/E/S/W order.
    constexpr std::array<int,4> clockwise_heading{0,2,1,3};
    bool ok=ptr(h,h->main_base+MethodSlot,a) && ptr(h,a+0x18,a) && ptr(h,a+0xc0,a) &&
        ptr(h,a,a) && ptr(h,a+0xb8,a) && ptr(h,a,manager) &&
        read(h,manager+0xdc,floor) && floor>=1 && floor<=30 &&
        read(h,manager+0x70,x) && read(h,manager+0x74,y) &&
        read(h,manager+0x80,game_direction) && game_direction>=0 && game_direction<int(clockwise_heading.size()) &&
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
#ifdef ETRIAN_EXP_TRIAL
        if(!set_deep_run(s,false)) s.exp_available=false;
#endif
        s.exploring=s.redraw=s.expedition_open=false; s.expedition={};
        clear_guide(s); publish_guide(h,s);
        clear_quests(s); publish_quests(h,s);
        s.reference={};
        h->publish_i64(h->userdata,"map.ready",0);
        h->publish_i64(h->userdata,"map.exploring",0);
        h->publish_i64(h->userdata,"map.hints_available",0);
        return;
    }
    const int direction=clockwise_heading[game_direction];
    unsigned generation;
    bool hints_available;
    {
        std::lock_guard<std::mutex> lock(s.mutex);
        auto expedition=floor<=25?etrian::read_expedition(reader,h->main_base):etrian::ExpeditionSnapshot{};
        const bool save_changed=expedition.source && expedition.source!=s.expedition.source;
        const bool expedition_was_open=s.expedition_open;
        if(s.floor!=floor || save_changed) s.expedition_open=false;
        auto reference=etrian::read_floor_reference(reader,h->main_base,floor,x,y,s.hints && !s.guide && !s.quests && !s.expedition_open
#ifdef ETRIAN_ARMOROAD_ROOMS
            , true
#endif
        );
        int32_t floor_after=0;
        if(!read(h,manager+0xdc,floor_after) || floor_after!=floor) {reference={}; expedition={}; s.expedition_open=false;}
        auto monsters=s.guide?etrian::read_floor_monsters(reader,h->main_base,reference):etrian::FloorMonsters{};
#ifdef ETRIAN_BISTRO_BOUNTY
        if(s.bounty_available && expedition.source==s.exp_save && reference.ready && floor>=1 && floor<=25 &&
           !(s.bounty_floors&(1u<<(floor-1)))) {
#ifdef ETRIAN_ARMOROAD_ROOMS
            const auto area=etrian::main_bounty_area(reference);
            const auto observed=etrian::read_floor_monsters(reader,h->main_base,reference,&area);
#else
            const auto observed=s.guide?monsters:etrian::read_floor_monsters(reader,h->main_base,reference);
#endif
            if(observed && !observed->empty()) {
                for(const auto& enemy:*observed)
                    if(enemy.id && std::ranges::find(s.bounty_pool,enemy.id,&etrian::FloorMonster::id)==s.bounty_pool.end())
                        s.bounty_pool.push_back(enemy);
                s.bounty_floors|=1u<<(floor-1);
            }
        }
#endif
#ifdef ETRIAN_ARMOROAD_ROOMS
        if(s.exp_available && expedition.source==s.exp_save && floor_after==floor && floor>0 && floor<=25) {
            s.weekly.visit(floor);
            if(reference.ready && !s.weekly.pool_ready && unsigned(floor)==s.weekly.deepest) {
                const auto area=etrian::main_bounty_area(reference);
                observe_weekly(s,reader,h->main_base,floor,etrian::read_floor_monsters(reader,h->main_base,reference,&area),area);
            }
        }
#endif
        const bool guide_changed=s.floor!=floor || save_changed || s.monsters!=monsters;
        if(guide_changed) { s.guide_page=0; s.guide_list_drawn=false; }
#ifdef ETRIAN_EXP_TRIAL
        const bool prior_run=s.deep_running;
        const uint32_t prior_wins=s.deep_wins;
        const bool run=s.deep_selected && s.exp_available && expedition.source && expedition.source==s.exp_save;
        if(!set_deep_run(s,run)) s.exp_available=false;
        if(s.deep_running && (!s.exp_box.load_u32(s.exp_box.userdata,etrian::exp_trial::win_offset,&s.deep_wins) ||
           s.deep_wins>4)) {
            if(!set_deep_run(s,false)) s.exp_available=false;
        }
        if(prior_run!=s.deep_running || prior_wins!=s.deep_wins) s.redraw=true;
#endif
        const auto selected=guide_changed?uint16_t(0):s.selected_monster;
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
            s.journal!=journal || s.expedition!=expedition || expedition_was_open!=s.expedition_open ||
            s.x!=x || s.y!=y || s.floor!=floor || s.direction!=direction ||
            s.selected_monster!=selected || s.drops!=drops) {
            s.selected_monster=selected; s.drops=std::move(drops);
            s.tiles=tiles; s.icons=icons; s.x=x; s.y=y; s.floor=floor; s.direction=direction;
            s.reference=std::move(reference); s.monsters=std::move(monsters); s.journal=std::move(journal);
            s.expedition=std::move(expedition); ++s.generation;
        }
        s.exploring=true;
        s.redraw=false;
        generation=s.generation;
        hints_available=s.reference.ready && !s.guide && !s.quests && !s.expedition_open;
        publish_guide(h,s);
        publish_quests(h,s);
    }
    const std::string key="module:etrian:map/"+std::to_string(generation);
    h->publish_text(h->userdata,"map.key",key.c_str());
    h->publish_i64(h->userdata,"map.floor",floor);
    h->publish_i64(h->userdata,"map.exploring",1);
    h->publish_i64(h->userdata,"map.hints_available",hints_available);
    h->publish_i64(h->userdata,"map.ready",1);
}
// ABI 1 requires a tick callback even for a companion that only samples.
void tick(void*,const EdenDsmodHostApi*) {}
void configure(void* p,const EdenDsmodHostExtensions* host) {
#ifdef ETRIAN_EXP_TRIAL
    if(p && host) static_cast<State*>(p)->exp_box=*host;
#else
    (void)p; (void)host;
#endif
}
EdenDsmodBool action(void* p,const char* name,int64_t argument) {
    if(!p || !name) return EDEN_DSMOD_FALSE;
    auto& s=*static_cast<State*>(p);
    std::lock_guard<std::mutex> lock(s.mutex);
    if(is_town(s.mode)) {
        if(!s.town_drawn || s.town_redraw || !s.town.source) return EDEN_DSMOD_FALSE;
        using Tab=etrian::TownQuestTab;
        const std::string_view request{name};
#ifdef ETRIAN_ARMOROAD_ROOMS
        if(request=="town_settings" || request=="town_settings_back") {
            if(s.facility!=etrian::TownFacility::plaza || s.town_board ||
               s.town_settings==(request=="town_settings")) return EDEN_DSMOD_FALSE;
            s.town_settings=request=="town_settings";
            s.town_drawn=false;
            s.town_redraw=true;
            return EDEN_DSMOD_TRUE;
        }
        const bool rule_action=request=="deep_run" || request=="exp_rate" || request=="guild_training" ||
            request=="guild_rotation" || request=="toggle_weekly" || request=="toggle_bounty";
        if(s.town_settings!=rule_action || request=="merchant_favor") return EDEN_DSMOD_FALSE;
        if(request=="toggle_weekly") {
            if(!s.exp_available || s.weekly_clear || s.exp_epoch>0xffffff) return EDEN_DSMOD_FALSE;
            const auto& box=s.exp_box;
            const bool current=s.weekly_day && int(*s.weekly_day/7)==s.weekly.week;
            const auto word=!s.weekly_enabled && current?s.weekly.command(s.exp_epoch):0;
            if(!box.store_u64(box.userdata,etrian::weekly::mailbox_offset,word)) return EDEN_DSMOD_FALSE;
            s.weekly_enabled=!s.weekly_enabled;
            s.town_drawn=false; s.town_redraw=true;
            return EDEN_DSMOD_TRUE;
        }
        if(request=="guild_prev" || request=="guild_next") {
            if(s.facility!=etrian::TownFacility::guild || !s.guild_roster || s.town_board) return EDEN_DSMOD_FALSE;
            if(request=="guild_prev" && s.guild_page) --s.guild_page;
            else if(request=="guild_next" && (s.guild_page+1)*etrian::guild_page_size<s.guild_roster->size()) ++s.guild_page;
            else return EDEN_DSMOD_FALSE;
            s.town_drawn=false; s.town_redraw=true;
            return EDEN_DSMOD_TRUE;
        }
        if(request=="town_board" && s.facility!=etrian::TownFacility::plaza &&
           s.facility!=etrian::TownFacility::bistro) return EDEN_DSMOD_FALSE;
#endif
#if defined(ETRIAN_BISTRO_BOUNTY) && defined(ETRIAN_ARMOROAD_ROOMS)
        if(request=="toggle_bounty" || request=="bistro_offer" || request=="bistro_report" || request=="bistro_abandon") {
            using namespace etrian::bistro_bounty;
            const auto& box=s.exp_box;
            uint64_t word=0;
            if(!s.bounty_available || s.town_board || s.town.source!=s.bounty_save ||
               !box.load_u64(box.userdata,contract_offset,&word) || word!=s.bounty_word || (word&report_requested))
                return EDEN_DSMOD_FALSE;
            const bool idle=!word || (word&reported);
            if(request=="toggle_bounty") {
                if(!idle && !box.store_u64(box.userdata,contract_offset,word^paused)) return EDEN_DSMOD_FALSE;
                s.bounty_enabled=!s.bounty_enabled;
            } else {
                if(!s.bounty_enabled || s.facility!=etrian::TownFacility::bistro) return EDEN_DSMOD_FALSE;
                if(request=="bistro_report" && ready_to_request(word)) word|=report_requested;
                else if(request=="bistro_abandon" && !idle) word=0;
                else if(request=="bistro_offer" && idle && argument>=0 && size_t(argument)<s.bounty_offers.size()) {
                    const auto& offer=s.bounty_offers[argument];
                    if(!offer.species || offer.completed || !box.store_u64(box.userdata,party_offset,s.bounty_party))
                        return EDEN_DSMOD_FALSE;
                    word=(uint64_t(s.bounty_epoch)<<32)|offer.species;
                } else return EDEN_DSMOD_FALSE;
                if(!box.store_u64(box.userdata,contract_offset,word)) return EDEN_DSMOD_FALSE;
            }
            s.town_drawn=false; s.town_redraw=true;
            return EDEN_DSMOD_TRUE;
        }
#elif defined(ETRIAN_BISTRO_BOUNTY)
        if(request=="bistro_bounty") {
            using namespace etrian::bistro_bounty;
            const auto& box=s.exp_box;
            uint64_t word=0;
            if(s.town_board || !s.bounty_available || s.town.source!=s.exp_save ||
               !box.load_u64(box.userdata,contract_offset,&word) || word!=s.bounty_word) return EDEN_DSMOD_FALSE;
            if(ready_to_request(word)) word|=report_requested;
            else if((!word || (word&reported)) && !s.bounty_pool.empty()) {
                try {
                    std::random_device random;
                    const auto index=std::uniform_int_distribution<size_t>(0,s.bounty_pool.size()-1)(random);
                    word=(uint64_t(s.exp_epoch)<<32)|s.bounty_pool[index].id;
                } catch(...) { return EDEN_DSMOD_FALSE; }
                if(!box.store_u64(box.userdata,party_offset,s.bounty_party)) return EDEN_DSMOD_FALSE;
            } else return EDEN_DSMOD_FALSE;
            if(!box.store_u64(box.userdata,contract_offset,word)) return EDEN_DSMOD_FALSE;
            s.town_drawn=false;
            s.town_redraw=true;
            return EDEN_DSMOD_TRUE;
        }
#endif
#ifdef ETRIAN_EXP_TRIAL
        if(request=="deep_run") {
            if(s.town_board || !s.exp_available || s.exp_save!=s.town.source) return EDEN_DSMOD_FALSE;
            s.deep_selected=!s.deep_selected;
            s.town_drawn=false;
            s.town_redraw=true;
            return EDEN_DSMOD_TRUE;
        }
        if(request=="exp_rate") {
            const auto& box=s.exp_box;
            if(s.town_board || !s.exp_available || s.exp_save!=s.town.source ||
               !etrian::exp_trial::supported_rate(argument) || argument==s.exp_rate) return EDEN_DSMOD_FALSE;
            if(!box.store_u64(box.userdata,0,etrian::exp_trial::command(argument,s.exp_epoch,s.deep_running,s.rules)))
                return EDEN_DSMOD_FALSE;
            s.exp_rate=static_cast<uint32_t>(argument);
            s.town_drawn=false;
            s.town_redraw=true;
            return EDEN_DSMOD_TRUE;
        }
#ifdef ETRIAN_EXPEDITION_RULES
        if(request=="guild_training" || request=="guild_rotation" || request=="merchant_favor") {
            if(s.town_board || !s.exp_available || s.exp_save!=s.town.source) return EDEN_DSMOD_FALSE;
            using namespace etrian::exp_trial;
            auto rules=s.rules;
            const auto& box=s.exp_box;
            if(request=="guild_training") rules.training=(rules.training+50)%150;
            else if(request=="guild_rotation") rules.rotation=!rules.rotation;
            else {
                rules.favor=!rules.favor;
                if(rules.favor) {
                    const auto now=static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
                    const uint32_t seed=static_cast<uint32_t>(now^(now>>32)^s.exp_save^s.exp_epoch);
                    if(!box.store_u32(box.userdata,favor_pending_offset,0) ||
                       !box.store_u32(box.userdata,favor_rng_offset,seed?seed:1)) return EDEN_DSMOD_FALSE;
                }
            }
            if(!box.store_u64(box.userdata,0,command(s.exp_rate,s.exp_epoch,s.deep_running,rules)))
                return EDEN_DSMOD_FALSE;
            s.rules=rules;
            if(!s.rules.favor) s.favor_ready=false;
            s.town_drawn=false;
            s.town_redraw=true;
            return EDEN_DSMOD_TRUE;
        }
#endif
#endif
        if(request=="town_board" && !s.town_board) {
            s.town_board=true;
            s.town_tab=town_quest_count(s.town,Tab::report)?Tab::report:Tab::available;
            s.town_page=0;
        } else if(!s.town_board) return EDEN_DSMOD_FALSE;
        else if(request=="town_back") s.town_board=false;
        else if(request=="town_board_prev" && s.town_page>0) --s.town_page;
        else if(request=="town_board_next" && town_next(s)) ++s.town_page;
        else {
            auto tab=s.town_tab;
            if(request=="town_available") tab=Tab::available;
            else if(request=="town_accepted") tab=Tab::accepted;
            else if(request=="town_ready") tab=Tab::report;
            else return EDEN_DSMOD_FALSE;
            if(tab==s.town_tab) return EDEN_DSMOD_FALSE;
            s.town_tab=tab;
            s.town_page=0;
        }
        s.town_drawn=false;
        s.town_redraw=true;
        return EDEN_DSMOD_TRUE;
    }
    if(!s.exploring) return EDEN_DSMOD_FALSE;
    const bool map_open=!s.guide && !s.quests && !s.expedition_open;
    if(!std::strcmp(name,"hints") && s.reference.ready && map_open) s.hints=!s.hints;
    else if(!std::strcmp(name,"guide") && map_open) s.guide=true;
    else if(!std::strcmp(name,"expedition") && map_open && s.floor<=25) s.expedition_open=true;
    else if(!std::strcmp(name,"map") && s.expedition_open) s.expedition_open=false;
    else if(!std::strcmp(name,"map") && s.guide && !s.selected_monster) clear_guide(s);
    else if(!std::strcmp(name,"map") && s.quests && !s.selected_quest) clear_quests(s);
    else if(!std::strcmp(name,"quests") && map_open) s.quests=true;
    else if(!std::strcmp(name,"quest_list") && s.quests && s.selected_quest) s.selected_quest.reset();
    else if(!std::strcmp(name,"quest_prev") && s.quest_list_drawn && !s.redraw && s.quest_page>0) --s.quest_page;
    else if(!std::strcmp(name,"quest_next") && s.quest_list_drawn && !s.redraw &&
            (s.quest_page+1)*etrian::quest_page_size<s.journal.entries.size()) ++s.quest_page;
    else if(!std::strcmp(name,"quest") && s.quest_list_drawn && !s.redraw &&
            argument>=0 && argument<etrian::quest_page_size &&
            s.quest_page*etrian::quest_page_size+argument<s.journal.entries.size())
        s.selected_quest=s.journal.entries[s.quest_page*etrian::quest_page_size+argument].key;
    else if(!std::strcmp(name,"roster") && s.guide && s.selected_monster) s.selected_monster=0;
    else if(!std::strcmp(name,"guide_prev") && s.guide_list_drawn && !s.redraw && s.guide_page>0) --s.guide_page;
    else if(!std::strcmp(name,"guide_next") && s.guide_list_drawn && !s.redraw && s.monsters &&
            (s.guide_page+1)*etrian::guide_page_size<s.monsters->size()) ++s.guide_page;
    else if(!std::strcmp(name,"monster") && s.guide_list_drawn && !s.redraw && s.monsters &&
            argument>=0 && uint64_t(argument)<etrian::guide_page_size &&
            s.guide_page*etrian::guide_page_size+argument<s.monsters->size())
        s.selected_monster=(*s.monsters)[s.guide_page*etrian::guide_page_size+argument].id;
    else return EDEN_DSMOD_FALSE;
    s.redraw=true;
    s.quest_list_drawn=s.guide_list_drawn=false;
    return EDEN_DSMOD_TRUE;
}
#if defined(ETRIAN_BISTRO_BOUNTY) && defined(ETRIAN_ARMOROAD_ROOMS)
void bistro_panel(etrian::glass::Canvas& canvas,const State& state) {
    using namespace etrian::glass;
    using namespace etrian::bistro_bounty;
    header(canvas,"01  /  TOWN","Butterfly Bistro","");
    canvas.frame(404,29,164,33,8,card);
    canvas.center(486,50,"Quest Board >",14,accent);
    if(!state.bounty_enabled || !state.bounty_available) {
        message(canvas,state.bounty_enabled?"Bounty data unavailable":"Bistro Bounties are off",
            state.bounty_enabled?"The companion will retry when data is readable.":"Enable bounties in Armoroad Settings.");
        return;
    }
    canvas.text(24,103,"BOUNTIES  /  ONE AT A TIME",12,accent);
    const auto word=state.bounty_word;
    for(size_t i=0;i<state.bounty_offers.size();++i) {
        const auto& offer=state.bounty_offers[i];
        const int y=117+int(i)*67;
        const bool active=offer.species && offer.species==species_of(word) && !(word&reported);
        canvas.frame(24,y,544,59,8,card,active?accent:border);
        const auto it=std::ranges::find(state.bounty_pool,offer.species,&etrian::FloorMonster::id);
        canvas.fit(39,y+25,offer.species?(it==state.bounty_pool.end()?"Unknown target":it->name):"No eligible offer",19,351,ink);
        if(offer.species) canvas.text(39,y+47,"5 kills  /  10% level EXP",12,muted);
        if(offer.completed) canvas.right(552,y+34,"REPORTED",12,accent);
        else if(active) canvas.right(552,y+34,std::to_string(kills_of(word))+" / 5",16,accent);
        else if(offer.species && (!word || (word&reported))) canvas.right(552,y+34,"Accept >",15,accent);
    }
    if(word&report_requested) canvas.text(24,431,"Choose Report results on the game screen.",16,accent);
    else {
        if(word && !(word&reported)) {
            canvas.frame(24,421,150,33,8,card); canvas.center(99,443,"Give up",14,accent);
        }
        if(ready_to_request(word)) {
            canvas.frame(400,421,168,33,8,card); canvas.center(484,443,"Prepare report",14,accent);
        }
    }
    footer(canvas,"Regular monsters only","Session board");
}
#endif
#ifdef ETRIAN_ARMOROAD_ROOMS
void weekly_panel(etrian::glass::Canvas& canvas,const State& state) {
    using namespace etrian::glass;
    header(canvas,"01  /  TOWN","Napier's Firm","");
    if(!state.weekly_enabled) {
        message(canvas,"Weekly Buyers' List is off","Enable it in Armoroad Settings.");
        return;
    }
    if(!state.weekly_day || state.weekly.week<0) {
        message(canvas,"No weekly prices yet","Visit a labyrinth floor to establish this week's list.");
        return;
    }
    if(!state.weekly_armed) {
        message(canvas,"Weekly prices unavailable","Leave the shop and reenter from Armoroad.");
        return;
    }
    const auto remaining=std::max<int64_t>(0,(int64_t(state.weekly.week)+1)*7-*state.weekly_day);
    canvas.text(24,103,"B"+std::to_string(state.weekly.floor)+"F materials  /  Refresh in "+std::to_string(remaining)+
        (remaining==1?" day":" days"),15,muted);
    constexpr std::array modifiers{"+15%","+20%","+30%","-25%"};
    for(size_t i=0;i<state.weekly.items.size();++i) {
        const int y=121+int(i)*69;
        canvas.frame(24,y,544,59,8,card,border);
        if(!state.weekly.items[i]) {
            canvas.text(39,y+35,"No eligible material",16,muted);
            continue;
        }
        const auto& name=state.weekly_names[i];
        canvas.fit(39,y+25,name.empty()?"Material label unavailable":name,19,364,ink);
        const int base=state.weekly_prices[i];
        canvas.text(39,y+46,base?std::to_string(base)+" en  >  "+std::to_string(etrian::weekly::price(base,i))+" en":
            "Unit price appears when quoted",12,muted);
        canvas.right(552,y+37,modifiers[i],20,i==3?ink:accent);
    }
    footer(canvas,"Native shop confirmation","Material prices only");
}
#endif
EdenDsmodBool dashboard_image(const State& state,void* receiver,EdenDsmodImageSink sink) {
    etrian::glass::Canvas canvas;
    if(is_town(state.mode)) {
        if(state.town_board)
            etrian::glass::town_board(canvas,state.town.quests,state.town_tab,state.town_page);
        else {
#ifdef ETRIAN_EXP_TRIAL
            using namespace etrian::glass;
#ifdef ETRIAN_ARMOROAD_ROOMS
            if(state.town_settings) {
                header(canvas,"01  /  ARMOROAD","Settings","");
                canvas.text(24,105,"Selections last until you reload the game.",15,muted);
                canvas.text(24,187,"EXPEDITION RULES",12,accent);
#ifdef ETRIAN_BISTRO_BOUNTY
                const bool toggle_ready=state.bounty_available && !(state.bounty_word&etrian::bistro_bounty::report_requested);
                canvas.frame(24,252,210,42,8,card,state.bounty_enabled?accent:border);
                canvas.center(129,279,state.bounty_enabled?"Bistro Bounties: On":"Bistro Bounties: Off",14,toggle_ready?ink:muted);
#endif
                canvas.frame(244,252,244,42,8,card,state.weekly_enabled?accent:border);
                canvas.center(366,279,state.weekly_enabled?"Weekly Buyers' List: On":"Weekly Buyers' List: Off",14,ink);
                canvas.frame(24,449,144,32,8,card);
                canvas.center(96,470,"< Armoroad",14,accent);
            } else if(state.facility==etrian::TownFacility::shop) {
                weekly_panel(canvas,state);
            } else if(state.facility==etrian::TownFacility::guild) {
                guild_roster(canvas,state.guild_roster,state.guild_page);
            } else if(state.facility==etrian::TownFacility::senate) {
                senate_missions(canvas,state.town.quests);
#ifdef ETRIAN_BISTRO_BOUNTY
            } else if(state.facility==etrian::TownFacility::bistro) {
                bistro_panel(canvas,state);
#endif
            } else {
                town(canvas,state.town);
                if(state.facility==etrian::TownFacility::plaza && state.town.source) {
                    canvas.frame(428,502,140,24,7,card,accent);
                    canvas.center(498,519,"Settings >",13,accent);
                }
            }
#else
            town(canvas,state.town,"");
#endif
#if defined(ETRIAN_BISTRO_BOUNTY) && !defined(ETRIAN_ARMOROAD_ROOMS)
            {
                using namespace etrian::bistro_bounty;
                const auto word=state.bounty_word;
                const auto it=std::ranges::find(state.bounty_pool,species_of(word),&etrian::FloorMonster::id);
                const std::string enemy=it==state.bounty_pool.end()?"Unknown target":it->name;
                const auto status=word&report_requested?"At the Bistro, choose Report results":
                    word&reported?"Bounty reported":word?enemy+"  "+std::to_string(kills_of(word))+" / 5":
                    !state.bounty_available?"Bounty unavailable":state.bounty_pool.empty()?"Explore a floor to unlock":"5 kills · 10% level EXP";
                canvas.box(24,502,544,26,bg);
                canvas.fit(24,519,"Bistro Bounty: "+status,12,374,ink);
                if(state.bounty_available && (ready_to_request(word) || ((!word || (word&reported)) && !state.bounty_pool.empty()))) {
                    canvas.frame(408,502,160,24,7,card,accent);
                    canvas.center(488,519,ready_to_request(word)?"Prepare report":word?"Accept next bounty":"Accept bounty",12,accent);
                }
            }
#endif
#ifdef ETRIAN_ARMOROAD_ROOMS
            if(state.town_settings) {
#endif
            canvas.text(302,27,state.exp_available?"BATTLE EXP BOOST":"EXP BOOST UNAVAILABLE",11,muted);
            if(state.exp_available) {
                constexpr std::array rates{0u,2u,5u,10u};
                constexpr std::array labels{"Off","2x","5x","10x"};
                for(int i=0;i<4;++i) {
                    const auto x=302+i*67;
                    const bool selected=state.exp_rate==rates[i];
                    canvas.frame(x,34,64,30,7,card,selected?accent:border);
                    canvas.center(x+32,54,labels[i],14,selected?accent:ink);
                }
#ifdef ETRIAN_EXPEDITION_RULES
                const auto toggle=[&](int x,const char* title,const char* value,bool enabled) {
                    canvas.frame(x,204,96,30,7,card,enabled?accent:border);
                    canvas.center(x+48,217,title,11,ink);
                    canvas.center(x+48,230,value,11,enabled?accent:muted);
                };
                toggle(124,"Guild Training",state.rules.training==50?"50%":state.rules.training==100?"100%":"Off",
                       state.rules.training!=0);
                toggle(224,"Guild Rotation",state.rules.rotation?"On":"Off",state.rules.rotation);
#ifndef ETRIAN_ARMOROAD_ROOMS
                toggle(324,"Merchant's Favor",state.rules.favor?(state.favor_ready?"Ready":"On"):"Off",state.rules.favor);
#endif
#endif
                canvas.frame(424,204,144,30,7,card,state.deep_selected?accent:border);
                canvas.center(496,224,state.deep_selected?"Deep Run: On":"Deep Run: Off",14,ink);
            }
#ifdef ETRIAN_ARMOROAD_ROOMS
            }
#endif
#else
            etrian::glass::town(canvas,state.town);
#endif
        }
    } else etrian::glass::battle(canvas,state.mode,state.battle);
    sink(receiver,canvas.width,canvas.height,canvas.pixels.data(),canvas.pixels.size());
    return EDEN_DSMOD_TRUE;
}
EdenDsmodBool image(void* p,const EdenDsmodHostApi* host,const char* key,void* receiver,EdenDsmodImageSink sink) {
    if(!p || !key || !sink) return EDEN_DSMOD_FALSE;
    auto& s=*static_cast<State*>(p);
    std::lock_guard<std::mutex> lock(s.mutex);
    const std::string expected="module:etrian:map/"+std::to_string(s.generation);
    if (!s.generation || key!=expected) return EDEN_DSMOD_FALSE;
    if (s.mode!=etrian::BattleMode::map) {
        if(s.town_redraw) return EDEN_DSMOD_FALSE;
        const auto drawn=dashboard_image(s,receiver,sink);
        s.town_drawn=drawn && is_town(s.mode);
        return drawn;
    }
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
        const auto selected=[&]() -> const etrian::FloorMonster* {
            if(!s.monsters || !s.selected_monster) return nullptr;
            const auto it=std::ranges::find(*s.monsters,s.selected_monster,&etrian::FloorMonster::id);
            return it==s.monsters->end()?nullptr:&*it;
        }();
        if(selected) etrian::glass::monster_details(canvas,s.floor,*selected,s.drops);
        else {
            etrian::glass::floor_guide(canvas,s.floor,s.monsters,s.guide_page);
            s.guide_list_drawn=true;
        }
        sink(receiver,canvas.width,canvas.height,canvas.pixels.data(),canvas.pixels.size());
        return EDEN_DSMOD_TRUE;
    }
    if (s.expedition_open) {
        etrian::glass::Canvas canvas;
        etrian::glass::expedition_details(canvas,s.floor,s.expedition);
        sink(receiver,canvas.width,canvas.height,canvas.pixels.data(),canvas.pixels.size());
        return EDEN_DSMOD_TRUE;
    }
    if (!s.artwork_loaded && host && host->read_romfs) {
        for (int n=1;n<=41;++n) s.artwork_ok[n]=etrian::load_game_icon(*host,n,s.artwork[n]);
        for (bool loaded : s.artwork_ok) s.artwork_loaded |= loaded;
    }
    etrian::glass::Canvas canvas;
    etrian::glass::map(canvas,s.tiles,s.icons,s.artwork,s.artwork_ok,s.x,s.y,s.floor,s.direction,&s.reference,s.hints,
                       s.floor<=25?&s.expedition:nullptr);
#ifdef ETRIAN_EXP_TRIAL
    if(s.deep_running) {
        constexpr std::array labels{"1x","1.25x","1.5x","1.75x","2x"};
        const auto step=etrian::exp_trial::deep_run_quarters(s.deep_wins)-4;
        etrian::glass::badge(canvas,233,13,159,std::string("DEEP RUN NEXT: ")+labels[step],etrian::glass::accent);
    }
#endif
    sink(receiver,canvas.width,canvas.height,canvas.pixels.data(),canvas.pixels.size());
    return EDEN_DSMOD_TRUE;
}
const EdenDsmodModuleApi api{EDEN_DSMOD_MODULE_ABI_VERSION,sizeof(EdenDsmodModuleApi),0,EDEN_DSMOD_MODULE_ABI_HASH,Title,"Etrian Odyssey III HD map",EDEN_DSMOD_CAP_EXTENSIONS,&supports,&create,&destroy,&sample,&tick};
const EdenDsmodModuleExtensions extensions{EDEN_DSMOD_EXT_VERSION,sizeof(EdenDsmodModuleExtensions),EDEN_DSMOD_EXT_HASH,&configure,&action,&image};
}
extern "C" __attribute__((visibility("default"))) const EdenDsmodModuleApi* eden_dsmod_get_module(uint32_t v,uint64_t h){return v==EDEN_DSMOD_MODULE_ABI_VERSION&&h==EDEN_DSMOD_MODULE_ABI_HASH?&api:nullptr;}
extern "C" __attribute__((visibility("default"))) const EdenDsmodModuleExtensions* eden_dsmod_get_extensions(uint32_t v,uint64_t h){return v==EDEN_DSMOD_EXT_VERSION&&h==EDEN_DSMOD_EXT_HASH?&extensions:nullptr;}
