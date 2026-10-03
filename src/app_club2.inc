// ------------------------------------------------------------------ gestion du club : menu, staff, réserves, amicaux, arbitres
static void drawIconTile(int kind, int x, int y, Color c) {
    switch (kind) {
    case 0: for (int i = 0; i < 3; i++) { DrawRectangle(x + i * 7, y + 6, 5, 10, c); DrawCircle(x + 2 + i * 7, y + 3, 2.5f, c); } break;          // effectif
    case 1: DrawCircle(x + 8, y + 5, 4, c); DrawRectangle(x + 2, y + 10, 12, 7, c); DrawRectangle(x + 16, y + 4, 5, 3, c); break;        // staff
    case 2: DrawRectangle(x, y + 8, 22, 9, c); DrawRectangle(x + 4, y + 2, 14, 6, c); break;                                              // réserves
    case 3: DrawCircle(x + 7, y + 9, 6, c); DrawCircle(x + 16, y + 9, 6, Color{ c.r, c.g, c.b, 150 }); break;                             // amicaux
    case 4: DrawTriangle(Vector2{ (float)x + 11, (float)y }, Vector2{ (float)x, (float)y + 16 }, Vector2{ (float)x + 22, (float)y + 16 }, c); break; // mercato
    case 5: DrawRectangle(x, y + 4, 22, 13, c); DrawRectangle(x + 4, y + 7, 14, 7, Color{ 44, 120, 44, 255 }); break;                   // stade
    case 6: DrawCircle(x + 11, y + 9, 8, c); drawTextPx("E", x + 8, y + 4, 10, BLACK); break;                                            // finances
    case 7: DrawRectangle(x + 3, y, 16, 18, c); DrawRectangle(x + 6, y + 4, 10, 2, BLACK); DrawRectangle(x + 6, y + 8, 10, 2, BLACK); break; // fiche
    case 8: for (int i = 0; i < 3; i++) DrawTriangle(Vector2{ (float)x + 3 + i * 8, (float)y + 6 }, Vector2{ (float)x + i * 8, (float)y + 17 }, Vector2{ (float)x + 7 + i * 8, (float)y + 17 }, i == 1 ? c : Color{ 255, 140, 40, 255 }); DrawCircle(x + 11, y + 3, 3, c); break; // entraînement
    case 10: DrawCircle(x + 7, y + 4, 3, c); DrawRectangle(x + 4, y + 8, 7, 9, c); for (int k = 0; k < 5; k++) { float a = k * 1.2566f - 1.57f; DrawTriangle(Vector2{ (float)x + 17, (float)y + 8 }, Vector2{ (float)x + 17 + std::cos(a) * 6, (float)y + 8 + std::sin(a) * 6 }, Vector2{ (float)x + 17 + std::cos(a + 0.6f) * 3, (float)y + 8 + std::sin(a + 0.6f) * 3 }, Color{ 255, 210, 60, 255 }); } break; // formation
    case 11: DrawRectangle(x + 5, y + 3, 12, 14, c); DrawRectangle(x, y + 3, 6, 6, c); DrawRectangle(x + 16, y + 3, 6, 6, c); DrawRectangle(x + 9, y + 3, 4, 2, BLACK); break;   // maillot
    default: DrawRectangle(x, y + 3, 22, 13, c); DrawTriangle(Vector2{ (float)x, (float)y + 3 }, Vector2{ (float)x + 11, (float)y + 11 }, Vector2{ (float)x + 22, (float)y + 3 }, BLACK); break;
    }
}

// ---------------- maillots du club (modifiables à l'intersaison : avant le premier match de championnat, ou saison terminée)
static bool kitChangeAllowed() {
    Season& S = g_career.season;
    if (g_career.userTeam < 0) return false;
    if (S.finished) return true;
    for (auto& C : S.comps) if (C.kind == 1) for (auto& m : C.matches) if (m.played && (m.home == g_career.userTeam || m.away == g_career.userTeam)) return false;
    return true;
}
static Kit g_kitEd[3];
static bool g_kitThird = false;
static int g_kitRow = 0;
static void openKits() {
    const Team& U = g_world.teams[g_career.userTeam];
    g_kitEd[0] = U.home; g_kitEd[1] = U.away; g_kitEd[2] = U.hasThird ? U.third : teamKit(U, 2); g_kitThird = U.hasThird; g_kitRow = 0;
    g_screen = SC_KITS;
}
static void screenKits() {
    Team& U = g_world.teams[g_career.userTeam];
    drawBackground("Maillots du club");
    bool ok = kitChangeAllowed();
    static float an = 0; an += GetFrameTime();
    // aperçu : joueurs domicile et extérieur
    int px = VW - 250;
    DrawRectangle(px, 36, 236, 140, Color{ 34, 102, 34, 255 });
    for (int i = 0; i < 7; i++) DrawRectangle(px, 36 + i * 20, 236, 10, Color{ 40, 116, 40, 255 });
    DrawRectangleLines(px, 36, 236, 140, Color{ 240, 240, 240, 160 });
    int nk = g_kitThird ? 3 : 2;
    for (int k = 0; k < nk; k++) {
        Kit kk = g_kitEd[k];
        int cx = px + (nk == 3 ? 40 + k * 78 : 60 + k * 116);
        drawPlayerSprite(cx, 150, kk, 2 + k, k, 0, (int)(an * 6) + k * 2, PS_NORMAL, false, 0, nk == 3 ? 4 : 5);
        drawTextCentered(k == 0 ? "DOMICILE" : k == 1 ? "EXTÉRIEUR" : "TROISIÈME", cx, 158, 10, WHITE, false);
    }
    drawTextCentered(U.name, px + 118, 182, 10, C_HI, false);
    if (!ok) drawTextCentered("Saison en cours : changement de maillot possible à l'intersaison.", px + 118, 196, 10, C_BAD, false);
    struct Row { const char* label; int kit; int field; };
    static const Row R[] = { { "Domicile - couleur principale", 0, 0 }, { "Domicile - couleur secondaire", 0, 1 }, { "Domicile - motif", 0, 4 }, { "Domicile - short", 0, 2 }, { "Domicile - chaussettes", 0, 3 },
                             { "Extérieur - couleur principale", 1, 0 }, { "Extérieur - couleur secondaire", 1, 1 }, { "Extérieur - motif", 1, 4 }, { "Extérieur - short", 1, 2 }, { "Extérieur - chaussettes", 1, 3 },
                             { "Troisième maillot", 2, 9 }, { "3e - couleur principale", 2, 0 }, { "3e - couleur secondaire", 2, 1 }, { "3e - motif", 2, 4 }, { "3e - short", 2, 2 } };
    const int NR = 15, NI = NR + 2;
    if (IN.up) g_kitRow = (g_kitRow + NI - 1) % NI;
    if (IN.down) g_kitRow = (g_kitRow + 1) % NI;
    int x = 12, w = VW - 280, y = 36;
    auto colorRef = [&](Kit& k, int f) -> unsigned& { return f == 0 ? k.shirt : f == 1 ? k.shirt2 : f == 2 ? k.shorts : k.socks; };
    int act = -1;
    for (int i = 0; i < NI; i++) {
        bool sel = i == g_kitRow;
        bool hover = IN.mouse.x >= x && IN.mouse.x < x + w && IN.mouse.y >= y && IN.mouse.y < y + 13;
        if (hover && IN.mouseMoved) g_kitRow = i;
        DrawRectangle(x, y, w, 13, sel ? C_SEL : (i < 5 ? C_ITEM : i < 10 ? Color{ 38, 62, 108, 255 } : i < NR ? Color{ 50, 50, 96, 255 } : Color{ 30, 46, 80, 255 }));
        Color c = sel ? BLACK : ok ? C_TXT : C_DIM;
        if (i < NR) {
            const Row& r = R[i];
            drawTextPx(r.label, x + 6, y + 2, 10, c);
            Kit& k = g_kitEd[r.kit];
            std::string v;
            if (r.field == 9) v = g_kitThird ? "oui" : "non";
            else if (r.kit == 2 && !g_kitThird) v = "-";
            else if (r.field == 4) v = PATTERN_NAMES[std::max(0, std::min(NUM_PATTERNS - 1, k.pattern))];
            else { unsigned col = colorRef(k, r.field); DrawRectangle(x + 200, y + 2, 10, 9, hexc(col)); DrawRectangleLines(x + 200, y + 2, 10, 9, BLACK); v = colorName(col); }
            drawTextPx("< " + v + " >", x + 216, y + 2, 10, c);
            if (hover && IN.click && ok) { g_kitRow = i; act = 1; }
        } else {
            drawTextCentered(i == NR ? ">>> PRÉSENTER LES NOUVEAUX MAILLOTS <<<" : "Annuler", x + w / 2, y + 3, 10, c, false);
            if (hover && IN.click) { g_kitRow = i; act = 2; }
        }
        y += i == NR - 1 || i == 4 || i == 9 ? 17 : 14;
    }
    int d = IN.left ? -1 : IN.right ? 1 : 0;
    if (ok && g_kitRow < NR && (d || act == 1 || IN.ok)) {
        if (!d) d = 1;
        const Row& r = R[g_kitRow];
        Kit& k = g_kitEd[r.kit];
        if (r.field == 9) g_kitThird = !g_kitThird;
        else if (r.kit == 2 && !g_kitThird) {}
        else if (r.field == 4) k.pattern = (k.pattern + d + NUM_PATTERNS) % NUM_PATTERNS;
        else { unsigned& col = colorRef(k, r.field); col = PALETTE[(paletteIndex(col) + d + NUM_PALETTE) % NUM_PALETTE]; }
    }
    if ((IN.ok || act == 2) && g_kitRow == NR) {
        if (!ok) toast("Changement impossible pendant la saison");
        else {
            U.home = g_kitEd[0]; U.away = g_kitEd[1];
            U.hasThird = g_kitThird ? 1 : 0; if (g_kitThird) { U.third = g_kitEd[2]; U.third.socks = U.third.shirt; }
            g_career.season.news.push_back(U.name + " présente ses nouveaux maillots pour la saison " + fmt("%d-%02d", g_career.year, (g_career.year + 1) % 100) + ".");
            addReputation(g_career.userTeam, 0);
            toast("Nouveaux maillots présentés !");
            g_screen = SC_CLUBMENU; return;
        }
    }
    if (IN.back || ((IN.ok || act == 2) && g_kitRow == NR + 1)) { g_screen = SC_CLUBMENU; return; }
    drawTextPx("Le maillot extérieur doit se distinguer du maillot domicile (couleurs de l'adversaire).", 12, VH - 34, 10, C_DIM);
    drawFooter("Haut/Bas : choisir   Gauche/Droite : changer   OK : valider   Retour");
}

