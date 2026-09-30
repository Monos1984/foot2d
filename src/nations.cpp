// Ligue des nations UEFA : ligues A à D (compositions réelles 2026-27), quarts de finale, Final Four,
// barrages de promotion/relégation, classement général (utilisé pour les barrages de l'Euro et de la Coupe du monde)
#include "game.h"
#include <cstring>
#include <cmath>
#include <algorithm>

int addCompPublic(Season& S, Competition c);

static int NI(const char* c) { return g_world.nationIndex(c); }
static bool isUefaNL(const Competition& C) { return C.kind >= 30 && C.kind <= 33 && C.name.find("UEFA") != std::string::npos; }

// compositions officielles de l'édition 2026-27 (tirage du 12 février 2026)
static const char* NL26[4][4][4] = {
    { { "POR", "NOR", "DEN", "WAL" }, { "BEL", "FRA", "TUR", "ITA" }, { "ESP", "CRO", "ENG", "CZE" }, { "GRE", "GER", "NED", "SRB" } },
    { { "SUI", "SCO", "SVN", "MKD" }, { "UKR", "NIR", "HUN", "GEO" }, { "AUT", "KVX", "IRL", "ISR" }, { "SWE", "POL", "BIH", "ROU" } },
    { { "FIN", "ALB", "BLR", "SMR" }, { "ARM", "MNE", "CYP", "LVA" }, { "SVK", "KAZ", "FRO", "MDA" }, { "LUX", "EST", "ISL", "BUL" } },
    { { "MLT", "GIB", "AND", nullptr }, { "LTU", "AZE", "LIE", nullptr }, { nullptr }, { nullptr } },
};

bool nlActive(const Career& K) {
    for (auto& c : K.season.comps) if (c.kind == 30 && isUefaNL(c)) return true;
    return false;
}

static std::vector<int> nlLeagueComps(const Career& K) {
    std::vector<int> v(4, -1);
    for (int i = 0; i < (int)K.season.comps.size(); i++) { auto& c = K.season.comps[i]; if (c.kind == 30 && isUefaNL(c) && c.tag >= 0 && c.tag < 4) v[c.tag] = i; }
    return v;
}

// classement d'une ligue : position dans le groupe, puis points par match, différence, buts marqués
static std::vector<Standing> leagueOrder(const Competition& C) {
    std::vector<Standing> all;
    std::vector<int> pos;
    for (int g = 0; g < (int)C.stages[0].groups.size(); g++) {
        auto tb = C.table(0, g);
        for (int i = 0; i < (int)tb.size(); i++) { all.push_back(tb[i]); pos.push_back(i); }
    }
    std::vector<int> idx(all.size()); for (size_t i = 0; i < idx.size(); i++) idx[i] = (int)i;
    std::stable_sort(idx.begin(), idx.end(), [&](int a, int b) {
        if (pos[a] != pos[b]) return pos[a] < pos[b];
        float pa = all[a].p ? (float)all[a].pts / all[a].p : 0, pb = all[b].p ? (float)all[b].pts / all[b].p : 0;
        if (pa != pb) return pa > pb;
        if (all[a].gd() != all[b].gd()) return all[a].gd() > all[b].gd();
        return all[a].gf > all[b].gf;
    });
    std::vector<Standing> out; for (int i : idx) out.push_back(all[i]);
    return out;
}
static int groupPos(const Competition& C, int team) {
    for (int g = 0; g < (int)C.stages[0].groups.size(); g++) { auto tb = C.table(0, g); for (int i = 0; i < (int)tb.size(); i++) if (tb[i].team == team) return i; }
    return 99;
}

// classement général (ligue A en tête ; en ligue A, le Final Four puis les quarts départagent les premiers)
std::vector<int> nlRanking(const Career& K) {
    std::vector<int> out;
    auto lc = nlLeagueComps(K);
    const Season& S = K.season;
    std::vector<int> top;
    for (auto& c : S.comps) if (c.kind == 31 && isUefaNL(c) && c.done) {
        if (c.winner >= 0) top.push_back(c.winner);
        for (int s = (int)c.stages.size() - 1; s >= 0; s--) for (auto& t : c.stages[s].ties) for (int x : { t.a, t.b }) if (x >= 0 && std::find(top.begin(), top.end(), x) == top.end()) top.push_back(x);
    }
    for (auto& c : S.comps) if (c.kind == 32 && isUefaNL(c) && c.done)
        for (auto& t : c.stages[0].ties) for (int x : { t.winner, t.a, t.b }) if (x >= 0 && std::find(top.begin(), top.end(), x) == top.end()) top.push_back(x);
    out = top;
    for (int l = 0; l < 4; l++) {
        if (lc[l] < 0) continue;
        for (auto& s : leagueOrder(S.comps[lc[l]])) if (std::find(out.begin(), out.end(), s.team) == out.end()) out.push_back(s.team);
    }
    return out;
}

