// Construction de la base de données (équipes, effectifs)
#include "game.h"
#include <cstdarg>
#include <cstring>
#include <map>
#include <set>
#include <map>

Rng g_rng(0xC0FFEE);
World g_world;

uint32_t hashStr(const std::string& s) {
    uint32_t h = 2166136261u;
    for (unsigned char c : s) { h ^= c; h *= 16777619u; }
    return h;
}

std::string fmt(const char* f, ...) {
    char buf[1024];
    va_list ap; va_start(ap, f); vsnprintf(buf, sizeof buf, f, ap); va_end(ap);
    return buf;
}

static void putUtf8(std::string& o, unsigned cp) {
    if (cp < 0x80) o += (char)cp;
    else if (cp < 0x800) { o += (char)(0xC0 | (cp >> 6)); o += (char)(0x80 | (cp & 0x3F)); }
    else { o += (char)(0xE0 | (cp >> 12)); o += (char)(0x80 | ((cp >> 6) & 0x3F)); o += (char)(0x80 | (cp & 0x3F)); }
}

std::string sanitize(const char* s) {
    static const struct { unsigned cp; char c; } MAP[] = {
        {0x15F,'s'},{0x15E,'S'},{0x131,'i'},{0x130,'I'},{0x11F,'g'},{0x11E,'G'},{0x119,'e'},{0x118,'E'},
        {0x105,'a'},{0x104,'A'},{0x144,'n'},{0x143,'N'},{0x148,'n'},{0x142,'l'},{0x141,'L'},{0x15B,'s'},
        {0x15A,'S'},{0x17A,'z'},{0x17C,'z'},{0x17B,'Z'},{0x17D,'Z'},{0x17E,'z'},{0x107,'c'},{0x106,'C'},
        {0x10D,'c'},{0x10C,'C'},{0x111,'d'},{0x110,'D'},{0x161,'s'},{0x160,'S'},{0x159,'r'},{0x158,'R'},
        {0x11B,'e'},{0x16F,'u'},{0x103,'a'},{0x102,'A'},{0x163,'t'},{0x21B,'t'},{0x219,'s'},{0x218,'S'},
        {0x151,'o'},{0x171,'u'},{0x150,'O'},{0x165,'t'},{0x10F,'d'},{0x13E,'l'},{0x13A,'l'},{0x155,'r'},
        {0x113,'e'},{0x101,'a'},{0x12B,'i'},{0x16B,'u'},{0x259,'e'},{0x2019,'\''},{0x2018,'\''},{0x2013,'-'},
    };
    std::string o;
    const unsigned char* p = (const unsigned char*)s;
    while (*p) {
        unsigned cp; int n;
        if (*p < 0x80) { cp = *p; n = 1; }
        else if ((*p & 0xE0) == 0xC0) { cp = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F); n = 2; }
        else if ((*p & 0xF0) == 0xE0) { cp = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F); n = 3; }
        else { cp = '?'; n = 4; }
        for (int i = 1; i < n; i++) if (!p[i]) { n = i; break; }
        p += n;
        if (cp == '_') { o += ' '; continue; }
        if (cp <= 0xFF) { putUtf8(o, cp); continue; }
        char rep = '?';
        for (auto& m : MAP) if (m.cp == cp) { rep = m.c; break; }
        o += rep;
    }
    return o;
}

const char* posName(int pos) { static const char* N[] = { "G", "D", "M", "A" }; return N[pos & 3]; }

int Player::value() const {
    double v = 5.0 * std::exp((overall() - 40) / 5.0);
    double af = age <= 20 ? 1.6 : age <= 23 ? 1.35 : age <= 27 ? 1.0 : age <= 30 ? 0.7 : age <= 32 ? 0.4 : 0.2;
    if (pot > overall() + 5 && age <= 23) af *= 1.0 + (pot - overall()) * 0.03;
    v *= af;
    if (v < 1) v = 1;
    if (v > 250000) v = 250000;
    return (int)v;
}
int Player::wage() const {
    double w = 9.0 * std::exp((overall() - 40) / 6.0);
    return (int)std::max(1.0, std::min(40000.0, w));
}

int Player::drib() const {
    if (dribble) return dribble;
    int v = (speed * 45 + pass * 35 + shoot * 20) / 100 + (int)((unsigned)id * 37u % 11u) - 5 - (pos == POS_GK ? 25 : 0);
    return std::max(10, std::min(97, v));
}
int Player::head() const {
    if (heading) return heading;
    int v = (tackle * 35 + shoot * 35 + stamina * 30) / 100 + (int)((unsigned)id * 53u % 13u) - 6 + (pos == POS_DF ? 6 : pos == POS_FW ? 4 : 0) - (pos == POS_GK ? 25 : 0);
    return std::max(10, std::min(97, v));
}

int Player::overall() const {
    switch (pos) {
    case POS_GK: return keep;
    case POS_DF: return (tackle * 2 + speed + pass) / 4;
    case POS_MF: return (pass * 2 + speed + shoot + tackle) / 5;
    default: return (shoot * 2 + speed * 2 + pass) / 5;
    }
}

// ------------------------------------------------------------------ Formations
const Formation FORMATIONS[] = {
    { "4-4-2", { .14f, .38f, .62f, .86f, .12f, .38f, .62f, .88f, .38f, .62f },
               { .20f, .17f, .17f, .20f, .44f, .40f, .40f, .44f, .66f, .66f }, { 1,1,1,1,2,2,2,2,3,3 } },
    { "4-3-3", { .14f, .38f, .62f, .86f, .28f, .50f, .72f, .18f, .50f, .82f },
               { .20f, .17f, .17f, .20f, .42f, .36f, .42f, .64f, .68f, .64f }, { 1,1,1,1,2,2,2,3,3,3 } },
    { "4-5-1", { .14f, .38f, .62f, .86f, .10f, .32f, .50f, .68f, .90f, .50f },
               { .20f, .17f, .17f, .20f, .44f, .40f, .36f, .40f, .44f, .66f }, { 1,1,1,1,2,2,2,2,2,3 } },
    { "5-3-2", { .08f, .30f, .50f, .70f, .92f, .30f, .50f, .70f, .38f, .62f },
               { .26f, .17f, .15f, .17f, .26f, .42f, .40f, .42f, .64f, .64f }, { 1,1,1,1,1,2,2,2,3,3 } },
    { "3-5-2", { .28f, .50f, .72f, .08f, .32f, .50f, .68f, .92f, .38f, .62f },
               { .18f, .16f, .18f, .44f, .40f, .36f, .40f, .44f, .66f, .66f }, { 1,1,1,2,2,2,2,2,3,3 } },
    { "4-2-3-1", { .14f, .38f, .62f, .86f, .38f, .62f, .18f, .50f, .82f, .50f },
                 { .20f, .17f, .17f, .20f, .34f, .34f, .52f, .52f, .52f, .68f }, { 1,1,1,1,2,2,2,2,3,3 } },
    { "3-4-3", { .28f, .50f, .72f, .10f, .38f, .62f, .90f, .20f, .50f, .80f },
               { .18f, .16f, .18f, .44f, .40f, .40f, .44f, .64f, .68f, .64f }, { 1,1,1,2,2,2,2,3,3,3 } },
};
const int NUM_FORMATIONS = sizeof(FORMATIONS) / sizeof(FORMATIONS[0]);

