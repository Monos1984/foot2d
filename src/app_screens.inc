static bool g_edU19 = true;     // création d'un club : avec une équipe U19
// ------------------------------------------------------------------ outils communs (fiches, éditeurs)
static std::string regionName(int r) { return r >= 0 && r < NUM_REGIONS ? sanitize(REGIONS[r].name) : std::string(); }
static std::string deptName(int d) { return d >= 0 && d < NUM_DEPTS ? fmt("%s (%s)", sanitize(DEPTS[d].name).c_str(), DEPTS[d].code) : std::string(); }

static int frMetroPyr(const std::vector<Pyramid>& P) {
    for (int i = 0; i < (int)P.size(); i++) if (P[i].country == "FRA" && P[i].dom < 0) return i;
    return -1;
}

// division d'une équipe (carrière en cours sinon pyramide de départ)
static std::string divisionOf(int team) {
    const Team& t = g_world.teams[team];
    if (t.kind != TK_CLUB) return "Sélection nationale";
    if (g_careerActive && g_career.kind == CK_CLUB) {
        std::string s = g_career.teamLevelName(team);
        if (!s.empty()) return s;
        if (std::find(g_career.pendingNewClubs.begin(), g_career.pendingNewClubs.end(), team) != g_career.pendingNewClubs.end())
            return "Nouveau club : débutera la saison prochaine";
    }
    for (auto& Y : g_basePyramids)
        for (auto& pl : Y.pools)
            for (int g = 0; g < (int)pl.groups.size(); g++)
                for (int x : pl.groups[g]) if (x == team) return poolLabel(Y, pl, g);
    if (t.custom && t.dept >= 0) {
        int fr = frMetroPyr(g_basePyramids);
        if (fr >= 0) {
            const Pyramid& Y = g_basePyramids[fr];
            int best = -1;
            for (int q = 0; q < (int)Y.pools.size(); q++)
                if (Y.tiers[Y.pools[q].tier].scope == SC_DEPT && Y.pools[q].key == t.district && (best < 0 || Y.pools[q].tier > Y.pools[best].tier)) best = q;
            if (best >= 0) return "Débutera en " + poolLabel(Y, Y.pools[best], 0);
        }
    }
    if (t.nation >= 0) return "Championnat de " + g_world.teams[t.nation].name;
    return "-";
}

// saisie de texte UTF-8 (caractères latins)
static void textEdit(std::string& s, size_t maxLen) {
    int cp;
    while ((cp = GetCharPressed()) > 0) {
        if (cp < 32 || cp == '|' || cp > 255) continue;
        int n = 0; const char* u = CodepointToUTF8(cp, &n);
        if (s.size() + n <= maxLen) s.append(u, n);
    }
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
        while (!s.empty()) { unsigned char c = (unsigned char)s.back(); s.pop_back(); if ((c & 0xC0) != 0x80) break; }
    }
}

static const unsigned PALETTE[] = { 0xFFFFFF, 0x101010, 0xE2001A, 0x8B1020, 0x7A1438, 0x1B2A63, 0x0055A4, 0x6CABDD, 0x00843D, 0x3FAE49,
                                    0xFFD700, 0xFF8C00, 0x6A2C91, 0x808080, 0x2BB5B0, 0xF3E5C0, 0x5B3A1A, 0xFF69B4 };
static const int NUM_PALETTE = sizeof(PALETTE) / sizeof(PALETTE[0]);
static const char* PALETTE_NAMES[] = { "Blanc", "Noir", "Rouge", "Grenat", "Bordeaux", "Bleu marine", "Bleu", "Bleu ciel", "Vert", "Vert clair",
                                       "Jaune", "Orange", "Violet", "Gris", "Turquoise", "Crème", "Marron", "Rose" };
static const char* PATTERN_NAMES[NUM_PATTERNS] = { "Uni", "Rayures verticales", "Cerceaux", "Moitiés", "Écharpe", "Damier", "Manches contrastées",
                                              "Fines rayures", "Quartiers", "Bande pectorale", "Épaules contrastées", "Chevron", "Croix", "Diagonale" };
