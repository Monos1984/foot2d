// Chargement d'une sauvegarde de l'ancien format (version 34, non compressée) puis conversion au nouveau format
#include "../src/game.h"
#include <cstdio>
int main(int argc, char** argv) {
    const char* path = argc > 1 ? argv[1] : "build/test-build18.sav";
    g_world.build();
    bool ok = g_career.load(path);
    printf("legacy load %d (teams %zu, comps %zu, museum %zu, supporters %zu, personalities %zu)\n", (int)ok, g_world.teams.size(), g_career.season.comps.size(), g_career.clubHistories.size(), g_career.supporters.profiles.size(), g_career.personalities.players.size());
    if (!ok) { printf("FAIL legacy load\n"); return 1; }
    bool s = g_career.save("build/legacy-converted.sav");
    bool l = s && g_career.load("build/legacy-converted.sav");
    FILE* f = fopen("build/legacy-converted.sav", "rb"); long n = 0; if (f) { fseek(f, 0, SEEK_END); n = ftell(f); fclose(f); }
    printf("converted save %d reload %d size %.1f MB\n", (int)s, (int)l, n / 1048576.0);
    printf(s && l ? "PASS legacy save\n" : "FAIL legacy save\n");
    return s && l ? 0 : 1;
}