static int g_cmSel = 0;
static void screenClubMenu() {
    drawBackground("Gestion du club");
    bool mm = g_career.mgr.managerMode;
    const Team& U = g_world.teams[g_career.userTeam];
    struct Tile { std::string title, info; int id, icon; };
    int nres = 0; for (auto& t : g_world.teams) if (t.parent == g_career.userTeam) nres++;
    std::vector<Tile> tiles = {
        { "Effectif", fmt("%d joueurs - %s", (int)U.squad.size(), FORMATIONS[U.formation].name), 5, 0 },
        { "Entraînement", std::string(trainFocusName(g_career.mgr.trainFocus)) + " - " + trainIntName(g_career.mgr.trainInt), 19, 8 },
        { "Réserves", fmt("%d équipe%s", nres, nres > 1 ? "s" : ""), 17, 2 },
        { "Matchs amicaux", "Inviter des clubs", 18, 3 },
        { "Fiche du club", std::string(statusName(U.status)), 9, 7 } };
    if (mm) {
        tiles.insert(tiles.begin() + 1, { "Staff", fmt("%d membre%s", (int)g_career.mgr.staff.size(), g_career.mgr.staff.size() > 1 ? "s" : ""), 16, 1 });
        tiles.push_back({ "Mercato", g_career.transferWindow() ? "OUVERT" : "fermé", 12, 4 });
        tiles.push_back({ "Stade et boutique", fmt("%d places", U.sta.capacity()), 15, 5 });
        tiles.push_back({ "Finances", "Budget " + money(g_career.mgr.budget), 13, 6 });
        tiles.push_back({ "Offres reçues", g_career.mgr.offers.empty() ? std::string("aucune") : fmt("%d en attente", (int)g_career.mgr.offers.size()), 21, 9 });
    }
    if (!mm && g_career.opts.lite) tiles.push_back({ "Transferts", "Simplifiés : prix affiché", 12, 4 });
    tiles.push_back({ "Formation", "Centre des jeunes", 22, 10 });
    tiles.push_back({ g_career.museumEnabled()?"Musée du club":"Archives", "Histoire, trophées, records", 23, 7 });
    tiles.push_back({ "Maillots", kitChangeAllowed() ? "Intersaison : modifiables" : "Changement à l'intersaison", 24, 11 });
    if(g_career.supportersEnabled())tiles.push_back({"Supporters", "Public, ambiance et mémoire", 26, 7});
    if(g_career.directorEnabled())tiles.push_back({ "Directeur sportif", "Recrutement et délégations", 25, 1 });
    int n = (int)tiles.size(), cols = 4;
    if (IN.left) g_cmSel = (g_cmSel + n - 1) % n;
    if (IN.right) g_cmSel = (g_cmSel + 1) % n;
    if (IN.up) g_cmSel = (g_cmSel - cols + n) % n;
    if (IN.down) g_cmSel = (g_cmSel + cols) % n;
    if (g_cmSel >= n) g_cmSel = 0;
    // bandeau du club : écusson (maillot), division, statut, budget, confiance du président
    {
        int bx = 12, by = 32, bw = VW - 24, bh = 50;
        Color c1 = hexc(U.home.shirt), c2 = hexc(U.home.shirt2 != U.home.shirt ? U.home.shirt2 : U.home.shorts);
        DrawRectangle(bx, by, bw, bh, Color{ 16, 26, 50, 255 });
        DrawRectangle(bx, by, 6, bh, c1); DrawRectangle(bx + 6, by, 3, bh, c2);
        DrawRectangle(bx, by + bh - 2, bw, 2, Color{ c1.r, c1.g, c1.b, 160 });
        drawKitIcon(U.home, bx + 18, by + 8, 3);
        drawTextPx(fitText(U.name, 300, 20), bx + 58, by + 2, 20, C_HI);
        drawTextPx(fitText(g_career.teamLevelName(g_career.userTeam) + "   -   " + statusName(U.status) + (U.stadium.empty() ? std::string("") : "   -   " + U.stadium), 340, 10), bx + 58, by + 25, 10, C_DIM);
        if (!g_career.managerName.empty()) drawTextPx(std::string("Manager : ") + g_career.managerName, bx + 58, by + 37, 10, C_TXT);
        int rx = VW - 230;
        drawTextPx("Budget", rx, by + 5, 10, C_DIM);
        drawTextPx(mm ? money(g_career.mgr.budget) : std::string("géré par le club"), rx + 70, by + 5, 10, g_career.mgr.budget >= 0 ? C_GOOD : C_BAD);
        drawTextPx("Président", rx, by + 18, 10, C_DIM);
        int cf = std::max(0, std::min(100, g_career.mgr.confidence));
        DrawRectangle(rx + 70, by + 20, 120, 7, Color{ 0, 0, 0, 140 });
        DrawRectangle(rx + 70, by + 20, 120 * cf / 100, 7, cf >= 60 ? C_GOOD : cf >= 30 ? C_HI : C_BAD);
        drawTextPx(fmt("%d%%", cf), rx + 194, by + 18, 10, C_TXT);
        drawTextPx("Objectif", rx, by + 31, 10, C_DIM);
        drawTextPx(fitText(g_career.objectiveText(), 150, 10), rx + 70, by + 31, 10, C_TXT);
    }
    // tuiles par familles : sportif (vert), administration (or), formation (bleu)
    auto family = [](int id) { return id == 5 || id == 19 || id == 17 || id == 18 ? 0 : id == 22 ? 2 : 1; };
    static const Color FAM[3] = { { 70, 190, 110, 255 }, { 230, 180, 60, 255 }, { 90, 160, 240, 255 } };
    int tw = 150, th = n > 12 ? 50 : 60, act = -1;
    int gx = (VW - (cols * tw + (cols - 1) * 8)) / 2;
    for (int i = 0; i < n; i++) {
        int x = gx + (i % cols) * (tw + 8), y = 90 + (i / cols) * (th + 6);
        bool hover = IN.mouse.x >= x && IN.mouse.x < x + tw && IN.mouse.y >= y && IN.mouse.y < y + th;
        if (hover && IN.mouseMoved) g_cmSel = i;
        bool sel = i == g_cmSel;
        Color fc = FAM[family(tiles[i].id)];
        DrawRectangle(x + 2, y + 3, tw, th, Color{ 0, 0, 0, 90 });                                   // ombre
        DrawRectangle(x, y, tw, th, sel ? Color{ 70, 64, 30, 255 } : Color{ 34, 54, 96, 255 });
        DrawRectangle(x, y, tw, th / 2, sel ? Color{ 84, 76, 36, 255 } : Color{ 40, 64, 112, 255 });  // reflet
        DrawRectangle(x, y, 4, th, fc);
        DrawRectangleLines(x, y, tw, th, sel ? C_SEL : Color{ 70, 100, 160, 255 });
        DrawCircle(x + 26, y + th / 2, 16, Color{ fc.r, fc.g, fc.b, (unsigned char)(sel ? 90 : 45) });
        drawIconTile(tiles[i].icon, x + 15, y + th / 2 - 10, sel ? C_HI : C_TXT);
        drawTextPx(fitText(tiles[i].title, tw - 56, 10), x + 50, y + 16, 10, sel ? C_HI : C_TXT);
        drawTextPx(fitText(tiles[i].info, tw - 56, 10), x + 50, y + 32, 10, C_DIM);
        if (sel) DrawRectangle(x + 50, y + 28, std::min(tw - 60, textWidth(tiles[i].title, 10)), 1, C_HI);
        if (hover && IN.click) act = i;
    }
    {   // légende
        int ly = 90 + ((n + cols - 1) / cols) * (th + 6) + 2;
        static const char* FN[3] = { "Sportif", "Administration", "Formation" };
        int lx = gx;
        for (int f = 0; f < 3; f++) { DrawRectangle(lx, ly + 2, 8, 8, FAM[f]); drawTextPx(FN[f], lx + 12, ly, 10, C_DIM); lx += textWidth(FN[f], 10) + 30; }
        if (!mm) drawTextPx("Mode « matchs seulement » : finances, mercato, stade et staff sont gérés automatiquement.", gx, ly + 14, 10, C_DIM);
    }
    if (IN.ok) act = g_cmSel;
    drawFooter("Flèches : choisir   OK : ouvrir   Retour");
    if (IN.back) { openHub(); return; }
    if (act < 0) return;
    switch (tiles[act].id) {
    case 5: g_screen = SC_SQUAD; break;
    case 9: openFiche(g_career.userTeam, SC_CLUBMENU); break;
    case 12: openMarket(); break;
    case 13: g_screen = SC_FINANCE; break;
    case 15: g_stTab = 0; g_screen = SC_STADIUM; break;
    case 16: g_screen = SC_STAFF; break;
    case 17: g_screen = SC_RESERVES; break;
    case 18: g_inviteSel.clear(); g_screen = SC_FRIENDLIES; break;
    case 19: g_screen = SC_TRAINING; break;
    case 21: g_screen = SC_OFFERS; break;
    case 22: g_screen = SC_ACADEMY; break;
    case 23: if(g_career.museumEnabled())openMuseum(g_career.userTeam,SC_CLUBMENU);else{openFiche(g_career.userTeam, SC_CLUBMENU); g_fichePage = 3;} break;
    case 24: openKits(); break;
    case 25: openDirector(); break;
    case 26: openSupporters(g_career.userTeam,SC_CLUBMENU);break;
    }
}

