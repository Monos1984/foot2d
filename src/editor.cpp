// Éditeur : clubs créés ou modifiés par le joueur (fichier clubs_perso.txt à côté de l'exe)
#include "game.h"
#include <cstring>

static const char* CUSTOM_FILE = "clubs_perso.txt";

static std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> v; std::string cur;
    for (char c : s) { if (c == sep) { v.push_back(cur); cur.clear(); } else cur += c; }
    v.push_back(cur);
    return v;
}
static std::string clean(const std::string& s) { std::string o; for (char c : s) if (c != '|' && c != '\n' && c != '\r') o += c; return o; }

void loadCustomClubs() {
    FILE* f = fopen(CUSTOM_FILE, "r");
    if (!f) return;
    char buf[1024];
    int fra = g_world.nationIndex("FRA");
    while (fgets(buf, sizeof buf, f)) {
        std::string line = buf;
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
        auto v = split(line, '|');
        if (v.size() >= 11 && v[0] == "CLUB") {
            Team t;
            t.name = v[1]; t.shortName = v[2]; t.stadium = v[3]; t.town = v[4];
            t.dept = atoi(v[5].c_str());
            if (t.dept < 0 || t.dept >= NUM_DEPTS) continue;
            t.region = DEPTS[t.dept].region;
            t.home.shirt = (unsigned)strtoul(v[6].c_str(), nullptr, 16);
            t.home.shirt2 = (unsigned)strtoul(v[7].c_str(), nullptr, 16);
            t.home.shorts = (unsigned)strtoul(v[8].c_str(), nullptr, 16);
            t.home.pattern = atoi(v[9].c_str());
            t.home.socks = t.home.shirt;
            t.away.shirt = t.home.shirt2; t.away.shirt2 = t.home.shirt; t.away.shorts = t.home.shirt2; t.away.socks = t.home.shirt2;
            t.rating = (float)atof(v[10].c_str());
            t.founded = v.size() > 11 ? atoi(v[11].c_str()) : 2026;
            if (v.size() > 17) {
                initStadium(t, 10, -1);
                stadiumSetCapacity(t.sta, std::max(150, atoi(v[12].c_str())));
                t.sta.s[0].kind = (uint8_t)std::max(0, std::min(2, atoi(v[13].c_str())));
                t.sta.buvette = (uint8_t)atoi(v[14].c_str()); t.sta.boutique = (uint8_t)atoi(v[15].c_str()); t.sta.lights = (uint8_t)atoi(v[16].c_str());
                t.sponsor = v[17];
            }
            t.kind = TK_CLUB; t.nation = fra; t.culture = CU_FR;
            t.seed = hashStr(t.name) ^ 0x1234;
            t.formation = 0;
            if (v.size() > 19) { int di = districtIndex(v[19]); if (di >= 0 && districtRegion(di) == t.region) t.district = di; }
            if (t.district < 0) t.district = districtFor(t.dept, t.town, t.name);
            if (v.size() > 24 && v[20] == "1") {       // troisième maillot
                t.hasThird = 1; t.third.shirt = (unsigned)strtoul(v[21].c_str(), nullptr, 16); t.third.shirt2 = (unsigned)strtoul(v[22].c_str(), nullptr, 16);
                t.third.shorts = (unsigned)strtoul(v[23].c_str(), nullptr, 16); t.third.pattern = atoi(v[24].c_str()); t.third.socks = t.third.shirt;
            }
            int idx = g_world.addCustomClub(t);
            if (v.size() > 18 && v[18] == "1" && idx >= 0) { makeU19Team(g_world, idx); makeU17Team(g_world, idx); }    // équipes U19 et U17 du club
        } else if (v.size() >= 10 && v[0] == "EDIT") {
            int id = atoi(v[1].c_str());
            if (id < 0 || id >= g_world.baseCount) continue;
            Team& t = g_world.teams[id];
            t.name = v[2]; t.shortName = v[3]; t.stadium = v[4];
            t.home.shirt = (unsigned)strtoul(v[5].c_str(), nullptr, 16);
            t.home.shirt2 = (unsigned)strtoul(v[6].c_str(), nullptr, 16);
            t.home.shorts = (unsigned)strtoul(v[7].c_str(), nullptr, 16);
            t.home.pattern = atoi(v[8].c_str());
            t.home.socks = t.home.shirt;
            if (v.size() > 13 && v[9] == "1") {
                t.hasThird = 1; t.third.shirt = (unsigned)strtoul(v[10].c_str(), nullptr, 16); t.third.shirt2 = (unsigned)strtoul(v[11].c_str(), nullptr, 16);
                t.third.shorts = (unsigned)strtoul(v[12].c_str(), nullptr, 16); t.third.pattern = atoi(v[13].c_str()); t.third.socks = t.third.shirt;
            }
            t.edited = true;
        }
    }
    fclose(f);
}

