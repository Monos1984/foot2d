// Mode hors-série « Légendes de la Coupe du monde » : rejouer les phases finales de 1930 à 2026 (effectifs d'époque,
// formule authentique, stades, points par victoire et remplacements de l'époque).
#include "game.h"
#include <algorithm>
#include <cstring>
#include <map>

int addCompPublic(Season& S, Competition c);

static std::vector<std::string> splitL(const char* s, char sep) {
    std::vector<std::string> v; std::string cur;
    for (const char* p = s; ; p++) { if (*p == sep || !*p) { v.push_back(cur); cur.clear(); if (!*p) break; } else cur += *p; }
    return v;
}

struct LTeam { std::string code, disp; int rating; };
static std::vector<std::vector<LTeam>> legendGroups(int ed) {
    std::vector<std::vector<LTeam>> g;
    if (!LEGENDS[ed].teams || !*LEGENDS[ed].teams) return g;
    for (auto& grp : splitL(LEGENDS[ed].teams, '/')) {
        std::vector<LTeam> v;
        for (auto& e : splitL(grp.c_str(), ';')) {
            auto f = splitL(e.c_str(), ':');
            if (f.empty() || f[0].empty()) continue;
            LTeam t; t.code = f[0]; t.rating = f.size() > 1 ? atoi(f[1].c_str()) : 60; t.disp = f.size() > 2 ? f[2] : std::string();
            v.push_back(t);
        }
        g.push_back(v);
    }
    return g;
}

static std::string histShort(const std::string& disp, const std::string& code) {
    static const char* M[][2] = { { "Yougoslavie", "YUG" }, { "Tchécoslovaquie", "TCH" }, { "URSS", "URS" }, { "RFA", "RFA" }, { "RDA", "RDA" },
                                  { "Zaïre", "ZAI" }, { "Indes néerlandaises", "DEI" }, { "Serbie-et-Monténégro", "SCG" }, { "Corée du Nord", "PRK" } };
    for (auto& m : M) if (disp == m[0]) return m[1];
    return code;
}