// ---------------- offres de transfert reçues
static ListW g_offLW;
static void screenOffers() {
    drawBackground("Offres reçues");
    auto& O = g_career.mgr.offers;
    drawTextPx(g_career.transferWindow() ? "Mercato ouvert : vous pouvez accepter, refuser ou négocier (+20 %, au risque que le club se retire)."
                                         : "Mercato fermé : les offres seront à nouveau possibles à l'ouverture du mercato.", 12, 31, 10, C_DIM);
    drawTextPx("Joueur", 18, 46, 10, C_DIM); drawTextPx("Âge", 200, 46, 10, C_DIM); drawTextPx("Note", 236, 46, 10, C_DIM);
    drawTextPx("Club acheteur", 280, 46, 10, C_DIM); drawTextPx("Offre", 500, 46, 10, C_DIM); drawTextPx("Valeur", 570, 46, 10, C_DIM);
    int sel = listRun(g_offLW, (int)O.size(), 12, 58, VW - 24, 12, 14, [&](int i, int x, int y, bool s2) {
        int idx; int t = g_world.findPlayer(O[i].pid, &idx);
        Color c = s2 ? BLACK : C_TXT;
        if (t < 0) { drawTextPx("(joueur parti)", x + 6, y + 2, 10, c); return; }
        const Player& P = g_world.teams[t].squad[idx];
        drawTextPx(fitText(P.name, 170, 10), x + 6, y + 2, 10, c);
        drawTextPx(fmt("%d", P.age), x + 188, y + 2, 10, c);
        drawTextPx(fmt("%d", P.overall()), x + 224, y + 2, 10, s2 ? BLACK : C_HI);
        drawKitIcon(g_world.teams[O[i].club].home, x + 266, y + 2, 1);
        drawTextPx(fitText(g_world.teams[O[i].club].name, 200, 10), x + 280, y + 2, 10, c);
        drawTextPx(money(O[i].fee), x + 488, y + 2, 10, s2 ? BLACK : C_GOOD);
        drawTextPx(money(P.value()), x + 558, y + 2, 10, s2 ? BLACK : C_DIM);
    });
    if (O.empty()) drawTextCentered("Aucune offre pour le moment. Les clubs se manifestent pendant les mercatos.", VW / 2, 90, 10, C_DIM);
    int cur = g_offLW.cur;
    int by = 58 + 12 * 14 + 12;
    bool open = g_career.transferWindow();
    if (cur >= 0 && cur < (int)O.size()) {
        std::string msg;
        if (button(12, by, 150, 16, "Accepter", false, open)) { g_career.answerOffer(cur, 0, msg); toast(msg); }
        else if (button(170, by, 150, 16, "Refuser")) { g_career.answerOffer(cur, 1, msg); toast(msg); }
        else if (button(328, by, 170, 16, "Négocier (+20 %)", false, open)) { g_career.answerOffer(cur, 2, msg); toast(msg); }
    }
    (void)sel;
    drawFooter("Retour");
    if (IN.back) g_screen = SC_CLUBMENU;
}

