// ------------------------------------------------------------------ mode manager : sauvegarde auto, mercato, finances, messages
static void autosave() {
    if (!g_careerActive) return;
#ifdef _WIN32
    mkdir("saves");
#else
    mkdir("saves", 0755);
#endif
    if (g_career.save("saves/auto.sav")) {
        FILE* f = fopen("saves/auto.txt", "w");
        if (f) {
            std::string d = g_career.kind == 0 ? g_world.teams[g_career.userTeam].name + fmt(" - saison %d-%02d - %s", g_career.year, (g_career.year + 1) % 100, dateOf(g_career.season.now, g_career.year).c_str())
                          : g_career.kind == CK_CUSTOM ? g_career.custom.name : fmt("%s %d", INTL_NAMES[g_career.intlType], g_career.year);
            fprintf(f, "%s\n", d.c_str()); fclose(f);
        }
    }
}

// ---------------- mercato
struct MkRow { int team; int pid; int idx; };
static int g_mkTab = 0, g_mkPos = -1;
static bool g_mkAfford = false;
static std::vector<MkRow> g_mkList;
static ListW g_mkLW;

static void buildMarket() {
    g_mkList.clear();
    int u = g_career.userTeam;
    int p, q, g;
    int ut = g_career.tierOfTeam(u, &p, &q, &g);
    std::set<int> clubs;
    for (int t = 0; t < (int)g_world.teams.size(); t++) if (g_world.teams[t].kind == TK_CLUB && g_world.teams[t].dbClub >= 0) clubs.insert(t);
    if (ut >= 0) {
        const Pyramid& P = g_career.pyramids[p];
        int reg = g_world.teams[u].region;
        for (auto& pl : P.pools) {
            if (std::abs(pl.tier - ut) > 1) continue;
            for (int t : pl.clubs) if (pl.tier <= 4 || g_world.teams[t].region == reg) clubs.insert(t);
        }
    }
    for (int t = 0; t < (int)g_world.teams.size(); t++) if (g_world.teams[t].parent == u) clubs.insert(t);   // réserves du club
    clubs.erase(u);
    // niveau de l'effectif
    g_world.ensureSquad(u);
    std::vector<int> ovs; for (auto& pl : g_world.teams[u].squad) ovs.push_back(pl.overall());
    std::sort(ovs.rbegin(), ovs.rend());
    int ref = ovs.size() > 10 ? ovs[10] : 40;
    for (int t : clubs) {
        g_world.ensureSquad(t);
        const Team& T = g_world.teams[t];
        for (int i = 0; i < (int)T.squad.size(); i++) {
            const Player& pl = T.squad[i];
            int ov = pl.overall();
            if (ov < ref - 10 || ov > ref + 12 + 2 * g_career.staffLevel(SR_SCOUT)) continue;
            if (g_mkPos >= 0 && pl.pos != g_mkPos) continue;
            g_mkList.push_back({ t, pl.id, i });
        }
    }
    std::sort(g_mkList.begin(), g_mkList.end(), [](const MkRow& a, const MkRow& b) {
        return g_world.teams[a.team].squad[a.idx].overall() > g_world.teams[b.team].squad[b.idx].overall(); });
    if (g_mkList.size() > 600) g_mkList.resize(600);
    g_mkLW = ListW();
}

static int askingPrice(int team, int idx) {
    const Team& T = g_world.teams[team];
    const Player& p = T.squad[idx];
    if (T.parent == g_career.userTeam) return 0;               // joueur de la réserve : gratuit
    auto lu = g_world.pickLineup(team, T.formation);
    bool starter = false; for (int k = 0; k < 11 && k < (int)lu.size(); k++) if (lu[k] == idx) starter = true;
    return (int)(p.value() * (starter ? 1.3 : 1.0));
}

static void openMarket() { g_mkTab = 0; buildMarket(); g_screen = SC_MARKET; }

