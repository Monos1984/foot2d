// ------------------------------------------------------------------ nouvelle carrière (options), statut du club, stade, clubs créés
static int g_newTeam = -1;
static Screen g_newBack = SC_MAIN;
static bool g_optNoSack = false;
static int g_optMode = 2;           // 0 jeu (matchs seulement), 1 manager (temps forts), 2 manager + jeu
static int g_optBudget = 1;          // 0 serré, 1 normal, 2 confortable
static ListW g_coptLW;

static void startCareerWith(int team, Screen back) {
    g_newTeam = team; g_newBack = back; g_coptLW = ListW(); g_coptLW.cur = 9;
    g_screen = SC_CAREEROPT;
}

static Career::Opts g_optRules;
static void screenCareerOpt() {
    drawBackground("Nouvelle carrière");
    const Team& T = g_world.teams[g_newTeam];
    drawKitIcon(T.home, VW / 2 - 12, 32, 3);
    drawTextCentered(T.name, VW / 2, 58, 20, C_HI);
    drawTextCentered(divisionOf(g_newTeam), VW / 2, 78, 10, C_DIM);
    static const char* BUD[] = { "Serré (budget divisé par 2)", "Normal", "Confortable (budget x 2)" };
    Career::Opts& O = g_optRules;
    std::vector<std::string> items = {
        std::string("Mode de jeu : ") + (g_optMode == 0 ? "JEU (vous jouez les matchs, pas de gestion)" : g_optMode == 1 ? "MANAGER (gestion, matchs en temps forts)" : "MANAGER + JEU (gestion et vous jouez les matchs)"),
        std::string("Licenciement par le président : ") + (g_optNoSack ? "NON (vous ne pouvez pas être renvoyé)" : "OUI"),
        std::string("Budget de départ : ") + BUD[g_optBudget],
        "Manager : " + managerChoiceName(),
        std::string("Coupe de la Ligue : ") + (O.cdl ? "OUI" : "NON (sa place européenne revient au championnat)"),
        std::string("Buts à l'extérieur en coupes d'Europe : ") + (O.awayGoals ? "OUI (comptent double à égalité)" : "NON"),
        std::string("Prolongation en Coupe de France : ") + (O.cdfET ? "OUI" : "NON (tirs au but directs, sauf en finale)"),
        std::string("Coupes d'Europe : ") + (O.euroFormat ? "NOUVELLE FORMULE (C1, C3, C4 : phase de ligue à 36)" : "FORMULE 2003 (C1 à groupes, Coupe UEFA, Intertoto)"),
        "Choisir un autre club",
        ">>> COMMENCER LA CARRIÈRE <<<" };
    const int NI = (int)items.size();
    int wd = 480, x0 = VW / 2 - wd / 2, y0 = 94;
    int s = listRun(g_coptLW, NI, x0, y0, wd, NI, 15, [&](int i, int xx, int yy, bool sel) {
        Color c = sel ? Color{ 20, 20, 40, 255 } : (i >= 4 && i <= 7 ? Color{ 170, 210, 255, 255 } : C_TXT);
        drawTextCentered(items[i], xx + wd / 2, yy + 3, 10, c, false);
    });
    int y = y0 + 15 * NI + 8;
    static const char* HELP[10][2] = {
        { "JEU : vous jouez vos matchs sans gestion. MANAGER : gestion complète, vous regardez les temps forts.", "MANAGER + JEU : gestion complète et vous jouez vos matchs. (Modifiable en cours de carrière.)" },
        { "Le président fixe un objectif chaque saison. Si sa confiance tombe à 5 % ou moins, vous êtes licencié", "(sauf si le licenciement est désactivé)." },
        { "Budget de départ du club (droits TV, sponsors et recettes viennent ensuite).", "" },
        { "Nom, nationalité et apparence du manager.", "" },
        { "Coupe de la Ligue (L1, L2, L3). Désactivée : pas de Coupe de la Ligue, la place en Coupe d'Europe", "de son vainqueur revient au club suivant du championnat." },
        { "Règle des buts marqués à l'extérieur dans les matchs aller-retour européens (supprimée par l'UEFA en 2021).", "Désactivée : à égalité sur les deux matchs, prolongation puis tirs au but." },
        { "Coupe de France : prolongation (2 x 15 min) avant les tirs au but.", "Désactivée : tirs au but dès la fin du temps réglementaire, la finale garde sa prolongation." },
        { "Formule 2003 : 3 tours de qualification, C1 à 8 groupes de 4, Coupe UEFA à élimination directe, Intertoto.", "Nouvelle formule (2024) : C1, Ligue Europa et Ligue Conférence, phase de ligue à 36 et barrages." },
        { "Revenir au choix du club.", "" },
        { "Statut du club : PRO en L1/L2/L3, SEMI-PRO en National, au choix en Régional, AMATEUR en Départemental.", "" } };
    int hc = std::max(0, std::min(NI - 1, g_coptLW.cur));
    drawTextCentered(HELP[hc][0], VW / 2, y, 10, C_DIM, false);
    if (HELP[hc][1][0]) drawTextCentered(HELP[hc][1], VW / 2, y + 12, 10, C_DIM, false);
    drawFooter("Gauche/Droite : modifier   OK : valider   Retour");
    int d = IN.left ? -1 : IN.right ? 1 : 0;
    if (d || (s >= 0 && s < 8)) {
        if (!d) d = 1;
        switch (g_coptLW.cur) {
        case 0: g_optMode = (g_optMode + d + 3) % 3; break;
        case 1: g_optNoSack = !g_optNoSack; break;
        case 2: g_optBudget = (g_optBudget + d + 3) % 3; break;
        case 3: managerCycle(d); break;
        case 4: O.cdl = !O.cdl; break;
        case 5: O.awayGoals = !O.awayGoals; break;
        case 6: O.cdfET = !O.cdfET; break;
        case 7: O.euroFormat = !O.euroFormat; if (O.euroFormat) O.awayGoals = 0; break;
        }
    }
    if (IN.back || s == 8) { g_screen = g_newBack; return; }
    if (s == 9) {
        applyChosenManager();
        g_career.opts = O;
        g_career.newClubCareer(g_newTeam, 2026);
        g_career.mgr.noSack = g_optNoSack || g_optMode == 0;
        g_career.mgr.managerMode = g_optMode != 0;
        g_career.mgr.fullManager = g_optMode == 1;
        if (g_optBudget == 0) g_career.mgr.budget /= 2;
        if (g_optBudget == 2) g_career.mgr.budget *= 2;
        g_careerActive = true; g_needAdvance = true;
        g_hubNotice = "Bienvenue ! Saison 2026-27 : " + g_career.teamLevelName(g_newTeam);
        levelCacheDirty();
        openHub();
        autosave();
        showSeasonArticle();
    }
}

