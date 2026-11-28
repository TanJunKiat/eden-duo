// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "etrian_art.h"
#include "etrian_battle.h"
#include "etrian_exploration.h"
#include "etrian_expedition.h"
#include "etrian_font_data.h"
#include "etrian_quests.h"
#include "etrian_town.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace etrian::glass {
using Color = uint32_t; // RGB; output is always opaque RGBA.
inline constexpr Color bg=0x090f21, deep=0x0b1429, card=0x121f3a, border=0x283d60,
    accent=0x91e3ff, ink=0xf1f8ff, muted=0xa5b8d0, hp_color=0x68e0b8,
    tp_color=0x90acff, danger=0xff9eab;

class Canvas {
public:
    static constexpr int width=592, height=528;
    std::vector<uint8_t> pixels=std::vector<uint8_t>(width*height*4);

    void put(int x,int y,Color color,unsigned alpha=255) {
        if(x<0 || y<0 || x>=width || y>=height || !alpha) return;
        auto* dst=&pixels[(y*width+x)*4];
        for(int c=0;c<3;++c) {
            const unsigned src=(color>>(16-8*c))&255;
            dst[c]=uint8_t((src*alpha+dst[c]*(255-alpha)+127)/255);
        }
        dst[3]=255;
    }
    void box(int x,int y,int w,int h,Color color) {
        for(int yy=std::max(0,y);yy<std::min(height,y+h);++yy)
            for(int xx=std::max(0,x);xx<std::min(width,x+w);++xx) put(xx,yy,color);
    }
    void round(int x,int y,int w,int h,int r,Color color) {
        r=std::min(std::max(0,r),std::min(w,h)/2);
        for(int yy=0;yy<h;++yy) for(int xx=0;xx<w;++xx) {
            const int dx=std::max({r-xx-1,xx-(w-r),0});
            const int dy=std::max({r-yy-1,yy-(h-r),0});
            if(dx*dx+dy*dy<=r*r) put(x+xx,y+yy,color);
        }
    }
    void frame(int x,int y,int w,int h,int r,Color fill,Color edge=border) {
        round(x,y,w,h,r,edge);
        round(x+1,y+1,w-2,h-2,std::max(0,r-1),fill);
    }
    void circle(int cx,int cy,int r,Color color) {
        for(int y=-r;y<=r;++y) for(int x=-r;x<=r;++x)
            if(x*x+y*y<=r*r) put(cx+x,cy+y,color);
    }
    void ring(int cx,int cy,int rx,int ry,Color color) {
        for(int x=-rx;x<=rx;++x) {
            const int y=int(std::lround(ry*std::sqrt(std::max(0.0,1.0-double(x*x)/(rx*rx)))));
            put(cx+x,cy-y,color,90); put(cx+x,cy+y,color,90);
        }
    }
    // Game-derived enemy labels use Windows-1252; existing ASCII labels are unchanged.
    static_assert(sizeof(glyphs)/sizeof(glyphs[0])==256-32);
    static const Glyph& glyph(unsigned char c) { return glyphs[(c>=32 && c!=127?c:'?')-32]; }
    static int measure(std::string_view s,int size) {
        int adv=0;
        for(unsigned char c:s) adv+=glyph(c).advance16;
        return (adv*size+font_size*8)/(font_size*16);
    }
    void text(int x,int baseline,std::string_view s,int size,Color color) {
        int offset=0;
        for(unsigned char c:s) {
            const auto& g=glyph(c);
            const auto scale=[&](int n) { return int(std::lround(double(n)*size/font_size)); };
            const int ox=x+(offset*size+font_size*8)/(font_size*16)+scale(g.left);
            const int oy=baseline+scale(g.top);
            if(g.width && g.height) {
                const int w=std::max(1,scale(g.width)), h=std::max(1,scale(g.height));
                for(int yy=0;yy<h;++yy) for(int xx=0;xx<w;++xx) {
                    unsigned alpha=0;
                    for(int qy:{1,3}) for(int qx:{1,3}) {
                        const int sx=(xx*4+qx)*g.width/(w*4), sy=(yy*4+qy)*g.height/(h*4);
                        const int pos=sy*g.width+sx;
                        const unsigned packed=static_cast<unsigned char>(glyph_alpha[g.offset+pos/2]);
                        alpha+=(pos%2?packed&15:packed>>4)*17;
                    }
                    put(ox+xx,oy+yy,color,(alpha+2)/4);
                }
            }
            offset+=g.advance16;
        }
    }
    void right(int x,int y,std::string_view s,int size,Color color) { text(x-measure(s,size),y,s,size,color); }
    void center(int x,int y,std::string_view s,int size,Color color) { text(x-measure(s,size)/2,y,s,size,color); }
    void fit(int x,int y,std::string_view s,int size,int max_width,Color color) {
        if(measure(s,size)<=max_width) { text(x,y,s,size,color); return; }
        const auto dots="...";
        while(!s.empty() && measure(s,size)+measure(dots,size)>max_width) s.remove_suffix(1);
        text(x,y,std::string(s)+dots,size,color);
    }
    void icon(int x,int y,int size,const IconPixels& data,int direction=0,unsigned opacity=255) {
        for(int yy=0;yy<size;++yy) for(int xx=0;xx<size;++xx) {
            int sx=xx,sy=yy;
            if(direction==1) { sx=yy; sy=size-1-xx; }
            else if(direction==2) { sx=size-1-xx; sy=size-1-yy; }
            else if(direction==3) { sx=size-1-yy; sy=xx; }
            const auto i=((53-sy*54/size)*54+sx*54/size)*4;
            put(x+xx,y+yy,(Color(data[i])<<16)|(Color(data[i+1])<<8)|data[i+2],data[i+3]*opacity/255);
        }
    }
};

