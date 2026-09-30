// Statut des clubs (PRO / SEMI-PRO / AMATEUR), contrats des joueurs, vie du stade (affluence, billetterie, boutique, travaux)
#include "game.h"
#include <cstring>

const char* statusName(int s) { return s == CS_PRO ? "PRO" : s == CS_SEMIPRO ? "SEMI-PRO" : "AMATEUR"; }

int playerWage(const Player& p) {
    if (p.contract == 2) return 0;
    if (p.wageK > 0) return p.wageK;                          // salaire négocié
    int w = p.wage();
    return p.contract == 1 ? std::max(1, w * 35 / 100) : w;
}

// contrats selon le statut (clubs de l'IA, ou changement de statut)
void applyContracts(Team& t, bool keepUser) {
    if (!t.squadGen) return;
    if (t.status == CS_PRO) { for (auto& p : t.squad) p.contract = 0; return; }
    if (t.status == CS_AMATEUR) { for (auto& p : t.squad) p.contract = 2; return; }
    if (keepUser) { for (auto& p : t.squad) if (p.contract == 0) p.contract = 1; return; }
    std::vector<int> idx(t.squad.size());
    for (int i = 0; i < (int)idx.size(); i++) idx[i] = i;
    std::sort(idx.begin(), idx.end(), [&](int a, int b) { return t.squad[a].overall() > t.squad[b].overall(); });
    for (int k = 0; k < (int)idx.size(); k++) t.squad[idx[k]].contract = k < 14 ? 1 : 2;
}

static int frTier(const Career& K, int team, int* dom = nullptr) {
    int p, q, g;
    int t = K.tierOfTeam(team, &p, &q, &g);
    if (t < 0) return -1;
    if (dom) *dom = K.pyramids[p].dom >= 0 ? 1 : K.pyramids[p].country == "FRA" ? 0 : 2;
    return t;
}

int Career::forcedStatus(int team) const {
    int dom = 0;
    int t = frTier(*this, team, &dom);
    if (t < 0) return CS_AMATEUR;
    if (dom == 2) return t <= 3 ? CS_PRO : CS_SEMIPRO;       // championnats étrangers
    if (dom == 1) return t == 0 ? -1 : CS_AMATEUR;           // outre-mer : R1 au choix
    if (t <= 2) return CS_PRO;                               // Ligue 1, 2, 3
    if (t <= 4) return CS_SEMIPRO;                           // National 1 et 2
    if (t <= 7) return -1;                                   // Régional : semi-pro ou amateur
    return CS_AMATEUR;                                       // Départemental
}

// capacités réelles (approximatives) des principaux stades
static const struct { const char* key; int cap; } REAL_STADIUMS[] = {
    { "Parc des Princes", 47929 }, { "Vélodrome", 67394 }, { "Groupama Stadium", 59186 }, { "Pierre-Mauroy", 50186 }, { "Allianz Riviera", 36178 },
    { "Roazhon Park", 29778 }, { "Bollaert-Delelis", 38223 }, { "Louis-II", 16360 }, { "Meinau", 29230 }, { "Raymond-Kopa", 19350 },
    { "Stade de l'Aube", 21684 }, { "Francis-Le Blé", 15220 }, { "Moustoir", 18110 }, { "Stadium de Toulouse", 33150 }, { "Océane", 25178 },
    { "Charléty", 19151 }, { "Marie-Marvingt", 25064 }, { "Abbé-Deschamps", 18541 }, { "Geoffroy-Guichard", 41965 }, { "Stade Bauer", 10000 },
    { "Auguste-Delaune", 21029 }, { "Mosson", 32900 }, { "Saint-Symphorien", 28786 }, { "Marcel-Picot", 20087 }, { "Parc des Sports d'Annecy", 15660 },
    { "Auguste-Bonal", 20005 }, { "Gaston-Gérard", 15995 }, { "Nouste Camp", 4000 }, { "Roudourou", 18378 }, { "Marcel-Tribut", 4200 },
    { "Stade des Alpes", 20068 }, { "Paul-Lignon", 5955 }, { "Beaujoire", 35322 }, { "Gabriel-Montpied", 11980 }, { "Stade de la Libération", 9500 },
    { "Francis-Le Basser", 18739 }, { "Michel-d'Ornano", 20453 }, { "Guy-Piriou", 6500 }, { "Stade du Hainaut", 25172 }, { "Armand-Chouffet", 3500 },
    { "Robert-Diochon", 8200 }, { "Marcel-Verchère", 11400 }, { "Licorne", 12097 }, { "Armand-Cesari", 16078 }, { "Henri-Desgrange", 5000 },
    { "Pierre-de-Coubertin", 6000 }, { "Charles-Massot", 5000 }, { "Stade de la Source", 7000 }, { "Stade Jean-Bouin", 20000 },
    { "Santiago Bernabéu", 83186 }, { "Spotify Camp Nou", 99354 }, { "Metropolitano", 70460 }, { "Old Trafford", 74310 }, { "Anfield", 61276 },
    { "Etihad", 53400 }, { "Emirates", 60704 }, { "Stamford Bridge", 40343 }, { "Tottenham Hotspur Stadium", 62850 }, { "St James", 52305 },
    { "Villa Park", 42640 }, { "Allianz Arena", 75024 }, { "Signal Iduna", 81365 }, { "BayArena", 30210 }, { "San Siro", 75817 }, { "Giuseppe Meazza", 75817 },
    { "Allianz Stadium", 41507 }, { "Olimpico", 70634 }, { "Diego Armando Maradona", 54726 }, { "Estádio da Luz", 64642 }, { "Dragão", 50033 },
    { "Johan Cruijff", 55865 }, { "Philips Stadion", 35000 }, { "Celtic Park", 60411 }, { "Ibrox", 50817 },
};