// ------------------------------------------------------------------ World
int World::nationIndex(const char* code) const {
    for (int i = 0; i < NUM_NATIONS; i++) if (!strcmp(NATIONS[i].code, code)) return i;
    return -1;
}

static bool isLight(unsigned c) {
    int r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255;
    return (r * 299 + g * 587 + b * 114) / 1000 > 150;
}

void makeKits(Team& t, unsigned s1, unsigned s2, unsigned sh, int pat) {
    t.home.shirt = s1; t.home.shirt2 = s2; t.home.shorts = sh; t.home.pattern = pat;
    t.home.socks = (pat == KP_PLAIN) ? s1 : s2;
    if (isLight(s1)) { t.away.shirt = (s2 != s1 && !isLight(s2)) ? s2 : 0x1B2A63; t.away.shirt2 = 0xFFFFFF; t.away.shorts = t.away.shirt; }
    else { t.away.shirt = 0xFFFFFF; t.away.shirt2 = s1; t.away.shorts = 0xFFFFFF; }
    t.away.socks = t.away.shirt;
    t.away.pattern = KP_PLAIN;
}

static std::map<std::string, int> g_dbIndex;
int dbClubIndex(const char* key) {
    if (!key || !*key) return -1;
    if (g_dbIndex.empty()) for (int i = 0; i < NUM_DBCLUBS; i++) g_dbIndex[DBCLUBS[i].name] = i;
    auto it = g_dbIndex.find(key);
    return it == g_dbIndex.end() ? -1 : it->second;
}

int makeClubFromDef(World& w, const ClubDef& d, int nat, int culture) {
    Team t;
    t.name = sanitize(d.name); t.shortName = sanitize(d.shortName); t.kind = TK_CLUB; t.nation = nat;
    t.dbClub = dbClubIndex(d.dbKey);
    t.rating = (float)(t.dbClub >= 0 ? DBCLUBS[t.dbClub].rating : d.rating);
    t.culture = culture;
    t.stadium = sanitize(d.stadium && *d.stadium ? d.stadium : "");
    if (t.stadium.empty()) t.stadium = "Stade de " + t.name;
    makeKits(t, d.shirt, d.shirt2, d.shorts, d.pattern);
    t.seed = hashStr(t.name + (d.dbKey ? d.dbKey : ""));
    t.formation = (int)(t.seed % NUM_FORMATIONS);
    w.teams.push_back(t);
    return (int)w.teams.size() - 1;
}

void buildBasePyramids(World& w);    // pyramid.cpp

static const char* stadiumOfNation(const char* code) {
    for (int i = 0; i < NUM_NATION_STADIUMS; i++) if (!strcmp(NATION_STADIUMS[i].code, code)) return NATION_STADIUMS[i].stadium;
    return nullptr;
}