// ---------------- statut du club (divisions régionales)
static void screenStatus() {
    drawBackground("Statut du club");
    const Team& T = g_world.teams[g_career.userTeam];
    drawTextCentered(T.name + " - " + g_career.teamLevelName(g_career.userTeam), VW / 2, 36, 10, C_HI);
    drawTextCentered("En division régionale, le club choisit son statut pour la saison :", VW / 2, 52, 10, C_TXT);
    std::vector<std::string> items = { "SEMI-PRO", "AMATEUR" };
    int s = menuRun(g_statusLW, items, 72, 300);
    int y = 124;
    const char* semi[] = { "SEMI-PRO : contrats semi-professionnels (salaire mensuel ~35 % d'un contrat pro)",
                           "pour les meilleurs joueurs, les autres restent sous licence amateur.",
                           "Permet de recruter et de garder des joueurs de meilleur niveau. Statut obligatoire en National." };
    const char* ama[] = { "AMATEUR : tous les joueurs sous licence amateur, aucun salaire à verser.",
                          "Les joueurs sous contrat pro de bon niveau refusent de signer. Obligatoire en Départemental." };
    for (auto l : semi) { drawTextCentered(l, VW / 2, y, 10, g_statusLW.cur == 0 ? C_TXT : C_DIM, false); y += 12; }
    y += 6;
    for (auto l : ama) { drawTextCentered(l, VW / 2, y, 10, g_statusLW.cur == 1 ? C_TXT : C_DIM, false); y += 12; }
    y += 10;
    drawTextCentered(fmt("Statut actuel : %s   -   masse salariale : %s / an", statusName(T.status), money(g_career.wageBill(g_career.userTeam)).c_str()), VW / 2, y, 10, C_HI);
    drawFooter("OK : choisir");
    if (s >= 0) {
        g_career.setUserStatus(s == 0 ? CS_SEMIPRO : CS_AMATEUR);
        toast(fmt("Statut %s choisi pour la saison", statusName(s == 0 ? CS_SEMIPRO : CS_AMATEUR)));
        openHub();
    }
    if (IN.back) { g_career.mgr.needStatus = false; openHub(); }
}

// ---------------- stade
static int g_stSel = 0;
static ListW g_stLW;
static const char* STAND_NAMES[4] = { "Tribune principale", "Tribune face", "Virage nord", "Virage sud" };
static const char* STAND_KIND[3] = { "places debout", "gradins (sièges)", "tribune couverte" };

static void drawStadiumPlan(const StadiumInfo& S, int x, int y, int w, int h) {
    // vue de dessus : pelouse + 4 tribunes proportionnelles
    int cap = std::max(1, S.capacity());
    DrawRectangle(x, y, w, h, Color{ 20, 26, 40, 255 });
    int px = x + w / 4, py = y + h / 4, pw = w / 2, ph = h / 2;
    for (int i = 0; i < 6; i++) DrawRectangle(px, py + i * ph / 6, pw, ph / 12 + 1, Color{ 44, 120, 44, 255 });
    for (int i = 0; i < 6; i++) DrawRectangle(px, py + i * ph / 6 + ph / 12, pw, ph / 12 + 1, Color{ 38, 108, 38, 255 });
    DrawRectangleLines(px + 2, py + 2, pw - 4, ph - 4, Color{ 240, 240, 240, 180 });
    DrawLine(px + pw / 2, py + 2, px + pw / 2, py + ph - 2, Color{ 240, 240, 240, 180 });
    for (int i = 0; i < 4; i++) {
        const Stand& st = S.s[i];
        int depth = std::max(3, (int)(std::sqrt((float)(st.seats + st.vip) / cap) * (w / 4 - 4)));
        Color c = st.kind == STK_COVERED ? Color{ 150, 160, 190, 255 } : st.kind == STK_SEATS ? Color{ 120, 130, 160, 255 } : Color{ 110, 100, 90, 255 };
        Rectangle r;
        if (i == 0) r = { (float)px, (float)(py + ph + 2), (float)pw, (float)std::min(depth, h / 4 - 3) };
        else if (i == 1) r = { (float)px, (float)(py - 2 - std::min(depth, h / 4 - 3)), (float)pw, (float)std::min(depth, h / 4 - 3) };
        else if (i == 2) r = { (float)(px - 2 - depth), (float)py, (float)depth, (float)ph };
        else r = { (float)(px + pw + 2), (float)py, (float)depth, (float)ph };
        DrawRectangleRec(r, c);
        if (st.kind == STK_COVERED) DrawRectangleLinesEx(r, 1, Color{ 230, 230, 240, 255 });
        if (i == g_stSel && g_stTab == 0) DrawRectangleLinesEx(r, 2, C_SEL);
        if (st.vip > 0) DrawRectangle((int)r.x + (int)r.width / 2 - 6, (int)r.y + 1, 12, 3, Color{ 240, 200, 60, 255 });
    }
    // projecteurs
    for (int k = 0; k < std::min(4, (int)S.lights); k++) {
        int lx = k % 2 ? x + w - 8 : x + 4, ly = k < 2 ? y + 4 : y + h - 8;
        DrawRectangle(lx, ly, 4, 4, Color{ 255, 250, 200, 255 });
    }
}

