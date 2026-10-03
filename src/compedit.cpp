// Editeur de compétitions de carrière : divisions, géographie régionale/district,
// coupes nationales et coupes personnalisées persistantes.
#include "game.h"
#include <map>
#include <set>
#include <cstdio>
#include <algorithm>
#include <cstdlib>

void formGroups(Pyramid& P);   // pyramid.cpp

static const char* COMP_FILE = "competitions_perso.txt";
static std::map<std::string, CupRule> g_cupRules;
static std::set<std::string> g_editedPyr;
static std::map<std::string, Pyramid> g_origPyr;
static std::map<std::string, std::map<int, std::string>> g_regions;
static std::map<std::string, std::map<int, std::pair<int, std::string>>> g_districts;
static std::vector<EditorCupDef> g_editorCups;
static int g_nextCupId = 1;

CupRule cupRuleFor(const std::string& country) { auto it = g_cupRules.find(country); return it == g_cupRules.end() ? CupRule() : it->second; }
void setCupRule(const std::string& country, const CupRule& r) { g_cupRules[country] = r; }
bool pyrEdited(const std::string& country) { return g_editedPyr.count(country) > 0; }
void markPyrEdited(const std::string& country) { g_editedPyr.insert(country); }

static bool frCountry(const std::string& c) { return c == "FRA" || c == "F:FRA"; }

// ------------------------------------------------------------------ géographie générique
std::vector<GeoRegionDef> geoRegions(const std::string& country) {
    std::vector<GeoRegionDef> out;
    std::set<int> used;
    if (country == "FRA") {
        for (int i = 0; i < NUM_REGIONS; i++) {
            auto itc = g_regions.find(country); auto it = itc == g_regions.end() ? std::map<int,std::string>::const_iterator() : itc->second.find(i);
            std::string n = (itc != g_regions.end() && it != itc->second.end()) ? it->second : sanitize(REGIONS[i].name);
            out.push_back({ i, country, n }); used.insert(i);
        }
    }
    auto itc = g_regions.find(country);
    if (itc != g_regions.end()) for (auto& kv : itc->second) if (!used.count(kv.first)) out.push_back({ kv.first, country, kv.second });
    std::sort(out.begin(), out.end(), [](const GeoRegionDef& a, const GeoRegionDef& b) { return a.id < b.id; });
    return out;
}

std::vector<GeoDistrictDef> geoDistricts(const std::string& country, int region) {
    std::vector<GeoDistrictDef> out;
    std::set<int> used;
    if (country == "FRA") {
        for (int i = 0; i < numDistricts(); i++) {
            int rg = districtRegion(i); if (region >= 0 && rg != region) continue;
            auto itc = g_districts.find(country);
            std::string n = districtName(i);
            if (itc != g_districts.end()) { auto it = itc->second.find(i); if (it != itc->second.end()) n = it->second.second; }
            out.push_back({ i, rg, country, n }); used.insert(i);
        }
    }
    auto itc = g_districts.find(country);
    if (itc != g_districts.end()) for (auto& kv : itc->second) {
        if (used.count(kv.first)) continue;
        if (region >= 0 && kv.second.first != region) continue;
        out.push_back({ kv.first, kv.second.first, country, kv.second.second });
    }
    std::sort(out.begin(), out.end(), [](const GeoDistrictDef& a, const GeoDistrictDef& b) { return a.id < b.id; });
    return out;
}

std::string geoRegionName(const std::string& country, int id) {
    for (auto& r : geoRegions(country)) if (r.id == id) return r.name;
    return id >= 0 ? fmt("Région %d", id + 1) : std::string("Sans région");
}
std::string geoDistrictName(const std::string& country, int id) {
    for (auto& d : geoDistricts(country)) if (d.id == id) return d.name;
    return id >= 0 ? fmt("District %d", id + 1) : std::string("Sans district");
}

