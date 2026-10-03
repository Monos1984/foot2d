#include "game.h"
#include <set>

int marneCupPart(const Competition& c) {
 if(c.kind!=5 || c.tag!=districtIndex("Marne")) return -1;
 if(c.shortName=="Marne Neotec") return 0;
 if(c.shortName=="Marne D1/D2") return 1;
 if(c.shortName=="Marne D3/D4") return 2;
 return -1;
}
// Calendar dates relative to the first Monday of August, retained in saved rounds.
static double dateTime(int year,int month,int day) {
 int dow=(year+year/4-year/100+year/400+2)%7;
 static const int dm[]={31,28,31,30,31,30,31,31,30,31,30,31};
 int days=0;for(int m=8;m<month;m++)days+=dm[m-1];
 if(month<8){for(int m=8;m<=12;m++)days+=dm[m-1];for(int m=1;m<month;m++)days+=dm[m-1]+(m==2 && (year+1)%4==0 && ((year+1)%100!=0 || (year+1)%400==0));}
 return (days+day-(1+(7-(dow+6)%7)%7))/7.0;
}
static bool aliveIn( const Competition& c,int t) {
 if(c.done)return false;
 for(int s=0;s<(int)c.stages.size();s++) if(!c.stages[s].finished)
  for(const auto& tie:c.stages[s].ties) if((tie.a==t || tie.b==t) && (tie.winner<0 || tie.winner==t)) return true;
 for(const auto& e:c.entrants) if(std::find(e.begin(),e.end(),t)!=e.end()) {
  bool lost=false;for(const auto& st:c.stages) for(const auto& tie:st.ties)
   if((tie.a==t || tie.b==t) && tie.winner>=0 && tie.winner!=t)lost=true;
  if(!lost)return true;
 }
 return false;
}
static bool protectedTeam(int t) {
 if(clubFirstSeason(t))return false;
 const auto& k=g_career;
 for(int i:k.cdfRegional) if(i>=0 && aliveIn(k.season.comps[i],t))return true;
 if(k.cdfNational>=0 && aliveIn(k.season.comps[k.cdfNational],t))return true;
 for(int i:k.regionalCups) if(i>=0 && k.season.comps[i].tag==5 && aliveIn(k.season.comps[i],t))return true;
 return false;
}
void marneDraw(Competition& c,int round,const std::vector<int>& survivors) {
 int part=marneCupPart(c);if(part<0)return;
 std::vector<int> pool=survivors;
 pool.insert(pool.end(),c.carry.begin(),c.carry.end());c.carry.clear();
 if(round<(int)c.entrants.size() && !(c.extReadyMask&(1<<round))){pool.insert(pool.end(),c.entrants[round].begin(),c.entrants[round].end());c.extReadyMask|=1<<round;}
 std::sort(pool.begin(),pool.end());pool.erase(std::unique(pool.begin(),pool.end()),pool.end());g_rng.shuffle(pool);
 int last=part?7:8;
 if(round>last){c.result=pool;c.winner=pool.empty()?-1:pool[0];c.done=true;return;}
 int target;
 if(!part && round<3)target=c.koTargets[round];
 else if(!part)target=1<<(last-round);
 else target=round<5?std::max(8,((int)pool.size()+1)/2):1<<(last-round);
 target=std::max(1,target);
 // With an odd field and nobody eligible for a sporting exemption, play
 // an adjustment tie and then redraw the same round with its winner.
 // Remaining teams wait in carry; no second exemption is recorded.
 if(part && pool.size()>2 && pool.size()%2){
  bool eligible=false;for(int t:pool)if(!clubFirstSeason(t)&&std::find(c.extra.begin(),c.extra.end(),t)==c.extra.end())eligible=true;
  if(!eligible || round>=5){
   c.carry.assign(pool.begin()+2,pool.end());c.regionalRounds=round-1;c.cur=(int)c.stages.size();
   auto streak=[&](int t){int n=0;for(int s=(int)c.stages.size()-1;s>=0;s--){bool home=false,away=false;for(const auto& tie:c.stages[s].ties)if(tie.b>=0){home|=tie.a==t;away|=tie.b==t;}if(away)break;if(home)n++;}return n;};
   int a=pool[0],b=pool[1];if(streak(a)>=2&&streak(b)<2)std::swap(a,b);
   c.addKOStage({{a,b}},1,c.koTimes[round]-1.0/7,"Match d'ajustement du tableau",false);return;
  }
 }
 std::vector<int> byes,play;
 for(int t:pool)if(!part && round<=4 && protectedTeam(t))byes.push_back(t);else play.push_back(t);
 int games=std::max(0,std::min((int)pool.size()-target,(int)play.size()/2));
 if(part && (int)pool.size()<=target && round<last) {
  // No draw is required until the bracket has enough entrants. This is waiting,
  // not a second sporting exemption for the same team.
  c.carry=pool;
  c.regionalRounds=round;c.cur=(int)c.stages.size();
  c.addKOStage({},1,c.koTimes[round],c.koNames[round],false);return;
 }
 if(part && round<last) {
  // A consolation team can be exempt only once before quarter-finals.
  std::stable_sort(play.begin(),play.end(),[&](int a,int b){return (std::find(c.extra.begin(),c.extra.end(),a)!=c.extra.end())>(std::find(c.extra.begin(),c.extra.end(),b)!=c.extra.end());});
  if(round<5) {
   int unused=0;for(int t:play)unused+=std::find(c.extra.begin(),c.extra.end(),t)==c.extra.end();
   games=std::max(games,((int)play.size()-unused+1)/2);
  }
 }
 std::stable_sort(play.begin(),play.end(),[](int a,int b){return clubFirstSeason(a)>clubFirstSeason(b);});
 int debut=0;for(int t:play)debut+=clubFirstSeason(t);games=std::min((int)play.size()/2,std::max(games,(debut+1)/2));
 if(part && round>=5)games=(int)play.size()/2;
 std::vector<std::pair<int,int>> pairs;
 for(int i=0;i<games;i++) {
  int a=play[i*2],b=play[i*2+1];
  if(!part) {if(g_career.tierOfTeam(b)>=g_career.tierOfTeam(a)+2)std::swap(a,b);}
  else if(!c.stages.empty()) {
   auto streak=[&](int t){int n=0;for(int s=(int)c.stages.size()-1;s>=0;s--){bool home=false,away=false;for(const auto& tie:c.stages[s].ties)if(tie.b>=0){home|=tie.a==t;away|=tie.b==t;}if(away)break;if(home)n++;}return n;};
   if(streak(a)>=2 && streak(b)<2)std::swap(a,b);
  }
  pairs.push_back({a,b});
 }
 for(int i=games*2;i<(int)play.size();i++)byes.push_back(play[i]);
 for(int t:byes){pairs.push_back({t,-1});if(part && std::find(c.extra.begin(),c.extra.end(),t)==c.extra.end())c.extra.push_back(t);}
 c.regionalRounds=round;c.cur=(int)c.stages.size();
 c.addKOStage(pairs,1,c.koTimes[round],c.koNames[round],round==last);
}
bool marneStageFinished(Competition& c,const std::vector<int>& winners) {
 int part=marneCupPart(c);if(part<0)return false;
 int round=c.regionalRounds;
 if(!part && round<4) {
  std::vector<int> losers;for(const auto& tie:c.stages[c.cur].ties)if(tie.a>=0 && tie.b>=0 && tie.winner>=0)losers.push_back(tie.winner==tie.a?tie.b:tie.a);
  for(int t:losers) {
   int tier=g_career.tierOfTeam(t);int dest=tier==8 || tier==9?1:tier==10 || tier==11?2:-1;
   if(dest<0)continue;
   for(int ci:g_career.deptCups){auto& minor=g_career.season.comps[ci];if(marneCupPart(minor)!=dest)continue;
    minor.entrants[round].push_back(t);
    g_career.season.news.push_back(g_world.teams[t].name+" est repêchée en "+minor.name+".");
   }
  }
  if(round==0)for(int ci:g_career.deptCups){auto& minor=g_career.season.comps[ci];if(marneCupPart(minor)>0 && minor.stages.empty())marneDraw(minor,0,{});}
 }
 if(round==(part?7:8)){c.done=true;c.winner=winners.empty()?-1:winners[0];c.result=winners;return true;}
 marneDraw(c,round+1,winners);return true;
}
void createMarneCups(Career& k,const std::vector<int>& teams) {
 if(teams.size()<4)return;
 std::vector<int> district,regional;for(int t:teams)(k.tierOfTeam(t)<8 && !clubFirstSeason(t)?regional:district).push_back(t);
 for(int part=0;part<3;part++) {
  Competition c;c.kind=5;c.tag=districtIndex("Marne");c.format=FMT_CUP;c.neutralFinal=true;c.homeRule=0;c.yellowLimit=3;
  c.name=part==0?"Coupe de la Marne Neotec - Challenge Éric Collinet":part==1?"Coupe de la Marne D1/D2":"Coupe de la Marne D3/D4";
  c.shortName=part==0?"Marne Neotec":part==1?"Marne D1/D2":"Marne D3/D4";
  std::vector<std::pair<int,int>> dates={{8,30},{9,13},{10,11},{11,29},{2,14},{3,7},{3,29},{4,25},{5,16}};
  if(part)dates.erase(dates.begin());
  for(auto d:dates)c.koTimes.push_back(dateTime(k.year,d.first,d.second));
  c.entrants.resize(dates.size());c.koNames.resize(dates.size());c.koLegs.assign(dates.size(),1);
  for(int r=0;r<(int)dates.size();r++)c.koNames[r]=fmt("Tour %d",r+1);
  int last=(int)dates.size()-1;c.koNames[last]="Finale";c.koNames[last-1]="Demi-finales";c.koNames[last-2]="Quarts de finale";
  if(!part){c.koNames[4]="16es de finale";c.koNames[5]="8es de finale";c.entrants[0]=district;c.entrants[3]=regional;
   int remain=std::max(8,32-(int)regional.size());int n=(int)district.size();
   c.koTargets={std::max(remain,(n*3+3)/4),std::max(remain,(n+1)/2),remain};}
  c.season=k.year;int i=(int)k.season.comps.size();k.season.comps.push_back(c);k.deptCups.push_back(i);
  // The 2027 venue has not been announced: one simulated selection for all finals.
  k.cupVenue[i]="Stade Auguste-Delaune, Reims (site choisi dans la simulation)";
 }
 for(int i:k.deptCups)if(marneCupPart(k.season.comps[i])==0)marneDraw(k.season.comps[i],0,{});
}