// vainqueurs de groupe, classés (ligue A, puis B, C, D)
std::vector<int> nlGroupWinners(const Career& K) {
    std::vector<int> out;
    auto lc = nlLeagueComps(K);
    auto rk = nlRanking(K);
    for (int l = 0; l < 4; l++) {
        if (lc[l] < 0) continue;
        std::vector<int> w;
        const Competition& C = K.season.comps[lc[l]];
        for (int g = 0; g < (int)C.stages[0].groups.size(); g++) { auto tb = C.table(0, g); if (!tb.empty()) w.push_back(tb[0].team); }
        std::stable_sort(w.begin(), w.end(), [&](int a, int b) { return std::find(rk.begin(), rk.end(), a) < std::find(rk.begin(), rk.end(), b); });
        for (int t : w) out.push_back(t);
    }
    return out;
}

static std::vector<std::vector<int>> seededGroups(std::vector<int> t, int ng, Rng& r) {
    std::stable_sort(t.begin(), t.end(), [](int a, int b) { return g_world.teams[a].rating > g_world.teams[b].rating; });
    std::vector<std::vector<int>> g(ng);
    for (size_t i = 0; i < t.size(); i += ng) {
        std::vector<int> pot(t.begin() + i, t.begin() + std::min(t.size(), i + ng));
        r.shuffle(pot);
        std::vector<int> ord(ng); for (int k = 0; k < ng; k++) ord[k] = k;
        std::stable_sort(ord.begin(), ord.end(), [&](int a, int b) { return g[a].size() < g[b].size(); });
        for (size_t k = 0; k < pot.size(); k++) g[ord[k]].push_back(pot[k]);
    }
    return g;
}

// création des ligues (automne de l'année nlYear)
void nlSetup(Career& K, int nlYear) {
    Season& S = K.season;
    const char* LN[4] = { "A", "B", "C", "D" };
    std::vector<std::vector<std::vector<int>>> L(4);
    Rng r((uint64_t)nlYear * 7919u + 17u);
    if (nlYear == 2026) {
        for (int l = 0; l < 4; l++) for (int g = 0; g < 4; g++) {
            std::vector<int> grp;
            for (int k = 0; k < 4; k++) { const char* c = NL26[l][g][k]; if (!c) break; int n = NI(c); if (n >= 0) grp.push_back(n); }
            if (grp.size() >= 2) L[l].push_back(grp);
        }
    } else {
        std::vector<int> all;
        for (int i = 0; i < NUM_NATIONS; i++) if (nationEligible(i) && NATIONS[i].conf == UEFA) all.push_back(i);
        std::vector<std::vector<int>> mem(4);
        bool have = (int)K.nlLeague.size() == NUM_NATIONS;
        if (have) { int n = 0; for (int t : all) if (K.nlLeague[t] >= 0) n++; have = n >= 40; }
        if (have) {
            std::vector<int> rest;
            for (int t : all) { int l = K.nlLeague[t]; if (l >= 0 && l < 4) mem[l].push_back(t); else rest.push_back(t); }
            for (int t : rest) mem[2].push_back(t);
        } else {
            // à partir de 2028-29 : trois ligues de 18 (la ligue D disparaît)
            std::stable_sort(all.begin(), all.end(), [](int a, int b) { return g_world.teams[a].rating > g_world.teams[b].rating; });
            for (size_t i = 0; i < all.size(); i++) mem[std::min<size_t>(2, i / 18)].push_back(all[i]);
        }
        for (int l = 0; l < 4; l++) {
            int n = (int)mem[l].size();
            if (n < 3) { if (n) for (int t : mem[l]) mem[std::max(0, l - 1)].push_back(t); continue; }
            int ng = n == 16 ? 4 : n % 3 == 0 && n >= 15 ? n / 3 : std::max(1, n / 4);
            L[l] = seededGroups(mem[l], ng, r);
        }
    }
    K.nlLeague.assign(NUM_NATIONS, -1);
    K.nlYear = nlYear;
    for (int l = 0; l < 4; l++) {
        if (L[l].empty()) continue;
        for (auto& g : L[l]) for (int t : g) K.nlLeague[t] = (int8_t)l;
        Competition c;
        c.format = FMT_QUAL_GROUPS; c.kind = 30; c.tag = l; c.legs = 2; c.tb = TB_H2H;
        c.name = fmt("Ligue des nations UEFA %d-%02d - Ligue %s", nlYear, (nlYear + 1) % 100, LN[l]);
        c.shortName = std::string("Ligue des nations ") + LN[l];
        c.addGroupStage(L[l], 2, { 5.0, 6.0, 9.0, 10.0, 14.0, 15.0 }, "Groupes");
        addCompPublic(S, std::move(c));
    }
}

