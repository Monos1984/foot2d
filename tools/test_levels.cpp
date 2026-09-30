#include "../src/game.h"
#include <cstdio>
int main(){
    g_world.build();
    int u=g_world.firstClub; g_career.newClubCareer(u,2026);
    for (auto& P: g_career.pyramids) if (P.country=="FRA" && P.dom<0) {
        for (int t=0;t<(int)P.tiers.size();t++){
            double rs=0, ov=0, mn=1e9, mx=0; int n=0, np=0; int sampled=0;
            for (auto& pl: P.pools) if (pl.tier==t) for (int c: pl.clubs) {
                auto& T=g_world.teams[c]; rs+=T.rating; n++; mn=std::min(mn,(double)T.rating); mx=std::max(mx,(double)T.rating);
                if (sampled<40) { g_world.ensureSquad(c); sampled++; auto lu=g_world.pickLineup(c,T.formation); for (int k=0;k<11&&k<(int)lu.size();k++) if (lu[k]>=0){ ov+=T.squad[lu[k]].overall(); np++; } }
            }
            if (n) printf("%-18s clubs %5d rating avg %.1f [%.0f-%.0f]  XI overall avg %.1f\n", P.tiers[t].name.c_str(), n, rs/n, mn, mx, np?ov/np:0);
        }
    }
}
