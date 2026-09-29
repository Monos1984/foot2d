#include "../src/match.h"
#include <map>
int main(int argc, char** argv) {
    g_world.build();
    int n = argc > 1 ? atoi(argv[1]) : 10;
    int fra = g_world.nationIndex("FRA"), bra = g_world.nationIndex("BRA"), smr = g_world.nationIndex("SMR");
    int tg=0, tf=0, ty=0, tr=0, toff=0, tpen=0, tcorn=0;
    std::map<int,int> spCount;
    for (int k = 0; k < n; k++) {
        Match m;
        MatchSetup s; s.home = k%3==2? smr: fra; s.away = bra; for (int i=0;i<NUM_INPUTS;i++) s.side[i]=-1; s.halfSeconds = 120; s.decisive = (k%4==0);
        s.pitch = k%5;
        m.init(s);
        int frames=0; int lastSp=-1; int lastState=-1;
        while (!m.finished && frames < 60*60*30) {
            m.update(1.f/60);
            if (m.state==MS_SETPIECE && lastState!=MS_SETPIECE) spCount[m.sp]++;
            if (m.state==MS_STOP && lastState!=MS_STOP && m.msg=="HORS-JEU") toff++;
            lastState=m.state;
            frames++;
            m.sfxN=0;
        }
        int y=0,r=0; for (auto&e:m.events){ if(e.type==1)y++; if(e.type==2)r++; }
        printf("match %d: %s %d-%d %s %s%s  shots %d/%d poss %.0f%% cards Y%d R%d  frames %d\n", k, m.team(0).shortName.c_str(), m.score[0], m.score[1], m.team(1).shortName.c_str(),
           m.aet?"ap ":"", m.shootout? fmt("tab %d-%d",m.pens[0],m.pens[1]).c_str():"", m.shots[0], m.shots[1], 100*m.possTime[0]/(m.possTime[0]+m.possTime[1]+0.001f), y, r, frames);
        tg += m.score[0]+m.score[1]; ty+=y; tr+=r;
    }
    printf("avg goals %.2f, yellows %.2f reds %.2f offsides %.2f\n", tg/(float)n, ty/(float)n, tr/(float)n, toff/(float)n);
    const char* SPN[]={"kickoff","throwin","corner","goalkick","freekick","penalty","indirect","shootout","gkball"};
    for (auto& kv: spCount) printf("  %s: %.1f per match\n", SPN[kv.first], kv.second/(float)n);
}