inline Color tint(Color a,Color b,int t,int limit) {
    Color result=0;
    for(int shift:{16,8,0}) result|=Color((int((a>>shift)&255)*(limit-t)+int((b>>shift)&255)*t)/limit)<<shift;
    return result;
}
inline void badge(Canvas& c,int x,int y,int w,std::string_view text,Color color=muted) {
    c.frame(x,y,w,23,11,deep); c.center(x+w/2,y+15,text,10,color);
}
inline void header(Canvas& c,std::string_view section,std::string_view title,std::string_view tag) {
    c.box(0,0,Canvas::width,Canvas::height,bg);
    for(int y=0;y<96;++y) c.box(0,y,Canvas::width,1,tint(card,bg,y,96));
    c.text(24,25,section,11,accent); c.text(24,60,title,32,ink);
    if(!tag.empty()) badge(c,442,32,126,tag);
    c.box(24,76,544,1,border);
}
inline void footer(Canvas& c,std::string_view left,std::string_view right) {
    c.box(24,500,544,1,border); c.text(24,518,left,11,muted); c.right(568,518,right,11,muted);
}
inline void message(Canvas& c,std::string_view title,std::string_view detail) {
    c.frame(24,112,544,108,14,card);
    c.text(44,153,title,22,ink); c.text(44,185,detail,14,muted);
}
inline void town_report_line(Canvas& c,const QuestEntry& entry,int x,int y,int w,int size) {
    const auto location=quest_hint(entry.key).turn_in;
    const auto destination=location.empty()?std::string_view{"Location unavailable"}:location;
    c.fit(x,y,entry.name,size,w-149,ink);
    c.right(x+w,y,destination,12,location.empty()?muted:accent);
}
inline void town(Canvas& c,const TownSnapshot& town,std::string_view tag="ACTIVE PARTY") {
    header(c,"01  /  TOWN","Armoroad",tag);
    if(!town.party) {
        message(c,"Party data unavailable","The companion will retry when the roster is readable.");
    } else if(town.party->empty()) {
        message(c,"No active party","Form an active party at the Explorers Guild.");
    } else {
        const bool formed=town.formation && town.formation->size()==town.party->size();
        c.text(24,101,formed?"FRONT ROW":"FORMATION UNAVAILABLE",12,formed?accent:muted);
        c.right(568,101,std::to_string(town.party->size())+" ACTIVE",11,hp_color);
        if(formed) c.text(24,225,"BACK ROW",12,accent);
        if(formed && town.formation->front()>=3) c.text(32,147,"No one in front",14,muted);
        if(formed && town.formation->back()<3) c.text(32,271,"No one in back",14,muted);
        for(size_t i=0;i<town.party->size() && i<5;++i) {
            const int slot=formed?(*town.formation)[i]:int(i);
            const int x=24+(slot%3)*184,y=(slot<3?114:238);
            const auto& m=(*town.party)[i];
            const bool ko=m.hp==0, low=m.hp>0 && m.tp<=5;
            constexpr Color caution=0xffd594;
            c.frame(x,y,176,86,10,card,ko?danger:low?caution:border);
            c.fit(x+12,y+26,m.name,21,ko?107:152,ink);
            if(ko) badge(c,x+126,y+7,39,"KO",danger);
            c.box(x+12,y+35,152,1,border);
            c.text(x+12,y+53,"HP",11,ko?danger:hp_color);
            c.text(x+92,y+53,low?"LOW TP":"TP",11,low?caution:tp_color);
            const auto value=[&](int offset,int current,int maximum,Color color) {
                auto label=std::to_string(current);
                if(maximum>0) label+="/"+std::to_string(maximum);
                int size=23;
                while(size>11 && c.measure(label,size)>72) --size;
                c.text(x+offset,y+77,label,size,color);
            };
            value(12,m.hp,m.max_hp,ko?danger:hp_color);
            value(92,m.tp,m.max_tp,low?caution:tp_color);
        }
    }
    c.frame(24,340,544,48,9,card);
    c.text(38,370,"Free item slots",15,muted);
    c.right(269,370,town.bag_free?std::to_string(*town.bag_free):"Unavailable",town.bag_free?21:14,ink);
    c.box(295,349,1,30,border);
    c.text(313,370,"Ariadne Threads",15,muted);
    c.right(553,370,town.threads?std::to_string(*town.threads):"Unavailable",town.threads?21:14,ink);
    c.text(24,416,"READY TO REPORT",12,accent);
    if(town.source) {
        c.frame(404,396,164,28,8,card);
        c.center(486,415,"Quest Board >",14,hp_color);
    }
    const auto& reports=town.quests.report;
    if(!reports || reports->empty()) {
        c.text(24,453,reports?"No quests or missions ready to report":"Turn-ins unavailable",16,muted);
    } else {
        const auto n=reports->size();
        for(size_t i=0;i<std::min(n,town_report_preview);++i) {
            const int y=448+int(i)*32;
            c.frame(24,y-21,544,29,7,deep);
            town_report_line(c,(*reports)[i],36,y,520,16);
        }
    }
    footer(c,"Departure checklist","Current HP and TP");
}
inline void town_board(Canvas& c,const TownQuestBoard& board,TownQuestTab tab,size_t page) {
    header(c,"01  /  TOWN","Quest Board","");
    c.frame(440,29,128,33,8,card);
    c.center(504,50,"< Armoroad",14,accent);
    constexpr std::array labels{"Available","Accepted","Ready to report"};
    constexpr std::array tabs{TownQuestTab::available,TownQuestTab::accepted,TownQuestTab::report};
    for(size_t i=0;i<tabs.size();++i) {
        const int x=24+184*int(i);
        const bool selected=tabs[i]==tab;
        c.frame(x,91,176,34,8,card,selected?accent:border);
        c.center(x+88,113,labels[i],14,selected?accent:ink);
    }
    const auto& list=town_quest_list(board,tab);
    if(!list || list->empty()) {
        c.frame(24,140,544,78,10,card);
        c.text(38,178,!list?"Quest data unavailable":tab==TownQuestTab::available?"No posted quests":
               tab==TownQuestTab::accepted?"No accepted quests or missions":"No quests or missions ready to report",17,muted);
    } else {
        const size_t first=page*town_quest_page_size;
        for(size_t i=first;i<list->size() && i<first+town_quest_page_size;++i) {
            const int y=140+int(i-first)*44;
            const auto& quest=(*list)[i];
            c.frame(24,y,544,39,8,card);
            if(tab==TownQuestTab::report) town_report_line(c,quest,38,y+25,516,16);
            else {
                c.fit(38,y+25,quest.name,16,420,ink);
                c.right(554,y+25,quest.key.mission?"MISSION":"QUEST",11,accent);
            }
        }
        if(page>0) { c.frame(24,459,114,34,8,card); c.center(81,481,"< Previous",14,accent); }
        if(first+town_quest_page_size<list->size()) {
            c.frame(454,459,114,34,8,card); c.center(511,481,"Next >",14,accent);
        }
        c.center(296,481,std::to_string(page+1)+" / "+std::to_string((list->size()+town_quest_page_size-1)/town_quest_page_size),14,muted);
    }
    footer(c,"Armoroad Quest Board","Read-only companion");
}

