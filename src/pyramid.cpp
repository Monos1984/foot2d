// Pyramides des championnats
// France 2026-2027 : L1, L2, L3 (pro), National 1 (3x16), National 2 (8x14), R1/R2/R3 (13 ligues),
// Départemental 1 à 5 selon la taille du district, équipes réserves ; outre-mer ; pays étrangers.
#include "game.h"
#include <map>
#include <set>
#include <cstring>

std::vector<Pyramid> g_basePyramids;
int dbClubIndex(const char* key);             // world.cpp
void makeKits(Team& t, unsigned s1, unsigned s2, unsigned sh, int pat);

static const int NUM_METRO_REGIONS = 13;      // index 0..12 dans REGIONS ; 13..17 = outre-mer

int zoneOfRegion(int region) { return region == 12 ? 11 : region; }
int numZones() { return 12; }
const char* zoneName(int z) {
    static std::string names[12];
    if (names[0].empty()) {
        for (int i = 0; i < 12; i++) names[i] = sanitize(REGIONS[i].name);
        names[11] = sanitize("Méditerranée - Corse");
    }
    return names[z].c_str();
}

int Pyramid::poolIndex(int tier, int key) const {
    for (int i = 0; i < (int)pools.size(); i++) if (pools[i].tier == tier && pools[i].key == key) return i;
    return -1;
}

int Pyramid::keyFor(int tier, int club) const {
    const Team& t = g_world.teams[club];
    switch (tiers[tier].scope) {
    case SC_ZONE: return zoneOfRegion(t.region);
    case SC_REGION: return t.region;
    case SC_DEPT: return t.district;
    default: return 0;
    }
}

static int deptIndex(const char* code) {
    for (int i = 0; i < NUM_DEPTS; i++) if (!strcmp(DEPTS[i].code, code)) return i;
    return -1;
}

int regionDeptCount(int region) {
    int n = 0;
    for (int i = 0; i < NUM_DEPTS; i++) if (DEPTS[i].region == region) n++;
    return n;
}

int poolGroupCount(const Pyramid& P, const Pool& pool) {
    const TierConf& T = P.tiers[pool.tier];
    if (T.flexible || pool.terminal) {
        int n = (int)std::lround((double)pool.clubs.size() / std::max(1, pool.size));
        return std::max(1, n);
    }
    return std::max(1, pool.nGroups);
}

int poolTarget(const Pyramid& P, const Pool& pool) {
    return poolGroupCount(P, pool) * pool.size;
}

// ordre géographique pour les découpages nationaux (tour de France)
static int regionGeoOrder(int r) {
    static const int ORDER[] = { 7, 6, 3, 1, 0, 4, 5, 2, 10, 11, 12, 9, 8 };
    for (int i = 0; i < 13; i++) if (ORDER[i] == r) return i;
    return 20 + r;
}

void formGroups(Pyramid& P) {
    for (auto& pool : P.pools) {
        int n = poolGroupCount(P, pool);
        std::vector<int> cl = pool.clubs;
        int scope = P.tiers[pool.tier].scope;
        std::stable_sort(cl.begin(), cl.end(), [&](int a, int b) {
            const Team& A = g_world.teams[a]; const Team& B = g_world.teams[b];
            if (scope == SC_NATIONAL) {
                int ra = regionGeoOrder(A.region), rb = regionGeoOrder(B.region);
                if (ra != rb) return ra < rb;
            }
            if (A.dept != B.dept) return A.dept < B.dept;
            if (scope == SC_DEPT) return hashStr(A.town + "g") < hashStr(B.town + "g");
            return a < b;
        });
        pool.groups.assign(n, {});
        int N = (int)cl.size();
        for (int i = 0; i < N; i++) pool.groups[(int)((long long)i * n / std::max(1, N))].push_back(cl[i]);
        // deux équipes d'un même club jamais dans la même poule
        if (n > 1) {
            auto clubOf = [](int t) { return g_world.teams[t].parent >= 0 ? g_world.teams[t].parent : t; };
            auto hasClub = [&](const std::vector<int>& g, int club, int except) {
                for (int x : g) if (x != except && clubOf(x) == club) return true;
                return false;
            };
            for (int g = 0; g < n; g++) {
                for (int i = 0; i < (int)pool.groups[g].size(); i++) {
                    int a = pool.groups[g][i];
                    if (!hasClub(pool.groups[g], clubOf(a), a)) continue;
                    bool done = false;
                    for (int h = 0; h < n && !done; h++) {
                        if (h == g || hasClub(pool.groups[h], clubOf(a), -1)) continue;
                        for (int j = 0; j < (int)pool.groups[h].size() && !done; j++) {
                            int b = pool.groups[h][j];
                            if (hasClub(pool.groups[g], clubOf(b), a)) continue;
                            std::swap(pool.groups[g][i], pool.groups[h][j]);
                            done = true;
                        }
                    }
                }
            }
        }
    }
}

// ------------------------------------------------------------------ génération des clubs amateurs
static const char* PREFIXES[] = { "US", "AS", "FC", "ES", "SC", "Stade", "Olympique", "JS", "AS", "CS", "AF", "US",
                                  "RC", "SO", "AC", "Entente", "UF", "FC", "Avenir", "Étoile", "Racing", "Union", "Sporting", "AJ",
                                  "ASC", "Espérance", "Jeunesse", "SA", "EF", "Football Club", "US", "FC", "AS", "Amicale", "Olympique", "ES" };
