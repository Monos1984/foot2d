// Carrières : saison de clubs (pyramides, coupes, Europe format 2000-01, coefficients UEFA),
// tournois internationaux (CdM 32, Euro 16...), compétitions personnalisées, sauvegarde
#include "game.h"
#include <map>
#include <set>
#include <cstring>

Career g_career;
float frTierBase(int tier);                                  // pyramid.cpp

const char* INTL_NAMES[NUM_INTL] = { "Coupe du Monde", "Euro", "Coupe d'Afrique des Nations", "Copa América",
                                     "Coupe d'Asie", "Gold Cup", "Coupe d'Océanie",
                                     "Tournoi olympique", "Euro Espoirs", "Euro U19", "Euro U17",
                                     "Tournoi olympique féminin", "Euro féminin", "Coupe du Monde féminine" };

bool nationEligible(int n) { return strcmp(NATIONS[n].code, "RUS") != 0; }

// ------------------------------------------------------------------ utilitaires
static std::vector<int> nationsOfConf(int conf) {
    std::vector<int> v;
    for (int i = 0; i < NUM_NATIONS; i++) if (NATIONS[i].conf == conf && nationEligible(i)) v.push_back(i);
    return v;
}

static void sortByRating(std::vector<int>& v) {
    std::stable_sort(v.begin(), v.end(), [](int a, int b) { return g_world.teams[a].rating > g_world.teams[b].rating; });
}

static std::vector<std::vector<int>> potGroups(std::vector<int> teams, int ngroups, int firstFixed = -1) {
    sortByRating(teams);
    std::vector<std::vector<int>> g(ngroups);
    if (firstFixed >= 0) {
        teams.erase(std::remove(teams.begin(), teams.end(), firstFixed), teams.end());
        g[0].push_back(firstFixed);
    }
    int idx = 0;
    while (idx < (int)teams.size()) {
        std::vector<int> pot;
        for (int i = 0; i < ngroups && idx < (int)teams.size(); i++) pot.push_back(teams[idx++]);
        g_rng.shuffle(pot);
        std::vector<int> order(ngroups);
        for (int i = 0; i < ngroups; i++) order[i] = i;
        std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return g[a].size() < g[b].size(); });
        for (int i = 0; i < (int)pot.size(); i++) g[order[i]].push_back(pot[i]);
    }
    return g;
}

static int addComp(Season& S, Competition c) {
    c.season = S.year;
    c.started = true;
    S.comps.push_back(std::move(c));
    return (int)S.comps.size() - 1;
}

int addCompPublic(Season& S, Competition c) { return addComp(S, std::move(c)); }
void coachOnCompDone(Career& K, int comp);                   // coach.cpp
bool cwcYear(int seasonYear);                                // cwc.cpp
void youthQualify(Career& K, int comp);                      // youthintl.cpp
int cwcCreate(Career& K);
void coachSave(Writer& w, const Career& K);
void coachLoad(Reader& r, Career& K);
void archiveSave(Writer& w, const Career& K);             // archive.cpp
void archiveLoad(Reader& r, Career& K);

static std::string seasonLabel(int y) { return fmt("%d-%02d", y, (y + 1) % 100); }

static int clubOf(int t) { const Team& T = g_world.teams[t]; return T.parent >= 0 ? T.parent : t; }

// ------------------------------------------------------------------ carrière : informations
int Career::tierOfTeam(int team, int* pyr, int* pool, int* grp) const {
    if (euroOnly) return -1;          // mode Coupes d'Europe : pas de championnat joué
    for (int p = 0; p < (int)pyramids.size(); p++) {
        const Pyramid& P = pyramids[p];
        for (int q = 0; q < (int)P.pools.size(); q++) {
            const Pool& pl = P.pools[q];
            for (int g = 0; g < (int)pl.groups.size(); g++)
                for (int c : pl.groups[g]) if (c == team) {
                    if (pyr) *pyr = p;
                    if (pool) *pool = q;
                    if (grp) *grp = g;
                    return pl.tier;
                }
        }
    }
    return -1;
}

std::string Career::teamLevelName(int team) const {
    int p, q, g;
    if (tierOfTeam(team, &p, &q, &g) < 0) return "";
    return poolLabel(pyramids[p], pyramids[p].pools[q], g);
}

static int pyramidOf(const std::vector<Pyramid>& P, const std::string& code) {
    for (int i = 0; i < (int)P.size(); i++) if (P[i].country == code && P[i].dom < 0) return i;
    return -1;
}

static bool isEuropean(const std::string& code) {
    int n = g_world.nationIndex(code.c_str());
    return n >= 0 && NATIONS[n].conf == UEFA;
}

// ------------------------------------------------------------------ coefficients UEFA
// coefficients des clubs : estimés d'après le niveau du club et la force de son championnat
static void initClubCoefs(const std::vector<UefaCountry>& uefa) {
    std::map<std::string, float> tot;
    for (auto& u : uefa) tot[u.code] = u.total();
    for (auto& t : g_world.teams) {
        for (float& c : t.coefs) c = 0;
        if (t.kind != TK_CLUB || t.nation < 0 || t.parent >= 0) continue;
        auto it = tot.find(NATIONS[t.nation].code);
        if (it == tot.end()) continue;
        float f = std::max(0.35f, std::min(1.1f, it->second / 100.f));
        float per = std::max(0.f, (t.rating - 64.f) * 0.9f * f);
        if (per <= 0) continue;
        for (int i = 0; i < 5; i++) t.coefs[i] = std::round(per * (0.85f + 0.06f * i) * 10.f) / 10.f;
    }
}
void Career::initUefa() {
    static const struct { const char* c; float tot; } INIT[] = {
        { "ENG", 115 }, { "ITA", 100 }, { "ESP", 95 }, { "GER", 90 }, { "FRA", 78 }, { "NED", 66 }, { "POR", 64 }, { "BEL", 58 },
        { "TUR", 48 }, { "CZE", 45 }, { "GRE", 44 }, { "NOR", 42 }, { "POL", 40 }, { "DEN", 38 }, { "AUT", 36 }, { "SUI", 34 },
        { "SCO", 33 }, { "CYP", 32 }, { "ISR", 31 }, { "CRO", 30 }, { "SWE", 28 }, { "SRB", 28 }, { "HUN", 27 }, { "UKR", 26 },
        { "SVN", 24 }, { "ROU", 24 }, { "AZE", 24 }, { "SVK", 22 }, { "BUL", 21 }, { "KAZ", 19 }, { "MDA", 18 }, { "BIH", 17 },
        { "IRL", 16 }, { "FIN", 16 }, { "NIR", 14 }, { "ARM", 14 }, { "ISL", 14 }, { "KVX", 14 }, { "LVA", 13 }, { "FRO", 12 },
        { "GEO", 12 }, { "MKD", 11 }, { "ALB", 11 }, { "BLR", 11 }, { "LTU", 10 }, { "MLT", 10 }, { "LUX", 10 }, { "MNE", 9 },
        { "EST", 8 }, { "GIB", 7 }, { "WAL", 7 }, { "LIE", 6 }, { "AND", 5 }, { "SMR", 3 },
    };
    uefa.clear();
    for (auto& x : INIT) {
        UefaCountry u; u.code = x.c;
        for (int i = 0; i < 5; i++) u.pts[i] = x.tot / 5.f * (0.9f + 0.05f * i);
        uefa.push_back(u);
    }
    initClubCoefs(uefa);
}

int Career::uefaRank(const std::string& code) const {
    std::vector<std::pair<float, std::string>> v;
    for (auto& u : uefa) v.push_back({ -u.total(), u.code });
    std::sort(v.begin(), v.end());
    for (int i = 0; i < (int)v.size(); i++) if (v[i].second == code) return i + 1;
    return 99;
}

// classement / vainqueur de coupe d'un pays pour l'attribution européenne
struct CountryRes { std::vector<int> standings; int cup = -1; int cupRunner = -1; };

static CountryRes countryResult(const Career& K, const std::string& code, bool useSeason) {
    CountryRes r;
    int p = pyramidOf(K.pyramids, code);
    const Season& S = K.season;
    if (p >= 0) {
        const Pyramid& P = K.pyramids[p];
        int q = P.poolIndex(0, 0);
        if (useSeason && q >= 0 && !P.pools[q].comps.empty() && S.comps[P.pools[q].comps[0]].done) r.standings = S.comps[P.pools[q].comps[0]].result;
        else if (K.euroOnly && q >= 0) {   // mode Coupes d'Europe : championnat simulé (hiérarchie + aléa)
            std::vector<std::pair<float, int>> sc;
            for (int t : P.pools[q].clubs) sc.push_back({ -(g_world.teams[t].rating + g_rng.frange(-6, 6)), t });
            std::sort(sc.begin(), sc.end());
            for (auto& x : sc) r.standings.push_back(x.second);
        }
        else { r.standings = P.pools[q].clubs; sortByRating(r.standings); }
        int cupc = -1;
        if (code == "FRA") cupc = K.cdf;
        else for (int c : K.nationalCups) if (c >= 0 && c < (int)S.comps.size() && S.comps[c].tag == p) cupc = c;
        if (useSeason && cupc >= 0 && S.comps[cupc].winner >= 0) {
            r.cup = S.comps[cupc].winner;
            const Competition& CC = S.comps[cupc];
            if (!CC.stages.empty() && CC.stages.back().ties.size() == 1) { const Tie& t = CC.stages.back().ties[0]; r.cupRunner = t.a == r.cup ? t.b : t.a; }
        }
        else if (r.standings.size() > 3) r.cup = r.standings[g_rng.range(1, 3)];
        // les réserves ne vont pas en Coupe d'Europe
        r.standings.erase(std::remove_if(r.standings.begin(), r.standings.end(), [](int t) { return g_world.teams[t].parent >= 0; }), r.standings.end());
        if (r.cup >= 0 && g_world.teams[r.cup].parent >= 0) r.cup = -1;
        if (r.cupRunner >= 0 && g_world.teams[r.cupRunner].parent >= 0) r.cupRunner = -1;
        return r;
    }
    auto it = g_world.countryClubs.find(code);
    if (it == g_world.countryClubs.end()) return r;
    std::vector<std::pair<float, int>> sc;
    for (int t : it->second) sc.push_back({ -(g_world.teams[t].rating + g_rng.frange(-7, 7)), t });
    std::sort(sc.begin(), sc.end());
    for (auto& x : sc) r.standings.push_back(x.second);
    if (r.standings.size() > 1) r.cup = r.standings[g_rng.range(0, std::min(3, (int)r.standings.size() - 1))];
    else if (!r.standings.empty()) r.cup = r.standings[0];
    if (code == "LIE") r.standings.clear();   // pas de championnat au Liechtenstein
    return r;
}

static void computeEuroNew(Career& K, bool useSeason);
static void computeEuro(Career& K, bool useSeason) {
    if (K.opts.euroFormat) { computeEuroNew(K, useSeason); return; }
    EuroSpots E;
    std::set<int> used;
    std::vector<std::pair<int, std::string>> ranked;
    for (auto& u : K.uefa) ranked.push_back({ K.uefaRank(u.code), u.code });
    std::sort(ranked.begin(), ranked.end());
    std::map<std::string, CountryRes> res;
    for (auto& rk : ranked) res[rk.second] = countryResult(K, rk.second, useSeason);
    auto take = [&](std::vector<int>& v, int t) { if (t >= 0 && !used.count(t)) { v.push_back(t); used.insert(t); } };
    auto place = [&](const std::string& c, int n) { auto& st = res[c].standings; return n - 1 < (int)st.size() ? st[n - 1] : -1; };
    // ---- Ligue des champions (règlement 2003-04)
    // groupes : tenant, champions 1-9, 2es 1-6 ; 3e tour : 3es et 4es 1-3, 3es 4-6, 2es 7-9, champions 10-16 ;
    // 2e tour : 2es 10-15, champions 17-28 ; 1er tour : champions au-delà du 28e rang
    if (K.prevUclWinner >= 0) take(E.uclGS, K.prevUclWinner);
    for (auto& rk : ranked) { int r = rk.first; const std::string& c = rk.second; if (c == "LIE") continue;
        if (r <= 9) take(E.uclGS, place(c, 1)); if (r <= 6) take(E.uclGS, place(c, 2)); }
    for (auto& rk : ranked) { int r = rk.first; const std::string& c = rk.second; if (c == "LIE") continue;
        if (r <= 3) { take(E.uclQ3, place(c, 3)); take(E.uclQ3, place(c, 4)); }
        else if (r <= 6) take(E.uclQ3, place(c, 3));
        else if (r <= 9) take(E.uclQ3, place(c, 2));
        else if (r <= 16) take(E.uclQ3, place(c, 1)); }
    for (auto& rk : ranked) { int r = rk.first; const std::string& c = rk.second; if (c == "LIE") continue;
        if (r >= 10 && r <= 15) take(E.uclQ2, place(c, 2));
        if (r >= 17 && r <= 28) take(E.uclQ2, place(c, 1));
        if (r > 28) take(E.uclQ1, place(c, 1)); }
    // ---- Coupe UEFA (liste d'accès 2002-03) : associations 1 à 6 -> 3 clubs au 1er tour (vainqueur de coupe + championnat ;
    // en France : Coupe de France, Coupe de la Ligue et championnat) ; 7-8 -> 3 au 1er tour + 1 au tour de qualification ;
    // 9-15 -> 1 au 1er tour + 1 au tour de qualification ; 16-18 -> 1 au 1er tour + 2 au tour de qualification ;
    // 19-49 -> 2 au tour de qualification ; au-delà -> 1 ; + 3 invitations fair-play
    auto nextFree = [&](const std::string& c, int from) {
        auto& st = res[c].standings;
        for (int i = from - 1; i < (int)st.size(); i++) if (!used.count(st[i])) return st[i];
        return -1;
    };
    if (K.prevUefaWinner >= 0) take(E.uefaR1, K.prevUefaWinner);
    std::vector<int> qrP[4];
    auto takeQ = [&](int pri, int t) { if (t >= 0 && !used.count(t)) { qrP[pri].push_back(t); used.insert(t); } };
    for (auto& rk : ranked) {
        int r = rk.first; const std::string& c = rk.second;
        int cup = res[c].cup;
        // vainqueur de la coupe déjà qualifié pour la Ligue des champions : la place revient au finaliste
        if (cup >= 0 && used.count(cup)) { int rn = res[c].cupRunner; cup = rn >= 0 && !used.count(rn) ? rn : -1; }
        if (c == "LIE") { takeQ(0, cup); continue; }          // Liechtenstein : pas de championnat, le vainqueur de la coupe (1 place)
        int nR1 = r <= 8 ? 3 : r <= 18 ? 1 : 0;
        int nQ = r <= 6 ? 0 : r <= 15 ? 1 : r <= 18 ? 2 : r <= 49 ? 2 : 1;
        std::vector<int> ent;                              // clubs de l'association par ordre de priorité
        int first = cup >= 0 ? cup : nextFree(c, 2);
        if (first >= 0) { ent.push_back(first); used.insert(first); }
        // France : le vainqueur de la Coupe de la Ligue prend une des places
        if (c == "FRA" && useSeason && K.cdl >= 0 && K.cdl < (int)K.season.comps.size()) {
            int w = K.season.comps[K.cdl].winner;
            if (w >= 0 && !used.count(w) && g_world.teams[w].parent < 0) { ent.push_back(w); used.insert(w); }
        }
        while ((int)ent.size() < nR1 + nQ) { int t = nextFree(c, 2); if (t < 0) break; ent.push_back(t); used.insert(t); }
        for (int t : ent) used.erase(t);
        for (int k = 0; k < (int)ent.size(); k++) {
            if (k < nR1) take(E.uefaR1, ent[k]);
            else takeQ(std::min(2, k - nR1), ent[k]);
        }
    }
    {   // 3 invitations fair-play (associations tirées au sort parmi les 30 premières)
        std::vector<std::string> fp;
        for (auto& rk : ranked) if (rk.first <= 30 && rk.second != "LIE") fp.push_back(rk.second);
        for (int k = 0; k < 3 && !fp.empty(); k++) {
            int i = (int)(g_rng.next() % fp.size());
            takeQ(1, nextFree(fp[i], 2));
            fp.erase(fp.begin() + i);
        }
    }
    // ---- équilibrage : C1 groupes 16, Q3 18, Q2 18, Q1 20 ; C3 1er tour 36 directs, tour de qualification 82
    {
        std::vector<int> all;
        for (int t : E.uclGS) all.push_back(t);
        for (int t : E.uclQ3) all.push_back(t);
        for (int t : E.uclQ2) all.push_back(t);
        for (int t : E.uclQ1) all.push_back(t);
        E.uclGS.clear(); E.uclQ3.clear(); E.uclQ2.clear(); E.uclQ1.clear();
        std::vector<int> over;
        for (size_t i = 0; i < all.size(); i++) {
            if (i < 16) E.uclGS.push_back(all[i]); else if (i < 34) E.uclQ3.push_back(all[i]); else if (i < 52) E.uclQ2.push_back(all[i]);
            else if (i < 72) E.uclQ1.push_back(all[i]); else over.push_back(all[i]);
        }
        std::vector<int> r1 = E.uefaR1;
        E.uefaR1.clear(); E.uefaQR.clear();
        for (size_t i = 0; i < r1.size(); i++) { if (i < 36) E.uefaR1.push_back(r1[i]); else qrP[1].insert(qrP[1].begin(), r1[i]); }
        for (int t : over) qrP[3].push_back(t);
        std::vector<int> dropped;
        for (int pri = 0; pri < 4; pri++) for (int t : qrP[pri]) { if (E.uefaQR.size() < 82) E.uefaQR.push_back(t); else dropped.push_back(t); }
        for (int t : dropped) used.erase(t);     // clubs non retenus : disponibles pour l'Intertoto
        // compléter si besoin (associations les mieux classées)
        for (int pass = 0; pass < 3 && (E.uefaR1.size() < 36 || E.uefaQR.size() < 82); pass++)
            for (auto& rk : ranked) {
                if (rk.second == "LIE") continue;
                if (E.uefaR1.size() < 36) take(E.uefaR1, nextFree(rk.second, 2));
                else if (E.uefaQR.size() < 82) take(E.uefaQR, nextFree(rk.second, 2));
            }
    }
    // ---- Coupe Intertoto (60 clubs) : 3e tour 8 clubs (associations 1 à 8), 2e tour 12 clubs (associations 1 à 12),
    // 1er tour 40 clubs (un club par association à partir de la 13e, le Liechtenstein n'a pas de championnat)
    {
        auto itFree = [&](const std::string& c) { int t = nextFree(c, 2); return t >= 0 && g_world.teams[t].parent < 0 ? t : -1; };
        for (auto& rk : ranked) if (rk.first <= 8 && rk.second != "LIE") take(E.itR3, itFree(rk.second));
        for (auto& rk : ranked) if (rk.first <= 12 && rk.second != "LIE") take(E.itR2, itFree(rk.second));
        for (auto& rk : ranked) { if (E.itR1.size() >= 40) break; if (rk.first >= 13 && rk.second != "LIE") take(E.itR1, itFree(rk.second)); }
        // association sans club disponible : la place revient aux associations suivantes (2e club)
        for (auto& rk : ranked) { if (E.itR3.size() >= 8) break; if (rk.first > 8 && rk.second != "LIE") take(E.itR3, itFree(rk.second)); }
        for (auto& rk : ranked) { if (E.itR2.size() >= 12) break; if (rk.first > 12 && rk.second != "LIE") take(E.itR2, itFree(rk.second)); }
        for (auto& rk : ranked) { if (E.itR1.size() >= 40) break; if (rk.first >= 13 && rk.second != "LIE") take(E.itR1, itFree(rk.second)); }
    }
    if (K.euroOnly) {   // pas d'Intertoto : ses trois places en Coupe UEFA reviennent aux clubs suivants
        for (int k = 0; k < 3 && k < (int)E.itR3.size(); k++) E.uefaR1.push_back(E.itR3[k]);
        E.itR1.clear(); E.itR2.clear(); E.itR3.clear();
    }
    K.nextEuro = E;
}


// ------------------------------------------------------------------ nouvelle formule (2024-25) : listes d'accès simplifiées
static void computeEuroNew(Career& K, bool useSeason) {
    NewEuroSpots N;
    std::set<int> used;
    std::vector<std::pair<int, std::string>> ranked;
    for (auto& u : K.uefa) ranked.push_back({ K.uefaRank(u.code), u.code });
    std::sort(ranked.begin(), ranked.end());
    std::map<std::string, CountryRes> res;
    for (auto& rk : ranked) res[rk.second] = countryResult(K, rk.second, useSeason);
    auto take = [&](int li, int t) { if (t >= 0 && !used.count(t) && g_world.teams[t].parent < 0) { N.l[li].push_back(t); used.insert(t); } };
    auto nextFree = [&](const std::string& c) {
        auto& st = res[c].standings;
        for (int t : st) if (!used.count(t) && g_world.teams[t].parent < 0) return t;
        return -1;
    };
    auto cupOf = [&](const std::string& c) {
        int cup = res[c].cup;
        if (cup >= 0 && used.count(cup)) { int rn = res[c].cupRunner; cup = rn >= 0 && !used.count(rn) ? rn : -1; }
        return cup >= 0 ? cup : nextFree(c);
    };
    // ---- Ligue des champions
    if (K.prevUclWinner >= 0) take(4, K.prevUclWinner);
    if (K.prevUefaWinner >= 0) take(4, K.prevUefaWinner);
    for (auto& rk : ranked) {
        int r = rk.first; const std::string& c = rk.second;
        if (c == "LIE") continue;
        int ch = res[c].standings.empty() ? -1 : res[c].standings[0];
        if (ch >= 0 && g_world.teams[ch].parent < 0) N.l[5].push_back(ch);
        int li = r <= 10 ? 4 : r <= 12 ? 3 : r <= 15 ? 2 : r <= 28 ? 1 : 0;
        take(li, ch >= 0 && !used.count(ch) ? ch : nextFree(c));
        int extra = (r <= 6) + (r <= 5) + (r <= 4);
        for (int k = 0; k < extra; k++) take(4, nextFree(c));
    }
    {   // deux places « performance européenne » : les deux meilleures associations de la saison écoulée
        std::vector<std::pair<float, std::string>> last;
        for (auto& u : K.uefa) if (u.code != "LIE") last.push_back({ -u.pts[4], u.code });
        std::sort(last.begin(), last.end());
        for (int k = 0; k < 2 && k < (int)last.size(); k++) take(4, nextFree(last[k].second));
    }
    for (auto& rk : ranked) {           // voie de la ligue
        int r = rk.first; const std::string& c = rk.second;
        if (c == "LIE") continue;
        if (r >= 5 && r <= 9) take(2, nextFree(c));
        else if (r >= 10 && r <= 15) take(1, nextFree(c));
    }
    // ---- Ligue Europa
    for (auto& rk : ranked) {
        int r = rk.first; const std::string& c = rk.second;
        if (c == "LIE") continue;
        if (r <= 12) take(9, cupOf(c)); else if (r <= 16) take(7, cupOf(c));
    }
    if (K.prevUeclWinner >= 0) take(9, K.prevUeclWinner);
    for (auto& rk : ranked) if (rk.first <= 5) take(9, nextFree(rk.second));
    // ---- Ligue Conférence
    for (auto& rk : ranked) {
        int r = rk.first; const std::string& c = rk.second;
        if (c == "LIE") { take(11, res[c].cup); continue; }
        if (r <= 6) take(15, nextFree(c));
        else if (r <= 9) { take(14, nextFree(c)); take(14, nextFree(c)); }
        else if (r <= 12) { take(13, nextFree(c)); take(13, nextFree(c)); }
        else if (r <= 16) take(12, nextFree(c));
        else if (r <= 33) { take(12, cupOf(c)); take(12, nextFree(c)); if (r <= 20) take(12, nextFree(c)); }
        else { take(11, cupOf(c)); take(11, nextFree(c)); }
    }
    // réserves (complètent une phase de ligue incomplète) : clubs suivants des meilleures associations
    for (int li : { 6, 10, 16 }) {
        for (auto& rk : ranked) {
            if (rk.second == "LIE" || (int)N.l[li].size() >= 12) continue;
            for (int t : res[rk.second].standings) if (!used.count(t) && g_world.teams[t].parent < 0 && std::find(N.l[li].begin(), N.l[li].end(), t) == N.l[li].end()) { N.l[li].push_back(t); break; }
        }
    }
    K.newEuro = N;
    // équivalent « ancienne formule » pour les écrans de synthèse (C1 / C3 / C4)
    EuroSpots E;
    E.uclGS = N.l[4]; E.uclQ3 = N.l[2]; for (int t : N.l[3]) E.uclQ3.push_back(t); E.uclQ2 = N.l[1]; E.uclQ1 = N.l[0];
    E.uefaR1 = N.l[9]; E.uefaQR = N.l[7];
    for (int li = 11; li <= 15; li++) for (int t : N.l[li]) E.itR1.push_back(t);
    K.nextEuro = E;
}

void Career::addNewEuroCups() {
    Season& S = season;
    NewEuroSpots& N = newEuro;
    bool ag = opts.awayGoals != 0;
    {
        Competition c;
        c.format = FMT_NEWEURO; c.name = "Ligue des champions"; c.shortName = "C1"; c.kind = 3; c.tb = TB_GD; c.neutralFinal = true; c.awayGoals = ag;
        c.host = prevUclWinner;
        c.regionalRounds = 4; c.swissRounds = 8; c.qualSpots = 4;
        c.koNames = { "1er tour de qualification", "2e tour de qualification", "3e tour de qualification", "Tour de barrage" };
        c.koTimes = { -2.6, -0.6, 1.2, 3.0, 7.0, 9.2, 12.0, 14.2, 18.0, 19.2, 24.0, 25.2, 28.2, 31.2, 35.0, 38.5, 43.0 };
        c.entrants = { N.l[0], N.l[1], N.l[2], N.l[3], N.l[4], N.l[6] };
        c.extra2 = N.l[5];
        ucl = addComp(S, std::move(c));
        newEuroStart(S.comps[ucl], 0, {});
    }
    {
        Competition c;
        c.format = FMT_NEWEURO; c.name = "Ligue Europa"; c.shortName = "C3"; c.kind = 8; c.tb = TB_GD; c.neutralFinal = true; c.awayGoals = ag;
        c.host = prevUefaWinner;
        c.regionalRounds = 2; c.swissRounds = 8; c.qualSpots = 4;
        c.koNames = { "3e tour de qualification", "Tour de barrage" };
        c.koTimes = { 1.4, 3.2, 7.4, 9.4, 12.4, 14.4, 18.4, 19.4, 24.4, 25.4, 28.4, 31.4, 35.2, 38.7, 42.0 };
        c.entrants = { N.l[7], N.l[8], N.l[9], N.l[10] };
        c.qualPlayoff = 1 | 2 | 4;           // perdants des 2e et 3e tours et des barrages de la C1
        uel = addComp(S, std::move(c));
        newEuroStart(S.comps[uel], 0, {});
    }
    {
        Competition c;
        c.format = FMT_NEWEURO; c.name = "Ligue Conférence"; c.shortName = "C4"; c.kind = 9; c.tb = TB_GD; c.neutralFinal = true; c.awayGoals = ag;
        c.host = prevUeclWinner;
        c.regionalRounds = 4; c.swissRounds = 6; c.qualSpots = 6;
        c.koNames = { "1er tour de qualification", "2e tour de qualification", "3e tour de qualification", "Tour de barrage" };
        c.koTimes = { -2.4, -0.4, 1.6, 3.4, 7.6, 9.6, 12.6, 14.6, 18.6, 19.6, 28.6, 31.6, 35.4, 38.9, 41.6 };
        c.entrants = { N.l[11], N.l[12], N.l[13], N.l[14], N.l[15], N.l[16] };
        c.qualPlayoff = 2 | 8 | 16;          // perdants du 1er tour de C1, du 3e tour et des barrages de C3
        uecl = addComp(S, std::move(c));
        newEuroStart(S.comps[uecl], 0, {});
    }
    routeNewEuro();
}

// perdants reversés dans la compétition inférieure
void Career::routeNewEuro() {
    Season& S = season;
    if (ucl < 0 || uel < 0 || uecl < 0) return;
    if (S.comps[ucl].format != FMT_NEWEURO) return;
    struct Rt { int src, sk, dst, dk; };
    const Rt R[] = { { ucl, 0, uecl, 1 }, { ucl, 1, uel, 0 }, { ucl, 2, uel, 1 }, { ucl, 3, uel, 2 }, { uel, 0, uecl, 3 }, { uel, 1, uecl, 4 } };
    bool changed = true;
    for (int guard = 0; changed && guard < 20; guard++) {
        changed = false;
        for (auto& r : R) {
            Competition& A = S.comps[r.src];
            Competition& B = S.comps[r.dst];
            if (!newEuroRoundDone(A, r.sk) || ((B.extReadyMask >> r.dk) & 1)) continue;
            if ((int)B.entrants.size() <= r.dk) B.entrants.resize(r.dk + 1);
            auto L = newEuroLosers(A, r.sk);
            for (int t : L) B.entrants[r.dk].push_back(t);
            B.extReadyMask |= 1 << r.dk;
            if (B.awaiting == r.dk) B.resume();
            changed = true;
        }
    }
}

