#include <set>
#include "../src/game.h"
// équilibre financier sur une saison pour plusieurs niveaux (primes incluses)
int main(int argc,char**argv){
    const char* names[]={"Paris Saint-Germain","Olympique Lyonnais","Stade Lavallois","FC Rouen 1899","Stade Briochin"};
    for (const char* nm: names){
        g_world=World(); g_world.build();
        int user=-1; for (int i=0;i<(int)g_world.teams.size();i++) if (g_world.teams[i].name==nm && g_world.teams[i].parent<0) user=i;
        if(user<0){printf("? %s\n",nm);continue;}
        g_career=Career(); g_career.newClubCareer(user, 2026);
        auto& K=g_career; auto& S=K.season;
        int64_t b0=K.mgr.budget;
        printf("== %s tier %d income %s budget %s obj %s\n", nm, K.tierOfTeam(user), money(K.mgr.incomeBase).c_str(), money(b0).c_str(), K.objectiveText().c_str());
        std::set<long long> done;
        while(true){ auto pm=S.advance(false); if(pm.comp<0)break;
            auto& C=S.comps[pm.comp]; auto& r=C.matches[pm.match];
            simulateMatch(r,&C); S.recordResult(pm.comp,pm.match); genMatchEvents(C,pm.match);
            K.mgrAfterMatch(pm.comp,pm.match); S.checkRound(pm.comp); }
        K.mgrTick();
        for (size_t i=0;i<S.news.size();i++) if (S.news[i].find("Prime")==0) printf("   %s\n", S.news[i].c_str());
        int64_t inc=K.mgr.seasonIncome, wg=K.mgr.seasonWages;
        std::vector<std::string> msgs; K.mgrEndSeason(msgs);
        for(auto&m:msgs) printf("   > %s\n", m.c_str());
        printf("   income %s wages %s budget %s -> %s\n", money(inc).c_str(), money(wg).c_str(), money(b0).c_str(), money(K.mgr.budget).c_str());
    }
}