// équipes de l'édition (créées à la demande, effectifs d'époque) ; 2026 : sélections actuelles
static std::map<int, std::vector<int>> g_legendCache;
std::vector<int> legendTeams(int ed) {
    std::vector<int> out;
    if (ed < 0 || ed >= NUM_LEGENDS) return out;
    auto it = g_legendCache.find(ed);
    if (it != g_legendCache.end()) {
        bool ok = true;
        for (int t : it->second) if (t < 0 || t >= (int)g_world.teams.size() || (g_world.teams[t].youth != 7 && LEGENDS[ed].year != 2026)) ok = false;
        if (ok) return it->second;
    }
    const LegendEdition& E = LEGENDS[ed];
    if (E.year == 2026) {       // 48 sélections actuelles, dans l'ordre des vrais groupes du tirage (5 décembre 2025)
        std::vector<int> v;
        for (auto& grp : legendGroups(ed)) for (auto& lt : grp) { int n = g_world.nationIndex(lt.code.c_str()); if (n >= 0) v.push_back(n); }
        if (v.size() == 48) { g_legendCache[ed] = v; return v; }
        v.clear();
        for (const char* h : { "USA", "MEX", "CAN" }) { int n = g_world.nationIndex(h); if (n >= 0) v.push_back(n); }
        std::vector<int> r; for (int i = 0; i < NUM_NATIONS; i++) if (nationEligible(i) && std::find(v.begin(), v.end(), i) == v.end()) r.push_back(i);
        std::stable_sort(r.begin(), r.end(), [](int a, int b) { return g_world.teams[a].rating > g_world.teams[b].rating; });
        for (int n : r) { if ((int)v.size() >= 48) break; v.push_back(n); }
        g_legendCache[ed] = v;
        return v;
    }
    for (auto& grp : legendGroups(ed)) for (auto& lt : grp) {
        int nat = g_world.nationIndex(lt.code.c_str());
        if (nat < 0) nat = g_world.nationIndex("FRA");
        const Team& N = g_world.teams[nat];
        Team t;
        std::string base = lt.disp.empty() ? N.name : sanitize(lt.disp.c_str());
        t.name = base + fmt(" %d", E.year);
        t.shortName = histShort(lt.disp, lt.code);
        t.kind = TK_NATION; t.nation = nat; t.youth = 7; t.rating = (float)lt.rating; t.culture = N.culture;
        t.home = N.home; t.away = N.away;
        auto recolor = [&](unsigned c1, unsigned c2) { t.home.shirt = c1; t.home.shirt2 = c1; t.home.shorts = c2; t.home.pattern = 0; };
        if (lt.disp == "RDA") recolor(0x0055A4, 0xFFFFFF);
        if (lt.disp == "Yougoslavie" || lt.disp == "Serbie-et-Monténégro") recolor(0x1B3A8C, 0xFFFFFF);
        if (lt.disp == "URSS") recolor(0xCC0000, 0xFFFFFF);
        if (lt.disp == "Zaïre") recolor(0x00843D, 0xFFE500);
        if (lt.disp == "Tchécoslovaquie") recolor(0xCC0000, 0xFFFFFF);
        t.stadium = std::string(); t.seed = hashStr(t.name) ^ 0x1930u; t.formation = (int)(t.seed % NUM_FORMATIONS);
        // effectif : joueurs générés au niveau de l'équipe, puis joueurs réels de l'époque
        std::vector<Player> sq;
        generateFakeSquad(t, 22, sq);
        Rng r(t.seed);
        for (auto& p : sq) {
            p.nation = (int16_t)nat; p.age = (uint8_t)r.range(20, 33); p.gender = 0;
            int cap = lt.rating - 4 + r.range(-5, 2), ov = p.overall();      // joueurs générés : sous le niveau des vedettes de l'époque
            if (ov > cap && ov > 0) {
                float f = (float)cap / ov;
                auto sc = [&](uint8_t& v) { v = (uint8_t)std::max(10, (int)(v * f)); };
                sc(p.speed); sc(p.shoot); sc(p.pass); sc(p.tackle); sc(p.stamina); if (p.pos == POS_GK) sc(p.keep);
            }
        }
        const char* list = nullptr;
        std::string k1 = lt.code, k2 = lt.code + ":" + lt.disp;
        for (int i = 0; i < NUM_LEGEND_SQUADS; i++) if (LEGEND_SQUADS[i].year == E.year && (k2 == LEGEND_SQUADS[i].key || (lt.disp.empty() || lt.disp == "RFA" ? k1 == LEGEND_SQUADS[i].key : false))) list = LEGEND_SQUADS[i].players;
        if (list) {
            std::vector<bool> done(sq.size(), false);
            for (auto& ps : splitL(list, ';')) {
                auto f = splitL(ps.c_str(), '|');
                if (f.size() < 3) continue;
                int pos = f[1] == "G" ? POS_GK : f[1] == "D" ? POS_DF : f[1] == "M" ? POS_MF : POS_FW;
                int slot = -1;
                for (int i = 0; i < (int)sq.size() && slot < 0; i++) if (!done[i] && sq[i].pos == pos) slot = i;
                if (slot < 0) for (int i = 0; i < (int)sq.size() && slot < 0; i++) if (!done[i] && sq[i].pos != POS_GK) slot = i;
                if (slot < 0) { Player np = sq.empty() ? Player() : sq.back(); sq.push_back(np); done.push_back(false); slot = (int)sq.size() - 1; }
                done[slot] = true;
                Player& p = sq[slot];
                int o = std::max(40, std::min(99, atoi(f[2].c_str())));
                p.name = sanitize(f[0].c_str()); p.pos = (uint8_t)pos;
                auto c = [&](int v) { return (uint8_t)std::max(15, std::min(99, v + r.range(-1, 1))); };
                switch (pos) {
                case POS_GK: p.keep = (uint8_t)o; p.speed = c(o - 25); p.pass = c(o - 30); p.shoot = c(o - 45); p.tackle = c(o - 40); break;
                case POS_DF: p.tackle = c(o + 2); p.speed = c(o - 2); p.pass = c(o - 2); p.shoot = c(o - 20); p.keep = 15; break;
                case POS_MF: p.pass = c(o + 2); p.speed = c(o - 1); p.shoot = c(o - 2); p.tackle = c(o - 3); p.keep = 15; break;
                default: p.shoot = c(o + 1); p.speed = c(o + 1); p.pass = c(o - 3); p.tackle = c(o - 25); p.keep = 12; break;
                }
                p.stamina = c(o - 4);
                p.dribble = p.heading = p.positioning = p.composure = 0;
            }
            // liste complète de l'époque : aucun joueur inventé
            int nReal = 0; for (bool d : done) nReal += d;
            if (nReal >= 18) { std::vector<Player> keep; for (int i = 0; i < (int)sq.size(); i++) if (done[i]) keep.push_back(sq[i]); sq = keep; }
            // les joueurs réels d'abord (titulaires)
            std::stable_sort(sq.begin(), sq.end(), [&](const Player& a, const Player& b) { return a.overall() > b.overall(); });
        }
        int num = 1;
        for (auto& p : sq) { p.num = (uint8_t)(num++); p.id = g_world.nextPid++; p.contract = 2; }
        t.squad = sq; t.squadGen = true;
        g_world.teams.push_back(t);
        out.push_back((int)g_world.teams.size() - 1);
    }
    g_legendCache[ed] = out;
    return out;
}