// capacité selon le niveau (plage par division) et le rang du club dans sa division, sauf stade réel connu
void initStadium(Team& t, int tier, float tierAvg) {
    StadiumInfo& S = t.sta;
    S = StadiumInfo();
    S.init = 1;
    int cap = -1;
    for (auto& r : REAL_STADIUMS) if (!t.stadium.empty() && t.stadium.find(r.key) != std::string::npos && t.parent < 0) { cap = r.cap; break; }
    bool foreign = t.nation >= 0 && strcmp(NATIONS[t.nation].code, "FRA") != 0;
    if (cap < 0) {
        static const int LO[13] = { 16000, 8000, 4000, 2000, 1000, 500, 300, 250, 200, 150, 150, 150, 150 };
        static const int HI[13] = { 48000, 30000, 20000, 8000, 5000, 2500, 1500, 1000, 800, 500, 450, 400, 350 };
        static const int FLO[4] = { 20000, 10000, 5000, 3000 }, FHI[4] = { 75000, 35000, 20000, 12000 };
        int tr = tier < 0 ? (t.rating >= 70 ? 0 : t.rating >= 60 ? 1 : t.rating >= 50 ? 3 : t.rating >= 40 ? 5 : 8) : std::min(12, tier);
        float lo = foreign ? FLO[std::min(3, tr)] : LO[tr], hi = foreign ? FHI[std::min(3, tr)] : HI[tr];
        float avg = tierAvg > 0 ? tierAvg : t.rating;
        float x = std::max(0.f, std::min(1.f, 0.5f + (t.rating - avg) / 12.f));
        cap = (int)(lo * std::pow(hi / lo, x));
        if (t.parent >= 0) cap = std::max(150, cap / 3);   // réserves : terrain annexe
    }
    cap = std::max(150, std::min(100000, cap));
    bool pro = cap >= 8000;
    int ref = cap >= 20000 ? 25 : cap >= 8000 ? 15 : cap >= 3000 ? 8 : cap >= 1000 ? 5 : 3;
    int share[4] = { 35, 29, 18, 18 };
    if (cap < 1000) { share[0] = 55; share[1] = 25; share[2] = 10; share[3] = 10; }   // petit stade : une tribune et des mains courantes
    for (int i = 0; i < 4; i++) {
        Stand& st = S.s[i];
        st.seats = cap * share[i] / 100;
        st.kind = pro ? STK_COVERED : i == 0 ? (cap >= 400 ? STK_COVERED : STK_SEATS) : cap >= 2000 ? STK_SEATS : STK_STANDING;
        st.price = (int16_t)(i == 0 ? ref + ref / 2 : ref);
        st.vip = (i == 0 && cap >= 5000) ? cap / 60 : 0;
        st.vipPrice = (int16_t)(ref * 5);
    }
    S.fans = (int)(cap * 0.75f);
    S.buvette = cap >= 1000 ? 2 : 1;
    S.boutique = cap >= 15000 ? 3 : cap >= 5000 ? 2 : cap >= 2000 ? 1 : 0;
    S.parking = cap >= 10000 ? 2 : cap >= 3000 ? 1 : 0;
    S.lights = cap >= 5000 ? 3 : cap >= 1500 ? 2 : 1;
    S.pitch = 0;
    S.screen = cap >= 20000 ? 1 : 0;
    S.vestiaires = cap >= 8000 ? 3 : cap >= 2000 ? 2 : 1;
    S.shirtPrice = (int16_t)(pro ? 85 : cap >= 2000 ? 45 : 30);
    // pelouse : les grands stades ont une pelouse mieux entretenue ; loyer à la mairie selon la taille du stade
    S.turf = (uint8_t)(cap >= 20000 ? 92 : cap >= 8000 ? 86 : cap >= 2000 ? 76 : 66);
    S.owner = 0;
    S.rentK = std::max(1, (int)(cap * (pro ? 0.012f : 0.004f)));
    S.annexReserve = cap >= 8000 ? 1 : 0;
    S.annexYouth = cap >= 20000 ? 1 : 0;
    S.annexTraining = cap >= 20000 ? 2 : cap >= 8000 ? 1 : 0;
}

void stadiumSetCapacity(StadiumInfo& S, int cap) {
    int share[4] = { 35, 29, 18, 18 };
    if (cap < 1000) { share[0] = 55; share[1] = 25; share[2] = 10; share[3] = 10; }
    for (int i = 0; i < 4; i++) S.s[i].seats = cap * share[i] / 100;
    S.fans = std::max(S.fans, cap * 3 / 4);
}

void ensureStadium(int team) {
    Team& t = g_world.teams[team];
    if (!t.sta.init) initStadium(t, teamLevel(team) < 99 ? teamLevel(team) : -1, -1);
}