static void screenMarket() {
    drawBackground("Mercato");
    const ManagerState& M = g_career.mgr;
    Team& U = g_world.teams[g_career.userTeam];
    drawTextPx("Budget : " + money(M.budget), 12, 32, 10, M.budget >= 0 ? C_GOOD : C_BAD);
    drawTextPx(fmt("Effectif : %d joueurs", (int)U.squad.size()), 170, 32, 10, C_TXT);
    drawTextPx(fitText(g_career.windowText(), 290, 10), 330, 32, 10, g_career.transferWindow() ? C_GOOD : C_BAD);
    static const char* TABS[] = { "Acheter", "Vendre / libérer", "Transferts récents" };
    for (int i = 0; i < 3; i++) if (button(12 + i * 130, 46, 126, 14, TABS[i], g_mkTab == i)) { g_mkTab = i; g_mkLW = ListW(); }
    if (IN.pgUp) { g_mkTab = (g_mkTab + 2) % 3; g_mkLW = ListW(); }
    if (IN.pgDn) { g_mkTab = (g_mkTab + 1) % 3; g_mkLW = ListW(); }
    int y = 64;
    if (g_mkTab == 0) {
        static const char* PF[] = { "Tous", "Gardiens", "Défenseurs", "Milieux", "Attaquants" };
        for (int i = 0; i < 5; i++) if (button(12 + i * 76, y, 74, 13, PF[i], g_mkPos == i - 1)) { g_mkPos = i - 1; buildMarket(); }
        if (button(400, y, 130, 13, g_mkAfford ? "Abordables : oui" : "Abordables : non", g_mkAfford)) g_mkAfford = !g_mkAfford;
        y += 17;
        std::vector<int> rows;
        for (int i = 0; i < (int)g_mkList.size(); i++) {
            const MkRow& r = g_mkList[i];
            const Team& T = g_world.teams[r.team];
            if (r.idx >= (int)T.squad.size() || T.squad[r.idx].id != r.pid) continue;
            if (g_mkAfford && askingPrice(r.team, r.idx) > M.budget) continue;
            rows.push_back(i);
        }
        drawTextPx("P   Joueur", 16, y, 10, C_DIM); drawTextPx("Âge", 190, y, 10, C_DIM); drawTextPx("Note", 222, y, 10, C_DIM);
        drawTextPx("Club", 262, y, 10, C_DIM); drawTextPx("Prix", VW - 150, y, 10, C_DIM); drawTextPx("Salaire", VW - 76, y, 10, C_DIM);
        y += 12;
        int sel = listRun(g_mkLW, (int)rows.size(), 12, y, VW - 24, (VH - 18 - y) / 12, 12, [&](int i, int x, int yy, bool s2) {
            const MkRow& r = g_mkList[rows[i]];
            const Team& T = g_world.teams[r.team];
            const Player& p = T.squad[r.idx];
            Color c = s2 ? BLACK : C_TXT;
            int price = askingPrice(r.team, r.idx);
            drawTextPx(posName(p.pos), x + 4, yy + 1, 10, c);
            drawTextPx(fitText(p.name, 150, 10), x + 22, yy + 1, 10, c);
            drawTextPx(fmt("%d", p.age), x + 182, yy + 1, 10, c);
            drawTextPx(fmt("%d", p.overall()), x + 214, yy + 1, 10, s2 ? BLACK : C_HI);
            if (g_career.staffLevel(SR_SCOUT) >= 3 && p.pot > p.overall() + 3) drawTextPx(fmt("+%d", p.pot - p.overall()), x + 230, yy - 3, 10, C_GOOD);
            drawKitIcon(T.home, x + 240, yy + 1, 1);
            drawTextPx(fitText(nameLvl(r.team), VW - 420, 10), x + 252, yy + 1, 10, s2 ? BLACK : C_DIM);
            drawTextPx(price ? money(price) : "gratuit", x + VW - 162, yy + 1, 10, price <= M.budget ? c : C_BAD);
            drawTextPx(money(p.wage()), x + VW - 88, yy + 1, 10, c);
        });
        if (rows.empty()) drawTextCentered("Aucun joueur ne correspond", VW / 2, y + 20, 10, C_DIM);
        drawFooter("OK : faire une offre   LB/RB : onglet   Retour");
        if (sel >= 0) {
            MkRow r = g_mkList[rows[sel]];
            const Player& p = g_world.teams[r.team].squad[r.idx];
            int price = askingPrice(r.team, r.idx);
            askConfirm(fmt("Recruter %s (%s, %d ans, note %d) pour %s ? Salaire : %s / an", p.name.c_str(), posName(p.pos), p.age, p.overall(),
                           price ? money(price).c_str() : "0", money(p.wage()).c_str()),
                       [r, price]() {
                           std::string err;
                           if (g_career.buyPlayer(r.pid, price, err)) { toast("Transfert réalisé !"); buildMarket(); }
                           else toast(err);
                       }, "Faire l'offre", "Annuler");
        }
    } else if (g_mkTab == 1) {
        drawTextPx("P   Joueur", 16, y, 10, C_DIM); drawTextPx("Âge", 190, y, 10, C_DIM); drawTextPx("Note", 222, y, 10, C_DIM);
        drawTextPx("Buts", 262, y, 10, C_DIM); drawTextPx("Valeur", VW - 200, y, 10, C_DIM); drawTextPx("Salaire", VW - 110, y, 10, C_DIM);
        y += 12;
        int sel = listRun(g_mkLW, (int)U.squad.size(), 12, y, VW - 24, (VH - 36 - y) / 12, 12, [&](int i, int x, int yy, bool s2) {
            const Player& p = U.squad[i];
            Color c = s2 ? BLACK : C_TXT;
            drawTextPx(posName(p.pos), x + 4, yy + 1, 10, c);
            drawTextPx(fitText(p.name, 150, 10), x + 22, yy + 1, 10, c);
            drawTextPx(fmt("%d", p.age), x + 182, yy + 1, 10, c);
            drawTextPx(fmt("%d", p.overall()), x + 214, yy + 1, 10, s2 ? BLACK : C_HI);
            drawTextPx(fmt("%d", p.goals), x + 254, yy + 1, 10, c);
            drawTextPx(money(p.value()), x + VW - 212, yy + 1, 10, c);
            drawTextPx(money(p.wage()), x + VW - 122, yy + 1, 10, c);
        });
        int cur = g_mkLW.cur;
        if (cur >= 0 && cur < (int)U.squad.size() && button(VW - 200, VH - 32, 188, 14, "Libérer le joueur sélectionné")) {
            int pid = U.squad[cur].id; std::string nm = U.squad[cur].name;
            if (U.squad.size() <= 16) toast("Effectif trop réduit (16 joueurs minimum)");
            else askConfirm("Libérer " + nm + " de son contrat (sans indemnité) ?", [pid]() { g_career.releasePlayer(pid); toast("Joueur libéré"); });
        }
        drawFooter("OK : chercher un acheteur   LB/RB : onglet   Retour");
        if (sel >= 0) {
            int pid = U.squad[sel].id;
            std::string nm = U.squad[sel].name;
            int fee = 0;
            int to = g_career.findBuyer(pid, fee);
            if (to < 0) toast("Aucun club intéressé pour l'instant");
            else askConfirm(g_world.teams[to].name + " (" + shortLevel(to) + ") propose " + (fee ? money(fee) : std::string("un transfert libre")) + " pour " + nm + ". Accepter ?",
                            [pid, to, fee]() { std::string err; if (g_career.sellPlayer(pid, to, fee, err)) toast("Joueur vendu"); else toast(err); }, "Accepter", "Refuser");
        }
    } else {
        auto& tr = g_career.mgr.transfers;
        drawTextPx("Saison  Joueur", 16, y, 10, C_DIM); drawTextPx("Transfert", 230, y, 10, C_DIM); drawTextPx("Montant", VW - 80, y, 10, C_DIM);
        y += 12;
        listRun(g_mkLW, (int)tr.size(), 12, y, VW - 24, (VH - 18 - y) / 12, 12, [&](int i, int x, int yy, bool s2) {
            const TransferRec& r = tr[tr.size() - 1 - i];
            bool mine = r.from == g_career.userTeam || r.to == g_career.userTeam;
            Color c = s2 ? BLACK : mine ? C_HI : C_TXT;
            drawTextPx(fmt("%d", r.year), x + 4, yy + 1, 10, c);
            drawTextPx(fitText(r.name, 160, 10), x + 50, yy + 1, 10, c);
            std::string mv = (r.from >= 0 ? g_world.teams[r.from].shortName : std::string("?")) + " -> " + (r.to >= 0 ? g_world.teams[r.to].name : std::string("?"));
            drawTextPx(fitText(mv, VW - 330, 10), x + 218, yy + 1, 10, s2 ? BLACK : C_DIM);
            drawTextPx(r.fee ? money(r.fee) : "libre", x + VW - 92, yy + 1, 10, c);
        });
        if (tr.empty()) drawTextCentered("Aucun transfert pour l'instant", VW / 2, y + 20, 10, C_DIM);
        drawFooter("LB/RB : onglet   Retour");
    }
    if (IN.back) g_screen = SC_CLUBMENU;
}