static int makeTies(Season& S, const std::string& name, const std::vector<std::pair<int, int>>& pairs, int legs, std::vector<double> times, int spots, int kind, int tag) {
    Competition c;
    c.format = FMT_KO_ONLY; c.name = name; c.shortName = name; c.kind = kind; c.tag = tag; c.qualSpots = spots;
    c.koLegs = { legs, 1 }; c.koTimes = times; c.neutralFinal = false;
    c.addKOStage(pairs, legs, times[0], kind == 32 ? "Quarts de finale" : "Barrages");
    return addCompPublic(S, std::move(c));
}

static void nlFinalFour(Career& K, std::vector<int> w, const std::string& name) {
    Season& S = K.season;
    if (w.size() < 4) return;
    w.resize(4);
    Competition f;
    f.format = FMT_CUP; f.kind = 31; f.neutralFinal = true;
    f.name = name; f.shortName = "Final Four";
    f.host = w[(size_t)(g_rng.next() % 4)];
    f.entrants = { w };
    f.koTargets = { 2, 1 }; f.koNames = { "Demi-finales", "Finale" }; f.koTimes = { 44.0, 44.6 };
    int idx = addCompPublic(S, std::move(f));
    S.comps[idx].cupRound(0, S.comps[idx].entrants[0]);
    for (auto& m : S.comps[idx].matches) m.neutral = true;
    S.news.push_back(S.comps[idx].name + " : " + g_world.teams[S.comps[idx].host].name + " organise la phase finale.");
}

