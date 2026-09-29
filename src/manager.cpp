// Gestion du club (mode manager) : finances, objectifs du président, mercato
#include "game.h"
#include <set>
#include <map>
#include <algorithm>
#include <cstring>

std::string money(int64_t k) {
    bool neg = k < 0; if (neg) k = -k;
    std::string s;
    if (k >= 1000) {
        int64_t m10 = (k + 50) / 100;   // dixièmes de million
        s = m10 % 10 ? fmt("%lld,%lld M EUR", (long long)(m10 / 10), (long long)(m10 % 10)) : fmt("%lld M EUR", (long long)(m10 / 10));
    } else s = fmt("%lld k EUR", (long long)k);
    return neg ? "-" + s : s;
}

int64_t Career::wageBill(int team) const {
    g_world.ensureSquad(team);
    int64_t w = 0;
    for (auto& p : g_world.teams[team].squad) w += playerWage(p);
    return w;
}

// mois de la saison (0 = août ... 10 = juin)
static int monthOf(double t) { return std::max(0, std::min(10, (int)(t / 4.2))); }

bool Career::transferWindow() const {
    if (kind != CK_CLUB) return false;
    int m = monthOf(season.now);
    return m == 0 || m == 5 || season.finished;
}

std::string Career::windowText() const {
    if (kind != CK_CLUB) return "";
    int m = monthOf(season.now);
    if (m == 0) return "Mercato d'été ouvert (jusqu'à fin août)";
    if (m == 5) return "Mercato d'hiver ouvert (janvier)";
    if (m < 5) return "Mercato fermé (réouverture en janvier)";
    return "Mercato fermé (réouverture l'été prochain)";
}

std::string Career::objectiveText() const {
    switch (mgr.objective) {
    case 0: return "Remporter le titre";
    case 1: return fmt("Monter (finir dans les %d premiers)", mgr.objTarget);
    case 2: return fmt("Haut de tableau (top %d)", mgr.objTarget);
    case 3: return fmt("Milieu de tableau (top %d)", mgr.objTarget);
    default: return fmt("Maintien (%de ou mieux)", mgr.objTarget);
    }
}

void Career::mgrInit() {
    if (kind != CK_CLUB || userTeam < 0) return;
    int p, q, g;
    if (tierOfTeam(userTeam, &p, &q, &g) < 0) return;
    const Pool& pl = pyramids[p].pools[q];
    const TierConf& T = pyramids[p].tiers[pl.tier];
    const auto& grp = pl.groups[g];
    // revenus : proches de la masse salariale moyenne de la division
    int64_t tot = 0;
    for (int t : grp) tot += wageBill(t);
    int64_t avg = grp.empty() ? 50 : tot / (int64_t)grp.size();
    double rel = 1.0;
    { float ar = 0; for (int t : grp) ar += g_world.teams[t].rating; ar /= std::max<size_t>(1, grp.size()); rel = std::max(0.6, std::min(1.8, 1.0 + (g_world.teams[userTeam].rating - ar) * 0.03)); }
    // plancher : subventions, sponsors locaux, licences (clubs amateurs sans salaires)
    int fr = pyramids[p].country == "FRA" && pyramids[p].dom < 0;
    static const int FLOOR[13] = { 15000, 4000, 1500, 600, 300, 150, 90, 60, 35, 25, 18, 12, 10 };
    int64_t floor = fr ? FLOOR[std::min(12, pl.tier)] : (pl.tier == 0 ? 15000 : pl.tier == 1 ? 4000 : 1000);
    if (pyramids[p].dom >= 0) floor = 40;
    mgr.incomeBase = std::max<int64_t>(floor, std::max((int64_t)(avg * 0.9 * rel), wageBill(userTeam) * 95 / 100));
    if (mgr.budget == 0 && mgr.transfers.empty()) mgr.budget = std::max<int64_t>(15, mgr.incomeBase * 35 / 100);
    mgr.seasonIncome = mgr.seasonWages = mgr.seasonTransfers = 0;
    mgr.lastMonth = 0;
    // objectif selon la hiérarchie des notes
    int pos = 1; int n = (int)grp.size();
    for (int t : grp) if (t != userTeam && g_world.teams[t].rating > g_world.teams[userTeam].rating) pos++;
    int down = (T.flexible || pl.terminal) ? 0 : T.down;
    if (pl.tier == 0 && pos == 1) { mgr.objective = 0; mgr.objTarget = 1; }
    else if (pl.tier > 0 && T.up > 0 && pos <= T.up) { mgr.objective = 1; mgr.objTarget = T.up; }
    else if (pos <= n / 3) { mgr.objective = 2; mgr.objTarget = std::max(2, n / 3); }
    else if (pos <= 2 * n / 3) { mgr.objective = 3; mgr.objTarget = std::max(3, 2 * n / 3); }
    else if (down == 0) { mgr.objective = 3; mgr.objTarget = std::max(3, n - 2); }
    else { mgr.objective = 4; mgr.objTarget = std::max(1, n - down); }
    if (mgr.confidence <= 0) mgr.confidence = 50;
    if (!g_world.teams[userTeam].sponsor.empty() && mgr.sponsorIncome == 0) mgr.sponsorIncome = std::max<int64_t>(2, mgr.incomeBase / 12);
    if (!mgr.managerMode) mgr.noSack = true;
    season.news.push_back("Objectif du président : " + objectiveText() + ".");
    mgr.offers.clear();
    if (mgr.managerMode) genOffers(g_rng.range(0, 2));
}

