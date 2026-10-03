#include "game.h"
#include <algorithm>
#include <set>
#include <cstring>

bool Career::ballonEnabled() const {
    return (kind==CK_CLUB && !opts.lite && !euroOnly) || (kind==CK_INTL && coach);
}

// Le calendrier du jeu part du premier lundi d'août. La remise est fixée au 28 octobre.
double ballonDate(int year) {
    int y=year;
    int dow=(y+y/4-y/100+y/400+1+1)%7; // Sakamoto, 1er août, dimanche=0
    int monday=(dow+6)%7;
    return (88-(7-monday)%7)/7.0;
}

int Career::ballonPending() const {
    if(!ballonEnabled()) return -1;
    for(int i=0;i<(int)ballonEditions.size();++i) if(!ballonEditions[i].presented) return i;
    return -1;
}

bool Career::ballonTick() {
    if(!ballonEnabled() || season.now<ballonDate(season.year)) return false;
    for(const auto& e:ballonEditions) if(e.year==season.year) return false;
    BallonEdition edition;edition.year=season.year;
    std::vector<BallonNominee> candidates[2];
    std::set<int> seen;
    for(int t=0;t<(int)g_world.teams.size();++t){
        const Team& T=g_world.teams[t];
        if(T.kind!=TK_CLUB || T.freeAgents || T.resLevel || (T.youth && T.youth!=6)) continue;
        // Pas d'identités inventées pour les clubs non générés hors de l'élite.
        if(!T.squadGen && T.rating<60 && t!=userTeam) continue;
        g_world.ensureSquad(t);
        for(const auto& p:g_world.teams[t].squad){
            if(p.id<=0 || p.name.empty() || !seen.insert(p.id).second) continue;
            BallonNominee n;n.team=t;n.pid=p.id;n.rating=p.overall();
            std::snprintf(n.name,sizeof n.name,"%s",p.name.c_str());
            n.goals=p.goals;n.assists=p.assists;n.apps=p.apps;
            for(const auto& h:p.hist) if(h.year==edition.year-1){n.goals+=h.goals;n.assists+=h.assists;n.apps+=h.apps;}
            // Jury simulé : niveau individuel + performances de la dernière saison et de l'automne.
            n.score=n.rating*4+std::min(80,n.goals)*5+std::min(60,n.assists)*3+std::min(80,n.apps)*2;
            candidates[p.gender ? 1 : 0].push_back(n);
        }
    }
    for(int gender=0;gender<2;++gender){
        auto& v=candidates[gender];
        std::stable_sort(v.begin(),v.end(),[](const BallonNominee& a,const BallonNominee& b){
            if(a.score!=b.score)return a.score>b.score;
            if(a.rating!=b.rating)return a.rating>b.rating;
            int c=std::strcmp(a.name,b.name);return c ? c<0 : a.pid<b.pid;
        });
        edition.count[gender]=(uint8_t)std::min(3,(int)v.size());
        for(int i=0;i<edition.count[gender];++i)edition.podium[gender][i]=v[i];
    }
    if(!edition.count[0] && !edition.count[1]) return false;
    ballonEditions.push_back(edition);
    for(int gender=0;gender<2;++gender)if(edition.count[gender]){
        const auto& winner=edition.podium[gender][0];
        season.news.push_back(fmt("Ballon d'or %d%s : %s (%s), lauréat du jury simulé.",edition.year,gender ? " féminin" : "",winner.name,g_world.teams[winner.team].name.c_str()));
    }
    return true;
}