// ------------------------------------------------------------------ nouvelle carrière club
void placeInBottomPool(Pyramid& P, int team) {
    Team& t = g_world.teams[team];
    if (t.district < 0 && t.parent >= 0) t.district = g_world.teams[t.parent].district;     // réserve : district de l'équipe fanion
    int best = -1, bt = -1;
    for (int q = 0; q < (int)P.pools.size(); q++) {
        const Pool& pl = P.pools[q];
        if (P.tiers[pl.tier].scope == SC_DEPT && pl.key == t.district && pl.tier > bt) { bt = pl.tier; best = q; }
    }
    if (best < 0) for (int q = 0; q < (int)P.pools.size(); q++) if (P.pools[q].tier > bt) { bt = P.pools[q].tier; best = q; }
    if (best < 0) return;
    P.pools[best].clubs.push_back(team);
    if (P.pools[best].groups.empty()) P.pools[best].groups.push_back({});
    // groupe le moins rempli
    int gi = 0;
    for (int g = 1; g < (int)P.pools[best].groups.size(); g++) if (P.pools[best].groups[g].size() < P.pools[best].groups[gi].size()) gi = g;
    P.pools[best].groups[gi].push_back(team);
    g_world.teams[team].lastTier = P.pools[best].tier;
}

void Career::addClubToPyramid(int team) {
    if (kind == CK_CLUB && !season.comps.empty()) { pendingNewClubs.push_back(team); return; }
    int p = pyramidOf(pyramids, "FRA");
    if (p >= 0) placeInBottomPool(pyramids[p], team);
}

void Career::newClubCareer(int team, int yr) {
    coach = false;
    kind = CK_CLUB;
    { std::string mn = managerName; int mnat = managerNation; uint8_t ms = managerSkin, mh = managerHair; int ma = managerAge;
      resetV7(); managerName = mn; managerNation = mnat; managerSkin = ms; managerHair = mh; managerAge = ma; }
    userTeam = team;
    year = yr;
    pyramids = g_basePyramids;
    history.clear();
    pendingNewClubs.clear(); lastU19Champ = youthPrelim = youthUcl = -1; youthDirect.clear(); reconversions.clear();
    season = Season();
    season.controlled = { team };
    season.year = yr;
    season.mode = 0;
    prevUclWinner = prevUefaWinner = -1;
    prevChampion.clear(); prevCupWinner.clear(); prevRunnerUp.clear(); prevRegCupWinner.clear(); superRegions = -1;
    initUefa();
    // clubs créés avec l'éditeur : dernière division de leur district
    int fr = pyramidOf(pyramids, "FRA");
    for (int i = g_world.baseCount; i < (int)g_world.teams.size(); i++)
        if (g_world.teams[i].custom && g_world.teams[i].dept >= 0 && fr >= 0 && tierOfTeam(i) < 0) placeInBottomPool(pyramids[fr], i);
    // équipes U19 des clubs créés : District U19 de leur département
    { int yp = u19Pyramid(); for (int i = g_world.baseCount; i < (int)g_world.teams.size(); i++)
        if (g_world.teams[i].youth && g_world.teams[i].youth != 6 && tierOfTeam(i) < 0) { int py = youthPyramid(i); if (py >= 0) placeInBottomPool(pyramids[py], i); } }
    // tenants réels 2025-26 : Ligue des champions PSG, Ligue Europa Aston Villa
    auto byName = [](const char* nm) { for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == nm && g_world.teams[i].parent < 0) return i; return -1; };
    prevUclWinner = byName("Paris Saint-Germain");
    prevUefaWinner = byName("Aston Villa");
    computeEuro(*this, false);
    // tenants 2025-26 estimés : champion = meilleur club, coupe = deuxième
    for (auto& P : pyramids) {
        if (P.dom >= 0) continue;
        int q = P.poolIndex(0, 0);
        if (q < 0) continue;
        std::vector<int> s = P.pools[q].clubs; sortByRating(s);
        if (s.size() >= 3) { prevChampion[P.country] = s[0]; prevCupWinner[P.country] = s[1]; prevRunnerUp[P.country] = s[2]; }
    }
    // France 2025-26 : champion PSG, Coupe de France RC Lens
    { int a = byName("Paris Saint-Germain"), b = byName("RC Lens"); if (a >= 0) prevChampion["FRA"] = a; if (b >= 0) prevCupWinner["FRA"] = b; }
    mgr = ManagerState();
    startSeason();
    mgrInit();
    refreshFreeAgents(40);
    initStaff();
}

static const char* cupNameFor(const std::string& c) {
    if (c == "ENG") return "FA Cup";
    if (c == "ESP") return "Copa del Rey";
    if (c == "ITA") return "Coppa Italia";
    if (c == "GER") return "DFB-Pokal";
    if (c == "POR") return "Taça de Portugal";
    if (c == "NED") return "KNVB Beker";
    if (c == "BEL") return "Coupe de Belgique";
    if (c == "SCO") return "Scottish Cup";
    if (c == "TUR") return "Coupe de Turquie";
    if (c == "AUT") return "Coupe d'Autriche";
    if (c == "SUI") return "Coupe de Suisse";
    if (c == "DEN") return "Coupe du Danemark";
    if (c == "NOR") return "Coupe de Norvège";
    if (c == "SWE") return "Coupe de Suède";
    if (c == "POL") return "Coupe de Pologne";
    if (c == "ROU") return "Coupe de Roumanie";
    if (c == "IRL") return "FAI Cup";
    if (const char* e = extCupName(c.c_str())) return e;
    static std::map<std::string, std::string> gen;     // coupe créée automatiquement
    auto it = gen.find(c);
    if (it == gen.end()) {
        int n = g_world.nationIndex(c.c_str());
        std::string nm = n >= 0 ? sanitize(NATIONS[n].name) : c;
        bool vowel = !nm.empty() && strchr("AEIOUÉÎaeiou", nm[0]);
        it = gen.emplace(c, (vowel ? "Coupe d'" : "Coupe de ") + nm).first;
    }
    return it->second.c_str();
}
static const char* superNameFor(const std::string& c) {
    if (c == "FRA") return "Trophée des Champions";
    if (c == "ENG") return "Community Shield";
    if (c == "ESP") return "Supercopa de España";
    if (c == "ITA") return "Supercoppa Italiana";
    if (c == "GER") return "DFL-Supercup";
    if (c == "POR") return "Supertaça";
    if (c == "NED") return "Johan Cruijff Schaal";
    if (c == "TUR") return "Supercoupe de Turquie";
    return nullptr;
}

// tours d'une coupe classique : tout le monde au 1er tour, puissance de 2 ensuite
static std::string neutralVenue(const std::vector<int>& teams) {
    std::vector<int> v; for (int t : teams) if (g_world.teams[t].parent < 0) v.push_back(t);
    if (v.empty()) return "";
    std::stable_sort(v.begin(), v.end(), [](int a, int b) { return g_world.teams[a].rating > g_world.teams[b].rating; });
    v.resize(std::max<size_t>(1, v.size() / 3));          // un club parmi les mieux équipés de la zone
    const Team& T = g_world.teams[v[g_rng.next() % v.size()]];
    return T.stadium + (T.town.empty() ? std::string() : " (" + T.town + ")");
}

static void simpleCup(Competition& c, const std::vector<int>& teams, double t0, double t1) {
    c.format = FMT_CUP;
    c.entrants.assign(1, teams);
    int n = (int)teams.size();
    int pw = 1; while (pw * 2 <= n) pw *= 2;
    if (pw == n) pw /= 2;
    c.koTargets.clear(); c.koTimes.clear();
    for (int x = pw; x >= 1; x /= 2) c.koTargets.push_back(x);
    int R = (int)c.koTargets.size();
    for (int i = 0; i < R; i++) c.koTimes.push_back(t0 + (t1 - t0) * i / std::max(1, R - 1));
}

// Coupe de France : tours régionaux d'une ligue (entrées : amateurs au 1er tour ou au tour préliminaire,
// N2 au 3e tour, N1 au 4e tour, Ligue 3 au 5e tour ; qualifiés pour le 7e tour = quota de la ligue)
// règlement des ligues : tour d'entrée (1 = 1er tour) des R1, R2, R3 et D1 ; les autres divisions de district
// jouent le(s) tour(s) préliminaire(s) si nécessaire
static const int CDF_ENTRY[13][4] = {
    { 3, 2, 2, 1 },   // Île-de-France
    { 2, 2, 1, 1 },   // Centre-Val de Loire
    { 3, 2, 1, 1 },   // Bourgogne-Franche-Comté
    { 3, 2, 1, 1 },   // Normandie
    { 3, 2, 1, 1 },   // Hauts-de-France
    { 3, 2, 1, 1 },   // Grand Est
    { 3, 2, 1, 1 },   // Pays de la Loire
    { 3, 2, 1, 1 },   // Bretagne
    { 3, 2, 1, 1 },   // Nouvelle-Aquitaine
    { 2, 2, 1, 1 },   // Occitanie
    { 3, 2, 1, 1 },   // Auvergne-Rhône-Alpes
    { 2, 1, 1, 1 },   // Provence-Alpes-Côte d'Azur
    { 1, 1, 1, 1 },   // Corse
};
std::string cdfEntryText(int region) {
    if (region < 0 || region >= 13) return "";
    const int* e = CDF_ENTRY[region];
    auto tr = [](int k) { return k == 1 ? std::string("1er tour") : fmt("%de tour", k); };
    return "Entrées (règlement de la ligue) : districts D2 et moins au tour préliminaire (si nécessaire), D1 au " + tr(e[3]) +
           ", R3 au " + tr(e[2]) + ", R2 au " + tr(e[1]) + ", R1 au " + tr(e[0]) + ", National 2 au 3e tour, National 1 au 4e tour, Ligue 3 au 5e tour. "
           "Les qualifiés du 6e tour rejoignent la phase nationale (7e tour, puis entrée de la Ligue 2 et de la Ligue 1 en 32es).";
}

// Coupe de France : tours régionaux d'une ligue. lv[0..4] = R1, R2, R3, D1, districts inférieurs
static void buildCdfRegional(Competition& c, int region, std::vector<int> lv[5], const std::vector<int>& n2, const std::vector<int>& n1, const std::vector<int>& l3, int Q) {
    c.format = FMT_CUP; c.kind = 2; c.regionalDraw = true; c.regionalRounds = 99; c.noReserves = true; c.neutralFinal = false; c.homeRule = 1;
    const int* ent = CDF_ENTRY[std::max(0, std::min(12, region))];
    std::vector<std::vector<int>> E(7);            // E[1..6] : entrants par tour
    for (int k = 0; k < 4; k++) for (int t : lv[k]) E[ent[k]].push_back(t);
    for (int t : n2) E[3].push_back(t);
    for (int t : n1) E[4].push_back(t);
    for (int t : l3) E[5].push_back(t);
    auto byRating = [](std::vector<int>& v) { std::stable_sort(v.begin(), v.end(), [](int a, int b) { return g_world.teams[a].rating > g_world.teams[b].rating; }); };
    std::vector<int> out(7, 0);
    out[6] = Q;
    for (int k = 6; k >= 2; k--) {
        int allowed = 2 * out[k];
        byRating(E[k]);
        // trop d'équipes à ce tour : les plus faibles entrent un tour plus tôt
        while ((int)E[k].size() > allowed - 1 && !E[k].empty()) { E[k - 1].push_back(E[k].back()); E[k].pop_back(); }
        out[k - 1] = std::max(1, allowed - (int)E[k].size());
    }
    int in1 = 2 * out[1];
    byRating(E[1]);
    std::vector<int> pre = lv[4];
    // Grand Est : R3 et toutes les divisions de district dès le 1er tour, R2 au 2e tour, R1 au 3e tour
    if (region == 5) { for (int t : pre) E[1].push_back(t); pre.clear(); byRating(E[1]); }
    while ((int)E[1].size() > in1 - 1 && !E[1].empty()) { pre.push_back(E[1].back()); E[1].pop_back(); }
    int needFromPre = in1 - (int)E[1].size();
    std::vector<int> prelimTargets;
    if ((int)pre.size() <= needFromPre) { for (int t : pre) E[1].push_back(t); pre.clear(); }
    else {
        int cur = (int)pre.size();
        while (cur > needFromPre) { int t = std::max(needFromPre, (cur + 1) / 2); prelimTargets.push_back(t); cur = t; }
    }
    int np = (int)prelimTargets.size();
    std::vector<int> targets = prelimTargets;
    for (int k = 1; k <= 6; k++) targets.push_back(out[k]);
    c.koTargets = targets;
    c.entrants.assign(targets.size(), {});
    if (np > 0) { c.entrants[0] = pre; for (int k = 1; k <= 6; k++) c.entrants[np + k - 1] = E[k]; }
    else for (int k = 1; k <= 6; k++) c.entrants[k - 1] = E[k];
    c.koNames.clear(); c.koTimes.clear();
    for (int i = 0; i < np; i++) { c.koNames.push_back(np > 1 ? fmt("Tour préliminaire %d", i + 1) : "Tour préliminaire"); c.koTimes.push_back(0.8 + i * 0.7); }
    const double TT[6] = { 2.5, 4.5, 6.5, 8.5, 10.5, 12.5 };
    for (int i = 0; i < 6; i++) { c.koNames.push_back(i == 0 ? "1er tour" : fmt("%de tour", i + 1)); c.koTimes.push_back(TT[i]); }
}

// niveau de chaque équipe dans sa pyramide (règle de réception en coupe)
static std::vector<int16_t> g_levelCache;
static void rebuildLevelCache(const Career& K) {
    g_levelCache.assign(g_world.teams.size(), 99);
    for (auto& P : K.pyramids) for (auto& pl : P.pools) {
        int lv = P.dom >= 0 || P.country == "U19" || P.country == "U17" || P.country == "U15" ? pl.tier + 5 : pl.tier;
        for (int t : pl.clubs) if (t < (int)g_levelCache.size()) g_levelCache[t] = (int16_t)lv;
    }
    // représentants d'outre-mer : niveau régional
    for (int i = 0; i < 3; i++) if (g_world.omReps[i] >= 0 && g_world.omReps[i] < (int)g_levelCache.size()) g_levelCache[g_world.omReps[i]] = 5;
}
int teamLevel(int team) { return team >= 0 && team < (int)g_levelCache.size() ? g_levelCache[team] : 99; }

