#include "../src/game.h"
#include <cstdio>
#include <cstdlib>
int main(int argc, char** argv){
    g_world.build();
    const char* code = argc > 1 ? argv[1] : "FRA";
    int n = g_world.nationIndex(code);
    g_career.coachCat = argc > 2 ? atoi(argv[2]) : 0;
    g_career.newCoachCareer(n);
    auto pool = coachPool(n);
    printf("pool %zu, calls %zu, squad %zu\n", pool.size(), g_career.coachCalls.size(), g_world.teams[n].squad.size());
    for (int i=0;i<5 && i<(int)g_world.teams[n].squad.size();i++){ auto&p=g_world.teams[n].squad[i]; printf("  %s %s %d\n", posName(p.pos), p.name.c_str(), p.overall()); }
    for (int camp=0; camp<4; camp++){
        auto& S = g_career.season;
        printf("== %s %d comps %zu hosts:", INTL_NAMES[g_career.intlType], g_career.year, S.comps.size());
        for (int h: g_career.intlHosts) printf(" %s", g_world.teams[h].name.c_str()); printf("\n");
        int guard=0; while(!S.finished && guard++<100000){ auto pm=S.advance(true); if(pm.comp<0 && S.finished) break; }
        int nm=0; for (auto&c:S.comps) for(auto&m:c.matches) if (m.played && (m.home==g_career.coachTeam()||m.away==g_career.coachTeam())) nm++;
        printf("matchs joués par la sélection : %d ; résultat : %s\n", nm, resultLevelName(g_career.coachResult()));
        for (auto&c:S.comps) if (c.kind==30||c.kind==31) printf("  %s done %d winner %s\n", c.name.c_str(), c.done, c.winner>=0?g_world.teams[c.winner].name.c_str():"-");
        if (g_career.finalComp>=0){ auto&F=S.comps[g_career.finalComp]; printf("  finale : %s vainqueur %s ; lieu match 0 : %s ; dernier : %s\n", F.name.c_str(), F.winner>=0?g_world.teams[F.winner].name.c_str():"-", matchVenue(g_career.finalComp,0).c_str(), matchVenue(g_career.finalComp,(int)F.matches.size()-1).c_str()); }
        std::vector<std::string> msgs; g_career.endCoachCampaign(msgs); for(auto&m:msgs) printf("  > %s\n", m.c_str());
        if (g_career.coachSacked) { printf("  limogé\n"); break; }
        g_career.startCoachCampaign();
    }
    bool ok=g_career.save("/tmp/c.sav"); ok = ok && g_career.load("/tmp/c.sav"); printf("save/load %d coach %d nation %d calls %zu\n", ok, g_career.coach, g_career.coachNation, g_career.coachCalls.size());
}