// ---------------- centre de formation : les U19 du club
static ListW g_acaLW;
// niveau du centre de formation : amélioration (coût, entretien mensuel) ; les jeunes formés au club progressent plus
static void academyPanel(int y) {
    int u = g_career.userTeam;
    Team& U = g_world.teams[u];
    static const char* LV[6] = { "aucun", "école de football", "centre régional", "centre agréé", "centre réputé", "centre d'élite" };
    int ac = std::min(5, (int)U.academy);
    DrawRectangle(12, y, VW - 24, 26, Color{ 20, 34, 64, 255 });
    drawTextPx(fmt("Centre de formation : niveau %d / 5 (%s)", ac, LV[ac]), 18, y + 3, 10, C_HI);
    for (int k = 0; k < 5; k++) DrawRectangle(260 + k * 12, y + 4, 10, 8, k < ac ? C_GOOD : Color{ 0, 0, 0, 120 });
    drawTextPx(fmt("Jeunes : potentiel +%d, niveau +%d. Entretien : %s / mois.", ac * 2, ac, money(ac * (U.status == CS_PRO ? 25 : 6)).c_str()), 18, y + 14, 10, C_DIM);
    if (ac < 5) {
        int64_t cost = (int64_t)(ac + 1) * (ac + 1) * (U.status == CS_PRO ? 150 : 40);
        if (button(VW - 200, y + 5, 180, 16, "Améliorer : " + money(cost))) {
            askConfirm(fmt("Améliorer le centre de formation (niveau %d) pour %s ?", ac + 1, money(cost).c_str()), [cost]() {
                Team& T = g_world.teams[g_career.userTeam];
                if (g_career.mgr.budget < cost) { toast("Budget insuffisant"); return; }
                g_career.mgr.budget -= cost; g_career.mgr.seasonStadiumCost += cost;
                T.academy = (uint8_t)std::min(5, T.academy + 1);
                addReputation(g_career.userTeam, 1);
                g_career.season.news.push_back(fmt("Le centre de formation passe au niveau %d.", (int)T.academy));
                toast("Centre de formation amélioré");
            }, "Améliorer", "Annuler");
        }
    }
    if (U.status == CS_PRO && ac == 0) drawTextPx("Obligatoire pour un club professionnel !", VW - 240, y + 14, 10, C_BAD);
}

static void screenAcademy() {
    drawBackground("Centre de formation");
    int u = g_career.userTeam, y19 = -1;
    academyPanel(VH - 44);
    for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].parent == u && g_world.teams[i].youth == 1) y19 = i;
    if (y19 < 0) {
        drawTextCentered("Le club n'a pas encore d'équipe U19.", VW / 2, 80, 10, C_TXT);
        drawTextCentered("Créez-la depuis l'écran Réserves (elle débute en District U19).", VW / 2, 96, 10, C_DIM);
        drawFooter("Retour");
        if (IN.back) g_screen = SC_CLUBMENU;
        return;
    }
    g_world.ensureSquad(y19);
    Team& Y = g_world.teams[y19];
    std::vector<int> ord; for (int i = 0; i < (int)Y.squad.size(); i++) ord.push_back(i);
    std::stable_sort(ord.begin(), ord.end(), [&](int a, int b) { return Y.squad[a].pot > Y.squad[b].pot; });
    drawTextPx(Y.name + "  -  " + g_career.teamLevelName(y19), 12, 31, 10, C_HI);
    drawTextPx(fmt("Formateur (staff jeunes) : niveau %d. À 20 ans, les joueurs quittent les U19 (réserve ou équipe première).", g_career.staffLevel(SR_YOUTH)), 12, 43, 10, C_DIM);
    drawTextPx("Joueur", 18, 58, 10, C_DIM); drawTextPx("Poste", 190, 58, 10, C_DIM); drawTextPx("Âge", 240, 58, 10, C_DIM);
    drawTextPx("Note", 280, 58, 10, C_DIM); drawTextPx("Potentiel", 320, 58, 10, C_DIM); drawTextPx("Avis du formateur", 400, 58, 10, C_DIM);
    static const char* PN[] = { "G", "D", "M", "A" };
    listRun(g_acaLW, (int)ord.size(), 12, 70, VW - 24, 16, 14, [&](int i, int x, int yy, bool s2) {
        const Player& P = Y.squad[ord[i]];
        Color c = s2 ? BLACK : C_TXT;
        drawTextPx(fitText(P.name, 165, 10), x + 6, yy + 2, 10, c);
        drawTextPx(PN[P.pos % 4], x + 184, yy + 2, 10, c);
        drawTextPx(fmt("%d", P.age), x + 228, yy + 2, 10, c);
        drawTextPx(fmt("%d", P.overall()), x + 268, yy + 2, 10, s2 ? BLACK : C_HI);
        int stars = P.pot >= 80 ? 5 : P.pot >= 72 ? 4 : P.pot >= 64 ? 3 : P.pot >= 56 ? 2 : 1;
        for (int k = 0; k < 5; k++) DrawRectangle(x + 308 + k * 9, yy + 4, 7, 7, k < stars ? Color{ 255, 210, 60, 255 } : Color{ 60, 70, 100, 255 });
        std::string av = stars >= 5 ? "Pépite : future star du club" : stars == 4 ? "Très prometteur" : stars == 3 ? "Bon potentiel" : stars == 2 ? "Joueur de complément" : "Doit progresser";
        drawTextPx(av, x + 388, yy + 2, 10, s2 ? BLACK : C_DIM);
    });
    int cur = g_acaLW.cur;
    int by = 70 + 16 * 14 + 6;
    if (cur >= 0 && cur < (int)ord.size()) {
        const Player& P = Y.squad[ord[cur]];
        if (button(12, by, 260, 16, "Promouvoir en équipe première")) {
            Team& U = g_world.teams[u];
            g_world.ensureSquad(u);
            if (U.squad.size() >= 32) toast("Effectif de l'équipe première complet (32)");
            else if (Y.squad.size() <= 14) toast("Effectif U19 trop réduit");
            else {
                Player q = P;
                std::vector<bool> used(100, false); for (auto& x : U.squad) if (x.num < 100) used[x.num] = true;
                if (q.num >= 100 || used[q.num]) { q.num = 1; while (q.num < 99 && used[q.num]) q.num++; }
                q.contract = (uint8_t)(U.status == CS_PRO ? 0 : U.status == CS_SEMIPRO ? 1 : 2);
                U.squad.push_back(q);
                Y.squad.erase(Y.squad.begin() + ord[cur]); Y.xi.clear();
                // un jeune du recrutement local prend sa place
                Player n = g_world.makeYouth(y19, q.pos, Y.rating - 4); n.age = 16;
                Y.squad.push_back(n);
                g_career.season.news.push_back(q.name + fmt(" (%d ans) est promu du centre de formation en équipe première.", q.age));
                toast(q.name + " rejoint l'équipe première");
            }
        }
    }
    drawFooter("OK / clic : choisir un jeune   Retour");
    if (IN.back) g_screen = SC_CLUBMENU;
}

