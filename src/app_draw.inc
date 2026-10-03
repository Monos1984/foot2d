// ------------------------------------------------------------------ tirage au sort (quand le club participe à un nouveau tour)
static std::vector<std::pair<int, int>> g_drawQueue;   // (compétition, phase)
static std::map<int, int> g_drawSeen;                  // compétition -> nombre de phases déjà vues
static int g_drawSeenYear = -1;
static float g_drawT = 0;
static bool g_drawAck = false;          // tirage du club validé par le joueur
static float g_drawHoldT = -1;          // instant où le tirage s'arrête sur le match du club
static ListW g_drawLW;

// repère les nouveaux tirages concernant les équipes contrôlées ; vrai s'il y en a à montrer
static bool collectDraws() {
    Season& S = g_career.season;
    if (g_drawQueue.empty()) { g_drawT = 0; g_drawAck = false; g_drawLW = ListW(); }
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

// urne en verre : boules qui tournent, étiquette dessous
static void drawBowl(int x, int y, const std::string& label, int nballs, float t, Color ballC) {
    DrawEllipse(x, y + 30, 26, 5, Color{ 0, 0, 0, 90 });
    DrawRectangle(x - 6, y + 20, 12, 10, Color{ 150, 150, 165, 255 });            // pied
    DrawRectangle(x - 14, y + 28, 28, 3, Color{ 120, 120, 135, 255 });
    DrawCircle(x, y, 22, Color{ 190, 215, 255, 55 });
    for (int k = 0; k < std::min(14, nballs); k++) {
        float an = t * (1.6f + k * 0.23f) + k * 1.7f;
        drawBall(x + (int)(std::cos(an) * (5 + (k % 3) * 5)), y + 5 + (int)(std::sin(an * 1.3f) * 7), 4, k % 2 ? Color{ 235, 235, 245, 255 } : ballC);
    }
    DrawCircleLines(x, y, 22, Color{ 225, 235, 255, 190 });
    DrawRectangle(x - 9, y - 23, 18, 3, Color{ 225, 235, 255, 160 });              // col de l'urne
    DrawCircle(x - 9, y - 9, 3, Color{ 255, 255, 255, 110 });                        // reflet
    drawTextCentered(label, x + (x < VW / 2 ? -62 : 62), y - 4, 10, C_DIM, false);
}
// boule qui s'ouvre et révèle le nom (animation 0..1)
static void drawOpening(int x, int y, float u, Color c) {
    if (u < 0.5f) { drawBall(x, y, 5, c); return; }
    int d = (int)((u - 0.5f) * 16);
    DrawCircleSector(Vector2{ (float)(x - d), (float)y }, 5, 90, 270, 8, c);
    DrawCircleSector(Vector2{ (float)(x + d), (float)y }, 5, -90, 90, 8, c);
    DrawRectangle(x - d + 1, y - 2, 2 * d - 2, 4, Color{ 250, 245, 225, 255 });      // papier
}

static void screenDraw(float dt) {
    if (g_drawQueue.empty()) { openHub(); return; }
    Season& S = g_career.season;
    int c = g_drawQueue.front().first, s = g_drawQueue.front().second;
    if (c < 0 || c >= (int)S.comps.size() || s >= (int)S.comps[c].stages.size()) { g_drawQueue.erase(g_drawQueue.begin()); return; }
    const Competition& C = S.comps[c];
    const Stage& st = C.stages[s];
    g_drawT += dt;
    g_drawHoldT = -1;
    bool drawWait = false;
    drawBackground("Tirage au sort");
    drawTextCentered(C.name + " - " + st.name, VW / 2, 32, 10, C_HI);
    bool done = false;
    auto nameLvl = [&](int t) { return g_world.teams[t].name + (g_world.teams[t].kind == TK_CLUB ? " (" + shortLevel(t) + ")" : std::string()); };
    if (st.type == ST_SWISS) {
        // ---- phase de ligue (nouvelle formule) : tirage des adversaires du club, chapeau par chapeau
        int me = -1; for (int t : st.groups[0]) if (S.isControlled(t)) me = t;
        std::vector<int> teams = st.groups[0];
        int P = std::max(1, C.qualSpots), per = std::max(1, (int)teams.size() / P);
        auto potOf = [&](int t) { for (int i = 0; i < (int)teams.size(); i++) if (teams[i] == t) return i / per; return 0; };
        struct Opp { int team, pot; bool home; double t; };
        std::vector<Opp> opp;
        for (auto& R : st.rounds) for (int mi : R.m) { const MatchRes& m = C.matches[mi]; if (m.home == me || m.away == me) { int o = m.home == me ? m.away : m.home; opp.push_back({ o, potOf(o), m.home == me, R.time }); } }
        std::stable_sort(opp.begin(), opp.end(), [](const Opp& a, const Opp& b) { if (a.pot != b.pot) return a.pot < b.pot; return a.home > b.home; });
        if (me >= 0) {
            drawKitIcon(g_world.teams[me].home, VW / 2 - 12, 46, 3);
            drawTextCentered(g_world.teams[me].name + fmt("  (chapeau %d)", potOf(me) + 1), VW / 2, 76, 10, C_GOOD);
        }
        int shown = std::min((int)opp.size(), (int)(g_drawT / 0.7f));
        done = shown >= (int)opp.size();
        int y0 = 94;
        for (int k = 0; k < shown; k++) {
            const Opp& o = opp[k];
            int col = k % 2, row = k / 2;
            int x = 20 + col * (VW - 40) / 2, y = y0 + row * 30;
            DrawRectangle(x, y, (VW - 40) / 2 - 8, 26, C_ITEM);
            DrawRectangleLines(x, y, (VW - 40) / 2 - 8, 26, Color{ 70, 100, 160, 255 });
            drawTextPx(fmt("Chapeau %d", o.pot + 1), x + 4, y + 1, 10, C_DIM);
            drawKitIcon(g_world.teams[o.team].home, x + 6, y + 12, 1);
            drawTextPx(fitText(g_world.teams[o.team].name + " (" + NATIONS[std::max(0, (int)g_world.teams[o.team].nation)].code + ")", 200, 10), x + 20, y + 12, 10, C_TXT);
            std::string ha = o.home ? "à domicile" : "à l'extérieur";
            drawTextPx(ha, x + (VW - 40) / 2 - 16 - textWidth(ha, 10), y + 12, 10, o.home ? C_GOOD : C_HI);
        }
        if (me < 0) { drawTextCentered("Tirage de la phase de ligue effectué.", VW / 2, 120, 10, C_TXT); done = true; }
        drawTextCentered(fmt("%d clubs, %d matchs chacun contre %d adversaires différents", (int)teams.size(), (int)opp.size(), (int)opp.size()), VW / 2, VH - 40, 10, C_DIM);
    } else if (!st.groups.empty()) {
        // ---- phase de groupes : les groupes se remplissent chapeau par chapeau
        int ng = (int)st.groups.size();
        int per = 0; for (auto& g : st.groups) per = std::max(per, (int)g.size());
        int total = ng * per;
        int shown = std::min(total, (int)(g_drawT / 0.35f));
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
                if (idx == shown - 1 && !done) { DrawRectangle(gx + 1, gy + 13 + k * 12, gw - 8, 11, Color{ 240, 200, 60, 90 }); drawBall(gx + gw - 16, gy + 18 + k * 12, 4, Color{ 240, 200, 60, 255 }); }
                drawKitIcon(g_world.teams[t].home, gx + 4, gy + 14 + k * 12, 1);
                drawTextPx(fitText(g_world.teams[t].name, gw - 30, 10), gx + 18, gy + 14 + k * 12, 10, S.isControlled(t) ? C_GOOD : C_TXT);
            }
        }
        (void)rowsG;
    } else {
        // ---- élimination directe : tirage boule par boule. Jusqu'à 16 équipes : tous les matchs sont tirés un par un ;
        // au-delà : une sélection d'affiches (dont celle du club, à une place quelconque), puis le reste du tableau
        int n = (int)st.ties.size();
        std::vector<int> myTies;
        for (int i = 0; i < n; i++) if (S.isControlled(st.ties[i].a) || (st.ties[i].b >= 0 && S.isControlled(st.ties[i].b))) myTies.push_back(i);
        Rng rr((uint64_t)(c * 7919 + s * 131 + g_career.year));
        std::vector<int> ord, rest;
        for (int i = 0; i < n; i++) ord.push_back(i);
        rr.shuffle(ord);
        int F = n <= 8 ? n : std::min(n, 6);
        if (n > 8) {
            // affiches : le(s) match(s) du club + les chocs entre les équipes les mieux cotées
            std::vector<int> feat = myTies;
            std::vector<int> byRate = ord;
            // valeur de chaque affiche tirée une seule fois : un comparateur qui change d'avis à chaque appel fait planter le tri
            std::vector<float> val(n);
            for (int k = 0; k < n; k++) { const Tie& T = st.ties[k]; val[k] = (T.a >= 0 ? g_world.teams[T.a].rating : 0.f) + (T.b >= 0 ? g_world.teams[T.b].rating : 0.f) + rr.frange(0, 10); }
            std::stable_sort(byRate.begin(), byRate.end(), [&](int x, int y) { return val[x] > val[y]; });
            for (int k : byRate) if ((int)feat.size() < F && std::find(feat.begin(), feat.end(), k) == feat.end()) feat.push_back(k);
            rr.shuffle(feat);
            F = (int)feat.size();
            for (int k : ord) if (std::find(feat.begin(), feat.end(), k) == feat.end()) rest.push_back(k);
            ord = feat; for (int k : rest) ord.push_back(k);
        }
        // deux urnes : équipes qui reçoivent (à gauche) et adversaires (à droite)
        int ux = VW / 2, uy = 60;
        int leftB = std::max(0, n - (int)(g_drawT / 1.7f));
        drawBowl(ux - 130, uy, "Équipe 1", std::max(2, leftB), g_drawT, Color{ 240, 200, 60, 255 });
        drawBowl(ux + 130, uy, "Équipe 2", std::max(2, leftB), g_drawT + 1.3f, Color{ 90, 170, 255, 255 });
        // table du tirage
        DrawRectangle(ux - 70, uy + 22, 140, 6, Color{ 60, 40, 30, 255 });
        DrawRectangle(ux - 70, uy + 21, 140, 2, Color{ 120, 90, 60, 255 });
        const float D = 1.7f, T0 = 0.4f;
        float tFeat = T0 + F * D;
        // arrêt sur le match du club : le joueur valide avant la suite du tirage
        {
            int uk = -1;
            for (int j = 0; j < F; j++) if (std::find(myTies.begin(), myTies.end(), ord[j]) != myTies.end()) { uk = j; break; }
            if (uk >= 0) {
                g_drawHoldT = T0 + uk * D + 1.25f;
                if (!g_drawAck && g_drawT >= g_drawHoldT) { g_drawT = g_drawHoldT; drawWait = true; }
            }
        }
        int cur = g_drawT < T0 ? -1 : std::min(F - 1, (int)((g_drawT - T0) / D));
        // match en cours de tirage (grand format)
        if (cur >= 0 && g_drawT < tFeat) {
            const Tie& T = st.ties[ord[cur]];
            int h = T.a, a = T.b;
            if (T.m1 >= 0) { h = C.matches[T.m1].home; a = C.matches[T.m1].away; }
            float lt = g_drawT - T0 - cur * D;
            int y = 100;
            bool mine = std::find(myTies.begin(), myTies.end(), ord[cur]) != myTies.end();
            DrawRectangle(40, y, VW - 80, 44, mine && lt > 1.1f ? Color{ 90, 80, 30, 200 } : Color{ 0, 0, 0, 110 });
            drawTextPx(fmt("Match %d", cur + 1), 44, y + 2, 10, C_DIM);
            if (lt > 0.5f) {
                drawKitIcon(g_world.teams[h].home, 60, y + 12, 2);
                drawTextPx(fitText(nameLvl(h), 220, 10), 88, y + 14, 10, S.isControlled(h) ? C_GOOD : C_TXT);
            } else { float u = lt / 0.5f; drawOpening(ux - 130 + (int)(u * 60), uy - 10 + (int)(u * u * 40), u, Color{ 240, 200, 60, 255 }); }
            drawTextCentered(a < 0 ? "" : "contre", VW / 2, y + 16, 10, C_DIM, false);
            if (a < 0 && lt > 0.5f) drawTextCentered("exempté : qualifié directement pour le tour suivant", VW / 2, y + 30, 10, C_HI, false);
            if (a >= 0 && lt > 1.1f) {
                drawKitIcon(g_world.teams[a].home, VW - 84, y + 12, 2);
                std::string nm = fitText(nameLvl(a), 220, 10);
                drawTextPx(nm, VW - 92 - textWidth(nm, 10), y + 14, 10, S.isControlled(a) ? C_GOOD : C_TXT);
                std::string info = st.legs == 2 ? "Match aller à " + g_world.teams[h].name + ", retour à " + g_world.teams[a].name : (C.matches[T.m1].neutral ? "Terrain neutre" : "Match à " + (g_world.teams[h].stadium.empty() ? g_world.teams[h].name : g_world.teams[h].stadium));
                drawTextCentered(fitText(info, VW - 100, 10), VW / 2, y + 32, 10, C_DIM, false);
            } else if (a >= 0 && lt > 0.6f) { float u = (lt - 0.6f) / 0.5f; drawOpening(ux + 130 - (int)(u * 60), uy - 10 + (int)(u * u * 40), u, Color{ 90, 170, 255, 255 }); }
        } else if (g_drawT >= tFeat && !myTies.empty()) {
            const Tie& T = st.ties[myTies[0]];
            int h = T.a, a = T.b;
            if (T.m1 >= 0) { h = C.matches[T.m1].home; a = C.matches[T.m1].away; }
            int y = 100;
            DrawRectangle(40, y, VW - 80, 44, Color{ 90, 80, 30, 200 });
            drawTextPx("Votre match", 44, y + 2, 10, C_HI);
            drawKitIcon(g_world.teams[h].home, 60, y + 12, 2);
            drawTextPx(fitText(nameLvl(h), 220, 10), 88, y + 14, 10, S.isControlled(h) ? C_GOOD : C_TXT);
            if (a >= 0) {
                drawTextCentered("contre", VW / 2, y + 16, 10, C_DIM, false);
                drawKitIcon(g_world.teams[a].home, VW - 84, y + 12, 2);
                std::string nm = fitText(nameLvl(a), 220, 10);
                drawTextPx(nm, VW - 92 - textWidth(nm, 10), y + 14, 10, S.isControlled(a) ? C_GOOD : C_TXT);
            } else drawTextCentered("exempté", VW / 2, y + 16, 10, C_HI, false);
        }
        if (drawWait) {
            float pl = 0.6f + 0.4f * std::sin((float)GetTime() * 5);
            DrawRectangle(16, 147, VW - 32, 13, Color{ 240, 200, 60, (unsigned char)(200 * pl) });
            drawTextCentered("VOTRE ADVERSAIRE EST CONNU !  OK : continuer le tirage", VW / 2, 148, 10, BLACK, false);
        }
        // matchs déjà tirés (ordre du tirage) ; au-delà des affiches, le reste du tableau défile rapidement
        int shown = cur < 0 ? 0 : g_drawT < tFeat ? cur + ((g_drawT - T0 - cur * D) > 1.1f ? 1 : 0) : std::min(n, F + (int)((g_drawT - tFeat) / 0.05f) + 1);
        if (F == n && g_drawT >= tFeat) shown = n;
        done = g_drawT >= tFeat && shown >= n;
        int y = 150;
        if (!drawWait) drawTextPx(n > 8 ? fmt("Matchs tirés : %d / %d  (affiches puis reste du tableau)", shown, n) : fmt("Matchs tirés : %d / %d", shown, n), 20, y, 10, C_DIM);
        y += 12;
        int rows = (VH - 40 - y) / 12;
        int half = (n + 1) / 2;
        int nrow = std::max(1, std::min(half, rows));
        if (!done) g_drawLW.top = std::max(0, std::min(half - nrow, (shown + 1) / 2 - nrow));
        else {
            if (IN.down || IN.wheel < 0) g_drawLW.top = std::min(std::max(0, half - nrow), g_drawLW.top + (IN.wheel < 0 ? 3 : 1));
            if (IN.up || IN.wheel > 0) g_drawLW.top = std::max(0, g_drawLW.top - (IN.wheel > 0 ? 3 : 1));
        }
        for (int r = 0; r < nrow; r++) for (int col = 0; col < 2; col++) {
            int j = (g_drawLW.top + r) * 2 + col;
            if (j >= n || j >= shown) continue;
            int i = ord[j];
            const Tie& T = st.ties[i];
            int h = T.a, a = T.b;
            if (T.m1 >= 0) { h = C.matches[T.m1].home; a = C.matches[T.m1].away; }
            int x = 16 + col * (VW - 32) / 2, yy = y + r * 12;
            bool mine = std::find(myTies.begin(), myTies.end(), i) != myTies.end();
            DrawRectangle(x, yy, (VW - 32) / 2 - 4, 11, mine ? Color{ 90, 80, 30, 255 } : j < F && n > 8 ? Color{ 60, 50, 90, 255 } : ((r % 2) ? C_ITEM : Color{ 38, 62, 108, 255 }));
            std::string l = fmt("%d. ", j + 1) + g_world.teams[h].shortName + (a >= 0 ? " - " + g_world.teams[a].shortName : "  (exempté)");
            drawTextPx(fitText(l, (VW - 32) / 2 - 12, 10), x + 4, yy, 10, mine ? C_HI : C_TXT);
        }
    }
    bool more = g_drawQueue.size() > 1;
    drawFooter(drawWait ? "OK : valider et continuer le tirage" : done ? (more ? "OK : tirage suivant" : "OK : continuer") : "OK : passer l'animation");
    if (IN.ok || IN.click || IN.start || IN.back) {
        if (drawWait) { g_drawAck = true; g_drawT += 0.02f; }
        else if (!done) g_drawT = (!g_drawAck && g_drawHoldT > 0) ? g_drawHoldT : 999;     // l'animation saute jusqu'au match du club
        else { g_drawQueue.erase(g_drawQueue.begin()); g_drawT = 0; g_drawAck = false; g_drawLW = ListW(); if (g_drawQueue.empty()) openHub(); }
    }
}
