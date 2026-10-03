// Coupe des Continents : chaque confédération envoie une sélection de ses meilleurs joueurs.
// Un groupe unique de 6 (matchs aller-retour), puis un Final Four (demi-finales et finale sur terrain neutre).
#include "game.h"
#include "data.h"
#include <algorithm>
#include <map>

static const char* CONT_NAMES[NUM_CONFEDS] = { "Sélection d'Europe", "Sélection d'Amérique du Sud", "Sélection CONCACAF", "Sélection d'Afrique", "Sélection d'Asie", "Sélection d'Océanie" };
static const char* CONT_SHORT[NUM_CONFEDS] = { "EUR", "AMS", "CCF", "AFR", "ASI", "OCE" };
// maillots : domicile / extérieur de chaque continent
static const unsigned CONT_KIT[NUM_CONFEDS][4] = { { 0x1A3A8A, 0xF2C230, 0x1A3A8A, 0xF2C230 }, { 0xF5D10A, 0x2A7DE1, 0x1F4FA8, 0xFFFFFF },
                                                   { 0xD62828, 0xFFFFFF, 0xFFFFFF, 0xD62828 }, { 0x1E8C3A, 0xF2C230, 0x111111, 0x1E8C3A },
                                                   { 0xFFFFFF, 0xC8102E, 0xC8102E, 0xFFFFFF }, { 0x111111, 0xFFFFFF, 0x111111, 0x7FD4F2 } };
static const int CONT_CODE = 9;      // valeur de Team::youth pour une sélection continentale

int continentTeam(int conf) {
    if (conf < 0 || conf >= NUM_CONFEDS) return -1;
    for (int i = NUM_NATIONS; i < (int)g_world.teams.size(); i++) {
        const Team& t = g_world.teams[i];
        if (t.kind == TK_NATION && t.youth == CONT_CODE && t.name == CONT_NAMES[conf]) return i;
    }
    // nation la plus forte de la confédération : stade et culture de jeu
    int lead = -1;
    for (int n = 0; n < NUM_NATIONS; n++) if (NATIONS[n].conf == conf && nationEligible(n) && (lead < 0 || g_world.teams[n].rating > g_world.teams[lead].rating)) lead = n;
    if (lead < 0) return -1;
    // les meilleurs joueurs de chaque poste parmi toutes les sélections de la confédération
    std::vector<Player> pool;
    for (int n = 0; n < NUM_NATIONS; n++) {
        if (NATIONS[n].conf != conf || !nationEligible(n)) continue;
        g_world.ensureSquad(n);
        for (const Player& p : g_world.teams[n].squad) pool.push_back(p);
    }
    std::stable_sort(pool.begin(), pool.end(), [](const Player& a, const Player& b) { return a.overall() > b.overall(); });
    static const int QUOTA[4] = { 3, 8, 7, 5 };
    int got[4] = { 0, 0, 0, 0 };
    std::map<int, int> perNation;          // au plus 6 joueurs d'une même nation : une vraie sélection continentale
    Team t;
    for (const Player& p0 : pool) {
        int ps = std::max(0, std::min(3, (int)p0.pos));
        if (got[ps] >= QUOTA[ps] || perNation[p0.nation] >= 6) continue;
        Player p = p0; p.suspended = 0; p.injured = 0; p.yellows = 0; p.goals = 0; p.apps = 0; p.assists = 0;
        t.squad.push_back(p); got[ps]++; perNation[p0.nation]++;
    }
    for (const Player& p0 : pool) {          // complément si une confédération manque de joueurs
        if ((int)t.squad.size() >= 23) break;
        bool in = false; for (auto& q : t.squad) if (q.id == p0.id && q.name == p0.name) in = true;
        if (!in) { Player p = p0; p.suspended = 0; p.injured = 0; p.goals = 0; p.apps = 0; t.squad.push_back(p); }
    }
    std::stable_sort(t.squad.begin(), t.squad.end(), [](const Player& a, const Player& b) { return a.pos != b.pos ? a.pos < b.pos : a.overall() > b.overall(); });
    for (int k = 0; k < (int)t.squad.size(); k++) t.squad[k].num = (uint8_t)(k + 1);
    const Team& L = g_world.teams[lead];
    t.name = CONT_NAMES[conf]; t.shortName = CONT_SHORT[conf];
    t.kind = TK_NATION; t.nation = lead; t.youth = CONT_CODE;
    float sum = 0; int n16 = std::min(16, (int)t.squad.size());
    std::vector<int> ov; for (auto& p : t.squad) ov.push_back(p.overall());
    std::sort(ov.rbegin(), ov.rend());
    for (int k = 0; k < n16; k++) sum += ov[k];
    t.rating = n16 ? std::min(99.f, sum / n16 + 2.f) : L.rating;
    t.home.shirt = CONT_KIT[conf][0]; t.home.shirt2 = CONT_KIT[conf][1]; t.home.shorts = CONT_KIT[conf][0]; t.home.socks = CONT_KIT[conf][1];
    t.away.shirt = CONT_KIT[conf][2]; t.away.shirt2 = CONT_KIT[conf][3]; t.away.shorts = CONT_KIT[conf][2]; t.away.socks = CONT_KIT[conf][3];
    t.culture = L.culture; t.stadium = L.stadium; t.town = L.town; t.formation = L.formation;
    t.seed = 0xC0A71E00u ^ (uint32_t)(conf * 7919);
    t.squadGen = true;
    g_world.teams.push_back(t);
    return (int)g_world.teams.size() - 1;
}

bool isContinentTeam(int team) { return team >= 0 && team < (int)g_world.teams.size() && g_world.teams[team].kind == TK_NATION && g_world.teams[team].youth == CONT_CODE; }

std::vector<int> continentTeams() {
    std::vector<int> v;
    for (int c = 0; c < NUM_CONFEDS; c++) { int t = continentTeam(c); if (t >= 0) v.push_back(t); }
    return v;
}

void continentsStart(Career& K, const std::vector<int>& ctrl) {
    std::vector<int> teams = continentTeams();
    CustomCompDef def; def.name = fmt("Coupe des Continents %d", 2026); def.format = 2; def.legs = 2; def.teams = teams; def.groups = 1;
    K.newCustom(def, ctrl);
    K.custom.format = CUSTOM_CONTINENTS;
    K.season.comps.clear();
    Competition c;
    c.name = def.name; c.shortName = "Coupe des Continents";
    c.format = FMT_TOURNAMENT; c.kind = KIND_CONTINENTS; c.tb = TB_GD; c.legs = 2;
    c.groupsAdvance = 4; c.bestThirds = 0; c.thirdPlace = false; c.neutralFinal = true; c.host = -1;
    std::vector<int> g = teams; g_rng.shuffle(g);
    c.addGroupStage({ g }, 2, { 1, 3, 5, 7, 9, 11, 13, 15, 17, 19 }, "Phase de groupes");
    c.koTimes = { 23, 26 };
    c.koNames = { "Final Four - demi-finales", "Final Four - finale" };
    addCompPublic(K.season, std::move(c));
}
