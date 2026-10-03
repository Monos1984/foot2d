// Vie privée (manager ou joueur incarné), carrière de joueur / joueuse, paris sportifs et « valise à l'arbitre »
#include "game.h"
#include "serial.h"
#include <cstring>
#include <cmath>
#include <algorithm>

extern Rng g_rng;

static const char* HOUSE_N[5] = { "studio en location", "appartement", "maison", "maison avec jardin", "villa avec piscine" };
static const int64_t HOUSE_P[5] = { 0, 120000, 320000, 750000, 2800000 };         // prix d'achat (€)
static const int HOUSE_M[5] = { 650, 350, 700, 1400, 5000 };                         // loyer / charges (€ / mois)
static const char* CAR_N[5] = { "aucune (transports en commun)", "citadine d'occasion", "berline", "coupé sport", "supercar" };
static const int64_t CAR_P[5] = { 0, 9000, 38000, 120000, 420000 };
static const int CAR_M[5] = { 60, 120, 280, 900, 2600 };

const char* lifeHouseName(int k) { return HOUSE_N[std::max(0, std::min(4, k))]; }
const char* lifeCarName(int k) { return CAR_N[std::max(0, std::min(4, k))]; }
int64_t lifeHousePrice(int k) { return HOUSE_P[std::max(0, std::min(4, k))]; }
int64_t lifeCarPrice(int k) { return CAR_P[std::max(0, std::min(4, k))]; }

static int lifeMonthOf(double t) { return std::max(0, std::min(10, (int)(t / 4.2))); }

Player* Career::lifePlayer(int* team) {
    if (!life.isPlayer || life.pid < 0) return nullptr;
    int idx = -1;
    int t = g_world.findPlayer(life.pid, &idx);
    if (t < 0 || idx < 0) return nullptr;
    if (team) *team = t;
    return &g_world.teams[t].squad[idx];
}

// salaire annuel (k€) : joueur selon son contrat ; manager selon la division du club
int32_t Career::lifeSalaryK() const {
    if (life.isPlayer) {
        Career* self = const_cast<Career*>(this);
        Player* p = self->lifePlayer();
        if (!p) return 0;
        int w = playerWage(*p);
        return w > 0 ? w : 14;          // licence amateur : petit boulot à côté du foot
    }
    int t = teamLevel(userTeam);
    static const int SAL[8] = { 1500, 450, 140, 60, 35, 24, 18, 14 };
    return SAL[std::max(0, std::min(7, t))];
}

void Career::lifeStart(bool isPlayer, int pid) {
    life = PlayerLife();
    life.isPlayer = isPlayer ? 1 : 0;
    life.pid = pid;
    life.salaryK = lifeSalaryK();
    life.cash = (int64_t)life.salaryK * 1000 / 5 + 3000;       // quelques économies
    life.morale = 65;
    life.partnerGender = 1;
}

