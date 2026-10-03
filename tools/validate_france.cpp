#include "../src/game.h"
int main() {
    g_world.build();
    for(const auto& p:g_basePyramids) if(p.country=="FRA"&&p.dom<0) {
        auto errors=validateFrancePyramid(p,true);
        for(const auto& e:errors) std::printf("ERROR %s\n",e.c_str());
        std::printf("%s: %d imported senior teams\n",errors.empty()?"PASS":"FAIL",NUM_FR_OFFICIAL_2627);
        return errors.empty()?0:1;
    }
    return 2;
}
