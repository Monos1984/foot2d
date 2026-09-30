// Football féminin (section 11) : clubs et pyramides féminines, joueuses réelles, compétitions
// (play-offs de l'Arkema Première Ligue, Coupe de France féminine, Coupe LFFP, UWCL, coupes nationales étrangères).
// Séparation stricte : une équipe féminine (Team::youth == 6) n'aligne que des joueuses, et inversement.
#include "game.h"
#include <algorithm>
#include <cstring>
#include <map>
#include <set>

void makeKits(Team& t, unsigned s1, unsigned s2, unsigned sh, int pat);
int addCompPublic(Season& S, Competition c);

static std::vector<std::string> splitW(const char* s, char sep) {
    std::vector<std::string> v; std::string cur;
    for (const char* p = s; ; p++) { if (*p == sep || !*p) { v.push_back(cur); cur.clear(); if (!*p) break; } else cur += *p; }
    return v;
}

bool isWomenPyramid(const Pyramid& P) { return P.country.size() > 2 && P.country[0] == 'F' && P.country[1] == ':'; }
bool isWomenTeam(int t) { return t >= 0 && t < (int)g_world.teams.size() && g_world.teams[t].youth == 6; }

// ------------------------------------------------------------------ construction
static int makeWomenTeam(World& w, const std::string& name, const std::string& sh, float rating, unsigned c1, unsigned c2,
                         const std::string& stadium, int nat, int men) {
    Team t;
    t.name = sanitize(name.c_str()); t.shortName = sanitize(sh.c_str());
    t.kind = TK_CLUB; t.nation = nat; t.youth = 6; t.parent = -1; t.resLevel = 0;
    t.culture = nat >= 0 ? NATIONS[nat].culture : CU_EN;
    t.rating = rating;
    if (men >= 0) {
        const Team& M = w.teams[men];
        t.home = M.home; t.away = M.away; t.third = M.third; t.hasThird = M.hasThird;
        t.region = M.region; t.dept = M.dept; t.district = M.district; t.town = M.town;
        t.culture = M.culture;
        t.stadium = stadium.empty() ? "Stade annexe (" + (M.town.empty() ? M.name : M.town) + ")" : sanitize(stadium.c_str());
    } else {
        makeKits(t, c1, c2, c1 == 0xFFFFFF ? c2 : c1, 0);
        t.stadium = stadium.empty() ? "Stade " + t.name : sanitize(stadium.c_str());
    }
    t.status = rating >= 60 ? CS_PRO : rating >= 45 ? CS_SEMIPRO : CS_AMATEUR;
    t.seed = hashStr(t.name) ^ 0xF3F3F3u;
    t.formation = (int)(t.seed % NUM_FORMATIONS);
    w.teams.push_back(t);
    int id = (int)w.teams.size() - 1;
    if (men >= 0) { w.menOf[id] = men; w.womenOf[men] = id; }
    return id;
}

