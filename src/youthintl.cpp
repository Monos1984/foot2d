// Sélections de jeunes (Espoirs U21, U19, U17, olympique U23) et leurs compétitions :
// Euro Espoirs (qualifications en groupes + barrages, phase finale à 16), Euro U19 et U17 (tour de qualification, tour Élite,
// phase finale à 8), tournoi olympique (12 équipes, qualifications continentales : Euro Espoirs, CAN U23, Coupe d'Asie U23,
// tournoi préolympique sud-américain, championnat CONCACAF U23, Océanie).
#include "game.h"
#include <algorithm>
#include <map>
#include <set>
#include <cstring>

int addCompPublic(Season& S, Competition c);
std::vector<int>& intlQualifiedRef();
std::vector<int>& intlHostsRef();
int& intlFinalTeamsRef();

static const int QK_FINAL_Y = 100;
enum { QY_U21_G = 120, QY_U21_PO, QY_Y_Q1, QY_Y_ELITE, QY_OLY_UEFA, QY_OLY_CAF, QY_OLY_AFC, QY_OLY_CSA, QY_OLY_CCF, QY_OLY_OFC };

int youthCatOfType(int type) {
    switch (type) { case IT_EURO21: return 1; case IT_EURO19: return 2; case IT_EURO17: return 3; case IT_OLYMPICS: return 4;
                    case IT_OLY_W: case IT_EURO_W: case IT_WC_W: return 5; default: return 0; }
}
int nationOfTeam(int team) { return team >= 0 && team < (int)g_world.teams.size() ? g_world.teams[team].nation : -1; }

static const int YCODE[6] = { 0, 4, 1, 2, 5, 6 };        // valeur de Team::youth pour chaque catégorie (6 : sélection féminine)
static const int YOFF[6] = { 0, 7, 12, 16, 5, 0 };       // écart de niveau avec l'équipe A
static const char* YSUF[6] = { "", " Espoirs", " U19", " U17", " olympique", " (F)" };
static const char* YSH[6] = { "", "-21", "-19", "-17", "-23", "-F" };
// niveau des sélections féminines (hiérarchie mondiale) ; les autres : d'après la sélection masculine
static float womenRating(int nation) {
    static const struct { const char* c; float r; } W[] = {
        { "ESP", 88 }, { "USA", 88 }, { "ENG", 87 }, { "GER", 86 }, { "FRA", 85 }, { "SWE", 85 }, { "NED", 83 }, { "JPN", 83 }, { "CAN", 82 }, { "BRA", 82 },
        { "PRK", 80 }, { "AUS", 80 }, { "DEN", 78 }, { "NOR", 78 }, { "ITA", 78 }, { "ISL", 76 }, { "CHN", 76 }, { "AUT", 76 }, { "KOR", 75 }, { "SUI", 75 },
        { "BEL", 75 }, { "POR", 74 }, { "COL", 74 }, { "SCO", 73 }, { "IRL", 73 }, { "POL", 72 }, { "FIN", 72 }, { "NZL", 71 }, { "NGA", 72 }, { "MEX", 71 },
        { "ARG", 70 }, { "WAL", 70 }, { "CZE", 70 }, { "UKR", 68 }, { "RSA", 69 }, { "ZAM", 68 }, { "MAR", 69 }, { "CRC", 67 }, { "JAM", 68 }, { "VIE", 64 } };
    for (auto& w : W) if (strcmp(NATIONS[nation].code, w.c) == 0) return w.r;
    return std::max(25.f, std::min(66.f, NATIONS[nation].rating * 0.8f));
}

