#include "01008970176A6000.cpp"
#include <cassert>
#include <map>
struct Fake { std::map<uint64_t,uint8_t> mem; std::map<std::string,int64_t> ints; std::string key; std::vector<uint8_t> pixels; };
template<class T> void store(Fake& f,uint64_t a,T v) { auto p=reinterpret_cast<const uint8_t*>(&v); for(size_t i=0;i<sizeof(v);++i)f.mem[a+i]=p[i]; }
EdenDsmodBool mapped(void* p,uint64_t a,uint64_t n) { auto& m=static_cast<Fake*>(p)->mem; for(uint64_t i=0;i<n;++i)if(!m.count(a+i))return false; return true; }
EdenDsmodBool memory(void* p,uint64_t a,void* out,size_t n) { if(!mapped(p,a,n))return false; auto& m=static_cast<Fake*>(p)->mem; for(size_t i=0;i<n;++i)static_cast<uint8_t*>(out)[i]=m[a+i]; return true; }
void integer(void* p,const char* name,int64_t v) { static_cast<Fake*>(p)->ints[name]=v; }
void text(void* p,const char*,const char* v) { static_cast<Fake*>(p)->key=v; }
void sink(void* p,uint32_t w,uint32_t h,const uint8_t* pixels,size_t n) { assert(w==560&&h==480); static_cast<Fake*>(p)->pixels.assign(pixels,pixels+n); }
int main() {
 Fake f; EdenDsmodHostApi host{}; host.title_id=Title; host.main_base=0x100000; host.userdata=&f; host.is_mapped=mapped; host.read_memory=memory; host.publish_i64=integer; host.publish_text=text;
 store(f,host.main_base+MethodSlot,uint64_t(0x200000)); store(f,0x200018,uint64_t(0x300000)); store(f,0x3000c0,uint64_t(0x400000)); store(f,0x400000,uint64_t(0x300000)); store(f,0x3000b8,uint64_t(0x500000)); store(f,0x500000,uint64_t(0x600000));
 store(f,0x6000d0,int32_t(1)); store(f,0x600068,int32_t(6)); store(f,0x60006c,int32_t(10)); store(f,0x6000d8,uint64_t(0x700000)); store(f,0x700018,int32_t(30));
 store(f,0x700040,uint64_t(0x800000)); store(f,0x700028,uint64_t(0xa00000)); store(f,0x800018,int32_t(2100)); store(f,0x800010,uint64_t(0x900000)); store(f,0x900018,int32_t(2100)); store(f,0xa00018,int32_t(1050));
 for(int i=0;i<1050;++i) { store(f,0x900020+i*4,uint32_t(i==321?46:0)); store(f,0xa00020+i*4,int32_t(i==321?2:-1)); }
 void* state=create(&host,nullptr); assert(state); sample(state,&host); assert(f.ints["map.ready"]==1); assert(f.ints["map.floor"]==1); assert(!f.key.empty()); assert(image(state,&host,f.key.c_str(),&f,sink));
 auto pixel=[&](int x,int y,int c){return f.pixels[(y*560+x)*4+c];};
 assert(pixel(6*16+7,20*16+7,0)==255); // marked stair
 assert(pixel(6*16+7,19*16+7,1)==255); // player
 store(f,0x6000d0,int32_t(0)); sample(state,&host); assert(f.ints["map.ready"]==0);
 destroy(state);
}
