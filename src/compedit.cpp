// Éditeur de compétitions de carrière : divisions (règles, clubs, ajout / suppression), coupes nationales.
// Les modifications des championnats servent de base aux nouvelles carrières (fichier competitions_perso.txt) ;
// en cours de carrière, seules les règles sont modifiables (appliquées à la saison suivante).
#include "game.h"
#include <map>
#include <set>
#include <cstdio>
#include <algorithm>

void formGroups(Pyramid& P);   // pyramid.cpp

static const char* COMP_FILE = "competitions_perso.txt";
static std::map<std::string, CupRule> g_cupRules;
static std::set<std::string> g_editedPyr;
static std::map<std::string, Pyramid> g_origPyr;       // pyramides d'origine (avant modifications)

CupRule cupRuleFor(const std::string& country) { auto it = g_cupRules.find(country); return it == g_cupRules.end() ? CupRule() : it->second; }
void setCupRule(const std::string& country, const CupRule& r) { g_cupRules[country] = r; }
bool pyrEdited(const std::string& country) { return g_editedPyr.count(country) > 0; }
void markPyrEdited(const std::string& country) { g_editedPyr.insert(country); }

bool pyrEditable(const Pyramid& P) {
    return P.dom < 0 && P.country != "U19" && P.country != "U17" && P.country != "U15" && !P.tiers.empty();
}
// ajout / suppression de divisions : pyramides entièrement nationales (pas la France, dont les coupes dépendent des niveaux)
bool pyrStructEditable(const Pyramid& P) {
    if (!pyrEditable(P) || P.country == "FRA" || P.country == "F:FRA") return false;
    for (auto& t : P.tiers) if (t.scope != SC_NATIONAL) return false;
    for (int k = 0; k < (int)P.tiers.size(); k++) { int n = 0; for (auto& pl : P.pools) if (pl.tier == k) n++; if (n != 1) return false; }
    return true;
}
// clubs déplaçables : division nationale formée d'un seul ensemble de clubs
bool tierClubsEditable(const Pyramid& P, int tier) {
    if (!pyrEditable(P) || tier < 0 || tier >= (int)P.tiers.size() || P.tiers[tier].scope != SC_NATIONAL) return false;
    int n = 0; for (auto& pl : P.pools) if (pl.tier == tier) n++;
    return n == 1;
}
static int poolOfTier(Pyramid& P, int tier) { for (int i = 0; i < (int)P.pools.size(); i++) if (P.pools[i].tier == tier) return i; return -1; }

static void refreshPools(Pyramid& P) {
    for (auto& pl : P.pools) {
        if (pl.nGroups <= 1) pl.size = std::max(2, (int)pl.clubs.size());
        if (P.tiers[pl.tier].scope == SC_NATIONAL && pl.nGroups <= 1) P.tiers[pl.tier].groupSize = (int)pl.clubs.size();
        for (int c : pl.clubs) if (c >= 0 && c < (int)g_world.teams.size()) g_world.teams[c].lastTier = pl.tier;
    }
    formGroups(P);
}

// cohérence montées / relégations : promus d'une division = relégués de la division supérieure
// (avec 2 barragistes, le vainqueur des barrages occupe la dernière place de promu)
static int upFrom(const TierConf& above, const TierConf& t) { return std::max(0, above.down - (t.barrageUp == 2 && above.down > 1 ? 1 : 0)); }
static int downFrom(const TierConf& t, const TierConf& below) { return below.up + (below.barrageUp == 2 && below.up > 0 ? 1 : 0); }
static bool frLike(const Pyramid& P) { return P.country == "FRA" || P.country == "F:FRA"; }
void pyrRelink(Pyramid& P, int changedTier, bool fromDown) {
    int T = (int)P.tiers.size();
    if (T == 0) return;
    P.tiers[0].up = 0; P.tiers[0].barrageUp = 0;
    P.tiers[T - 1].down = 0;
    if (frLike(P)) return;       // France : règles réelles laissées au joueur
    if (changedTier >= 0 && changedTier < T) {
        if (fromDown && changedTier + 1 < T) P.tiers[changedTier + 1].up = upFrom(P.tiers[changedTier], P.tiers[changedTier + 1]);
        if (!fromDown && changedTier > 0) P.tiers[changedTier - 1].down = downFrom(P.tiers[changedTier - 1], P.tiers[changedTier]);
    }
}

