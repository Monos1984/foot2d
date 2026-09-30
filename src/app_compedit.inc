// ------------------------------------------------------------------ éditeur de compétitions de carrière
// Hors carrière : modèle des nouvelles carrières (competitions_perso.txt). En carrière : règles de la carrière en cours.
static ListW g_ceLW, g_ceLW2;
static int g_ceMode = 0;        // 0 pays, 1 divisions, 2 règles d'une division, 3 clubs, 4 destination d'un club, 5 ajouter un club, 6 coupe
static int g_cePyr = -1, g_ceTier = -1, g_ceClub = -1;
static bool g_ceTyping = false;

static bool ceInCareer() { return g_careerActive && g_career.kind == CK_CLUB; }
static std::vector<Pyramid>& cePyrs() { return ceInCareer() ? g_career.pyramids : g_basePyramids; }
static void openCompEditor() { g_ceMode = 0; g_ceLW = ListW(); g_cePyr = -1; g_ceTyping = false; g_screen = SC_COMPEDIT; }
static void ceChanged(Pyramid& P) {
    if (ceInCareer()) return;           // carrière : règles conservées dans la sauvegarde
    markPyrEdited(P.country); saveCompEdits();
}
static const char* TB_NAMES[] = { "différence de buts", "règlement LFP", "règlement FFF (particulière)", "confrontations directes", "buts marqués (Angleterre)", "règlement FIFA" };
static const char* LEGS_NAMES[] = { "", "aller simple", "aller-retour", "3 matchs", "4 matchs" };

