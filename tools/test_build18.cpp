// Build 18 : gardien hors des tableaux de formation, persistance des états mensuels, rôles tactiques en match carrière
#include "../src/game.h"
#include "../src/match.h"
#include <cmath>
#include <memory>
static int checks = 0;
static void check(bool v, const char* text) { checks++; if (!v) { printf("FAIL build18: %s\n", text); exit(1); } }
int main(int argc, char** argv) {
    g_world.build();
    int a = -1, b = -1;
    for (auto& P : g_basePyramids) if (P.country == "FRA" && P.dom < 0) for (auto& pool : P.pools) if (pool.tier == 0 && pool.clubs.size() > 1) { a = pool.clubs[0]; b = pool.clubs[1]; }
    check(a >= 0 && b >= 0, "two clubs");
    g_career.opts = Career::Opts(); g_career.newClubCareer(a, 2026); g_world.ensureSquad(b);

    // 1. gardien : formationTarget(slot 0) ne lit plus les tableaux de formation à l'index -1
    {
        auto m = std::make_unique<Match>(); MatchSetup s; s.home = a; s.away = b; s.rules = RULESET_CAREER; for (int i = 0; i < NUM_INPUTS; i++) s.side[i] = -1;
        m->init(s);
        for (int t = 0; t < 2; t++) for (float bu : { 0.1f, 0.5f, 0.9f }) {
            m->ball.pos = m->fromTeamFrame(t, bu, 0.3f);
            V2 o; m->formationTarget(t * 11, o);
            check(std::isfinite(o.x) && std::isfinite(o.y), "goalkeeper target finite");
            check((o - m->ownGoal(t)).len() < 3.5f, "goalkeeper target stays in front of own goal");
        }
    }
    // 2. persistance : conférence de presse, discussions, trophées du mois
    {
        auto& MS = g_career.monthly;
        MS.pressYear = g_career.year; MS.pressComp = 3; MS.pressMatch = 17; MS.awMonth = 4; MS.awYear = g_career.year; MS.awTeam = a;
        MS.awGoals = { { 11, 2 }, { 12, 5 } }; MS.awPts = { { a, 9 } }; MS.talked = { { g_world.teams[a].squad[0].id, g_career.year * 12 + 4 } };
        check(g_career.save("build/test-build18.sav"), "save career");
        g_career.monthly = Career::MonthlyState();
        check(g_career.load("build/test-build18.sav"), "reload career");
        auto& R = g_career.monthly;
        check(R.pressYear == g_career.year && R.pressComp == 3 && R.pressMatch == 17, "press conference state persisted");
        check(R.awMonth == 4 && R.awTeam == a && R.awGoals.size() == 2 && R.awGoals[1].v == 5 && R.awPts.size() == 1 && R.awPts[0].v == 9, "monthly awards snapshot persisted");
        check(R.talked.size() == 1 && R.talked[0].k == g_world.teams[a].squad[0].id, "player talks persisted");
        g_career.newClubCareer(a, 2026);
        check(g_career.monthly.pressComp == -1 && g_career.monthly.talked.empty(), "new career resets monthly state");
    }
    // 3. rôles : latéral inversé dans l'axe, latéral offensif plus haut, libéro dernier homme, mezzala dans le demi-espace
    {
        auto& T = g_world.teams[a]; T.tactical.customized = 1; for (int s = 0; s < 11; s++) T.tactical.slot[s] = SlotTactic();
        auto m = std::make_unique<Match>(); MatchSetup s; s.home = a; s.away = b; s.rules = RULESET_CAREER; s.formation[0] = 1; s.formation[1] = 1; for (int i = 0; i < NUM_INPUTS; i++) s.side[i] = -1;
        m->init(s);
        int fb = -1, dc = -1, mc = -1;
        for (int i = 1; i < 11; i++) { int dp = m->pl[i].detailedPosition; if ((dp == DP_DD || dp == DP_DG) && fb < 0) fb = i; if (dp == DP_DC && dc < 0) dc = i; if (dp == DP_MC && mc < 0) mc = i; }
        check(fb > 0 && dc > 0, "formation has fullback and centre back");
        auto target = [&](int i, int role, bool attack, float bu, float bv) {
            T.tactical.slot[m->pl[i].slot].role = role; m->setFormation(0, 1);
            m->possTeam = attack ? 0 : 1; m->ball.pos = m->fromTeamFrame(0, bu, bv);
            for (int j = 11; j < 22; j++) m->pl[j].pos = m->fromTeamFrame(0, 0.9f, 0.1f + 0.08f * (j - 11));
            V2 o; m->formationTarget(i, o); return o;
        };
        V2 inv = target(fb, 3, true, 0.45f, 0.5f), std0 = target(fb, 1, true, 0.45f, 0.5f);
        check(std::fabs(inv.x - PITCH_W / 2) < std::fabs(std0.x - PITCH_W / 2) - 4.f, "inverted fullback steps into midfield");
        float side = m->pl[fb].pos.x < PITCH_W / 2 ? 0.2f : 0.8f;
        V2 att = target(fb, 2, true, 0.55f, side), sup = target(fb, 1, true, 0.55f, side);
        check(m->progress(0, att) > m->progress(0, sup) + 0.02f, "attacking fullback overlaps");
        V2 lib = target(dc, 4, false, 0.5f, 0.5f), def = target(dc, 0, false, 0.5f, 0.5f);
        check(m->progress(0, lib) < m->progress(0, def), "libero is the last man without the ball");
        V2 libA = target(dc, 4, true, 0.3f, 0.5f), defA = target(dc, 0, true, 0.3f, 0.5f);
        check(m->progress(0, libA) > m->progress(0, defA), "libero steps out with the ball");
        if (mc > 0) { V2 mez = target(mc, 4, true, 0.6f, 0.5f); float hs = std::fabs(mez.x - PITCH_W / 2) / PITCH_W; check(hs > 0.12f && hs < 0.32f, "mezzala occupies half-space"); }
        T.tactical.customized = 0;
    }
    // 4. matchs IA carrière complets : scores plausibles avec les rôles par défaut
    int n = argc > 1 ? atoi(argv[1]) : 6, goals = 0, shots = 0;
    for (int k = 0; k < n; k++) {
        auto m = std::make_unique<Match>(); MatchSetup s; s.home = k % 2 ? b : a; s.away = k % 2 ? a : b; s.rules = RULESET_CAREER; s.halfSeconds = 120; for (int i = 0; i < NUM_INPUTS; i++) s.side[i] = -1;
        g_world.teams[a].seed = k; g_world.teams[b].seed = k + 1;
        m->init(s);
        int frames = 0;
        while (!m->finished && frames < 60 * 60 * 30) { m->update(1.f / 60); frames++; m->sfxN = 0; }
        check(m->finished, "career match finishes");
        printf("career match %d: %d-%d shots %d/%d\n", k, m->score[0], m->score[1], m->shots[0], m->shots[1]);
        goals += m->score[0] + m->score[1]; shots += m->shots[0] + m->shots[1];
    }
    printf("career avg goals %.2f shots %.1f\n", goals / (float)n, shots / (float)n);
    check(shots / (float)n > 3.f, "career matches create chances");
    check(goals / (float)n < 6.f, "career matches not absurd");
    printf("PASS build18: %d checks\n", checks);
}
