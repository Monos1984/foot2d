// Coupes d'Europe « nouvelle formule » (depuis 2024-25) : tours de qualification (voie des champions / voie de la ligue),
// phase de ligue à 36 (chapeaux, adversaires imposés), barrages 9e-24e, 8es de finale avec tableau fixe, finale sur terrain neutre.
#include "game.h"
#include <algorithm>
#include <functional>
#include <map>
#include <set>
#include <cstdio>
#include <cstdlib>

// round k de qualification (0..nQ-1), nQ = phase de ligue
void newEuroStart(Competition& C, int k, std::vector<int> pool);

static void dedupe(std::vector<int>& v) {
    std::vector<int> o; std::set<int> s;
    for (int t : v) if (t >= 0 && s.insert(t).second) o.push_back(t);
    v = o;
}

static void seededPairs(std::vector<int> v, std::vector<std::pair<int, int>>& pairs) {
    if (v.size() < 2) { for (int t : v) pairs.push_back({ t, -1 }); return; }
    std::stable_sort(v.begin(), v.end(), [](int a, int b) { return clubSeed(a) > clubSeed(b); });
    if (v.size() % 2) { pairs.push_back({ v[0], -1 }); v.erase(v.begin()); }
    int h = (int)v.size() / 2;
    std::vector<int> top(v.begin(), v.begin() + h), bot(v.begin() + h, v.end());
    // éviter deux clubs du même pays
    for (int attempt = 0; attempt < 60; attempt++) {
        g_rng.shuffle(bot);
        bool ok = true;
        for (int i = 0; i < h && ok; i++) if (g_world.teams[top[i]].nation == g_world.teams[bot[i]].nation) ok = false;
        if (ok) break;
    }
    for (int i = 0; i < h; i++) pairs.push_back({ bot[i], top[i] });   // la tête de série reçoit au retour
}

