#include "etrian_art.h"
#include <cassert>
size_t unexpected(void*,const char*,uint64_t,void*,size_t) { assert(false && "III must not read first-game asset offsets"); return 0; }
int main() {
    EdenDsmodHostApi h{}; h.read_romfs=unexpected; etrian::IconPixels p{};
    for(int i=1;i<=41;++i) assert(!etrian::load_game_icon(h,i,p));
}
