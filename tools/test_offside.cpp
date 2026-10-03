// Hors-jeu et arbitres assistants : cas de test du cahier des charges (build 19)
#include "../src/match.h"
#include <cmath>
#include <memory>
static int checks = 0;
static void check(bool v, const char* t) { checks++; if (!v) { printf("FAIL offside: %s\n", t); exit(1); } }

static AssistantReferee arOf(int acc, int pos, int conc, int exp) { AssistantReferee a; a.id = acc * 7 + pos; a.offsideAccuracy = acc; a.positioning = pos; a.concentration = conc; a.experience = exp; a.decisiveness = 70; return a; }

struct Scene {
    std::unique_ptr<Match> m;
    int T = 0, kicker = 0, att = 0, def1 = 0, def2 = 0, gk = 0;
    // équipe T attaque ; deux défenseurs sur la ligne u = 0.8, gardien au fond ; attaquant décalé de `margin` mètres
    void setup(int team, int dir, float margin, const AssistantReferee& ar, bool human = false) {
        m = std::make_unique<Match>();
        MatchSetup s; s.home = g_world.nationIndex("FRA"); s.away = g_world.nationIndex("BRA"); for (int i = 0; i < NUM_INPUTS; i++) s.side[i] = -1;
        m->init(s); m->ceremony = false; m->cerPhase = 99; m->state = MS_PLAY; m->clock = 30; m->period = 1;
        T = team; m->attackDir[T] = dir; m->attackDir[1 - T] = -dir;
        Match& M = *m;
        for (int i = 0; i < 22; i++) { M.pl[i].vel = V2(); M.pl[i].state = PS_NORMAL; M.pl[i].human = -1; }
        int o = 1 - T;
        for (int k = 0; k < 11; k++) M.pl[o * 11 + k].pos = M.fromTeamFrame(T, k == 0 ? 0.985f : 0.6f + k * 0.01f, k == 0 ? 0.5f : 0.1f + k * 0.07f);   // défenseurs restants plus haut
        gk = o * 11; def1 = o * 11 + 1; def2 = o * 11 + 2;
        M.pl[def1].pos = M.fromTeamFrame(T, 0.80f, 0.35f); M.pl[def2].pos = M.fromTeamFrame(T, 0.80f, 0.65f);
        for (int k = 0; k < 11; k++) M.pl[T * 11 + k].pos = M.fromTeamFrame(T, k == 0 ? 0.03f : 0.4f, 0.1f + k * 0.07f);
        kicker = T * 11 + 6; att = T * 11 + 9;
        M.pl[kicker].pos = M.fromTeamFrame(T, 0.6f, 0.5f); M.ball.pos = M.pl[kicker].pos; M.ball.owner = -1;
        M.pl[att].pos = M.fromTeamFrame(T, 0.8f + margin / PITCH_L, 0.5f);
        if (human) M.pl[kicker].human = 0;
        M.assistants[0] = M.assistants[1] = ar;
    }
    bool resolve() { Match& M = *m; for (int k = 0; k < 120 && M.state == MS_PLAY; k++) M.offsideUpdate(1.f / 60); return (M.state == MS_STOP || (M.state == MS_REPLAY && M.offReplay)) && M.msg == "HORS-JEU"; }
    bool passAndReceive(int restart = -1) { m->offsidePhotograph(kicker, restart); m->ball.pos = m->pl[att].pos; m->checkOffsideTouch(att); return resolve(); }
};

