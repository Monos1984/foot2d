#include "../src/game.h"
int main(){
    g_world.build();
    // club créé
    Team t; t.name="AS Test Monos"; t.shortName="ASTM"; t.dept=0; t.region=DEPTS[0].region; t.kind=TK_CLUB; t.nation=g_world.nationIndex("FRA"); t.rating=30; t.culture=CU_FR;
    int idx=g_world.addCustomClub(t);
    g_career.newClubCareer(idx,2026);
    printf("club créé: %s -> %s\n", g_world.teams[idx].name.c_str(), g_career.teamLevelName(idx).c_str());
    auto& S=g_career.season;
    while(true){ auto pm=S.advance(true); if(pm.comp<0) break; }
    g_career.endSeason();
    printf("après 1 saison: %s\n", g_career.teamLevelName(idx).c_str());
    // club créé en cours de saison
    Team t2=t; t2.name="FC Nouveau"; int i2=g_world.addCustomClub(t2); g_career.addClubToPyramid(i2);
    printf("pendant la saison: '%s' pending=%zu\n", g_career.teamLevelName(i2).c_str(), g_career.pendingNewClubs.size());
    while(true){ auto pm=S.advance(true); if(pm.comp<0) break; }
    g_career.endSeason();
    printf("saison suivante: %s\n", g_career.teamLevelName(i2).c_str());
    // compétitions perso
    for(int f=0;f<3;f++){
        CustomCompDef d; d.name="Test"; d.format=f; d.legs=f==1?1:2; d.groups=4;
        for(int i=0;i<(f==1?13:16);i++) d.teams.push_back(g_world.nationIndex("FRA")+0*i), d.teams.back()=i;
        g_career.newCustom(d,{d.teams[0]});
        auto& S2=g_career.season; int guard=0;
        while(guard++<100000){ auto pm=S2.advance(true); if(pm.comp<0) break; }
        auto& C=S2.comps[0];
        printf("format %d: done=%d finished=%d winner=%d result=%zu stages=%zu\n", f, C.done, S2.finished, C.winner, C.result.size(), C.stages.size());
    }
}