int youthNationTeam(int nation, int cat) {
    if (nation < 0 || nation >= NUM_NATIONS) return -1;
    if (cat <= 0 || cat > 5) return nation;
    static std::map<int, int> cache;
    int key = nation * 8 + cat;
    auto it = cache.find(key);
    if (it != cache.end() && it->second < (int)g_world.teams.size()) {
        const Team& t = g_world.teams[it->second];
        if (t.kind == TK_NATION && t.nation == nation && t.youth == YCODE[cat]) return it->second;
    }
    for (int i = NUM_NATIONS; i < (int)g_world.teams.size(); i++) {
        const Team& t = g_world.teams[i];
        if (t.kind == TK_NATION && t.nation == nation && t.youth == YCODE[cat]) { cache[key] = i; return i; }
    }
    const Team& A = g_world.teams[nation];
    Team t;
    t.name = A.name + YSUF[cat]; t.shortName = std::string(NATIONS[nation].code) + YSH[cat];
    t.kind = TK_NATION; t.nation = nation; t.youth = YCODE[cat];
    t.rating = cat == 5 ? womenRating(nation) : std::max(15.f, A.rating - YOFF[cat]);
    t.home = A.home; t.away = A.away; t.culture = A.culture; t.stadium = A.stadium; t.town = A.town;
    t.seed = 0x9E3779B9u ^ (uint32_t)(nation * 131 + cat * 7919);
    t.squadGen = false; t.formation = A.formation;
    g_world.teams.push_back(t);
    cache[key] = (int)g_world.teams.size() - 1;
    return cache[key];
}

static std::vector<int> natsOf(int conf) {
    std::vector<int> v;
    for (int i = 0; i < NUM_NATIONS; i++) if (nationEligible(i) && (conf < 0 || NATIONS[i].conf == conf)) v.push_back(i);
    return v;
}
static void byRating(std::vector<int>& v) { std::stable_sort(v.begin(), v.end(), [](int a, int b) { return g_world.teams[a].rating > g_world.teams[b].rating; }); }

static std::vector<std::vector<int>> pots(std::vector<int> teams, int ng, int first = -1) {
    byRating(teams);
    std::vector<std::vector<int>> g(ng);
    if (first >= 0) { teams.erase(std::remove(teams.begin(), teams.end(), first), teams.end()); g[0].push_back(first); }
    size_t idx = 0;
    while (idx < teams.size()) {
        std::vector<int> pot;
        for (int i = 0; i < ng && idx < teams.size(); i++) pot.push_back(teams[idx++]);
        g_rng.shuffle(pot);
        std::vector<int> ord(ng); for (int i = 0; i < ng; i++) ord[i] = i;
        std::stable_sort(ord.begin(), ord.end(), [&](int a, int b) { return g[a].size() < g[b].size(); });
        for (size_t i = 0; i < pot.size(); i++) g[ord[i]].push_back(pot[i]);
    }
    return g;
}

static int groupComp(Season& S, const std::string& name, const std::vector<std::vector<int>>& groups, int legs, std::vector<double> times, int kind) {
    Competition c;
    c.format = FMT_QUAL_GROUPS; c.name = name; c.shortName = name; c.kind = kind; c.legs = legs; c.tb = TB_H2H;
    int maxR = 0;
    for (auto& g : groups) maxR = std::max(maxR, (int)g.size() - (g.size() % 2 == 0 ? 1 : 0));
    maxR *= legs;
    while ((int)times.size() < maxR) times.push_back(times.back() + 0.4);
    times.resize(std::max(1, maxR));
    c.addGroupStage(groups, legs, times, "Groupes");
    return addCompPublic(S, std::move(c));
}

static int tourComp(Season& S, const std::string& name, const std::vector<int>& teams, int ng, int bestThirds, bool third, double t0, int kind) {
    Competition c;
    c.format = FMT_TOURNAMENT; c.kind = kind; c.name = name; c.shortName = name; c.tb = TB_H2H; c.legs = 1;
    c.groupsAdvance = 2; c.bestThirds = bestThirds; c.thirdPlace = third; c.host = -1;
    c.addGroupStage(pots(teams, ng), 1, { t0, t0 + 0.5, t0 + 1.0 }, "Phase de groupes");
    for (int i = 0; i < 5; i++) c.koTimes.push_back(t0 + 1.8 + 0.6 * i);
    return addCompPublic(S, std::move(c));
}