// icônes des produits dérivés (couleurs du club)
static void drawMerchIcon(int k, int x, int y, const Kit& kit) {
    Color a = hexc(kit.shirt), b = hexc(kit.shirt2 != kit.shirt ? kit.shirt2 : (((((kit.shirt >> 16) & 255) * 0.3f + ((kit.shirt >> 8) & 255) * 0.59f + (kit.shirt & 255) * 0.11f) / 255.f) > 0.55f ? 0x202020 : 0xF2F2F2));
    switch (k) {
    case -1: drawKitIcon(kit, x + 2, y + 1, 1); break;
    case 0: for (int i = 0; i < 6; i++) DrawRectangle(x + i * 3, y + 5 + (i % 2), 3, 6, i % 2 ? b : a); DrawRectangle(x - 1, y + 11, 2, 4, b); DrawRectangle(x + 17, y + 11, 2, 4, b); break;   // écharpe
    case 1: DrawRectangle(x + 3, y + 5, 12, 7, a); DrawRectangle(x + 3, y + 10, 12, 3, b); DrawRectangle(x + 8, y + 3, 3, 2, b); break;                                          // bonnet
    case 2: DrawCircle(x + 9, y + 8, 7, RAYWHITE); DrawCircle(x + 9, y + 8, 3, a); DrawCircle(x + 4, y + 5, 1.5f, BLACK); DrawCircle(x + 14, y + 11, 1.5f, BLACK); break;       // ballon
    case 3: drawKitIcon(kit, x + 4, y + 3, 1); DrawRectangle(x + 1, y + 13, 4, 2, C_HI); break;                                                                                 // maillot enfant
    case 4: DrawRectangle(x + 3, y + 4, 10, 11, RAYWHITE); DrawRectangle(x + 3, y + 7, 10, 4, a); DrawRectangleLines(x + 13, y + 6, 4, 6, RAYWHITE); break;                     // mug
    case 5: DrawRectangle(x + 3, y + 2, 12, 15, Color{ 230, 225, 210, 255 }); DrawRectangle(x + 5, y + 4, 8, 7, a); DrawRectangle(x + 5, y + 12, 8, 1, b); DrawRectangle(x + 5, y + 14, 6, 1, b); break;   // poster
    case 6: DrawRectangle(x + 4, y + 2, 10, 8, a); DrawRectangle(x + 1, y + 3, 3, 6, a); DrawRectangle(x + 14, y + 3, 3, 6, a); DrawRectangle(x + 4, y + 10, 4, 7, b); DrawRectangle(x + 10, y + 10, 4, 7, b); DrawRectangle(x + 8, y + 2, 2, 8, b); break;  // survêtement
    case 7: DrawCircle(x + 9, y + 11, 5, a); DrawCircle(x + 9, y + 5, 4, a); DrawCircle(x + 6, y + 2, 2, b); DrawCircle(x + 12, y + 2, 2, b); DrawRectangle(x + 7, y + 4, 1, 1, BLACK); DrawRectangle(x + 11, y + 4, 1, 1, BLACK); break;   // peluche
    }
}

