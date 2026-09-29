// Déroulement chronologique d'une saison (toutes compétitions confondues)
#include "game.h"

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
        now = best;
        Competition& C = comps[bc];
        Stage& ST = C.stages[bs];
        Round& R = ST.rounds[ST.nextR];
        int userMatch = -1;
        // dernière journée d'un championnat / d'une phase de groupes : les matchs se jouent en même temps ;
        // ceux de la poule du joueur ne sont connus qu'à la fin de son match
        int userGroup = -2;
        bool lastDay = ST.type == ST_LEAGUE && ST.nextR == (int)ST.rounds.size() - 1 && !simulateUser;
        if (lastDay) for (int mi : R.m) { const MatchRes& m = C.matches[mi]; if (!m.played && (isControlled(m.home) || isControlled(m.away))) { userGroup = m.group; break; } }
        for (int mi : R.m) {
            MatchRes& m = C.matches[mi];
            if (m.played) continue;
            if (!simulateUser && (isControlled(m.home) || isControlled(m.away))) { if (userMatch < 0) userMatch = mi; continue; }
            if (userGroup != -2 && m.group == userGroup) continue;
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
            simulateMatch(m, &C);
            recordResult(c, x);
            genMatchEvents(C, x);
        }
    }
}

void Season::recordResult(int c, int mi) {
    MatchRes& m = comps[c].matches[mi];
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
    }
}

// ------------------------------------------------------------------ condition physique et moral
static float recoveryRate(int team) {
    float r = 12.f;   // points par semaine
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
    bool user = g_career.kind == CK_CLUB && team == g_career.userTeam;
    float extra = user ? (g_career.mgr.trainInt == 2 ? 2.f : g_career.mgr.trainInt == 0 ? -2.f : 0.f) : 0.f;
    for (int i = 0; i < (int)T.squad.size(); i++) {
        Player& p = T.squad[i];
        int mo = p.morale;
        if (started[i]) {
            float load = 11.f + (100 - p.stamina) * 0.1f + (float)(g_rng.next() % 5) + extra;
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

float formFactor(int team) {
    const Team& T = g_world.teams[team];
    if (!T.squadGen || T.squad.empty()) return 0;
    auto lu = g_world.pickLineup(team, T.formation);
    float c = 0, m = 0; int n = 0;
    for (int k = 0; k < 11 && k < (int)lu.size(); k++) if (lu[k] >= 0) { c += playerCond(team, T.squad[lu[k]]); m += T.squad[lu[k]].morale; n++; }
    if (!n) return 0;
    c /= n; m /= n;
    return (c - 88.f) * 0.06f + (m - 60.f) * 0.03f;
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