void pyrMoveClub(Pyramid& P, int club, int toTier) {
    int dst = poolOfTier(P, toTier);
    if (dst < 0) return;
    for (auto& pl : P.pools) pl.clubs.erase(std::remove(pl.clubs.begin(), pl.clubs.end(), club), pl.clubs.end());
    P.pools[dst].clubs.push_back(club);
    refreshPools(P);
}

void pyrAddTier(Pyramid& P, int after, const std::string& name) {
    int at = std::max(0, std::min((int)P.tiers.size(), after + 1));
    TierConf t; t.name = name; t.scope = SC_NATIONAL; t.groupsPerPool = 1; t.groupSize = 0; t.tb = P.tiers.empty() ? TB_GD : P.tiers.back().tb;
    P.tiers.insert(P.tiers.begin() + at, t);
    for (auto& pl : P.pools) if (pl.tier >= at) pl.tier++;
    Pool p; p.tier = at; p.key = 0; p.nGroups = 1; p.size = 2;
    P.pools.push_back(p);
    std::stable_sort(P.pools.begin(), P.pools.end(), [](const Pool& a, const Pool& b) { return a.tier < b.tier; });
    int T = (int)P.tiers.size();
    if (!frLike(P)) {
        if (at > 0) { if (P.tiers[at - 1].down == 0) P.tiers[at - 1].down = 2; P.tiers[at].up = upFrom(P.tiers[at - 1], P.tiers[at]); }
        if (at + 1 < T) { if (P.tiers[at + 1].up == 0) P.tiers[at + 1].up = 2; P.tiers[at].down = downFrom(P.tiers[at], P.tiers[at + 1]); }
    }
    pyrRelink(P, -1, true);
    refreshPools(P);
}

bool pyrDeleteTier(Pyramid& P, int tier) {
    int T = (int)P.tiers.size();
    if (T <= 1 || tier < 0 || tier >= T) return false;
    int into = tier + 1 < T ? tier + 1 : tier - 1;      // les clubs rejoignent la division inférieure (ou supérieure pour la dernière)
    int src = poolOfTier(P, tier), dst = poolOfTier(P, into);
    if (src < 0 || dst < 0) return false;
    for (int c : P.pools[src].clubs) P.pools[dst].clubs.push_back(c);
    P.pools.erase(P.pools.begin() + src);
    P.tiers.erase(P.tiers.begin() + tier);
    for (auto& pl : P.pools) if (pl.tier > tier) pl.tier--;
    if (!frLike(P) && tier > 0 && tier < (int)P.tiers.size()) P.tiers[tier].up = upFrom(P.tiers[tier - 1], P.tiers[tier]);
    pyrRelink(P, -1, true);
    refreshPools(P);
    return true;
}

bool pyrRestore(Pyramid& P) {
    auto it = g_origPyr.find(P.country);
    if (it == g_origPyr.end()) return false;
    P = it->second;
    g_editedPyr.erase(P.country);
    refreshPools(P);
    return true;
}

// ------------------------------------------------------------------ fichier
static std::vector<std::string> splitC(const std::string& s, char sep) {
    std::vector<std::string> v; std::string cur;
    for (char c : s) { if (c == sep) { v.push_back(cur); cur.clear(); } else cur += c; }
    v.push_back(cur);
    return v;
}
static std::string cleanC(const std::string& s) { std::string o; for (char c : s) o += (c == '|' || c == ';' || c == '\n' || c == '\r') ? ' ' : c; return o; }

void saveCompEdits() {
    FILE* f = fopen(COMP_FILE, "w");
    if (!f) return;
    fprintf(f, "# Compétitions de carrière modifiées (Super Soccer World)\n");
    for (auto& P : g_basePyramids) {
        if (!g_editedPyr.count(P.country)) continue;
        fprintf(f, "PYR|%s|%d\n", P.country.c_str(), (int)P.tiers.size());
        for (int k = 0; k < (int)P.tiers.size(); k++) {
            const TierConf& t = P.tiers[k];
            fprintf(f, "TIER|%d|%s|%d|%d|%d|%d|%d|%d|%d\n", k, cleanC(t.name).c_str(), t.up, t.down, t.barrageUp, t.tb, t.ptsWin, t.legs, t.noReserves ? 1 : 0);
            if (!tierClubsEditable(P, k)) continue;
            for (auto& pl : P.pools) if (pl.tier == k) {
                std::string line;
                for (int c : pl.clubs) { if (!line.empty()) line += ";"; line += cleanC(g_world.teams[c].name); }
                fprintf(f, "CLUBS|%d|%s\n", k, line.c_str());
            }
        }
    }
    for (auto& kv : g_cupRules) fprintf(f, "CUP|%s|%d|%d|%d\n", kv.first.c_str(), kv.second.enabled ? 1 : 0, kv.second.et ? 1 : 0, kv.second.neutralFinal ? 1 : 0);
    fclose(f);
}