// petits championnats européens : clubs supplémentaires (au moins 7 par pays) pour les places européennes et l'Intertoto
struct SmallLeague { const char* code; const char* prefix; const char* cities; };
static const SmallLeague SMALL_LEAGUES[] = {
    { "ISR", "Maccabi|Hapoël|Beitar|Bnei", "Tel-Aviv;Haïfa;Beer-Sheva;Netanya;Ashdod;Petah-Tikva;Jérusalem" },
    { "CRO", "NK|HNK", "Zagreb;Split;Rijeka;Osijek;Varaždin;Gorica;Šibenik" },
    { "SRB", "FK|OFK", "Belgrade;Novi Sad;Niš;Kragujevac;Subotica;Čukarički;Vojvodina" },
    { "HUN", "FC|Vasas|Újpest", "Budapest;Debrecen;Győr;Szeged;Pécs;Székesfehérvár;Paks" },
    { "UKR", "FK|FC", "Kiev;Kharkiv;Odessa;Lviv;Dnipro;Poltava;Zaporijia" },
    { "AZE", "FK|FC", "Bakou;Gandja;Sumgaït;Lankaran;Sabail;Zira;Chéki" },
    { "SVN", "NK", "Ljubljana;Maribor;Celje;Koper;Domžale;Mura;Bravo" },
    { "SVK", "MFK|FK|ŠK", "Bratislava;Žilina;Trnava;Košice;Nitra;Ružomberok;Trenčín" },
    { "BUL", "PFK|FK", "Sofia;Plovdiv;Varna;Bourgas;Razgrad;Lovech;Stara Zagora" },
    { "KAZ", "FK|FC", "Astana;Almaty;Karaganda;Chimkent;Aktobe;Pavlodar;Kostanaï" },
    { "MDA", "FC|CS", "Chișinău;Tiraspol;Bălți;Orhei;Sheriff;Zimbru;Petrocub" },
    { "BIH", "FK|NK", "Sarajevo;Mostar;Zenica;Tuzla;Banja Luka;Široki Brijeg;Velež" },
    { "FIN", "FC|IFK|HJK", "Helsinki;Tampere;Turku;Kuopio;Vaasa;Lahti;Mariehamn" },
    { "ARM", "FC", "Erevan;Gyumri;Vanadzor;Abovyan;Ararat;Alashkert;Noah" },
    { "ISL", "UMF|KR|FH", "Reykjavik;Hafnarfjörður;Kópavogur;Akureyri;Keflavík;Garðabær;Vestmannaeyjar" },
    { "KVX", "KF|FC", "Pristina;Prizren;Gjilan;Peja;Drita;Ballkani;Llapi" },
    { "NIR", "FC", "Belfast;Lurgan;Coleraine;Larne;Portadown;Ballymena;Dungannon" },
    { "LVA", "FK|FS", "Riga;Liepāja;Daugavpils;Jelgava;Valmiera;Ventspils;Jūrmala" },
    { "FRO", "B36|HB|KÍ|NSÍ", "Tórshavn;Klaksvík;Runavík;Tvøroyri;Vágur;Fuglafjørður;Sandavágur" },
    { "GEO", "FC|Dinamo", "Tbilissi;Batoumi;Koutaïssi;Roustavi;Zougdidi;Gori;Telavi" },
    { "ALB", "KF|FK", "Tirana;Durrës;Shkodër;Elbasan;Vlorë;Korçë;Laçi" },
    { "BLR", "FC|FK", "Minsk;Borisov;Gomel;Brest;Soligorsk;Grodno;Vitebsk" },
    { "MKD", "FK|KF", "Skopje;Struga;Tetovo;Shkupi;Strumica;Kumanovo;Bitola" },
    { "LTU", "FK|FC", "Vilnius;Kaunas;Klaipėda;Panevėžys;Šiauliai;Marijampolė;Alytus" },
    { "LUX", "FC|Racing|Union", "Luxembourg;Differdange;Esch-sur-Alzette;Dudelange;Niederkorn;Hesperange;Wiltz" },
    { "MLT", "FC", "La Valette;Birkirkara;Sliema;Ħamrun;Floriana;Marsaxlokk;Hibernians" },
    { "MNE", "FK|OFK", "Podgorica;Nikšić;Budva;Bar;Titograd;Mornar;Dečić" },
    { "EST", "FC|JK", "Tallinn;Tartu;Narva;Pärnu;Paide;Kuressaare;Nõmme" },
    { "GIB", "FC", "Lincoln Red Imps;St Joseph's;Europa;Lions;Magpies;Glacis;Manchester 62" },
    { "WAL", "FC|Town", "Bala;Connah's Quay;Bangor;Newtown;Aberystwyth;Caernarfon;Penybont" },
    { "AND", "FC|UE", "Andorre-la-Vieille;Santa Coloma;Encamp;Escaldes;Sant Julià;Ordino;La Massana" },
    { "SMR", "SS|SP|AC", "Serravalle;Domagnano;Faetano;Fiorentino;Murata;Tre Penne;Cosmos" },
    { "CZE", "FK|SK|FC", "Prague;Plzeň;Ostrava;Olomouc;Liberec;Jablonec;Mladá Boleslav" },
    { "GRE", "PAS|AE|FC", "Athènes;Thessalonique;Le Pirée;Patras;Volos;Héraklion;Larissa" },
    { "CYP", "AEL|AEK|APOEL", "Nicosie;Limassol;Larnaca;Paphos;Famagouste;Paralimni;Aradippou" },
};
static std::vector<std::string> splitOn(const char* s, char sep) {
    std::vector<std::string> v; std::string cur;
    for (const char* p = s; ; p++) { if (*p == sep || !*p) { v.push_back(cur); cur.clear(); if (!*p) break; } else cur += *p; }
    return v;
}
static void addSmallLeagueClubs(World& w) {
    for (auto& L : SMALL_LEAGUES) {
        auto& lst = w.countryClubs[L.code];
        int nat = w.nationIndex(L.code);
        if (nat < 0 || lst.size() >= 7) continue;
        bool real = false;      // championnat réel disponible (section 10) : pas de clubs génériques
        for (int l = 0; l < NUM_EXT_LEAGUES; l++) if (!strcmp(EXT_LEAGUES[l].country, L.code) && EXT_LEAGUES[l].tier == 1) real = true;
        if (real) continue;
        float lo = 60; for (int t : lst) lo = std::min(lo, w.teams[t].rating);
        if (lst.empty()) lo = 45;
        std::set<std::string> have; for (int t : lst) have.insert(w.teams[t].name);
        auto pre = splitOn(L.prefix, '|'), cities = splitOn(L.cities, ';');
        for (size_t k = 0; k < cities.size() && lst.size() < 7; k++) {
            std::string city = sanitize(cities[k].c_str());
            bool dup = false; for (auto& h : have) if (h.find(city) != std::string::npos) dup = true;
            if (dup) continue;
            Team t;
            t.name = sanitize(pre[k % pre.size()].c_str()) + " " + city;
            t.shortName = ""; for (char c : city) { if (isalpha((unsigned char)c)) t.shortName += (char)toupper(c); if (t.shortName.size() >= 3) break; }
            t.kind = TK_CLUB; t.nation = nat; t.culture = NATIONS[nat].culture; t.town = city;
            t.rating = std::max(20.f, lo - 2.f - (float)(lst.size() % 4));
            t.stadium = "Stade de " + city;
            uint32_t h = hashStr(t.name);
            static const unsigned C[] = { 0xE2001A, 0x0055A4, 0x00843D, 0xFFE500, 0xFFFFFF, 0x000000, 0xF47920, 0x7B1E2B, 0x3FA9F5, 0x5B2C83 };
            unsigned c1 = C[h % 10], c2 = C[(h / 10) % 10]; if (c2 == c1) c2 = c1 == 0xFFFFFF ? 0x000000 : 0xFFFFFF;
            makeKits(t, c1, c2, c1, 0);
            t.seed = h; t.formation = (int)(h % NUM_FORMATIONS);
            w.teams.push_back(t);
            lst.push_back((int)w.teams.size() - 1);
            have.insert(t.name);
        }
    }
}

// après chargement d'une sauvegarde : clubs étrangers hors championnats simulés, par pays
void World::rebuildCountryClubs(const std::vector<Pyramid>& pyr) {
    std::vector<char> inPyr(teams.size(), 0);
    for (auto& P : pyr) for (auto& pl : P.pools) for (int t : pl.clubs) if (t >= 0 && t < (int)teams.size()) inPyr[t] = 1;
    countryClubs.clear();
    for (int i = firstClub; i < (int)teams.size(); i++) {
        const Team& t = teams[i];
        if (inPyr[i] || t.kind != TK_CLUB || t.parent >= 0 || t.youth || t.custom || t.nation < 0) continue;
        if (i == omReps[0] || i == omReps[1] || i == omReps[2]) continue;
        std::string c = NATIONS[t.nation].code;
        if (c == "FRA") continue;
        countryClubs[c].push_back(i);
    }
}