inline void guild_roster(Canvas& c,const GuildRoster& roster,size_t page) {
    header(c,"01  /  TOWN","Explorers Guild","");
    if(!roster || roster->empty()) message(c,roster?"No registered members":"Roster unavailable",
        roster?"Register a member in the game.":"The companion will retry when the guild is readable.");
    else {
        c.text(36,101,"NAME",11,accent); c.text(282,101,"JOB",11,accent); c.text(441,101,"LEVEL",11,accent);
        const size_t first=page*guild_page_size;
        for(size_t i=first;i<roster->size() && i<first+guild_page_size;++i) {
            const auto& member=(*roster)[i];
            const int y=110+int(i-first)*42;
            c.frame(24,y,544,36,8,card);
            c.fit(36,y+23,member.name,16,236,ink);
            c.fit(282,y+23,member.job,14,145,muted);
            c.text(445,y+23,std::to_string(member.level),15,ink);
            c.right(554,y+23,member.active?"ACTIVE":"RESERVE",11,member.active?accent:muted);
        }
        if(page) { c.frame(24,459,114,34,8,card); c.center(81,481,"< Previous",14,accent); }
        if(first+guild_page_size<roster->size()) { c.frame(454,459,114,34,8,card); c.center(511,481,"Next >",14,accent); }
        c.center(296,481,std::to_string(page+1)+" / "+std::to_string((roster->size()+guild_page_size-1)/guild_page_size),14,muted);
    }
    footer(c,"Registered members","Read-only companion");
}
inline void senate_missions(Canvas& c,const TownQuestBoard& board) {
    header(c,"01  /  TOWN","Via Senatus","");
    size_t count=0;
    for(const auto* list:{&board.report,&board.accepted}) {
        if(!*list) continue;
        for(const auto& entry:**list) {
            if(!entry.key.mission || count>=8) continue;
            const int y=99+int(count++)*44;
            c.frame(24,y,544,38,8,card);
            c.fit(36,y+25,entry.name,16,372,ink);
            c.right(554,y+24,list==&board.report?"READY TO REPORT":"IN PROGRESS",11,accent);
        }
    }
    if(!count) message(c,board.report && board.accepted?"No active main missions":"Mission status unavailable",
        "Accept and report main missions on the game screen.");
    footer(c,"Main missions","Read-only companion");
}
inline void bar(Canvas& c,int x,int y,int w,int value,int max,Color color) {
    if(w<2 || value<0 || max<=0 || value>max) return;
    c.round(x,y,w,4,2,deep);
    const int fill=int(int64_t(w)*value/max);
    if(fill) c.round(x,y,fill,4,2,color);
}
inline void stat(Canvas& c,int x,int baseline,std::string_view label,int value,int max,Color color) {
    std::string amount=value<0?"--":std::to_string(value);
    if(value>=0 && max>0) amount+=" / "+std::to_string(max);
    c.text(x+10,baseline,label,10,color);
    const int metrics=Canvas::measure(amount,12);
    c.right(x+160,baseline,amount,12,ink);
    bar(c,x+33,baseline-5,std::min(56,160-38-metrics),value,max,color);
}
inline void battle_card(Canvas& c,const Combatant& m,int x,int y,int height,bool ally) {
    constexpr int w=170;
    const bool compact=height<65, ko=m.hp==0;
    c.frame(x,y,w,height,compact?6:9,card,ko?danger:border);
    if(ally) c.round(x+2,y+2,w-4,compact?18:25,5,0x1c3152);
    const int name_y=y+(compact?16:21);
    c.fit(x+10,name_y,m.name,compact?13:17,ko?112:150,ink);
    if(ko) c.right(x+w-10,name_y,"KO",10,danger);
    const bool show_timer=ally && !ko && m.guard_order.has_value();
    const int hp_y=y+(compact?(show_timer?28:34):43);
    stat(c,x,hp_y,"HP",m.hp,m.max_hp,(!ally||ko)?danger:hp_color);
    if(ally) stat(c,x,hp_y+(compact?(show_timer?11:15):18),"TP",m.tp,m.max_tp,tp_color);
    if(show_timer) {
        const auto& timer=*m.guard_order;
        const int line=y+(compact?50:79);
        const auto duration=std::to_string(timer.turns)+(timer.turns==1?" turn":" turns");
        c.fit(x+10,line,timer.name,compact?10:12,150-8-Canvas::measure(duration,10),accent);
        c.right(x+w-10,line,duration,10,accent);
    }
}
inline void battle(Canvas& c,BattleMode mode,const BattleSnapshot& snapshot) {
    if(mode==BattleMode::unknown) {
        header(c,"EO III  /  COMPANION","Panel unavailable","");
        message(c,"No supported scene","Open Armoroad, the labyrinth, or a battle.");
        return;
    }
    if(mode!=BattleMode::battle) {
        header(c,"03  /  BATTLE","Encounter","");
        message(c,"Battle data unavailable","Waiting for a readable roster.");
        return;
    }
    header(c,"03  /  BATTLE","Encounter",std::to_string(snapshot.enemies.size())+
           (snapshot.enemies.size()==1?" ENEMY":" ENEMIES"));
    c.frame(24,93,544,176,12,deep);
    for(int rx:{236,184,132}) c.ring(296,211,rx,rx/6,0x395b88);
    c.text(39,115,"ENEMY LINE",11,danger);
    c.text(24,290,"PARTY",11,hp_color);
    c.box(75,286,493,1,border);
    const auto side=[&](const auto& members,bool ally) {
        const int n=int(std::min<size_t>(members.size(),8)), rows=(n+2)/3;
        const int height=ally?(rows>2?55:91):(rows>2?43:rows==2?65:97);
        const int top=ally?(rows==1?396:304):(rows>2?125:rows==2?126:153);
        const int stride=ally?(rows>2?60:92):(rows>2?45:68);
        for(int i=0;i<n;++i) {
            const int row=i/3,col=i%3,cols=std::min(3,n-row*3);
            const int x=(Canvas::width-(cols*170+(cols-1)*10))/2+180*col,y=top+row*stride;
            if(!ally && rows==1) {
                c.circle(x+85,y-15,10,border); c.circle(x+85,y-15,9,deep);
                for(int dy=-5;dy<=5;++dy) for(int dx=-5;dx<=5;++dx)
                    if(std::abs(dx)+std::abs(dy)<=5) c.put(x+85+dx,y-15+dy,danger);
            }
            battle_card(c,members[i],x,y,height,ally);
        }
    };
    side(snapshot.enemies,false); side(snapshot.allies,true);
    footer(c,"Timer trial: Guard Order only","Roster order");
}