// ------------------------------------------------------------------ phase de ligue
static bool buildLeaguePhase(Competition& C, std::vector<int> teams, int P, int M, int nQ) {
    int N = (int)teams.size();
    int per = N / P;
    int opp = M / P;          // adversaires par chapeau (2 en C1/C3, 1 en C4)
    std::vector<std::vector<int>> pots(P);
    for (int i = 0; i < N; i++) pots[i / per].push_back(teams[i]);
    auto nat = [](int t) { return g_world.teams[t].nation; };
    std::vector<std::pair<int, int>> edges;
    bool built = false;

    for (int attempt = 0; attempt < 600 && !built; attempt++) {
        bool relax = attempt > 450, relax2 = attempt > 200;
        edges.clear();
        std::map<int, std::map<int, int>> natCnt;
        std::set<std::pair<int, int>> met;
        bool rlLocal = false;
        auto okEdge = [&](int a, int b) {
            if (a == b || met.count({ std::min(a, b), std::max(a, b) })) return false;
            if (!relax && !rlLocal && nat(a) == nat(b)) return false;
            if (!relax2 && (natCnt[a][nat(b)] >= 2 || natCnt[b][nat(a)] >= 2)) return false;
            return true;
        };
        auto addEdge = [&](int h, int a) { edges.push_back({ h, a }); met.insert({ std::min(h, a), std::max(h, a) }); natCnt[h][nat(a)]++; natCnt[a][nat(h)]++; };
        bool ok = true;
        for (int i = 0; i < P && ok; i++) {
            // même chapeau
            auto& p = pots[i];
            bool done = false;
            for (int tr = 0; tr < 260 && !done; tr++) {
                rlLocal = tr >= 200 && attempt > 20;
                std::vector<int> q = p; g_rng.shuffle(q);
                std::vector<std::pair<int, int>> cand;
                if (opp == 2) for (int k = 0; k < per; k++) cand.push_back({ q[k], q[(k + 1) % per] });
                else for (int k = 0; k + 1 < per; k += 2) cand.push_back({ q[k], q[k + 1] });
                bool good = true;
                for (auto& e : cand) if (!okEdge(e.first, e.second)) { good = false; break; }
                if (!good) continue;
                for (auto& e : cand) addEdge(e.first, e.second);
                done = true;
            }
            if (!done) ok = false;
            // chapeaux suivants
            for (int j = i + 1; j < P && ok; j++) {
                auto& A = pots[i]; auto& B = pots[j];
                bool d2 = false;
                for (int tr = 0; tr < 360 && !d2; tr++) {
                    rlLocal = tr >= 300 && attempt > 20;
                    std::vector<int> s1(per), s2(per);
                    for (int k = 0; k < per; k++) s1[k] = s2[k] = k;
                    g_rng.shuffle(s1); g_rng.shuffle(s2);
                    std::vector<std::pair<int, int>> cand;
                    bool good = true;
                    for (int k = 0; k < per; k++) cand.push_back({ A[k], B[s1[k]] });                      // A reçoit
                    if (opp == 2) {
                        for (int k = 0; k < per; k++) if (s1[k] == s2[k]) good = false;
                        for (int k = 0; k < per; k++) cand.push_back({ B[s2[k]], A[k] });                  // B reçoit
                    }
                    if (!good) continue;
                    // vérification incrémentale (compteurs de pays)
                    std::map<int, std::map<int, int>> save = natCnt;
                    auto metSave = met;
                    size_t es = edges.size();
                    for (auto& e : cand) { if (!okEdge(e.first, e.second)) { good = false; break; } addEdge(e.first, e.second); }
                    if (!good) { natCnt = save; met = metSave; edges.resize(es); continue; }
                    d2 = true;
                }
                if (!d2) ok = false;
            }
            rlLocal = false;
        }
        if (!ok) continue;
        // C4 : orientation eulérienne (3 matchs à domicile, 3 à l'extérieur)
        if (opp == 1) {
            std::map<int, std::vector<int>> adj;
            for (int e = 0; e < (int)edges.size(); e++) { adj[edges[e].first].push_back(e); adj[edges[e].second].push_back(e); }
            std::vector<char> usedE(edges.size(), 0);
            std::map<int, size_t> ptr;
            for (int start : teams) {
                std::vector<int> stack = { start };
                while (!stack.empty()) {
                    int v = stack.back();
                    size_t& pi = ptr[v];
                    while (pi < adj[v].size() && usedE[adj[v][pi]]) pi++;
                    if (pi == adj[v].size()) { stack.pop_back(); continue; }
                    int e = adj[v][pi]; usedE[e] = 1;
                    int w = edges[e].first == v ? edges[e].second : edges[e].first;
                    edges[e] = { v, w };
                    stack.push_back(w);
                }
            }
        }
        // répartition en M journées (couplages parfaits successifs)
        std::vector<int> roundOf(edges.size(), -1);
        bool sched = false;
        for (int tr = 0; tr < 200 && !sched; tr++) {
            std::fill(roundOf.begin(), roundOf.end(), -1);
            bool okR = true;
            for (int r = 0; r < M && okR; r++) {
                std::map<int, std::vector<int>> av;
                for (int e = 0; e < (int)edges.size(); e++) if (roundOf[e] < 0) { av[edges[e].first].push_back(e); av[edges[e].second].push_back(e); }
                for (auto& kv : av) g_rng.shuffle(kv.second);
                std::set<int> matched;
                std::vector<int> chosen;
                long budget = 60000;
                std::function<bool()> rec = [&]() -> bool {
                    if ((int)matched.size() == N) return true;
                    if (--budget < 0) return false;
                    int best = -1; int bc = 1 << 30;
                    for (int t : teams) if (!matched.count(t)) {
                        int c = 0; for (int e : av[t]) { int o = edges[e].first == t ? edges[e].second : edges[e].first; if (!matched.count(o)) c++; }
                        if (c < bc) { bc = c; best = t; }
                    }
                    if (best < 0 || bc == 0) return false;
                    for (int e : av[best]) {
                        int o = edges[e].first == best ? edges[e].second : edges[e].first;
                        if (matched.count(o)) continue;
                        matched.insert(best); matched.insert(o); chosen.push_back(e);
                        if (rec()) return true;
                        matched.erase(best); matched.erase(o); chosen.pop_back();
                    }
                    return false;
                };
                if (!rec()) { okR = false; break; }
                for (int e : chosen) roundOf[e] = r;
            }
            if (okR) sched = true;
        }
        if (!sched) continue;
        Stage st; st.type = ST_SWISS; st.name = "Phase de ligue"; st.legs = 1; st.groups.push_back(teams);
        for (int r = 0; r < M; r++) {
            Round R; R.time = nQ + r < (int)C.koTimes.size() ? C.koTimes[nQ + r] : 10 + 2 * r; R.name = fmt("Journée %d", r + 1);
            for (int e = 0; e < (int)edges.size(); e++) if (roundOf[e] == r) {
                MatchRes m; m.home = edges[e].first; m.away = edges[e].second; m.group = 0;
                C.matches.push_back(m); R.m.push_back((int)C.matches.size() - 1);
            }
            st.rounds.push_back(R);
        }
        C.stages.push_back(st);
        C.cur = (int)C.stages.size() - 1;
        built = true;
    }
    return built;
}