void World::build() {
    teams.clear();
    teams.reserve(20000);
    euroPool.clear(); worldPool.clear(); countryClubs.clear();
    for (auto& l : leagueClubs) l.clear();
    for (int i = 0; i < NUM_NATIONS; i++) {
        const NationDef& n = NATIONS[i];
        Team t;
        t.name = sanitize(n.name); t.shortName = n.code; t.kind = TK_NATION; t.nation = i;
        float r = (float)n.rating;
        if (NATION_DB_RATING[i] > 0) r = std::max(r - 6.f, std::min(r + 6.f, (r + NATION_DB_RATING[i]) * 0.5f));
        t.rating = r; t.culture = n.culture;
        const char* st = stadiumOfNation(n.code);
        t.stadium = st ? sanitize(st) : "Stade national (" + t.name + ")";
        makeKits(t, n.shirt, n.shirt2, n.shorts, n.pattern);
        t.seed = hashStr(t.name) ^ 0xA5A5;
        t.formation = (int)(t.seed % 3);
        teams.push_back(t);
    }
    firstClub = (int)teams.size();
    for (int l = 0; l < NUM_LEAGUES; l++) {
        const LeagueDef& L = LEAGUES[l];
        int nat = nationIndex(L.country);
        for (int c = 0; c < L.numClubs; c++) leagueClubs[l].push_back(makeClubFromDef(*this, L.clubs[c], nat, L.culture));
    }
    for (int c = 0; c < NUM_EURO_POOL; c++) {
        const ClubDef& d = EURO_POOL[c];
        int nat = nationIndex(d.dept);
        int id = makeClubFromDef(*this, d, nat, nat >= 0 ? NATIONS[nat].culture : CU_EN);
        euroPool.push_back(id);
        countryClubs[d.dept].push_back(id);
    }
    for (int c = 0; c < NUM_WORLD_POOL; c++) {
        const ClubDef& d = WORLD_POOL[c];
        int nat = nationIndex(d.dept);
        int id = makeClubFromDef(*this, d, nat, nat >= 0 ? NATIONS[nat].culture : CU_LATAM);
        worldPool.push_back(id);
        countryClubs[d.dept].push_back(id);
    }
    // championnats complémentaires (section 10) : un club déjà créé (clubs européens / sud-américains) est repris
    {
        std::map<std::string, int> byName;
        for (int i = firstClub; i < (int)teams.size(); i++) byName.emplace(teams[i].name, i);
        extLeagueClubs.assign(NUM_EXT_LEAGUES, {});
        for (int l = 0; l < NUM_EXT_LEAGUES; l++) {
            const ExtLeagueDef& L = EXT_LEAGUES[l];
            int nat = nationIndex(L.country);
            for (auto& cs : splitOn(L.clubs, ';')) {
                auto f = splitOn(cs.c_str(), '|');
                if (f.size() < 5 || f[0].empty()) continue;
                std::string nm = sanitize(f[0].c_str());
                auto it = byName.find(nm);
                if (it != byName.end()) { extLeagueClubs[l].push_back(it->second); continue; }
                ClubDef d{};
                d.name = f[0].c_str(); d.shortName = f[1].c_str(); d.rating = atoi(f[2].c_str());
                unsigned c1 = (unsigned)strtoul(f[3].c_str(), nullptr, 16), c2 = (unsigned)strtoul(f[4].c_str(), nullptr, 16);
                d.shirt = c1; d.shirt2 = c2; d.shorts = c1 == 0xFFFFFF ? c2 : (hashStr(nm) % 3 == 0 ? c1 : c2 == 0xFFFFFF ? 0xFFFFFF : c1);
                d.pattern = (int)(hashStr(nm + "p") % 5 == 0 ? KP_VSTRIPES : KP_PLAIN);
                d.dept = ""; d.stadium = f.size() > 5 ? f[5].c_str() : ""; d.dbKey = "";
                int id = makeClubFromDef(*this, d, nat, nat >= 0 ? NATIONS[nat].culture : CU_EN);
                if (f.size() <= 5) teams[id].stadium = "Stade " + nm;
                byName.emplace(nm, id);
                extLeagueClubs[l].push_back(id);
            }
        }
    }
    // représentants d'outre-mer en Coupe de France (hors championnats simulés)
    {
        struct OM { const char* name; const char* sh; const char* st; unsigned c1, c2; float r; };
        static const OM oms[3] = { { "AS Magenta", "MAG", "Stade Numa-Daly", 0xE2001A, 0xFFFFFF, 36 },
                                   { "AS Pirae", "PIR", "Stade Pater", 0x0055A4, 0xFFFFFF, 35 },
                                   { "AS Saint-Pierraise", "ASSP", "Stade John-Girardin", 0x00843D, 0xFFFFFF, 26 } };
        int fra = nationIndex("FRA");
        for (int i = 0; i < 3; i++) {
            int ex = -1;        // déjà créé par un championnat réel (Nouvelle-Calédonie, Tahiti)
            for (int k = firstClub; k < (int)teams.size(); k++) if (teams[k].name == sanitize(oms[i].name)) ex = k;
            if (ex >= 0) { omReps[i] = ex; continue; }
            Team t; t.name = sanitize(oms[i].name); t.shortName = oms[i].sh; t.stadium = sanitize(oms[i].st); t.kind = TK_CLUB; t.nation = fra;
            t.rating = oms[i].r; t.culture = CU_FR; t.seed = hashStr(t.name) ^ 0x5151;
            makeKits(t, oms[i].c1, oms[i].c2, oms[i].c2, 0);
            t.town = i == 0 ? "Nouméa" : i == 1 ? "Pirae" : "Saint-Pierre";
            omReps[i] = (int)teams.size();
            teams.push_back(t);
        }
    }
    buildBasePyramids(*this);
    addSmallLeagueClubs(*this);
    baseCount = (int)teams.size();
    loadCustomClubs();
    assignDistricts(*this);
    applyPresidents();
}

std::string World::clubBaseName(int team) const {
    const Team& t = teams[team];
    if (t.parent >= 0) return teams[t.parent].name;
    return t.name;
}

int World::addCustomClub(const Team& t) {
    teams.push_back(t);
    teams.back().custom = true;
    return (int)teams.size() - 1;
}

// ------------------------------------------------------------------ Effectifs
static std::vector<std::string> splitWords(const char* s) {
    std::vector<std::string> out; std::string cur;
    for (const char* p = s; ; p++) {
        if (*p == ' ' || *p == 0) { if (!cur.empty()) out.push_back(cur); cur.clear(); if (!*p) break; }
        else cur += *p;
    }
    return out;
}

struct NameCache { std::vector<std::string> first, last; };
static NameCache& namesFor(int cu) {
    static NameCache cache[NUM_CULTURES];
    static bool init = false;
    if (!init) {
        for (int i = 0; i < NUM_CULTURES; i++) {
            for (auto& w : splitWords(NAME_POOLS[i].first)) cache[i].first.push_back(sanitize(w.c_str()));
            for (auto& w : splitWords(NAME_POOLS[i].last)) cache[i].last.push_back(sanitize(w.c_str()));
        }
        init = true;
    }
    return cache[cu];
}

static int cultureSkin(int cu, Rng& r) {
    float x = r.f();
    switch (cu) {
    case CU_AFW: case CU_AFE: return x < 0.9f ? 3 : 2;
    case CU_BR: return x < 0.35f ? 3 : (x < 0.65f ? 2 : (x < 0.85f ? 1 : 0));
    case CU_FR: case CU_EN: case CU_NL: case CU_PT: return x < 0.3f ? 3 : (x < 0.45f ? 2 : (x < 0.6f ? 1 : 0));
    case CU_ARAB: case CU_TR: case CU_PERS: case CU_IND: return x < 0.6f ? 2 : 1;
    case CU_LATAM: case CU_ES: case CU_IT: case CU_GR: return x < 0.2f ? 2 : (x < 0.6f ? 1 : 0);
    case CU_JP: case CU_KR: case CU_CN: case CU_SEA: return 1;
    case CU_PAC: return 2;
    default: return x < 0.1f ? 1 : 0;
    }
}

static int clampi(int v, int a, int b) { return v < a ? a : (v > b ? b : v); }