// fin d'une compétition de la Ligue des nations
void nlOnCompDone(Career& K, int comp) {
    Season& S = K.season;
    Competition& C = S.comps[comp];
    if (C.kind < 30 || C.kind > 33) return;
    bool uefa = isUefaNL(C);
    if (!uefa) {
        // Ligue des nations CONCACAF : Final Four des vainqueurs de groupe de la ligue A (+ meilleur deuxième)
        if (C.kind == 30 && C.tag == 0) {
            std::vector<int> w;
            for (int g = 0; g < (int)C.stages[0].groups.size(); g++) { auto tb = C.table(0, g); if (!tb.empty()) w.push_back(tb[0].team); }
            if (w.size() < 4) {
                auto o = leagueOrder(C);
                for (auto& s : o) if ((int)w.size() < 4 && std::find(w.begin(), w.end(), s.team) == w.end()) w.push_back(s.team);
            }
            nlFinalFour(K, w, "Ligue des nations CONCACAF - Final Four");
        }
        if (C.kind == 31 && C.winner >= 0) S.news.push_back(C.name + " : " + g_world.teams[C.winner].name + " remporte la Ligue des nations !");
        return;
    }
    std::string ed = fmt("Ligue des nations UEFA %d-%02d", K.nlYear, (K.nlYear + 1) % 100);
    if (C.kind == 30) {
        auto lc = nlLeagueComps(K);
        for (int c : lc) if (c >= 0 && !S.comps[c].done) return;
        for (auto& x : S.comps) if ((x.kind == 32 || x.kind == 33) && isUefaNL(x)) return;
        // ---- quarts de finale (ligue A) : premiers contre deuxièmes d'un autre groupe, retour chez le premier
        const Competition& A = S.comps[lc[0]];
        int ng = (int)A.stages[0].groups.size();
        std::vector<int> W, R;
        for (int g = 0; g < ng; g++) { auto tb = A.table(0, g); if (tb.size() > 1) { W.push_back(tb[0].team); R.push_back(tb[1].team); } }
        std::vector<std::pair<int, int>> qf;
        if (W.size() == 4) {
            std::vector<int> perm = { 1, 2, 3, 0 };
            for (int a = 0; a < 20; a++) { g_rng.shuffle(perm); bool ok = true; for (int i = 0; i < 4; i++) if (perm[i] == i) ok = false; if (ok) break; }
            for (int i = 0; i < 4; i++) if (perm[i] == i) std::swap(perm[i], perm[(i + 1) % 4]);
            for (int i = 0; i < 4; i++) qf.push_back({ R[perm[i]], W[i] });
        } else {
            auto o = leagueOrder(A);
            std::vector<int> e; for (auto& s : o) if ((int)e.size() < 8) e.push_back(s.team);
            for (int i = 0; i < (int)e.size() / 2; i++) qf.push_back({ e[e.size() - 1 - i], e[i] });
        }
        if (qf.size() >= 2) makeTies(S, ed + " - Quarts de finale", qf, 2, { 32.0, 33.0 }, (int)qf.size(), 32, 0);
        // ---- promotions / relégations
        std::vector<int8_t> next = K.nlLeague;
        int nL = 0; for (int c : lc) if (c >= 0) nL++;
        bool real26 = K.nlYear == 2026;
        for (int l = 0; l + 1 < 4; l++) {
            if (lc[l] < 0 || lc[l + 1] < 0) continue;
            const Competition& U = S.comps[lc[l]];
            const Competition& D = S.comps[lc[l + 1]];
            auto uo = leagueOrder(U), dO = leagueOrder(D);
            int ngU = (int)U.stages[0].groups.size(), ngD = (int)D.stages[0].groups.size();
            std::vector<std::pair<int, int>> po;
            if (real26 && l == 2) {
                // la ligue D disparaît : ses équipes rejoignent la ligue C ; les premiers de C montent
                for (int g = 0; g < ngD; g++) for (int t : D.stages[0].groups[g]) next[t] = 2;
                for (int g = 0; g < ngU; g++) { auto tb = U.table(0, g); if (!tb.empty()) next[tb[0].team] = 1; }
                continue;
            }
            // montées directes : premiers de groupe de la ligue inférieure
            std::vector<int> upDirect, downDirect, upPO, downPO;
            for (auto& s : dO) if (groupPos(D, s.team) == 0) upDirect.push_back(s.team);
            if (!real26) upDirect.resize(std::min<size_t>(upDirect.size(), 2));
            // deuxièmes (et suivants) de la ligue inférieure : barrages
            for (auto& s : dO) if (groupPos(D, s.team) == 1 && (int)upPO.size() < 4) upPO.push_back(s.team);
            if (!real26) for (auto& s : dO) if ((int)upPO.size() < 4 && groupPos(D, s.team) == 0 && std::find(upDirect.begin(), upDirect.end(), s.team) == upDirect.end()) upPO.insert(upPO.begin(), s.team);
            // ligue supérieure : les derniers
            std::vector<int> rev; for (auto it = uo.rbegin(); it != uo.rend(); ++it) rev.push_back(it->team);
            int nDown = real26 ? (l == 0 ? 2 : 0) : (int)upDirect.size();
            if (real26 && l == 1) nDown = 0;
            for (int i = 0; i < nDown && i < (int)rev.size(); i++) downDirect.push_back(rev[i]);
            if (real26 && l == 1) {
                // ligue B : les quatre derniers de groupe jouent les barrages contre les deuxièmes de C ; les premiers de B montent
                for (int t : rev) if (groupPos(U, t) == (int)U.stages[0].groups[0].size() - 1 && (int)downPO.size() < 4) downPO.push_back(t);
                for (int t : upDirect) next[t] = 1;
                // premiers de B : montée en A (traitée à l'étape l = 0 via dO) ; ici upDirect = premiers de C
            } else {
                for (int t : rev) if ((int)downPO.size() < (int)upPO.size() && std::find(downDirect.begin(), downDirect.end(), t) == downDirect.end()) downPO.push_back(t);
            }
            for (int t : upDirect) next[t] = (int8_t)l;
            for (int t : downDirect) next[t] = (int8_t)(l + 1);
            size_t np = std::min(upPO.size(), downPO.size());
            for (size_t i = 0; i < np; i++) po.push_back({ upPO[i], downPO[np - 1 - i] });   // aller chez l'équipe de la ligue inférieure
            (void)ngU; (void)ngD;
            if (!po.empty()) makeTies(S, ed + fmt(" - Barrages %s/%s", l == 0 ? "A" : l == 1 ? "B" : "C", l == 0 ? "B" : l == 1 ? "C" : "D"), po, 2, { 32.0, 33.0 }, (int)po.size(), 33, l);
        }
        (void)nL;
        K.nlLeague = next;
        return;
    }
    if (C.kind == 32 && C.stages.size()) {
        std::vector<int> w = C.result;
        if (w.size() < 4) for (auto& t : C.stages[0].ties) if (t.winner >= 0 && std::find(w.begin(), w.end(), t.winner) == w.end()) w.push_back(t.winner);
        nlFinalFour(K, w, ed + " - Final Four");
        return;
    }
    if (C.kind == 33) {
        int l = C.tag;
        for (auto& t : C.stages[0].ties) {
            if (t.winner < 0 || t.a < 0 || t.b < 0) continue;
            int loser = t.winner == t.a ? t.b : t.a;
            if ((int)K.nlLeague.size() == NUM_NATIONS) { K.nlLeague[t.winner] = (int8_t)l; K.nlLeague[loser] = (int8_t)(l + 1); }
        }
        return;
    }
    if (C.kind == 31 && C.winner >= 0) S.news.push_back(C.name + " : " + g_world.teams[C.winner].name + " remporte la Ligue des nations !");
}

std::string nlText(const Career& K) {
    return fmt("Ligue des nations UEFA %d-%02d", K.nlYear, (K.nlYear + 1) % 100);
}