static void screenCompEdit() {
    auto& PY = cePyrs();
    std::string sub = ceInCareer() ? "Carrière en cours : les règles s'appliquent à la fin de saison et à la saison suivante" : "Modèle des nouvelles carrières (enregistré dans competitions_perso.txt)";
    drawBackground("Éditeur de compétitions");
    drawTextCentered(sub, VW / 2, 30, 10, C_DIM, false);
    if (g_cePyr >= (int)PY.size()) g_ceMode = 0;
    // ---------------- 0 : choix du pays
    if (g_ceMode == 0) {
        struct LI { std::string name; int pyr; int grp; };
        std::vector<LI> v;
        for (int p = 0; p < (int)PY.size(); p++) {
            const Pyramid& P = PY[p];
            if (!pyrEditable(P)) continue;
            bool fem = isWomenPyramid(P);
            int ni = g_world.nationIndex(P.country.c_str());
            int grp = P.country == "FRA" ? 0 : fem ? 10 : ni >= 0 ? 1 + NATIONS[ni].conf : 9;
            v.push_back({ std::string(fem ? "[F] " : "") + P.name + "  -  " + P.tiers[0].name, p, grp });
        }
        std::stable_sort(v.begin(), v.end(), [](const LI& a, const LI& b) { if (a.grp != b.grp) return a.grp < b.grp; return sortKey(a.name) < sortKey(b.name); });
        int s = listRun(g_ceLW, (int)v.size(), 40, 44, VW - 80, 20, 14, [&](int i, int x, int y, bool sel) {
            const Pyramid& P = PY[v[i].pyr];
            drawTextPx(fitText(v[i].name, VW - 260, 10), x + 6, y + 2, 10, sel ? BLACK : v[i].grp == 10 ? Color{ 240, 150, 200, 255 } : C_TXT);
            std::string r = fmt("%d division%s", (int)P.tiers.size(), P.tiers.size() > 1 ? "s" : "") + (pyrEdited(P.country) && !ceInCareer() ? "  (modifié)" : "");
            drawTextPx(r, x + VW - 80 - 8 - textWidth(r, 10), y + 2, 10, sel ? BLACK : pyrEdited(P.country) ? C_HI : C_DIM);
        });
        drawFooter("OK : modifier ce championnat   Retour");
        if (s >= 0) { g_cePyr = v[s].pyr; g_ceMode = 1; g_ceLW2 = ListW(); }
        if (IN.back) g_screen = SC_EDITMENU;
        return;
    }
    Pyramid& P = PY[g_cePyr];
    bool structOk = !ceInCareer() && pyrStructEditable(P);
    // ---------------- 1 : divisions du pays
    if (g_ceMode == 1) {
        drawTextCentered(P.name + (structOk ? "" : ceInCareer() ? "  -  structure figée pendant la carrière" : P.country == "FRA" || P.country == "F:FRA" ? "  -  nombre de divisions fixe ; clubs déplaçables entre divisions nationales" : ""), VW / 2, 42, 10, C_HI, false);
        int T = (int)P.tiers.size();
        std::vector<std::string> extra;
        if (structOk) extra.push_back("+ Ajouter une division en bas de la pyramide");
        extra.push_back(P.country == "FRA" ? "Coupe de France : règles (options de carrière)" : "Coupe nationale : règles");
        if (!ceInCareer() && pyrEdited(P.country)) extra.push_back("Rétablir le championnat d'origine");
        extra.push_back("Retour");
        int n = T + (int)extra.size();
        int s = listRun(g_ceLW2, n, 24, 56, VW - 48, 15, 17, [&](int i, int x, int y, bool sel) {
            Color c = sel ? BLACK : C_TXT;
            if (i >= T) { drawTextPx(extra[i - T], x + 6, y + 3, 10, sel ? BLACK : C_HI); return; }
            const TierConf& t = P.tiers[i];
            int nc = 0, ng = 0; for (auto& pl : P.pools) if (pl.tier == i) { nc += (int)pl.clubs.size(); ng += (int)pl.groups.size(); }
            drawTextPx(fmt("%d.", i + 1), x + 6, y + 3, 10, sel ? BLACK : C_DIM);
            drawTextPx(fitText(t.name, 190, 10), x + 26, y + 3, 10, c);
            drawTextPx(fitText(fmt("%d clubs%s", nc, ng > 1 ? fmt(" (%d gr.)", ng).c_str() : ""), 112, 10), x + 222, y + 3, 10, sel ? BLACK : C_DIM);
            drawTextPx(fmt("%d pts  %s", t.ptsWin, LEGS_NAMES[std::max(1, std::min(4, t.legs))]), x + 340, y + 3, 10, c);
            drawTextPx(fmt("+%d  -%d%s", t.up, t.down, t.barrageUp ? fmt("  barr. %d", t.barrageUp).c_str() : ""), x + 470, y + 3, 10, sel ? BLACK : C_GOOD);
        });
        drawFooter("OK : règles et clubs de la division   Retour");
        if (s >= 0 && s < T) { g_ceTier = s; g_ceMode = 2; g_ceLW = ListW(); return; }
        if (s >= T) {
            const std::string& e = extra[s - T];
            if (e[0] == '+') { pyrAddTier(P, (int)P.tiers.size() - 1, fmt("Division %d", (int)P.tiers.size() + 1)); ceChanged(P); toast("Division ajoutée : déplacez-y des clubs"); }
            else if (e.rfind("Coupe", 0) == 0) { if (P.country == "FRA") toast("Coupe de France : prolongation et engagements dans les options de carrière"); else { g_ceMode = 6; g_ceLW = ListW(); } }
            else if (e.rfind("Rétablir", 0) == 0) askConfirm("Rétablir le championnat d'origine (" + P.name + ") ?", []() { auto& PY2 = cePyrs(); if (g_cePyr >= 0 && g_cePyr < (int)PY2.size() && pyrRestore(PY2[g_cePyr])) { saveCompEdits(); toast("Championnat d'origine rétabli"); } }, "Rétablir", "Annuler");
            else { g_ceMode = 0; g_ceLW = ListW(); }
        }
        if (IN.back) { g_ceMode = 0; g_ceLW = ListW(); }
        return;
    }
    if (g_ceTier < 0 || g_ceTier >= (int)P.tiers.size()) { g_ceMode = 1; return; }
    TierConf& t = P.tiers[g_ceTier];
    bool clubsOk = tierClubsEditable(P, g_ceTier) && !ceInCareer();
    // ---------------- 2 : règles d'une division
    if (g_ceMode == 2) {
        drawTextCentered(fmt("%s - division %d", P.name.c_str(), g_ceTier + 1), VW / 2, 42, 10, C_HI, false);
        int nc = 0; for (auto& pl : P.pools) if (pl.tier == g_ceTier) nc += (int)pl.clubs.size();
        std::vector<std::string> rows = {
            "Nom : " + t.name + (g_ceTyping && ((int)(GetTime() * 2)) % 2 ? "_" : ""),
            fmt("Montées vers la division supérieure : %d", t.up),
            fmt("Relégations vers la division inférieure : %d", t.down),
            fmt("Barragistes pour la montée : %d", t.barrageUp),
            fmt("Points pour une victoire : %d", t.ptsWin),
            std::string("Matchs contre chaque adversaire : ") + LEGS_NAMES[std::max(1, std::min(4, t.legs))],
            std::string("Départage : ") + TB_NAMES[std::max(0, std::min(5, t.tb))],
            std::string("Équipes réserves : ") + (t.noReserves ? "interdites" : "autorisées"),
            clubsOk ? fmt("Clubs de la division (%d) : déplacer, ajouter", nc) : fmt("Clubs de la division (%d) : %s", nc, ceInCareer() ? "non modifiables en cours de carrière" : "division régionale, non modifiable"),
        };
        if (structOk) { rows.push_back("Insérer une nouvelle division en dessous"); rows.push_back("Supprimer cette division"); }
        rows.push_back("Retour");
        if (g_ceTyping) {
            textEdit(t.name, 40);
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_ESCAPE)) { g_ceTyping = false; g_textMode = false; if (t.name.empty()) t.name = fmt("Division %d", g_ceTier + 1); ceChanged(P); }
        }
        int s = listRun(g_ceLW, (int)rows.size(), VW / 2 - 200, 58, 400, (int)rows.size(), 18, [&](int i, int x, int y, bool sel) {
            bool val = i >= 1 && i <= 7;
            drawTextPx(fitText(rows[i], 380, 10), x + 8, y + 4, 10, sel ? BLACK : (i == 8 && !clubsOk) ? C_DIM : C_TXT);
            if (val && sel) drawTextPx("< >", x + 400 - 28, y + 4, 10, BLACK);
        }, !g_ceTyping);
        int r = g_ceLW.cur;
        int dl = g_ceTyping ? 0 : IN.left ? -1 : IN.right ? 1 : 0;
        int T = (int)P.tiers.size();
        if (dl && r >= 1 && r <= 7) {
            switch (r) {
            case 1: if (g_ceTier > 0) { t.up = std::max(0, std::min(10, t.up + dl)); pyrRelink(P, g_ceTier, false); } else toast("La première division n'a pas de montée"); break;
            case 2: if (g_ceTier + 1 < T) { t.down = std::max(0, std::min(10, t.down + dl)); pyrRelink(P, g_ceTier, true); } else toast("La dernière division n'a pas de relégation"); break;
            case 3: if (g_ceTier > 0) t.barrageUp = std::max(0, std::min(4, t.barrageUp + dl)); break;
            case 4: t.ptsWin = std::max(1, std::min(5, t.ptsWin + dl)); break;
            case 5: t.legs = std::max(1, std::min(4, t.legs + dl)); break;
            case 6: t.tb = (t.tb + dl + 6) % 6; break;
            case 7: t.noReserves = !t.noReserves; break;
            }
            ceChanged(P);
        }
        drawTextCentered("Montées et relégations restent liées : les promus d'une division sont les relégués de celle du dessus.", VW / 2, VH - 44, 10, C_DIM, false);
        drawFooter(g_ceTyping ? "Tapez le nom   Entrée : valider" : "Gauche / Droite : modifier   OK : ouvrir   Retour");
        if (s == 0) { g_ceTyping = true; g_textMode = true; while (GetCharPressed() > 0) {} }
        else if (s == 7) { t.noReserves = !t.noReserves; ceChanged(P); }
        else if (s == 8) { if (clubsOk) { g_ceMode = 3; g_ceLW2 = ListW(); } else toast(ceInCareer() ? "Carrière en cours : clubs non modifiables" : "Division régionale : clubs non modifiables"); }
        else if (s >= 9 && s < (int)rows.size()) {
            const std::string& e = rows[s];
            if (e.rfind("Insérer", 0) == 0) { pyrAddTier(P, g_ceTier, fmt("Division %d", g_ceTier + 2)); ceChanged(P); g_ceTier++; toast("Division insérée : déplacez-y des clubs"); }
            else if (e.rfind("Supprimer", 0) == 0) {
                if (T <= 1) toast("Il faut garder au moins une division");
                else askConfirm("Supprimer " + t.name + " ? Ses clubs rejoindront la division voisine.", []() {
                    auto& PY2 = cePyrs(); Pyramid& P2 = PY2[g_cePyr];
                    if (pyrDeleteTier(P2, g_ceTier)) { ceChanged(P2); toast("Division supprimée"); }
                    g_ceMode = 1; g_ceTier = -1; }, "Supprimer", "Annuler");
            } else { g_ceMode = 1; }
        }
        if (IN.back && !g_ceTyping) g_ceMode = 1;
        return;
    }
    // clubs de la division
    int pool = -1; for (int i = 0; i < (int)P.pools.size(); i++) if (P.pools[i].tier == g_ceTier) pool = i;
    if (pool < 0) { g_ceMode = 2; return; }
    std::vector<int> cl = P.pools[pool].clubs;
    sortTeamsByName(cl);
    // ---------------- 3 : liste des clubs
    if (g_ceMode == 3) {
        drawTextCentered(fmt("%s : %d clubs", t.name.c_str(), (int)cl.size()), VW / 2, 42, 10, C_HI, false);
        int n = (int)cl.size() + 2;
        int s = listRun(g_ceLW2, n, 40, 56, VW - 80, 16, 16, [&](int i, int x, int y, bool sel) {
            if (i == 0) { drawTextPx("+ Ajouter un club d'une autre division", x + 6, y + 2, 10, sel ? BLACK : C_HI); return; }
            if (i == n - 1) { drawTextPx("Retour", x + 6, y + 2, 10, sel ? BLACK : C_HI); return; }
            const Team& T = g_world.teams[cl[i - 1]];
            drawTextPx(fitText(T.name, 300, 10), x + 6, y + 2, 10, sel ? BLACK : C_TXT);
            drawTextPx(fmt("note %d", (int)T.rating), x + 330, y + 2, 10, sel ? BLACK : C_DIM);
            if (T.parent >= 0) drawTextPx("réserve", x + 420, y + 2, 10, sel ? BLACK : C_DIM);
        });
        drawFooter("OK : déplacer le club vers une autre division   Retour");
        if (s == 0) { g_ceMode = 5; g_ceLW = ListW(); }
        else if (s == n - 1 || IN.back) g_ceMode = 2;
        else if (s > 0) { g_ceClub = cl[s - 1]; g_ceMode = 4; g_ceLW = ListW(); }
        return;
    }
    // ---------------- 4 : division de destination
    if (g_ceMode == 4) {
        drawTextCentered(g_world.teams[g_ceClub].name + " : nouvelle division", VW / 2, 42, 10, C_HI, false);
        std::vector<int> dst;
        for (int k = 0; k < (int)P.tiers.size(); k++) if (k != g_ceTier && tierClubsEditable(P, k)) dst.push_back(k);
        int s = listRun(g_ceLW, (int)dst.size(), VW / 2 - 180, 60, 360, 14, 18, [&](int i, int x, int y, bool sel) {
            drawTextPx(fmt("%d. ", dst[i] + 1) + P.tiers[dst[i]].name, x + 8, y + 4, 10, sel ? BLACK : C_TXT);
        });
        if (dst.empty()) drawTextCentered("Aucune autre division nationale.", VW / 2, 80, 10, C_DIM);
        drawFooter("OK : déplacer   Retour");
        if (s >= 0) { pyrMoveClub(P, g_ceClub, dst[s]); ceChanged(P); toast(g_world.teams[g_ceClub].name + " passe en " + P.tiers[dst[s]].name); g_ceMode = 3; }
        if (IN.back) g_ceMode = 3;
        return;
    }
    // ---------------- 5 : ajouter un club venant d'une autre division
    if (g_ceMode == 5) {
        drawTextCentered("Ajouter à " + t.name + " un club d'une autre division", VW / 2, 42, 10, C_HI, false);
        std::vector<std::pair<int, int>> v;       // club, division
        for (auto& pl : P.pools) if (pl.tier != g_ceTier && tierClubsEditable(P, pl.tier)) for (int c : pl.clubs) v.push_back({ c, pl.tier });
        std::stable_sort(v.begin(), v.end(), [](const std::pair<int, int>& a, const std::pair<int, int>& b) { if (a.second != b.second) return a.second < b.second; return sortKey(g_world.teams[a.first].name) < sortKey(g_world.teams[b.first].name); });
        int s = listRun(g_ceLW, (int)v.size(), 40, 56, VW - 80, 16, 16, [&](int i, int x, int y, bool sel) {
            drawTextPx(fitText(g_world.teams[v[i].first].name, 300, 10), x + 6, y + 2, 10, sel ? BLACK : C_TXT);
            drawTextPx(fitText(P.tiers[v[i].second].name, 220, 10), x + 330, y + 2, 10, sel ? BLACK : C_DIM);
        });
        drawFooter("OK : ajouter à cette division   Retour");
        if (s >= 0) { pyrMoveClub(P, v[s].first, g_ceTier); ceChanged(P); toast(g_world.teams[v[s].first].name + " rejoint " + t.name); }
        if (IN.back) g_ceMode = 3;
        return;
    }
    g_ceMode = 1;
}

