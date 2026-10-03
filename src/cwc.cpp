// Coupe du monde des clubs de la FIFA (format 2025 : 32 clubs, tous les 4 ans, en juin-juillet)
// UEFA 12 (vainqueurs de la Ligue des champions des 4 dernières saisons + classement des clubs sur 4 saisons),
// CONMEBOL 6, AFC 4, CAF 4, CONCACAF 4, OFC 1, pays hôte 1 ; 2 clubs maximum par pays (sauf vainqueurs continentaux).
// 8 groupes de 4 (les deux premiers en 8es), puis élimination directe en matchs secs, finale sans match pour la 3e place.
#include "game.h"
#include <algorithm>
#include <functional>
#include <map>
#include <set>

int addCompPublic(Season& S, Competition c);

bool cwcYear(int seasonYear) { int y = seasonYear + 1; return y >= 2029 && (y - 2029) % 4 == 0; }

static int confOf(int team) { int n = g_world.teams[team].nation; return n >= 0 ? NATIONS[n].conf : -1; }

// clubs des confédérations absentes de la base (créés à la première Coupe du monde des clubs)
struct ExtraClub { const char* name; const char* sh; const char* code; int rating; unsigned shirt, shorts, socks; int pattern; unsigned shirt2; const char* town; };
static const ExtraClub EXTRA[] = {
    { "Al Ahly SC", "Al Ahly", "EGY", 67, 0xC8102E, 0xFFFFFF, 0xC8102E, 0, 0, "Le Caire" },
    { "Pyramids FC", "Pyramids", "EGY", 64, 0x1B2A5B, 0x1B2A5B, 0x1B2A5B, 0, 0, "Le Caire" },
    { "Zamalek SC", "Zamalek", "EGY", 62, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0, 0, "Gizeh" },
    { "Mamelodi Sundowns", "Sundowns", "RSA", 65, 0xFFD700, 0x006B3F, 0xFFD700, 0, 0, "Pretoria" },
    { "Orlando Pirates", "Pirates", "RSA", 61, 0x111111, 0x111111, 0x111111, 0, 0, "Johannesburg" },
    { "Wydad AC", "Wydad", "MAR", 63, 0xD01020, 0xD01020, 0xD01020, 0, 0, "Casablanca" },
    { "Raja Casablanca", "Raja", "MAR", 62, 0x00843D, 0xFFFFFF, 0x00843D, 0, 0, "Casablanca" },
    { "RS Berkane", "Berkane", "MAR", 62, 0xF47920, 0x111111, 0xF47920, 0, 0, "Berkane" },
    { "Espérance de Tunis", "Espérance", "TUN", 63, 0xD01020, 0x111111, 0xD01020, 1, 0xF4C300, "Tunis" },
    { "TP Mazembe", "Mazembe", "COD", 60, 0x111111, 0x111111, 0x111111, 1, 0xFFFFFF, "Lubumbashi" },
    { "Club América", "América", "MEX", 68, 0xFFE000, 0x0B1F4B, 0xFFE000, 0, 0, "Mexico" },
    { "CF Monterrey", "Monterrey", "MEX", 67, 0x0B1F4B, 0x0B1F4B, 0x0B1F4B, 1, 0xFFFFFF, "Monterrey" },
    { "Tigres UANL", "Tigres", "MEX", 67, 0xF7B500, 0x0033A0, 0xF7B500, 0, 0, "Monterrey" },
    { "Cruz Azul", "Cruz Azul", "MEX", 66, 0x0047AB, 0x0047AB, 0x0047AB, 0, 0, "Mexico" },
    { "CF Pachuca", "Pachuca", "MEX", 64, 0x0B2A6B, 0x0B2A6B, 0x0B2A6B, 1, 0xFFFFFF, "Pachuca" },
    { "Auckland City FC", "Auckland", "NZL", 50, 0x0B2A6B, 0x0B2A6B, 0x0B2A6B, 0, 0, "Auckland" },
    { "Urawa Red Diamonds", "Urawa", "JPN", 63, 0xD01020, 0xFFFFFF, 0x111111, 0, 0, "Saitama" },
    { "Kashima Antlers", "Kashima", "JPN", 63, 0x8B0A1E, 0x8B0A1E, 0x8B0A1E, 0, 0, "Kashima" },
    { "Vissel Kobe", "Kobe", "JPN", 63, 0x7A0019, 0x7A0019, 0x7A0019, 0, 0, "Kobe" },
    { "Al Sadd SC", "Al Sadd", "QAT", 63, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0, 0, "Doha" },
    { "Al-Duhail SC", "Al-Duhail", "QAT", 61, 0x7A0019, 0x7A0019, 0x7A0019, 0, 0, "Doha" },
};
static void ensureExtraClubs() {
    for (auto& e : EXTRA) {
        bool have = false;
        for (auto& t : g_world.teams) if (t.kind == TK_CLUB && t.name == e.name) { have = true; break; }
        if (have) continue;
        int n = g_world.nationIndex(e.code);
        if (n < 0) continue;
        Team t;
        t.name = e.name; t.shortName = e.sh; t.kind = TK_CLUB; t.nation = n; t.rating = (float)e.rating;
        t.home.shirt = e.shirt; t.home.shorts = e.shorts; t.home.socks = e.socks; t.home.pattern = e.pattern; t.home.shirt2 = e.shirt2;
        t.away.shirt = 0xFFFFFF; t.away.shorts = 0xFFFFFF; t.away.socks = 0xFFFFFF;
        if (e.shirt == 0xFFFFFF) { t.away.shirt = 0x111111; t.away.shorts = 0x111111; t.away.socks = 0x111111; }
        t.culture = NATIONS[n].culture; t.town = e.town; t.stadium = std::string("Stade de ") + e.town;
        t.status = CS_PRO; t.squadGen = false;
        t.seed = 0; for (const char* c = e.name; *c; c++) t.seed = t.seed * 131 + (unsigned char)*c;
        g_world.teams.push_back(t);
    }
}