static void screenStadium() {
    Team& T = g_world.teams[g_career.userTeam];
    StadiumInfo& S = T.sta;
    if (!S.init) initStadium(T, g_career.tierOfTeam(g_career.userTeam), -1);
    drawBackground(T.stadium.empty() ? "Stade" : T.stadium);
    ManagerState& M = g_career.mgr;
    int cap = S.capacity();
    drawTextPx(fmt("Capacité : %d places   Supporters : %d   Dernière affluence : %d (%d%%)", cap, S.fans, S.lastAtt, cap ? S.lastAtt * 100 / cap : 0), 12, 31, 10, C_TXT);
    drawTextPx("Budget : " + money(M.budget), VW - 170, 31, 10, M.budget >= 0 ? C_GOOD : C_BAD);
    static const char* TABS[] = { "Tribunes et billets", "Équipements", "Boutique", "Sponsors et club", "Travaux en cours" };
    for (int i = 0; i < 5; i++) if (button(12 + i * 124, 44, 120, 14, TABS[i], g_stTab == i)) { g_stTab = i; g_stLW = ListW(); }
    if (IN.pgUp) g_stTab = (g_stTab + 4) % 5;
    if (IN.pgDn) g_stTab = (g_stTab + 1) % 5;
    { static int prevTab = -1; if (prevTab != g_stTab) { g_stSel = 0; prevTab = g_stTab; } }
    int y = 62;
    auto launch = [&](int kind, int stand, int amount) {
        int64_t cost = Career::projectCost(T, kind, stand, amount);
        int months = Career::projectMonths(kind, amount);
        askConfirm(fmt("%s : coût %s, durée %d mois. Lancer les travaux ?", projectName(kind), money(cost).c_str(), months), [kind, stand, amount]() {
            Project p; p.kind = kind; p.stand = stand; p.amount = amount;
            std::string err;
            if (g_career.startProject(p, err)) toast("Travaux lancés"); else toast(err);
        }, "Lancer", "Annuler");
    };
    if (g_stTab == 0) {
        if (g_stSel < 0 || g_stSel > 3) g_stSel = 0;
        drawStadiumPlan(S, 12, y, 200, 150);
        if (IN.up) g_stSel = (g_stSel + 3) % 4;
        if (IN.down) g_stSel = (g_stSel + 1) % 4;
        int x = 222;
        for (int i = 0; i < 4; i++) {
            const Stand& st = S.s[i];
            bool sel = i == g_stSel;
            int yy = y + i * 38;
            bool hover = IN.mouse.x >= x && IN.mouse.x < VW - 10 && IN.mouse.y >= yy && IN.mouse.y < yy + 36;
            if (hover && IN.click) g_stSel = i;
            DrawRectangle(x, yy, VW - 10 - x, 36, sel ? Color{ 90, 80, 30, 255 } : C_ITEM);
            drawTextPx(STAND_NAMES[i], x + 6, yy + 2, 10, sel ? C_HI : C_TXT);
            drawTextPx(fmt("%d places (%s)", st.seats, STAND_KIND[st.kind]) + (st.vip ? fmt(" + %d VIP", st.vip) : ""), x + 150, yy + 2, 10, C_TXT);
            drawTextPx(fmt("Prix : %d EUR", st.price) + (st.vip ? fmt("   VIP : %d EUR", st.vipPrice) : ""), x + 6, yy + 14, 10, C_DIM);
            drawTextPx(fmt("Dernier match : %d   Prévision : %d", st.lastAtt, g_career.expectedAttendance(i, false) + g_career.expectedAttendance(i, true)), x + 180, yy + 14, 10, C_DIM);
            float fill = st.seats + st.vip ? (float)st.lastAtt / (st.seats + st.vip) : 0;
            DrawRectangle(x + 6, yy + 27, VW - 30 - x, 4, Color{ 0, 0, 0, 120 });
            DrawRectangle(x + 6, yy + 27, (int)((VW - 30 - x) * std::min(1.f, fill)), 4, fill > 0.9f ? C_GOOD : fill > 0.5f ? C_HI : C_BAD);
        }
        Stand& st = S.s[g_stSel];
        int by = y + 156;
        drawTextPx(std::string("Actions sur : ") + STAND_NAMES[g_stSel], 12, by, 10, C_HI);
        by += 13;
        if (button(12, by, 70, 14, "Prix -1")) st.price = (int16_t)std::max(1, st.price - 1);
        if (button(86, by, 70, 14, "Prix +1")) st.price = (int16_t)std::min(500, st.price + 1);
        if (button(160, by, 90, 14, "VIP -5 EUR", false, st.vip > 0)) st.vipPrice = (int16_t)std::max(5, st.vipPrice - 5);
        if (button(254, by, 90, 14, "VIP +5 EUR", false, st.vip > 0)) st.vipPrice = (int16_t)std::min(2000, st.vipPrice + 5);
        if (IN.left) st.price = (int16_t)std::max(1, st.price - 1);
        if (IN.right) st.price = (int16_t)std::min(500, st.price + 1);
        by += 18;
        if (button(12, by, 150, 14, "+500 places " + money(Career::projectCost(T, PJ_SEATS, g_stSel, 500)))) launch(PJ_SEATS, g_stSel, 500);
        if (button(166, by, 150, 14, "+2000 places " + money(Career::projectCost(T, PJ_SEATS, g_stSel, 2000)))) launch(PJ_SEATS, g_stSel, 2000);
        if (st.kind == STK_STANDING) { if (button(320, by, 150, 14, "Installer des sièges")) launch(PJ_UPGRADE_SEATS, g_stSel, 0); }
        else if (st.kind == STK_SEATS) { if (button(320, by, 150, 14, "Couvrir la tribune")) launch(PJ_COVER, g_stSel, 0); }
        if (button(474, by, 150, 14, "+50 places VIP " + money(Career::projectCost(T, PJ_VIP, g_stSel, 50)))) launch(PJ_VIP, g_stSel, 50);
        drawFooter("Haut/Bas : tribune   Gauche/Droite : prix du billet   LB/RB : onglet   Retour");
    } else if (g_stTab == 1) {
        struct Eq { const char* name; int kind; int level; int maxL; const char* desc; };
        Eq eqs[] = {
            { "Buvette", PJ_BUVETTE, S.buvette, 5, "Recette par spectateur (boissons, restauration)" },
            { "Boutique", PJ_BOUTIQUE, S.boutique, 5, "Vente de maillots et produits du club" },
            { "Parking", PJ_PARKING, S.parking, 4, "Recettes de stationnement, affluence un peu plus forte" },
            { "Éclairage", PJ_LIGHTS, S.lights, 4, "Matchs en nocturne : meilleure affluence" },
            { "Pelouse", PJ_PITCH, S.pitch, 2, "0 naturelle, 1 hybride, 2 synthétique : attire du public" },
            { "Écran géant", PJ_SCREEN, S.screen, 1, "Animation du stade : affluence" },
            { "Vestiaires", PJ_VESTIAIRES, S.vestiaires, 5, "Confort des joueurs et image du club" },
            { "Musée du club", PJ_MUSEUM, S.museum, 1, "Visites (revenus mensuels) et nouveaux supporters" },
        };
        for (int i = 0; i < 8; i++) {
            const Eq& e = eqs[i];
            int yy = y + i * 30;
            DrawRectangle(12, yy, VW - 24, 28, C_ITEM);
            drawTextPx(e.name, 18, yy + 3, 10, C_HI);
            for (int k = 0; k < e.maxL; k++) DrawRectangle(120 + k * 12, yy + 4, 10, 8, k < e.level ? C_GOOD : Color{ 0, 0, 0, 120 });
            drawTextPx(e.desc, 18, yy + 15, 10, C_DIM);
            bool maxed = e.level >= e.maxL;
            if (button(VW - 180, yy + 6, 164, 16, maxed ? "Niveau maximum" : "Améliorer : " + money(Career::projectCost(T, e.kind, 0, 0)), false, !maxed) && !maxed) launch(e.kind, 0, 0);
        }
        drawFooter("Clic : lancer des travaux   LB/RB : onglet   Retour");
    } else if (g_stTab == 2) {
        // ---- boutique : maillot, produits dérivés, boutique en ligne
        Merch& MC = T.merch; if (!MC.init) initMerch(T);
        int nrow = 1 + NUM_MERCH;
        if (IN.up) g_stSel = (g_stSel + nrow - 1) % nrow;
        if (IN.down) g_stSel = (g_stSel + 1) % nrow;
        if (g_stSel >= nrow) g_stSel = 0;
        // en-tête : vitrine
        DrawRectangle(12, y, VW - 24, 30, Color{ 20, 34, 64, 255 });
        DrawRectangle(12, y, VW - 24, 2, hexc(T.home.shirt));
        drawKitIcon(T.home, 20, y + 5, 2);
        drawTextPx(fmt("Boutique officielle - niveau %d / 5", S.boutique), 44, y + 3, 10, C_HI);
        int64_t tot = MC.shirtRevenue + MC.onlineRevenue; for (int k = 0; k < NUM_MERCH; k++) tot += MC.revenue[k];
        drawTextPx("Ventes de la saison : " + money(M.seasonShop) + "   (produits : " + money(tot) + ")", 44, y + 15, 10, C_GOOD);
        {   // boutique en ligne
            static const char* ONL[] = { "aucune", "site simple", "site + livraison", "e-boutique internationale" };
            drawTextPx(std::string("Boutique en ligne : ") + ONL[MC.online], VW - 300, y + 3, 10, MC.online ? C_GOOD : C_DIM);
            int64_t cost = 60000LL * (MC.online + 1) * (MC.online + 1);
            bool can = MC.online < 3;
            if (button(VW - 300, y + 14, 180, 13, can ? "Développer : " + money(cost / 1000) : "Niveau maximum", false, can) && can) {
                askConfirm(fmt("Développer la boutique en ligne pour %s ? (frais mensuels : %s)", money(cost / 1000).c_str(), money((MC.online + 1) * 1500 / 1000 + 1).c_str()), [cost]() {
                    Team& TT = g_world.teams[g_career.userTeam];
                    if (g_career.mgr.budget < cost / 1000) { toast("Budget insuffisant"); return; }
                    g_career.mgr.budget -= cost / 1000; g_career.mgr.seasonStadiumCost += cost / 1000;
                    TT.merch.online++; toast("Boutique en ligne améliorée"); }, "Développer", "Annuler");
            }
            if (MC.online) drawTextPx("En ligne : " + money(MC.onlineRevenue / 1000), VW - 110, y + 15, 10, C_GOOD);
        }
        y += 34;
        // tableau des produits
        drawTextPx("Produit", 44, y, 10, C_DIM); drawTextPx("Prix", 250, y, 10, C_DIM); drawTextPx("Réf.", 330, y, 10, C_DIM);
        drawTextPx("Vendus", 380, y, 10, C_DIM); drawTextPx("Bénéfice", 450, y, 10, C_DIM); drawTextPx("En vente", VW - 80, y, 10, C_DIM);
        y += 12;
        float shirtRef = T.status == CS_PRO ? 75.f : T.status == CS_SEMIPRO ? 45.f : 28.f;
        for (int r = 0; r < nrow; r++) {
            int k = r - 1;
            int yy = y + r * 21;
            bool sel = r == g_stSel;
            bool hover = IN.mouse.x >= 12 && IN.mouse.x < VW - 12 && IN.mouse.y >= yy && IN.mouse.y < yy + 20;
            if (hover && IN.click) g_stSel = r;
            bool locked = k >= 0 && S.boutique < merchMinShop(k);
            DrawRectangle(12, yy, VW - 24, 20, sel ? Color{ 90, 80, 30, 255 } : (r % 2 ? C_ITEM : Color{ 38, 62, 108, 255 }));
            drawMerchIcon(k, 18, yy + 2, T.home);
            int price = k < 0 ? S.shirtPrice : MC.price[k];
            int ref = k < 0 ? (int)shirtRef : merchRefPrice(k, T.status);
            drawTextPx(k < 0 ? std::string("Maillot officiel") : std::string(merchName(k)), 44, yy + 5, 10, locked ? C_DIM : sel ? C_HI : C_TXT);
            if (locked) { drawTextPx(fmt("boutique niveau %d requis", merchMinShop(k)), 250, yy + 5, 10, C_BAD); continue; }
            drawTextPx(fmt("%d EUR", price), 250, yy + 5, 10, price > ref * 1.3f ? C_BAD : price < ref * 0.8f ? C_HI : C_TXT);
            if (button(208, yy + 4, 14, 12, "-")) { if (k < 0) S.shirtPrice = (int16_t)std::max(5, S.shirtPrice - 5); else MC.price[k] = (int16_t)std::max(1, MC.price[k] - 1); }
            if (button(296, yy + 4, 14, 12, "+")) { if (k < 0) S.shirtPrice = (int16_t)std::min(250, S.shirtPrice + 5); else MC.price[k] = (int16_t)std::min(300, MC.price[k] + 1); }
            drawTextPx(fmt("%d", ref), 330, yy + 5, 10, C_DIM);
            int units = k < 0 ? MC.shirtUnits : MC.units[k];
            int64_t rev = k < 0 ? MC.shirtRevenue : MC.revenue[k];
            drawTextPx(fmt("%d", units), 380, yy + 5, 10, C_TXT);
            drawTextPx(money(rev / 1000), 450, yy + 5, 10, rev >= 0 ? C_GOOD : C_BAD);
            bool on = k < 0 ? true : MC.on[k] != 0;
            if (k >= 0) { if (button(VW - 80, yy + 3, 56, 14, on ? "OUI" : "NON", on)) MC.on[k] = !MC.on[k]; }
            else drawTextPx("toujours", VW - 76, yy + 5, 10, C_DIM);
        }
        if (g_stSel >= 0) {
            int k = g_stSel - 1;
            if (IN.left) { if (k < 0) S.shirtPrice = (int16_t)std::max(5, S.shirtPrice - 5); else MC.price[k] = (int16_t)std::max(1, MC.price[k] - 1); }
            if (IN.right) { if (k < 0) S.shirtPrice = (int16_t)std::min(250, S.shirtPrice + 5); else MC.price[k] = (int16_t)std::min(300, MC.price[k] + 1); }
            if (IN.ok && k >= 0) MC.on[k] = !MC.on[k];
        }
        y += nrow * 21 + 2;
        drawTextPx("Au-dessus du prix de référence : moins de ventes. Supporters, victoires et vente en ligne les font grimper.", 14, y, 10, C_DIM);
        drawFooter("Haut/Bas : produit   Gauche/Droite : prix   OK : mettre en vente   LB/RB : onglet   Retour");
    } else if (g_stTab == 3) {
        int x = 20;
        drawSection(12, y, VW - 24, "SPONSOR MAILLOT"); y += 14;
        {
            ManagerState& MM = g_career.mgr;
            static int dur = 2;         // durée proposée (saisons)
            // durée : plus le contrat est long, plus le montant annuel est garanti mais négocié à la baisse
            static const int PCT[5] = { 0, 110, 100, 94, 88 };
            if (!T.sponsor.empty()) {
                drawTextPx("Sponsor : " + T.sponsor + "  -  " + money(MM.sponsorIncome) + " par an", x, y, 10, C_GOOD);
                std::string ct = MM.sponsorYears > 0 ? fmt("Contrat : encore %d saison%s (fin en juin %d)", MM.sponsorYears, MM.sponsorYears > 1 ? "s" : "", g_career.year + MM.sponsorYears) : std::string("Contrat sans durée fixée");
                drawTextPx(ct, x, y + 12, 10, C_DIM);
                int64_t pen = MM.sponsorYears > 0 ? MM.sponsorIncome * MM.sponsorYears / 2 : 0;
                if (button(VW - 220, y - 3, 200, 16, pen > 0 ? "Rompre (indemnité " + money(pen) + ")" : "Changer de sponsor"))
                    askConfirm(pen > 0 ? "Rompre le contrat ? Le club paie une indemnité de " + money(pen) + "." : std::string("Mettre fin au contrat de sponsoring ?"), [pen]() {
                        Team& TT = g_world.teams[g_career.userTeam];
                        g_career.mgr.budget -= pen; g_career.mgr.seasonStadiumCost += pen;
                        TT.sponsor.clear(); g_career.mgr.sponsorIncome = 0; g_career.mgr.sponsorYears = 0; toast("Contrat rompu"); });
                y += 12;
            } else {
                static const char* SPN[] = { "Boulangerie Martin", "Garage du Centre", "Pixel Cola", "Crédit Régional", "Transports Leroy", "Amigoal", "Assurances du Sud", "Supermarché Plus", "BTP Moreau", "Brasserie des Sports",
                                             "Broke Studio", "Alekmaul Studio", "SGDK", "PVSNESLIB", "OFFGAME", "Double Side Game", "Champagne Beurton et Fils", "Electric Dreams",
                                             "Rétro Arcade", "Cartouche 16 bits", "Mutuelle du Sport", "Optique Vision+", "Pizzeria Da Luigi", "Radio Stade FM" };
                const int NSPN = (int)(sizeof SPN / sizeof SPN[0]);
                int cap = S.capacity();
                int64_t base = std::max<int64_t>(2, (int64_t)(MM.incomeBase / 12 + cap / 200));
                int64_t off = std::max<int64_t>(2, base * PCT[dur] / 100);
                int k = (int)((hashStr(T.name) + g_career.year) % NSPN);
                if (T.status == CS_PRO) { static const int PRO[] = { 2, 3, 5, 10, 11, 14, 16, 17 }; k = PRO[(hashStr(T.name) + g_career.year) % 8]; }
                drawTextPx(fmt("Offre : %s - ", SPN[k]) + money(off) + " par an", x, y, 10, C_TXT);
                drawTextPx(fmt("Durée : %d saison%s  (total %s)", dur, dur > 1 ? "s" : "", money(off * dur).c_str()), x, y + 12, 10, C_DIM);
                if (button(VW - 330, y - 3, 50, 16, "- an", false, dur > 1) && dur > 1) dur--;
                if (button(VW - 276, y - 3, 50, 16, "+ an", false, dur < 4) && dur < 4) dur++;
                if (button(VW - 220, y - 3, 200, 16, "Signer avec ce sponsor")) { T.sponsor = SPN[k]; MM.sponsorIncome = off; MM.sponsorYears = dur; toast(fmt("Contrat de sponsoring signé pour %d saison%s", dur, dur > 1 ? "s" : "")); }
                y += 12;
            }
            y += 20;
        }
        drawSection(12, y, VW - 24, "NOM DU STADE (SPONSOR)"); y += 14;
        if (S.namingIncome > 0) {
            drawTextPx(std::string("Nom commercial : ") + S.sponsor, x, y, 10, C_HI); y += 12;
            drawTextPx("Revenu : " + money(S.namingIncome) + " par an" + (g_career.mgr.namingYears > 0 ? fmt("  -  encore %d saison%s", g_career.mgr.namingYears, g_career.mgr.namingYears > 1 ? "s" : "") : std::string()), x, y, 10, C_GOOD);
            if (button(VW - 220, y - 12, 200, 16, "Mettre fin au contrat")) askConfirm("Mettre fin au contrat de naming ?", [&S]() { S.namingIncome = 0; S.sponsor[0] = 0; });
            y += 16;
        } else {
            int64_t off = g_career.namingOffer();
            drawTextPx("Offre d'un sponsor pour donner son nom au stade : " + money(off) + " par an", x, y, 10, C_TXT);
            if (button(VW - 220, y - 3, 200, 16, "Accepter l'offre")) {
                static const char* SP[] = { "Pixel Cola", "Amigoal", "Crampons+", "Stade TV", "Goal FM", "Super But", "Banque du Sud", "Assurances Mutuelles" };
                int k = (int)(hashStr(T.name) % 8);
                snprintf(S.sponsor, sizeof S.sponsor, "%s Arena", SP[k]);
                S.namingIncome = (int32_t)off; g_career.mgr.namingYears = 5;
                toast("Contrat de naming signé pour 5 saisons");
            }
            y += 16;
        }
        y += 8;
        drawSection(12, y, VW - 24, "VIE DU CLUB"); y += 14;
        drawTextPx(fmt("Supporters : %d   Record d'affluence : %d   Affluence moyenne : %d", S.fans, S.bestAtt, S.seasonHomeMatches ? S.seasonAttTotal / S.seasonHomeMatches : 0), x, y, 10, C_TXT); y += 12;
        drawTextPx("Billetterie, buvette et parking cette saison : " + money(M.seasonGate), x, y, 10, C_GOOD); y += 12;
        drawTextPx("Coûts du stade cette saison (travaux, entretien, amendes) : " + money(M.seasonStadiumCost), x, y, 10, C_BAD); y += 12;
        drawTextPx(std::string("Statut du club : ") + statusName(T.status), x, y, 10, C_HI); y += 12;
        int t = g_career.tierOfTeam(g_career.userTeam);
        int req = minCapacity(t);
        if (req > 0) drawTextPx(fmt("Cahier des charges de la division : %d places minimum (%s)", req, cap >= req ? "conforme" : "NON CONFORME : amende chaque mois"), x, y, 10, cap >= req ? C_GOOD : C_BAD);
        y += 12;
        drawTextPx(fmt("Réputation du club : %d / 100 (%s)", teamReputation(g_career.userTeam), reputationLabel(teamReputation(g_career.userTeam))), x, y, 10, C_HI); y += 14;
        // dernière recette et politique de la Coupe de France
        if (g_lastGate.att > 0) {
            const GateInfo& G = g_lastGate;
            drawSection(12, y, VW - 24, "DERNIÈRE RECETTE À DOMICILE" + std::string(G.opp.empty() ? "" : " (contre " + G.opp + ")")); y += 13;
            drawTextPx(fmt("%d spectateurs   Brut ", G.att) + money(G.brut / 1000) + "   TVA 5,5 % -" + money(G.tva / 1000) + "   Prélèvement -" + money(G.prelev / 1000)
                       + "   Arbitrage -" + money(G.arbitrage / 1000), x, y, 10, C_TXT); y += 12;
            drawTextPx("Organisation (sécurité, stadiers) -" + money(G.orga / 1000) + "   Net " + money(G.net / 1000) + (G.shared ? "   Partage Coupe de France : part du club " + money(G.part / 1000) : std::string()), x, y, 10, C_GOOD); y += 14;
        }
        if (T.status == CS_PRO && button(x, y, 420, 14, std::string("Coupe de France contre un amateur : ") + (M.cdfGiveAll ? "laisser toute la recette (bonne image)" : "partager la recette (50/50)"), M.cdfGiveAll))
            g_career.mgr.cdfGiveAll = !g_career.mgr.cdfGiveAll;
        drawFooter("LB/RB : onglet   Retour");
    } else if (g_stTab == 4) {
        auto& P = M.projects;
        if (P.empty()) drawTextCentered("Aucun chantier en cours", VW / 2, y + 20, 10, C_DIM);
        for (int i = 0; i < (int)P.size(); i++) {
            const Project& p = P[i];
            int yy = y + i * 16;
            DrawRectangle(12, yy, VW - 24, 14, C_ITEM);
            std::string what = std::string(projectName(p.kind)) + (p.kind == PJ_SEATS || p.kind == PJ_COVER || p.kind == PJ_VIP || p.kind == PJ_UPGRADE_SEATS ? std::string(" - ") + STAND_NAMES[p.stand] : "");
            if (p.amount) what += fmt(" (+%d)", p.amount);
            drawTextPx(what, 18, yy + 2, 10, C_TXT);
            drawTextPx(fmt("%d mois restant%s", p.monthsLeft, p.monthsLeft > 1 ? "s" : ""), VW - 260, yy + 2, 10, C_HI);
            drawTextPx(money(p.cost), VW - 110, yy + 2, 10, C_DIM);
        }
        drawFooter("LB/RB : onglet   Retour");
    }
    if (IN.back) g_screen = SC_CLUBMENU;
}