const char* legendFormatName(int ed) {
    switch (LEGENDS[ed].format) {
    case 1: return "4 groupes, demi-finales, finale";
    case 2: return "élimination directe dès les 8es de finale";
    case 3: return "4 groupes puis poule finale (sans finale)";
    case 4: return "4 groupes de 4, quarts, demies, finale";
    case 5: return "2 tours de groupes, finale";
    case 6: return "24 équipes, 2 tours de groupes, demies";
    case 7: return "24 équipes, 6 groupes, 8es de finale";
    case 8: return "32 équipes, 8 groupes, 8es de finale";
    default: return "48 équipes, 12 groupes, 16es de finale";
    }
}

// démarre le tournoi (le Career est déjà réinitialisé en mode personnalisé)
void legendStart(Career& K, int ed, const std::vector<int>& ctrl) {
    const LegendEdition& E = LEGENDS[ed];
    auto teams = legendTeams(ed);
    CustomCompDef def; def.name = fmt("Coupe du monde %d (%s)", E.year, E.host); def.format = 0; def.legs = 1; def.teams = teams;
    K.newCustom(def, ctrl);
    K.custom.format = 10 + ed;
    K.season.comps.clear();
    K.season.year = K.year = E.year;
    if(ed<22) K.season.now=legendCalendarTime(ed,0)-1.0/7;
    Competition c;
    c.name = fmt("Coupe du monde %d", E.year); c.shortName = "Coupe du monde";
    c.kind = 50; c.tag = ed; c.ptsWin = E.ptsWin; c.tb = TB_GD; c.legs = 1; c.neutralFinal = true;
    int hostNat = g_world.nationIndex(E.hostCode);
    for (int t : teams) if (g_world.teams[t].nation == hostNat && (E.year != 1974 || g_world.teams[t].shortName == "RFA")) c.host = t;
    // groupes : ceux de l'époque
    std::vector<std::vector<int>> groups;
    if (E.year == 2026 && legendGroups(ed).size() == 12 && teams.size() == 48) {      // vrais groupes
        for (int g = 0; g < 12; g++) groups.push_back(std::vector<int>(teams.begin() + g * 4, teams.begin() + g * 4 + 4));
    } else if (E.year == 2026) {
        std::vector<int> v = teams;
        std::stable_sort(v.begin() + 3, v.end(), [](int a, int b) { return g_world.teams[a].rating > g_world.teams[b].rating; });
        groups.assign(12, {});
        for (int pot = 0; pot < 4; pot++) {
            std::vector<int> p(v.begin() + pot * 12, v.begin() + pot * 12 + 12);
            if (pot == 0) g_rng.shuffle(p);
            else g_rng.shuffle(p);
            for (int g = 0; g < 12; g++) groups[g].push_back(p[g]);
        }
    } else {
        int k = 0;
        for (auto& grp : legendGroups(ed)) { std::vector<int> g; for (size_t i = 0; i < grp.size(); i++) g.push_back(teams[k++]); groups.push_back(g); }
    }
    if (E.format == 2) {        // 1934, 1938 : élimination directe
        c.format = FMT_KO_ONLY; c.qualSpots = 1;
        c.koLegs = { 1, 1, 1, 1 }; c.koTimes = { 2, 5, 8, 11 };
        std::vector<std::pair<int, int>> pairs;
        for (auto& g : groups) pairs.push_back({ g[0], g.size() > 1 ? g[1] : -1 });
        c.addKOStage(pairs, 1, 2, "8es de finale");
    } else {
        c.format = FMT_TOURNAMENT;
        c.groupsAdvance = (E.format == 1 || E.format == 3) ? 1 : 2;
        c.bestThirds = E.format == 7 ? 4 : E.format == 9 ? 8 : 0;
        c.thirdPlace = E.format == 4 || E.format == 6 || E.format >= 7;
        c.addGroupStage(groups, 1, { 1, 3, 5 }, E.format == 5 || E.format == 6 ? "Premier tour" : "Phase de groupes");
    }
    for (auto& m : c.matches) {         // terrain neutre, sauf pour le pays organisateur
        m.neutral = !(c.host >= 0 && (m.home == c.host || m.away == c.host));
        if (c.host >= 0 && m.away == c.host) std::swap(m.home, m.away);
    }
    c.cur = 0;
    addCompPublic(K.season, std::move(c));
    K.season.news.push_back(fmt("Coupe du monde %d en %s : %s. Victoire à %d points ; %s.", E.year, E.host, legendFormatName(ed), E.ptsWin,
                                E.subs == 0 ? "aucun remplacement autorisé" : fmt("%d remplacements autorisés", E.subs).c_str()));
}

