// Gardien en 1 contre 1 face à un joueur humain qui tente de le contourner (crochet au dernier moment)
#include "../src/match.h"
#include <cmath>
#include <memory>
int main(int argc, char** argv) {
    g_world.build();
    int n = argc > 1 ? atoi(argv[1]) : 60, goals = 0, rounded = 0;
    for (int k = 0; k < n; k++) {
        auto m = std::make_unique<Match>(); MatchSetup s; s.home = g_world.nationIndex("FRA"); s.away = g_world.nationIndex("ITA");
        for (int i = 0; i < NUM_INPUTS; i++) s.side[i] = -1; s.side[IN_KB1] = 0;
        g_rng = Rng(1000 + k);
        m->init(s); Match& M = *m; M.ceremony = false; M.cerPhase = 99; M.state = MS_PLAY; M.period = 1; M.clock = 20;
        int att = 9, gk = 11; V2 g = M.goalCenter(0); float sg = g.y < 1 ? -1.f : 1.f;
        for (int i = 0; i < 22; i++) { M.pl[i].vel = V2(); M.pl[i].state = PS_NORMAL; if (i != att && i != gk) M.pl[i].pos = V2(5.f + (i % 11) * 3.f, PITCH_L / 2 - sg * 20.f); }
        M.pl[att].pos = V2(PITCH_W / 2 + (k % 5 - 2) * 2.f, g.y - sg * 22.f); M.pl[gk].pos = V2(PITCH_W / 2, g.y - sg * 1.f);
        M.ball.pos = M.pl[att].pos; M.takePossession(att); M.ctrlPlayer[IN_KB1] = att;
        float side = k % 2 ? 1.f : -1.f; bool cut = false;
        for (int f = 0; f < 60 * 8 && M.state == MS_PLAY; f++) {
            Controls ct; MPlayer& p = M.pl[att];
            float dGk = (M.pl[gk].pos - p.pos).len();
            if (!cut && dGk < 4.5f) cut = true;
            if (M.ball.owner == att) {
                if (!cut) ct.dir = (g - p.pos).norm();
                else if (std::fabs(p.pos.x - M.pl[gk].pos.x) < 2.2f && (g - p.pos).len() > 3.f) ct.dir = V2(side, sg * 0.25f).norm();    // crochet
                else { ct.dir = (g - p.pos).norm(); if ((g - p.pos).len() < 9.f) { ct.f1 = true; } }
            } else ct.dir = (M.ball.pos - p.pos).norm();
            ct.f1p = ct.f1; ct.sprint = true;
            M.ctl[IN_KB1] = ct; M.update(1.f / 60); M.sfxN = 0;
            if (cut && M.ball.owner == att && std::fabs(p.pos.x - M.pl[gk].pos.x) > 2.5f && sg * (p.pos.y - M.pl[gk].pos.y) > -1.f) { rounded++; cut = false; side = 0; }
            if (M.state == MS_GOAL) { goals++; break; }
        }
    }
    printf("1v1 human: goals %d/%d (%.0f%%), keeper rounded %d\n", goals, n, 100.f * goals / n, rounded);
}
