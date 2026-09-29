// ------------------------------------------------------------------ nouvelle carrière (options), statut du club, stade, clubs créés
static int g_newTeam = -1;
static Screen g_newBack = SC_MAIN;
static bool g_optNoSack = false;
static int g_optMode = 2;           // 0 jeu (matchs seulement), 1 manager (temps forts), 2 manager + jeu
static int g_optBudget = 1;          // 0 serré, 1 normal, 2 confortable
static ListW g_coptLW;

static void startCareerWith(int team, Screen back) {
    g_newTeam = team; g_newBack = back; g_coptLW = ListW(); g_coptLW.cur = 5;
    g_screen = SC_CAREEROPT;
}

static void screenCareerOpt() {
    drawBackground("Nouvelle carrière");
    const Team& T = g_world.teams[g_newTeam];
    drawKitIcon(T.home, VW / 2 - 12, 36, 3);
    drawTextCentered(T.name, VW / 2, 64, 20, C_HI);
    drawTextCentered(divisionOf(g_newTeam), VW / 2, 86, 10, C_DIM);
    static const char* BUD[] = { "Serré (budget divisé par 2)", "Normal", "Confortable (budget x 2)" };
    std::vector<std::string> items = {
        std::string("Mode de jeu : ") + (g_optMode == 0 ? "JEU (vous jouez les matchs, pas de gestion)" : g_optMode == 1 ? "MANAGER (gestion, matchs en temps forts)" : "MANAGER + JEU (gestion et vous jouez les matchs)"),
        std::string("Licenciement par le président : ") + (g_optNoSack ? "NON (vous ne pouvez pas être renvoyé)" : "OUI"),
        std::string("Budget de départ : ") + BUD[g_optBudget],
        "Manager : " + managerChoiceName(),
        "Choisir un autre club",
        ">>> COMMENCER LA CARRIÈRE <<<" };
    int s = menuRun(g_coptLW, items, 104, 480);
    int y = 104 + 20 * 6 + 10;
    const char* help[] = {
        "Le président fixe un objectif chaque saison. Si sa confiance tombe à 5 % ou moins, vous êtes licencié",
        "(sauf si le licenciement est désactivé). Statut du club : PRO en L1/L2/L3, SEMI-PRO en National,",
        "au choix SEMI-PRO ou AMATEUR en Régional, AMATEUR en Départemental.",
        "JEU : vous jouez vos matchs sans gestion. MANAGER : gestion complète, vous regardez les temps forts",
        "et donnez vos consignes. MANAGER + JEU : gestion complète et vous jouez vos matchs. (Modifiable en cours.)",
    };
    for (auto h : help) { drawTextCentered(h, VW / 2, y, 10, C_DIM, false); y += 12; }
    drawFooter("Gauche/Droite : modifier   OK : valider   Retour");
    int d = IN.left ? -1 : IN.right ? 1 : 0;
    if (d || (s >= 0 && s < 4)) {
        if (!d) d = 1;
        if (g_coptLW.cur == 0) g_optMode = (g_optMode + d + 3) % 3;
        if (g_coptLW.cur == 1) g_optNoSack = !g_optNoSack;
        if (g_coptLW.cur == 2) g_optBudget = (g_optBudget + d + 3) % 3;
        if (g_coptLW.cur == 3) managerCycle(d);
    }
    if (IN.back || s == 4) { g_screen = g_newBack; return; }
    if (s == 5) {
        applyChosenManager();
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

static void screenStadium() {
    Team& T = g_world.teams[g_career.userTeam];
    StadiumInfo& S = T.sta;
    if (!S.init) initStadium(T, g_career.tierOfTeam(g_career.userTeam), -1);
    drawBackground(T.stadium.empty() ? "Stade" : T.stadium);
    ManagerState& M = g_career.mgr;
    int cap = S.capacity();
    drawTextPx(fmt("Capacité : %d places   Supporters : %d   Dernière affluence : %d (%d%%)", cap, S.fans, S.lastAtt, cap ? S.lastAtt * 100 / cap : 0), 12, 31, 10, C_TXT);
    drawTextPx("Budget : " + money(M.budget), VW - 170, 31, 10, M.budget >= 0 ? C_GOOD : C_BAD);
    static const char* TABS[] = { "Tribunes et billets", "Équipements", "Boutique et sponsors", "Travaux en cours" };
    for (int i = 0; i < 4; i++) if (button(12 + i * 154, 44, 150, 14, TABS[i], g_stTab == i)) { g_stTab = i; g_stLW = ListW(); }
    if (IN.pgUp) g_stTab = (g_stTab + 3) % 4;
    if (IN.pgDn) g_stTab = (g_stTab + 1) % 4;
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
        int x = 20;
        drawSection(12, y, VW - 24, "BOUTIQUE"); y += 14;
        drawTextPx(fmt("Prix du maillot : %d EUR", S.shirtPrice), x, y, 10, C_TXT);
        if (button(200, y - 2, 60, 14, "-5")) S.shirtPrice = (int16_t)std::max(10, S.shirtPrice - 5);
        if (button(264, y - 2, 60, 14, "+5")) S.shirtPrice = (int16_t)std::min(250, S.shirtPrice + 5);
        if (IN.left) S.shirtPrice = (int16_t)std::max(10, S.shirtPrice - 5);
        if (IN.right) S.shirtPrice = (int16_t)std::min(250, S.shirtPrice + 5);
        y += 16;
        drawTextPx(fmt("Niveau de la boutique : %d / 5   (améliorable dans Équipements)", S.boutique), x, y, 10, C_DIM); y += 12;
        drawTextPx("Ventes de la saison : " + money(M.seasonShop), x, y, 10, C_GOOD); y += 12;
        drawTextPx("Un maillot trop cher se vend moins ; les supporters et les victoires font grimper les ventes.", x, y, 10, C_DIM); y += 20;
        drawSection(12, y, VW - 24, "SPONSOR MAILLOT"); y += 14;
        {
            ManagerState& MM = g_career.mgr;
            if (!T.sponsor.empty()) {
                drawTextPx("Sponsor : " + T.sponsor + "  -  " + money(MM.sponsorIncome) + " par an", x, y, 10, C_GOOD);
                if (button(VW - 220, y - 3, 200, 16, "Changer de sponsor")) { T.sponsor.clear(); MM.sponsorIncome = 0; }
            } else {
                static const char* SPN[] = { "Boulangerie Martin", "Garage du Centre", "Pixel Cola", "Crédit Régional", "Transports Leroy", "Amigoal", "Assurances du Sud", "Supermarché Plus", "BTP Moreau", "Brasserie des Sports" };
                int cap = S.capacity();
                int64_t off = std::max<int64_t>(2, (int64_t)(MM.incomeBase / 12 + cap / 200));
                int k = (int)((hashStr(T.name) + g_career.year) % 10);
                if (T.status == CS_PRO) k = 2 + (int)(hashStr(T.name) % 4);
                drawTextPx(fmt("Offre : %s - ", SPN[k]) + money(off) + " par an", x, y, 10, C_TXT);
                if (button(VW - 220, y - 3, 200, 16, "Signer avec ce sponsor")) { T.sponsor = SPN[k]; MM.sponsorIncome = off; toast("Contrat de sponsoring signé"); }
            }
            y += 20;
        }
        drawSection(12, y, VW - 24, "NOM DU STADE (SPONSOR)"); y += 14;
        if (S.namingIncome > 0) {
            drawTextPx(std::string("Nom commercial : ") + S.sponsor, x, y, 10, C_HI); y += 12;
            drawTextPx("Revenu : " + money(S.namingIncome) + " par an", x, y, 10, C_GOOD);
            if (button(VW - 220, y - 12, 200, 16, "Mettre fin au contrat")) askConfirm("Mettre fin au contrat de naming ?", [&S]() { S.namingIncome = 0; S.sponsor[0] = 0; });
            y += 16;
        } else {
            int64_t off = g_career.namingOffer();
            drawTextPx("Offre d'un sponsor pour donner son nom au stade : " + money(off) + " par an", x, y, 10, C_TXT);
            if (button(VW - 220, y - 3, 200, 16, "Accepter l'offre")) {
                static const char* SP[] = { "Pixel Cola", "Amigoal", "Crampons+", "Stade TV", "Goal FM", "Super But", "Banque du Sud", "Assurances Mutuelles" };
                int k = (int)(hashStr(T.name) % 8);
                snprintf(S.sponsor, sizeof S.sponsor, "%s Arena", SP[k]);
                S.namingIncome = (int32_t)off;
                toast("Contrat de naming signé");
            }
            y += 16;
        }
        y += 8;
        drawSection(12, y, VW - 24, "VIE DU CLUB"); y += 14;
        drawTextPx(fmt("Supporters : %d   Record d'affluence : %d   Affluence moyenne : %d", S.fans, S.bestAtt, S.seasonHomeMatches ? S.seasonAttTotal / S.seasonHomeMatches : 0), x, y, 10, C_TXT); y += 12;
        drawTextPx("Billetterie, buvette et parking cette saison : " + money(M.seasonGate), x, y, 10, C_GOOD); y += 12;
        drawTextPx("Coûts du stade cette saison (travaux, entretien, amendes) : " + money(M.seasonStadiumCost), x, y, 10, C_BAD); y += 12;
        drawTextPx(std::string("Statut du club : ") + statusName(T.status), x, y, 10, C_HI); y += 12;
        static const int REQ[5] = { 15000, 8000, 4000, 2000, 1000 };
        int t = g_career.tierOfTeam(g_career.userTeam);
        if (t >= 0 && t <= 4) drawTextPx(fmt("Cahier des charges de la division : %d places minimum (%s)", REQ[t], cap >= REQ[t] ? "conforme" : "NON CONFORME : amende chaque mois"), x, y, 10, cap >= REQ[t] ? C_GOOD : C_BAD);
        drawFooter("Gauche/Droite : prix du maillot   LB/RB : onglet   Retour");
    } else {
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