// ------------------------------------------------------------------ offres de transfert reçues
void Career::genOffers(int n) {
    if (kind != CK_CLUB || euroOnly || userTeam < 0 || !mgr.managerMode) return;
    Team& U = g_world.teams[userTeam];
    g_world.ensureSquad(userTeam);
    if (U.squad.size() <= 17) return;
    for (int k = 0; k < n; k++) {
        // les meilleurs éléments et les jeunes à fort potentiel attirent les recruteurs
        std::vector<std::pair<float, int>> cand;
        for (int i = 0; i < (int)U.squad.size(); i++) {
            const Player& p = U.squad[i];
            bool already = false; for (auto& o : mgr.offers) if (o.pid == p.id) already = true;
            if (already || std::find(retiring.begin(), retiring.end(), p.id) != retiring.end()) continue;
            float w = p.overall() * 1.0f + std::max(0, p.pot - p.overall()) * 0.8f - std::max(0, p.age - 28) * 3.f + g_rng.frange(0, 12);
            cand.push_back({ -w, p.id });
        }
        if (cand.empty()) return;
        std::sort(cand.begin(), cand.end());
        int pick = cand[std::min((int)cand.size() - 1, g_rng.range(0, 3))].second;
        int fee = 0;
        int to = findBuyer(pick, fee);
        if (to < 0) continue;
        int idx; g_world.findPlayer(pick, &idx);
        const Player& P = U.squad[idx];
        fee = std::max(fee, (int)(P.value() * g_rng.frange(0.9f, 1.35f)));
        if (fee <= 0) fee = std::max(5, P.overall() / 4);
        TransferOffer o; o.pid = pick; o.club = to; o.fee = fee; o.expires = std::min(11, std::max(0, mgr.lastMonth) + 1);
        mgr.offers.push_back(o);
        season.news.push_back("Offre de transfert : " + g_world.teams[to].name + " propose " + money(fee) + " pour " + P.name + " (Gestion du club > Offres reçues).");
    }
}

bool Career::answerOffer(int k, int action, std::string& msg) {
    if (k < 0 || k >= (int)mgr.offers.size()) return false;
    TransferOffer o = mgr.offers[k];
    int idx; int src = g_world.findPlayer(o.pid, &idx);
    if (src != userTeam) { mgr.offers.erase(mgr.offers.begin() + k); msg = "Le joueur n'est plus au club."; return false; }
    std::string nm = g_world.teams[src].squad[idx].name;
    if (action == 0) {
        std::string err;
        if (!sellPlayer(o.pid, o.club, o.fee, err)) { msg = err; return false; }
        mgr.offers.erase(mgr.offers.begin() + k);
        msg = nm + " est vendu à " + g_world.teams[o.club].name + " pour " + money(o.fee) + ".";
        return true;
    }
    if (action == 1) {
        mgr.offers.erase(mgr.offers.begin() + k);
        Player& P = g_world.teams[src].squad[idx];
        // un joueur courtisé par un plus grand club peut mal le prendre
        if (g_world.teams[o.club].rating > g_world.teams[src].rating + 4) { P.morale = (uint8_t)std::max(10, P.morale - 8); msg = "Offre refusée. " + nm + " est déçu de ne pas pouvoir partir."; }
        else msg = "Offre refusée.";
        return true;
    }
    // négociation : le club acheteur accepte de monter de 20 % ... ou se retire
    if (g_rng.chance(0.55f)) {
        mgr.offers[k].fee = mgr.offers[k].fee * 6 / 5;
        msg = g_world.teams[o.club].name + " accepte de monter son offre à " + money(mgr.offers[k].fee) + ".";
    } else {
        mgr.offers.erase(mgr.offers.begin() + k);
        msg = g_world.teams[o.club].name + " retire son offre.";
    }
    return true;
}

// point du président à mi-saison : classement comparé à l'objectif
void Career::boardReview() {
    if (kind != CK_CLUB || euroOnly || userTeam < 0) return;
    int p, q, g;
    if (tierOfTeam(userTeam, &p, &q, &g) < 0) return;
    const Competition& C = season.comps[pyramids[p].pools[q].comps[g]];
    auto tb = C.table(0, 0);
    int pos = 0; for (int i = 0; i < (int)tb.size(); i++) if (tb[i].team == userTeam) pos = i + 1;
    if (!pos) return;
    std::string s = fmt("Le président fait le point à mi-saison : %d%s au classement, objectif ", pos, pos == 1 ? "er" : "e") + objectiveText() + ". ";
    int d = mgr.objTarget - pos;
    if (d >= 3) { s += "Il se dit ravi et débloque une petite enveloppe pour le mercato d'hiver."; mgr.confidence = std::min(100, mgr.confidence + 8); mgr.budget += std::max<int64_t>(10, mgr.incomeBase / 25); }
    else if (d >= 0) { s += "Il est satisfait et vous encourage à continuer."; mgr.confidence = std::min(100, mgr.confidence + 3); }
    else if (d >= -3) { s += "Il attend une réaction en deuxième partie de saison."; mgr.confidence = std::max(0, mgr.confidence - 4); }
    else { s += "Il est très inquiet : les résultats doivent vite s'améliorer."; mgr.confidence = std::max(0, mgr.confidence - 10); }
    season.news.push_back(s);
}