// ---------------- finances et objectifs
static void screenFinance() {
    drawBackground("Finances et objectifs");
    const ManagerState& M = g_career.mgr;
    const Team& U = g_world.teams[g_career.userTeam];
    int y = 40;
    auto line = [&](const std::string& k, const std::string& v, Color c = C_TXT) {
        drawTextPx(k, 30, y, 10, C_DIM); drawTextPx(v, 330, y, 10, c); y += 14;
    };
    drawSection(20, y, VW - 40, "FINANCES"); y += 14;
    line("Budget disponible", money(M.budget), M.budget >= 0 ? C_GOOD : C_BAD);
    line("Revenus annuels (TV, sponsors, subventions)", money(M.incomeBase));
    line("Masse salariale annuelle", money(g_career.wageBill(g_career.userTeam)));
    line("Revenus encaissés cette saison", money(M.seasonIncome), C_GOOD);
    line("Salaires et frais de fonctionnement versés", money(-M.seasonWages), C_BAD);
    line("Bilan des transferts cette saison", money(M.seasonTransfers), M.seasonTransfers >= 0 ? C_GOOD : C_BAD);
    int64_t val = 0; for (auto& p : U.squad) val += p.value();
    line("Valeur de l'effectif", money(val));
    line("Stade", U.stadium.empty() ? "-" : U.stadium);
    y += 6;
    drawSection(20, y, VW - 40, "PRÉSIDENT"); y += 14;
    line("Objectif de la saison", g_career.objectiveText(), C_HI);
    line("Confiance du président", fmt("%d %%", M.confidence), M.confidence >= 50 ? C_GOOD : M.confidence >= 25 ? C_HI : C_BAD);
    DrawRectangle(330, y - 2, 200, 6, Color{ 0, 0, 0, 120 });
    DrawRectangle(330, y - 2, 2 * M.confidence, 6, M.confidence >= 50 ? C_GOOD : M.confidence >= 25 ? C_HI : C_BAD);
    y += 10;
    line("Mercato", g_career.windowText(), g_career.transferWindow() ? C_GOOD : C_DIM);
    y += 4;
    const char* tips[] = { "Recettes : chaque match à domicile rapporte une recette, les victoires en coupe et en Europe des primes.",
                           "Salaires et frais prélevés chaque mois. Primes réelles : droits TV au classement, Coupe de France, Coupe de la Ligue, UEFA.",
                           "Objectif manqué ou finances dans le rouge : la confiance baisse. À 5 % ou moins, c'est le licenciement." };
    for (auto t : tips) { drawTextPx(t, 30, y, 10, C_DIM); y += 12; }
    // demandes au président (2 par saison)
    ManagerState& MM = g_career.mgr;
    y += 4;
    int left = std::max(0, 2 - MM.boardRequests);
    drawTextPx(fmt("Demandes au président (%d restante%s cette saison) :", left, left > 1 ? "s" : ""), 30, y + 3, 10, C_HI);
    bool b1 = button(300, y, 160, 15, "Rallonge budgétaire", false, left > 0);
    bool b2 = button(466, y, 150, 15, "Demander du temps", false, left > 0);
    if (b1 && left > 0) {
        MM.boardRequests++;
        if (MM.confidence >= 55 && g_rng.chance(0.4f + (MM.confidence - 55) / 100.f)) {
            int64_t add = std::max<int64_t>(10, MM.incomeBase / 10);
            MM.budget += add; MM.confidence = std::max(0, MM.confidence - 8);
            toast("Accordé : +" + money(add) + " (confiance -8 %)");
            g_career.season.news.push_back("Le président accepte une rallonge de " + money(add) + " pour renforcer l'équipe.");
        } else { MM.confidence = std::max(0, MM.confidence - 4); toast("Refusé par le président (confiance -4 %)"); g_career.season.news.push_back("Le président refuse d'augmenter le budget : « Gagnez d'abord des matchs ! »"); }
    }
    if (b2 && left > 0) {
        MM.boardRequests++;
        if (MM.confidence < 45) { MM.confidence = std::min(100, MM.confidence + 10); toast("Le président vous renouvelle sa confiance (+10 %)"); g_career.season.news.push_back("Le président vous accorde du temps : « Je crois en votre projet. »"); }
        else toast("Le président n'a aucune inquiétude pour l'instant");
    }
    drawFooter("Retour");
    if (IN.back) g_screen = SC_CLUBMENU;
}

