#include "../src/game.h"
int main(){
    g_world.build();
    int user=-1; for (int i=0;i<(int)g_world.teams.size();i++) if (g_world.teams[i].name=="Stade Lavallois") user=i;
    g_career.newClubCareer(user, 2026);
    auto& K=g_career; auto& S=K.season;
    printf("budget %s income %s objectif %s window %d (%s)\n", money(K.mgr.budget).c_str(), money(K.mgr.incomeBase).c_str(), K.objectiveText().c_str(), K.transferWindow(), K.windowText().c_str());
    // achat d'un joueur de L2
    int src=-1; for (int i=0;i<(int)g_world.teams.size();i++) if (g_world.teams[i].name=="FC Metz") src=i;
    g_world.ensureSquad(src); g_world.ensureSquad(user);
    const Player& p=g_world.teams[src].squad[5];
    std::string err; int pid=p.id; std::string nm=p.name;
    bool ok=K.buyPlayer(pid, std::min<int>(p.value(), (int)K.mgr.budget), err);
    printf("buy %s : %d %s -> team %d squad %zu budget %s\n", nm.c_str(), ok, err.c_str(), g_world.findPlayer(pid), g_world.teams[user].squad.size(), money(K.mgr.budget).c_str());
    int fee=0; int to=K.findBuyer(g_world.teams[user].squad[0].id, fee);
    ok=K.sellPlayer(g_world.teams[user].squad[0].id, to, fee, err);
    printf("sell to %s for %s : %d %s\n", to>=0?g_world.teams[to].name.c_str():"-", money(fee).c_str(), ok, err.c_str());
    // tenant de la coupe forcé : un club de N1
    int p0,q0,g0; K.tierOfTeam(user,&p0,&q0,&g0);
    int n1=-1; for (auto& pl: K.pyramids[p0].pools) if (pl.tier==3 && n1<0) for (int t: pl.clubs) if (g_world.teams[t].parent<0) { n1=t; break; }
    while(true){ auto pm=S.advance(true); if(pm.comp<0)break; K.mgrTick(); }
    K.prevCupWinner["FRA"]=n1;   // sera écrasé par endSeason -> on le remet après
    // on simule la situation : on relance une saison avec ce tenant
    K.endSeason();
    printf("after endSeason budget %s conf %d sacked %d news:\n", money(K.mgr.budget).c_str(), K.mgr.confidence, K.mgr.sacked);
    for (auto& n: S.news) printf("   %s\n", n.c_str());
    K.prevCupWinner["FRA"]=n1; K.startSeason(); K.mgrInit();
    printf("holder direct: %d (%s, tier %d)\n", K.cdfHolderDirect, n1>=0?g_world.teams[n1].name.c_str():"-", K.tierOfTeam(n1));
    bool inReg=false; for (int c: K.cdfRegional) for (auto& e: S.comps[c].entrants) for (int t: e) if (t==n1) inReg=true;
    printf("holder in regional rounds: %d\n", inReg);
    while(true){ auto pm=S.advance(true); if(pm.comp<0)break; }
    const Competition& N=S.comps[K.cdfNational];
    bool in32=false; for (int t: N.entrants[2]) if (t==n1) in32=true;
    printf("holder in 32es entrants: %d  ; stages:", in32); for (auto& st: N.stages) printf(" [%s %zu]", st.name.c_str(), st.ties.size()); printf("\n");
    printf("transfers logged %zu\n", K.mgr.transfers.size());
    if (K.cdl>=0){ const Competition& L=S.comps[K.cdl]; printf("CdL:"); for (auto& st: L.stages) printf(" [%s %zu]", st.name.c_str(), st.ties.size()); printf(" winner %s\n", L.winner>=0?g_world.teams[L.winner].name.c_str():"-"); }
    const Team& U=g_world.teams[user];
    printf("statut %s cap %d fans %d lastAtt %d budget %s gate %s shop %s\n", statusName(U.status), U.sta.capacity(), U.sta.fans, U.sta.lastAtt, money(K.mgr.budget).c_str(), money(K.mgr.seasonGate).c_str(), money(K.mgr.seasonShop).c_str());
    std::string e; Project pj; pj.kind=PJ_SEATS; pj.stand=2; pj.amount=1000; printf("project: %d %s budget %s\n", K.startProject(pj,e), e.c_str(), money(K.mgr.budget).c_str());
    // statuts de quelques niveaux
    int cnt[3][14]={{0}}; for (auto& pl: K.pyramids[p0].pools) for (int t: pl.clubs) cnt[g_world.teams[t].status][pl.tier]++;
    for (int t=0;t<13;t++) printf("tier %d: PRO %d SEMI %d AMA %d\n", t, cnt[0][t], cnt[1][t], cnt[2][t]);
}