// un mois de vie : salaire, dépenses, couple, moral, enquête éventuelle
void Career::lifeMonth() {
    if (kind != CK_CLUB || userTeam < 0) return;
    if(managerLifeEnabled()) {
    life.salaryK = lifeSalaryK();
    int64_t income = (int64_t)life.salaryK * 1000 / 11;
    int64_t spend = HOUSE_M[life.house] + CAR_M[life.car] + 450 + (life.relation >= 1 ? 350 : 0) + (life.relation == 3 ? 600 : 0);
    life.cash += income - spend;
    // couple : la complicité s'érode sans attention
    if (life.relation > 0) {
        life.love = (uint8_t)std::max(0, (int)life.love - 4);
        if (life.love < 12 && g_rng.chance(0.35f)) {
            season.news.push_back(std::string("Vie privée : ") + life.partner + " vous quitte... Trop de football, pas assez d'attention.");
            life.relation = 0; life.love = 0; life.partner[0] = 0;
            life.morale = (uint8_t)std::max(5, (int)life.morale - 20);
        }
    }
    // moral : confort de vie, couple, résultats récents, temps de jeu (joueur)
    int target = 42 + life.house * 3 + life.car * 2 + life.relation * 4 + life.love / 12;
    {
        int pts = 0, n = 0;
        for (int c = 0; c < (int)season.comps.size() && n < 60; c++) {
            const Competition& C = season.comps[c];
            for (auto& m : C.matches) {
                if (!m.played || (m.home != userTeam && m.away != userTeam)) continue;
                int gf = m.home == userTeam ? m.hg : m.ag, ga = m.home == userTeam ? m.ag : m.hg;
                pts += gf > ga ? 3 : gf == ga ? 1 : 0; n++;
            }
        }
        if (n) target += (int)((pts / (float)n - 1.4f) * 8);
    }
    Player* me = nullptr; int myTeam = -1;
    if (life.isPlayer) {
        me = lifePlayer(&myTeam);
        if (me) target += me->apps >= 3 ? 6 : -6;
        if (life.suspendedM) { target -= 15; life.suspendedM--; if (me) me->suspended = std::max<int8_t>(me->suspended, 3); }
    }
    if (life.cash < 0) target -= 18;
    if (life.heat > 50) target -= 6;
    target = std::max(10, std::min(98, target));
    life.morale = (uint8_t)std::max(5, std::min(100, (int)life.morale + (target - (int)life.morale) * 4 / 10 + g_rng.range(-3, 3)));
    // le moral du joueur incarné est celui de sa vie ; le manager transmet un peu du sien au groupe
    if (me) me->morale = life.morale;
    else if (!life.isPlayer) for (auto& p : g_world.teams[userTeam].squad) p.morale = (uint8_t)std::max(10, std::min(100, (int)p.morale + ((int)life.morale - 60) / 12));
    if (life.cash < -20000) season.news.push_back("Vie privée : votre banquier s'inquiète, votre compte est très à découvert.");
    // événements de la vie
    if (g_rng.chance(0.06f)) {
        static const char* EV[] = { "Vie privée : un magazine people vous consacre un article flatteur.", "Vie privée : votre voisin se plaint du bruit de vos soirées.",
                                    "Vie privée : vous inaugurez le tournoi de foot des écoles de votre ville.", "Vie privée : une panne de voiture vous met en retard à l'entraînement." };
        season.news.push_back(EV[g_rng.range(0, 3)]);
    }
    } // vie privée facultative ; les enquêtes restent indépendantes
    // corruption : les soupçons retombent lentement ; une enquête peut tomber
    if (!opts.disableBribes && life.heat > 0) {
        if (g_rng.chance(life.heat / 300.f)) {
            life.bribesCaught++;
            int64_t fineK = std::max<int64_t>(5, (int64_t)life.bribes * 20 + mgr.incomeBase / 20);
            std::string who = life.isPlayer ? "vous" : "le club";
            season.news.push_back("AFFAIRE DE CORRUPTION : la commission de discipline ouvre une enquête sur des matchs arrangés.");
            if (life.isPlayer) {
                life.cash -= fineK * 1000 / 4;
                life.suspendedM = (uint8_t)(life.heat > 60 ? 6 : 3);
                season.news.push_back(fmt("Sanction : amende de %s et suspension de %d mois.", money(fineK / 4).c_str(), (int)life.suspendedM));
            } else {
                mgr.budget -= fineK; mgr.seasonStadiumCost += fineK;
                life.cash -= fineK * 100;
                season.news.push_back("Sanction : amende de " + money(fineK) + " pour " + who + ".");
                if (life.heat >= 70) { life.adminRelegate = 1; season.news.push_back("La ligue prononce une rétrogradation administrative du club à la fin de la saison !"); }
                if (life.heat >= 45 && !mgr.noSack) { mgr.confidence = 0; mgr.sacked = 1; season.news.push_back("Le président vous licencie pour faute grave."); }
                else mgr.confidence = std::max(0, mgr.confidence - 25);
            }
            life.morale = (uint8_t)std::max(5, (int)life.morale - 25);
            life.heat = 0;
        } else life.heat = (uint8_t)std::max(0, (int)life.heat - 4);
    }
    lifeResolveBets();
}

