// Districts de football (FFF) : un district couvre un département, plusieurs départements (Alsace, Drôme-Ardèche...)
// ou une partie de département (Nord : Flandres / Escaut ; Pas-de-Calais : Artois / Côte d'Opale...).
// Source : liste des districts par ligue (FFF / Wikipédia « District départemental de football »).
#include "game.h"
#include <cstring>

struct DistrictDef {
    const char* name;       // nom court (« Marne », « Alsace »...)
    const char* full;       // nom officiel
    int region;             // ligue (index REGIONS)
    const char* depts;      // départements couverts (codes séparés par des virgules)
    float share;            // part de la population du (des) département(s) partagé(s)
};

static const DistrictDef DISTRICTS_DEF[] = {
    // Paris Île-de-France
    { "Paris", "District de Paris", 0, "75", 1 }, { "Hauts-de-Seine", "District des Hauts-de-Seine", 0, "92", 1 },
    { "Seine-Saint-Denis", "District de la Seine-Saint-Denis", 0, "93", 1 }, { "Val-de-Marne", "District du Val-de-Marne", 0, "94", 1 },
    { "Essonne", "District de l'Essonne", 0, "91", 1 }, { "Yvelines", "District des Yvelines", 0, "78", 1 },
    { "Val-d'Oise", "District du Val-d'Oise", 0, "95", 1 }, { "Seine-et-Marne", "District de Seine-et-Marne", 0, "77", 1 },
    // Centre-Val de Loire
    { "Cher", "District du Cher", 1, "18", 1 }, { "Eure-et-Loir", "District d'Eure-et-Loir", 1, "28", 1 }, { "Indre", "District de l'Indre", 1, "36", 1 },
    { "Indre-et-Loire", "District d'Indre-et-Loire", 1, "37", 1 }, { "Loir-et-Cher", "District de Loir-et-Cher", 1, "41", 1 }, { "Loiret", "District du Loiret", 1, "45", 1 },
    // Bourgogne-Franche-Comté
    { "Côte-d'Or", "District de la Côte-d'Or", 2, "21", 1 }, { "Doubs-Territoire de Belfort", "District Doubs-Territoire de Belfort", 2, "25,90", 1 },
    { "Jura", "District du Jura", 2, "39", 1 }, { "Nièvre", "District de la Nièvre", 2, "58", 1 }, { "Saône-et-Loire", "District de Saône-et-Loire", 2, "71", 1 },
    { "Haute-Saône", "District de Haute-Saône", 2, "70", 1 }, { "Yonne", "District de l'Yonne", 2, "89", 1 },
    // Normandie
    { "Calvados", "District du Calvados", 3, "14", 1 }, { "Eure", "District de l'Eure", 3, "27", 1 }, { "Manche", "District de la Manche", 3, "50", 1 },
    { "Orne", "District de l'Orne", 3, "61", 1 }, { "Seine-Maritime", "District de Seine-Maritime", 3, "76", 1 },
    // Hauts-de-France
    { "Aisne", "District de l'Aisne", 4, "02", 1 }, { "Artois", "District Artois", 4, "62", 0.6f }, { "Côte d'Opale", "District Côte d'Opale", 4, "62", 0.4f },
    { "Escaut", "District Escaut", 4, "59", 0.38f }, { "Flandres", "District des Flandres", 4, "59", 0.62f },
    { "Oise", "District de l'Oise", 4, "60", 1 }, { "Somme", "District de la Somme", 4, "80", 1 },
    // Grand Est
    { "Alsace", "District d'Alsace", 5, "67,68", 1 }, { "Ardennes", "District des Ardennes", 5, "08", 1 }, { "Aube", "District de l'Aube", 5, "10", 1 },
    { "Marne", "District de la Marne", 5, "51", 1 }, { "Haute-Marne", "District de la Haute-Marne", 5, "52", 1 },
    { "Meurthe-et-Moselle", "District de Meurthe-et-Moselle", 5, "54", 1 }, { "Meuse", "District de la Meuse", 5, "55", 1 },
    { "Moselle", "District mosellan", 5, "57", 1 }, { "Vosges", "District des Vosges", 5, "88", 1 },
    // Pays de la Loire
    { "Loire-Atlantique", "District de Loire-Atlantique", 6, "44", 1 }, { "Maine-et-Loire", "District de Maine-et-Loire", 6, "49", 1 },
    { "Mayenne", "District de la Mayenne", 6, "53", 1 }, { "Sarthe", "District de la Sarthe", 6, "72", 1 }, { "Vendée", "District de Vendée", 6, "85", 1 },
    // Bretagne
    { "Côtes-d'Armor", "District des Côtes-d'Armor", 7, "22", 1 }, { "Finistère", "District du Finistère", 7, "29", 1 },
    { "Ille-et-Vilaine", "District d'Ille-et-Vilaine", 7, "35", 1 }, { "Morbihan", "District du Morbihan", 7, "56", 1 },
    // Nouvelle-Aquitaine
    { "Charente", "District de la Charente", 8, "16", 1 }, { "Charente-Maritime", "District de Charente-Maritime", 8, "17", 1 },
    { "Corrèze", "District de la Corrèze", 8, "19", 1 }, { "Creuse", "District de la Creuse", 8, "23", 1 }, { "Deux-Sèvres", "District des Deux-Sèvres", 8, "79", 1 },
    { "Dordogne-Périgord", "District Dordogne-Périgord", 8, "24", 1 }, { "Gironde", "District de la Gironde", 8, "33", 1 }, { "Landes", "District des Landes", 8, "40", 1 },
    { "Lot-et-Garonne", "District de Lot-et-Garonne", 8, "47", 1 }, { "Pyrénées-Atlantiques", "District des Pyrénées-Atlantiques", 8, "64", 1 },
    { "Vienne", "District de la Vienne", 8, "86", 1 }, { "Haute-Vienne", "District de la Haute-Vienne", 8, "87", 1 },
    // Occitanie
    { "Ariège", "District de l'Ariège", 9, "09", 1 }, { "Aude", "District de l'Aude", 9, "11", 1 }, { "Aveyron", "District de l'Aveyron", 9, "12", 1 },
    { "Gard-Lozère", "District Gard-Lozère", 9, "30,48", 1 }, { "Haute-Garonne", "District de la Haute-Garonne", 9, "31", 1 }, { "Gers", "District du Gers", 9, "32", 1 },
    { "Hérault", "District de l'Hérault", 9, "34", 1 }, { "Lot", "District du Lot", 9, "46", 1 }, { "Pyrénées-Orientales", "District des Pyrénées-Orientales", 9, "66", 1 },
    { "Hautes-Pyrénées", "District des Hautes-Pyrénées", 9, "65", 1 }, { "Tarn", "District du Tarn", 9, "81", 1 }, { "Tarn-et-Garonne", "District de Tarn-et-Garonne", 9, "82", 1 },
    // Auvergne-Rhône-Alpes
    { "Ain", "District de l'Ain", 10, "01", 0.85f }, { "Allier", "District de l'Allier", 10, "03", 1 }, { "Cantal", "District du Cantal", 10, "15", 1 },
    { "Isère", "District de l'Isère", 10, "38", 1 }, { "Drôme-Ardèche", "District Drôme-Ardèche", 10, "26,07", 1 }, { "Loire", "District de la Loire", 10, "42", 1 },
    { "Haute-Loire", "District de la Haute-Loire", 10, "43", 1 }, { "Lyon et Rhône", "District de Lyon et du Rhône", 10, "69", 1 },
    { "Puy-de-Dôme", "District du Puy-de-Dôme", 10, "63", 1 }, { "Savoie", "District de Savoie", 10, "73", 1 },
    { "Haute-Savoie Pays de Gex", "District de Haute-Savoie Pays de Gex", 10, "74,01", 1 },
    // Méditerranée
    { "Alpes", "District des Alpes", 11, "04,05", 1 }, { "Côte d'Azur", "District de la Côte d'Azur", 11, "06", 1 },
    { "Provence", "District de Provence", 11, "13", 0.84f }, { "Grand Vaucluse", "District du Grand Vaucluse", 11, "84,13", 1 }, { "Var", "District du Var", 11, "83", 1 },
    // Corse : pas de district, divisions départementales organisées par la ligue
    { "Corse", "Ligue corse (divisions départementales)", 12, "2A,2B", 1 },
};
static const int NUM_DIST = sizeof(DISTRICTS_DEF) / sizeof(DISTRICTS_DEF[0]);