// ---------------- entraînement (carrière)
static ListW g_trnLW;
static void screenTraining() {
    if(!careerRules()){g_screen=SC_HUB;return;}
    drawBackground("Entraînement");
    if(button(370,18,252,13,"Poste / rôle du joueur sélectionné")&&!g_world.teams[g_career.userTeam].squad.empty()){openPlayer(g_career.userTeam,std::clamp(g_trnLW.cur,0,(int)g_world.teams[g_career.userTeam].squad.size()-1),SC_TRAINING);g_rolePlayerTab=1;return;}
    ManagerState& M = g_career.mgr;
    int ti = g_career.userTeam;
    Team& T = g_world.teams[ti];
    int y = 32;
    drawTextPx("Programme :", 16, y + 3, 10, C_DIM);
    for (int i = 0; i < 6; i++) if (button(90 + i * 90, y, 86, 14, trainFocusName(i), M.trainFocus == i)) M.trainFocus = (uint8_t)i;
    y += 18;
    drawTextPx("Intensité :", 16, y + 3, 10, C_DIM);
    for (int i = 0; i < 3; i++) if (button(90 + i * 90, y, 86, 14, trainIntName(i), M.trainInt == i)) M.trainInt = (uint8_t)i;
    if (IN.left) M.trainInt = (uint8_t)std::max(0, M.trainInt - 1);
    if (IN.right) M.trainInt = (uint8_t)std::min(2, M.trainInt + 1);
    if (IN.tab) M.trainFocus = (uint8_t)((M.trainFocus + 1) % 6);
    y += 18;
    static const char* FD[] = { "Travail varié : progression répartie sur toutes les qualités.", "Vitesse et endurance : les joueurs tiennent mieux physiquement.",
                                "Passes et frappes : jeu au sol plus précis.", "Tacles et endurance : défense plus solide.", "Frappes et vitesse : attaquants plus efficaces.",
                                "Séances spécifiques des gardiens (les joueurs de champ travaillent l'endurance)." };
    static const char* ID[] = { "Récupération rapide, moral en hausse, progression lente, peu de blessures.", "Équilibre entre progression et fraîcheur.",
                                "Progression rapide mais fatigue accrue, récupération plus lente et risque de blessure." };
    drawTextPx(FD[M.trainFocus % 6], 16, y, 10, C_TXT); y += 11;
    drawTextPx(ID[M.trainInt % 3], 16, y, 10, C_DIM); y += 14;
    // moyenne condition / moral
    int cs = 0, ms = 0, n = 0;
    for (auto& p : T.squad) { cs += playerCond(ti, p); ms += p.morale; n++; }
    if (n) drawTextPx(fmt("Condition moyenne : %d %%    Moral moyen : %d %%    Préparateur physique : niveau %d", cs / n, ms / n, g_career.staffLevel(SR_PHYSIO_PREP)), 16, y, 10, C_HI);
    y += 14;
    const char* cols[] = { "ÂGE", "NOTE", "POT", "VIT", "TIR", "PAS", "TAC", "DRI", "TÊT", "END", "CONDITION", "MORAL" };
    int cx[] = { 190, 222, 254, 286, 314, 342, 370, 398, 426, 454, 490, 570 };
    drawTextPx("P   Joueur", 16, y, 10, C_DIM);
    for (int k = 0; k < 12; k++) drawTextPx(cols[k], cx[k], y, 10, C_DIM);
    y += 12;
    listRun(g_trnLW, (int)T.squad.size(), 12, y, VW - 24, std::max(4, (VH - 30 - y) / 12), 12, [&](int i, int x, int yy, bool sel) {
        const Player& p = T.squad[i];
        Color c = sel ? BLACK : C_TXT;
        drawTextPx(posName(p.pos), x + 4, yy + 1, 10, c);
        drawTextPx(fitText(p.name, 130, 10), x + 30, yy + 1, 10, c);
        int v[] = { p.age, p.overall(), p.pot, p.speed, p.shoot, p.pass, p.tackle, p.drib(), p.head(), p.stamina };
        for (int k = 0; k < 10; k++) drawTextPx(fmt("%d", v[k]), cx[k] - 12 + x + 4, yy + 1, 10, k == 1 ? (sel ? BLACK : C_HI) : c);
        int cd = playerCond(ti, p);
        Color cc = cd >= 80 ? C_GOOD : cd >= 60 ? C_HI : C_BAD;
        DrawRectangle(cx[10] - 12 + x + 4, yy + 3, 50, 5, Color{ 0, 0, 0, 120 });
        DrawRectangle(cx[10] - 12 + x + 4, yy + 3, cd / 2, 5, cc);
        drawTextPx(fmt("%d", cd), cx[10] + 46 + x, yy + 1, 10, sel ? BLACK : cc);
        Color mc = p.morale >= 70 ? C_GOOD : p.morale >= 45 ? C_HI : C_BAD;
        DrawRectangle(cx[11] - 12 + x + 4, yy + 3, 40, 5, Color{ 0, 0, 0, 120 });
        DrawRectangle(cx[11] - 12 + x + 4, yy + 3, p.morale * 2 / 5, 5, mc);
        if (p.injured > 0) drawTextPx("BLES", x + 160, yy + 1, 10, C_BAD);
    });
    // discours au groupe : une fois par mois
    static int talkMonth = -99, talkYear = -1;
    int month = (int)(g_career.season.now / 4.2);
    bool canTalk = !(talkMonth == month && talkYear == g_career.year);
    if (button(VW - 170, 68, 150, 14, "Discours au groupe", false, canTalk) && canTalk) {
        talkMonth = month; talkYear = g_career.year;
        int avg = 0; for (auto& p : T.squad) avg += p.morale; avg /= std::max(1, (int)T.squad.size());
        int gain = avg < 50 ? 8 : avg < 70 ? 5 : 2;
        bool flop = g_rng.chance(0.12f);
        for (auto& p : T.squad) p.morale = (uint8_t)std::max(10, std::min(100, p.morale + (flop ? -3 : gain)));
        toast(flop ? "Le discours passe mal... moral -3" : fmt("Le groupe est remotivé : moral +%d", gain));
    }
    drawFooter("Tab : programme   Gauche/Droite : intensité   La condition baisse après chaque match et remonte chaque semaine   Retour");
    if (IN.back) g_screen = SC_CLUBMENU;
}

