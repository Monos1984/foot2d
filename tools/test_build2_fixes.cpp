#include "../src/game.h"
#include <cstdlib>
#include <cmath>
#include <set>
static int checks=0;
static void check(bool b,const char* s){checks++;if(!b){std::printf("FAIL %s\n",s);std::exit(1);}}
int main(){
 g_world.build();
 const char* starts[]={"13 juillet 1930","27 mai 1934","4 juin 1938","24 juin 1950","16 juin 1954","8 juin 1958","30 mai 1962","11 juillet 1966","31 mai 1970","13 juin 1974","1 juin 1978","13 juin 1982","31 mai 1986","8 juin 1990","17 juin 1994","10 juin 1998","31 mai 2002","9 juin 2006","11 juin 2010","12 juin 2014","14 juin 2018","20 novembre 2022"};
 const char* ends[]={"30 juillet 1930","10 juin 1934","19 juin 1938","16 juillet 1950","4 juillet 1954","29 juin 1958","17 juin 1962","30 juillet 1966","21 juin 1970","7 juillet 1974","25 juin 1978","11 juillet 1982","29 juin 1986","8 juillet 1990","17 juillet 1994","12 juillet 1998","30 juin 2002","9 juillet 2006","11 juillet 2010","13 juillet 2014","15 juillet 2018","18 décembre 2022"};
 for(int ed=0;ed<22;ed++){
  check(legendDateText(legendCalendarTime(ed,0),LEGENDS[ed].year)==starts[ed],"historic start date");
  check(legendDateText(legendCalendarTime(ed,1),LEGENDS[ed].year)==ends[ed],"historic end date");
  legendStart(g_career,ed,{});
  double first=1e30,last=-1e30;
  for(int n=0;n<500&&!g_career.season.finished;n++){
   auto& C=g_career.season.comps[0];
   for(auto& st:C.stages)for(auto& r:st.rounds){first=std::min(first,r.time);last=std::max(last,r.time);check(r.time>=legendCalendarTime(ed,0)-.001 && r.time<=legendCalendarTime(ed,1)+.001,"historic round within tournament dates");}
   g_career.season.advance(true);
  }
  check(g_career.season.finished && g_career.season.comps[0].winner>=0,"historic tournament completes");
  for(auto& st:g_career.season.comps[0].stages)for(auto& r:st.rounds){first=std::min(first,r.time);last=std::max(last,r.time);check(r.time>=legendCalendarTime(ed,0)-.001 && r.time<=legendCalendarTime(ed,1)+.001,"completed historic round within date bounds");}
  check(std::fabs(first-legendCalendarTime(ed,0))<.001,"first historic fixture on start date");
  check(std::fabs(last-legendCalendarTime(ed,1))<.001,"last historic fixture on end date");
 }
 int py=-1;for(int i=0;i<(int)g_basePyramids.size();i++)if(g_basePyramids[i].country=="FRA"&&g_basePyramids[i].dom<0)py=i;
 check(py>=0,"France pyramid");
 auto& base=g_basePyramids[py];int user=-1;
 for(int t:base.pools[base.poolIndex(8,districtIndex("Marne"))].clubs)if(g_world.teams[t].parent<0){bool has=false;for(auto& x:g_world.teams)has|=x.parent==t&&!x.youth;if(!has){user=t;break;}}
 check(user>=0,"Marne fanion without reserve");
 g_career.opts=Career::Opts();g_career.newClubCareer(user,2026);
 std::string err;int res=g_career.createReserve(err);
 check(res>=0,"reserve created");check(g_world.teams[res].district==districtIndex("Marne"),"reserve keeps actual district");
 int qi=-1,gi=-1;int tier=g_career.tierOfTeam(res,&py,&qi,&gi);
 check(tier<0,"reserve waits until next season");
 check(g_career.pendingNewClubs.size()==1,"reserve queued for next season");
 g_career.season.now=5;int before=(int)g_career.season.comps.size();
 int late=g_career.createReserve(err);check(late>=0,"midseason reserve request accepted");
 check(g_career.tierOfTeam(late)<0,"midseason reserve not registered in current league");
 check(g_career.pendingNewClubs.size()==2,"both reserves queued");
 check((int)g_career.season.comps.size()==before,"current competitions preserved");
 bool absent=true;for(auto& c:g_career.season.comps)for(auto& m:c.matches)absent&=m.home!=res&&m.away!=res&&m.home!=late&&m.away!=late;check(absent,"no current fixture for pending reserves");
 int cups=0;for(auto& C:g_career.season.comps)if(marneCupPart(C)>=0)cups++;else {C.done=true;for(auto& st:C.stages)st.finished=true;}
 check(cups==3,"three Marne cups replace generic cups");
 for(int n=0;n<500&&!g_career.season.finished;n++)g_career.season.advance(true);
 check(g_career.season.finished,"Marne season finishes");
 std::set<int> routed;double finalTime=-1;std::string site;
 for(int c=0;c<(int)g_career.season.comps.size();c++){
  auto& C=g_career.season.comps[c];int part=marneCupPart(C);if(part<0)continue;
  check(C.done && C.winner>=0,"each Marne cup has a champion");
  auto rules=g_career.divisionRules(c);check(rules.find("Finale : prolongation")==std::string::npos,"Marne rules never promise final extra time");
  int sheet,subs;bool rolling;g_career.sheetRules(c,sheet,subs,rolling);check(sheet==16&&subs==5&&!rolling,"Marne final sheet and substitutions");
  int savedCur=C.cur;C.cur=0;g_career.sheetRules(c,sheet,subs,rolling);check(sheet==14&&rolling,"Marne previous rounds allow returning substitutes");C.cur=savedCur;
  for(auto& m:C.matches)check(m.noET && m.played,"Marne games played without extra time");
  auto& final=C.stages.back();check(final.name=="Finale" && final.ties.size()==1 && final.ties[0].b>=0,"real final pairing");
  if(finalTime<0)finalTime=final.rounds[0].time;else check(std::fabs(finalTime-final.rounds[0].time)<.001,"all three finals same day");
  if(site.empty())site=g_career.cupVenue[c];else check(site==g_career.cupVenue[c],"all three finals same venue");
  if(part){
   std::set<int> exempt;std::map<int,int> home;
   for(auto& st:C.stages)for(auto& tie:st.ties) {
    if(tie.b<0){check(exempt.insert(tie.a).second,"one consolation exemption maximum");check(st.name!="Quarts de finale"&&st.name!="Demi-finales"&&st.name!="Finale","no exemption after quarters");}
    else {if(home[tie.a]>=2 && home[tie.b]<2)std::printf("HOME ERROR cup=%s stage=%s a=%d (%d) b=%d (%d)\n",C.name.c_str(),st.name.c_str(),tie.a,home[tie.a],tie.b,home[tie.b]);check(home[tie.a]<2 || home[tie.b]>=2,"no third home game unless both constrained");home[tie.a]++;home[tie.b]=0;}
   }
   for(auto& e:C.entrants)for(int t:e){check(routed.insert(t).second,"no duplicate consolation entrant");int tlevel=g_career.tierOfTeam(t);check(part==1?(tlevel==8||tlevel==9):(tlevel==10||tlevel==11),"repechage respects division");}
  }
 }
 check(g_career.save("build/test-marne.sav") && g_career.load("build/test-marne.sav"),"Marne cups save and load");
 for(auto& C:g_career.season.comps)if(marneCupPart(C)>=0)check(C.done&&C.winner>=0,"Marne champions persist");
 std::printf("PASS build2 fixes: %d checks\n",checks);
}
