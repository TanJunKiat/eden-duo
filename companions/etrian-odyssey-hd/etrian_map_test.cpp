#include "etrian_map.h"
#include <cassert>
int main() {
    assert(etrian::index(6, 9) == 321);
    assert(etrian::screen_y(9) == 20);
    assert(etrian::screen_y(29) == 0);
    assert(etrian::floor_color(0) == 0xff082744);
    assert(etrian::floor_color(0x28) == 0xff1996ce);
    assert(etrian::has_right_wall(2));
    assert(!etrian::has_right_wall(4));
    assert(etrian::has_lower_wall(4));
}
