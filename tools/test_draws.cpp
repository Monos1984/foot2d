#include "../src/game.h"
#include <cstdio>
#include <map>
int main() {
    g_world.build();
    int fra = g_world.nationIndex("FRA"), usa = g_world.nationIndex("USA");
    int bad = 0, total = 0;
    for (int rep = 0; rep < 3; rep++) {
        g_career.newInternational(IT_WORLDCUP, false, { fra }, 2026, { usa }, 0);
        auto& C = g_career.season.comps[g_career.finalComp];
        for (auto& g : C.stages[0].groups) {
            std::map<int,int> cnt; for (int t : g) cnt[NATIONS[t].conf]++;
            total++;
            for (auto& kv : cnt) if ((kv.first == UEFA && kv.second > 2) || (kv.first != UEFA && kv.second > 1)) { bad++; break; }
            if (rep == 0) { for (int t : g) printf("%s ", NATIONS[t].code); printf("\n"); }
        }
    }
    printf("groupes non conformes : %d / %d\n", bad, total);
    // C1 : répartition des clubs d'un même pays
    int user=-1; for (int i=0;i<(int)g_world.teams.size();i++) if (g_world.teams[i].name=="Stade Brestois") user=i;
    g_career.newClubCareer(user, 2026);
    auto& S = g_career.season;
    while (S.comps[g_career.ucl].stages.size() < 4) { auto pm = S.advance(true); if (pm.comp < 0) break; }
    auto& U = S.comps[g_career.ucl];
    for (auto& st : U.stages) if (st.type == ST_LEAGUE) {
        std::map<int, std::vector<int>> gh;
        for (int g = 0; g < (int)st.groups.size(); g++) for (int t : st.groups[g]) gh[g_world.teams[t].nation].push_back(g);
        for (auto& kv : gh) if (kv.second.size() >= 2) { printf("%s :", NATIONS[kv.first].code); for (int g : kv.second) printf(" %c", 'A' + g); printf("\n"); }
        break;
    }
}
