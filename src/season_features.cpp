#include "game.h"
#include <cstring>
#include <set>

int lowestLocalTier(const Pyramid& p,int team) {
 int best=-1;
 for(const auto& pool:p.pools)if(pool.tier>=0 && pool.tier<(int)p.tiers.size() && pool.key==p.keyFor(pool.tier,team))best=std::max(best,pool.tier);
 return best;
}
void Career::markClubDebut(int t,int y){for(auto& d:clubDebuts)if(d.team==t)return;clubDebuts.push_back({t,y});}
bool clubFirstSeason(int team){
 if(team<0 || team>=(int)g_world.teams.size())return false;
 for(const auto& d:g_career.clubDebuts)if(d.year==g_career.season.year && (d.team==team || d.team==g_world.teams[team].parent))return true;
 return false;
}
bool Career::awardsEnabled() const{return kind==CK_CLUB && !opts.lite && !euroOnly;}
int Career::prepareSeasonAwards(){
 if(!awardsEnabled() || !season.finished)return -1;
 for(int i=0;i<(int)seasonAwards.size();i++)if(seasonAwards[i].year==year)return i;
 int pi=-1,qi=-1,gi=-1;if(tierOfTeam(userTeam,&pi,&qi,&gi)<0 || gi<0)return -1;
 const auto& pool=pyramids[pi].pools[qi];if(gi>=(int)pool.comps.size())return -1;
 int ci=pool.comps[gi];if(ci<0 || ci>=(int)season.comps.size())return -1;
 const auto& c=season.comps[ci];if(c.stages.empty())return -1;
 SeasonAwards a;a.year=year;std::snprintf(a.competition,sizeof a.competition,"%s",c.name.c_str());
 std::map<int,int> goals,assists,games,conceded,points;
 std::set<int> teams;
 for(const auto& st:c.stages)for(const auto& g:st.groups)for(int t:g)teams.insert(t);
 for(const auto& m:c.matches)if(m.played){games[m.home]++;games[m.away]++;conceded[m.home]+=m.ag;conceded[m.away]+=m.hg;}
 for(const auto& s:c.table(0,0))points[s.team]=s.pts;
 for(const auto& e:c.events)if(e.type==0){goals[e.pid]++;if(e.aid>0)assists[e.aid]++;}
 int best[5]={-999999,-999999,-999999,-999999,-999999};
 for(int t:teams){g_world.ensureSquad(t);for(const auto& p:g_world.teams[t].squad){
  if(p.id<=0 || p.name.empty())continue;
  BallonNominee n;n.pid=p.id;n.team=t;n.rating=p.overall();n.goals=goals[p.id];n.assists=assists[p.id];n.apps=std::min<int>(p.apps,games[t]);
  if(n.apps==0 && n.goals==0 && n.assists==0)continue;
  std::snprintf(n.name,sizeof n.name,"%s",p.name.c_str());
  int score[5]={n.goals*8+n.assists*5+n.rating+n.apps+points[t],n.goals*100+n.assists,n.assists*100+n.goals,
    p.pos==POS_GK?10000-conceded[t]*100/std::max(1,games[t])+n.rating:-999999,
    p.age<=21?n.goals*8+n.assists*5+n.rating+n.apps:-999999};
  for(int k=0;k<5;k++)if(score[k]>best[k] || (score[k]==best[k] && a.valid[k] && n.pid<a.winners[k].pid)){
   best[k]=score[k];a.winners[k]=n;a.winners[k].score=score[k];a.valid[k]=1;
  }
 }}
 int winner=c.winner;if(winner<0&&!c.result.empty())winner=c.result[0];
 if(winner>=0){auto& n=a.winners[5];n.team=winner;n.pid=-1;n.score=points[winner];
  std::string name=winner==userTeam&&!managerName.empty()?managerName:"Entraîneur de "+g_world.teams[winner].name;
  std::snprintf(n.name,sizeof n.name,"%s",name.c_str());a.valid[5]=1;}
 seasonAwards.push_back(a);return (int)seasonAwards.size()-1;
}