// ------------------------------------------------------------------ entraînement (effet mensuel)
const char* trainFocusName(int f) {
    static const char* N[] = { "Général", "Physique", "Technique", "Défense", "Attaque", "Gardiens" };
    return N[std::max(0, std::min(5, f))];
}
const char* trainIntName(int i) {
    static const char* N[] = { "Légère", "Normale", "Intense" };
    return N[std::max(0, std::min(2, i))];
}

void Career::trainingMonth() {
    if (kind != CK_CLUB || userTeam < 0) return;
    Team& T = g_world.teams[userTeam];
    if (mgr.delegTrain) {
        // l'adjoint choisit le thème (ligne la plus faible) et l'intensité (état physique de l'effectif)
        float cond = 0, df = 0, mf = 0, fw = 0, gk = 0; int n = 0, nd = 0, nm = 0, nf = 0, ng = 0;
        for (auto& p : T.squad) {
            cond += playerCond(userTeam, p); n++;
            if (p.pos == POS_DF) { df += p.tackle; nd++; } else if (p.pos == POS_MF) { mf += p.pass; nm++; } else if (p.pos == POS_FW) { fw += p.shoot; nf++; } else { gk += p.keep; ng++; }
        }
        cond /= std::max(1, n); df /= std::max(1, nd); mf /= std::max(1, nm); fw /= std::max(1, nf); gk /= std::max(1, ng);
        uint8_t f = 0, in = 1;
        float lo = std::min(std::min(df, mf), std::min(fw, gk + 4));
        if (cond < 72) f = 1;
        else if (lo == df) f = 3; else if (lo == fw) f = 4; else if (lo == mf) f = 2; else f = 5;
        in = cond < 75 ? 0 : cond > 90 ? 2 : 1;
        if (f != mgr.trainFocus || in != mgr.trainInt)
            season.news.push_back(std::string("Entraînement : votre adjoint oriente les séances vers « ") + trainFocusName(f) + " » (intensité " + trainIntName(in) + ").");
        mgr.trainFocus = f; mgr.trainInt = in;
    }
    float im = mgr.trainInt == 0 ? 0.6f : mgr.trainInt == 2 ? 1.5f : 1.f;
    float staff = 1.f + 0.08f * (staffLevel(SR_ADJOINT) + staffLevel(SR_PHYSIO_PREP));
    for (auto& p : T.squad) {
        int ov = p.overall();
        float ch = (p.pot > ov ? 0.14f : 0.035f) * im * staff * (p.age <= 23 ? 1.5f : p.age >= 31 ? 0.4f : 1.f);
        if (g_rng.chance(ch)) {
            uint8_t* a[3] = { nullptr, nullptr, nullptr };
            switch (mgr.trainFocus) {
            case 1: a[0] = &p.speed; a[1] = &p.stamina; break;
            case 2: a[0] = &p.pass; a[1] = &p.shoot; break;
            case 3: a[0] = &p.tackle; a[1] = &p.stamina; break;
            case 4: a[0] = &p.shoot; a[1] = &p.speed; break;
            case 5: if (p.pos == POS_GK) a[0] = &p.keep; else a[0] = &p.stamina; break;
            default: { uint8_t* all[5] = { &p.speed, &p.shoot, &p.pass, &p.tackle, &p.stamina }; a[0] = all[g_rng.next() % 5]; if (p.pos == POS_GK) a[1] = &p.keep; }
            }
            int n = a[1] ? 2 : 1;
            uint8_t* v = a[g_rng.next() % n];
            if (v && *v < 99) (*v)++;
            if (p.pos == POS_GK && p.keep < 99 && mgr.trainFocus != 5 && g_rng.chance(0.3f)) p.keep++;
        }
        // blessures à l'entraînement (intensité)
        if (p.injured <= 0 && g_rng.chance((mgr.trainInt == 2 ? 0.02f : mgr.trainInt == 1 ? 0.006f : 0.002f) * (staffLevel(SR_MEDIC) > 0 ? 0.7f : 1.f))) {
            p.injured = (int8_t)(1 + g_rng.next() % 3);
            season.news.push_back(p.name + fmt(" s'est blessé à l'entraînement (%d match%s).", p.injured, p.injured > 1 ? "s" : ""));
        }
        // moral : un entraînement léger détend le groupe, intense le fatigue
        int mo = p.morale + (mgr.trainInt == 0 ? 2 : mgr.trainInt == 2 ? -1 : 0);
        p.morale = (uint8_t)std::max(10, std::min(100, mo));
    }
}

