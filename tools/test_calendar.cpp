#include "../src/game.h"
// vérifie : pas plus de 2 matchs consécutifs à domicile / à l'extérieur en championnat
int main(){
    g_world.build();
    int user=-1; for (int i=0;i<(int)g_world.teams.size();i++) if (g_world.teams[i].name=="Stade Lavallois") user=i;
    g_career.newClubCareer(user, 2026);
    auto& S=g_career.season;
    int bad=0, leagues=0, worst=0;
    for (auto& C: S.comps){
        if (C.format!=FMT_LEAGUE || C.stages.empty()) continue;
        leagues++;
        std::map<int,std::vector<std::pair<double,int>>> seq;
        for (auto& st: C.stages) for (auto& R: st.rounds) for (int mi: R.m){ auto& m=C.matches[mi]; seq[m.home].push_back({R.time,1}); seq[m.away].push_back({R.time,0}); }
        for (auto& kv: seq){ auto v=kv.second; std::sort(v.begin(),v.end()); int run=0,last=-1,mx=0; for(auto&x:v){ if(x.second==last) run++; else {run=1; last=x.second;} mx=std::max(mx,run);} worst=std::max(worst,mx); if(mx>2) bad++; }
    }
    printf("leagues %d, teams with >2 in a row: %d, worst streak %d\n", leagues, bad, worst);
    // coefficients clubs
    std::vector<int> v; for (int t=0;t<(int)g_world.teams.size();t++) if (g_world.teams[t].coefTotal()>0) v.push_back(t);
    std::sort(v.begin(),v.end(),[](int a,int b){return g_world.teams[a].coefTotal()>g_world.teams[b].coefTotal();});
    for (int i=0;i<8 && i<(int)v.size();i++) printf("  %s %.1f seed %.1f\n", g_world.teams[v[i]].name.c_str(), g_world.teams[v[i]].coefTotal(), clubSeed(v[i]));
    printf("superRegions %d : %s\n", g_career.superRegions, g_career.superRegions>=0 ? S.comps[g_career.superRegions].name.c_str() : "-");
    if (g_career.superRegions>=0) { auto& C=S.comps[g_career.superRegions]; for (auto& t: C.entrants[0]) printf("   %s (%s)\n", g_world.teams[t].name.c_str(), g_career.teamLevelName(t).c_str()); }
}
