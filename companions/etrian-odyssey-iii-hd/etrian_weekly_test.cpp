// SPDX-License-Identifier: GPL-3.0-or-later
#include "etrian_weekly.h"
#include <cassert>

int main() {
    using namespace etrian::weekly;
    static_assert(percent == std::array<unsigned,4>{115,120,130,75});
    static_assert(price(101,0)==116 && price(101,1)==121 && price(101,2)==131);
    static_assert(price(101,3)==75 && price(1,3)==1 && price(0,3)==0);
    static_assert(price(-1,1)==-1 && price(200,4)==200);
    List list;
    list.observe(1,{925,926,927,928,925,0,1555});
    assert(list.refresh(0));
    assert(list.week==0 && list.floor==1 && list.items[3]);
    for (size_t i=0;i<4;++i) for(size_t j=0;j<i;++j) assert(list.items[i]!=list.items[j]);
    const auto first=list.items;
    list.observe(3,{929,930});
    list.observe(2,{931});
    assert(!list.refresh(6) && list.items==first);
    assert(list.refresh(7) && list.week==1 && list.floor==3);
    assert(list.items[0] && list.items[1] && !list.items[2] && !list.items[3]);
    assert(!list.refresh(6) && list.week==1);
    list.visit(4);
    assert(list.deepest==4 && !list.refresh(14) && list.week==1);
    list.observe(3,{931}); // Never fall back to a shallower pool.
    assert(!list.refresh(14));
    assert(list.command(0xabcdef12)==0);
    const auto word=list.command(0xabcdef);
    assert(word>>40==0xabcdef && (word&1023)==list.items[0]-923);
    assert((word>>10&1023)==list.items[1]-923);
    list.observe(4,{931});
    assert(list.refresh(14) && list.floor==4 && list.items[0]==931);
    list={};
    assert(!list.refresh(5) && list.command(12)==0);
    list.observe(1,{924});
    assert(list.refresh(5) && list.floor==1);
}