static void leaguePhase(Competition& C, std::vector<int> pool) {
    int nQ = C.regionalRounds, M = C.swissRounds, P = std::max(1, C.qualSpots);
    const int N = 36;
    dedupe(pool);
    std::stable_sort(pool.begin(), pool.end(), [&](int a, int b) {
        if ((a == C.host) != (b == C.host)) return a == C.host;         // tenant du titre : chapeau 1
        return clubSeed(a) > clubSeed(b); });
    if ((int)pool.size() > N) pool.resize(N);
    if ((int)pool.size() < N && (int)C.entrants.size() > nQ + 1) {
        std::vector<int> res = C.entrants[nQ + 1];
        std::stable_sort(res.begin(), res.end(), [](int a, int b) { return clubSeed(a) > clubSeed(b); });
        for (int t : res) if ((int)pool.size() < N && std::find(pool.begin(), pool.end(), t) == pool.end()) pool.push_back(t);
        std::stable_sort(pool.begin(), pool.end(), [&](int a, int b) {
            if ((a == C.host) != (b == C.host)) return a == C.host;
            return clubSeed(a) > clubSeed(b); });
    }
    C.koTargets.push_back(nQ);
    if ((int)pool.size() == N && buildLeaguePhase(C, pool, P, M, nQ)) return;
    // secours : appariements libres
    std::vector<double> times;
    for (int r = 0; r < M; r++) times.push_back(nQ + r < (int)C.koTimes.size() ? C.koTimes[nQ + r] : 10 + 2 * r);
    C.setupSwiss(pool, M, times);
    C.cur = (int)C.stages.size() - 1;
}

void newEuroStart(Competition& C, int k, std::vector<int> pool) {
    int nQ = C.regionalRounds;
    while (k <= nQ) {
        bool ext = (C.qualPlayoff >> k) & 1;
        if (ext && !((C.extReadyMask >> k) & 1)) { C.carry = pool; C.awaiting = k; return; }
        if (k < (int)C.entrants.size()) for (int e : C.entrants[k]) pool.push_back(e);
        dedupe(pool);
        if (k == nQ) { leaguePhase(C, pool); return; }
        std::vector<int> cp, lp;
        std::set<int> champs(C.extra2.begin(), C.extra2.end());
        for (int t : pool) (champs.count(t) ? cp : lp).push_back(t);
        std::vector<std::pair<int, int>> pairs;
        if (cp.size() >= 2) seededPairs(cp, pairs); else for (int t : cp) pairs.push_back({ t, -1 });
        if (lp.size() >= 2) seededPairs(lp, pairs); else for (int t : lp) pairs.push_back({ t, -1 });
        bool any = false; for (auto& p : pairs) if (p.second >= 0) any = true;
        if (!any) { C.koTargets.push_back(k); k++; continue; }        // tour sans match : tout le monde passe
        C.addKOStage(pairs, 2, k < (int)C.koTimes.size() ? C.koTimes[k] : 0, k < (int)C.koNames.size() ? C.koNames[k] : "Qualification");
        C.cur = (int)C.stages.size() - 1;
        return;
    }
}