void Career::updateStatuses() {
    for (auto& P : pyramids) {
        // note moyenne par niveau (choix de l'IA en régional)
        std::vector<float> avg(P.tiers.size(), 0); std::vector<int> cnt(P.tiers.size(), 0);
        for (auto& pl : P.pools) for (int t : pl.clubs) { avg[pl.tier] += g_world.teams[t].rating; cnt[pl.tier]++; }
        for (size_t i = 0; i < avg.size(); i++) if (cnt[i]) avg[i] /= cnt[i];
        for (auto& pl : P.pools) for (int t : pl.clubs) {
            Team& T = g_world.teams[t];
            if (!T.sta.init) initStadium(T, pl.tier, avg[pl.tier]);
            int f = forcedStatus(t);
            int old = T.status;
            if (t == userTeam && kind == CK_CLUB) {
                if (f >= 0) { T.status = f; mgr.needStatus = false; }
                else {
                    if (mgr.statusChoice == CS_SEMIPRO || mgr.statusChoice == CS_AMATEUR) T.status = mgr.statusChoice;
                    else if (T.status == CS_PRO) T.status = CS_SEMIPRO;
                    mgr.needStatus = true;
                }
                if (old != T.status) {
                    applyContracts(T, true);
                    season.news.push_back(fmt("Statut du club : %s (%s).", statusName(T.status), f >= 0 ? "imposé par la division" : "choisi par le club"));
                }
            } else {
                T.status = f >= 0 ? f : (T.rating >= avg[pl.tier] + 2 ? CS_SEMIPRO : CS_AMATEUR);
                if (old != T.status || !T.squadGen) applyContracts(T, false);
            }
        }
    }
}

void Career::setUserStatus(int st) {
    Team& T = g_world.teams[userTeam];
    int f = forcedStatus(userTeam);
    if (f >= 0) st = f;
    mgr.statusChoice = st;
    mgr.needStatus = false;
    if (T.status != st) {
        T.status = st;
        g_world.ensureSquad(userTeam);
        if (st == CS_AMATEUR) for (auto& p : T.squad) p.contract = 2;
        else if (st == CS_SEMIPRO) for (auto& p : T.squad) if (p.contract == 0) p.contract = 1;
        season.news.push_back(fmt("Le club passe sous statut %s.", statusName(st)));
    }
}

// ------------------------------------------------------------------ affluence et recettes
static int refPrice(int cap) { return cap >= 20000 ? 25 : cap >= 8000 ? 15 : cap >= 3000 ? 8 : cap >= 1000 ? 5 : 3; }
static float kindWeight(int k) { return k == STK_COVERED ? 1.2f : k == STK_SEATS ? 1.0f : 0.9f; }

int Career::expectedAttendance(int stand, bool vip) const {
    const Team& T = g_world.teams[userTeam];
    const StadiumInfo& S = T.sta;
    int cap = S.capacity();
    float ref = (float)refPrice(cap);
    float D = S.fans * (0.8f + 0.03f * S.lights + 0.04f * S.screen + 0.03f * S.pitch + 0.02f * S.parking) * (0.85f + teamReputation(userTeam) / 330.f);
    float wsum = 0;
    for (auto& x : S.s) wsum += kindWeight(x.kind) * x.seats;
    const Stand& st = S.s[stand];
    if (vip) {
        float d = S.fans * 0.02f * std::exp(-1.0f * (st.vipPrice / (ref * 5.f) - 1.f));
        return std::min(st.vip, (int)d);
    }
    if (wsum <= 0) return 0;
    float pf = std::max(0.03f, std::min(1.7f, std::exp(-1.1f * (st.price / ref - 1.f))));
    float d = D * kindWeight(st.kind) * st.seats / wsum * pf;
    return std::min(st.seats, (int)d);
}

static int64_t g_eurRem = 0;
static void addEuros(Career& K, int64_t eur, int64_t* bucket) {
    g_eurRem += eur;
    int64_t k = g_eurRem / 1000;
    g_eurRem -= k * 1000;
    K.mgr.budget += k;
    if (k >= 0) K.mgr.seasonIncome += k; else K.mgr.seasonStadiumCost -= k;
    if (bucket) *bucket += k;
}

