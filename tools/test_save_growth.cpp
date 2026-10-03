// Taille des sauvegardes saison après saison (profil par bloc), relecture et budget de taille
#include "../src/game.h"
#include <chrono>
#include <cstdio>
#include <algorithm>
struct SaveProf { const char* name; long bytes; long count; };
const std::vector<SaveProf>& saveProfile();
int main(int argc, char** argv) {
    int seasons = argc > 1 ? atoi(argv[1]) : 3;
    double budgetMB = argc > 2 ? atof(argv[2]) : 0;      // échec si une sauvegarde dépasse ce budget (Mo sur disque)
    g_world.build();
    int user = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "US Saint-Malo") user = i;
    g_career.newClubCareer(user, 2026);
    int fails = 0;
    for (int y = 0; y <= seasons; y++) {
        if (y > 0) {
            for (int k = 0; k < 20000; k++) { auto pm = g_career.season.advance(false); if (pm.comp < 0) break; auto& C = g_career.season.comps[pm.comp]; simulateMatch(C.matches[pm.match], &C); g_career.season.recordResult(pm.comp, pm.match); g_career.season.checkRound(pm.comp); }
            g_career.endSeason();
        }
        auto t0 = std::chrono::steady_clock::now();
        bool ok = g_career.save("build/growth.sav");
        auto t1 = std::chrono::steady_clock::now();
        FILE* f = fopen("build/growth.sav", "rb"); long sz = 0; if (f) { fseek(f, 0, SEEK_END); sz = ftell(f); fclose(f); }
        long raw = 0; for (auto& s : saveProfile()) raw += s.bytes;
        std::vector<SaveProf> v = saveProfile(); std::sort(v.begin(), v.end(), [](const SaveProf& a, const SaveProf& b) { return a.bytes > b.bytes; });
        printf("season %d (%d) : disk %.1f MB raw %.1f MB save %.2f s  top:", y, g_career.year, sz / 1048576.0, raw / 1048576.0, std::chrono::duration<double>(t1 - t0).count());
        for (int k = 0; k < 5 && k < (int)v.size(); k++) printf("  %s %.1f", v[k].name, v[k].bytes / 1048576.0);
        printf("\n"); fflush(stdout);
        if (!ok) { printf("FAIL save\n"); fails++; }
        if (budgetMB > 0 && sz / 1048576.0 > budgetMB) { printf("FAIL budget %.1f MB > %.1f MB\n", sz / 1048576.0, budgetMB); fails++; }
        if (y == seasons) {
            auto t2 = std::chrono::steady_clock::now();
            bool lok = g_career.load("build/growth.sav");
            auto t3 = std::chrono::steady_clock::now();
            printf("load %d in %.2f s\n", (int)lok, std::chrono::duration<double>(t3 - t2).count());
            if (!lok) fails++;
        }
    }
    printf(fails ? "FAIL save growth\n" : "PASS save growth\n");
    return fails ? 1 : 0;
}