void loadCompEdits() {
    g_origPyr.clear();
    for (auto& P : g_basePyramids) if (pyrEditable(P)) g_origPyr[P.country] = P;
    FILE* f = fopen(COMP_FILE, "r");
    if (!f) return;
    char buf[65536];
    Pyramid* cur = nullptr;
    bool rebuild = false;
    std::vector<TierConf> tiers;
    std::map<int, std::vector<std::string>> clubs;
    auto finish = [&]() {
        if (!cur) return;
        Pyramid& P = *cur;
        std::map<std::string, int> byName;
        for (auto& pl : P.pools) for (int c : pl.clubs) byName[g_world.teams[c].name] = c;
        if (rebuild && !tiers.empty()) {
            // structure libre : les divisions sont reconstruites ; les clubs non listés rejoignent la dernière division
            std::set<int> placed;
            std::vector<int> all; for (auto& pl : P.pools) for (int c : pl.clubs) all.push_back(c);
            P.tiers = tiers; P.pools.clear();
            for (int k = 0; k < (int)tiers.size(); k++) {
                Pool p; p.tier = k; p.key = 0; p.nGroups = 1;
                for (auto& nm : clubs[k]) { auto it = byName.find(nm); if (it != byName.end() && !placed.count(it->second)) { p.clubs.push_back(it->second); placed.insert(it->second); } }
                P.pools.push_back(p);
            }
            for (int c : all) if (!placed.count(c)) P.pools.back().clubs.push_back(c);
        } else if ((int)tiers.size() == (int)P.tiers.size()) {
            for (int k = 0; k < (int)tiers.size(); k++) {
                TierConf& t = P.tiers[k]; const TierConf& n = tiers[k];
                t.name = n.name; t.up = n.up; t.down = n.down; t.barrageUp = n.barrageUp; t.tb = n.tb; t.ptsWin = n.ptsWin; t.legs = n.legs; t.noReserves = n.noReserves;
            }
            for (auto& kv : clubs) {
                if (!tierClubsEditable(P, kv.first)) continue;
                for (auto& nm : kv.second) { auto it = byName.find(nm); if (it != byName.end()) pyrMoveClub(P, it->second, kv.first); }
            }
        }
        refreshPools(P);
        g_editedPyr.insert(P.country);
        cur = nullptr; tiers.clear(); clubs.clear();
    };
    while (fgets(buf, sizeof buf, f)) {
        std::string l = buf;
        while (!l.empty() && (l.back() == '\n' || l.back() == '\r')) l.pop_back();
        if (l.empty() || l[0] == '#') continue;
        auto v = splitC(l, '|');
        if (v[0] == "PYR" && v.size() >= 3) {
            finish();
            for (auto& P : g_basePyramids) if (P.country == v[1] && pyrEditable(P)) cur = &P;
            rebuild = cur && pyrStructEditable(*cur);
        } else if (v[0] == "TIER" && v.size() >= 10 && cur) {
            TierConf t; t.name = v[2]; t.scope = SC_NATIONAL; t.groupsPerPool = 1;
            t.up = atoi(v[3].c_str()); t.down = atoi(v[4].c_str()); t.barrageUp = atoi(v[5].c_str()); t.tb = atoi(v[6].c_str());
            t.ptsWin = std::max(1, std::min(5, atoi(v[7].c_str()))); t.legs = std::max(1, std::min(4, atoi(v[8].c_str()))); t.noReserves = atoi(v[9].c_str()) != 0;
            tiers.push_back(t);
        } else if (v[0] == "CLUBS" && v.size() >= 3 && cur) {
            int k = atoi(v[1].c_str());
            for (auto& nm : splitC(v[2], ';')) if (!nm.empty()) clubs[k].push_back(nm);
        } else if (v[0] == "CUP" && v.size() >= 5) {
            finish();
            CupRule r; r.enabled = atoi(v[2].c_str()) != 0; r.et = atoi(v[3].c_str()) != 0; r.neutralFinal = atoi(v[4].c_str()) != 0;
            g_cupRules[v[1]] = r;
        }
    }
    finish();
    fclose(f);
}
