// Palmarès historiques (sources : Wikipédia) - année = fin de saison
#include "game.h"
#include <map>

struct HistRow { short year; const char* winner; const char* runner; };

static const HistRow L1[] = {
 {1933,"Olympique lillois",0},{1934,"FC Sète",0},{1935,"FC Sochaux",0},{1936,"RC Paris",0},{1937,"Olympique de Marseille",0},{1938,"FC Sochaux",0},{1939,"FC Sète",0},
 {1946,"Lille OSC",0},{1947,"CO Roubaix-Tourcoing",0},{1948,"Olympique de Marseille",0},{1949,"Stade de Reims",0},{1950,"Girondins de Bordeaux",0},{1951,"OGC Nice",0},{1952,"OGC Nice",0},
 {1953,"Stade de Reims",0},{1954,"Lille OSC",0},{1955,"Stade de Reims",0},{1956,"OGC Nice",0},{1957,"AS Saint-Étienne",0},{1958,"Stade de Reims",0},{1959,"OGC Nice",0},{1960,"Stade de Reims",0},
 {1961,"AS Monaco",0},{1962,"Stade de Reims",0},{1963,"AS Monaco",0},{1964,"AS Saint-Étienne",0},{1965,"FC Nantes",0},{1966,"FC Nantes",0},{1967,"AS Saint-Étienne",0},{1968,"AS Saint-Étienne",0},
 {1969,"AS Saint-Étienne",0},{1970,"AS Saint-Étienne",0},{1971,"Olympique de Marseille",0},{1972,"Olympique de Marseille",0},{1973,"FC Nantes",0},{1974,"AS Saint-Étienne",0},{1975,"AS Saint-Étienne",0},
 {1976,"AS Saint-Étienne",0},{1977,"FC Nantes",0},{1978,"AS Monaco",0},{1979,"RC Strasbourg",0},{1980,"FC Nantes",0},{1981,"AS Saint-Étienne",0},{1982,"AS Monaco",0},{1983,"FC Nantes",0},
 {1984,"Girondins de Bordeaux",0},{1985,"Girondins de Bordeaux",0},{1986,"Paris Saint-Germain",0},{1987,"Girondins de Bordeaux",0},{1988,"AS Monaco",0},{1989,"Olympique de Marseille",0},
 {1990,"Olympique de Marseille",0},{1991,"Olympique de Marseille",0},{1992,"Olympique de Marseille",0},{1993,"(titre non attribué)",0},{1994,"Paris Saint-Germain",0},{1995,"FC Nantes",0},{1996,"AJ Auxerre",0},
 {1997,"AS Monaco",0},{1998,"RC Lens",0},{1999,"Girondins de Bordeaux",0},{2000,"AS Monaco",0},{2001,"FC Nantes",0},{2002,"Olympique Lyonnais",0},{2003,"Olympique Lyonnais",0},{2004,"Olympique Lyonnais",0},
 {2005,"Olympique Lyonnais",0},{2006,"Olympique Lyonnais",0},{2007,"Olympique Lyonnais",0},{2008,"Olympique Lyonnais",0},{2009,"Girondins de Bordeaux",0},{2010,"Olympique de Marseille",0},{2011,"Lille OSC",0},
 {2012,"Montpellier HSC",0},{2013,"Paris Saint-Germain",0},{2014,"Paris Saint-Germain",0},{2015,"Paris Saint-Germain",0},{2016,"Paris Saint-Germain",0},{2017,"AS Monaco",0},{2018,"Paris Saint-Germain",0},
 {2019,"Paris Saint-Germain",0},{2020,"Paris Saint-Germain",0},{2021,"Lille OSC",0},{2022,"Paris Saint-Germain",0},{2023,"Paris Saint-Germain",0},{2024,"Paris Saint-Germain",0},{2025,"Paris Saint-Germain",0},{2026,"Paris Saint-Germain",0},
};
static const HistRow L2[] = {
 {1934,"Red Star",0},{1935,"FC Metz",0},{1936,"FC Rouen",0},{1937,"RC Lens",0},{1938,"Le Havre AC",0},{1939,"Red Star",0},{1946,"FC Nancy",0},{1947,"FC Sochaux",0},{1948,"OGC Nice",0},{1949,"RC Lens",0},
 {1950,"Le Havre AC",0},{1951,"Stade Français",0},{1952,"Toulouse FC",0},{1953,"Olympique Lyonnais",0},{1954,"Lille OSC",0},{1955,"CS Sedan",0},{1956,"Stade Rennais",0},{1957,"Olympique d'Alès",0},{1958,"FC Nancy",0},
 {1959,"Le Havre AC",0},{1960,"FC Grenoble",0},{1961,"Montpellier",0},{1962,"US Valenciennes",0},{1963,"AS Saint-Étienne",0},{1964,"Lille OSC",0},{1965,"OGC Nice",0},{1966,"Stade de Reims",0},{1967,"SEC Bastia",0},
 {1968,"SCO Angers",0},{1969,"SCO Angers",0},{1970,"AS Nancy-Lorraine",0},{1971,"US Valenciennes",0},{1972,"Troyes / Strasbourg",0},{1973,"Lens / Troyes",0},{1974,"Valenciennes / Red Star",0},{1975,"Laval / Nancy",0},
 {1976,"Caen / Angers",0},{1977,"Monaco / Strasbourg",0},{1978,"Alès / Dunkerque",0},{1979,"Gueugnon / Brest",0},{1980,"Reims / Montpellier",0},{1981,"Toulouse / Abbeville",0},{1982,"Marseille / Le Havre",0},
 {1983,"Valenciennes / Toulon",0},{1984,"Limoges / Tours",0},{1985,"Mulhouse / Nice",0},{1986,"Montpellier / RC Paris",0},{1987,"Mulhouse / Lyon",0},{1988,"Nancy / Rouen",0},{1989,"Brest / Orléans",0},
 {1990,"Strasbourg / Rouen",0},{1991,"Strasbourg / Angers",0},{1992,"Rouen / Strasbourg",0},{1993,"Cannes / Rouen",0},{1994,"OGC Nice",0},{1995,"Olympique de Marseille",0},{1996,"SM Caen",0},{1997,"FC Sochaux",0},
 {1998,"AS Nancy-Lorraine",0},{1999,"AS Saint-Étienne",0},{2000,"Lille OSC",0},{2001,"FC Sochaux",0},{2002,"Toulouse FC",0},{2003,"Toulouse FC",0},{2004,"AS Nancy-Lorraine",0},{2005,"FC Lorient",0},
 {2006,"Valenciennes FC",0},{2007,"FC Metz",0},{2008,"Le Havre AC",0},{2009,"RC Lens",0},{2010,"SM Caen",0},{2011,"Dijon FCO",0},{2012,"SC Bastia",0},{2013,"AS Monaco",0},{2014,"FC Metz",0},{2015,"ESTAC Troyes",0},
 {2016,"AS Nancy-Lorraine",0},{2017,"RC Strasbourg",0},{2018,"Stade de Reims",0},{2019,"FC Metz",0},{2020,"FC Lorient",0},{2021,"ESTAC Troyes",0},{2022,"Toulouse FC",0},{2023,"Le Havre AC",0},{2024,"AJ Auxerre",0},
 {2025,"FC Lorient",0},{2026,"ESTAC Troyes",0},
};
static const HistRow CDF[] = {
 {1918,"Olympique de Pantin","FC Lyon"},{1919,"CASG Paris","Olympique de Pantin"},{1920,"CA Paris","Le Havre AC"},{1921,"Red Star","Olympique de Pantin"},{1922,"Red Star","Stade Rennais"},{1923,"Red Star","FC Sète"},
 {1924,"Olympique de Marseille","FC Sète"},{1925,"CASG Paris","FC Rouen"},{1926,"Olympique de Marseille","AS Valentigney"},{1927,"Olympique de Marseille","US Quevilly"},{1928,"Red Star","CA Paris"},{1929,"Montpellier","FC Sète"},
 {1930,"FC Sète","RC Paris"},{1931,"Club Français","Montpellier"},{1932,"AS Cannes","RC Roubaix"},{1933,"Excelsior Roubaix","RC Roubaix"},{1934,"FC Sète","Olympique de Marseille"},{1935,"Olympique de Marseille","Stade Rennais"},
 {1936,"RC Paris","FCO Charleville"},{1937,"FC Sochaux","RC Strasbourg"},{1938,"Olympique de Marseille","FC Metz"},{1939,"RC Paris","Olympique lillois"},{1940,"RC Paris","Olympique de Marseille"},{1941,"Girondins de Bordeaux","SC Fives"},
 {1942,"Red Star","FC Sète"},{1943,"Olympique de Marseille","Girondins de Bordeaux"},{1944,"ÉF Nancy-Lorraine","ÉF Reims-Champagne"},{1945,"RC Paris","Lille OSC"},{1946,"Lille OSC","Red Star"},{1947,"Lille OSC","RC Strasbourg"},
 {1948,"Lille OSC","RC Lens"},{1949,"RC Paris","Lille OSC"},{1950,"Stade de Reims","RC Paris"},{1951,"RC Strasbourg","US Valenciennes"},{1952,"OGC Nice","Girondins de Bordeaux"},{1953,"Lille OSC","FC Nancy"},
 {1954,"OGC Nice","Olympique de Marseille"},{1955,"Lille OSC","Girondins de Bordeaux"},{1956,"CS Sedan","AS Troyes-Savinienne"},{1957,"Toulouse FC","SCO Angers"},{1958,"Stade de Reims","Nîmes Olympique"},{1959,"Le Havre AC","FC Sochaux"},
 {1960,"AS Monaco","AS Saint-Étienne"},{1961,"CS Sedan","Nîmes Olympique"},{1962,"AS Saint-Étienne","FC Nancy"},{1963,"AS Monaco","Olympique Lyonnais"},{1964,"Olympique Lyonnais","Girondins de Bordeaux"},{1965,"Stade Rennais","CS Sedan"},
 {1966,"RC Strasbourg","FC Nantes"},{1967,"Olympique Lyonnais","FC Sochaux"},{1968,"AS Saint-Étienne","Girondins de Bordeaux"},{1969,"Olympique de Marseille","Girondins de Bordeaux"},{1970,"AS Saint-Étienne","FC Nantes"},{1971,"Stade Rennais","Olympique Lyonnais"},
 {1972,"Olympique de Marseille","SEC Bastia"},{1973,"Olympique Lyonnais","FC Nantes"},{1974,"AS Saint-Étienne","AS Monaco"},{1975,"AS Saint-Étienne","RC Lens"},{1976,"Olympique de Marseille","Olympique Lyonnais"},{1977,"AS Saint-Étienne","Stade de Reims"},
 {1978,"AS Nancy-Lorraine","OGC Nice"},{1979,"FC Nantes","AJ Auxerre"},{1980,"AS Monaco","US Orléans"},{1981,"SEC Bastia","AS Saint-Étienne"},{1982,"Paris Saint-Germain","AS Saint-Étienne"},{1983,"Paris Saint-Germain","FC Nantes"},
 {1984,"FC Metz","AS Monaco"},{1985,"AS Monaco","Paris Saint-Germain"},{1986,"Girondins de Bordeaux","Olympique de Marseille"},{1987,"Girondins de Bordeaux","Olympique de Marseille"},{1988,"FC Metz","FC Sochaux"},{1989,"Olympique de Marseille","AS Monaco"},
 {1990,"Montpellier HSC","RC Paris"},{1991,"AS Monaco","Olympique de Marseille"},{1992,"(non attribuée)",0},{1993,"Paris Saint-Germain","FC Nantes"},{1994,"AJ Auxerre","Montpellier HSC"},{1995,"Paris Saint-Germain","RC Strasbourg"},
 {1996,"AJ Auxerre","Nîmes Olympique"},{1997,"OGC Nice","EA Guingamp"},{1998,"Paris Saint-Germain","RC Lens"},{1999,"FC Nantes","CS Sedan"},{2000,"FC Nantes","Calais RUFC"},{2001,"RC Strasbourg","Amiens SC"},
 {2002,"FC Lorient","SC Bastia"},{2003,"AJ Auxerre","Paris Saint-Germain"},{2004,"Paris Saint-Germain","LB Châteauroux"},{2005,"AJ Auxerre","CS Sedan"},{2006,"Paris Saint-Germain","Olympique de Marseille"},{2007,"FC Sochaux","Olympique de Marseille"},
 {2008,"Olympique Lyonnais","Paris Saint-Germain"},{2009,"EA Guingamp","Stade Rennais"},{2010,"Paris Saint-Germain","AS Monaco"},{2011,"Lille OSC","Paris Saint-Germain"},{2012,"Olympique Lyonnais","US Quevilly"},{2013,"Girondins de Bordeaux","Évian TG"},
 {2014,"EA Guingamp","Stade Rennais"},{2015,"Paris Saint-Germain","AJ Auxerre"},{2016,"Paris Saint-Germain","Olympique de Marseille"},{2017,"Paris Saint-Germain","SCO Angers"},{2018,"Paris Saint-Germain","Les Herbiers VF"},
 {2019,"Stade Rennais","Paris Saint-Germain"},{2020,"Paris Saint-Germain","AS Saint-Étienne"},{2021,"Paris Saint-Germain","AS Monaco"},{2022,"FC Nantes","OGC Nice"},{2023,"Toulouse FC","FC Nantes"},{2024,"Paris Saint-Germain","Olympique Lyonnais"},
 {2025,"Paris Saint-Germain","Stade de Reims"},{2026,"RC Lens","OGC Nice"},
};
static const HistRow CDL[] = {
 {1995,"Paris Saint-Germain","SC Bastia"},{1996,"FC Metz","Olympique Lyonnais"},{1997,"RC Strasbourg","Girondins de Bordeaux"},{1998,"Paris Saint-Germain","Girondins de Bordeaux"},{1999,"RC Lens","FC Metz"},
 {2000,"FC Gueugnon","Paris Saint-Germain"},{2001,"Olympique Lyonnais","AS Monaco"},{2002,"Girondins de Bordeaux","FC Lorient"},{2003,"AS Monaco","FC Sochaux"},{2004,"FC Sochaux","FC Nantes"},{2005,"RC Strasbourg","SM Caen"},
 {2006,"AS Nancy-Lorraine","OGC Nice"},{2007,"Girondins de Bordeaux","Olympique Lyonnais"},{2008,"Paris Saint-Germain","RC Lens"},{2009,"Girondins de Bordeaux","Vannes OC"},{2010,"Olympique de Marseille","Girondins de Bordeaux"},
 {2011,"Olympique de Marseille","Montpellier HSC"},{2012,"Olympique Lyonnais","Olympique de Marseille"},{2013,"AS Saint-Étienne","Stade Rennais"},{2014,"Paris Saint-Germain","Olympique Lyonnais"},{2015,"Paris Saint-Germain","SC Bastia"},
 {2016,"Paris Saint-Germain","Lille OSC"},{2017,"Paris Saint-Germain","AS Monaco"},{2018,"Paris Saint-Germain","AS Monaco"},{2019,"RC Strasbourg","EA Guingamp"},{2020,"Paris Saint-Germain","Olympique Lyonnais"},
};
static const HistRow TDC[] = {
 {1955,"Stade de Reims","Lille OSC"},{1956,"CS Sedan","OGC Nice"},{1957,"AS Saint-Étienne","Toulouse FC"},{1958,"Stade de Reims","Nîmes Olympique"},{1959,"Le Havre AC","OGC Nice"},{1960,"Stade de Reims","AS Monaco"},
 {1961,"AS Monaco","CS Sedan"},{1962,"AS Saint-Étienne","Stade de Reims"},{1965,"FC Nantes","Stade Rennais"},{1966,"Stade de Reims","FC Nantes"},{1967,"AS Saint-Étienne","Olympique Lyonnais"},{1968,"AS Saint-Étienne","Girondins de Bordeaux"},
 {1969,"AS Saint-Étienne","Olympique de Marseille"},{1970,"OGC Nice","AS Saint-Étienne"},{1971,"Stade Rennais / Marseille",0},{1972,"SEC Bastia","Olympique de Marseille"},{1973,"Olympique Lyonnais","FC Nantes"},
 {1985,"AS Monaco","Girondins de Bordeaux"},{1986,"Girondins de Bordeaux","Paris Saint-Germain"},{1995,"Paris Saint-Germain","FC Nantes"},{1997,"AS Monaco","OGC Nice"},{1998,"Paris Saint-Germain","RC Lens"},
 {1999,"FC Nantes","Girondins de Bordeaux"},{2000,"AS Monaco","FC Nantes"},{2001,"FC Nantes","RC Strasbourg"},{2002,"Olympique Lyonnais","FC Lorient"},{2003,"Olympique Lyonnais","AJ Auxerre"},{2004,"Olympique Lyonnais","Paris Saint-Germain"},
 {2005,"Olympique Lyonnais","AJ Auxerre"},{2006,"Olympique Lyonnais","Paris Saint-Germain"},{2007,"Olympique Lyonnais","FC Sochaux"},{2008,"Girondins de Bordeaux","Olympique Lyonnais"},{2009,"Girondins de Bordeaux","EA Guingamp"},
 {2010,"Olympique de Marseille","Paris Saint-Germain"},{2011,"Olympique de Marseille","Lille OSC"},{2012,"Olympique Lyonnais","Montpellier HSC"},{2013,"Paris Saint-Germain","Girondins de Bordeaux"},{2014,"Paris Saint-Germain","EA Guingamp"},
 {2015,"Paris Saint-Germain","Olympique Lyonnais"},{2016,"Paris Saint-Germain","Olympique Lyonnais"},{2017,"Paris Saint-Germain","AS Monaco"},{2018,"Paris Saint-Germain","AS Monaco"},{2019,"Paris Saint-Germain","Stade Rennais"},
 {2020,"Paris Saint-Germain","Olympique de Marseille"},{2021,"Lille OSC","Paris Saint-Germain"},{2022,"Paris Saint-Germain","FC Nantes"},{2023,"Paris Saint-Germain","Toulouse FC"},{2024,"Paris Saint-Germain","AS Monaco"},
 {2025,"Paris Saint-Germain","Olympique de Marseille"},{2026,"RC Lens","Paris Saint-Germain"},
};
static const HistRow C1H[] = {
 {1956,"Real Madrid",0},{1957,"Real Madrid",0},{1958,"Real Madrid",0},{1959,"Real Madrid",0},{1960,"Real Madrid",0},{1961,"Benfica",0},{1962,"Benfica",0},{1963,"AC Milan",0},{1964,"Inter Milan",0},{1965,"Inter Milan",0},
 {1966,"Real Madrid",0},{1967,"Celtic",0},{1968,"Manchester United",0},{1969,"AC Milan",0},{1970,"Feyenoord",0},{1971,"Ajax Amsterdam",0},{1972,"Ajax Amsterdam",0},{1973,"Ajax Amsterdam",0},{1974,"Bayern Munich",0},
 {1975,"Bayern Munich",0},{1976,"Bayern Munich","AS Saint-Étienne"},{1977,"Liverpool",0},{1978,"Liverpool",0},{1979,"Nottingham Forest",0},{1980,"Nottingham Forest",0},{1981,"Liverpool",0},{1982,"Aston Villa",0},
 {1983,"Hambourg SV",0},{1984,"Liverpool",0},{1985,"Juventus",0},{1986,"Steaua Bucarest",0},{1987,"FC Porto",0},{1988,"PSV Eindhoven",0},{1989,"AC Milan",0},{1990,"AC Milan",0},{1991,"Étoile rouge de Belgrade","Olympique de Marseille"},
 {1992,"FC Barcelone",0},{1993,"Olympique de Marseille","AC Milan"},{1994,"AC Milan",0},{1995,"Ajax Amsterdam",0},{1996,"Juventus",0},{1997,"Borussia Dortmund",0},{1998,"Real Madrid",0},{1999,"Manchester United",0},
 {2000,"Real Madrid",0},{2001,"Bayern Munich",0},{2002,"Real Madrid",0},{2003,"AC Milan",0},{2004,"FC Porto","AS Monaco"},{2005,"Liverpool",0},{2006,"FC Barcelone",0},{2007,"AC Milan",0},{2008,"Manchester United",0},
 {2009,"FC Barcelone",0},{2010,"Inter Milan",0},{2011,"FC Barcelone",0},{2012,"Chelsea",0},{2013,"Bayern Munich",0},{2014,"Real Madrid",0},{2015,"FC Barcelone",0},{2016,"Real Madrid",0},{2017,"Real Madrid",0},
 {2018,"Real Madrid",0},{2019,"Liverpool",0},{2020,"Bayern Munich","Paris Saint-Germain"},{2021,"Chelsea",0},{2022,"Real Madrid",0},{2023,"Manchester City",0},{2024,"Real Madrid",0},{2025,"Paris Saint-Germain","Inter Milan"},
 {2026,"Paris Saint-Germain","Arsenal"},
};
static const HistRow C3H[] = {
 {1972,"Tottenham Hotspur",0},{1973,"Liverpool",0},{1974,"Feyenoord",0},{1975,"Borussia Mönchengladbach",0},{1976,"Liverpool",0},{1977,"Juventus",0},{1978,"PSV Eindhoven","SEC Bastia"},{1979,"Borussia Mönchengladbach",0},
 {1980,"Eintracht Francfort",0},{1981,"Ipswich Town",0},{1982,"IFK Göteborg",0},{1983,"Anderlecht",0},{1984,"Tottenham Hotspur",0},{1985,"Real Madrid",0},{1986,"Real Madrid",0},{1987,"IFK Göteborg",0},
 {1988,"Bayer Leverkusen",0},{1989,"Naples",0},{1990,"Juventus",0},{1991,"Inter Milan",0},{1992,"Ajax Amsterdam",0},{1993,"Juventus",0},{1994,"Inter Milan",0},{1995,"Parme",0},{1996,"Bayern Munich","Girondins de Bordeaux"},
 {1997,"Schalke 04",0},{1998,"Inter Milan",0},{1999,"Parme","Olympique de Marseille"},{2000,"Galatasaray",0},{2001,"Liverpool",0},{2002,"Feyenoord",0},{2003,"FC Porto",0},{2004,"Valence","Olympique de Marseille"},
 {2005,"CSKA Moscou",0},{2006,"FC Séville",0},{2007,"FC Séville",0},{2008,"Zénith Saint-Pétersbourg",0},{2009,"Chakhtar Donetsk",0},{2010,"Atlético Madrid",0},{2011,"FC Porto",0},{2012,"Atlético Madrid",0},
 {2013,"Chelsea",0},{2014,"FC Séville",0},{2015,"FC Séville",0},{2016,"FC Séville",0},{2017,"Manchester United",0},{2018,"Atlético Madrid","Olympique de Marseille"},{2019,"Chelsea",0},{2020,"FC Séville",0},
 {2021,"Villarreal",0},{2022,"Eintracht Francfort",0},{2023,"FC Séville",0},{2024,"Atalanta",0},{2025,"Tottenham Hotspur",0},{2026,"Aston Villa",0},
};