static void generateFake(Team& t, int count, std::vector<Player>& out) {
    Rng r(t.seed * 2654435761ULL + 12345);
    static const int POS_LIST[20] = { POS_GK, POS_GK, POS_DF, POS_DF, POS_DF, POS_DF, POS_DF, POS_DF, POS_DF,
                                      POS_MF, POS_MF, POS_MF, POS_MF, POS_MF, POS_MF, POS_FW, POS_FW, POS_FW, POS_FW, POS_FW };
    std::set<std::string> used;
    // équilibrage : le onze type d'un club généré vaut à peu près sa note (les 11 meilleurs d'un effectif de 20 sont au-dessus de la moyenne)
    float base = t.rating - (t.kind == TK_CLUB && !t.youth ? 3.0f : 0.0f);
    for (int i = 0; i < count; i++) {
        Player p;
        p.pos = (uint8_t)POS_LIST[i % 20];
        int cu = t.culture;
        if (t.kind == TK_CLUB) {
            float foreign = t.nation >= 0 && !strcmp(NATIONS[t.nation].code, "FRA") ? 0.25f : 0.3f + (t.rating - 60) * 0.008f;
            if (r.chance(foreign)) {
                static const int POP[] = { CU_BR, CU_FR, CU_ES, CU_PT, CU_AFW, CU_AFE, CU_ARAB, CU_NL, CU_DE, CU_LATAM, CU_BALK, CU_NORD, CU_SLAV, CU_EN, CU_IT, CU_JP, CU_KR };
                cu = POP[r.range(0, (int)(sizeof(POP) / sizeof(int)) - 1)];
            }
        }
        NameCache& nc = namesFor(cu);
        std::string nm;
        for (int tries = 0; tries < 20; tries++) {
            std::string first = nc.first[r.range(0, (int)nc.first.size() - 1)];
            std::string last = nc.last[r.range(0, (int)nc.last.size() - 1)];
            // initiale (UTF-8 : on prend le premier caractère complet)
            int len = 1; unsigned char c0 = (unsigned char)first[0];
            if (c0 >= 0xC0) len = (c0 >= 0xE0) ? 3 : 2;
            nm = first.substr(0, len) + ". " + last;
            if (!used.count(nm)) break;
        }
        used.insert(nm);
        p.name = nm;
        float ov = base + r.frange(-7, 7) - (i >= 11 ? 2 : 0);
        int o = (int)ov;
        auto v = [&](int d) { return (uint8_t)clampi(o + d + r.range(-6, 6), 5, 99); };
        switch (p.pos) {
        case POS_GK: p.keep = v(4); p.speed = v(-15); p.shoot = v(-35); p.pass = v(-20); p.tackle = v(-30); break;
        case POS_DF: p.keep = 10; p.tackle = v(8); p.speed = v(0); p.pass = v(-5); p.shoot = v(-18); break;
        case POS_MF: p.keep = 10; p.pass = v(8); p.shoot = v(-2); p.tackle = v(-4); p.speed = v(0); break;
        default:     p.keep = 10; p.shoot = v(8); p.speed = v(5); p.pass = v(-4); p.tackle = v(-22); break;
        }
        p.stamina = v(0);
        p.age = (uint8_t)r.range(17, 34);
        p.pot = (uint8_t)std::min(99, (int)p.overall() + (p.age < 23 ? r.range(0, 12) : r.range(0, 2)));
        p.skin = (uint8_t)cultureSkin(cu, r);
        p.hair = (uint8_t)(p.skin >= 2 ? (r.chance(0.8f) ? 0 : 1) : r.range(0, 4));
        out.push_back(p);
    }
}

void generateFakeSquad(Team& t, int count, std::vector<Player>& out) { generateFake(t, count, out); }

static int realNumbersFix(std::vector<Player>& sq) {
    std::vector<bool> used(100, false);
    for (auto& p : sq) if (p.num > 0 && p.num < 100 && !used[p.num]) used[p.num] = true; else p.num = 0;
    int n = 1;
    for (auto& p : sq) if (p.num == 0) { while (n < 99 && used[n]) n++; p.num = (uint8_t)n; used[n] = true; }
    return 0;
}

static Player fromRec(const PlayerRec& r, Rng& rng) {
    Player p;
    p.name = sanitize(r.name);
    p.pos = r.pos; p.num = r.num;
    p.speed = r.speed; p.shoot = r.shoot; p.pass = r.pass; p.tackle = r.tackle; p.keep = r.keep; p.stamina = r.stamina;
    p.nation = r.nation;
    p.age = r.age ? r.age : 25; p.pot = r.potential;
    int cu = r.nation >= 0 ? NATIONS[r.nation].culture : CU_EN;
    p.skin = (uint8_t)cultureSkin(cu, rng);
    p.hair = (uint8_t)(p.skin >= 2 ? (rng.chance(0.8f) ? 0 : 1) : rng.range(0, 4));
    return p;
}

// joueurs réels par sélection (triés : sélectionnés d'abord puis note)
static std::vector<std::vector<int>>& nationPlayers() {
    static std::vector<std::vector<int>> v;
    if (v.empty()) {
        v.resize(NUM_NATIONS);
        for (int i = 0; i < NUM_PLAYERS; i++) if (PLAYERS[i].nation >= 0 && PLAYERS[i].nation < NUM_NATIONS) v[PLAYERS[i].nation].push_back(i);
        for (auto& l : v) std::stable_sort(l.begin(), l.end(), [](int a, int b) {
            if (PLAYERS[a].intl != PLAYERS[b].intl) return PLAYERS[a].intl > PLAYERS[b].intl;
            return PLAYERS[a].overall > PLAYERS[b].overall; });
    }
    return v;
}