static int paletteIndex(unsigned c) {
    int best = 0; long long bd = 1LL << 40;
    for (int i = 0; i < NUM_PALETTE; i++) {
        long long dr = (long long)((c >> 16) & 255) - ((PALETTE[i] >> 16) & 255), dg = (long long)((c >> 8) & 255) - ((PALETTE[i] >> 8) & 255), db = (long long)(c & 255) - (PALETTE[i] & 255);
        long d = dr * dr + dg * dg + db * db;
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}
static std::string colorName(unsigned c) {
    int i = paletteIndex(c);
    return PALETTE[i] == c ? PALETTE_NAMES[i] : fmt("#%06X", c);
}

// ------------------------------------------------------------------ fiche d'un club / d'une sélection
static ListW g_ficheLW;
static std::string g_ficheDiv;
static std::vector<std::pair<int, std::string>> g_ficheRelated;   // équipe première / réserves
static int g_fichePage = 0;

static std::string districtLabel(int d) { return d >= 0 ? districtFullName(d) + " - Ligue " + regionName(districtRegion(d)) : std::string(); }
static void openFiche(int team, Screen back) {
    if (team < 0 || team >= (int)g_world.teams.size()) return;
    g_ficheTeam = team; g_ficheBack = back;
    g_world.ensureSquad(team);
    g_ficheLW = ListW();
    g_fichePage = 0;
    g_ficheDiv = divisionOf(team);
    g_ficheRelated.clear();
    const Team& t = g_world.teams[team];
    if (t.kind == TK_CLUB) {
        int root = t.parent >= 0 ? t.parent : team;
        if (root != team) g_ficheRelated.push_back({ root, "Équipe première" });
        for (int i = 0; i < (int)g_world.teams.size(); i++)
            if (i != team && g_world.teams[i].parent == root) g_ficheRelated.push_back({ i, g_world.teams[i].resLevel >= 2 ? "Équipe réserve (C)" : "Équipe réserve" });
    }
    g_screen = SC_FICHE;
}

// archives d'un club : records et parcours (carrière), buteurs et passeurs de l'histoire du club
static void drawClubArchive(int team, int y0) {
    const Career& K = g_career;
    auto it = K.archive.find(team);
    int x = 12, y = y0;
    drawSection(x, y, 300, "RECORDS DU CLUB (depuis le début de la carrière)"); y += 12;
    auto row = [&](const std::string& k, const std::string& v) { drawTextPx(k, x + 4, y, 10, C_DIM); drawTextPx(fitText(v, 190, 10), x + 110, y, 10, C_TXT); y += 11; };
    if (it == K.archive.end()) { drawTextPx("Aucune archive : les données sont enregistrées à chaque fin de saison.", x + 4, y, 10, C_DIM); y += 11; }
    else {
        const ClubArch& A = it->second;
        auto run = [&](const char* k, const ArchRun& r) { if (r.depth >= 0) row(k, fmt("%s (%d)", r.label, r.year)); };
        if (A.lgTier < 99) row("Meilleur classement", fmt("%d%s en %s (%d)", A.lgPos, A.lgPos == 1 ? "er" : "e", A.lgName, A.lgYear));
        run("Coupe de France", A.cdf);
        run("Coupe de la Ligue", A.cdl);
        run("Coupe régionale", A.reg);
        run("Coupe de district", A.dist);
        run("Méga Coupe rég.", A.regSuper);
        run("Ligue des champions", A.ucl);
        run("Coupe UEFA", A.uefa);
        if (A.bigWinGF >= 0 && A.bigWinOpp >= 0) row("Plus large victoire", fmt("%d-%d contre ", A.bigWinGF, A.bigWinGA) + g_world.teams[A.bigWinOpp].shortName + fmt(" (%d)", A.bigWinYear));
        if (A.bigLossGA >= 0 && A.bigLossOpp >= 0) row("Plus lourde défaite", fmt("%d-%d contre ", A.bigLossGF, A.bigLossGA) + g_world.teams[A.bigLossOpp].shortName + fmt(" (%d)", A.bigLossYear));
        row("Saisons / titres", fmt("%d saison%s, %d titre%s de champion", A.seasons, A.seasons > 1 ? "s" : "", A.titles, A.titles > 1 ? "s" : ""));
    }
    const Team& T = g_world.teams[team];
    if (T.sta.bestAtt > 0) row("Record d'affluence", fmt("%d spectateurs", T.sta.bestAtt));
    // buteurs et passeurs : archives + saison en cours
    std::vector<ArchScorer> v;
    auto is = K.archScorers.find(team);
    if (is != K.archScorers.end()) v = is->second;
    for (auto& p : T.squad) {
        if (!p.goals && !p.assists) continue;
        ArchScorer* e = nullptr; for (auto& a : v) if (a.pid == p.id) e = &a;
        if (!e) { ArchScorer n; n.pid = p.id; snprintf(n.name, sizeof n.name, "%s", p.name.c_str()); v.push_back(n); e = &v.back(); }
        e->goals += p.goals; e->assists += p.assists; e->apps += p.apps;
    }
    int x2 = 324, y2 = y0;
    drawSection(x2, y2, VW - x2 - 8, "MEILLEURS BUTEURS"); y2 += 12;
    std::vector<ArchScorer> g = v; std::sort(g.begin(), g.end(), [](const ArchScorer& a, const ArchScorer& b) { return a.goals > b.goals; });
    for (int i = 0; i < (int)g.size() && i < 6 && g[i].goals > 0; i++) { drawTextPx(fmt("%d. %s", i + 1, g[i].name), x2 + 4, y2, 10, i == 0 ? C_HI : C_TXT); drawTextPx(fmt("%d buts", g[i].goals), VW - 70, y2, 10, C_DIM); y2 += 11; }
    if (g.empty() || g[0].goals == 0) { drawTextPx("-", x2 + 4, y2, 10, C_DIM); y2 += 11; }
    y2 += 4;
    drawSection(x2, y2, VW - x2 - 8, "MEILLEURS PASSEURS"); y2 += 12;
    std::sort(g.begin(), g.end(), [](const ArchScorer& a, const ArchScorer& b) { return a.assists > b.assists; });
    for (int i = 0; i < (int)g.size() && i < 6 && g[i].assists > 0; i++) { drawTextPx(fmt("%d. %s", i + 1, g[i].name), x2 + 4, y2, 10, i == 0 ? C_HI : C_TXT); drawTextPx(fmt("%d passes", g[i].assists), VW - 70, y2, 10, C_DIM); y2 += 11; }
    if (g.empty() || g[0].assists == 0) drawTextPx("-", x2 + 4, y2, 10, C_DIM);
}

static void screenFiche(float dt) {
    static float anim = 0; anim += dt;
    Team& t = g_world.teams[g_ficheTeam];
    drawBackground(fitText(t.name, VW - 120, 20));
    if (!t.sponsor.empty()) {
        Texture2D* lg = sponsorLogoFwd(t.sponsor);
        DrawRectangle(VW - 124, 32, 114, 36, Color{ 235, 235, 240, 255 });
        if (lg) drawLogo2(lg, VW - 120, 34, 106, 24); else drawTextCentered(fitText(t.sponsor, 106, 10), VW - 67, 40, 10, BLACK, false);
        drawTextCentered("Sponsor maillot", VW - 67, 58, 10, Color{ 80, 80, 90, 255 }, false);
    }
    // maillots
    DrawRectangle(10, 32, 128, 92, Color{ 10, 18, 36, 200 });
    int fr = (int)(anim * 6);
    if (t.hasThird) {
        drawPlayerSprite(32, 100, t.home, 2, 1, 0, fr, PS_NORMAL, false, 0, 4);
        drawPlayerSprite(74, 100, t.away, 1, 2, 0, fr + 2, PS_NORMAL, false, 0, 4);
        drawPlayerSprite(116, 100, t.third, 3, 0, 0, fr + 1, PS_NORMAL, false, 0, 4);
        drawTextCentered("Dom.", 32, 108, 10, C_DIM, false); drawTextCentered("Ext.", 74, 108, 10, C_DIM, false); drawTextCentered("3e", 116, 108, 10, C_DIM, false);
    } else {
        drawPlayerSprite(42, 100, t.home, 2, 1, 0, fr, PS_NORMAL, false, 0, 5);
        drawPlayerSprite(104, 100, t.away, 1, 2, 0, fr + 2, PS_NORMAL, false, 0, 5);
        drawTextCentered("Domicile", 42, 108, 10, C_DIM, false);
        drawTextCentered("Extérieur", 104, 108, 10, C_DIM, false);
    }
    // informations
    int x = 148, y = 34;
    auto line = [&](const std::string& k, const std::string& v, Color c = C_TXT) {
        if (v.empty()) return;
        drawTextPx(k, x, y, 10, C_DIM);
        drawTextPx(fitText(v, VW - x - 110, 10), x + 92, y, 10, c);
        y += 12;
    };
    line("Nom court", t.shortName);
    line("Stade", (t.stadium.empty() ? std::string("-") : t.stadium) + (t.sta.init ? fmt("  (%d places)", t.sta.capacity()) : std::string("")) + (t.sta.namingIncome > 0 ? std::string("  - ") + t.sta.sponsor : std::string("")));
    if (t.kind == TK_CLUB) line("Statut", statusName(t.status));
    if (t.kind == TK_CLUB) {
        if (t.dept < 0 || t.town != sanitize(DEPTS[t.dept].name)) line("Ville", t.town);
        line("District", districtLabel(t.district));
        line("Département", deptName(t.dept));
        line("Ligue / région", regionName(t.region));
        if (t.nation >= 0) line("Pays", g_world.teams[t.nation].name);
        line("Division", g_ficheDiv, C_HI);
        if (t.founded > 0) line("Fondé en", fmt("%d", t.founded));
        if (t.parent < 0) line(t.presGender ? "Présidente" : "Président", presidentName(g_ficheTeam) + fmt(" (%d ans, né à ", presidentAge(g_ficheTeam)) + presidentPlace(g_ficheTeam) + ")");
        if (t.custom) line("Origine", "Club créé avec l'éditeur", C_GOOD);
        else if (t.edited) line("Origine", "Club modifié avec l'éditeur", C_GOOD);
    } else {
        line("Confédération", NATIONS[g_ficheTeam].conf >= 0 && NATIONS[g_ficheTeam].conf < NUM_CONFEDS ? CONF_NAMES[NATIONS[g_ficheTeam].conf] : "-");
        line("Statut", g_ficheDiv);
    }
    line("Note", fmt("%d / 100", (int)t.rating), C_HI);
    // jauge de note
    DrawRectangle(x + 92, y, 150, 5, Color{ 0, 0, 0, 120 });
    DrawRectangle(x + 92, y, (int)(150 * std::min(100.f, t.rating) / 100.f), 5, t.rating >= 75 ? C_GOOD : t.rating >= 50 ? C_HI : C_BAD);
    y += 10;
    int yInfoEnd = std::max(y, 130);
    // onglets
    const char* tabs[] = { "Effectif", "Palmarès", "Réserves / club", "Archives", "Calendrier" };
    bool clubCareer = t.kind == TK_CLUB && g_careerActive && g_career.kind == CK_CLUB;
    int nTabs = clubCareer ? 5 : g_careerActive ? 3 : 3;
    int tabIds[5] = { 0, 1, 2, 3, 4 };
    if (!clubCareer && g_careerActive) { nTabs = 4; tabIds[3] = 4; }       // sélections, autres modes : calendrier sans archives
    int curTab = 0; for (int i = 0; i < nTabs; i++) if (tabIds[i] == g_fichePage) curTab = i;
    if (g_fichePage != tabIds[curTab]) g_fichePage = tabIds[0];
    for (int i = 0; i < nTabs; i++) {
        int tx = 12 + i * 110;
        DrawRectangle(tx, yInfoEnd, 106, 13, g_fichePage == tabIds[i] ? C_SEL : C_ITEM);
        drawTextCentered(tabs[tabIds[i]], tx + 53, yInfoEnd + 2, 10, g_fichePage == tabIds[i] ? BLACK : C_TXT, false);
        if (IN.click && IN.mouse.x >= tx && IN.mouse.x < tx + 106 && IN.mouse.y >= yInfoEnd && IN.mouse.y < yInfoEnd + 13) { g_fichePage = tabIds[i]; g_ficheLW = ListW(); }
    }
    if (IN.left) { g_fichePage = tabIds[(curTab + nTabs - 1) % nTabs]; g_ficheLW = ListW(); }
    if (IN.right) { g_fichePage = tabIds[(curTab + 1) % nTabs]; g_ficheLW = ListW(); }
    int ly = yInfoEnd + 16;
    int rows = (VH - 18 - ly) / 12;
    static const char* PN[] = { "G", "D", "M", "A" };
    int act = -1;
    if (g_fichePage == 0) {
        drawTextPx("N°  Poste  Joueur", 16, ly, 10, C_DIM);
        const char* cols[] = { "VIT", "TIR", "PAS", "TAC", "GAR", "NOTE" };
        for (int k = 0; k < 6; k++) drawTextPx(cols[k], 330 + k * 44, ly, 10, C_DIM);
        ly += 12; rows--;
        act = listRun(g_ficheLW, (int)t.squad.size(), 12, ly, VW - 24, rows, 12, [&](int i, int xx, int yy, bool sel) {
            const Player& p = t.squad[i];
            Color c = sel ? BLACK : C_TXT;
            drawTextPx(fmt("%2d  %s", p.num, PN[p.pos]), xx + 4, yy + 1, 10, c);
            drawTextPx(fitText(p.name, 220, 10), xx + 56, yy + 1, 10, c);
            if (p.nation >= 0 && p.nation < NUM_NATIONS && p.nation != t.nation) drawTextPx(NATIONS[p.nation].code, xx + 282, yy + 1, 10, sel ? BLACK : C_DIM);
            int v[] = { p.speed, p.shoot, p.pass, p.tackle, p.keep, p.overall() };
            for (int k = 0; k < 6; k++) drawTextPx(fmt("%d", v[k]), xx + 318 + k * 44, yy + 1, 10, k == 5 ? (sel ? BLACK : C_HI) : c);
        });
    } else if (g_fichePage == 1) {
        // palmarès regroupé par titre : nombre de trophées et saisons
        struct HG { std::string title; std::vector<std::string> years; };
        std::vector<HG> hg;
        static int palmSub = 0;
        const char* SUB[] = { "Tout", "Championnat", "Coupe" };
        for (int i = 0; i < 3; i++) if (button(356 + i * 92, yInfoEnd, 88, 13, SUB[i], palmSub == i)) { palmSub = i; g_ficheLW = ListW(); }
        if (IN.pgDn) { palmSub = (palmSub + 1) % 3; g_ficheLW = ListW(); }
        if (IN.pgUp) { palmSub = (palmSub + 2) % 3; g_ficheLW = ListW(); }
        static int allHTeam = -1; static size_t allHN = 0; static std::vector<std::string> allH;
        if (allHTeam != g_ficheTeam || allHN != t.honours.size()) {
            allH = t.kind == TK_CLUB ? historicHonours(g_ficheTeam) : std::vector<std::string>();
            for (auto& h : t.honours) allH.push_back(h);
            allHTeam = g_ficheTeam; allHN = t.honours.size();
        }
        int nLeague = 0, nCup = 0;
        for (auto& h : allH) (h.rfind("Champion", 0) == 0 ? nLeague : nCup)++;
        for (auto& h : allH) {
            bool isLeague = h.rfind("Champion", 0) == 0;
            if ((palmSub == 1 && !isLeague) || (palmSub == 2 && isLeague)) continue;
            std::string ti = h, yr;
            size_t po = h.rfind(" (");
            if (po != std::string::npos && h.back() == ')') { ti = h.substr(0, po); yr = h.substr(po + 2, h.size() - po - 3); }
            // « Coupe de France 2027 » -> « Coupe de France »
            if (ti.size() > 5 && isdigit((unsigned char)ti.back()) && ti[ti.size() - 5] == ' ') ti = ti.substr(0, ti.size() - 5);
            auto it = std::find_if(hg.begin(), hg.end(), [&](const HG& x) { return x.title == ti; });
            if (it == hg.end()) { hg.push_back({ ti, {} }); it = hg.end() - 1; }
            if (!yr.empty()) it->years.push_back(yr);
        }
        std::stable_sort(hg.begin(), hg.end(), [](const HG& a, const HG& b) { return a.years.size() > b.years.size(); });
        if (hg.empty()) drawTextPx(palmSub == 0 ? "Aucun titre pour l'instant (les titres gagnés en carrière s'affichent ici)." : "Aucun titre dans cette catégorie.", 16, ly + 4, 10, C_DIM);
        else drawTextPx(fmt("%d titre%s : %d championnat%s, %d coupe%s", (int)allH.size(), allH.size() > 1 ? "s" : "", nLeague, nLeague > 1 ? "s" : "", nCup, nCup > 1 ? "s" : ""), 16, VH - 28, 10, C_HI);
        act = listRun(g_ficheLW, (int)hg.size(), 12, ly, VW - 24, rows, 12, [&](int i, int xx, int yy, bool sel) {
            const HG& h = hg[i];
            // petite coupe
            Color gold = { 240, 200, 60, 255 };
            DrawRectangle(xx + 5, yy + 2, 7, 4, gold); DrawRectangle(xx + 7, yy + 6, 3, 2, gold); DrawRectangle(xx + 5, yy + 8, 7, 2, gold);
            drawTextPx(fmt("%dx", (int)std::max<size_t>(1, h.years.size())), xx + 16, yy + 1, 10, sel ? BLACK : C_HI);
            drawTextPx(fitText(h.title, 300, 10), xx + 40, yy + 1, 10, sel ? BLACK : C_TXT);
            std::string ys; for (auto& y2 : h.years) ys += (ys.empty() ? "" : ", ") + y2;
            drawTextPx(fitText(ys, VW - 390, 10), xx + 350, yy + 1, 10, sel ? BLACK : C_DIM);
        });
    } else if (g_fichePage == 3) {
        drawClubArchive(g_ficheTeam, ly);
    } else if (g_fichePage == 4) {
        // calendrier complet de l'équipe (toutes compétitions) et compétitions en cours
        const Season& S = g_career.season;
        struct FM { double t; int comp, mi; };
        std::vector<FM> v;
        std::vector<std::string> comps;
        for (int c = 0; c < (int)S.comps.size(); c++) {
            const Competition& C = S.comps[c];
            bool in = false;
            for (auto& st : C.stages) for (auto& R : st.rounds) for (int mi : R.m) {
                const MatchRes& m = C.matches[mi];
                if (m.home != g_ficheTeam && m.away != g_ficheTeam) continue;
                v.push_back({ R.time, c, mi }); in = true;
            }
            if (in) comps.push_back(C.shortName + (C.done ? (C.winner == g_ficheTeam ? " (vainqueur)" : " (terminé)") : " (en cours)"));
        }
        std::stable_sort(v.begin(), v.end(), [](const FM& a, const FM& b) { return a.t < b.t; });
        std::string cl; for (auto& s : comps) cl += (cl.empty() ? "" : "   ") + s;
        for (auto& l : wrapText("Compétitions : " + (cl.empty() ? std::string("aucune") : cl), VW - 30)) { drawTextPx(l, 16, ly, 10, C_HI); ly += 11; rows--; }
        ly += 2;
        int nextI = -1; for (int i = 0; i < (int)v.size(); i++) if (!S.comps[v[i].comp].matches[v[i].mi].played) { nextI = i; break; }
        static int lastFicheCal = -1;
        if (lastFicheCal != g_ficheTeam) { lastFicheCal = g_ficheTeam; if (nextI > 3) { g_ficheLW.cur = nextI; g_ficheLW.top = nextI - 3; } }
        act = listRun(g_ficheLW, (int)v.size(), 12, ly, VW - 24, rows, 12, [&](int i, int xx, int yy, bool sel) {
            const Competition& C = S.comps[v[i].comp];
            const MatchRes& m = C.matches[v[i].mi];
            bool home = m.home == g_ficheTeam;
            int opp = home ? m.away : m.home;
            Color c = sel ? BLACK : i == nextI ? C_HI : C_TXT;
            drawTextPx(fitText(dateOf(v[i].t, g_career.year), 90, 10), xx + 4, yy + 1, 10, c);
            drawTextPx(fitText(C.shortName, 110, 10), xx + 96, yy + 1, 10, sel ? BLACK : C_DIM);
            drawTextPx(home ? "dom." : "ext.", xx + 210, yy + 1, 10, sel ? BLACK : C_DIM);
            if (opp >= 0) { drawKitIcon(g_world.teams[opp].home, xx + 244, yy + 1, 1); drawTextPx(fitText(g_world.teams[opp].name, 220, 10), xx + 256, yy + 1, 10, c); }
            if (m.played) {
                int gf = home ? m.hg : m.ag, ga = home ? m.ag : m.hg;
                Color rc = gf > ga ? C_GOOD : gf < ga ? C_BAD : C_HI;
                if (gf == ga && m.ph >= 0) rc = ((home ? m.ph : m.pa) > (home ? m.pa : m.ph)) ? C_GOOD : C_BAD;
                drawTextPx(fmt("%d - %d", gf, ga) + (m.aet ? " ap" : "") + (m.ph >= 0 ? fmt(" (%d-%d tab)", home ? m.ph : m.pa, home ? m.pa : m.ph) : ""), xx + VW - 150, yy + 1, 10, sel ? BLACK : rc);
            } else drawTextPx("à jouer", xx + VW - 150, yy + 1, 10, sel ? BLACK : C_DIM);
        });
        if (v.empty()) drawTextPx("Aucun match programmé pour cette équipe.", 16, ly + 2, 10, C_DIM);
        if (act >= 0) { openMatchInfo(v[act].comp, v[act].mi, SC_FICHE); return; }
        act = -1;
    } else {
        if (g_ficheRelated.empty()) drawTextPx(t.kind == TK_CLUB ? "Ce club n'a pas d'équipe réserve engagée." : "-", 16, ly + 4, 10, C_DIM);
        act = listRun(g_ficheLW, (int)g_ficheRelated.size(), 12, ly, VW - 24, rows, 12, [&](int i, int xx, int yy, bool sel) {
            int o = g_ficheRelated[i].first;
            drawKitIcon(g_world.teams[o].home, xx + 3, yy + 1, 1);
            Color c = sel ? BLACK : C_TXT;
            drawTextPx(fitText(g_world.teams[o].name, 200, 10), xx + 16, yy + 1, 10, c);
            drawTextPx(g_ficheRelated[i].second, xx + 220, yy + 1, 10, sel ? BLACK : C_DIM);
            drawTextPx(fitText(divisionOf(o), 240, 10), xx + 350, yy + 1, 10, c);
        });
        if (act >= 0) { Screen b = g_ficheBack; openFiche(g_ficheRelated[act].first, b); return; }
    }
    if (g_fichePage == 0 && act >= 0) { openPlayer(g_ficheTeam, act, SC_FICHE); return; }
    std::string help = "G/D : onglet   Retour";
    if (t.kind == TK_CLUB) help = "G/D : onglet   Tab : modifier le club (éditeur)   Retour";
    drawFooter(help);
    if (t.kind == TK_CLUB && IN.tab) { openClubEditor(g_ficheTeam, false, SC_FICHE); return; }
    if (IN.back) g_screen = g_ficheBack;
}

// ------------------------------------------------------------------ éditeur de clubs
static Team g_ed;
static int g_edRow = 0;
static int g_edTyping = -1;
static ListW g_edMenuLW, g_deptLW;

static void openClubEditor(int team, bool forCareer, Screen back) {
    g_editTeam = team; g_editForCareer = forCareer; g_editBack = back;
    g_edRow = 0; g_edTyping = -1; g_textMode = false;
    if (team >= 0) {
        g_ed = g_world.teams[team];
        if (g_ed.president.empty()) g_ed.president = presidentName(team);
        if (!g_ed.presAge) g_ed.presAge = presidentAge(team);
        if (g_ed.presPlace.empty()) g_ed.presPlace = presidentPlace(team);
        g_edU19 = false; for (auto& o : g_world.teams) if (o.parent == team && o.youth) g_edU19 = true;
    }
    else {
        g_ed = Team();
        g_ed.name = "Mon Club"; g_ed.shortName = "MONCLUB"; g_ed.stadium = "Stade Municipal";
        g_ed.town = ""; g_ed.dept = -1; g_ed.district = -1; g_ed.rating = 30;
        g_ed.home.shirt = 0x0055A4; g_ed.home.shirt2 = 0xFFFFFF; g_ed.home.shorts = 0xFFFFFF; g_ed.home.pattern = KP_PLAIN;
        g_ed.founded = g_careerActive && g_career.kind == CK_CLUB ? g_career.year : 2026;
        g_ed.president = randomFullName(CU_FR, (uint32_t)GetTime() * 7919u + 13u); g_ed.presAge = 50; g_ed.presPlace = "";
        g_edU19 = true;
    }
    g_screen = SC_CLUBEDIT;
}

static void screenEditMenu() {
    drawBackground("Éditeurs");
    int nCustom = 0, nEdited = 0;
    for (int i = 0; i < (int)g_world.teams.size(); i++) { if (g_world.teams[i].custom) nCustom++; else if (g_world.teams[i].edited) nEdited++; }
    drawTextCentered(fmt("%d club%s créé%s   -   %d club%s modifié%s", nCustom, nCustom > 1 ? "s" : "", nCustom > 1 ? "s" : "", nEdited, nEdited > 1 ? "s" : "", nEdited > 1 ? "s" : ""), VW / 2, 32, 10, C_DIM);
    // tuiles regroupées par thème : CLUBS / JOUEURS ET PERSONNES / PARTENAIRES
    std::vector<std::string> items = { "Créer un nouveau club", fmt("Mes clubs créés (%d)", nCustom), "Modifier un club existant", "Joueurs, présidents, staff", "Managers", "Sponsors", "Retour" };
    static const int GRP[7] = { 0, 0, 0, 1, 1, 2, 3 };
    static const char* GN[3] = { "CLUBS", "PERSONNES", "PARTENAIRES" };
    static const Color GC[3] = { { 80, 150, 240, 255 }, { 60, 190, 90, 255 }, { 240, 190, 50, 255 } };
    static const char* DESC[7] = {
        "Nom, couleurs (domicile, extérieur, 3e maillot, 13 motifs), stade, district, président. Le club débute dans la dernière division de son district.",
        "Reprendre une carrière avec un club créé, le modifier ou le supprimer.",
        "Renommer un club, changer ses maillots (dont un 3e maillot), son stade et son président.",
        "Créer de nouveaux joueurs ou joueuses, modifier les caractéristiques, l'apparence (6 couleurs de peau) et le staff.",
        "Créer votre manager (obligatoire en carrière) : nom, nationalité, apparence, diplôme.",
        "Ajouter des sponsors (nom et logo PNG/JPG) proposés aux clubs.",
        "Revenir au menu principal." };
    int mx = 40, mw = 290, my = 50, rh = 20;
    int s = listRun(g_edMenuLW, (int)items.size(), mx, my, mw, (int)items.size(), rh, [&](int i, int x, int y, bool sel) {
        if (GRP[i] < 3) DrawRectangle(x + 4, y + 5, 6, 9, sel ? BLACK : GC[GRP[i]]);
        drawTextPx(items[i], x + 16, y + 5, 10, sel ? BLACK : C_TXT);
        if (GRP[i] < 3 && (i == 0 || GRP[i] != GRP[i - 1])) { std::string g = GN[GRP[i]]; drawTextPx(g, x + mw - 8 - textWidth(g, 10), y + 5, 10, sel ? Color{ 90, 60, 10, 255 } : Color{ GC[GRP[i]].r, GC[GRP[i]].g, GC[GRP[i]].b, 170 }); }
    });
    {   // panneau de description
        int cur = std::max(0, std::min(6, g_edMenuLW.cur));
        int px = mx + mw + 16, pw = VW - px - 16, py = my, ph = rh * 7;
        Color gc = GRP[cur] < 3 ? GC[GRP[cur]] : Color{ 150, 160, 180, 255 };
        DrawRectangle(px, py, pw, ph, Color{ 6, 12, 28, 200 });
        DrawRectangle(px, py, pw, 16, gc);
        drawTextPx(GRP[cur] < 3 ? GN[GRP[cur]] : "", px + 6, py + 3, 10, BLACK);
        DrawRectangleLines(px, py, pw, ph, gc);
        drawTextPx(fitText(items[cur], pw - 12, 10), px + 8, py + 24, 10, C_HI);
        int yy = py + 42;
        for (auto& l : wrapText(DESC[cur], pw - 18)) { drawTextPx(l, px + 8, yy, 10, C_TXT); yy += 12; }
    }
    int y = my + rh * 7 + 14;
    const char* txt[] = {
        "Les clubs créés ou modifiés sont enregistrés dans clubs_perso.txt (à côté du jeu).",
        "Joueurs et présidents : joueurs_perso.txt ; staff : staff_perso.txt ; managers : managers.txt.",
        "Les joueurs se modifient avant de lancer une carrière ; un club créé en cours de carrière joue la saison suivante.",
    };
    for (auto l : txt) { drawTextCentered(l, VW / 2, y, 10, C_DIM, false); y += 13; }
    drawFooter("OK : valider   Retour");
    if (IN.back || s == 6) { g_screen = SC_MAIN; return; }
    if (s == 5) { g_screen = SC_SPONSORS; return; }
    if (s == 4) { g_screen = SC_MANAGERS; return; }
    if (s == 0) openClubEditor(-1, false, SC_EDITMENU);
    else if (s == 1) g_screen = SC_CUSTOMLIST;
    else if (s == 2) openPick(PM_EDIT);
    else if (s == 3) { extern void openEditDbFwd(); openEditDbFwd(); }
}

// création de club : on choisit le district (rattaché à sa ligue), pas le département
static std::vector<int> editorDepts() {
    static std::vector<int> v;
    if (!v.empty()) return v;
    int fr = frMetroPyr(g_basePyramids);
    std::set<int> s;
    if (fr >= 0) for (auto& pl : g_basePyramids[fr].pools) if (g_basePyramids[fr].tiers[pl.tier].scope == SC_DEPT) s.insert(pl.key);
    for (int d : s) v.push_back(d);
    std::stable_sort(v.begin(), v.end(), [](int a, int b) { if (districtRegion(a) != districtRegion(b)) return districtRegion(a) < districtRegion(b); return sortKey(districtName(a)) < sortKey(districtName(b)); });
    return v;
}

static void screenDeptPick() {
    drawBackground("District du club");
    auto v = editorDepts();
    int s = listRun(g_deptLW, (int)v.size(), 20, 36, VW - 40, 24, 12, [&](int i, int x, int y, bool sel) {
        int d = v[i];
        Color c = sel ? BLACK : C_TXT;
        drawTextPx(fitText(districtFullName(d), 280, 10), x + 6, y + 1, 10, c);
        std::string dl; for (int k : districtDepts(d)) dl += (dl.empty() ? "" : ", ") + std::string(DEPTS[k].code);
        drawTextPx(dl, x + 290, y + 1, 10, sel ? BLACK : C_DIM);
        drawTextPx(fitText("Ligue " + regionName(districtRegion(d)), 230, 10), x + 360, y + 1, 10, sel ? BLACK : C_DIM);
    });
    drawFooter("OK : choisir le district (il détermine la ligue du club)   Retour");
    if (IN.back) { g_screen = SC_CLUBEDIT; return; }
    if (s >= 0) {
        int d = v[s];
        g_ed.district = d; g_ed.region = districtRegion(d);
        g_ed.dept = districtDepts(d).empty() ? -1 : districtDepts(d)[0];
        // première commune du district
        std::string first;
        for (int dp : districtDepts(d)) {
            std::string tw = DEPTS[dp].towns, cur;
            for (size_t i = 0; i <= tw.size() && first.empty(); i++) {
                if (i == tw.size() || tw[i] == ';') {
                    std::string nm = sanitize(cur.substr(0, cur.find('|')).c_str());
                    if (!nm.empty() && districtFor(dp, nm, "") == d) { first = nm; g_ed.dept = dp; }
                    cur.clear();
                } else cur += tw[i];
            }
            if (!first.empty()) break;
        }
        if (g_ed.town.empty() || districtFor(g_ed.dept, g_ed.town, "") != d) g_ed.town = first;
        g_screen = SC_CLUBEDIT;
    }
}

static void finishClubEditor() {
    bool creating = g_editTeam < 0;
    auto trim = [](std::string s) { while (!s.empty() && s.back() == ' ') s.pop_back(); while (!s.empty() && s[0] == ' ') s.erase(0, 1); return s; };
    g_ed.name = trim(g_ed.name); g_ed.shortName = trim(g_ed.shortName); g_ed.stadium = trim(g_ed.stadium); g_ed.town = trim(g_ed.town);
    if (g_ed.name.empty()) { toast("Le club doit avoir un nom"); return; }
    if (g_ed.shortName.empty()) g_ed.shortName = g_ed.name.substr(0, std::min<size_t>(10, g_ed.name.size()));
    g_ed.home.socks = g_ed.home.shirt;
    g_ed.away.shirt = g_ed.home.shirt2; g_ed.away.shirt2 = g_ed.home.shirt; g_ed.away.shorts = g_ed.home.shirt2;
    g_ed.away.socks = g_ed.home.shirt2; g_ed.away.pattern = g_ed.home.pattern;
    if (g_ed.away.shirt == g_ed.home.shirt) { g_ed.away.shirt = 0xFFFFFF; g_ed.away.shirt2 = 0xFFFFFF; }
    if (creating) {
        if (g_ed.dept < 0 || g_ed.district < 0) { toast("Choisissez le district du club"); return; }
        for (auto& o : g_world.teams) if (o.kind == TK_CLUB && sortKey(o.name) == sortKey(g_ed.name)) { toast("Un club porte déjà ce nom"); return; }
        Team t = g_ed;
        t.kind = TK_CLUB; t.nation = g_world.nationIndex("FRA"); t.culture = CU_FR;
        t.region = districtRegion(t.district); t.seed = hashStr(t.name) ^ 0x1234; t.formation = 0;
        t.squadGen = false; t.squad.clear(); t.honours.clear(); t.parent = -1; t.resLevel = 0;
        int idx = g_world.addCustomClub(t);
        savePresident(g_world.teams[idx]);
        bool withU19 = g_edU19;
        if (withU19 && !(g_careerActive && g_career.kind == CK_CLUB && !g_editForCareer)) { makeU19Team(g_world, idx); makeU17Team(g_world, idx); }
        saveCustomClubs();
        if (g_editForCareer) { startCareerWith(idx, SC_MAIN); return; }
        if (g_careerActive && g_career.kind == CK_CLUB) {
            g_career.addClubToPyramid(idx);
            if (withU19) { std::string err; g_career.createU19(idx, err); g_career.createYouth(idx, 2, err); saveCustomClubs(); }
            toast(t.name + " entrera en compétition la saison prochaine" + std::string(withU19 ? " (avec son équipe U19)" : ""));
        } else toast(t.name + " a été créé" + std::string(withU19 ? " avec son équipe U19" : ""));
        g_screen = g_editBack;
        if (g_editBack == SC_PICK) g_screen = SC_EDITMENU;
        return;
    }
    Team& T = g_world.teams[g_editTeam];
    T.name = g_ed.name; T.shortName = g_ed.shortName; T.stadium = g_ed.stadium; T.town = g_ed.town;
    T.home = g_ed.home; T.away = g_ed.away; T.third = g_ed.third; T.hasThird = g_ed.hasThird;
    T.sponsor = g_ed.sponsor;
    T.president = g_ed.president; T.presAge = g_ed.presAge; T.presPlace = g_ed.presPlace; T.presGender = g_ed.presGender;
    savePresident(T);
    if (g_edU19) {
        bool has = false; for (auto& o : g_world.teams) if (o.parent == g_editTeam && o.youth == 1) has = true;
        if (!has) {
            if (g_careerActive && g_career.kind == CK_CLUB) { std::string err; int u = g_career.createU19(g_editTeam, err); if (u < 0) toast(err); g_career.createYouth(g_editTeam, 2, err); }
            else { makeU19Team(g_world, g_editTeam); makeU17Team(g_world, g_editTeam); }
        }
    }
    if (T.custom) T.sta = g_ed.sta;
    if (T.custom) T.rating = g_ed.rating;
    else T.edited = true;
    saveCustomClubs();
    toast("Club modifié et enregistré");
    if (g_editBack == SC_FICHE) { openFiche(g_editTeam, g_ficheBack); return; }
    g_screen = g_editBack;
}

static void screenClubEdit() {
    bool creating = g_editTeam < 0;
    drawBackground(creating ? "Créer un club" : "Modifier un club");
    // aperçu
    DrawRectangle(VW - 150, 36, 140, 110, Color{ 10, 18, 36, 200 });
    static float an = 0; an += GetFrameTime();
    Kit away; away.shirt = g_ed.home.shirt2; away.shirt2 = g_ed.home.shirt; away.shorts = g_ed.home.shirt2; away.socks = g_ed.home.shirt2; away.pattern = g_ed.home.pattern;
    Kit home = g_ed.home; home.socks = home.shirt;
    if (g_ed.hasThird) {
        drawPlayerSprite(VW - 128, 110, home, 2, 1, 0, (int)(an * 6), PS_NORMAL, false, 0, 4);
        drawPlayerSprite(VW - 80, 110, away, 1, 2, 0, (int)(an * 6) + 2, PS_NORMAL, false, 0, 4);
        Kit t3 = g_ed.third; t3.socks = t3.shirt;
        drawPlayerSprite(VW - 32, 110, t3, 3, 0, 0, (int)(an * 6) + 1, PS_NORMAL, false, 0, 4);
    } else {
        drawPlayerSprite(VW - 115, 110, home, 2, 1, 0, (int)(an * 6), PS_NORMAL, false, 0, 5);
        drawPlayerSprite(VW - 45, 110, away, 1, 2, 0, (int)(an * 6) + 2, PS_NORMAL, false, 0, 5);
    }
    drawTextCentered(fitText(g_ed.name, 130, 10), VW - 80, 124, 10, C_HI, false);
    drawTextCentered(g_ed.hasThird ? "Domicile / Extérieur / 3e" : "Domicile / Extérieur", VW - 80, 134, 10, C_DIM, false);

    struct Row { std::string label, value; int kind; }; // 0 texte, 1 cycle, 2 action, 3 département
    bool hasU19 = false; if (!creating) for (auto& o : g_world.teams) if (o.parent == g_editTeam && o.youth) hasU19 = true;
    bool stadEd = creating || (g_editTeam >= 0 && g_world.teams[g_editTeam].custom);
    if (!g_ed.sta.init) initStadium(g_ed, 10, -1);
    std::vector<Row> rows = {
        { "Nom", g_ed.name, 0 }, { "Nom court", g_ed.shortName, 0 }, { "Stade", g_ed.stadium, 0 }, { "Ville", g_ed.town, 0 },
        { "District", creating ? (g_ed.district >= 0 ? districtLabel(g_ed.district) : "(à choisir)") : districtLabel(g_ed.district), creating ? 3 : 4 },
        { "Couleur principale", colorName(g_ed.home.shirt), 1 }, { "Couleur secondaire", colorName(g_ed.home.shirt2), 1 },
        { "Couleur du short", colorName(g_ed.home.shorts), 1 }, { "Motif du maillot", PATTERN_NAMES[std::max(0, std::min(NUM_PATTERNS - 1, g_ed.home.pattern))], 1 },
        { "Niveau (note)", fmt("%d", (int)g_ed.rating), (creating || (g_editTeam >= 0 && g_world.teams[g_editTeam].custom)) ? 1 : 4 },
        { "Stade : capacité", fmt("%d places", g_ed.sta.capacity()), stadEd ? 1 : 4 },
        { "Tribune principale", g_ed.sta.s[0].kind == STK_COVERED ? "couverte" : g_ed.sta.s[0].kind == STK_SEATS ? "gradins (sièges)" : "places debout", stadEd ? 1 : 4 },
        { "Buvette (niveau)", fmt("%d / 5", g_ed.sta.buvette), stadEd ? 1 : 4 },
        { "Boutique (niveau)", fmt("%d / 5", g_ed.sta.boutique), stadEd ? 1 : 4 },
        { "Éclairage (niveau)", fmt("%d / 4", g_ed.sta.lights), stadEd ? 1 : 4 },
        { "Sponsor maillot", g_ed.sponsor.empty() ? std::string("(aucun)") : g_ed.sponsor, 0 },
        { "Équipes de jeunes (U19, U17)", g_edU19 ? (hasU19 ? "oui" : "oui (District U19 et U17, dès la 1re saison)") : "non", (creating || !hasU19) ? 1 : 4 },
        { "Président", g_ed.president, 0 }, { "Âge du président", fmt("%d ans", g_ed.presAge ? g_ed.presAge : 50), 1 }, { "Président né à", g_ed.presPlace.empty() ? std::string("(non renseigné)") : g_ed.presPlace, 0 },
        { "Genre (président / présidente)", g_ed.presGender ? "Femme" : "Homme", 1 },
        { "Troisième maillot", g_ed.hasThird ? "oui" : "non", 1 },
        { "3e : couleur 1", colorName(g_ed.third.shirt), g_ed.hasThird ? 1 : 4 },
        { "3e : couleur 2", colorName(g_ed.third.shirt2), g_ed.hasThird ? 1 : 4 },
        { "3e : motif", PATTERN_NAMES[std::max(0, std::min(NUM_PATTERNS - 1, g_ed.third.pattern))], g_ed.hasThird ? 1 : 4 },
        { creating ? ">>> CRÉER LE CLUB <<<" : ">>> ENREGISTRER <<<", "", 2 }, { "Annuler", "", 2 } };
    int n = (int)rows.size();
    // onglets : identité, maillots, stade, président (lignes de l'onglet + les deux actions)
    static int edTab = 0;
    static const char* ETABS[] = { "Identité", "Maillots", "Stade", "Président" };
    static const std::vector<int> TABROWS[4] = { { 0, 1, 3, 4, 9, 16 }, { 5, 6, 7, 8, 21, 22, 23, 24, 15 }, { 2, 10, 11, 12, 13, 14 }, { 17, 18, 19, 20 } };
    if (g_edTyping < 0) for (int i = 0; i < 4; i++) if (button(12 + i * 116, 30, 112, 14, ETABS[i], edTab == i)) { edTab = i; g_edRow = TABROWS[i][0]; }
    if (g_edTyping < 0 && (IN.pgDn || IN.tab)) { edTab = (edTab + 1) % 4; g_edRow = TABROWS[edTab][0]; }
    if (g_edTyping < 0 && IN.pgUp) { edTab = (edTab + 3) % 4; g_edRow = TABROWS[edTab][0]; }
    if (g_edRow < n - 2 && std::find(TABROWS[edTab].begin(), TABROWS[edTab].end(), g_edRow) == TABROWS[edTab].end())
        for (int i = 0; i < 4; i++) if (std::find(TABROWS[i].begin(), TABROWS[i].end(), g_edRow) != TABROWS[i].end()) edTab = i;   // retour d'un sous-écran
    std::vector<int> vis = TABROWS[edTab]; vis.push_back(n - 2); vis.push_back(n - 1);
    if (std::find(vis.begin(), vis.end(), g_edRow) == vis.end()) g_edRow = vis[0];
    if (g_edTyping < 0) {
        int k = (int)(std::find(vis.begin(), vis.end(), g_edRow) - vis.begin());
        if (IN.up) g_edRow = vis[(k - 1 + (int)vis.size()) % vis.size()];
        if (IN.down) g_edRow = vis[(k + 1) % vis.size()];
    }
    int y = 50, x = 12, w = VW - 180;
    for (int vi = 0; vi < (int)vis.size(); vi++) {
        int i = vis[vi];
        if (vi == (int)vis.size() - 2) y += 8;           // actions séparées
        bool sel = i == g_edRow;
        if (IN.click && g_edTyping < 0 && IN.mouse.x >= x && IN.mouse.x < x + w && IN.mouse.y >= y && IN.mouse.y < y + 15) g_edRow = i;
        DrawRectangle(x, y, w, 14, sel ? C_SEL : (rows[i].kind == 4 ? Color{ 30, 46, 80, 255 } : C_ITEM));
        Color c = sel ? BLACK : (rows[i].kind == 4 ? C_DIM : C_TXT);
        if (rows[i].kind == 2) drawTextCentered(rows[i].label, x + w / 2, y + 1, 10, c, false);
        else {
            drawTextPx(rows[i].label, x + 6, y + 2, 10, c);
            std::string v = rows[i].value;
            if (g_edTyping == i && fmod(GetTime(), 1.0) < 0.5) v += "_";
            if (rows[i].kind == 1 && ((i >= 5 && i <= 7) || i == 22 || i == 23)) {
                unsigned col = i == 5 ? g_ed.home.shirt : i == 6 ? g_ed.home.shirt2 : i == 7 ? g_ed.home.shorts : i == 22 ? g_ed.third.shirt : g_ed.third.shirt2;
                DrawRectangle(x + 150, y + 2, 10, 8, hexc(col)); DrawRectangleLines(x + 150, y + 2, 10, 8, BLACK);
                drawTextPx("< " + v + " >", x + 166, y + 2, 10, c);
            } else if (rows[i].kind == 1) drawTextPx("< " + v + " >", x + 150, y + 2, 10, c);
            else drawTextPx(fitText(v, w - 160, 10), x + 150, y + 2, 10, g_edTyping == i ? (sel ? Color{ 120, 0, 0, 255 } : C_HI) : c);
        }
        y += 16;
    }
    if (creating) {
        drawTextPx("Le club débutera dans la dernière division de son district (la ligue dépend du district).", 12, y + 2, 10, C_DIM);
    }
    // saisie de texte
    if (g_edTyping >= 0) {
        std::string* dst = g_edTyping == 0 ? &g_ed.name : g_edTyping == 1 ? &g_ed.shortName : g_edTyping == 2 ? &g_ed.stadium : g_edTyping == 15 ? &g_ed.sponsor : g_edTyping == 17 ? &g_ed.president : g_edTyping == 19 ? &g_ed.presPlace : &g_ed.town;
        textEdit(*dst, g_edTyping == 1 ? 14 : 40);
        drawFooter("Saisissez le texte   Entrée : valider   Échap : terminer");
        if (IN.start || IN.back || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_ESCAPE)) { g_edTyping = -1; g_textMode = false; }
        return;
    }
    drawFooter("OK : modifier / valider   Gauche/Droite : changer   Retour : annuler");
    int d = IN.left ? -1 : IN.right ? 1 : 0;
    const Row& R = rows[g_edRow];
    if (R.kind == 1 && (d || IN.ok || (IN.click && IN.mouse.x < x + w))) {
        if (!d) d = 1;
        switch (g_edRow) {
        case 5: g_ed.home.shirt = PALETTE[(paletteIndex(g_ed.home.shirt) + d + NUM_PALETTE) % NUM_PALETTE]; break;
        case 6: g_ed.home.shirt2 = PALETTE[(paletteIndex(g_ed.home.shirt2) + d + NUM_PALETTE) % NUM_PALETTE]; break;
        case 7: g_ed.home.shorts = PALETTE[(paletteIndex(g_ed.home.shorts) + d + NUM_PALETTE) % NUM_PALETTE]; break;
        case 8: g_ed.home.pattern = (g_ed.home.pattern + d + NUM_PATTERNS) % NUM_PATTERNS; break;
        case 9: g_ed.rating = std::max(5.f, std::min(95.f, g_ed.rating + d)); break;
        case 10: {
            static const int CAPS[] = { 150, 300, 500, 800, 1000, 1500, 2000, 3000, 5000 };
            int k = 0; for (int i = 0; i < 9; i++) if (g_ed.sta.capacity() >= CAPS[i]) k = i;
            k = (k + d + 9) % 9; stadiumSetCapacity(g_ed.sta, CAPS[k]); break;
        }
        case 11: g_ed.sta.s[0].kind = (uint8_t)((g_ed.sta.s[0].kind + d + 3) % 3); break;
        case 12: g_ed.sta.buvette = (uint8_t)((g_ed.sta.buvette + d + 6) % 6); break;
        case 13: g_ed.sta.boutique = (uint8_t)((g_ed.sta.boutique + d + 6) % 6); break;
        case 14: g_ed.sta.lights = (uint8_t)((g_ed.sta.lights - 1 + d + 4) % 4 + 1); break;
        case 16: g_edU19 = !g_edU19; break;
        case 18: g_ed.presAge = std::max(25, std::min(90, (g_ed.presAge ? g_ed.presAge : 50) + d)); break;
        case 20: g_ed.presGender = g_ed.presGender ? 0 : 1; break;
        case 21:
            g_ed.hasThird = !g_ed.hasThird;
            if (g_ed.hasThird && g_ed.third.shirt == 0xFFFFFF && g_ed.third.shirt2 == 0) { g_ed.hasThird = 0; Kit t3 = teamKit(g_ed, 2); g_ed.third = t3; g_ed.hasThird = 1; }   // proposition de départ
            break;
        case 22: g_ed.third.shirt = PALETTE[(paletteIndex(g_ed.third.shirt) + d + NUM_PALETTE) % NUM_PALETTE]; g_ed.third.socks = g_ed.third.shorts = g_ed.third.shirt; break;
        case 23: g_ed.third.shirt2 = PALETTE[(paletteIndex(g_ed.third.shirt2) + d + NUM_PALETTE) % NUM_PALETTE]; break;
        case 24: g_ed.third.pattern = (g_ed.third.pattern + d + NUM_PATTERNS) % NUM_PATTERNS; break;
        }
    } else if (IN.ok || (IN.click && IN.mouse.x < x + w && IN.mouse.y >= 50 && IN.mouse.y < y)) {
        if (R.kind == 0) { g_edTyping = g_edRow; g_textMode = true; while (GetCharPressed() > 0) {} }
        else if (R.kind == 3) { g_deptLW = ListW(); auto v = editorDepts(); for (int i = 0; i < (int)v.size(); i++) if (v[i] == g_ed.district) g_deptLW.cur = i; g_screen = SC_DEPTPICK; }
        else if (R.kind == 2) { if (g_edRow == n - 2) finishClubEditor(); else { g_screen = g_editBack == SC_PICK ? SC_PICK : g_editBack; } }
        return;
    }
    if (IN.back) g_screen = g_editBack;
}