void Career::lifeAfterMatch(int comp, int mi) {
    if (life.bribeComp == comp && life.bribeMatch == mi) { life.bribeComp = life.bribeMatch = life.bribeTeam = -1; }
    lifeResolveBets();
    (void)comp; (void)mi;
}

// ------------------------------------------------------------------ achats et couple
bool Career::lifeBuy(int what, int level, std::string& err) {
    uint8_t& cur = what == 0 ? life.house : life.car;
    if (level < 0 || level > 4 || level == cur) { err = "Déjà votre situation actuelle."; return false; }
    int64_t price = what == 0 ? HOUSE_P[level] : CAR_P[level];
    int64_t resale = (what == 0 ? HOUSE_P[cur] * 85 : CAR_P[cur] * 55) / 100;   // revente de l'ancien bien
    int64_t net = price - resale;
    if (net > life.cash) { err = fmt("Pas assez d'argent : il manque %s.", money((net - life.cash) / 1000 + 1).c_str()); return false; }
    life.cash -= net;
    cur = (uint8_t)level;
    life.morale = (uint8_t)std::min(100, life.morale + (level > 0 ? 6 : 0));
    season.news.push_back(std::string("Vie privée : nouvel achat, ") + (what == 0 ? HOUSE_N[level] : CAR_N[level]) + ".");
    return true;
}

bool Career::lifeGift(int kind, std::string& err) {
    if (life.relation == 0) { err = "Vous êtes célibataire."; return false; }
    static const int COST[4] = { 60, 220, 2500, 7000 }, LOVE[4] = { 4, 8, 16, 26 };
    static const char* N[4] = { "un bouquet de fleurs", "un dîner au restaurant", "un bijou", "un week-end en amoureux" };
    if (COST[kind] > life.cash) { err = "Pas assez d'argent."; return false; }
    life.cash -= COST[kind];
    life.love = (uint8_t)std::min(100, life.love + LOVE[kind]);
    life.morale = (uint8_t)std::min(100, life.morale + 2);
    season.news.push_back(std::string("Vie privée : vous offrez ") + N[kind] + " à " + life.partner + ".");
    return true;
}

bool Career::lifeDate(std::string& err) {
    if (life.relation > 0) { err = "Vous êtes déjà en couple."; return false; }
    if (life.cash < 150) { err = "Pas assez d'argent pour sortir."; return false; }
    life.cash -= 150;
    float ch = 0.18f + life.morale / 400.f + life.house * 0.03f + life.car * 0.03f;
    if (!g_rng.chance(ch)) { err = "Soirée sympa, mais pas de rencontre cette fois."; life.morale = (uint8_t)std::min(100, life.morale + 1); return false; }
    static const char* FN[] = { "Léa", "Camille", "Inès", "Manon", "Sarah", "Chloé", "Emma", "Jade", "Lina", "Zoé", "Clara", "Yasmine" };
    static const char* MN[] = { "Lucas", "Hugo", "Nathan", "Karim", "Théo", "Louis", "Adam", "Mathis", "Yanis", "Enzo", "Jules", "Rayan" };
    const char* nm = life.partnerGender ? FN[g_rng.range(0, 11)] : MN[g_rng.range(0, 11)];
    snprintf(life.partner, sizeof life.partner, "%s", nm);
    life.relation = 1; life.love = 45;
    life.morale = (uint8_t)std::min(100, life.morale + 8);
    season.news.push_back(std::string("Vie privée : coup de foudre ! Vous êtes désormais en couple avec ") + life.partner + ".");
    return true;
}

bool Career::lifePropose(std::string& err) {
    if (life.relation != 1) { err = "Il faut d'abord être en couple."; return false; }
    if (life.cash < 2500) { err = "Il faut une bague (2 500 EUR)."; return false; }
    life.cash -= 2500;
    if (life.love < 60 || !g_rng.chance(0.3f + life.love / 150.f)) {
        life.love = (uint8_t)std::max(0, life.love - 10);
        err = std::string(life.partner) + " préfère attendre encore un peu...";
        return false;
    }
    life.relation = 2; life.love = (uint8_t)std::min(100, life.love + 10);
    life.morale = (uint8_t)std::min(100, life.morale + 10);
    season.news.push_back(std::string("Vie privée : ") + life.partner + " a dit oui ! Vous voilà fiancé(e)s.");
    return true;
}