void World::generateSquad(Team& t) {
    Rng r(t.seed * 2654435761ULL + 12345);
    t.squad.clear();
    if (t.kind == TK_CLUB && t.dbClub >= 0) {
        const DbClub& c = DBCLUBS[t.dbClub];
        for (int i = 0; i < c.count; i++) t.squad.push_back(fromRec(PLAYERS[c.first + i], r));
    } else if (t.kind == TK_NATION && !t.youth) {
        auto& lst = nationPlayers()[t.nation];
        // équilibre des postes : 3 gardiens, puis les meilleurs
        int gk = 0;
        for (int idx : lst) {
            const PlayerRec& pr = PLAYERS[idx];
            if (pr.pos == POS_GK) { if (gk >= 3) continue; gk++; }
            if ((int)t.squad.size() >= 23) break;
            t.squad.push_back(fromRec(pr, r));
        }
        if (!t.squad.empty()) { int k = 1; for (auto& p : t.squad) p.num = (uint8_t)(k++); }
    }
    int have = (int)t.squad.size();
    bool hasGk = false; for (auto& p : t.squad) if (p.pos == POS_GK) hasGk = true;
    if (have < 18 || !hasGk) {
        std::vector<Player> fake;
        generateFake(t, 20, fake);
        for (auto& p : fake) { if (!hasGk && p.pos == POS_GK) { t.squad.push_back(p); hasGk = true; continue; } if ((int)t.squad.size() < 20) t.squad.push_back(p); }
    }
    if (t.youth == 6) {   // équipe féminine : joueuses de 18 à 35 ans, prénoms féminins
        int cu = t.culture >= 0 && t.culture < NUM_CULTURES ? t.culture : CU_FR;
        std::vector<std::string> fn; { std::string s = FEMALE_FIRST[cu], w; for (char c : s) { if (c == ' ') { if (!w.empty()) fn.push_back(w); w.clear(); } else w += c; } if (!w.empty()) fn.push_back(w); }
        for (auto& p : t.squad) {
            p.gender = 1;
            p.age = (uint8_t)r.range(18, 34);
            if (t.kind == TK_NATION) p.nation = (int16_t)t.nation;
            else if (t.nation >= 0 && r.chance(0.8f)) p.nation = (int16_t)t.nation;
            {   // niveau des joueuses générées : autour du niveau de l'équipe (les joueuses réelles restent les meilleures)
                int cap = (int)t.rating - 3 + r.range(-4, 3);
                int ov = p.overall();
                if (ov > cap && ov > 0) {
                    float f = (float)cap / ov;
                    auto sc = [&](uint8_t& v) { v = (uint8_t)std::max(10, (int)(v * f)); };
                    sc(p.speed); sc(p.shoot); sc(p.pass); sc(p.tackle); sc(p.stamina); if (p.pos == POS_GK) sc(p.keep);
                }
            }
            size_t sp = p.name.find(' ');
            std::string last = sp == std::string::npos ? p.name : p.name.substr(sp + 1);
            if (!fn.empty()) p.name = fn[r.range(0, (int)fn.size() - 1)] + " " + last;
            p.pot = (uint8_t)std::min(99, p.overall() + std::max(0, 27 - p.age) + r.range(0, 4));
            p.contract = 2;
        }
    } else if (t.youth) {   // équipe U19 : joueurs de 16 à 19 ans (U17 : 15 et 16 ans), fort potentiel ; sélections Espoirs (U21) et olympique (U23)
        int k = 0;
        for (auto& p : t.squad) {
            p.age = (uint8_t)(t.youth == 5 ? (k < 3 ? r.range(24, 31) : r.range(20, 23)) : t.youth == 4 ? r.range(18, 21) : t.youth == 3 ? r.range(13, 14) : t.youth == 2 ? r.range(15, 16) : r.range(16, 19));
            k++;
            if (t.kind == TK_NATION) p.nation = (int16_t)t.nation;
            p.pot = (uint8_t)std::min(99, p.overall() + r.range(2, 14) - (p.age - 16) + (r.chance(0.12f) ? r.range(6, 14) : 0));
            p.contract = 2;
        }
    }
    injectWomenStars(t, r);
    realNumbersFix(t.squad);
    for (auto& p : t.squad) p.id = g_world.nextPid++;
    applyPlayerEdits(t);
    t.squadGen = true;
}

Player World::makeYouth(int team, int pos, float level) {
    Team& t = teams[team];
    Rng r(g_rng.next());
    std::vector<Player> tmp;
    Team copy; copy.seed = (uint32_t)r.next(); copy.rating = level; copy.culture = t.culture; copy.kind = t.kind; copy.nation = t.nation;
    generateFake(copy, 20, tmp);
    Player p = tmp[std::min(19, pos == POS_GK ? 0 : pos == POS_DF ? 3 + r.range(0, 5) : pos == POS_MF ? 9 + r.range(0, 5) : 15 + r.range(0, 4))];
    p.age = (uint8_t)r.range(17, 20);
    p.pot = (uint8_t)std::min(99, p.overall() + r.range(4, 18));
    std::vector<bool> used(100, false);
    for (auto& q : t.squad) if (q.num < 100) used[q.num] = true;
    p.num = 1; while (p.num < 99 && used[p.num]) p.num++;
    p.id = nextPid++;
    if (t.kind == TK_CLUB && t.nation >= 0 && (!t.youth || t.youth == 6)) p.nation = (int16_t)t.nation;   // formé au club : nationalité du pays
    if (t.youth == 6) {   // section féminine : une jeune joueuse (prénom féminin)
        p.gender = 1;
        int cu = t.culture >= 0 && t.culture < NUM_CULTURES ? t.culture : CU_FR;
        std::vector<std::string> fn; { std::string s = FEMALE_FIRST[cu], w; for (char c : s) { if (c == ' ') { if (!w.empty()) fn.push_back(w); w.clear(); } else w += c; } if (!w.empty()) fn.push_back(w); }
        size_t sp = p.name.find(' ');
        if (!fn.empty()) p.name = fn[r.range(0, (int)fn.size() - 1)] + " " + (sp == std::string::npos ? p.name : p.name.substr(sp + 1));
    }
    {   // centre de formation du club : meilleurs jeunes
        int club = t.parent >= 0 ? t.parent : team;
        int ac = club >= 0 && club < (int)teams.size() ? teams[club].academy + teams[club].sta.annexYouth : 0;   // centre de formation + stade des jeunes
        if (ac > 0) {
            p.pot = (uint8_t)std::min(99, p.pot + ac * 2 + r.range(0, ac));
            auto up = [&](uint8_t& v) { v = (uint8_t)std::min(99, v + ac); };
            up(p.speed); up(p.shoot); up(p.pass); up(p.tackle); if (p.pos == POS_GK) up(p.keep);
        }
    }
    p.goals = p.apps = p.assists = 0; p.suspended = p.injured = p.yellows = 0;
    return p;
}

int World::findPlayer(int pid, int* idx) const {
    for (int t = 0; t < (int)teams.size(); t++) {
        if (!teams[t].squadGen) continue;
        const auto& sq = teams[t].squad;
        for (int i = 0; i < (int)sq.size(); i++) if (sq[i].id == pid) { if (idx) *idx = i; return t; }
    }
    return -1;
}

void World::ensureSquad(int team) {
    if (team < 0 || team >= (int)teams.size()) return;
    Team& t = teams[team];
    if (!t.squadGen) { generateSquad(t); applyContracts(t, false); }
}

