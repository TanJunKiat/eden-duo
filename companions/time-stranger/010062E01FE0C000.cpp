// SPDX-License-Identifier: GPL-3.0-or-later
// Experimental read-only Time Stranger v1.2.2 active-party names.
#include "core/mods/dsmod_module_abi.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>

namespace {
constexpr uint64_t Title=UINT64_C(0x010062E01FE0C000);
constexpr char Build[]="135C83CDEA06AEC2D858E1BC7D53D72F7257166C000000000000000000000000";
EdenDsmodBool supports(const char* id) {return id && !std::strcmp(id,Build);}
bool compatible(const EdenDsmodHostApi* h) {
    if(!h || h->title_id!=Title) return false;
    constexpr char hex[]="0123456789ABCDEF";
    for(size_t i=0;i<32;++i) if(Build[2*i]!=hex[h->build_id[i]>>4] || Build[2*i+1]!=hex[h->build_id[i]&15])return false;
    return h->is_mapped && h->read_memory && h->publish_text;
}
bool read64(const EdenDsmodHostApi* h,uint64_t base,uint64_t offset,uint64_t& out) {
    if(!base || base>std::numeric_limits<uint64_t>::max()-offset-8)return false;
    const auto a=base+offset;
    return h->is_mapped(h->userdata,a,8) && h->read_memory(h->userdata,a,&out,8);
}
void* create(const EdenDsmodHostApi* h,const char*) {return compatible(h)?new(std::nothrow) int(0):nullptr;}
void destroy(void* p) {delete static_cast<int*>(p);}
void sample(void*,const EdenDsmodHostApi* h) {
    if(!h || !h->publish_text)return;
    std::array<std::string,3> names{"Unavailable","Unavailable","Unavailable"};
    uint64_t root=0,manager=0,begin=0,end=0;
    bool ok=compatible(h) && read64(h,h->main_base,0x146f500,root) && root &&
        read64(h,root,0x10,manager) && manager && read64(h,manager,8,begin) &&
        read64(h,manager,16,end) && begin && end>=begin && end-begin<=48 && (end-begin)%8==0;
    const uint64_t count=ok?(end-begin)/8:0;
    std::array<uint64_t,3> holders{},records{};
    if(ok) for(size_t i=0;i<3;++i) {
        names[i]="Empty";
        if(i>=count)continue;
        if(!read64(h,begin,i*8,holders[i])) {ok=false;break;}
        if(!holders[i])continue;
        if(!read64(h,holders[i],8,records[i])) {ok=false;break;}
        if(!records[i])continue;
        if(records[i]>std::numeric_limits<uint64_t>::max()-72) {ok=false;break;}
        std::array<char,64> text{};
        const auto a=records[i]+8;
        if(!h->is_mapped(h->userdata,a,text.size()) || !h->read_memory(h->userdata,a,text.data(),text.size())) {ok=false;break;}
        size_t n=0;
        while(n<text.size() && text[n]) {
            const auto c=static_cast<unsigned char>(text[n]);
            if(c<32 || c>126) {ok=false;break;}
            ++n;
        }
        if(!ok || n==0 || n==text.size()) {ok=false;break;}
        names[i].assign(text.data(),n);
    }
    // Discard a detected roster transition rather than mixing slots from different snapshots.
    uint64_t v=0;
    if(ok) ok=read64(h,h->main_base,0x146f500,v) && v==root &&
        read64(h,root,0x10,v) && v==manager && read64(h,manager,8,v) && v==begin &&
        read64(h,manager,16,v) && v==end;
    if(ok) for(size_t i=0;i<3 && i<count;++i) {
        if(!read64(h,begin,i*8,v) || v!=holders[i] ||
           (holders[i] && (!read64(h,holders[i],8,v) || v!=records[i]))) {ok=false;break;}
    }
    if(!ok) names.fill("Unavailable");
    for(size_t i=0;i<3;++i) {
        const auto key="party."+std::to_string(i);
        h->publish_text(h->userdata,key.c_str(),names[i].c_str());
    }
}
void tick(void*,const EdenDsmodHostApi*) {}
const EdenDsmodModuleApi api{EDEN_DSMOD_MODULE_ABI_VERSION,sizeof(EdenDsmodModuleApi),0,
    EDEN_DSMOD_MODULE_ABI_HASH,Title,"Time Stranger active party",EDEN_DSMOD_CAP_NO_TICK_WHEN_HIDDEN,
    &supports,&create,&destroy,&sample,&tick};
}
extern "C" __attribute__((visibility("default"))) const EdenDsmodModuleApi* eden_dsmod_get_module(uint32_t v,uint64_t h) {
    return v==EDEN_DSMOD_MODULE_ABI_VERSION && h==EDEN_DSMOD_MODULE_ABI_HASH?&api:nullptr;
}