void saveCustomClubs() {
    FILE* f = fopen(CUSTOM_FILE, "w");
    if (!f) return;
    for (int i = 0; i < (int)g_world.teams.size(); i++) {
        const Team& t = g_world.teams[i];
        if (t.custom && !t.youth) {
            bool u19 = false; for (auto& o : g_world.teams) if (o.parent == i && o.youth == 1) u19 = true;
            fprintf(f, "CLUB|%s|%s|%s|%s|%d|%06X|%06X|%06X|%d|%.1f|%d|%d|%d|%d|%d|%d|%s|%d|%s|%d|%06X|%06X|%06X|%d\n", clean(t.name).c_str(), clean(t.shortName).c_str(), clean(t.stadium).c_str(),
                    clean(t.town).c_str(), t.dept, t.home.shirt, t.home.shirt2, t.home.shorts, t.home.pattern, t.rating, t.founded,
                    t.sta.init ? t.sta.capacity() : 300, (int)t.sta.s[0].kind, (int)t.sta.buvette, (int)t.sta.boutique, (int)t.sta.lights, clean(t.sponsor).c_str(), u19 ? 1 : 0,
                    clean(districtName(t.district)).c_str(), (int)t.hasThird, t.third.shirt, t.third.shirt2, t.third.shorts, t.third.pattern);
        }
        else if (t.edited && i < g_world.baseCount)
            fprintf(f, "EDIT|%d|%s|%s|%s|%06X|%06X|%06X|%d|%d|%06X|%06X|%06X|%d\n", i, clean(t.name).c_str(), clean(t.shortName).c_str(), clean(t.stadium).c_str(),
                    t.home.shirt, t.home.shirt2, t.home.shorts, t.home.pattern, (int)t.hasThird, t.third.shirt, t.third.shirt2, t.third.shorts, t.third.pattern);
    }
    fclose(f);
}

// ------------------------------------------------------------------ éditeur de joueurs et présidents (fichier joueurs_perso.txt)
static const char* PLAYERS_FILE = "joueurs_perso.txt";
struct PlayerEditRec { std::string team; int slot; std::vector<std::string> f; };
static std::vector<PlayerEditRec> g_pedits;
struct PresRec { std::string team, name, place; int age; };
static std::vector<PresRec> g_pres;
static bool g_peditsLoaded = false;

static void loadPlayerEdits() {
    if (g_peditsLoaded) return;
    g_peditsLoaded = true;
    FILE* f = fopen(PLAYERS_FILE, "r");
    if (!f) return;
    char buf[2048];
    while (fgets(buf, sizeof buf, f)) {
        std::string line = buf;
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
        auto v = split(line, '|');
        if (v.size() >= 4 && v[0] == "P") { PlayerEditRec r; r.team = v[1]; r.slot = atoi(v[2].c_str()); r.f.assign(v.begin() + 3, v.end()); g_pedits.push_back(r); }
        else if (v.size() >= 5 && v[0] == "PRES") g_pres.push_back({ v[1], v[2], v[4], atoi(v[3].c_str()) });
    }
    fclose(f);
}

static void writePlayerEdits() {
    FILE* f = fopen(PLAYERS_FILE, "w");
    if (!f) return;
    for (auto& r : g_pedits) {
        fprintf(f, "P|%s|%d", clean(r.team).c_str(), r.slot);
        for (auto& x : r.f) fprintf(f, "|%s", clean(x).c_str());
        fprintf(f, "\n");
    }
    for (auto& p : g_pres) fprintf(f, "PRES|%s|%s|%d|%s\n", clean(p.team).c_str(), clean(p.name).c_str(), p.age, clean(p.place).c_str());
    fclose(f);
}

void applyPresidents() {
    loadPlayerEdits();
    for (auto& p : g_pres)
        for (auto& t : g_world.teams) if (t.name == p.team && t.parent < 0) { t.president = p.name; t.presAge = p.age; t.presPlace = p.place; }
}

void savePresident(const Team& t) {
    loadPlayerEdits();
    for (auto& p : g_pres) if (p.team == t.name) { p.name = t.president; p.age = t.presAge; p.place = t.presPlace; writePlayerEdits(); return; }
    g_pres.push_back({ t.name, t.president, t.presPlace, t.presAge });
    writePlayerEdits();
}

