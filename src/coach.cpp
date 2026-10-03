// Carrière de sélectionneur : campagnes successives (Coupe du monde, championnats continentaux, Ligue des nations),
// convocations des joueurs de la nation, pays organisateurs, stades des pays
#include "game.h"
#include "data.h"
#include "serial.h"
#include <map>
#include <set>
#include <cstring>
#include <cmath>

int addCompPublic(Season& S, Competition c);           // career.cpp
void ageSquads(int userTeam, std::vector<std::string>& news);

// ------------------------------------------------------------------ stades des pays
static const struct { const char* code; const char* venues; } HOST_VENUES[] = {
    { "FRA", "Stade de France (Saint-Denis);Stade Vélodrome (Marseille);Groupama Stadium (Lyon);Parc des Princes (Paris);Matmut Atlantique (Bordeaux);Stade Pierre-Mauroy (Lille);Allianz Riviera (Nice);Stade Geoffroy-Guichard (Saint-Étienne);Stade Bollaert-Delelis (Lens);Stadium de Toulouse" },
    { "ENG", "Wembley (Londres);Old Trafford (Manchester);Anfield (Liverpool);Tottenham Hotspur Stadium (Londres);Emirates Stadium (Londres);Etihad Stadium (Manchester);St James' Park (Newcastle);Villa Park (Birmingham);Stadium of Light (Sunderland)" },
    { "SCO", "Hampden Park (Glasgow);Celtic Park (Glasgow);Ibrox (Glasgow);Murrayfield (Édimbourg)" },
    { "WAL", "Principality Stadium (Cardiff);Cardiff City Stadium;Swansea.com Stadium" },
    { "IRL", "Aviva Stadium (Dublin);Croke Park (Dublin)" },
    { "NIR", "Windsor Park (Belfast);Casement Park (Belfast)" },
    { "ESP", "Santiago Bernabéu (Madrid);Camp Nou (Barcelone);Metropolitano (Madrid);La Cartuja (Séville);San Mamés (Bilbao);Mestalla (Valence);Anoeta (Saint-Sébastien);La Rosaleda (Malaga);Riazor (La Corogne);La Romareda (Saragosse)" },
    { "POR", "Estádio da Luz (Lisbonne);Estádio do Dragão (Porto);José Alvalade (Lisbonne);Estádio Municipal de Braga;Estádio Algarve (Faro)" },
    { "MAR", "Grand Stade Hassan II (Casablanca);Stade Prince Moulay Abdellah (Rabat);Grand Stade de Marrakech;Grand Stade d'Agadir;Grand Stade de Fès;Grand Stade de Tanger" },
    { "ITA", "Stadio Olimpico (Rome);San Siro (Milan);Allianz Stadium (Turin);Stadio Diego Armando Maradona (Naples);Artemio Franchi (Florence);Luigi Ferraris (Gênes);Stadio Olimpico Grande Torino (Turin);Dall'Ara (Bologne);Via del Mare (Lecce);Marc'Antonio Bentegodi (Vérone)" },
    { "TUR", "Atatürk Olimpiyat (Istanbul);Rams Park (Istanbul);Şükrü Saracoğlu (Istanbul);Tüpraş Stadyumu (Istanbul);Ankara Eryaman;Bursa Büyükşehir;Trabzon Papara Park;Antalya Stadyumu" },
    { "GER", "Olympiastadion (Berlin);Allianz Arena (Munich);Signal Iduna Park (Dortmund);Veltins-Arena (Gelsenkirchen);MHPArena (Stuttgart);Volksparkstadion (Hambourg);Deutsche Bank Park (Francfort);Merkur Spiel-Arena (Düsseldorf);RheinEnergieStadion (Cologne);Red Bull Arena (Leipzig)" },
    { "NED", "Johan Cruijff ArenA (Amsterdam);De Kuip (Rotterdam);Philips Stadion (Eindhoven);De Grolsch Veste (Enschede)" },
    { "BEL", "Stade Roi-Baudouin (Bruxelles);Stade Jan Breydel (Bruges);Stade Maurice-Dufrasne (Liège);Bosuil (Anvers)" },
    { "SUI", "Stade de Suisse (Berne);Parc Saint-Jacques (Bâle);Letzigrund (Zurich);Stade de Genève" },
    { "AUT", "Ernst-Happel-Stadion (Vienne);Red Bull Arena (Salzbourg);Wörthersee Stadion (Klagenfurt);Tivoli (Innsbruck)" },
    { "USA", "MetLife Stadium (New York);SoFi Stadium (Los Angeles);AT&T Stadium (Dallas);Mercedes-Benz Stadium (Atlanta);NRG Stadium (Houston);Arrowhead Stadium (Kansas City);Lincoln Financial Field (Philadelphie);Levi's Stadium (San Francisco);Lumen Field (Seattle);Hard Rock Stadium (Miami);Gillette Stadium (Boston)" },
    { "MEX", "Estadio Azteca (Mexico);Estadio Akron (Guadalajara);Estadio BBVA (Monterrey);Estadio Universitario (Monterrey)" },
    { "CAN", "BMO Field (Toronto);BC Place (Vancouver);Stade olympique (Montréal);Commonwealth Stadium (Edmonton)" },
    { "KSA", "King Fahd Stadium (Riyad);King Salman Stadium (Riyad);King Abdullah Sports City (Djeddah);Prince Mohammed bin Salman Stadium (Qiddiya);Aramco Stadium (Khobar);Stade de NEOM;King Khalid University Stadium (Abha);Prince Faisal bin Fahd Stadium (Riyad)" },
    { "QAT", "Lusail Stadium;Al Bayt Stadium (Al Khor);Khalifa International (Doha);Education City Stadium;Al Janoub (Al Wakrah);Ahmad bin Ali Stadium;Al Thumama (Doha);Stadium 974 (Doha)" },
    { "BRA", "Maracanã (Rio de Janeiro);Mané Garrincha (Brasília);Arena Corinthians (São Paulo);Mineirão (Belo Horizonte);Arena Fonte Nova (Salvador);Beira-Rio (Porto Alegre);Castelão (Fortaleza);Arena Pernambuco (Recife)" },
    { "ARG", "Monumental (Buenos Aires);La Bombonera (Buenos Aires);Mario Alberto Kempes (Córdoba);Estadio Único (La Plata);Estadio Malvinas Argentinas (Mendoza);Estadio Gigante de Arroyito (Rosario)" },
    { "CHI", "Estadio Nacional (Santiago);Estadio Monumental (Santiago);Estadio Sausalito (Viña del Mar);Estadio Ester Roa (Concepción)" },
    { "COL", "Metropolitano Roberto Meléndez (Barranquilla);El Campín (Bogota);Atanasio Girardot (Medellín);Pascual Guerrero (Cali)" },
    { "URU", "Centenario (Montevideo);Gran Parque Central (Montevideo);Campeón del Siglo (Montevideo)" },
    { "ECU", "Rodrigo Paz Delgado (Quito);Monumental Banco Pichincha (Guayaquil);Estadio Olímpico Atahualpa (Quito)" },
    { "PER", "Estadio Nacional de Lima;Estadio Monumental (Lima);Estadio de la UNSA (Arequipa)" },
    { "PAR", "Defensores del Chaco (Asunción);Estadio General Pablo Rojas (Asunción)" },
    { "JPN", "Stade national (Tokyo);Nissan Stadium (Yokohama);Saitama Stadium;Toyota Stadium;Panasonic Stadium Suita (Osaka)" },
    { "KOR", "Seoul World Cup Stadium;Munsu (Ulsan);Suwon World Cup Stadium;Daegu Stadium" },
    { "AUS", "Stadium Australia (Sydney);Melbourne Cricket Ground;Suncorp Stadium (Brisbane);Perth Stadium;Adelaide Oval" },
    { "RSA", "FNB Stadium (Johannesburg);Cape Town Stadium;Moses Mabhida (Durban);Ellis Park (Johannesburg);Loftus Versfeld (Pretoria)" },
    { "KEN", "Kasarani (Nairobi);Nyayo National Stadium (Nairobi);Talanta Stadium (Nairobi)" },
    { "UGA", "Mandela National Stadium (Kampala);Hoima City Stadium" },
    { "TAN", "Benjamin Mkapa Stadium (Dar es Salaam);Amaan Stadium (Zanzibar);New Arusha Stadium" },
    { "EGY", "Stade international du Caire;Stade Borg El Arab (Alexandrie);Stade d'Ismaïlia;Stade de la Nouvelle Capitale" },
    { "CIV", "Stade Alassane-Ouattara (Abidjan);Stade Félix Houphouët-Boigny (Abidjan);Stade de la Paix (Bouaké);Stade Charles Konan Banny (Yamoussoukro);Stade Laurent Pokou (San-Pédro)" },
    { "SEN", "Stade Abdoulaye-Wade (Diamniadio);Stade Léopold-Sédar-Senghor (Dakar);Stade Lat-Dior (Thiès)" },
    { "ALG", "Stade Nelson-Mandela (Baraki);Stade du 5-Juillet (Alger);Stade Miloud-Hadefi (Oran);Stade Hocine-Aït-Ahmed (Tizi Ouzou)" },
    { "NGA", "Godswill Akpabio Stadium (Uyo);Moshood Abiola Stadium (Abuja);Teslim Balogun Stadium (Lagos)" },
    { "CMR", "Stade Paul-Biya (Yaoundé);Stade Japoma (Douala);Stade Ahmadou-Ahidjo (Yaoundé);Stade Roumdé Adjia (Garoua)" },
    { "NZL", "Eden Park (Auckland);Sky Stadium (Wellington);Forsyth Barr Stadium (Dunedin);Waikato Stadium (Hamilton)" },
};