void Career::startSeason() {
    Season& S = season;
    cupVenue.clear();
    rebuildLevelCache(*this);
    S.comps.clear();
    S.comps.reserve(8000);
    S.now = 0; S.finished = false; S.year = year;
    // intersaison : tout le monde est reposé
    for (auto& t : g_world.teams) { t.condT = -1; for (auto& p : t.squad) p.cond = 100; }
    S.news.clear();
    // ---------------- championnats (pas en mode Coupes d'Europe)
    if (euroOnly) for (auto& P : pyramids) for (auto& pl : P.pools) pl.comps.clear();
    for (int p = 0; !euroOnly && p < (int)pyramids.size(); p++) {
        Pyramid& P = pyramids[p];
        for (int q = 0; q < (int)P.pools.size(); q++) {
            Pool& pl = P.pools[q];
            pl.comps.clear();
            for (int g = 0; g < (int)pl.groups.size(); g++) {
                Competition c;
                c.format = FMT_LEAGUE;
                c.name = poolLabel(P, pl, g);
                c.shortName = P.tiers[pl.tier].name;
                c.kind = 1;
                c.tb = P.tiers[pl.tier].tb;
                c.tag = p * 100000 + q * 100 + g;
                c.yellowLimit = P.country == "FRA" ? 3 : 5;
                int n = (int)pl.groups[g].size();
                double t0 = 1.0, t1 = 40.0;
                bool yth = P.country == "U19" || P.country == "U17" || P.country == "U15";
                bool fem = isWomenPyramid(P);
                bool pro = (P.country != "FRA" && !yth && !fem) || (P.country == "FRA" && pl.tier <= 2) || (fem && pl.tier <= 1);
                if (!pro) { t0 = 2; t1 = n <= 10 ? 36 : 38; }
                if (yth) { t0 = 3; t1 = pl.tier == 0 ? 35 : 36; c.yellowLimit = 3; }
                if (fem) { t0 = 2; t1 = P.country == "F:FRA" && pl.tier == 0 ? 36 : 38; c.yellowLimit = P.country == "F:FRA" ? 3 : 5; }
                c.setupLeague(pl.groups[g], 2, t0, t1, 1);
                if (pl.groups[g].size() < 2) { c.done = true; c.result = pl.groups[g]; for (auto& st : c.stages) st.finished = true; }   // poule vide : rien à jouer
                pl.comps.push_back(addComp(S, std::move(c)));
            }
        }
    }
    // ---------------- Coupe de France (tours régionaux par ligue)
    cdf = -1; cdfNational = -1; cdfRegional.clear(); regionalCups.clear(); deptCups.clear(); superCups.clear(); nationalCups.clear();
    int fr = pyramidOf(pyramids, "FRA");
    if (fr >= 0 && !euroOnly) {
        Pyramid& P = pyramids[fr];
        std::vector<std::vector<int>> am(13), n2(13), n1(13), l3(13);
        std::vector<int> lvR[13][5];
        // tenant du titre hors Ligue 1 : qualifié directement pour les 32es de finale
        cdfHolderDirect = -1;
        {
            auto it = prevCupWinner.find("FRA");
            if (it != prevCupWinner.end() && it->second >= 0 && g_world.teams[it->second].parent < 0) {
                int h = it->second;
                bool inL1 = false;
                for (auto& pl : P.pools) if (pl.tier == 0) for (int t : pl.clubs) if (t == h) inL1 = true;
                if (!inL1 && tierOfTeam(h) >= 0) {
                    cdfHolderDirect = h;
                    S.news.push_back(g_world.teams[h].name + ", tenant de la Coupe de France, est qualifié directement pour les 32es de finale.");
                }
            }
        }
        for (auto& pl : P.pools) for (int t : pl.clubs) {
            const Team& T = g_world.teams[t];
            if (T.parent >= 0 || T.region < 0 || T.region >= 13) continue;   // pas de réserves
            if (t == cdfHolderDirect) continue;
            if (pl.tier <= 1) continue;
            if (pl.tier == 2) l3[T.region].push_back(t);
            else if (pl.tier == 3) n1[T.region].push_back(t);
            else if (pl.tier == 4) n2[T.region].push_back(t);
            else {
                // divisions de district : inscription facultative (le club du joueur choisit en début de saison)
                if (pl.tier >= 8 && t != userTeam) {
                    static const float REG[5] = { 0.85f, 0.7f, 0.55f, 0.42f, 0.35f };
                    if (!g_rng.chance(REG[std::min(4, pl.tier - 8)])) continue;
                }
                am[T.region].push_back(t);
                int k = pl.tier == 5 ? 0 : pl.tier == 6 ? 1 : pl.tier == 7 ? 2 : pl.tier == 8 ? 3 : 4;
                lvR[T.region][k].push_back(t);
            }
        }
        // Saint-Pierre-et-Miquelon : représentant au 3e tour (ligue de Normandie)
        if (g_world.omReps[2] >= 0) n2[3].push_back(g_world.omReps[2]);
        int total = 0; for (int r = 0; r < 13; r++) total += (int)(am[r].size() + n2[r].size() + n1[r].size() + l3[r].size());
        // 7e tour = 4 x (64 - clubs de L1 - tenant) équipes : qualifiés des ligues + outre-mer + Ligue 2 + NC / Polynésie
        int nL1 = 0, nL2 = 0;
        for (auto& pl : P.pools) for (int t : pl.clubs) if (g_world.teams[t].parent < 0 && t != cdfHolderDirect) { if (pl.tier == 0) nL1++; if (pl.tier == 1) nL2++; }
        int nDom = 0;
        for (auto& D : pyramids) if (D.dom >= 0) nDom += (D.name.find("Mayotte") != std::string::npos) ? 1 : 2;
        int t2 = 64 - nL1 - (cdfHolderDirect >= 0 ? 1 : 0);
        int QTOT = std::max(40, 4 * t2 - nDom - 2 - nL2);
        std::vector<int> Q(13); int given = 0;
        std::vector<std::pair<double, int>> rem;
        for (int r = 0; r < 13; r++) {
            double x = (double)QTOT * (am[r].size() + n2[r].size() + n1[r].size() + l3[r].size()) / std::max(1, total);
            Q[r] = std::max(1, (int)x); given += Q[r]; rem.push_back({ x - (int)x, r });
        }
        std::sort(rem.rbegin(), rem.rend());
        for (int i = 0; given < QTOT && i < 13; i++, given++) Q[rem[i].second]++;
        for (int r = 0; r < 13; r++) {
            Competition c;
            c.name = "Coupe de France - tours régionaux (" + sanitize(REGIONS[r].name) + ")";
            c.shortName = "CdF (rég.)";
            c.tag = r;
            buildCdfRegional(c, r, lvR[r], n2[r], n1[r], l3[r], Q[r]);
            int idx = addComp(S, std::move(c));
            S.comps[idx].cupRound(0, S.comps[idx].entrants[0]);
            cdfRegional.push_back(idx);
        }
        // outre-mer : 2 qualifiés par territoire
        for (int p2 = 0; p2 < (int)pyramids.size(); p2++) {
            Pyramid& D = pyramids[p2];
            if (D.dom < 0) continue;
            std::vector<int> t;
            for (auto& pl : D.pools) for (int x : pl.clubs) if (g_world.teams[x].parent < 0) t.push_back(x);
            Competition c;
            c.name = "Coupe de France - tours régionaux (" + D.name + ")"; c.shortName = "CdF (rég.)"; c.tag = 100 + p2;
            simpleCup(c, t, 1.5, 12.5);
            // on s'arrête à 2 qualifiés
            std::vector<int> tg;
            int keep = D.name.find("Mayotte") != std::string::npos ? 1 : 2;   // Mayotte : un représentant
            for (int x : c.koTargets) if (x >= keep) tg.push_back(x);
            c.koTargets = tg; c.koTimes.resize(tg.size());
            c.kind = 2; c.noReserves = true; c.neutralFinal = false; c.homeRule = 1;
            int idx = addComp(S, std::move(c));
            S.comps[idx].cupRound(0, S.comps[idx].entrants[0]);
            cdfRegional.push_back(idx);
        }
        // coupes régionales et départementales (une seule équipe par club : la plus haute engagée)
        auto oneTeamPerClub = [&](const std::vector<int>& cand) {
            std::map<int, int> best;
            std::map<int, int> tierOf;
            for (auto& pl : P.pools) for (int t : pl.clubs) tierOf[t] = pl.tier;
            for (int t : cand) {
                int c = clubOf(t);
                if (!best.count(c) || tierOf[t] < tierOf[best[c]]) best[c] = t;
            }
            std::vector<int> v; for (auto& kv : best) v.push_back(kv.second);
            return v;
        };
        for (int r = 0; r < 13; r++) {
            std::vector<int> cand;
            // réservée aux équipes de Régional 1, 2 et 3 ; une seule équipe par club : la plus haute (l'équipe fanion si elle
            // y joue, sinon sa meilleure réserve)
            for (auto& pl : P.pools) if (pl.tier >= 5 && pl.tier <= 7) for (int t : pl.clubs) if (g_world.teams[t].region == r && !g_world.teams[t].youth) cand.push_back(t);
            cand = oneTeamPerClub(cand);
            if (cand.size() < 4) continue;
            Competition c;
            c.name = "Coupe régionale - " + sanitize(REGIONS[r].name); c.shortName = "Coupe rég."; c.kind = 4; c.tag = r; c.homeRule = 1;
            simpleCup(c, cand, 5.3, 38.3);
            int idx = addComp(S, std::move(c));
            cupVenue[idx] = neutralVenue(cand);
            S.comps[idx].cupRound(0, S.comps[idx].entrants[0]);
            regionalCups.push_back(idx);
        }
        // coupes de district : une par district, ou deux dans les grands districts (D1-D2 et D3 et plus, comme dans la Marne)
        for (int d = 0; d < numDistricts(); d++) {
            if (districtRegion(d) < 0 || districtRegion(d) >= 13) continue;
            int nLv = 0; for (auto& pl : P.pools) if (P.tiers[pl.tier].scope == SC_DEPT && pl.key == d) nLv = std::max(nLv, pl.tier - 7);
            bool split = nLv >= 4;
            std::string base = districtRegion(d) == 12 ? std::string("Coupe de Corse (divisions départementales)") : "Coupe du " + districtFullName(d);
            for (int part = 0; part < (split ? 2 : 1); part++) {
                std::vector<int> cand;
                for (auto& pl : P.pools) {
                    if (pl.tier < 8 || P.tiers[pl.tier].scope != SC_DEPT || pl.key != d) continue;
                    if (split && (part == 0) != (pl.tier <= 9)) continue;
                    for (int t : pl.clubs) cand.push_back(t);
                }
                cand = oneTeamPerClub(cand);
                if (cand.size() < 4) continue;
                Competition c;
                c.name = base + (split ? (part == 0 ? " (D1-D2)" : " (D3 et plus)") : std::string());
                c.shortName = split ? (part == 0 ? "Coupe district D1-D2" : "Coupe district D3+") : "Coupe district";
                c.kind = 5; c.tag = d; c.homeRule = 1;
                simpleCup(c, cand, 6.2 + part * 0.5, 37.2);
                int idx = addComp(S, std::move(c));
                cupVenue[idx] = neutralVenue(cand);
                S.comps[idx].cupRound(0, S.comps[idx].entrants[0]);
                deptCups.push_back(idx);
            }
        }
    }
    // ---------------- coupes nationales étrangères : chaque pays doté d'un championnat a sa coupe (nom réel ou « Coupe de <pays> »)
    for (int p = 0; !euroOnly && p < (int)pyramids.size(); p++) {
        Pyramid& P = pyramids[p];
        if (P.dom >= 0 || P.country == "FRA" || P.country == "U19" || P.country == "U17" || P.country == "U15" || isWomenPyramid(P)) continue;
        std::vector<int> t;
        for (auto& pl : P.pools) for (int x : pl.clubs) if (g_world.teams[x].parent < 0 && !g_world.teams[x].youth) t.push_back(x);
        if (t.size() < 4) continue;
        Competition c;
        c.name = cupNameFor(P.country); c.shortName = c.name; c.kind = 2; c.tag = p;
        simpleCup(c, t, 6, 39);
        int idx = addComp(S, std::move(c));
        S.comps[idx].cupRound(0, S.comps[idx].entrants[0]);
        nationalCups.push_back(idx);
    }
    // ---------------- football féminin : coupes nationales, Coupe LFFP, Ligue des champions féminine
    womenStartSeason(*this);
    continentalStartSeason(*this);
    // ---------------- supercoupes (champion contre vainqueur de la coupe)
    for (int p = 0; !euroOnly && p < (int)pyramids.size(); p++) {
        Pyramid& P = pyramids[p];
        if (P.dom >= 0) continue;
        const char* nm = superNameFor(P.country);
        if (!nm || !prevChampion.count(P.country)) continue;
        int a = prevChampion[P.country], b = prevCupWinner.count(P.country) ? prevCupWinner[P.country] : -1;
        if (b < 0 || b == a) b = prevRunnerUp.count(P.country) ? prevRunnerUp[P.country] : -1;
        // Trophée des Champions : un club amateur ou semi-pro (hors L1, L2, L3) ne peut pas y participer -> 2e de Ligue 1
        if (P.country == "FRA" && b >= 0 && b != a && teamLevel(b) > 2) {
            int rep = prevRunnerUp.count(P.country) ? prevRunnerUp[P.country] : -1;
            if (kind == CK_CLUB) S.news.push_back(g_world.teams[b].name + ", vainqueur de la Coupe de France, n'a pas le statut professionnel : " + (rep >= 0 ? g_world.teams[rep].name + ", 2e de Ligue 1, le remplace au Trophée des Champions." : std::string("il est remplacé au Trophée des Champions.")));
            b = rep;
        }
        if (a < 0 || b < 0 || a == b) continue;
        Competition c;
        c.format = FMT_SINGLE; c.name = fmt("%s %d", sanitize(nm).c_str(), year); c.shortName = sanitize(nm); c.kind = 6; c.tag = p;
        c.addKOStage({ { a, b } }, 1, 0.15, "Finale", true);
        superCups.push_back(addComp(S, std::move(c)));
    }
    // lieux tirés au sort : Trophée des Champions (souvent à l'étranger), finales européennes (annoncées en début de saison)
    {
        static const char* TDC_V[] = { "Stade de France (Saint-Denis)", "Stade olympique (Montréal)", "Stade Ibn-Batouta (Tanger)", "Stade Jassim-bin-Hamad (Doha)",
                                       "Stade Bloomfield (Tel-Aviv)", "Stade Olympique de Radès (Tunis)", "Stade Jaber-Al-Ahmad (Koweït)", "Stade Bollaert-Delelis (Lens)",
                                       "Stade de l'Amitié (Libreville)", "Wörthersee Stadion (Klagenfurt)" };
        static const char* UCL_V[] = { "Wembley (Londres)", "Allianz Arena (Munich)", "Estadio Metropolitano (Madrid)", "Stade de France (Saint-Denis)", "Puskás Aréna (Budapest)",
                                       "San Siro (Milan)", "Stade olympique (Istanbul)", "Principality Stadium (Cardiff)", "Stade Vélodrome (Marseille)", "Estádio da Luz (Lisbonne)" };
        static const char* UEFA_V[] = { "Parc Olympique lyonnais (Lyon)", "Aviva Stadium (Dublin)", "San Mamés (Bilbao)", "Stade national (Bucarest)", "Stadio Olimpico (Rome)",
                                        "Philips Stadion (Eindhoven)", "Stade du Nouveau Karaïskakis (Le Pirée)", "Estadio de la Cartuja (Séville)", "Veltins-Arena (Gelsenkirchen)", "Stade de la Beaujoire (Nantes)" };
        // pas deux fois la même ville en finale sur une courte période (5 saisons), ni les deux finales européennes dans la même ville
        auto cityOf = [](const std::string& v) { size_t a = v.find('('), b = v.rfind(')'); return a != std::string::npos && b > a ? v.substr(a + 1, b - a - 1) : v; };
        auto recent = [&](int hc, int yrs) {
            std::set<std::string> c;
            for (int y = year - yrs + 1; y <= year; y++) { const char* hv = histVenue(hc, y); if (hv && *hv) c.insert(cityOf(hv)); }
            for (size_t i = 0; i < honourLog.size() && i < honourVenue.size(); i++)
                if (honourLog[i].comp == hc && honourLog[i].year > year - yrs && !honourVenue[i].empty()) c.insert(cityOf(honourVenue[i]));
            return c;
        };
        auto pick = [&](const char* const* L, int n, std::set<std::string> avoid) {
            std::vector<int> ok;
            for (int i = 0; i < n; i++) if (!avoid.count(cityOf(L[i]))) ok.push_back(i);
            if (ok.empty()) for (int i = 0; i < n; i++) ok.push_back(i);
            return std::string(L[ok[g_rng.next() % ok.size()]]);
        };
        static const char* UCL_C[] = { "ENG", "GER", "ESP", "FRA", "HUN", "ITA", "TUR", "WAL", "FRA", "POR" };
        static const char* UEFA_C[] = { "FRA", "IRL", "ESP", "ROU", "ITA", "NED", "GRE", "ESP", "GER", "FRA" };
        (void)TDC_V;
        // Trophée des Champions : un stade français différent chaque année (grande capacité)
        {
            std::vector<std::string> cand;
            int frp = pyramidOf(pyramids, "FRA");
            auto rec = recent(HC_TDC, 4);
            if (frp >= 0) for (auto& pl : pyramids[frp].pools) if (pl.tier <= 1) for (int t : pl.clubs) {
                Team& T = g_world.teams[t];
                if (T.parent >= 0 || T.stadium.empty()) continue;
                ensureStadium(t);
                if (T.sta.capacity() < 25000) continue;
                std::string v = T.stadium + " (" + (T.town.empty() ? T.name : T.town) + ")";
                if (!rec.count(cityOf(v)) && std::find(cand.begin(), cand.end(), v) == cand.end()) cand.push_back(v);
            }
            cand.push_back("Stade de France (Saint-Denis)");
            tdcVenue = cand[g_rng.next() % cand.size()];
        }
        uclFinalVenue = pick(UCL_V, 10, recent(HC_UCL, 6));
        // finale de la Coupe UEFA : pas la même ville récemment, et jamais dans le même pays que celle de la Ligue des champions
        {
            std::string uc; for (int i = 0; i < 10; i++) if (uclFinalVenue == UCL_V[i]) uc = UCL_C[i];
            auto av = recent(HC_UEFA, 6); av.insert(cityOf(uclFinalVenue));
            std::vector<int> ok;
            for (int i = 0; i < 10; i++) if (UEFA_C[i] != uc && !av.count(cityOf(UEFA_V[i]))) ok.push_back(i);
            if (ok.empty()) for (int i = 0; i < 10; i++) if (UEFA_C[i] != uc) ok.push_back(i);
            uefaFinalVenue = UEFA_V[ok[g_rng.next() % ok.size()]];
        }
        if (kind == CK_CLUB && euroOnly) S.news.push_back("Finale de la Ligue des champions : " + uclFinalVenue + ". Finale de la Coupe UEFA : " + uefaFinalVenue + ".");
        else if (kind == CK_CLUB) S.news.push_back("Le Trophée des Champions se jouera au " + tdcVenue + ". Finale de la Ligue des champions : " + uclFinalVenue + ". Finale de la Coupe UEFA : " + uefaFinalVenue + ".");
    }
    // Supercoupes de région : champion de Régional 1 contre vainqueur de la coupe régionale (saison précédente) ;
    // leurs vainqueurs disputent ensuite la Méga Coupe des Régions (tirs au but directs, même en finale)
    superRegions = -1; regSuperCups.clear();
    {
        int fr = pyramidOf(pyramids, "FRA");
        if (kind == CK_CLUB && fr >= 0 && !euroOnly) {
            const Pyramid& P = pyramids[fr];
            for (int r = 0; r < 13; r++) {
                std::vector<int> r1;
                for (auto& pl : P.pools) if (pl.tier == 5 && pl.key == r) for (int x : pl.clubs) if (g_world.teams[x].parent < 0) r1.push_back(x);
                sortByRating(r1);
                int a = -1, b = -1;
                auto ic = prevR1Champ.find(r);
                if (ic != prevR1Champ.end() && ic->second >= 0 && ic->second < (int)g_world.teams.size()) a = ic->second;
                auto it = prevRegCupWinner.find(r);
                if (it != prevRegCupWinner.end() && it->second >= 0 && it->second < (int)g_world.teams.size() && g_world.teams[it->second].parent < 0) b = it->second;
                if (a < 0 && !r1.empty()) a = r1[0];
                if (b < 0 || b == a) { for (int x : r1) if (x != a) { b = x; break; } }
                if (a < 0 || b < 0 || a == b) continue;
                Competition c;
                c.format = FMT_SINGLE; c.kind = 26; c.tag = r; c.neutralFinal = false;
                c.name = fmt("Supercoupe de %s %d", sanitize(REGIONS[r].name).c_str(), year); c.shortName = "Supercoupe rég.";
                c.addKOStage({ { a, b } }, 1, 0.05, "Finale", false);
                c.matches[0].noET = 1;
                int idx = addComp(S, std::move(c));
                regSuperCups.push_back(idx);
            }
        }
    }
    uefaSuper = -1;
    if (!euroOnly && prevUclWinner >= 0 && prevUefaWinner >= 0 && prevUclWinner != prevUefaWinner) {
        Competition c;
        c.format = FMT_SINGLE; c.name = fmt("Supercoupe de l'UEFA %d", year); c.shortName = "Supercoupe UEFA"; c.kind = 7;
        c.addKOStage({ { prevUclWinner, prevUefaWinner } }, 1, 0.3, "Finale", true);
        uefaSuper = addComp(S, std::move(c));
        S.comps[uefaSuper].matches[0].noET = 0;
    }
    uecl = -1; intertoto = -1;
    if (opts.euroFormat) addNewEuroCups();
    else {
    // ---------------- Ligue des champions (format 2003-04)
    // semaines depuis le 1er août : Q1 mi-juillet, Q2 fin juillet, Q3 mi-août, groupes de mi-septembre à début décembre,
    // 8es fin février, quarts fin mars, demies fin avril, finale fin mai (lieu annoncé en début de saison)
    {
        Competition c;
        c.format = FMT_UCL2000; c.name = "Ligue des Champions"; c.shortName = "C1"; c.kind = 3; c.tb = TB_H2H; c.neutralFinal = true; c.awayGoals = opts.awayGoals != 0;
        c.host = prevUclWinner;       // tenant du titre : chapeau 1
        c.entrants = { nextEuro.uclQ1, nextEuro.uclQ2, nextEuro.uclQ3, nextEuro.uclGS };
        c.koNames = { "1er tour de qualification", "2e tour de qualification", "3e tour de qualification" };
        c.koTimes = { -2.6, -0.6, 1.9, 6.5, 9.0, 12.0, 13.5, 16.5, 18.0, 29.0, 34.0, 38.5, 43.0 };
        std::vector<int> q1 = nextEuro.uclQ1;
        int idx = addComp(S, std::move(c));
        Competition& C = S.comps[idx];
        if (!q1.empty()) C.koRound(q1, 2, C.koTimes[0], C.koNames[0], false);
        else C.koRound(C.entrants[1], 2, C.koTimes[1], C.koNames[1], false);
        ucl = idx;
    }
    // ---------------- Coupe UEFA (format 2003-04)
    {
        Competition c;
        c.format = FMT_UEFA2000; c.name = "Coupe UEFA"; c.shortName = "C3"; c.kind = 8; c.neutralFinal = true; c.awayGoals = opts.awayGoals != 0;
        c.entrants = { nextEuro.uefaQR, nextEuro.uefaR1, {}, {} };
        c.koNames = { "Tour de qualification", "1er tour", "2e tour", "16es de finale", "8es de finale", "Quarts de finale", "Demi-finales", "Finale" };
        c.koTimes = { 2.2, 7.5, 13.0, 29.2, 32.0, 35.2, 38.7, 41.8 };
        c.koLegs = { 2, 2, 2, 2, 2, 2, 2, 1 };
        int idx = addComp(S, std::move(c));
        Competition& C = S.comps[idx];
        C.koRound(C.entrants[0], 2, C.koTimes[0], C.koNames[0], false);
        uel = idx;
    }
    // ---------------- Coupe Intertoto (règles 2003 : 61 clubs, 3 vainqueurs qualifiés pour le 1er tour de la Coupe UEFA)
    intertoto = -1;
    if (!euroOnly && nextEuro.itR1.size() >= 4) {
        Competition c;
        c.format = FMT_INTERTOTO; c.name = "Coupe Intertoto"; c.shortName = "Intertoto"; c.kind = 14; c.awayGoals = opts.awayGoals != 0; c.neutralFinal = false;
        c.entrants = { nextEuro.itR1, nextEuro.itR2, nextEuro.itR3, {}, {} };
        c.koNames = { "1er tour", "2e tour", "3e tour", "Demi-finales", "Finales" };
        c.koTimes = { -6.0, -4.6, -3.2, -1.8, 0.2 };
        int idx = addComp(S, std::move(c));
        S.comps[idx].koRound(S.comps[idx].entrants[0], 2, S.comps[idx].koTimes[0], "1er tour", false);
        intertoto = idx;
    }
    }
    // ---------------- Coupe de la Ligue (L1, L2, L3)
    cdl = -1;
    if (fr >= 0 && !euroOnly && opts.cdl) {
        Pyramid& P = pyramids[fr];
        Competition c;
        c.format = FMT_CUP; c.name = "Coupe de la Ligue"; c.shortName = "CdL"; c.kind = 11; c.noReserves = true; c.neutralFinal = true;
        std::vector<int> l1, l2, l3;
        for (auto& pl : P.pools) for (int t : pl.clubs) if (g_world.teams[t].parent < 0) { if (pl.tier == 0) l1.push_back(t); else if (pl.tier == 1) l2.push_back(t); else if (pl.tier == 2) l3.push_back(t); }
        if (!l1.empty() && !l3.empty()) {
            c.entrants = { l3, l2, l1, {}, {}, {}, {} };
            int w1 = (int)l3.size() / 2;
            int w2 = 32 - (int)l1.size();
            c.koTargets = { w1, w2, 16, 8, 4, 2, 1 };
            c.koNames = { "1er tour", "2e tour", "16es de finale", "8es de finale", "Quarts de finale", "Demi-finales", "Finale" };
            c.koTimes = { 3.2, 8.2, 14.2, 20.2, 24.2, 30.2, 35.2 };
            int idx = addComp(S, std::move(c));
            S.comps[idx].cupRound(0, S.comps[idx].entrants[0]);
            cdl = idx;
        }
    }
    // ---------------- jeunes : Coupe Gambardella (tours régionaux), coupes régionales et de district U19
    u19Final = -1; u17Final = -1; gambNational = -1; gambRegional.clear(); u19Cups.clear();
    int yp = u19Pyramid();
    if (yp >= 0 && !euroOnly && kind == CK_CLUB) {
        Pyramid& Y = pyramids[yp];
        std::vector<std::vector<int>> byReg(13);
        int nNat = 0;
        for (auto& pl : Y.pools) for (int t : pl.clubs) {
            if (pl.tier == 0) { nNat++; continue; }
            int rg = g_world.teams[t].region;
            if (rg >= 0 && rg < 13) byReg[rg].push_back(t);
        }
        int QT = std::max(16, 128 - nNat), tot = 0;
        for (auto& v : byReg) tot += (int)v.size();
        std::vector<int> Q(13, 0); int given = 0;
        std::vector<std::pair<double, int>> rem;
        for (int r = 0; r < 13; r++) {
            if (byReg[r].size() < 2) continue;
            double x = (double)QT * byReg[r].size() / std::max(1, tot);
            Q[r] = std::max(1, std::min((int)byReg[r].size() / 2, (int)x)); given += Q[r]; rem.push_back({ x - (int)x, r });
        }
        std::sort(rem.rbegin(), rem.rend());
        for (size_t i = 0; given < QT && i < rem.size(); i++) { int r = rem[i].second; if (Q[r] < (int)byReg[r].size() / 2) { Q[r]++; given++; } }
        for (int r = 0; r < 13; r++) {
            if (Q[r] <= 0) continue;
            Competition c;
            c.format = FMT_CUP; c.kind = 16; c.tag = r; c.homeRule = 1;
            c.name = "Coupe Gambardella - tours régionaux (" + sanitize(REGIONS[r].name) + ")"; c.shortName = "Gambardella (rég.)";
            c.entrants.assign(1, byReg[r]);
            int x = (int)byReg[r].size();
            while (x > Q[r]) { x = std::max(Q[r], (x + 1) / 2); c.koTargets.push_back(x); }
            int R = (int)c.koTargets.size();
            for (int i = 0; i < R; i++) c.koTimes.push_back(4.0 + 11.5 * i / std::max(1, R - 1));
            int idx = addComp(S, std::move(c));
            S.comps[idx].cupRound(0, S.comps[idx].entrants[0]);
            gambRegional.push_back(idx);
        }
        // coupes régionales U19 (R1 et R2 U19) et coupes de district U19
        for (int r = 0; r < 13; r++) {
            std::vector<int> v;
            for (auto& pl : Y.pools) if (pl.tier >= 1 && pl.tier <= 3 && pl.key == r) v.insert(v.end(), pl.clubs.begin(), pl.clubs.end());
            if (v.size() < 4) continue;
            Competition c;
            c.name = "Coupe régionale U19 - " + sanitize(REGIONS[r].name); c.shortName = "Coupe rég. U19"; c.kind = 17; c.tag = r; c.homeRule = 1;
            simpleCup(c, v, 7.3, 37.3);
            int idx = addComp(S, std::move(c));
            cupVenue[idx] = neutralVenue(v);
            S.comps[idx].cupRound(0, S.comps[idx].entrants[0]);
            u19Cups.push_back(idx);
        }
        std::map<int, std::vector<int>> byDept;
        for (auto& pl : Y.pools) if (Y.tiers[pl.tier].scope == SC_DEPT) byDept[pl.key].insert(byDept[pl.key].end(), pl.clubs.begin(), pl.clubs.end());   // par district
        for (auto& kv : byDept) {
            if (kv.second.size() < 4) continue;
            Competition c;
            c.name = "Coupe U19 - " + districtFullName(kv.first); c.shortName = "Coupe district U19"; c.kind = 18; c.tag = kv.first; c.homeRule = 1;
            simpleCup(c, kv.second, 8.2, 36.2);
            int idx = addComp(S, std::move(c));
            cupVenue[idx] = neutralVenue(kv.second);
            S.comps[idx].cupRound(0, S.comps[idx].entrants[0]);
            u19Cups.push_back(idx);
        }
    }
    setupYouthUcl();
    // finales des coupes régionales / de district du club : terrain neutre désigné en début de saison
    if (kind == CK_CLUB && userTeam >= 0)
        for (auto& kv : cupVenue) {
            if (kv.first < 0 || kv.first >= (int)S.comps.size() || kv.second.empty()) continue;
            bool in = false; for (auto& e : S.comps[kv.first].entrants) for (int t : e) if (t == userTeam) in = true;
            if (in) S.news.push_back(S.comps[kv.first].name + " : la finale aura lieu sur terrain neutre, " + kv.second + ".");
        }
    updateStatuses();
    // coefficients : saison en cours remise à zéro
    for (auto& u : uefa) { u.cur = 0; u.clubs = 0; }
}

// ------------------------------------------------------------------ crochets
void onCompetitionDoneHook(int comp) { g_career.onCompetitionDone(comp); }
void onStageDoneHook(int comp, int stage) { g_career.onStageDone(comp, stage); }

static void qualifyIntl(Career& K, int comp);

// petit journal européen : qualification, repêchage en Coupe UEFA ou élimination des clubs français (et du club du joueur)
static void euroStageNews(Career& K, int comp, int stage) {
    Season& S = K.season;
    const Competition& C = S.comps[comp];
    if (stage < 0 || stage >= (int)C.stages.size()) return;
    const Stage& st = C.stages[stage];
    std::set<int> in, later;
    for (auto& g : st.groups) for (int t : g) in.insert(t);
    for (auto& t : st.ties) { if (t.a >= 0) in.insert(t.a); if (t.b >= 0) in.insert(t.b); }
    for (auto& R : st.rounds) for (int mi : R.m) { in.insert(C.matches[mi].home); in.insert(C.matches[mi].away); }
    bool last = stage == (int)C.stages.size() - 1;
    if (last && C.done) return;           // finale : palmarès
    for (int s2 = stage + 1; s2 < (int)C.stages.size(); s2++) {
        for (auto& g : C.stages[s2].groups) for (int t : g) later.insert(t);
        for (auto& t : C.stages[s2].ties) { later.insert(t.a); later.insert(t.b); }
    }
    for (int t : C.carry) later.insert(t);
    std::string nextName = stage + 1 < (int)C.stages.size() ? C.stages[stage + 1].name : std::string("le tour suivant");
    bool isUcl = comp == K.ucl;
    std::string cn = isUcl ? "Ligue des champions" : C.name;
    for (int t : in) {
        if (t < 0) continue;
        const Team& T = g_world.teams[t];
        bool fr = T.nation >= 0 && std::string(NATIONS[T.nation].code) == "FRA";
        bool user = t == K.userTeam;
        if (!fr && !user) continue;
        std::string pre = user ? "[Votre club] " : "";
        if (later.count(t)) S.news.push_back(pre + cn + " (" + st.name + ") : " + T.name + " se qualifie pour " + (nextName == "le tour suivant" ? nextName : "le tour suivant : " + nextName) + " !");
        else if (C.format == FMT_NEWEURO && st.type == ST_KO && [&]() { for (int oc : { K.uel, K.uecl }) if (oc >= 0 && oc != comp) for (auto& e : S.comps[oc].entrants) for (int x : e) if (x == t) return true; return false; }())
            S.news.push_back(pre + cn + " (" + st.name + ") : " + T.name + " est éliminé mais reversé en " + (comp == K.ucl ? "Ligue Europa." : "Ligue Conférence."));
        else if (C.format != FMT_NEWEURO && isUcl && (std::find(C.extra.begin(), C.extra.end(), t) != C.extra.end() || std::find(C.extra2.begin(), C.extra2.end(), t) != C.extra2.end()))
            S.news.push_back(pre + cn + " (" + st.name + ") : " + T.name + " est éliminé mais reversé en Coupe UEFA.");
        else if (!last || !C.done) S.news.push_back(pre + cn + " (" + st.name + ") : " + T.name + " est éliminé.");
    }
}

void Career::onStageDone(int comp, int stage) {
    if (kind != CK_CLUB) return;
    Season& S = season;
    if ((comp == ucl || comp == uel || comp == uecl) && comp >= 0) euroStageNews(*this, comp, stage);
    if (opts.euroFormat) { routeNewEuro(); return; }
    if (comp != ucl || uel < 0) return;
    Competition& U = S.comps[ucl];
    Competition& E = S.comps[uel];
    // éliminés du 3e tour de qualification -> 1er tour de la Coupe UEFA
    if (stage == 2 && !(E.extReadyMask & 2)) {
        for (int t : U.extra) E.entrants[1].push_back(t);
        E.extReadyMask |= 2;
        if (E.awaiting == 1) E.resume();
    }
    // 3es de la 1re phase de groupes -> 3e tour de la Coupe UEFA
    if (U.stages[stage].type == ST_LEAGUE && !(E.extReadyMask & 8)) {
        for (int t : U.extra2) E.entrants[3].push_back(t);
        E.extReadyMask |= 8;
        if (E.awaiting == 3) E.resume();
    }
}

static void addHonour(int team, const std::string& s) {
    if (team < 0) return;
    g_world.teams[team].honours.push_back(s);
}

// palmarès structuré (écran Palmarès)
void Career::logHonour(int comp) {
    if (kind != CK_CLUB || comp < 0 || comp >= (int)season.comps.size()) return;
    const Competition& C = season.comps[comp];
    if (C.winner < 0) return;
    int fr = pyramidOf(pyramids, "FRA");
    int hc = -1, yr = year + 1;
    if (C.format == FMT_LEAGUE && fr >= 0) {
        int q0 = pyramids[fr].poolIndex(0, 0), q1 = pyramids[fr].poolIndex(1, 0);
        if (q0 >= 0 && !pyramids[fr].pools[q0].comps.empty() && pyramids[fr].pools[q0].comps[0] == comp) hc = HC_L1;
        if (q1 >= 0 && !pyramids[fr].pools[q1].comps.empty() && pyramids[fr].pools[q1].comps[0] == comp) hc = HC_L2;
    }
    else if (comp == cdfNational) hc = HC_CDF;
    else if (comp == cdl) hc = HC_CDL;
    else if (comp == ucl) hc = HC_UCL;
    else if (comp == uel) hc = HC_UEFA;
    else if (comp == uefaSuper) { hc = HC_SUPERUEFA; yr = year; }
    else if (comp == intertoto) { hc = HC_INTERTOTO; yr = year; }
    else if (C.kind == 6 && C.tag == fr) { hc = HC_TDC; yr = year; }
    if (hc < 0) return;
    std::string venue = hc == HC_UCL ? uclFinalVenue : hc == HC_UEFA ? uefaFinalVenue : hc == HC_TDC ? tdcVenue :
                        hc == HC_CDF || hc == HC_CDL ? std::string("Stade de France (Saint-Denis)") : hc == HC_SUPERUEFA ? std::string("Stade Louis-II (Monaco)") : std::string();
    honourVenue.resize(honourLog.size());
    if (hc == HC_INTERTOTO) { for (int t : C.result) { HonourRec h; h.comp = hc; h.year = yr; h.winner = t; honourLog.push_back(h); honourVenue.push_back("aller-retour"); } return; }
    HonourRec h; h.comp = hc; h.year = yr; h.winner = C.winner;
    if (C.format == FMT_LEAGUE) h.runner = C.result.size() > 1 ? C.result[1] : -1;
    else if (!C.stages.empty() && C.stages.back().ties.size() == 1) { const Tie& t = C.stages.back().ties[0]; h.runner = t.a == C.winner ? t.b : t.a; }
    honourLog.push_back(h);
    honourVenue.push_back(C.format == FMT_LEAGUE ? std::string() : venue);
}

void Career::onCompetitionDone(int comp) {
    Competition& C = season.comps[comp];
    if (comp == intertoto && uel >= 0 && kind == CK_CLUB) {
        Competition& E = season.comps[uel];
        for (int t : C.result) E.entrants[1].push_back(t);
        for (int t : C.result) season.news.push_back(g_world.teams[t].name + " remporte une finale de la Coupe Intertoto et se qualifie pour la Coupe UEFA.");
    }
    if (kind == CK_INTL) { nlOnCompDone(*this, comp); if (coach) coachOnCompDone(*this, comp); if (C.kind >= 30 && C.kind <= 33) return; if (intlType >= IT_OLYMPICS) youthQualify(*this, comp); else qualifyIntl(*this, comp); return; }
    if (kind == CK_CUSTOM) { if (C.winner >= 0) history.push_back(C.name + " : " + g_world.teams[C.winner].name); return; }
    if (kind == CK_CLUB && comp == ucl && cwcYear(year)) cwcCreate(*this);
    if (C.kind == 28 && C.winner >= 0) {
        addHonour(C.winner, fmt("Vainqueur de la Coupe du monde des clubs de la FIFA (%d)", year + 1));
        history.push_back(seasonLabel(year) + " " + C.name + " : " + g_world.teams[C.winner].name);
        season.news.push_back(C.name + " : " + g_world.teams[C.winner].name + " est champion du monde des clubs !");
        return;
    }
    if (C.kind == 26 && superRegions < 0) {
        bool all = true; std::vector<int> w;
        for (int c : regSuperCups) { if (!season.comps[c].done) all = false; else if (season.comps[c].winner >= 0) w.push_back(season.comps[c].winner); }
        if (all && w.size() >= 4) {
            Competition m;
            m.name = fmt("Méga Coupe des Régions %d", year); m.shortName = "Méga Coupe";
            simpleCup(m, w, 3.0, 9.0);
            m.kind = 27; m.homeRule = 1; m.neutralFinal = true; m.noReserves = true;
            m.koNames.clear();
            for (int x : m.koTargets) m.koNames.push_back(x == 1 ? "Finale" : x == 2 ? "Demi-finales" : x == 4 ? "Quarts de finale" : "Tour préliminaire");
            int idx = addComp(season, std::move(m));
            season.comps[idx].cupRound(0, season.comps[idx].entrants[0]);
            superRegions = idx;
            season.news.push_back(season.comps[idx].name + " : les vainqueurs des supercoupes de région s'affrontent (tirs au but directs, même en finale).");
        }
    }
    onU19CompDone(comp);
    womenOnCompDone(*this, comp);
    if (isContinentalKind(C.kind) && C.winner >= 0) season.news.push_back(C.name + " : " + g_world.teams[C.winner].name + " est champion continental !");
    std::string y = seasonLabel(year);
    if (C.format != FMT_LEAGUE && C.winner >= 0 && C.kind != 10 && C.kind != 12 && C.kind != 20 && C.name.find("tours régionaux") == std::string::npos)
        addHonour(C.winner, "Vainqueur : " + C.name + " (" + y + ")");
    logHonour(comp);
    // Coupe de France : phase nationale quand toutes les ligues ont désigné leurs qualifiés
    if (std::find(cdfRegional.begin(), cdfRegional.end(), comp) != cdfRegional.end() && cdfNational < 0) {
        for (int c : cdfRegional) if (!season.comps[c].done) return;
        int fr = pyramidOf(pyramids, "FRA");
        Competition n;
        n.format = FMT_CUP; n.name = "Coupe de France"; n.shortName = "CdF"; n.kind = 2; n.noReserves = true; n.homeRule = 1;
        n.entrants.assign(3, {});
        for (int c : cdfRegional) for (int t : season.comps[c].result) n.entrants[0].push_back(t);
        if (fr >= 0) {
            for (int t : pyramids[fr].pools[1].clubs) if (g_world.teams[t].parent < 0 && t != cdfHolderDirect) n.entrants[0].push_back(t);
            // représentants de Nouvelle-Calédonie et de Polynésie française au 7e tour
            for (int i = 0; i < 2; i++) if (g_world.omReps[i] >= 0) n.entrants[0].push_back(g_world.omReps[i]);
            for (int t : pyramids[fr].pools[0].clubs) n.entrants[2].push_back(t);
            if (cdfHolderDirect >= 0) n.entrants[2].push_back(cdfHolderDirect);
        }
        int in0 = (int)n.entrants[0].size();
        int t2 = 64 - (int)n.entrants[2].size();
        n.koTargets = { std::max(t2, in0 / 2), t2, 32, 16, 8, 4, 2, 1 };
        n.koNames = { "7e tour", "8e tour", "32es de finale", "16es de finale", "8es de finale", "Quarts de finale", "Demi-finales", "Finale" };
        n.koTimes = { std::max(15.0, season.now + 0.5), 17, 20, 23, 26, 30, 35, 41.3 };     // finale : après la dernière journée de L1 (40), avant la finale de la C1 (43)
        n.regionalDraw = true; n.regionalRounds = 2;
        int idx = addComp(season, std::move(n));
        season.comps[idx].cupRound(0, season.comps[idx].entrants[0]);
        cdf = cdfNational = idx;
        return;
    }
    if (C.format != FMT_LEAGUE) return;
    // barrages
    int pyr = C.tag / 100000;
    if (pyr < 0 || pyr >= (int)pyramids.size()) return;
    Pyramid& P = pyramids[pyr];
    for (int t = 1; t < (int)P.tiers.size(); t++) {
        int type = P.tiers[t].barrageUp;
        if (!type) continue;
        int qa = P.poolIndex(t - 1, 0), qb = P.poolIndex(t, 0);
        if (qa < 0 || qb < 0 || P.pools[qa].comps.empty() || P.pools[qb].comps.empty()) continue;
        int ca = P.pools[qa].comps[0], cb = P.pools[qb].comps[0];
        if (!season.comps[cb].done || (type != 2 && !season.comps[ca].done)) continue;
        bool exists = false;
        for (auto& c : season.comps) if (c.kind == 10 && c.tag == pyr * 100 + t) exists = true;
        if (exists) continue;
        auto ra = season.comps[ca].result; auto rb = season.comps[cb].result;
        // pas de réserves dans les barrages d'accession
        rb.erase(std::remove_if(rb.begin(), rb.end(), [](int x) { return g_world.teams[x].parent >= 0; }), rb.end());
        int up = P.tiers[t].up;
        if ((int)rb.size() < up + 4 || (int)ra.size() < 3) continue;
        Competition c;
        c.format = FMT_KO_ONLY; c.kind = 10; c.tag = pyr * 100 + t; c.qualSpots = 1; c.qualPlayoff = type;
        c.name = "Barrages " + P.tiers[t - 1].name + " / " + P.tiers[t].name; c.shortName = "Barrages";
        c.neutralFinal = P.country == "ENG";
        double t0 = std::max(40.3, season.now + 0.3);
        if (type == 1) {
            c.koLegs = { 2 };
            c.addKOStage({ { rb[up], ra[ra.size() - 3] } }, 2, t0, "Barrage");
        } else if (type == 2) {
            c.koLegs = { 2, P.country == "ENG" ? 1 : 2 };
            c.koTimes = { t0, t0 + 1.2 };
            c.koNames = { "Play-offs : demi-finales", "Play-offs : finale" };
            c.addKOStage({ { rb[up + 3], rb[up] }, { rb[up + 2], rb[up + 1] } }, 2, t0, "Play-offs : demi-finales");
        } else if (type == 3) {
            c.koLegs = { 1, 1, 2 };
            c.koTimes = { t0, t0 + 0.4, t0 + 0.8 };
            c.koNames = { "Barrages L2 : 4e - 5e", "Barrages L2 : contre le 3e", "Barrage L1 / L2" };
            c.extra = { rb[up], ra[ra.size() - 3] };
            c.addKOStage({ { rb[up + 1], rb[up + 2] } }, 1, t0, "Barrages L2 : 4e - 5e");
        }
        addComp(season, std::move(c));
    }
}