int main() {
    g_world.build();
    AssistantReferee good = arOf(91, 88, 84, 79), avg = arOf(61, 58, 63, 54), worst = arOf(25, 25, 25, 25);
    Scene S;
    for (int team = 0; team < 2; team++) for (int dir : { -1, 1 }) {          // test 18 : deux équipes, deux sens d'attaque
        // test 1 / 2 : flagrant
        S.setup(team, dir, 2.0f, worst); check(S.passAndReceive(), "obvious offside always flagged (worst assistant)");
        S.setup(team, dir, -2.0f, worst); check(!S.passAndReceive(), "clearly onside never flagged (worst assistant)");
        S.setup(team, dir, 2.0f, good); check(std::fabs(S.m->offsideMarginMeters(S.att) - 2.0f) < 0.02f, "margin measured in metres, positive when offside");
        // test 5 : parade du gardien
        S.setup(team, dir, 1.5f, good); S.m->offsidePhotograph(S.kicker); S.m->offsideOpponentTouch(S.gk, OT_SAVE); S.m->checkOffsideTouch(S.att);
        check(S.resolve(), "goalkeeper save does not put offside attacker back onside");
        // test 6 : déviation d'un défenseur
        S.setup(team, dir, 1.5f, good); S.m->offsidePhotograph(S.kicker); S.m->checkOffsideTouch(S.def1, false); S.m->checkOffsideTouch(S.att);
        check(S.resolve(), "defender deflection keeps offside");
        // test 7 : jeu volontaire d'un défenseur
        S.setup(team, dir, 1.5f, good); S.m->offsidePhotograph(S.kicker); S.m->checkOffsideTouch(S.def1, true); S.m->checkOffsideTouch(S.att);
        check(!S.resolve(), "deliberate defender play resets offside");
        // test 8 : poteau (aucun toucher intermédiaire)
        S.setup(team, dir, 1.5f, good); S.m->offsidePhotograph(S.kicker); S.m->ball.vel = S.m->ball.vel * -0.5f; S.m->checkOffsideTouch(S.att);
        check(S.resolve(), "rebound off the post keeps offside");
        // test 9 : relance du gardien pendant le jeu
        S.setup(team, dir, 1.5f, good); S.kicker = S.T * 11; S.m->pl[S.kicker].pos = S.m->fromTeamFrame(S.T, 0.55f, 0.5f); S.m->ball.pos = S.m->pl[S.kicker].pos;
        check(S.passAndReceive(), "goalkeeper distribution in open play can produce offside");
        // tests 10 à 12 : réception directe sur six mètres, corner, touche
        S.setup(team, dir, 1.5f, good); check(!S.passAndReceive(SP_GOALKICK), "no offside directly from a goal kick");
        S.setup(team, dir, 1.5f, good); check(!S.passAndReceive(SP_CORNER), "no offside directly from a corner");
        S.setup(team, dir, 1.5f, good); check(!S.passAndReceive(SP_THROWIN), "no offside directly from a throw-in");
        // test 13 : corner, puis un partenaire touche : nouvelle phase
        S.setup(team, dir, 1.5f, good); S.m->offsidePhotograph(S.kicker, SP_CORNER); S.m->checkOffsideTouch(S.kicker + 1);
        S.m->pl[S.kicker + 1].pos = S.m->fromTeamFrame(S.T, 0.7f, 0.4f); S.m->ball.pos = S.m->pl[S.kicker + 1].pos;
        S.m->offsidePhotograph(S.kicker + 1); S.m->checkOffsideTouch(S.att);
        check(S.resolve(), "new teammate touch after a corner starts a normal phase");
        // test 14 : gêne du gardien sans toucher le ballon
        S.setup(team, dir, 1.5f, good); S.m->offsidePhotograph(S.kicker);
        { Match& M = *S.m; V2 g = M.pl[S.gk].pos; M.ball.pos = M.fromTeamFrame(S.T, 0.86f, 0.5f); M.ball.owner = -1; M.ball.vel = (g - M.ball.pos).norm() * 20.f;
          M.lastShooter = S.kicker; M.lastShotAge = 0.1f; M.pl[S.att].pos = M.ball.pos + (g - M.ball.pos) * 0.75f; }
        check(S.m->interferesWithGoalkeeper(S.att), "player in the shot line interferes with the keeper");
        check(S.resolve(), "offside player blocking the keeper's view is penalised without touching the ball");
        // test 15 : joueur passif loin de l'action
        S.setup(team, dir, 1.5f, good); S.m->offsidePhotograph(S.kicker);
        { Match& M = *S.m; M.pl[S.att].pos = M.fromTeamFrame(S.T, 0.82f, 0.05f); M.ball.pos = M.fromTeamFrame(S.T, 0.62f, 0.9f); M.ball.owner = -1; }
        check(!S.resolve(), "passive offside player far from play is not penalised");
        // test 16 : dispute le ballon à un adversaire
        S.setup(team, dir, 1.5f, good); S.m->offsidePhotograph(S.kicker);
        { Match& M = *S.m; M.ball.owner = -1; M.ball.vel = V2(); M.ball.pos = M.pl[S.att].pos + V2(1.2f, 0); M.pl[S.def1].pos = M.ball.pos + V2(1.0f, 0); M.pl[S.att].vel = V2(4.f, 0); }
        check(S.resolve(), "offside player challenging an opponent for the ball is penalised");
        // test 17 : humain et IA, mêmes règles
        S.setup(team, dir, 1.5f, good, true); check(S.passAndReceive(), "human passer: same offside");
        S.setup(team, dir, -1.5f, good, true); check(!S.passAndReceive(), "human passer: same onside");
        // but marqué avec drapeau levé : refusé
        S.setup(team, dir, 1.5f, good); S.m->offsidePhotograph(S.kicker); S.m->checkOffsideTouch(S.att);
        check(S.m->offPend == S.att, "flag being raised with a short human delay");
        check(S.m->offsideGoalCheck(S.T) && S.m->msg == "BUT REFUSÉ", "goal scored while the flag is pending is disallowed");
    }
    // test 3 / 4 / 19 : situations serrées, erreurs dans les deux sens et selon la qualité de l'assistant
    auto rate = [&](const AssistantReferee& a, float lo, float hi, int& fn, int& fp) {
        int err = 0, n = 2000; fn = fp = 0;
        for (int k = 0; k < n; k++) {
            float mg = lo + (hi - lo) * (k + 0.5f) / n; if (std::fabs(mg) < OFFSIDE_ENGINE_EPSILON * 2) continue;
            bool f = offsideDecision(a, mg, 2.f, 40.f, offsideHash(12345, k));
            bool t = mg > OFFSIDE_ENGINE_EPSILON;
            if (f != t) { err++; if (t) fn++; else fp++; }
        }
        return err / (float)n;
    };
    int fnG, fpG, fnA, fpA, fnW, fpW;
    float eG = rate(good, -0.2f, 0.2f, fnG, fpG), eA = rate(avg, -0.2f, 0.2f, fnA, fpA), eW = rate(worst, -0.2f, 0.2f, fnW, fpW);
    printf("close calls -0.20..+0.20 m : error good %.1f%%  average %.1f%%  worst %.1f%%  (worst: %d missed, %d wrongly flagged)\n", eG * 100, eA * 100, eW * 100, fnW, fpW);
    check(eG < eA && eA < eW, "error rate follows assistant quality");
    check(eG < 0.10f, "top assistant very accurate on close calls");
    check(eW > 0.05f && eW < 0.40f, "weak assistant makes mistakes but stays reasonable");
    check(fnA > 0 && fpA > 0 && std::abs(fnA - fpA) < (fnA + fpA) / 2 + 10, "errors happen in both directions without systematic bias");
    { float pg; bool a1 = offsideDecision(avg, 0.05f, 1.f, 30.f, 999, &pg); bool a2 = offsideDecision(avg, 0.05f, 1.f, 30.f, 999); check(a1 == a2, "decisions are deterministic for a given seed"); }
    // test 20 : situations flagrantes
    int wrong = 0;
    for (int k = 0; k < 1000; k++) {
        float mg = (k % 2 ? 1.f : -1.f) * (0.5f + (k % 97) * 0.02f);
        for (auto* a : { &good, &avg, &worst }) if (offsideDecision(*a, mg, 9.f, 89.f, offsideHash(777, k)) != (mg > 0)) wrong++;
    }
    check(wrong == 0, "100% correct decisions on obvious positions");
    // zone serrée 15-40 cm : erreurs rares
    int fn, fp; float eT = rate(worst, 0.15f, 0.40f, fn, fp);
    printf("tight 0.15..0.40 m : worst assistant error %.2f%%\n", eT * 100);
    check(eT < 0.05f, "tight zone mistakes stay exceptional");
    // assistants générés : caractéristiques bornées, reproductibles
    { Match a, b; MatchSetup s; s.home = g_world.nationIndex("FRA"); s.away = g_world.nationIndex("ITA"); s.referee = 3; s.kickoffDate = "12/05/2027";
      a.init(s); b.init(s);
      check(std::string(a.assistants[0].name) == b.assistants[0].name && a.assistants[1].offsideAccuracy == b.assistants[1].offsideAccuracy, "match officials reproducible");
      for (auto& A : a.assistants) check(A.offsideAccuracy >= 25 && A.offsideAccuracy <= 99 && A.positioning >= 25 && A.positioning <= 99, "assistant attributes bounded"); }
    printf("PASS offside: %d checks\n", checks);
}