void buildWomen(World& w) {
    w.menOf.clear(); w.womenOf.clear();
    std::map<std::string, int> byName;
    for (int i = w.firstClub; i < (int)w.teams.size(); i++) if (w.teams[i].kind == TK_CLUB && w.teams[i].youth == 0) byName.emplace(w.teams[i].name, i);
    std::map<std::string, Pyramid> pyrs;
    std::vector<std::string> order;
    for (int l = 0; l < NUM_WOMEN_LEAGUES; l++) {
        const WomenLeagueDef& L = WOMEN_LEAGUES[l];
        std::string code = std::string("F:") + L.country;
        int nat = w.nationIndex(L.country);
        if (!pyrs.count(code)) {
            Pyramid P; P.country = code; P.name = (nat >= 0 ? w.teams[nat].name : std::string(L.country)) + " (F)";
            pyrs[code] = P; order.push_back(code);
        }
        Pyramid& P = pyrs[code];
        TierConf tc; tc.name = sanitize(L.name); tc.scope = SC_NATIONAL; tc.groupsPerPool = 1; tc.tb = TB_GD; tc.down = L.down;
        Pool pl; pl.tier = (int)P.tiers.size(); pl.key = 0; pl.nGroups = 1;
        for (auto& cs : splitW(L.clubs, ';')) {
            auto f = splitW(cs.c_str(), '|');
            if (f.size() < 5 || f[0].empty()) continue;
            int men = -1;
            if (f.size() > 6 && !f[6].empty()) { auto it = byName.find(sanitize(f[6].c_str())); if (it != byName.end()) men = it->second; }
            int id = makeWomenTeam(w, f[0], f[1], (float)atoi(f[2].c_str()), (unsigned)strtoul(f[3].c_str(), nullptr, 16), (unsigned)strtoul(f[4].c_str(), nullptr, 16),
                                   f.size() > 5 ? f[5] : std::string(), nat, men);
            pl.clubs.push_back(id);
        }
        tc.groupSize = (int)pl.clubs.size(); pl.size = tc.groupSize;
        P.tiers.push_back(tc); P.pools.push_back(pl);
    }
    // France : D3 féminine (4 poules de 12) et Régional 1 féminin (une poule par ligue), sections féminines de clubs existants
    if (pyrs.count("F:FRA") && !g_basePyramids.empty() && g_basePyramids[0].country == "FRA") {
        Pyramid& P = pyrs["F:FRA"];
        const Pyramid& M = g_basePyramids[0];
        int fra = w.nationIndex("FRA");
        Rng r(0xF3A1E5);
        std::set<int> used; for (auto& kv : w.womenOf) used.insert(kv.first);
        auto candidates = [&](int t0, int t1) {
            std::vector<int> v;
            for (auto& pl : M.pools) if (pl.tier >= t0 && pl.tier <= t1) for (int c : pl.clubs)
                if (w.teams[c].parent < 0 && !used.count(c) && w.teams[c].region >= 0 && w.teams[c].region < 13) v.push_back(c);
            std::stable_sort(v.begin(), v.end(), [&](int a, int b) { return w.teams[a].rating > w.teams[b].rating; });
            return v;
        };
        auto section = [&](int men, float rating) {
            used.insert(men);
            const Team& Mt = w.teams[men];
            std::string sh = Mt.shortName.size() > 5 ? Mt.shortName.substr(0, 5) : Mt.shortName;
            return makeWomenTeam(w, Mt.name + " Féminines", sh + "F", rating, 0, 0, std::string(), fra, men);
        };
        // D3 : 48 clubs parmi les meilleurs clubs amateurs et pros restants
        TierConf d3; d3.name = sanitize("D3 Féminine"); d3.scope = SC_NATIONAL; d3.groupsPerPool = 4; d3.groupSize = 12; d3.up = 1; d3.down = 3; d3.tb = TB_FFF;
        Pool p3; p3.tier = (int)P.tiers.size(); p3.key = 0; p3.nGroups = 4; p3.size = 12; p3.upCap = 2;
        auto c3 = candidates(0, 4);
        for (int i = 0; i < (int)c3.size() && (int)p3.clubs.size() < 48; i++) if (r.chance(0.55f)) p3.clubs.push_back(section(c3[i], r.frange(42, 52)));
        P.tiers.push_back(d3); P.pools.push_back(p3);
        // Régional 1 : 8 clubs par ligue (Corse : pas d'accession)
        TierConf r1; r1.name = sanitize("Régional 1 Féminin"); r1.scope = SC_REGION; r1.groupsPerPool = 1; r1.groupSize = 8; r1.up = 1; r1.down = 0; r1.flexible = true; r1.tb = TB_FFF;
        int rt = (int)P.tiers.size();
        P.tiers.push_back(r1);
        auto cR = candidates(2, 7);
        for (int rg = 0; rg < 13; rg++) {
            Pool pr; pr.tier = rt; pr.key = rg; pr.nGroups = 1; pr.size = 8; pr.terminal = true;
            if (rg == 12) pr.upCap = 0;
            for (int c : cR) { if ((int)pr.clubs.size() >= 8) break; if (w.teams[c].region == rg && !used.count(c) && r.chance(0.35f)) pr.clubs.push_back(section(c, r.frange(30, 42))); }
            for (int c : cR) { if ((int)pr.clubs.size() >= 8) break; if (w.teams[c].region == rg && !used.count(c)) pr.clubs.push_back(section(c, r.frange(28, 38))); }
            P.pools.push_back(pr);
        }
        // montées de Seconde Ligue (2) et de D3 (2 champions de poule)
        P.tiers[0].down = 2; P.tiers[1].up = 2; P.tiers[1].down = 2; P.tiers[2].up = 1;
    }
    for (auto& code : order) {
        Pyramid& P = pyrs[code];
        for (int k = 0; k < (int)P.tiers.size(); k++) {
            if (k + 1 >= (int)P.tiers.size()) P.tiers[k].down = 0;
            if (k == 0) P.tiers[k].up = 0;
            else if (P.tiers[k].up == 0) P.tiers[k].up = P.tiers[k - 1].down;
        }
        for (auto& pl : P.pools) for (int c : pl.clubs) w.teams[c].lastTier = pl.tier;
        formGroups(P);
        g_basePyramids.push_back(P);
    }
}

