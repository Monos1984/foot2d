#include "../src/game.h"
#include <cstdio>
int main(){
    g_world.build();
    int u=-1; for (int i=0;i<(int)g_world.teams.size();i++) if (g_world.teams[i].name=="Stade de Reims") u=i;
    g_career.newClubCareer(u, 2026);
    int p = g_career.u15Pyramid(); auto& P = g_career.pyramids[p];
    std::vector<int> byTier(P.tiers.size());
    for (auto& pl : P.pools) byTier[pl.tier] += (int)pl.clubs.size();
    for (size_t t=0;t<P.tiers.size();t++) printf("%s : %d équipes\n", P.tiers[t].name.c_str(), byTier[t]);
    int nsc=0; for (int c : g_career.regSuperCups) nsc++; printf("supercoupes de région : %d\n", nsc);
    while(true){ auto pm=g_career.season.advance(true); if(g_career.season.finished) break; (void)pm; }
    auto& S=g_career.season;
    if (g_career.superRegions>=0) printf("%s : vainqueur %s\n", S.comps[g_career.superRegions].name.c_str(), g_world.teams[S.comps[g_career.superRegions].winner].name.c_str());
    int ets=0, tab=0; if (g_career.superRegions>=0) for (auto&m: S.comps[g_career.superRegions].matches){ if(m.aet) ets++; if (m.ph>=0) tab++; }
    printf("méga : prolongations %d, tab %d\n", ets, tab);
    g_career.endSeason();
    byTier.assign(P.tiers.size(),0); auto& P2=g_career.pyramids[p]; for (auto& pl : P2.pools) byTier[pl.tier] += (int)pl.clubs.size();
    for (size_t t=0;t<P2.tiers.size();t++) printf("après : %s : %d\n", P2.tiers[t].name.c_str(), byTier[t]);
    printf("champions R1 : %zu\n", g_career.prevR1Champ.size());
    bool ok = g_career.save("/tmp/u.sav") && g_career.load("/tmp/u.sav"); printf("save/load %d\n", ok);
}
