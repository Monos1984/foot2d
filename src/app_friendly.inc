// ------------------------------------------------------------------ match amical : choix des équipes (build 28)
// écran face-à-face : les deux équipes côte à côte, recherche par nom, tirage au sort (filtres), adversaire de même niveau,
// grandes affiches (classiques et derbys), affiches récentes, puis réglages du match
static ListW g_frVsLW, g_frSearchLW;
static int g_frFilter = 0, g_frClassic = 0, g_frRecentSel = 0;
static int g_frSearching = 0;            // 1 domicile, 2 extérieur
static std::string g_frQuery, g_frQueryDone = "\x01";
static std::vector<int> g_frResults;
static std::vector<std::pair<int, int>> g_frRecent;
static const char* FR_FILTERS[6] = { "Toutes les équipes", "Clubs", "Sélections", "Clubs français", "Grands clubs (note 75+)", "Football féminin" };

static bool frSenior(int t) { const Team& T = g_world.teams[t]; return T.parent < 0 && T.resLevel == 0 && (T.youth == 0 || T.youth == 6); }
static bool frFilterOk(int t, int f) {
    const Team& T = g_world.teams[t];
    if (!frSenior(t)) return false;
    bool women = isWomenTeam(t);
    if (f == 5) return women;
    if (women) return false;
    switch (f) {
    case 1: return T.kind == TK_CLUB && (T.dbClub >= 0 || T.custom || T.rating >= 58);
    case 2: return T.kind == TK_NATION;
    case 3: return T.kind == TK_CLUB && T.nation == g_world.teams[g_world.nationIndex("FRA")].nation && (T.dbClub >= 0 || T.custom || T.rating >= 50);
    case 4: return T.kind == TK_CLUB && T.rating >= 75;
    default: return T.kind == TK_NATION || T.dbClub >= 0 || T.custom || T.rating >= 60;
    }
}
static int frRandomTeam(int f, int avoid, int gender) {   // gender : -1 libre, 0 masculin, 1 féminin
    std::vector<int> pool;
    for (int i = 0; i < (int)g_world.teams.size(); i++) if (i != avoid && frFilterOk(i, f) && (gender < 0 || (int)isWomenTeam(i) == gender)) pool.push_back(i);
    if (pool.empty()) return -1;
    return pool[GetRandomValue(0, (int)pool.size() - 1)];
}
static int frSameLevel(int ref) {
    if (ref < 0) return -1;
    const Team& R = g_world.teams[ref];
    std::vector<int> pool;
    for (float tol = 3.f; tol <= 15.f && pool.size() < 3; tol += 4.f) {
        pool.clear();
        for (int i = 0; i < (int)g_world.teams.size(); i++) {
            if (i == ref || !frSenior(i) || isWomenTeam(i) != isWomenTeam(ref)) continue;
            const Team& T = g_world.teams[i];
            if (T.kind != R.kind) continue;
            if (T.kind == TK_CLUB && !(T.dbClub >= 0 || T.custom || T.rating >= 50)) continue;
            if (std::fabs(T.rating - R.rating) <= tol) pool.push_back(i);
        }
    }
    if (pool.empty()) return -1;
    return pool[GetRandomValue(0, (int)pool.size() - 1)];
}
// équipe d'après son nom (variantes séparées par '|') : la mieux notée qui contient le texte ; « #FRA » : sélection
static int frFindTeam(const std::string& spec) {
    size_t a = 0;
    while (a <= spec.size()) {
        size_t b = spec.find('|', a); if (b == std::string::npos) b = spec.size();
        std::string q = spec.substr(a, b - a);
        if (!q.empty() && q[0] == '#') { int n = g_world.nationIndex(q.c_str() + 1); if (n >= 0) return n; }
        else if (!q.empty()) {
            std::string k = sortKey(q); int best = -1;
            for (int i = 0; i < (int)g_world.teams.size(); i++) {
                const Team& T = g_world.teams[i];
                if (T.kind != TK_CLUB || !frSenior(i) || isWomenTeam(i)) continue;
                std::string n = sortKey(T.name);
                if (n.find(k) == std::string::npos) continue;
                if (best < 0 || T.rating > g_world.teams[best].rating) best = i;
            }
            if (best >= 0) return best;
        }
        a = b + 1;
    }
    return -1;
}
struct FrClassic { std::string label; int home, away; };
static const std::vector<FrClassic>& frClassics() {
    static std::vector<FrClassic> v; static size_t built = 0;
    if (built == g_world.teams.size() && !v.empty()) return v;
    built = g_world.teams.size(); v.clear();
    static const char* L[][3] = {
        { "Le Classique", "Paris Saint-Germain|Paris SG|PSG", "Olympique de Marseille|Marseille" },
        { "Derby du Rhône", "Olympique Lyonnais|Lyon", "Saint-Étienne|Saint-Etienne" },
        { "Derby du Nord", "Lens", "Lille|LOSC" },
        { "Derby de la Côte d'Azur", "Nice", "Monaco" },
        { "Derby breton", "Rennes", "Nantes" },
        { "El Clásico", "Real Madrid", "Barcelona|Barcelone|Barça" },
        { "Der Klassiker", "Bayern", "Dortmund" },
        { "Derby della Madonnina", "Internazionale|Inter Milan|Inter", "AC Milan|Milan" },
        { "North West Derby", "Liverpool", "Manchester United|Man United" },
        { "Old Firm", "Celtic", "Rangers" },
        { "Superclásico", "Boca Juniors|Boca", "River Plate|River" },
        { "De Klassieker", "Ajax", "Feyenoord" },
        { "France - Allemagne", "#FRA", "#GER" },
        { "France - Italie", "#FRA", "#ITA" },
        { "France - Brésil", "#FRA", "#BRA" },
        { "Brésil - Argentine", "#BRA", "#ARG" },
        { "Angleterre - Écosse", "#ENG", "#SCO" },
        { "Espagne - Portugal", "#ESP", "#POR" },
        { "Pays-Bas - Allemagne", "#NED", "#GER" },
    };
    for (auto& c : L) {
        int h = frFindTeam(c[1]), a = frFindTeam(c[2]);
        if (h >= 0 && a >= 0 && h != a) v.push_back({ c[0], h, a });
    }
    return v;
}
static void frStartVs() {
    if (g_friendlyHome < 0 || g_friendlyHome >= (int)g_world.teams.size()) g_friendlyHome = frRandomTeam(1, -1, 0);
    if (g_friendlyAway < 0 || g_friendlyAway >= (int)g_world.teams.size() || g_friendlyAway == g_friendlyHome || isWomenTeam(g_friendlyAway) != isWomenTeam(g_friendlyHome))
        g_friendlyAway = frSameLevel(g_friendlyHome);
    g_frVsPick = 0; g_frSearching = 0; g_textMode = false;
    g_screen = SC_FRIENDLYVS;
}