struct HistList { const HistRow* rows; int n; };
static HistList histList(int comp) {
    switch (comp) {
    case HC_L1: return { L1, (int)(sizeof(L1) / sizeof(L1[0])) };
    case HC_L2: return { L2, (int)(sizeof(L2) / sizeof(L2[0])) };
    case HC_CDF: return { CDF, (int)(sizeof(CDF) / sizeof(CDF[0])) };
    case HC_CDL: return { CDL, (int)(sizeof(CDL) / sizeof(CDL[0])) };
    case HC_TDC: return { TDC, (int)(sizeof(TDC) / sizeof(TDC[0])) };
    case HC_UCL: return { C1H, (int)(sizeof(C1H) / sizeof(C1H[0])) };
    case HC_UEFA: return { C3H, (int)(sizeof(C3H) / sizeof(C3H[0])) };
    default: return { nullptr, 0 };
    }
}
int histCount(int comp) { return histList(comp).n; }
void histEntry(int comp, int i, int& year, const char*& winner, const char*& runner) {
    HistList h = histList(comp);
    year = h.rows[i].year; winner = h.rows[i].winner; runner = h.rows[i].runner;
}
const char* honourCompName(int comp) {
    static const char* N[NUM_HC] = { "Ligue 1", "Ligue 2", "Coupe de France", "Coupe de la Ligue", "Trophée des Champions", "Ligue des champions", "Coupe UEFA", "Supercoupe de l'UEFA", "Coupe Intertoto" };
    return comp >= 0 && comp < NUM_HC ? N[comp] : "";
}