std::vector<std::string> nationVenues(int nation) {
    std::vector<std::string> v;
    if (nation < 0 || nation >= NUM_NATIONS) return v;
    auto add = [&](const std::string& s) { if (!s.empty() && std::find(v.begin(), v.end(), s) == v.end()) v.push_back(s); };
    const char* code = NATIONS[nation].code;
    for (auto& h : HOST_VENUES) if (!strcmp(h.code, code)) {
        std::string all = h.venues, cur;
        for (char c : all + ";") { if (c == ';') { add(cur); cur.clear(); } else cur += c; }
    }
    if (v.empty()) add(g_world.teams[nation].stadium);
    // grands stades des clubs du pays
    std::vector<std::pair<float, int>> cl;
    for (int i = 0; i < (int)g_world.teams.size(); i++) {
        const Team& t = g_world.teams[i];
        if (t.kind != TK_CLUB || t.nation != nation || t.parent >= 0 || t.youth || t.stadium.empty()) continue;
        if (t.rating < 45) continue;
        cl.push_back({ -t.rating, i });
    }
    std::sort(cl.begin(), cl.end());
    for (auto& c : cl) { if (v.size() >= 12) break; const Team& t = g_world.teams[c.second]; add(t.stadium + (t.town.empty() || t.stadium.find('(') != std::string::npos ? std::string() : " (" + t.town + ")")); }
    // petits pays : stades municipaux
    static const char* EXTRA[] = { "Stade municipal", "Stade olympique", "Stade de la capitale", "Stade régional" };
    for (int k = 0; v.size() < 4 && k < 4; k++) add(std::string(EXTRA[k]) + " (" + g_world.teams[nation].name + ")");
    return v;
}

