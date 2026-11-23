// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "etrian_art.h"
#include "etrian_battle.h"
#include "etrian_font_data.h"
#include "etrian_map.h"

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
    void icon(int x,int y,int size,const IconPixels& data,int direction=0) {
        for(int yy=0;yy<size;++yy) for(int xx=0;xx<size;++xx) {
            int sx=xx,sy=yy;
            if(direction==1) { sx=yy; sy=size-1-xx; }
            else if(direction==2) { sx=size-1-xx; sy=size-1-yy; }
            else if(direction==3) { sx=size-1-yy; sy=xx; }
            const auto i=((53-sy*54/size)*54+sx*54/size)*4;
            put(x+xx,y+yy,(Color(data[i])<<16)|(Color(data[i+1])<<8)|data[i+2],data[i+3]);
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
inline void town(Canvas& c,BattleMode mode,const std::vector<Combatant>& party) {
    header(c,"01  /  TOWN","Armoroad","ACTIVE PARTY");
    if(mode==BattleMode::town_loading) {
        message(c,"Party data unavailable","The companion will retry when the roster is readable.");
    } else if(party.empty()) {
        message(c,"No active party","Form an active party at the Explorers Guild.");
    } else {
        c.text(24,106,"EXPEDITION ROSTER",11,muted);
        c.right(568,106,std::to_string(party.size())+" ACTIVE",11,hp_color);
        constexpr int w=264,h=107;
        for(size_t i=0;i<party.size() && i<5;++i) {
            const int x=24+int(i%2)*280,y=125+int(i/2)*118;
            const auto& m=party[i];
            const bool ko=m.hp==0;
            c.frame(x,y,w,h,12,card,ko?danger:border);
            c.text(x+15,y+23,"0"+std::to_string(i+1),11,muted);
            c.fit(x+45,y+27,m.name,21,ko?143:202,ink);
            if(ko) badge(c,x+204,y+9,46,"KO",danger);
            c.box(x+15,y+39,w-30,1,border);
            c.text(x+16,y+65,"HP",11,ko?danger:hp_color);
            c.text(x+121,y+65,"TP",11,tp_color);
            c.text(x+16,y+92,std::to_string(m.hp),27,ko?danger:hp_color);
            c.text(x+121,y+92,std::to_string(m.tp),27,tp_color);
        }
    }
    footer(c,"Current HP and TP","Formation order");
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
    const int hp_y=y+(compact?34:43);
    stat(c,x,hp_y,"HP",m.hp,m.max_hp,(!ally||ko)?danger:hp_color);
    if(ally) stat(c,x,hp_y+(compact?15:18),"TP",m.tp,m.max_tp,tp_color);
    // The larger cards leave a stable effect strip. No effect claims until EO III is validated.
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
    footer(c,"Effects unavailable","Roster order");
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
inline void map(Canvas& c,const std::array<uint32_t,cells>& tiles,const std::array<int32_t,cells>& icons,
                const std::array<IconPixels,42>& artwork,const std::array<bool,42>& ok,
                int px,int py,int floor,int direction) {
    header(c,"02  /  DUNGEON","Labyrinth","FLOOR  B"+std::to_string(floor)+"F");
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
        c.box(ox,oy,map_scale,map_scale,tile_fill(flags));
        c.box(ox,oy,1,map_scale,x%5==0?0x314762:0x233351);
        c.box(ox,oy,map_scale,1,sy%5==0?0x314762:0x233351);
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
    c.text(55,513,"Recorded map",10,muted);
    if(direction>=0 && direction<int(directions.size())) c.right(535,513,directions[direction],10,accent);
}
} // namespace etrian::glass
