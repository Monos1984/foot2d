#include "../src/game.h"
int main(int argc,char** argv) {
    g_world.build();
    g_career.newClubCareer(g_world.firstClub,2026);
    return g_career.save(argc>1?argv[1]:"build/test-legacy-v23.sav") ? 0 : 1;
}
