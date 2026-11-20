#include "010062E01FE0C000.cpp"
#include <cassert>
#include <cstdio>
#include <map>

struct Fixture {
    std::map<uint64_t,uint8_t> memory;
    std::map<std::string,std::string> text;
    EdenDsmodHostApi host{};
    bool mutate=false;
    uint64_t base=0x100000, root=0x300000, manager=0x400000, begin=0x500000;
    template<class T> void put(uint64_t a,T value) {
        auto* b=reinterpret_cast<const uint8_t*>(&value);
        for(size_t i=0;i<sizeof(T);++i) memory[a+i]=b[i];
    }
    void name(uint64_t a,const std::string& value) {
        for(size_t i=0;i<64;++i) memory[a+8+i]=i<value.size()?uint8_t(value[i]):0;
    }
    Fixture() {
        host.title_id=Title; host.main_base=base; host.userdata=this;
        for(size_t i=0;i<32;++i) host.build_id[i]=uint8_t(std::stoul(std::string(Build+i*2,2),nullptr,16));
        host.is_mapped=[](void* p,uint64_t a,uint64_t n)->EdenDsmodBool {
            auto& f=*static_cast<Fixture*>(p);
            for(uint64_t i=0;i<n;++i) if(!f.memory.count(a+i))return false;
            return true;
        };
        host.read_memory=[](void* p,uint64_t a,void* out,size_t n)->EdenDsmodBool {
            auto& f=*static_cast<Fixture*>(p); auto* b=static_cast<uint8_t*>(out);
            for(size_t i=0;i<n;++i) {auto it=f.memory.find(a+i);if(it==f.memory.end())return false;b[i]=it->second;}
            if(f.mutate && a==0x600008) f.put<uint64_t>(f.begin,0x700100);
            return true;
        };
        host.publish_text=[](void* p,const char* k,const char* v) {static_cast<Fixture*>(p)->text[k]=v;};
        setup();
    }
    void setup() {
        put<uint64_t>(base+0x146f500,root);put<uint64_t>(root+0x10,manager);
        put<uint64_t>(manager+8,begin);put<uint64_t>(manager+16,begin+48);
        for(unsigned i=0;i<6;++i) {
            put<uint64_t>(begin+i*8,0x700000+i*0x100);
            put<uint64_t>(0x700008+i*0x100,0x600000+i*0x100);
            name(0x600000+i*0x100,i==0?"Wanyamon":i==1?"Koromon":"Botamon");
        }
    }
    void run() {sample(nullptr,&host);}
};
int main() {
    assert(supports(Build)); assert(!supports(nullptr)); assert(!supports("wrong"));
    Fixture f; void* state=create(&f.host,nullptr); assert(state); destroy(state);
    f.run(); assert(f.text["party.0"]=="Wanyamon" && f.text["party.1"]=="Koromon" && f.text["party.2"]=="Botamon");
    f.put<uint64_t>(f.begin,0x700100);f.put<uint64_t>(f.begin+8,0x700000);f.run();assert(f.text["party.0"]=="Koromon");
    f.setup(); f.name(0x600100,"Wanyamon");f.run();assert(f.text["party.0"]==f.text["party.1"]);
    f.setup();f.put<uint64_t>(f.begin+8,0);f.run();assert(f.text["party.1"]=="Empty" && f.text["party.2"]=="Botamon");
    f.setup();f.put<uint64_t>(f.manager+16,f.begin+8);f.run();assert(f.text["party.1"]=="Empty");
    f.setup();f.put<uint64_t>(f.manager+16,f.begin+56);f.run();assert(f.text["party.0"]=="Unavailable");
    f.setup();f.put<uint64_t>(f.manager+16,f.begin-8);f.run();assert(f.text["party.0"]=="Unavailable");
    f.setup();f.put<uint64_t>(f.manager+16,f.begin+7);f.run();assert(f.text["party.0"]=="Unavailable");
    f.setup();f.put<uint64_t>(f.begin,0xfffffffffffffff8ULL);f.run();assert(f.text["party.0"]=="Unavailable");
    f.setup();f.name(0x600000,std::string(64,'a'));f.run();assert(f.text["party.0"]=="Unavailable");
    f.setup();f.memory[0x600008]=1;f.run();assert(f.text["party.0"]=="Unavailable");
    f.setup();f.mutate=true;f.run();assert(f.text["party.0"]=="Unavailable");f.mutate=false;
    f.setup();f.memory.erase(f.base+0x146f500);f.run();assert(f.text["party.0"]=="Unavailable");
    f.setup();f.host.title_id=0;assert(!create(&f.host,nullptr));f.run();assert(f.text["party.0"]=="Unavailable");
    f.host.title_id=Title;f.host.build_id[0]^=1;assert(!create(&f.host,nullptr));f.run();assert(f.text["party.0"]=="Unavailable");
    Fixture relocated;relocated.base+=0x10000;relocated.root+=0x10000;relocated.manager+=0x10000;relocated.begin+=0x10000;relocated.setup();relocated.run();assert(relocated.text["party.0"]=="Wanyamon");
    std::puts("party tests passed");
}