// ---------------- messages
static ListW g_newsLW;
// ---------------- licenciement : offres d'emploi
static std::vector<int> g_jobs;
static ListW g_jobsLW;
static void screenJobs() {
    if (g_jobs.empty()) {
        int u = g_career.userTeam;
        float r = g_world.teams[u].rating;
        int p, q, g; int ut = g_career.tierOfTeam(u, &p, &q, &g);
        std::vector<int> cand;
        if (ut >= 0) for (auto& pl : g_career.pyramids[p].pools) if (std::abs(pl.tier - ut) <= 1)
            for (int t : pl.clubs) if (t != u && g_world.teams[t].parent < 0 && std::fabs(g_world.teams[t].rating - r) < 8) cand.push_back(t);
        g_rng.shuffle(cand);
        if (cand.size() > 6) cand.resize(6);
        g_jobs = cand;
    }
    drawBackground("Vous êtes licencié");
    drawTextCentered("Le président a mis fin à votre contrat. Des clubs vous proposent de prendre leur équipe :", VW / 2, 40, 10, C_TXT);
    int s = listRun(g_jobsLW, (int)g_jobs.size() + 1, 60, 60, VW - 120, 8, 16, [&](int i, int x, int y, bool sel) {
        if (i == (int)g_jobs.size()) { drawTextCentered("Quitter la carrière", x + (VW - 120) / 2, y + 3, 10, sel ? BLACK : C_TXT, false); return; }
        int t = g_jobs[i];
        drawKitIcon(g_world.teams[t].home, x + 4, y + 3, 1);
        drawTextPx(fitText(g_world.teams[t].name, 260, 10), x + 18, y + 3, 10, sel ? BLACK : C_TXT);
        drawTextPx(fitText(g_career.teamLevelName(t), 220, 10), x + 290, y + 3, 10, sel ? BLACK : C_DIM);
    });
    g_noBackBtn = false;
    drawFooter("OK : accepter l'offre");
    if (s < 0) return;
    if (s == (int)g_jobs.size()) { g_jobs.clear(); g_careerActive = false; g_screen = SC_MAIN; return; }
    int t = g_jobs[s];
    g_jobs.clear();
    g_career.changeClub(t);
    g_hubNotice = "Nouveau défi : " + g_world.teams[t].name + " !";
    levelCacheDirty();
    openHub();
}
