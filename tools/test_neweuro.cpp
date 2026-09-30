#include "../src/game.h"
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
int main(int argc,char**argv){
    g_world.build();
    int u=-1; for (int i=g_world.firstClub;i<(int)g_world.teams.size();i++) if (g_world.teams[i].name=="Paris Saint-Germain") u=i;
    g_career.opts.euroFormat=1; g_career.opts.awayGoals=0;
    bool euroOnly = argc>1;
    if (euroOnly) g_career.newEuroCareer({u},2026); else g_career.newClubCareer(u,2026);
    for (int season=0; season<2; season++){
    auto& S=g_career.season;
    int guard=0; while(guard++<200000){ auto pm=S.advance(true); if(pm.comp<0)break; }
    for (int ci : {g_career.ucl,g_career.uel,g_career.uecl}){
        auto& C=S.comps[ci];
        printf("== %s done=%d winner=%s awaiting=%d stages=%zu\n", C.name.c_str(), C.done, C.winner>=0?g_world.teams[C.winner].name.c_str():"-", C.awaiting, C.stages.size());
        for (auto& st: C.stages){
            int nm=0; for (auto&R:st.rounds) nm+=R.m.size();
            printf("   %-28s type%d ties%zu matches%d t=%.1f\n", st.name.c_str(), st.type, st.ties.size(), nm, st.rounds.empty()?-1:st.rounds[0].time);
            if (st.type==ST_SWISS){
                auto& T=st.groups[0]; printf("      LP teams %zu\n", T.size());
                std::map<int,int> home, games, sameNat; std::map<int,std::map<int,int>> perTeamRound;
                for (size_t r=0;r<st.rounds.size();r++) for(int mi:st.rounds[r].m){ auto&m=C.matches[mi]; home[m.home]++; games[m.home]++; games[m.away]++; perTeamRound[m.home][r]++; perTeamRound[m.away][r]++; if (g_world.teams[m.home].nation==g_world.teams[m.away].nation) sameNat[0]++; }
                int bad=0; for (int t:T){ if (games[t]!=(int)st.rounds.size()) bad++; for (auto&kv:perTeamRound[t]) if (kv.second>1) bad++; }
                int hmin=99,hmax=0; for(int t:T){hmin=std::min(hmin,home[t]);hmax=std::max(hmax,home[t]);}
                printf("      bad=%d home %d-%d sameNat=%d\n", bad,hmin,hmax,sameNat[0]);
                auto tb=C.table(&st-&C.stages[0],0); for (int i=0;i<3;i++) printf("      %d %s %d pts\n", i+1, g_world.teams[tb[i].team].name.c_str(), tb[i].pts);
            }
        }
    }
    // French clubs
    for (int ci : {g_career.ucl,g_career.uel,g_career.uecl}){ auto& C=S.comps[ci]; printf("%s FR: ", C.shortName.c_str()); std::set<int> in; for(auto&st:C.stages){for(auto&g:st.groups)for(int t:g)in.insert(t); for(auto&t:st.ties){in.insert(t.a); if(t.b>=0)in.insert(t.b);}} for(int t:in) if(t>=0&&!strcmp(NATIONS[g_world.teams[t].nation].code,"FRA")) printf("%s, ", g_world.teams[t].shortName.c_str()); printf("\n"); }
    g_career.endSeason(); g_career.startSeason();
    printf("---- next season FRA rank %d\n", g_career.uefaRank("FRA"));
    }
}