// ------------------------------------------------------------------ compétition personnalisée
static CustomCompDef g_cdef;
static int g_customCtrl[2] = { -1, -1 };     // index dans g_customSel (-1 : aucune)
static ListW g_customLW;
static bool g_cdefTyping = false;
static const char* CUSTOM_FMT[] = { "Championnat (tous contre tous)", "Coupe (élimination directe)", "Groupes + phase finale" };

static void screenCustom() {
    drawBackground("Compétition personnalisée");
    if (g_cdef.name.empty() && !g_cdefTyping) g_cdef.name = "Ma Coupe";
    for (int k = 0; k < 2; k++) if (g_customCtrl[k] >= (int)g_customSel.size()) g_customCtrl[k] = -1;
    if (g_customCtrl[0] < 0 && !g_customSel.empty() && g_customCtrl[1] != 0) g_customCtrl[0] = 0;
    int nt = (int)g_customSel.size();
    int maxGroups = std::max(1, nt / 3);
    if (g_cdef.groups > maxGroups) g_cdef.groups = maxGroups;
    if (g_cdef.groups < 1) g_cdef.groups = 1;
    auto ctrlName = [&](int k) { return g_customCtrl[k] >= 0 ? g_world.teams[g_customSel[g_customCtrl[k]]].name : std::string("aucune"); };
    std::vector<std::string> items = {
        "Nom : " + g_cdef.name + (g_cdefTyping && fmod(GetTime(), 1.0) < 0.5 ? "_" : ""),
        std::string("Format : ") + CUSTOM_FMT[g_cdef.format],
        g_cdef.legs == 2 ? "Matchs : aller-retour" : "Matchs : aller simple",
        g_cdef.format == 2 ? fmt("Nombre de groupes : %d", g_cdef.groups) : std::string("Nombre de groupes : -"),
        fmt("Choisir les équipes (%d sélectionnée%s)", nt, nt > 1 ? "s" : ""),
        "Équipe contrôlée (joueur 1) : " + ctrlName(0),
        "Équipe contrôlée (joueur 2) : " + ctrlName(1),
        ">>> LANCER LA COMPÉTITION <<<", "Retour" };
    int s = menuRun(g_customLW, items, 40, 440);
    // équipes choisies
    int y = 40 + 20 * (int)items.size() + 6;
    std::string list;
    for (int t : g_customSel) list += (list.empty() ? "" : ", ") + g_world.teams[t].shortName;
    int ly = y;
    for (auto& l : wrapText(list.empty() ? "Aucune équipe choisie" : list, VW - 40)) { if (ly > VH - 30) break; drawTextPx(l, 20, ly, 10, C_DIM); ly += 12; }
    if (g_cdefTyping) {
        textEdit(g_cdef.name, 40);
        drawFooter("Saisissez le nom   Entrée : valider");
        if (IN.start || IN.back || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) { g_cdefTyping = false; g_textMode = false; }
        return;
    }
    drawFooter("Gauche/Droite : modifier   OK : valider   Retour");
    int d = IN.left ? -1 : IN.right ? 1 : 0;
    int c = g_customLW.cur;
    if (s >= 0 && (s == 1 || s == 2 || s == 3 || s == 5 || s == 6)) d = 1;
    if (d) {
        switch (c) {
        case 1: g_cdef.format = (g_cdef.format + d + 3) % 3; break;
        case 2: g_cdef.legs = g_cdef.legs == 2 ? 1 : 2; break;
        case 3: if (g_cdef.format == 2) g_cdef.groups = (g_cdef.groups - 1 + d + maxGroups) % maxGroups + 1; break;
        case 5: case 6: { int k = c - 5; int v = g_customCtrl[k] + 1; v = (v + d + nt + 1) % (nt + 1); g_customCtrl[k] = v - 1;
                          if (g_customCtrl[k] >= 0 && g_customCtrl[k] == g_customCtrl[1 - k]) { v = (v + d + nt + 1) % (nt + 1); g_customCtrl[k] = v - 1; } break; }
        }
    }
    if (IN.back || s == 8) { g_screen = SC_MAIN; return; }
    if (s == 0) { g_cdefTyping = true; g_textMode = true; while (GetCharPressed() > 0) {} return; }
    if (s == 4) { openPick(PM_CUSTOM); return; }
    if (s == 7) {
        int minT = g_cdef.format == 2 ? g_cdef.groups * 3 : 2;
        if (nt < minT) { toast(fmt("Il faut au moins %d équipes", minT)); return; }
        std::vector<int> ctrl;
        for (int k = 0; k < 2; k++) if (g_customCtrl[k] >= 0) ctrl.push_back(g_customSel[g_customCtrl[k]]);
        if (ctrl.empty()) { toast("Choisissez au moins une équipe contrôlée"); return; }
        g_cdef.teams = g_customSel;
        g_career.newCustom(g_cdef, ctrl);
        g_careerActive = true; g_needAdvance = true;
        g_hubNotice = g_cdef.name + fmt(" - %d équipes", nt);
        openHub();
    }
}