static const int NUM_PREFIXES = sizeof(PREFIXES) / sizeof(PREFIXES[0]);
static const unsigned CLUB_COLORS[] = { 0xE2001A, 0x0055A4, 0x00843D, 0xFFE500, 0xFFFFFF, 0x000000, 0xF47920,
                                        0x7B1E2B, 0x3FA9F5, 0x5B2C83, 0x1B2A63, 0x9E9E9E };
static const char* STADIUM_NAMES[] = { "Stade Léo-Lagrange", "Stade Jean-Bouin", "Stade Pierre-de-Coubertin", "Stade Jean-Moulin",
    "Stade municipal", "Stade Jules-Ladoumègue", "Stade Roger-Salengro", "Stade Jean-Jaurès", "Stade Georges-Carpentier",
    "Stade Auguste-Delaune", "Stade Paul-Éluard", "Stade Charles-de-Gaulle", "Stade des Sports", "Stade du Moulin", "Stade de la Plaine",
    "Stade Henri-Barbusse", "Stade Émile-Zola", "Stade Victor-Hugo", "Stade Louis-Pasteur", "Stade Jacques-Anquetil", "Stade Guy-Môquet",
    "Stade Maurice-Baquet", "Stade Marcel-Cerdan", "Complexe sportif", "Parc des sports", "Stade de la Prairie", "Stade des Peupliers",
    "Stade Michel-Hidalgo", "Stade Just-Fontaine", "Stade Raymond-Kopa", "Stade du Stade", "Stade Robert-Bobin", "Stade de l'Europe",
    "Stade Pierre-Mendès-France", "Stade Lucien-Choine", "Stade René-Gaillard", "Stade des Lilas", "Stade du Parc", "Stade de la Forêt" };
static const int NUM_STADIUM_NAMES = sizeof(STADIUM_NAMES) / sizeof(STADIUM_NAMES[0]);

struct GenClub { std::string name, town; };
struct DeptGen {
    std::vector<GenClub> list;
    int next = 0;
};

static std::set<std::string> g_usedNames;

static void buildDeptGen(int d, DeptGen& g) {
    struct Town { std::string name; int pop; };
    std::vector<Town> towns;
    std::string s = DEPTS[d].towns;
    size_t p = 0;
    while (p < s.size()) {
        size_t e = s.find(';', p); if (e == std::string::npos) e = s.size();
        std::string item = s.substr(p, e - p);
        size_t bar = item.find('|');
        if (bar != std::string::npos) towns.push_back({ sanitize(item.substr(0, bar).c_str()), atoi(item.c_str() + bar + 1) });
        p = e + 1;
    }
    struct Cand { std::string town; int k; double score; };
    std::vector<Cand> cands;
    for (auto& t : towns) {
        int maxk = 1 + std::min(8, t.pop / 20000);
        for (int k = 0; k < maxk; k++) cands.push_back({ t.name, k, (t.pop + 400.0) / std::pow(k + 1.0, 1.5) });
    }
    std::stable_sort(cands.begin(), cands.end(), [](const Cand& a, const Cand& b) { return a.score > b.score; });
    for (auto& c : cands) {
        uint32_t h = hashStr(c.town);
        for (int tries = 0; tries < NUM_PREFIXES + 6; tries++) {
            int pi = (int)((h + c.k * 7 + tries) % NUM_PREFIXES);
            std::string nm;
            if (tries >= NUM_PREFIXES) nm = fmt("%s %s %d", sanitize(PREFIXES[pi]).c_str(), c.town.c_str(), tries - NUM_PREFIXES + 2);
            else if (!strcmp(PREFIXES[pi], "Football Club")) nm = c.town + " FC";
            else nm = sanitize(PREFIXES[pi]) + " " + c.town;
            if (!g_usedNames.count(nm)) { g_usedNames.insert(nm); g.list.push_back({ nm, c.town }); break; }
        }
    }
}

static const float TIER_BASE_FR[] = { 72, 60, 53, 46, 41, 37, 33, 29, 26, 23, 20, 17, 15 };
float frTierBase(int tier) { return TIER_BASE_FR[std::max(0, std::min(12, tier))]; }

static std::string shortOf(const std::string& town) {
    std::string sn;
    for (char c : town) { if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) sn += (char)toupper(c); if (sn.size() >= 3) break; }
    return sn.size() < 3 ? std::string("CLB") : sn;
}

static int makeGenClub(World& w, const GenClub& g, int dept, float rating) {
    Team t;
    t.name = g.name; t.town = g.town;
    t.shortName = shortOf(g.town);
    t.kind = TK_CLUB;
    t.nation = w.nationIndex("FRA");
    t.rating = rating;
    t.culture = CU_FR;
    t.dept = dept;
    t.region = dept >= 0 ? DEPTS[dept].region : 0;
    t.district = districtFor(dept, g.town, g.name);
    t.seed = hashStr(g.name) ^ 0x51;
    uint32_t h = t.seed;
    t.stadium = sanitize(STADIUM_NAMES[(h >> 7) % NUM_STADIUM_NAMES]);
    if (t.stadium == sanitize("Stade municipal")) t.stadium = sanitize("Stade municipal de ") + g.town;
    unsigned c1 = CLUB_COLORS[h % 12], c2 = CLUB_COLORS[(h / 12) % 12];
    if (c2 == c1) c2 = (c1 == 0xFFFFFF) ? 0x0055A4 : 0xFFFFFF;
    int pat = (h / 144) % 10;
    int pattern = pat < 6 ? KP_PLAIN : pat < 8 ? KP_VSTRIPES : pat < 9 ? KP_HALVES : KP_HOOPS;
    t.home.shirt = c1; t.home.shirt2 = c2; t.home.shorts = (h & 0x100) ? c2 : c1; t.home.socks = c1; t.home.pattern = pattern;
    t.away.shirt = c2; t.away.shirt2 = c1; t.away.shorts = c2; t.away.socks = c2; t.away.pattern = KP_PLAIN;
    t.formation = (int)(h % NUM_FORMATIONS);
    w.teams.push_back(t);
    return (int)w.teams.size() - 1;
}

