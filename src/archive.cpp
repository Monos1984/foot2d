// Archives des clubs : meilleurs parcours en coupes, meilleur classement, plus larges victoires, meilleurs buteurs et passeurs
#include "game.h"
#include "serial.h"
#include <cstring>
#include <set>
#include <algorithm>

static void setLabel(char* dst, size_t n, const std::string& s) { snprintf(dst, n, "%s", s.c_str()); }

// profondeur atteinte par une équipe dans une coupe (index de la dernière phase jouée ; 1000 = vainqueur)
static bool reach(const Competition& C, int team, int& depth, std::string& label) {
    depth = -1;
    for (int s = 0; s < (int)C.stages.size(); s++) {
        const Stage& st = C.stages[s];
        bool in = false;
        for (auto& g : st.groups) for (int t : g) if (t == team) in = true;
        for (auto& t : st.ties) if (t.a == team || t.b == team) in = true;
        if (in) { depth = s; label = st.name; }
    }
    if (depth < 0) return false;
    if (C.winner == team) { depth = 1000; label = "Vainqueur"; }
    else if (!C.stages.empty() && depth == (int)C.stages.size() - 1 && C.stages.back().ties.size() == 1) label = "Finaliste";
    return true;
}

static void better(ArchRun& r, int depth, const std::string& label, int year) {
    if (depth > r.depth) { r.depth = (int16_t)depth; r.year = (int16_t)year; setLabel(r.label, sizeof r.label, label); }
}

void Career::archiveSeason() {
    if (kind != CK_CLUB) return;
    Season& S = season;
    int yr = year + 1;
    auto cupRuns = [&](int comp, int offset, ArchRun ClubArch::*field) {
        if (comp < 0 || comp >= (int)S.comps.size()) return;
        const Competition& C = S.comps[comp];
        std::set<int> teams;
        for (auto& st : C.stages) { for (auto& g : st.groups) for (int t : g) teams.insert(t); for (auto& t : st.ties) { if (t.a >= 0) teams.insert(t.a); if (t.b >= 0) teams.insert(t.b); } }
        for (int t : teams) {
            int d; std::string l;
            if (!reach(C, t, d, l)) continue;
            better(archive[t].*field, d == 1000 ? 1000 : d + offset, l, yr);
        }
    };
    for (int c : cdfRegional) cupRuns(c, 0, &ClubArch::cdf);
    cupRuns(cdfNational >= 0 ? cdfNational : cdf, 100, &ClubArch::cdf);
    cupRuns(cdl, 0, &ClubArch::cdl);
    for (int c : regionalCups) cupRuns(c, 0, &ClubArch::reg);
    for (int c : deptCups) cupRuns(c, 0, &ClubArch::dist);
    cupRuns(ucl, 0, &ClubArch::ucl);
    cupRuns(uel, 0, &ClubArch::uefa);
    cupRuns(superRegions, 0, &ClubArch::regSuper);
    // championnats : meilleur classement (niveau le plus haut, puis place)
    for (auto& P : pyramids) {
        for (auto& pl : P.pools) for (int ci : pl.comps) {
            if (ci < 0 || ci >= (int)S.comps.size()) continue;
            const Competition& C = S.comps[ci];
            for (int i = 0; i < (int)C.result.size(); i++) {
                int t = C.result[i];
                ClubArch& A = archive[t];
                A.seasons++;
                if (i == 0) A.titles++;
                int tier = P.dom >= 0 ? pl.tier + 5 : pl.tier;
                if (tier < A.lgTier || (tier == A.lgTier && i + 1 < A.lgPos)) {
                    A.lgTier = (int16_t)tier; A.lgPos = (int16_t)(i + 1); A.lgYear = (int16_t)yr;
                    setLabel(A.lgName, sizeof A.lgName, C.name.substr(0, C.name.find(" - ")));
                }
            }
        }
    }
    // plus larges victoires / défaites
    for (auto& C : S.comps) {
        if (C.kind == 12) continue;
        for (auto& m : C.matches) {
            if (!m.played || m.home < 0 || m.away < 0) continue;
            for (int side = 0; side < 2; side++) {
                int t = side ? m.away : m.home, o = side ? m.home : m.away;
                int gf = side ? m.ag : m.hg, ga = side ? m.hg : m.ag;
                if (gf == ga) continue;
                auto it = archive.find(t);
                if (it == archive.end() && g_world.teams[t].nation >= 0 && strcmp(NATIONS[g_world.teams[t].nation].code, "FRA")) continue;
                ClubArch& A = archive[t];
                if (gf > ga && (gf - ga > A.bigWinGF - A.bigWinGA || A.bigWinGF < 0 || (gf - ga == A.bigWinGF - A.bigWinGA && gf > A.bigWinGF))) { A.bigWinGF = (int16_t)gf; A.bigWinGA = (int16_t)ga; A.bigWinYear = (int16_t)yr; A.bigWinOpp = o; }
                if (ga > gf && (ga - gf > A.bigLossGA - A.bigLossGF || A.bigLossGA < 0)) { A.bigLossGF = (int16_t)gf; A.bigLossGA = (int16_t)ga; A.bigLossYear = (int16_t)yr; A.bigLossOpp = o; }
            }
        }
    }
    // buteurs et passeurs de l'histoire du club (clubs dont l'effectif est suivi)
    for (int ti = 0; ti < (int)g_world.teams.size(); ti++) {
        Team& t = g_world.teams[ti];
        if (!t.squadGen || t.kind != TK_CLUB) continue;
        bool any = false; for (auto& p : t.squad) if (p.goals || p.assists) { any = true; break; }
        if (!any) continue;
        auto& v = archScorers[ti];
        for (auto& p : t.squad) {
            if (!p.goals && !p.assists && !p.apps) continue;
            ArchScorer* e = nullptr;
            for (auto& x : v) if (x.pid == p.id) e = &x;
            if (!e) { ArchScorer n; n.pid = p.id; setLabel(n.name, sizeof n.name, p.name); v.push_back(n); e = &v.back(); }
            e->goals += p.goals; e->assists += p.assists; e->apps += p.apps;
        }
        // on garde les 40 plus importants
        std::sort(v.begin(), v.end(), [](const ArchScorer& a, const ArchScorer& b) { return a.goals * 3 + a.assists * 2 + a.apps / 10 > b.goals * 3 + b.assists * 2 + b.apps / 10; });
        if (v.size() > 40) v.resize(40);
    }
}

void archiveSave(Writer& w, const Career& K) {
    unsigned n = (unsigned)K.archive.size(); w.pod(n);
    for (auto& kv : K.archive) { w.pod(kv.first); w.pod(kv.second); }
    n = (unsigned)K.archScorers.size(); w.pod(n);
    for (auto& kv : K.archScorers) { w.pod(kv.first); w.vpod(kv.second); }
}
void archiveLoad(Reader& r, Career& K) {
    K.archive.clear(); K.archScorers.clear();
    unsigned n = 0; r.pod(n);
    for (unsigned i = 0; i < n && r.ok && i < 2000000; i++) { int k = 0; ClubArch a; r.pod(k); r.pod(a); K.archive[k] = a; }
    n = 0; r.pod(n);
    for (unsigned i = 0; i < n && r.ok && i < 2000000; i++) { int k = 0; std::vector<ArchScorer> v; r.pod(k); r.vpod(v); K.archScorers[k] = v; }
}