// joueuses réelles : remplacent des joueuses générées du même poste (effectif de club ou sélection)
void injectWomenStars(Team& t, Rng& r) {
    if (t.youth != 6) return;
    const char* list = nullptr;
    if (t.kind == TK_NATION) { if (t.nation >= 0) for (int i = 0; i < NUM_WOMEN_NATIONS; i++) if (!strcmp(WOMEN_NATIONS[i].key, NATIONS[t.nation].code)) list = WOMEN_NATIONS[i].players; }
    else for (int i = 0; i < NUM_WOMEN_SQUADS; i++) if (t.name == sanitize(WOMEN_SQUADS[i].key)) list = WOMEN_SQUADS[i].players;
    if (!list) return;
    std::vector<bool> done(t.squad.size(), false);
    for (auto& ps : splitW(list, ';')) {
        auto f = splitW(ps.c_str(), '|');
        if (f.size() < 5) continue;
        int pos = f[1] == "G" ? POS_GK : f[1] == "D" ? POS_DF : f[1] == "M" ? POS_MF : POS_FW;
        int slot = -1;
        for (int i = 0; i < (int)t.squad.size() && slot < 0; i++) if (!done[i] && t.squad[i].pos == pos) slot = i;
        if (slot < 0) for (int i = 0; i < (int)t.squad.size() && slot < 0; i++) if (!done[i] && t.squad[i].pos != POS_GK) slot = i;
        if (slot < 0) { Player np = t.squad.empty() ? Player() : t.squad.back(); t.squad.push_back(np); done.push_back(false); slot = (int)t.squad.size() - 1; }
        done[slot] = true;
        Player& p = t.squad[slot];
        int o = std::max(30, std::min(95, atoi(f[2].c_str())));
        p.name = sanitize(f[0].c_str()); p.pos = (uint8_t)pos; p.gender = 1;
        int n = -1; for (int k = 0; k < NUM_NATIONS; k++) if (f[3] == NATIONS[k].code) n = k;
        p.nation = (int16_t)n;
        p.age = (uint8_t)std::max(16, std::min(42, atoi(f[4].c_str())));
        auto c = [&](int v) { return (uint8_t)std::max(15, std::min(99, v + r.range(-1, 1))); };
        switch (pos) {
        case POS_GK: p.keep = (uint8_t)o; p.speed = c(o - 25); p.pass = c(o - 30); p.shoot = c(o - 45); p.tackle = c(o - 40); break;
        case POS_DF: p.tackle = c(o + 2); p.speed = c(o - 2); p.pass = c(o - 2); p.shoot = c(o - 20); p.keep = 15; break;
        case POS_MF: p.pass = c(o + 2); p.speed = c(o - 1); p.shoot = c(o - 2); p.tackle = c(o - 3); p.keep = 15; break;
        default: p.shoot = c(o + 1); p.speed = c(o + 1); p.pass = c(o - 3); p.tackle = c(o - 25); p.keep = 12; break;
        }
        p.stamina = c(o - 5);
        p.pot = (uint8_t)std::min(99, o + std::max(0, 27 - (int)p.age));
        p.dribble = p.heading = p.positioning = p.composure = 0;
    }
}