// classement final d'un tournoi : vainqueur, finaliste, vainqueur du match pour la 3e place
static std::vector<int> podium(const Competition& C) {
    std::vector<int> v;
    if (C.winner >= 0) v.push_back(C.winner);
    for (int s = (int)C.stages.size() - 1; s >= 0; s--) {
        const Stage& st = C.stages[s];
        if (st.type != ST_KO || st.ties.size() != 1) continue;
        const Tie& t = st.ties[0];
        if (st.name == "Finale") { int o = t.winner == t.a ? t.b : t.a; if (o >= 0 && std::find(v.begin(), v.end(), o) == v.end()) v.push_back(o); }
    }
    for (auto& st : C.stages) if (st.name == "Match pour la 3e place" && !st.ties.empty() && st.ties[0].winner >= 0) v.push_back(st.ties[0].winner);
    return v;
}

static std::vector<std::vector<Standing>> positions(const Competition& C) {
    std::vector<std::vector<Standing>> pos;
    for (int g = 0; g < (int)C.stages[0].groups.size(); g++) {
        auto tb = C.table(0, g);
        for (int i = 0; i < (int)tb.size(); i++) { if ((int)pos.size() <= i) pos.resize(i + 1); pos[i].push_back(tb[i]); }
    }
    for (auto& p : pos) std::stable_sort(p.begin(), p.end(), [](const Standing& a, const Standing& b) {
        float pa = a.p ? (float)a.pts / a.p : 0, pb = b.p ? (float)b.pts / b.p : 0;
        if (pa != pb) return pa > pb;
        if (a.gd() != b.gd()) return a.gd() > b.gd();
        return a.gf > b.gf; });
    return pos;
}

static double finalsWeekY(const Career& K) { return (K.year - K.season.year - 1) * 52.0 + 45.0 + (K.intlType == IT_OLYMPICS ? 4.0 : 0.0); }

static void createYouthFinals(Career& K) {
    Season& S = K.season;
    int cat = youthCatOfType(K.intlType);
    auto& Q = intlQualifiedRef();
    auto& H = intlHostsRef();
    int N = intlFinalTeamsRef();
    std::vector<int> teams;
    for (int h : H) { int t = youthNationTeam(h, cat); if (t >= 0) teams.push_back(t); }
    for (int t : Q) if (std::find(teams.begin(), teams.end(), t) == teams.end()) teams.push_back(t);
    bool uefaOnly = K.intlType == IT_EURO21 || K.intlType == IT_EURO19 || K.intlType == IT_EURO17 || K.intlType == IT_EURO_W;
    if ((int)teams.size() < N) {
        std::vector<int> pool = natsOf(uefaOnly ? UEFA : -1);
        byRating(pool);
        for (int n : pool) { if ((int)teams.size() >= N) break; int t = youthNationTeam(n, cat); if (std::find(teams.begin(), teams.end(), t) == teams.end()) teams.push_back(t); }
    }
    teams.resize(N);
    int ng = N / 4;
    int hostTeam = H.empty() ? -1 : youthNationTeam(H[0], cat);
    Competition c;
    c.format = FMT_TOURNAMENT; c.kind = QK_FINAL_Y; c.tb = TB_H2H; c.legs = 1;
    c.name = fmt("%s %d", INTL_NAMES[K.intlType], K.year); c.shortName = INTL_NAMES[K.intlType];
    c.host = hostTeam;
    bool oly = K.intlType == IT_OLYMPICS || K.intlType == IT_OLY_W;
    c.groupsAdvance = 2; c.bestThirds = K.intlType == IT_OLY_W ? 2 : 0;          // JO féminins : 3 groupes, les 2 meilleurs 3es en quarts
    c.thirdPlace = oly || K.intlType == IT_WC_W;
    double t0 = std::max(S.now + 3, finalsWeekY(K));
    double sp = oly ? 3.0 / 7.0 : 0.6;                                              // JO : un match tous les 3 jours
    auto g = pots(teams, ng, hostTeam);
    // deuxième pays hôte : tête de série d'un autre groupe
    if (H.size() > 1 && ng > 1) { int h2 = youthNationTeam(H[1], cat); for (auto& gr : g) { auto it = std::find(gr.begin(), gr.end(), h2); if (it != gr.end() && &gr != &g[1]) { std::swap(*it, g[1][0]); break; } } }
    c.addGroupStage(g, 1, { t0, t0 + sp, t0 + 2 * sp }, "Phase de groupes");
    for (int i = 0; i < 5; i++) c.koTimes.push_back(t0 + 3 * sp + (oly ? sp : 0.7) * i + (oly ? 0 : 0.2));
    K.finalComp = addCompPublic(S, std::move(c));
    if (oly) S.news.push_back(fmt("Jeux olympiques %d : le tournoi de football débute deux jours avant la cérémonie d'ouverture. Un match tous les trois jours : gérez la fatigue et faites tourner l'effectif !", K.year));
}