// lieu d'un match de carrière internationale (phase finale chez l'organisateur, qualifications dans les stades du pays qui reçoit)
std::string matchVenue(int comp, int mi) {
    const Season& S = g_career.season;
    if (comp < 0 || comp >= (int)S.comps.size() || g_career.kind != CK_INTL) return "";
    const Competition& C = S.comps[comp];
    if (mi < 0 || mi >= (int)C.matches.size()) return "";
    const MatchRes& m = C.matches[mi];
    uint32_t h = (uint32_t)(comp * 7919 + mi * 104729 + S.year);
    h ^= h >> 13; h *= 0x5bd1e995; h ^= h >> 15;
    if (C.format == FMT_TOURNAMENT) {
        const auto& H = g_career.intlHosts;
        if (H.empty()) return "";
        int st = C.stageOfMatch(mi);
        bool finalM = st >= 0 && st == (int)C.stages.size() - 1 && C.stages[st].type == ST_KO && C.stages[st].ties.size() <= 2;
        int host = H[0];
        if (!finalM) {
            int g = std::max(0, (int)m.group);
            host = (st == 0) ? H[g % H.size()] : H[(h >> 4) % H.size()];
            for (int x : H) if (nationOfTeam(m.home) == x || nationOfTeam(m.away) == x) host = x;           // un pays hôte joue chez lui
        }
        auto v = nationVenues(host);
        if (v.empty()) return "";
        if (finalM) return v[0];
        return v[h % v.size()];
    }
    // match chez une sélection : stade national le plus souvent
    if (g_world.teams[m.home].kind == TK_NATION && !m.neutral) {
        auto v = nationVenues(nationOfTeam(m.home));
        if (v.empty()) return "";
        if (g_world.teams[m.home].youth) return v[(h >> 3) % v.size()];     // sélections de jeunes : stades variés
        return (h % 10) < 7 ? v[0] : v[h % v.size()];
    }
    if (m.neutral && C.kind == 31 && C.host >= 0) { auto v = nationVenues(nationOfTeam(C.host)); return v.empty() ? "" : v[h % v.size()]; }
    return "";
}

// ------------------------------------------------------------------ joueurs sélectionnables
static std::vector<Player>& localPlayers(int nation) {
    static std::map<int, std::vector<Player>> cache;
    auto it = cache.find(nation);
    if (it != cache.end()) return it->second;
    std::vector<Player>& v = cache[nation];
    Team t; t.kind = TK_NATION; t.nation = nation; t.culture = NATIONS[nation].culture;
    t.rating = std::max(20.f, g_world.teams[nation].rating - 8.f); t.seed = hashStr(std::string("local-") + NATIONS[nation].code);
    t.name = g_world.teams[nation].name;
    std::vector<Player> a, b;
    generateFakeSquad(t, 20, a);
    t.seed ^= 0x5A5A5A5A; t.rating -= 4;
    generateFakeSquad(t, 20, b);
    for (auto& p : b) a.push_back(p);
    int k = 0;
    for (auto& p : a) { p.id = 0x50000000 + nation * 64 + (k++); p.nation = (int16_t)nation; p.contract = 1; }
    v = a;
    return v;
}