std::vector<int> World::pickLineup(int team, int formation) const {
    const Team& t = teams[team];
    const Formation& F = FORMATIONS[formation];
    std::vector<int> out;
    std::vector<bool> used(t.squad.size(), false);
    // extra-communautaires : quota sur la feuille de match (clubs français)
    int neLimit = nonEuLimit(team), neUsed = 0;
    std::vector<bool> ne(t.squad.size(), false);
    if (neLimit < 99) for (size_t i = 0; i < t.squad.size(); i++) ne[i] = isNonEU(t.squad[i], team);
    auto availRaw = [&](int i) { return !used[i] && t.squad[i].suspended <= 0 && t.squad[i].injured <= 0; };
    auto avail = [&](int i) { return availRaw(i) && (!ne[i] || neUsed < neLimit); };
    auto take = [&](int i) { used[i] = true; if (ne[i]) neUsed++; };
    // valeur de sélection : niveau, pénalisé si le joueur est fatigué
    auto sv = [&](int i) { int c = playerCond(team, t.squad[i]); return t.squad[i].overall() * 4 - std::max(0, 72 - c); };
    auto best = [&](int pos) {
        int b = -1, bv = -1000;
        for (int i = 0; i < (int)t.squad.size(); i++)
            if (avail(i) && t.squad[i].pos == pos && sv(i) > bv) { bv = sv(i); b = i; }
        if (b < 0) for (int i = 0; i < (int)t.squad.size(); i++)
            if (avail(i) && sv(i) > bv && t.squad[i].pos != POS_GK) { bv = sv(i); b = i; }
        if (b < 0) for (int i = 0; i < (int)t.squad.size(); i++) if (!used[i]) { b = i; break; }
        if (b >= 0) take(b);
        return b;
    };
    // titulaires choisis par l'entraîneur : on les place au poste le plus proche
    std::vector<int> manual;
    if (t.xi.size() == 11) {
        int mNe = 0;
        for (int id : t.xi) for (int i = 0; i < (int)t.squad.size(); i++) if (t.squad[i].id == id && avail(i) && (!ne[i] || mNe++ < neLimit)) manual.push_back(i);
    }
    if (!manual.empty()) {
        // gardien
        int gk = -1;
        for (int i : manual) if (t.squad[i].pos == POS_GK) { gk = i; break; }
        if (gk >= 0) take(gk); else gk = best(POS_GK);
        out.push_back(gk);
        std::vector<int> rest; for (int i : manual) if (i != gk && t.squad[i].pos != POS_GK) rest.push_back(i);
        std::vector<int> slot(10, -1);
        for (int pass = 0; pass < 2; pass++)
            for (int k = 0; k < 10; k++) {
                if (slot[k] >= 0) continue;
                int want = F.role[k] == 1 ? POS_DF : F.role[k] == 2 ? POS_MF : POS_FW;
                for (int i : rest) if (avail(i) && (pass == 1 || t.squad[i].pos == want)) { slot[k] = i; take(i); break; }
            }
        for (int k = 0; k < 10; k++) if (slot[k] < 0) slot[k] = best(F.role[k] == 1 ? POS_DF : F.role[k] == 2 ? POS_MF : POS_FW);
        for (int k = 0; k < 10; k++) out.push_back(slot[k]);
    } else {
        out.push_back(best(POS_GK));
        for (int i = 0; i < 10; i++) out.push_back(best(F.role[i] == 1 ? POS_DF : F.role[i] == 2 ? POS_MF : POS_FW));
    }
    // remplaçants : 1 gardien + 8 meilleurs restants
    int gk = -1;
    for (int i = 0; i < (int)t.squad.size(); i++) if (avail(i) && t.squad[i].pos == POS_GK) { gk = i; break; }
    if (gk >= 0) { take(gk); out.push_back(gk); }
    std::vector<int> rest;
    for (int i = 0; i < (int)t.squad.size(); i++) if (avail(i)) rest.push_back(i);
    std::sort(rest.begin(), rest.end(), [&](int a, int b) { return t.squad[a].overall() > t.squad[b].overall(); });
    for (int i = 0, n = 0; i < (int)rest.size() && n < 8; i++) if (avail(rest[i])) { out.push_back(rest[i]); take(rest[i]); n++; }
    // règle JFL (football féminin français) : nombre minimal de joueuses formées localement sur la feuille de match
    int jmin = jflMin(team);
    if (jmin > 0) {
        auto jfl = [&](int i) { return i >= 0 && isJfl(t.squad[i], team); };
        int have = 0; for (int i : out) if (jfl(i)) have++;
        for (int k = (int)out.size() - 1; k >= 1 && have < jmin; k--) {
            if (jfl(out[k]) || t.squad[out[k]].pos == POS_GK) continue;
            int bi = -1;
            for (int i = 0; i < (int)t.squad.size(); i++) if (!used[i] && availRaw(i) && jfl(i) && t.squad[i].pos != POS_GK && (bi < 0 || sv(i) > sv(bi))) bi = i;
            if (bi < 0) break;
            used[out[k]] = false; used[bi] = true; out[k] = bi; have++;
        }
    }
    return out;
}

// ------------------------------------------------------------------ version 11 : caractéristiques dérivées, infos personnelles
int Player::posi() const {
    if (positioning) return positioning;
    int v;
    switch (pos) {
    case POS_GK: v = (keep * 70 + pass * 30) / 100; break;
    case POS_DF: v = (tackle * 60 + pass * 20 + stamina * 20) / 100; break;
    case POS_MF: v = (pass * 50 + tackle * 25 + stamina * 25) / 100; break;
    default: v = (shoot * 45 + speed * 35 + pass * 20) / 100; break;
    }
    v += (int)((unsigned)id * 29u % 11u) - 5 + std::min(6, std::max(0, (int)age - 24));
    return std::max(10, std::min(97, v));
}
int Player::comp() const {
    if (composure) return composure;
    int v = overall() - 6 + std::min(10, std::max(-8, ((int)age - 23) * 2)) + (int)((unsigned)id * 41u % 13u) - 6;
    return std::max(10, std::min(97, v));
}

std::string randomFullName(int culture, uint32_t seed) {
    if (culture < 0 || culture >= NUM_CULTURES) culture = CU_FR;
    NameCache& nc = namesFor(culture);
    Rng r(seed * 2246822519ULL + 99);
    if (nc.first.empty() || nc.last.empty()) return "M. Dupont";
    return nc.first[r.range(0, (int)nc.first.size() - 1)] + " " + nc.last[r.range(0, (int)nc.last.size() - 1)];
}