// ------------------------------------------------------------------ compétitions
static int findKind(const Season& S, int kind, int tag = -999) {
    for (int i = 0; i < (int)S.comps.size(); i++) if (S.comps[i].kind == kind && (tag == -999 || S.comps[i].tag == tag)) return i;
    return -1;
}
int womenPyramid(const Career& K, const char* country) {
    std::string code = std::string("F:") + country;
    for (int i = 0; i < (int)K.pyramids.size(); i++) if (K.pyramids[i].country == code) return i;
    return -1;
}

void womenStartSeason(Career& K) {
    Season& S = K.season;
    if (K.euroOnly || K.kind != CK_CLUB) return;
    // coupes nationales féminines
    for (int p = 0; p < (int)K.pyramids.size(); p++) {
        const Pyramid& P = K.pyramids[p];
        if (!isWomenPyramid(P)) continue;
        std::string cc = P.country.substr(2);
        std::vector<std::vector<int>> byTier(P.tiers.size());
        for (auto& pl : P.pools) for (int t : pl.clubs) byTier[pl.tier].push_back(t);
        Competition c;
        c.format = FMT_CUP; c.name = womenCupName(cc.c_str()); c.shortName = c.name; c.homeRule = 1; c.neutralFinal = true;
        if (cc == "FRA" && P.tiers.size() >= 4) {
            // tirage intégral à chaque tour (aucun tableau prédéfini) ; l'Arkema Première Ligue entre en 16es
            c.kind = 41; c.tag = p;
            std::vector<int> low; for (int k = 2; k < (int)byTier.size(); k++) for (int t : byTier[k]) low.push_back(t);
            int nSL = (int)byTier[1].size(), nPL = (int)byTier[0].size();
            c.entrants = { low, {}, byTier[1], {}, byTier[0] };
            int x0 = (int)low.size(), a = (x0 + 1) / 2, b2 = (a + 1) / 2, x2 = b2 + nSL, c3 = (x2 + 1) / 2;
            int d4 = std::max((c3 + 1) / 2, std::min(c3, 32 - nPL));
            c.koTargets = { a, b2, c3, d4, 16, 8, 4, 2, 1 };
            c.koNames = { "1er tour fédéral", "2e tour fédéral", "3e tour fédéral", "4e tour fédéral", "16es de finale", "8es de finale", "Quarts de finale", "Demi-finales", "Finale" };
            c.koTimes = { 5, 8, 11, 14, 17, 21, 26, 31, 39.2 };
            c.regionalDraw = false;
        } else {
            std::vector<int> all; for (auto& v : byTier) for (int t : v) all.push_back(t);
            if (all.size() < 4) continue;
            c.kind = 44; c.tag = p;
            c.entrants.assign(1, all);
            int n = (int)all.size(), pw = 1; while (pw * 2 <= n) pw *= 2; if (pw == n) pw /= 2;
            for (int x = pw; x >= 1; x /= 2) c.koTargets.push_back(x);
            int R = (int)c.koTargets.size();
            for (int i = 0; i < R; i++) c.koTimes.push_back(8 + 30.0 * i / std::max(1, R - 1));
        }
        int idx = addCompPublic(S, std::move(c));
        S.comps[idx].cupRound(0, S.comps[idx].entrants[0]);
    }
    int fp = womenPyramid(K, "FRA");
    // Coupe LFFP : poule unique des clubs professionnels (Arkema Première Ligue), matchs secs, tirs au but directs en cas d'égalité ; finale entre les deux premiers
    if (fp >= 0) {
        const Pyramid& P = K.pyramids[fp];
        int q0 = P.poolIndex(0, 0);
        if (q0 >= 0 && P.pools[q0].clubs.size() >= 4) {
            std::vector<int> t = P.pools[q0].clubs;
            Competition c;
            c.format = FMT_TOURNAMENT; c.kind = 42; c.tag = fp; c.legs = 1; c.tb = TB_GD;
            c.name = fmt("Coupe LFFP %d", K.year); c.shortName = "Coupe LFFP";
            c.groupsAdvance = 2; c.bestThirds = 0; c.thirdPlace = false;
            int n = (int)t.size(), R = n % 2 ? n : n - 1;
            std::vector<double> times; for (int i = 0; i < R; i++) times.push_back(5.0 + 28.0 * i / std::max(1, R - 1));
            c.addGroupStage({ t }, 1, times, "Poule unique");
            for (auto& m : c.matches) { m.decisive = 1; m.noET = 1; m.neutral = false; }
            c.koTimes = { 35.0 };
            addCompPublic(S, std::move(c));
        }
    }
    // UWCL : phase de ligue unique (18 clubs, 6 journées), barrages 5e-12e, quarts, demies, finale sur terrain neutre
    {
        static const struct { const char* c; int n; } Q[] = { { "ESP", 4 }, { "ENG", 4 }, { "FRA", 4 }, { "GER", 4 }, { "ITA", 2 } };
        std::vector<int> pool;
        for (auto& q : Q) {
            int p = womenPyramid(K, q.c);
            if (p < 0) continue;
            int q0 = K.pyramids[p].poolIndex(0, 0);
            if (q0 < 0) continue;
            std::vector<int> cl = K.pyramids[p].pools[q0].clubs;
            std::vector<int> pick;
            std::string code = K.pyramids[p].country;
            for (auto* m : { &K.prevChampion, &K.prevRunnerUp }) { auto it = m->find(code); if (it != m->end() && std::find(cl.begin(), cl.end(), it->second) != cl.end()) pick.push_back(it->second); }
            std::stable_sort(cl.begin(), cl.end(), [](int a, int b) { return g_world.teams[a].rating > g_world.teams[b].rating; });
            for (int t : cl) if ((int)pick.size() < q.n && std::find(pick.begin(), pick.end(), t) == pick.end()) pick.push_back(t);
            for (int t : pick) pool.push_back(t);
        }
        if (pool.size() >= 8) {
            Competition c;
            c.format = FMT_EUROPE; c.kind = 43; c.name = fmt("Ligue des champions féminine %d-%02d", K.year, (K.year + 1) % 100); c.shortName = "UWCL";
            c.neutralFinal = true; c.tb = TB_GD;
            std::vector<double> times = { 8, 10, 12, 14, 17, 19 };
            c.koTimes = { 23, 28, 33, 42.2 };
            c.setupSwiss(pool, 6, times);
            addCompPublic(S, std::move(c));
        }
    }
}