// ---------------- mode entraînement (menu principal)
static ListW g_tmLW;
static int g_trainCtl = -1;      // contrôleur utilisé à l'entraînement (-1 : automatique, manette en priorité)
static void screenTrainMode() {
    drawBackground("Entraînement");
    if (g_trainTeam < 0) { g_screen = SC_MAIN; return; }
    const Team& T = g_world.teams[g_trainTeam];
    drawKitIcon(T.home, 20, 34, 2);
    drawTextPx(T.name, 44, 36, 10, C_HI);
    std::vector<int> devs; for (int d = 0; d < NUM_INPUTS; d++) if (inputAvailable(d)) devs.push_back(d);
    if (g_trainCtl < 0 || !inputAvailable(g_trainCtl)) g_trainCtl = IsGamepadAvailable(0) ? IN_PAD1 : IN_KB1;
    std::vector<std::string> items = { "Penalties : tirer face au gardien", "Coups francs directs (18 à 30 m)", "Corners : centres et reprises",
                                       "Attaque contre défense", "Gardien : arrêter des penalties", std::string("Commande : < ") + inputName(g_trainCtl) + " >", "Changer d'équipe", "Retour" };
    static const char* DESC[] = { "Visez avec la direction, maintenez Tir pour la puissance, relâchez pour frapper.", "Visée avec la direction, Tir (appui long) pour frapper, Passe ou Lob pour jouer court/long.",
                                  "Centre au bouton lob, reprise de la tête ou de volée dans la surface.", "Construisez une action à 40 m du but contre toute la défense (25 s par essai).",
                                  "Orientez le plongeon du gardien au moment de la frappe.", "Gauche/Droite ou OK : clavier ou manette utilisé pendant l'exercice.", "", "" };
    int s = menuRun(g_tmLW, items, 64, 340);
    int cur = std::max(0, std::min(7, g_tmLW.cur));
    if (DESC[cur][0]) drawTextCentered(DESC[cur], VW / 2, 64 + 8 * 20 + 8, 10, C_DIM);
    // rappel des touches du contrôleur choisi
    {
        std::string help = inputHelp(g_trainCtl);
        drawTextCentered(fitText(help, VW - 20, 10), VW / 2, 64 + 8 * 20 + 20, 10, C_TXT);
    }
    drawFooter("Le compteur de réussite s'affiche en haut à gauche   Pause : changer d'exercice");
    if (cur == 5 && (IN.left || IN.right) && !devs.empty()) {
        int k = 0; for (int i = 0; i < (int)devs.size(); i++) if (devs[i] == g_trainCtl) k = i;
        k = (k + (IN.left ? -1 : 1) + (int)devs.size()) % (int)devs.size(); g_trainCtl = devs[k];
    }
    if (IN.back || s == 7) { g_screen = SC_MAIN; return; }
    if (s == 5) { int k = 0; for (int i = 0; i < (int)devs.size(); i++) if (devs[i] == g_trainCtl) k = i; g_trainCtl = devs[(k + 1) % devs.size()]; return; }
    if (s == 6) { openPick(PM_TRAIN); return; }
    if (s < 0) return;
    MatchSetup ms;
    ms.home = g_trainTeam; ms.away = g_trainTeam;
    for (int d = 0; d < NUM_INPUTS; d++) ms.side[d] = -1;
    ms.side[g_trainCtl] = 0;
    ms.training = s + 1;
    ms.title = "Entraînement"; ms.stadium = "Centre d'entraînement";
    ms.snes = g_settings.controlStyle == 1;
    ms.difficulty = g_settings.difficulty;
    ms.halfSeconds = 600; ms.crowdFill = 0.02f; ms.commentary = false; ms.pitch = 0;
    ms.referee = 0;
    g_mctx = MatchCtx();
    g_paused = false;
    g_match.reset(new Match());
    g_match->init(ms);
    g_screen = SC_MATCH;
}

// ---------------- staff
static int g_stfRole = 0;
static ListW g_stfCandLW;
static std::vector<StaffMember> candidatesFor(int role) {
    std::vector<StaffMember> v;
    const Team& U = g_world.teams[g_career.userTeam];
    Rng r((uint64_t)(g_career.year * 131 + role * 7 + g_career.mgr.lastMonth * 3 + 11));
    int maxL = U.status == CS_PRO ? 5 : U.status == CS_SEMIPRO ? 4 : 3;
    // membres créés avec l'éditeur (sans club, ou de ce club) en tête des candidats
    for (auto& c : customStaff()) {
        if (c.role != role || (!c.club.empty() && c.club != U.name) || v.size() >= 3) continue;
        bool emp = false; for (auto& s : g_career.mgr.staff) if (c.name == s.name) emp = true;
        if (emp) continue;
        StaffMember m = makeStaff(role, c.level, U.status, r);
        snprintf(m.name, sizeof m.name, "%s", c.name.c_str()); m.age = c.age;
        v.push_back(m);
    }
    for (int k = (int)v.size(); k < 5; k++) v.push_back(makeStaff(role, std::max(1, std::min(maxL, 1 + k + r.range(-1, 0))), U.status, r));
    return v;
}

