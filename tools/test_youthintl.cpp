#include "../src/game.h"
#include <cstdio>
static double matchT(const Competition& C, int mi){ for (auto& st: C.stages) for (auto& R: st.rounds) for (int x: R.m) if (x==mi) return R.time; return -1; }
int main(){
    g_world.build();
    int FRA=g_world.nationIndex("FRA");
    for (int type : {IT_EURO21, IT_EURO19, IT_EURO17, IT_OLYMPICS}) for (int q=1;q>=0;q--) {
        g_career.coach=false;
        g_career.newInternational(type, q, {FRA}, 0, {}, 0);
        auto& S=g_career.season; int guard=0; while(guard++<100000){ auto pm=S.advance(true); if(pm.comp<0)break; }
        printf("=== %s %d q%d season %d finished %d ctrl %s\n", INTL_NAMES[type], g_career.year, q, S.year, S.finished, g_world.teams[S.controlled[0]].name.c_str());
        for (auto& C: S.comps){ double t0=1e9,t1=-1; for(int m=0;m<(int)C.matches.size();m++){double t=matchT(C,m); t0=std::min(t0,t); t1=std::max(t1,t);} printf("  k%d %-55s done%d t%.0f-%.0f win %s\n", C.kind, C.name.c_str(), C.done, t0, t1, C.winner>=0?g_world.teams[C.winner].name.c_str():"-"); }
        if (g_career.finalComp>=0){ auto& F=S.comps[g_career.finalComp]; for (auto&g:F.stages[0].groups){ printf("   |"); for(int t:g) printf(" %s", g_world.teams[t].shortName.c_str()); } printf("\n"); 
          auto& T=g_world.teams[F.stages[0].groups[0][0]]; int amin=99,amax=0; for(auto&p:T.squad){amin=std::min(amin,(int)p.age); amax=std::max(amax,(int)p.age);} printf("   squad %zu ages %d-%d\n", T.squad.size(), amin, amax); }
    }
}
