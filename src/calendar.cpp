// Calendrier réel : jour et heure du coup d'envoi de chaque rencontre d'une journée / d'un tour.
// Les matchs d'une même journée s'échelonnent (vendredi soir, samedi, dimanche...) ; ceux programmés avant le
// match du joueur sont connus avant, ceux joués en même temps ou après ne le sont qu'à la fin de la journée.
#include "game.h"
#include <map>

static const char* DAYS[7] = { "lun.", "mar.", "mer.", "jeu.", "ven.", "sam.", "dim." };
static const char* MONTHS[12] = { "janv.", "févr.", "mars", "avr.", "mai", "juin", "juil.", "août", "sept.", "oct.", "nov.", "déc." };

static uint32_t mix(uint32_t h) { h ^= h >> 16; h *= 0x7feb352d; h ^= h >> 15; h *= 0x846ca68b; h ^= h >> 16; return h; }

// niveau (0 = élite) du championnat : pool de la pyramide qui contient cette compétition
static int compTier(int comp) {
    static std::map<int, int> cache; static size_t n = 0; static int yr = -1; static const void* sp = nullptr;
    const Season& S = g_career.season;
    if (n != S.comps.size() || yr != S.year || sp != (const void*)S.comps.data()) {
        cache.clear(); n = S.comps.size(); yr = S.year; sp = S.comps.data();
        for (auto& P : g_career.pyramids) for (auto& pl : P.pools) for (int c : pl.comps) cache[c] = P.dom >= 0 ? 90 + pl.tier : pl.tier;
    }
    auto it = cache.find(comp);
    return it == cache.end() ? -1 : it->second;
}

struct RoundPos { int stage = -1, round = -1; bool lastLeague = false; int n = 1; };

// rang du match dans sa journée (ordre pseudo-aléatoire stable)
static int roundIdx(const Competition& C, const RoundPos& p, int mi) {
    if (p.stage < 0) return 0;
    const auto& m = C.stages[p.stage].rounds[p.round].m;
    uint32_t salt = (uint32_t)(p.round * 97 + p.stage * 13 + g_career.season.year);
    uint32_t hm = mix((uint32_t)(C.matches[mi].home * 2654435761u) ^ salt);
    int idx = 0;
    for (int x : m) { if (x == mi) continue; uint32_t hx = mix((uint32_t)(C.matches[x].home * 2654435761u) ^ salt); if (hx < hm || (hx == hm && x < mi)) idx++; }
    return idx;
}

static RoundPos findRound(const Competition& C, int mi) {
    RoundPos p;
    for (int s = 0; s < (int)C.stages.size(); s++) {
        const Stage& st = C.stages[s];
        for (int r = 0; r < (int)st.rounds.size(); r++) {
            const auto& m = st.rounds[r].m;
            for (int k = 0; k < (int)m.size(); k++) if (m[k] == mi) {
                p.stage = s; p.round = r; p.n = (int)m.size();
                p.lastLeague = st.type == ST_LEAGUE && r == (int)st.rounds.size() - 1;
                return p;
            }
        }
    }
    return p;
}

static int T(int day, int h, int m) { return day * 1440 + h * 60 + m; }