static void screenStaff() {
    drawBackground("Staff technique");
    ManagerState& M = g_career.mgr;
    const Team& U = g_world.teams[g_career.userTeam];
    int64_t tot = 0; for (auto& s : M.staff) tot += s.wage;
    drawTextPx(fmt("Salaires du staff : %s / an   -   Budget : %s   -   Statut %s", money(tot).c_str(), money(M.budget).c_str(), statusName(U.status)), 12, 31, 10, C_TXT);
    int y = 46;
    for (int r = 0; r < NUM_SR; r++) {
        int yy = y + r * 26;
        bool sel = r == g_stfRole;
        bool hover = IN.mouse.x >= 12 && IN.mouse.x < 330 && IN.mouse.y >= yy && IN.mouse.y < yy + 24;
        if (hover && IN.click) { g_stfRole = r; g_stfCandLW = ListW(); }
        DrawRectangle(12, yy, 318, 24, sel ? Color{ 90, 80, 30, 255 } : C_ITEM);
        drawTextPx(staffRoleName(r), 18, yy + 2, 10, sel ? C_HI : C_TXT);
        const StaffMember* m = nullptr; for (auto& s : M.staff) if (s.role == r) m = &s;
        if (m) {
            drawTextPx(fitText(std::string(m->name) + (m->gender ? " (F)" : " (H)"), 150, 10), 18, yy + 13, 10, C_DIM);
            for (int k = 0; k < 5; k++) DrawRectangle(180 + k * 9, yy + 15, 7, 6, k < m->level ? C_GOOD : Color{ 0, 0, 0, 120 });
            drawTextPx(m->wage ? money(m->wage) : "bénévole", 232, yy + 13, 10, C_DIM);
        } else drawTextPx("(poste vacant)", 18, yy + 13, 10, C_BAD);
    }
    if (IN.up) { g_stfRole = (g_stfRole + NUM_SR - 1) % NUM_SR; g_stfCandLW = ListW(); }
    if (IN.down) { g_stfRole = (g_stfRole + 1) % NUM_SR; g_stfCandLW = ListW(); }
    // candidats
    int x = 340;
    drawSection(x, y, VW - x - 10, "CANDIDATS");
    drawTextPx(fitText(staffRoleDesc(g_stfRole), VW - x - 10, 10), x, y + 12, 10, C_DIM);
    auto cand = candidatesFor(g_stfRole);
    for (int k = 0; k < (int)cand.size(); k++) {
        const StaffMember& c = cand[k];
        int yy = y + 28 + k * 24;
        DrawRectangle(x, yy, VW - x - 10, 22, C_ITEM);
        drawTextPx(fitText(c.name, 130, 10), x + 4, yy + 2, 10, C_TXT);
        drawTextPx(fmt("%d ans - %s", c.age, c.gender ? "femme" : "homme"), x + 4, yy + 11, 10, C_DIM);
        for (int l = 0; l < 5; l++) DrawRectangle(x + 140 + l * 9, yy + 4, 7, 6, l < c.level ? C_GOOD : Color{ 0, 0, 0, 120 });
        drawTextPx(c.wage ? money(c.wage) + "/an" : "bénévole", x + 140, yy + 11, 10, C_DIM);
        if (button(VW - 90, yy + 3, 74, 16, "Engager")) {
            StaffMember cc = c;
            askConfirm(fmt("Engager %s (%s, niveau %d) pour %s par an ?", cc.name, staffRoleName(cc.role), cc.level, cc.wage ? money(cc.wage).c_str() : "0"), [cc]() {
                auto& st = g_career.mgr.staff;
                st.erase(std::remove_if(st.begin(), st.end(), [&](const StaffMember& s) { return s.role == cc.role; }), st.end());
                st.push_back(cc);
                toast("Nouveau membre du staff engagé");
            }, "Engager", "Annuler");
        }
    }
    bool has = false; for (auto& s : M.staff) if (s.role == g_stfRole) has = true;
    if (has && button(12, y + NUM_SR * 26 + 4, 200, 16, "Se séparer du titulaire du poste")) {
        int role = g_stfRole;
        askConfirm("Se séparer de ce membre du staff ?", [role]() { auto& st = g_career.mgr.staff; st.erase(std::remove_if(st.begin(), st.end(), [&](const StaffMember& s) { return s.role == role; }), st.end()); });
    }
    if(has&&button(225,258,105,17,"Prolonger")){g_career.directorSync();for(auto& c:g_career.director.staffContracts)if(c.role==g_stfRole)c.years=3;if(g_stfRole==SR_DIRECTOR)g_career.director.profile.years=3;toast("Contrat du staff renouvelé pour trois saisons.");}
    if(g_stfRole==SR_DIRECTOR&&has&&button(12,291,200,18,"Gestion du directeur sportif")){openDirector();return;}
    drawFooter("Haut/Bas : poste   Clic : engager un candidat   Retour");
    if (IN.back) g_screen = SC_CLUBMENU;
}

