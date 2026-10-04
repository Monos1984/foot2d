// build 28 : remplacement — le joueur remplacé sort complètement avant que le remplaçant n'entre ; mi-temps : attente avant le retour au vestiaire
#include "../src/match.h"
#include <cmath>
#include <memory>
static int fails = 0, checks = 0;
static void check(bool c, const char* m) { checks++; if (!c) { fails++; printf("FAIL %s\n", m); } }
int main() {
    g_world.build();
    for (int side = 0; side < 2; side++) {
        auto m = std::make_unique<Match>();
        MatchSetup s; s.home = g_world.nationIndex("FRA"); s.away = g_world.nationIndex("BRA"); for (int i = 0; i < NUM_INPUTS; i++) s.side[i] = -1;
        m->init(s); m->ceremony = false; m->cerPhase = 99; m->startPeriod(0); m->state = MS_PLAY; m->clock = 30;
        int slot = 6; MPlayer& p = m->pl[slot];
        p.pos = V2(side ? PITCH_W - 12.f : 12.f, PITCH_L * 0.7f);
        m->substitute(0, slot, 0);
        m->state = MS_STOP; m->stateT = 0; m->nextSp = SP_THROWIN; m->nextSpTeam = 0; m->nextSpPos = V2(0.2f, 45.f);
        bool sawWalker = false, enteredEarly = false, entered = false; float tOut = -1, tIn = -1;
        for (int k = 0; k < 60 * 25 && m->state == MS_STOP; k++) {
            m->update(1.f / 60);
            bool w = m->subOutWalking();
            if (w) sawWalker = true;
            bool inside = m->pl[slot].pos.x >= 0;
            if (w && m->pl[slot].pos.x > -0.85f) enteredEarly = true;
            if (sawWalker && !w && tOut < 0) tOut = k / 60.f;
            if (inside && !entered) { entered = true; tIn = k / 60.f; }
        }
        printf("side %d : sortie %.1f s, entrée %.1f s\n", side, tOut, tIn);
        check(sawWalker, "le joueur remplacé sort en marchant");
        check(!enteredEarly, "le remplaçant attend la sortie complète");
        check(entered && tIn >= tOut, "le remplaçant entre après");
    }
    {   // mi-temps
        auto m = std::make_unique<Match>();
        MatchSetup s; s.home = g_world.nationIndex("FRA"); s.away = g_world.nationIndex("BRA"); for (int i = 0; i < NUM_INPUTS; i++) s.side[i] = -1;
        m->init(s); m->ceremony = false; m->cerPhase = 99; m->startPeriod(0); m->state = MS_PLAY; m->clock = 30;
        m->endPeriod();
        V2 p0 = m->pl[5].pos;
        for (int k = 0; k < 60 * 3; k++) m->update(1.f / 60);
        check((m->pl[5].pos - p0).len() < 3.f && m->state == MS_BREAK, "mi-temps : les joueurs restent sur la pelouse 3 s");
        for (int k = 0; k < 60 * 20 && !m->htWaiting; k++) m->update(1.f / 60);
        check(m->htWaiting || m->period == 1, "mi-temps : retour au vestiaire ensuite");
    }
    printf("%s sub seq: %d checks\n", fails ? "FAIL" : "PASS", checks);
    return fails ? 1 : 0;
}
