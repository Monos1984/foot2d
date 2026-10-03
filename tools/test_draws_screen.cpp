// Données lues par l'écran de tirage au sort : indices d'équipes et de matchs valides pour chaque phase de coupe
#include "../src/game.h"
#include <cstdio>
#include <algorithm>
static int checks = 0, bad = 0;
static void check(bool v, const char* t, const Competition& C, int s) { checks++; if (!v) { bad++; if (bad < 20) printf("BAD %s : %s / phase %d\n", t, C.name.c_str(), s); } }
int main(int argc, char** argv) {
    g_world.build();
    std::vector<int> users;
    std::vector<int> tiers;
    for (auto& P : g_basePyramids) if (P.country == "FRA" && P.dom < 0) for (auto& pool : P.pools) if (!pool.clubs.empty() && std::find(tiers.begin(), tiers.end(), pool.tier) == tiers.end()) { tiers.push_back(pool.tier); users.push_back(pool.clubs[pool.clubs.size() / 2]); }
    int seasons = argc > 1 ? atoi(argv[1]) : 1;
    for (int u : users) {
        g_career.opts = Career::Opts(); g_career.newClubCareer(u, 2026);
        g_career.mgr.ctrlReserves.clear(); for (int t = 0; t < (int)g_world.teams.size(); t++) if (g_world.teams[t].parent == u) g_career.mgr.ctrlReserves.push_back(t); g_career.syncControlled();
        printf("controlled %zu\n", g_career.season.controlled.size());
        for (int y = 0; y < seasons; y++) {
            Season& S = g_career.season;
            for (int k = 0; k < 6000; k++) {
                auto pm = S.advance(false);
                if (pm.comp < 0) break;
                auto& C = S.comps[pm.comp]; simulateMatch(C.matches[pm.match], &C); S.recordResult(pm.comp, pm.match); S.checkRound(pm.comp);
                for (auto& Cc : S.comps) for (int s = 0; s < (int)Cc.stages.size(); s++) {
                    const Stage& st = Cc.stages[s];
                    bool in = false;
                    for (auto& t : st.ties) if (S.isControlled(t.a) || (t.b >= 0 && S.isControlled(t.b))) in = true;
                    for (auto& g : st.groups) for (int t : g) if (S.isControlled(t)) in = true;
                    if (!in) continue;
                    int nt = (int)g_world.teams.size(), nm = (int)Cc.matches.size();
                    for (auto& T : st.ties) {
                        check(T.a >= 0 && T.a < nt, "tie.a", Cc, s);
                        check(T.b < nt, "tie.b", Cc, s);
                        check(T.m1 < nm, "tie.m1 range", Cc, s);
                        check(!(T.b >= 0 && T.m1 < 0), "tie with opponent but no match", Cc, s);
                        if (T.m1 >= 0 && T.m1 < nm) check(Cc.matches[T.m1].home >= 0 && Cc.matches[T.m1].home < nt && Cc.matches[T.m1].away >= 0 && Cc.matches[T.m1].away < nt, "match teams", Cc, s);
                    }
                    for (auto& g : st.groups) for (int t : g) check(t >= 0 && t < nt, "group team", Cc, s);
                }
            }
            if (y + 1 < seasons) g_career.endSeason();
        }
        printf("club %s : %d checks, %d bad\n", g_world.teams[u].name.c_str(), checks, bad);
    }
    printf(bad ? "FAIL draws screen data\n" : "PASS draws screen data\n");
    return bad ? 1 : 0;
}