// réserve d'un club existant (équipe 2, 3...)
int makeReserve(World& w, int parent, int level, float rating) {
    const Team& P = w.teams[parent];
    Team t = P;
    t.squad.clear(); t.squadGen = false; t.honours.clear();
    t.parent = parent; t.resLevel = level;
    bool pro = P.rating >= 50;
    const char* suf[] = { "", " B", " C", " D" };
    const char* sufA[] = { "", " 2", " 3", " 4" };
    t.name = P.name + (pro ? suf[std::min(level, 3)] : sufA[std::min(level, 3)]);
    t.shortName = P.shortName.substr(0, 4) + (pro ? "B" : fmt("%d", level + 1));
    t.rating = rating;
    t.dbClub = -1;
    t.seed = hashStr(t.name) ^ 0x77;
    w.teams.push_back(t);
    return (int)w.teams.size() - 1;
}

static int addFr(World& w, const FrClubDef& d, int nat, std::map<std::string, int>& byName) {
    Team t;
    t.name = sanitize(d.name); t.shortName = sanitize(d.shortName);
    t.kind = TK_CLUB; t.nation = nat;
    t.dbClub = dbClubIndex(d.dbKey);
    t.rating = (float)(t.dbClub >= 0 ? DBCLUBS[t.dbClub].rating : d.rating);
    t.culture = CU_FR;
    t.dept = deptIndex(d.dept);
    t.region = t.dept >= 0 ? DEPTS[t.dept].region : 0;
    t.town = t.dept >= 0 ? sanitize(DEPTS[t.dept].name) : "";
    t.district = districtFor(t.dept, t.town, t.name);
    t.stadium = sanitize(d.stadium);
    makeKits(t, d.shirt, d.shirt2, d.shorts, d.pattern);
    t.seed = hashStr(t.name) ^ 0x77;
    t.formation = (int)(t.seed % NUM_FORMATIONS);
    if (d.parent && *d.parent) {
        auto it = byName.find(sanitize(d.parent));
        if (it != byName.end()) {
            t.parent = it->second;
            int lvl = 1;
            for (auto& x : w.teams) if (x.parent == t.parent && x.resLevel >= lvl) lvl = x.resLevel + 1;
            t.resLevel = lvl;
            t.home = w.teams[t.parent].home; t.away = w.teams[t.parent].away;
        }
    }
    g_usedNames.insert(t.name);
    w.teams.push_back(t);
    int id = (int)w.teams.size() - 1;
    if (t.parent < 0) byName[t.name] = id;
    return id;
}

// Taille des niveaux régionaux par ligue : R1 (groupes, taille, montées en N2), R2, R3
struct RegTable { int r1g, r1s, up, r2g, r2s, r3g, r3s; };
static const RegTable REG_TABLE[13] = {
    { 2, 12, 2, 4, 12, 8, 12 },  // Paris Île-de-France
    { 1, 12, 1, 2, 12, 4, 12 },  // Centre-Val de Loire
    { 1, 14, 1, 2, 12, 5, 12 },  // Bourgogne-Franche-Comté
    { 2, 12, 1, 3, 12, 6, 12 },  // Normandie
    { 2, 14, 2, 4, 12, 8, 12 },  // Hauts-de-France
    { 2, 14, 2, 4, 14, 9, 12 },  // Grand Est
    { 2, 12, 1, 4, 12, 8, 12 },  // Pays de la Loire
    { 2, 12, 1, 4, 12, 8, 12 },  // Bretagne
    { 2, 12, 2, 6, 12, 10, 12 }, // Nouvelle-Aquitaine
    { 2, 12, 2, 4, 12, 8, 12 },  // Occitanie
    { 2, 12, 2, 4, 12, 10, 12 }, // Auvergne-Rhône-Alpes
    { 1, 14, 1, 2, 14, 4, 12 },  // Méditerranée
    { 1, 12, 1, 1, 10, 0, 12 },  // Corse
};

// niveaux départementaux selon la taille du district : nombre de groupes par niveau (0 = niveau absent)
static std::vector<int> distLevels(int d) {
    int pop = districtPopulation(d);
    if (districtRegion(d) == 12) return { 1, 2 };                   // Corse
    if (pop > 1400000) return { 1, 2, 4, 6, 8 };
    if (pop > 800000) return { 1, 2, 3, 5, 6 };
    if (pop > 450000) return { 1, 2, 3, 4 };
    if (pop > 250000) return { 1, 2, 2, 3 };
    return { 1, 2, 2 };
}
int maxDeptLevels() { return 5; }