void youthNewIntl(Career& K, int type, bool withQual, const std::vector<int>& ctrl, int yr, const std::vector<int>& hosts) {
    K.kind = CK_INTL;
    K.resetV7();
    K.intlType = type; K.intlWithQual = withQual; K.intlFormat = 0;
    K.season = Season();
    Season& S = K.season;
    S.mode = 1;
    S.comps.reserve(400);
    K.history.clear();
    K.finalComp = -1;
    int cat = youthCatOfType(type);
    if (cat == 5) { withQual = false; K.intlWithQual = false; }        // compétitions féminines : phase finale directe
    auto& Q = intlQualifiedRef(); Q.clear();
    auto& H = intlHostsRef(); H = hosts.empty() ? defaultHosts(type) : hosts;
    H.erase(std::remove(H.begin(), H.end(), -1), H.end());
    K.intlHosts = H;
    K.year = yr > 0 ? yr : intlYear(type);
    intlFinalTeamsRef() = type == IT_OLYMPICS ? 16 : type == IT_OLY_W ? 12 : type == IT_WC_W ? 32 : type == IT_EURO21 || type == IT_EURO_W ? 16 : 8;
    int N = intlFinalTeamsRef();
    S.year = withQual && (type == IT_OLYMPICS || type == IT_EURO21) ? K.year - 2 : K.year - 1;
    S.controlled.clear();
    for (int n : ctrl) { int t = youthNationTeam(n < NUM_NATIONS ? n : nationOfTeam(n), cat); if (t >= 0) S.controlled.push_back(t); }
    auto isHost = [&](int n) { return std::find(H.begin(), H.end(), n) != H.end(); };
    auto Y = [&](int n) { return youthNationTeam(n, cat); };
    auto Yv = [&](const std::vector<int>& nats) { std::vector<int> v; for (int n : nats) if (!isHost(n)) v.push_back(Y(n)); return v; };
    if (!withQual) {
        std::vector<std::pair<int, int>> quota;
        if (type == IT_OLYMPICS) quota = { { UEFA, 3 }, { CAF, 3 }, { AFC, 3 }, { CONMEBOL, 2 }, { CONCACAF, 2 }, { OFC, 1 } };
        else if (type == IT_OLY_W) quota = { { UEFA, 3 }, { CAF, 2 }, { AFC, 2 }, { CONMEBOL, 2 }, { CONCACAF, 1 }, { OFC, 1 } };
        else if (type == IT_WC_W) quota = { { UEFA, 11 }, { AFC, 6 }, { CAF, 4 }, { CONCACAF, 4 }, { CONMEBOL, 3 }, { OFC, 1 } };
        else quota = { { UEFA, N - (int)H.size() } };
        for (auto& q : quota) {
            auto v = Yv(natsOf(q.first));
            std::vector<std::pair<float, int>> sc;
            for (int t : v) sc.push_back({ g_world.teams[t].rating + g_rng.frange(-4, 4), t });
            std::sort(sc.rbegin(), sc.rend());
            for (int i = 0; i < q.second && i < (int)sc.size(); i++) Q.push_back(sc[i].second);
        }
        for (int t : S.controlled) if (std::find(Q.begin(), Q.end(), t) == Q.end() && !isHost(nationOfTeam(t))) {
            // l'équipe choisie prend la place la plus faible de sa confédération
            int conf = NATIONS[nationOfTeam(t)].conf, worst = -1;
            for (int i = 0; i < (int)Q.size(); i++) if (NATIONS[nationOfTeam(Q[i])].conf == conf && (worst < 0 || g_world.teams[Q[i]].rating < g_world.teams[Q[worst]].rating)) worst = i;
            if (worst >= 0) Q[worst] = t; else Q.push_back(t);
        }
        createYouthFinals(K);
        return;
    }
    switch (type) {
    case IT_EURO21: {
        // 9 groupes aller-retour (septembre à octobre de l'année suivante)
        groupComp(S, "Qualifications de l'Euro Espoirs", pots(Yv(natsOf(UEFA)), 9), 2, { 5, 6, 9, 10, 14, 15, 33, 34, 40, 41, 57, 58, 61, 62 }, QY_U21_G);
        break;
    }
    case IT_EURO19: case IT_EURO17: {
        auto v = Yv(natsOf(UEFA));
        byRating(v);
        int nb = (int)v.size() - 52;                                     // meilleures nations exemptées du premier tour
        std::vector<int> bye;
        for (int i = 0; i < nb && !v.empty(); i++) { bye.push_back(v.front()); v.erase(v.begin()); }
        int ci = groupComp(S, type == IT_EURO19 ? "Euro U19 - tour de qualification" : "Euro U17 - tour de qualification", pots(v, (int)v.size() / 4), 1, { 10.0, 10.4, 10.8 }, QY_Y_Q1);
        S.comps[ci].extra2 = bye;
        break;
    }
    case IT_OLYMPICS: {
        // Europe : l'Euro Espoirs de l'année précédente (les trois premiers)
        if (K.coach && K.coachU21Podium.size() >= 3) {
            for (int i = 0; i < 3; i++) { int t = youthNationTeam(K.coachU21Podium[i], 4); if (t >= 0 && std::find(Q.begin(), Q.end(), t) == Q.end()) Q.push_back(t); }
            std::string s; for (int i = 0; i < 3; i++) s += (i ? ", " : "") + g_world.teams[K.coachU21Podium[i]].name;
            S.news.push_back(fmt("Europe : qualifiés pour le tournoi olympique grâce à l'Euro Espoirs %d : %s.", K.year - 1, s.c_str()));
        } else {
            std::vector<int> v; for (int n : natsOf(UEFA)) v.push_back(youthNationTeam(n, 1));
            byRating(v);
            std::vector<int> e;
            for (int n : { g_world.nationIndex("ALB"), g_world.nationIndex("SRB") }) if (n >= 0) e.push_back(youthNationTeam(n, 1));
            for (int t : v) { if (e.size() >= 16) break; if (std::find(e.begin(), e.end(), t) == e.end()) e.push_back(t); }
            for (int t : S.controlled) { int n = nationOfTeam(t); if (NATIONS[n].conf == UEFA) { int u = youthNationTeam(n, 1); if (std::find(e.begin(), e.end(), u) == e.end()) e.back() = u; } }
            tourComp(S, fmt("Euro Espoirs %d (qualificatif olympique)", K.year - 1), e, 4, 0, true, 45.0, QY_OLY_UEFA);
        }
        auto best = [&](int conf, int n) { auto v = Yv(natsOf(conf)); byRating(v); if ((int)v.size() > n) v.resize(n);
            for (int t : S.controlled) if (NATIONS[nationOfTeam(t)].conf == conf && std::find(v.begin(), v.end(), t) == v.end() && !v.empty()) v.back() = t;
            return v; };
        tourComp(S, "Coupe d'Afrique U23 (qualificatif olympique)", best(CAF, 8), 2, 0, true, 66.0, QY_OLY_CAF);
        tourComp(S, "Coupe d'Asie U23 (qualificatif olympique)", best(AFC, 16), 4, 0, true, 75.0, QY_OLY_AFC);
        groupComp(S, "Tournoi préolympique sud-américain", { Yv(natsOf(CONMEBOL)) }, 1, { 78.0, 78.3, 78.6, 78.9, 79.2, 79.5, 79.8, 80.1, 80.4 }, QY_OLY_CSA);
        tourComp(S, "Championnat CONCACAF U23 (qualificatif olympique)", best(CONCACAF, 8), 2, 0, false, 70.0, QY_OLY_CCF);
        groupComp(S, "Qualifications olympiques d'Océanie", { best(OFC, 4) }, 1, { 72.0, 72.4, 72.8 }, QY_OLY_OFC);
        break;
    }
    default: break;
    }
}