struct NatCities { const char* code; const char* cities; };
static const NatCities NAT_CITIES[] = {
    { "BRA", "Rio de Janeiro;São Paulo;Belo Horizonte;Porto Alegre;Salvador;Recife" }, { "ARG", "Buenos Aires;Rosario;Córdoba;La Plata;Mendoza" },
    { "ESP", "Madrid;Barcelone;Séville;Valence;Bilbao;Malaga" }, { "POR", "Lisbonne;Porto;Braga;Coimbra;Setúbal" },
    { "ITA", "Rome;Milan;Naples;Turin;Florence;Gênes;Palerme" }, { "GER", "Berlin;Munich;Hambourg;Cologne;Dortmund;Stuttgart" },
    { "ENG", "Londres;Manchester;Liverpool;Birmingham;Leeds;Newcastle" }, { "NED", "Amsterdam;Rotterdam;La Haye;Eindhoven;Utrecht" },
    { "BEL", "Bruxelles;Anvers;Liège;Gand;Charleroi" }, { "SUI", "Zurich;Genève;Bâle;Berne;Lausanne" },
    { "SEN", "Dakar;Thiès;Saint-Louis;Ziguinchor" }, { "CIV", "Abidjan;Bouaké;Yamoussoukro;San-Pédro" }, { "CMR", "Douala;Yaoundé;Garoua;Bafoussam" },
    { "MAR", "Casablanca;Rabat;Marrakech;Fès;Tanger" }, { "ALG", "Alger;Oran;Constantine;Annaba" }, { "TUN", "Tunis;Sfax;Sousse;Bizerte" },
    { "MLI", "Bamako;Sikasso;Kayes" }, { "NGA", "Lagos;Abuja;Kano;Ibadan" }, { "GHA", "Accra;Kumasi;Tamale" },
    { "CRO", "Zagreb;Split;Rijeka;Osijek" }, { "SRB", "Belgrade;Novi Sad;Niš" }, { "POL", "Varsovie;Cracovie;Łódź;Wrocław;Poznań" },
    { "USA", "New York;Los Angeles;Chicago;Houston;Seattle" }, { "MEX", "Mexico;Guadalajara;Monterrey;Puebla" },
    { "URU", "Montevideo;Salto;Paysandú" }, { "COL", "Bogota;Medellín;Cali;Barranquilla" }, { "JPN", "Tokyo;Osaka;Yokohama;Nagoya" },
    { "KOR", "Séoul;Busan;Incheon;Daegu" }, { "SCO", "Glasgow;Édimbourg;Aberdeen;Dundee" }, { "TUR", "Istanbul;Ankara;Izmir;Bursa" },
    { "GRE", "Athènes;Thessalonique;Le Pirée;Patras" }, { "DEN", "Copenhague;Aarhus;Odense" }, { "SWE", "Stockholm;Göteborg;Malmö" },
    { "NOR", "Oslo;Bergen;Trondheim" }, { "AUT", "Vienne;Graz;Salzbourg;Linz" }, { "CZE", "Prague;Brno;Ostrava" }, { "UKR", "Kiev;Kharkiv;Odessa;Lviv" },
};

static std::string pickFrenchTown(Rng& r, int dept) {
    int d = dept;
    if (d < 0 || d >= NUM_DEPTS || r.chance(0.35f)) {
        int tot = 0; for (int i = 0; i < NUM_DEPTS; i++) tot += DEPTS[i].population / 1000;
        int x = r.range(0, std::max(1, tot) - 1);
        for (int i = 0; i < NUM_DEPTS; i++) { x -= DEPTS[i].population / 1000; if (x < 0) { d = i; break; } }
        if (d < 0) d = 0;
    }
    std::string s = DEPTS[d].towns;
    std::vector<std::string> towns;
    size_t p = 0;
    while (p < s.size() && towns.size() < 6) {
        size_t e = s.find(';', p); if (e == std::string::npos) e = s.size();
        std::string item = s.substr(p, e - p);
        size_t bar = item.find('|');
        towns.push_back(sanitize(item.substr(0, bar).c_str()));
        p = e + 1;
    }
    if (towns.empty()) return sanitize(DEPTS[d].name);
    int n = (int)towns.size();
    std::string t = towns[r.chance(0.6f) ? r.range(0, std::min(2, n - 1)) : r.range(0, n - 1)];
    return t + " (" + DEPTS[d].code + ")";
}

void playerBirth(const Player& p, int year, int& d, int& m, int& y, std::string& place, int culture, int dept) {
    Rng r((uint64_t)(uint32_t)p.id * 0x9E3779B1ULL + 7 + hashStr(p.name));
    int dd = r.range(1, 28), mm = r.range(1, 12);
    if (p.bday) dd = p.bday;
    if (p.bmonth) mm = p.bmonth;
    d = dd; m = mm;
    // saison y-(y+1) : âge au 1er janvier de y+1
    y = year + 1 - p.age - (mm >= 7 ? 1 : 0);
    if (!p.birthPlace.empty()) { place = p.birthPlace; return; }
    const char* code = p.nation >= 0 && p.nation < NUM_NATIONS ? NATIONS[p.nation].code : nullptr;
    bool fr = code ? !strcmp(code, "FRA") : (culture == CU_FR || culture < 0);
    if (fr) { place = pickFrenchTown(r, dept); return; }
    if (code) {
        for (auto& nc : NAT_CITIES) if (!strcmp(nc.code, code)) {
            std::vector<std::string> v; std::string cur;
            for (const char* q = nc.cities; ; q++) { if (*q == ';' || !*q) { v.push_back(cur); cur.clear(); if (!*q) break; } else cur += *q; }
            place = sanitize(v[r.range(0, std::min((int)v.size() - 1, r.chance(0.5f) ? 1 : 5))].c_str()) + " (" + sanitize(NATIONS[p.nation].name) + ")";
            return;
        }
        place = sanitize(NATIONS[p.nation].name);
        return;
    }
    place = "(inconnu)";
}

std::string presidentName(int team) {
    if (team < 0 || team >= (int)g_world.teams.size()) return "";
    const Team& t = g_world.teams[team];
    if (t.parent >= 0) return presidentName(t.parent);
    if (!t.president.empty()) return t.president;
    return randomFullName(t.culture, t.seed ^ 0x9E57u);
}
int presidentAge(int team) {
    if (team < 0 || team >= (int)g_world.teams.size()) return 55;
    const Team& t = g_world.teams[team];
    if (t.parent >= 0) return presidentAge(t.parent);
    if (t.presAge) return t.presAge;
    return 42 + (int)((t.seed ^ 0x51u) * 2654435761u % 33u);
}
std::string presidentPlace(int team) {
    if (team < 0 || team >= (int)g_world.teams.size()) return "";
    const Team& t = g_world.teams[team];
    if (t.parent >= 0) return presidentPlace(t.parent);
    if (!t.presPlace.empty()) return t.presPlace;
    if (t.culture == CU_FR && t.dept >= 0) { Rng r(t.seed ^ 0x7777u); return pickFrenchTown(r, t.dept); }
    if (!t.town.empty()) return t.town;
    return t.nation >= 0 ? sanitize(NATIONS[t.nation].name) : std::string();
}

void recordPlayerSeason(Player& p, int team, int year, bool moved) {
    if (p.apps <= 0 && p.goals <= 0 && !moved) return;
    PlayerSeason s; s.year = (int16_t)year; s.team = team; s.apps = p.apps; s.goals = p.goals; s.assists = p.assists; s.yel = p.sYel; s.red = p.sRed; s.moved = moved ? 1 : 0;
    // même club et même saison (transfert aller-retour) : cumul
    if (!p.hist.empty() && p.hist.back().year == s.year && p.hist.back().team == team) {
        auto& b = p.hist.back(); b.apps += s.apps; b.goals += s.goals; b.assists += s.assists; b.yel += s.yel; b.red += s.red;
    } else p.hist.push_back(s);
    if (p.hist.size() > 40) p.hist.erase(p.hist.begin());
    p.apps = p.goals = p.assists = 0; p.sYel = p.sRed = 0;
}