static void frDrawCard(int t, int x, int y, int w, int h, const char* side, bool left) {
    DrawRectangle(x + 3, y + 3, w, h, Color{ 0, 0, 0, 90 });
    if (t < 0) {
        DrawRectangle(x, y, w, h, Color{ 14, 20, 44, 235 }); DrawRectangleLines(x, y, w, h, C_DIM);
        drawTextCentered(side, x + w / 2, y + 8, 10, C_HI, false);
        drawTextCentered("(aucune équipe)", x + w / 2, y + h / 2 - 5, 10, C_DIM, false);
        return;
    }
    const Team& T = g_world.teams[t];
    Color k = hexc(T.home.shirt);
    DrawRectangleGradientV(x, y, w, h, Color{ (unsigned char)(k.r * 0.45f), (unsigned char)(k.g * 0.45f), (unsigned char)(k.b * 0.45f), 240 }, Color{ 8, 12, 30, 245 });
    DrawRectangle(x, y, w, 2, k); DrawRectangle(x, y + h - 2, w, 2, hexc(T.home.shirt2));
    DrawRectangleLines(x, y, w, h, Color{ 255, 255, 255, 60 });
    drawTextPx(side, x + 6, y + 5, 10, C_HI);
    float bob = std::sin((float)GetTime() * 2.f + (left ? 0 : 1.6f)) * 1.5f;
    int kx = left ? x + 8 : x + w - 56;
    drawKitIcon(T.home, kx, y + 20 + (int)bob, 6);
    drawKitIcon(T.away, left ? kx + 50 : kx - 18, y + 52, 2);
    int tx = left ? x + 80 : x + 8, tw = w - 88;
    int ty = y + 20;
    for (auto& l : wrapText(T.name, tw)) { if (ty > y + 42) break; drawTextPx(l, tx, ty, 10, WHITE); ty += 11; }
    drawTextPx(T.kind == TK_NATION ? "Sélection nationale" : T.custom ? "Club créé" : isWomenTeam(t) ? "Club féminin" : "Club", tx, y + 46, 10, T.custom ? C_GOOD : C_DIM);
    int ry = y + 60;
    drawTextPx(fmt("Note %d", (int)T.rating), tx, ry, 10, C_TXT);
    DrawRectangle(tx + 48, ry + 2, tw - 50, 7, Color{ 0, 0, 0, 140 });
    DrawRectangle(tx + 48, ry + 2, (int)((tw - 50) * std::min(1.f, T.rating / 100.f)), 7, T.rating >= 80 ? C_GOOD : T.rating >= 65 ? C_HI : C_BAD);
    int yy = y + 76;
    if (!T.stadium.empty()) { drawTextPx(fitText(T.stadium, w - 14, 10), x + 7, yy, 10, C_DIM); yy += 12; }
    g_world.ensureSquad(t);
    if (!T.squad.empty()) {
        std::vector<int> ord(T.squad.size()); for (size_t q = 0; q < ord.size(); q++) ord[q] = (int)q;
        std::partial_sort(ord.begin(), ord.begin() + std::min<size_t>(3, ord.size()), ord.end(), [&](int a, int b) { return T.squad[a].overall() > T.squad[b].overall(); });
        for (int q = 0; q < 3 && q < (int)ord.size() && yy < y + h - 10; q++) {
            const Player& P = T.squad[ord[q]];
            drawTextPx(posName(P.pos), x + 7, yy, 10, C_DIM);
            drawTextPx(fitText(P.name, w - 50, 10), x + 26, yy, 10, C_TXT);
            std::string ov = fmt("%d", P.overall()); drawTextPx(ov, x + w - 8 - textWidth(ov, 10), yy, 10, C_HI);
            yy += 11;
        }
    }
}