// ------------------------------------------------------------------ fin de saison : montées / descentes
struct RankInfo { float ratio = 1; float ppg = 0; };

static bool tierAllowsReserve(const Pyramid& P, int tier) { return !P.tiers[tier].noReserves; }

static void movePyramid(Career& K, int pyr, std::map<int, RankInfo>& rank) {
    Pyramid& P = K.pyramids[pyr];
    int T = (int)P.tiers.size();
    std::map<int, int> newTier, oldTier;
    for (auto& pl : P.pools) for (int c : pl.clubs) { newTier[c] = pl.tier; oldTier[c] = pl.tier; }
    auto parentTier = [&](int t) -> int {
        const Team& tm = g_world.teams[t];
        if (tm.parent < 0) return -1;
        int m = newTier.count(tm.parent) ? newTier[tm.parent] : -1;
        for (auto& kv : newTier) {
            const Team& s = g_world.teams[kv.first];
            if (s.parent == tm.parent && s.resLevel < tm.resLevel) m = std::max(m, kv.second);
        }
        return m;
    };
    auto eligible = [&](int t, int tier) {
        if (g_world.teams[t].parent < 0) return true;
        if (!tierAllowsReserve(P, tier)) return false;
        return tier > parentTier(t);
    };
    for (auto& pl : P.pools) {
        const TierConf& TC = P.tiers[pl.tier];
        std::vector<std::pair<float, int>> winners;
        for (int g = 0; g < (int)pl.comps.size(); g++) {
            const Competition& C = K.season.comps[pl.comps[g]];
            const auto& res = C.result;
            int n = (int)res.size();
            auto tb = C.table(0, 0);
            std::map<int, float> ppg;
            for (auto& s : tb) ppg[s.team] = s.p ? (float)s.pts / s.p : 0;
            for (int i = 0; i < n; i++) {
                RankInfo ri; ri.ratio = n > 1 ? (float)i / (n - 1) : 0; ri.ppg = ppg[res[i]];
                rank[res[i]] = ri;
            }
            // montées : les premiers éligibles (réserves exclues si le niveau supérieur leur est interdit)
            if (pl.tier > 0) {
                int got = 0;
                for (int i = 0; i < n && got < TC.up; i++) {
                    if (!eligible(res[i], pl.tier - 1)) continue;
                    winners.push_back({ -rank[res[i]].ppg - (TC.up - got) * 10.f, res[i] });
                    got++;
                }
            }
            // descentes
            bool lowerExists = false;
            for (int t2 = pl.tier + 1; t2 < T && !lowerExists; t2++)
                for (auto& p2 : P.pools) if (p2.tier == t2 && (P.tiers[t2].scope == SC_NATIONAL || p2.key == pl.key ||
                    (P.tiers[t2].scope == SC_DEPT && P.tiers[pl.tier].scope == SC_REGION && districtRegion(p2.key) == pl.key) ||
                    (P.tiers[pl.tier].scope == SC_NATIONAL))) { lowerExists = true; break; }
            if (!TC.flexible && !pl.terminal && lowerExists)
                for (int i = std::max(0, n - TC.down); i < n; i++) newTier[res[i]] = pl.tier + 1;
        }
        std::sort(winners.begin(), winners.end());
        int cap = pl.upCap >= 0 ? pl.upCap : (int)winners.size();
        for (int i = 0; i < (int)winners.size() && i < cap; i++) newTier[winners[i].second] = pl.tier - 1;
    }
    // barrages
    for (auto& c : K.season.comps) {
        if (c.kind != 10 || c.tag / 100 != pyr || !c.done) continue;
        int lowT = c.tag % 100;
        int w = c.winner;
        if (w < 0) continue;
        if (c.qualPlayoff == 2) { if (oldTier[w] == lowT && eligible(w, lowT - 1)) newTier[w] = lowT - 1; continue; }
        // vainqueur issu de la division inférieure : il monte, le club de l'étage supérieur descend
        const Stage& last = c.stages.back();
        if (last.ties.empty()) continue;
        int a = last.ties[0].a, b = last.ties[0].b;
        int upper = oldTier[a] < oldTier[b] ? a : b, lower = upper == a ? b : a;
        if (w == lower) { newTier[lower] = lowT - 1; newTier[upper] = lowT; K.season.news.push_back(g_world.teams[lower].name + " remporte le barrage et monte en " + P.tiers[lowT - 1].name + " !"); }
    }
    // règles des réserves : toujours au moins un niveau sous l'équipe supérieure, jamais en division interdite
    for (int pass = 0; pass < 3; pass++) {
        std::vector<int> res;
        for (auto& kv : newTier) if (g_world.teams[kv.first].parent >= 0) res.push_back(kv.first);
        std::stable_sort(res.begin(), res.end(), [](int a, int b) { return g_world.teams[a].resLevel < g_world.teams[b].resLevel; });
        for (int r : res) {
            int m = parentTier(r) + 1;
            while (m < T && !tierAllowsReserve(P, m)) m++;
            if (newTier[r] < m) newTier[r] = std::min(m, T - 1);
        }
    }
    // placement : pool de la bonne zone ; si le niveau n'existe pas dans la zone, niveau inférieur existant
    for (auto& pl : P.pools) pl.clubs.clear();
    auto place = [&](int club, int tier) {
        for (int t = tier; t < T; t++) {
            int q = P.poolIndex(t, P.keyFor(t, club));
            if (q >= 0) { P.pools[q].clubs.push_back(club); newTier[club] = t; return; }
        }
        for (int t = tier - 1; t >= 0; t--) {
            int q = P.poolIndex(t, P.keyFor(t, club));
            if (q >= 0) { P.pools[q].clubs.push_back(club); newTier[club] = t; return; }
        }
    };
    for (auto& kv : newTier) place(kv.first, kv.second);
    // ajustement des effectifs, du haut vers le bas
    for (int t = 0; t < T; t++) {
        for (int q = 0; q < (int)P.pools.size(); q++) {
            Pool& pl = P.pools[q];
            if (pl.tier != t || P.tiers[t].flexible || pl.terminal) continue;
            int target = poolTarget(P, pl);
            int guard = 0;
            while ((int)pl.clubs.size() > target && guard++ < 500) {
                int worst = -1; float wv = -1e9;
                for (int c : pl.clubs) {
                    float v = rank[c].ratio * 10 - rank[c].ppg + (oldTier[c] == t ? 100 : 0);
                    if (v > wv) { wv = v; worst = c; }
                }
                pl.clubs.erase(std::find(pl.clubs.begin(), pl.clubs.end(), worst));
                size_t before = 0; for (auto& p2 : P.pools) before += p2.clubs.size();
                place(worst, t + 1);
                size_t after = 0; for (auto& p2 : P.pools) after += p2.clubs.size();
                if (after == before || newTier[worst] == t) { pl.clubs.push_back(worst); break; }
            }
            guard = 0;
            while ((int)pl.clubs.size() < target && guard++ < 500) {
                int bestc = -1; float bv = 1e9; int bq = -1;
                for (int t2 = t + 1; t2 < T && t2 <= t + 1 && bestc < 0; t2++) {     // jamais deux niveaux d'un coup
                    for (int q2 = 0; q2 < (int)P.pools.size(); q2++) {
                        Pool& lo = P.pools[q2];
                        if (lo.tier != t2) continue;
                        for (int c : lo.clubs) {
                            if (P.tiers[t].scope != SC_NATIONAL && P.keyFor(t, c) != pl.key) continue;
                            if (oldTier[c] != t2) continue;
                            if (!eligible(c, t)) continue;
                            float v = rank[c].ratio * 10 - rank[c].ppg;
                            if (v < bv) { bv = v; bestc = c; bq = q2; }
                        }
                    }
                }
                if (bestc < 0) break;
                auto& lc = P.pools[bq].clubs;
                lc.erase(std::find(lc.begin(), lc.end(), bestc));
                pl.clubs.push_back(bestc);
                newTier[bestc] = t;
            }
        }
    }
    // contrôle final des réserves (après ajustement des effectifs) : une réserve descend sous son équipe supérieure,
    // le meilleur club éligible du niveau inférieur prend sa place
    {
        std::vector<int> res;
        for (auto& kv : newTier) if (g_world.teams[kv.first].parent >= 0) res.push_back(kv.first);
        std::stable_sort(res.begin(), res.end(), [](int a, int b) { return g_world.teams[a].resLevel < g_world.teams[b].resLevel; });
        for (int r : res) {
            int m = parentTier(r) + 1;
            while (m < T && !tierAllowsReserve(P, m)) m++;
            int cur = newTier[r];
            if (cur >= m || m >= T) continue;
            int qa = P.poolIndex(cur, P.keyFor(cur, r));
            if (qa < 0) continue;
            int tgt = -1;
            for (int t = m; t < T && tgt < 0; t++) { int q = P.poolIndex(t, P.keyFor(t, r)); if (q >= 0) tgt = q; }
            if (tgt < 0) continue;   // dernier niveau du district : même division tolérée, dans une autre poule
            auto& A = P.pools[qa].clubs;
            auto it = std::find(A.begin(), A.end(), r);
            if (it == A.end()) continue;
            A.erase(it);
            P.pools[tgt].clubs.push_back(r);
            newTier[r] = P.pools[tgt].tier;
            if (P.tiers[cur].flexible || P.pools[qa].terminal) continue;
            int bestc = -1, bq = -1; float bv = 1e9;
            for (int q2 = 0; q2 < (int)P.pools.size(); q2++) {
                const Pool& lo = P.pools[q2];
                if (lo.tier != cur + 1) continue;
                for (int c : lo.clubs) {
                    if (c == r) continue;
                    if (oldTier.count(c) && oldTier[c] != cur + 1) continue;       // un promu de l'étage inférieur ne saute pas un niveau
                    if (P.tiers[cur].scope != SC_NATIONAL && P.keyFor(cur, c) != P.pools[qa].key) continue;
                    if (!eligible(c, cur)) continue;
                    float v = rank[c].ratio * 10 - rank[c].ppg;
                    if (v < bv) { bv = v; bestc = c; bq = q2; }
                }
            }
            if (bestc < 0) continue;
            auto& lc = P.pools[bq].clubs;
            lc.erase(std::find(lc.begin(), lc.end(), bestc));
            A.push_back(bestc);
            newTier[bestc] = cur;
        }
    }
    // évolution des niveaux
    for (auto& pl : P.pools) {
        for (int c : pl.clubs) {
            Team& tm = g_world.teams[c];
            float before = tm.rating;
            if (P.country == "FRA" && (pl.tier >= 3 || P.dom >= 0)) {
                int ti = P.dom >= 0 ? pl.tier + 5 : pl.tier;
                float base = frTierBase(ti);
                tm.rating += (base - tm.rating) * 0.25f + g_rng.frange(-1.5f, 1.5f);
            } else {
                float perf = 0.5f - rank[c].ratio;
                tm.rating += perf * 2.0f + g_rng.frange(-1.2f, 1.2f);
                if (pl.tier != oldTier[c]) tm.rating += (pl.tier < oldTier[c]) ? 2.0f : -2.0f;
            }
            if (tm.youth && tm.parent >= 0) tm.rating += (14.f + g_world.teams[tm.parent].rating * 0.6f - tm.rating) * 0.3f;
            tm.rating = std::max(8.f, std::min(95.f, tm.rating));
            int delta = (int)std::lround(tm.rating - before);
            if (tm.squadGen && delta != 0) {
                for (auto& p : tm.squad) {
                    auto adj = [&](uint8_t& v) { v = (uint8_t)std::max(5, std::min(99, (int)v + delta)); };
                    adj(p.speed); adj(p.shoot); adj(p.pass); adj(p.tackle); adj(p.stamina); if (p.pos == POS_GK) adj(p.keep);
                }
            }
            tm.lastTier = pl.tier;
        }
    }
    formGroups(P);
}

// points UEFA de la saison (victoire 2, nul 1 ; moitié en qualifications ; bonus phase de groupes et tours finaux)
static void computeUefaPoints(Career& K) {
    std::map<std::string, float> pts;
    std::map<int, float> cp;                   // points des clubs
    std::map<std::string, std::set<int>> clubs;
    auto code = [](int t) { int n = g_world.teams[t].nation; return n >= 0 ? std::string(NATIONS[n].code) : std::string(); };
    for (int ci : { K.ucl, K.uel, K.uecl }) {
        if (ci < 0 || ci >= (int)K.season.comps.size()) continue;
        const Competition& C = K.season.comps[ci];
        for (int s = 0; s < (int)C.stages.size(); s++) {
            const Stage& st = C.stages[s];
            bool qual = (C.format == FMT_UCL2000 && s < 3) || (C.format == FMT_UEFA2000 && s == 0);
            if (C.format == FMT_NEWEURO) {
                for (int k = 0; k < C.regionalRounds && k < (int)C.koNames.size(); k++) if (st.name == C.koNames[k]) qual = true;
                if (st.type == ST_SWISS && !st.groups.empty()) {
                    // phase de ligue : bonus de participation, bonus des 8 premiers (qualifiés directs pour les 8es)
                    float part = C.kind == 3 ? 6.f : C.kind == 8 ? 4.f : 2.5f, top = C.kind == 3 ? 4.f : C.kind == 8 ? 2.f : 1.f;
                    for (int t : st.groups[0]) { pts[code(t)] += part; cp[t] += part; }
                    for (int i = 0; i < 8 && i < (int)C.extra.size(); i++) { pts[code(C.extra[i])] += top; cp[C.extra[i]] += top; }
                }
            }
            float k = qual ? 0.5f : 1.f;
            for (auto& g : st.groups) for (int t : g) clubs[code(t)].insert(t);
            for (auto& t : st.ties) { if (t.a >= 0) clubs[code(t.a)].insert(t.a); if (t.b >= 0) clubs[code(t.b)].insert(t.b); }
            for (auto& R : st.rounds) for (int mi : R.m) {
                const MatchRes& m = C.matches[mi];
                if (!m.played) continue;
                if (m.hg > m.ag) { pts[code(m.home)] += 2 * k; cp[m.home] += 2 * k; }
                else if (m.hg < m.ag) { pts[code(m.away)] += 2 * k; cp[m.away] += 2 * k; }
                else { pts[code(m.home)] += k; pts[code(m.away)] += k; cp[m.home] += k; cp[m.away] += k; }
            }
            // bonus
            if (C.format == FMT_UCL2000 && st.type == ST_LEAGUE && s == 3) for (auto& g : st.groups) for (int t : g) { pts[code(t)] += 1; cp[t] += 1; }
            if (st.type == ST_KO && !qual && (st.name.find("Quarts") != std::string::npos || st.name.find("Demi") != std::string::npos || st.name == "Finale" || (C.format == FMT_NEWEURO && st.name == "8es de finale")))
                for (auto& t : st.ties) { if (t.a >= 0) { pts[code(t.a)] += 1; cp[t.a] += 1; } if (t.b >= 0) { pts[code(t.b)] += 1; cp[t.b] += 1; } }
        }
    }
    // Coupe Intertoto : 3e tour, demi-finales et finales comptent comme des tours de qualification ;
    // points ajoutés au pays, mais les clubs de l'Intertoto n'entrent pas dans le diviseur
    if (K.intertoto >= 0 && K.intertoto < (int)K.season.comps.size()) {
        const Competition& C = K.season.comps[K.intertoto];
        for (int s = 0; s < (int)C.stages.size(); s++) {
            const std::string& nm = C.stages[s].name;
            if (nm.find("3e tour") == std::string::npos && nm.find("Demi") == std::string::npos && nm.find("Finales") == std::string::npos) continue;
            for (auto& R : C.stages[s].rounds) for (int mi : R.m) {
                const MatchRes& m = C.matches[mi];
                if (!m.played) continue;
                if (m.hg > m.ag) { pts[code(m.home)] += 1; cp[m.home] += 1; }
                else if (m.hg < m.ag) { pts[code(m.away)] += 1; cp[m.away] += 1; }
                else { pts[code(m.home)] += 0.5f; pts[code(m.away)] += 0.5f; cp[m.home] += 0.5f; cp[m.away] += 0.5f; }
            }
        }
    }
    for (auto& u : K.uefa) {
        int n = (int)clubs[u.code].size();
        float v = n > 0 ? pts[u.code] / n : 0;
        for (int i = 0; i < 4; i++) u.pts[i] = u.pts[i + 1];
        u.pts[4] = v;
    }
    for (int t = 0; t < (int)g_world.teams.size(); t++) {
        Team& T = g_world.teams[t];
        if (T.kind != TK_CLUB) continue;
        if (T.parent >= 0 || T.youth) { for (float& c : T.coefs) c = 0; continue; }     // réserves et jeunes : pas de coefficient UEFA
        for (int i = 0; i < 4; i++) T.coefs[i] = T.coefs[i + 1];
        auto it = cp.find(t);
        T.coefs[4] = it != cp.end() ? it->second : 0;
    }
}

void ageSquads(int userTeam, std::vector<std::string>& news);

void Career::endSeason() {
    Season& S = season;
    std::string y = seasonLabel(year);
    auto champ = [&](int comp) { return comp >= 0 && S.comps[comp].winner >= 0 ? g_world.teams[S.comps[comp].winner].name : std::string("?"); };
    // palmarès : tous les championnats
    for (int p = 0; p < (int)pyramids.size(); p++) {
        for (auto& pl : pyramids[p].pools) for (int c : pl.comps) {
            const Competition& C = S.comps[c];
            if (C.winner >= 0) addHonour(C.winner, "Champion : " + C.name + " (" + y + ")");
        }
        if (pyramids[p].dom >= 0) continue;
        int q = pyramids[p].poolIndex(0, 0);
        if (q >= 0 && !pyramids[p].pools[q].comps.empty()) history.push_back(y + " " + S.comps[pyramids[p].pools[q].comps[0]].name + " : " + champ(pyramids[p].pools[q].comps[0]));
    }
    if (cdf >= 0) history.push_back(y + " Coupe de France : " + champ(cdf));
    for (int c : nationalCups) history.push_back(y + " " + S.comps[c].name + " : " + champ(c));
    if (u19Final >= 0) history.push_back(y + " Champion de France U19 : " + champ(u19Final));
    if (u17Final >= 0) history.push_back(y + " Champion de France U17 : " + champ(u17Final));
    if (gambNational >= 0) history.push_back(y + " Coupe Gambardella : " + champ(gambNational));
    history.push_back(y + " Ligue des Champions : " + champ(ucl));
    history.push_back(y + (opts.euroFormat ? " Ligue Europa : " : " Coupe UEFA : ") + champ(uel));
    if (uecl >= 0) history.push_back(y + " Ligue Conférence : " + champ(uecl));
    {
        int p, q, g;
        if (tierOfTeam(userTeam, &p, &q, &g) >= 0) {
            const Competition& C = S.comps[pyramids[p].pools[q].comps[g]];
            int pos = 1;
            for (int i = 0; i < (int)C.result.size(); i++) if (C.result[i] == userTeam) pos = i + 1;
            history.push_back(y + " " + g_world.teams[userTeam].name + " : " + fmt("%d", pos) + (pos == 1 ? "er" : "e") + " en " + C.name);
        }
    }
    // tenants pour les supercoupes
    for (int c : regionalCups) if (S.comps[c].done && S.comps[c].winner >= 0) prevRegCupWinner[S.comps[c].tag] = S.comps[c].winner;
    {   // champions de Régional 1 (meilleur premier des poules de la région)
        int fr = pyramidOf(pyramids, "FRA");
        prevR1Champ.clear();
        if (fr >= 0) {
            std::map<int, float> best;
            for (auto& pl : pyramids[fr].pools) {
                if (pl.tier != 5) continue;
                for (int ci : pl.comps) {
                    const Competition& C = S.comps[ci];
                    if (C.result.empty()) continue;
                    auto tb = C.table(0, 0);
                    float v = tb.empty() || !tb[0].p ? 0.f : (float)tb[0].pts / tb[0].p;
                    int ch = C.result[0];
                    if (g_world.teams[ch].parent >= 0) continue;
                    if (!best.count(pl.key) || v > best[pl.key]) { best[pl.key] = v; prevR1Champ[pl.key] = ch; }
                }
            }
        }
    }
    prevUclWinner = ucl >= 0 ? S.comps[ucl].winner : -1;
    prevUefaWinner = uel >= 0 ? S.comps[uel].winner : -1;
    prevUeclWinner = uecl >= 0 ? S.comps[uecl].winner : -1;
    for (int p = 0; p < (int)pyramids.size(); p++) {
        if (pyramids[p].dom >= 0) continue;
        CountryRes r = countryResult(*this, pyramids[p].country, true);
        if (r.standings.size() >= 2) { prevChampion[pyramids[p].country] = r.standings[0]; prevRunnerUp[pyramids[p].country] = r.standings[1]; }
        if (r.cup >= 0) prevCupWinner[pyramids[p].country] = r.cup;
    }
    std::vector<std::string> msgs;
    if (kind == CK_CLUB && !euroOnly) { mgrEndSeason(msgs); fpfCheck(msgs); returnLoans(msgs); }      // avant les montées/descentes : classement final réel
    computeUefaPoints(*this);
    computeEuro(*this, true);
    std::map<int, RankInfo> rank;
    // journal : montées et descentes (club du joueur et ses équipes, clubs de L1 à National 2)
    std::map<int, int> tierBefore;
    {
        int frp = pyramidOf(pyramids, "FRA");
        if (frp >= 0) for (auto& pl : pyramids[frp].pools) if (pl.tier <= 3) for (int c : pl.clubs) if (g_world.teams[c].parent < 0) tierBefore[c] = pl.tier;
        if (kind == CK_CLUB) for (int i = 0; i < (int)g_world.teams.size(); i++) if (i == userTeam || g_world.teams[i].parent == userTeam) { int tb = tierOfTeam(i); if (tb >= 0) tierBefore[i] = tb; }
    }
    for (int p = 0; !euroOnly && p < (int)pyramids.size(); p++) movePyramid(*this, p, rank);
    for (auto& kv : tierBefore) {
        int ta = tierOfTeam(kv.first);
        if (ta < 0 || ta == kv.second) continue;
        const Team& t = g_world.teams[kv.first];
        bool mine = kv.first == userTeam || t.parent == userTeam;
        std::string lvl = teamLevelName(kv.first);
        std::string cut = lvl.substr(0, lvl.find(" - "));
        if (ta < kv.second) msgs.push_back(std::string(mine ? "[Votre club] " : "") + "PROMOTION : " + t.name + " monte en " + cut + " !");
        else msgs.push_back(std::string(mine ? "[Votre club] " : "") + "RELÉGATION : " + t.name + " descend en " + cut + ".");
        addReputation(kv.first, ta < kv.second ? 4 : -5);
    }
    // corruption prouvée : rétrogradation administrative d'une division
    if (kind == CK_CLUB && life.adminRelegate && userTeam >= 0) {
        life.adminRelegate = 0;
        int p, q, g;
        int cur = tierOfTeam(userTeam, &p, &q, &g);
        if (cur >= 0) {
            Pyramid& P = pyramids[p];
            int tq = -1;
            for (int t2 = cur + 1; t2 < (int)P.tiers.size() && tq < 0; t2++) tq = P.poolIndex(t2, P.keyFor(t2, userTeam));
            if (tq >= 0) {
                Pool& A = P.pools[q];
                A.clubs.erase(std::remove(A.clubs.begin(), A.clubs.end(), userTeam), A.clubs.end());
                for (auto& gr : A.groups) gr.erase(std::remove(gr.begin(), gr.end(), userTeam), gr.end());
                Pool& B = P.pools[tq];
                B.clubs.push_back(userTeam);
                if (B.groups.empty()) B.groups.push_back({});
                int gi = 0; for (int k = 1; k < (int)B.groups.size(); k++) if (B.groups[k].size() < B.groups[gi].size()) gi = k;
                B.groups[gi].push_back(userTeam);
                msgs.push_back("[Votre club] AFFAIRE DE CORRUPTION : rétrogradation administrative en " + P.tiers[B.tier].name + ".");
                addReputation(userTeam, -15);
            }
        }
    }
    // dépôt de bilan : rétrogradation administrative (au moins deux divisions, jusqu'au Régional 1)
    if (kind == CK_CLUB && mgr.managerMode && mgr.bankrupt && userTeam >= 0) {
        mgr.bankrupt = 0;
        int p, q, g;
        int cur = tierOfTeam(userTeam, &p, &q, &g);
        if (cur >= 0) {
            Pyramid& P = pyramids[p];
            int target = std::min((int)P.tiers.size() - 1, std::max(cur + 2, 5));
            int tq = -1;
            for (int t2 = target; t2 < (int)P.tiers.size() && tq < 0; t2++) tq = P.poolIndex(t2, P.keyFor(t2, userTeam));
            if (tq >= 0) {
                Pool& A = P.pools[q];
                A.clubs.erase(std::remove(A.clubs.begin(), A.clubs.end(), userTeam), A.clubs.end());
                for (auto& gr : A.groups) gr.erase(std::remove(gr.begin(), gr.end(), userTeam), gr.end());
                Pool& B = P.pools[tq];
                B.clubs.push_back(userTeam);
                if (B.groups.empty()) B.groups.push_back({});
                int gi = 0; for (int k = 1; k < (int)B.groups.size(); k++) if (B.groups[k].size() < B.groups[gi].size()) gi = k;
                B.groups[gi].push_back(userTeam);
                msgs.push_back("[Votre club] DÉPÔT DE BILAN : le club repart en " + P.tiers[B.tier].name + " (rétrogradation administrative).");
            }
        }
        Team& U = g_world.teams[userTeam];
        // les contrats professionnels sont rompus : les joueurs les mieux payés s'en vont
        std::vector<std::pair<int, int>> w; for (auto& pl : U.squad) w.push_back({ -playerWage(pl), pl.id });
        std::sort(w.begin(), w.end());
        for (size_t k = 0; k < w.size() && U.squad.size() > 18 && k < 10; k++) {
            int idx; if (g_world.findPlayer(w[k].second, &idx) == userTeam) { Player pl = U.squad[idx]; U.squad.erase(U.squad.begin() + idx); pl.wageK = 0; pl.years = 1; int fa = freeAgentsTeam(); g_world.teams[fa].squad.push_back(pl); }
        }
        U.xi.clear();
        addReputation(userTeam, -25);
        mgr.budget = std::max<int64_t>(10, mgr.incomeBase / 20); mgr.dncg = 0; mgr.confidence = 50;
    }
    // clubs créés en cours de saison
    int fr = pyramidOf(pyramids, "FRA");
    int ypy = u19Pyramid();
    for (int t : pendingNewClubs) {
        if (g_world.teams[t].youth && g_world.teams[t].youth != 6) { int py = youthPyramid(t); if (py >= 0) placeInBottomPool(pyramids[py], t); }
        else if (fr >= 0) placeInBottomPool(pyramids[fr], t);
    }
    if (fr >= 0 && !pendingNewClubs.empty()) formGroups(pyramids[fr]);
    if (ypy >= 0 && !pendingNewClubs.empty()) formGroups(pyramids[ypy]);
    { int p17 = u17Pyramid(); if (p17 >= 0 && !pendingNewClubs.empty()) formGroups(pyramids[p17]); }
    { int p15 = u15Pyramid(); if (p15 >= 0 && !pendingNewClubs.empty()) formGroups(pyramids[p15]); }
    pendingNewClubs.clear();
    archiveSeason();                  // avant l'historique des joueurs (qui remet les compteurs à zéro)
    archiveComps();
    recordSeasonHistory();
    ageSquads(userTeam, msgs);
    for (auto& t : g_world.teams) if (t.squadGen) for (auto& pl : t.squad) { pl.suspended = 0; pl.yellows = 0; pl.goals = 0; pl.apps = 0; pl.assists = 0; pl.injured = 0; }
    retiring.clear();
    mgr.boardRequests = 0;
    year++;
    startSeason();
    if (kind == CK_CLUB && !euroOnly) { mgrInit(); aiTransfers(120); refreshFreeAgents(40); }
    else if (euroOnly) aiTransfers(60);
    for (auto& n : msgs) season.news.push_back(n);
}