int geoAddRegion(const std::string& country, const std::string& name) {
    int id = country == "FRA" ? NUM_REGIONS : 0;
    for (auto& r : geoRegions(country)) id = std::max(id, r.id + 1);
    g_regions[country][id] = name.empty() ? fmt("Région %d", id + 1) : name;
    return id;
}
bool geoRenameRegion(const std::string& country, int id, const std::string& name) {
    if (id < 0 || name.empty()) return false; g_regions[country][id] = name; return true;
}
bool geoDeleteRegion(const std::string& country, int id) {
    if (country == "FRA" && id < NUM_REGIONS) return false;
    auto itc = g_regions.find(country); if (itc == g_regions.end() || !itc->second.erase(id)) return false;
    auto itd = g_districts.find(country); if (itd != g_districts.end()) for (auto it = itd->second.begin(); it != itd->second.end();) { if (it->second.first == id) it = itd->second.erase(it); else ++it; }
    return true;
}
int geoAddDistrict(const std::string& country, int region, const std::string& name) {
    if (region < 0) return -1;
    int id = country == "FRA" ? numDistricts() : 0;
    for (auto& d : geoDistricts(country)) id = std::max(id, d.id + 1);
    g_districts[country][id] = { region, name.empty() ? fmt("District %d", id + 1) : name };
    return id;
}
bool geoRenameDistrict(const std::string& country, int id, const std::string& name) {
    if (id < 0 || name.empty()) return false;
    if (country == "FRA" && id < numDistricts()) { g_districts[country][id] = { districtRegion(id), name }; return true; }
    auto itc = g_districts.find(country); if (itc == g_districts.end()) return false;
    auto it = itc->second.find(id); if (it == itc->second.end()) return false; it->second.second = name; return true;
}
bool geoDeleteDistrict(const std::string& country, int id) {
    if (country == "FRA" && id < numDistricts()) return false;
    auto itc = g_districts.find(country); return itc != g_districts.end() && itc->second.erase(id) > 0;
}

// ------------------------------------------------------------------ coupes personnalisées persistantes
std::vector<EditorCupDef>& editorCups() { return g_editorCups; }
EditorCupDef* editorCupById(int id) { for (auto& c : g_editorCups) if (c.id == id) return &c; return nullptr; }
int addEditorCup(const std::string& country, const std::string& name) {
    EditorCupDef c; c.id = g_nextCupId++; c.country = country; c.name = name.empty() ? "Nouvelle coupe" : name; g_editorCups.push_back(c); return c.id;
}
bool deleteEditorCup(int id) { auto it = std::remove_if(g_editorCups.begin(), g_editorCups.end(), [&](const EditorCupDef& c){return c.id == id;}); bool ok = it != g_editorCups.end(); g_editorCups.erase(it, g_editorCups.end()); return ok; }

// ------------------------------------------------------------------ pyramides
bool pyrEditable(const Pyramid& P) {
    return P.dom < 0 && P.country != "U19" && P.country != "U17" && P.country != "U15" && !P.tiers.empty();
}
bool pyrStructEditable(const Pyramid& P) {
    // La structure française historique reste protégée ; les autres pays peuvent devenir régionaux.
    return pyrEditable(P) && !frCountry(P.country) && !isWomenPyramid(P);
}
bool tierClubsEditable(const Pyramid& P, int tier) {
    if (!pyrEditable(P) || tier < 0 || tier >= (int)P.tiers.size()) return false;
    int n = 0; for (auto& pl : P.pools) if (pl.tier == tier) n++;
    return n >= 1;
}
static int poolOfTierKey(Pyramid& P, int tier, int key) {
    for (int i = 0; i < (int)P.pools.size(); i++) if (P.pools[i].tier == tier && P.pools[i].key == key) return i;
    return -1;
}
static int poolForClub(Pyramid& P, int tier, int club) {
    if (tier < 0 || tier >= (int)P.tiers.size()) return -1;
    int key = P.tiers[tier].scope == SC_NATIONAL ? 0 : P.keyFor(tier, club);
    int q = poolOfTierKey(P, tier, key);
    if (q >= 0) return q;
    for (int i = 0; i < (int)P.pools.size(); i++) if (P.pools[i].tier == tier) return i;
    return -1;
}

static void refreshPools(Pyramid& P) {
    for (auto& pl : P.pools) {
        if (pl.tier < 0 || pl.tier >= (int)P.tiers.size()) continue;
        if (pl.nGroups <= 1) pl.size = std::max(2, std::max(P.tiers[pl.tier].groupSize, (int)pl.clubs.size()));
        if (P.tiers[pl.tier].scope == SC_NATIONAL && pl.nGroups <= 1) P.tiers[pl.tier].groupSize = (int)pl.clubs.size();
        for (int c : pl.clubs) if (c >= 0 && c < (int)g_world.teams.size()) g_world.teams[c].lastTier = pl.tier;
    }
    formGroups(P);
}