// ------------------------------------------------------------------ correspondance palmarès historique -> clubs du jeu
static std::string keyOf(const std::string& w) {
    static const char* K[][2] = {
        { "Paris Saint-Germain", "Paris Saint-Germain" }, { "Lille OSC", "Lille" }, { "Olympique lillois", "Lille" }, { "Olympique Lyonnais", "Lyonnais" },
        { "Stade Rennais", "Rennais" }, { "FC Nancy", "Nancy" }, { "AS Nancy-Lorraine", "Nancy" }, { "SM Caen", "Caen" }, { "SEC Bastia", "Bastia" },
        { "SC Bastia", "Bastia" }, { "ESTAC Troyes", "Troyes" }, { "SCO Angers", "Angers" }, { "US Valenciennes", "Valenciennes" }, { "Valenciennes FC", "Valenciennes" },
        { "Montpellier HSC", "Montpellier" }, { "RC Strasbourg", "Strasbourg" }, { "FC Sochaux", "Sochaux" }, { "Le Havre AC", "Le Havre" },
        { "Dijon FCO", "Dijon" }, { "Red Star", "Red Star" }, { "AS Cannes", "Cannes" }, { "EA Guingamp", "Guingamp" }, { "FC Rouen", "Rouen" },
    };
    for (auto& k : K) if (w == k[0]) return k[1];
    return "";
}
static int resolveWinner(const std::string& w) {
    static std::map<std::string, int> cache;
    auto it = cache.find(w);
    if (it != cache.end()) return it->second;
    int best = -1;
    for (int i = 0; i < (int)g_world.teams.size(); i++) { const Team& t = g_world.teams[i]; if (t.kind == TK_CLUB && t.parent < 0 && t.name == w) { best = i; break; } }
    if (best < 0) {
        std::string k = keyOf(w);
        if (k.empty()) { size_t sp = w.rfind(' '); k = sp == std::string::npos ? w : w.substr(sp + 1); if (k.size() < 4) k = w; }
        float br = -1;
        for (int i = 0; i < (int)g_world.teams.size(); i++) {
            const Team& t = g_world.teams[i];
            if (t.kind != TK_CLUB || t.parent >= 0 || t.custom) continue;
            if (t.name.find(k) != std::string::npos && t.rating > br) { br = t.rating; best = i; }
        }
        if (br < 55) best = -1;     // clubs disparus ou amateurs : pas d'attribution
    }
    cache[w] = best;
    return best;
}
std::vector<std::string> historicHonours(int team) {
    std::vector<std::string> out;
    for (int c = 0; c < NUM_HC; c++) {
        HistList h = histList(c);
        for (int i = 0; i < h.n; i++) {
            if (!h.rows[i].winner || resolveWinner(h.rows[i].winner) != team) continue;
            bool league = c == HC_L1 || c == HC_L2;
            bool single = c == HC_TDC;
            std::string yr = single ? fmt("%d", h.rows[i].year) : fmt("%d-%02d", h.rows[i].year - 1, h.rows[i].year % 100);
            out.push_back(std::string(league ? "Champion : " : "Vainqueur : ") + honourCompName(c) + " (" + yr + ")");
        }
    }
    return out;
}
int resolveHistWinner(const char* w) { return w ? resolveWinner(w) : -1; }

