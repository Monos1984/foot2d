#include "../src/game.h"
int main(){
    g_world.build();
    struct T{int type,fmt; bool q;} cases[]={{IT_WORLDCUP,0,false},{IT_WORLDCUP,1,false},{IT_WORLDCUP,1,true},{IT_EURO,1,false},{IT_EURO,1,true},{IT_EURO,0,false},{IT_COPA,0,false}};
    for (auto& c: cases){
        g_career.newInternational(c.type,c.q,{g_world.nationIndex("FRA")},0,{},c.fmt);
        auto& S=g_career.season;
        while(true){ auto pm=S.advance(true); if(pm.comp<0)break; }
        auto& C=S.comps[g_career.finalComp];
        printf("%s fmt%d q%d: teams? ", INTL_NAMES[c.type], c.fmt, c.q);
        for (auto& st: C.stages) printf("[%s %zu%s L%d] ", st.name.c_str(), st.groups.empty()?st.ties.size():st.groups.size(), st.groups.empty()?"":"g", st.legs);
        printf("winner %s\n", C.winner>=0?g_world.teams[C.winner].name.c_str():"-");
        if (c.type==IT_WORLDCUP && c.fmt==0){ auto& k=C.stages[1]; for (auto&t:k.ties) printf("   %s - %s\n", g_world.teams[t.a].name.c_str(), g_world.teams[t.b].name.c_str()); }
    }
}