static std::vector<int> geoKeysFor(const Pyramid& P, int scope) {
    std::vector<int> k;
    if (scope == SC_NATIONAL) return { 0 };
    if (scope == SC_REGION) for (auto& r : geoRegions(P.country)) k.push_back(r.id);
    if (scope == SC_DEPT) for (auto& d : geoDistricts(P.country)) k.push_back(d.id);
    if (k.empty()) k.push_back(0);
    return k;
}

void pyrSetTierScope(Pyramid& P, int tier, int scope) {
    if (tier < 0 || tier >= (int)P.tiers.size()) return;
    scope = std::max((int)SC_NATIONAL, std::min((int)SC_DEPT, scope));
    std::vector<int> clubs;
    for (auto& pl : P.pools) if (pl.tier == tier) clubs.insert(clubs.end(), pl.clubs.begin(), pl.clubs.end());
    P.pools.erase(std::remove_if(P.pools.begin(), P.pools.end(), [&](const Pool& p){ return p.tier == tier; }), P.pools.end());
    P.tiers[tier].scope = scope;
    auto keys = geoKeysFor(P, scope);
    for (int key : keys) { Pool p; p.tier = tier; p.key = key; p.nGroups = 1; p.size = std::max(2, P.tiers[tier].groupSize); P.pools.push_back(p); }
    for (int c : clubs) {
        Team& T = g_world.teams[c];
        int key = 0;
        if (scope == SC_REGION) {
            bool ok = false; for (int x : keys) if (x == T.region) ok = true;
            if (!ok) T.region = keys[hashStr(T.name) % keys.size()]; key = T.region;
        } else if (scope == SC_DEPT) {
            bool ok = false; for (int x : keys) if (x == T.district) ok = true;
            if (!ok) {
                auto ds = geoDistricts(P.country, T.region);
                if (ds.empty()) ds = geoDistricts(P.country);
                T.district = ds.empty() ? keys[hashStr(T.name) % keys.size()] : ds[hashStr(T.name) % ds.size()].id;
            }
            key = T.district;
        }
        int q = poolOfTierKey(P, tier, key); if (q < 0) q = poolOfTierKey(P, tier, keys[0]); if (q >= 0) P.pools[q].clubs.push_back(c);
    }
    std::stable_sort(P.pools.begin(), P.pools.end(), [](const Pool& a, const Pool& b){ return a.tier != b.tier ? a.tier < b.tier : a.key < b.key; });
    refreshPools(P);
}

void pyrRefreshGeo(Pyramid& P) {
    for (int tier = 0; tier < (int)P.tiers.size(); tier++) {
        int scope = P.tiers[tier].scope; if (scope != SC_REGION && scope != SC_DEPT) continue;
        auto keys = geoKeysFor(P, scope);
        for (int key : keys) if (poolOfTierKey(P, tier, key) < 0) { Pool p; p.tier = tier; p.key = key; p.nGroups = 1; p.size = std::max(2, P.tiers[tier].groupSize); P.pools.push_back(p); }
        for (auto& pl : P.pools) if (pl.tier == tier && std::find(keys.begin(), keys.end(), pl.key) == keys.end() && pl.clubs.empty()) pl.tier = -9999;
        P.pools.erase(std::remove_if(P.pools.begin(), P.pools.end(), [](const Pool& p){return p.tier == -9999;}), P.pools.end());
    }
    std::stable_sort(P.pools.begin(), P.pools.end(), [](const Pool& a, const Pool& b){ return a.tier != b.tier ? a.tier < b.tier : a.key < b.key; });
    refreshPools(P);
}

static int upFrom(const TierConf& above, const TierConf& t) { return std::max(0, above.down - (t.barrageUp == 2 && above.down > 1 ? 1 : 0)); }
static int downFrom(const TierConf& t, const TierConf& below) { return below.up + (below.barrageUp == 2 && below.up > 0 ? 1 : 0); }
static bool frLike(const Pyramid& P) { return frCountry(P.country); }
void pyrRelink(Pyramid& P, int changedTier, bool fromDown) {
    int T = (int)P.tiers.size(); if (!T) return;
    P.tiers[0].up = 0; P.tiers[0].barrageUp = 0; P.tiers[T - 1].down = 0;
    if (frLike(P)) return;
    if (changedTier >= 0 && changedTier < T) {
        if (fromDown && changedTier + 1 < T) P.tiers[changedTier + 1].up = upFrom(P.tiers[changedTier], P.tiers[changedTier + 1]);
        if (!fromDown && changedTier > 0) P.tiers[changedTier - 1].down = downFrom(P.tiers[changedTier - 1], P.tiers[changedTier]);
    }
}

