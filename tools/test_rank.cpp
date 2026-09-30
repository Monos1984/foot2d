#include "../src/game.h"
#include <cstdio>
#include <cstring>
int main(){
    g_world.build();
    int u=-1; for (int i=g_world.firstClub;i<(int)g_world.teams.size();i++) if (g_world.teams[i].name=="Paris Saint-Germain") u=i;
    g_career.newClubCareer(u,2026);
    auto cnt=[&](const EuroSpots& E,const char* code){ int a=0,b=0; for (auto* l : {&E.uclGS,&E.uclQ3,&E.uclQ2,&E.uclQ1}) for(int t:*l) if(!strcmp(NATIONS[g_world.teams[t].nation].code,code)) a++; for (auto* l : {&E.uefaR1,&E.uefaQR}) for(int t:*l) if(!strcmp(NATIONS[g_world.teams[t].nation].code,code)) b++; printf("%s rank %d: C1 %d C3 %d\n",code,g_career.uefaRank(code),a,b); };
    for (auto c : {"POR","SCO","FRA","ESP"}) cnt(g_career.nextEuro,c);
    for (auto& x : g_career.uefa) if (x.code=="SCO") for (auto& p : x.pts) p = 30;
    for (auto& x : g_career.uefa) if (x.code=="ESP") for (auto& p : x.pts) p = 1;
    auto E=g_career.previewEuro();
    printf("-- after change (preview)\n");
    for (auto c : {"POR","SCO","FRA","ESP"}) cnt(E,c);
    // simulate season to the end
    auto& S=g_career.season; while(true){ auto pm=S.advance(true); if(pm.comp<0)break; }
    g_career.endSeason();
    printf("-- after endSeason\n");
    for (auto c : {"POR","SCO","FRA","ESP"}) cnt(g_career.nextEuro,c);
}