void Career::mgrTick() {
    if (kind != CK_CLUB || userTeam < 0) return;
    int m = monthOf(season.now);
    if (season.finished) m = 11;
    while (mgr.lastMonth < m && mgr.lastMonth < 11) {
        int64_t staffW = 0; for (auto& st : mgr.staff) staffW += st.wage;
        // frais de fonctionnement (administratif, déplacements, équipements, formation) : 18 % des revenus de base
        int64_t inc = (mgr.incomeBase + mgr.sponsorIncome) / 11, wg = (wageBill(userTeam) + staffW + mgr.incomeBase * 18 / 100) / 11;
        mgr.budget += inc - wg;
        mgr.seasonIncome += inc; mgr.seasonWages += wg;
        mgr.lastMonth++;
        stadiumMonth(*this);
        trainingMonth();
        if (mgr.lastMonth == 5) { aiTransfers(40); genOffers(1 + g_rng.range(0, 2)); boardReview(); }     // mercato d'hiver
        if (mgr.lastMonth == 11) genOffers(1 + g_rng.range(0, 2));                                            // mercato d'été
        // les offres non traitées expirent
        mgr.offers.erase(std::remove_if(mgr.offers.begin(), mgr.offers.end(), [&](const TransferOffer& o) { return o.expires < mgr.lastMonth; }), mgr.offers.end());
        if (mgr.lastMonth == 7) {
            // annonces de fin de carrière (effectives à la fin de la saison)
            for (auto& p : g_world.teams[userTeam].squad) {
                if (p.age < 33 || std::find(retiring.begin(), retiring.end(), p.id) != retiring.end()) continue;
                float ch = p.age >= 37 ? 0.9f : p.age >= 35 ? 0.55f : p.age == 34 ? 0.3f : 0.12f;
                if (g_rng.chance(ch)) { retiring.push_back(p.id); season.news.push_back(p.name + fmt(" (%d ans) annonce qu'il prendra sa retraite à la fin de la saison.", p.age)); }
            }
        }
    }
}

// ------------------------------------------------------------------ primes (montants réels, en k EUR)
// Coupe de France (FFF, dotations 2023-24, montants cumulés) : 7e tour 6k, 8e 18k, 32es 43k, 16es 83k,
// 8es 108k, quarts 133k, demies 218k, finale 388k, finaliste 838k, vainqueur 1,238 M.
// Coupe de la Ligue (LFP, dernière édition 2019-20, cumulés) : 1er tour 228k, 2e 276k, 16es 336k, 8es 400k,
// quarts 604k, demies 880k, finale 1,294 M, finaliste 2,298 M, vainqueur 2,872 M.
// Ligue des champions (UEFA 2025-26) : participation 18,62 M, victoire 2,1 M, nul 0,7 M, 8es (2e phase) 11 M,
// quarts 12,5 M, demies 15 M, finale 18,5 M, vainqueur + 6,5 M.
// Coupe UEFA (barème Ligue Europa 2025-26) : participation 4,31 M, victoire 450k, nul 150k, 8es 1,75 M,
// quarts 2,5 M, demies 3,5 M, finale 7 M, vainqueur + 6 M. Supercoupe de l'UEFA : 5 M / 4 M.
static int stageOfMatch(const Competition& C, int mi) {
    for (int s = 0; s < (int)C.stages.size(); s++) for (auto& R : C.stages[s].rounds) for (int m : R.m) if (m == mi) return s;
    return -1;
}
static int userPlayedIn(const Competition& C, int s, int team) {
    int n = 0;
    for (auto& R : C.stages[s].rounds) for (int m : R.m) { const MatchRes& x = C.matches[m]; if (x.played && (x.home == team || x.away == team)) n++; }
    return n;
}
static bool has(const std::string& a, const char* b) { return a.find(b) != std::string::npos; }

