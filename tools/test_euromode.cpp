// test : mode Coupes d'Europe (C1 + C3 uniquement, plusieurs saisons)
#include "../src/game.h"
#include <cstdio>
int main() {
    g_world.build();
    auto cand = Career::euroCandidates();
    printf("candidats : %d\n", (int)cand.size());
    int psg = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Paris Saint-Germain") psg = i;
    g_career.newEuroCareer({ psg }, 2026);
    for (int s = 0; s < 3; s++) {
        Season& S = g_career.season;
        printf("Saison %d : %d compétitions (", g_career.year, (int)S.comps.size());
        for (auto& c : S.comps) printf("%s; ", c.name.c_str());
        printf(")\n");
        int guard = 0;
        while (guard++ < 100000) { auto pm = S.advance(true); if (pm.comp < 0) break; }
        auto w = [&](int c) { return c >= 0 && S.comps[c].winner >= 0 ? g_world.teams[S.comps[c].winner].name : std::string("?"); };
        int nd = 0; for (auto& c : S.comps) if (!c.done) nd++;
        printf("  C1 : %s | C3 : %s | non terminées : %d\n", w(g_career.ucl).c_str(), w(g_career.uel).c_str(), nd);
        const Competition& E = S.comps[g_career.uel];
        for (auto& st : E.stages) printf("  [%s %d]", st.name.c_str(), (int)st.ties.size()); printf("\n");
        g_career.endSeason();
    }
    return 0;
}