static void screenFriendlyVs() {
    drawBackground("Match amical");
    int H = g_friendlyHome, A = g_friendlyAway;
    if (H >= (int)g_world.teams.size()) H = g_friendlyHome = -1;
    if (A >= (int)g_world.teams.size()) A = g_friendlyAway = -1;
    const int cw = 262, chh = 122, cy = 32;
    frDrawCard(H, 14, cy, cw, chh, "DOMICILE", true);
    frDrawCard(A, VW - 14 - cw, cy, cw, chh, "EXTÉRIEUR", false);
    {   // VS et pronostic
        int cx = VW / 2, vy = cy + 30;
        float pulse = 1.f + std::sin((float)GetTime() * 4.f) * 0.06f;
        DrawCircle(cx, vy + 8, 20 * pulse, Color{ 200, 30, 40, 230 }); DrawCircleLines(cx, vy + 8, 20 * pulse, Color{ 255, 230, 120, 255 });
        drawTextCentered("VS", cx, vy + 3, 10, WHITE, true);
        if (H >= 0 && A >= 0) {
            float d = g_world.teams[H].rating + 3.f - g_world.teams[A].rating;     // avantage du terrain
            float ph = 1.f / (1.f + std::pow(10.f, -d / 12.f));
            int pd = (int)std::lround(26 - std::min(20.f, std::fabs(d) * 0.9f));
            int phI = (int)std::lround(ph * (100 - pd)), paI = 100 - pd - phI;
            int by = vy + 40;
            drawTextCentered("PRONOSTIC", cx, by, 10, C_DIM, false);
            int bw = 66, bx = cx - bw / 2;
            DrawRectangle(bx, by + 13, bw * phI / 100, 6, hexc(g_world.teams[H].home.shirt));
            DrawRectangle(bx + bw * phI / 100, by + 13, bw * pd / 100, 6, Color{ 150, 150, 160, 255 });
            DrawRectangle(bx + bw * (phI + pd) / 100, by + 13, bw - bw * (phI + pd) / 100, 6, hexc(g_world.teams[A].home.shirt));
            DrawRectangleLines(bx - 1, by + 12, bw + 2, 8, Color{ 255, 255, 255, 120 });
            drawTextCentered(fmt("%d  %d  %d", phI, pd, paI), cx, by + 22, 10, C_TXT, false);
            const char* fav = std::fabs(d) < 4 ? "Match équilibré" : d > 0 ? "Favori : domicile" : "Favori : extérieur";
            drawTextCentered(fav, cx, by + 36, 10, C_HI, false);
        }
    }
    const auto& CL = frClassics();
    if (!CL.empty()) g_frClassic = (g_frClassic % (int)CL.size() + (int)CL.size()) % (int)CL.size();
    if (!g_frRecent.empty()) g_frRecentSel = (g_frRecentSel % (int)g_frRecent.size() + (int)g_frRecent.size()) % (int)g_frRecent.size();

    // recherche par nom
    if (g_frSearching) {
        textEdit(g_frQuery, 30);
        if (g_frQuery != g_frQueryDone) {
            g_frQueryDone = g_frQuery; g_frResults.clear(); g_frSearchLW = ListW();
            std::string k = sortKey(g_frQuery);
            if (k.size() >= 2) {
                for (int i = 0; i < (int)g_world.teams.size() && g_frResults.size() < 300; i++) if (frSenior(i) && sortKey(g_world.teams[i].name).find(k) != std::string::npos) g_frResults.push_back(i);
                std::stable_sort(g_frResults.begin(), g_frResults.end(), [](int a, int b) { return g_world.teams[a].rating > g_world.teams[b].rating; });
            }
        }
        int py = cy + chh + 6;
        DrawRectangle(14, py, VW - 28, VH - py - 18, Color{ 6, 10, 26, 240 }); DrawRectangleLines(14, py, VW - 28, VH - py - 18, C_HI);
        drawTextPx(g_frSearching == 1 ? "Rechercher l'équipe à domicile :" : "Rechercher l'équipe à l'extérieur :", 22, py + 6, 10, C_HI);
        DrawRectangle(210, py + 4, 300, 14, C_SEL);
        drawTextPx(g_frQuery + (((int)(GetTime() * 2)) % 2 ? "_" : ""), 214, py + 6, 10, BLACK);
        drawTextPx(fmt("%d résultat%s", (int)g_frResults.size(), g_frResults.size() > 1 ? "s" : ""), 520, py + 6, 10, C_DIM);
        int sel = listRun(g_frSearchLW, (int)g_frResults.size(), 22, py + 24, VW - 44, 11, 14, [&](int i, int x, int y, bool s) {
            int t = g_frResults[i]; const Team& T = g_world.teams[t];
            drawTeamRow(t, x, y + 1, VW - 44, s, fmt("%s  %d", T.kind == TK_NATION ? "sélection" : isWomenTeam(t) ? "féminin" : "club", (int)T.rating));
        });
        if (IN.start && !g_frResults.empty()) sel = g_frSearchLW.cur;
        if (sel >= 0 && sel < (int)g_frResults.size()) {
            int t = g_frResults[sel];
            int other = g_frSearching == 1 ? g_friendlyAway : g_friendlyHome;
            if (t == other) toast("Cette équipe est déjà choisie de l'autre côté");
            else {
                if (g_frSearching == 1) g_friendlyHome = t; else g_friendlyAway = t;
                if (other >= 0 && isWomenTeam(other) != isWomenTeam(t)) { toast("Séparation stricte : adversaire changé"); if (g_frSearching == 1) g_friendlyAway = frSameLevel(t); else g_friendlyHome = frSameLevel(t); }
                g_frSearching = 0; g_textMode = false;
            }
        }
        drawFooter("Tapez le nom   Haut/Bas : choisir   Entrée : valider   Échap : annuler");
        if (IN.back) { g_frSearching = 0; g_textMode = false; }
        return;
    }

    enum { R_HOME, R_AWAY, R_SHOME, R_SAWAY, R_SWAP, R_RAND, R_LEVEL, R_CLASSIC, R_RECENT, R_GO, R_N };
    std::vector<int> rows = { R_HOME, R_AWAY, R_SHOME, R_SAWAY, R_SWAP, R_RAND, R_LEVEL };
    if (!CL.empty()) rows.push_back(R_CLASSIC);
    if (!g_frRecent.empty()) rows.push_back(R_RECENT);
    rows.push_back(R_GO);
    int ly = cy + chh + 8;
    int cur = g_frVsLW.cur >= 0 && g_frVsLW.cur < (int)rows.size() ? rows[g_frVsLW.cur] : -1;
    if (IN.left || IN.right) {
        int d = IN.right ? 1 : -1;
        if (cur == R_RAND) { g_frFilter = (g_frFilter + d + 6) % 6; audioPlay(SFX_UI_MOVE); }
        else if (cur == R_CLASSIC && !CL.empty()) { g_frClassic = (g_frClassic + d + (int)CL.size()) % (int)CL.size(); audioPlay(SFX_UI_MOVE); }
        else if (cur == R_RECENT && !g_frRecent.empty()) { g_frRecentSel = (g_frRecentSel + d + (int)g_frRecent.size()) % (int)g_frRecent.size(); audioPlay(SFX_UI_MOVE); }
    }
    auto name = [](int t) { return t >= 0 ? g_world.teams[t].name : std::string("-"); };
    int sel = listRun(g_frVsLW, (int)rows.size(), 60, ly, VW - 120, 10, 17, [&](int i, int x, int y, bool s) {
        Color c = s ? Color{ 20, 20, 40, 255 } : C_TXT, cd = s ? Color{ 40, 40, 70, 255 } : C_DIM;
        int w = VW - 120;
        auto lr = [&](const std::string& lab, const std::string& val) {
            drawTextPx(lab, x + 8, y + 3, 10, c);
            std::string v = (s ? "< " : "") + val + (s ? " >" : "");
            v = fitText(v, w - 200, 10);
            drawTextPx(v, x + w - 8 - textWidth(v, 10), y + 3, 10, s ? c : C_HI);
        };
        switch (rows[i]) {
        case R_HOME: drawTextPx("Domicile : parcourir les championnats", x + 8, y + 3, 10, c); drawTextPx(fitText(name(g_friendlyHome), 220, 10), x + w - 8 - std::min(220, textWidth(name(g_friendlyHome), 10)), y + 3, 10, s ? c : C_HI); break;
        case R_AWAY: drawTextPx("Extérieur : parcourir les championnats", x + 8, y + 3, 10, c); drawTextPx(fitText(name(g_friendlyAway), 220, 10), x + w - 8 - std::min(220, textWidth(name(g_friendlyAway), 10)), y + 3, 10, s ? c : C_HI); break;
        case R_SHOME: drawTextPx("Rechercher l'équipe à domicile par son nom", x + 8, y + 3, 10, c); break;
        case R_SAWAY: drawTextPx("Rechercher l'équipe à l'extérieur par son nom", x + 8, y + 3, 10, c); break;
        case R_SWAP: drawTextPx("Inverser domicile / extérieur", x + 8, y + 3, 10, c); break;
        case R_RAND: lr("Tirage au sort de l'affiche", FR_FILTERS[g_frFilter]); break;
        case R_LEVEL: drawTextPx("Adversaire au hasard, de même niveau", x + 8, y + 3, 10, c); drawTextPx("note ± 3", x + w - 8 - textWidth("note ± 3", 10), y + 3, 10, cd); break;
        case R_CLASSIC: lr("Grandes affiches", CL[g_frClassic].label + " : " + g_world.teams[CL[g_frClassic].home].shortName + " - " + g_world.teams[CL[g_frClassic].away].shortName); break;
        case R_RECENT: lr("Affiches récentes", g_world.teams[g_frRecent[g_frRecentSel].first].shortName + " - " + g_world.teams[g_frRecent[g_frRecentSel].second].shortName); break;
        case R_GO: {
            bool ok = g_friendlyHome >= 0 && g_friendlyAway >= 0;
            drawTextCentered(ok ? ">>>  RÉGLER ET JOUER LE MATCH  <<<" : "Choisissez les deux équipes", x + w / 2, y + 3, 10, s ? c : ok ? C_GOOD : C_DIM, false);
            break;
        }
        }
    });
    drawFooter("Haut/Bas : choisir   Gauche/Droite : changer   OK : valider   Retour : menu principal");
    if (IN.back) { g_screen = SC_MAIN; return; }
    if (sel < 0 || sel >= (int)rows.size()) return;
    int gender = g_friendlyHome >= 0 ? (int)isWomenTeam(g_friendlyHome) : -1;
    switch (rows[sel]) {
    case R_HOME: g_frVsPick = 1; openPick(PM_FRIENDLY_HOME); break;
    case R_AWAY: g_frVsPick = 2; openPick(PM_FRIENDLY_AWAY); break;
    case R_SHOME: case R_SAWAY:
        g_frSearching = rows[sel] == R_SHOME ? 1 : 2; g_frQuery.clear(); g_frQueryDone = "\x01"; g_textMode = true; while (GetCharPressed() > 0) {}
        break;
    case R_SWAP: std::swap(g_friendlyHome, g_friendlyAway); break;
    case R_RAND: {
        int h = frRandomTeam(g_frFilter, -1, -1);
        int a = h >= 0 ? frRandomTeam(g_frFilter, h, (int)isWomenTeam(h)) : -1;
        if (h < 0 || a < 0) toast("Aucune équipe pour ce filtre"); else { g_friendlyHome = h; g_friendlyAway = a; }
        break;
    }
    case R_LEVEL: { int a = frSameLevel(g_friendlyHome); if (a >= 0) g_friendlyAway = a; else toast("Aucun adversaire de ce niveau"); (void)gender; break; }
    case R_CLASSIC: g_friendlyHome = CL[g_frClassic].home; g_friendlyAway = CL[g_frClassic].away; break;
    case R_RECENT: g_friendlyHome = g_frRecent[g_frRecentSel].first; g_friendlyAway = g_frRecent[g_frRecentSel].second; break;
    case R_GO:
        if (g_friendlyHome < 0 || g_friendlyAway < 0) { toast("Choisissez les deux équipes"); break; }
        if (g_friendlyHome == g_friendlyAway) { toast("Choisissez deux équipes différentes"); break; }
        if (isWomenTeam(g_friendlyHome) != isWomenTeam(g_friendlyAway)) { toast("Séparation stricte : deux équipes masculines ou deux équipes féminines"); break; }
        {
            std::pair<int, int> m(g_friendlyHome, g_friendlyAway);
            g_frRecent.erase(std::remove(g_frRecent.begin(), g_frRecent.end(), m), g_frRecent.end());
            g_frRecent.insert(g_frRecent.begin(), m);
            if (g_frRecent.size() > 8) g_frRecent.resize(8);
            g_frRecentSel = 0;
        }
        startSetup(g_friendlyHome, g_friendlyAway, false, -1, -1);
        g_mctx.back = SC_FRIENDLYVS;            // retour (réglages, fin du match) sur le face-à-face
        break;
    }
}
