// build 27 : l'arbitre ne colle pas le tireur de corner ; effet L2 / R2 sur corner
#include "../src/match.h"
#include <cmath>
#include <cstdio>
#include <memory>
static int fails = 0, checks = 0;
static void check(bool c, const char* m) { checks++; if (!c) { fails++; printf("FAIL %s\n", m); } }
int main() {
    g_world.build();
    for (int corner = 0; corner < 4; corner++) {
        auto m = std::make_unique<Match>();
        MatchSetup s; s.home = g_world.nationIndex("FRA"); s.away = g_world.nationIndex("BRA"); for (int i = 0; i < NUM_INPUTS; i++) s.side[i] = -1;
        m->init(s); m->ceremony = false; m->cerPhase = 99; m->clock = 30; m->period = 1;
        V2 spot((corner & 1) ? PITCH_W - 0.3f : 0.3f, (corner & 2) ? PITCH_L - 0.3f : 0.3f);
        int t = m->goalCenter(0).y * 1.f > PITCH_L / 2 == ((corner & 2) != 0) ? 0 : 1;
        m->refPos = spot + V2((corner & 1) ? -1.f : 1.f, (corner & 2) ? -1.f : 1.f);
        m->beginSetPiece(SP_CORNER, t, spot);
        float minD = 1e9f;
        for (int k = 0; k < 60 * 4 && m->state == MS_SETPIECE; k++) { m->update(1.f / 60); if (k > 60 * 1.5f) minD = std::min(minD, (m->refPos - spot).len()); }
        char b[96]; snprintf(b, sizeof b, "corner %d: referee keeps away from taker (%.1f m)", corner, minD);
        check(minD > 8.f && minD < 100.f, b); printf("%s\n", b);
    }
    printf("%s corner ref: %d checks\n", fails ? "FAIL" : "PASS", checks);
    return fails ? 1 : 0;
}
