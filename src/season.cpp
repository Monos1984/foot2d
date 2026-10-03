// Déroulement chronologique d'une saison (toutes compétitions confondues)
#include "game.h"
#include "crashlog.h"

void onCompetitionDoneHook(int comp); // career.cpp
void onStageDoneHook(int comp, int stage);

static double compNextTime(const Competition& c, int& stage) {
    double best = 1e18; stage = -1;
    if (c.done && c.stages.empty()) return best;
    for (int s = 0; s < (int)c.stages.size(); s++) {
        const Stage& st = c.stages[s];
        if (st.finished || st.nextR >= (int)st.rounds.size()) continue;
        double t = st.rounds[st.nextR].time;
        if (t < best) { best = t; stage = s; }
    }
    return best;
}

PendingMatch Season::peekNext() const {
    PendingMatch pm;
    double best = 1e18; int bc = -1, bs = -1;
    for (int c = 0; c < (int)comps.size(); c++) {
        int s; double t = compNextTime(comps[c], s);
        if (s >= 0 && t < best) { best = t; bc = c; bs = s; }
    }
    if (bc < 0) return pm;
    (void)best;
    const Round& R = comps[bc].stages[bs].rounds[comps[bc].stages[bs].nextR];
    for (int mi : R.m) {
        const MatchRes& m = comps[bc].matches[mi];
        if (!m.played && (isControlled(m.home) || isControlled(m.away))) { pm.comp = bc; pm.match = mi; return pm; }
    }
    return pm;
}

PendingMatch Season::advance(bool simulateUser) {
    PendingMatch pm;
    for (int guard = 0; guard < 2000000; guard++) {
        double best = 1e18; int bc = -1, bs = -1;
        for (int c = 0; c < (int)comps.size(); c++) {
            int s; double t = compNextTime(comps[c], s);
            if (s >= 0 && t < best) { best = t; bc = c; bs = s; }
        }
        if (bc < 0) { finished = true; return pm; }
        if(this==&g_career.season && g_career.ballonEnabled()) {
            if(!simulateUser && now<ballonDate(year) && best>=ballonDate(year)) {
                bool exists=false;for(const auto& e:g_career.ballonEditions)if(e.year==year)exists=true;
                if(!exists){now=ballonDate(year);if(g_career.ballonTick())return pm;}
            }
        }
        now = best;
        if(this==&g_career.season)g_career.ballonTick();
        Competition& C = comps[bc];
        Stage& ST = C.stages[bs];
        Round& R = ST.rounds[ST.nextR];
        int userMatch = -1;
        // calendrier réel : les matchs programmés avant celui du joueur sont joués avant ; ceux joués en même temps
        // (même heure) ou plus tard ne sont connus qu'à la fin de son match (fin de journée)
        int userSlot = -1;
        if (!simulateUser) for (int mi : R.m) { const MatchRes& m = C.matches[mi]; if (!m.played && (isControlled(m.home) || isControlled(m.away))) { userSlot = kickoffMinutes(bc, mi); break; } }
        for (int mi : R.m) {
            MatchRes& m = C.matches[mi];
            if (m.played) continue;
            if (!simulateUser && (isControlled(m.home) || isControlled(m.away))) { if (userMatch < 0) userMatch = mi; continue; }
            if (userSlot >= 0 && kickoffMinutes(bc, mi) >= userSlot) continue;
            if(this==&g_career.season)g_career.supportersPrepareMatch(bc,mi);
            simulateMatch(m, &C);
            recordResult(bc, mi);
            genMatchEvents(C, mi);
        }
        if (userMatch >= 0) { pm.comp = bc; pm.match = userMatch; return pm; }
        checkRound(bc);
    }
    return pm;
}