// jeunes des championnats locaux et des centres de formation (sélections de jeunes)
static std::vector<Player>& localYouth(int nation, int cat) {
    static std::map<int, std::vector<Player>> cache;
    int key = nation * 8 + cat;
    auto it = cache.find(key);
    if (it != cache.end()) return it->second;
    std::vector<Player>& v = cache[key];
    Team t; t.kind = TK_NATION; t.nation = nation; t.culture = NATIONS[nation].culture;
    static const int OFF[5] = { 0, 8, 13, 17, 6 };
    t.rating = std::max(15.f, g_world.teams[nation].rating - OFF[cat]); t.seed = hashStr(std::string("youth-") + NATIONS[nation].code + fmt("%d", cat));
    std::vector<Player> a, b;
    generateFakeSquad(t, 20, a);
    t.seed ^= 0x5A5A5A5A; t.rating -= 3;
    generateFakeSquad(t, 20, b);
    for (auto& p : b) a.push_back(p);
    Rng r(t.seed);
    int k = 0;
    for (auto& p : a) {
        p.age = (uint8_t)(cat == 3 ? r.range(15, 17) : cat == 2 ? r.range(17, 19) : cat == 4 ? r.range(20, 23) : r.range(18, 21));
        p.id = 0x58000000 + (nation * 5 + cat) * 64 + (k++); p.nation = (int16_t)nation; p.contract = 2;
    }
    v = a;
    return v;
}

std::vector<PoolPlayer> coachPool(int nation, int maxAge, int cat) {
    std::vector<PoolPlayer> v;
    std::set<std::string> names;
    for (int i = 0; i < (int)g_world.teams.size(); i++) {
        Team& t = g_world.teams[i];
        if (t.kind != TK_CLUB || !t.squadGen || (t.youth && !cat)) continue;
        for (int k = 0; k < (int)t.squad.size(); k++) {
            const Player& p = t.squad[k];
            if (p.nation != nation || p.age < (cat == 3 ? 14 : cat ? 15 : 17)) continue;
            if ((p.gender == 1) != (cat == 5)) continue;            // séparation stricte : sélections féminines = joueuses
            if (maxAge > 0 && p.age > maxAge) continue;
            if (names.count(p.name + fmt("%d", p.age))) continue;
            names.insert(p.name + fmt("%d", p.age));
            v.push_back({ i, k, &t.squad[k] });
        }
    }
    if (cat) {
        auto& L = localYouth(nation, cat);
        for (int k = 0; k < (int)L.size(); k++) v.push_back({ -1, k, &L[k] });
    } else if (v.size() < 60) {
        auto& L = localPlayers(nation);
        for (int k = 0; k < (int)L.size(); k++) v.push_back({ -1, k, &L[k] });
    }
    std::stable_sort(v.begin(), v.end(), [](const PoolPlayer& a, const PoolPlayer& b) { return a.p->overall() > b.p->overall(); });
    return v;
}

const Player* coachFind(const CoachCall& c, int* clubOut) {
    if (c.club == -1 && (c.pid & 0xF8000000) == 0x58000000) {
        int key = (c.pid - 0x58000000) / 64, k = (c.pid - 0x58000000) % 64;
        int nation = key / 5, cat = key % 5;
        if (nation >= 0 && nation < NUM_NATIONS && cat > 0) { auto& L = localYouth(nation, cat); if (k < (int)L.size()) { if (clubOut) *clubOut = -1; return &L[k]; } }
        return nullptr;
    }
    if (c.club == -1 && (c.pid & 0x50000000) == 0x50000000) {
        int nation = (c.pid - 0x50000000) / 64, k = (c.pid - 0x50000000) % 64;
        if (nation >= 0 && nation < NUM_NATIONS) { auto& L = localPlayers(nation); if (k < (int)L.size()) { if (clubOut) *clubOut = -1; return &L[k]; } }
        return nullptr;
    }
    if (c.club >= 0 && c.club < (int)g_world.teams.size()) {
        for (auto& p : g_world.teams[c.club].squad) if (p.id == c.pid) { if (clubOut) *clubOut = c.club; return &p; }
    }
    int idx = -1; int t = g_world.findPlayer(c.pid, &idx);
    if (t >= 0 && g_world.teams[t].kind == TK_CLUB) { if (clubOut) *clubOut = t; return &g_world.teams[t].squad[idx]; }
    return nullptr;
}

int Career::coachTeamCat() const { return coachCat == 1 && intlType == IT_OLYMPICS ? 4 : coachCat; }
int Career::coachTeam() const { return coachCat ? youthNationTeam(coachNation, coachTeamCat()) : coachNation; }
int Career::coachMaxAge() const { int c = coachTeamCat(); return c == 1 ? 21 : c == 2 ? 19 : c == 3 ? 17 : 0; }

void Career::coachAutoSelect() {
    auto pool = coachPool(coachNation, coachMaxAge(), coachTeamCat());
    int QUOTA[4] = { 3, 9, 8, 6 };                  // gardiens, défenseurs, milieux, attaquants (26)
    if (coachCat) { QUOTA[1] = 8; QUOTA[2] = 7; QUOTA[3] = 5; }      // sélections de jeunes : 23
    int n[4] = { 0, 0, 0, 0 };
    std::vector<CoachCall> keep;
    int over = 0;
    bool oly = coachTeamCat() == 4;
    for (auto& pp : pool) {
        const Player& p = *pp.p;
        if (p.injured > 2) continue;
        if (oly && p.age > 23) { if (over >= 3) continue; over++; }       // tournoi olympique : 3 joueurs de plus de 23 ans
        int k = std::min(3, (int)p.pos);
        if (n[k] >= QUOTA[k]) continue;
        n[k]++;
        CoachCall c; c.pid = p.id; c.club = pp.club;
        for (auto& o : coachCalls) if (o.pid == c.pid) { c.caps = o.caps; c.goals = o.goals; }
        keep.push_back(c);
        if ((int)keep.size() >= coachSquadMax()) break;
    }
    coachCalls = keep;
}