// ------------------------------------------------------------------ jeunes (U19)
int Career::u19Pyramid() const { for (int i = 0; i < (int)pyramids.size(); i++) if (pyramids[i].country == "U19") return i; return -1; }
int Career::u17Pyramid() const { for (int i = 0; i < (int)pyramids.size(); i++) if (pyramids[i].country == "U17") return i; return -1; }
int Career::u15Pyramid() const { for (int i = 0; i < (int)pyramids.size(); i++) if (pyramids[i].country == "U15") return i; return -1; }
int Career::youthPyramid(int team) const { int y = g_world.teams[team].youth; return y == 3 ? u15Pyramid() : y == 2 ? u17Pyramid() : u19Pyramid(); }

void Career::onU19CompDone(int comp) {
    Season& S = season;
    const Competition& C = S.comps[comp];
    // National U19 / U17 : les deux premiers de chaque poule disputent la phase finale (quarts, demies, finale en matchs secs)
    auto natFinal = [&](int yp, const char* A, int kd, int& out) {
        if (yp < 0 || out >= 0 || C.format != FMT_LEAGUE || C.tag / 100000 != yp) return false;
        const Pyramid& Y = pyramids[yp];
        int q0 = Y.poolIndex(0, 0);
        if (q0 < 0 || Y.pools[q0].comps.empty()) return false;
        bool mine = false;
        for (int c : Y.pools[q0].comps) { if (c == comp) mine = true; }
        if (!mine) return false;
        for (int c : Y.pools[q0].comps) if (!S.comps[c].done) return true;
        std::vector<std::vector<int>> top;
        for (int c : Y.pools[q0].comps) top.push_back(S.comps[c].result);
        std::vector<std::pair<int, int>> pairs;
        int G = (int)top.size();
        for (int g = 0; g < G; g++) {
            const auto& a = top[g]; const auto& b = top[(g + 1) % G];
            if (a.size() >= 1 && b.size() >= 2) pairs.push_back({ a[0], b[1] });
        }
        if (pairs.size() < 2) return true;
        Competition n;
        n.format = FMT_KO_ONLY; n.name = std::string("Championnat National ") + A + " - phase finale"; n.shortName = std::string("National ") + A + " (finale)"; n.kind = kd; n.qualSpots = 1;
        n.neutralFinal = true;
        double t0 = std::max(36.3, S.now + 0.4);
        n.koLegs = { 1, 1, 1 }; n.koTimes = { t0, t0 + 1.0, t0 + 2.0 };
        n.koNames = { "Quarts de finale", "Demi-finales", "Finale" };
        n.addKOStage(pairs, 1, t0, "Quarts de finale");
        out = addComp(S, std::move(n));
        S.news.push_back(std::string("National ") + A + " : la phase finale oppose les deux premiers de chaque poule (quarts, demies et finale en matchs secs).");
        return true;
    };
    if (natFinal(u17Pyramid(), "U17", 22, u17Final)) return;
    if (comp == u17Final && C.winner >= 0) { lastU17Champ = C.winner; addHonour(C.winner, "Champion de France U17 (" + seasonLabel(year) + ")"); S.news.push_back(g_world.teams[C.winner].name + " est sacré champion de France U17 !"); return; }
    int yp = u19Pyramid();
    if (yp < 0) return;
    if (natFinal(yp, "U19", 19, u19Final)) return;
    if (comp == youthPrelim && youthUcl < 0) {
        std::vector<int> v = youthDirect;
        for (int t : C.result) v.push_back(t);
        startYouthGroups(v);
        return;
    }
    if (comp == youthUcl && C.winner >= 0) { S.news.push_back(g_world.teams[C.winner].name + " remporte la Ligue des champions U19 !"); return; }
    if (comp == u19Final && C.winner >= 0) { lastU19Champ = C.winner; addHonour(C.winner, "Champion de France U19 (" + seasonLabel(year) + ")"); S.news.push_back(g_world.teams[C.winner].name + " est sacré champion de France U19 !"); return; }
    if (comp == gambNational && C.winner >= 0) { S.news.push_back(g_world.teams[C.winner].name + " remporte la Coupe Gambardella !"); return; }
    // Gambardella : phase nationale quand toutes les ligues ont désigné leurs qualifiés
    if (std::find(gambRegional.begin(), gambRegional.end(), comp) != gambRegional.end() && gambNational < 0) {
        for (int c : gambRegional) if (!S.comps[c].done) return;
        Competition n;
        n.format = FMT_CUP; n.name = "Coupe Gambardella"; n.shortName = "Gambardella"; n.kind = 15; n.homeRule = 1; n.neutralFinal = true;
        n.entrants.assign(1, {});
        for (int c : gambRegional) for (int t : S.comps[c].result) n.entrants[0].push_back(t);
        const Pyramid& Y = pyramids[yp];
        for (auto& pl : Y.pools) if (pl.tier == 0) for (int t : pl.clubs) n.entrants[0].push_back(t);
        n.koTargets = { 64, 32, 16, 8, 4, 2, 1 };
        n.koNames = { "1er tour fédéral", "32es de finale", "16es de finale", "8es de finale", "Quarts de finale", "Demi-finales", "Finale" };
        n.koTimes = { std::max(18.0, S.now + 0.5), 22, 25, 28, 31.5, 35, 39.5 };
        n.regionalDraw = true; n.regionalRounds = 1;
        int idx = addComp(S, std::move(n));
        S.comps[idx].cupRound(0, S.comps[idx].entrants[0]);
        gambNational = idx;
        S.news.push_back("Coupe Gambardella : début de la phase nationale avec l'entrée des clubs du National U19.");
    }
}

// ------------------------------------------------------------------ Ligue des champions U19
// seuls les champions nationaux U19 ; entrée selon le coefficient UEFA du pays ; aucun point UEFA
static int youthOf(int club) {
    for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].parent == club && g_world.teams[i].youth == 1) return i;
    int id = makeU19Team(g_world, club);
    g_world.teams[id].lastTier = -1;
    return id;
}

void Career::setupYouthUcl() {
    Season& S = season;
    youthPrelim = youthUcl = -1; youthDirect.clear();
    int yp = u19Pyramid();
    if (yp < 0 || euroOnly || kind != CK_CLUB) return;
    std::vector<std::pair<int, int>> ent;   // (rang du pays, équipe)
    std::set<int> seen;
    for (auto& u : uefa) {
        int rank = uefaRank(u.code), team = -1;
        if (u.code == "FRA") {
            team = lastU19Champ;
            if (team < 0 || team >= (int)g_world.teams.size() || g_world.teams[team].youth != 1) {
                team = -1; float best = -1;
                for (auto& pl : pyramids[yp].pools) if (pl.tier == 0) for (int t : pl.clubs) if (g_world.teams[t].rating > best) { best = g_world.teams[t].rating; team = t; }
            }
        } else {
            if (u.code == "LIE") continue;   // pas de championnat
            int club = prevChampion.count(u.code) ? prevChampion[u.code] : -1;
            if (club < 0) {
                auto it = g_world.countryClubs.find(u.code);
                if (it != g_world.countryClubs.end()) { float best = -1; for (int t : it->second) if (g_world.teams[t].parent < 0 && g_world.teams[t].rating > best) { best = g_world.teams[t].rating; club = t; } }
            }
            if (club < 0) continue;
            if (g_world.teams[club].parent >= 0) club = g_world.teams[club].parent;
            team = youthOf(club);
        }
        if (team >= 0 && !seen.count(team)) { seen.insert(team); ent.push_back({ rank, team }); }
    }
    std::sort(ent.begin(), ent.end());
    int N = (int)ent.size();
    if (N < 8) return;
    if (N <= 32) {
        std::vector<int> v; for (auto& e : ent) v.push_back(e.second);
        startYouthGroups(v);
        return;
    }
    int D = std::max(0, 64 - N);                 // qualifiés directs : les pays les mieux classés
    for (int i = 0; i < D; i++) youthDirect.push_back(ent[i].second);
    std::vector<int> pre;
    for (int i = D; i < N; i++) pre.push_back(ent[i].second);
    std::vector<std::pair<int, int>> pairs;
    int P = (int)pre.size();
    for (int i = 0; i < P / 2; i++) {
        int a = pre[i], b = pre[P - 1 - i];
        if (g_rng.chance(0.5f)) std::swap(a, b);
        pairs.push_back({ a, b });
    }
    if (P % 2) youthDirect.push_back(pre[P / 2]);
    Competition c;
    c.format = FMT_KO_ONLY; c.kind = 20; c.qualSpots = (int)pairs.size();
    c.name = "Ligue des champions U19 - tour préliminaire"; c.shortName = "UYL (tour prél.)";
    c.koLegs = { 1 }; c.koTimes = { 5.2 }; c.koNames = { "Tour préliminaire" };
    c.addKOStage(pairs, 1, 5.2, "Tour préliminaire");
    youthPrelim = addComp(S, std::move(c));
}

void Career::startYouthGroups(const std::vector<int>& teams0) {
    Season& S = season;
    std::vector<std::pair<int, int>> r;
    for (int t : teams0) { int c = clubOf(t); r.push_back({ uefaRank(sanitize(NATIONS[std::max(0, g_world.teams[c].nation)].code)), t }); }
    std::stable_sort(r.begin(), r.end());
    std::vector<int> teams; for (auto& x : r) teams.push_back(x.second);
    int n = std::min(32, (int)teams.size());
    int ng = n >= 32 ? 8 : n >= 16 ? 4 : 2;
    int per = n / ng;
    std::vector<std::vector<int>> groups(ng);
    for (int pot = 0; pot < per; pot++) {
        std::vector<int> v(teams.begin() + pot * ng, teams.begin() + (pot + 1) * ng);
        g_rng.shuffle(v);
        for (int g = 0; g < ng; g++) groups[g].push_back(v[g]);
    }
    Competition c;
    c.format = FMT_TOURNAMENT; c.kind = 21; c.tb = TB_H2H; c.legs = 1; c.groupsAdvance = 2; c.bestThirds = 0;
    c.name = "Ligue des champions U19"; c.shortName = "Ligue des champions U19";
    c.koTimes = { 26.2, 29.2, 33.2, 38.5 };
    std::vector<double> times = { 7.2, 9.2, 12.2, 14.2, 18.2, 20.2 };
    c.addGroupStage(groups, 2, times, "Phase de groupes");
    youthUcl = addComp(S, std::move(c));
    S.news.push_back(fmt("Ligue des champions U19 : tirage de la phase de groupes (%d équipes, %d groupes de %d, aller-retour ; phase finale en matchs secs).", n, ng, per));
}

int Career::createU19(int club, std::string& err) { return createYouth(club, 1, err); }
int Career::createYouth(int club, int ykind, std::string& err) {
    const char* A = ykind == 3 ? "U15" : ykind == 2 ? "U17" : "U19";
    for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].parent == club && g_world.teams[i].youth == ykind) { err = std::string("Le club a déjà une équipe ") + A + "."; return -1; }
    for (int t : pendingNewClubs) if (g_world.teams[t].parent == club && g_world.teams[t].youth == ykind) { err = std::string("L'équipe ") + A + " est déjà créée."; return -1; }
    int idx = makeYouthTeam(g_world, club, ykind);
    Team& t = g_world.teams[idx];
    t.rating = std::max(10.f, g_world.teams[club].rating * (ykind == 3 ? 0.35f : ykind == 2 ? 0.4f : 0.45f) + 8.f);   // section naissante
    t.founded = year;
    initStadium(t, 8, -1);
    if (kind == CK_CLUB && !season.comps.empty()) {
        pendingNewClubs.push_back(idx);
        season.news.push_back(t.name + " est créée : elle débutera la saison prochaine en District " + A + ".");
    } else {
        int yp = ykind == 3 ? u15Pyramid() : ykind == 2 ? u17Pyramid() : u19Pyramid();
        if (yp >= 0) { placeInBottomPool(pyramids[yp], idx); }
    }
    return idx;
}

// ------------------------------------------------------------------ mode Coupes d'Europe
std::vector<int> Career::euroCandidates() {
    Career* K = new Career();
    K->pyramids = g_basePyramids;
    K->euroOnly = true;
    K->initUefa();
    auto byName = [](const char* nm) { for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == nm && g_world.teams[i].parent < 0) return i; return -1; };
    K->prevUclWinner = byName("Paris Saint-Germain"); K->prevUefaWinner = byName("Aston Villa");
    computeEuro(*K, false);
    std::vector<int> v;
    for (auto* L : { &K->nextEuro.uclGS, &K->nextEuro.uclQ3, &K->nextEuro.uclQ2, &K->nextEuro.uclQ1, &K->nextEuro.uefaR1, &K->nextEuro.uefaQR }) for (int t : *L) v.push_back(t);
    delete K;
    return v;
}

void Career::newEuroCareer(const std::vector<int>& ctrl, int yr) {
    coach = false;
    newClubCareer(ctrl.empty() ? 0 : ctrl[0], yr);      // base commune (coefficients, tenants, manager)
    // on repart d'une saison vide avec uniquement les coupes d'Europe
    euroOnly = true;
    season.controlled = ctrl;
    mgr.managerMode = false; mgr.noSack = true;
    computeEuro(*this, false);
    startSeason();
}

// ------------------------------------------------------------------ vieillissement des effectifs
void ageSquads(int userTeam, std::vector<std::string>& news) {
    Rng& r = g_rng;
    for (int ti = 0; ti < (int)g_world.teams.size(); ti++) {
        Team& t = g_world.teams[ti];
        if (!t.squadGen || t.kind != TK_CLUB) continue;
        for (int i = 0; i < (int)t.squad.size(); i++) {
            Player& p = t.squad[i];
            p.age++;
            int d = 0;
            int ov = p.overall();
            int train = ti == userTeam ? g_career.staffLevel(SR_PHYSIO_PREP) : 0;
            if (ti == userTeam && p.pos == POS_GK) train += g_career.staffLevel(SR_GK);
            if (p.age <= 21) d = std::min(r.range(1, 5) + (r.chance(train * 0.12f) ? 1 : 0), std::max(0, p.pot - ov));
            else if (p.age <= 24) d = std::min(r.range(0, 3), std::max(0, p.pot - ov));
            else if (p.age <= 29) d = r.range(-1, 1);
            else if (p.age <= 32) d = r.range(-3, 0);
            else d = r.range(-5, -1);
            auto adj = [&](uint8_t& v) { v = (uint8_t)std::max(5, std::min(99, (int)v + d)); };
            if (d) { adj(p.speed); adj(p.shoot); adj(p.pass); adj(p.tackle); adj(p.stamina); if (p.pos == POS_GK) adj(p.keep); }
            if (t.youth && t.parent >= 0 && p.age >= (t.youth == 3 ? 15 : t.youth == 2 ? 17 : 20)) {
                // trop âgé : U17 -> U19 du club ; U19 -> réserve (ou équipe première) ; un jeune arrive à sa place
                int dest = t.parent;
                for (int k = 0; k < (int)g_world.teams.size(); k++) if (g_world.teams[k].parent == t.parent && !g_world.teams[k].youth && g_world.teams[k].resLevel == 1) dest = k;
                if (t.youth == 2) for (int k = 0; k < (int)g_world.teams.size(); k++) if (g_world.teams[k].parent == t.parent && g_world.teams[k].youth == 1) dest = k;
                if (t.youth == 3) {   // U15 -> U17 du club (sinon U19, sinon réserve)
                    int d17 = -1, d19 = -1;
                    for (int k = 0; k < (int)g_world.teams.size(); k++) if (g_world.teams[k].parent == t.parent) { if (g_world.teams[k].youth == 2) d17 = k; if (g_world.teams[k].youth == 1) d19 = k; }
                    if (d19 >= 0) dest = d19;
                    if (d17 >= 0) dest = d17;
                }
                Team& D = g_world.teams[dest];
                bool isUser = t.parent == userTeam;
                if (D.squadGen) {
                    Player q = p;
                    std::vector<bool> usedN(100, false); for (auto& x : D.squad) if (x.num < 100) usedN[x.num] = true;
                    if (q.num >= 100 || usedN[q.num]) { q.num = 1; while (q.num < 99 && usedN[q.num]) q.num++; }
                    D.squad.push_back(q);
                    // effectif trop fourni : le plus faible des plus âgés est libéré
                    if (D.squad.size() > 30) {
                        int worst = -1, wv = 999;
                        for (int k = 0; k < (int)D.squad.size(); k++) { int v = D.squad[k].overall() - (D.squad[k].age > 28 ? 5 : 0); if (v < wv) { wv = v; worst = k; } }
                        if (worst >= 0) { D.xi.clear(); D.squad.erase(D.squad.begin() + worst); }
                    }
                    if (isUser) news.push_back(p.name + fmt(" (%d ans) quitte les %s et rejoint %s.", p.age, t.youth == 3 ? "U15" : t.youth == 2 ? "U17" : "U19", D.name.c_str()));
                }
                int pos = p.pos;
                Player y = g_world.makeYouth(ti, pos, t.rating - 4);
                y.age = (uint8_t)((t.youth == 3 ? 13 : t.youth == 2 ? 15 : 16) + r.range(0, 1));
                t.squad[i] = y;
                t.xi.clear();
                continue;
            }
            bool announced = std::find(g_career.retiring.begin(), g_career.retiring.end(), p.id) != g_career.retiring.end();
            if (announced || p.age >= 38 || (p.age >= 34 && ti != userTeam && r.chance((p.age - 33) * 0.25f)) || (p.age >= 36 && ti == userTeam)) {
                if (ti == userTeam) news.push_back(p.name + fmt(" (%d ans) prend sa retraite.", p.age));
                else if (p.overall() >= 78) news.push_back(fmt("Fin d'une époque : %s (%s, %d ans) raccroche les crampons.", p.name.c_str(), t.shortName.c_str(), p.age));
                // reconversion
                if (t.kind == TK_CLUB && t.parent < 0 && (ti == userTeam || p.overall() >= 62 || r.chance(0.15f)) && r.chance(0.6f)) {
                    static const char* ROLES[] = { "entraîneur des jeunes", "entraîneur adjoint", "recruteur", "consultant TV", "entraîneur d'un club amateur",
                                                   "préparateur physique", "dirigeant", "entraîneur des gardiens", "agent de joueurs", "directeur sportif" };
                    int k = p.pos == POS_GK && r.chance(0.5f) ? 7 : r.range(0, 9);
                    if (k == 7 && p.pos != POS_GK) k = 1;
                    std::string line = fmt("%d : ", g_career.year) + p.name + " (" + t.name + ") devient " + ROLES[k];
                    int sr = k == 0 ? SR_YOUTH : k == 1 ? SR_ADJOINT : k == 2 ? SR_SCOUT : k == 5 ? SR_PHYSIO_PREP : k == 7 ? SR_GK : -1;
                    if (ti == userTeam && sr >= 0 && g_career.staffLevel(sr) == 0) {
                        StaffMember sm = makeStaff(sr, std::max(1, std::min(4, 1 + p.overall() / 25)), t.status, r);
                        snprintf(sm.name, sizeof sm.name, "%s", p.name.c_str()); sm.age = p.age;
                        g_career.mgr.staff.push_back(sm);
                        line += " au club";
                        news.push_back(p.name + " se reconvertit : il rejoint le staff du club (" + ROLES[k] + ").");
                    } else if (ti == userTeam || p.overall() >= 72) news.push_back("Reconversion : " + p.name + " devient " + ROLES[k] + ".");
                    g_career.reconversions.push_back(line);
                    if (g_career.reconversions.size() > 300) g_career.reconversions.erase(g_career.reconversions.begin());
                }
                int pos = p.pos;
                float lvl = t.rating - 6 + (ti == userTeam ? g_career.staffLevel(SR_YOUTH) * 1.5f : 0);
                t.squad[i] = g_world.makeYouth(ti, pos, lvl);
                if (ti == userTeam) news.push_back(t.squad[i].name + fmt(" (%d ans) est promu du centre de formation.", t.squad[i].age));
                t.xi.clear();
            }
        }
    }
}

// ------------------------------------------------------------------ règles d'une compétition (texte structuré)
// Format : « # » titre, « ## » sous-titre, « - » puce, sinon paragraphe.
static void tbLines(int tb, std::string& s) {
    switch (tb) {
    case TB_LFP:
        s += "- 1. Différence de buts générale\n- 2. Points obtenus dans les matchs entre les équipes à égalité\n- 3. Différence de buts dans ces matchs\n- 4. Buts marqués (général)\n- 5. Nombre de victoires, puis victoires à l'extérieur\n";
        break;
    case TB_FFF:
        s += "- 1. Points obtenus dans les matchs entre les équipes à égalité (classement particulier)\n- 2. Différence de buts particulière\n- 3. Différence de buts générale\n- 4. Buts marqués (général)\n";
        break;
    case TB_H2H:
        s += "- 1. Points dans les confrontations directes\n- 2. Différence de buts dans les confrontations directes\n- 3. Différence de buts générale\n- 4. Buts marqués\n";
        break;
    case TB_ENG: case TB_FIFA:
        s += "- 1. Différence de buts générale\n- 2. Buts marqués\n- 3. Points dans les confrontations directes\n";
        break;
    default:
        s += "- 1. Différence de buts\n- 2. Buts marqués\n";
        break;
    }
}

