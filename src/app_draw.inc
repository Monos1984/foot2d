// ------------------------------------------------------------------ tirage au sort (quand le club participe à un nouveau tour)
static std::vector<std::pair<int, int>> g_drawQueue;   // (compétition, phase)
static std::map<int, int> g_drawSeen;                  // compétition -> nombre de phases déjà vues
static int g_drawSeenYear = -1;
static float g_drawT = 0;
static ListW g_drawLW;

// repère les nouveaux tirages concernant les équipes contrôlées ; vrai s'il y en a à montrer
static bool collectDraws() {
    Season& S = g_career.season;
    if (g_drawQueue.empty()) { g_drawT = 0; g_drawLW = ListW(); }
    if (g_drawSeenYear != g_career.year) { g_drawSeen.clear(); g_drawSeenYear = g_career.year; }
    for (int c = 0; c < (int)S.comps.size(); c++) {
        const Competition& C = S.comps[c];
        int& seen = g_drawSeen[c];
        if ((int)C.stages.size() <= seen) continue;
        for (int s = seen; s < (int)C.stages.size(); s++) {
            if (C.format == FMT_LEAGUE || C.format == FMT_SINGLE || C.kind == 10 || C.kind == 12) continue;
            const Stage& st = C.stages[s];
            if (C.format == FMT_TOURNAMENT && st.type == ST_KO) continue;   // tableau fixé : pas de tirage
            bool in = false;
            for (auto& t : st.ties) if (S.isControlled(t.a) || (t.b >= 0 && S.isControlled(t.b))) in = true;
            for (auto& g : st.groups) for (int t : g) if (S.isControlled(t)) in = true;
            if (in && (st.ties.size() > 1 || !st.groups.empty())) g_drawQueue.push_back({ c, s });
        }
        seen = (int)C.stages.size();
    }
    return !g_drawQueue.empty();
}

// après un chargement : les tirages déjà effectués ne sont pas remontrés
static void primeDraws() {
    g_drawQueue.clear(); g_drawSeen.clear(); g_drawSeenYear = g_career.year;
    for (int c = 0; c < (int)g_career.season.comps.size(); c++) g_drawSeen[c] = (int)g_career.season.comps[c].stages.size();
}

static void drawBall(int x, int y, int r, Color c) {
    DrawCircle(x, y, (float)r, c);
    DrawCircle(x - r / 3, y - r / 3, (float)std::max(1, r / 3), Color{ 255, 255, 255, 140 });
}