bool Career::lifeWedding(std::string& err) {
    if (life.relation != 2) { err = "Il faut d'abord être fiancé(e)s."; return false; }
    int64_t cost = 12000 + (int64_t)life.salaryK * 20;
    if (cost > life.cash) { err = fmt("Le mariage coûte %s.", money(cost / 1000).c_str()); return false; }
    life.cash -= cost;
    life.relation = 3; life.love = 100;
    life.morale = (uint8_t)std::min(100, life.morale + 15);
    season.news.push_back(std::string("Vie privée : mariage avec ") + life.partner + " ! Vos coéquipiers ont fait la fête toute la nuit.");
    return true;
}

bool Career::lifeBreakUp(std::string& err) {
    if (life.relation == 0) { err = "Vous êtes déjà célibataire."; return false; }
    if (life.relation == 3) life.cash -= std::max<int64_t>(0, life.cash / 2);      // divorce : partage des biens
    season.news.push_back(std::string("Vie privée : séparation avec ") + life.partner + ".");
    life.relation = 0; life.love = 0; life.partner[0] = 0;
    life.morale = (uint8_t)std::max(5, (int)life.morale - 15);
    return true;
}

// ------------------------------------------------------------------ paris sportifs (argent personnel)
float Career::lifeOdds(int comp, int mi, int pick) const {
    const MatchRes& m = season.comps[comp].matches[mi];
    double d = teamStrength(m.home,RULESET_CAREER) - teamStrength(m.away,RULESET_CAREER) + (m.neutral ? 0 : 4.0);
    double ph = 1.0 / (1.0 + std::exp(-d / 9.0)) * 0.78, pd = 0.26 - std::min(0.12, std::fabs(d) / 150.0);
    double pa = std::max(0.04, 1.0 - ph - pd);
    ph = std::max(0.04, ph);
    double p = pick == 0 ? ph : pick == 1 ? pd : pa;
    return (float)std::max(1.05, std::min(25.0, 0.92 / p));        // marge du bookmaker
}

bool Career::lifeBet(int comp, int mi, int pick, int stake, std::string& err) {
    if (life.betComp >= 0) { err = "Un pari est déjà en cours."; return false; }
    if (stake <= 0 || stake > life.cash) { err = "Mise impossible avec votre argent personnel."; return false; }
    const MatchRes& m = season.comps[comp].matches[mi];
    if (m.played) { err = "Ce match est déjà joué."; return false; }
    if (life.isPlayer && (m.home == userTeam || m.away == userTeam)) { err = "Interdit : un joueur ne peut pas parier sur un match de son équipe."; return false; }
    life.cash -= stake;
    life.betComp = comp; life.betMatch = mi; life.betPick = (uint8_t)pick; life.betStake = stake; life.betOdds = lifeOdds(comp, mi, pick);
    return true;
}

void Career::lifeResolveBets() {
    if (life.betComp < 0 || life.betComp >= (int)season.comps.size()) { life.betComp = -1; return; }
    const Competition& C = season.comps[life.betComp];
    if (life.betMatch < 0 || life.betMatch >= (int)C.matches.size()) { life.betComp = -1; return; }
    const MatchRes& m = C.matches[life.betMatch];
    if (!m.played) return;
    int res = m.hg > m.ag ? 0 : m.hg == m.ag ? 1 : 2;
    std::string label = g_world.teams[m.home].name + fmt(" %d-%d ", m.hg, m.ag) + g_world.teams[m.away].name;
    if (res == life.betPick) {
        int64_t gain = (int64_t)(life.betStake * life.betOdds);
        life.cash += gain; life.betsWon++; life.betBalance += gain - life.betStake;
        season.news.push_back(fmt("Paris sportifs : pari gagné sur %s ! Gain : %lld EUR.", label.c_str(), (long long)gain));
        life.morale = (uint8_t)std::min(100, life.morale + 3);
    } else {
        life.betsLost++; life.betBalance -= life.betStake;
        season.news.push_back(fmt("Paris sportifs : pari perdu sur %s (mise : %d EUR).", label.c_str(), life.betStake));
    }
    life.betComp = life.betMatch = -1; life.betStake = 0;
}

