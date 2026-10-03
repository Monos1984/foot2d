#include "game.h"
#include <cstring>
#include <set>

struct PoleDef{const char* town;const char* dept;};
static const PoleDef POLES[]={
 {"INF Clairefontaine","78"},{"Aix-en-Provence","13"},{"Ajaccio","2A"},{"Castelmaurou","31"},
 {"Châteauroux","36"},{"Dijon","21"},{"Liévin","62"},{"Lisieux","14"},
 {"Lyon","69"},{"Nancy","54"},{"Ploufragan","22"},{"Reims","51"},
 {"Saint-Sébastien-sur-Loire","44"},{"Talence","33"},{"Guadeloupe","971"},{"Océan Indien","974"}};
bool Career::polesEnabled() const{
 if(kind!=CK_CLUB || opts.lite || euroOnly || userTeam<0 || userTeam>=(int)g_world.teams.size())return false;
 return g_world.teams[userTeam].nation==g_world.nationIndex("FRA") && !isWomenTeam(userTeam);
}
static void fillPole(int team,bool reset){
 auto& t=g_world.teams[team];if(reset)t.squad.clear();
 while(t.squad.size()<22){int ix=(int)t.squad.size();Player p=g_world.makeYouth(team,ix<2?POS_GK:ix<9?POS_DF:ix<16?POS_MF:POS_FW,t.rating);
  p.age=reset?(ix%2?14:13):13;p.pot=(uint8_t)g_rng.range(65,94);p.num=(uint8_t)(ix+1);p.gender=0;p.nation=(int16_t)t.nation;t.squad.push_back(p);}
 t.squadGen=true;t.xi.clear();
}
void Career::startPoleCup(){
 poleCup=-1;if(!polesEnabled())return;
 bool fresh=poleTeams.empty();
 if(fresh)for(int i=0;i<16;i++){
  std::string name="Pôle Espoirs - "+sanitize(POLES[i].town);int id=-1;
  for(int t=0;t<(int)g_world.teams.size();t++)if(g_world.teams[t].youth==8&&g_world.teams[t].name==name){id=t;break;}
  if(id<0){Team t;t.name=name;t.shortName=fmt("Pôle %d",i+1);t.stadium="Site du Pôle Espoirs - "+sanitize(POLES[i].town);t.town=sanitize(POLES[i].town);
   t.nation=g_world.nationIndex("FRA");t.kind=TK_CLUB;t.youth=8;t.parent=-1;t.dept=-1;for(int d=0;d<NUM_DEPTS;d++)if(std::string(DEPTS[d].code)==POLES[i].dept)t.dept=d;t.region=t.dept>=0?DEPTS[t.dept].region:-1;
   t.district=t.dept>=0?districtFor(t.dept,t.town,t.name):-1;t.rating=35+i%5;t.seed=hashStr(name);t.culture=CU_FR;t.home.shirt=0x164B91;t.home.shirt2=0xFFFFFF;t.home.shorts=0xFFFFFF;t.away.shirt=0xFFFFFF;t.away.shorts=0x164B91;
   g_world.teams.push_back(t);id=(int)g_world.teams.size()-1;}
  poleTeams.push_back(id);fillPole(id,true);
 }
 if(poleTeams.size()!=16)return;
 for(int t:poleTeams){if(t<0||t>=(int)g_world.teams.size()||g_world.teams[t].youth!=8)return;fillPole(t,false);}
 Competition c;c.name=fmt("Coupe des Pôles Espoirs %d",year+1);c.shortName="Pôles Espoirs";c.kind=150;c.format=FMT_TOURNAMENT;c.legs=1;c.groupsAdvance=2;c.neutralFinal=true;c.season=year;
 c.koTimes={42.0+3.0/7,42.0+4.0/7,42.0+5.0/7};
 std::vector<int> order=poleTeams;g_rng.shuffle(order);std::vector<std::vector<int>> groups(4);
 for(int i=0;i<16;i++)groups[i/4].push_back(order[i]);
 c.addGroupStage(groups,1,{42,42.0+1.0/7,42.0+2.0/7},"Phase de groupes");
 for(auto& m:c.matches){m.neutral=true;m.noET=true;}
 poleCup=(int)season.comps.size();season.comps.push_back(c);cupVenue[poleCup]=g_world.teams[poleTeams[((year-2026)%16+16)%16]].stadium;
 season.news.push_back("Coupe des Pôles Espoirs : 4 groupes, quarts, demi-finales et finale sur "+cupVenue[poleCup]+". Matchs de 2 x 20 minutes ; TAB directs en phase finale.");
}
std::vector<BallonNominee> Career::poleRanking() const{
 std::vector<BallonNominee> out;if(poleCup<0||poleCup>=(int)season.comps.size())return out;const auto& c=season.comps[poleCup];
 std::map<int,int> goals,assists,games;
 for(const auto& m:c.matches)if(m.played){games[m.home]++;games[m.away]++;}
 for(const auto& e:c.events)if(e.type==0){goals[e.pid]++;if(e.aid>0)assists[e.aid]++;}
 for(int t:poleTeams)if(t>=0&&t<(int)g_world.teams.size())for(const auto& p:g_world.teams[t].squad){BallonNominee n;n.pid=p.id;n.team=t;n.goals=goals[p.id];n.assists=assists[p.id];n.apps=std::min<int>(p.apps,games[t]);n.rating=p.overall();n.score=n.goals*8+n.assists*5+n.rating+n.apps;
  std::snprintf(n.name,sizeof n.name,"%s",p.name.c_str());out.push_back(n);}
 std::sort(out.begin(),out.end(),[](const BallonNominee&a,const BallonNominee&b){return a.score!=b.score?a.score>b.score:a.pid<b.pid;});return out;
}
bool Career::recruitPole(int pid,int dest,std::string& err){
 if(!polesEnabled()||poleCup<0||poleCup>=(int)season.comps.size()||!season.comps[poleCup].done){err="Le recrutement ouvre après la finale des Pôles Espoirs.";return false;}
 int ix=-1,t=g_world.findPlayer(pid,&ix);if(t<0||g_world.teams[t].youth!=8||ix<0){err="Ce joueur n'est plus au Pôle Espoirs.";return false;}
 if(dest<0||dest>=(int)g_world.teams.size()||g_world.teams[dest].youth!=3){err="Choisissez une équipe U15.";return false;}
 int root=g_world.teams[userTeam].parent>=0?g_world.teams[userTeam].parent:userTeam;
 if(g_world.teams[dest].parent!=root){err="Choisissez une équipe U15 de votre club.";return false;}
 for(const auto& r:poleRecruits)if(r.pid==pid){err="Ce joueur est déjà réservé.";return false;}
 g_world.ensureSquad(dest);int reserved=0;for(auto& r:poleRecruits)reserved+=r.dest==dest;
 if(g_world.teams[dest].squad.size()+reserved>=30){err="Effectif U15 complet (30 joueurs maximum).";return false;}
 poleRecruits.push_back({pid,dest,year+1});season.news.push_back(g_world.teams[t].squad[ix].name+" rejoindra "+g_world.teams[dest].name+" la saison prochaine.");return true;
}
void Career::finishPoleSeason(std::vector<std::string>& news){
 if(poleTeams.empty())return;
 std::vector<int> destinations;
 for(const auto& p:pyramids)if(p.country=="U15")for(const auto& q:p.pools)for(int t:q.clubs)destinations.push_back(t);
 if(destinations.empty())return;
 std::map<int,int> slots;for(const auto& r:poleRecruits)slots[r.dest]++;
 for(int t:poleTeams){auto& source=g_world.teams[t];
  for(int i=(int)source.squad.size()-1;i>=0;i--){auto p=source.squad[i];int dest=-1;bool reserved=false;
   for(const auto& r:poleRecruits)if(r.pid==p.id&&r.year<=year+1){dest=r.dest;reserved=true;}
   if(dest<0&&p.age>=15){std::vector<int> local;for(int d:destinations)if(g_world.teams[d].region==source.region)local.push_back(d);auto& v=local.empty()?destinations:local;dest=v[g_rng.range(0,(int)v.size()-1)];}
   if(dest<0)continue;g_world.ensureSquad(dest);auto& target=g_world.teams[dest];
   if(target.squad.size()+(reserved?0:slots[dest])>=30){if(reserved){news.push_back(p.name+" : arrivée U15 reportée, effectif complet.");continue;}dest=-1;for(int d:destinations){g_world.ensureSquad(d);if(g_world.teams[d].squad.size()+slots[d]<30){dest=d;break;}}if(dest<0)continue;}
   auto& d=g_world.teams[dest];p.contract=2;p.wageK=0;p.years=0;p.goals=p.assists=p.apps=0;
   std::set<int> nums;for(auto& q:d.squad)nums.insert(q.num);p.num=1;while(nums.count(p.num)&&p.num<99)p.num++;
   d.squad.push_back(p);if(reserved)slots[dest]--;d.xi.clear();source.squad.erase(source.squad.begin()+i);source.xi.clear();
   news.push_back(p.name+fmt(" (%d ans) quitte ",p.age)+source.name+" et rejoint "+d.name+".");
  }
  // A new 13-year-old intake replaces those who have moved to club U15 teams.
  fillPole(t,false);
 }
 poleRecruits.erase(std::remove_if(poleRecruits.begin(),poleRecruits.end(),[](const PoleRecruit& r){int t=g_world.findPlayer(r.pid);return t<0 || g_world.teams[t].youth!=8;}),poleRecruits.end());
}