void Career::mgrAfterMatch(int comp, int mi) {
    if (kind != CK_CLUB || userTeam < 0) return;
    const Competition& C = season.comps[comp];
    const MatchRes& m = C.matches[mi];
    if (!m.played) return;
    int64_t add = 0;
    std::string why;
    if (m.home == userTeam && !m.neutral) homeMatchDay(comp, mi);   // billetterie, buvette, boutique
    bool win = (m.home == userTeam && (m.hg > m.ag || (m.hg == m.ag && m.ph > m.pa))) || (m.away == userTeam && (m.ag > m.hg || (m.hg == m.ag && m.pa > m.ph)));
    bool draw = m.hg == m.ag && m.ph < 0;
    bool rawWin = (m.home == userTeam && m.hg > m.ag) || (m.away == userTeam && m.ag > m.hg);
    int s = stageOfMatch(C, mi);
    std::string sn = s >= 0 ? C.stages[s].name : "";
    bool entering = s >= 0 && userPlayedIn(C, s, userTeam) == 1;
    bool earlier = false;
    for (int k = 0; k < s; k++) if (userPlayedIn(C, k, userTeam) > 0) earlier = true;
    bool isFinal = sn == "Finale";
    bool isCdf = comp == cdfNational || std::find(cdfRegional.begin(), cdfRegional.end(), comp) != cdfRegional.end();
    auto cumPay = [&](const char* const* names, const int* cum, int n) {
        for (int i = 0; i < n; i++) if (has(sn, names[i])) {
            int64_t v = earlier && i > 0 ? cum[i] - cum[i - 1] : cum[i];
            add += v; why = C.shortName + " (" + sn + ")"; return; }
    };
    if (isCdf && comp == cdfNational) {
        static const char* N[] = { "7e tour", "8e tour", "32es", "16es", "8es", "Quarts", "Demi", "Finale" };
        static const int CUM[] = { 6, 18, 43, 83, 108, 133, 218, 388 };
        if (entering) cumPay(N, CUM, 8);
        if (isFinal) { add += win ? 850 : 450; why = win ? "victoire en Coupe de France" : "finaliste de la Coupe de France"; }
    } else if (isCdf) {
        if (entering && s >= 0 && !has(sn, "préliminaire")) { add += has(sn, "4e") || has(sn, "5e") || has(sn, "6e") ? 2 : 1; why = "Coupe de France (" + sn + ")"; }
    } else if (C.kind == 11) {
        static const char* N[] = { "1er tour", "2e tour", "16es", "8es", "Quarts", "Demi", "Finale" };
        static const int CUM[] = { 228, 276, 336, 400, 604, 880, 1294 };
        if (entering) cumPay(N, CUM, 7);
        if (isFinal) { add += win ? 1578 : 1004; why = win ? "victoire en Coupe de la Ligue" : "finaliste de la Coupe de la Ligue"; }
    } else if (C.kind == 3) {
        // barème 2003-04 : participation 1,625 M ; par match de poule 325 k (+ victoire 325 k, nul 162,5 k) ;
        // qualification 8es 1,625 M, quarts 1,95 M, demies 2,6 M ; finaliste 3,9 M, vainqueur 6,5 M
        if (entering) {
            if (has(sn, "qualification")) { add += has(sn, "1er") ? 100 : has(sn, "2e") ? 150 : 200; why = "C1 (" + sn + ")"; }
            else if (has(sn, "Phase de groupes")) { add += 1625; why = "participation à la Ligue des Champions"; }
            else if (has(sn, "8es")) { add += 1625; why = "C1 : qualification en 8es de finale"; }
            else if (has(sn, "Quarts")) { add += 1950; why = "C1 : qualification en quarts de finale"; }
            else if (has(sn, "Demi")) { add += 2600; why = "C1 : qualification en demi-finales"; }
        }
        if (has(sn, "Phase de groupes")) { add += 325 + (rawWin ? 325 : draw ? 162 : 0); if (why.empty()) why = rawWin ? "C1 : match de poule (victoire)" : draw ? "C1 : match de poule (nul)" : "C1 : match de poule"; }
        if (isFinal) { add += win ? 6500 : 3900; why = win ? "victoire en Ligue des Champions" : "finaliste de la Ligue des Champions"; }
    } else if (C.kind == 8) {
        if (entering) {
            if (has(sn, "qualification")) { add += 250; why = "Coupe UEFA (" + sn + ")"; }
            else if (sn == "1er tour") { add += 4310; why = "participation à la Coupe UEFA"; }
            else if (sn == "2e tour") { add += 500; why = "Coupe UEFA (2e tour)"; }
            else if (sn == "3e tour" || sn == "16es de finale") { add += 750; why = "Coupe UEFA (16es de finale)"; }
            else if (has(sn, "8es")) { add += 1750; why = "Coupe UEFA (8es de finale)"; }
            else if (has(sn, "Quarts")) { add += 2500; why = "Coupe UEFA : quarts de finale"; }
            else if (has(sn, "Demi")) { add += 3500; why = "Coupe UEFA : demi-finales"; }
            else if (isFinal) { add += 7000; why = "Coupe UEFA : finale"; }
        }
        if (!has(sn, "qualification")) add += rawWin ? 450 : draw ? 150 : 0;
        if (isFinal && win) { add += 6000; why = "victoire en Coupe UEFA"; }
    } else if (C.kind == 14) {
        if (entering) { add += 150; why = "Coupe Intertoto (" + sn + ")"; }
    } else if (C.kind == 7) {
        add += win ? 5000 : 4000; why = "Supercoupe de l'UEFA";
    } else if (C.kind == 6) {
        bool frc = C.tag >= 0 && C.tag < (int)pyramids.size() && pyramids[C.tag].country == "FRA";
        add += frc ? (win ? 500 : 250) : win ? std::max<int64_t>(20, mgr.incomeBase / 60) : std::max<int64_t>(10, mgr.incomeBase / 120); why = C.shortName;
    } else if (C.format != FMT_LEAGUE && C.kind != 12) {
        if (win) add += std::max<int64_t>(2, mgr.incomeBase / 60);
        if (C.format == FMT_SINGLE && win) add += std::max<int64_t>(5, mgr.incomeBase / 20);
    }
    if (add > 0 && !why.empty()) season.news.push_back("Prime " + why + " : " + money(add) + ".");
    mgr.budget += add; mgr.seasonIncome += add;
    mgrTick();
}