static int64_t merchSales(Team& T, double base);
void Career::homeMatchDay(int comp, int mi) {
    Team& T = g_world.teams[userTeam];
    StadiumInfo& S = T.sta;
    if (!S.init) initStadium(T, tierOfTeam(userTeam), -1);
    const Competition& C = season.comps[comp];
    const MatchRes& m = C.matches[mi];
    const Team& O = g_world.teams[m.away];
    // attractivité : adversaire, enjeu, forme
    float att = 1.0f;
    att += std::max(-0.3f, std::min(0.5f, (O.rating - T.rating) * 0.02f));
    if (C.format != FMT_LEAGUE) att += 0.15f;
    if (C.kind == 3 || C.kind == 8 || C.format == FMT_SINGLE) att += 0.4f;
    if (O.dept == T.dept && O.dept >= 0) att += 0.2f;              // derby
    int total = 0; int64_t gate = 0;
    for (int i = 0; i < 4; i++) {
        Stand& st = S.s[i];
        int a = std::min(st.seats, (int)(expectedAttendance(i, false) * att * g_rng.frange(0.9f, 1.1f)));
        int v = std::min(st.vip, (int)(expectedAttendance(i, true) * att));
        st.lastAtt = a + v;
        total += a + v;
        gate += (int64_t)a * st.price + (int64_t)v * st.vipPrice;
    }
    int64_t buv = (int64_t)(total * (0.8 + 1.2 * S.buvette));
    float shirtRef = T.status == CS_PRO ? 75.f : T.status == CS_SEMIPRO ? 45.f : 28.f;
    int64_t shop = (int64_t)(total * 0.012 * S.boutique * std::exp(-1.3 * (S.shirtPrice / shirtRef - 1.0)) * S.shirtPrice);
    int64_t park = (int64_t)(total * 0.25 * S.parking * 3);
    shop += merchSales(T, total);                                                 // écharpes, goodies... les jours de match
    { Merch& M = T.merch; int su = (int)(total * 0.012 * S.boutique * std::exp(-1.3 * (S.shirtPrice / shirtRef - 1.0))); M.shirtUnits += su; M.shirtRevenue += su * S.shirtPrice; }
    S.lastAtt = total; S.lastGate = (int32_t)(gate / 1000);
    S.bestAtt = std::max(S.bestAtt, total);
    S.seasonAttTotal += total; S.seasonHomeMatches++;
    {   // usure de la pelouse : hiver, pluie, affluence ; la réserve joue ici s'il n'y a pas de stade annexe
        int mo = std::max(0, std::min(10, (int)(season.now / 4.2)));
        bool winter = mo >= 3 && mo <= 7;
        float wear = S.pitch == 2 ? 0.7f : S.pitch == 1 ? 3.f : 5.5f;
        if (winter) wear *= 1.6f;
        wear *= g_rng.frange(0.8f, 1.3f);
        if (!S.annexReserve) wear += S.pitch == 2 ? 0.3f : 1.5f;
        S.turf = (uint8_t)std::max(0, (int)S.turf - (int)(wear + 0.5f));
    }
    // recette : TVA (5,5 % sur la billetterie), prélèvement de la fédération / de la ligue, arbitrage, organisation
    GateInfo gi; gi.brut = gate; gi.att = total; gi.opp = O.name;
    gi.tva = gate * 55 / 1055;
    bool cdfM = C.kind == 2;
    int pct = cdfM ? 10 : (C.kind == 4 || C.kind == 5) ? 5 : C.kind == 1 && teamLevel(userTeam) >= 3 ? 3 : 0;
    gi.prelev = (gate - gi.tva) * pct / 100;
    gi.arbitrage = refereeFee(comp, m.home);
    gi.orga = (gate - gi.tva) / 5 + total * 1200LL / 1000;              // sécurité, stadiers, billetterie : 20 % + 1,2 EUR par spectateur
    gi.net = gate - gi.tva - gi.prelev - gi.arbitrage - gi.orga;
    gi.part = gi.net;
    // Coupe de France : la recette nette est partagée entre les deux clubs ; un club pro peut laisser toute la recette à un amateur
    if (cdfM && gi.net > 0) {
        gi.shared = true;
        bool amateurOpp = O.status != CS_PRO;
        if (T.status == CS_PRO && amateurOpp && mgr.cdfGiveAll) { gi.part = 0; addReputation(userTeam, 2); season.news.push_back("Coupe de France : le club laisse toute la recette (" + money(gi.net / 1000) + ") à " + O.name + ". Beau geste salué par la presse."); }
        else {
            gi.part = gi.net / 2;
            if (T.status == CS_PRO && amateurOpp) addReputation(userTeam, -1);
        }
    }
    g_lastGate = gi;
    addEuros(*this, gi.part, &mgr.seasonGate);
    addEuros(*this, buv + park, &mgr.seasonGate);
    addEuros(*this, shop, &mgr.seasonShop);
    // supporters : les victoires attirent du monde
    bool win = m.hg > m.ag || (m.hg == m.ag && m.ph > m.pa);
    bool loss = m.hg < m.ag || (m.hg == m.ag && m.ph >= 0 && m.ph < m.pa);
    if (win) S.fans += S.fans / 250 + 3;
    if (loss) S.fans -= S.fans / 500;
    S.fans = std::max(50, S.fans);
}

