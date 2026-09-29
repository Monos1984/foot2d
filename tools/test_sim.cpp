#include "../src/game.h"
#include <chrono>
#include <map>
static void playSeason(int& um){
    while(true){
        PendingMatch pm = g_career.season.advance(false);
        if (pm.comp<0) break;
        auto& m = g_career.season.comps[pm.comp].matches[pm.match];
        simulateMatch(m, &g_career.season.comps[pm.comp]);
        g_career.season.recordResult(pm.comp, pm.match);
        g_career.season.checkRound(pm.comp);
        um++;
    }
}
static void dumpFR(){
    for (auto& P : g_career.pyramids) if (P.country=="FRA" && P.dom<0) {
        for (int t=0;t<(int)P.tiers.size();t++){ int n=0,g=0,res=0; std::vector<int> sizes; for(auto&pl:P.pools) if(pl.tier==t){n+=pl.clubs.size(); g+=pl.groups.size(); for(auto&gg:pl.groups) sizes.push_back(gg.size()); for(int c:pl.clubs) if(g_world.teams[c].parent>=0) res++;} if(sizes.empty())continue; int mn=*std::min_element(sizes.begin(),sizes.end()), mx=*std::max_element(sizes.begin(),sizes.end()); printf("  %-16s %5d équipes %4d groupes (%d-%d) dont %d réserves\n", P.tiers[t].name.c_str(), n, g, mn, mx, res);}
    }
}
int main() {
    auto t0 = std::chrono::steady_clock::now();
    g_world.build();
    auto t1 = std::chrono::steady_clock::now();
    printf("teams=%zu build=%.2fs pyramids=%zu\n", g_world.teams.size(), std::chrono::duration<double>(t1-t0).count(), g_basePyramids.size());
    int user = -1;
    for (int i=0;i<(int)g_world.teams.size();i++) if (g_world.teams[i].name=="Stade Brestois") user=i;
    g_career.newClubCareer(user, 2026);
    dumpFR();
    printf("comps=%zu\n", g_career.season.comps.size());
    for (int s=0;s<3;s++){
        auto a=std::chrono::steady_clock::now();
        int um=0; playSeason(um);
        auto b=std::chrono::steady_clock::now();
        auto& S=g_career.season;
        int notdone=0; for(auto&c:S.comps) if(!c.done) { notdone++; if(notdone<5) printf("   NOT DONE: %s (stages %zu awaiting %d)\n", c.name.c_str(), c.stages.size(), c.awaiting); }
        printf("season %d: %.2fs, user matches %d, comps %zu not done %d\n", g_career.year, std::chrono::duration<double>(b-a).count(), um, S.comps.size(), notdone);
        auto W=[&](int c){ return c>=0 && S.comps[c].winner>=0 ? g_world.teams[S.comps[c].winner].name : std::string("-"); };
        printf("  CdF: %s | C1: %s | C3: %s | SuperUEFA: %s\n", W(g_career.cdf).c_str(), W(g_career.ucl).c_str(), W(g_career.uel).c_str(), W(g_career.uefaSuper).c_str());
        for (int c: g_career.superCups) printf("  %s -> %s\n", S.comps[c].name.c_str(), W(c).c_str());
        for (auto& c : S.comps) if (c.kind==10) printf("  %s -> %s\n", c.name.c_str(), c.winner>=0?g_world.teams[c.winner].name.c_str():"?");
        const Competition& U=S.comps[g_career.ucl];
        printf("  C1 stages:"); for(auto&st:U.stages) printf(" [%s %zu/%zu]", st.name.c_str(), st.ties.size(), st.groups.size()); printf("\n");
        const Competition& E=S.comps[g_career.uel];
        printf("  C3 stages:"); for(auto&st:E.stages) printf(" [%s %zu]", st.name.c_str(), st.ties.size()); printf("\n");
        if (g_career.cdfNational>=0){ const Competition& C=S.comps[g_career.cdfNational]; printf("  CdF nat:"); for(auto&st:C.stages) printf(" [%s %zu]", st.name.c_str(), st.ties.size()); printf("\n"); }
        const Competition& R0=S.comps[g_career.cdfRegional[7]]; printf("  %s:", R0.name.c_str()); for(auto&st:R0.stages) printf(" [%s %zu]", st.name.c_str(), st.ties.size()); printf(" -> %zu qualifiés\n", R0.result.size());
        g_career.endSeason();
        printf("  user now: %s\n", g_career.teamLevelName(user).c_str());
        dumpFR();
        std::vector<std::pair<float,std::string>> co; for(auto&u:g_career.uefa) co.push_back({-u.total(),u.code}); std::sort(co.begin(),co.end());
        printf("  UEFA:"); for(int i=0;i<10;i++) printf(" %s %.1f", co[i].second.c_str(), -co[i].first); printf("\n");
        printf("  nextEuro Q1 %zu Q2 %zu Q3 %zu GS %zu | UEFA QR %zu R1 %zu\n", g_career.nextEuro.uclQ1.size(), g_career.nextEuro.uclQ2.size(), g_career.nextEuro.uclQ3.size(), g_career.nextEuro.uclGS.size(), g_career.nextEuro.uefaQR.size(), g_career.nextEuro.uefaR1.size());
    }
    // réserves : vérification des contraintes
    int bad=0; for (int i=0;i<(int)g_world.teams.size();i++){ const Team&t=g_world.teams[i]; if(t.parent<0) continue; int pp,qq,gg; int a=g_career.tierOfTeam(i,&pp,&qq,&gg), b=g_career.tierOfTeam(t.parent); bool term = a>=0 && g_career.pyramids[pp].pools[qq].terminal; if(a>=0&&b>=0&&(a<b || (a==b && !term))) bad++; if (a>=0 && a<=2) bad++; }
    printf("violations réserves: %d\n", bad);
    { std::map<std::string,int> cat; int shown=0; for (int i=0;i<(int)g_world.teams.size();i++){ const Team&t=g_world.teams[i]; if(t.parent<0) continue; int a=g_career.tierOfTeam(i), b=g_career.tierOfTeam(t.parent); int p2,q2,g2; g_career.tierOfTeam(i,&p2,&q2,&g2); bool term2=a>=0&&g_career.pyramids[p2].pools[q2].terminal; if(a>=0&&b>=0&&(a<b||(a==b&&!term2))){ cat[fmt("res %d parent %d",a,b)]++; if(shown++<5) printf("  %s (%s) parent %s (%s)\n", t.name.c_str(), g_career.teamLevelName(i).c_str(), g_world.teams[t.parent].name.c_str(), g_career.teamLevelName(t.parent).c_str()); } if(a>=0&&a<=2) cat["pro"]++; } for(auto&kv:cat) printf("  %s: %d\n", kv.first.c_str(), kv.second); }
    for (int it=0; it<NUM_INTL; it++) {
        int fra = g_world.nationIndex(it==IT_CAN?"SEN": it==IT_ASIA?"JPN": it==IT_GOLD?"MEX": it==IT_COPA?"ARG": it==IT_OFC?"NZL":"FRA");
        g_career.newInternational(it, true, {fra}, 0, {});
        int um=0; playSeason(um);
        auto& S=g_career.season;
        int fc = g_career.finalComp;
        int nt=0; if(fc>=0) for(auto&g:S.comps[fc].stages[0].groups) nt+=g.size();
        printf("%s: comps=%zu userMatches=%d final teams=%d winner=%s\n", INTL_NAMES[it], S.comps.size(), um, nt, fc>=0 && S.comps[fc].winner>=0? g_world.teams[S.comps[fc].winner].name.c_str():"NONE");
    }
    g_world.ensureSquad(g_world.nationIndex("FRA")); for (int i=0;i<5;i++) printf("%s ", g_world.teams[g_world.nationIndex("FRA")].squad[i].name.c_str()); printf("\n");
    g_world.ensureSquad(user); for (int i=0;i<5;i++) printf("%s(%d) ", g_world.teams[user].squad[i].name.c_str(), g_world.teams[user].squad[i].num); printf("\n");
}