// l'effectif de la sélection est une copie des convoqués (identifiants propres à la sélection)
void Career::coachApplySquad() {
    if (!coach || coachNation < 0) return;
    Team& N = g_world.teams[coachTeam()];
    // statistiques internationales accumulées
    for (auto& c : coachCalls) {
        int cid = 0x40000000 | (c.pid & 0x0FFFFFFF);
        for (auto& p : N.squad) if (p.id == cid) { c.caps = p.apps; c.goals = p.goals; }
    }
    std::vector<Player> sq;
    std::vector<int> susp;
    for (auto& c : coachCalls) {
        const Player* o = coachFind(c);
        if (!o) continue;
        Player p = *o;
        int cid = 0x40000000 | (c.pid & 0x0FFFFFFF);
        int8_t s = 0;
        for (auto& q : N.squad) if (q.id == cid) s = q.suspended;
        p.id = cid; p.apps = c.caps; p.goals = c.goals; p.suspended = s; p.yellows = 0;
        sq.push_back(p);
    }
    if (sq.size() < 11) return;               // sélection incomplète : on garde l'effectif actuel
    // numéros : gardien 1, puis dans l'ordre des postes
    std::stable_sort(sq.begin(), sq.end(), [](const Player& a, const Player& b) { return a.pos < b.pos; });
    int k = 1; for (auto& p : sq) p.num = (uint8_t)(k++);
    if (sq[0].pos == POS_GK) {}
    N.squad = sq;
    N.squadGen = true;
    N.xi.clear();
}

// ------------------------------------------------------------------ calendrier des campagnes
std::vector<CoachCampaign> Career::coachPlan() const {
    std::vector<CoachCampaign> v;
    if (coachNation < 0) return v;
    int conf = NATIONS[coachNation].conf;
    if (coachCat == 1) {            // Espoirs : Euro Espoirs (années impaires) et tournoi olympique (2028, 2032...)
        for (int y = 2027; (int)v.size() < 14; y++) {
            if (conf == UEFA && y % 2 == 1) { CoachCampaign c; c.type = IT_EURO21; c.year = y; c.format = 0; v.push_back(c); }
            if (y % 4 == 0) { CoachCampaign c; c.type = IT_OLYMPICS; c.year = y; c.format = 0; v.push_back(c); }
        }
        return v;
    }
    if (coachCat == 2 || coachCat == 3) {   // U19 / U17 : championnat d'Europe chaque année
        for (int y = 2027; (int)v.size() < 14; y++) { CoachCampaign c; c.type = coachCat == 2 ? IT_EURO19 : IT_EURO17; c.year = y; c.format = 0; v.push_back(c); }
        return v;
    }
    int ctype = conf == UEFA ? IT_EURO : conf == CAF ? IT_CAN : conf == AFC ? IT_ASIA : conf == CONCACAF ? IT_GOLD : conf == CONMEBOL ? IT_COPA : IT_OFC;
    int cy = conf == UEFA || conf == CONMEBOL || conf == OFC ? 2028 : 2027;
    int wy = 2030;
    for (int i = 0; i < 12; i++) {
        CoachCampaign c;
        if (i % 2 == 0) { c.type = ctype; c.year = cy + (i / 2) * 4; c.format = ctype == IT_EURO ? 1 : 0; }
        else { c.type = IT_WORLDCUP; c.year = wy + (i / 2) * 4; c.format = 1; }
        v.push_back(c);
    }
    return v;
}