// communes rattachées au district « minoritaire » d'un département partagé
struct SplitRule { const char* dept; const char* district; const char* towns; };
static const SplitRule SPLITS[] = {
    { "59", "Escaut", "Valenciennes;Douai;Cambrai;Maubeuge;Denain;Anzin;Saint-Amand-les-Eaux;Condé-sur-l'Escaut;Onnaing;Raismes;Aulnoye-Aymeries;Avesnes-sur-Helpe;"
                      "Fourmies;Hautmont;Jeumont;Le Quesnoy;Caudry;Le Cateau-Cambrésis;Sin-le-Noble;Somain;Aniche;Waziers;Cuincy;Lallaing;Marly;Trith-Saint-Léger;"
                      "Bruay-sur-l'Escaut;Vieux-Condé;Quiévrechain;Feignies;Louvroil;Guesnain;Lambres-lez-Douai;Auby;Flers-en-Escrebieux;Dechy;Roost-Warendin;"
                      "Escaudain;Lourches;Douchy-les-Mines;Neuville-sur-Escaut;Saint-Saulve;Beuvrages;Famars;Aulnoy-lez-Valenciennes;Proville;Escaudœuvres;"
                      "Neuville-Saint-Rémy;Solesmes;Wignehies;Anor;Trélon;Landrecies;Bavay;Ferrière-la-Grande;Recquignies;Marpent;Pecquencourt;"
                      "Montigny-en-Ostrevent;Masny;Hornaing;Wallers;Hérin;Petite-Forêt;Crespin;Quarouble;Vicq;Fresnes-sur-Escaut;Haulchin;Thiant;Prouvy;"
                      "Rouvignies;Haveluy;Abscon;Erre;Fenain;Bouchain;Valenciennes;Arleux;Iwuy;Avesnes-les-Aubert;Marcoing;Gouzeaucourt;Villers-Outréaux" },
    { "62", "Côte d'Opale", "Calais;Boulogne-sur-Mer;Outreau;Berck;Saint-Omer;Saint-Martin-Boulogne;Étaples;Longuenesse;Marck;Le Portel;Arques;Coulogne;Wimereux;"
                            "Aire-sur-la-Lys;Blendecques;Montreuil;Le Touquet-Paris-Plage;Guînes;Oye-Plage;Desvres;Samer;Hesdin;Wizernes;Lumbres;Saint-Léonard;"
                            "Équihen-Plage;Condette;Neufchâtel-Hardelot;Camiers;Cucq;Rang-du-Fliers;Merlimont;Audruicq;Ardres;Marquise;Eperlecques;Fruges;"
                            "Sangatte;Coquelles;Saint-Martin-lez-Tatinghem;Helfaut;Isques;Wimille;Calais;Fréthun;Nortkerque;Tournehem-sur-la-Hem;Thérouanne" },
    { "13", "Grand Vaucluse", "Arles;Tarascon;Châteaurenard;Saint-Rémy-de-Provence;Saint-Martin-de-Crau;Noves;Eyragues;Rognonas;Barbentane;Graveson;Cabannes;"
                              "Saintes-Maries-de-la-Mer;Port-Saint-Louis-du-Rhône;Maillane;Fontvieille;Mouriès;Maussane-les-Alpilles;Plan-d'Orgon;Orgon;"
                              "Verquières;Mollégès;Saint-Andiol;Boulbon;Saint-Étienne-du-Grès;Paradou;Sénas;Mallemort" },
    { "01", "Haute-Savoie Pays de Gex", "Gex;Ferney-Voltaire;Divonne-les-Bains;Saint-Genis-Pouilly;Prévessin-Moëns;Thoiry;Cessy;Ornex;Ségny;Versonnex;Sergy;"
                                        "Crozet;Chevry;Échenevex;Grilly;Sauverny;Péron;Collonges;Farges;Challex;Pougny;Saint-Jean-de-Gonville;Vesancy;Mijoux" },
};

