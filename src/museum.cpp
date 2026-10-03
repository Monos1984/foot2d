#include "game.h"
#include "serial.h"
#include "packio.h"
#include <cstring>
#include <set>
#include <limits>
template<size_t N> static void label(char (&dst)[N],const std::string& s){snprintf(dst,N,"%s",s.c_str());}
// clubs suivis par le Musée : ceux du joueur (et ceux qu'il a dirigés), et le football national suivi par le joueur
// (6 premiers niveaux de son pays, outre-mer compris). Les autres clubs ont leurs « archives antérieures » (archive.cpp) ;
// leur Musée commence le jour où le joueur y arrive.
static bool validClub(const Career& K,int t){
 if(!(K.museumEnabled()&&t>=0&&t<(int)g_world.teams.size()&&g_world.teams[t].kind==TK_CLUB&&!g_world.teams[t].freeAgents&&g_world.teams[t].youth!=8))return false;
 if(K.clubHistories.count(t)||t==K.userTeam)return true;
 const Team& T=g_world.teams[t];int u=K.userTeam;if(u<0||u>=(int)g_world.teams.size())return false;
 if(T.parent>=0)return T.parent==u;
 return T.nation==g_world.teams[u].nation&&teamLevel(t)<=5;
}
bool Career::museumEnabled() const{return kind==CK_CLUB&&!opts.lite&&!euroOnly;}
static uint64_t key(int c,int m){return (uint64_t)(uint32_t)c<<32|(uint32_t)m;}
static bool remember(std::vector<uint64_t>& v,uint64_t id){auto it=std::lower_bound(v.begin(),v.end(),id);if(it!=v.end()&&*it==id)return false;v.insert(it,id);return true;}
static void event(ClubHistory& H,int year,double time,int category,const std::string& text){MuseumEvent e;e.year=year;e.time=time;e.category=category;label(e.text,text);H.timeline.push_back(e);}
static MuseumSeason& active(Career& K,ClubHistory& H){
 if(H.seasons.empty()||H.seasons.back().year!=K.year){MuseumSeason s;s.year=K.year;s.tier=s.newTier=K.tierOfTeam(H.clubId);label(s.division,K.teamLevelName(H.clubId));
  int p,q,g;if(K.tierOfTeam(H.clubId,&p,&q,&g)>=0&&g>=0&&g<(int)K.pyramids[p].pools[q].comps.size()){s.league=K.pyramids[p].pools[q].comps[g];if(s.league>=0&&s.league<(int)K.season.comps.size())label(s.competition,K.season.comps[s.league].name);}
  H.seasons.push_back(s);}
 if(H.seenYear!=K.year){H.seenYear=K.year;H.seenMatches.clear();H.seenEvents.clear();H.seenGates.clear();}return H.seasons.back();
}
static ClubHistory& club(Career& K,int t){auto& H=K.clubHistories[t];if(H.clubId<0){H.clubId=t;H.firstYear=K.year;event(H,K.year,K.season.now,MH_ALL,g_world.teams[t].custom?"Création du club / début des archives":"Début des archives de la carrière");}active(K,H);return H;}
static MuseumPlayer& player(ClubHistory& H,const Player& p){for(auto& v:H.players)if(v.pid==p.id)return v;MuseumPlayer v;v.pid=p.id;label(v.name,p.name);H.players.push_back(v);return H.players.back();}
static bool record(ClubHistory& H,int kind,int64_t value,int year,double time,const std::string& holder,const std::string& comp={},bool minimum=false,int pid=0){
 MuseumRecord* found=nullptr;for(auto& r:H.records)if(r.kind==kind)found=&r;
 if(found&&(minimum?value>=found->value:value<=found->value))return false;
 if(!found){H.records.push_back(MuseumRecord());found=&H.records.back();}found->kind=kind;found->value=value;found->year=year;found->time=time;found->pid=pid;label(found->holder,holder);label(found->competition,comp);
 if(H.managers.empty())return true;   // clubs jamais dirigés : le record suffit (liste des records), pas d'événement en double dans la chronologie
 std::string prefix=std::string(museumRecordName(kind))+" : ";std::string text=prefix+holder+fmt(" (%lld)",(long long)value);bool consolidated=false;for(auto it=H.timeline.rbegin();it!=H.timeline.rend();++it){if(it->year!=year)break;if(it->category==MH_RECORD&&std::string(it->text).rfind(prefix,0)==0){it->time=time;label(it->text,text);consolidated=true;break;}}if(!consolidated)event(H,year,time,MH_RECORD,text);return true;
}
static void observe(Career& K,ClubHistory& H,const Player& p){
 // pas de fiche pour un joueur qui n'a encore rien fait au club (elle serait créée à l'identique à son premier match)
 {bool known=false;for(auto& e:H.players)if(e.pid==p.id){known=true;break;}if(!known&&p.apps==0&&p.goals==0&&p.assists==0)return;}
 auto& v=player(H,p);if(v.observedYear!=K.year){v.observedYear=K.year;v.seenApps=v.seenGoals=v.seenAssists=v.seasonGoals=0;}
 int apps=std::max(0,(int)p.apps-v.seenApps),goals=std::max(0,(int)p.goals-v.seenGoals),assists=std::max(0,(int)p.assists-v.seenAssists);v.apps+=apps;v.goals+=goals;v.assists+=assists;v.seasonGoals+=goals;v.seenApps=p.apps;v.seenGoals=p.goals;v.seenAssists=p.assists;
 if(apps){record(H,MR_YOUNGEST,p.age,K.year,K.season.now,p.name,{},true,p.id);record(H,MR_OLDEST,p.age,K.year,K.season.now,p.name,{},false,p.id);}
 if(goals){record(H,MR_YOUNG_GOAL,p.age,K.year,K.season.now,p.name,{},true,p.id);record(H,MR_OLD_GOAL,p.age,K.year,K.season.now,p.name,{},false,p.id);record(H,MR_SEASON_GOALS,v.seasonGoals,K.year,K.season.now,p.name,{},false,p.id);}
 if(v.apps)record(H,MR_APPS,v.apps,K.year,K.season.now,p.name,{},false,p.id);if(v.goals)record(H,MR_GOALS,v.goals,K.year,K.season.now,p.name,{},false,p.id);if(v.assists)record(H,MR_ASSISTS,v.assists,K.year,K.season.now,p.name,{},false,p.id);
}
static bool european(const Career& K,int c){return c==K.ucl||c==K.uel||c==K.uecl||c==K.intertoto||c==K.uefaSuper;}
static bool preliminary(const Career& K,int c){return std::find(K.cdfRegional.begin(),K.cdfRegional.end(),c)!=K.cdfRegional.end()||c==K.intertoto;}
static double matchTime(const Career& K,const Competition& C,int mi){for(auto& st:C.stages)for(auto& r:st.rounds)if(std::find(r.m.begin(),r.m.end(),mi)!=r.m.end())return r.time;return K.season.now;}
static MuseumMatch summary(Career& K,int ci,int mi){const auto& C=K.season.comps[ci];const auto& r=C.matches[mi];MuseumMatch m;m.year=K.year;m.comp=ci;m.match=mi;m.home=r.home;m.away=r.away;m.hg=r.hg;m.ag=r.ag;m.ph=r.ph;m.pa=r.pa;m.extraTime=r.aet;m.time=matchTime(K,C,mi);m.winner=r.hg==r.ag?(r.ph<0?-1:r.ph>r.pa?r.home:r.away):r.hg>r.ag?r.home:r.away;
 label(m.competition,C.name);label(m.homeName,g_world.teams[r.home].name);label(m.awayName,g_world.teams[r.away].name);label(m.venue,r.neutral?(K.cupVenue.count(ci)?K.cupVenue[ci]:"Terrain neutre (non renseigné)"):g_world.teams[r.home].stadium);
 int st=C.stageOfMatch(mi);if(st>=0&&st==(int)C.stages.size()-1&&C.stages[st].ties.size()==1&&!preliminary(K,ci)){const auto& name=C.stages[st].name;m.final=C.done||name.find("Finale")!=std::string::npos||name=="Final";}
 for(int side=0;side<2;side++){int t=side?r.away:r.home;const Team& T=g_world.teams[t];if(!T.squadGen||T.squad.size()<11)continue;auto lu=g_world.pickLineup(t,T.formation,RULESET_CAREER);for(int s=0;s<11&&s<(int)lu.size();s++)if(lu[s]>=0){auto& p=T.squad[lu[s]];m.xi[side][s]=p.id;label(m.lineupNames[side][s],p.name);if(p.id==T.captainPid)label(m.captain[side],p.name);}}
 return m;
}
void Career::museumState(int t){if(!validClub(*this,t))return;auto& H=club(*this,t);auto& S=active(*this,H);const Team& T=g_world.teams[t];
 if(H.stadiums.empty()||H.stadiums.back().name!=T.stadium){if(!H.stadiums.empty())H.stadiums.back().toYear=year;MuseumStadium st;st.fromYear=year;st.capacityStart=st.capacityEnd=T.sta.capacity();label(st.name,T.stadium);label(st.town,T.town);H.stadiums.push_back(st);event(H,year,season.now,MH_STADIUM,"Stade : "+T.stadium);}
 else if(H.stadiums.back().capacityEnd!=T.sta.capacity()){event(H,year,season.now,MH_STADIUM,fmt("Capacité du stade : %d -> %d places",H.stadiums.back().capacityEnd,T.sta.capacity()));H.stadiums.back().capacityEnd=T.sta.capacity();}
 if(t==userTeam){if(!S.budgetKnown){S.budgetKnown=1;S.budgetStart=mgr.budget;}S.budgetEnd=mgr.budget;if(mgr.budget>0)record(H,MR_BUDGET,mgr.budget,year,season.now,T.name);std::string name=managerName.empty()?"Manager du joueur":managerName;
  if(H.managers.empty()||std::string(H.managers.back().name)!=name||H.managers.back().toYear){MuseumManager m;m.fromYear=year;label(m.name,name);H.managers.push_back(m);event(H,year,season.now,MH_MANAGER,"Arrivée : "+name);}label(S.manager,name);}
 if(T.squadGen){int64_t value=0;for(auto& p:T.squad)value+=p.value();if(value>0)record(H,MR_SQUAD_VALUE,value,year,season.now,T.name);}
 MuseumShirt sh;sh.year=year;const Kit* kits[]={&T.home,&T.away,&T.third};for(int i=0;i<3;i++){sh.shirt[i]=kits[i]->shirt;sh.shirt2[i]=kits[i]->shirt2;sh.shorts[i]=kits[i]->shorts;sh.socks[i]=kits[i]->socks;sh.pattern[i]=kits[i]->pattern;}sh.third=T.hasThird;label(sh.sponsor,T.sponsor);
 if(H.shirts.empty()||H.shirts.back().year!=year||memcmp(H.shirts.back().shirt,sh.shirt,sizeof sh.shirt)||memcmp(H.shirts.back().shirt2,sh.shirt2,sizeof sh.shirt2)||memcmp(H.shirts.back().shorts,sh.shorts,sizeof sh.shorts)||memcmp(H.shirts.back().socks,sh.socks,sizeof sh.socks)||memcmp(H.shirts.back().pattern,sh.pattern,sizeof sh.pattern)||H.shirts.back().third!=sh.third||strcmp(H.shirts.back().sponsor,sh.sponsor))H.shirts.push_back(sh);
}
void Career::museumMatch(int ci,int mi){if(!museumEnabled()||ci<0||ci>=(int)season.comps.size())return;auto& C=season.comps[ci];if(mi<0||mi>=(int)C.matches.size()||C.kind==12)return;const auto& r=C.matches[mi];if(!r.played||r.home<0||r.away<0)return;
 MuseumMatch m=summary(*this,ci,mi);
 for(int side=0;side<2;side++){int t=side?r.away:r.home;if(!validClub(*this,t))continue;auto& H=club(*this,t);auto& S=active(*this,H);if(!remember(H.seenMatches,key(ci,mi)))continue;int gf=side?r.ag:r.hg,ga=side?r.hg:r.ag;const std::string opponent=side?m.homeName:m.awayName;
  if(ci==S.league){S.played++;S.gf+=gf;S.ga+=ga;if(gf>ga)S.wins++;else if(gf<ga)S.losses++;else S.draws++;S.points+=gf>ga?C.ptsWin:gf==ga?C.ptsDraw:C.ptsLoss;}
  H.unbeaten=gf>=ga?H.unbeaten+1:0;H.winStreak=gf>ga?H.winStreak+1:0;H.cleanStreak=ga==0?H.cleanStreak+1:0;
  record(H,MR_UNBEATEN,H.unbeaten,year,m.time,C.name);record(H,MR_WIN_STREAK,H.winStreak,year,m.time,C.name);record(H,MR_CLEAN_STREAK,H.cleanStreak,year,m.time,C.name);
  bool landmark=false;bool notable=gf-ga>=4||ga-gf>=4||gf+ga>=7;   // un record n'en fait un match de légende que s'il est marquant
  if(gf>ga)landmark|=record(H,MR_BIG_WIN,gf-ga,year,m.time,opponent+fmt(" (%d-%d)",gf,ga),C.name);if(ga>gf)landmark|=record(H,MR_BIG_LOSS,ga-gf,year,m.time,opponent+fmt(" (%d-%d)",gf,ga),C.name);landmark|=record(H,MR_TOTAL_GOALS,gf+ga,year,m.time,opponent+fmt(" (%d-%d)",gf,ga),C.name);
  if(!H.managers.empty()&&!H.managers.back().toYear){auto& manager=H.managers.back();manager.apps++;if(gf>ga)manager.wins++;else if(gf<ga)manager.losses++;else manager.draws++;}
  if(m.final){H.finals.push_back(m);event(H,year,m.time,MH_TROPHY,"Finale : "+C.name+" contre "+opponent+fmt(" (%d-%d)",gf,ga));}
  uint32_t reasons=(m.final?1u:0u)|(gf>=5||ga>=5?2u:0u)|(r.ph>=0?4u:0u)|(landmark&&notable?8u:0u);
  if(european(*this,ci)){if(!(H.firstFlags&1)){H.firstFlags|=1;reasons|=16;event(H,year,m.time,MH_EUROPE,"Premier match européen : "+C.name);}if(gf>ga&&!(H.firstFlags&2)){H.firstFlags|=2;reasons|=32;event(H,year,m.time,MH_EUROPE,"Première victoire européenne : "+C.name);}}
  if(g_world.teams[t].status==CS_PRO&&!(H.firstFlags&4)){H.firstFlags|=4;event(H,year,m.time,MH_ALL,"Premier match avec le statut professionnel");}
  if(reasons){m.reasons=reasons;H.legendaryMatches.push_back(m);}for(auto& p:g_world.teams[t].squad)observe(*this,H,p);
 }
}
void Career::museumEvents(int ci,int mi){if(!museumEnabled()||ci<0||ci>=(int)season.comps.size())return;auto& C=season.comps[ci];if(mi<0||mi>=(int)C.matches.size()||C.kind==12)return;auto& m=C.matches[mi];
 for(int side=0;side<2;side++){int t=side?m.away:m.home;if(!validClub(*this,t))continue;auto& H=club(*this,t);if(!remember(H.seenEvents,key(ci,mi)))continue;
  for(auto& p:g_world.teams[t].squad)observe(*this,H,p);int ga=side?m.hg:m.ag;
  for(auto& p:g_world.teams[t].squad)if(p.pos==POS_GK&&p.apps>0&&ga==0){auto lu=g_world.pickLineup(t,g_world.teams[t].formation);if(!lu.empty()&&lu[0]>=0&&g_world.teams[t].squad[lu[0]].id==p.id){auto& v=player(H,p);v.clean++;record(H,MR_CLEAN,v.clean,year,season.now,p.name,{},false,p.id);}}
  for(auto& p:g_world.teams[t].squad)if(p.id==g_world.teams[t].captainPid)player(H,p).captainMatches++;
  for(auto& e:C.events)if(e.match==mi&&e.type==0&&e.team==t){for(auto& p:g_world.teams[t].squad)if(p.id==e.pid){auto& v=player(H,p);if(ci==active(*this,H).league)v.leagueGoals++;else if(european(*this,ci))v.euroGoals++;else v.cupGoals++;}}
 }
}
void Career::museumAttendance(int ci,int mi,int attendance){if(!museumEnabled()||attendance<0||ci<0||ci>=(int)season.comps.size())return;auto& C=season.comps[ci];if(mi<0||mi>=(int)C.matches.size())return;int t=C.matches[mi].home;if(!validClub(*this,t))return;auto& H=club(*this,t);auto& S=active(*this,H);if(!remember(H.seenGates,key(ci,mi)))return;
 // A gate is booked once by the match-day system; retain its exact attendance.
 S.attendanceTotal+=attendance;S.homeMatches++;S.averageAttendance=S.attendanceTotal/std::max(1,S.homeMatches);S.maxAttendance=std::max(S.maxAttendance,attendance);record(H,MR_ATTENDANCE,attendance,year,season.now,g_world.teams[t].stadium,C.name);if(!H.stadiums.empty())H.stadiums.back().recordAttendance=std::max(H.stadiums.back().recordAttendance,attendance);
 for(auto* v:{&H.finals,&H.legendaryMatches})for(auto& m:*v)if(m.year==year&&m.comp==ci&&m.match==mi)m.attendance=attendance;
}
void Career::museumCompetition(int ci){if(!museumEnabled()||ci<0||ci>=(int)season.comps.size())return;auto& C=season.comps[ci];if(!C.done||C.winner<0||C.kind==12||preliminary(*this,ci)||!validClub(*this,C.winner))return;auto& H=club(*this,C.winner);
 for(auto& trophy:H.trophies)if(trophy.year==year&&trophy.comp==ci)return;
 MuseumTrophy trophy;trophy.year=year;trophy.comp=ci;trophy.kind=C.kind;trophy.time=season.now;label(trophy.name,C.name);
 for(int mi=0;mi<(int)C.matches.size();mi++)if(C.matches[mi].played){auto m=summary(*this,ci,mi);if(m.final)trophy.finalMatch=m;}
 for(auto& m:H.finals)if(m.year==year&&m.comp==ci){trophy.finalMatch=m;trophy.finalMatch.winner=C.winner;}
 H.trophies.push_back(trophy);active(*this,H).trophies++;event(H,year,season.now,MH_TROPHY,"Vainqueur : "+C.name);if(!H.managers.empty()&&!H.managers.back().toYear)H.managers.back().trophies++;
 for(auto& p:g_world.teams[C.winner].squad)if(p.apps>0)player(H,p).trophies++;
 for(int t:{trophy.finalMatch.home,trophy.finalMatch.away}){auto it=clubHistories.find(t);if(it==clubHistories.end())continue;for(auto* matches:{&it->second.finals,&it->second.legendaryMatches})for(auto& m:*matches)if(m.year==year&&m.comp==ci&&m.final)m.winner=C.winner;}
}
// bornage des historiques : les faits marquants restent, les détails les plus anciens et les moins importants partent
// (clubs dirigés par le joueur : limites larges ; autres clubs : archive plus légère)
static void museumTrim(ClubHistory& H,int year){
 bool managed=!H.managers.empty();
 if(!managed)H.timeline.erase(std::remove_if(H.timeline.begin(),H.timeline.end(),[](const MuseumEvent& e){return e.category==MH_RECORD;}),H.timeline.end());
 size_t capLeg=managed?150:8,capTime=managed?1200:60;
 // clubs jamais dirigés : matchs de légende sans les compositions détaillées (score, adversaire, compétition conservés)
 if(!managed)for(auto& m:H.legendaryMatches)if(!m.exactLineup){memset(m.lineupNames,0,sizeof m.lineupNames);memset(m.xi,0,sizeof m.xi);memset(m.captain,0,sizeof m.captain);}
 if(H.legendaryMatches.size()>capLeg){
  auto score=[](const MuseumMatch& m){return (m.reasons&1?100:0)+(m.reasons&48?60:0)+(m.reasons&2?20:0)+(m.reasons&4?15:0)+(m.reasons&8?10:0);};
  std::vector<size_t> idx(H.legendaryMatches.size());for(size_t i=0;i<idx.size();i++)idx[i]=i;
  std::stable_sort(idx.begin(),idx.end(),[&](size_t a,size_t b){int sa=score(H.legendaryMatches[a]),sb=score(H.legendaryMatches[b]);if(sa!=sb)return sa>sb;return a>b;});
  idx.resize(capLeg);std::sort(idx.begin(),idx.end());std::vector<MuseumMatch> keep;keep.reserve(capLeg);for(size_t i:idx)keep.push_back(H.legendaryMatches[i]);H.legendaryMatches.swap(keep);}
 if(H.timeline.size()>capTime){
  auto minor=[](const MuseumEvent& e){return e.category==MH_RECORD||e.category==MH_STADIUM||e.category==MH_ALL;};
  size_t extra=H.timeline.size()-capTime;std::vector<MuseumEvent> keep;keep.reserve(capTime);
  for(auto& e:H.timeline){if(extra&&minor(e)){extra--;continue;}keep.push_back(e);}
  if(keep.size()>capTime)keep.erase(keep.begin(),keep.begin()+(keep.size()-capTime));
  H.timeline.swap(keep);}
 // fiches joueurs vides (aucun match, aucun but, aucune saison) : recréées à l'identique au besoin
 // clubs jamais dirigés : les 80 joueurs les plus marquants (panthéon, effectif actuel et saison en cours toujours conservés)
 if(!managed&&H.players.size()>80&&H.clubId>=0&&H.clubId<(int)g_world.teams.size()){
  std::set<int> cur;for(auto& p:g_world.teams[H.clubId].squad)cur.insert(p.id);
  std::vector<MuseumPlayer> keep,rest;for(auto& p:H.players)(p.hall||cur.count(p.pid)||p.observedYear==year?keep:rest).push_back(p);
  std::stable_sort(rest.begin(),rest.end(),[](const MuseumPlayer& a,const MuseumPlayer& b){return a.legendScore()>b.legendScore();});
  for(auto& p:rest){if(keep.size()>=80)break;keep.push_back(p);}
  H.players.swap(keep);}
 H.players.erase(std::remove_if(H.players.begin(),H.players.end(),[](const MuseumPlayer& p){return !p.hall&&!p.apps&&!p.goals&&!p.assists&&!p.clean&&!p.seasons&&!p.captainMatches&&!p.trophies&&!p.awards&&!p.leagueGoals&&!p.cupGoals&&!p.euroGoals&&!p.seenApps&&!p.seenGoals&&!p.seenAssists&&!p.seasonGoals;}),H.players.end());
}
void museumTrimAll(Career& K){for(auto& e:K.clubHistories)museumTrim(e.second,K.year);}
// anciennes sauvegardes : Musée des clubs lointains jamais dirigés retiré (une fois, au chargement)
void museumPruneUntracked(Career& K){int u=K.userTeam;if(u<0||u>=(int)g_world.teams.size())return;int nat=g_world.teams[u].nation;
 for(auto it=K.clubHistories.begin();it!=K.clubHistories.end();){int t=it->first;bool keep=t==u||!it->second.managers.empty();if(!keep&&t>=0&&t<(int)g_world.teams.size()){const Team& T=g_world.teams[t];keep=T.parent>=0?T.parent==u:(T.nation==nat&&teamLevel(t)<=5);}if(keep)++it;else it=K.clubHistories.erase(it);}}
