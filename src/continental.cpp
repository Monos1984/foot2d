// Coupes continentales des clubs hors Europe (build 6) : Copa Libertadores (CONMEBOL), Ligue des champions de la CAF,
// Ligue des champions de l'AFC, Coupe des champions de la CONCACAF, Ligue des champions de l'OFC.
// Phase de groupes (aller-retour), puis phase finale aller-retour et finale sur match unique (terrain neutre).
#include "game.h"
#include <algorithm>
#include <map>

int addCompPublic(Season& S, Competition c);

struct ContDef { int conf; int kind; const char* name; const char* sh; int teams; };
static const ContDef CONT[] = {
    { CONMEBOL, 45, "Copa Libertadores", "Libertadores", 32 },
    { CAF, 46, "Ligue des champions de la CAF", "C1 CAF", 16 },
    { AFC, 47, "Ligue des champions de l'AFC", "C1 AFC", 16 },
    { CONCACAF, 48, "Coupe des champions de la CONCACAF", "C1 CONCACAF", 16 },
    { OFC, 49, "Ligue des champions de l'OFC", "C1 OFC", 8 },
};
bool isContinentalKind(int k) { return k >= 45 && k <= 49; }

void continentalStartSeason(Career& K) {
    if (K.euroOnly || K.kind != CK_CLUB) return;
    Season& S = K.season;
    for (auto& D : CONT) {
        // pays de la confédération dotés d'un championnat, classés par niveau de la sélection
        struct Ct { int nat; std::vector<int> clubs; };
        std::vector<Ct> cs;
        for (auto& P : K.pyramids) {
            if (P.dom >= 0 || isWomenPyramid(P)) continue;
            int nat = g_world.nationIndex(P.country.c_str());
            if (nat < 0 || NATIONS[nat].conf != D.conf) continue;
            int q = P.poolIndex(0, 0);
            if (q < 0 || P.pools[q].clubs.size() < 4) continue;
            Ct c; c.nat = nat;
            auto it = K.prevChampion.find(P.country);
            std::vector<int> cl = P.pools[q].clubs;
            std::stable_sort(cl.begin(), cl.end(), [](int a, int b) { return g_world.teams[a].rating > g_world.teams[b].rating; });
            if (it != K.prevChampion.end() && std::find(cl.begin(), cl.end(), it->second) != cl.end()) c.clubs.push_back(it->second);
            auto it2 = K.prevRunnerUp.find(P.country);
            if (it2 != K.prevRunnerUp.end() && std::find(cl.begin(), cl.end(), it2->second) != cl.end() && std::find(c.clubs.begin(), c.clubs.end(), it2->second) == c.clubs.end()) c.clubs.push_back(it2->second);
            for (int t : cl) if (std::find(c.clubs.begin(), c.clubs.end(), t) == c.clubs.end() && g_world.teams[t].parent < 0) c.clubs.push_back(t);
            cs.push_back(c);
        }
        std::stable_sort(cs.begin(), cs.end(), [](const Ct& a, const Ct& b) { return g_world.teams[a.nat].rating > g_world.teams[b.nat].rating; });
        std::vector<int> pool;
        for (int r = 0; r < 12 && (int)pool.size() < D.teams; r++)
            for (auto& c : cs) if (r < (int)c.clubs.size() && (int)pool.size() < D.teams && (r < 4 || cs.size() < 4)) pool.push_back(c.clubs[r]);
        int N = D.teams;
        while (N > 4 && (int)pool.size() < N) N /= 2;
        if ((int)pool.size() < N || N < 8) continue;
        pool.resize(N);
        // tirage des groupes : chapeaux par niveau, pas deux clubs d'un même pays dans un groupe si possible
        int G = N / 4;
        std::stable_sort(pool.begin(), pool.end(), [](int a, int b) { return g_world.teams[a].rating > g_world.teams[b].rating; });
        std::vector<std::vector<int>> groups(G);
        for (int pot = 0; pot < 4; pot++) {
            std::vector<int> p(pool.begin() + pot * G, pool.begin() + (pot + 1) * G);
            for (int attempt = 0; attempt < 80; attempt++) {
                g_rng.shuffle(p);
                bool ok = true;
                for (int g = 0; g < G && ok; g++) for (int t : groups[g]) if (g_world.teams[t].nation == g_world.teams[p[g]].nation) ok = false;
                if (ok) break;
            }
            for (int g = 0; g < G; g++) groups[g].push_back(p[g]);
        }
        Competition c;
        c.format = FMT_TOURNAMENT; c.kind = D.kind; c.tag = D.conf; c.legs = 2; c.tb = TB_H2H;
        c.name = fmt("%s %d", D.name, K.year + 1); c.shortName = D.sh;
        c.groupsAdvance = 2; c.bestThirds = 0; c.thirdPlace = false; c.neutralFinal = true;
        std::vector<double> times = { 8, 10, 12, 15, 17, 19 };
        c.addGroupStage(groups, 2, times, "Phase de groupes");
        int ko = G * 2;
        if (ko >= 16) c.koTimes = { 24, 29, 34, 42.6 };
        else if (ko >= 8) c.koTimes = { 27, 33, 42.6 };
        else c.koTimes = { 33, 42.6 };
        addCompPublic(S, std::move(c));
    }
}

// vainqueur continental de la saison (Coupe du monde des clubs), -1 si inconnu
int continentalWinner(const Career& K, int conf) {
    for (auto& C : K.season.comps) if (isContinentalKind(C.kind) && C.tag == conf && C.done && C.winner >= 0) return C.winner;
    return -1;
}