static std::string keyOfTown(const std::string& s) {
    std::string o;
    for (char c : s) { unsigned char u = (unsigned char)c; if (isalnum(u) || u >= 0x80) o += (char)tolower(u); }
    return o;
}

static int deptIdx(const char* code) {
    for (int i = 0; i < NUM_DEPTS; i++) if (!strcmp(DEPTS[i].code, code)) return i;
    return -1;
}

static std::vector<std::vector<int>>& distDepts() {
    static std::vector<std::vector<int>> v;
    if (v.empty()) {
        v.resize(NUM_DIST);
        for (int d = 0; d < NUM_DIST; d++) {
            std::string s = DISTRICTS_DEF[d].depts, cur;
            for (size_t i = 0; i <= s.size(); i++) {
                if (i == s.size() || s[i] == ',') { int k = deptIdx(cur.c_str()); if (k >= 0) v[d].push_back(k); cur.clear(); }
                else cur += s[i];
            }
        }
    }
    return v;
}

int numDistricts() { return NUM_DIST; }
std::string districtName(int d) { return d >= 0 && d < NUM_DIST ? sanitize(DISTRICTS_DEF[d].name) : std::string(); }
std::string districtFullName(int d) { return d >= 0 && d < NUM_DIST ? sanitize(DISTRICTS_DEF[d].full) : std::string(); }
int districtRegion(int d) { return d >= 0 && d < NUM_DIST ? DISTRICTS_DEF[d].region : -1; }
const std::vector<int>& districtDepts(int d) { static std::vector<int> none; return d >= 0 && d < NUM_DIST ? distDepts()[d] : none; }
int districtPopulation(int d) {
    if (d < 0 || d >= NUM_DIST) return 0;
    double p = 0;
    const std::string nm = DISTRICTS_DEF[d].name;
    for (int k : distDepts()[d]) {
        double share = 1;
        // département partagé : part du district principal, ou le reste pour le district « minoritaire »
        for (auto& S : SPLITS) if (!strcmp(S.dept, DEPTS[k].code)) {
            if (nm == S.district) share = !strcmp(S.dept, "59") ? 0.38 : !strcmp(S.dept, "62") ? 0.4 : !strcmp(S.dept, "13") ? 0.16 : 0.15;
            else share = !strcmp(S.dept, "59") ? 0.62 : !strcmp(S.dept, "62") ? 0.6 : !strcmp(S.dept, "13") ? 0.84 : 0.85;
        }
        p += DEPTS[k].population * share;
    }
    return (int)p;
}

