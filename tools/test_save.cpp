#include "../src/game.h"
int main(){
    g_world.build();
    int user=-1; for (int i=0;i<(int)g_world.teams.size();i++) if (g_world.teams[i].name=="US Saint-Malo") user=i;
    g_career.newClubCareer(user, 2026);
    printf("user level: %s\n", g_career.teamLevelName(user).c_str());
    for (int k=0;k<10;k++){ auto pm=g_career.season.advance(false); if(pm.comp<0)break; auto&m=g_career.season.comps[pm.comp].matches[pm.match]; simulateMatch(m,&g_career.season.comps[pm.comp]); g_career.season.recordResult(pm.comp,pm.match); g_career.season.checkRound(pm.comp);}
    g_world.ensureSquad(user);
    double now=g_career.season.now; size_t nc=g_career.season.comps.size();
    bool ok=g_career.save("/tmp/t.sav"); printf("save %d now %.2f comps %zu\n", ok, now, nc);
    ok=g_career.load("/tmp/t.sav"); printf("load %d now %.2f comps %zu squad %zu\n", ok, g_career.season.now, g_career.season.comps.size(), g_world.teams[user].squad.size());
    int um=0; while(true){ auto pm=g_career.season.advance(false); if(pm.comp<0)break; auto&m=g_career.season.comps[pm.comp].matches[pm.match]; simulateMatch(m,&g_career.season.comps[pm.comp]); g_career.season.recordResult(pm.comp,pm.match); g_career.season.checkRound(pm.comp); um++;}
    printf("after load: %d more user matches, finished %d\n", um, g_career.season.finished);
    g_career.endSeason(); printf("new level: %s\n", g_career.teamLevelName(user).c_str());
    FILE*f=fopen("/tmp/t.sav","rb"); fseek(f,0,SEEK_END); printf("save size %.1f MB\n", ftell(f)/1e6); fclose(f);
}