static void buildFrance(World& w) {
    Rng r(0xF4A7CE);
    int fra = w.nationIndex("FRA");
    Pyramid P;
    P.country = "FRA"; P.name = "France"; P.barrageUp = 1;
    auto T = [&](const char* n, int scope, int size, int up, int down, bool flex, bool noRes, int tb, int barrage) {
        TierConf c; c.name = sanitize(n); c.scope = scope; c.groupsPerPool = -1; c.groupSize = size; c.up = up; c.down = down;
        c.flexible = flex; c.noReserves = noRes; c.tb = tb; c.barrageUp = barrage;
        P.tiers.push_back(c);
    };
    T("Ligue 1", SC_NATIONAL, 18, 0, 2, false, true, TB_LFP, 0);
    T("Ligue 2", SC_NATIONAL, 18, 2, 2, false, true, TB_LFP, 3);   // 3 = barrages L2 (5e-4e, puis 3e, puis 16e de L1)
    T("Ligue 3", SC_NATIONAL, 18, 2, 3, false, true, TB_FFF, 1);   // 1 = 3e de L3 contre 16e de L2
    T("National 1", SC_NATIONAL, 16, 1, 3, false, false, TB_FFF, 0);
    T("National 2", SC_NATIONAL, 14, 1, 2, false, false, TB_FFF, 0);
    T("Régional 1", SC_REGION, 12, 1, 2, false, false, TB_FFF, 0);
    T("Régional 2", SC_REGION, 12, 1, 2, false, false, TB_FFF, 0);
    T("Régional 3", SC_REGION, 12, 1, 3, false, false, TB_FFF, 0);
    for (int k = 1; k <= 5; k++) T(fmt("Départemental %d", k).c_str(), SC_DEPT, 12, 1, k == 5 ? 0 : 2, false, false, TB_FFF, 0);
    const int TD1 = 8;
    auto addPool = [&](int tier, int key, int ng, int size) { Pool p; p.tier = tier; p.key = key; p.nGroups = ng; p.size = size; P.pools.push_back(p); return (int)P.pools.size() - 1; };
    addPool(0, 0, 1, 18); addPool(1, 0, 1, 18); addPool(2, 0, 1, 18);
    addPool(3, 0, 3, 16); addPool(4, 0, 8, 14);
    for (int rg = 0; rg < NUM_METRO_REGIONS; rg++) {
        const RegTable& R = REG_TABLE[rg];
        P.pools[addPool(5, rg, R.r1g, R.r1s)].upCap = R.up;
        addPool(6, rg, R.r2g, R.r2s);
        if (R.r3g > 0) addPool(7, rg, R.r3g, R.r3s);
    }
    for (int d = 0; d < numDistricts(); d++) {
        if (districtRegion(d) < 0 || districtRegion(d) >= NUM_METRO_REGIONS) continue;
        auto lv = distLevels(d);
        for (int k = 0; k < (int)lv.size(); k++) addPool(TD1 + k, d, lv[k], 12);
    }
    // dernier niveau de chaque district : taille libre
    for (auto& pl : P.pools) if (P.tiers[pl.tier].scope == SC_DEPT && P.poolIndex(pl.tier + 1, pl.key) < 0) pl.terminal = true;
    std::map<std::string, int> byName;
    auto poolOf = [&](int tier, int key) { return P.poolIndex(tier, key); };
    for (int i = 0; i < NUM_FR_L1; i++) P.pools[0].clubs.push_back(addFr(w, FR_L1[i], fra, byName));
    for (int i = 0; i < NUM_FR_L2; i++) P.pools[1].clubs.push_back(addFr(w, FR_L2[i], fra, byName));
    for (int i = 0; i < NUM_FR_L3; i++) P.pools[2].clubs.push_back(addFr(w, FR_L3[i], fra, byName));
    for (int i = 0; i < NUM_FR_N1; i++) P.pools[3].clubs.push_back(addFr(w, FR_N1[i], fra, byName));
    for (int i = 0; i < NUM_FR_N2; i++) P.pools[4].clubs.push_back(addFr(w, FR_N2[i], fra, byName));
    // groupes réels de N1 / N2
    P.pools[3].groups.assign(3, {});
    for (int i = 0; i < (int)P.pools[3].clubs.size(); i++) P.pools[3].groups[std::min(2, i / 16)].push_back(P.pools[3].clubs[i]);
    P.pools[4].groups.assign(8, {});
    for (int i = 0; i < (int)P.pools[4].clubs.size(); i++) P.pools[4].groups[std::min(7, i / 14)].push_back(P.pools[4].clubs[i]);
    for (int i = 0; i < NUM_FR_RESERVES_R1; i++) {
        int id = addFr(w, FR_RESERVES_R1[i], fra, byName);
        P.pools[poolOf(5, w.teams[id].region)].clubs.push_back(id);
    }

    // générateurs : communes de chaque département, réparties par district
    std::vector<DeptGen> gens(NUM_DEPTS);
    for (int d = 0; d < NUM_DEPTS; d++) buildDeptGen(d, gens[d]);
    const int ND = numDistricts();
    struct Sub { int dept; std::vector<GenClub> list; size_t next = 0; };
    std::vector<std::vector<Sub>> distGen(ND);
    for (int di = 0; di < ND; di++) for (int d : districtDepts(di)) { Sub sb; sb.dept = d; distGen[di].push_back(sb); }
    for (int d = 0; d < NUM_DEPTS; d++) {
        if (DEPTS[d].region >= NUM_METRO_REGIONS) continue;
        for (auto& gc : gens[d].list) {
            int di = districtFor(d, gc.town, gc.name);
            if (di < 0) continue;
            for (auto& sb : distGen[di]) if (sb.dept == d) { sb.list.push_back(gc); break; }
        }
    }
    std::vector<std::vector<int>> distFirstTeams(ND);          // clubs (équipes 1) par district
    std::map<int, int> teamsOfClub;                            // nombre d'équipes par club
    std::map<int, int> lowestTier;
    for (auto& pool : P.pools) for (int c : pool.clubs) {
        int club = w.teams[c].parent >= 0 ? w.teams[c].parent : c;
        teamsOfClub[club]++;
        lowestTier[club] = std::max(lowestTier.count(club) ? lowestTier[club] : 0, pool.tier);
        if (w.teams[c].parent < 0 && w.teams[c].district >= 0) distFirstTeams[w.teams[c].district].push_back(c);
    }
    std::vector<int> fallbackN(ND, 0);
    auto genFrom = [&](int di, int tier) {
        auto& subs = distGen[di];
        double tot = 0;
        for (auto& sb : subs) if (sb.next < sb.list.size()) tot += std::sqrt((double)DEPTS[sb.dept].population);
        GenClub gc; int dept = subs.empty() ? 0 : subs[0].dept;
        if (tot > 0) {
            double x = r.f() * tot;
            for (auto& sb : subs) {
                if (sb.next >= sb.list.size()) continue;
                x -= std::sqrt((double)DEPTS[sb.dept].population);
                if (x <= 0) { gc = sb.list[sb.next++]; dept = sb.dept; break; }
            }
            if (gc.name.empty()) for (auto& sb : subs) if (sb.next < sb.list.size()) { gc = sb.list[sb.next++]; dept = sb.dept; break; }
        }
        if (gc.name.empty()) { gc.town = districtName(di); gc.name = fmt("FC %s %d", gc.town.c_str(), ++fallbackN[di]); }
        int id = makeGenClub(w, gc, dept, TIER_BASE_FR[tier] + r.frange(-3, 3));
        w.teams[id].district = di;
        distFirstTeams[di].push_back(id);
        teamsOfClub[id] = 1; lowestTier[id] = tier;
        return id;
    };
    // outre-mer (pas de district) : communes du département
    auto genFromDept = [&](int d, int tier) {
        DeptGen& g = gens[d];
        GenClub gc;
        if (g.next < (int)g.list.size()) gc = g.list[g.next++];
        else { gc.town = sanitize(DEPTS[d].name); gc.name = fmt("FC %s %d", gc.town.c_str(), g.next++); }
        return makeGenClub(w, gc, d, TIER_BASE_FR[tier] + r.frange(-3, 3));
    };
    // réserve d'un club du district déjà placé plus haut
    auto reserveFrom = [&](int di, int tier) {
        std::vector<int> cand;
        for (int c : distFirstTeams[di]) {
            if (teamsOfClub[c] >= 3) continue;
            if (lowestTier[c] >= tier) continue;
            cand.push_back(c);
        }
        if (cand.empty()) return -1;
        int c = cand[r.range(0, (int)cand.size() - 1)];
        int lvl = teamsOfClub[c];
        int id = makeReserve(w, c, lvl, TIER_BASE_FR[tier] + r.frange(-3, 2));
        teamsOfClub[c]++; lowestTier[c] = tier;
        return id;
    };
    auto pickDist = [&](const std::vector<int>& dists) {
        double tot = 0;
        for (int d : dists) tot += std::sqrt((double)districtPopulation(d));
        double x = r.f() * tot;
        for (int d : dists) { x -= std::sqrt((double)districtPopulation(d)); if (x <= 0) return d; }
        return dists.back();
    };
    auto distsOfRegion = [&](int rg) { std::vector<int> v; for (int d = 0; d < ND; d++) if (districtRegion(d) == rg) v.push_back(d); return v; };
    auto deptsOfRegion = [&](int rg) { std::vector<int> v; for (int d = 0; d < NUM_DEPTS; d++) if (DEPTS[d].region == rg) v.push_back(d); return v; };
    static const float RES_P[] = { 0, 0, 0, 0, 0, 0.04f, 0.10f, 0.18f, 0.22f, 0.28f, 0.33f, 0.38f, 0.40f };
    for (int tier = 5; tier <= 7; tier++) {
        for (int rg = 0; rg < NUM_METRO_REGIONS; rg++) {
            int q = poolOf(tier, rg);
            if (q < 0) continue;
            Pool& pool = P.pools[q];
            int target = pool.nGroups * pool.size;
            std::vector<int> dists = distsOfRegion(rg);
            while ((int)pool.clubs.size() < target) {
                int d = pickDist(dists);
                int id = -1;
                if (r.chance(RES_P[tier])) id = reserveFrom(d, tier);
                if (id < 0) id = genFrom(d, tier);
                pool.clubs.push_back(id);
            }
        }
    }
    for (int d = 0; d < ND; d++) {
        if (districtRegion(d) < 0 || districtRegion(d) >= NUM_METRO_REGIONS) continue;
        for (int k = 0; k < 5; k++) {
            int tier = TD1 + k;
            int q = poolOf(tier, d);
            if (q < 0) continue;
            Pool& pool = P.pools[q];
            int target = pool.nGroups * pool.size;
            while ((int)pool.clubs.size() < target) {
                int id = -1;
                if (r.chance(RES_P[tier])) id = reserveFrom(d, tier);
                if (id < 0) id = genFrom(d, tier);
                pool.clubs.push_back(id);
            }
        }
    }
    for (auto& pool : P.pools) for (int c : pool.clubs) w.teams[c].lastTier = pool.tier;
    // groupes (sauf N1/N2 déjà constitués)
    std::vector<std::vector<int>> keepN1 = P.pools[3].groups, keepN2 = P.pools[4].groups;
    formGroups(P);
    P.pools[3].groups = keepN1; P.pools[4].groups = keepN2;
    g_basePyramids.push_back(P);

    // Outre-mer : ligues régionales indépendantes (pas d'accession au National 2)
    for (int rg = NUM_METRO_REGIONS; rg < NUM_REGIONS; rg++) {
        Pyramid D;
        D.country = "FRA"; D.name = sanitize(REGIONS[rg].name); D.dom = rg;
        TierConf a; a.name = sanitize("Régional 1"); a.scope = SC_REGION; a.groupSize = 14; a.up = 0; a.down = 2; a.tb = TB_FFF;
        TierConf b; b.name = sanitize("Régional 2"); b.scope = SC_REGION; b.groupSize = 12; b.up = 2; b.down = 0; b.flexible = true; b.tb = TB_FFF;
        D.tiers.push_back(a); D.tiers.push_back(b);
        Pool p1; p1.tier = 0; p1.key = rg; p1.nGroups = 1; p1.size = 14;
        Pool p2; p2.tier = 1; p2.key = rg; p2.nGroups = 2; p2.size = 12;
        std::vector<int> depts = deptsOfRegion(rg);
        for (int i = 0; i < 14; i++) p1.clubs.push_back(genFromDept(depts[0], 5));
        for (int i = 0; i < 24; i++) p2.clubs.push_back(genFromDept(depts[0], 6));
        D.pools.push_back(p1); D.pools.push_back(p2);
        for (auto& pool : D.pools) for (int c : pool.clubs) w.teams[c].lastTier = pool.tier;
        formGroups(D);
        g_basePyramids.push_back(D);
    }
}