// règles de la coupe nationale (mode 6) : traité avant le contrôle de division
static void screenCompEditCup() {
    auto& PY = cePyrs();
    if (g_cePyr < 0 || g_cePyr >= (int)PY.size()) { g_ceMode = 0; return; }
    Pyramid& P = PY[g_cePyr];
    drawBackground("Éditeur de compétitions");
    CupRule r = cupRuleFor(P.country);
    drawTextCentered("Coupe nationale : " + P.name, VW / 2, 36, 10, C_HI, false);
    std::vector<std::string> rows = { std::string("Coupe disputée : ") + (r.enabled ? "oui" : "non"), std::string("Prolongation avant les tirs au but : ") + (r.et ? "oui" : "non (tirs au but directs, sauf finale)"),
                                      std::string("Finale sur terrain neutre : ") + (r.neutralFinal ? "oui" : "non"), "Retour" };
    int s = menuRun(g_ceLW, rows, 70, 420);
    bool ch = false;
    int cur = g_ceLW.cur;
    if ((s == 0) || ((IN.left || IN.right) && cur == 0)) { r.enabled = !r.enabled; ch = true; }
    if ((s == 1) || ((IN.left || IN.right) && cur == 1)) { r.et = !r.et; ch = true; }
    if ((s == 2) || ((IN.left || IN.right) && cur == 2)) { r.neutralFinal = !r.neutralFinal; ch = true; }
    if (ch) { setCupRule(P.country, r); saveCompEdits(); }
    drawTextCentered("Les règles de la coupe s'appliquent dès la prochaine saison (carrière en cours comprise).", VW / 2, VH - 44, 10, C_DIM, false);
    drawFooter("OK / Gauche / Droite : modifier   Retour");
    if (s == 3 || IN.back) { g_ceMode = 1; g_ceLW2 = ListW(); }
}