static void checkDone(Career& K) {
    for (auto& c : K.season.comps) if (c.kind >= QY_U21_G && c.kind <= QY_OLY_OFC && !c.done) return;
    if (K.finalComp < 0) createYouthFinals(K);
}

void youthQualify(Career& K, int comp) {
    Season& S = K.season;
    Competition& C = S.comps[comp];
    auto& Q = intlQualifiedRef();
    int cat = youthCatOfType(K.intlType);
    auto add = [&](int t) { if (t >= 0 && std::find(Q.begin(), Q.end(), t) == Q.end()) Q.push_back(t); };
    auto addNat = [&](int team) { add(youthNationTeam(nationOfTeam(team), cat)); };
    double tn = S.now + 1.0;
    switch (C.kind) {
    case QK_FINAL_Y:
        K.history.push_back(fmt("%d ", K.year) + C.name + " : " + (C.winner >= 0 ? g_world.teams[C.winner].name : "?"));
        return;
    case QY_U21_G: {
        auto pos = positions(C);
        for (auto& s : pos[0]) add(s.team);
        if (pos.size() > 1 && !pos[1].empty()) {
            add(pos[1][0].team);                                              // meilleur deuxième
            std::vector<int> po; for (size_t i = 1; i < pos[1].size(); i++) po.push_back(pos[1][i].team);
            if (po.size() >= 2) {
                Competition c;
                c.format = FMT_KO_ONLY; c.name = "Barrages de l'Euro Espoirs"; c.shortName = c.name; c.kind = QY_U21_PO; c.qualSpots = (int)po.size() / 2;
                c.koLegs = { 2 }; c.koTimes = { std::max(tn, 67.0) }; c.neutralFinal = false;
                std::vector<std::pair<int, int>> pr;
                int n = (int)po.size() / 2 * 2;
                for (int i = 0; i < n / 2; i++) pr.push_back({ po[n - 1 - i], po[i] });
                c.addKOStage(pr, 2, c.koTimes[0], "Barrages");
                addCompPublic(S, std::move(c));
                return;
            }
        }
        break;
    }
    case QY_U21_PO: for (int t : C.result) add(t); break;
    case QY_Y_Q1: {
        auto pos = positions(C);
        std::vector<int> el = C.extra2;
        for (auto& s : pos[0]) el.push_back(s.team);
        if (pos.size() > 1) for (auto& s : pos[1]) el.push_back(s.team);
        if (pos.size() > 2) for (auto& s : pos[2]) { if (el.size() % 4 == 0) break; el.push_back(s.team); }
        el.resize(el.size() / 4 * 4);
        int ci = groupComp(S, K.intlType == IT_EURO19 ? "Euro U19 - tour Élite" : "Euro U17 - tour Élite", pots(el, (int)el.size() / 4), 1, { std::max(tn, 32.0), std::max(tn, 32.0) + 0.4, std::max(tn, 32.0) + 0.8 }, QY_Y_ELITE);
        (void)ci;
        return;
    }
    case QY_Y_ELITE: { auto pos = positions(C); for (auto& s : pos[0]) add(s.team); break; }
    case QY_OLY_UEFA: { auto p = podium(C); for (int i = 0; i < 3 && i < (int)p.size(); i++) addNat(p[i]); break; }
    case QY_OLY_CAF: case QY_OLY_AFC: { auto p = podium(C); for (int i = 0; i < 3 && i < (int)p.size(); i++) addNat(p[i]); break; }
    case QY_OLY_CCF: { auto p = podium(C); for (int i = 0; i < 2 && i < (int)p.size(); i++) addNat(p[i]); break; }
    case QY_OLY_CSA: { auto tb = C.table(0, 0); for (int i = 0; i < 2 && i < (int)tb.size(); i++) addNat(tb[i].team); break; }
    case QY_OLY_OFC: { auto tb = C.table(0, 0); if (!tb.empty()) addNat(tb[0].team); break; }
    default: return;
    }
    checkDone(K);
}