int districtIndex(const std::string& name) {
    for (int d = 0; d < NUM_DIST; d++) if (districtName(d) == name || sanitize(DISTRICTS_DEF[d].name) == name) return d;
    return -1;
}

// district d'un club : département + commune (ou nom du club) pour les départements partagés
int districtFor(int dept, const std::string& town, const std::string& club) {
    if (dept < 0 || dept >= NUM_DEPTS) return -1;
    const char* code = DEPTS[dept].code;
    for (auto& S : SPLITS) {
        if (strcmp(S.dept, code)) continue;
        std::string tk = keyOfTown(town), ck = keyOfTown(club);
        std::string list = S.towns, cur;
        bool hit = false;
        for (size_t i = 0; i <= list.size() && !hit; i++) {
            if (i == list.size() || list[i] == ';') {
                std::string k = keyOfTown(sanitize(cur.c_str()));
                if (!k.empty() && (k == tk || (k.size() >= 5 && ck.find(k) != std::string::npos))) hit = true;
                cur.clear();
            } else cur += list[i];
        }
        if (hit) return districtIndex(sanitize(S.district));
    }
    // district principal du département (le premier qui le couvre, hors district « minoritaire »)
    for (int d = 0; d < NUM_DIST; d++) {
        const auto& v = distDepts()[d];
        if (std::find(v.begin(), v.end(), dept) == v.end()) continue;
        bool minority = false;
        for (auto& S : SPLITS) if (!strcmp(S.dept, code) && !strcmp(S.district, DISTRICTS_DEF[d].name)) minority = true;
        if (!minority) return d;
    }
    return -1;
}

void assignDistricts(World& w) {
    std::map<std::string,int> declaredDistricts;
    for(int i=0;i<NUM_FR_OFFICIAL_2627;i++)if(FR_OFFICIAL_2627[i].district[0])declaredDistricts[sanitize(FR_OFFICIAL_2627[i].name)]=districtIndex(sanitize(FR_OFFICIAL_2627[i].district));
    for (auto& t : w.teams) {
        if (t.kind != TK_CLUB || t.dept < 0) { if (t.kind != TK_CLUB) t.district = -1; continue; }
        // Keep an explicit, valid affiliation (e.g. Ornel's fourth team in Meuse).
        auto declared=declaredDistricts.find(t.name);
        if(declared!=declaredDistricts.end()) {t.district=declared->second;continue;}
        if (t.parent >= 0 && t.parent < (int)w.teams.size() && w.teams[t.parent].district >= 0) { t.district = w.teams[t.parent].district; continue; }
        if (t.district < 0 || t.district >= NUM_DIST || districtRegion(t.district) != t.region) t.district = districtFor(t.dept, t.town, t.name);
    }
}