// ------------------------------------------------------------------ « valise à l'arbitre »
int64_t Career::bribeCost(int comp, int mi, int kind) const {
    const MatchRes& m = season.comps[comp].matches[mi];
    int opp = m.home == userTeam ? m.away : m.home;
    int lvl = std::min(teamLevel(userTeam), teamLevel(opp));
    static const int BASE[8] = { 400, 150, 60, 25, 12, 6, 4, 3 };
    static const int MUL[3] = { 100, 60, 180 };          // arbitre, joueur adverse, équipe adverse
    int64_t c = (int64_t)BASE[std::max(0, std::min(7, lvl))] * MUL[kind] / 100;
    const Competition& C = season.comps[comp];
    if (C.format != FMT_LEAGUE) c = c * 3 / 2;          // un match de coupe « vaut » plus cher
    return std::max<int64_t>(1, c);
}

bool Career::bribe(int comp, int mi, int kind, std::string& err) {
    if(opts.disableBribes) { err="La valise à l'arbitre est désactivée pour cette carrière."; return false; }
    if(comp<0 || comp>=(int)season.comps.size() || mi<0 || mi>=(int)season.comps[comp].matches.size() || kind<0 || kind>2) { err="Match ou action invalide."; return false; }
    if(season.comps[comp].matches[mi].home!=userTeam && season.comps[comp].matches[mi].away!=userTeam) { err="Ce match ne concerne pas votre équipe."; return false; }
    if (life.bribeComp >= 0) { err = "Une valise est déjà en route pour le prochain match."; return false; }
    const MatchRes& m = season.comps[comp].matches[mi];
    if (m.played) { err = "Match déjà joué."; return false; }
    int64_t c = bribeCost(comp, mi, kind);
    if (life.isPlayer) {
        if (c * 1000 > life.cash) { err = "Pas assez d'argent personnel (" + money(c) + ")."; return false; }
        life.cash -= c * 1000;
    } else {
        if (c > mgr.budget) { err = "Caisse noire insuffisante (" + money(c) + ")."; return false; }
        mgr.budget -= c; mgr.seasonStadiumCost += c;
    }
    life.bribeComp = comp; life.bribeMatch = mi; life.bribeKind = (uint8_t)kind;
    life.bribeTeam = m.home == userTeam ? m.away : m.home;
    static const int HEAT[3] = { 22, 16, 32 };
    life.heat = (uint8_t)std::min(100, life.heat + HEAT[kind] + life.bribes * 2);
    life.bribes++;
    return true;
}

// écart de niveau (points de note) dû à une valise sur ce match : positif = avantage à domicile
float lifeBribeDelta(const MatchRes& m) {
    if(g_career.opts.disableBribes) return 0;
    const PlayerLife& L = g_career.life;
    if (L.bribeComp < 0 || L.bribeComp >= (int)g_career.season.comps.size()) return 0;
    const Competition& C = g_career.season.comps[L.bribeComp];
    if (L.bribeMatch < 0 || L.bribeMatch >= (int)C.matches.size() || &C.matches[L.bribeMatch] != &m) return 0;
    static const float D[3] = { 7.f, 5.f, 11.f };
    float d = D[L.bribeKind % 3];
    return L.bribeTeam == m.away ? d : -d;
}

void Career::saveV17(Writer& w) const { w.pod(life); w.vpod(loans); w.vpod(compArch); w.vpod(compAllTime); w.pod(mgr.mercatoFlags); w.pod(mgr.jokerUsed); w.pod(mgr.fpfStrikes); w.pod(mgr.fpfBan); w.pod(mgr.wageCapK); }
void Career::loadV17(Reader& r) { r.pod(life); r.vpod(loans); r.vpod(compArch); r.vpod(compAllTime); r.pod(mgr.mercatoFlags); r.pod(mgr.jokerUsed); r.pod(mgr.fpfStrikes); r.pod(mgr.fpfBan); r.pod(mgr.wageCapK); }
