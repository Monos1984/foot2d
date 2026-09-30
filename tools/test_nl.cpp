#include "../src/game.h"
#include <cstdio>
static double matchT(const Competition& C, int mi){ for (auto& st: C.stages) for (auto& R: st.rounds) for (int x: R.m) if (x==mi) return R.time; return -1; }
int main(int argc, char** argv){
    g_world.build();
    auto N=[&](const char*c){return g_world.nationIndex(c);};
    struct T{int type,fmt; std::vector<int> hosts;} cases[]={{IT_EURO,1,{N("ENG"),N("SCO"),N("WAL"),N("IRL")}},{IT_WORLDCUP,1,{}}};
    for (auto& c: cases){
        g_career.coach=false; g_career.nlLeague.clear();
        g_career.newInternational(c.type,true,{N("FRA")},0,c.hosts,c.fmt);
        auto& S=g_career.season;
        int guard=0;
        while(guard++<100000){ auto pm=S.advance(true); if(pm.comp<0)break; }
        printf("=== %s %d (saison %d) finished=%d\n", INTL_NAMES[c.type], g_career.year, S.year, (int)S.finished);
        for (int i=0;i<(int)S.comps.size();i++){ auto& C=S.comps[i]; double t0=1e9,t1=-1; for(int m=0;m<(int)C.matches.size();m++){double t=matchT(C,m); t0=std::min(t0,t); t1=std::max(t1,t);} 
            printf("%3d k%d %-60s done%d t%.0f-%.0f win %s\n", i, C.kind, C.name.c_str(), C.done, t0,t1, C.winner>=0?g_world.teams[C.winner].name.c_str():"-");
            if (C.kind==31||C.kind==32||C.kind==33||C.kind==102||C.kind==115) for (auto& st: C.stages) for (auto& t: st.ties) printf("      %s: %s - %s -> %s\n", st.name.c_str(), t.a>=0?g_world.teams[t.a].shortName.c_str():"-", t.b>=0?g_world.teams[t.b].shortName.c_str():"-", t.winner>=0?g_world.teams[t.winner].shortName.c_str():"?");
        }
        auto& F=S.comps[g_career.finalComp];
        printf("finale: %zu equipes groupes:", F.stages[0].groups.size()*4);
        for (auto&g:F.stages[0].groups){ printf(" |"); for(int t:g) printf(" %s", g_world.teams[t].shortName.c_str()); }
        printf("\nnext NL: "); for (int l=0;l<4;l++){ int n=0; for (int t=0;t<NUM_NATIONS;t++) if ((int)g_career.nlLeague.size()==NUM_NATIONS && g_career.nlLeague[t]==l) n++; printf("%c=%d ", 'A'+l, n);} printf("\n");
    }
}