// ------------------------------------------------------------------ coefficients UEFA
static ListW g_coeffLW;
static int g_coeffTab = 0;
static ListW g_coeffClubLW;
static void screenEuroSpots();
static void screenCoeff() {
    drawBackground(g_coeffTab == 0 ? "Coefficients UEFA des pays" : g_coeffTab == 1 ? "Coefficients UEFA des clubs" : g_coeffTab == 2 ? "Qualifications européennes" : "Les points UEFA");
    if (button(10, 30, 96, 14, "Pays", g_coeffTab == 0)) g_coeffTab = 0;
    if (button(108, 30, 96, 14, "Clubs", g_coeffTab == 1)) g_coeffTab = 1;
    if (button(206, 30, 130, 14, "Places européennes", g_coeffTab == 2)) g_coeffTab = 2;
    if (button(338, 30, 98, 14, "Points UEFA", g_coeffTab == 3)) g_coeffTab = 3;
    if (IN.tab) g_coeffTab = (g_coeffTab + 1) % 4;
    if (g_coeffTab == 3) {
        static const char* TXT[] = {
            "#Comment les clubs gagnent des points (matchs de Coupe d'Europe uniquement)",
            "Victoire : 2 points - match nul : 1 point - défaite : 0 point.",
            "Tours préliminaires et de qualification (C1, C3) : victoire 1 point, nul 0,5 point.",
            "Tirs au but : le match compte comme un nul (le résultat après prolongation est retenu).",
            "Bonus : +1 point pour chaque club qualifié en phase de groupes de la Ligue des champions,",
            "+1 point par tour atteint à partir des quarts de finale (C1 et Coupe UEFA : quart, demie, finale).",
            "Intertoto : 3e tour, demi-finales et finales comptent comme des tours de qualification (1 / 0,5).",
            "Les compétitions de jeunes (Ligue des champions U19) ne rapportent aucun point.",
            "#Coefficient d'un pays",
            "Saison : total des points de tous ses clubs divisé par le nombre de clubs engagés en début de saison",
            "(les points de l'Intertoto s'ajoutent au total mais ses clubs ne comptent pas dans le diviseur).",
            "Classement : somme des 5 dernières saisons. Il fixe les places de la saison suivante :",
            "C1 : rangs 1-3 : 4 clubs, 4-6 : 3, 7-15 : 2, au-delà : 1 (le champion, dès le 1er tour pour les petits pays).",
            "C3 : rangs 1-18 : 1er tour (vainqueur de coupe + clubs du championnat), 19 et plus : tour de qualification.",
            "Intertoto : rangs 1-8 : un club au 3e tour, 1-12 : un club au 2e tour, 13 et plus : un club au 1er tour.",
            "Liechtenstein : pas de championnat, seul le vainqueur de la coupe a une place (C3, tour de qualification).",
            "#Coefficient d'un club",
            "Saison : points gagnés par le club. Classement sur 5 saisons = total du club + 33 % du coefficient du pays.",
            "Il sert aux têtes de série des tirages (C1, C3, Intertoto).",
        };
        int yy = 52;
        for (auto l : TXT) {
            if (l[0] == '#') { yy += 4; drawTextPx(l + 1, 16, yy, 10, C_HI); yy += 13; }
            else { drawTextPx(l, 24, yy, 10, C_TXT); yy += 12; }
        }
        drawFooter("Tab : onglet   Retour");
        if (IN.back) openHub();
        return;
    }
    int yr = g_career.year;
    int y = 48;
    if (g_coeffTab == 2) { screenEuroSpots(); return; }
    if (g_coeffTab == 1) {
        static std::vector<int> clubs; static int builtYear = -1; static size_t builtN = 0;
        if (builtYear != yr || builtN != g_world.teams.size()) {
            clubs.clear();
            for (int t = 0; t < (int)g_world.teams.size(); t++) if (g_world.teams[t].kind == TK_CLUB && g_world.teams[t].coefTotal() > 0) clubs.push_back(t);
            std::sort(clubs.begin(), clubs.end(), [](int a, int b) { return g_world.teams[a].coefTotal() > g_world.teams[b].coefTotal(); });
            if (clubs.size() > 200) clubs.resize(200);
            builtYear = yr; builtN = g_world.teams.size();
        }
        drawTextPx("Rang  Club", 16, y, 10, C_DIM);
        for (int k = 0; k < 5; k++) drawTextPx(fmt("%02d-%02d", (yr - 5 + k) % 100, (yr - 4 + k) % 100), 250 + k * 42, y, 10, C_DIM);
        drawTextPx("Total", 462, y, 10, C_HI);
        drawTextPx("Coef.", 520, y, 10, C_DIM);
        y += 12;
        int sel = listRun(g_coeffClubLW, (int)clubs.size(), 12, y, VW - 24, 22, 12, [&](int i, int x, int yy, bool sel) {
            const Team& T = g_world.teams[clubs[i]];
            bool mine = g_career.season.isControlled(clubs[i]);
            bool fr = T.nation >= 0 && std::string(NATIONS[T.nation].code) == "FRA";
            Color c = sel ? BLACK : mine ? C_GOOD : fr ? C_HI : C_TXT;
            drawTextPx(fmt("%3d", i + 1), x + 2, yy + 1, 10, c);
            drawKitIcon(T.home, x + 28, yy + 1, 1);
            drawTextPx(fitText(T.name, 150, 10), x + 42, yy + 1, 10, c);
            drawTextPx(T.nation >= 0 ? NATIONS[T.nation].code : "", x + 196, yy + 1, 10, sel ? BLACK : C_DIM);
            for (int k = 0; k < 5; k++) drawTextPx(fmt("%.1f", T.coefs[k]), x + 238 + k * 42, yy + 1, 10, c);
            drawTextPx(fmt("%.1f", T.coefTotal()), x + 450, yy + 1, 10, sel ? BLACK : C_HI);
            drawTextPx(fmt("%.1f", clubSeed(clubs[i])), x + 508, yy + 1, 10, c);
        });
        if (sel >= 0) { openFiche(clubs[sel], SC_COEFF); return; }
        drawFooter("Victoire 2, nul 1 (qualif./Intertoto : 1 et 0,5), groupes C1 +1, dès les quarts +1 par tour. Coef. = club + 33 % du pays   Tab");
        if (IN.back) openHub();
        return;
    }
    struct R { int rank; const UefaCountry* u; };
    std::vector<R> v;
    for (auto& u : g_career.uefa) v.push_back({ g_career.uefaRank(u.code), &u });
    std::sort(v.begin(), v.end(), [](const R& a, const R& b) { return a.rank < b.rank; });
    drawTextPx("Rang  Pays", 16, y, 10, C_DIM);
    for (int k = 0; k < 5; k++) drawTextPx(fmt("%02d-%02d", (yr - 5 + k) % 100, (yr - 4 + k) % 100), 200 + k * 42, y, 10, C_DIM);
    drawTextPx("Total", 412, y, 10, C_HI);
    drawTextPx("En cours", 456, y, 10, C_DIM);
    drawTextPx(g_career.opts.euroFormat ? "C1 / C3 / C4" : "C1 / C3 / IT", 520, y, 10, C_DIM);
    y += 12;
    listRun(g_coeffLW, (int)v.size(), 12, y, VW - 24, 22, 12, [&](int i, int x, int yy, bool sel) {
        const UefaCountry& u = *v[i].u;
        int r = v[i].rank;
        int nat = g_world.nationIndex(u.code.c_str());
        bool fr = u.code == "FRA";
        Color c = sel ? BLACK : (fr ? C_HI : C_TXT);
        Color zone = r <= 3 ? C_GOOD : r <= 6 ? Color{ 150, 220, 120, 255 } : r <= 15 ? C_HI : r <= 21 ? Color{ 255, 170, 60, 255 } : C_DIM;
        DrawRectangle(x, yy, 3, 11, zone);
        drawTextPx(fmt("%2d", r), x + 6, yy + 1, 10, c);
        drawTextPx(fitText(nat >= 0 ? g_world.teams[nat].name : u.code, 150, 10), x + 34, yy + 1, 10, c);
        for (int k = 0; k < 5; k++) drawTextPx(fmt("%.1f", u.pts[k]), x + 188 + k * 42, yy + 1, 10, c);
        drawTextPx(fmt("%.1f", u.total()), x + 400, yy + 1, 10, sel ? BLACK : C_HI);
        drawTextPx(fmt("%.1f", u.cur), x + 450, yy + 1, 10, c);
        const EuroSpots& E = g_career.nextEuro;
        auto cnt = [&](std::initializer_list<const std::vector<int>*> L) { int n = 0; for (auto* l : L) for (int t : *l) { int nt = g_world.teams[t].nation; if (nt >= 0 && u.code == NATIONS[nt].code) n++; } return n; };
        int ucl = cnt({ &E.uclGS, &E.uclQ3, &E.uclQ2, &E.uclQ1 }), uefaN = cnt({ &E.uefaR1, &E.uefaQR }), itN = cnt({ &E.itR1, &E.itR2, &E.itR3 });
        drawTextPx(fmt("%d / %d / %d", ucl, uefaN, itN), x + 520, yy + 1, 10, c);
    });
    drawFooter("Points sur 5 saisons : ils fixent le nombre de clubs européens de la saison suivante   Tab : onglet   Retour");
    if (IN.back) openHub();
}