int cwcCreate(Career& K) {
    ensureExtraClubs();
    Season& S = K.season;
    int y = K.year + 1;
    for (auto& c : S.comps) if (c.kind == 28) return -1;
    std::vector<int> teams;
    std::map<int, int> perNation;
    std::set<int> in;
    auto nat = [](int t) { return g_world.teams[t].nation; };
    auto add = [&](int t, bool force) {
        if (t < 0 || in.count(t) || g_world.teams[t].parent >= 0 || g_world.teams[t].kind != TK_CLUB) return false;
        if (!force && perNation[nat(t)] >= 2) return false;
        teams.push_back(t); in.insert(t); perNation[nat(t)]++; return true;
    };
    // ---- UEFA : vainqueurs de la C1 des quatre dernières saisons
    std::vector<int> winners;
    for (auto& h : K.honourLog) if (h.comp == HC_UCL && h.year >= y - 3 && h.year <= y && h.winner >= 0) winners.push_back(h.winner);
    if (K.ucl >= 0 && K.ucl < (int)S.comps.size() && S.comps[K.ucl].winner >= 0) winners.push_back(S.comps[K.ucl].winner);
    int nU = 0;
    for (int w : winners) if (add(w, true)) nU++;
    // puis classement des clubs sur quatre saisons
    std::vector<std::pair<float, int>> rk;
    for (int t = 0; t < (int)g_world.teams.size(); t++) {
        const Team& T = g_world.teams[t];
        if (T.kind != TK_CLUB || T.parent >= 0 || T.youth || confOf(t) != UEFA) continue;
        float c = T.coefs[1] + T.coefs[2] + T.coefs[3] + T.coefs[4];
        if (c > 0) rk.push_back({ -c, t });
    }
    std::sort(rk.begin(), rk.end());
    for (auto& x : rk) { if (nU >= 12) break; if (add(x.second, false)) nU++; }
    // ---- autres confédérations : meilleurs clubs (vainqueurs continentaux simulés par le niveau)
    auto confPick = [&](int conf, int n) {
        std::vector<std::pair<float, int>> v;
        for (int t = 0; t < (int)g_world.teams.size(); t++) {
            const Team& T = g_world.teams[t];
            if (T.kind != TK_CLUB || T.parent >= 0 || T.custom || T.youth || confOf(t) != conf) continue;
            v.push_back({ -(T.rating + g_rng.frange(-3, 3)), t });
        }
        std::sort(v.begin(), v.end());
        int k = 0;
        int cw = continentalWinner(K, conf);          // vainqueur de la coupe continentale : qualifié d'office
        if (cw >= 0 && k < n && add(cw, true)) k++;
        for (auto& x : v) { if (k >= n) break; if (add(x.second, false)) k++; }
        return k;
    };
    int missing = 0;
    missing += 6 - confPick(CONMEBOL, 6);
    missing += 4 - confPick(AFC, 4);
    missing += 4 - confPick(CAF, 4);
    missing += 4 - confPick(CONCACAF, 4);
    missing += 1 - confPick(OFC, 1);
    // ---- pays hôte (désigné par la FIFA)
    static const char* HOSTS[] = { "BRA", "ESP", "MAR", "KSA", "USA", "JPN", "MEX", "QAT" };
    Rng hr((uint64_t)y * 7717u);
    int hostNation = -1;
    for (int tries = 0; tries < 8 && hostNation < 0; tries++) { int n = g_world.nationIndex(HOSTS[hr.range(0, 7)]); if (n >= 0) hostNation = n; }
    {
        std::vector<std::pair<float, int>> v;
        for (int t = 0; t < (int)g_world.teams.size(); t++) if (g_world.teams[t].kind == TK_CLUB && g_world.teams[t].parent < 0 && nat(t) == hostNation) v.push_back({ -g_world.teams[t].rating, t });
        std::sort(v.begin(), v.end());
        bool ok = false; for (auto& x : v) if (add(x.second, true)) { ok = true; break; }
        if (!ok) missing++;
    }
    // compléter (clubs européens suivants)
    for (auto& x : rk) { if ((int)teams.size() >= 32) break; add(x.second, false); }
    for (auto& x : rk) { if ((int)teams.size() >= 32) break; add(x.second, true); }
    if (teams.size() < 32) return -1;
    teams.resize(32);
    (void)missing;
    // ---- tirage : chapeaux par niveau ; pas deux clubs d'une même confédération dans un groupe (UEFA : 2 au plus), ni d'un même pays
    std::stable_sort(teams.begin(), teams.end(), [](int a, int b) { return g_world.teams[a].rating > g_world.teams[b].rating; });
    std::vector<std::vector<int>> g(8);
    std::vector<int> slot(32, -1);
    std::vector<char> used(32, 0);
    long budget = 300000;
    std::function<bool(int)> place = [&](int s) -> bool {
        if (s == 32) return true;
        if (--budget < 0) return false;
        int pot = s / 8, grp = s % 8;
        std::vector<int> cand; for (int i = pot * 8; i < pot * 8 + 8; i++) if (!used[i]) cand.push_back(i);
        g_rng.shuffle(cand);
        for (int ci : cand) {
            int t = teams[ci], cf = confOf(t), nu = 0; bool ok = true;
            for (int k = 0; k < pot; k++) { int o = slot[k * 8 + grp]; if (o < 0) continue; if (nat(o) == nat(t)) ok = false; if (confOf(o) == cf) nu++; }
            if (!ok || (cf == UEFA ? nu >= 2 : nu >= 1)) continue;
            used[ci] = 1; slot[s] = t;
            if (place(s + 1)) return true;
            used[ci] = 0; slot[s] = -1;
        }
        return false;
    };
    if (place(0)) for (int s = 0; s < 32; s++) g[s % 8].push_back(slot[s]);
    else for (int i = 0; i < 32; i++) g[i % 8].push_back(teams[i]);
    Competition c;
    c.format = FMT_TOURNAMENT; c.kind = 28; c.tb = TB_FIFA; c.legs = 1;
    c.name = fmt("Coupe du monde des clubs de la FIFA %d", y);
    c.shortName = "Mondial des clubs";
    c.host = -1; c.tag = hostNation;
    c.groupsAdvance = 2; c.bestThirds = 0; c.thirdPlace = false;
    double t0 = std::max(S.now + 1.0, 45.5);
    c.addGroupStage(g, 1, { t0, t0 + 0.7, t0 + 1.4 }, "Phase de groupes");
    c.koTimes = { t0 + 2.3, t0 + 3.1, t0 + 3.9, t0 + 4.7 };
    int idx = addCompPublic(S, std::move(c));
    std::string hn = hostNation >= 0 ? g_world.teams[hostNation].name : std::string("?");
    S.news.push_back(fmt("Coupe du monde des clubs %d : 32 clubs, organisation : %s (juin-juillet).", y, hn.c_str()));
    for (int t : teams) {
        const Team& T = g_world.teams[t];
        if (S.isControlled(t)) S.news.push_back("[Votre club] " + T.name + " est qualifié pour la Coupe du monde des clubs !");
        else if (T.nation >= 0 && std::string(NATIONS[T.nation].code) == "FRA") S.news.push_back(T.name + " représentera la France à la Coupe du monde des clubs.");
    }
    return idx;
}
