#include "../src/match.h"
int main(int argc, char** argv) {
    g_world.build();
    int n = atoi(argv[1]);
    int a = g_world.nationIndex(argv[2]), b = g_world.nationIndex(argv[3]);
    int gh=0, ga=0, wh=0, wa=0; int goalsTop[2]={0,0};
    int shotsH=0, shotsA=0;
    for (int k = 0; k < n; k++) {
        Match m; MatchSetup s; s.home=a; s.away=b; for (int i=0;i<NUM_INPUTS;i++) s.side[i]=-1; s.halfSeconds=120;
        m.init(s);
        int prevScore[2]={0,0};
        while (!m.finished) { m.update(1.f/60); m.sfxN=0;
            for (int t=0;t<2;t++) if (m.score[t]!=prevScore[t]) { prevScore[t]=m.score[t]; goalsTop[m.attackDir[t]<0?0:1]++; } }
        gh+=m.score[0]; ga+=m.score[1]; if(m.score[0]>m.score[1])wh++; if(m.score[1]>m.score[0])wa++;
        shotsH+=m.shots[0]; shotsA+=m.shots[1];
    }
    printf("home goals %d away %d, wins H%d A%d, goals into top goal %d bottom %d, shots %d/%d\n", gh, ga, wh, wa, goalsTop[0], goalsTop[1], shotsH, shotsA);
}
