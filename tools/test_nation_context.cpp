#include "../src/game.h"
#include <cstdlib>
#include <cstring>
#include <set>
static int checks=0;
static void check(bool ok,const char* what){++checks;if(!ok){std::printf("FAIL %s\n",what);std::exit(1);}}
static int club(const char* code){for(const auto& p:g_basePyramids)if(p.country==code)for(const auto& pl:p.pools)if(pl.tier==0&&!pl.clubs.empty())return pl.clubs[0];return -1;}
int main(){
    static_assert(sizeof(Career::Opts)==8,"Preserve legacy save layout");
    g_world.build();
    for(const char* path:{"build/test-legacy-v23.sav","build/test-legacy-v24.sav"}){
        check(g_career.load(path),"load original legacy fixture");
        check(!g_career.opts.disableManagerLife&&!g_career.opts.disableBribes,"old reserved bytes keep both options enabled");
    }
    for(const char* code:{"FRA","GER","BEL","ENG","BRA","F:GER","F:FRA"}){
        int team=club(code);check(team>=0,"country playable");
        g_career.opts=Career::Opts();g_career.newClubCareer(team,2026);
        std::string country=code; if(country.rfind("F:",0)==0)country=country.substr(2);
        check(g_career.selectedCountryCode()==country,"selected country, including women");
        int p=g_career.selectedPyramid();check(p>=0&&g_career.pyramids[p].country==code,"exact pyramid");
        check(!g_career.selectedCountryName().empty(),"country display name");
        int c=g_career.domesticCup();check(c>=0,"domestic cup exists");
        if(country!="FRA"){
            check(g_career.season.comps[c].tag==p,"cup belongs to selected pyramid");
            check(g_career.season.comps[c].name.find("France")==std::string::npos,"not a French regional cup sharing the same tag");
        }
        if(std::string(code)=="GER")check(g_career.season.comps[c].name=="DFB-Pokal","German cup");
        if(std::string(code)=="BEL")check(g_career.season.comps[c].name=="Coupe de Belgique","Belgian cup");
        check(g_career.hasDncg()==(country=="FRA"),"DNCG France only");
        int q,g;g_career.tierOfTeam(team,&p,&q,&g);auto& C=g_career.season.comps[g_career.pyramids[p].pools[q].comps[g]];
        check((g_career.divisionRules(g_career.pyramids[p].pools[q].comps[g]).find("DNCG")!=std::string::npos)==(country=="FRA"),"division rules country context");
        C.result=g_career.pyramids[p].pools[q].groups[g];
        g_career.mgr.noSack=true;g_career.mgr.managerMode=true;g_career.mgr.budget=-1000000000;g_career.mgr.dncg=2;
        std::vector<std::string> news;g_career.mgrEndSeason(news);
        if(country!="FRA"){
            check(g_career.mgr.bankrupt==0&&g_career.mgr.dncg==0&&g_career.mgr.wageCapK==0,"foreign deficit does not trigger French administration");
            for(auto& n:news)check(n.find("DNCG")==std::string::npos,"foreign news without DNCG");
        }else check(g_career.mgr.bankrupt==1,"French severe deficit triggers DNCG");
        std::printf("PASS nation %s / %s\n",code,g_career.season.comps[c].name.c_str());
    }
    g_career.opts=Career::Opts();g_career.newClubCareer(club("GER"),2026);
    g_career.opts.disableManagerLife=1;g_career.opts.disableBribes=1;
    auto cash=g_career.life.cash;auto morale=g_career.life.morale;auto news=g_career.season.news.size();
    g_career.lifeMonth();check(g_career.life.cash==cash&&g_career.life.morale==morale&&g_career.season.news.size()==news,"disabled manager life has no monthly events or money changes");
    std::string err;check(!g_career.bribe(-1,-1,0,err)&&err.find("désactivée")!=std::string::npos,"disabled bribe rejected before accessing match");
    g_career.mgr.dncg=2;g_career.mgr.bankrupt=1;g_career.mgr.wageCapK=1;
    check(g_career.save("build/test-nation.sav"),"save both disabled options");g_career.opts=Career::Opts();
    check(g_career.load("build/test-nation.sav"),"load options");
    check(g_career.opts.disableBribes==1&&g_career.opts.disableManagerLife==1,"options persist");
    check(g_career.mgr.dncg==0&&g_career.mgr.bankrupt==0&&g_career.mgr.wageCapK==0,"loading foreign save clears obsolete DNCG penalties");
    g_career.lifeStart(true,g_world.teams[g_career.userTeam].squad[0].id);check(g_career.managerLifeEnabled(),"manager toggle preserves player career life");g_career.life.isPlayer=0;
    g_career.opts.disableBribes=0;check(!g_career.bribe(-1,-1,0,err),"invalid match safely rejected");
    int comp=-1,mi=-1;for(int c=0;c<(int)g_career.season.comps.size()&&comp<0;c++)for(int m=0;m<(int)g_career.season.comps[c].matches.size();m++){const auto& M=g_career.season.comps[c].matches[m];if(M.home==g_career.userTeam||M.away==g_career.userTeam){comp=c;mi=m;break;}}
    check(comp>=0,"scheduled user match");g_career.mgr.budget=1000000;
    check(g_career.bribe(comp,mi,0,err),"bribe available independently of manager life");
    auto heat=g_career.life.heat;g_career.lifeMonth();check(g_career.life.heat<heat,"corruption enquiry continues without manager private life");
    check(lifeBribeDelta(g_career.season.comps[comp].matches[mi])!=0,"enabled bribe affects only committed match");g_career.opts.disableBribes=1;
    check(lifeBribeDelta(g_career.season.comps[comp].matches[mi])==0,"disabled bribe has no simulation effect");
    for(const char* code:{"GER","BEL"}){
        g_career.opts=Career::Opts();g_career.opts.lite=1;g_career.newClubCareer(club(code),2026);g_career.mgr.noSack=true;g_career.mgr.managerMode=false;
        while(!g_career.season.finished)g_career.season.advance(true);
        int cup=g_career.domesticCup();auto cupName=g_career.season.comps[cup].name;int winner=g_career.season.comps[cup].winner;
        check(winner>=0,"foreign season cup winner");g_career.endSeason();check(g_career.year==2027&&g_career.selectedCountryCode()==code,"foreign next season in same country");
        check(g_career.domesticCup()>=0&&g_career.season.comps[g_career.domesticCup()].name==cupName,"cup preserved next season");
        check(g_career.archive[winner].cdf.depth==1000,"foreign winner cup archive");
        bool archived=false;for(const auto& a:g_career.compArch)if(std::string(a.comp)==cupName&&a.winner==winner)archived=true;check(archived,"foreign cup palmares archived");
        std::printf("PASS full season %s\n",code);
    }
    legendStart(g_career,0,{});check(g_career.year==1930&&g_career.season.comps[0].ptsWin==2,"1930 two points");
    auto& historic=g_career.season.comps[0];for(auto& m:historic.matches){m.played=true;m.hg=2;m.ag=0;}
    for(int g=0;g<(int)historic.stages[0].groups.size();g++)for(auto& row:historic.table(0,g))check(row.pts==row.w*2+row.d,"1930 group standings use 2 points");
    historic.ptsWin=3;check(g_career.save("build/test-1930.sav"),"old erroneous historical save");check(g_career.load("build/test-1930.sav"),"historical save reload");check(g_career.season.comps[0].ptsWin==2,"historical bar restored on load");
    std::printf("PASS nation context: %d checks\n",checks);
}