inline constexpr int map_scale=13,map_left=68,map_top=107;
inline Color tile_fill(uint32_t flags) {
    if(!(flags&8)) return deep;
    if(flags&128) return 0x7d4a55;
    if(flags&64) return 0x6e5b39;
    return 0x274868;
}
inline void fallback_icon(Canvas& c,int x,int y,int icon) {
    if(icon<0) return;
    constexpr Color gold=0xffdd77;
    const Color color=icon==5 || icon==6?0xffa261:icon>=8 && icon<=10?0x96ef9b:icon==18?danger:gold;
    c.round(x+1,y+1,11,11,2,deep);
    if(icon==2 || icon==3) {
        for(int n=0;n<4;++n) c.box(x+3+n*2,y+3+(icon==2?3-n:n)*2,3,2,gold);
    } else if(icon==17 || icon==20 || icon==24) {
        c.frame(x+2,y+3,9,7,1,0x6e5b39,icon==24?0xb3a684:gold);
        c.box(x+3,y+6,7,1,gold); c.box(x+6,y+5,1,3,gold);
    } else {
        constexpr char symbols[]="DD^vM!!?TTT^v<>^>TFOTOXAT^>M";
        const char marker=static_cast<size_t>(icon)<sizeof(symbols)-1?symbols[icon]:'?';
        c.center(x+6,y+10,std::string_view(&marker,1),10,color);
    }
}
inline void hint_marker(Canvas& c,int x,int y,HintIcon kind,Color bg_color,
                        const std::array<IconPixels,42>& artwork,const std::array<bool,42>& ok) {
    int map_icon=-1;
    char symbol='?';
    using enum HintIcon;
    switch(kind) {
    case none: return;
    case pit: map_icon=19; symbol='O'; break;
    case north: map_icon=11; symbol='^'; break;
    case south: map_icon=12; symbol='v'; break;
    case west: map_icon=13; symbol='<'; break;
    case east: map_icon=14; symbol='>'; break;
    case camp: map_icon=23; symbol='A'; break;
    case door: map_icon=0; symbol='D'; break;
    case save: symbol='S'; break;
    case stairs_up: map_icon=2; symbol='^'; break;
    case stairs_down: map_icon=3; symbol='v'; break;
    case treasure: map_icon=17; symbol='T'; break;
    case shortcut: symbol='H'; break;
    case switch_wall: symbol='W'; break;
    case flag_door: map_icon=1; symbol='D'; break;
    case warp: symbol='@'; break;
    case switch_plate: symbol='P'; break;
    case harvest: map_icon=9; symbol='H'; break;
    case mine: map_icon=10; symbol='M'; break;
    case take: map_icon=8; symbol='G'; break;
    case event: map_icon=5; symbol='!'; break;
    }
    constexpr unsigned opacity=145;
    const int art=art_for_map_icon(map_icon);
    if(art && ok[art]) c.icon(x+1,y+1,11,artwork[art],0,opacity);
    else {
        const Color color=tint(bg_color,0xffdd77,opacity,255);
        if(kind==treasure) {
            c.frame(x+2,y+3,9,7,1,bg_color,color);
            c.box(x+3,y+6,7,1,color); c.box(x+6,y+5,1,3,color);
        } else if(kind==shortcut) {
            c.box(x+2,y+6,9,1,color);
            for(int i=0;i<3;++i) {
                c.put(x+2+i,y+6-i,color); c.put(x+2+i,y+6+i,color);
                c.put(x+10-i,y+6-i,color); c.put(x+10-i,y+6+i,color);
            }
        } else c.center(x+6,y+10,std::string_view(&symbol,1),10,color);
    }
}
inline void exploration_tab(Canvas& c,int x,int w,std::string_view label,bool active) {
    c.frame(x,47,w,31,5,active?0x234050:deep);
    c.center(x+w/2,68,label,12,active?accent:muted);
}
inline std::string quest_floors(uint32_t mask) {
    std::string text;
    for(int floor=1;floor<=30;++floor) if(mask&quest_floor(floor)) {
        if(!text.empty()) text+=" / ";
        text+="B"+std::to_string(floor)+"F";
    }
    return text;
}
inline void quest_header(Canvas& c,int floor,std::string_view button) {
    header(c,"02  /  DUNGEON","Quests","");
    badge(c,422,15,126,"FLOOR  B"+std::to_string(floor)+"F");
    exploration_tab(c,445,103,button,false);
}
inline void quest_list(Canvas& c,int floor,const QuestJournal& journal,size_t page) {
    quest_header(c,floor,"Back to Map");
    if(!journal.source) { message(c,"Quest data unavailable","Open the in-game journal if you need to check now."); return; }
    if(journal.entries.empty()) { message(c,"No active quests","Accepted side quests and main missions appear here."); return; }
    c.text(44,107,"ACTIVE JOURNAL",12,accent);
    const auto first=page*quest_page_size;
    for(size_t i=first;i<std::min(first+quest_page_size,journal.entries.size());++i) {
        const auto& q=journal.entries[i];
        const auto hint=quest_hint(q.key);
        const bool here=q.status==QuestStatus::active && (hint.floors&quest_floor(floor));
        const int y=116+66*int(i-first);
        c.frame(44,y,504,59,7,card,here?0x406c74:border);
        c.text(58,y+16,q.key.mission?"MAIN MISSION":"SIDE QUEST",10,here?hp_color:muted);
        c.fit(58,y+39,q.name,18,310,ink);
        const auto floors=quest_floors(hint.floors);
        c.right(532,y+18,q.status==QuestStatus::report?"Ready to report":here?"On this floor":"In progress",12,here?hp_color:accent);
        if(!floors.empty()) c.right(532,y+38,floors,11,muted);
        const auto summary=q.status==QuestStatus::report?hint.turn_in:hint.action;
        if(!summary.empty()) c.fit(58,y+53,summary,11,463,muted);
    }
    if(page) { c.frame(44,463,110,31,5,deep); c.center(99,484,"Previous",12,accent); }
    if(first+quest_page_size<journal.entries.size()) { c.frame(438,463,110,31,5,deep); c.center(493,484,"Next",12,accent); }
    c.center(296,484,std::to_string(page+1)+" / "+std::to_string((journal.entries.size()+quest_page_size-1)/quest_page_size),12,muted);
    footer(c,"Tap a quest for actions, locations and items.","");
}
inline void quest_wrap(Canvas& c,int x,int y,std::string_view text,int max_lines) {
    constexpr int width=464, size=16;
    while(!text.empty() && max_lines-->0) {
        if(max_lines==0 || Canvas::measure(text,size)<=width) { c.fit(x,y,text,size,width,ink); return; }
        size_t end=0, space=0;
        while(end<text.size() && Canvas::measure(text.substr(0,end+1),size)<=width) {
            if(text[end]==' ') space=end;
            ++end;
        }
        if(space) end=space;
        c.text(x,y,text.substr(0,end),size,ink);
        text.remove_prefix(end);
        while(!text.empty() && text.front()==' ') text.remove_prefix(1);
        y+=22;
    }
}
inline void quest_details(Canvas& c,int floor,const QuestEntry& q) {
    quest_header(c,floor,"Back to List");
    const auto hint=quest_hint(q.key);
    c.frame(44,95,504,401,8,deep);
    c.text(62,119,q.key.mission?"MAIN MISSION":"SIDE QUEST",11,accent);
    c.fit(62,149,q.name,24,464,ink);
    auto floors=quest_floors(hint.floors);
    if(floors.empty()) floors=hint.action.empty()?"Location not verified":"No dungeon floor listed";
    c.text(62,177,floors,14,muted);
    c.right(530,177,q.status==QuestStatus::report?"Ready to report":"In progress",14,hp_color);
    c.text(62,213,hint.action.empty()?"JOURNAL OBJECTIVE":"WHAT TO DO",11,accent);
    const auto objective=hint.action.empty()?std::string_view(q.description):hint.action;
    quest_wrap(c,62,238,objective.empty()?"Objective unavailable":objective,q.items.empty()?6:3);
    if(!q.items.empty()) {
        c.text(62,311,"REQUIRED ITEMS",11,accent);
        for(size_t i=0;i<q.items.size();++i) {
            const auto& item=q.items[i];
            const int y=321+32*int(i);
            c.frame(62,y,468,29,5,card);
            c.fit(72,y+20,item.name,15,270,ink);
            c.right(519,y+20,(item.in_bag?std::to_string(*item.in_bag)+" / ":"Bag unavailable / ")+
                    std::to_string(item.required)+(item.in_bag?" in bag":" required"),13,muted);
        }
        c.text(62,438,"Bag counts are not quest credit.",13,muted);
    }
    c.text(62,474,"REPORT TO",11,accent);
    c.fit(153,474,hint.turn_in.empty()?"Check the in-game journal":hint.turn_in,15,370,ink);
    footer(c,"Live journal status","Objective counters not verified");
}
inline void floor_guide(Canvas& c,int floor,const FloorMonsters& monsters,size_t page) {
    header(c,"02  /  DUNGEON","Floor Guide","");
    badge(c,422,15,126,"FLOOR  B"+std::to_string(floor)+"F");
    exploration_tab(c,445,103,"Back to Map",false);
    c.frame(44,100,504,377,7,deep);
    c.text(62,136,"Standard random encounters",22,accent);
    if(!monsters || monsters->empty()) {
        c.text(62,171,monsters?"No standard encounters":"Encounter data unavailable",16,ink);
    } else {
        const auto first=page*guide_page_size;
        for(size_t i=0;i<guide_page_size && first+i<monsters->size();++i) {
            const int x=62+248*(i%2),y=169+44*(i/2);
            c.frame(x,y,220,34,5,card);
            c.fit(x+10,y+23,(*monsters)[first+i].name,16,181,ink);
            c.text(x+199,y+23,">",14,accent);
        }
        const auto pages=(monsters->size()+guide_page_size-1)/guide_page_size;
        if(pages>1) {
            if(page>0) c.frame(62,432,106,31,5,card),c.center(115,452,"Previous",13,ink);
            if(page+1<pages) c.frame(424,432,106,31,5,card),c.center(477,452,"Next",13,ink);
            c.center(296,452,std::to_string(page+1)+" / "+std::to_string(pages),14,muted);
        } else c.text(62,451,"Tap a monster to see its drops.",14,muted);
    }
    c.text(62,498,"Special encounter variants are not listed.",14,muted);
}
inline void monster_details(Canvas& c,int floor,const FloorMonster& monster,const DropList& drops) {
    header(c,"02  /  DUNGEON","Floor Guide","");
    badge(c,422,15,126,"FLOOR  B"+std::to_string(floor)+"F");
    exploration_tab(c,445,103,"Back to List",false);
    c.frame(44,100,504,377,7,deep);
    c.fit(62,144,monster.name,26,464,ink);
    c.text(62,173,"ITEM DROPS",13,accent);
    if(!drops || drops->empty()) {
        c.text(62,219,drops?"No item drops":"Drop data unavailable",17,muted);
        return;
    }
    for(size_t i=0;i<drops->size();++i) {
        const auto& d=(*drops)[i];
        const int y=193+73*int(i);
        c.frame(62,y,468,62,5,card);
        c.fit(76,y+26,d.name,19,427,ink);
        const std::string note=d.conditional?"Conditional drop - requirement and chance unverified":
            d.base_chance?"Base chance: "+std::to_string(*d.base_chance)+"%":"Chance unverified";
        c.fit(76,y+48,note,14,427,d.conditional?muted:accent);
    }
    c.text(62,449,"Base chances are per defeated monster, before modifiers.",13,muted);
}
inline std::string expedition_count(std::optional<unsigned> n) { return n?std::to_string(*n):"-"; }
inline void expedition_details(Canvas& c,int floor,const ExpeditionSnapshot& expedition) {
    header(c,"02  /  DUNGEON","Expedition","");
    badge(c,422,15,126,"FLOOR  B"+std::to_string(floor)+"F");
    exploration_tab(c,445,103,"Back to Map",false);
    c.frame(44,102,504,82,7,deep);
    c.text(62,130,"ARIADNE THREADS",12,muted);
    c.text(62,162,expedition_count(expedition.threads),23,ink);
    c.text(308,130,"FREE BAG SLOTS",12,muted);
    c.text(308,162,expedition_count(expedition.bag_free),23,ink);
    c.text(44,216,"ACTIVE PARTY",12,accent);
    if(!expedition.party || expedition.party->empty()) {
        c.text(44,244,expedition.party?"No active party members":"Party data unavailable",16,ink);
        return;
    }
    for(size_t i=0;i<expedition.party->size();++i) {
        const auto& member=(*expedition.party)[i];
        const int y=227+48*int(i);
        c.frame(44,y,504,41,5,card,member.hp==0?danger:border);
        c.fit(58,y+27,member.name,17,245,ink);
        if(member.hp==0) c.text(308,y+27,"KO",14,danger);
        c.right(434,y+27,std::to_string(member.hp)+" HP",15,hp_color);
        c.right(531,y+27,std::to_string(member.tp)+" TP",15,accent);
    }
}
inline void map(Canvas& c,const std::array<uint32_t,cells>& tiles,const std::array<int32_t,cells>& icons,
                const std::array<IconPixels,42>& artwork,const std::array<bool,42>& ok,
                int px,int py,int floor,int direction,const FloorReference* ref=nullptr,bool hints=false,
                const ExpeditionSnapshot* expedition=nullptr) {
    header(c,"02  /  DUNGEON","Labyrinth","");
    badge(c,422,15,126,"FLOOR  B"+std::to_string(floor)+"F");
    exploration_tab(c,445,103,"Floor Guide",false);
    exploration_tab(c,219,104,"Quests",false);
    if(ref && ref->ready) exploration_tab(c,332,104,hints?"Hints: On":"Hints: Off",hints);
    c.frame(44,87,504,433,7,deep);
    for(int col=0;col<7;++col)
        c.center(map_left+col*5*map_scale+32,103,std::to_string(col+1),10,muted);
    for(int row=0;row<6;++row) {
        const char letter=char('A'+row);
        c.center(56,map_top+row*5*map_scale+36,std::string_view(&letter,1),10,muted);
    }
    for(int y=0;y<etrian::height;++y) for(int x=0;x<etrian::width;++x) {
        const int i=index(x,y),sy=screen_y(y),ox=map_left+x*map_scale,oy=map_top+sy*map_scale;
        const auto flags=tiles[i];
        const bool unrecorded=hints && ref && ref->ready && !(flags&8) && icons[i]<0;
        const bool hinted=unrecorded && ref->open(x,sy);
        const auto fill=hinted?0x192b40:tile_fill(flags);
        c.box(ox,oy,map_scale,map_scale,fill);
        c.box(ox,oy,1,map_scale,x%5==0?0x314762:0x233351);
        c.box(ox,oy,map_scale,1,sy%5==0?0x314762:0x233351);
        if(hinted) {
            constexpr uint32_t outline=0x40576b;
            if(!ref->open(x-1,sy) && (x==0 || !has_right_wall(tiles[index(x-1,y)]))) c.box(ox,oy,1,map_scale,outline);
            if(!ref->open(x+1,sy)) c.box(ox+map_scale-1,oy,1,map_scale,outline);
            if(!ref->open(x,sy-1) && (y==height-1 || !has_lower_wall(tiles[index(x,y+1)]))) c.box(ox,oy,map_scale,1,outline);
            if(!ref->open(x,sy+1)) c.box(ox,oy+map_scale-1,map_scale,1,outline);
        }
        if(unrecorded && (x!=px || y!=py)) hint_marker(c,ox,oy,ref->markers[index(x,sy)],fill,artwork,ok);
        const int icon=icons[i], art=art_for_map_icon(icon);
        if(art && ok[art]) c.icon(ox,oy,map_scale,artwork[art]);
        else fallback_icon(c,ox,oy,icon);
        if(has_right_wall(flags)) c.box(ox+map_scale-1,oy,1,map_scale,0x85bce3);
        if(has_lower_wall(flags)) c.box(ox,oy+map_scale-1,map_scale,1,0x85bce3);
    }
    const int x=map_left+px*map_scale,y=map_top+screen_y(py)*map_scale;
    c.circle(x+6,y+6,7,hp_color); c.circle(x+6,y+6,6,deep);
    if(ok[19]) c.icon(x+1,y+1,11,artwork[19],direction);
    else for(int yy=1;yy<12;++yy) for(int xx=1;xx<12;++xx) {
        int dx=xx-6,dy=yy-6,rx=dx,ry=dy;
        if(direction==1) { rx=dy; ry=-dx; }
        else if(direction==2) { rx=-dx; ry=-dy; }
        else if(direction==3) { rx=-dy; ry=dx; }
        if(ry>=-4 && ry<=3 && std::abs(rx)<=(ry+4)/2) c.put(x+xx,y+yy,hp_color);
    }
    constexpr std::array<const char*,4> directions{"NORTH", "EAST", "SOUTH", "WEST"};
    c.text(55,513,hints && ref && ref->ready?"Recorded map + hints":"Recorded map",10,muted);
    if(expedition) {
        std::string party="Party -";
        if(expedition->party) {
            const auto ko=std::ranges::count_if(*expedition->party,[](const auto& m) {return m.hp==0;});
            const auto low=std::ranges::count_if(*expedition->party,[](const auto& m) {return m.hp>0 && m.tp<=5;});
            party=expedition->party->empty()?"Party empty":std::to_string(ko)+" KO / "+std::to_string(low)+" low TP";
        }
        c.frame(173,500,312,18,4,card);
        c.center(329,513,"Threads "+expedition_count(expedition->threads)+"  /  Bag "+
                 expedition_count(expedition->bag_free)+" free  /  "+party+"  >",11,accent);
    }
    if(direction>=0 && direction<int(directions.size())) c.right(535,513,directions[direction],10,accent);
}
} // namespace etrian::glass
