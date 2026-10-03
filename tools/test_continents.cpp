// Coupe des Continents : 6 sélections continentales, groupe aller-retour, Final Four, sauvegarde
#include "../src/game.h"
#include "../src/data.h"
#include <cstdio>
static int checks = 0;
static void check(bool v, const char* t) { checks++; if (!v) { printf("FAIL continents: %s\n", t); exit(1); } }
int main() {
    g_world.build();
    auto T = continentTeams();
    check(T.size() == 6, "six continental teams");
    for (int t : T) {
        const Team& X = g_world.teams[t];
        int gk = 0; for (auto& p : X.squad) gk += p.pos == POS_GK;
        printf("%-30s %s rating %.0f squad %zu (gk %d) best %s %d\n", X.name.c_str(), X.shortName.c_str(), X.rating, X.squad.size(), gk, X.squad.empty() ? "-" : X.squad[gk].name.c_str(), X.squad.empty() ? 0 : X.squad[gk].overall());
        check(X.squad.size() >= 16 && gk >= 2, "squad with goalkeepers");
        check(isContinentTeam(t), "flag");
    }
    check(continentTeam(UEFA) == T[0], "teams are reused, not duplicated");
    continentsStart(g_career, { T[0] });
    auto& S = g_career.season;
    check(S.comps.size() == 1 && S.comps[0].kind == KIND_CONTINENTS, "one competition");
    const Competition& C0 = S.comps[0];
    check(C0.matches.size() == 30, "6-team group, home and away: 30 matches");
    int neutral = 0; for (auto& m : C0.matches) neutral += m.neutral; check(neutral == 0, "group matches are played at home");
    // sauvegarde en cours de groupe
    for (int k = 0; k < 9; k++) { auto pm = S.advance(true); if (pm.comp < 0) break; }
    check(g_career.save("build/continents.sav"), "save");
    size_t nt = g_world.teams.size();
    check(g_career.load("build/continents.sav"), "load");
    check(g_world.teams.size() == nt && isContinentTeam(T[0]) && g_career.custom.format == CUSTOM_CONTINENTS, "teams and mode restored");
    int guard = 0;
    while (guard++ < 100000) { auto pm = S.advance(true); if (pm.comp < 0) break; }
    const Competition& C = S.comps[0];
    printf("stages %zu:", C.stages.size()); for (auto& st : C.stages) printf(" [%s %zu ties %zu rounds]", st.name.c_str(), st.ties.size(), st.rounds.size()); printf("\n");
    check(C.done && C.winner >= 0, "winner decided");
    check(C.stages.size() == 3, "group, semi-finals, final");
    check(C.stages[1].ties.size() == 2 && C.stages[2].ties.size() == 1, "final four ties");
    check(C.stages[1].name.find("Final Four") != std::string::npos, "final four naming");
    int semiMatches = 0; for (auto& r : C.stages[1].rounds) semiMatches += (int)r.m.size();
    check(semiMatches == 2, "single-leg semi-finals");
    printf("winner: %s\n", g_world.teams[C.winner].name.c_str());
    printf("PASS continents: %d checks\n", checks);
}