// ---------------- clubs créés : reprendre, modifier, supprimer
static ListW g_clLW;
static void deleteCustomClub(int idx) {
    if (idx < g_world.baseCount || idx >= (int)g_world.teams.size() || !g_world.teams[idx].custom) return;
    g_careerActive = false;          // la carrière en cours référence peut-être ce club
    g_world.teams.erase(g_world.teams.begin() + idx);
    for (int i = g_world.baseCount; i < (int)g_world.teams.size(); i++) { Team& t = g_world.teams[i]; if (t.parent > idx) t.parent--; else if (t.parent == idx) t.parent = -1; }
    saveCustomClubs();
    levelCacheDirty();
}

static void screenCustomList() {
    drawBackground("Mes clubs créés");
    std::vector<int> v;
    for (int i = g_world.baseCount; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].custom) v.push_back(i);
    int s = listRun(g_clLW, (int)v.size(), 20, 36, VW - 40, 20, 13, [&](int i, int x, int y, bool sel) {
        const Team& t = g_world.teams[v[i]];
        drawKitIcon(t.home, x + 3, y + 2, 1);
        drawTextPx(fitText(t.name, 220, 10), x + 16, y + 1, 10, sel ? BLACK : C_TXT);
        drawTextPx(fitText(t.dept >= 0 ? deptName(t.dept) : std::string("-"), 170, 10), x + 250, y + 1, 10, sel ? BLACK : C_DIM);
        drawTextPx(fmt("note %d  (fondé en %d)", (int)t.rating, t.founded), x + 430, y + 1, 10, sel ? BLACK : C_DIM);
    });
    if (v.empty()) drawTextCentered("Aucun club créé. Utilisez « Créer un nouveau club » dans l'éditeur.", VW / 2, 80, 10, C_DIM);
    int cur = g_clLW.cur;
    bool ok = cur >= 0 && cur < (int)v.size();
    int by = VH - 34;
    if (button(20, by, 140, 16, "Démarrer une carrière", false, ok) && ok) { startCareerWith(v[cur], SC_CUSTOMLIST); return; }
    if (button(166, by, 110, 16, "Fiche", false, ok) && ok) { openFiche(v[cur], SC_CUSTOMLIST); return; }
    if (button(282, by, 110, 16, "Modifier", false, ok) && ok) { openClubEditor(v[cur], false, SC_CUSTOMLIST); return; }
    if (button(398, by, 110, 16, "Supprimer", false, ok) && ok) {
        int id = v[cur];
        askConfirm("Supprimer définitivement " + g_world.teams[id].name + " ?" + (g_careerActive ? " La carrière en cours sera fermée (la sauvegarde automatique reste disponible)." : ""),
                   [id]() { deleteCustomClub(id); toast("Club supprimé"); }, "Supprimer", "Annuler");
    }
    drawFooter("OK : démarrer une carrière   Retour");
    if (s >= 0) { startCareerWith(v[s], SC_CUSTOMLIST); return; }
    if (IN.back) g_screen = SC_EDITMENU;
}