int kickoffMinutes(int comp, int mi) {
    const Season& S = g_career.season;
    if (comp < 0 || comp >= (int)S.comps.size()) return T(5, 15, 0);
    const Competition& C = S.comps[comp];
    if (mi < 0 || mi >= (int)C.matches.size()) return T(5, 15, 0);
    const MatchRes& M = C.matches[mi];
    RoundPos rp = findRound(C, mi);
    uint32_t h = mix((uint32_t)(comp * 7919 + mi * 31 + S.year));
    bool finalM = rp.stage >= 0 && rp.stage == (int)C.stages.size() - 1 && C.stages[rp.stage].type == ST_KO && rp.n == 1;
    switch (C.kind) {
    case 3: {       // Ligue des champions : mardi / mercredi 20h45, les matchs d'un même groupe en même temps
        if (finalM) return T(2, 20, 45);
        if (rp.stage >= 0 && C.stages[rp.stage].type == ST_LEAGUE) {
            int g = std::max(0, (int)M.group);
            bool tue = ((g % 8) < 4) != (rp.round % 2 == 1);
            return T(tue ? 1 : 2, 20, 45);
        }
        return T((roundIdx(C, rp, mi) % 2) ? 2 : 1, 20, 45);
    }
    case 8:         // Coupe UEFA : jeudi 18h45 ou 21h00
        if (finalM) return T(2, 20, 45);
        return (roundIdx(C, rp, mi) % 2) ? T(3, 21, 0) : T(3, 18, 45);
    case 7: return T(4, 20, 45);
    case 6: return T(5, 20, 45);
    case 12: return T(5, 17, 0);
    case 20: case 21: return T((roundIdx(C, rp, mi) % 2) ? 2 : 1, 14, 0);
    default: break;
    }
    if (C.format == FMT_TOURNAMENT || C.format == FMT_QUAL_GROUPS) {
        if (rp.stage >= 0 && C.stages[rp.stage].type == ST_LEAGUE) {
            int g = std::max(0, (int)M.group);
            if (rp.lastLeague) return T(g % 4, g % 2 ? 21 : 18, 0);          // dernière journée : les deux matchs du groupe en même temps
            static const int HR[3] = { 15, 18, 21 };
            return T((g + rp.round) % 4, HR[(roundIdx(C, rp, mi) + g) % 3], 0);
        }
        if (finalM) return T(6, 20, 0);
        static const int HR2[2] = { 17, 21 };
        return T(5 + (roundIdx(C, rp, mi) / 2) % 2, HR2[roundIdx(C, rp, mi) % 2], 0);
    }
    if (C.kind == 1) {
        int lv = compTier(comp);
        if (lv >= 90) lv -= 90;
        if (lv < 0) lv = teamLevel(M.home);
        if (rp.lastLeague) {         // dernière journée : tous les matchs en même temps
            if (lv == 0) return T(6, 21, 0);
            if (lv == 1) return T(4, 20, 0);
            if (lv == 2) return T(4, 19, 30);
            return T(5, 18, 0);
        }
        int k = roundIdx(C, rp, mi);
        if (lv == 0) {
            static const int SL[9] = { T(4, 21, 0), T(5, 17, 0), T(5, 21, 0), T(6, 15, 0), T(6, 17, 15), T(6, 17, 15), T(6, 20, 45), T(5, 19, 0), T(6, 17, 15) };
            return k < 9 ? SL[k] : T(5, 19, 0);
        }
        if (lv == 1) return k == 0 ? T(5, 15, 0) : k == 1 ? T(7, 20, 45) : T(4, 20, 0);
        if (lv == 2) return k == 0 ? T(5, 18, 0) : T(4, 19, 30);
        if (lv <= 4) return k == 0 ? T(5, 17, 0) : k == 1 ? T(6, 15, 0) : T(5, 18, 0);
        return k == 0 ? T(5, 20, 0) : k == 1 ? T(5, 18, 0) : T(6, 15, 0);
    }
    // coupes nationales, régionales, de district, jeunes
    int lv = std::min(teamLevel(M.home), teamLevel(M.away));
    if (finalM) return C.kind == 2 ? T(5, 21, 0) : T(6, 15, 0);
    if ((C.kind == 2 && lv <= 2) || C.kind == 11) {
        static const int SL[6] = { T(1, 18, 0), T(1, 21, 0), T(2, 18, 30), T(2, 21, 0), T(3, 18, 0), T(3, 21, 10) };
        return SL[h % 6];
    }
    if (C.kind >= 15) { static const int SY[3] = { T(5, 14, 30), T(5, 15, 0), T(6, 11, 0) }; return SY[h % 3]; }
    static const int SC[5] = { T(5, 15, 0), T(5, 18, 0), T(5, 20, 0), T(6, 14, 30), T(6, 15, 0) };
    return SC[h % 5];
}

// date du lundi de la semaine « t » de la saison (semaine 0 = début août)
static void dateOf(double t, int addDays, int& d, int& mo, int& y) {
    int yr = g_career.season.year;
    auto dim = [](int m, int yy) { static const int D[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }; return m == 1 && ((yy % 4 == 0 && yy % 100) || yy % 400 == 0) ? 29 : D[m]; };
    // jour de la semaine du 1er août (0 = lundi) : algorithme de Sakamoto
    auto dow = [](int yy, int m, int dd) { static const int tt[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 }; if (m < 3) yy -= 1; int w = (yy + yy / 4 - yy / 100 + yy / 400 + tt[m - 1] + dd) % 7; return (w + 6) % 7; };
    int w1 = dow(yr, 8, 1);
    int day = 1 + (7 - w1) % 7;                 // premier lundi d'août
    int m = 7; y = yr;
    int add = (int)std::floor(t) * 7 + addDays;
    day += add;
    while (day > dim(m, y)) { day -= dim(m, y); m++; if (m == 12) { m = 0; y++; } }
    while (day < 1) { m--; if (m < 0) { m = 11; y--; } day += dim(m, y); }
    d = day; mo = m;
}

static double matchTime(const Competition& C, int mi) {
    for (auto& st : C.stages) for (auto& R : st.rounds) for (int x : R.m) if (x == mi) return R.time;
    return 0;
}

std::string kickoffText(int comp, int mi, bool withDate) {
    const Season& S = g_career.season;
    if (comp < 0 || comp >= (int)S.comps.size()) return "";
    int k = kickoffMinutes(comp, mi);
    int day = k / 1440, hh = (k % 1440) / 60, mm = k % 60;
    std::string s = fmt("%s %dh%02d", DAYS[day % 7], hh, mm);
    if (withDate) {
        int d, mo, y; dateOf(matchTime(S.comps[comp], mi), day, d, mo, y);
        s = fmt("%s %d %s %dh%02d", DAYS[day % 7], d, MONTHS[mo], hh, mm);
    }
    return s;
}

std::string kickoffHour(int comp, int mi) {
    int k = kickoffMinutes(comp, mi);
    return fmt("%dh%02d", (k % 1440) / 60, k % 60);
}
