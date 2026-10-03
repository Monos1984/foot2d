#include "../src/game.h"
#include "../src/match.h"
#include <set>
#include <cstring>
#include <cstdlib>
#include <cmath>
static int checks=0;
static void check(bool x,const char* message){checks++;if(!x){std::printf("FAIL %s\n",message);std::exit(1);}}
static int club(const char* code){for(auto& p:g_basePyramids)if(p.country==code)for(auto& q:p.pools)if(q.tier==0&&!q.clubs.empty())return q.clubs[0];return -1;}
static void finishSubset(const std::set<int>& keep){for(int i=0;i<(int)g_career.season.comps.size();i++)if(!keep.count(i)){auto& c=g_career.season.comps[i];c.done=true;for(auto& s:c.stages)s.finished=true;}g_career.season.advance(true);check(g_career.season.finished,"subset season completes");}
int main(){
 g_world.build();check(g_career.load("build/test-legacy-v26.sav"),"load actual build2 v26 save");
 check(g_career.seasonAwards.empty()&&g_career.poleTeams.empty()&&g_career.poleCup==-1,"new fields default for old save");
 // A deliberately high division in a custom club must never bypass its district.
 int root=club("FRA");for(auto& p:g_basePyramids)if(p.country=="FRA"&&p.dom<0){int q=p.poolIndex(8,districtIndex("Marne"));check(q>=0,"Marne exists");root=p.pools[q].clubs[0];}
 Team custom=g_world.teams[root];custom.name="Test nouveau club build 3";custom.shortName="Test B3";custom.parent=-1;custom.resLevel=0;custom.youth=0;custom.squad.clear();custom.squadGen=false;custom.xi.clear();custom.honours.clear();custom.lastTier=0;custom.dbClub=-1;custom.rating=99;custom.founded=2026;custom.status=CS_AMATEUR;
 root=g_world.addCustomClub(custom);g_career.opts=Career::Opts();g_career.opts.disableManagerLife=1;g_career.opts.disableBribes=1;g_career.newClubCareer(root,2026);
 int pi,qi,gi;check(g_career.tierOfTeam(root,&pi,&qi,&gi)==11,"new club forced to Marne D4 despite editor tier 0");
 check(g_career.pyramids[pi].pools[qi].key==districtIndex("Marne"),"correct local district");check(clubFirstSeason(root),"first season tracked");
 for(auto& c:g_career.season.comps)for(auto& st:c.stages)for(auto& t:st.ties)if(t.a==root||t.b==root)check(t.b>=0,"new club has a played first-round tie");
 // Stress the former strongest-team exemption rule in national / regional / district cups.
 std::vector<int> draw={root};for(auto& q:g_career.pyramids[pi].pools)for(int t:q.clubs)if(t!=root&&draw.size()<7)draw.push_back(t);
 float originalRating=g_world.teams[root].rating;g_world.teams[root].rating=99;
 for(int kind:{2,4,5}){Competition c;c.kind=kind;c.format=FMT_CUP;c.koTargets={4,2,1};c.koTimes={1,2,3};c.cupRound(0,draw);bool found=false;for(auto& t:c.stages[0].ties)if(t.a==root||t.b==root){found=true;check(t.b>=0,"highest rated debut club never gets cup exemption");}check(found,"new club present in cup draw");}

 // Even if all survivors are new clubs, every qualification requires a played
 // game; the extra adjustment stage must survive a save/resume as well.
 auto actualDebuts=g_career.clubDebuts;for(int t:draw)g_career.markClubDebut(t,2026);
 for(int kind:{2,4,5}){
  Competition c;c.kind=kind;c.format=FMT_CUP;c.koTargets={4,2,1};c.koTimes={1,2,3};c.cupRound(0,draw);
  check(c.awaiting==0&&c.carry.size()==5,"all-new odd field uses played adjustment instead of exemption");
  int ci=(int)g_career.season.comps.size();g_career.season.comps.push_back(c);
  check(g_career.save("build/test-adjustment.sav")&&g_career.load("build/test-adjustment.sav"),"save during adjustment round");
  auto& cup=g_career.season.comps[ci];check(cup.awaiting==0&&cup.carry.size()==5,"adjustment survivors and logical round restored");
  for(int round=0;round<20&&!cup.done;round++){
   int stage=cup.cur;for(auto& tie:cup.stages[stage].ties)check(tie.b>=0,"no exemptions even when every entrant is new");
   for(auto& m:cup.matches)if(!m.played)simulateMatch(m,&cup);
   cup.onStageFinished();
  }
  check(cup.done&&cup.winner>=0,"all-new cup finishes with a real winner");g_career.season.comps.pop_back();
 }
 g_career.clubDebuts=actualDebuts;
 g_world.teams[root].rating=originalRating;
 for(int ci:g_career.cdfRegional){const auto& c=g_career.season.comps[ci];int first=-1,entry=-1;for(int i=0;i<(int)c.entrants.size();i++)if(!c.entrants[i].empty()){if(first<0)first=i;for(int t:c.entrants[i])if(t==root)entry=i;}if(entry>=0)check(entry==first,"new club enters first Coupe de France round including preliminary");}
 check(g_career.polesEnabled()&&g_career.poleTeams.size()==16,"16 men's poles in French career");int poleCup=g_career.poleCup;
 auto& tournament=g_career.season.comps[poleCup];check(tournament.stages[0].groups.size()==4&&tournament.matches.size()==24,"4 groups of 4, 24 group games");
 std::set<int> pids;for(int t:g_career.poleTeams){check(g_world.teams[t].youth==8&&g_world.teams[t].squad.size()==22,"pole squad of 22");for(auto& p:g_world.teams[t].squad){check(p.age==13||p.age==14,"pole intake 13-14");check(pids.insert(p.id).second,"unique pole player identifier");}}
 std::string err;int reserved=g_career.createReserve(err),u15=g_career.createYouth(root,3,err);check(reserved>=0&&u15>=0,"reserve and youth requests accepted");
 check(g_career.tierOfTeam(reserved)<0&&g_career.tierOfTeam(u15)<0,"pending sections not playing this season");
 check(g_career.pendingNewClubs.size()==2,"two sections queued");
 bool absent=true;for(auto& c:g_career.season.comps)for(auto& m:c.matches)absent&=m.home!=reserved&&m.away!=reserved&&m.home!=u15&&m.away!=u15;check(absent,"current calendar unchanged for pending sections");
 int player=g_world.teams[g_career.poleTeams[0]].squad[2].id;check(!g_career.recruitPole(player,u15,err),"cannot recruit before finale");
 int league=g_career.pyramids[pi].pools[qi].comps[gi];check(g_career.prepareSeasonAwards()<0,"no premature ceremony");
 finishSubset({league,poleCup});
 auto& finishedCup=g_career.season.comps[poleCup];check(finishedCup.done&&finishedCup.winner>=0,"pole tournament has winner");check(finishedCup.matches.size()==31&&finishedCup.stages.size()==4,"24 group games and seven knockout games");
 for(auto& m:finishedCup.matches){check(m.neutral&&m.noET&&!m.aet&&m.played,"pole venue neutral, no extra time");if(m.decisive&&m.hg==m.ag)check(m.ph>=0&&m.pa>=0&&m.ph!=m.pa,"knockout ties resolved by penalties");}
 for(auto& e:finishedCup.events)check(e.minute<=40,"all pole events within forty minutes");
 check(g_career.divisionRules(poleCup).find("20 minutes")!=std::string::npos,"tournament rules disclose duration");
 auto ranks=g_career.poleRanking();check(ranks.size()==352,"final ranking includes every prospect");for(int i=1;i<(int)ranks.size();i++)check(ranks[i-1].score>=ranks[i].score,"prospect ranking sorted");
 check(g_career.recruitPole(player,u15,err),"reserve a prospect for own U15");check(!g_career.recruitPole(player,u15,err),"duplicate recruitment rejected");check(g_world.findPlayer(player)==g_career.poleTeams[0],"no immediate midyear player move");
 int age=g_world.teams[g_career.poleTeams[0]].squad[2].age;
 int edition=g_career.prepareSeasonAwards();check(edition==0,"one end-season awards edition");auto awards=g_career.seasonAwards[0];check(std::string(awards.competition)==g_career.season.comps[league].name,"ceremony is actual played league");
 std::set<int> eligible;for(auto& group:g_career.season.comps[league].stages[0].groups)for(int t:group)eligible.insert(t);
 for(int k=0;k<6;k++){check(awards.valid[k],"six winners available");check(eligible.count(awards.winners[k].team),"every winner from user's league");}
 check(awards.winners[5].team==g_career.season.comps[league].winner,"coach award to championship winner");

 // A rival with huge aggregate/cup statistics must not steal league-only awards.
 int outsider=0;for(int t:eligible)for(const auto& p:g_world.teams[t].squad)if(p.id!=awards.winners[1].pid&&p.id!=awards.winners[2].pid&&p.pos!=POS_GK)outsider=p.id;
 int outsiderIx=-1,outsiderTeam=g_world.findPlayer(outsider,&outsiderIx);check(outsiderTeam>=0,"league rival found for separation test");
 auto& outsidePlayer=g_world.teams[outsiderTeam].squad[outsiderIx];int oldGoals=outsidePlayer.goals,oldAssists=outsidePlayer.assists;outsidePlayer.goals=outsidePlayer.assists=999;
 int unrelated=g_career.cdfRegional.front();auto eventSize=g_career.season.comps[unrelated].events.size();
 for(int i=0;i<300;i++){MEv e;e.type=0;e.pid=outsider;e.team=outsiderTeam;g_career.season.comps[unrelated].events.push_back(e);}
 g_career.seasonAwards.clear();g_career.prepareSeasonAwards();
 check(g_career.seasonAwards[0].winners[1].pid==awards.winners[1].pid&&g_career.seasonAwards[0].winners[2].pid==awards.winners[2].pid,"cup and global stats excluded from league scorer and passer awards");
 outsidePlayer.goals=oldGoals;outsidePlayer.assists=oldAssists;g_career.season.comps[unrelated].events.resize(eventSize);
 check(g_career.prepareSeasonAwards()==0&&g_career.seasonAwards.size()==1,"ceremony cannot duplicate");
 check(g_career.save("build/test-build3-v27.sav")&&g_career.load("build/test-build3-v27.sav"),"new features save and load together");check(g_career.pendingNewClubs.size()==2&&g_career.poleRecruits.size()==1&&g_career.poleTeams.size()==16&&g_career.poleCup==poleCup,"pending registrations, scout choice and tournament retained");
 check(!std::strcmp(g_career.seasonAwards[0].winners[1].name,awards.winners[1].name),"frozen ceremony winners after load");
 // Physical match clock and direct penalties, not just simulated results.
 MatchSetup setup;setup.home=g_career.poleTeams[0];setup.away=g_career.poleTeams[1];setup.halfMinutes=20;setup.noET=true;setup.decisive=true;
 Match match;match.init(setup);match.startPeriod(0);check(match.clock==0&&match.periodEnd==20,"first physical period ends at 20");match.startPeriod(1);check(match.clock==20&&match.periodEnd==40,"second physical period ends at 40");match.score[0]=match.score[1]=0;match.endPeriod();check(match.nextSp==4&&!match.aet,"physical knockout draw goes straight to penalties");
 g_career.mgr.noSack=true;g_career.endSeason();check(g_career.year==2027&&g_career.pendingNewClubs.empty(),"new season consumes registration requests");
 int rp,rq,rg;check(g_career.tierOfTeam(reserved,&rp,&rq,&rg)==11,"reserve now enters bottom division");check(g_career.pyramids[rp].pools[rq].key==districtIndex("Marne"),"reserve correct district next year");check(g_career.tierOfTeam(u15)>=0,"U15 section enrolled next year");check(g_world.findPlayer(player)==u15,"reserved prospect joins requested U15 team");int ix=-1;g_world.findPlayer(player,&ix);check(g_world.teams[u15].squad[ix].age==age+1,"young player's age advances exactly once");check(g_career.poleRecruits.empty(),"completed recruit request removed");
 check(g_career.poleTeams.size()==16&&g_career.season.comps[g_career.poleCup].matches.size()==24,"next annual pole tournament created");for(int t:g_career.poleTeams)for(auto& p:g_world.teams[t].squad)check(p.age==13||p.age==14,"outgoing cohort replaced by 13-year-olds");
 // Standalone Championship can continue through three years with four human clubs.
 int germany=club("GER");g_career.opts=Career::Opts();g_career.opts.lite=1;g_career.newClubCareer(germany,2026);std::vector<int> ctrl;
 for(auto& p:g_career.pyramids)if(p.country=="GER")for(auto& q:p.pools)if(q.tier==0){ctrl.assign(q.clubs.begin(),q.clubs.begin()+4);break;}
 g_career.season.controlled=ctrl;
 for(int season=0;season<3;season++){
  int p,q,g;check(g_career.tierOfTeam(germany,&p,&q,&g)>=0,"Championship club still registered");int c=g_career.pyramids[p].pools[q].comps[g];finishSubset({c});g_career.endSeason();
  check(g_career.year==2027+season,"Championship advances a calendar year");check(g_career.season.controlled==ctrl,"all four human clubs retained across seasons");check(g_career.mgr.noSack&&!g_career.mgr.managerMode&&!g_career.mgr.sacked,"Championship stays without managerial dismissal");check(!g_career.awardsEnabled()&&!g_career.polesEnabled(),"career features excluded from Championship");
 }
 check(g_career.save("build/test-championship-multiseason.sav")&&g_career.load("build/test-championship-multiseason.sav")&&g_career.season.controlled==ctrl&&g_career.year==2029,"multiseason Championship persists after load");

 // Extra draw seeds exercise the interaction of new entrants, repeat exemptions,
 // odd consolation fields, and the consecutive-home-game constraint.
 for(int seed=1;seed<=8;seed++){
  g_rng.s=seed*765431ULL;g_career.opts=Career::Opts();g_career.newClubCareer(root,2026);
  std::set<int> cups;for(int ci=0;ci<(int)g_career.season.comps.size();ci++)if(marneCupPart(g_career.season.comps[ci])>=0)cups.insert(ci);
  finishSubset(cups);
  for(int ci:cups){const auto& c=g_career.season.comps[ci];check(c.done&&c.winner>=0,"Marne stress: all three cups finish");
   check(c.stages.back().name=="Finale"&&c.stages.back().ties.size()==1&&c.stages.back().ties[0].b>=0,"Marne stress: actual two-team final");
   for(auto& st:c.stages)for(auto& t:st.ties)if(t.b<0)check(!clubFirstSeason(t.a),"Marne stress: new club never exempt");
   if(marneCupPart(c)==0)continue;
   std::set<int> byes;std::map<int,int> home;
   for(const auto& st:c.stages)for(const auto& t:st.ties)if(t.b<0){check(byes.insert(t.a).second,"Marne stress: no second exemption");check(st.name!="Quarts de finale"&&st.name!="Demi-finales"&&st.name!="Finale","Marne stress: no late exemption");}
   else {check(home[t.a]<2||home[t.b]>=2,"Marne stress: home streak rule");home[t.a]++;home[t.b]=0;}
  }
 }
 g_career.newClubCareer(u15,2026);check(g_career.polesEnabled()&&g_career.poleTeams.size()==16,"Pôles also available in U15 club career");
 std::printf("PASS build3 features: %d checks\n",checks);
}