// fin d'une compétition féminine (appelé pour chaque compétition terminée)
void womenOnCompDone(Career& K, int comp) {
    Season& S = K.season;
    Competition& C = S.comps[comp];
    int fp = womenPyramid(K, "FRA");
    // Arkema Première Ligue : les 4 premiers disputent les play-offs (demi-finales et finale sur match unique)
    if (fp >= 0 && C.format == FMT_LEAGUE && C.tag / 100000 == fp) {
        const Pyramid& P = K.pyramids[fp];
        int q0 = P.poolIndex(0, 0);
        if (q0 >= 0 && !P.pools[q0].comps.empty() && P.pools[q0].comps[0] == comp && findKind(S, 40) < 0 && C.result.size() >= 4) {
            Competition c;
            c.format = FMT_KO_ONLY; c.kind = 40; c.tag = comp; c.qualSpots = 1; c.neutralFinal = false;
            c.name = "Arkema Première Ligue - Play-offs"; c.shortName = "Play-offs";
            double t0 = std::max(37.0, S.now + 0.4);
            c.koLegs = { 1, 1 }; c.koTimes = { t0, t0 + 1.2 }; c.koNames = { "Demi-finales", "Finale" };
            c.addKOStage({ { C.result[0], C.result[3] }, { C.result[1], C.result[2] } }, 1, t0, "Demi-finales");
            for (auto& m : c.matches) m.decisive = 1;
            addCompPublic(S, std::move(c));
            S.news.push_back("Arkema Première Ligue : " + g_world.teams[C.result[0]].name + " termine en tête de la saison régulière. Play-offs : "
                             + g_world.teams[C.result[0]].shortName + " - " + g_world.teams[C.result[3]].shortName + " et " + g_world.teams[C.result[1]].shortName + " - " + g_world.teams[C.result[2]].shortName + ".");
        }
    }
    if (C.kind == 40 && C.winner >= 0 && C.tag >= 0 && C.tag < (int)S.comps.size()) {
        // le vainqueur des play-offs est sacré champion : il passe en tête du classement final
        Competition& L = S.comps[C.tag];
        int fin = -1;
        if (!C.stages.empty() && C.stages.back().ties.size() == 1) { const Tie& t = C.stages.back().ties[0]; fin = t.a == C.winner ? t.b : t.a; }
        std::vector<int> r = L.result;
        r.erase(std::remove(r.begin(), r.end(), C.winner), r.end());
        if (fin >= 0) r.erase(std::remove(r.begin(), r.end(), fin), r.end());
        std::vector<int> nr = { C.winner }; if (fin >= 0) nr.push_back(fin);
        for (int t : r) nr.push_back(t);
        L.result = nr; L.winner = C.winner;
        S.news.push_back(g_world.teams[C.winner].name + " est sacré champion de France (Arkema Première Ligue) !");
    }
    if (C.kind == 41 && C.winner >= 0) S.news.push_back(g_world.teams[C.winner].name + " remporte la Coupe de France féminine !");
    if (C.kind == 42 && C.winner >= 0) S.news.push_back(g_world.teams[C.winner].name + " remporte la Coupe LFFP !");
    if (C.kind == 43 && C.winner >= 0) S.news.push_back(g_world.teams[C.winner].name + " remporte la Ligue des champions féminine !");
}

