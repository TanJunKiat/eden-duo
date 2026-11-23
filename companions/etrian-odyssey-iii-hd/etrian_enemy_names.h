// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace etrian {
// Use one byte per glyph, matching the Abyss Glass Windows-1252 atlas.
inline unsigned char enemy_name_glyph(uint16_t c) {
    if (c==0x00a0 || c==0x202f) return ' ';
    if ((c>=32 && c<=126) || (c>=161 && c<=255)) return static_cast<unsigned char>(c);
    constexpr std::array<uint16_t,32> extended{
        0x20ac,0,0x201a,0x0192,0x201e,0x2026,0x2020,0x2021,
        0x02c6,0x2030,0x0160,0x2039,0x0152,0,0x017d,0,
        0,0x2018,0x2019,0x201c,0x201d,0x2022,0x2013,0x2014,
        0x02dc,0x2122,0x0161,0x203a,0x0153,0,0x017e,0x0178};
    for (unsigned i=0;c && i<extended.size();++i)
        if (extended[i]==c) return static_cast<unsigned char>(128+i);
    return 0;
}

class EnemyLabels {
public:
    template<class Reader>
    EnemyLabels(const Reader& memory,uint64_t base) {
        // SingletonMonoBehaviour<TextLabelDataManager>.get_Instance(), then its
        // loaded Dictionary<string,TextLabelData> and GameLanguageManager.
        uint64_t manager=base,lang=0;
        for (uint64_t offset:{0x61a6208,0x18,0xc0,0,0xb8,0})
            if (!read(memory,manager,offset,manager) || !manager) return;
        if (!read(memory,manager,0x48,lang) || !read(memory,lang,0x10,language) ||
            language<0 || language>4 || !read(memory,manager,0x20,dictionary) ||
            !read(memory,dictionary,0x10,buckets) || !read(memory,dictionary,0x18,entries) ||
            !read(memory,dictionary,0x20,count) || !read(memory,dictionary,0x24,version) ||
            !read(memory,buckets,0x18,bucket_count) || !read(memory,entries,0x18,entry_capacity) ||
            count<=0 || uint64_t(count)>entry_capacity || !bucket_count ||
            bucket_count!=entry_capacity || entry_capacity>65536) return;
        ready=true;
    }

    template<class Reader>
    std::string lookup(const Reader& memory,int16_t id) const {
        return id<0?std::string{}:lookup_key(memory,"enemyname_"+std::to_string(id));
    }

    template<class Reader>
    std::string material(const Reader& memory,uint16_t item) const {
        // GetUseItemNameId subtracts the material base from the global item ID.
        if(item<925 || item>=0x63f) return {};
        return lookup_key(memory,"useitemname_"+std::to_string(item-924));
    }

    template<class Reader>
    std::string quest(const Reader& memory,bool mission,unsigned id,bool explain=false) const {
        if (id>9999) return {};
        const auto suffix=std::to_string(10000+id).substr(1);
        return lookup_key(memory,std::string(mission?(explain?"mission_explain.msg":"mission_name"):
                              (explain?"quest_explain":"quest_name"))+suffix,explain);
    }

private:
    template<class Reader>
    std::string lookup_key(const Reader& memory,std::string_view key,bool paragraph=false) const {
        if (!ready) return {};
        const uint32_t hash=string_hash(key);
        int32_t index=-1;
        if (!read(memory,buckets,0x20+4*(hash%bucket_count),index)) return {};
        for (int depth=0;depth<32 && index>=0 && index<count;++depth) {
            const uint64_t offset=0x20+0x70*uint64_t(index);
            uint32_t stored_hash=0;
            if (!read(memory,entries,offset,stored_hash) || stored_hash>0x7fffffffu) break;
            if (stored_hash==hash) {
                uint64_t label=0;
                if (!read(memory,entries,offset+8,label)) break;
                if (read_string(memory,label)==key) {
                    uint64_t value=0;
                    if (!read(memory,entries,offset+0x48+8*language,value)) return {};
                    auto name=paragraph?read_string<640>(memory,value,true):read_string(memory,value);
                    int32_t current_version=0;
                    if (name.find_first_not_of(" \xa0")==std::string::npos || name.front()=='<' ||
                        !read(memory,dictionary,0x24,current_version) || current_version!=version) return {};
                    return name;
                }
            }
            int32_t next=-1;
            if (!read(memory,entries,offset+4,next) || next==index) break;
            index=next;
        }
        return {};
    }

    template<class Reader,class T>
    static bool read(const Reader& memory,uint64_t object,uint64_t offset,T& value) {
        return object && object<=UINT64_MAX-offset-sizeof(T) && memory(object+offset,&value,sizeof(T));
    }

    template<size_t Limit=80,class Reader>
    static std::string read_string(const Reader& memory,uint64_t object,bool paragraph=false) {
        std::array<uint16_t,Limit> text{};
        int32_t length=0;
        if (!read(memory,object,0x10,length) || length<1 || size_t(length)>text.size() ||
            object>UINT64_MAX-0x14-length*2 || !memory(object+0x14,text.data(),length*2)) return {};
        std::string result;
        result.reserve(length);
        for (int32_t i=0;i<length;++i) {
            if (paragraph && text[i]=='<') {
                int32_t end=i+1;
                while (end<length && text[end]!='>' && end-i<64) ++end;
                if (end>=length || text[end]!='>') return {};
                if (end==i+3 && text[i+1]=='b' && text[i+2]=='r') result+=' ';
                i=end;
                continue;
            }
            const auto glyph=paragraph && (text[i]=='\n' || text[i]=='\r')?' ':enemy_name_glyph(text[i]);
            if (!glyph) return {};
            result+=char(glyph);
        }
        return result;
    }

    // This build's managed String.GetHashCode uses two interleaved DJB2-XOR accumulators.
    // Hashes find candidate entries only; the full key must still match.
    static uint32_t string_hash(std::string_view key) {
        std::array<uint32_t,2> hash{5381,5381};
        for (size_t i=0;i<key.size();++i) hash[i%2]=(hash[i%2]*33)^uint8_t(key[i]);
        return (hash[0]+hash[1]*0x5d588b65u)&0x7fffffffu;
    }

    uint64_t dictionary=0,buckets=0,entries=0,bucket_count=0,entry_capacity=0;
    int32_t language=-1,count=0,version=0;
    bool ready=false;
};
} // namespace etrian