// barrages : 0 aucun, 1 16e/3e aller-retour, 2 play-offs 3e-6e (Angleterre, Espagne, Italie)
static void buildForeign(World& w) {
    struct Lvl { const char* id; int down; int up; int barrage; int tb; };
    struct Def { const char* country; Lvl lv[4]; };
    static const Def DEFS[] = {
        { "ENG", { { "ENG1", 3, 0, 0, TB_ENG }, { "ENG2", 3, 2, 2, TB_ENG }, { "ENG3", 4, 2, 2, TB_ENG }, { "ENG4", 0, 3, 2, TB_ENG } } },
        { "ESP", { { "ESP1", 3, 0, 0, TB_H2H }, { "ESP2", 0, 2, 2, TB_H2H }, {}, {} } },
        { "ITA", { { "ITA1", 3, 0, 0, TB_H2H }, { "ITA2", 0, 2, 2, TB_H2H }, {}, {} } },
        { "GER", { { "GER1", 2, 0, 0, TB_GD }, { "GER2", 2, 2, 1, TB_GD }, { "GER3", 0, 2, 1, TB_GD }, {} } },
        { "POR", { { "POR1", 0, 0, 0, TB_H2H }, {}, {}, {} } }, { "NED", { { "NED1", 0, 0, 0, TB_GD }, {}, {}, {} } },
        { "BEL", { { "BEL1", 0, 0, 0, TB_GD }, {}, {}, {} } }, { "SCO", { { "SCO1", 0, 0, 0, TB_ENG }, {}, {}, {} } },
        { "TUR", { { "TUR1", 0, 0, 0, TB_H2H }, {}, {}, {} } }, { "AUT", { { "AUT1", 0, 0, 0, TB_GD }, {}, {}, {} } },
        { "SUI", { { "SUI1", 0, 0, 0, TB_GD }, {}, {}, {} } }, { "DEN", { { "DEN1", 0, 0, 0, TB_GD }, {}, {}, {} } },
        { "NOR", { { "NOR1", 0, 0, 0, TB_GD }, {}, {}, {} } }, { "SWE", { { "SWE1", 0, 0, 0, TB_GD }, {}, {}, {} } },
        { "POL", { { "POL1", 0, 0, 0, TB_H2H }, {}, {}, {} } }, { "ROU", { { "ROU1", 0, 0, 0, TB_H2H }, {}, {}, {} } },
        { "IRL", { { "IRL1", 0, 0, 0, TB_GD }, {}, {}, {} } },
        { "USA", { { "USA1", 0, 0, 0, TB_GD }, {}, {}, {} } }, { "ARG", { { "ARG1", 0, 0, 0, TB_GD }, {}, {}, {} } },
        { "BRA", { { "BRA1", 0, 0, 0, TB_GD }, {}, {}, {} } }, { "KSA", { { "KSA1", 0, 0, 0, TB_H2H }, {}, {}, {} } },
        { "KOR", { { "KOR1", 0, 0, 0, TB_GD }, {}, {}, {} } }, { "CHN", { { "CHN1", 0, 0, 0, TB_H2H }, {}, {}, {} } },
        { "AUS", { { "AUS1", 0, 0, 0, TB_GD }, {}, {}, {} } }, { "IND", { { "IND1", 0, 0, 0, TB_GD }, {}, {}, {} } },
    };
    for (auto& d : DEFS) {
        Pyramid P;
        P.country = d.country;
        int nat = w.nationIndex(d.country);
        P.name = w.teams[nat].name;
        for (int lvl = 0; lvl < 4; lvl++) {
            const Lvl& L = d.lv[lvl];
            if (!L.id) break;
            int li = -1;
            for (int i = 0; i < NUM_LEAGUES; i++) if (!strcmp(LEAGUES[i].id, L.id)) li = i;
            if (li < 0) break;
            TierConf t; t.name = sanitize(LEAGUES[li].name); t.scope = SC_NATIONAL; t.groupsPerPool = 1;
            t.groupSize = LEAGUES[li].numClubs; t.up = L.up; t.down = L.down; t.tb = L.tb; t.barrageUp = L.barrage;
            P.tiers.push_back(t);
            Pool p; p.tier = lvl; p.key = 0; p.nGroups = 1; p.size = LEAGUES[li].numClubs; p.clubs = w.leagueClubs[li];
            for (int c : p.clubs) w.teams[c].lastTier = lvl;
            P.pools.push_back(p);
        }
        // les barrages de l'Allemagne : 16e contre 3e
        formGroups(P);
        g_basePyramids.push_back(P);
    }
}

