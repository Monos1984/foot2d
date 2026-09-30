#include "../src/game.h"
#include <cstdio>
int main(){
    g_world.build();
    int user=-1;
    g_career.newClubCareer(0, 2026);
    for (int i=0;i<(int)g_world.teams.size();i++){ std::string l=g_career.teamLevelName(i); if(l.find("4 - Marne")!=std::string::npos && g_world.teams[i].parent<0){ user=i; break; } }
    printf("user %s : %s\n", g_world.teams[user].name.c_str(), g_career.teamLevelName(user).c_str());
    g_career.newClubCareer(user, 2026);
    int p,q,g; g_career.tierOfTeam(user,&p,&q,&g);
    auto&P=g_career.pyramids[p];
    for (auto&pl:P.pools) if (pl.key==g_world.teams[user].district && P.tiers[pl.tier].scope==SC_DEPT) printf(" pool tier %d %s groups %zu clubs %zu up %d terminal %d\n", pl.tier, P.tiers[pl.tier].name.c_str(), pl.groups.size(), pl.clubs.size(), P.tiers[pl.tier].up, pl.terminal);
    for (int yr=0; yr<3; yr++){
        g_world.teams[user].rating = 95;
        while(true){ auto pm=g_career.season.advance(false); if(pm.comp<0)break; auto&m=g_career.season.comps[pm.comp].matches[pm.match]; simulateMatch(m,&g_career.season.comps[pm.comp]); g_career.season.recordResult(pm.comp,pm.match); g_career.season.finishRoundOthers(pm.comp,pm.match); g_career.season.checkRound(pm.comp);}
        g_career.endSeason();
        printf("saison %d -> %s (tier %d)\n", yr, g_career.teamLevelName(user).c_str(), g_career.tierOfTeam(user));
    }
}