// champs : nom, poste, numéro, âge, vitesse, tir, passe, tacle, gardien, endurance, dribble, tête, placement, sang-froid, jour, mois, lieu, nation, potentiel
void applyPlayerEdits(Team& t) {
    loadPlayerEdits();
    // joueurs créés avec l'éditeur : emplacements au-delà de l'effectif généré, ajoutés dans l'ordre
    std::vector<int> extra;
    for (auto& r : g_pedits) if (r.team == t.name && r.slot >= (int)t.squad.size()) extra.push_back(r.slot);
    std::sort(extra.begin(), extra.end());
    for (int sl : extra) {
        if (sl != (int)t.squad.size()) continue;
        Player p; p.id = g_world.nextPid++; p.contract = t.youth ? 2 : 0;
        t.squad.push_back(p);
    }
    for (auto& r : g_pedits) {
        if (r.team != t.name || r.slot < 0 || r.slot >= (int)t.squad.size() || r.f.size() < 19) continue;
        Player& p = t.squad[r.slot];
        auto I = [&](int k) { return atoi(r.f[k].c_str()); };
        auto U = [&](int k) { return (uint8_t)std::max(0, std::min(99, I(k))); };
        p.name = r.f[0]; p.pos = (uint8_t)std::max(0, std::min(3, I(1))); p.num = (uint8_t)std::max(1, std::min(99, I(2))); p.age = (uint8_t)std::max(15, std::min(45, I(3)));
        p.speed = U(4); p.shoot = U(5); p.pass = U(6); p.tackle = U(7); p.keep = U(8); p.stamina = U(9);
        p.dribble = U(10); p.heading = U(11); p.positioning = U(12); p.composure = U(13);
        p.bday = (uint8_t)std::max(0, std::min(31, I(14))); p.bmonth = (uint8_t)std::max(0, std::min(12, I(15))); p.birthPlace = r.f[16];
        p.nation = (int16_t)std::max(-1, std::min(NUM_NATIONS - 1, I(17))); p.pot = U(18);
    }
}

void savePlayerEdit(const Team& t, int idx) {
    loadPlayerEdits();
    if (idx < 0 || idx >= (int)t.squad.size()) return;
    const Player& p = t.squad[idx];
    PlayerEditRec r; r.team = t.name; r.slot = idx;
    auto S = [](int v) { return std::to_string(v); };
    r.f = { p.name, S(p.pos), S(p.num), S(p.age), S(p.speed), S(p.shoot), S(p.pass), S(p.tackle), S(p.keep), S(p.stamina),
            S(p.dribble), S(p.heading), S(p.positioning), S(p.composure), S(p.bday), S(p.bmonth), p.birthPlace, S(p.nation), S(p.pot) };
    for (auto& x : g_pedits) if (x.team == r.team && x.slot == idx) { x = r; writePlayerEdits(); return; }
    g_pedits.push_back(r);
    writePlayerEdits();
}

// ------------------------------------------------------------------ staff créé avec l'éditeur (staff_perso.txt)
static std::vector<CustomStaff> g_cstaff;
static bool g_cstaffLoaded = false;
std::vector<CustomStaff>& customStaff() {
    if (!g_cstaffLoaded) {
        g_cstaffLoaded = true;
        FILE* f = fopen("staff_perso.txt", "r");
        if (f) {
            char buf[1024];
            while (fgets(buf, sizeof buf, f)) {
                std::string line = buf;
                while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
                auto v = split(line, '|');
                if (v.size() >= 7 && v[0] == "S") {
                    CustomStaff c; c.name = v[1]; c.role = std::max(0, std::min(NUM_SR - 1, atoi(v[2].c_str()))); c.level = std::max(1, std::min(5, atoi(v[3].c_str())));
                    c.age = atoi(v[4].c_str()); c.place = v[5]; c.club = v[6];
                    g_cstaff.push_back(c);
                }
            }
            fclose(f);
        }
    }
    return g_cstaff;
}
void saveCustomStaff() {
    FILE* f = fopen("staff_perso.txt", "w");
    if (!f) return;
    for (auto& c : customStaff()) fprintf(f, "S|%s|%d|%d|%d|%s|%s\n", clean(c.name).c_str(), c.role, c.level, c.age, clean(c.place).c_str(), clean(c.club).c_str());
    fclose(f);
}