// droits TV de la Ligue des champions (« market pool ») : enveloppe par pays selon le poids de son marché TV,
// répartie entre les clubs du pays présents en phase de groupes : 50 % selon le classement du championnat précédent,
// 50 % selon le nombre de matchs joués dans la compétition (phase de groupes et au-delà)
int64_t Career::uclMarketPool(std::vector<std::string>& msgs) {
    if (ucl < 0 || ucl >= (int)season.comps.size() || userTeam < 0) return 0;
    const Competition& C = season.comps[ucl];
    auto nat = [](int t) { int n = g_world.teams[t].nation; return n >= 0 ? std::string(NATIONS[n].code) : std::string(); };
    std::string myc = nat(userTeam);
    // clubs du pays en phase de groupes et matchs joués
    std::map<int, int> played;
    for (int s = 3; s < (int)C.stages.size(); s++)
        for (auto& R : C.stages[s].rounds) for (int mi : R.m) {
            const MatchRes& m = C.matches[mi];
            if (!m.played) continue;
            if (nat(m.home) == myc) played[m.home]++;
            if (nat(m.away) == myc) played[m.away]++;
        }
    if (!played.count(userTeam)) return 0;
    static const std::map<std::string, int> POOL = { { "ENG", 60000 }, { "ITA", 45000 }, { "ESP", 40000 }, { "GER", 40000 }, { "FRA", 30000 },
                                                     { "POR", 8000 }, { "NED", 9000 }, { "TUR", 9000 }, { "SCO", 5000 }, { "BEL", 5000 } };
    auto it = POOL.find(myc);
    int64_t pool = it != POOL.end() ? it->second : 3000;
    // part « classement » : 40 / 30 / 20 / 10 % selon l'ordre du championnat précédent (tenant du titre en tête)
    std::vector<int> clubs; for (auto& kv : played) clubs.push_back(kv.first);
    std::stable_sort(clubs.begin(), clubs.end(), [&](int a, int b) { return g_world.teams[a].rating > g_world.teams[b].rating; });
    auto pc = prevChampion.find(myc);
    if (pc != prevChampion.end()) { auto f = std::find(clubs.begin(), clubs.end(), pc->second); if (f != clubs.end()) std::rotate(clubs.begin(), f, f + 1); }
    static const double RANKW[4] = { 0.40, 0.30, 0.20, 0.10 };
    double wsum = 0; for (size_t i = 0; i < clubs.size() && i < 4; i++) wsum += RANKW[i];
    int rk = (int)(std::find(clubs.begin(), clubs.end(), userTeam) - clubs.begin());
    double part1 = rk < 4 ? RANKW[rk] / wsum : 0.05;
    int totM = 0; for (auto& kv : played) totM += kv.second;
    double part2 = totM > 0 ? (double)played[userTeam] / totM : 0;
    int64_t v = (int64_t)(pool * 0.5 * part1 + pool * 0.5 * part2);
    msgs.push_back(fmt("Droits TV de la Ligue des champions (market pool %s : ", myc.c_str()) + money(pool) + fmt(" pour %d club%s) : ", (int)clubs.size(), clubs.size() > 1 ? "s" : "") + money(v) + ".");
    return v;
}

void Career::mgrEndSeason(std::vector<std::string>& msgs) {
    if (kind != CK_CLUB || userTeam < 0) return;
    mgrTick();
    { int64_t tv = uclMarketPool(msgs); mgr.budget += tv; mgr.seasonIncome += tv; }
    int p, q, g;
    if (tierOfTeam(userTeam, &p, &q, &g) < 0) return;
    const Competition& C = season.comps[pyramids[p].pools[q].comps[g]];
    int n = (int)C.result.size(), pos = n;
    for (int i = 0; i < n; i++) if (C.result[i] == userTeam) pos = i + 1;
    // prime de classement : droits TV au mérite (L1 2025-26 : 112,5 M répartis, la moitié selon le classement ;
    // L2 : 29,2 M dont 40 % au classement ; L3 : aide FFF/LFP ; N1 et en dessous : primes fédérales modestes)
    int64_t prize;
    bool frMain = pyramids[p].country == "FRA" && pyramids[p].dom < 0;
    int tier = pyramids[p].pools[q].tier;
    double share = n > 1 ? (double)(n + 1 - pos) / ((double)n * (n + 1) / 2) : 1.0;   // part décroissante 18/171 ... 1/171
    if (frMain && tier == 0) prize = (int64_t)(56250 * share);
    else if (frMain && tier == 1) prize = (int64_t)(11700 * share);
    else if (frMain && tier == 2) prize = (int64_t)(1500 * share);
    else if (frMain && tier == 3) prize = (int64_t)(300 * share);
    else {
        prize = (int64_t)(mgr.incomeBase * 0.30 * (1.0 - (double)(pos - 1) / std::max(1, n - 1)));
        if (pos == 1) prize += mgr.incomeBase / 5;
    }
    if (frMain && tier >= 4 && pos == 1) prize += std::max<int64_t>(2, mgr.incomeBase / 10);
    mgr.budget += prize; mgr.seasonIncome += prize;
    msgs.push_back(fmt("Prime de classement (%de) : ", pos) + money(prize) + ". Budget : " + money(mgr.budget) + ".");
    bool ok = pos <= mgr.objTarget;
    int diff = mgr.objTarget - pos;
    int delta = std::max(-40, std::min(30, diff * 5)) + (ok ? 10 : -10);
    mgr.confidence = std::max(0, std::min(100, mgr.confidence + delta));
    if (ok) msgs.push_back(fmt("Objectif atteint (%de). Le président est satisfait : confiance %d%%.", pos, mgr.confidence));
    else msgs.push_back(fmt("Objectif manqué (%de pour un objectif de top %d). Confiance du président : %d%%.", pos, mgr.objTarget, mgr.confidence));
    if (mgr.budget < -mgr.incomeBase / 2) { mgr.confidence = std::max(0, mgr.confidence - 20); msgs.push_back("Le président s'inquiète des finances du club."); }
    if (mgr.noSack) mgr.confidence = std::max(mgr.confidence, 15);
    else if (mgr.confidence <= 5) { mgr.sacked = 1; msgs.push_back("Le président vous a limogé. Choisissez un nouveau club."); }
    // semi-pro : les bons joueurs restés sous licence amateur peuvent partir
    {
        Team& U = g_world.teams[userTeam];
        if (U.status == CS_SEMIPRO) {
            std::vector<int> ov; for (auto& pl : U.squad) ov.push_back(pl.overall());
            std::sort(ov.rbegin(), ov.rend());
            int bar = ov.size() > 8 ? ov[8] : 99;
            for (int i = (int)U.squad.size() - 1; i >= 0 && U.squad.size() > 16; i--) {
                const Player& pl = U.squad[i];
                if (pl.contract == 2 && pl.overall() >= bar && g_rng.chance(0.35f)) {
                    msgs.push_back(pl.name + " quitte le club : il a trouvé un club qui lui offre un contrat semi-pro.");
                    releasePlayer(pl.id);
                }
            }
        }
    }
    // supporters et bilan du stade
    StadiumInfo& SI = g_world.teams[userTeam].sta;
    if (SI.seasonHomeMatches > 0) msgs.push_back(fmt("Affluence moyenne : %d spectateurs (record : %d). Billetterie et buvette : ", SI.seasonAttTotal / SI.seasonHomeMatches, SI.bestAtt) + money(mgr.seasonGate) + ", boutique : " + money(mgr.seasonShop) + ".");
    double ratio = n > 1 ? (double)(pos - 1) / (n - 1) : 0.5;
    SI.fans = std::max(50, (int)(SI.fans * (1.0 + (0.5 - ratio) * 0.12)));
    SI.seasonAttTotal = 0; SI.seasonHomeMatches = 0;
    mgr.seasonGate = mgr.seasonShop = mgr.seasonStadiumCost = 0;
}