// ------------------------------------------------------------------ produits dérivés
static const struct { const char* name; int ref[3]; float rate; int minShop; } MERCH[NUM_MERCH] = {
    { "Écharpe",              { 18, 12, 10 }, 0.020f, 0 },
    { "Bonnet / casquette",   { 22, 15, 12 }, 0.012f, 1 },
    { "Ballon officiel",      { 30, 20, 15 }, 0.006f, 1 },
    { "Maillot enfant",       { 60, 38, 25 }, 0.007f, 2 },
    { "Mug et goodies",       { 12, 9, 7 },   0.015f, 1 },
    { "Poster et calendrier", { 10, 8, 6 },   0.010f, 0 },
    { "Survêtement",          { 80, 50, 35 }, 0.003f, 3 },
    { "Peluche mascotte",     { 20, 15, 12 }, 0.006f, 2 },
    { "Maillot extérieur",    { 85, 50, 32 }, 0.004f, 2 },
    { "Troisième maillot",    { 90, 55, 35 }, 0.003f, 3 },
    { "Gourde et sac",        { 25, 18, 14 }, 0.008f, 1 },
    { "Coque de téléphone",   { 20, 15, 12 }, 0.009f, 2 },
    { "Livre du centenaire",  { 35, 28, 22 }, 0.003f, 3 },
    { "Figurine rétro 16 bits",{ 30, 25, 20 }, 0.004f, 4 },
};
const char* merchName(int k) { return k >= 0 && k < NUM_MERCH ? MERCH[k].name : "?"; }
int merchRefPrice(int k, int status) { return k >= 0 && k < NUM_MERCH ? MERCH[k].ref[std::max(0, std::min(2, status))] : 10; }
int merchMinShop(int k) { return k >= 0 && k < NUM_MERCH ? MERCH[k].minShop : 0; }
void initMerch(Team& t) {
    Merch& M = t.merch;
    if (M.init) return;
    M = Merch(); M.init = 1;
    for (int k = 0; k < NUM_MERCH; k++) { M.price[k] = (int16_t)merchRefPrice(k, t.status); M.on[k] = t.sta.boutique >= MERCH[k].minShop && k != 6 ? 1 : 0; }
}
// ventes de produits dérivés pour « base » acheteurs potentiels ; renvoie le bénéfice (€)
static int64_t merchSales(Team& T, double base) {
    Merch& M = T.merch;
    if (!M.init) initMerch(T);
    int64_t tot = 0;
    for (int k = 0; k < NUM_MERCH; k++) {
        if (!M.on[k] || T.sta.boutique < MERCH[k].minShop) continue;
        double ref = merchRefPrice(k, T.status);
        double u = base * MERCH[k].rate * (0.6 + 0.4 * T.sta.boutique) * std::exp(-1.5 * (M.price[k] / ref - 1.0)) * (0.7 + teamReputation(&T - &g_world.teams[0]) / 160.0);
        int units = (int)(u * g_rng.frange(0.85f, 1.15f));
        int64_t profit = (int64_t)(units * (M.price[k] - ref * 0.45));       // coût de fabrication : ~45 % du prix de référence
        M.units[k] += units; M.revenue[k] += (int32_t)profit;
        tot += profit;
    }
    return tot;
}

// ------------------------------------------------------------------ travaux
static const char* PJ_NAMES[NUM_PJ] = { "Agrandissement", "Couverture de tribune", "Loges VIP", "Buvette", "Boutique", "Parking", "Éclairage",
                                         "Pelouse", "Écran géant", "Vestiaires", "Musée du club", "Sièges (tribune debout)" };

// barème des travaux (k€) : les normes et les matériaux coûtent plus cher en professionnel qu'en amateur
static double statusCostMul(const Team& t) { return t.status == CS_PRO ? 1.0 : t.status == CS_SEMIPRO ? 0.7 : 0.45; }
int64_t Career::projectCost(const Team& t, int kind, int stand, int amount) {
    const StadiumInfo& S = t.sta;
    double m = statusCostMul(t);
    auto lv = [&](double base, int L) { return (int64_t)(base * m * (L * L + L) / 2.0 + 0.5); };   // niveaux : 1x, 3x, 6x, 10x...
    switch (kind) {
    case PJ_SEATS: { int k = S.s[stand].kind; double per = k == STK_COVERED ? 0.7 : k == STK_SEATS ? 0.3 : 0.1; return (int64_t)(amount * per * m) + 5; }
    case PJ_COVER: return (int64_t)(S.s[stand].seats * 0.25 * m) + 15;
    case PJ_UPGRADE_SEATS: return (int64_t)(S.s[stand].seats * 0.12 * m) + 8;
    case PJ_VIP: return (int64_t)(amount * 2.5 * m) + 15;
    case PJ_BUVETTE: return std::max<int64_t>(3, lv(8, S.buvette + 1));
    case PJ_BOUTIQUE: return std::max<int64_t>(5, lv(15, S.boutique + 1));
    case PJ_PARKING: return std::max<int64_t>(8, lv(30, S.parking + 1));
    case PJ_LIGHTS: return std::max<int64_t>(6, lv(20, S.lights + 1));
    case PJ_PITCH: return (int64_t)((S.pitch == 0 ? 300 : 400) * m);
    case PJ_SCREEN: return (int64_t)(120 * m);
    case PJ_VESTIAIRES: return std::max<int64_t>(6, lv(25, S.vestiaires + 1));
    case PJ_MUSEUM: return (int64_t)(180 * m);
    }
    return 0;
}

int Career::projectMonths(int kind, int amount) {
    switch (kind) {
    case PJ_SEATS: return std::min(12, 1 + amount / 1500);
    case PJ_COVER: return 3;
    case PJ_UPGRADE_SEATS: return 2;
    case PJ_VIP: return std::min(6, 2 + amount / 200);
    case PJ_PITCH: return 2;
    case PJ_MUSEUM: return 4;
    default: return 1;
    }
}