// enchaînement des tours propres aux formules anciennes (appelé par Competition::onStageFinished, kind 50)
bool legendStageFinished(Competition& C, const std::vector<int>& winners) {
    (void)winners;
    if (C.kind != 50 || C.tag < 0 || C.tag >= NUM_LEGENDS) return false;
    int F = LEGENDS[C.tag].format;
    Stage& st = C.stages[C.cur];
    double t = st.rounds.empty() ? 6 : st.rounds.back().time;
    auto pos = [&](int g, int k) { auto tb = C.table(C.cur, g); return k < (int)tb.size() ? tb[k].team : -1; };
    if (st.type != ST_LEAGUE) return false;
    int ng = (int)st.groups.size();
    if (F == 1 && ng == 4) {     // 1930 : demi-finales entre les vainqueurs de groupe (1A-1D, 1C-1B)
        std::vector<std::pair<int, int>> sf = { { pos(0, 0), pos(3, 0) }, { pos(2, 0), pos(1, 0) } };
        C.cur = (int)C.stages.size();
        C.addKOStage(sf, 1, t + 2, "Demi-finales");
        return true;
    }
    if (F == 3) {
        if (C.cur == 0) {
            std::vector<int> fin; for (int g = 0; g < ng; g++) fin.push_back(pos(g, 0));
            C.addGroupStage({ fin }, 1, { t + 2, t + 4, t + 6 }, "Poule finale");
            C.cur = (int)C.stages.size() - 1;
            for (auto& m : C.matches) if (C.stageOfMatch((int)(&m - &C.matches[0])) == C.cur) { m.neutral = !(C.host >= 0 && (m.home == C.host || m.away == C.host)); if (C.host >= 0 && m.away == C.host) std::swap(m.home, m.away); }
            return true;
        }
        auto tb = C.table(C.cur, 0);
        C.result.clear(); for (auto& s : tb) C.result.push_back(s.team);
        C.done = true; C.winner = C.result.empty() ? -1 : C.result[0];
        return true;
    }
    if (F == 5) {
        if (C.cur == 0 && ng == 4) {
            std::vector<int> A = { pos(0, 0), pos(1, 1), pos(2, 0), pos(3, 1) }, B = { pos(1, 0), pos(0, 1), pos(3, 0), pos(2, 1) };
            C.addGroupStage({ A, B }, 1, { t + 2, t + 4, t + 6 }, "Deuxième tour");
            C.cur = (int)C.stages.size() - 1;
            return true;
        }
        int a1 = pos(0, 0), b1 = pos(1, 0), a2 = pos(0, 1), b2 = pos(1, 1);
        C.addKOStage({ { a2, b2 } }, 1, t + 1.8, "Match pour la 3e place", true);
        C.cur = (int)C.stages.size();
        C.addKOStage({ { a1, b1 } }, 1, t + 2, "Finale", true);
        return true;
    }
    if (F == 6) {
        if (C.cur == 0 && ng == 6) {
            std::vector<std::vector<int>> G2 = { { pos(0, 0), pos(5, 1), pos(2, 0) }, { pos(1, 0), pos(3, 0), pos(4, 1) },
                                                 { pos(0, 1), pos(5, 0), pos(2, 1) }, { pos(4, 0), pos(1, 1), pos(3, 1) } };
            C.addGroupStage(G2, 1, { t + 2, t + 4, t + 6 }, "Deuxième tour");
            C.cur = (int)C.stages.size() - 1;
            return true;
        }
        std::vector<std::pair<int, int>> sf = { { pos(0, 0), pos(2, 0) }, { pos(1, 0), pos(3, 0) } };
        C.cur = (int)C.stages.size();
        C.addKOStage(sf, 1, t + 2, "Demi-finales");
        return true;
    }
    return false;
}

std::string legendVenue(int comp, int mi) {
    const Season& S = g_career.season;
    if (comp < 0 || comp >= (int)S.comps.size()) return "";
    const Competition& C = S.comps[comp];
    if (C.kind != 50 || C.tag < 0 || C.tag >= NUM_LEGENDS) return "";
    const LegendEdition& E = LEGENDS[C.tag];
    int st = C.stageOfMatch(mi);
    if (st >= 0 && C.stages[st].name == "Finale") return E.finalStadium;
    auto v = splitL(E.stadiums, ';');
    uint32_t h = (uint32_t)(mi * 2654435761u + st * 97);
    return v.empty() ? std::string() : v[h % v.size()];
}

void legendSheet(int ed, int& sheet, int& subs) {
    int s = LEGENDS[ed].subs;
    subs = s;
    sheet = s == 0 ? 16 : s == 2 ? 16 : s == 3 ? 18 : 20;
}
bool legendGoldenGoal(int ed) { return LEGENDS[ed].year == 1998 || LEGENDS[ed].year == 2002; }