void pyrMoveClub(Pyramid& P, int club, int toTier) {
    int dst = poolForClub(P, toTier, club); if (dst < 0) return;
    for (auto& pl : P.pools) pl.clubs.erase(std::remove(pl.clubs.begin(), pl.clubs.end(), club), pl.clubs.end());
    P.pools[dst].clubs.push_back(club); refreshPools(P);
}

void pyrAddTier(Pyramid& P, int after, const std::string& name) {
    int at = std::max(0, std::min((int)P.tiers.size(), after + 1));
    TierConf t; t.name = name; t.scope = SC_NATIONAL; t.groupsPerPool = 1; t.groupSize = 16; t.tb = P.tiers.empty() ? TB_GD : P.tiers.back().tb;
    P.tiers.insert(P.tiers.begin() + at, t);
    for (auto& pl : P.pools) if (pl.tier >= at) pl.tier++;
    Pool p; p.tier = at; p.key = 0; p.nGroups = 1; p.size = 16; P.pools.push_back(p);
    std::stable_sort(P.pools.begin(), P.pools.end(), [](const Pool& a, const Pool& b) { return a.tier < b.tier; });
    int T = (int)P.tiers.size();
    if (!frLike(P)) {
        if (at > 0) { if (P.tiers[at - 1].down == 0) P.tiers[at - 1].down = 2; P.tiers[at].up = upFrom(P.tiers[at - 1], P.tiers[at]); }
        if (at + 1 < T) { if (P.tiers[at + 1].up == 0) P.tiers[at + 1].up = 2; P.tiers[at].down = downFrom(P.tiers[at], P.tiers[at + 1]); }
    }
    pyrRelink(P, -1, true); refreshPools(P);
}

bool pyrDeleteTier(Pyramid& P, int tier) {
    int T = (int)P.tiers.size(); if (T <= 1 || tier < 0 || tier >= T) return false;
    std::vector<int> moving; for (auto& pl : P.pools) if (pl.tier == tier) moving.insert(moving.end(), pl.clubs.begin(), pl.clubs.end());
    int into = tier + 1 < T ? tier + 1 : tier - 1;
    P.pools.erase(std::remove_if(P.pools.begin(), P.pools.end(), [&](const Pool& p){return p.tier == tier;}), P.pools.end());
    P.tiers.erase(P.tiers.begin() + tier);
    for (auto& pl : P.pools) if (pl.tier > tier) pl.tier--;
    if (into > tier) into--;
    for (int c : moving) { int q = poolForClub(P, into, c); if (q >= 0) P.pools[q].clubs.push_back(c); }
    if (!frLike(P) && tier > 0 && tier < (int)P.tiers.size()) P.tiers[tier].up = upFrom(P.tiers[tier - 1], P.tiers[tier]);
    pyrRelink(P, -1, true); refreshPools(P); return true;
}

bool pyrRestore(Pyramid& P) {
    auto it = g_origPyr.find(P.country); if (it == g_origPyr.end()) return false;
    P = it->second; g_editedPyr.erase(P.country); refreshPools(P); return true;
}

// ------------------------------------------------------------------ fichier
static std::vector<std::string> splitC(const std::string& s, char sep) { std::vector<std::string> v; std::string cur; for (char c : s) { if (c == sep) { v.push_back(cur); cur.clear(); } else cur += c; } v.push_back(cur); return v; }
static std::string cleanC(const std::string& s) { std::string o; for (char c : s) o += (c == '|' || c == ';' || c == '\n' || c == '\r') ? ' ' : c; return o; }

