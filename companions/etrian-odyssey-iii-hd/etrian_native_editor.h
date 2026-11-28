#pragma once
#include "etrian_quests.h"

namespace etrian {
// Supported EO III HD build only. Mirror MappingManager::UpdateInput's read-side
// checks (not the OpenMap/CloseMap preference, which needn't track actual editor visibility).
template<class Reader> bool read_native_editor(const Reader& memory,uint64_t base) {
    using quest_data::follow;
    using quest_data::read;
    const auto scene=follow(memory,base,{0x618e708,0x58,0xb8,0});
    int32_t kind=-1,floor=0;
    if(!read(memory,scene,0x44,kind) || kind!=4) return false;
    const auto data=follow(memory,base,{0x61a60b8,0x18,0xc0,0,0xb8,0});
    if(!read(memory,data,0xdc,floor) || floor<1 || floor>25) return false;
    const auto ui=follow(memory,base,{0x61a60e0,0x18,0xc0,0,0xb8,0});
    if(follow(memory,ui,{0x178})!=data) return false;
    const auto zero=[&](uint64_t ptr,uint64_t offset) {
        uint8_t value=255;
        return read(memory,ptr,offset,value) && value==0;
    };
    if(!zero(data,0xd1) || !zero(follow(memory,ui,{0x88}),0xad)) return false;
    const auto map=follow(memory,base,{0x618d6a8,0xb8,0});
    if(!zero(follow(memory,map,{0x18}),0x88) || !zero(follow(memory,map,{0x28}),0x30) ||
       !zero(follow(memory,base,{0x618ae60,0xb8,0,0x18}),0x28)) return false;
    int32_t after=0;
    return read(memory,scene,0x44,kind) && kind==4 && read(memory,data,0xdc,after) &&
           after==floor && zero(data,0xd1);
}
} // namespace etrian