std::string Career::divisionRules(int comp) const {
    const Competition& C = season.comps[comp];
    std::string s = "# " + C.name + "\n";
    auto H = [&](const std::string& t) { s += "## " + t + "\n"; };
    auto P = [&](const std::string& t) { s += t + "\n"; };
    auto B = [&](const std::string& t) { s += "- " + t + "\n"; };
    auto koDraw = [&](bool etOk, bool finalET) {
        H("En cas d'égalité");
        if (!etOk) { B("Pas de prolongation : tirs au but directement (5 tirs chacun, puis mort subite)."); if (finalET) B("Finale : prolongation (2 x 15 min) puis tirs au but."); }
        else B("Prolongation (2 x 15 min), puis tirs au but (5 tirs chacun, puis mort subite).");
    };
    auto twoLegs = [&](bool ag) {
        H("Matchs aller-retour");
        B("L'équipe qui marque le plus de buts sur les deux matchs est qualifiée.");
        if (ag) B("À égalité : les buts marqués à l'extérieur comptent double (y compris pendant la prolongation).");
        else B("Pas de règle des buts à l'extérieur : à égalité, prolongation au match retour.");
        B("Puis tirs au but.");
    };
    bool club = kind == CK_CLUB;
    if (C.format == FMT_LEAGUE && C.kind == 1 && C.tag >= 0 && club) {
        int p = C.tag / 100000, q = (C.tag / 100) % 1000;
        const Pyramid& PY = pyramids[p];
        const Pool& pl = PY.pools[q];
        const TierConf& T = PY.tiers[pl.tier];
        int n = C.stages.empty() || C.stages[0].groups.empty() ? 0 : (int)C.stages[0].groups[0].size();
        H("Format");
        B(fmt("%d équipes, matchs aller-retour (%d journées).", n, n > 1 ? 2 * (n - 1) : 0));
        H("Points");
        B(fmt("Victoire : %d points, nul : 1 point, défaite : 0 point.", C.ptsWin));
        H("Départage en cas d'égalité de points");
        tbLines(C.tb, s);
        if (PY.country == "FRA" || PY.country == "U19" || PY.country == "U17" || PY.country == "U15") H("Montées et descentes");
        else H("Montées et descentes");
        if (pl.tier > 0 && T.up > 0) {
            std::string m = fmt("Montée en %s : %d premier%s", PY.tiers[pl.tier - 1].name.c_str(), T.up, T.up > 1 ? "s" : "");
            if (pl.upCap >= 0) m += fmt(" (au total %d montée%s pour la ligue : les meilleurs premiers)", pl.upCap, pl.upCap > 1 ? "s" : "");
            B(m + ".");
        }
        if (pl.tier == 0 && PY.country == "FRA" && PY.dom < 0) B("Le champion est sacré champion de France. Les places européennes dépendent du coefficient UEFA de la France.");
        if (T.barrageUp == 1) B("Le 3e dispute un barrage aller-retour contre le 16e de la division supérieure.");
        if (T.barrageUp == 2) B(fmt("Play-offs pour une montée supplémentaire (du %de au %de).", T.up + 1, T.up + 4));
        if (T.barrageUp == 3) B("Barrages : le 5e reçoit le 4e, le vainqueur va chez le 3e, puis barrage aller-retour contre le 16e de Ligue 1.");
        if (!T.flexible && !pl.terminal && T.down > 0) B(fmt("Descente : %d dernier%s (plus d'éventuelles descentes en cascade, rétrogradations administratives de la DNCG).", T.down, T.down > 1 ? "s" : ""));
        if (pl.terminal || T.flexible) B("Dernier niveau du district : pas de relégation ; deux équipes d'un même club sont placées dans des poules différentes.");
        if (PY.country == "FRA" && pl.tier >= 3) B("Montées et descentes selon la zone géographique (région, district).");
        H("Équipes réserves");
        if (T.noReserves) B("Division interdite aux équipes réserves.");
        else if (PY.country == "FRA") { B("Autorisées, mais jamais au même niveau ou au-dessus de l'équipe supérieure du club."); B("Une réserve ne peut pas accéder à la Ligue 3."); }
        else B("Selon le règlement du pays.");
        if (isWomenPyramid(PY)) {
            H("Football féminin");
            B("Séparation stricte : seules des joueuses peuvent évoluer dans ce championnat.");
            if (PY.country == "F:FRA" && pl.tier == 0) {
                B("Saison régulière en 22 journées. Les 4 premières disputent les play-offs pour le titre : demi-finales (1re - 4e, 2e - 3e) et finale sur match unique.");
                B("Les 2 dernières descendent en Seconde Ligue.");
            }
            if (PY.country == "F:FRA" && pl.tier == 1) B("22 journées : les 2 premières montent en Arkema Première Ligue, les 2 dernières descendent en D3.");
            if (PY.country == "F:FRA" && pl.tier <= 1) {
                B("Feuille de match : 20 joueuses, 5 remplacements.");
                B("Règle JFL : au moins 10 joueuses formées localement (JFL) sur la feuille de match.");
            }
        }
        if (PY.country == "U15" || PY.country == "U17" || PY.country == "U19") {
            H("Catégorie de jeunes");
            if (PY.country == "U15") { B("Joueurs de moins de 15 ans (U15). Championnats de ligue et de district uniquement : pas de championnat national."); B("En fin de saison, les joueurs trop âgés rejoignent l'équipe U17 (ou U19) du club."); }
            if (PY.country == "U17") { B("Joueurs de moins de 17 ans (U17). En fin de saison, les joueurs trop âgés rejoignent l'équipe U19 du club."); if (pl.tier == 0) B("National U17 : 4 poules de 14 ; les deux premiers de chaque poule disputent la phase finale (quarts, demies, finale en matchs secs)."); if (pl.tier == 1) B("Le meilleur premier de chaque ligue accède au National U17."); }
            if (PY.country == "U19") { B("Joueurs de moins de 20 ans (U19). En fin de saison, les joueurs trop âgés rejoignent la réserve ou l'équipe première."); if (pl.tier == 0) B("National U19 : 4 poules de 14 ; les deux premiers de chaque poule disputent la phase finale (quarts, demies, finale en matchs secs)."); if (pl.tier == 1) B("Le meilleur premier de chaque ligue accède au National U19."); }
        }
    } else if (C.format == FMT_LEAGUE) {
        H("Format"); B("Championnat en matchs aller-retour.");
        H("Points"); B(fmt("Victoire : %d points, nul : 1 point.", C.ptsWin));
        H("Départage en cas d'égalité de points"); tbLines(C.tb, s);
    } else if (C.kind == 26) {
        H("Format"); B("Match unique chez le champion de Régional 1 de la saison précédente, contre le vainqueur de la coupe régionale (ou le 2e de Régional 1).");
        koDraw(false, false);
        H("Qualification"); B("Le vainqueur est qualifié pour la Méga Coupe des Régions.");
    } else if (C.kind == 27) {
        H("Format"); B("Les vainqueurs de toutes les supercoupes de région."); B("Élimination directe en match unique, finale sur terrain neutre.");
        koDraw(false, false); B("Aucune prolongation, même en finale.");
    } else if (C.kind == 28) {
        H("Format"); B("32 clubs, tous les quatre ans, en juin et juillet, dans le pays organisateur.");
        B("8 groupes de 4 (victoire 3 points, nul 1) : les deux premiers en 8es de finale.");
        B("Élimination directe en matchs secs (8es, quarts, demies, finale), pas de match pour la 3e place.");
        H("Départage en groupe"); tbLines(C.tb, s);
        H("Qualification");
        B("UEFA : 12 clubs (vainqueurs de la Ligue des champions des quatre dernières saisons, puis classement des clubs sur quatre saisons).");
        B("CONMEBOL 6, AFC 4, CAF 4, CONCACAF 4, OFC 1, pays hôte 1.");
        B("Deux clubs au plus par pays, sauf vainqueurs continentaux.");
        koDraw(true, false);
    } else if (C.format == FMT_CUP) {
        bool pens = C.penaltiesOnly();
        bool cdfNoET = C.kind == 2 && club && !opts.cdfET;
        H("Format");
        B("Coupe à élimination directe en match unique.");
        if (C.kind == 11) B("Clubs de Ligue 1, Ligue 2 et Ligue 3 : entrée échelonnée (L3 au 1er tour, L2 au 2e tour, L1 en 16es).");
        if (comp == cdfNational) B("Phase nationale : qualifiés des ligues et Ligue 2 au 7e tour, Ligue 1 en 32es (9e tour). Le tenant du titre hors Ligue 1 entre aussi en 32es.");
        if (C.name.find("tours régionaux") != std::string::npos) B(C.tag >= 0 && C.tag < 13 ? cdfEntryText(C.tag) : std::string("Deux qualifiés par territoire pour le 7e tour."));
        if (C.kind == 4) B("Réservée aux équipes de Régional 1, 2 et 3 : une seule équipe par club (l'équipe fanion si elle joue en Régional, sinon sa réserve la mieux classée).");
        if (C.kind == 5) B("Une seule équipe par club : une réserve ne joue que si aucune équipe supérieure du club n'est engagée.");
        if (C.kind == 15) B("Coupe Gambardella (U19) : phase nationale à partir du 1er tour fédéral avec l'entrée des clubs du National U19 ; finale au Stade de France.");
        if (C.kind == 16) B("Coupe Gambardella (U19) : tours régionaux organisés par la ligue ; les qualifiés rejoignent la phase nationale.");
        if (C.kind == 6) B("Trophée des Champions : match unique entre le champion et le vainqueur de la coupe, dans un grand stade différent chaque saison.");
        if (C.noReserves) B("Les équipes réserves ne participent pas.");
        H("Tirage au sort");
        if (C.kind == 11) B("Tirage des tours préliminaires et des 16es ; le tirage des 8es fixe le tableau jusqu'à la finale.");
        if (C.homeRule == 1) B("Le premier club tiré reçoit ; si deux divisions (ou plus) séparent les clubs, le club de la plus petite division reçoit.");
        else B("Le premier club tiré reçoit, sans protection du club de division inférieure.");
        if (C.regionalDraw) B("Tirage régionalisé (clubs géographiquement proches) dans les premiers tours.");
        koDraw(!(pens || cdfNoET), true);
        if (cdfNoET) B("Option de la carrière : prolongation supprimée avant la finale.");
        H("Finale");
        auto cv = cupVenue.find(comp);
        if (cv != cupVenue.end() && !cv->second.empty()) B("Sur terrain neutre, désigné en début de saison : " + cv->second + ".");
        else if (C.neutralFinal) B("Sur terrain neutre (Stade de France pour les coupes nationales).");
        else B("Chez le club tiré en premier.");
        if (club && comp == cdfNational) { H("Europe"); B(opts.euroFormat ? "Le vainqueur est qualifié pour la phase de ligue de la Ligue Europa." : "Le vainqueur est qualifié pour la Coupe UEFA (le finaliste si le vainqueur joue déjà la Ligue des champions)."); }
        if (club && C.kind == 11 && !opts.euroFormat) { H("Europe"); B("Le vainqueur prend une des places françaises en Coupe UEFA."); }
    } else if (C.format == FMT_UCL2000) {
        H("Format (2003-04)");
        B("Trois tours de qualification aller-retour, puis phase de groupes : 8 groupes de 4 (victoire 3 points, nul 1).");
        B("Les deux premiers en 8es de finale ; le 3e est reversé au 3e tour de la Coupe UEFA.");
        B("8es, quarts et demi-finales aller-retour, finale sur terrain neutre.");
        H("Tirage au sort");
        B("Chapeaux selon le coefficient des clubs, tenant du titre tête de série.");
        B("Pas deux clubs d'un même pays dans un groupe ; deux clubs d'un même pays sont répartis entre les groupes A-D et E-H.");
        B("8es : un premier contre un deuxième d'un autre groupe et d'un autre pays ; le premier reçoit au retour.");
        H("Départage en groupe"); tbLines(C.tb, s);
        twoLegs(C.awayGoals);
    } else if (C.format == FMT_UEFA2000) {
        H("Format (2003-04)");
        B("Tour de qualification, 1er et 2e tours aller-retour, puis 16es (avec les 3es de la Ligue des champions), 8es, quarts, demies ; finale sur un match.");
        B("Les éliminés du 3e tour de qualification de la Ligue des champions entrent au 1er tour.");
        H("Places");
        B("Vainqueurs des coupes nationales, clubs classés selon le coefficient du pays ; en France, le vainqueur de la Coupe de la Ligue aussi.");
        B("Trois vainqueurs de la Coupe Intertoto entrent au 1er tour.");
        twoLegs(C.awayGoals);
    } else if (C.format == FMT_NEWEURO) {
        bool c1 = C.kind == 3, c4 = C.kind == 9;
        H("Format (depuis 2024-25)");
        B(c1 ? "Tours de qualification (voie des champions et voie de la ligue), puis phase de ligue à 36 clubs." :
                c4 ? "Quatre tours de qualification, puis phase de ligue à 36 clubs." : "3e tour de qualification et tour de barrage, puis phase de ligue à 36 clubs.");
        B(c4 ? "Phase de ligue : 6 chapeaux de 6, chaque club joue 6 matchs (un adversaire par chapeau, 3 à domicile et 3 à l'extérieur)."
             : "Phase de ligue : 4 chapeaux de 9, chaque club joue 8 matchs contre 8 adversaires différents (2 par chapeau, un à domicile et un à l'extérieur).");
        B("Pas d'adversaire du même pays, au plus deux adversaires d'un même pays.");
        B("Classement unique : les 8 premiers sont qualifiés pour les 8es de finale, du 9e au 24e : barrages aller-retour, du 25e au 36e : éliminés.");
        B("Barrages : 9e/10e contre 23e/24e, ..., 15e/16e contre 17e/18e (le mieux classé reçoit au retour).");
        B("8es : tableau fixé par le classement (1er et 2e contre les qualifiés des barrages 15e à 18e, ..., 7e et 8e contre ceux des barrages 9e/10e-23e/24e).");
        B("Quarts et demi-finales aller-retour, finale sur terrain neutre.");
        H("Points et départage de la phase de ligue");
        B("Victoire 3 points, nul 1.");
        tbLines(C.tb, s);
        H("Accès");
        if (c1) { B("Phase de ligue directe : tenants de la C1 et de la Ligue Europa, champions des associations 1 à 10, 2es des 1 à 6, 3es des 1 à 5, 4es des 1 à 4, et deux places « performance européenne »."); B("Voie des champions : champions des associations 11 et suivantes (barrage, 3e, 2e ou 1er tour selon le rang)."); B("Voie de la ligue : 2es des associations 7 à 15, 3e de la 6e, 4e de la 5e."); B("Les éliminés sont reversés en Ligue Europa (ou en Ligue Conférence après le 1er tour)."); }
        else if (!c4) { B("Phase de ligue directe : tenant de la Ligue Conférence, vainqueurs de coupe des associations 1 à 12, 5es des associations 1 à 5."); B("Qualifications : vainqueurs de coupe des associations 13 à 16 et éliminés des 2e et 3e tours de la Ligue des champions ; les éliminés des barrages de la C1 entrent en phase de ligue."); B("Les éliminés sont reversés en Ligue Conférence."); }
        else { B("Phase de ligue directe : 6es des associations 1 à 5, 4e de la 6e, et les éliminés des barrages de la Ligue Europa."); B("Qualifications : vainqueurs de coupe et clubs classés des autres associations, éliminés du 1er tour de la C1 et du 3e tour de la Ligue Europa."); }
        H("Coefficient UEFA");
        B("Victoire 2 points, nul 1 (tours de qualification : moitié, pour le pays uniquement).");
        B(fmt("Bonus : participation à la phase de ligue (%s), 8 premiers (%s), puis +1 par tour atteint à partir des 8es.", c1 ? "6" : c4 ? "2,5" : "4", c1 ? "4" : c4 ? "1" : "2"));
        twoLegs(C.awayGoals);
    } else if (C.format == FMT_SINGLE) {
        H("Format"); B("Match unique sur terrain neutre entre le champion et le vainqueur de la coupe (ou le 2e du championnat).");
        koDraw(C.kind == 7, false);
    } else if (C.kind == 21 || C.kind == 20) {
        H("Format");
        B("Réservée aux champions nationaux U19. Les pays les mieux classés au coefficient UEFA entrent en phase de groupes, les autres jouent un tour préliminaire (match sec).");
        B("Phase de groupes : 8 groupes de 4, aller-retour (victoire 3 points, nul 1), les deux premiers qualifiés.");
        B("Phase finale en matchs secs, finale sur terrain neutre.");
        H("Départage en groupe"); tbLines(C.tb, s);
        koDraw(true, false);
        H("Coefficient"); B("Les matchs des jeunes ne rapportent aucun point au coefficient UEFA.");
    } else if (C.kind == 30 || C.kind == 31 || C.kind == 32 || C.kind == 33) {
        bool uefa = C.name.find("UEFA") != std::string::npos;
        H("Ligue des nations");
        if (uefa) {
            B("Édition 2026-27 : ligues A, B et C de 16 sélections (4 groupes de 4), ligue D de 6 (2 groupes de 3). À partir de 2028-29 : trois ligues de 18.");
            B("Phase de ligue en matchs aller-retour de septembre à novembre (victoire 3 points, nul 1).");
            B("Ligue A : quarts de finale aller-retour en mars (premiers contre deuxièmes d'un autre groupe, retour chez le premier), puis Final Four en juin chez l'un des qualifiés (demies et finale en matchs secs).");
            H("Promotions et relégations");
            B("Ligue A : les deux plus mauvais derniers descendent ; les deux meilleurs derniers et les deux plus mauvais troisièmes affrontent les deuxièmes de la ligue B (barrages aller-retour en mars).");
            B("Ligue B : les premiers montent ; les derniers affrontent les deuxièmes de la ligue C en barrages.");
            B("Ligue C : les premiers montent ; ligue D : les premiers montent (la ligue D disparaît en 2028-29).");
            H("Lien avec l'Euro et la Coupe du monde");
            B("Euro 2028 : les meilleurs vainqueurs de groupe de la Ligue des nations non qualifiés disputent les barrages de mars 2028 avec les deuxièmes de groupe.");
            B("Coupe du monde : les 4 meilleurs vainqueurs de groupe de la Ligue des nations non classés dans les deux premiers de leur groupe de qualification complètent les 16 barragistes.");
        } else {
            B("Ligues A et B, groupes aller-retour ; Final Four des meilleurs de la ligue A (demies et finale en matchs secs).");
        }
        H("Départage en groupe"); tbLines(C.tb, s);
        if (C.kind != 30) koDraw(true, false);
    } else if (C.format == FMT_QUAL_GROUPS || C.kind > 100) {
        H("Éliminatoires");
        B("Groupes en matchs aller-retour (sauf mini-tournois des jeunes en match unique), victoire 3 points, nul 1.");
        H("Départage en groupe"); tbLines(C.tb, s);
        H("Qualification");
        switch (C.kind) {
        case 101:
            if (intlFormat == 1) { B("Coupe du monde, zone Europe : 12 groupes ; les premiers sont qualifiés."); B("Barrages (mars) : les 12 deuxièmes et les 4 meilleurs vainqueurs de groupe de la Ligue des nations non qualifiés ; 4 voies de 4 (demi-finale et finale en matchs secs)."); }
            else B("Coupe du monde, zone Europe : les premiers de groupe sont qualifiés ; les meilleurs deuxièmes disputent des barrages aller-retour.");
            break;
        case 102: B("Barrages UEFA : élimination directe pour les dernières places européennes."); break;
        case 103: B("Amérique du Sud : poule unique de toutes les sélections en aller-retour ; les premiers sont qualifiés, le suivant dispute le barrage intercontinental."); break;
        case 104: B("CONCACAF, 1er tour : les premiers de groupe accèdent au tour final."); break;
        case 105: B("CONCACAF, tour final : les premiers sont qualifiés, le suivant dispute le barrage intercontinental."); break;
        case 106: B("Afrique, 1er tour : les deux premiers de chaque groupe accèdent au tour final."); break;
        case 107: B("Afrique, tour final : les premiers de groupe sont qualifiés."); break;
        case 108: B("Asie, 2e tour : les premiers et les meilleurs deuxièmes accèdent au 3e tour."); break;
        case 109: B("Asie, 3e tour : les meilleurs sont qualifiés ; les troisièmes disputent un barrage."); break;
        case 110: B("Barrage Asie entre les troisièmes : le vainqueur dispute le barrage intercontinental."); break;
        case 111: B("Océanie : groupes puis phase finale ; le vainqueur est qualifié (ou barragiste)."); break;
        case 112: B("Océanie, phase finale : demi-finales et finale."); break;
        case 113: B("Barrages intercontinentaux : matchs aller-retour entre zones."); break;
        case 114:
            if (intlFormat == 1) { B("Euro à 24 : 12 groupes ; les 12 premiers et les 8 meilleurs deuxièmes sont qualifiés."); B("Pays organisateurs : ils disputent les éliminatoires, deux places leur sont garanties."); B("Barrages en mars : les autres deuxièmes et les vainqueurs de groupe de la Ligue des nations non qualifiés (voies de 4, matchs secs)."); }
            else B("Qualifications de l'Euro : les premiers sont qualifiés, barrages entre les meilleurs deuxièmes ; l'organisateur est qualifié d'office.");
            break;
        case 115: B("Barrages de l'Euro : demi-finales et finales en matchs secs, un qualifié par voie."); break;
        case 116: B("Coupe d'Afrique : les meilleurs de chaque groupe rejoignent l'organisateur."); break;
        case 117: B("Coupe d'Asie : les meilleurs de chaque groupe rejoignent l'organisateur."); break;
        case 118: B("Gold Cup : les meilleurs de chaque groupe rejoignent l'organisateur."); break;
        case 119: B("Océanie : les meilleurs rejoignent la phase finale."); break;
        case 120: B("Euro Espoirs : 9 groupes ; les premiers et le meilleur deuxième sont qualifiés, les 8 autres deuxièmes jouent des barrages aller-retour ; les organisateurs sont qualifiés d'office."); break;
        case 121: B("Barrages de l'Euro Espoirs : matchs aller-retour."); break;
        case 122: B("Tour de qualification : mini-tournois de 4 en match unique ; les deux premiers et le meilleur troisième accèdent au tour Élite (les meilleures nations en sont exemptées)."); break;
        case 123: B("Tour Élite : mini-tournois de 4 ; les premiers rejoignent l'organisateur en phase finale (8 équipes)."); break;
        case 127: B("Tournoi préolympique sud-américain : les deux premiers sont qualifiés pour les Jeux."); break;
        case 129: B("Océanie : le premier est qualifié pour les Jeux."); break;
        default: break;
        }
    } else if (C.format == FMT_TOURNAMENT) {
        bool oly = C.kind == 124 || C.kind == 125 || C.kind == 126 || C.kind == 128;
        H("Format");
        B("Phase de groupes (victoire 3 points, nul 1), puis élimination directe en matchs secs.");
        if (C.bestThirds) B(fmt("Les deux premiers de chaque groupe et les %d meilleurs troisièmes sont qualifiés.", C.bestThirds));
        else B("Les deux premiers de chaque groupe sont qualifiés.");
        if (C.thirdPlace) B("Match pour la 3e place.");
        if (C.kind == 124) B("Qualificatif olympique : les trois premiers (finalistes et vainqueur du match pour la 3e place) vont aux Jeux.");
        if (C.kind == 125 || C.kind == 126) B("Qualificatif olympique : les deux finalistes vont aux Jeux.");
        if (C.kind == 128) B("Qualificatif olympique : le vainqueur va aux Jeux.");
        if (!oly && kind == CK_INTL && intlType == IT_OLYMPICS) { B("Tournoi olympique : sélections de moins de 23 ans, trois joueurs plus âgés autorisés."); B("12 équipes : pays hôte, 3 européennes, 2 africaines, 2 asiatiques, 2 sud-américaines, 1 CONCACAF, 1 océanienne."); }
        H("Départage en groupe"); tbLines(C.tb, s);
        koDraw(true, false);
    } else if (C.format == FMT_KO_ONLY) {
        H("Format"); B("Matchs à élimination directe.");
        B("Aller-retour : l'équipe qui marque le plus sur les deux matchs est qualifiée, sinon prolongation puis tirs au but.");
        B("Match unique : prolongation puis tirs au but.");
    }
    {
        int sh, sb; bool rl; sheetRules(comp, sh, sb, rl);
        H("Feuille de match");
        B(fmt("%d joueurs : 11 titulaires et %d remplaçants.", sh, sh - 11));
        B(rl ? std::string("Remplacements illimités : un joueur remplacé peut revenir en jeu.") : fmt("%d remplacements autorisés.", sb));
    }
    if (C.kind != 12) {
        H("Discipline");
        B(fmt("%d avertissements : 1 match de suspension.", C.yellowLimit));
        B("Carton rouge direct : 1 à 3 matchs ; deux avertissements dans un match : 1 match.");
    }
    return s;
}

// ------------------------------------------------------------------ International
enum {
    QK_FINAL = 100, QK_UEFA_G, QK_UEFA_PO, QK_CSA, QK_CCF_1, QK_CCF_2, QK_CAF_G, QK_CAF_2, QK_AFC_1, QK_AFC_2, QK_AFC_PO,
    QK_OFC_G, QK_OFC_KO, QK_ICPO, QK_EURO_G, QK_EURO_PO, QK_CAN_G, QK_ASIA_G, QK_GOLD_G, QK_OFCN_G
};

struct IntlState {
    std::vector<int> qualified;
    std::vector<int> icpoCSA, icpoAFC, icpoCCF, icpoOFC;
    std::vector<int> hosts;
    int finalTeams = 32;
};
static IntlState g_intl;
std::vector<int>& intlQualifiedRef() { return g_intl.qualified; }
std::vector<int>& intlHostsRef() { return g_intl.hosts; }
int& intlFinalTeamsRef() { return g_intl.finalTeams; }
void youthNewIntl(Career& K, int type, bool withQual, const std::vector<int>& ctrl, int yr, const std::vector<int>& hosts);   // youthintl.cpp
void youthQualify(Career& K, int comp);

int intlYear(int type) {
    switch (type) { case IT_WORLDCUP: return 2030; case IT_EURO: return 2028; case IT_CAN: return 2027; case IT_COPA: return 2028;
                    case IT_ASIA: return 2027; case IT_GOLD: return 2027; case IT_OLYMPICS: return 2028;
                    case IT_EURO21: case IT_EURO19: case IT_EURO17: return 2027;
                    case IT_OLY_W: return 2028; case IT_EURO_W: return 2029; case IT_WC_W: return 2027; default: return 2028; }
}

std::vector<int> defaultHosts(int type) {
    auto N = [&](const char* c) { return g_world.nationIndex(c); };
    switch (type) {
    case IT_WORLDCUP: return { N("MAR"), N("ESP"), N("POR") };
    case IT_EURO: return { N("ENG") };
    case IT_CAN: return { N("KEN") };
    case IT_COPA: return { N("ARG") };
    case IT_ASIA: return { N("KSA") };
    case IT_GOLD: return { N("USA") };
    case IT_OLYMPICS: return { N("USA") };                    // Los Angeles 2028
    case IT_EURO21: return { N("ALB"), N("SRB") };            // Euro Espoirs 2027 : Albanie et Serbie
    case IT_EURO19: return { N("SVK") };
    case IT_EURO17: return { N("CRO") };
    case IT_OLY_W: return { N("USA") };                       // Los Angeles 2028
    case IT_EURO_W: return { N("GER") };                      // Euro féminin 2029
    case IT_WC_W: return { N("BRA") };                        // Coupe du monde féminine 2027
    default: return { N("NZL") };
    }
}

static int makeGroupComp(Season& S, const std::string& name, const std::vector<std::vector<int>>& groups, int legs, double t0, double t1, int kind) {
    Competition c;
    c.format = FMT_QUAL_GROUPS; c.name = name; c.shortName = name; c.kind = kind; c.legs = legs; c.tb = TB_H2H;
    int maxR = 0;
    for (auto& g : groups) maxR = std::max(maxR, (int)g.size() - (g.size() % 2 == 0 ? 1 : 0));
    maxR *= legs;
    std::vector<double> times;
    for (int r = 0; r < std::max(1, maxR); r++) times.push_back(maxR > 1 ? t0 + (t1 - t0) * r / (maxR - 1) : t0);
    c.addGroupStage(groups, legs, times, "Groupes");
    return addComp(S, std::move(c));
}

static int makeGroupCompT(Season& S, const std::string& name, const std::vector<std::vector<int>>& groups, const std::vector<double>& windows, int kind) {
    Competition c;
    c.format = FMT_QUAL_GROUPS; c.name = name; c.shortName = name; c.kind = kind; c.legs = 2; c.tb = TB_H2H;
    int maxR = 0;
    for (auto& g : groups) maxR = std::max(maxR, (int)g.size() - (g.size() % 2 == 0 ? 1 : 0));
    maxR *= 2;
    std::vector<double> times;
    for (int r = 0; r < maxR; r++) times.push_back(r < (int)windows.size() ? windows[r] : windows.back() + 0.3 * (r - (int)windows.size() + 1));
    c.addGroupStage(groups, 2, times, "Groupes");
    return addComp(S, std::move(c));
}

static int makeKO(Season& S, const std::string& name, const std::vector<std::pair<int, int>>& pairs, std::vector<int> legs,
                  std::vector<double> times, int spots, int kind) {
    Competition c;
    c.format = FMT_KO_ONLY; c.name = name; c.shortName = name; c.kind = kind; c.qualSpots = spots;
    c.koLegs = legs; c.koTimes = times; c.neutralFinal = false;
    c.addKOStage(pairs, legs[0], times[0], "Barrages");
    return addComp(S, std::move(c));
}

static std::vector<std::vector<Standing>> byPosition(const Competition& C) {
    std::vector<std::vector<Standing>> pos;
    for (int g = 0; g < (int)C.stages[0].groups.size(); g++) {
        auto tb = C.table(0, g);
        for (int i = 0; i < (int)tb.size(); i++) { if ((int)pos.size() <= i) pos.resize(i + 1); pos[i].push_back(tb[i]); }
    }
    for (auto& p : pos) std::stable_sort(p.begin(), p.end(), [](const Standing& a, const Standing& b) {
        float pa = a.p ? (float)a.pts / a.p : 0, pb = b.p ? (float)b.pts / b.p : 0;
        if (pa != pb) return pa > pb;
        if (a.gd() != b.gd()) return a.gd() > b.gd();
        return a.gf > b.gf;
    });
    return pos;
}

static std::vector<int> flatten(const std::vector<std::vector<Standing>>& pos) {
    std::vector<int> v;
    for (auto& p : pos) for (auto& s : p) v.push_back(s.team);
    return v;
}

static std::vector<std::pair<int, int>> seededPairs(std::vector<int> t) {
    std::vector<std::pair<int, int>> p;
    int n = (int)t.size();
    for (int i = 0; i < n / 2; i++) p.push_back({ t[n - 1 - i], t[i] });
    return p;
}

static void createFinals(Career& K);

static void checkAllQualDone(Career& K) {
    for (auto& c : K.season.comps) if (c.kind > QK_FINAL && !c.done) return;
    if (K.finalComp < 0) createFinals(K);
}

static int hostsIn(int conf, bool skipFirst) {
    int n = 0;
    for (size_t i = 0; i < g_intl.hosts.size(); i++) { if (skipFirst && i == 0) continue; if (NATIONS[g_intl.hosts[i]].conf == conf) n++; }
    return n;
}

// Euro co-organisé (2028, 2032) : les pays hôtes disputent les éliminatoires, deux places leur sont garanties
static bool hostsPlay(const Career& K) { return K.intlType == IT_EURO && g_intl.finalTeams == 24 && g_intl.hosts.size() >= 2 && K.intlWithQual; }
static bool isHost(int t) { return std::find(g_intl.hosts.begin(), g_intl.hosts.end(), t) != g_intl.hosts.end(); }
// semaine de la phase finale (juin de l'année de la compétition)
static double finalsWeek(const Career& K) { return (K.year - K.season.year - 1) * 52.0 + 45.0; }

// places directes en Coupe du monde à 32 : UEFA 13, CAF 5, AFC 4 (+1 barrage), CONMEBOL 4 (+1), CONCACAF 3 (+1), OFC 0 (+1), 1 pays hôte
// Coupe du monde à 48 (format 2026) : UEFA 16, CAF 9, AFC 8, CONMEBOL 6, CONCACAF 6, OFC 1 (+2 barragistes)
static int wcSlots(int conf) {
    int base = conf == UEFA ? 13 : conf == CAF ? 5 : conf == AFC ? 4 : conf == CONMEBOL ? 4 : conf == CONCACAF ? 3 : 0;
    if (g_intl.finalTeams == 48) base = conf == UEFA ? 16 : conf == CAF ? 9 : conf == AFC ? 8 : conf == CONMEBOL ? 6 : conf == CONCACAF ? 6 : conf == OFC ? 1 : 0;
    return std::max(0, base - hostsIn(conf, true));
}

