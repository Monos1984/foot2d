#include "../src/game.h"
#include <cstdio>
int main(){ g_world.build(); int u=-1; for (int i=g_world.firstClub;i<(int)g_world.teams.size();i++) if (g_world.teams[i].name=="Stade Brestois") u=i;
 g_career.newClubCareer(u,2026); g_career.mgr.managerMode=true; g_world.teams[u].sponsor="Pixel Cola"; g_career.mgr.sponsorIncome=500; g_career.mgr.sponsorYears=1;
 std::vector<std::string> msgs; g_career.mgrEndSeason(msgs); for (auto&m:msgs) if (m.find("ponsor")!=std::string::npos) printf("%s\n", m.c_str());
 printf("sponsor now '%s' income %lld years %d\n", g_world.teams[u].sponsor.c_str(), (long long)g_career.mgr.sponsorIncome, g_career.mgr.sponsorYears); }
