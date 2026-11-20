#pragma once
#include "../../src/core/mods/dsmod_module_abi.h"
#include <array>
#include <cstdint>
namespace etrian {
using IconPixels = std::array<uint8_t,54*54*4>;
inline int art_for_map_icon(int type) {
    static constexpr int art[]={4,30,1,2,10,7,31,32,6,9,12,13,14,16,17,15,18,33,41,8,33,35,36,11,34,38,39,40};
    return type>=0 && type<28 ? art[type] : 0;
}
// III stores these assets inside a compressed Unity bundle; use the fallback until its layout is supported.
inline bool load_game_icon(const EdenDsmodHostApi&,int,IconPixels&) { return false; }
}