// fin du match du joueur : on joue les autres matchs de la même journée restés en attente (matchs simultanés)
void Season::finishRoundOthers(int c, int mi) {
    if (c < 0 || c >= (int)comps.size()) return;
    Competition& C = comps[c];
    int st = C.stageOfMatch(mi);
    if (st < 0) return;
    for (auto& R : C.stages[st].rounds) {
        if (std::find(R.m.begin(), R.m.end(), mi) == R.m.end()) continue;
        for (int x : R.m) {
            MatchRes& m = C.matches[x];
            if (m.played || x == mi || isControlled(m.home) || isControlled(m.away)) continue;
            if(this==&g_career.season)g_career.supportersPrepareMatch(c,x);
            simulateMatch(m, &C);
            recordResult(c, x);
            genMatchEvents(C, x);
        }
    }
}

void Season::recordResult(int c, int mi, bool physical) {
    MatchRes& m = comps[c].matches[mi];
    if(this==&g_career.season){g_career.museumMatch(c,mi);g_career.supportersOnMatchEnd(c,mi);g_career.personalityMatchEnd(c,mi);if(g_career.sportingMode())g_career.sportingMatch(c,mi);}
    // journal : résultats de la réserve et des équipes de jeunes du club du joueur
    if (g_career.kind == CK_CLUB && m.played && m.home >= 0 && m.away >= 0) {
        int u = g_career.userTeam;
        for (int side = 0; side < 2; side++) {
            int t = side ? m.away : m.home;
            const Team& T = g_world.teams[t];
            if (T.parent != u || u < 0) continue;
            const char* lab = T.youth == 1 ? "U19" : T.youth == 2 ? "U17" : T.youth == 3 ? "U15" : "Réserve";
            int gf = side ? m.ag : m.hg, ga = side ? m.hg : m.ag;
            std::string res = gf > ga ? "victoire" : gf < ga ? "défaite" : "nul";
            if (m.ph >= 0) res = ((side ? m.pa : m.ph) > (side ? m.ph : m.pa)) ? "qualifié aux tirs au but" : "éliminé aux tirs au but";
            news.push_back(fmt("[%s] %s : %s %d - %d %s (%s)", lab, comps[c].shortName.empty() ? comps[c].name.c_str() : comps[c].shortName.c_str(),
                g_world.teams[m.home].name.c_str(), m.hg, m.ag, g_world.teams[m.away].name.c_str(), res.c_str()));
            break;
        }
    }
    for (int side = 0; side < 2; side++) {
        int t = side ? m.away : m.home;
        Team& T = g_world.teams[t];
        if (!T.squadGen) continue;
        for (auto& p : T.squad) {
            if (p.suspended > 0) p.suspended--;
            if (p.injured > 0) p.injured--;
        }
        int gf = side ? m.ag : m.hg, ga = side ? m.hg : m.ag;
        int res = gf > ga ? 1 : gf < ga ? -1 : 0;
        if (res == 0 && m.ph >= 0) res = ((side ? m.pa : m.ph) > (side ? m.ph : m.pa)) ? 1 : -1;
        applyMatchLoad(t, res);
        if(careerRules()&&!physical){auto lu=g_world.pickLineup(t,T.formation,RULESET_CAREER);int minutes=comps[c].kind==150?40:m.aet?120:90;for(int s=0;s<11&&s<(int)lu.size();s++)if(lu[s]>=0)learnPosition(T.squad[lu[s]],teamSlotPosition(T,T.formation,s),minutes);}
    }
}

// ------------------------------------------------------------------ condition physique et moral
static bool olympicSchedule() { return g_career.kind == CK_INTL && (g_career.intlType == IT_OLYMPICS || g_career.intlType == IT_OLY_W); }
static float recoveryRate(int team) {
    float r = olympicSchedule() ? 9.f : 12.f;   // points par semaine (JO : chaleur, voyages, un match tous les 3 jours)
    if (g_career.kind == CK_CLUB && team == g_career.userTeam) {
        r += 1.5f * g_career.staffLevel(SR_PHYSIO_PREP);
        r += g_career.mgr.trainInt == 0 ? 4.f : g_career.mgr.trainInt == 2 ? -4.f : 0.f;
    }
    return r;
}

int playerCond(int team, const Player& p) {
    const Team& T = g_world.teams[team];
    if (T.condT < 0) return p.cond;
    double dt = std::max(0.0, g_career.season.now - T.condT);
    return std::min(100, (int)(p.cond + dt * recoveryRate(team)));
}