void saveCompEdits() {
    FILE* f = fopen(COMP_FILE, "w"); if (!f) return;
    fprintf(f, "# Compétitions, régions, districts et coupes personnalisés (Super Soccer World)\n");
    for (auto& cc : g_regions) for (auto& kv : cc.second) fprintf(f, "REGION|%s|%d|%s\n", cc.first.c_str(), kv.first, cleanC(kv.second).c_str());
    for (auto& cc : g_districts) for (auto& kv : cc.second) fprintf(f, "DISTRICT|%s|%d|%d|%s\n", cc.first.c_str(), kv.first, kv.second.first, cleanC(kv.second.second).c_str());
    for (auto& P : g_basePyramids) {
        if (!g_editedPyr.count(P.country)) continue;
        fprintf(f, "PYR|%s|%d\n", P.country.c_str(), (int)P.tiers.size());
        for (int k = 0; k < (int)P.tiers.size(); k++) {
            const TierConf& t = P.tiers[k];
            fprintf(f, "TIER|%d|%s|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d\n", k, cleanC(t.name).c_str(), t.up, t.down, t.barrageUp, t.tb, t.ptsWin, t.legs, t.noReserves ? 1 : 0, t.scope, t.groupsPerPool, t.groupSize, t.ptsDraw, t.ptsLoss, t.ptsForfeit);
            for (auto& pl : P.pools) if (pl.tier == k) {
                std::string line; for (int c : pl.clubs) if (c >= 0 && c < (int)g_world.teams.size()) { if (!line.empty()) line += ";"; line += cleanC(g_world.teams[c].name); }
                fprintf(f, "CLUBS|%d|%d|%s\n", k, pl.key, line.c_str());
            }
        }
    }
    for (auto& kv : g_cupRules) fprintf(f, "CUP|%s|%d|%d|%d|%d|%d\n", kv.first.c_str(), kv.second.enabled ? 1 : 0, kv.second.et ? 1 : 0, kv.second.neutralFinal ? 1 : 0, kv.second.euroQual ? 1 : 0, kv.second.finalSize);
    for (auto& c : g_editorCups) fprintf(f, "XCUP|%d|%s|%s|%d|%d|%d|%d|%d|%d|%d|%d|%d\n", c.id, c.country.c_str(), cleanC(c.name).c_str(), c.scope, c.geoKey, c.minTier, c.maxTier, c.finalSize, c.qual, c.enabled ? 1 : 0, c.et ? 1 : 0, c.neutralFinal ? 1 : 0);
    fclose(f);
}

