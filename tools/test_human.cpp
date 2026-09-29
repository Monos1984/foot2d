#include "../src/match.h"
int main(int argc, char** argv) {
    g_world.build();
    int n = argc > 1 ? atoi(argv[1]) : 5;
    int tg[2]={0,0};
    for (int k=0;k<n;k++){
        Match m; MatchSetup s; s.home=g_world.nationIndex("FRA"); s.away=g_world.nationIndex("ITA");
        for (int i=0;i<NUM_INPUTS;i++) s.side[i]=-1; s.side[IN_KB1]=0; if (k%2) s.side[IN_KB2]=1; s.halfSeconds=90; s.decisive = true;
        m.init(s);
        Controls prev[NUM_INPUTS];
        int frames=0; float hold[NUM_INPUTS]={0}; int stuck=0; int lastState=-1; float stateTime=0;
        while(!m.finished && frames < 60*60*20){
            for (int c=0;c<2;c++){
                if (s.side[c]<0) continue;
                Controls ct;
                int p = m.ctrlPlayer[c];
                int team = s.side[c];
                if (m.state==MS_SETPIECE) {
                    // tireur humain : appuie après un moment
                    ct.dir = V2(0,0);
                    if (m.spReady && ((frames/40)%2==0)) ct.f1 = true;
                } else if (p>=0) {
                    const MPlayer& pl = m.pl[p];
                    if (m.ball.owner==p) {
                        V2 g = m.goalCenter(team);
                        ct.dir = (g - pl.pos).norm();
                        float d=(g-pl.pos).len();
                        if (d<22) { hold[c]+=1.f/60; ct.f1 = hold[c] < 0.35f; if (!ct.f1) hold[c]=0; }
                        else if (frames%90==0) ct.f1 = true; // passe
                    } else {
                        ct.dir = (m.ball.pos - pl.pos).norm();
                        if ((m.ball.pos-pl.pos).len()<2.5f && m.ball.owner>=0 && m.pl[m.ball.owner].team!=team && frames%50==0) ct.f2=true;
                    }
                    if (m.pl[p].state==PS_GKHOLD && frames%60==0) ct.f2=true;
                }
                ct.f1p = ct.f1 && !prev[c].f1; ct.f2p = ct.f2 && !prev[c].f2; ct.f1r = !ct.f1 && prev[c].f1; ct.f2r=!ct.f2&&prev[c].f2;
                prev[c]=ct; m.ctl[c]=ct;
            }
            m.update(1.f/60); m.sfxN=0; frames++;
            if (m.state==lastState) stateTime+=1.f/60; else { stateTime=0; lastState=m.state; }
            if (m.state==MS_SETPIECE && stateTime>20) { printf("  STUCK in setpiece %d kicker %d human %d\n", m.sp, m.spKicker, m.spKicker>=0?m.pl[m.spKicker].human:-9); stuck++; break; }
        }
        printf("match %d: FRA %d-%d ITA %s %s frames %d finished %d\n", k, m.score[0], m.score[1], m.aet?"ap":"", m.shootout?fmt("tab %d-%d",m.pens[0],m.pens[1]).c_str():"", frames, m.finished);
        tg[0]+=m.score[0]; tg[1]+=m.score[1];
    }
    printf("human goals %d cpu goals %d\n", tg[0], tg[1]);
}
