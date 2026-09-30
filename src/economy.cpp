// Économie : réputation des clubs, joueurs libres, contrats (années, salaires), frais d'arbitrage, capacités minimales
#include "game.h"
#include <cstring>
#include <cmath>

GateInfo g_lastGate;
const char* CONTRACT_NAMES[3] = { "Contrat professionnel", "Contrat fédéral", "Licence amateur" };

// ------------------------------------------------------------------ réputation
int teamReputation(int team) {
    if (team < 0 || team >= (int)g_world.teams.size()) return 1;
    Team& t = g_world.teams[team];
    if (t.reputation == 0) {
        int lv = teamLevel(team);
        float r = t.rating * 0.85f + (lv == 0 ? 14.f : lv == 1 ? 8.f : lv == 2 ? 4.f : 0.f) + (t.status == CS_PRO ? 4.f : 0.f);
        if (t.kind == TK_NATION) r = t.rating;
        if (t.parent >= 0) r *= 0.6f;
        t.reputation = (uint8_t)std::max(1, std::min(99, (int)r));
    }
    return t.reputation;
}
void addReputation(int team, int delta) {
    if (team < 0 || team >= (int)g_world.teams.size()) return;
    int r = teamReputation(team) + delta;
    g_world.teams[team].reputation = (uint8_t)std::max(1, std::min(99, r));
}
const char* reputationLabel(int rep) {
    return rep >= 85 ? "mondiale" : rep >= 72 ? "internationale" : rep >= 58 ? "nationale" : rep >= 40 ? "régionale" : rep >= 22 ? "départementale" : "locale";
}

// ------------------------------------------------------------------ joueurs libres
int freeAgentsTeam() {
    static int cached = -1;
    if (cached >= 0 && cached < (int)g_world.teams.size() && g_world.teams[cached].freeAgents) return cached;
    for (int i = (int)g_world.teams.size() - 1; i >= 0; i--) {
        Team& t = g_world.teams[i];
        if (t.freeAgents || (t.name == "Joueurs libres (sans club)" && t.nation < 0)) { t.freeAgents = true; cached = i; return i; }
    }
    Team t;
    t.name = "Joueurs libres (sans club)"; t.shortName = "Libres"; t.kind = TK_CLUB; t.nation = -1; t.rating = 45;
    t.freeAgents = true; t.squadGen = true; t.status = CS_AMATEUR; t.stadium = "-";
    t.seed = 0xF4EE;
    g_world.teams.push_back(t);
    cached = (int)g_world.teams.size() - 1;
    return cached;
}

int contractYears(const Player& p) {
    if (p.years) return p.years;
    return 1 + (int)((unsigned)p.id * 2654435761u >> 28) % 4;
}

// salaire demandé : barème du joueur, corrigé par l'écart de réputation (un grand club paie moins cher, un petit doit convaincre)
int wageDemand(const Player& p, int toTeam) {
    int base = p.wageK > 0 ? p.wageK : p.wage();
    int rep = teamReputation(toTeam);
    float k = 1.0f + std::max(-0.25f, std::min(0.6f, (p.overall() - rep) * 0.012f));
    return std::max(1, (int)(base * k));
}

// frais d'arbitrage payés par le club qui reçoit (€) : pris en charge par la LFP en pro, barèmes des ligues et districts
int refereeFee(int comp, int home) {
    const Season& S = g_career.season;
    int lv = teamLevel(home);
    static const int FEE[13] = { 0, 0, 0, 1800, 1100, 700, 450, 320, 220, 170, 140, 120, 100 };
    int f = FEE[std::max(0, std::min(12, lv))];
    if (comp >= 0 && comp < (int)S.comps.size()) {
        const Competition& C = S.comps[comp];
        if (C.kind == 2 && C.name.find("tours régionaux") == std::string::npos) f = 0;   // Coupe de France (phase nationale) : prise en charge par la FFF
        if (C.kind == 3 || C.kind == 8 || C.kind == 7 || C.kind == 11 || C.kind == 6) f = 0;
        if (C.kind == 4 || C.kind == 5) f = f * 3 / 4;
    }
    return f;
}

// cahier des charges des stades (places minimum)
int minCapacity(int tier) {
    static const int REQ[13] = { 15000, 8000, 5000, 2000, 1000, 500, 300, 200, 100, 0, 0, 0, 0 };
    return tier >= 0 && tier < 13 ? REQ[tier] : 0;
}
