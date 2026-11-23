// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "etrian_enemy_names.h"
#include <algorithm>
#include <array>
#include <optional>
#include <vector>

namespace etrian {
struct QuestKey {
    bool mission;
    uint16_t index;
    bool operator==(const QuestKey&) const = default;
};
enum class QuestStatus { active, report };
struct QuestItem {
    uint16_t id=0;
    uint8_t required=0;
};
struct QuestHint {
    uint32_t floors=0;
    std::string_view action, turn_in;
    std::array<QuestItem,3> items{};
};
constexpr uint32_t quest_floor(int floor) {
    return floor>=1 && floor<=30 ? 1u<<(floor-1) : 0;
}
// Original summaries of matching HD guide/installed-journal evidence. No game text assets.
inline QuestHint quest_hint(QuestKey key) {
    constexpr auto f=quest_floor;
    constexpr auto bistro="Butterfly Bistro", senatus="Via Senatus", city="Deep City";
    if(key.mission) {
        switch(key.index) {
            case 0: return {f(1),"Map the accessible floor, including the central room with two chests.",senatus};
            case 1: return {f(4),"Defeat Narmer at B6. Use the side passages to corner it.",senatus};
            case 2: return {f(6),"Meet Olympia at A4, trigger the ambush at A5, then return to Olympia before reporting.",senatus};
            case 3: return {f(6)|f(7)|f(8),"Obtain the Sea Bead at B8F A1. Report at the B7F campsite and the Senatus, then use the bead at B6F B3. Find Ketos at B8F A6.",senatus};
            case 4: return {f(10),"Win the Hall of Conviction battle at C4, then choose Wait.",city};
            case 5: return {f(12),"Fight the Gatekeeper at D4.",city};
            case 6: return {f(12),"Fight the Gatekeeper at D4.",senatus};
            case 7: return {f(15),"Find the Amarantos flower at A4.",senatus};
            case 8: return {f(16),"Reach the teleporter at F3 and defeat Kirin.",senatus};
            case 9: return {f(15),"Investigate the facility at A4 using the Ibn Ghazi Powder.",city};
            case 10: return {f(16),"Reach the teleporter at F3 and defeat Shin.",city};
            case 11: return {f(20),"Reach the boss room at E3.",senatus};
            case 12: return {f(20),"Reach the boss room at E3.",city};
            default: return {};
        }
    }
    switch(key.index) {
        case 0: return {f(1),"Enter B1F and take a step, then return to town.",bistro};
        case 1: return {f(1),"Defeat Forest Frogs. They drop both ingredients.",bistro,{{{928,3},{929,1}}}};
        case 2: return {f(1),"Examine the resting spot in A2 during the day.",bistro};
        case 3: return {f(2)|f(3)|f(4),"Hunt Gigapedes at night for their leg drop.",bistro,{{{935,1}}}};
        case 4: return {f(2)|f(4),"Gather a Small Flower at a Take point and sell it to Napier, then report to the Bistro.",bistro,{{{1189,1}}}};
        case 5: return {f(2)|f(3),"Ask for information at the Bistro. Visit the campsite in B2F D6, then investigate B3F D3 and the B3F campsite.",bistro};
        case 6: return {0,"Bring an active-party Farmer of level 2 or higher to Missy.",bistro};
        case 7: return {f(4),"At A4, smoke the nest, climb the tree and cut the nest off the branch.",bistro};
        case 8: return {f(4),"Defeat at least 15 Fanged Fish or Devilfish without leaving the floor. Aim for 50 or more for the top reward.",bistro};
        case 9: return {f(7),"Gather Indigo Pith at a Chop point.",bistro,{{{1211,1}}}};
        case 10: return {f(8),"Investigate B2 and defeat the Glutton Beast.",bistro};
        case 11: return {f(1),"Defeat all three Greedy Lizards that spawn when you enter the floor.",bistro};
        case 12: return {f(7),"Investigate D5 at night, then defeat the three waves of starfish.",bistro};
        case 13: return {f(6),"Finish a Koolasuchus with Volt damage for Thick Throat.",bistro,{{{965,1}}}};
        case 14: return {f(1),"Speak to the B1F guard, then investigate the theft with Aman and the blond patron at the Bistro.",bistro};
        case 15: return {f(6),"Leave the offering at the statue in the dead end at C3.",bistro};
        case 16: return {f(5),"Look for fossils near the treasure chests.",bistro};
        case 17: return {f(11)|f(12),"Gather a mining material. The material you hand in determines the reward.",bistro};
        case 18: return {f(9)|f(10)|f(11),"Hunt Stegosaurus on B9F, Cave Elephant on B10F and Flame Lizard on B11F.",bistro,{{{996,1},{992,1},{988,1}}}};
        case 20: return {f(9),"Break a branch from the tree at E4.",bistro};
        case 21: return {f(10),"Inspect all three hot springs for the best reward: A1, A7 and D1.",bistro};
        case 22: return {f(11),"Finish Lava Beasts with Ice damage for Igneous Rocks.",bistro,{{{984,3}}}};
        case 23: return {f(9),"Follow the ant from E3 to D3, defeat it and search where it stopped. More ants appear after you take the fruit.",bistro};
        case 24: return {f(9),"Take the new passage at E2 north to A1 and defeat the Wicked Queen.",bistro};
        case 25: return {f(14),"Ask Napier about an antique. If you decline her offer, ask the warrior at the Bistro about the northwest passage.",bistro};
        case 26: return {f(15),"At E4, place Sky at the top, Land at the left and Man at the right.",bistro};
        case 27: return {f(13),"Inspect the wall at D6 for the fruit.",bistro};
        case 28: return {f(15)|f(16),"Hunt Deep Marines for chips. Finish a Deep Raider with Strike damage for the edge.",bistro,{{{1016,5},{1024,1}}}};
        case 30: return {f(15),"Defeat the armored beast on B15F.",bistro};
        case 31: return {f(14),"At A5, inspect the pillar. Press Triangle, Filled Square, Filled Triangle, Diamond, Star, Ring, Filled Star, Filled Circle.",bistro};
        case 32: return {0,"Help Aman at the inn write a reply.",bistro};
        case 35: return {0,"Equip Ned's Harpoon and catch a Tanniyn at sea.",bistro};
        case 37: return {f(18),"Gather salt at C5.",bistro};
        case 39: return {f(19),"Inspect the boulder at D1. Breaking it starts a fight.",bistro};
        case 40: return {f(1),"Inspect the clover at D3. Either choice works; leaving it gives the better reward.",bistro};
        case 41: return {f(18),"Equip the Mushroom Dagger you receive on entering B18F. Finish off the enemy at F7 with that weapon.",bistro};
        case 42: return {f(19),"Visit Napier's Branch by day and night. If you agree to help, ask around town. A Silver Fox on B19F can supply the tail.",bistro};
        case 43: return {f(1),"Go to the treasure room and defeat the birds.",bistro};
        case 44: return {f(9)|f(10)|f(11)|f(12),"Defeat monsters in the third stratum. Deeper floors and certain FOEs score more points.",bistro};
        case 45: return {f(13)|f(14)|f(15)|f(16),"Gather materials in the fourth stratum. Aim for 45 or more for the top reward.",bistro};
        case 46: return {f(13),"Press the switch at A5, then find Kraken at B5.",bistro};
        case 47: return {f(9),"Talk to Aman, then Angie in the Deep City. Gather rare Scarlet Sage from B9F and bring it to Angie.",bistro};
        case 48: return {f(17),"Speak to Aman, then defeat Alraune at D6.",bistro};
        case 49: return {0,"Report monsters to Flowdia at the Senatus, then return to Missy.",bistro};
        case 50: return {0,"Record discovered items in the compendium.",bistro};
        case 51: return {f(5),"Find the Anemone at E1.",bistro};
        default: return {};
    }
}
struct QuestRequirement {
    std::string name;
    int required=0;
    std::optional<int> in_bag;
    bool operator==(const QuestRequirement&) const = default;
};
struct QuestEntry {
    QuestKey key;
    QuestStatus status;
    std::string name, description;
    std::vector<QuestRequirement> items;
    bool operator==(const QuestEntry&) const = default;
};
struct QuestJournal {
    uint64_t source=0; // session-local identity; never rendered or published
    std::vector<QuestEntry> entries;
    bool operator==(const QuestJournal&) const = default;
};
inline constexpr int quest_page_size=5;

namespace quest_data {
template<class Reader,class T>
bool read(const Reader& memory,uint64_t object,uint64_t offset,T& value) {
    return object && object<=UINT64_MAX-offset-sizeof(T) && memory(object+offset,&value,sizeof(T));
}
template<class Reader>
uint64_t follow(const Reader& memory,uint64_t address,std::initializer_list<uint64_t> offsets) {
    for(auto offset:offsets) if(!read(memory,address,offset,address) || !address) return 0;
    return address;
}
template<class Reader>
std::optional<std::array<int16_t,60>> bag(const Reader& memory,uint64_t save) {
    const auto array=follow(memory,save,{0x20,0x58});
    uint64_t size=0;
    std::array<uint64_t,60> slots{};
    if(!read(memory,array,0x18,size) || size!=slots.size() || !read(memory,array,0x20,slots)) return {};
    std::array<int16_t,60> ids{};
    for(size_t i=0;i<slots.size();++i)
        if(!read(memory,slots[i],0x10,ids[i]) || ids[i]<0) return {};
    return ids;
}
}

template<class Reader>
QuestJournal read_quest_journal(const Reader& memory,uint64_t base,int floor) {
    using namespace quest_data;
    const auto save=follow(memory,base,{0x639a810,0,0xb8,0,0x10});
    const auto flag_array=follow(memory,save,{0x48});
    const auto master=follow(memory,base,{0x61a60f0,0x18,0xc0,0,0xb8,0});
    uint64_t size=0;
    std::array<uint8_t,2048> flags{};
    if(!read(memory,flag_array,0x18,size) || size!=flags.size() || !read(memory,flag_array,0x20,flags)) return {};
    const auto set=[&](uint16_t bit) { return (flags[bit/8] & (1u<<(bit%8)))!=0; };
    QuestJournal journal{save,{}};
    const EnemyLabels labels(memory,base);
    std::optional<std::array<int16_t,60>> inventory;
    bool checked_inventory=false;
    for(const bool mission:{true,false}) {
        const auto records=follow(memory,master,{mission?0xc8u:0xc0u,0x10});
        const uint16_t count=mission?13:52;
        std::array<std::array<uint16_t,8>,52> rows{};
        if(!read(memory,records,0x18,size) || size!=count || records>UINT64_MAX-0x20-sizeof(rows) ||
           !memory(records+0x20,rows.data(),count*sizeof(rows.front()))) return {};
        for(uint16_t i=0;i<count;++i) {
            // QuestMissionGetState checks complete, report, then accepted. Only global flags
            // use this bitmap; do not interpret a local-event slot as global state.
            const auto& row=rows[i];
            const auto accepted=row[3],report=row[4],complete=row[5];
            if(accepted/8>=flags.size() || report/8>=flags.size() || complete/8>=flags.size() ||
               accepted==report || report==complete || accepted==complete) return {};
            if(set(complete) || (!set(report) && !set(accepted))) continue;
            auto name=labels.quest(memory,mission,i);
            if(name.empty()) name=std::string(mission?"Mission ":"Quest ")+std::to_string(i+1);
            QuestEntry entry{{mission,i},set(report)?QuestStatus::report:QuestStatus::active,
                             std::move(name),labels.quest(memory,mission,i,true),{}};
            for(const auto [item,required]:quest_hint(entry.key).items) {
                if(!item) continue;
                if(!checked_inventory) { inventory=bag(memory,save); checked_inventory=true; }
                auto label=labels.material(memory,item);
                if(label.empty()) label="Item "+std::to_string(item);
                QuestRequirement requirement{std::move(label),required,{}};
                if(inventory) requirement.in_bag=std::ranges::count(*inventory,static_cast<int16_t>(item));
                entry.items.push_back(std::move(requirement));
            }
            journal.entries.push_back(std::move(entry));
        }
    }
    if(follow(memory,base,{0x639a810,0,0xb8,0,0x10})!=save || follow(memory,save,{0x48})!=flag_array) return {};
    std::stable_sort(journal.entries.begin(),journal.entries.end(),[floor](const auto& a,const auto& b) {
        const auto rank=[floor](const auto& q) {
            if(q.key.mission) return 0;
            return q.status==QuestStatus::active && (quest_hint(q.key).floors & quest_floor(floor))?1:2;
        };
        return rank(a)<rank(b);
    });
    return journal;
}
} // namespace etrian