// ------------------------------------------------------------------ configuration des commandes
static float g_ctlCaptureT = 0;
static int g_ctlPad = 0;
static const char* ACTION_NAMES[] = { "Haut", "Bas", "Gauche", "Droite", "Passe", "Tir (lob/tacle en classique)", "Pause", "Lob / centre", "Passe en profondeur", "Sprint" };
static const char* PADACT_NAMES[] = { "Passe (défense : changer de joueur)", "Tir (défense : tacle glissé)", "Pause", "Lob / centre (défense : presser)", "Passe en profondeur", "Sprint" };

// lignes : 0 style, 1..20 claviers (2 x 10), 21 manette choisie, 22..27 boutons, 28 zone morte, 29 réinit, 30 retour
static void screenControls(float dt) {
    drawBackground("Configuration des commandes");
    const int NKB = NUM_ACTIONS, N = 1 + 2 * NKB + 1 + NUM_PA + 3;
    const int R_PAD = 1 + 2 * NKB, R_BTN = R_PAD + 1, R_DZ = R_BTN + NUM_PA, R_RESET = R_DZ + 1, R_BACK = R_RESET + 1;
    auto label = [&](int i) -> std::string {
        if (i == 0) return std::string("Style de jeu : ") + (g_settings.controlStyle ? "SUPER NINTENDO (4 boutons + sprint)" : "CLASSIQUE (2 boutons)");
        if (i < R_PAD) { int kb = (i - 1) / NKB, a = (i - 1) % NKB; return fmt("Clavier %d - %s", kb + 1, ACTION_NAMES[a]); }
        if (i == R_PAD) return fmt("Manette à configurer : %d  %s", g_ctlPad + 1, IsGamepadAvailable(g_ctlPad) ? "(branchée)" : "(non détectée)");
        if (i < R_DZ) return fmt("Manette %d - %s", g_ctlPad + 1, PADACT_NAMES[i - R_BTN]);
        if (i == R_DZ) return fmt("Zone morte des sticks : %d%%", (int)std::lround(g_settings.deadzone * 100));
        if (i == R_RESET) return "Rétablir les commandes par défaut";
        return "Retour";
    };
    auto value = [&](int i) -> std::string {
        if (i >= 1 && i < R_PAD) return keyName(g_settings.keys[(i - 1) / NKB][(i - 1) % NKB]);
        if (i >= R_BTN && i < R_DZ) return padButtonName(g_settings.pad[g_ctlPad][i - R_BTN]);
        return "";
    };
    bool capturing = g_ctlCapture >= 0;
    int s = listRun(g_ctlLW, N, 60, 34, VW - 120, 21, 14, [&](int i, int x, int y, bool sel) {
        Color c = sel ? BLACK : (i == 0 ? C_HI : C_TXT);
        drawTextPx(label(i), x + 8, y + 2, 10, c);
        std::string v = value(i);
        if (capturing && g_ctlCapture == i) v = fmod(GetTime(), 0.6) < 0.3 ? "???" : "";
        if (!v.empty()) drawTextPx(v, x + VW - 120 - 150, y + 2, 10, sel ? BLACK : C_HI);
    }, !capturing);
    if (capturing) {
        g_ctlCaptureT += dt;
        bool isKey = g_ctlCapture < R_PAD;
        DrawRectangle(VW / 2 - 170, VH / 2 - 20, 340, 40, Color{ 0, 0, 0, 220 });
        DrawRectangleLines(VW / 2 - 170, VH / 2 - 20, 340, 40, C_SEL);
        drawTextCentered(isKey ? "Appuyez sur une touche du clavier" : fmt("Appuyez sur un bouton de la manette %d", g_ctlPad + 1), VW / 2, VH / 2 - 12, 10, C_HI, false);
        drawTextCentered("Échap : annuler", VW / 2, VH / 2 + 4, 10, C_DIM, false);
        if (g_ctlCaptureT < 0.2f) { while (GetKeyPressed() > 0) {} return; }
        if (isKey) {
            int k = GetKeyPressed();
            if (k == KEY_ESCAPE) { g_ctlCapture = -1; g_textMode = false; return; }
            if (k > 0) {
                int kb = (g_ctlCapture - 1) / NKB, a = (g_ctlCapture - 1) % NKB;
                for (int b = 0; b < NUM_ACTIONS; b++) if (b != a && g_settings.keys[kb][b] == k) g_settings.keys[kb][b] = g_settings.keys[kb][a];
                g_settings.keys[kb][a] = k;
                g_settings.save();
                g_ctlCapture = -1; g_textMode = false;
            }
        } else {
            if (IsKeyPressed(KEY_ESCAPE)) { g_ctlCapture = -1; g_textMode = false; return; }
            for (int b = 1; b <= GAMEPAD_BUTTON_RIGHT_THUMB; b++) {
                if (!IsGamepadButtonPressed(g_ctlPad, b)) continue;
                int a = g_ctlCapture - R_BTN;
                for (int o = 0; o < NUM_PA; o++) if (o != a && g_settings.pad[g_ctlPad][o] == b) g_settings.pad[g_ctlPad][o] = g_settings.pad[g_ctlPad][a];
                g_settings.pad[g_ctlPad][a] = b;
                g_settings.save();
                g_ctlCapture = -1; g_textMode = false;
                break;
            }
        }
        return;
    }
    drawFooter("OK : changer la touche / le bouton   Gauche/Droite : régler   Retour");
    int d = IN.left ? -1 : IN.right ? 1 : 0;
    int c = g_ctlLW.cur;
    if (d) {
        if (c == 0) { g_settings.controlStyle = 1 - g_settings.controlStyle; g_settings.save(); }
        if (c == R_PAD) g_ctlPad = (g_ctlPad + d + 4) % 4;
        if (c == R_DZ) { g_settings.deadzone = std::max(0.1f, std::min(0.8f, g_settings.deadzone + d * 0.05f)); g_settings.save(); }
    }
    if (IN.back || s == R_BACK) { g_settings.save(); g_screen = g_ctlBack; return; }
    if (s < 0) return;
    if (s == 0) { g_settings.controlStyle = 1 - g_settings.controlStyle; g_settings.save(); }
    else if ((s >= 1 && s < R_PAD) || (s >= R_BTN && s < R_DZ)) {
        if (s >= R_BTN && !IsGamepadAvailable(g_ctlPad)) { toast(fmt("Manette %d non détectée", g_ctlPad + 1)); return; }
        g_ctlCapture = s; g_ctlCaptureT = 0; g_textMode = true;
    } else if (s == R_PAD) g_ctlPad = (g_ctlPad + 1) % 4;
    else if (s == R_RESET) { g_settings.resetControls(); g_settings.save(); toast("Commandes par défaut rétablies"); }
}