// ---------------- réserves
static ListW g_resLW, g_resPlLW;
static int g_resPromote = -1;           // réserve dont on consulte les joueurs (promotion en équipe A)
static void promoteToFirst(int from, int idx) {
    int u = g_career.userTeam;
    Team& R = g_world.teams[from]; Team& A = g_world.teams[u];
    if (idx < 0 || idx >= (int)R.squad.size()) return;
    if ((int)A.squad.size() >= 32) { toast("Effectif de l'équipe A complet (32 joueurs)"); return; }
    Player p = R.squad[idx];
    std::vector<bool> used(100, false); for (auto& q : A.squad) if (q.num < 100) used[q.num] = true;
    if (p.num >= 100 || used[p.num]) { p.num = 1; while (p.num < 99 && used[p.num]) p.num++; }
    if (A.status == CS_PRO && p.contract == 2) p.contract = 1;          // premier contrat (fédéral) en montant en équipe A
    A.squad.push_back(p);
    R.squad.erase(R.squad.begin() + idx); R.xi.clear();
    if ((int)R.squad.size() < 14) R.squad.push_back(g_world.makeYouth(from, p.pos, R.rating - 3));   // un jeune le remplace
    g_career.season.news.push_back(p.name + " monte de " + R.name + " en équipe première.");
    toast(p.name + " rejoint l'équipe A");
}
static void screenReserves() {
    int u = g_career.userTeam;
    if (g_resPromote >= 0) {
        int t = g_resPromote;
        g_world.ensureSquad(t);
        Team& R = g_world.teams[t];
        drawBackground("Joueurs de " + R.name);
        drawTextPx("OK / clic : faire monter le joueur en équipe première (" + g_world.teams[u].name + ")", 12, 31, 10, C_DIM);
        std::vector<int> ord; for (int i = 0; i < (int)R.squad.size(); i++) ord.push_back(i);
        std::stable_sort(ord.begin(), ord.end(), [&](int a, int b) { return R.squad[a].overall() > R.squad[b].overall(); });
        drawTextPx("Joueur", 36, 44, 10, C_DIM); drawTextPx("Âge", 250, 44, 10, C_DIM); drawTextPx("Note", 300, 44, 10, C_DIM); drawTextPx("Potentiel", 350, 44, 10, C_DIM);
        int s = listRun(g_resPlLW, (int)ord.size(), 12, 56, VW - 24, 18, 15, [&](int i, int x, int y, bool sl) {
            const Player& p = R.squad[ord[i]];
            drawTextPx(posName(p.pos), x + 4, y + 2, 10, sl ? BLACK : C_DIM);
            drawTextPx(p.name, x + 24, y + 2, 10, sl ? BLACK : C_TXT);
            drawTextPx(fmt("%d", p.age), x + 238, y + 2, 10, sl ? BLACK : C_TXT);
            drawTextPx(fmt("%d", p.overall()), x + 288, y + 2, 10, sl ? BLACK : C_HI);
            drawTextPx(fmt("%d", p.pot), x + 338, y + 2, 10, sl ? BLACK : C_GOOD);
        });
        if (s >= 0 && s < (int)ord.size()) promoteToFirst(t, ord[s]);
        drawFooter("OK : monter en équipe A   Retour");
        if (IN.back) g_resPromote = -1;
        return;
    }
    drawBackground("Équipes réserves");
    std::vector<int> res;
    for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].parent == u) res.push_back(i);
    bool inter = true; // Requests can be made all season; sporting registration is next season.
    drawTextPx("3 réserves maximum + U19, U17, U15. Une nouvelle équipe débute la saison suivante en dernière division du district.", 12, 31, 10, C_DIM);
    int sel = listRun(g_resLW, (int)res.size(), 12, 46, VW - 24, 10, 16, [&](int i, int x, int y, bool s) {
        int t = res[i];
        const Team& T = g_world.teams[t];
        bool ctrl = std::find(g_career.mgr.ctrlReserves.begin(), g_career.mgr.ctrlReserves.end(), t) != g_career.mgr.ctrlReserves.end();
        bool pend = std::find(g_career.pendingNewClubs.begin(), g_career.pendingNewClubs.end(), t) != g_career.pendingNewClubs.end();
        drawKitIcon(T.home, x + 3, y + 3, 1);
        drawTextPx(T.name, x + 18, y + 3, 10, s ? BLACK : C_TXT);
        drawTextPx(fitText(pend ? std::string("engagée la saison prochaine") : g_career.teamLevelName(t), 250, 10), x + 200, y + 3, 10, s ? BLACK : C_DIM);
        drawTextPx(ctrl ? "Vous la dirigez" : "Dirigée par le staff", x + VW - 180, y + 3, 10, s ? BLACK : (ctrl ? C_HI : C_DIM));
    });
    if (res.empty()) drawTextCentered("Aucune équipe réserve.", VW / 2, 70, 10, C_DIM);
    int by = 46 + 10 * 16 + 10;
    int nRes = 0; bool hasU19 = false;
    for (int t : res) { if (g_world.teams[t].youth) hasU19 = true; else nRes++; }
    if (button(12, by, 220, 16, "Créer une équipe réserve", false, inter && nRes < 3)) {
        std::string err;
        int t = g_career.createReserve(err);
        if (t < 0) toast(err); else { toast(g_world.teams[t].name + " créée"); levelCacheDirty(); }
    }
    {   // équipes de jeunes : U19, U17, U15
        bool hasY[4] = { false, false, false, false };
        for (int t : res) if (g_world.teams[t].youth >= 1 && g_world.teams[t].youth <= 3) hasY[g_world.teams[t].youth] = true;
        static const char* YN[4] = { "", "U19", "U17", "U15" };
        int bx = VW - 330;
        for (int k = 1; k <= 3; k++) {
            if (hasY[k]) continue;
            if (button(bx, by, 104, 16, std::string("Créer les ") + YN[k], false, inter)) {
                std::string err;
                int t = g_career.createYouth(u, k, err);
                if (t < 0) toast(err); else { toast(g_world.teams[t].name + " créée : début en District " + YN[k]); levelCacheDirty(); }
            }
            bx += 108;
        }
        (void)hasU19;
    }
    if (!inter) drawTextPx("Création possible à l'intersaison (août ou fin de saison).", 240, by + 3, 10, C_DIM);
    int cur = g_resLW.cur;
    if (cur >= 0 && cur < (int)res.size()) {
        int t = res[cur];
        auto& cr = g_career.mgr.ctrlReserves;
        bool ctrl = std::find(cr.begin(), cr.end(), t) != cr.end();
        if (button(12, by + 22, 220, 16, ctrl ? "Confier l'équipe au staff" : "Prendre le contrôle de l'équipe")) {
            if (ctrl) cr.erase(std::find(cr.begin(), cr.end(), t)); else cr.push_back(t);
            g_career.syncControlled();
            g_needAdvance = true;
            toast(ctrl ? "L'équipe est confiée au staff" : "Vous jouerez aussi les matchs de cette équipe");
        }
        if (button(240, by + 22, 150, 16, "Fiche")) { openFiche(t, SC_RESERVES); return; }
        if (button(398, by + 22, 220, 16, "Joueurs / monter en équipe A")) { g_resPromote = t; g_resPlLW = ListW(); return; }
    }
    drawTextPx(fmt("Entraîneur de la réserve (staff) : niveau %d", g_career.staffLevel(SR_RESERVE)), 12, by + 46, 10, C_DIM);
    drawTextPx("Transferts de joueurs entre équipes : écran Effectif (PgPréc/PgSuiv pour changer d'équipe).", 12, by + 58, 10, C_DIM);
    (void)sel;
    drawFooter("Retour");
    if (IN.back) g_screen = SC_CLUBMENU;
}

// ---------------- matchs amicaux
static void screenFriendlies() {
    drawBackground("Matchs amicaux");
    int y = 40;
    drawTextPx("Invitez 1 club pour un match amical, ou 3 clubs pour un tournoi amical (demi-finales puis finale).", 12, y, 10, C_TXT); y += 14;
    drawTextPx("Les matchs amicaux se jouent dès la prochaine date disponible ; les recettes vont au club qui reçoit.", 12, y, 10, C_DIM); y += 20;
    std::string inv;
    for (int t : g_inviteSel) inv += (inv.empty() ? "" : ", ") + g_world.teams[t].name;
    drawTextPx("Clubs invités : " + (inv.empty() ? std::string("aucun") : inv), 12, y, 10, C_HI); y += 20;
    if (button(12, y, 200, 18, "Choisir les clubs invités")) { openPick(PM_INVITE); return; }
    if (button(220, y, 200, 18, "Organiser le match amical", false, g_inviteSel.size() == 1)) {
        g_career.addFriendly(g_inviteSel, false); g_inviteSel.clear(); g_needAdvance = true; toast("Match amical programmé"); openHub(); return;
    }
    if (button(428, y, 200, 18, "Organiser le tournoi", false, g_inviteSel.size() == 3)) {
        g_career.addFriendly(g_inviteSel, true); g_inviteSel.clear(); g_needAdvance = true; toast("Tournoi amical programmé"); openHub(); return;
    }
    drawFooter("Retour");
    if (IN.back) g_screen = SC_CLUBMENU;
}

// ---------------- arbitres
static ListW g_refLW;
static void screenReferees() {
    drawBackground("Arbitres");
    int y = 32;
    drawTextPx("Arbitre", 16, y, 10, C_DIM); drawTextPx("Catégorie", 200, y, 10, C_DIM); drawTextPx("Sévérité", 290, y, 10, C_DIM);
    drawTextPx("Avantage", 380, y, 10, C_DIM); drawTextPx("Influence du public", 470, y, 10, C_DIM);
    y += 12;
    auto bar = [](int x, int y, int v, Color c) { DrawRectangle(x, y + 3, 60, 5, Color{ 0, 0, 0, 120 }); DrawRectangle(x, y + 3, v * 60 / 100, 5, c); };
    listRun(g_refLW, NUM_REFEREES, 12, y, VW - 24, 24, 12, [&](int i, int x, int yy, bool s) {
        const Referee& R = REFEREES[i];
        static const char* CAT[] = { "", "Fédéral 3", "Fédéral 2", "Fédéral 1", "Ligue 2", "Élite / FIFA" };
        drawTextPx(R.name, x + 4, yy + 1, 10, s ? BLACK : C_TXT);
        drawTextPx(CAT[R.level], x + 188, yy + 1, 10, s ? BLACK : C_DIM);
        bar(x + 278, yy, R.severity, R.severity > 70 ? C_BAD : C_HI);
        bar(x + 368, yy, R.advantage, C_GOOD);
        bar(x + 458, yy, R.homeBias, R.homeBias > 60 ? C_BAD : C_DIM);
    });
    drawFooter("Sévérité : fréquence des cartons   Avantage : laisse jouer   Influence du public : favorise l'équipe qui reçoit   Retour");
    if (IN.back) openHub();
}
