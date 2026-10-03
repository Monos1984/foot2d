// Compétitions personnalisées (build 24) : formats, phases finales aller-retour, finale, 3e place, play-offs, clubs créés
#include "../src/game.h"
#include <cstdio>
static int checks = 0;
static void check(bool v, const char* t) { checks++; if (!v) { printf("FAIL custom rules: %s\n", t); exit(1); } }
static Competition& run(CustomCompDef d) {
    g_career.newCustom(d, { d.teams[0] });
    int guard = 0; while (guard++ < 200000) { auto pm = g_career.season.advance(true); if (pm.comp < 0) break; }
    return g_career.season.comps[0];
}
static int stageMatches(const Stage& s) { int n = 0; for (auto& r : s.rounds) n += (int)r.m.size(); return n; }
int main() {
    g_world.build();
    // deux clubs créés
    std::vector<int> created;
    for (int k = 0; k < 2; k++) { Team t; t.name = k ? "AS Monos" : "FC Pixel"; t.shortName = k ? "ASM" : "FCP"; t.kind = TK_CLUB; t.nation = g_world.nationIndex("FRA"); t.rating = 55; t.culture = CU_FR; t.dept = 0; t.region = DEPTS[0].region; created.push_back(g_world.addCustomClub(t)); }
    std::vector<int> clubs = created;
    for (int i = NUM_NATIONS; (int)clubs.size() < 24 && i < (int)g_world.teams.size(); i++) if (g_world.teams[i].kind == TK_CLUB && g_world.teams[i].parent < 0 && !g_world.teams[i].custom && g_world.teams[i].rating > 60) clubs.push_back(i);
    // 1. groupes + phase finale aller-retour, finale aller-retour, 3e place, meilleurs 3es
    {
        CustomCompDef d; d.name = "Test groupes"; d.format = 2; d.legs = 2; d.groups = 6; d.advance = 2; d.bestThirds = true; d.koLegs = 2; d.finalLegs = 2; d.thirdPlace = true;
        d.teams = clubs; d.ptsWin = 2;
        Competition& C = run(d);
        check(C.done && C.winner >= 0, "groups: winner");
        check(C.ptsWin == 2, "groups: 2 points per win kept");
        check(C.stages[1].ties.size() == 8 && stageMatches(C.stages[1]) == 16, "groups: 6x2 + 4 best thirds = 16 teams, two-legged round of 16");
        const Stage& fin = C.stages.back();
        check(fin.ties.size() == 1 && stageMatches(fin) == 2, "groups: two-legged final");
        bool third = false; for (auto& st : C.stages) if (st.name.find("3e place") != std::string::npos) third = true;
        check(third, "groups: third place match");
        bool createdPlayed = false; for (auto& m : C.matches) if (m.home == created[0] || m.away == created[0]) createdPlayed = true;
        check(createdPlayed, "created club plays");
        printf("groups: winner %s, stages %zu\n", g_world.teams[C.winner].name.c_str(), C.stages.size());
    }
    // 2. championnat + play-offs (8 qualifiés), matchs secs
    {
        CustomCompDef d; d.name = "Test play-offs"; d.format = 3; d.legs = 2; d.advance = 8; d.koLegs = 1; d.finalLegs = 1; d.teams = std::vector<int>(clubs.begin(), clubs.begin() + 12);
        Competition& C = run(d);
        check(C.done && C.winner >= 0, "playoffs: winner");
        check(C.stages.size() == 4 && C.stages[1].ties.size() == 4 && stageMatches(C.stages[1]) == 4, "playoffs: regular season then single-leg quarter-finals");
        check(C.stages[1].name.find("Play-offs") != std::string::npos, "playoffs: naming");
        check(stageMatches(C.stages[0]) == 12 * 11, "playoffs: home and away regular season");
        printf("play-offs: winner %s\n", g_world.teams[C.winner].name.c_str());
    }
    // 3. coupe aller-retour, finale aller-retour
    {
        CustomCompDef d; d.name = "Test coupe"; d.format = 1; d.koLegs = 2; d.finalLegs = 2; d.teams = std::vector<int>(clubs.begin(), clubs.begin() + 16);
        Competition& C = run(d);
        check(C.done && C.winner >= 0, "cup: winner");
        check(stageMatches(C.stages[0]) == 16, "cup: two-legged first round");
        check(stageMatches(C.stages.back()) == 2, "cup: two-legged final");
    }
    // 4. championnat 3 matchs, départage confrontations directes
    {
        CustomCompDef d; d.name = "Test championnat"; d.format = 0; d.legs = 3; d.tb = 1; d.teams = std::vector<int>(clubs.begin(), clubs.begin() + 6);
        Competition& C = run(d);
        check(C.done, "league: done");
        check((int)C.matches.size() == 6 * 5 / 2 * 3, "league: three rounds of matches");
        check(C.tb == TB_H2H, "league: head-to-head tiebreak");
    }
    printf("PASS custom rules: %d checks\n", checks);
}
