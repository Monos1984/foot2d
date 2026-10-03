// Sauvegardes : aller-retour déterministe, contrôle d'intégrité, corruption (en-tête, données, troncature), copie de secours
#include "../src/game.h"
#include <cstdio>
#include <vector>
static int checks = 0;
static void check(bool v, const char* t) { checks++; if (!v) { printf("FAIL save integrity: %s\n", t); exit(1); } }
static std::vector<unsigned char> readAll(const char* p) { std::vector<unsigned char> v; FILE* f = fopen(p, "rb"); if (!f) return v; fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET); v.resize(n); if (n) fread(v.data(), 1, n, f); fclose(f); return v; }
static void writeAll(const char* p, const std::vector<unsigned char>& v) { FILE* f = fopen(p, "wb"); fwrite(v.data(), 1, v.size(), f); fclose(f); }
int main() {
    g_world.build();
    int user = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Stade Brestois") user = i;
    g_career.newClubCareer(user, 2026);
    for (int k = 0; k < 40; k++) { auto pm = g_career.season.advance(false); if (pm.comp < 0) break; auto& C = g_career.season.comps[pm.comp]; simulateMatch(C.matches[pm.match], &C); g_career.season.recordResult(pm.comp, pm.match); g_career.season.checkRound(pm.comp); }
    remove("build/integ.sav"); remove("build/integ.sav.bak");
    // état logique de référence
    auto& U = g_world.teams[user];
    size_t teams = g_world.teams.size(), comps = g_career.season.comps.size(), squad = U.squad.size(), museum = g_career.clubHistories.size(), sup = g_career.supporters.profiles.size(), pers = g_career.personalities.players.size();
    double now = g_career.season.now; int64_t budget = g_career.mgr.budget; int goals0 = U.squad.empty() ? 0 : U.squad[0].goals;
    check(g_career.save("build/integ.sav"), "first save");
    auto A = readAll("build/integ.sav");
    check(A.size() > 16 && A[0] == 'S' && A[1] == 'S' && A[2] == 'W' && A[3] == 'Z', "compressed container header");
    check(g_career.load("build/integ.sav"), "load");
    check(g_world.teams.size() == teams && g_career.season.comps.size() == comps && g_world.teams[user].squad.size() == squad, "teams, competitions, squad restored");
    check(g_career.clubHistories.size() == museum && g_career.supporters.profiles.size() == sup && g_career.personalities.players.size() == pers, "museum, supporters, personalities restored");
    check(g_career.season.now == now && g_career.mgr.budget == budget && (g_world.teams[user].squad.empty() || g_world.teams[user].squad[0].goals == goals0), "calendar, finances, stats restored");
    check(g_career.save("build/integ.sav"), "second save");
    auto B = readAll("build/integ.sav");
    check(A == B, "save -> load -> save gives identical bytes");
    check(!readAll("build/integ.sav.bak").empty(), "previous save kept as .bak");
    // corruption : en-tête
    auto C = B; C[1] ^= 0x55; writeAll("build/corrupt.sav", C); remove("build/corrupt.sav.bak");
    check(!g_career.load("build/corrupt.sav"), "corrupted header refused (no backup)");
    // corruption au milieu des données compressées : CRC
    C = B; C[C.size() / 2] ^= 0xFF; writeAll("build/corrupt.sav", C);
    check(!g_career.load("build/corrupt.sav"), "corrupted payload refused by checksum");
    // troncature
    C = B; C.resize(C.size() / 3); writeAll("build/corrupt.sav", C);
    check(!g_career.load("build/corrupt.sav"), "truncated save refused");
    // nombre d'éléments absurde dans l'en-tête des blocs
    C = B; C[24] = 0xFF; C[25] = 0xFF; C[26] = 0xFF; C[27] = 0x7F; writeAll("build/corrupt.sav", C);
    check(!g_career.load("build/corrupt.sav"), "absurd block count refused");
    // repli automatique sur la copie de secours
    writeAll("build/corrupt.sav.bak", B);
    C = B; C[C.size() / 2] ^= 0xFF; writeAll("build/corrupt.sav", C);
    check(g_career.load("build/corrupt.sav"), "corrupted save falls back to .bak");
    check(g_world.teams[user].squad.size() == squad, "backup content valid");
    printf("PASS save integrity: %d checks (save %.1f MB)\n", checks, B.size() / 1048576.0);
}