// ------------------------------------------------------------------ places européennes par pays
static ListW g_esLW;
static int g_esMode = 0;              // 0 saison en cours, 1 prévision de la saison prochaine
static int g_esOpen = -1;             // pays détaillé (index dans la liste)
static void screenEuroSpots() {
    static int pvYear = -1; static EuroSpots pv;
    if (button(440, 30, 90, 14, "En cours", g_esMode == 0)) { g_esMode = 0; g_esOpen = -1; }
    if (button(534, 30, 96, 14, "Prévision", g_esMode == 1)) { g_esMode = 1; g_esOpen = -1; if (pvYear != g_career.year) { pv = g_career.previewEuro(); pvYear = g_career.year; } }
    const EuroSpots& E = g_esMode == 1 && pvYear == g_career.year ? pv : g_career.nextEuro;
    const std::vector<int>* L[9] = { &E.uclGS, &E.uclQ3, &E.uclQ2, &E.uclQ1, &E.uefaR1, &E.uefaQR, &E.itR3, &E.itR2, &E.itR1 };
    static const char* LN0[9] = { "Ligue des champions - phase de groupes", "Ligue des champions - 3e tour de qualification", "Ligue des champions - 2e tour de qualification",
                                 "Ligue des champions - 1er tour de qualification", "Coupe UEFA - 1er tour", "Coupe UEFA - tour de qualification",
                                 "Coupe Intertoto - 3e tour", "Coupe Intertoto - 2e tour", "Coupe Intertoto - 1er tour" };
    static const char* LN1[9] = { "Ligue des champions - phase de ligue", "Ligue des champions - 3e tour de qualification ou barrage", "Ligue des champions - 2e tour de qualification",
                                 "Ligue des champions - 1er tour de qualification", "Ligue Europa - phase de ligue", "Ligue Europa - 3e tour de qualification",
                                 "", "", "Ligue Conférence (qualifications ou phase de ligue)" };
    const char* const* LN = g_career.opts.euroFormat ? LN1 : LN0;
    struct R { int rank; std::string code; int n[9]; };
    std::vector<R> v;
    for (auto& u : g_career.uefa) { R r; r.rank = g_career.uefaRank(u.code); r.code = u.code; for (int k = 0; k < 9; k++) r.n[k] = 0; v.push_back(r); }
    std::sort(v.begin(), v.end(), [](const R& a, const R& b) { return a.rank < b.rank; });
    std::map<std::string, int> idx; for (int i = 0; i < (int)v.size(); i++) idx[v[i].code] = i;
    auto codeOf = [](int t) { int n = g_world.teams[t].nation; return n >= 0 ? std::string(NATIONS[n].code) : std::string(); };
    for (int k = 0; k < 9; k++) for (int t : *L[k]) { auto it = idx.find(codeOf(t)); if (it != idx.end()) v[it->second].n[k]++; }
    int y = 50;
    if (g_esOpen >= 0 && g_esOpen < (int)v.size()) {
        // détail d'un pays : clubs et tour d'entrée
        const R& r = v[g_esOpen];
        int ni = g_world.nationIndex(r.code.c_str());
        drawTextPx(fmt("%d. ", r.rank) + (ni >= 0 ? g_world.teams[ni].name : r.code) + (g_esMode ? "  -  prévision de la saison prochaine" : "  -  saison en cours"), 16, y, 10, C_HI);
        y += 14;
        for (int k = 0; k < 9; k++) {
            std::string names;
            for (int t : *L[k]) if (codeOf(t) == r.code) names += (names.empty() ? "" : ", ") + g_world.teams[t].name;
            if (names.empty()) continue;
            drawTextPx(LN[k], 20, y, 10, C_DIM); y += 11;
            for (auto& l : wrapText(names, VW - 60)) { if (y > VH - 30) break; drawTextPx(l, 34, y, 10, C_TXT); y += 11; }
            y += 2;
        }
        drawFooter("Retour : liste des pays");
        if (IN.back || IN.ok) g_esOpen = -1;
        IN.back = false;
        return;
    }
    static const char* CH[9] = { "C1 G", "Q3", "Q2", "Q1", "C3 T1", "TQ", "IT 3", "IT 2", "IT 1" };
    drawTextPx("Rang  Pays", 16, y, 10, C_DIM);
    for (int k = 0; k < 9; k++) drawTextPx(CH[k], 214 + k * 44, y, 10, k < 4 ? C_HI : k < 6 ? C_GOOD : C_DIM);
    drawTextPx("Total", 610, y, 10, C_DIM);
    y += 12;
    int sel = listRun(g_esLW, (int)v.size(), 12, y, VW - 24, (VH - 44 - y) / 12, 12, [&](int i, int x, int yy, bool s2) {
        const R& r = v[i];
        int ni = g_world.nationIndex(r.code.c_str());
        Color c = s2 ? BLACK : r.code == "FRA" ? C_HI : C_TXT;
        drawTextPx(fmt("%2d", r.rank), x + 4, yy + 1, 10, c);
        drawTextPx(fitText(ni >= 0 ? g_world.teams[ni].name : r.code, 150, 10), x + 30, yy + 1, 10, c);
        int tot = 0;
        for (int k = 0; k < 9; k++) { tot += r.n[k]; if (r.n[k]) drawTextPx(fmt("%d", r.n[k]), x + 212 + k * 44, yy + 1, 10, c); else drawTextPx("-", x + 212 + k * 44, yy + 1, 10, s2 ? BLACK : Color{ 90, 100, 130, 255 }); }
        drawTextPx(fmt("%d", tot), x + 600, yy + 1, 10, c);
    });
    int ry = VH - 42;
    drawTextPx("C1 : Q1 20, Q2 +18, Q3 +18, groupes +16 (tenant inclus).  C3 : TQ 82, 1er tour 36 + 41 + 16 éliminés du Q3 + 3 Intertoto.", 12, ry, 10, C_DIM);
    drawTextPx("Coupe nationale : si le vainqueur est en C1, sa place en C3 revient au finaliste (pas pour la Coupe de la Ligue).", 12, ry + 11, 10, C_DIM);
    drawFooter("OK : clubs qualifiés du pays   Tab : onglet   Retour");
    if (sel >= 0) g_esOpen = sel;
    if (IN.back) openHub();
    IN.back = false;
}