void loadCompEdits() {
    g_origPyr.clear(); for (auto& P : g_basePyramids) if (pyrEditable(P)) g_origPyr[P.country] = P;
    g_cupRules.clear(); g_editedPyr.clear(); g_regions.clear(); g_districts.clear(); g_editorCups.clear(); g_nextCupId = 1;
    FILE* f = fopen(COMP_FILE, "r"); if (!f) return;
    char buf[65536]; Pyramid* cur = nullptr; bool rebuild = false;
    std::vector<TierConf> tiers; std::map<std::pair<int,int>, std::vector<std::string>> clubs;
    auto finish = [&]() {
        if (!cur) return; Pyramid& P = *cur;
        std::map<std::string, int> byName; std::vector<int> all;
        for (auto& pl : P.pools) for (int c : pl.clubs) if (c >= 0 && c < (int)g_world.teams.size()) { byName[g_world.teams[c].name] = c; all.push_back(c); }
        // inclure les clubs personnalisés du pays, même s'ils ne sont pas encore dans la pyramide de base
        int ni = g_world.nationIndex(P.country.c_str());
        for (int c = 0; c < (int)g_world.teams.size(); c++) if (g_world.teams[c].kind == TK_CLUB && g_world.teams[c].nation == ni && g_world.teams[c].parent < 0) byName[g_world.teams[c].name] = c;
        if (rebuild && !tiers.empty()) {
            P.tiers = tiers; P.pools.clear(); std::set<int> placed;
            for (int k = 0; k < (int)P.tiers.size(); k++) {
                auto keys = geoKeysFor(P, P.tiers[k].scope);
                for (int key : keys) { Pool p; p.tier = k; p.key = key; p.nGroups = std::max(1, P.tiers[k].groupsPerPool); p.size = std::max(2, P.tiers[k].groupSize); P.pools.push_back(p); }
            }
            for (auto& kv : clubs) {
                int k = kv.first.first, key = kv.first.second; if (k < 0 || k >= (int)P.tiers.size()) continue;
                int q = poolOfTierKey(P, k, P.tiers[k].scope == SC_NATIONAL ? 0 : key); if (q < 0) continue;
                for (auto& nm : kv.second) { auto it = byName.find(nm); if (it == byName.end() || placed.count(it->second)) continue; int c = it->second; P.pools[q].clubs.push_back(c); placed.insert(c); if (P.tiers[k].scope == SC_REGION) g_world.teams[c].region = key; if (P.tiers[k].scope == SC_DEPT) g_world.teams[c].district = key; }
            }
            // clubs de la pyramide d'origine non listés : dernier niveau compatible
            for (int c : all) if (!placed.count(c)) { int k = (int)P.tiers.size() - 1; int q = poolForClub(P, k, c); if (q >= 0) P.pools[q].clubs.push_back(c); }
        } else if ((int)tiers.size() == (int)P.tiers.size()) {
            for (int k = 0; k < (int)tiers.size(); k++) {
                TierConf old = P.tiers[k], n = tiers[k];
                // France : conserver la portée/pools historiques, mais accepter les règles de points et noms
                if (frLike(P)) { n.scope = old.scope; n.groupsPerPool = old.groupsPerPool; n.groupSize = old.groupSize; }
                P.tiers[k] = n;
            }
        }
        refreshPools(P); g_editedPyr.insert(P.country); cur = nullptr; tiers.clear(); clubs.clear();
    };
    while (fgets(buf, sizeof buf, f)) {
        std::string l = buf; while (!l.empty() && (l.back() == '\n' || l.back() == '\r')) l.pop_back(); if (l.empty() || l[0] == '#') continue;
        auto v = splitC(l, '|'); if (v.empty()) continue;
        if (v[0] == "REGION" && v.size() >= 4) { g_regions[v[1]][atoi(v[2].c_str())] = v[3]; }
        else if (v[0] == "DISTRICT" && v.size() >= 5) { g_districts[v[1]][atoi(v[2].c_str())] = { atoi(v[3].c_str()), v[4] }; }
        else if (v[0] == "PYR" && v.size() >= 3) {
            finish(); for (auto& P : g_basePyramids) if (P.country == v[1] && pyrEditable(P)) cur = &P; rebuild = cur && pyrStructEditable(*cur);
        } else if (v[0] == "TIER" && v.size() >= 10 && cur) {
            TierConf t; t.name = v[2]; t.up = atoi(v[3].c_str()); t.down = atoi(v[4].c_str()); t.barrageUp = atoi(v[5].c_str()); t.tb = atoi(v[6].c_str());
            t.ptsWin = std::max(-10, std::min(20, atoi(v[7].c_str()))); t.legs = std::max(1, std::min(4, atoi(v[8].c_str()))); t.noReserves = atoi(v[9].c_str()) != 0;
            if (v.size() >= 16) { t.scope = std::max(0, std::min(3, atoi(v[10].c_str()))); t.groupsPerPool = atoi(v[11].c_str()); t.groupSize = std::max(2, atoi(v[12].c_str())); t.ptsDraw = std::max(-10, std::min(20, atoi(v[13].c_str()))); t.ptsLoss = std::max(-10, std::min(20, atoi(v[14].c_str()))); t.ptsForfeit = std::max(-20, std::min(20, atoi(v[15].c_str()))); }
            else { t.scope = SC_NATIONAL; t.groupsPerPool = 1; t.groupSize = 16; }
            tiers.push_back(t);
        } else if (v[0] == "CLUBS" && v.size() >= 3 && cur) {
            int k = atoi(v[1].c_str()), key = v.size() >= 4 ? atoi(v[2].c_str()) : 0; const std::string& names = v.size() >= 4 ? v[3] : v[2];
            for (auto& nm : splitC(names, ';')) if (!nm.empty()) clubs[{k,key}].push_back(nm);
        } else if (v[0] == "CUP" && v.size() >= 5) {
            finish(); CupRule r; r.enabled = atoi(v[2].c_str()) != 0; r.et = atoi(v[3].c_str()) != 0; r.neutralFinal = atoi(v[4].c_str()) != 0; if (v.size() >= 6) r.euroQual = atoi(v[5].c_str()) != 0; if (v.size() >= 7) r.finalSize = std::max(0, atoi(v[6].c_str())); g_cupRules[v[1]] = r;
        } else if (v[0] == "XCUP" && v.size() >= 13) {
            finish(); EditorCupDef c; c.id = atoi(v[1].c_str()); c.country = v[2]; c.name = v[3]; c.scope = atoi(v[4].c_str()); c.geoKey = atoi(v[5].c_str()); c.minTier = atoi(v[6].c_str()); c.maxTier = atoi(v[7].c_str()); c.finalSize = std::max(2, atoi(v[8].c_str())); c.qual = atoi(v[9].c_str()); c.enabled = atoi(v[10].c_str()) != 0; c.et = atoi(v[11].c_str()) != 0; c.neutralFinal = atoi(v[12].c_str()) != 0; g_editorCups.push_back(c); g_nextCupId = std::max(g_nextCupId, c.id + 1);
        }
    }
    finish(); fclose(f);
    for (auto& P : g_basePyramids) if (pyrEditable(P)) pyrRefreshGeo(P);
}