static void qualifyIntl(Career& K, int comp) {
    Season& S = K.season;
    Competition& C = S.comps[comp];
    IntlState& I = g_intl;
    double tn = S.now + 1.0;
    auto Q = [&](int t) { if (t >= 0 && std::find(I.qualified.begin(), I.qualified.end(), t) == I.qualified.end()) I.qualified.push_back(t); };
    switch (C.kind) {
    case QK_FINAL: {
        K.history.push_back(fmt("%d ", K.year) + C.name + " : " + (C.winner >= 0 ? g_world.teams[C.winner].name : "?"));
        return;
    }
    case QK_UEFA_G: {
        auto pos = byPosition(C);
        if (I.finalTeams == 48 && nlActive(K)) {
            // format 2026 : premiers qualifiés ; barrages à 16 (deuxièmes + meilleurs vainqueurs de groupe de la Ligue des nations)
            for (auto& s : pos[0]) Q(s.team);
            int spots = wcSlots(UEFA) - (int)pos[0].size();
            if (spots > 0 && pos.size() > 1) {
                std::set<int> top2; for (auto& s : pos[0]) top2.insert(s.team); for (auto& s : pos[1]) top2.insert(s.team);
                std::vector<int> ru; for (auto& s : pos[1]) if ((int)ru.size() < 3 * spots) ru.push_back(s.team);
                std::vector<int> pool = ru;
                auto add = [&](int t) { if ((int)pool.size() < 4 * spots && !top2.count(t) && std::find(pool.begin(), pool.end(), t) == pool.end() && std::find(I.qualified.begin(), I.qualified.end(), t) == I.qualified.end() && !isHost(t) && NATIONS[t].conf == UEFA) pool.push_back(t); };
                for (int t : nlGroupWinners(K)) add(t);
                for (int t : nlRanking(K)) add(t);
                for (int t : flatten(pos)) add(t);
                spots = (int)pool.size() / 4;
                if (spots > 0) {
                    std::vector<int> p4(pool.begin() + 3 * spots, pool.begin() + 4 * spots), p3(pool.begin() + 2 * spots, pool.begin() + 3 * spots);
                    g_rng.shuffle(p4); g_rng.shuffle(p3);
                    std::vector<std::pair<int, int>> pairs;
                    for (int k = 0; k < spots; k++) { pairs.push_back({ pool[k], p4[k] }); pairs.push_back({ pool[spots + k], p3[k] }); }
                    double tb = std::max(S.now + 1, finalsWeek(K) - 13);
                    { int ci = makeKO(S, "Barrages UEFA (voies A à D)", pairs, { 1, 1 }, { tb, tb + 0.5 }, spots, QK_UEFA_PO); S.comps[ci].stages[0].name = "Demi-finales des barrages"; S.comps[ci].koNames = { "Demi-finales des barrages", "Finales des barrages" }; }
                    S.news.push_back("Coupe du monde : les barrages européens réunissent les deuxièmes de groupe et les meilleurs vainqueurs de groupe de la Ligue des nations.");
                    return;
                }
            }
            break;
        }
        for (auto& s : pos[0]) Q(s.team);
        // 8 meilleurs deuxièmes : barrages aller-retour
        std::vector<int> po;
        for (auto& s : pos[1]) if ((int)po.size() < 8) po.push_back(s.team);
        int need = wcSlots(UEFA) - (int)pos[0].size();
        if (need > 0 && po.size() >= 2) { po.resize(std::min((int)po.size(), need * 2)); makeKO(S, "Barrages UEFA", seededPairs(po), { 2 }, { tn }, need, QK_UEFA_PO); return; }
        break;
    }
    case QK_UEFA_PO: for (int t : C.result) Q(t); break;
    case QK_CSA: {
        auto tb = C.table(0, 0);
        int direct = wcSlots(CONMEBOL);
        for (int i = 0; i < (int)tb.size(); i++) { if (i < direct) Q(tb[i].team); else if (i == direct) I.icpoCSA.push_back(tb[i].team); }
        break;
    }
    case QK_CCF_1: {
        auto pos = byPosition(C);
        std::vector<int> hex;
        for (auto& s : pos[0]) hex.push_back(s.team);
        makeGroupComp(S, "Qualifications CONCACAF - hexagonal", { hex }, 2, tn, tn + 12, QK_CCF_2);
        return;
    }
    case QK_CCF_2: {
        auto tb = C.table(0, 0);
        int direct = wcSlots(CONCACAF);
        for (int i = 0; i < (int)tb.size(); i++) { if (i < direct) Q(tb[i].team); else if (i == direct) I.icpoCCF.push_back(tb[i].team); }
        break;
    }
    case QK_CAF_G: {
        auto pos = byPosition(C);
        std::vector<int> adv;
        for (auto& s : pos[0]) adv.push_back(s.team);
        for (auto& s : pos[1]) adv.push_back(s.team);
        int ng = std::max(1, wcSlots(CAF));
        makeGroupComp(S, "Qualifications CAF - tour final", potGroups(adv, ng), 2, tn, tn + 10, QK_CAF_2);
        return;
    }
    case QK_CAF_2: {
        auto pos = byPosition(C);
        int direct = wcSlots(CAF);
        for (int i = 0; i < (int)pos[0].size() && i < direct; i++) Q(pos[0][i].team);
        break;
    }
    case QK_AFC_1: {
        auto pos = byPosition(C);
        std::vector<int> adv;
        for (auto& s : pos[0]) adv.push_back(s.team);
        for (int i = 0; i < 4 && i < (int)pos[1].size(); i++) adv.push_back(pos[1][i].team);
        makeGroupComp(S, "Qualifications AFC - 3e tour", potGroups(adv, 2), 2, tn, tn + 12, QK_AFC_2);
        return;
    }
    case QK_AFC_2: {
        auto pos = byPosition(C);
        int direct = wcSlots(AFC);
        std::vector<int> all;
        for (auto& s : pos[0]) all.push_back(s.team);
        for (auto& s : pos[1]) all.push_back(s.team);
        if (I.finalTeams == 48) for (size_t k = 2; k < pos.size(); k++) for (auto& s : pos[k]) all.push_back(s.team);
        for (int i = 0; i < (int)all.size() && i < direct; i++) Q(all[i]);
        if (I.finalTeams == 48) break;
        if (pos.size() > 2 && pos[2].size() >= 2) { makeKO(S, "Barrage AFC (3es)", { { pos[2][1].team, pos[2][0].team } }, { 2 }, { tn }, 1, QK_AFC_PO); return; }
        break;
    }
    case QK_AFC_PO: for (int t : C.result) I.icpoAFC.push_back(t); break;
    case QK_OFC_G: {
        auto pos = byPosition(C);
        std::vector<int> sf = { pos[0][0].team, pos[1][1].team, pos[0][1].team, pos[1][0].team };
        makeKO(S, "Qualifications OFC - phase finale", { { sf[0], sf[1] }, { sf[2], sf[3] } }, { 1, 1 }, { tn, tn + 1 }, 1, QK_OFC_KO);
        return;
    }
    case QK_OFC_KO: if (I.finalTeams == 48) Q(C.winner); else I.icpoOFC.push_back(C.winner); break;
    case QK_ICPO: for (int t : C.result) Q(t); break;
    case QK_EURO_G: {
        auto pos = byPosition(C);
        if (I.finalTeams == 24 && nlActive(K)) {
            // Euro 2028 : 12 premiers + 8 meilleurs deuxièmes ; places garanties aux pays hôtes ; barrages avec la Ligue des nations
            for (auto& s : pos[0]) Q(s.team);
            for (int i = 0; i < 8 && pos.size() > 1 && i < (int)pos[1].size(); i++) Q(pos[1][i].team);
            if (hostsPlay(K)) {
                int g = std::min<int>(2, (int)I.hosts.size()), n = 0;
                for (int h : I.hosts) if (std::find(I.qualified.begin(), I.qualified.end(), h) != I.qualified.end()) n++;
                for (int t : flatten(pos)) if (n < g && isHost(t) && std::find(I.qualified.begin(), I.qualified.end(), t) == I.qualified.end()) {
                    Q(t); n++; S.news.push_back(g_world.teams[t].name + " utilise sa place garantie de pays organisateur.");
                }
            }
            int P = I.finalTeams - (hostsPlay(K) ? 0 : (int)I.hosts.size()) - (int)I.qualified.size();
            if (P > 0) {
                std::vector<int> pool;
                auto add = [&](int t) { if ((int)pool.size() < 4 * P && std::find(pool.begin(), pool.end(), t) == pool.end() && std::find(I.qualified.begin(), I.qualified.end(), t) == I.qualified.end() && (hostsPlay(K) || !isHost(t)) && NATIONS[t].conf == UEFA) pool.push_back(t); };
                if (pos.size() > 1) for (auto& s : pos[1]) add(s.team);
                for (int t : nlGroupWinners(K)) add(t);
                for (int t : nlRanking(K)) add(t);
                for (int t : flatten(pos)) add(t);
                auto rk = nlRanking(K);
                auto rkOf = [&](int t) { auto it = std::find(rk.begin(), rk.end(), t); return (int)(it - rk.begin()); };
                std::stable_sort(pool.begin(), pool.end(), [&](int a, int b) { return rkOf(a) < rkOf(b); });
                P = (int)pool.size() / 4;
                if (P > 0) {
                    std::vector<std::pair<int, int>> pairs;
                    for (int k = 0; k < P; k++) { pairs.push_back({ pool[k], pool[3 * P + k] }); pairs.push_back({ pool[P + k], pool[2 * P + k] }); }
                    double tb = std::max(S.now + 1, finalsWeek(K) - 13);
                    { int ci = makeKO(S, "Barrages de l'Euro", pairs, { 1, 1 }, { tb, tb + 0.5 }, P, QK_EURO_PO); S.comps[ci].stages[0].name = "Demi-finales des barrages"; S.comps[ci].koNames = { "Demi-finales des barrages", "Finales des barrages" }; }
                    S.news.push_back(fmt("Euro : %d places en jeu lors des barrages (deuxièmes de groupe et vainqueurs de groupe de la Ligue des nations).", P));
                    return;
                }
            }
            break;
        }
        for (auto& s : pos[0]) Q(s.team);
        if (I.finalTeams == 24) {
            // Euro à 24 : les 2 premiers de chaque groupe, puis barrages entre les meilleurs 3es
            for (auto& s : pos[1]) Q(s.team);
            int need = I.finalTeams - (int)I.hosts.size() - (int)I.qualified.size();
            std::vector<int> th; if (pos.size() > 2) for (auto& s : pos[2]) th.push_back(s.team);
            if (need > 0 && th.size() >= 2) { th.resize(std::min((int)th.size(), need * 2)); makeKO(S, "Barrages de l'Euro", seededPairs(th), { 1, 1 }, { tn, tn + 0.5 }, need, QK_EURO_PO); return; }
            break;
        }
        std::vector<int> sec;
        for (auto& s : pos[1]) sec.push_back(s.team);
        int need = I.finalTeams - (int)I.hosts.size() - (int)pos[0].size();
        if (need > 0 && sec.size() >= 2) { sec.resize(std::min((int)sec.size(), need * 2)); makeKO(S, "Barrages de l'Euro", seededPairs(sec), { 2 }, { tn }, need, QK_EURO_PO); return; }
        break;
    }
    case QK_EURO_PO: for (int t : C.result) Q(t); break;
    case QK_CAN_G: case QK_ASIA_G: case QK_GOLD_G: case QK_OFCN_G: {
        auto all = flatten(byPosition(C));
        int need = I.finalTeams - (int)I.hosts.size();
        for (int i = 0; i < (int)all.size() && (int)I.qualified.size() < need; i++) Q(all[i]);
        break;
    }
    default: break;
    }
    // barrages intercontinentaux (AFC-CONMEBOL, CONCACAF-OFC) quand toutes les zones ont terminé
    if (K.intlType == IT_WORLDCUP) {
        bool others = false, exists = false;
        for (auto& c : S.comps) { if (c.kind > QK_FINAL && c.kind != QK_ICPO && !c.done) others = true; if (c.kind == QK_ICPO) exists = true; }
        if (!others && !exists) {
            std::vector<std::pair<int, int>> pairs;
            if (!I.icpoAFC.empty() && !I.icpoCSA.empty()) pairs.push_back({ I.icpoAFC[0], I.icpoCSA[0] });
            if (!I.icpoOFC.empty() && !I.icpoCCF.empty()) pairs.push_back({ I.icpoOFC[0], I.icpoCCF[0] });
            if (!pairs.empty()) { makeKO(S, "Barrages intercontinentaux", pairs, { 2 }, { tn + 1 }, (int)pairs.size(), QK_ICPO); return; }
        }
    }
    checkAllQualDone(K);
}

// tirage de la Coupe du monde : au plus deux nations européennes par groupe, au plus une nation des autres confédérations
static std::vector<std::vector<int>> worldCupGroups(std::vector<int> teams, int ng, const std::vector<int>& hosts) {
    std::vector<std::vector<int>> g(ng);
    for (size_t h = 0; h < hosts.size() && (int)h < ng; h++) { g[h].push_back(hosts[h]); teams.erase(std::remove(teams.begin(), teams.end(), hosts[h]), teams.end()); }
    sortByRating(teams);
    auto confOk = [&](const std::vector<int>& grp, int t) {
        int c = NATIONS[t].conf, n = 0;
        for (int x : grp) if (NATIONS[x].conf == c) n++;
        return c == UEFA ? n < 2 : n < 1;
    };
    // chapeau 1 : hôtes + meilleures nations ; puis chapeaux suivants
    size_t idx = 0;
    for (int pot = 0; idx < teams.size(); pot++) {
        std::vector<int> P;
        std::vector<int> open;
        for (int k = 0; k < ng; k++) if ((int)g[k].size() == pot) open.push_back(k);
        for (size_t k = 0; k < open.size() && idx < teams.size(); k++) P.push_back(teams[idx++]);
        std::vector<std::vector<int>> best;
        for (int attempt = 0; attempt < 3000; attempt++) {
            auto G = g;
            g_rng.shuffle(P);
            bool ok = true;
            std::vector<int> free = open;
            for (int t : P) {
                std::vector<int> el;
                for (int k : free) if (confOk(G[k], t) || attempt > 2500) el.push_back(k);
                if (el.empty()) { ok = false; break; }
                int k = el[g_rng.next() % el.size()];
                G[k].push_back(t);
                free.erase(std::find(free.begin(), free.end(), k));
            }
            if (ok) { best = G; break; }
        }
        if (best.empty()) { for (size_t k = 0; k < P.size(); k++) g[open[k]].push_back(P[k]); }
        else g = best;
    }
    return g;
}

static void createFinals(Career& K) {
    IntlState& I = g_intl;
    Season& S = K.season;
    std::vector<int> teams;
    if (!hostsPlay(K)) teams = I.hosts;
    else for (int h : I.hosts) if (std::find(I.qualified.begin(), I.qualified.end(), h) != I.qualified.end()) teams.push_back(h);
    for (int t : I.qualified) if (std::find(teams.begin(), teams.end(), t) == teams.end()) teams.push_back(t);
    std::vector<int> pool;
    int conf = -1;
    switch (K.intlType) { case IT_EURO: conf = UEFA; break; case IT_CAN: conf = CAF; break; case IT_ASIA: conf = AFC; break;
                          case IT_GOLD: conf = CONCACAF; break; case IT_OFC: conf = OFC; break; default: break; }
    for (int i = 0; i < NUM_NATIONS; i++) if (nationEligible(i) && (conf < 0 || NATIONS[i].conf == conf)) pool.push_back(i);
    sortByRating(pool);
    for (int t : pool) { if ((int)teams.size() >= I.finalTeams) break; if (std::find(teams.begin(), teams.end(), t) == teams.end()) teams.push_back(t); }
    teams.resize(I.finalTeams);
    int ng = I.finalTeams / 4;
    Competition c;
    c.format = FMT_TOURNAMENT; c.kind = QK_FINAL;
    c.name = fmt("%s %d", INTL_NAMES[K.intlType], K.year);
    c.shortName = INTL_NAMES[K.intlType];
    c.host = teams.empty() || I.hosts.empty() || !isHost(teams[0]) ? -1 : teams[0];
    c.tb = K.intlType == IT_WORLDCUP ? TB_FIFA : TB_H2H;
    auto groups = K.intlType == IT_WORLDCUP ? worldCupGroups(teams, ng, I.hosts) : potGroups(teams, ng, c.host);
    for (size_t h = 1; K.intlType != IT_WORLDCUP && h < I.hosts.size() && h < groups.size(); h++) {
        int ht = I.hosts[h];
        if (std::find(teams.begin(), teams.end(), ht) == teams.end()) continue;
        for (auto& g : groups) {
            auto it = std::find(g.begin(), g.end(), ht);
            if (it != g.end()) { std::swap(*it, groups[h][0]); break; }
        }
    }
    double t0 = std::max(S.now + 3, finalsWeek(K));
    c.bestThirds = I.finalTeams == 24 ? 4 : I.finalTeams == 48 ? 8 : 0;
    c.thirdPlace = (K.intlType == IT_WORLDCUP || K.intlType == IT_CAN || K.intlType == IT_COPA);
    c.legs = 1;                   // phase finale : matchs secs, tableau fixé à l'avance
    c.addGroupStage(groups, 1, { t0, t0 + 2, t0 + 4 }, "Phase de groupes");
    for (int i = 0; i < 6; i++) c.koTimes.push_back(t0 + 6 + 2.0 * i);
    K.finalComp = addComp(S, std::move(c));
}

void Career::newInternational(int type, bool withQual, const std::vector<int>& ctrl, int yr, const std::vector<int>& hosts, int format) {
    if (type >= IT_OLYMPICS) { youthNewIntl(*this, type, withQual, ctrl, yr, hosts); return; }
    kind = CK_INTL;
    resetV7();
    intlType = type; intlWithQual = withQual; intlFormat = format;
    season = Season();
    season.controlled = ctrl;
    season.mode = 1;
    season.comps.reserve(300);
    history.clear();
    finalComp = -1;
    IntlState& I = g_intl;
    I = IntlState();
    I.hosts = hosts.empty() ? defaultHosts(type) : hosts;
    intlHosts = I.hosts;
    switch (type) {
    case IT_WORLDCUP: I.finalTeams = format ? 48 : 32; break;
    case IT_EURO: I.finalTeams = format ? 24 : 16; break;
    case IT_CAN: case IT_ASIA: I.finalTeams = 24; break;
    case IT_COPA: case IT_GOLD: I.finalTeams = 16; break;
    default: I.finalTeams = 8; break;
    }
    year = yr > 0 ? yr : intlYear(type);
    // calendrier : les éliminatoires commencent deux ans avant la phase finale (la Ligue des nations à l'automne)
    season.year = withQual ? year - 2 : year - 1;
    auto without = [&](std::vector<int> v) {
        v.erase(std::remove_if(v.begin(), v.end(), [&](int t) { return std::find(I.hosts.begin(), I.hosts.end(), t) != I.hosts.end(); }), v.end());
        return v;
    };
    Season& S = season;
    if (type == IT_COPA) {
        auto v = nationsOfConf(CONMEBOL);
        for (int t : v) I.qualified.push_back(t);
        auto c = nationsOfConf(CONCACAF); sortByRating(c);
        for (int i = 0; i < 6; i++) I.qualified.push_back(c[i]);
        createFinals(*this);
        return;
    }
    if (!withQual) {
        std::vector<std::pair<int, int>> quota;
        switch (type) {
        case IT_WORLDCUP:
            if (format) quota = { { UEFA, 16 }, { CONMEBOL, 6 }, { CONCACAF, 6 }, { CAF, 10 }, { AFC, 9 }, { OFC, 1 } };
            else quota = { { UEFA, 13 }, { CONMEBOL, 5 }, { CONCACAF, 4 }, { CAF, 5 }, { AFC, 4 }, { OFC, 0 } };
            break;
        case IT_EURO: quota = { { UEFA, format ? 24 : 16 } }; break;
        case IT_CAN: quota = { { CAF, 24 } }; break;
        case IT_ASIA: quota = { { AFC, 24 } }; break;
        case IT_GOLD: quota = { { CONCACAF, 16 } }; break;
        case IT_OFC: quota = { { OFC, 8 } }; break;
        }
        for (auto& q : quota) {
            auto v = without(nationsOfConf(q.first));
            std::vector<std::pair<float, int>> sc;
            for (int t : v) sc.push_back({ g_world.teams[t].rating + g_rng.frange(-5, 5), t });
            std::sort(sc.rbegin(), sc.rend());
            int n = q.second - hostsIn(q.first, type == IT_WORLDCUP);
            for (int i = 0; i < n && i < (int)sc.size(); i++) I.qualified.push_back(sc[i].second);
        }
        for (int t : ctrl) if (std::find(I.qualified.begin(), I.qualified.end(), t) == I.qualified.end() &&
                                std::find(I.hosts.begin(), I.hosts.end(), t) == I.hosts.end()) {
            if (!I.qualified.empty()) I.qualified.pop_back();
            I.qualified.insert(I.qualified.begin(), t);
        }
        createFinals(*this);
        return;
    }
    double T0 = 1, T1 = 40;
    // fenêtres internationales de l'année précédant la phase finale (mars, juin, septembre, octobre, novembre)
    const std::vector<double> WIN = { 33, 34, 40, 41, 57, 58, 61, 62, 66, 67 };
    if (type == IT_WORLDCUP || type == IT_EURO) nlSetup(*this, season.year);
    switch (type) {
    case IT_WORLDCUP: {
        auto u = without(nationsOfConf(UEFA));
        if (format) makeGroupCompT(S, "Qualifications UEFA", potGroups(u, 12), WIN, QK_UEFA_G);
        else makeGroupCompT(S, "Qualifications UEFA", potGroups(u, std::max(1, wcSlots(UEFA) - 4)), WIN, QK_UEFA_G);
        makeGroupComp(S, "Qualifications CONMEBOL", { without(nationsOfConf(CONMEBOL)) }, 2, T0, T1 + 10, QK_CSA);
        makeGroupComp(S, "Qualifications CONCACAF - 1er tour", potGroups(without(nationsOfConf(CONCACAF)), 6), 2, T0, 22, QK_CCF_1);
        makeGroupComp(S, "Qualifications CAF - 1er tour", potGroups(without(nationsOfConf(CAF)), 10), 2, T0, 24, QK_CAF_G);
        makeGroupComp(S, "Qualifications AFC - 2e tour", potGroups(without(nationsOfConf(AFC)), 8), 2, T0, 22, QK_AFC_1);
        makeGroupComp(S, "Qualifications OFC", potGroups(without(nationsOfConf(OFC)), 2), 1, 30, 36, QK_OFC_G);
        break;
    }
    case IT_EURO:
        if (format) makeGroupCompT(S, "Qualifications Euro", potGroups(hostsPlay(*this) ? nationsOfConf(UEFA) : without(nationsOfConf(UEFA)), 12), WIN, QK_EURO_G);
        else makeGroupCompT(S, "Qualifications Euro", potGroups(without(nationsOfConf(UEFA)), 10), WIN, QK_EURO_G);
        break;
    case IT_CAN: makeGroupComp(S, "Qualifications CAN", potGroups(without(nationsOfConf(CAF)), 12), 2, T0, T1, QK_CAN_G); break;
    case IT_ASIA: makeGroupComp(S, "Qualifications Coupe d'Asie", potGroups(without(nationsOfConf(AFC)), 9), 2, T0, T1, QK_ASIA_G); break;
    case IT_GOLD: makeGroupComp(S, "Qualifications Gold Cup", potGroups(without(nationsOfConf(CONCACAF)), 7), 2, T0, T1, QK_GOLD_G); break;
    case IT_OFC: {
        auto a = without(nationsOfConf(OFC));
        sortByRating(a);
        std::vector<int> low(a.end() - 4, a.end());
        for (int i = 0; i < (int)a.size() - 4; i++) I.qualified.push_back(a[i]);
        makeGroupComp(S, "Tour préliminaire OFC", { low }, 1, T0, 5, QK_OFCN_G);
        break;
    }
    }
}

// ------------------------------------------------------------------ compétition personnalisée
void Career::newCustom(const CustomCompDef& def, const std::vector<int>& ctrl) {
    kind = CK_CUSTOM;
    resetV7();
    custom = def;
    season = Season();
    season.controlled = ctrl;
    season.mode = 2;
    season.year = year = 2026;
    history.clear();
    Competition c;
    c.name = def.name.empty() ? "Tournoi personnalisé" : def.name;
    c.shortName = c.name;
    std::vector<int> teams = def.teams;
    g_rng.shuffle(teams);
    if (def.format == 0) {
        c.format = FMT_LEAGUE; c.tb = TB_GD;
        c.setupLeague(teams, def.legs, 1, 40, 1);
        addComp(season, std::move(c));
    } else if (def.format == 1) {
        c.format = FMT_KO_ONLY; c.qualSpots = 1; c.neutralFinal = true;
        int n = (int)teams.size();
        for (int i = 0; i < 10; i++) { c.koLegs.push_back(def.legs); c.koTimes.push_back(2 + 5.0 * i); }
        c.koTimes[0] = 2;
        std::vector<std::pair<int, int>> pairs;
        sortByRating(teams);
        // exemptions pour atteindre une puissance de 2
        int pw = 1; while (pw < n) pw *= 2;
        int byes = pw - n;
        std::vector<int> play(teams.begin() + byes, teams.end());
        g_rng.shuffle(play);
        for (int i = 0; i < byes; i++) pairs.push_back({ teams[i], -1 });
        for (size_t i = 0; i + 1 < play.size(); i += 2) pairs.push_back({ play[i], play[i + 1] });
        g_rng.shuffle(pairs);
        c.addKOStage(pairs, def.legs, 2, koName(pw));
        addComp(season, std::move(c));
    } else {
        c.format = FMT_TOURNAMENT; c.tb = TB_H2H; c.legs = def.legs;
        int ng = std::max(1, def.groups);
        auto groups = potGroups(teams, ng);
        c.addGroupStage(groups, def.legs, { 1, 4, 7, 10, 13, 16, 19, 22, 25, 28 }, "Phase de groupes");
        // qualifiés : 2 par groupe (+ meilleurs 3es pour une puissance de 2)
        int q = ng * 2; int pw = 1; while (pw < q) pw *= 2;
        c.bestThirds = std::min(ng, pw - q);
        if (pw - q > ng) c.bestThirds = 0;
        for (int i = 0; i < 6; i++) c.koTimes.push_back(30 + 2.0 * i);
        c.host = -1;
        addComp(season, std::move(c));
    }
}

void Career::update() {}

// ------------------------------------------------------------------ sauvegarde
#include "serial.h"

static const unsigned SAVE_MAGIC = 0x46325344;
static const unsigned SAVE_VERSION = 21;

static void wStage(Writer& w, const Stage& s) {
    w.pod(s.type); w.str(s.name); w.pod(s.legs); w.vvi(s.groups); w.vpod(s.ties);
    unsigned n = (unsigned)s.rounds.size(); w.pod(n);
    for (auto& r : s.rounds) { w.pod(r.time); w.str(r.name); w.vpod(r.m); w.pod(r.done); }
    w.pod(s.finished); w.pod(s.nextR);
}
static void rStage(Reader& r, Stage& s) {
    r.pod(s.type); r.str(s.name); r.pod(s.legs); r.vvi(s.groups); r.vpod(s.ties);
    unsigned n = 0; r.pod(n); if (!r.ok || n > 1000000) { r.ok = false; return; }
    s.rounds.resize(n);
    for (auto& x : s.rounds) { r.pod(x.time); r.str(x.name); r.vpod(x.m); r.pod(x.done); }
    r.pod(s.finished); r.pod(s.nextR);
}
template <class IO, class C> static void ioCompScalars(IO& io, C& c) {
    io.pod(c.format); io.pod(c.legs); io.pod(c.tb); io.pod(c.ptsWin); io.pod(c.cur); io.pod(c.done); io.pod(c.started); io.pod(c.t0); io.pod(c.t1);
    io.pod(c.awaiting); io.pod(c.winner); io.pod(c.groupsAdvance); io.pod(c.bestThirds); io.pod(c.thirdPlace); io.pod(c.regionalDraw);
    io.pod(c.regionalRounds); io.pod(c.swissRounds); io.pod(c.qualSpots); io.pod(c.qualPlayoff); io.pod(c.kind); io.pod(c.host); io.pod(c.tag);
    io.pod(c.season); io.pod(c.noReserves); io.pod(c.neutralFinal); io.pod(c.extReadyMask); io.pod(c.awayGoals); io.pod(c.homeRule); io.pod(c.yellowLimit);
}
static void wComp(Writer& w, const Competition& c) {
    w.str(c.name); w.str(c.shortName); ioCompScalars(w, c); w.vpod(c.matches);
    unsigned n = (unsigned)c.stages.size(); w.pod(n); for (auto& s : c.stages) wStage(w, s);
    w.vpod(c.koTimes); w.vstr(c.koNames); w.vpod(c.koLegs); w.vvi(c.entrants); w.vpod(c.koTargets); w.vpod(c.result);
    w.vpod(c.extra); w.vpod(c.extra2); w.vpod(c.carry); w.vpod(c.events);
}
static void rComp(Reader& r, Competition& c) {
    r.str(c.name); r.str(c.shortName); ioCompScalars(r, c); r.vpod(c.matches);
    unsigned n = 0; r.pod(n); if (!r.ok || n > 100000) { r.ok = false; return; }
    c.stages.resize(n); for (auto& s : c.stages) rStage(r, s);
    r.vpod(c.koTimes); r.vstr(c.koNames); r.vpod(c.koLegs); r.vvi(c.entrants); r.vpod(c.koTargets); r.vpod(c.result);
    r.vpod(c.extra); r.vpod(c.extra2); r.vpod(c.carry); r.vpod(c.events);
}
template <class IO> static void ioKit(IO& io, Kit& k) { io.pod(k.shirt); io.pod(k.shirt2); io.pod(k.shorts); io.pod(k.socks); io.pod(k.pattern); }

bool Career::save(const char* path) const {
    FILE* f = fopen(path, "wb");
    if (!f) return false;
    static char wbuf[1 << 20]; setvbuf(f, wbuf, _IOFBF, sizeof wbuf);
    Writer w{ f };
    w.pod(SAVE_MAGIC); w.pod(SAVE_VERSION);
    unsigned nt = (unsigned)g_world.teams.size(); w.pod(nt);
    for (auto& t0 : g_world.teams) {
        Team& t = const_cast<Team&>(t0);
        w.str(t.name); w.str(t.shortName); w.str(t.stadium); w.str(t.town);
        w.pod(t.kind); w.pod(t.nation); w.pod(t.rating); ioKit(w, t.home); ioKit(w, t.away); ioKit(w, t.third); w.pod(t.hasThird); w.pod(t.culture); w.pod(t.region); w.pod(t.dept);
        w.pod(t.seed); w.pod(t.formation); w.pod(t.dbClub); w.pod(t.parent); w.pod(t.resLevel); w.pod(t.custom); w.pod(t.edited);
        w.pod(t.lastTier); w.pod(t.founded); w.pod(t.squadGen); w.vstr(t.honours);
        unsigned np = (unsigned)t.squad.size(); w.pod(np);
        for (auto& p : t.squad) {
            w.str(p.name); w.pod(p.pos); w.pod(p.num); w.pod(p.speed); w.pod(p.shoot); w.pod(p.pass); w.pod(p.tackle); w.pod(p.keep);
            w.pod(p.stamina); w.pod(p.skin); w.pod(p.hair); w.pod(p.suspended); w.pod(p.yellows); w.pod(p.injured); w.pod(p.goals); w.pod(p.apps); w.pod(p.nation);
            w.pod(p.id); w.pod(p.age); w.pod(p.pot); w.pod(p.assists); w.pod(p.contract);
            w.pod(p.cond); w.pod(p.morale); w.pod(p.dribble); w.pod(p.heading);
        }
        w.vpod(t.xi); w.pod(t.status); w.pod(t.sta); w.str(t.sponsor);
        for (float c : t.coefs) w.pod(c);
        w.pod(t.condT);
    }
    w.pod(kind); w.pod(userTeam); w.pod(year); w.pod(cdf); w.pod(cdfNational); w.vpod(cdfRegional); w.vpod(nationalCups);
    w.vpod(regionalCups); w.vpod(deptCups); w.vpod(superCups); w.pod(ucl); w.pod(uel); w.pod(uecl); w.pod(uefaSuper);
    w.vpod(nextEuro.uclQ1); w.vpod(nextEuro.uclQ2); w.vpod(nextEuro.uclQ3); w.vpod(nextEuro.uclGS); w.vpod(nextEuro.uefaQR); w.vpod(nextEuro.uefaR1);
    unsigned nu = (unsigned)uefa.size(); w.pod(nu);
    for (auto& u : uefa) { w.str(u.code); for (float p : u.pts) w.pod(p); w.pod(u.cur); w.pod(u.clubs); }
    w.vstr(history); w.vpod(pendingNewClubs); w.pod(prevUclWinner); w.pod(prevUefaWinner);
    for (auto* m : { &prevChampion, &prevCupWinner, &prevRunnerUp }) {
        unsigned n = (unsigned)m->size(); w.pod(n);
        for (auto& kv : *m) { w.str(kv.first); w.pod(kv.second); }
    }
    w.pod(intlType); w.pod(intlWithQual); w.vpod(intlHosts); w.vpod(qualComps); w.pod(finalComp); w.pod(intlStage);
    w.vpod(g_intl.qualified); w.vpod(g_intl.icpoCSA); w.vpod(g_intl.icpoAFC); w.vpod(g_intl.icpoCCF); w.vpod(g_intl.icpoOFC);
    w.vpod(g_intl.hosts); w.pod(g_intl.finalTeams);
    w.str(custom.name); w.pod(custom.format); w.pod(custom.legs); w.vpod(custom.teams); w.pod(custom.groups);
    w.pod(g_world.nextPid); w.pod(cdfHolderDirect);
    w.pod(mgr.budget); w.pod(mgr.seasonIncome); w.pod(mgr.seasonWages); w.pod(mgr.seasonTransfers); w.pod(mgr.objective); w.pod(mgr.objTarget);
    w.pod(mgr.confidence); w.pod(mgr.lastMonth); w.pod(mgr.sacked); w.vpod(mgr.transfers); w.pod(mgr.incomeBase);
    w.pod(mgr.managerMode); w.pod(mgr.sponsorIncome); w.vpod(mgr.staff); w.vpod(mgr.ctrlReserves); w.pod(mgr.noSack); w.pod(mgr.statusChoice); w.pod(mgr.needStatus); w.vpod(mgr.projects); w.pod(mgr.seasonGate); w.pod(mgr.seasonShop); w.pod(mgr.seasonStadiumCost); w.pod(cdl);
    unsigned np = (unsigned)pyramids.size(); w.pod(np);
    for (auto& P : pyramids) {
        w.str(P.country); w.str(P.name); w.pod(P.barrageUp); w.pod(P.dom);
        unsigned n = (unsigned)P.tiers.size(); w.pod(n);
        for (auto& t : P.tiers) { w.str(t.name); w.pod(t.scope); w.pod(t.groupsPerPool); w.pod(t.groupSize); w.pod(t.up); w.pod(t.down); w.pod(t.flexible); w.pod(t.noReserves); w.pod(t.tb); w.pod(t.barrageUp); }
        n = (unsigned)P.pools.size(); w.pod(n);
        for (auto& pl : P.pools) { w.pod(pl.tier); w.pod(pl.key); w.pod(pl.nGroups); w.pod(pl.size); w.pod(pl.upCap); w.pod(pl.terminal); w.vpod(pl.clubs); w.vvi(pl.groups); w.vpod(pl.comps); }
    }
    const Season& S = season;
    w.vpod(S.controlled); w.pod(S.now); w.pod(S.year); w.pod(S.finished); w.vstr(S.news); w.pod(S.mode);
    unsigned nc = (unsigned)S.comps.size(); w.pod(nc);
    for (auto& c : S.comps) wComp(w, c);
    // version 6
    w.pod(mgr.trainFocus); w.pod(mgr.trainInt); w.pod(superRegions);
    { std::vector<std::pair<int, int>> v(prevRegCupWinner.begin(), prevRegCupWinner.end()); w.vpod(v); }
    // version 7
    saveV7(w);
    w.vstr(honourVenue);          // version 8
    saveV9(w);                    // version 9
    w.vpod(mgr.offers);           // version 10
    saveV11(w);                   // version 11
    saveV12(w);                   // version 12
    saveV13(w);                   // version 13
    saveV14(w);                   // version 14
    saveV15(w);                   // version 15
    saveV17(w);                   // version 17 : vie privée, corruption
    fclose(f);
    return true;
}

