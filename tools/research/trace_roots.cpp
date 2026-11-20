#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <unordered_set>
#include <vector>
struct Edge { uint64_t address,value; };
struct Node { uint64_t address; int parent; uint64_t offset; };
int main(int argc,char**argv) {
 if(argc!=5) return 2;
 std::ifstream f(argv[1],std::ios::binary); std::vector<Edge> edges; Edge e{};
 while(f.read(reinterpret_cast<char*>(&e),sizeof(e))) edges.push_back(e);
 std::sort(edges.begin(),edges.end(),[](auto a,auto b){return a.value<b.value;});
 uint64_t target=strtoull(argv[2],nullptr,16), base=strtoull(argv[3],nullptr,16), width=strtoull(argv[4],nullptr,16);
 std::vector<Node> nodes{{target,-1,0}}; std::unordered_set<uint64_t> seen{target}; size_t lo=0,hi=1; unsigned roots=0;
 for(int depth=1;depth<=10 && lo<hi;depth++) {
  size_t end=hi;
  for(size_t i=lo;i<end;i++) {
   auto n=nodes[i];
   auto it=std::lower_bound(edges.begin(),edges.end(),n.address-width,[](auto a,uint64_t v){return a.value<v;});
   for(;it!=edges.end() && it->value<=n.address;++it) {
    if(!seen.insert(it->address).second)continue;
    nodes.push_back({it->address,int(i),n.address-it->value});
    if(it->address>=base && it->address<base+0x2000000) {
     printf("root main+%llX",(unsigned long long)(it->address-base));
     int at=int(nodes.size()-1);
     while(nodes[at].parent>=0) {printf(" -> +%llX",(unsigned long long)nodes[at].offset);at=nodes[at].parent;}
     printf("\n"); if(++roots>=40)return 0;
    }
   }
  }
  fprintf(stderr,"depth %d nodes %zu roots %u\n",depth,nodes.size(),roots); lo=end;hi=nodes.size();
  if(nodes.size()>3000000)break;
 }
}
