#include "../src/game.h"
#include <cstdlib>
#include <cstring>
#include <set>
#include <cmath>
static int checks=0;
static void check(bool ok,const char* text){++checks;if(!ok){std::printf("FAIL %s\n",text);std::exit(1);}}
static int team(const char* country){for(auto& p:g_basePyramids)if(p.country==country)for(auto& q:p.pools)if(q.tier==0&&!q.clubs.empty())return q.clubs[0];return -1;}
int main(){
    g_world.build();g_career.opts=Career::Opts();g_career.newClubCareer(team("GER"),2026);
    check(g_career.ballonEnabled(),"German club career eligible");
    check(std::fabs(ballonDate(2026)-86.0/7)<0.00001,"28 October 2026 from first August Monday");
    check(std::fabs(ballonDate(2027)-87.0/7)<0.00001,"28 October 2027 calendar");
    g_career.season.now=ballonDate(2026)-0.001;
    check(!g_career.ballonTick()&&g_career.ballonEditions.empty(),"no award before date");
    // An actual calendar transition stops before playing the next match.
    for(auto& C:g_career.season.comps)C.done=true,C.stages.clear();
    Competition C;C.addGroupStage({{team("GER"),team("BEL")}},1,{ballonDate(2026)+1},"Test");C.kind=1;
    g_career.season.comps={C};g_career.season.controlled={team("GER")};g_career.season.finished=false;
    auto stop=g_career.season.advance(false);
    check(stop.comp<0&&!g_career.season.finished,"ceremony is calendar stop, not season end");
    check(std::fabs(g_career.season.now-ballonDate(2026))<0.00001,"exact ceremony date");
    check(!g_career.season.comps[0].matches[0].played,"next match not simulated before ceremony");
    check(g_career.ballonEditions.size()==1&&g_career.ballonPending()==0,"one pending edition");
    auto edition=g_career.ballonEditions[0];
    for(int gender=0;gender<2;gender++){
        check(edition.count[gender]==3,"male and female podium");std::set<int> ids;
        for(int i=0;i<3;i++){
            const auto& n=edition.podium[gender][i];
            check(n.team>=0&&n.team<(int)g_world.teams.size()&&n.name[0]&&n.pid>0,"valid nominee identity");
            check(ids.insert(n.pid).second,"no duplicate podium player");
            if(i)check(n.score<=edition.podium[gender][i-1].score,"podium follows jury ranking");
            int ix=-1;int t=g_world.findPlayer(n.pid,&ix);check(t>=0&&ix>=0&&g_world.teams[t].squad[ix].gender==gender,"gender category respected");
        }
    }
    check(!g_career.ballonTick()&&g_career.ballonEditions.size()==1,"no duplicate same year");
    check(g_career.save("build/test-ballon.sav"),"save pending ceremony v26");
    g_career.ballonEditions.clear();check(g_career.load("build/test-ballon.sav"),"load pending ceremony");
    check(g_career.ballonPending()==0&&!std::strcmp(g_career.ballonEditions[0].podium[0][0].name,edition.podium[0][0].name),"same winner after load");
    g_career.ballonEditions[0].presented=1;check(g_career.save("build/test-ballon-seen.sav"),"save watched ceremony");
    check(g_career.load("build/test-ballon-seen.sav")&&g_career.ballonPending()<0,"watched ceremony does not replay on load");
    auto next=g_career.season.advance(false);check(next.comp==0&&next.match>=0,"calendar resumes at preserved user match");
    // Next year gets a distinct edition without changing the former podium.
    g_career.season.year=g_career.year=2027;g_career.season.now=ballonDate(2027);
    check(g_career.ballonTick()&&g_career.ballonEditions.size()==2,"annual award next year");
    check(!std::strcmp(g_career.ballonEditions[0].podium[0][0].name,edition.podium[0][0].name),"old edition identity preserved");
    g_career.opts.lite=1;check(!g_career.ballonEnabled(),"league-only mode excluded");g_career.opts.lite=0;g_career.euroOnly=true;check(!g_career.ballonEnabled(),"standalone European mode excluded");g_career.euroOnly=false;
    g_career.kind=CK_INTL;g_career.coach=true;check(g_career.ballonEnabled(),"international coach career eligible");
    g_career.coach=false;check(!g_career.ballonEnabled(),"single international tournament excluded");g_career.kind=CK_CUSTOM;check(!g_career.ballonEnabled(),"custom tournament excluded");
    for(const char* path:{"build/test-legacy-v23.sav","build/test-legacy-v24.sav","build/test-legacy-v25.sav"}){
        check(g_career.load(path),"legacy career loads");check(g_career.ballonEditions.empty(),"legacy career initializes no fictional award history");
    }
    g_career.opts=Career::Opts();g_career.newClubCareer(team("F:GER"),2026);g_career.life.isPlayer=1;
    check(g_career.ballonEnabled(),"female player career eligible");g_career.season.now=ballonDate(2026);check(g_career.ballonTick(),"female player career award");
    g_career.newClubCareer(team("BEL"),2026);check(g_career.ballonEditions.empty(),"new career clears old awards");
    std::printf("PASS Ballon d'or: %d checks\n",checks);
}