void applyMatchLoad(int team, int result) {
    Team& T = g_world.teams[team];
    if (!T.squadGen || T.squad.empty()) return;
    double now = g_career.season.now;
    for (auto& p : T.squad) p.cond = (uint8_t)playerCond(team, p);
    T.condT = (float)now;
    auto lu = g_world.pickLineup(team, T.formation);
    std::vector<char> started(T.squad.size(), 0);
    for (int k = 0; k < 11 && k < (int)lu.size(); k++) if (lu[k] >= 0) started[lu[k]] = 1;
    bool user = careerRules() && g_career.kind == CK_CLUB && team == g_career.userTeam;
    float extra = user ? (g_career.mgr.trainInt == 2 ? 2.f : g_career.mgr.trainInt == 0 ? -2.f : 0.f) : 0.f;
    for (int i = 0; i < (int)T.squad.size(); i++) {
        Player& p = T.squad[i];
        int mo = p.morale;
        if (started[i]) {
            float load = 11.f + (100 - p.stamina) * 0.1f + (float)(g_rng.next() % 5) + extra;
            if (olympicSchedule()) load *= 1.3f;            // fatigue renforcée aux JO
            if (p.pos == POS_GK) load *= 0.45f;
            p.cond = (uint8_t)std::max(25, (int)(p.cond - load));
            mo += result > 0 ? 6 : result < 0 ? -5 : 1;
        } else {
            mo += result > 0 ? 1 : 0;
            if (p.pos != POS_GK) mo -= 1;
        }
        mo += (60 - mo) / 10;                       // retour progressif vers la normale
        p.morale = (uint8_t)std::max(10, std::min(100, mo));
    }
}

float formFactor(int team, RuleProfile rules) {
    const Team& T = g_world.teams[team];
    if (!T.squadGen || T.squad.empty()) return 0;
    auto lu = g_world.pickLineup(team, T.formation);
    float c = 0, m = 0; int n = 0;
    for (int k = 0; k < 11 && k < (int)lu.size(); k++) if (lu[k] >= 0) { c += playerCond(team, T.squad[lu[k]]); m += T.squad[lu[k]].morale; n++; }
    if (!n) return 0;
    c /= n; m /= n;
    float tactical=0;
    if(rules==RULESET_CAREER){auto detail=g_world.pickLineup(team,T.formation,RULESET_CAREER);for(int s=0;s<11&&s<(int)detail.size();s++)if(detail[s]>=0){int dp=teamSlotPosition(T,T.formation,s);auto tactic=T.tactical.customized?T.tactical.slot[s]:defaultSlotTactic(dp,T.seed%2,s);const auto& p=T.squad[detail[s]];tactical+=(positionFamiliarity(p,dp)-75)*.025f+(roleAptitude(p,dp,tactic.role)-p.overall())*.025f;}tactical/=11;}
    return (c - 88.f) * 0.06f + (rules==RULESET_CAREER?(m - 60.f) * 0.03f+tactical:0.f);
}

void Season::checkRound(int c) {
    Competition& C = comps[c];
    bool changed = true;
    while (changed) {
        changed = false;
        for (int s = 0; s < (int)C.stages.size(); s++) {
            Stage& st = C.stages[s];
            if (st.finished) continue;
            while (st.nextR < (int)st.rounds.size()) {
                Round& R = st.rounds[st.nextR];
                bool all = true;
                for (int mi : R.m) if (!C.matches[mi].played) { all = false; break; }
                if (!all) break;
                R.done = true; st.nextR++;
            }
            if (st.nextR >= (int)st.rounds.size() && !st.finished) {
                if (s == C.cur) {
                    bool wasDone = C.done;
                    C.onStageFinished();
                    onStageDoneHook(c, s);
                    if (C.done && !wasDone) { onCompetitionDoneHook(c); return; }
                } else st.finished = true;
                changed = true;
                break; // les vecteurs ont pu être modifiés
            }
        }
    }
}
