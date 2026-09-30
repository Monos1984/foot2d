#include "../src/game.h"
#include <cstdio>
#include <map>
int main(){
    g_world.build();
    g_career.newClubCareer(0, 2026);
    for (int yr=0; yr<3; yr++){
        while(true){ auto pm=g_career.season.advance(true); if(g_career.season.finished||pm.comp>=0&&false) break; if(pm.comp<0) break; }
        std::map<int,int> before; int fr=0; for(int i=0;i<(int)g_career.pyramids.size();i++) if(g_career.pyramids[i].country=="FRA"&&g_career.pyramids[i].dom<0) fr=i;
        for (auto&pl:g_career.pyramids[fr].pools) for(int c:pl.clubs) before[c]=pl.tier;
        g_career.endSeason();
        int up2=0, down2=0; std::map<int,int> hist;
        for (auto&pl:g_career.pyramids[fr].pools) for(int c:pl.clubs){ if(!before.count(c)) continue; int d=before[c]-pl.tier; hist[d]++; if(d>=2){ up2++; if(up2<8) printf("  +%d %s : %d -> %d (%s)\n", d, g_world.teams[c].name.c_str(), before[c], pl.tier, g_career.teamLevelName(c).c_str()); } if(d<=-2) down2++; }
        printf("saison %d : sauts montée>=2 %d, descente>=2 %d |", yr, up2, down2); for(auto&kv:hist) printf(" %d:%d", kv.first, kv.second); printf("\n");
    }
}