void Career::changeClub(int team) {
    userTeam = team;
    season.controlled = { team };
    mgr = ManagerState();
    mgr.confidence = 50;
    mgrInit();
    mgr.lastMonth = monthOf(season.now);
}

// ------------------------------------------------------------------ transferts
static void removeFromSquad(int team, int idx) {
    Team& t = g_world.teams[team];
    int id = t.squad[idx].id;
    t.squad.erase(t.squad.begin() + idx);
    t.xi.erase(std::remove(t.xi.begin(), t.xi.end(), id), t.xi.end());
    // effectif minimum : un jeune du centre de formation complète
    int gk = 0; for (auto& p : t.squad) if (p.pos == POS_GK) gk++;
    while ((int)t.squad.size() < 16 || gk < 2) {
        int pos = gk < 2 ? POS_GK : (int)(g_rng.next() % 3) + 1;
        t.squad.push_back(g_world.makeYouth(team, pos, t.rating - 5));
        if (pos == POS_GK) gk++;
    }
}

static void addToSquad(int team, Player p, int from = -1) {
    Team& t = g_world.teams[team];
    if (from >= 0) recordPlayerSeason(p, from, g_career.year, true);
    std::vector<bool> used(100, false);
    for (auto& q : t.squad) if (q.num < 100) used[q.num] = true;
    if (p.num == 0 || p.num >= 100 || used[p.num]) { p.num = 1; while (p.num < 99 && used[p.num]) p.num++; }
    p.goals = p.assists = p.apps = 0;
    t.squad.push_back(p);
}

static void logTransfer(Career& K, const Player& p, int from, int to, int fee) {
    TransferRec r; r.pid = p.id; r.from = from; r.to = to; r.fee = fee; r.year = K.year;
    strncpy(r.name, p.name.c_str(), sizeof r.name - 1);
    K.mgr.transfers.push_back(r);
    if (K.mgr.transfers.size() > 3000) K.mgr.transfers.erase(K.mgr.transfers.begin(), K.mgr.transfers.begin() + 500);
}

bool Career::buyPlayer(int pid, int fee, std::string& err) {
    if (!transferWindow()) { err = "Le mercato est fermé."; return false; }
    int idx; int src = g_world.findPlayer(pid, &idx);
    if (src < 0) { err = "Joueur introuvable."; return false; }
    if (src == userTeam) { err = "Ce joueur est déjà au club."; return false; }
    Team& U = g_world.teams[userTeam];
    if (U.squad.size() >= 32) { err = "Effectif complet (32 joueurs maximum)."; return false; }
    if (fee > mgr.budget) { err = "Budget insuffisant."; return false; }
    const Player& P = g_world.teams[src].squad[idx];
    bool ownReserve = g_world.teams[src].parent == userTeam;
    if (!ownReserve && P.overall() > U.rating + 14 && g_world.teams[src].rating > U.rating + 6) { err = P.name + " refuse de rejoindre un club de ce niveau."; return false; }
    if (U.status == CS_AMATEUR && P.contract == 0 && P.overall() > 55 && !ownReserve) { err = P.name + " (contrat pro) refuse de signer une licence amateur."; return false; }
    Player copy = P;
    copy.contract = (uint8_t)(U.status == CS_PRO ? 0 : U.status == CS_SEMIPRO ? 1 : 2);
    removeFromSquad(src, idx);
    addToSquad(userTeam, copy, src);
    mgr.budget -= fee; mgr.seasonTransfers -= fee;
    logTransfer(*this, copy, src, userTeam, fee);
    season.news.push_back("Mercato : " + copy.name + " rejoint " + U.name + " (" + g_world.teams[src].name + (fee > 0 ? ", " + money(fee) : ", libre") + ").");
    return true;
}