// UWCL : enchaînement des phases (appelé par Competition::onStageFinished, format FMT_EUROPE, kind 43)
bool uwclStageFinished(Competition& C, const std::vector<int>& winners) {
    if (C.kind != 43) return false;
    Stage& st = C.stages[C.cur];
    auto T = [&](int i) { return i < (int)C.koTimes.size() ? C.koTimes[i] : 30.0 + 3 * i; };
    if (st.type == ST_SWISS) {
        auto tb = C.swissTable(C.cur);
        C.result.clear(); for (auto& s : tb) C.result.push_back(s.team);
        if (tb.size() < 12) { C.done = true; C.winner = tb.empty() ? -1 : tb[0].team; return true; }
        C.extra.clear(); for (int i = 0; i < 4; i++) C.extra.push_back(tb[i].team);
        std::vector<std::pair<int, int>> pairs;
        for (int i = 0; i < 4; i++) pairs.push_back({ tb[11 - i].team, tb[4 + i].team });      // 5e-12e, 6e-11e... le mieux classé reçoit au retour
        C.cur = (int)C.stages.size();
        C.addKOStage(pairs, 2, T(0), "Barrages");
        return true;
    }
    if (st.name == "Barrages" && C.extra.size() == 4 && winners.size() == 4) {
        std::vector<std::pair<int, int>> pairs;
        for (int i = 0; i < 4; i++) pairs.push_back({ winners[3 - i], C.extra[i] });            // 1er contre le vainqueur 8e-9e, etc.
        C.cur = (int)C.stages.size();
        C.addKOStage(pairs, 2, T(1), "Quarts de finale");
        return true;
    }
    if (winners.size() == 1) { C.done = true; C.winner = winners[0]; return true; }
    bool fin = winners.size() == 2;
    std::vector<std::pair<int, int>> pairs;
    for (size_t i = 0; i + 1 < winners.size(); i += 2) pairs.push_back({ winners[i], winners[i + 1] });
    C.cur = (int)C.stages.size();
    C.addKOStage(pairs, fin ? 1 : 2, fin ? T(3) : T(2), fin ? std::string("Finale") : std::string("Demi-finales"), fin);
    return true;
}

// JFL (joueuses formées localement) : quota minimal sur la feuille de match des clubs de l'Arkema Première Ligue et de la Seconde Ligue
int jflMin(int team) {
    if (!isWomenTeam(team)) return 0;
    const Team& t = g_world.teams[team];
    if (t.kind != TK_CLUB || t.nation < 0 || strcmp(NATIONS[t.nation].code, "FRA")) return 0;
    return t.status == CS_AMATEUR ? 0 : 10;
}
bool isJfl(const Player& p, int team) {
    const Team& t = g_world.teams[team];
    return p.nation < 0 || p.nation == t.nation;
}