// organisateurs officiels connus, sinon désignation par la confédération (niveau, stades, rotation)
std::vector<int> Career::campaignHosts(const CoachCampaign& c) const {
    auto N = [&](const char* s) { return g_world.nationIndex(s); };
    std::vector<int> h;
    if (c.type == IT_WORLDCUP && c.year == 2030) h = { N("MAR"), N("ESP"), N("POR") };
    else if (c.type == IT_WORLDCUP && c.year == 2034) h = { N("KSA") };
    else if (c.type == IT_EURO && c.year == 2028) h = { N("ENG"), N("SCO"), N("WAL"), N("IRL") };
    else if (c.type == IT_EURO && c.year == 2032) h = { N("ITA"), N("TUR") };
    else if (c.type == IT_CAN && c.year == 2027) h = { N("KEN"), N("UGA"), N("TAN") };
    else if (c.type == IT_ASIA && c.year == 2027) h = { N("KSA") };
    else if (c.type == IT_OLYMPICS && c.year == 2028) h = { N("USA") };          // Los Angeles
    else if (c.type == IT_OLYMPICS && c.year == 2032) h = { N("AUS") };          // Brisbane
    else if (c.type == IT_EURO21 && c.year == 2027) h = { N("ALB"), N("SRB") };
    h.erase(std::remove(h.begin(), h.end(), -1), h.end());
    if (!h.empty()) return h;
    int conf = c.type == IT_EURO || c.type == IT_EURO21 || c.type == IT_EURO19 || c.type == IT_EURO17 ? UEFA : c.type == IT_CAN ? CAF : c.type == IT_ASIA ? AFC : c.type == IT_GOLD ? CONCACAF : c.type == IT_COPA ? CONMEBOL : c.type == IT_OFC ? OFC : -1;
    std::vector<std::pair<float, int>> cand;
    Rng r((uint64_t)(c.type * 1000003 + c.year * 31 + 7));
    for (int i = 0; i < NUM_NATIONS; i++) {
        if (!nationEligible(i)) continue;
        if (conf >= 0 && NATIONS[i].conf != conf) continue;
        if (conf < 0 && (NATIONS[i].conf == UEFA && c.year == 2038)) continue;
        int nv = 0; for (auto& t : g_world.teams) if (t.kind == TK_CLUB && t.nation == i && t.rating >= 55 && t.parent < 0) nv++;
        float sc = g_world.teams[i].rating * 0.7f + std::min(12, nv) * 1.5f + r.frange(0, 25);
        cand.push_back({ -sc, i });
    }
    std::sort(cand.begin(), cand.end());
    if (cand.empty()) return h;
    h.push_back(cand[0].second);
    // Coupe du monde à 48 : un co-organisateur si le pays est modeste
    if (c.type == IT_EURO21 && g_world.teams[h[0]].rating < 70) for (auto& x : cand) if (x.second != h[0]) { h.push_back(x.second); break; }
    if (c.type == IT_WORLDCUP && g_world.teams[h[0]].rating < 78) for (auto& x : cand) if (x.second != h[0] && NATIONS[x.second].conf == NATIONS[h[0]].conf) { h.push_back(x.second); break; }
    return h;
}

static int stageLevel(const Competition& C, int team) {
    // 0 vainqueur, 1 finale/demies, 2 quarts, 3 8es (ou 16es), 4 phase de groupes
    if (C.winner == team) return 0;
    int best = 4;
    bool in = false;
    for (auto& g : C.stages.empty() ? std::vector<std::vector<int>>() : C.stages[0].groups) for (int x : g) if (x == team) in = true;
    if (!in) return 5;
    for (size_t s = 1; s < C.stages.size(); s++) for (auto& t : C.stages[s].ties) if (t.a == team || t.b == team) {
        int n = (int)C.stages[s].ties.size();
        int lvl = n <= 2 ? 1 : n <= 4 ? 2 : 3;
        best = std::min(best, lvl);
    }
    return best;
}

int Career::coachResult() const {
    if (finalComp < 0 || finalComp >= (int)season.comps.size()) return 5;
    return stageLevel(season.comps[finalComp], coachTeam());
}

const char* resultLevelName(int lvl) {
    static const char* N[6] = { "vainqueur", "demi-finale ou finale", "quarts de finale", "8es de finale", "phase de groupes", "non qualifié" };
    return N[std::max(0, std::min(5, lvl))];
}

std::string Career::coachObjectiveText() const {
    static const char* O[6] = { "Remporter le trophée", "Atteindre le dernier carré", "Atteindre les quarts de finale", "Passer la phase de groupes", "Se qualifier pour la phase finale", "Progresser et faire bonne figure" };
    return O[std::max(0, std::min(5, coachObjective))];
}

// Ligue des nations (UEFA : 4 ligues A-D, groupes aller-retour à l'automne, Final Four en juin ; CONCACAF : ligues A et B)
static void addNationsLeague(Career& K, int conf) {
    Season& S = K.season;
    std::vector<int> v;
    for (int i = 0; i < NUM_NATIONS; i++) if (nationEligible(i) && NATIONS[i].conf == conf) v.push_back(i);
    std::stable_sort(v.begin(), v.end(), [](int a, int b) { return g_world.teams[a].rating > g_world.teams[b].rating; });
    const char* LN[4] = { "A", "B", "C", "D" };
    int per = conf == UEFA ? 16 : 12;
    int nl = conf == UEFA ? 4 : 2;
    for (int l = 0; l < nl; l++) {
        std::vector<int> t(v.begin() + std::min((int)v.size(), l * per), v.begin() + std::min((int)v.size(), (l == nl - 1) ? (int)v.size() : (l + 1) * per));
        if (t.size() < 3) continue;
        int ng = std::max(1, (int)t.size() / 4);
        std::vector<std::vector<int>> g(ng);
        Rng r((uint64_t)(K.year * 131 + l + conf * 7));
        for (size_t i = 0; i < t.size(); i += ng) {
            std::vector<int> pot(t.begin() + i, t.begin() + std::min(t.size(), i + ng));
            r.shuffle(pot);
            for (size_t k = 0; k < pot.size(); k++) g[k % ng].push_back(pot[k]);
        }
        Competition c;
        c.format = FMT_QUAL_GROUPS; c.kind = 30; c.tag = l; c.legs = 2; c.tb = TB_H2H;
        c.name = std::string(conf == UEFA ? "Ligue des nations UEFA - Ligue " : "Ligue des nations CONCACAF - Ligue ") + LN[l];
        c.shortName = std::string("Ligue des nations ") + LN[l];
        c.addGroupStage(g, 2, { 5.0, 6.0, 9.0, 10.0, 14.0, 15.0 }, "Groupes");
        addCompPublic(S, std::move(c));
    }
}