void Career::museumSeasonEnd(bool afterMoves){if(!museumEnabled())return;
 if(!afterMoves){museumState(userTeam);for(auto& entry:clubHistories){auto& H=entry.second;if(H.seasons.empty()||H.seasons.back().year!=year)continue;for(auto& p:g_world.teams[H.clubId].squad)observe(*this,H,p);auto& S=active(*this,H);if(S.league>=0&&S.league<(int)season.comps.size()){auto& C=season.comps[S.league];if(!C.stages.empty())for(int g=0;g<(int)C.stages[0].groups.size();g++){auto table=C.table(0,g);for(int i=0;i<(int)table.size();i++)if(table[i].team==H.clubId){auto& row=table[i];S.position=i+1;S.played=row.p;S.wins=row.w;S.draws=row.d;S.losses=row.l;S.gf=row.gf;S.ga=row.ga;S.points=row.pts;}}}
   if(H.clubId==userTeam)S.budgetEnd=mgr.budget;
  }return;}
 for(int ci=0;ci<(int)season.comps.size();ci++)museumCompetition(ci);
 for(auto& entry:clubHistories){auto& H=entry.second;if(H.seasons.empty()||H.seasons.back().year!=year||H.seasons.back().finished)continue;auto& S=H.seasons.back();S.finished=1;S.newTier=tierOfTeam(H.clubId);label(S.newDivision,teamLevelName(H.clubId));S.promoted=S.tier>=0&&S.newTier>=0&&S.newTier<S.tier;S.relegated=S.tier>=0&&S.newTier>S.tier;
  if(S.promoted||S.relegated)event(H,year,season.now,MH_MOVE,std::string(S.promoted?"Promotion : ":"Relégation : ")+S.division+" -> "+S.newDivision);
  if(S.promoted&&!H.managers.empty()&&!H.managers.back().toYear)H.managers.back().promotions++;
  for(auto& p:H.players)if(p.observedYear==year&&p.seenApps>0){if(p.countedYear!=year){p.countedYear=year;p.seasons++;}record(H,MR_LOYALTY,p.seasons,year,season.now,p.name,{},false,p.pid);if(p.seasonGoals>S.scorerGoals){S.scorerGoals=p.seasonGoals;S.scorerPid=p.pid;label(S.scorer,p.name);}if(p.seenAssists>S.assisterCount){S.assisterCount=p.seenAssists;S.assisterPid=p.pid;label(S.assister,p.name);}}
  MuseumPlayer* best=nullptr;for(auto& v:H.players)if(v.observedYear==year&&v.seenApps>0&&(!best||v.seenGoals*3+v.seenAssists*2+v.seenApps>best->seenGoals*3+best->seenAssists*2+best->seenApps))best=&v;if(best){S.playerPid=best->pid;label(S.bestPlayer,best->name);}
  for(auto& award:seasonAwards)if(award.year==year)for(int i=0;i<6;i++)if(award.valid[i]&&award.winners[i].team==H.clubId)for(auto& p:H.players)if(p.pid==award.winners[i].pid){p.awards++;S.awards++;}
  int inducted=0;for(auto& p:H.players)inducted+=p.hall!=0;for(auto& p:H.players)if(!p.hall&&inducted<12&&(p.legendScore()+personalityLegendBonus(*this,p.pid,H.clubId)>=1500||p.seasons>=12&&p.apps>=150)){p.hall=1;inducted++;event(H,year,season.now,MH_PLAYER,"Entrée au panthéon : "+std::string(p.name));}
  museumTrim(H,year);
  if(S.played){bool historic=false;historic|=record(H,MR_POINTS,S.points,year,season.now,S.competition);historic|=record(H,MR_WINS,S.wins,year,season.now,S.competition);historic|=record(H,MR_SEASON_GF,S.gf,year,season.now,S.competition);historic|=record(H,MR_SEASON_GA,S.ga,year,season.now,S.competition,{},true);historic|=record(H,MR_GD,S.gf-S.ga,year,season.now,S.competition);S.historic=historic||S.promoted||S.trophies>0||S.losses==0;if(S.losses==0)event(H,year,season.now,MH_ALL,"Saison de championnat invaincue");}
 }
}
void Career::museumTransfer(const Player& p,int from,int to,int fee){for(int t:{from,to})if(validClub(*this,t)){auto& H=club(*this,t);if(t==from)observe(*this,H,p);if(fee>0)record(H,t==from?MR_SELL:MR_BUY,fee,year,season.now,p.name);}}
void Career::museumLeave(int t){auto it=clubHistories.find(t);if(it==clubHistories.end()||it->second.managers.empty())return;auto& m=it->second.managers.back();if(!m.toYear){m.toYear=year;label(m.departure,"Changement de club");event(it->second,year,season.now,MH_MANAGER,"Départ : "+std::string(m.name));}}
const char* museumCategoryName(int k){static const char* n[]={"Tous","Trophées","Montées / descentes","Joueurs","Stades","Records","Europe","Direction"};return n[std::clamp(k,0,7)];}
const char* museumRecordName(int k){static const char* n[]={"Apparitions","Buts","Passes décisives","Matchs sans encaisser (G)","Saisons au club","Plus jeune joueur","Plus vieux joueur","Plus jeune buteur","Plus vieux buteur","Plus large victoire (écart)","Plus large défaite (écart)","Buts dans un match","Série sans défaite","Série de victoires","Série sans encaisser","Points en championnat","Victoires en championnat","Buts en championnat","Moins de buts encaissés","Différence de buts","Affluence","Transfert entrant (kEUR)","Transfert sortant (kEUR)","Budget (kEUR)","Valeur d'effectif (kEUR)","Buts d'un joueur / saison"};return n[std::clamp(k,0,MR_COUNT-1)];}
template<class T> static void museumVector(Writer& w,std::vector<T>& v){w.vpod(v);}
template<class T> static void museumVector(Reader& r,std::vector<T>& v){if(r.packed){r.vpod(v);return;}unsigned n=0;r.pod(n);if(!r.ok)return;long pos=ftell(r.f);fseek(r.f,0,SEEK_END);long end=ftell(r.f);fseek(r.f,pos,SEEK_SET);if(n>1000000||pos<0||end<pos||(uint64_t)n*sizeof(T)>(uint64_t)(end-pos)){r.ok=false;return;}v.resize(n);if(n&&fread(v.data(),sizeof(T),n,r.f)!=n)r.ok=false;}
template<class T> static void museumVector(PackW& w,std::vector<T>& v){w.vpod(v);}
template<class T> static void museumVector(PackR& r,std::vector<T>& v){r.vpod(v,1000000);}
static long g_mb[13];static long ioPos(PackW& w){return ftell(w.w.f);}template<class IO>static long ioPos(IO&){return 0;}
#define MV(i,v) {long a_=ioPos(io);museumVector(io,v);g_mb[i]+=ioPos(io)-a_;}
template<class IO> static void historyIO(IO& io,ClubHistory& h){io.pod(h.clubId);io.pod(h.firstYear);io.pod(h.seenYear);io.pod(h.unbeaten);io.pod(h.winStreak);io.pod(h.cleanStreak);io.pod(h.firstFlags);MV(0,h.seasons);MV(1,h.trophies);MV(2,h.finals);MV(3,h.legendaryMatches);MV(4,h.records);MV(5,h.timeline);MV(6,h.players);MV(7,h.managers);MV(8,h.stadiums);MV(9,h.shirts);MV(10,h.seenMatches);MV(11,h.seenEvents);MV(12,h.seenGates);}
#undef MV
void museumProfile(const Career& K,FILE* out){const char* N[13]={"seasons","trophies","finals","legendary","records","timeline","players","managers","stadiums","shirts","seenMatches","seenEvents","seenGates"};size_t B[13]={},C[13]={};for(auto& pair:K.clubHistories){auto& h=pair.second;size_t c[13]={h.seasons.size(),h.trophies.size(),h.finals.size(),h.legendaryMatches.size(),h.records.size(),h.timeline.size(),h.players.size(),h.managers.size(),h.stadiums.size(),h.shirts.size(),h.seenMatches.size(),h.seenEvents.size(),h.seenGates.size()};size_t z[13]={sizeof(MuseumSeason),sizeof(MuseumTrophy),sizeof(MuseumMatch),sizeof(MuseumMatch),sizeof(MuseumRecord),sizeof(MuseumEvent),sizeof(MuseumPlayer),sizeof(MuseumManager),sizeof(MuseumStadium),sizeof(MuseumShirt),8,8,8};for(int k=0;k<13;k++){C[k]+=c[k];B[k]+=c[k]*z[k];}}fprintf(out,"MUSEUM detail (%zu clubs)\n",K.clubHistories.size());for(int k=0;k<13;k++)fprintf(out,"  %-12s %9.2f MB  count %zu\n",N[k],B[k]/1048576.0,C[k]);}
void museumSave(Writer& w,const Career& K){museumTrimAll(const_cast<Career&>(K));if(getenv("FOOT_SAVE_PROFILE"))museumProfile(K,stderr);unsigned n=(unsigned)K.clubHistories.size();w.pod(n);PackW pw{w};memset(g_mb,0,sizeof g_mb);for(auto& pair:K.clubHistories){auto& h=const_cast<ClubHistory&>(pair.second);historyIO(pw,h);}if(getenv("FOOT_SAVE_PROFILE")){const char* N[13]={"seasons","trophies","finals","legendary","records","timeline","players","managers","stadiums","shirts","seenMatches","seenEvents","seenGates"};fprintf(stderr,"MUSEUM packed :");for(int k=0;k<13;k++)fprintf(stderr," %s %.1f",N[k],g_mb[k]/1048576.0);fprintf(stderr,"\n");}}
void museumLoad(Reader& r,Career& K,int ver){K.clubHistories.clear();unsigned n=0;r.pod(n);if(n>g_world.teams.size()){r.ok=false;return;}PackR pr(r);for(unsigned i=0;i<n&&r.ok;i++){ClubHistory h;if(ver>=35)historyIO(pr,h);else historyIO(r,h);if(h.clubId<0||h.clubId>=(int)g_world.teams.size()||K.clubHistories.count(h.clubId)){r.ok=false;return;}for(auto& rec:h.records)if(rec.kind<0||rec.kind>=MR_COUNT)r.ok=false;K.clubHistories.emplace(h.clubId,std::move(h));}}

void Career::museumEnrich(int ci,int mi,const MuseumMatch& actual){if(!museumEnabled()||ci<0||ci>=(int)season.comps.size()||mi<0||mi>=(int)season.comps[ci].matches.size())return;const auto& r=season.comps[ci].matches[mi];for(int t:{r.home,r.away}){auto it=clubHistories.find(t);if(it==clubHistories.end())continue;auto& H=it->second;if(actual.attendance>=0)record(H,MR_ATTENDANCE,actual.attendance,year,season.now,actual.venue,season.comps[ci].name);for(auto* v:{&H.finals,&H.legendaryMatches})for(auto& m:*v)if(m.year==year&&m.comp==ci&&m.match==mi){m.exactLineup=actual.exactLineup;memcpy(m.xi,actual.xi,sizeof m.xi);memcpy(m.lineupNames,actual.lineupNames,sizeof m.lineupNames);memcpy(m.captain,actual.captain,sizeof m.captain);m.attendance=actual.attendance;label(m.venue,actual.venue);}}}