// ------------------------------------------------------------------ jeunes : équipes U19 (youth = 1) et U17 (youth = 2)
int makeU19Team(World& w, int parent) { return makeYouthTeam(w, parent, 1); }
int makeU17Team(World& w, int parent) { return makeYouthTeam(w, parent, 2); }
int makeYouthTeam(World& w, int parent, int kind) {
    const Team& P = w.teams[parent];
    Team t = P;
    const char* suf = kind == 3 ? " U15" : kind == 2 ? " U17" : " U19";
    t.squad.clear(); t.squadGen = false; t.honours.clear(); t.xi.clear();
    t.parent = parent; t.resLevel = kind == 3 ? 11 : kind == 2 ? 10 : 9; t.youth = kind;
    t.name = P.name + suf;
    std::string sn = P.shortName.size() > 5 ? P.shortName.substr(0, 5) : P.shortName;
    t.shortName = sn + suf;
    t.rating = kind == 3 ? std::max(10.f, std::min(55.f, 8.f + P.rating * 0.5f)) : kind == 2 ? std::max(12.f, std::min(62.f, 10.f + P.rating * 0.55f)) : std::max(15.f, std::min(70.f, 14.f + P.rating * 0.6f));
    t.dbClub = -1;
    t.seed = hashStr(t.name) ^ (kind == 3 ? 0x1515 : kind == 2 ? 0x1717 : 0x1919);
    t.stadium = "Stade annexe (" + (P.town.empty() ? P.name : P.town) + ")";
    t.sta = StadiumInfo();
    t.status = CS_AMATEUR;
    t.sponsor.clear();
    t.captainPid = t.vicePid = 0;
    t.custom = false; t.edited = false;
    w.teams.push_back(t);
    return (int)w.teams.size() - 1;
}

