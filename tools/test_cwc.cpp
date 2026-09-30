#include "../src/game.h"
#include <cstdio>
int main(){
    g_world.build();
    int u=-1; for (int i=g_world.firstClub;i<(int)g_world.teams.size();i++) if (g_world.teams[i].name=="Paris Saint-Germain") u=i;
    g_career.newClubCareer(u,2028);
    auto& S=g_career.season; int guard=0; while(guard++<300000){ auto pm=S.advance(true); if(pm.comp<0)break; }
    for (auto& C: S.comps) if (C.kind==28) {
        printf("%s done=%d winner=%s\n", C.name.c_str(), C.done, C.winner>=0?g_world.teams[C.winner].name.c_str():"-");
        for (auto& g: C.stages[0].groups){ printf("  |"); for(int t:g) printf(" %s(%s)", g_world.teams[t].shortName.c_str(), NATIONS[g_world.teams[t].nation].code); printf("\n"); }
        for (size_t s=1;s<C.stages.size();s++){ printf(" %s:", C.stages[s].name.c_str()); for (auto&t:C.stages[s].ties) printf(" %s-%s", g_world.teams[t.a].shortName.c_str(), g_world.teams[t.b].shortName.c_str()); printf("\n"); }
        double t0=1e9,t1=0; for (auto& st:C.stages) for(auto&R:st.rounds){t0=std::min(t0,R.time);t1=std::max(t1,R.time);} printf(" t %.1f-%.1f\n",t0,t1);
    }
    int nd=0; for (auto&C:S.comps) if(!C.done) nd++; printf("not done %d\n", nd);
    for (auto& n : S.news) if (n.find("monde des clubs")!=std::string::npos) printf("NEWS %s\n", n.c_str());
}
