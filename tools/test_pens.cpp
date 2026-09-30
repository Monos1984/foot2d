#include "../src/match.h"
#include <cstdio>
int main() {
    g_world.build();
    int fra = g_world.nationIndex("FRA"), bra = g_world.nationIndex("BRA");
    int st[4] = {0,0,0,0}, n = 0; int zh[10] = {};
    for (int k = 0; k < 40; k++) {
        Match m; MatchSetup s; s.home = fra; s.away = bra; for (int i = 0; i < NUM_INPUTS; i++) s.side[i] = -1; s.halfSeconds = 90; s.decisive = true;
        m.init(s);
        m.ceremony = false; m.startPeriod(1); m.clock = 120; m.state = MS_BREAK; m.nextSp = 4; m.stateT = 2.95f; m.tossDoneTAB = true; m.tossDoneET = true;
        int frames = 0, lastTaken = 0;
        while (!m.finished && frames < 60 * 60 * 10) {
            float py = m.ball.pos.y, pz = m.ball.z;
            m.update(1.f / 60); frames++; m.sfxN = 0;
            if (m.shootout && m.state == MS_PLAY && ((py > 0 && m.ball.pos.y <= 0) || (py < PITCH_L && m.ball.pos.y >= PITCH_L))) zh[std::min(9, (int)(pz / 0.5f))]++;
            int tk = m.penTaken[0] + m.penTaken[1];
            if (tk != lastTaken) { lastTaken = tk; st[m.lastPenMiss]++; n++; if (m.penMaxZ > 2.2f) printf("haut z%.2f res %d ball(%.1f,%.1f,%.1f) lt %d\n", m.penMaxZ, m.lastPenMiss, m.ball.pos.x, m.ball.pos.y, m.ball.z, m.ball.lastTouch); }
        }
    }
    for (int i = 0; i < 10; i++) printf("z%.1f:%d ", i * 0.5, zh[i]); printf("\n");
    printf("tirs %d : marqués %.0f%%  arrêtés %.0f%%  au-dessus %.0f%%  à côté/poteau %.0f%%\n", n, 100.0*st[0]/n, 100.0*st[1]/n, 100.0*st[2]/n, 100.0*st[3]/n);
}
