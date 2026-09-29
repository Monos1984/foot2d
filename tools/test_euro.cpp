#include "../src/game.h"
static void dump(const char* lbl, int ci){
    auto& S=g_career.season; if(ci<0){printf("%s: none\n",lbl);return;}
    auto& C=S.comps[ci];
    printf("%s:", lbl);
    for (auto& st: C.stages) printf(" [%s %zu%s t=%.1f]", st.name.c_str(), st.groups.empty()? st.ties.size(): st.groups.size(), st.groups.empty()?"":"g", st.rounds.empty()?0:st.rounds[0].time);
    printf(" winner=%s\n", C.winner>=0? g_world.teams[C.winner].name.c_str():"-");
}
int main(){
    g_world.build();
    int user=-1; for (int i=0;i<(int)g_world.teams.size();i++) if (g_world.teams[i].name=="Stade Lavallois") user=i;
    g_career.newClubCareer(user, 2026);
    auto& K=g_career; auto& S=K.season;
    for (int y=0;y<2;y++){
        auto& E=K.nextEuro;
        printf("season %d\n", K.year);
        while(true){ auto pm=S.advance(true); if(pm.comp<0)break; }
        dump("UCL",K.ucl); dump("UEFA",K.uel); dump("Intertoto",K.intertoto); dump("SuperUEFA",K.uefaSuper);
        for (int c: K.superCups) if (S.comps[c].tag==0 || S.comps[c].name.find("Troph")!=std::string::npos) dump(S.comps[c].name.c_str(), c);
        K.endSeason();
        printf("next: Q1 %zu Q2 %zu Q3 %zu GS %zu | QR %zu R1 %zu | IT %zu/%zu/%zu  honours %zu\n", E.uclQ1.size(),E.uclQ2.size(),E.uclQ3.size(),E.uclGS.size(),E.uefaQR.size(),E.uefaR1.size(),E.itR1.size(),E.itR2.size(),E.itR3.size(), K.honourLog.size());
    }
    for (auto& h: K.honourLog) printf("  %s %d %s / %s\n", honourCompName(h.comp), h.year, h.winner>=0?g_world.teams[h.winner].name.c_str():"-", h.runner>=0?g_world.teams[h.runner].name.c_str():"-");
}
