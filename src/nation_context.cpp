#include "game.h"

// Contexte de la nation jouée, partagé par tous les écrans et les règles de gestion.
int Career::selectedPyramid() const {
    int p=-1;
    if (userTeam>=0 && tierOfTeam(userTeam,&p)>=0) return p;
    for (int t:season.controlled) if (tierOfTeam(t,&p)>=0) return p;
    return -1;
}
std::string Career::selectedCountryCode() const {
    int p=selectedPyramid();
    if(p>=0) { auto code=pyramids[p].country; if(code.rfind("F:",0)==0) code=code.substr(2); if(code!="U19"&&code!="U17"&&code!="U15") return code; }
    int t=userTeam; if(t<0 && !season.controlled.empty()) t=season.controlled.front();
    if(t>=0 && t<(int)g_world.teams.size()) { int n=g_world.teams[t].nation; if(n>=0&&n<NUM_NATIONS) return NATIONS[n].code; }
    return "";
}
std::string Career::selectedCountryName() const {
    auto code=selectedCountryCode(); int n=g_world.nationIndex(code.c_str());
    return n>=0 ? sanitize(NATIONS[n].name) : code.empty() ? "Compétitions" : code;
}
int Career::domesticCup(int team) const {
    int p=-1; if(team<0) team=userTeam;
    if(tierOfTeam(team,&p)<0) return -1;
    const auto& P=pyramids[p];
    if(P.country=="FRA") { if(cdfNational>=0) return cdfNational; if(cdf>=0) return cdf; for(int c:cdfRegional) if(c>=0 && c<(int)season.comps.size() && team>=0 && team<(int)g_world.teams.size() && season.comps[c].tag==g_world.teams[team].region) return c; return -1; }
    for(int c:nationalCups) if(c>=0 && c<(int)season.comps.size() && season.comps[c].tag==p) return c;
    for(int c=0;c<(int)season.comps.size();++c) {
        const auto& C=season.comps[c];
        if((C.kind==41 || C.kind==44) && C.tag==p) return c;
    }
    return -1;
}
bool Career::hasDncg() const { return kind==CK_CLUB && selectedCountryCode()=="FRA"; }