bool Career::startProject(const Project& pj, std::string& err) {
    Team& T = g_world.teams[userTeam];
    StadiumInfo& S = T.sta;
    for (auto& p : mgr.projects) if (p.kind == pj.kind && (p.stand == pj.stand || (pj.kind >= PJ_BUVETTE && pj.kind != PJ_UPGRADE_SEATS))) { err = "Des travaux de ce type sont déjà en cours."; return false; }
    int maxLv[NUM_PJ] = { 0, 0, 0, 5, 5, 4, 4, 2, 1, 5, 1, 0 };
    int cur = pj.kind == PJ_BUVETTE ? S.buvette : pj.kind == PJ_BOUTIQUE ? S.boutique : pj.kind == PJ_PARKING ? S.parking : pj.kind == PJ_LIGHTS ? S.lights :
              pj.kind == PJ_PITCH ? S.pitch : pj.kind == PJ_SCREEN ? S.screen : pj.kind == PJ_VESTIAIRES ? S.vestiaires : pj.kind == PJ_MUSEUM ? S.museum : -1;
    if (cur >= 0 && cur >= maxLv[pj.kind]) { err = "Niveau maximum déjà atteint."; return false; }
    if (pj.kind == PJ_COVER && S.s[pj.stand].kind == STK_COVERED) { err = "Cette tribune est déjà couverte."; return false; }
    if (pj.kind == PJ_COVER && S.s[pj.stand].kind == STK_STANDING) { err = "Installez d'abord des sièges dans cette tribune."; return false; }
    if (pj.kind == PJ_UPGRADE_SEATS && S.s[pj.stand].kind != STK_STANDING) { err = "Cette tribune a déjà des sièges."; return false; }
    if (pj.kind == PJ_SEATS && S.s[pj.stand].seats + pj.amount > 40000) { err = "Tribune trop grande (40 000 places maximum)."; return false; }
    if (pj.kind == PJ_SEATS && S.owner == 0 && pj.amount > S.extQuota) {
        err = S.extQuota > 0 ? fmt("La mairie n'a autorisé que %d places supplémentaires.", S.extQuota) : "Stade municipal : demandez d'abord l'autorisation d'agrandir à la mairie (onglet Foncier).";
        return false;
    }
    Project p = pj;
    p.cost = (int32_t)projectCost(T, pj.kind, pj.stand, pj.amount);
    if (S.owner == 0 && (pj.kind == PJ_SEATS || pj.kind == PJ_COVER || pj.kind == PJ_UPGRADE_SEATS)) p.cost = p.cost * 6 / 10;   // la mairie finance 40 % des tribunes
    p.monthsLeft = projectMonths(pj.kind, pj.amount);
    if (p.cost > mgr.budget) { err = "Budget insuffisant (" + money(p.cost) + ")."; return false; }
    if (pj.kind == PJ_SEATS && S.owner == 0) S.extQuota -= pj.amount;
    mgr.budget -= p.cost; mgr.seasonStadiumCost += p.cost;
    mgr.projects.push_back(p);
    season.news.push_back(fmt("Travaux lancés : %s (%s, %d mois).", PJ_NAMES[p.kind], money(p.cost).c_str(), p.monthsLeft));
    return true;
}

const char* projectName(int k) { return k >= 0 && k < NUM_PJ ? PJ_NAMES[k] : "?"; }

