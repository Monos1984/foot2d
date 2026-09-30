#include "../src/game.h"
#include <cstdio>
#include <map>
int main(){
    g_world.build();
    int u=-1; for (int i=g_world.firstClub;i<(int)g_world.teams.size();i++) if (g_world.teams[i].name=="Stade Brestois") u=i;
    g_career.newClubCareer(u,2026);
    auto& S=g_career.season; while(true){ auto pm=S.advance(true); if(pm.comp<0)break; }
    std::map<int,int> tot, margin; int n=0; double sum=0; int big=0; std::map<int,int> kindBig;
    for (auto& C: S.comps) for (auto& m: C.matches) if (m.played && m.hg>=0) { n++; int t=m.hg+m.ag; sum+=t; tot[std::min(t,12)]++; int mg=abs(m.hg-m.ag); margin[std::min(mg,10)]++; if (mg>=6) {big++; kindBig[C.kind]++;} }
    printf("matches %d avg goals %.2f  margin>=6: %d (%.2f%%)\n", n, sum/n, big, 100.0*big/n);
    for (auto&kv:margin) printf("  margin %d: %.2f%%\n", kv.first, 100.0*kv.second/n);
    for (auto&kv:kindBig) printf("  kind %d big %d\n", kv.first, kv.second);
    for (int i=0;i<(int)S.comps.size();i++){ auto&C=S.comps[i]; if (C.kind==15||C.kind==16) { static int cnt=0; if (cnt++<3 || C.kind==15) printf("Gambardella comp %s done=%d winner=%s matches=%zu\n", C.name.c_str(), C.done, C.winner>=0?g_world.teams[C.winner].name.c_str():"-", C.matches.size()); } }
}
