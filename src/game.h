// Structures principales du jeu
#pragma once
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include "data.h"
#include "museum.h"
#include "director.h"
#include "sporting.h"
#include "supporters.h"
#include "personality.h"

// ------------------------------------------------------------------ RNG
struct Rng {
    uint64_t s;
    explicit Rng(uint64_t seed = 0x12345678ULL) : s(seed ? seed : 0x9E3779B97F4A7C15ULL) {}
    uint64_t next() {
        uint64_t z = (s += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
    float f() { return (next() >> 40) / 16777216.0f; }
    int range(int a, int b) { return a + (int)(next() % (uint64_t)(b - a + 1)); }
    float frange(float a, float b) { return a + (b - a) * f(); }
    bool chance(float p) { return f() < p; }
    template <class T> void shuffle(std::vector<T>& v) {
        for (int i = (int)v.size() - 1; i > 0; i--) { int j = (int)(next() % (uint64_t)(i + 1)); std::swap(v[i], v[j]); }
    }
};
extern Rng g_rng;

uint32_t hashStr(const std::string& s);
std::string sanitize(const char* utf8);
std::string fmt(const char* f, ...);

// ------------------------------------------------------------------ Joueurs / équipes
enum PlayerPos { POS_GK = 0, POS_DF, POS_MF, POS_FW };

// historique d'un joueur : une ligne par saison et par club (version 11)
struct PlayerSeason { int16_t year = 0; int16_t apps = 0; int32_t team = -1; int16_t goals = 0, assists = 0; uint8_t yel = 0, red = 0; uint8_t moved = 0, pad = 0; };

enum RuleProfile { RULESET_SIMPLE=0, RULESET_CAREER=1 };
enum DetailedPosition { DP_GB,DP_DD,DP_DC,DP_DG,DP_PIS_D,DP_PIS_G,DP_MDC,DP_MC,DP_MD,DP_MG,DP_MOC,DP_AD,DP_AG,DP_SA,DP_BU,DP_COUNT };
enum TacticalDuty { DUTY_DEFENSE, DUTY_SUPPORT, DUTY_ATTACK };
enum TacticalInstruction { TI_STAY,TI_ADVANCE,TI_PRESS,TI_NO_PRESS,TI_DROP,TI_DEPTH,TI_INSIDE,TI_WIDE,TI_EARLY_CROSS,TI_DEEP_CROSS,TI_MARK,TI_FREE,TI_SIMPLE,TI_SHOOT,TI_HOLD,TI_DRIBBLE,TI_NO_DRIBBLE,TI_COUNT };
struct PositionKnowledge { uint8_t initialized=0,primary=DP_MC,foot=0,target=255; uint8_t familiarity[DP_COUNT]={}; uint16_t practice[DP_COUNT]={}; uint8_t developmentRole=255; };
struct SlotTactic { uint8_t role=0,duty=DUTY_SUPPORT,position=255; uint32_t instructions=0; int32_t markPid=0; };
struct TacticalPlan { uint8_t customized=0,model=0; SlotTactic slot[11]; };
struct RoleEffects { float advance=0,width=1,press=1,shot=1,risk=1,dribble=1,crossAt=.8f,hold=0; bool drop=false,halfback=false; };
bool careerRules();
int positionFamily(int p);
int slotPosition(int formation,int slot);
const char* detailedPositionName(int p);
const char* familiarityName(int value);
const char* tacticalRoleName(int position,int role);
const char* tacticalInstructionName(int i);
int tacticalRoleCount(int position);
struct Player;
void initPositions(Player& p);
int positionFamiliarity(const Player& p,int position);
int roleAptitude(const Player& p,int position,int role);
void learnPosition(Player& p,int position,int minutes,int staff=0);
SlotTactic defaultSlotTactic(int position,int model,int slot);
RoleEffects roleEffects(int position,const SlotTactic& tactic);

struct Player {
    PositionKnowledge positions;
    std::string name;
    uint8_t pos = POS_MF;
    uint8_t num = 1;
    uint8_t speed = 50, shoot = 50, pass = 50, tackle = 50, keep = 20, stamina = 60;
    uint8_t skin = 0, hair = 0;
    int8_t suspended = 0;
    int8_t yellows = 0;
    int8_t injured = 0;
    int16_t goals = 0, apps = 0;
    int16_t nation = -1;
    int32_t id = 0;            // identifiant unique (buteurs, transferts)
    uint8_t gender = 0;        // 0 homme, 1 femme (version 14)
    uint8_t years = 0;         // années de contrat restantes (0 = non renseigné), version 14
    int32_t wageK = 0;         // salaire négocié (k€ / an, 0 = barème)
    uint8_t contract = 0;      // 0 contrat pro, 1 semi-pro, 2 licence amateur (pas de salaire)
    uint8_t age = 25, pot = 50;
    int16_t assists = 0;
    uint8_t cond = 100;        // condition physique (fatigue) 0-100
    uint8_t morale = 60;       // moral 0-100
    uint8_t dribble = 0, heading = 0;   // 0 = dérivé des autres caractéristiques
    // version 11 : infos personnelles, nouvelles caractéristiques, historique
    uint8_t bday = 0, bmonth = 0;       // 0 = dérivé de l'identifiant
    std::string birthPlace;             // vide = dérivé
    uint8_t positioning = 0, composure = 0;   // placement (défense / appels), sang-froid (0 = dérivé)
    uint8_t sYel = 0, sRed = 0;         // cartons de la saison
    std::vector<PlayerSeason> hist;
    int drib() const;          // dribble
    int head() const;          // jeu de tête
    int posi() const;          // placement
    int comp() const;          // sang-froid
    int overall() const;
    int value() const;         // valeur marchande (k€)
    int wage() const;          // salaire annuel (k€)
};
const char* posName(int pos);
struct Team;
void playerBirth(const Player& p, int year, int& d, int& m, int& y, std::string& place, int culture = -1, int dept = -1);
std::string presidentName(int team);
int presidentAge(int team);
std::string presidentPlace(int team);
std::string randomFullName(int culture, uint32_t seed);
void recordPlayerSeason(Player& p, int team, int year, bool moved);
void applyPlayerEdits(Team& t);        // éditeur de joueurs (fichier) appliqué à la génération de l'effectif
void savePlayerEdit(const Team& t, int idx);
void applyPresidents();
struct CustomStaff { std::string name = "Nouveau membre"; int role = 0, level = 2, age = 45; std::string place, club; };
std::vector<CustomStaff>& customStaff();     // staff créé avec l'éditeur (staff_perso.txt)
void saveCustomStaff();
void savePresident(const Team& t);

// ------------------------------------------------------------------ Stade
enum StandKind { STK_STANDING = 0, STK_SEATS = 1, STK_COVERED = 2 };
struct Stand { int32_t seats = 0; int32_t vip = 0; uint8_t kind = STK_SEATS; uint8_t pad[3] = { 0, 0, 0 }; int16_t price = 10, vipPrice = 40; int32_t lastAtt = 0; };
struct StadiumInfo {
    uint8_t init = 0;
    uint8_t buvette = 1, boutique = 0, parking = 0, lights = 1, pitch = 0, screen = 0, vestiaires = 1, museum = 0, pad = 0;
    Stand s[4];                 // 0 tribune principale, 1 tribune face, 2 et 3 virages
    int32_t fans = 500;         // supporters
    int16_t shirtPrice = 50;    // prix du maillot (€)
    int32_t namingIncome = 0;   // k€ / an (nom du stade vendu à un sponsor)
    char sponsor[32] = { 0 };
    int32_t lastAtt = 0, lastGate = 0, bestAtt = 0;
    int32_t seasonAttTotal = 0, seasonHomeMatches = 0;
    // pelouse et foncier (build 5)
    uint8_t turf = 85;              // état de la pelouse : 100 parfaite ... 0 champ de patates
    uint8_t owner = 0;              // 0 : stade loué à la mairie ; 1 : stade acheté par le club
    uint8_t annexReserve = 0;       // stade annexe de l'équipe réserve (0 / 1)
    uint8_t annexYouth = 0;         // stade des jeunes (0 à 2)
    uint8_t annexTraining = 0;      // complexe d'entraînement (0 à 3)
    uint8_t extPending = 0;         // demande d'agrandissement à la mairie : mois restants avant la réponse
    uint8_t repaired = 0;           // regarnissage déjà fait ce mois-ci
    uint8_t pad2 = 0;
    int32_t extQuota = 0;           // places d'agrandissement accordées par la mairie
    int32_t rentK = 0;              // loyer annuel versé à la mairie (k€)
    int capacity() const { int c = 0; for (auto& x : s) c += x.seats + x.vip; return c; }
};
// produits dérivés (boutique) en plus du maillot
enum { NUM_MERCH = 14 };
struct Merch {
    uint8_t init = 0, online = 0;           // boutique en ligne : niveau 0 à 3
    uint8_t on[NUM_MERCH] = {};             // produit en vente
    int16_t price[NUM_MERCH] = {};          // prix de vente (€)
    int32_t units[NUM_MERCH] = {};          // ventes de la saison (unités)
    int32_t revenue[NUM_MERCH] = {};        // bénéfice de la saison (€)
    int32_t shirtUnits = 0, shirtRevenue = 0, onlineRevenue = 0;
};
const char* merchName(int k);
const char* turfStateName(int q);
int turfQualityFor(int team, int month);    // état de la pelouse du stade d'un club (0-100) ; month 0 août ... 10 juin
int merchRefPrice(int k, int status);
int merchMinShop(int k);                    // niveau de boutique requis
void initMerch(Team& t);
enum ClubStatus { CS_PRO = 0, CS_SEMIPRO = 1, CS_AMATEUR = 2 };
const char* statusName(int s);

struct Kit { unsigned shirt = 0xFFFFFF, shirt2 = 0, shorts = 0xFFFFFF, socks = 0xFFFFFF; int pattern = 0; };

enum TeamKind { TK_NATION = 0, TK_CLUB = 1 };

struct Team {
    TacticalPlan tactical;
    std::string name, shortName;
    std::string stadium, town;
    int kind = TK_CLUB;
    int nation = -1;          // sélection : index NATIONS ; club : pays
    float rating = 50;
    Kit home, away;
    Kit third; uint8_t hasThird = 0;       // troisième maillot (build 5)
    int culture = CU_FR;
    int region = -1, dept = -1;
    int district = -1;        // district de football (districts.cpp) ; peut couvrir plusieurs départements ou une partie
    uint32_t seed = 0;
    int formation = 0;
    int dbClub = -1;          // club de la base de joueurs réels
    int parent = -1;          // équipe première (réserve) ou -1
    int resLevel = 0;         // 0 = équipe 1, 1 = réserve (B / 2), 2 = C / 3...
    bool custom = false;      // créé avec l'éditeur
    bool edited = false;
    bool squadGen = false;
    std::vector<Player> squad;
    std::vector<std::string> honours;
    int lastTier = -1;
    int founded = 0;
    std::vector<int> xi;      // titulaires choisis par l'entraîneur (identifiants joueurs), vide = automatique
    int status = CS_AMATEUR;  // statut du club
    std::string sponsor;      // sponsor maillot
    StadiumInfo sta;
    uint8_t reputation = 0;   // réputation 1-100 (0 = à calculer), version 14
    uint8_t academy = 0;      // niveau du centre de formation 0-5, version 14
    bool freeAgents = false;  // équipe technique : joueurs libres (sans club)
    Merch merch;              // produits dérivés (sauvegarde v13)
    float coefs[5] = { 0, 0, 0, 0, 0 };   // points UEFA du club (5 dernières saisons)
    float condT = -1;                     // date de la dernière mise à jour de la condition physique
    int mentality = 2;                    // 0 ultra défensif, 1 défensif, 2 neutre, 3 offensif, 4 ultra offensif (version 9)
    int captainPid = 0, vicePid = 0;      // capitaine et vice-capitaine (0 = automatique)
    int youth = 0;                        // 1 : équipe U19 du club parent
    // version 11 : président, consignes tactiques (0 faible, 1 normal, 2 fort)
    std::string president; int presAge = 0; std::string presPlace; uint8_t presGender = 0;   // présidente : 1
    uint8_t pressing = 1, defLine = 1, width = 1, tempo = 1, passStyle = 1;   // passStyle : 0 court, 1 mixte, 2 long
    float coefTotal() const { float c = 0; for (float v : coefs) c += v; return c; }
};
float clubSeed(int team);
int teamSlotPosition(const Team& team,int formation,int slot);
int playerCond(int team, const Player& p);          // condition physique actuelle (récupération incluse)
void applyMatchLoad(int team, int result);          // fatigue et moral après un match (result : 1 victoire, 0 nul, -1 défaite)
float formFactor(int team, RuleProfile rules=RULESET_SIMPLE);
const char* trainFocusName(int f);
const char* trainIntName(int i);                         // effet condition + moral sur la force (simulation)                 // coefficient de tirage européen (club ou 20 % du pays)

struct Formation { const char* name; float x[10]; float y[10]; uint8_t role[10]; };
extern const Formation FORMATIONS[];
extern const int NUM_FORMATIONS;

// ------------------------------------------------------------------ Base de données
struct World {
    std::vector<Team> teams;
    int firstClub = 0;
    int baseCount = 0;                  // nombre d'équipes générées automatiquement (hors éditeur)
    std::vector<int> leagueClubs[64];
    std::vector<std::vector<int>> extLeagueClubs;   // clubs des championnats complémentaires (EXT_LEAGUES)
    std::map<int, int> menOf, womenOf;              // section féminine <-> club masculin (reconstruit à chaque génération du monde)
    std::vector<int> euroPool;          // clubs UEFA hors championnats simulés
    std::vector<int> worldPool;         // autres clubs du monde
    std::map<std::string, std::vector<int>> countryClubs; // clubs par pays (hors championnats simulés)
    int omReps[3] = { -1, -1, -1 };
    void rebuildCountryClubs(const std::vector<struct Pyramid>& pyr);
    int nationIndex(const char* code) const;
    void build();
    void ensureSquad(int team);
    void generateSquad(Team& t);
    std::vector<int> pickLineup(int team, int formation, RuleProfile rules=RULESET_SIMPLE) const;
    int addCustomClub(const Team& t);
    int nextPid = 1;
    Player makeYouth(int team, int pos, float level);   // jeune joueur généré (centre de formation / remplaçant)
    int findPlayer(int pid, int* idx = nullptr) const;  // équipe du joueur (recherche linéaire)
    std::string clubBaseName(int team) const;
};
extern World g_world;

// ------------------------------------------------------------------ Compétitions
struct MatchRes {
    int home = -1, away = -1;
    int16_t hg = -1, ag = -1;
    int16_t ph = -1, pa = -1;
    bool aet = false;
    bool played = false;
    bool neutral = false;
    int8_t decisive = 0;
    int16_t tie = -1;
    int8_t leg = 0;
    int16_t group = -1;
    int8_t noET = 0;             // tirs au but directs sans prolongation (supercoupes)
};

// événement de match (buteurs, passeurs, cartons) : type 0 but, 1 csc, 2 jaune, 3 rouge, 4 blessure
struct MEv { int32_t match = -1, pid = 0, aid = 0, team = -1; uint8_t type = 0, minute = 0, side = 0, pen = 0; };

struct Round {
    double time = 0;
    std::string name;
    std::vector<int> m;
    bool done = false;
};

enum StageType { ST_LEAGUE = 0, ST_KO, ST_SWISS };

struct Tie { int a = -1, b = -1; int winner = -1; int m1 = -1, m2 = -1; };

struct Stage {
    int type = ST_LEAGUE;
    std::string name;
    int legs = 1;
    std::vector<std::vector<int>> groups;
    std::vector<Tie> ties;
    std::vector<Round> rounds;
    bool finished = false;
    int nextR = 0;
};

enum CompFormat {
    FMT_LEAGUE = 0, FMT_CUP, FMT_EUROPE, FMT_TOURNAMENT, FMT_QUAL_GROUPS, FMT_KO_ONLY,
    FMT_UCL2000,        // Ligue des champions 2000-01 : 3 tours de qualif, 2 phases de groupes, KO
    FMT_UEFA2000,       // Coupe UEFA 2000-01 : tours à élimination directe aller-retour, finale sèche
    FMT_SINGLE,         // match unique (supercoupe)
    FMT_INTERTOTO,      // Coupe Intertoto (tours à élimination directe, 3 vainqueurs)
    FMT_NEWEURO,        // nouvelle formule européenne : qualifications (2 voies), phase de ligue à 36, barrages, KO
};

// règles de départage
enum TieBreak { TB_GD = 0, TB_LFP, TB_FFF, TB_H2H, TB_ENG, TB_FIFA };
const char* tieBreakText(int tb);

struct Standing {
    int team = -1, p = 0, w = 0, d = 0, l = 0, gf = 0, ga = 0, pts = 0, aw = 0;
    int gd() const { return gf - ga; }
};

struct Competition {
    std::string name;
    std::string shortName;
    int format = FMT_LEAGUE;
    int legs = 2;
    int tb = TB_GD;
    int ptsWin = 3;
    int ptsDraw = 1;
    int ptsLoss = 0;
    int ptsForfeit = 0;       // points de l'équipe déclarée forfait (réservé aux décisions administratives)
    std::vector<MatchRes> matches;
    std::vector<Stage> stages;
    int cur = 0;
    bool done = false;
    bool started = false;
    double t0 = 1, t1 = 40;
    std::vector<double> koTimes;
    std::vector<std::string> koNames;
    std::vector<int> koLegs;
    std::vector<std::vector<int>> entrants;
    std::vector<int> koTargets;
    std::vector<int> result;
    std::vector<int> extra;             // UCL : éliminés du 3e tour ; UCL : 3es de 1re phase
    std::vector<int> extra2;
    std::vector<int> carry;             // qualifiés en attente d'un tour (entrées extérieures)
    int awaiting = -1;                  // tour en attente d'entrées extérieures
    int winner = -1;
    int groupsAdvance = 2;
    int bestThirds = 0;
    bool thirdPlace = false;
    bool regionalDraw = false;
    int regionalRounds = 0;
    int swissRounds = 8;
    int qualSpots = 0;
    int qualPlayoff = 0;
    int kind = 0;
    int host = -1;
    int tag = -1;
    int season = 0;
    bool noReserves = false;
    bool neutralFinal = true;
    int extReadyMask = 0;
    bool awayGoals = false;             // règle des buts à l'extérieur (coupes d'Europe)
    int yellowLimit = 3;                // cartons jaunes avant suspension (règlement)
    int homeRule = 0;                   // 0 tirage libre, 1 règle FFF (2 divisions d'écart : le plus petit reçoit)
    std::vector<MEv> events;

    void setupLeague(const std::vector<int>& teams, int legs, double t0, double t1, int numGroups = 1);
    void setupSwiss(const std::vector<int>& teams, int rounds, const std::vector<double>& times);
    void addGroupStage(const std::vector<std::vector<int>>& groups, int legs, const std::vector<double>& times, const std::string& name);
    // coupes régionales / de district (seniors et jeunes) et Gambardella : pas de prolongation, tirs au but directs (sauf finale)
    bool penaltiesOnly() const { return kind == 4 || kind == 5 || kind == 15 || kind == 16 || kind == 17 || kind == 18 || kind == 23 || kind == 24 || kind == 25 || kind == 26 || kind == 27; }
    bool rollingSubs() const { return kind == 4 || kind == 5 || kind == 17 || kind == 18 || kind == 24 || kind == 25; }
    Stage& addKOStage(const std::vector<std::pair<int, int>>& pairs, int legs, double time, const std::string& name, bool neutralFinal = false);
    std::vector<Standing> table(int stage, int group) const;
    std::vector<Standing> swissTable(int stage) const;
    bool stageComplete(int st) const;
    void onStageFinished();
    void cupRound(int k, const std::vector<int>& pool);
    void koRound(const std::vector<int>& teams, int legs, double time, const std::string& name, bool final);
    void resume();
    int stageOfMatch(int mi) const;
    int tieWinner(const Tie& t) const;
    const char* roundName(int st, int r) const;
};

void sortStandings(std::vector<Standing>& v);
void newEuroStart(Competition& C, int k, std::vector<int> pool);            // neweuro.cpp
void newEuroStageFinished(Competition& C, const std::vector<int>& winners);
std::vector<int> newEuroLosers(const Competition& C, int k);
bool newEuroRoundDone(const Competition& C, int k);
void simulateMatch(MatchRes& m, const Competition* c);
void genMatchEvents(Competition& C, int mi);      // buteurs / passeurs / cartons d'un match simulé
float teamStrength(int team, RuleProfile rules=RULESET_SIMPLE);
int teamLevel(int team);           // niveau dans la pyramide (0 = élite) ou 99
std::string koName(int nteams);
// calendrier (calendar.cpp) : coup d'envoi en minutes depuis le lundi de la semaine du match
int kickoffMinutes(int comp, int mi);
std::string kickoffText(int comp, int mi, bool withDate);
std::string kickoffHour(int comp, int mi);
// stades d'un pays (stade national puis grands stades de ses clubs) et lieu d'un match de carrière
std::vector<std::string> nationVenues(int nation);
std::string matchVenue(int comp, int mi);
// joueurs sélectionnables d'une nation : (club, index dans l'effectif) ; club = -1 : joueur du championnat local
struct PoolPlayer { int club; int idx; const Player* p; };
std::vector<PoolPlayer> coachPool(int nation, int maxAge = 0, int cat = 0);   // cat : 0 A, 1 Espoirs, 2 U19, 3 U17, 4 olympique
void generateFakeSquad(Team& t, int count, std::vector<Player>& out);   // world.cpp
struct CoachCall;
const Player* coachFind(const CoachCall& c, int* club = nullptr);
const char* resultLevelName(int lvl);

// ------------------------------------------------------------------ Saison
struct PendingMatch { int comp = -1; int match = -1; };

struct Season {
    std::vector<Competition> comps;
    std::vector<int> controlled;
    double now = 0;
    int year = 2026;
    bool finished = false;
    std::vector<std::string> news;
    bool isControlled(int t) const { return std::find(controlled.begin(), controlled.end(), t) != controlled.end(); }
    PendingMatch advance(bool simulateUserMatches = false);
    PendingMatch peekNext() const;
    void recordResult(int comp, int match, bool physical=false);
    void finishRoundOthers(int comp, int match);
    void checkRound(int comp);
    int mode = 0;
};

// ------------------------------------------------------------------ Pyramides
enum TierScope { SC_NATIONAL = 0, SC_ZONE, SC_REGION, SC_DEPT };

struct TierConf {
    std::string name;
    int scope = SC_NATIONAL;
    int groupsPerPool = 1;    // -1 : dépend de la ligue / du district (table)
    int groupSize = 18;
    int up = 0;
    int down = 0;
    bool flexible = false;
    bool noReserves = false;  // les réserves ne peuvent pas y évoluer
    int tb = TB_FFF;
    int barrageUp = 0;        // nombre de barragistes qui jouent la montée
    int ptsWin = 3;           // points pour une victoire (éditeur de compétitions)
    int ptsDraw = 1;          // points pour un match nul
    int ptsLoss = 0;          // points pour une défaite
    int ptsForfeit = 0;       // points pour une équipe déclarée forfait
    int legs = 2;             // matchs contre chaque adversaire : 1 aller simple, 2 aller-retour, 3, 4
};

struct Pool {
    int tier = 0;
    int key = 0;
    int nGroups = 1;          // nombre de groupes cible (pools régionaux / départementaux)
    int size = 12;            // taille des groupes
    int upCap = -1;           // nombre maximum de promus pour tout le pool (-1 = 1 par groupe)
    bool terminal = false;    // dernier niveau du district : taille libre, pas de relégation
    std::vector<int> clubs;
    std::vector<std::vector<int>> groups;
    std::vector<int> comps;
    std::vector<int> groupSizes;
    // Regulatory capacities for subsequent seasons; empty uses nGroups * size.
    std::vector<int> nextGroupSizes;
    bool officialGroups = false;
};

int makeU19Team(World& w, int parent);
int makeU17Team(World& w, int parent);
int makeYouthTeam(World& w, int parent, int kind);     // kind 1 : U19, 2 : U17, 3 : U15
// districts de football (districts.cpp)
int numDistricts();
std::string districtName(int d);
std::string districtFullName(int d);
int districtRegion(int d);
const std::vector<int>& districtDepts(int d);
int districtPopulation(int d);
int districtIndex(const std::string& name);
int districtFor(int dept, const std::string& town, const std::string& club);
void assignDistricts(World& w);   // pyramid.cpp : équipe U19 d'un club

struct Pyramid {
    std::string country;
    std::string name;
    std::vector<TierConf> tiers;
    std::vector<Pool> pools;
    int barrageUp = 0;
    int dom = -1;
    int poolIndex(int tier, int key) const;
    int keyFor(int tier, int club) const;
};

int zoneOfRegion(int region);
int numZones();
const char* zoneName(int z);

// ------------------------------------------------------------------ Coefficients UEFA
struct UefaCountry {
    std::string code;
    float pts[5] = { 0, 0, 0, 0, 0 };  // 5 dernières saisons (0 = la plus ancienne)
    float cur = 0;                     // saison en cours
    int clubs = 0;                     // clubs engagés cette saison
    float total() const { float s = 0; for (float p : pts) s += p; return s; }
};

struct Writer; struct Reader;
// palmarès enregistré : code de compétition (HC_*), saison (année de fin), vainqueur, finaliste
enum HonourComp { HC_L1 = 0, HC_L2, HC_CDF, HC_CDL, HC_TDC, HC_UCL, HC_UEFA, HC_SUPERUEFA, HC_INTERTOTO, NUM_HC };
struct HonourRec { int32_t comp = 0, year = 0, winner = -1, runner = -1; };
int histCount(int comp);                                                     // palmarès historique (data_honours.cpp)
void histEntry(int comp, int i, int& year, const char*& winner, const char*& runner);
const char* honourCompName(int comp);
const char* histVenue(int comp, int year);                  // lieu de la finale (palmarès historique)
std::vector<std::string> historicHonours(int team);           // titres réels d'un club (avant la carrière)
int resolveHistWinner(const char* name);
// ------------------------------------------------------------------ Carrière
struct EuroSpots { std::vector<int> uclQ1, uclQ2, uclQ3, uclGS, uefaQR, uefaR1, itR1, itR2, itR3; };
// nouvelle formule : listes d'accès (C1 : 0 Q1, 1 Q2, 2 Q3, 3 barrage, 4 phase de ligue, 5 champions, 6 réserve ;
// C3 : 7 Q3, 8 barrage, 9 phase de ligue, 10 réserve ; C4 : 11 Q1, 12 Q2, 13 Q3, 14 barrage, 15 phase de ligue, 16 réserve)
enum { NE_COUNT = 17 };
struct NewEuroSpots { std::vector<int> l[NE_COUNT]; };

// ------------------------------------------------------------------ Manager
struct TransferOffer { int32_t pid = 0, club = -1, fee = 0, expires = 0; };
struct TransferRec { int32_t pid = 0; int from = -1, to = -1, fee = 0, year = 0; char name[40] = { 0 }; };
// travaux au stade
enum ProjectKind { PJ_SEATS = 0, PJ_COVER, PJ_VIP, PJ_BUVETTE, PJ_BOUTIQUE, PJ_PARKING, PJ_LIGHTS, PJ_PITCH, PJ_SCREEN, PJ_VESTIAIRES, PJ_MUSEUM, PJ_UPGRADE_SEATS, NUM_PJ };
struct Project { int32_t kind = 0, stand = 0, amount = 0, monthsLeft = 0, cost = 0; };
// staff
enum StaffRole { SR_ADJOINT = 0, SR_PHYSIO_PREP, SR_GK, SR_SCOUT, SR_MEDIC, SR_YOUTH, SR_RESERVE, SR_DIRECTOR, NUM_SR };
const char* staffRoleName(int r);
const char* staffRoleDesc(int r);
struct StaffMember { char name[32] = { 0 }; int32_t role = 0, level = 1, wage = 0, age = 40; uint8_t gender = 0, pad[3] = { 0, 0, 0 }; };   // genre : 0 homme, 1 femme (version 14)
struct StaffMemberV13 { char name[32]; int32_t role, level, wage, age; };

// arbitres
struct Referee { const char* name; int severity; int homeBias; int advantage; int consistency; int level; bool female = false; };
extern const Referee REFEREES[];
extern const int NUM_REFEREES;
int refereeFor(int comp, int match);

struct ManagerState {
    int boardRequests = 0;          // demandes au président cette saison
    uint8_t trainFocus = 0;         // entraînement : 0 général, 1 physique, 2 technique, 3 défense, 4 attaque, 5 gardiens
    uint8_t trainInt = 1;           // intensité : 0 légère, 1 normale, 2 intense
    bool managerMode = true;        // mode manager (finances, mercato, stade, staff) ou matchs seulement
    int64_t sponsorIncome = 0;      // sponsor maillot (k€ / an)
    std::vector<StaffMember> staff;
    std::vector<int> ctrlReserves;  // réserves dirigées par le joueur
    bool noSack = false;            // option : pas de licenciement
    int statusChoice = -1;          // statut choisi (divisions régionales)
    bool needStatus = false;        // choix du statut à faire en début de saison
    std::vector<Project> projects;
    int64_t seasonGate = 0, seasonShop = 0, seasonStadiumCost = 0;
    int64_t budget = 0;             // k€
    int64_t seasonIncome = 0, seasonWages = 0, seasonTransfers = 0;
    int64_t incomeBase = 0;         // revenus annuels (droits TV, sponsors, subventions)
    int objective = 3;              // 0 titre, 1 montée, 2 haut de tableau, 3 milieu de tableau, 4 maintien
    int objTarget = 10;             // place à atteindre
    int confidence = 60;            // confiance du président (0-100)
    int lastMonth = -1;
    int sacked = 0;
    std::vector<TransferRec> transfers;
    std::vector<TransferOffer> offers;   // offres reçues pour les joueurs du club (version 10)
    // version 11 : délégation à l'adjoint, mode Full Manager
    bool delegTrain = false, delegSubs = false, fullManager = false;
    // version 14 : recettes de Coupe de France, droits TV, DNCG
    bool cdfGiveAll = false;        // club pro contre un amateur : lui laisser toute la recette
    int64_t tvShare = 0;            // droits TV (part fixe) inclus dans les revenus
    int dncg = 0;                   // avertissements de la DNCG (finances)
    int bankrupt = 0;               // dépôt de bilan prononcé (appliqué à la fin de la saison)
    int sponsorYears = 0;           // version 15 : saisons restantes du contrat de sponsor maillot (0 = sans durée)
    int namingYears = 0;            // saisons restantes du contrat de nom du stade
    // build 5 : mercato, joker, DNCG, fair-play financier (version 18)
    uint8_t mercatoFlags = 0;       // annonces de presse déjà publiées (ouverture, dernier jour, clôture)
    uint8_t jokerUsed = 0;          // joueur joker recruté hors mercato cette saison
    uint8_t fpfStrikes = 0;         // infractions au fair-play financier de l'UEFA
    uint8_t fpfBan = 0;             // exclusion des coupes d'Europe la saison suivante
    int64_t wageCapK = 0;           // masse salariale maximale fixée par la DNCG (k€ / an, 0 = pas d'encadrement)
};
// prêt d'un joueur (retour au club d'origine en fin de saison)
struct Loan { int32_t pid = -1, from = -1, to = -1; int16_t year = 0; uint8_t wagePct = 50, pad = 0; };

enum CareerKind { CK_CLUB = 0, CK_INTL = 1, CK_CUSTOM = 2 };

// archives d'un club (version 14) : meilleurs parcours, meilleur classement, plus larges victoires
struct ArchRun { int16_t depth = -1, year = 0; char label[26] = { 0 }; };
struct ClubArch {
    int16_t lgTier = 99, lgPos = 0, lgYear = 0; char lgName[40] = { 0 };
    ArchRun cdf, cdl, reg, dist, ucl, uefa, regSuper;
    int16_t bigWinGF = -1, bigWinGA = 0, bigWinYear = 0; int32_t bigWinOpp = -1;
    int16_t bigLossGF = 0, bigLossGA = -1, bigLossYear = 0; int32_t bigLossOpp = -1;
    int16_t seasons = 0, titles = 0, promotions = 0, relegations = 0;
};
// archives des compétitions : vainqueur, finaliste, meilleurs buteur et passeur de chaque saison ; classement « all-time »
// Distinctions annuelles : identité figée pour conserver les lauréats après un transfert ou une retraite.
struct BallonNominee { char name[64]={}; int32_t team=-1,pid=0,score=0,rating=0,goals=0,assists=0,apps=0; };
struct SeasonAwards { int32_t year=0; char competition[96]={}; uint8_t presented=0,valid[6]={},pad=0; BallonNominee winners[6]; };
struct PoleRecruit { int32_t pid=0,dest=-1,year=0; };
struct ClubDebut { int32_t team=-1,year=0; };
bool clubFirstSeason(int team);
int lowestLocalTier(const Pyramid& p,int team);

struct BallonEdition { int32_t year=0; uint8_t presented=0,count[2]={},pad=0; BallonNominee podium[2][3]; };
double ballonDate(int year);

struct CompArchRec { char comp[48] = { 0 }; int16_t year = 0, goals = 0, assists = 0, pad = 0; int32_t winner = -1, runner = -1, scorerTeam = -1, assistTeam = -1; char scorer[30] = { 0 }, assister[30] = { 0 }; };
struct CompAllTime { char comp[48] = { 0 }; int32_t pid = 0, team = -1; char name[30] = { 0 }; int16_t goals = 0, assists = 0, seasons = 0, pad = 0; };
std::string compFamily(const std::string& name);    // nom de la compétition sans l'année
struct ArchScorer { int32_t pid = 0; char name[30] = { 0 }; int16_t goals = 0, assists = 0, apps = 0, pad = 0; };

// carrière de sélectionneur : joueur convoqué (pid du joueur dans son club, club = -1 : championnat local)
struct CoachCall { int32_t pid = 0; int32_t club = -1; int16_t caps = 0, goals = 0; };
struct CoachCampaign { int type = 0, year = 2028, format = 0; };

struct CustomCompDef {
    std::string name;
    int format = 0;           // 0 championnat, 1 coupe, 2 groupes + phase finale
    int legs = 2;
    std::vector<int> teams;
    int groups = 4;
};

// vie privée du manager ou du joueur incarné (carrière de joueur / joueuse) ; « valise à l'arbitre »
struct PlayerLife {
    uint8_t isPlayer = 0;           // 1 : carrière de joueur / joueuse ; 0 : vie privée du manager
    uint8_t house = 0, car = 0;     // logement 0 studio ... 4 villa ; voiture 0 aucune ... 4 supercar
    uint8_t relation = 0;           // 0 célibataire, 1 en couple, 2 fiancé(e), 3 marié(e)
    uint8_t love = 0;               // complicité du couple (0-100)
    uint8_t morale = 65;            // moral (0-100) : influence les performances
    uint8_t betPick = 0;            // pari : 0 victoire à domicile, 1 nul, 2 victoire à l'extérieur
    uint8_t bribeKind = 0;          // 0 arbitre, 1 joueur adverse, 2 équipe adverse
    uint8_t heat = 0;               // soupçons (0-100) : risque d'enquête
    uint8_t adminRelegate = 0;      // rétrogradation administrative prononcée (fin de saison)
    uint8_t suspendedM = 0;         // joueur : mois de suspension (corruption)
    uint8_t partnerGender = 1;
    int32_t pid = -1;               // identifiant du joueur incarné
    int32_t salaryK = 0;            // salaire annuel (k€)
    int64_t cash = 0;               // argent personnel (€)
    int32_t betComp = -1, betMatch = -1, betStake = 0; float betOdds = 0;
    int32_t betsWon = 0, betsLost = 0; int64_t betBalance = 0;
    int32_t bribeComp = -1, bribeMatch = -1, bribeTeam = -1;
    int32_t bribes = 0, bribesCaught = 0;
    char partner[32] = { 0 };
};
const char* lifeHouseName(int k);
const char* lifeCarName(int k);
int64_t lifeHousePrice(int k);
int64_t lifeCarPrice(int k);
float lifeBribeDelta(const MatchRes& m);      // écart de niveau dû à une valise (simulation)

struct Career {
    Season season;
    std::vector<Pyramid> pyramids;
    int userTeam = -1;
    int year = 2026;
    int kind = 0;
    int cdf = -1;
    int cdfNational = -1;
    int cdfHolderDirect = -1;        // tenant de la Coupe de France hors L1 : entrée directe en 32es
    std::vector<int> cdfRegional;
    std::vector<int> nationalCups;
    std::vector<int> customCups;             // coupes créées dans l'éditeur (kind 72)
    std::map<int, int> prevCustomCupWinner;  // id de coupe éditeur -> vainqueur de la saison précédente
    std::vector<int> regionalCups;
    std::vector<int> deptCups;
    std::vector<int> superCups;
    int ucl = -1, uel = -1, uecl = -1, uefaSuper = -1;
    EuroSpots nextEuro;
    std::vector<UefaCountry> uefa;
    std::vector<std::string> history;
    std::vector<int> pendingNewClubs;  // clubs créés en cours de saison (entrent la saison suivante)
    int prevUclWinner = -1, prevUefaWinner = -1;
    std::map<std::string, int> prevChampion, prevCupWinner, prevRunnerUp;
    // International
    int intlType = -1;
    int intlFormat = 0;                     // 0 format classique, 1 grand format (Coupe du monde 48, Euro 24)
    // version 7
    std::vector<HonourRec> honourLog;       // vainqueurs des grandes compétitions en carrière
    std::vector<std::string> honourVenue;   // lieu de la finale de chaque entrée de honourLog (version 8)
    std::string uclFinalVenue, uefaFinalVenue, tdcVenue;
    std::map<int, std::string> cupVenue;    // finales des coupes régionales / de district : terrain neutre désigné en début de saison (version 12)
    int intertoto = -1;                     // Coupe Intertoto
    // jeunes (version 9) : phase finale du National U19, Coupe Gambardella (tours régionaux + phase nationale), coupes U19
    int u19Final = -1, gambNational = -1;
    std::vector<int> gambRegional, u19Cups;
    int u19Pyramid() const;
    int u17Pyramid() const;
    int u15Pyramid() const;
    int youthPyramid(int team) const;
    int createYouth(int club, int ykind, std::string& err);
    int u17Final = -1;                   // phase finale du National U17 (version 12)
    int lastU17Champ = -1;
    int createU19(int club, std::string& err);
    void onU19CompDone(int comp);
    // Ligue des champions U19 (version 11) : tour préliminaire (match sec) puis phase de groupes (8 x 4) et phase finale
    int youthPrelim = -1, youthUcl = -1, lastU19Champ = -1;
    std::vector<int> youthDirect;        // qualifiés directs pour la phase de groupes (en attente du tour préliminaire)
    void setupYouthUcl();
    void startYouthGroups(const std::vector<int>& teams);
    std::string managerName; int managerNation = -1; uint8_t managerSkin = 0, managerHair = 0; int managerAge = 45;
    std::string managerPlace; uint8_t managerBday = 0, managerBmonth = 0;   // version 11
    uint8_t managerGender = 0;                                              // version 14
    std::vector<std::string> reconversions;                                 // anciens joueurs reconvertis (version 11)
    void saveV11(Writer& w) const;
    void saveV12(Writer& w) const;
    void loadV12(Reader& r);
    void saveV13(Writer& w) const;
    void loadV13(Reader& r);
    void saveV14(Writer& w) const;
    void loadV14(Reader& r);
    // ---- version 15 : Ligue des nations UEFA (ligue de chaque sélection), options de carrière
    std::vector<int8_t> nlLeague;             // ligue (0 = A ... 3 = D) de chaque nation pour la prochaine édition
    int nlYear = -1;                          // édition en cours (année de début)
    struct Opts {
        uint8_t cdl = 1;                      // Coupe de la Ligue (sinon sa place européenne revient au championnat)
        uint8_t awayGoals = 1;                // buts à l'extérieur en coupes d'Europe
        uint8_t cdfET = 1;                    // prolongation en Coupe de France (la finale en a toujours une)
        uint8_t euroFormat = 0;               // 0 formule 2003, 1 nouvelle formule (phase de ligue à 36)
        uint8_t lite = 0;                     // mode Championnat (mode foot) : sans gestion, transferts simplifiés
        uint8_t disableManagerLife = 0;       // anciennes sauvegardes : options actives (octets réservés à zéro)
        uint8_t disableBribes = 0;
        uint8_t pad[1] = {};
    } opts;
    NewEuroSpots newEuro;                     // qualifiés (nouvelle formule)
    int prevUeclWinner = -1;
    void addNewEuroCups();
    void routeNewEuro();
    // vie privée, carrière de joueur, corruption (build 5)
    PlayerLife life;
    std::vector<BallonEdition> ballonEditions;
    std::map<int,ClubHistory> clubHistories;
    DirectorState director;
    SportingCareer sporting;
    SupporterState supporters;
    PersonalityState personalities;
    bool personalityEnabled() const;
    PersonalityPlayer& personalityPlayer(const Player& p,int club);
    PlayerClubRelation& personalityRelation(PersonalityPlayer& p,int club);
    void personalityInitClub(int club);
    void personalityMonth(int club);
    void personalityMatchEnd(int ci,int mi);
    void personalitySeasonEnd();
    void personalityTransfer(const Player& p,int from,int to);
    void personalityRetire(const Player& p,int club);
    bool personalityMentor(int club,int pid,int mentor,std::string& msg);
    void personalityCaptain(int club,int pid);
    void personalityDecision(Player& p,int club,int type);
    void personalityScout(const Player& p,int club,int quality,int knowledge);
    void personalityEvent(PersonalityPlayer& p,int club,int type,const std::string& msg);
    bool personalityContract(const Player& p,int club,int years,std::string& msg) const;
    bool supportersEnabled() const;
    void supportersInit();
    SupporterProfile& supportersProfile(int club);
    void supportersStartSeason();
    void supportersPrepareMatch(int comp,int match,int weather=-1);
    void supportersOnMatchEnd(int comp,int match);
    bool supportersBookGate(int comp,int match);
    void supportersOnSeasonEnd();
    void supportersOnTransfer(const Player& p,int from,int to,int fee);
    void supportersOnPromotion(int club,int newTier);
    void supportersOnTitle(int club,int comp);
    void supportersOnTicketPriceChange(int club);
    void supportersOnManagerChange(int club,const std::string& name);
    void supportersOnStadiumChange(int club);
    void supportersCampaign(int club,bool charge=true);
    void supportersEvent(int club,int type,const std::string& reason,int intensity,int duration=0,int comp=-1,int match=-1,int pid=0);
    int supportersPendingEvent() const;

    bool museumEnabled() const;
    void museumState(int team);
    void museumMatch(int comp,int match);
    void museumEvents(int comp,int match);
    void museumAttendance(int comp,int match,int attendance);
    void museumEnrich(int comp,int match,const MuseumMatch& actual);
    void museumCompetition(int comp);
    void museumSeasonEnd(bool afterMoves);
    void museumTransfer(const Player& player,int from,int to,int fee);
    void museumLeave(int team);
    std::vector<SeasonAwards> seasonAwards;
    std::vector<ClubDebut> clubDebuts;
    std::vector<int> poleTeams;
    std::vector<PoleRecruit> poleRecruits;
    int poleCup=-1;
    bool awardsEnabled() const;
    int prepareSeasonAwards();
    bool polesEnabled() const;
    void startPoleCup();
    std::vector<BallonNominee> poleRanking() const;
    bool recruitPole(int pid,int destination,std::string& err);
    void finishPoleSeason(std::vector<std::string>& news);
    void markClubDebut(int team,int year);
    bool ballonEnabled() const;
    bool ballonTick();
    int ballonPending() const;
    int selectedPyramid() const;
    std::string selectedCountryCode() const;
    std::string selectedCountryName() const;
    int domesticCup(int team = -1) const;
    bool hasDncg() const;
    bool managerLifeEnabled() const { return life.isPlayer || !opts.disableManagerLife; }
    void lifeStart(bool isPlayer, int pid);
    int32_t lifeSalaryK() const;
    Player* lifePlayer(int* team = nullptr);
    void lifeMonth();
    void lifeAfterMatch(int comp, int mi);
    bool lifeBuy(int what, int level, std::string& err);     // what : 0 logement, 1 voiture
    bool lifeGift(int kind, std::string& err);                // 0 fleurs, 1 restaurant, 2 bijou, 3 voyage
    bool lifeDate(std::string& err);                          // sortir pour faire une rencontre
    bool lifePropose(std::string& err);
    bool lifeWedding(std::string& err);
    bool lifeBreakUp(std::string& err);
    bool lifeBet(int comp, int mi, int pick, int stake, std::string& err);
    float lifeOdds(int comp, int mi, int pick) const;
    void lifeResolveBets();
    int64_t bribeCost(int comp, int mi, int kind) const;      // k€
    bool bribe(int comp, int mi, int kind, std::string& err);
    void saveV17(Writer& w) const;
    void loadV17(Reader& r);
    void saveV15(Writer& w) const;
    void loadV15(Reader& r);
    // ---- archives des clubs (version 14)
    std::map<int, ClubArch> archive;
    std::map<int, std::vector<ArchScorer>> archScorers;
    void archiveSeason();                     // fin de saison : parcours, classements, buteurs
    // ---- carrière de sélectionneur (version 14) : campagnes successives (qualifications + phase finale), convocations
    bool coach = false;
    int coachNation = -1, coachCamp = 0, coachObjective = 3, coachSacked = 0;
    int coachCat = 0;                         // version 15 : 0 sélection A, 1 Espoirs (et olympique), 2 U19, 3 U17
    std::vector<int> coachU21Podium;          // trois premiers du dernier Euro Espoirs (qualifiés pour les JO)
    int coachTeam() const;                    // équipe dirigée (A ou sélection de jeunes)
    int coachTeamCat() const;                 // catégorie de l'équipe dirigée pour la campagne en cours
    int coachMaxAge() const;
    int coachSquadMax() const { return coachCat ? 23 : 26; }
    std::vector<CoachCall> coachCalls;
    std::vector<int> coachNextHosts;          // organisateurs désignés de la campagne suivante
    std::vector<std::string> coachLog;        // bilan des campagnes
    void newCoachCareer(int nation);
    void startCoachCampaign();
    void endCoachCampaign(std::vector<std::string>& msgs);   // bilan, vieillissement, campagne suivante
    std::vector<CoachCampaign> coachPlan() const;
    std::vector<int> campaignHosts(const CoachCampaign& c) const;   // organisateurs officiels ou désignés
    void coachAutoSelect();
    void coachApplySquad();                   // copie des convoqués dans l'effectif de la sélection
    int coachResult() const;                  // 0 vainqueur ... 5 non qualifié
    std::string coachObjectiveText() const;
    int cdfAskYear = -1;                 // inscription (facultative en district) à la Coupe de France : saison déjà demandée
    int userCdfComp() const;
    void sheetRules(int comp, int& sheet, int& subs, bool& rolling) const;   // joueurs sur la feuille, remplacements             // tours régionaux de Coupe de France du club (s'il joue en district), sinon -1
    bool withdrawFromCup(int comp, int team);
    void loadV11(Reader& r);
    void recordSeasonHistory();          // fin de saison : lignes d'historique des joueurs
    std::vector<std::string> newsRead;      // messages déjà lus (texte)
    std::vector<int> retiring;              // joueurs (id) qui ont annoncé leur retraite en fin de saison
    void saveV7(Writer& w) const;
    void loadV7(Reader& r);
    void resetV7();
    void logHonour(int comp);
    EuroSpots previewEuro();                // qualifiés européens de la saison prochaine (aperçu fin de saison)
    bool intlWithQual = true;
    std::vector<int> intlHosts;
    std::vector<int> qualComps;
    int finalComp = -1;
    int intlStage = 0;
    CustomCompDef custom;
    ManagerState mgr;

    void newClubCareer(int team, int year);
    // mode Coupes d'Europe : seules la Ligue des champions et la Coupe UEFA sont jouées (qualifications simulées)
    bool euroOnly = false;
    void newEuroCareer(const std::vector<int>& ctrl, int year);
    static std::vector<int> euroCandidates();
    void addEuroCups();
    // version 9
    void saveV9(Writer& w) const;
    void loadV9(Reader& r);
    void saveU19(Writer& w) const;
    void loadU19(Reader& r);
    void startSeason();
    void endSeason();
    void newInternational(int type, bool withQual, const std::vector<int>& controlled, int year, const std::vector<int>& hosts, int format = 0);
    void newCustom(const CustomCompDef& def, const std::vector<int>& controlled);
    void onCompetitionDone(int comp);
    void onStageDone(int comp, int stage);
    void update();
    bool save(const char* path) const;
    bool load(const char* path);
    int tierOfTeam(int team, int* pyramidOut = nullptr, int* poolOut = nullptr, int* groupOut = nullptr) const;
    std::string teamLevelName(int team) const;
    int uefaRank(const std::string& code) const;
    void initUefa();
    void addClubToPyramid(int team);
    std::string divisionRules(int comp) const;
    // manager
    bool transferWindow() const;       // mercato ouvert ?
    std::string windowText() const;
    // build 5 : joker, extra-communautaires, prêts, numéros, DNCG, fair-play financier, presse du mercato
    bool jokerAvailable() const;
    std::vector<Loan> loans;
    std::vector<CompArchRec> compArch;
    std::vector<CompAllTime> compAllTime;
    void archiveComps();
    bool loanIn(int pid, std::string& err, int wagePct=50);
    bool loanOut(int pid, std::string& err, int preferred=-1, int wagePct=50);
    bool buyLoanOption(int pid,int fee,std::string& err);
    void returnLoans(std::vector<std::string>& msgs);
    int loansIn(int team) const;
    int loansOut(int team) const;
    int loanLimitIn(int team) const;
    int loanLimitOut(int team) const;
    void mercatoPress();
    void fpfCheck(std::vector<std::string>& msgs);
    void mgrInit();                    // budget / objectifs en début de saison
    void mgrTick();                    // mois écoulés : salaires, droits TV
    void mgrAfterMatch(int comp, int mi);
    void mgrEndSeason(std::vector<std::string>& msgs);
    void aiTransfers(int n);
    int64_t tvIncome(int team) const;
    bool buyPlayer(int pid, int fee, std::string& err);
    // négociation (version 14) : indemnité, salaire, durée ; prime à la signature
    int signPlayer(int pid, int fee, int wage, int years, std::string& msg);   // 1 signé, 0 refus du club, -1 refus du joueur, -2 impossible
    void refreshFreeAgents(int n);            // clubs qui libèrent des joueurs, joueurs en fin de contrat
    bool changeWage(int pid, int pct, std::string& msg);
    bool extendContract(int pid, std::string& msg);
    bool sellPlayer(int pid, int toTeam, int fee, std::string& err);
    void releasePlayer(int pid);
    std::string objectiveText() const;
    void trainingMonth();
    void changeClub(int team);
    int findBuyer(int pid, int& fee) const;
    int64_t wageBill(int team) const;
    // stade / statut
    void updateStatuses();
    void setUserStatus(int st);
    int forcedStatus(int team) const;      // -1 si libre (ligue régionale)
    void homeMatchDay(int comp, int mi);   // affluence et recettes
    bool startProject(const Project& p, std::string& err);
    static int64_t projectCost(const Team& t, int kind, int stand, int amount);
    static int projectMonths(int kind, int amount);
    int64_t namingOffer() const;
    // pelouse et foncier
    int64_t turfRepairCost() const;
    int64_t turfReplaceCost() const;
    bool repairTurf(std::string& err);
    bool replaceTurf(std::string& err);
    int64_t stadiumBuyPrice() const;
    bool buyStadium(std::string& err);
    bool requestExtension(std::string& err);
    int64_t annexCost(int kind) const;          // 0 réserve, 1 jeunes, 2 entraînement
    bool buildAnnex(int kind, std::string& err);
    int expectedAttendance(int stand, bool vip) const;
    int cdl = -1;                           // Coupe de la Ligue
    int superRegions = -1;                  // Méga Coupe des Régions (vainqueurs des supercoupes de région)
    std::vector<int> regSuperCups;          // supercoupes de région (champion R1 - vainqueur coupe régionale), version 14
    std::map<int, int> prevR1Champ;         // champion de Régional 1 de chaque région (saison précédente)
    std::map<int, int> prevRegCupWinner;    // région -> vainqueur de la coupe régionale précédente
    // staff, réserves, amicaux
    int staffLevel(int role) const;
    void initStaff();
    bool sportingMode()const{return sporting.active!=0;}
    void sportingStart(int club,int year,const SportingProfile& profile);
    void sportingAttach(int club);
    void sportingTick();
    void sportingSeasonEnd();
    void sportingPrepare(int club);
    void sportingMatch(int comp,int match);
    void sportingJobs();
    bool sportingApply(int club,bool negotiate,std::string& msg);
    void sportingLeave(int when,const std::string& reason);
    bool sportingRenew(std::string& msg);
    bool sportingHireCoach(const SportingCoach& c,bool dismiss,std::string& msg);
    bool sportingProject(std::string& msg);
    bool sportingBoard(int pid,int fee,int wage,std::string& msg);
    bool sportingSpend(int pid,int fee,int wage,std::string& msg,int oldWage=0)const;
    void sportingTransfer(const Player& p,int from,int to,int fee);
    bool sportingMeeting(int choice,std::string& msg);
    void sportingLearn(int skill,int amount);
    bool directorEnabled() const;
    void directorSync();void directorAnalyse(bool research=true);void directorTick();
    int directorTargetScore(const Player& p,int club,int position) const;
    bool directorCanSpend(int fee,int wage,std::string& reason,int oldWage=0) const;
    bool directorProtected(int pid) const;
    void directorPolicy(int pid,int status,bool locked);
    bool directorStart(int pid,bool loan,std::string& msg);
    bool directorApprove(int dossier,std::string& msg);
    int directorLoanClub(int pid) const;
    bool directorExercise(int pid,std::string& msg);
    bool directorRenew(int pid,std::string& msg);
    int createReserve(std::string& err);
    void genOffers(int n);                   // offres des autres clubs pour nos joueurs
    bool answerOffer(int k, int action, std::string& msg);   // 0 accepter, 1 refuser, 2 négocier
    void boardReview();                      // point du président à mi-saison
    int64_t uclMarketPool(std::vector<std::string>& msgs);   // droits TV de la C1
    void addFriendly(const std::vector<int>& invited, bool tournament);
    void syncControlled();
};
float teamBonus(int team);     // bonus du staff (équipe du joueur)
int injuryReduction(int team);
int staffWage(int level, int status);
StaffMember makeStaff(int role, int level, int status, Rng& r);
void initStadium(Team& t, int tierHint, float tierAvg);
void ensureStadium(int team);
void stadiumSetCapacity(StadiumInfo& S, int cap);
void applyContracts(Team& t, bool keepUser);
const char* projectName(int k);
struct Career;
void stadiumMonth(Career& K);
std::string cdfEntryText(int region);
int playerWage(const Player& p);
bool isNonEU(const Player& p, int team);       // joueur extra-communautaire (hors UE / EEE / Suisse)
int nonEuLimit(int team);                      // extra-communautaires autorisés sur la feuille de match (clubs français)
void autoNumbers(int team);                    // attribution automatique des numéros (1 à 99)
// économie (economy.cpp)
int teamReputation(int team);
void addReputation(int team, int delta);
const char* reputationLabel(int rep);
int freeAgentsTeam();                       // équipe technique des joueurs libres
int contractYears(const Player& p);
int wageDemand(const Player& p, int toTeam);   // salaire demandé pour rejoindre un club (k€ / an)
int refereeFee(int comp, int home);         // frais d'arbitrage d'un match (€)
int minCapacity(int tier);                  // capacité minimale du stade selon la division
struct GateInfo { int64_t brut = 0, tva = 0, prelev = 0, arbitrage = 0, orga = 0, net = 0, part = 0; int att = 0; bool shared = false; std::string opp; };
extern GateInfo g_lastGate;
extern const char* CONTRACT_NAMES[3];
std::string money(int64_t k);        // montant en k€ -> texte
extern Career g_career;
extern std::vector<Pyramid> g_basePyramids;
// éditeur de compétitions de carrière (compedit.cpp)
struct CupRule {
    bool enabled = true, et = true, neutralFinal = true;
    bool euroQual = true;            // le vainqueur utilise la place européenne attribuée à la coupe nationale
    int finalSize = 0;               // 0 = tous les clubs éligibles ; sinon nombre de clubs dans le tableau principal
};
struct GeoRegionDef { int id = -1; std::string country, name; };
struct GeoDistrictDef { int id = -1, region = -1; std::string country, name; };
enum EditorCupScope { ECS_NATIONAL = 0, ECS_REGION = 1, ECS_DISTRICT = 2 };
enum EditorCupQual { ECQ_NONE = 0, ECQ_NATIONAL_CUP = 1, ECQ_EURO_CUP_SLOT = 2, ECQ_UCL = 3, ECQ_UEL = 4, ECQ_UECL = 5 };
struct EditorCupDef {
    int id = -1;
    std::string country, name;
    int scope = ECS_NATIONAL;
    int geoKey = -1;                 // région ou district selon scope
    int minTier = 0, maxTier = 99;  // niveaux de championnat éligibles
    int finalSize = 32;
    int qual = ECQ_NONE;
    bool enabled = true, et = true, neutralFinal = true;
};
CupRule cupRuleFor(const std::string& country);
void setCupRule(const std::string& country, const CupRule& r);
std::vector<GeoRegionDef> geoRegions(const std::string& country);
std::vector<GeoDistrictDef> geoDistricts(const std::string& country, int region = -1);
std::string geoRegionName(const std::string& country, int id);
std::string geoDistrictName(const std::string& country, int id);
int geoAddRegion(const std::string& country, const std::string& name);
bool geoRenameRegion(const std::string& country, int id, const std::string& name);
bool geoDeleteRegion(const std::string& country, int id);
int geoAddDistrict(const std::string& country, int region, const std::string& name);
bool geoRenameDistrict(const std::string& country, int id, const std::string& name);
bool geoDeleteDistrict(const std::string& country, int id);
std::vector<EditorCupDef>& editorCups();
int addEditorCup(const std::string& country, const std::string& name);
bool deleteEditorCup(int id);
EditorCupDef* editorCupById(int id);
void pyrSetTierScope(Pyramid& P, int tier, int scope);
void pyrRefreshGeo(Pyramid& P);
bool pyrEditable(const Pyramid& P);
bool pyrStructEditable(const Pyramid& P);
bool tierClubsEditable(const Pyramid& P, int tier);
bool pyrEdited(const std::string& country);
void markPyrEdited(const std::string& country);
void pyrRelink(Pyramid& P, int changedTier, bool fromDown);
void pyrMoveClub(Pyramid& P, int club, int toTier);
void pyrAddTier(Pyramid& P, int after, const std::string& name);
bool pyrDeleteTier(Pyramid& P, int tier);
bool pyrRestore(Pyramid& P);
void saveCompEdits();
void loadCompEdits();
void formGroups(Pyramid& P);
// football féminin (women.cpp)
void buildWomen(World& w);
void injectWomenStars(Team& t, Rng& r);
bool isWomenPyramid(const Pyramid& P);
bool isWomenTeam(int t);
struct Career;
int womenPyramid(const Career& K, const char* country);
void womenStartSeason(Career& K);
void womenOnCompDone(Career& K, int comp);
bool uwclStageFinished(Competition& C, const std::vector<int>& winners);
int jflMin(int team);
// coupes continentales hors Europe (continental.cpp)
void continentalStartSeason(Career& K);
bool isContinentalKind(int k);
int continentalWinner(const Career& K, int conf);
// légendes de la Coupe du monde (legends.cpp)
std::vector<int> legendTeams(int ed);
void legendStart(Career& K, int ed, const std::vector<int>& ctrl);
bool legendStageFinished(Competition& C, const std::vector<int>& winners);
std::string legendVenue(int comp, int mi);
void legendSheet(int ed, int& sheet, int& subs);
bool legendGoldenGoal(int ed);
double legendCalendarTime(int ed, double fraction);
double legendRoundTime(int ed, int teams, const std::string& name);
double legendGroupTime(int ed, int stage, int round, int count);
std::string legendDateText(double time, int year);
void legendRepairCalendar(Competition& c);
int marneCupPart(const Competition& c);
void createMarneCups(Career& k, const std::vector<int>& teams);
void marneDraw(Competition& c, int round, const std::vector<int>& survivors);
bool marneStageFinished(Competition& c, const std::vector<int>& winners);
const char* legendFormatName(int ed);
bool isJfl(const Player& p, int team);
int poolGroupCount(const Pyramid& P, const Pool& pool);
int poolTarget(const Pyramid& P, const Pool& pool);
void applyPyramidSeasonResults(Career& career, int pyramid);
std::vector<std::string> validateFrancePyramid(const Pyramid& P, bool published, bool normalized = false);
int regionDeptCount(int region);
std::string poolLabel(const Pyramid& P, const Pool& pl, int g);

enum IntlType { IT_WORLDCUP = 0, IT_EURO, IT_CAN, IT_COPA, IT_ASIA, IT_GOLD, IT_OFC,
                IT_OLYMPICS, IT_EURO21, IT_EURO19, IT_EURO17,
                IT_OLY_W, IT_EURO_W, IT_WC_W, NUM_INTL };      // féminines : tournoi olympique, Euro, Coupe du monde      // tournoi olympique (U23), Euro Espoirs, Euro U19, Euro U17
int youthCatOfType(int type);                  // youthintl.cpp : 0 A, 1 Espoirs (U21), 2 U19, 3 U17, 4 olympique (U23)
int youthNationTeam(int nation, int cat);      // sélection de jeunes d'une nation (créée à la demande)
int nationOfTeam(int team);
extern const char* INTL_NAMES[NUM_INTL];
bool nationEligible(int n);
bool nlActive(const Career& K);                     // nations.cpp : Ligue des nations UEFA en cours
std::vector<int> nlRanking(const Career& K);
std::vector<int> nlGroupWinners(const Career& K);
void nlSetup(Career& K, int nlYear);
void nlOnCompDone(Career& K, int comp);
std::vector<int> defaultHosts(int type);
int intlYear(int type);

// Éditeur : clubs personnalisés persistants
void loadCustomClubs();
void saveCustomClubs();
