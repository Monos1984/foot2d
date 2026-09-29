// test : pyramide U19, Coupe Gambardella, passage des joueurs en seniors
#include "../src/game.h"
#include <cstdio>
int main() {
    g_world.build();
    int user = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Stade Brestois") user = i;
    g_career.newClubCareer(user, 2026);
    int yp = g_career.u19Pyramid();
    const Pyramid& Y = g_career.pyramids[yp];
    int cnt[6] = {0,0,0,0,0,0}; for (auto& pl : Y.pools) cnt[pl.tier] += (int)pl.clubs.size();
    printf("U19 : National %d, R1 %d, R2 %d, R3 %d, D1 %d, D2 %d\n", cnt[0], cnt[1], cnt[2], cnt[3], cnt[4], cnt[5]);
    int u19 = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].parent == user && g_world.teams[i].youth) u19 = i;
    printf("U19 du club : %s (%s)\n", u19 >= 0 ? g_world.teams[u19].name.c_str() : "-", u19 >= 0 ? g_career.teamLevelName(u19).c_str() : "");
    for (int season = 0; season < 2; season++) {
        Season& S = g_career.season;
        while (true) { auto pm = S.advance(true); if (pm.comp < 0) break; }
        auto name = [&](int c) { return c >= 0 && S.comps[c].winner >= 0 ? g_world.teams[S.comps[c].winner].name : std::string("?"); };
        printf("Saison %d : champion U19 %s | Gambardella %s (regionaux %d)\n", g_career.year, name(g_career.u19Final).c_str(), name(g_career.gambNational).c_str(), (int)g_career.gambRegional.size());
        if (g_career.gambNational >= 0) { auto& C = S.comps[g_career.gambNational]; for (auto& st : C.stages) printf("  [%s %d]", st.name.c_str(), (int)st.ties.size()); printf("\n"); }
        if (u19 >= 0) { g_world.ensureSquad(u19); int mx = 0, mn = 99; for (auto& p : g_world.teams[u19].squad) { mx = std::max(mx, (int)p.age); mn = std::min(mn, (int)p.age); } printf("  âges U19 : %d-%d, squad %d\n", mn, mx, (int)g_world.teams[u19].squad.size()); }
        int created = -1;
        if (season == 0) { std::string err; int ama = -1; for (auto& pl : g_career.pyramids[0].pools) if (pl.tier == 10) { for (int t : pl.clubs) { bool has = false; for (auto& T : g_world.teams) if (T.parent == t && T.youth) has = true; if (!has && g_world.teams[t].parent < 0) { ama = t; break; } } if (ama >= 0) break; }
            created = g_career.createU19(ama, err); printf("  création U19 : %s %s\n", created >= 0 ? g_world.teams[created].name.c_str() : err.c_str(), ""); }
        g_career.endSeason();
        if (created >= 0) printf("  -> placée en %s\n", g_career.teamLevelName(created).c_str());
    }
    for (auto& pl : g_career.pyramids[yp].pools) cnt[pl.tier] = 0;
    for (auto& pl : g_career.pyramids[yp].pools) cnt[pl.tier] += (int)pl.clubs.size();
    printf("Après 2 saisons : National %d, R1 %d, R2 %d, R3 %d, D1 %d, D2 %d\n", cnt[0], cnt[1], cnt[2], cnt[3], cnt[4], cnt[5]);
    return 0;
}
