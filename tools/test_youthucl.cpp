// test : Ligue des champions U19, historique des joueurs, sauvegarde v11
#include "../src/game.h"
#include <cstdio>
int main() {
    g_world.build();
    int user = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Stade Brestois") user = i;
    g_career.newClubCareer(user, 2026);
    for (int season = 0; season < 2; season++) {
        Season& S = g_career.season;
        printf("Saison %d : prelim %d, UYL %d, direct %d\n", g_career.year, g_career.youthPrelim, g_career.youthUcl, (int)g_career.youthDirect.size());
        while (true) { auto pm = S.advance(true); if (pm.comp < 0) break; }
        if (g_career.youthPrelim >= 0) { auto& C = S.comps[g_career.youthPrelim]; printf("  prélim : %d ties, %d qualifiés\n", (int)C.stages[0].ties.size(), (int)C.result.size()); }
        if (g_career.youthUcl >= 0) {
            auto& C = S.comps[g_career.youthUcl];
            for (auto& st : C.stages) printf("  [%s g%d t%d legs%d]", st.name.c_str(), (int)st.groups.size(), (int)st.ties.size(), st.legs);
            printf("\n  vainqueur %s ; neutres :", C.winner >= 0 ? g_world.teams[C.winner].name.c_str() : "?");
            int nn = 0; for (auto& m : C.matches) nn += m.neutral; printf(" %d / %d matchs\n", nn, (int)C.matches.size());
            for (int t : C.stages[0].groups[0]) printf("   G1: %s\n", g_world.teams[t].name.c_str());
        } else printf("  pas de UYL\n");
        g_career.endSeason();
    }
    Team& U = g_world.teams[user];
    for (auto& p : U.squad) if (!p.hist.empty()) {
        int d, m, y; std::string pl; playerBirth(p, g_career.year, d, m, y, pl, U.culture, U.dept);
        printf("%s né le %02d/%02d/%d à %s :", p.name.c_str(), d, m, y, pl.c_str());
        for (auto& h : p.hist) printf(" [%d %s %dm %db %dp %dj %dr]", h.year, g_world.teams[h.team].shortName.c_str(), h.apps, h.goals, h.assists, h.yel, h.red);
        printf("\n"); break;
    }
    printf("Président : %s (%d ans, %s)\n", presidentName(user).c_str(), presidentAge(user), presidentPlace(user).c_str());
    bool ok = g_career.save("/tmp/t11.sav");
    Career K2; bool ok2 = K2.load("/tmp/t11.sav");
    printf("save %d load %d, UYL %d\n", ok, ok2, K2.youthUcl);
    return 0;
}
