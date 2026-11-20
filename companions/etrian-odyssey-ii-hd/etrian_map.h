#pragma once
#include <cstdint>
namespace etrian {
constexpr int width = 35, height = 30, cells = width * height;
constexpr int index(int x, int y) { return y * width + x; }
constexpr int screen_y(int y) { return height - 1 - y; }
constexpr uint32_t floor_color(uint32_t flags) {
    if (!(flags & 8)) return 0xff082744;
    if (flags & 128) return 0xffdc6d66;
    if (flags & 64) return 0xffdfc25b;
    return 0xff1996ce;
}
constexpr bool has_right_wall(uint32_t flags) { return (flags & 2) != 0; }
constexpr bool has_lower_wall(uint32_t flags) { return (flags & 4) != 0; }
}
