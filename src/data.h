// Données statiques du jeu (nations, clubs, joueurs réels, géographie française)
#pragma once

enum Confed { UEFA = 0, CONMEBOL, CONCACAF, CAF, AFC, OFC, NUM_CONFEDS };

// Cultures pour la génération des noms de joueurs
enum Culture {
    CU_FR = 0, CU_EN, CU_ES, CU_PT, CU_BR, CU_IT, CU_DE, CU_NL, CU_NORD, CU_SLAV, CU_BALK,
    CU_TR, CU_ARAB, CU_AFW, CU_AFE, CU_JP, CU_KR, CU_CN, CU_LATAM, CU_PERS, CU_SEA, CU_IND,
    CU_PAC, CU_GR, CU_HU, NUM_CULTURES
};

// Motifs de maillot
enum KitPattern { KP_PLAIN = 0, KP_VSTRIPES, KP_HOOPS, KP_HALVES, KP_SASH, KP_CHECK, KP_SLEEVES,
                  KP_PINSTRIPES, KP_QUARTERS, KP_BAND, KP_SHOULDERS, KP_CHEVRON, KP_CROSS, KP_DIAGHALF, NUM_PATTERNS };

struct NationDef {
    const char* code;     // trigramme FIFA
    const char* name;     // nom français
    int conf;
    int rating;           // 10..95
    unsigned shirt, shirt2, shorts;
    int pattern;
    int culture;
};

struct ClubDef {
    const char* name;
    const char* shortName;
    int rating;
    unsigned shirt, shirt2, shorts;
    int pattern;
    const char* dept;     // code pays (clubs étrangers hors championnat) ou ""
    const char* stadium;
    const char* dbKey;    // nom du club dans la base de joueurs réels ("" = effectif généré)
};

struct FrClubDef {
    const char* name;
    const char* shortName;
    int rating;
    unsigned shirt, shirt2, shorts;
    int pattern;
    const char* dept;     // code département
    const char* stadium;
    const char* dbKey;
    const char* parent;   // équipe première (pour une réserve) ou ""
};

struct LeagueDef {
    const char* id;
    const char* name;
    const char* country;   // code nation
    int tier;              // 1 = élite
    const ClubDef* clubs;
    int numClubs;
    int culture;
};

struct RegionDef { const char* code; const char* name; };
struct DeptDef { const char* code; const char* name; int region; int population; const char* towns; };
struct NationStadium { const char* code; const char* stadium; };

// Joueurs réels (base EA SPORTS FC 26)
struct PlayerRec {
    const char* name;
    unsigned char pos, num, speed, shoot, pass, tackle, keep, stamina;
    short nation;          // index dans NATIONS (-1 inconnu)
    unsigned char intl;    // 1 = sélectionné en équipe nationale
    unsigned char overall;
    unsigned char age;     // âge (saison 2025-26)
    unsigned char potential; // potentiel (échelle du jeu)
};
struct DbClub { const char* name; int first, count, rating; };

extern const NationDef NATIONS[];
extern const int NUM_NATIONS;
extern const LeagueDef LEAGUES[];
extern const int NUM_LEAGUES;
// championnats complémentaires du monde (data_world_leagues.cpp) : clubs "Nom|ABR|note|c1|c2|stade;..."
struct ExtLeagueDef { const char* id; const char* name; const char* country; int tier; int down; const char* clubs; };
extern const ExtLeagueDef EXT_LEAGUES[];
extern const int NUM_EXT_LEAGUES;
const char* extCupName(const char* country);
// football féminin (data_women.cpp)
struct WomenLeagueDef { const char* country; int tier; const char* name; int down; const char* clubs; };
struct WomenSquadDef { const char* key; const char* players; };   // key : nom du club ou code pays (sélection)
extern const WomenLeagueDef WOMEN_LEAGUES[];
extern const int NUM_WOMEN_LEAGUES;
extern const WomenSquadDef WOMEN_SQUADS[];
extern const int NUM_WOMEN_SQUADS;
extern const WomenSquadDef WOMEN_NATIONS[];
extern const int NUM_WOMEN_NATIONS;
const char* womenCupName(const char* country);
extern const ClubDef EURO_POOL[];
extern const int NUM_EURO_POOL;
extern const ClubDef WORLD_POOL[];
extern const int NUM_WORLD_POOL;
extern const RegionDef REGIONS[];
extern const int NUM_REGIONS;
extern const DeptDef DEPTS[];
extern const int NUM_DEPTS;
extern const PlayerRec PLAYERS[];
extern const int NUM_PLAYERS;
extern const DbClub DBCLUBS[];
extern const int NUM_DBCLUBS;
extern const int NATION_DB_RATING[];
extern const NationStadium NATION_STADIUMS[];
extern const int NUM_NATION_STADIUMS;

// Clubs français
extern const FrClubDef FR_L1[]; extern const int NUM_FR_L1;
extern const FrClubDef FR_L2[]; extern const int NUM_FR_L2;
extern const FrClubDef FR_L3[]; extern const int NUM_FR_L3;
extern const FrClubDef FR_N1[]; extern const int NUM_FR_N1;
extern const FrClubDef FR_N2[]; extern const int NUM_FR_N2;
extern const FrClubDef FR_RESERVES_R1[]; extern const int NUM_FR_RESERVES_R1;

// Noms de joueurs
struct NamePool { const char* first; const char* last; };
extern const NamePool NAME_POOLS[NUM_CULTURES];
extern const char* FEMALE_FIRST[NUM_CULTURES];