bool Career::load(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    static char rbuf[1 << 20]; setvbuf(f, rbuf, _IOFBF, sizeof rbuf);
    Reader r{ f };
    unsigned magic = 0, ver = 0; r.pod(magic); r.pod(ver);
    if (magic != SAVE_MAGIC || ver < 21 || ver > SAVE_VERSION) { fclose(f); return false; }   // build 5 : anciennes sauvegardes incompatibles
    g_world.build();
    unsigned nt = 0; r.pod(nt);
    if (!r.ok || nt > 200000) { fclose(f); return false; }
    g_world.teams.resize(nt);
    for (auto& t : g_world.teams) {
        r.str(t.name); r.str(t.shortName); r.str(t.stadium); r.str(t.town);
        r.pod(t.kind); r.pod(t.nation); r.pod(t.rating); ioKit(r, t.home); ioKit(r, t.away); ioKit(r, t.third); r.pod(t.hasThird); r.pod(t.culture); r.pod(t.region); r.pod(t.dept);
        r.pod(t.seed); r.pod(t.formation); r.pod(t.dbClub); r.pod(t.parent); r.pod(t.resLevel); r.pod(t.custom); r.pod(t.edited);
        r.pod(t.lastTier); r.pod(t.founded); r.pod(t.squadGen); r.vstr(t.honours);
        unsigned np = 0; r.pod(np); if (!r.ok || np > 200) { fclose(f); return false; }
        t.squad.resize(np);
        for (auto& p : t.squad) {
            r.str(p.name); r.pod(p.pos); r.pod(p.num); r.pod(p.speed); r.pod(p.shoot); r.pod(p.pass); r.pod(p.tackle); r.pod(p.keep);
            r.pod(p.stamina); r.pod(p.skin); r.pod(p.hair); r.pod(p.suspended); r.pod(p.yellows); r.pod(p.injured); r.pod(p.goals); r.pod(p.apps); r.pod(p.nation);
            r.pod(p.id); r.pod(p.age); r.pod(p.pot); r.pod(p.assists); r.pod(p.contract);
            if (ver >= 6) { r.pod(p.cond); r.pod(p.morale); r.pod(p.dribble); r.pod(p.heading); }
            p.gender = 0; p.years = 0; p.wageK = 0;          // données version 14 (relues plus loin)
        }
        t.reputation = 0; t.academy = 0; t.freeAgents = t.name == "Joueurs libres (sans club)" && t.nation < 0; t.presGender = 0;
        r.vpod(t.xi); r.pod(t.status); r.pod(t.sta); r.str(t.sponsor);
        if (ver >= 6) { for (float& c : t.coefs) r.pod(c); r.pod(t.condT); }
    }
    r.pod(kind); r.pod(userTeam); r.pod(year); r.pod(cdf); r.pod(cdfNational); r.vpod(cdfRegional); r.vpod(nationalCups);
    r.vpod(regionalCups); r.vpod(deptCups); r.vpod(superCups); r.pod(ucl); r.pod(uel); r.pod(uecl); r.pod(uefaSuper);
    r.vpod(nextEuro.uclQ1); r.vpod(nextEuro.uclQ2); r.vpod(nextEuro.uclQ3); r.vpod(nextEuro.uclGS); r.vpod(nextEuro.uefaQR); r.vpod(nextEuro.uefaR1);
    unsigned nu = 0; r.pod(nu); if (!r.ok || nu > 200) { fclose(f); return false; }
    uefa.resize(nu);
    for (auto& u : uefa) { r.str(u.code); for (float& p : u.pts) r.pod(p); r.pod(u.cur); r.pod(u.clubs); }
    if (ver < 6) initClubCoefs(uefa);
    r.vstr(history); r.vpod(pendingNewClubs); r.pod(prevUclWinner); r.pod(prevUefaWinner);
    for (auto* m : { &prevChampion, &prevCupWinner, &prevRunnerUp }) {
        unsigned n = 0; r.pod(n); if (!r.ok || n > 1000) { fclose(f); return false; }
        m->clear();
        for (unsigned i = 0; i < n; i++) { std::string k; int v = 0; r.str(k); r.pod(v); (*m)[k] = v; }
    }
    r.pod(intlType); r.pod(intlWithQual); r.vpod(intlHosts); r.vpod(qualComps); r.pod(finalComp); r.pod(intlStage);
    r.vpod(g_intl.qualified); r.vpod(g_intl.icpoCSA); r.vpod(g_intl.icpoAFC); r.vpod(g_intl.icpoCCF); r.vpod(g_intl.icpoOFC);
    r.vpod(g_intl.hosts); r.pod(g_intl.finalTeams);
    r.str(custom.name); r.pod(custom.format); r.pod(custom.legs); r.vpod(custom.teams); r.pod(custom.groups);
    r.pod(g_world.nextPid); r.pod(cdfHolderDirect);
    r.pod(mgr.budget); r.pod(mgr.seasonIncome); r.pod(mgr.seasonWages); r.pod(mgr.seasonTransfers); r.pod(mgr.objective); r.pod(mgr.objTarget);
    r.pod(mgr.confidence); r.pod(mgr.lastMonth); r.pod(mgr.sacked); r.vpod(mgr.transfers); r.pod(mgr.incomeBase);
    r.pod(mgr.managerMode); r.pod(mgr.sponsorIncome);
    if (ver >= 14) r.vpod(mgr.staff);
    else { std::vector<StaffMemberV13> old; r.vpod(old); mgr.staff.clear(); for (auto& o : old) { StaffMember m; memcpy(m.name, o.name, 32); m.role = o.role; m.level = o.level; m.wage = o.wage; m.age = o.age; mgr.staff.push_back(m); } }
    r.vpod(mgr.ctrlReserves); r.pod(mgr.noSack); r.pod(mgr.statusChoice); r.pod(mgr.needStatus); r.vpod(mgr.projects); r.pod(mgr.seasonGate); r.pod(mgr.seasonShop); r.pod(mgr.seasonStadiumCost); r.pod(cdl);
    unsigned np = 0; r.pod(np); if (!r.ok || np > 1000) { fclose(f); return false; }
    pyramids.assign(np, Pyramid());
    for (auto& P : pyramids) {
        r.str(P.country); r.str(P.name); r.pod(P.barrageUp); r.pod(P.dom);
        unsigned n = 0; r.pod(n); if (!r.ok || n > 100) { fclose(f); return false; }
        P.tiers.resize(n);
        for (auto& t : P.tiers) { r.str(t.name); r.pod(t.scope); r.pod(t.groupsPerPool); r.pod(t.groupSize); r.pod(t.up); r.pod(t.down); r.pod(t.flexible); r.pod(t.noReserves); r.pod(t.tb); r.pod(t.barrageUp); }
        r.pod(n); if (!r.ok || n > 100000) { fclose(f); return false; }
        P.pools.resize(n);
        for (auto& pl : P.pools) { r.pod(pl.tier); r.pod(pl.key); r.pod(pl.nGroups); r.pod(pl.size); r.pod(pl.upCap); r.pod(pl.terminal); r.vpod(pl.clubs); r.vvi(pl.groups); r.vpod(pl.comps); }
    }
    season = Season();
    Season& S = season;
    r.vpod(S.controlled); r.pod(S.now); r.pod(S.year); r.pod(S.finished); r.vstr(S.news); r.pod(S.mode);
    unsigned nc = 0; r.pod(nc); if (!r.ok || nc > 100000) { fclose(f); return false; }
    S.comps.reserve(std::max(8000u, nc + 100));
    S.comps.resize(nc);
    for (auto& c : S.comps) rComp(r, c);
    superRegions = -1; prevRegCupWinner.clear();
    if (ver >= 6) {
        r.pod(mgr.trainFocus); r.pod(mgr.trainInt); r.pod(superRegions);
        std::vector<std::pair<int, int>> v; r.vpod(v); for (auto& kv : v) prevRegCupWinner[kv.first] = kv.second;
        if (superRegions >= (int)S.comps.size()) superRegions = -1;
    }
    if (ver >= 7) loadV7(r); else resetV7();
    honourVenue.clear();
    if (ver >= 8) r.vstr(honourVenue);
    honourVenue.resize(honourLog.size());
    euroOnly = false; u19Final = gambNational = -1; gambRegional.clear(); u19Cups.clear();
    if (ver >= 9) loadV9(r);
    mgr.offers.clear();
    if (ver >= 10) r.vpod(mgr.offers);
    youthPrelim = youthUcl = lastU19Champ = -1; youthDirect.clear(); reconversions.clear();
    mgr.delegTrain = mgr.delegSubs = mgr.fullManager = false; managerPlace.clear(); managerBday = managerBmonth = 0;
    if (ver >= 11) loadV11(r);
    assignDistricts(g_world);
    cupVenue.clear(); cdfAskYear = -1; u17Final = -1; lastU17Champ = -1;
    if (ver >= 12) loadV12(r);
    for (auto& t : g_world.teams) t.merch = Merch();
    if (ver >= 13) loadV13(r);
    coach = false; coachNation = -1; coachCalls.clear(); coachLog.clear(); coachNextHosts.clear(); coachCamp = 0;
    regSuperCups.clear(); prevR1Champ.clear(); archive.clear(); archScorers.clear();
    if (ver >= 14) loadV14(r);
    nlLeague.clear(); nlYear = -1; opts = Opts(); newEuro = NewEuroSpots(); prevUeclWinner = -1; coachCat = 0; coachU21Podium.clear(); mgr.sponsorYears = mgr.namingYears = 0;
    if (ver >= 15) loadV15(r);
    if (ver >= 17) loadV17(r);
    g_world.rebuildCountryClubs(pyramids);
    fclose(f);
    return r.ok;
}

// ------------------------------------------------------------------ version 11 : Ligue des champions U19, infos personnelles, historique, tactique
void Career::saveV11(Writer& w) const {
    w.pod(youthPrelim); w.pod(youthUcl); w.pod(lastU19Champ); w.vpod(youthDirect);
    w.pod(mgr.delegTrain); w.pod(mgr.delegSubs); w.pod(mgr.fullManager);
    w.str(managerPlace); w.pod(managerBday); w.pod(managerBmonth); w.vstr(reconversions);
    for (auto& t : g_world.teams) {
        w.str(t.president); w.pod(t.presAge); w.str(t.presPlace);
        w.pod(t.pressing); w.pod(t.defLine); w.pod(t.width); w.pod(t.tempo); w.pod(t.passStyle);
        unsigned np = (unsigned)t.squad.size(); w.pod(np);
        for (auto& p : t.squad) {
            w.pod(p.bday); w.pod(p.bmonth); w.str(p.birthPlace); w.pod(p.positioning); w.pod(p.composure); w.pod(p.sYel); w.pod(p.sRed);
            w.vpod(p.hist);
        }
    }
}
void Career::loadV11(Reader& r) {
    r.pod(youthPrelim); r.pod(youthUcl); r.pod(lastU19Champ); r.vpod(youthDirect);
    r.pod(mgr.delegTrain); r.pod(mgr.delegSubs); r.pod(mgr.fullManager);
    r.str(managerPlace); r.pod(managerBday); r.pod(managerBmonth); r.vstr(reconversions);
    for (auto& t : g_world.teams) {
        if (!r.ok) break;
        r.str(t.president); r.pod(t.presAge); r.str(t.presPlace);
        r.pod(t.pressing); r.pod(t.defLine); r.pod(t.width); r.pod(t.tempo); r.pod(t.passStyle);
        unsigned np = 0; r.pod(np);
        if (!r.ok || np != t.squad.size()) { r.ok = false; break; }
        for (auto& p : t.squad) {
            r.pod(p.bday); r.pod(p.bmonth); r.str(p.birthPlace); r.pod(p.positioning); r.pod(p.composure); r.pod(p.sYel); r.pod(p.sRed);
            r.vpod(p.hist);
        }
    }
    int n = (int)season.comps.size();
    if (youthPrelim >= n) youthPrelim = -1;
    if (youthUcl >= n) youthUcl = -1;
    if (lastU19Champ >= (int)g_world.teams.size()) lastU19Champ = -1;
}

void Career::saveV12(Writer& w) const {
    for (auto& t : g_world.teams) w.pod(t.district);
    unsigned n = (unsigned)cupVenue.size(); w.pod(n);
    for (auto& kv : cupVenue) { w.pod(kv.first); w.str(kv.second); }
    w.pod(cdfAskYear); w.pod(u17Final); w.pod(lastU17Champ);
}
void Career::loadV12(Reader& r) {
    for (auto& t : g_world.teams) { int d = -1; r.pod(d); if (d >= -1 && d < numDistricts()) t.district = d; }
    unsigned n = 0; r.pod(n);
    for (unsigned i = 0; i < n && r.ok && i < 100000; i++) { int k = 0; std::string v; r.pod(k); r.str(v); cupVenue[k] = v; }
    r.pod(cdfAskYear); r.pod(u17Final); r.pod(lastU17Champ);
    if (u17Final >= (int)season.comps.size()) u17Final = -1;
}

// ------------------------------------------------------------------ version 13 : produits dérivés
void Career::saveV13(Writer& w) const {
    unsigned n = 0; for (auto& t : g_world.teams) if (t.merch.init) n++;
    w.pod(n);
    for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].merch.init) { w.pod(i); w.pod(g_world.teams[i].merch); }
}
void Career::loadV13(Reader& r) {
    unsigned n = 0; r.pod(n);
    for (unsigned k = 0; k < n && r.ok && k < 1000000; k++) {
        int i = -1; Merch m; r.pod(i); r.pod(m);
        if (r.ok && i >= 0 && i < (int)g_world.teams.size()) g_world.teams[i].merch = m;
    }
}

// ------------------------------------------------------------------ version 14 : sélectionneur, genre, réputation...
void Career::saveV14(Writer& w) const {
    coachSave(w, *this);
    w.vpod(regSuperCups);
    { std::vector<std::pair<int, int>> v(prevR1Champ.begin(), prevR1Champ.end()); w.vpod(v); }
    archiveSave(w, *this);
    {   // genre des personnes (femmes uniquement : tout le monde est un homme par défaut)
        std::vector<int32_t> pw; for (auto& t : g_world.teams) for (auto& p : t.squad) if (p.gender) pw.push_back(p.id);
        w.vpod(pw);
        std::vector<int32_t> pr; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].presGender) pr.push_back(i);
        w.vpod(pr);
        w.pod(managerGender);
    }
    // économie : réputation et centre de formation des clubs, contrats des joueurs, politique des recettes
    {
        std::vector<uint8_t> rep, aca;
        for (auto& t : g_world.teams) { rep.push_back(t.reputation); aca.push_back(t.academy); }
        w.vpod(rep); w.vpod(aca);
        struct PC { int32_t pid; int32_t wage; uint8_t years, pad[3]; };
        std::vector<PC> pc;
        for (auto& t : g_world.teams) for (auto& p : t.squad) if (p.years || p.wageK) { PC x; x.pid = p.id; x.wage = p.wageK; x.years = p.years; x.pad[0] = x.pad[1] = x.pad[2] = 0; pc.push_back(x); }
        w.vpod(pc);
        w.pod(mgr.cdfGiveAll); w.pod(mgr.tvShare); w.pod(mgr.dncg); w.pod(mgr.bankrupt);
    }
}
void Career::loadV14(Reader& r) {
    coachLoad(r, *this);
    r.vpod(regSuperCups);
    { std::vector<std::pair<int, int>> v; r.vpod(v); prevR1Champ.clear(); for (auto& kv : v) prevR1Champ[kv.first] = kv.second; }
    regSuperCups.erase(std::remove_if(regSuperCups.begin(), regSuperCups.end(), [&](int c) { return c < 0 || c >= (int)season.comps.size(); }), regSuperCups.end());
    archiveLoad(r, *this);
    {
        std::vector<int32_t> pw; r.vpod(pw);
        std::set<int32_t> ws(pw.begin(), pw.end());
        if (!ws.empty()) for (auto& t : g_world.teams) for (auto& p : t.squad) if (ws.count(p.id)) p.gender = 1;
        std::vector<int32_t> pr; r.vpod(pr);
        for (int i : pr) if (i >= 0 && i < (int)g_world.teams.size()) g_world.teams[i].presGender = 1;
        r.pod(managerGender);
    }
    {
        std::vector<uint8_t> rep, aca; r.vpod(rep); r.vpod(aca);
        for (size_t i = 0; i < g_world.teams.size(); i++) { if (i < rep.size()) g_world.teams[i].reputation = rep[i]; if (i < aca.size()) g_world.teams[i].academy = aca[i]; }
        struct PC { int32_t pid; int32_t wage; uint8_t years, pad[3]; };
        std::vector<PC> pc; r.vpod(pc);
        if (!pc.empty()) {
            std::map<int32_t, PC> m; for (auto& x : pc) m[x.pid] = x;
            for (auto& t : g_world.teams) for (auto& p : t.squad) { auto it = m.find(p.id); if (it != m.end()) { p.wageK = it->second.wage; p.years = it->second.years; } }
        }
        r.pod(mgr.cdfGiveAll); r.pod(mgr.tvShare); r.pod(mgr.dncg); r.pod(mgr.bankrupt);
    }
}

// ------------------------------------------------------------------ version 15 : Ligue des nations, options de carrière
void Career::saveV15(Writer& w) const {
    w.vpod(nlLeague); w.pod(nlYear); w.pod(opts);
    for (auto& l : newEuro.l) w.vpod(l);
    w.pod(prevUeclWinner);
    w.pod(coachCat); w.vpod(coachU21Podium);
    w.pod(mgr.sponsorYears); w.pod(mgr.namingYears);
}
void Career::loadV15(Reader& r) {
    r.vpod(nlLeague); r.pod(nlYear); r.pod(opts);
    for (auto& l : newEuro.l) r.vpod(l);
    r.pod(prevUeclWinner);
    if (prevUeclWinner >= (int)g_world.teams.size()) prevUeclWinner = -1;
    r.pod(coachCat); r.vpod(coachU21Podium);
    if (coachCat < 0 || coachCat > 3) coachCat = 0;
    r.pod(mgr.sponsorYears); r.pod(mgr.namingYears);
    if (!nlLeague.empty() && (int)nlLeague.size() != NUM_NATIONS) nlLeague.clear();
}

// feuille de match et remplacements selon la compétition (anciennes règles en pro et en National)
void Career::sheetRules(int comp, int& sheet, int& subs, bool& rolling) const {
    sheet = 18; subs = 3; rolling = false;
    if (comp < 0 || comp >= (int)season.comps.size()) return;
    const Competition& C = season.comps[comp];
    if (C.rollingSubs()) { sheet = 14; subs = 99; rolling = true; return; }
    if (C.kind == 12) { sheet = 18; subs = 5; return; }
    if (C.kind == 50 && C.tag >= 0 && C.tag < NUM_LEGENDS) { legendSheet(C.tag, sheet, subs); return; }
    if (C.kind >= 40 && C.kind <= 44) { sheet = 20; subs = 5; return; }                 // football féminin : 20 joueuses, 5 remplacements
    if (C.kind == 15 || C.kind == 16 || C.kind == 19 || C.kind == 22 || C.kind == 20 || C.kind == 21) { sheet = 16; subs = 3; return; }
    if (C.kind == 1 && C.tag >= 0) {
        int p = C.tag / 100000, q = (C.tag / 100) % 1000;
        if (p < (int)pyramids.size() && q < (int)pyramids[p].pools.size()) {
            const Pyramid& P = pyramids[p]; int tier = pyramids[p].pools[q].tier;
            if (P.country == "U15") { sheet = 14; subs = 99; rolling = true; return; }
            if (isWomenPyramid(P)) { if (tier <= 1 || P.country != "F:FRA") { sheet = 20; subs = 5; } else { sheet = 16; subs = 3; } return; }                                   // U15 : remplacements libres
            if (P.country == "U19" || P.country == "U17") { sheet = tier == 0 ? 16 : 14; subs = 3; return; }
            if (P.country == "FRA" && P.dom < 0 && (tier == 3 || tier == 4)) { sheet = 16; subs = 3; return; }             // National 2 et National 3
            if (P.country == "FRA" && P.dom < 0 && tier >= 5 && tier <= 7) { sheet = 14; subs = 3; return; }             // Régional 1 à 3
            if (P.country == "FRA" && (P.dom >= 0 || tier >= 8)) { sheet = 14; subs = 99; rolling = true; return; }     // District : remplacements libres
        }
        return;
    }
    if (C.kind == 2 && C.name.find("tours régionaux") != std::string::npos) { sheet = 14; subs = 3; return; }
    if (C.kind == 26 || C.kind == 27 || C.kind == 13) { sheet = 14; subs = 3; return; }
}

// Coupe de France : tours régionaux où joue le club du joueur, s'il évolue en district (inscription facultative)
int Career::userCdfComp() const {
    if (kind != CK_CLUB || euroOnly || userTeam < 0) return -1;
    int p, q, g;
    int tier = tierOfTeam(userTeam, &p, &q, &g);
    if (tier < 8 || pyramids[p].country != "FRA" || pyramids[p].dom >= 0) return -1;
    for (int c : cdfRegional) {
        const Competition& C = season.comps[c];
        for (auto& e : C.entrants) for (int t : e) if (t == userTeam) return c;
    }
    return -1;
}

// retrait d'une équipe d'une coupe avant son premier match (nouveau tirage du tour si rien n'est joué)
bool Career::withdrawFromCup(int comp, int team) {
    if (comp < 0 || comp >= (int)season.comps.size()) return false;
    Competition& C = season.comps[comp];
    for (auto& m : C.matches) if (m.played && (m.home == team || m.away == team)) return false;
    bool any = false;
    for (auto& e : C.entrants) { auto it = std::find(e.begin(), e.end(), team); if (it != e.end()) { e.erase(it); any = true; } }
    bool played = false; for (auto& m : C.matches) if (m.played) played = true;
    if (!played && C.format == FMT_CUP && !C.entrants.empty()) {
        C.stages.clear(); C.matches.clear(); C.cur = 0; C.events.clear();
        C.cupRound(0, C.entrants[0]);
    } else if (any) {
        // tour déjà tiré et entamé : forfait (l'adversaire est qualifié)
        for (auto& m : C.matches) if (!m.played && (m.home == team || m.away == team)) {
            m.hg = (int16_t)(m.home == team ? 0 : 3); m.ag = (int16_t)(m.home == team ? 3 : 0); m.played = true;
        }
    }
    return any;
}

void Career::recordSeasonHistory() {
    for (int ti = 0; ti < (int)g_world.teams.size(); ti++) {
        Team& t = g_world.teams[ti];
        if (!t.squadGen || t.kind != TK_CLUB) continue;
        for (auto& p : t.squad) recordPlayerSeason(p, ti, year, false);
    }
}

void Career::saveU19(Writer& w) const { w.pod(u19Final); w.pod(gambNational); w.vpod(gambRegional); w.vpod(u19Cups); }
void Career::loadU19(Reader& r) {
    r.pod(u19Final); r.pod(gambNational); r.vpod(gambRegional); r.vpod(u19Cups);
    int n = (int)season.comps.size();
    if (u19Final >= n) u19Final = -1;
    if (gambNational >= n) gambNational = -1;
}

// ------------------------------------------------------------------ données ajoutées en version 9 (mentalité, capitaines, U19, mode Europe)
void Career::saveV9(Writer& w) const {
    w.pod(euroOnly);
    unsigned n = (unsigned)g_world.teams.size(); w.pod(n);
    for (auto& t : g_world.teams) { w.pod(t.mentality); w.pod(t.captainPid); w.pod(t.vicePid); w.pod(t.youth); }
    saveU19(w);
}
void Career::loadV9(Reader& r) {
    r.pod(euroOnly);
    unsigned n = 0; r.pod(n);
    if (!r.ok || n != g_world.teams.size()) { r.ok = false; return; }
    for (auto& t : g_world.teams) { r.pod(t.mentality); r.pod(t.captainPid); r.pod(t.vicePid); r.pod(t.youth); }
    loadU19(r);
}

// ------------------------------------------------------------------ données ajoutées en version 7
void Career::saveV7(Writer& w) const {
    w.pod(intlFormat);
    w.vpod(honourLog);
    w.vpod(nextEuro.itR1); w.vpod(nextEuro.itR2); w.vpod(nextEuro.itR3);
    w.str(uclFinalVenue); w.str(uefaFinalVenue); w.str(tdcVenue);
    w.pod(intertoto);
    w.str(managerName); w.pod(managerNation); w.pod(managerSkin); w.pod(managerHair); w.pod(managerAge);
    w.vstr(newsRead);
    w.vpod(retiring);
    w.pod(mgr.boardRequests);
}
void Career::loadV7(Reader& r) {
    r.pod(intlFormat);
    r.vpod(honourLog);
    r.vpod(nextEuro.itR1); r.vpod(nextEuro.itR2); r.vpod(nextEuro.itR3);
    r.str(uclFinalVenue); r.str(uefaFinalVenue); r.str(tdcVenue);
    r.pod(intertoto);
    r.str(managerName); r.pod(managerNation); r.pod(managerSkin); r.pod(managerHair); r.pod(managerAge);
    r.vstr(newsRead);
    r.vpod(retiring);
    r.pod(mgr.boardRequests);
    if (intertoto >= (int)season.comps.size()) intertoto = -1;
}
void Career::resetV7() {
    euroOnly = false;
    intlFormat = 0; honourLog.clear(); honourVenue.clear(); uclFinalVenue.clear(); uefaFinalVenue.clear(); tdcVenue.clear(); intertoto = -1;
    managerName.clear(); managerNation = -1; managerSkin = 0; managerHair = 0; managerAge = 45; newsRead.clear(); retiring.clear(); mgr.boardRequests = 0;
    life = PlayerLife(); loans.clear(); compArch.clear(); compAllTime.clear();
}

// aperçu des qualifiés européens sans modifier la carrière (bilan de fin de saison)
EuroSpots Career::previewEuro() {
    EuroSpots keep = nextEuro;
    std::vector<UefaCountry> ukeep = uefa;
    std::vector<std::vector<float>> ckeep;
    for (auto& t : g_world.teams) ckeep.push_back(std::vector<float>(t.coefs, t.coefs + 5));
    computeUefaPoints(*this);
    computeEuro(*this, true);
    EuroSpots out = nextEuro;
    nextEuro = keep; uefa = ukeep;
    for (size_t i = 0; i < g_world.teams.size(); i++) for (int k = 0; k < 5; k++) g_world.teams[i].coefs[k] = ckeep[i][k];
    return out;
}