// un mois : travaux, entretien, boutique, sponsor du stade
void stadiumMonth(Career& K) {
    Team& T = g_world.teams[K.userTeam];
    StadiumInfo& S = T.sta;
    if (!S.init) initStadium(T, K.tierOfTeam(K.userTeam), -1);
    for (size_t i = 0; i < K.mgr.projects.size();) {
        Project& p = K.mgr.projects[i];
        if (--p.monthsLeft > 0) { i++; continue; }
        Stand& st = S.s[p.stand];
        switch (p.kind) {
        case PJ_SEATS: st.seats += p.amount; break;
        case PJ_COVER: st.kind = STK_COVERED; break;
        case PJ_UPGRADE_SEATS: st.kind = STK_SEATS; break;
        case PJ_VIP: st.vip += p.amount; break;
        case PJ_BUVETTE: S.buvette++; break;
        case PJ_BOUTIQUE: S.boutique++; break;
        case PJ_PARKING: S.parking++; break;
        case PJ_LIGHTS: S.lights++; break;
        case PJ_PITCH: S.pitch++; break;
        case PJ_SCREEN: S.screen++; break;
        case PJ_VESTIAIRES: S.vestiaires++; break;
        case PJ_MUSEUM: S.museum++; S.fans += S.fans / 20; break;
        }
        K.season.news.push_back(std::string("Travaux terminés : ") + PJ_NAMES[p.kind] + fmt(" (capacité : %d places).", S.capacity()));
        K.mgr.projects.erase(K.mgr.projects.begin() + i);
    }
    int cap = S.capacity();
    int64_t upkeep = (int64_t)(cap * 0.35 + S.s[0].vip * 2.0 + (S.buvette + S.boutique + S.parking + S.lights + S.vestiaires) * 150.0 + S.screen * 800 + S.pitch * 500);   // €
    {   // pelouse : repousse selon la saison (août = 0 ... juin = 10) ; le synthétique vieillit lentement
        int mo = std::max(0, std::min(10, K.mgr.lastMonth));
        bool winter = mo >= 3 && mo <= 7;
        int regrow = S.pitch == 2 ? -1 : S.pitch == 1 ? (winter ? 3 : 6) : (winter ? 1 : 5);
        regrow += S.annexTraining > 0 ? 1 : 0;           // l'équipe s'entraîne ailleurs : la pelouse respire
        S.turf = (uint8_t)std::max(0, std::min(100, (int)S.turf + regrow));
        S.repaired = 0;
        if (S.turf < 25) K.season.news.push_back(fmt("Pelouse : état %s (%d %%). Le ballon rebondit n'importe comment : un regarnissage ou une nouvelle pelouse s'impose.", turfStateName(S.turf), (int)S.turf));
    }
    if (S.owner == 0) upkeep += (int64_t)S.rentK * 1000 / 11;                                        // loyer à la mairie
    else upkeep += (int64_t)(cap * 0.25);                                                            // propriétaire : gros entretien à sa charge
    upkeep += (int64_t)(S.annexReserve * 1500 + S.annexYouth * 1200 + S.annexTraining * 2500) * (T.status == CS_PRO ? 3 : 1);
    if (S.extPending > 0 && --S.extPending == 0) {
        // réponse de la mairie : dépend du statut, du public et des résultats
        float ch = 0.35f + (T.status == CS_PRO ? 0.25f : T.status == CS_SEMIPRO ? 0.12f : 0.f) + std::min(0.25f, S.fans / std::max(1.f, (float)cap) * 0.2f) + (K.mgr.confidence - 50) / 250.f;
        if (g_rng.chance(std::max(0.15f, std::min(0.9f, ch)))) {
            int q = std::max(500, (cap * 3 / 10) / 500 * 500);
            S.extQuota += q;
            K.season.news.push_back(fmt("La mairie accepte l'agrandissement du stade : %d places supplémentaires autorisées (elle financera 40 %% des tribunes).", q));
        } else K.season.news.push_back("La mairie refuse pour l'instant l'agrandissement du stade. Nouvelle demande possible.");
    }
    float shirtRef = T.status == CS_PRO ? 75.f : T.status == CS_SEMIPRO ? 45.f : 28.f;
    int64_t shop = (int64_t)(S.fans * 0.004 * S.boutique * std::exp(-1.3 * (S.shirtPrice / shirtRef - 1.0)) * S.shirtPrice);
    int64_t museum = (int64_t)(S.museum * S.fans * 0.02 * 8);
    // produits dérivés hors match (supporters) et boutique en ligne
    {
        Merch& M = T.merch; if (!M.init) initMerch(T);
        int64_t m1 = merchSales(T, S.fans * 0.15);
        int64_t online = 0;
        if (M.online > 0) {
            online = merchSales(T, S.fans * 0.12 * M.online) + (int64_t)(S.fans * 0.0015 * M.online * std::exp(-1.3 * (S.shirtPrice / shirtRef - 1.0)) * S.shirtPrice);
            online -= 1500LL * M.online;                                            // hébergement, logistique
            M.onlineRevenue += (int32_t)online;
        }
        int su = (int)(S.fans * 0.004 * S.boutique * std::exp(-1.3 * (S.shirtPrice / shirtRef - 1.0))); M.shirtUnits += su; M.shirtRevenue += su * S.shirtPrice;
        shop += m1 + online;
    }
    int64_t naming = (int64_t)S.namingIncome * 1000 / 11;
    addEuros(K, shop + museum, &K.mgr.seasonShop);
    addEuros(K, naming, nullptr);
    addEuros(K, -upkeep, nullptr);
    // cahier des charges de la division
    int t = K.tierOfTeam(K.userTeam);
    int p0, q0, g0; K.tierOfTeam(K.userTeam, &p0, &q0, &g0);
    if (t >= 0 && K.pyramids[p0].country == "FRA" && K.pyramids[p0].dom < 0) {
        int req = minCapacity(t);
        if (req > 0 && cap < req) {
            int64_t fine = t <= 1 ? 50 : t == 2 ? 15 : t <= 4 ? 3 : 1;
            K.mgr.budget -= fine; K.mgr.seasonStadiumCost += fine;
        }
    }
}

int64_t Career::namingOffer() const {
    const Team& T = g_world.teams[userTeam];
    int cap = T.sta.capacity();
    double base = cap * (T.status == CS_PRO ? 0.12 : T.status == CS_SEMIPRO ? 0.05 : 0.02);
    return std::max<int64_t>(2, (int64_t)(base * (1.0 + T.sta.fans / std::max(1.0, (double)cap) * 0.3)));
}