static void screenDraw(float dt) {
    if (g_drawQueue.empty()) { openHub(); return; }
    Season& S = g_career.season;
    int c = g_drawQueue.front().first, s = g_drawQueue.front().second;
    if (c < 0 || c >= (int)S.comps.size() || s >= (int)S.comps[c].stages.size()) { g_drawQueue.erase(g_drawQueue.begin()); return; }
    const Competition& C = S.comps[c];
    const Stage& st = C.stages[s];
    g_drawT += dt;
    drawBackground("Tirage au sort");
    drawTextCentered(C.name + " - " + st.name, VW / 2, 32, 10, C_HI);
    bool done = false;
    auto nameLvl = [&](int t) { return g_world.teams[t].name + (g_world.teams[t].kind == TK_CLUB ? " (" + shortLevel(t) + ")" : std::string()); };
    if (!st.groups.empty()) {
        // ---- phase de groupes : les groupes se remplissent chapeau par chapeau
        int ng = (int)st.groups.size();
        int per = 0; for (auto& g : st.groups) per = std::max(per, (int)g.size());
        int total = ng * per;
        int shown = std::min(total, (int)(g_drawT / 0.18f));
        done = shown >= total;
        int cols = std::min(4, ng), rowsG = (ng + cols - 1) / cols;
        int gw = (VW - 20) / cols, gh = 14 + per * 12;
        for (int g = 0; g < ng; g++) {
            int gx = 10 + (g % cols) * gw, gy = 48 + (g / cols) * (gh + 6);
            bool mine = false; for (int t : st.groups[g]) if (S.isControlled(t)) mine = true;
            DrawRectangle(gx, gy, gw - 6, gh, mine ? Color{ 90, 80, 30, 255 } : C_ITEM);
            DrawRectangleLines(gx, gy, gw - 6, gh, mine ? C_SEL : Color{ 70, 100, 160, 255 });
            drawTextPx(fmt("Groupe %c", 'A' + g), gx + 4, gy + 1, 10, C_HI);
            for (int k = 0; k < (int)st.groups[g].size(); k++) {
                int idx = k * ng + g;      // ordre de tirage : chapeau k, groupe g
                if (idx >= shown) continue;
                int t = st.groups[g][k];
                drawKitIcon(g_world.teams[t].home, gx + 4, gy + 14 + k * 12, 1);
                drawTextPx(fitText(g_world.teams[t].name, gw - 30, 10), gx + 18, gy + 14 + k * 12, 10, S.isControlled(t) ? C_GOOD : C_TXT);
            }
        }
        (void)rowsG;
    } else {
        // ---- élimination directe : d'abord le match du club, puis tous les autres
        int myTie = -1;
        for (int i = 0; i < (int)st.ties.size(); i++) if (S.isControlled(st.ties[i].a) || (st.ties[i].b >= 0 && S.isControlled(st.ties[i].b))) myTie = i;
        // urne
        int ux = VW / 2, uy = 78;
        DrawCircle(ux, uy, 26, Color{ 200, 220, 255, 60 });
        DrawCircleLines(ux, uy, 26, Color{ 220, 230, 255, 160 });
        for (int k = 0; k < 9; k++) {
            float a = g_drawT * (2.f + k * 0.3f) + k;
            drawBall(ux + (int)(std::cos(a) * (8 + k % 3 * 5)), uy + 6 + (int)(std::sin(a * 1.3f) * 8), 4, k % 2 ? Color{ 230, 230, 240, 255 } : Color{ 240, 200, 60, 255 });
        }
        if (myTie >= 0) {
            const Tie& T = st.ties[myTie];
            int h = T.a, a = T.b;
            if (T.m1 >= 0) { h = C.matches[T.m1].home; a = C.matches[T.m1].away; }
            float t1 = 0.9f, t2 = 1.9f;
            int y = 112;
            DrawRectangle(40, y, VW - 80, 44, Color{ 0, 0, 0, 110 });
            if (g_drawT > t1) {
                drawKitIcon(g_world.teams[h].home, 60, y + 8, 2);
                drawTextPx(fitText(nameLvl(h), 220, 10), 88, y + 8, 10, S.isControlled(h) ? C_GOOD : C_TXT);
            } else if (g_drawT > 0.2f) drawBall(ux - 100 + (int)((g_drawT - 0.2f) / 0.7f * -60), uy + 40, 5, C_HI);
            drawTextCentered(a < 0 ? "" : "contre", VW / 2, y + 12, 10, C_DIM, false);
            if (a < 0 && g_drawT > t1) drawTextCentered("exempté : qualifié directement pour le tour suivant", VW / 2, y + 28, 10, C_HI, false);
            if (a >= 0 && g_drawT > t2) {
                drawKitIcon(g_world.teams[a].home, VW - 84, y + 8, 2);
                std::string nm = fitText(nameLvl(a), 220, 10);
                drawTextPx(nm, VW - 92 - textWidth(nm, 10), y + 8, 10, S.isControlled(a) ? C_GOOD : C_TXT);
            } else if (a >= 0 && g_drawT > t1 + 0.1f) drawBall(ux + 100 + (int)((g_drawT - t1) * 60), uy + 40, 5, C_HI);
            if (g_drawT > t2 && a >= 0) {
                std::string info = st.legs == 2 ? "Match aller à " + g_world.teams[h].name + ", retour à " + g_world.teams[a].name : (C.matches[T.m1].neutral ? "Terrain neutre" : "Match à " + (g_world.teams[h].stadium.empty() ? g_world.teams[h].name : g_world.teams[h].stadium));
                drawTextCentered(fitText(info, VW - 100, 10), VW / 2, y + 28, 10, C_DIM, false);
            }
        }
        float t0 = myTie >= 0 ? 2.4f : 0.3f;
        int n = (int)st.ties.size();
        int shown = g_drawT < t0 ? 0 : std::min(n, (int)((g_drawT - t0) / 0.06f) + 1);
        done = g_drawT > t0 && shown >= n;
        int y = 162;
        drawTextPx(fmt("Tous les matchs (%d)", n), 20, y, 10, C_DIM);
        y += 12;
        int rows = (VH - 40 - y) / 12;
        int half = (n + 1) / 2;
        int nrow = std::max(1, std::min(half, rows));
        // défilement automatique pendant l'animation, puis libre
        if (!done) g_drawLW.top = std::max(0, std::min(half - nrow, (shown + 1) / 2 - nrow));
        else {
            if (IN.down || IN.wheel < 0) g_drawLW.top = std::min(std::max(0, half - nrow), g_drawLW.top + (IN.wheel < 0 ? 3 : 1));
            if (IN.up || IN.wheel > 0) g_drawLW.top = std::max(0, g_drawLW.top - (IN.wheel > 0 ? 3 : 1));
        }
        for (int r = 0; r < nrow; r++) for (int col = 0; col < 2; col++) {
            int i = (g_drawLW.top + r) * 2 + col;
            if (i >= n || i >= shown) continue;
            const Tie& T = st.ties[i];
            int h = T.a, a = T.b;
            if (T.m1 >= 0) { h = C.matches[T.m1].home; a = C.matches[T.m1].away; }
            int x = 16 + col * (VW - 32) / 2, yy = y + r * 12;
            bool mine = i == myTie;
            DrawRectangle(x, yy, (VW - 32) / 2 - 4, 11, mine ? Color{ 90, 80, 30, 255 } : ((r % 2) ? C_ITEM : Color{ 38, 62, 108, 255 }));
            std::string l = g_world.teams[h].shortName + (a >= 0 ? " - " + g_world.teams[a].shortName : "  (exempté)");
            drawTextPx(fitText(l, (VW - 32) / 2 - 12, 10), x + 4, yy, 10, mine ? C_HI : C_TXT);
        }
    }
    bool more = g_drawQueue.size() > 1;
    drawFooter(done ? (more ? "OK : tirage suivant" : "OK : continuer") : "OK : passer l'animation");
    if (IN.ok || IN.click || IN.start || IN.back) {
        if (!done) g_drawT = 999;
        else { g_drawQueue.erase(g_drawQueue.begin()); g_drawT = 0; g_drawLW = ListW(); if (g_drawQueue.empty()) openHub(); }
    }
}