// ------------------------------------------------------------------ lieux des finales
const char* histVenue(int comp, int year) {
    static const struct { short y; const char* v; } C1[] = {
        {1956,"Parc des Princes (Paris)"},{1957,"Santiago-Bernabéu (Madrid)"},{1958,"Heysel (Bruxelles)"},{1959,"Neckarstadion (Stuttgart)"},{1960,"Hampden Park (Glasgow)"},
        {1961,"Wankdorf (Berne)"},{1962,"Stade olympique (Amsterdam)"},{1963,"Wembley (Londres)"},{1964,"Prater (Vienne)"},{1965,"San Siro (Milan)"},{1966,"Heysel (Bruxelles)"},
        {1967,"Estádio Nacional (Lisbonne)"},{1968,"Wembley (Londres)"},{1969,"Santiago-Bernabéu (Madrid)"},{1970,"San Siro (Milan)"},{1971,"Wembley (Londres)"},{1972,"De Kuip (Rotterdam)"},
        {1973,"Marakana (Belgrade)"},{1974,"Heysel (Bruxelles)"},{1975,"Parc des Princes (Paris)"},{1976,"Hampden Park (Glasgow)"},{1977,"Stadio Olimpico (Rome)"},{1978,"Wembley (Londres)"},
        {1979,"Olympiastadion (Munich)"},{1980,"Santiago-Bernabéu (Madrid)"},{1981,"Parc des Princes (Paris)"},{1982,"De Kuip (Rotterdam)"},{1983,"Stade olympique (Athènes)"},
        {1984,"Stadio Olimpico (Rome)"},{1985,"Heysel (Bruxelles)"},{1986,"Sánchez-Pizjuán (Séville)"},{1987,"Prater (Vienne)"},{1988,"Neckarstadion (Stuttgart)"},{1989,"Camp Nou (Barcelone)"},
        {1990,"Prater (Vienne)"},{1991,"San Nicola (Bari)"},{1992,"Wembley (Londres)"},{1993,"Olympiastadion (Munich)"},{1994,"Stade olympique (Athènes)"},{1995,"Ernst-Happel (Vienne)"},
        {1996,"Stadio Olimpico (Rome)"},{1997,"Olympiastadion (Munich)"},{1998,"Amsterdam ArenA (Amsterdam)"},{1999,"Camp Nou (Barcelone)"},{2000,"Stade de France (Saint-Denis)"},
        {2001,"San Siro (Milan)"},{2002,"Hampden Park (Glasgow)"},{2003,"Old Trafford (Manchester)"},{2004,"Arena AufSchalke (Gelsenkirchen)"},{2005,"Stade olympique Atatürk (Istanbul)"},
        {2006,"Stade de France (Saint-Denis)"},{2007,"Stade olympique (Athènes)"},{2008,"Loujniki (Moscou)"},{2009,"Stadio Olimpico (Rome)"},{2010,"Santiago-Bernabéu (Madrid)"},
        {2011,"Wembley (Londres)"},{2012,"Allianz Arena (Munich)"},{2013,"Wembley (Londres)"},{2014,"Estádio da Luz (Lisbonne)"},{2015,"Olympiastadion (Berlin)"},{2016,"San Siro (Milan)"},
        {2017,"Millennium Stadium (Cardiff)"},{2018,"Stade olympique (Kiev)"},{2019,"Metropolitano (Madrid)"},{2020,"Estádio da Luz (Lisbonne)"},{2021,"Estádio do Dragão (Porto)"},
        {2022,"Stade de France (Saint-Denis)"},{2023,"Stade olympique Atatürk (Istanbul)"},{2024,"Wembley (Londres)"},{2025,"Allianz Arena (Munich)"},{2026,"Puskás Aréna (Budapest)"} };
    static const struct { short y; const char* v; } C3[] = {
        {1998,"Parc des Princes (Paris)"},{1999,"Loujniki (Moscou)"},{2000,"Parken (Copenhague)"},{2001,"Westfalenstadion (Dortmund)"},{2002,"De Kuip (Rotterdam)"},{2003,"Estadio Olímpico (Séville)"},
        {2004,"Ullevi (Göteborg)"},{2005,"José-Alvalade (Lisbonne)"},{2006,"Philips Stadion (Eindhoven)"},{2007,"Hampden Park (Glasgow)"},{2008,"City of Manchester Stadium (Manchester)"},
        {2009,"Şükrü-Saracoğlu (Istanbul)"},{2010,"Volksparkstadion (Hambourg)"},{2011,"Aviva Stadium (Dublin)"},{2012,"Arena Națională (Bucarest)"},{2013,"Amsterdam ArenA (Amsterdam)"},
        {2014,"Juventus Stadium (Turin)"},{2015,"Stade national (Varsovie)"},{2016,"Parc Saint-Jacques (Bâle)"},{2017,"Friends Arena (Stockholm)"},{2018,"Parc Olympique lyonnais (Lyon)"},
        {2019,"Stade olympique (Bakou)"},{2020,"RheinEnergieStadion (Cologne)"},{2021,"Stadion Energa (Gdańsk)"},{2022,"Sánchez-Pizjuán (Séville)"},{2023,"Puskás Aréna (Budapest)"},
        {2024,"Aviva Stadium (Dublin)"},{2025,"San Mamés (Bilbao)"},{2026,"Tüpraş Stadyumu (Istanbul)"} };
    static const struct { short y; const char* v; } TDC[] = {
        {1955,"Marseille"},{1956,"Paris"},{1957,"Toulouse"},{1958,"Marseille"},{1959,"Paris"},{1960,"Nantes"},{1961,"Marseille"},{1962,"Limoges"},{1965,"Lorient"},{1966,"Nantes"},
        {1967,"Saint-Étienne"},{1968,"Montpellier"},{1969,"Paris"},{1970,"Nice"},{1971,"Brest"},{1972,"Toulon"},{1973,"Brest"},{1985,"Bordeaux"},{1986,"Les Abymes"},{1995,"Brest"},
        {1997,"Béziers"},{1998,"Tours"},{1999,"Amiens"},{2000,"Montbéliard"},{2001,"Strasbourg"},{2002,"Cannes"},{2003,"Lyon"},{2004,"Cannes"},{2005,"Auxerre"},{2006,"Lyon"},
        {2008,"Bordeaux"},{2009,"Montréal"},{2010,"Tunis"},{2011,"Tanger"},{2012,"Harrison"},{2013,"Libreville"},{2014,"Pékin"},{2015,"Montréal"},{2016,"Klagenfurt"},{2017,"Tanger"},
        {2018,"Shenzhen"},{2020,"Lens"},{2021,"Tel-Aviv"},{2023,"Paris"},{2024,"Doha"},{2025,"Koweït"},{2026,"Lens"} };
    switch (comp) {
    case HC_UCL: for (auto& e : C1) if (e.y == year) return e.v; return "";
    case HC_UEFA: if (year <= 1997) return "aller-retour"; for (auto& e : C3) if (e.y == year) return e.v; return "";
    case HC_TDC: for (auto& e : TDC) if (e.y == year) return e.v; return "";
    case HC_CDF: return year >= 1998 ? "Stade de France (Saint-Denis)" : year >= 1972 ? "Parc des Princes (Paris)" : year >= 1937 ? "Stade de Colombes" : "";
    case HC_CDL: return year >= 1998 ? "Stade de France (Saint-Denis)" : "Parc des Princes (Paris)";
    default: return "";
    }
}
