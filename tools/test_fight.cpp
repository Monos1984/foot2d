#include "../src/match.h"
#include <cstdio>
int main() {
    g_world.build();
    int fra = g_world.nationIndex("FRA"), bra = g_world.nationIndex("BRA");
    int fights = 0, duels = 0, scuffles = 0, aband = 0, gg = 0;
    for (int k = 0; k < 12; k++) {
        Match m; MatchSetup s; s.home = fra; s.away = bra; for (int i = 0; i < NUM_INPUTS; i++) s.side[i] = -1; s.halfSeconds = 90; s.decisive = true; s.goldenGoal = k % 2; s.anthems = true;
        s.sevOverride = 90;
        m.init(s);
        int frames = 0; bool inF = false;
        while (!m.finished && frames < 60 * 60 * 40) {
            // provoquer : énervement maximal de temps en temps
            if (frames % 3000 == 0) for (auto& p : m.pl) p.anger = std::min(1.f, p.anger + 0.4f);
            m.update(1.f / 60); frames++; m.sfxN = 0;
            if (m.fightT > 0 && !inF) { inF = true; if (m.fightLevel == 1) scuffles++; else fights++; if (m.duel) duels++; }
            if (m.fightT <= 0) inF = false;
        }
        if (m.abandoned) aband++;
        printf("match %d: %d-%d %s %s frames %d finished %d\n", k, m.score[0], m.score[1], m.aet ? "ap" : "", m.shootout ? fmt("tab %d-%d", m.pens[0], m.pens[1]).c_str() : "", frames, m.finished);
    }
    printf("bousculades %d bagarres %d combats %d arrêtés %d\n", scuffles, fights, duels, aband);
}