void coachOnCompDone(Career& K, int comp) {
    Season& S = K.season;
    Competition& C = S.comps[comp];
    if (C.kind == 31 && C.winner >= 0 && C.winner == K.coachTeam()) K.coachLog.push_back(fmt("%d : ", K.season.year + 1) + C.name + " - VAINQUEUR");
}

// matchs amicaux de préparation
static void addCoachFriendlies(Career& K) {
    Season& S = K.season;
    int meN = K.coachNation, cat = K.coachTeamCat();
    int me = K.coachTeam();
    std::vector<int> opp;
    for (int i = 0; i < NUM_NATIONS; i++) if (i != meN && nationEligible(i) && std::fabs(g_world.teams[i].rating - g_world.teams[meN].rating) < 14) opp.push_back(i);
    if (opp.size() < 4) for (int i = 0; i < NUM_NATIONS; i++) if (i != meN && nationEligible(i)) opp.push_back(i);
    if (cat) for (int& o : opp) o = youthNationTeam(o, cat);
    Rng r((uint64_t)(K.year * 977 + me));
    r.shuffle(opp);
    double times[4] = { 12.0, 36.0, 53.0, 91.0 };
    for (int k = 0; k < 4 && k < (int)opp.size(); k++) {
        bool home = k % 2 == 0;
        int a = home ? me : opp[k], b = home ? opp[k] : me;
        Competition c;
        c.kind = 12; c.format = FMT_SINGLE; c.neutralFinal = false; c.shortName = "Amical";
        c.name = "Match amical international : " + g_world.teams[a].name + " - " + g_world.teams[b].name;
        c.addKOStage({ { a, b } }, 1, times[k], "Match amical", false);
        c.matches[0].decisive = 0; c.matches[0].noET = 1; c.matches[0].neutral = false;
        addCompPublic(S, std::move(c));
    }
}

void Career::startCoachCampaign() {
    auto plan = coachPlan();
    if (plan.empty()) return;
    CoachCampaign c = plan[std::min(coachCamp, (int)plan.size() - 1)];
    std::vector<int> hosts = coachNextHosts.empty() ? campaignHosts(c) : coachNextHosts;
    // conserver l'identité du sélectionneur et le bilan
    std::string mn = managerName, mp = managerPlace; int mnat = managerNation, ma = managerAge; uint8_t ms = managerSkin, mh = managerHair, bd = managerBday, bm = managerBmonth;
    auto hist = history; auto log = coachLog; auto calls = coachCalls; int nat = coachNation, camp = coachCamp;
    bool noSack = mgr.noSack;
    newInternational(c.type, true, { nat }, c.year, hosts, c.format);
    managerName = mn; managerPlace = mp; managerNation = mnat; managerAge = ma; managerSkin = ms; managerHair = mh; managerBday = bd; managerBmonth = bm;
    history = hist; coachLog = log; coachCalls = calls; coachNation = nat; coachCamp = camp; coach = true; mgr.noSack = noSack;
    // la saison de la campagne commence deux ans avant la phase finale (automne)
    year = c.year;          // la saison (season.year) commence deux ans avant la phase finale
    int conf = NATIONS[nat].conf;
    if (conf == CONCACAF && !coachCat) addNationsLeague(*this, conf);
    addCoachFriendlies(*this);
    // objectif de la fédération selon le rang de la sélection
    std::vector<int> v; for (int i = 0; i < NUM_NATIONS; i++) if (nationEligible(i) && (c.type == IT_WORLDCUP || c.type == IT_OLYMPICS || NATIONS[i].conf == conf)) v.push_back(i);
    std::stable_sort(v.begin(), v.end(), [](int a, int b) { return g_world.teams[a].rating > g_world.teams[b].rating; });
    int rank = (int)(std::find(v.begin(), v.end(), nat) - v.begin());
    coachObjective = rank < 3 ? 1 : rank < 6 ? 2 : rank < 12 ? 3 : rank < 24 ? 4 : 5;
    if (c.type != IT_WORLDCUP && rank < 1) coachObjective = 0;
    if (c.type == IT_OLYMPICS) coachObjective = std::min(5, coachObjective + 1);
    // organisateurs de la campagne suivante
    coachNextHosts.clear();
    if (coachCamp + 1 < (int)plan.size()) coachNextHosts = campaignHosts(plan[coachCamp + 1]);
    std::string hs; for (int h : hosts) hs += (hs.empty() ? "" : ", ") + g_world.teams[h].name;
    season.news.push_back(fmt("%s %d : début de la campagne. Organisation : %s.", INTL_NAMES[c.type], c.year, hs.c_str()));
    season.news.push_back("La fédération fixe l'objectif : " + coachObjectiveText() + ".");
    if (!coachNextHosts.empty()) {
        std::string nh; for (int h : coachNextHosts) nh += (nh.empty() ? "" : ", ") + g_world.teams[h].name;
        season.news.push_back(fmt("%s %d : organisation confiée à %s.", INTL_NAMES[plan[coachCamp + 1].type], plan[coachCamp + 1].year, nh.c_str()));
    }
    if (coachCat) {       // sélections de jeunes : les joueurs trop âgés quittent la liste
        int ma = coachMaxAge();
        coachCalls.erase(std::remove_if(coachCalls.begin(), coachCalls.end(), [&](const CoachCall& x) { const Player* p = coachFind(x); return !p || (ma > 0 && p->age > ma); }), coachCalls.end());
        if (c.type == IT_OLYMPICS) coachCalls.clear();
    }
    if ((int)coachCalls.size() < (coachCat ? 18 : 23)) coachAutoSelect();
    coachApplySquad();
}