static std::vector<std::pair<int, int>> bracket(const std::vector<int>& w) {
    std::vector<std::pair<int, int>> p;
    for (size_t i = 0; i + 1 < w.size(); i += 2) p.push_back({ w[i], w[i + 1] });
    if (w.size() % 2) p.push_back({ w.back(), -1 });
    return p;
}

void newEuroStageFinished(Competition& C, const std::vector<int>& winners) {
    Stage& st = C.stages[C.cur];
    int nQ = C.regionalRounds, L = C.swissRounds;
    int base = nQ + L;
    auto T = [&](int i) { return base + i < (int)C.koTimes.size() ? C.koTimes[base + i] : 30.0 + 3 * i; };
    int k = -1;
    for (int i = 0; i < nQ && i < (int)C.koNames.size(); i++) if (C.koNames[i] == st.name) k = i;
    if (st.type == ST_KO && k >= 0) { C.koTargets.push_back(k); newEuroStart(C, k + 1, winners); return; }
    if (st.type == ST_SWISS) {
        auto tb = C.table(C.cur, 0);
        C.result.clear(); for (auto& s : tb) C.result.push_back(s.team);
        if (tb.size() < 24) { C.done = true; C.winner = tb.empty() ? -1 : tb[0].team; return; }
        C.extra.clear(); for (int i = 0; i < 8; i++) C.extra.push_back(tb[i].team);
        std::vector<std::pair<int, int>> pairs;
        for (int i = 0; i < 8; i++) pairs.push_back({ tb[23 - i].team, tb[8 + i].team });   // le mieux classé reçoit au retour
        C.addKOStage(pairs, 2, T(0), "Barrages");
        C.cur = (int)C.stages.size() - 1;
        return;
    }
    if (st.name == "Barrages" && C.extra.size() == 8 && winners.size() == 8) {
        // 8es : 1er et 2e contre les qualifiés des barrages 15/16-17/18, ..., 7e et 8e contre ceux de 9/10-23/24 ; tableau fixé
        static const int ORD[8] = { 0, 7, 3, 4, 1, 6, 2, 5 };
        std::vector<std::pair<int, int>> pairs;
        for (int j : ORD) pairs.push_back({ winners[7 - j], C.extra[j] });
        C.addKOStage(pairs, 2, T(1), "8es de finale");
        C.cur = (int)C.stages.size() - 1;
        return;
    }
    if (winners.size() == 1) { C.done = true; C.winner = winners[0]; C.result = winners; return; }
    bool fin = winners.size() == 2;
    double t = winners.size() >= 8 ? T(2) : winners.size() >= 4 ? T(3) : T(4);
    C.addKOStage(bracket(winners), fin ? 1 : 2, t, fin ? std::string("Finale") : koName((int)winners.size()), fin && C.neutralFinal);
    C.cur = (int)C.stages.size() - 1;
}

// perdants d'un tour de qualification (0..nQ-1), vide si le tour n'a pas eu lieu
std::vector<int> newEuroLosers(const Competition& C, int k) {
    std::vector<int> v;
    if (k < 0 || k >= (int)C.koNames.size()) return v;
    for (auto& st : C.stages) if (st.type == ST_KO && st.name == C.koNames[k])
        for (auto& t : st.ties) if (t.b >= 0 && t.winner >= 0) v.push_back(t.winner == t.a ? t.b : t.a);
    return v;
}
bool newEuroRoundDone(const Competition& C, int k) { return std::find(C.koTargets.begin(), C.koTargets.end(), k) != C.koTargets.end(); }