// pyramide U19 calquée sur les ligues et districts : National U19 (4 poules de 14), Régional 1 / 2 / 3 U19 selon la taille
// de la ligue, District 1 U19 (et District 2 U19 dans les grands districts)
struct U19Reg { int r1g, r2g, r3g; };
static const U19Reg U19_REG[13] = {
    { 1, 2, 2 },  // Paris Île-de-France
    { 1, 1, 0 },  // Centre-Val de Loire
    { 1, 1, 0 },  // Bourgogne-Franche-Comté
    { 1, 1, 1 },  // Normandie
    { 1, 2, 2 },  // Hauts-de-France
    { 1, 2, 2 },  // Grand Est
    { 1, 1, 1 },  // Pays de la Loire
    { 1, 1, 1 },  // Bretagne
    { 1, 2, 2 },  // Nouvelle-Aquitaine
    { 1, 2, 1 },  // Occitanie
    { 1, 2, 2 },  // Auvergne-Rhône-Alpes
    { 1, 1, 1 },  // Méditerranée
    { 1, 0, 0 },  // Corse
};
static void buildYouthPyr(World& w, int kind) {
    if (g_basePyramids.empty() || g_basePyramids[0].country != "FRA") return;
    const Pyramid& F = g_basePyramids[0];
    Rng r(kind == 3 ? 0x1515AB : kind == 2 ? 0x1717AB : 0x1919AB);
    const std::string A = kind == 3 ? "U15" : kind == 2 ? "U17" : "U19";
    Pyramid P;
    P.country = A; P.name = "France " + A; P.barrageUp = 0;
    auto T = [&](const char* n, int scope, int size, int up, int down) {
        TierConf c; c.name = sanitize(n); c.scope = scope; c.groupsPerPool = -1; c.groupSize = size; c.up = up; c.down = down; c.tb = TB_FFF;
        P.tiers.push_back(c);
    };
    // U15 : pas de championnat national, la ligue est le plus haut niveau
    const int R0 = kind == 3 ? 0 : 1;      // premier niveau régional
    if (kind != 3) T(("National " + A).c_str(), SC_NATIONAL, 14, 0, 3);
    T(("Régional 1 " + A).c_str(), SC_REGION, 12, kind == 3 ? 0 : 1, 2);
    T(("Régional 2 " + A).c_str(), SC_REGION, 12, 1, 2);
    if (kind == 1) T(("Régional 3 " + A).c_str(), SC_REGION, 12, 1, 3);
    const int TD = kind == 1 ? 4 : kind == 2 ? 3 : 2;      // premier niveau de district
    T(("District 1 " + A).c_str(), SC_DEPT, 12, 1, 2);
    T(("District 2 " + A).c_str(), SC_DEPT, 12, 1, 0);
    auto addPool = [&](int tier, int key, int ng, int size) { Pool p; p.tier = tier; p.key = key; p.nGroups = ng; p.size = size; P.pools.push_back(p); return (int)P.pools.size() - 1; };
    if (kind != 3) addPool(0, 0, 4, 14);
    for (int rg = 0; rg < NUM_METRO_REGIONS; rg++) {
        const U19Reg& R = U19_REG[rg];
        int q = addPool(R0, rg, R.r1g, rg == 12 ? 8 : 12);
        if (rg == 12 && kind != 3) P.pools[q].upCap = 0;      // Corse : pas d'accession directe
        if (R.r2g) addPool(R0 + 1, rg, R.r2g, 12);
        if (R.r3g && kind == 1) addPool(3, rg, R.r3g, 12);   // U17 : deux niveaux régionaux
    }
    for (int d = 0; d < numDistricts(); d++) {
        if (districtRegion(d) < 0 || districtRegion(d) >= NUM_METRO_REGIONS) continue;
        bool big = districtPopulation(d) > 800000;
        if (big) { addPool(TD, d, 1, 12); int q = addPool(TD + 1, d, 1, 12); P.pools[q].terminal = true; }
        else { int q = addPool(TD, d, 1, 12); P.pools[q].terminal = true; }
    }
    // clubs candidats : équipes premières métropolitaines, du plus fort au plus faible
    std::vector<std::pair<float, int>> cand;
    for (auto& pl : F.pools) for (int c : pl.clubs) {
        const Team& t = w.teams[c];
        if (t.parent >= 0 || t.region < 0 || t.region >= NUM_METRO_REGIONS) continue;
        cand.push_back({ -(t.rating + (pl.tier <= 2 ? 30.f : 0.f) + (kind >= 2 ? r.frange(-4, 4) : 0.f)), c });
    }
    std::sort(cand.begin(), cand.end());
    std::set<int> used;
    auto give = [&](int club, int q) { int id = makeYouthTeam(w, club, kind); P.pools[q].clubs.push_back(id); used.insert(club); };
    if (kind != 3) for (auto& c : cand) { if ((int)P.pools[0].clubs.size() >= 56) break; give(c.second, 0); }
    for (int tier = R0; tier < TD; tier++)
        for (int rg = 0; rg < NUM_METRO_REGIONS; rg++) {
            int q = P.poolIndex(tier, rg);
            if (q < 0) continue;
            int target = P.pools[q].nGroups * P.pools[q].size;
            for (auto& c : cand) {
                if ((int)P.pools[q].clubs.size() >= target) break;
                if (used.count(c.second) || w.teams[c.second].region != rg) continue;
                give(c.second, q);
            }
        }
    for (int d = 0; d < numDistricts(); d++) {
        if (districtRegion(d) < 0 || districtRegion(d) >= NUM_METRO_REGIONS) continue;
        std::vector<int> v;
        for (auto& c : cand) if (!used.count(c.second) && w.teams[c.second].district == d) v.push_back(c.second);
        int q1 = P.poolIndex(TD, d), q2 = P.poolIndex(TD + 1, d);
        size_t k = 0;
        if (q2 >= 0) { for (; k < v.size() && P.pools[q1].clubs.size() < 12; k++) give(v[k], q1); }
        int qt = q2 >= 0 ? q2 : q1;
        int want = std::max(4, std::min(20, (int)((v.size() - k) * 0.3f)));
        for (; k < v.size() && (int)P.pools[qt].clubs.size() < want; k++) if (P.pools[qt].clubs.size() < 4 || r.chance(0.35f)) give(v[k], qt);
    }
    for (auto& pool : P.pools) for (int c : pool.clubs) w.teams[c].lastTier = pool.tier;
    formGroups(P);
    g_basePyramids.push_back(P);
}

void buildBasePyramids(World& w) {
    g_basePyramids.clear();
    g_usedNames.clear();
    for (auto& t : w.teams) g_usedNames.insert(t.name);
    buildFrance(w);
    buildYouthPyr(w, 1);
    buildYouthPyr(w, 2);
    buildYouthPyr(w, 3);          // U15 : ligues et districts uniquement
    buildForeign(w);
}

std::string poolLabel(const Pyramid& P, const Pool& pl, int g) {
    const TierConf& T = P.tiers[pl.tier];
    std::string s = T.name;
    if (P.dom >= 0) return s + " " + P.name + (pl.groups.size() > 1 ? " - Poule " + std::string(1, (char)('A' + g)) : "");
    int ng = (int)pl.groups.size();
    switch (T.scope) {
    case SC_ZONE: s += " - " + std::string(zoneName(pl.key)); break;
    case SC_REGION: s += " - " + sanitize(REGIONS[pl.key].name); break;
    case SC_DEPT: s += " - " + districtName(pl.key); break;
    default: break;
    }
    if (ng > 1) s += (T.scope == SC_NATIONAL ? " - Groupe " : " - Poule ") + std::string(1, (char)('A' + g));
    return s;
}