// ------------------------------------------------------------------ pelouse
const char* turfStateName(int q) {
    return q >= 85 ? "excellente" : q >= 65 ? "bonne" : q >= 45 ? "moyenne" : q >= 25 ? "usée" : q >= 10 ? "très abîmée" : "champ de patates";
}
// état de la pelouse d'un club : celle du joueur est suivie match après match, les autres sont estimées
int turfQualityFor(int team, int month) {
    if (team < 0 || team >= (int)g_world.teams.size()) return 80;
    const Team& T = g_world.teams[team];
    if (T.sta.init && g_career.kind == CK_CLUB && team == g_career.userTeam) return T.sta.turf;
    int base = T.status == CS_PRO ? 88 : T.status == CS_SEMIPRO ? 72 : 58;
    if (T.sta.init && T.sta.pitch == 2) base = 90;
    bool winter = month >= 3 && month <= 7;
    if (winter && !(T.sta.init && T.sta.pitch == 2)) base -= T.status == CS_PRO ? 8 : 18;
    base += (int)(hashStr(T.name) % 21) - 10;
    return std::max(5, std::min(100, base));
}
int64_t Career::turfRepairCost() const {
    const Team& T = g_world.teams[userTeam];
    return std::max<int64_t>(2, (int64_t)((8 + T.sta.capacity() / 2500) * statusCostMul(T)));
}
int64_t Career::turfReplaceCost() const {
    const Team& T = g_world.teams[userTeam];
    double base = T.sta.pitch == 2 ? 350 : T.sta.pitch == 1 ? 280 : 90;
    return std::max<int64_t>(8, (int64_t)(base * statusCostMul(T)));
}
bool Career::repairTurf(std::string& err) {
    Team& T = g_world.teams[userTeam]; StadiumInfo& S = T.sta;
    if (S.pitch == 2) { err = "Pelouse synthétique : pas de regarnissage possible, il faut la remplacer."; return false; }
    if (S.repaired) { err = "Un regarnissage a déjà été fait ce mois-ci."; return false; }
    if (S.turf >= 95) { err = "La pelouse est déjà en parfait état."; return false; }
    int64_t c = turfRepairCost();
    if (c > mgr.budget) { err = "Budget insuffisant (" + money(c) + ")."; return false; }
    mgr.budget -= c; mgr.seasonStadiumCost += c;
    S.turf = (uint8_t)std::min(100, S.turf + 25); S.repaired = 1;
    season.news.push_back(fmt("Pelouse regarnie (%s) : état %s.", money(c).c_str(), turfStateName(S.turf)));
    return true;
}
bool Career::replaceTurf(std::string& err) {
    Team& T = g_world.teams[userTeam]; StadiumInfo& S = T.sta;
    int64_t c = turfReplaceCost();
    if (c > mgr.budget) { err = "Budget insuffisant (" + money(c) + ")."; return false; }
    mgr.budget -= c; mgr.seasonStadiumCost += c;
    S.turf = 100; S.repaired = 1;
    season.news.push_back(fmt("Nouvelle pelouse posée (%s) : terrain comme neuf.", money(c).c_str()));
    return true;
}
// ------------------------------------------------------------------ foncier : achat du stade, mairie, stades annexes
int64_t Career::stadiumBuyPrice() const {
    const Team& T = g_world.teams[userTeam];
    int cap = T.sta.capacity();
    double perSeat = T.status == CS_PRO ? 0.5 : T.status == CS_SEMIPRO ? 0.25 : 0.1;   // k€ par place
    return std::max<int64_t>(50, (int64_t)(cap * perSeat + T.sta.rentK * 8));
}
bool Career::buyStadium(std::string& err) {
    Team& T = g_world.teams[userTeam]; StadiumInfo& S = T.sta;
    if (S.owner) { err = "Le club est déjà propriétaire de son stade."; return false; }
    int64_t c = stadiumBuyPrice();
    if (c > mgr.budget) { err = "Budget insuffisant (" + money(c) + ")."; return false; }
    mgr.budget -= c; mgr.seasonStadiumCost += c;
    S.owner = 1; S.extPending = 0; S.extQuota = 0;
    season.news.push_back(fmt("Le club rachète son stade à la mairie pour %s : plus de loyer, agrandissements libres, mais entretien à sa charge.", money(c).c_str()));
    return true;
}
bool Career::requestExtension(std::string& err) {
    StadiumInfo& S = g_world.teams[userTeam].sta;
    if (S.owner) { err = "Propriétaire : pas besoin d'autorisation pour agrandir."; return false; }
    if (S.extPending) { err = "Une demande est déjà à l'étude à la mairie."; return false; }
    S.extPending = 2;
    season.news.push_back("Demande d'agrandissement du stade déposée à la mairie : réponse dans deux mois.");
    return true;
}
static const int ANNEX_MAX[3] = { 1, 2, 3 };
int64_t Career::annexCost(int kind) const {
    const Team& T = g_world.teams[userTeam]; const StadiumInfo& S = T.sta;
    int L = (kind == 0 ? S.annexReserve : kind == 1 ? S.annexYouth : S.annexTraining) + 1;
    static const double BASE[3] = { 120, 90, 150 };
    return std::max<int64_t>(10, (int64_t)(BASE[kind] * L * statusCostMul(T)));
}
bool Career::buildAnnex(int kind, std::string& err) {
    Team& T = g_world.teams[userTeam]; StadiumInfo& S = T.sta;
    uint8_t& lv = kind == 0 ? S.annexReserve : kind == 1 ? S.annexYouth : S.annexTraining;
    if (lv >= ANNEX_MAX[kind]) { err = "Niveau maximum déjà atteint."; return false; }
    int64_t c = annexCost(kind);
    if (c > mgr.budget) { err = "Budget insuffisant (" + money(c) + ")."; return false; }
    mgr.budget -= c; mgr.seasonStadiumCost += c;
    lv++;
    static const char* N[3] = { "stade de l'équipe réserve", "stade des jeunes", "complexe d'entraînement" };
    season.news.push_back(fmt("Construction : %s (niveau %d) pour %s.", N[kind], (int)lv, money(c).c_str()));
    return true;
}