bool Career::sellPlayer(int pid, int to, int fee, std::string& err) {
    if (!transferWindow()) { err = "Le mercato est fermé."; return false; }
    int idx; int src = g_world.findPlayer(pid, &idx);
    if (src != userTeam) { err = "Ce joueur n'appartient pas au club."; return false; }
    if (g_world.teams[userTeam].squad.size() <= 16) { err = "Effectif trop réduit (16 joueurs minimum)."; return false; }
    Player copy = g_world.teams[src].squad[idx];
    removeFromSquad(src, idx);
    g_world.ensureSquad(to);
    addToSquad(to, copy, src);
    mgr.budget += fee; mgr.seasonTransfers += fee;
    logTransfer(*this, copy, src, to, fee);
    season.news.push_back("Mercato : " + copy.name + " part pour " + g_world.teams[to].name + " (" + money(fee) + ").");
    return true;
}

void Career::releasePlayer(int pid) {
    int idx; int src = g_world.findPlayer(pid, &idx);
    if (src != userTeam) return;
    std::string nm = g_world.teams[src].squad[idx].name;
    removeFromSquad(src, idx);
    season.news.push_back(nm + " est libéré de son contrat.");
}

int Career::findBuyer(int pid, int& fee) const {
    int idx; int src = g_world.findPlayer(pid, &idx);
    if (src < 0) return -1;
    const Player& P = g_world.teams[src].squad[idx];
    int ov = P.overall();
    std::vector<int> cand;
    for (int t = 0; t < (int)g_world.teams.size(); t++) {
        const Team& T = g_world.teams[t];
        if (t == src || T.kind != TK_CLUB || T.parent >= 0 || T.custom) continue;
        if (T.rating < ov - 10 || T.rating > ov + 12) continue;
        if (T.dbClub < 0 && T.rating > 55) continue;
        if (T.nation < 0 || NATIONS[T.nation].conf != UEFA) continue;
        cand.push_back(t);
    }
    if (cand.empty()) return -1;
    Rng r(g_rng.next());
    int to = cand[r.range(0, (int)cand.size() - 1)];
    fee = (int)(P.value() * r.frange(0.75f, 1.15f));
    if (P.value() < 20) fee = 0;
    return to;
}

// transferts entre clubs de l'IA (clubs professionnels de la base)
void Career::aiTransfers(int n) {
    std::vector<int> pro;
    for (int t = 0; t < (int)g_world.teams.size(); t++)
        if (g_world.teams[t].kind == TK_CLUB && g_world.teams[t].dbClub >= 0 && t != userTeam && g_world.teams[t].parent < 0) pro.push_back(t);
    if (pro.size() < 10) return;
    Rng& r = g_rng;
    int newsN = 0;
    std::vector<std::pair<int, std::string>> big;
    for (int k = 0; k < n; k++) {
        int a = pro[r.range(0, (int)pro.size() - 1)], b = pro[r.range(0, (int)pro.size() - 1)];
        if (a == b) continue;
        Team& A = g_world.teams[a]; Team& B = g_world.teams[b];
        if (B.rating >= A.rating - 1 || B.rating < A.rating - 15) continue;
        g_world.ensureSquad(a); g_world.ensureSquad(b);
        if (A.squad.size() >= 30 || B.squad.size() <= 18) continue;
        // meilleur joueur de B susceptible de renforcer A
        std::vector<int> ovA; for (auto& p : A.squad) ovA.push_back(p.overall());
        std::sort(ovA.rbegin(), ovA.rend());
        int bar = ovA.size() > 13 ? ovA[13] : 0;
        int bi = -1, bv = -1;
        for (int i = 0; i < (int)B.squad.size(); i++) {
            const Player& p = B.squad[i];
            if (p.overall() > bar && p.age <= 30 && p.overall() > bv && r.chance(0.6f)) { bv = p.overall(); bi = i; }
        }
        if (bi < 0) continue;
        Player copy = B.squad[bi];
        int fee = (int)(copy.value() * r.frange(0.9f, 1.4f));
        removeFromSquad(b, bi);
        addToSquad(a, copy, b);
        logTransfer(*this, copy, b, a, fee);
        big.push_back({ fee, "Mercato : " + copy.name + " (" + B.name + " -> " + A.name + ", " + money(fee) + ")" });
        // A se sépare d'un joueur en fin de liste
        if (A.squad.size() > 26) {
            int wi = 0;
            for (int i = 1; i < (int)A.squad.size(); i++) if (A.squad[i].overall() < A.squad[wi].overall()) wi = i;
            A.squad.erase(A.squad.begin() + wi);
        }
    }
    std::sort(big.rbegin(), big.rend());
    for (auto& x : big) { if (newsN++ >= 6) break; season.news.push_back(x.second); }
}