void Career::newCoachCareer(int nation) {
    // effectifs des clubs réels : vivier des joueurs sélectionnables
    for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].kind == TK_CLUB && g_world.teams[i].dbClub >= 0) g_world.ensureSquad(i);
    coach = true; coachNation = nation; coachCamp = 0; coachSacked = 0; nlLeague.clear(); coachU21Podium.clear();
    coachCalls.clear(); coachLog.clear(); coachNextHosts.clear();
    history.clear();
    g_world.ensureSquad(nation);
    coachAutoSelect();
    startCoachCampaign();
}

// fin de campagne : bilan, jugement de la fédération, vieillissement des joueurs, campagne suivante
void Career::endCoachCampaign(std::vector<std::string>& msgs) {
    auto plan = coachPlan();
    CoachCampaign c = plan[std::min(coachCamp, (int)plan.size() - 1)];
    int res = coachResult();
    // statistiques internationales
    { Team& N = g_world.teams[coachTeam()];
      for (auto& cc : coachCalls) { int cid = 0x40000000 | (cc.pid & 0x0FFFFFFF); for (auto& p : N.squad) if (p.id == cid) { cc.caps = p.apps; cc.goals = p.goals; } } }
    // Euro Espoirs : les trois premiers se qualifient pour le tournoi olympique suivant
    if (c.type == IT_EURO21 && finalComp >= 0 && finalComp < (int)season.comps.size()) {
        const Competition& F = season.comps[finalComp];
        std::vector<std::pair<int, int>> lv;
        for (auto& g : F.stages[0].groups) for (int t : g) lv.push_back({ stageLevel(F, t) * 100 - (int)g_world.teams[t].rating, t });
        std::sort(lv.begin(), lv.end());
        coachU21Podium.clear();
        for (int i = 0; i < 3 && i < (int)lv.size(); i++) coachU21Podium.push_back(nationOfTeam(lv[i].second));
    }
    std::string line = fmt("%s %d : %s (objectif : %s)", INTL_NAMES[c.type], c.year, resultLevelName(res), coachObjectiveText().c_str());
    coachLog.push_back(line);
    msgs.push_back(line);
    coachSacked = 0;
    if (res == 0) msgs.push_back("Triomphe ! La fédération prolonge votre contrat.");
    else if (res <= coachObjective) msgs.push_back("Objectif atteint : la fédération vous renouvelle sa confiance.");
    else if (res >= coachObjective + 2 && res >= 3 && !mgr.noSack) { msgs.push_back("Objectif largement manqué : la fédération met fin à votre mission."); coachSacked = 1; }
    else msgs.push_back("Objectif manqué de peu : la fédération vous laisse une nouvelle chance.");
    // le temps passe jusqu'à la campagne suivante
    int ny = coachCamp + 1 < (int)plan.size() ? plan[coachCamp + 1].year : c.year + 2;
    int dy = std::max(1, std::min(4, ny - c.year));
    std::vector<std::string> news;
    for (int k = 0; k < dy; k++) ageSquads(-1, news);
    for (auto& cc : coachCalls) if (!coachFind(cc)) cc.pid = 0;
    coachCalls.erase(std::remove_if(coachCalls.begin(), coachCalls.end(), [](const CoachCall& x) { return x.pid == 0; }), coachCalls.end());
    coachCamp++;
}

// ------------------------------------------------------------------ sauvegarde (version 14)
void coachSave(Writer& w, const Career& K) {
    w.pod(K.coach); w.pod(K.coachNation); w.pod(K.coachCamp); w.pod(K.coachObjective); w.pod(K.coachSacked);
    w.vpod(K.coachCalls); w.vpod(K.coachNextHosts); w.vstr(K.coachLog);
}
void coachLoad(Reader& r, Career& K) {
    r.pod(K.coach); r.pod(K.coachNation); r.pod(K.coachCamp); r.pod(K.coachObjective); r.pod(K.coachSacked);
    r.vpod(K.coachCalls); r.vpod(K.coachNextHosts); r.vstr(K.coachLog);
    if (K.coachNation >= NUM_NATIONS) { K.coach = false; K.coachNation = -1; }
}
