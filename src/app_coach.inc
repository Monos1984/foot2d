// ------------------------------------------------------------------ carrière de sélectionneur : convocations, organisateurs et stades, bilan, offres
static std::vector<PoolPlayer> g_poolCache;
static int g_callFilter = 0, g_callCol = 0;
static ListW g_callPoolLW, g_callSelLW;
static Screen g_callBack = SC_HUB;

static void openCallup(Screen back) {
    g_poolCache = coachPool(g_career.coachNation, g_career.coachMaxAge(), g_career.coachTeamCat());
    g_callBack = back; g_callPoolLW = ListW(); g_callSelLW = ListW(); g_callCol = 0;
    g_screen = SC_CALLUP;
}

static bool isCalled(int pid) { for (auto& c : g_career.coachCalls) if (c.pid == pid) return true; return false; }

static void screenCallup() {
    Career& K = g_career;
    const Team& N = g_world.teams[K.coachTeam()];
    drawBackground("Convocations - " + N.name);
    static const char* FIL[] = { "Tous", "Gardiens", "Défenseurs", "Milieux", "Attaquants" };
    for (int f = 0; f < 5; f++) if (button(8 + f * 64, 32, 60, 13, FIL[f], g_callFilter == f)) { g_callFilter = f; g_callPoolLW = ListW(); }
    if (IN.pgUp) { g_callFilter = (g_callFilter + 4) % 5; g_callPoolLW = ListW(); }
    if (IN.pgDn) { g_callFilter = (g_callFilter + 1) % 5; g_callPoolLW = ListW(); }
    if (IN.left) g_callCol = 0;
    if (IN.right) g_callCol = 1;
    std::vector<int> pool;
    for (int i = 0; i < (int)g_poolCache.size(); i++) if (g_callFilter == 0 || g_poolCache[i].p->pos == g_callFilter - 1) pool.push_back(i);
    int cnt[4] = { 0, 0, 0, 0 };
    std::vector<int> selIdx;          // index dans coachCalls, triés par poste
    for (int i = 0; i < (int)K.coachCalls.size(); i++) selIdx.push_back(i);
    std::vector<const Player*> selP(K.coachCalls.size());
    std::vector<int> selClub(K.coachCalls.size(), -1);
    for (int i = 0; i < (int)K.coachCalls.size(); i++) { selP[i] = coachFind(K.coachCalls[i], &selClub[i]); if (selP[i]) cnt[std::min(3, (int)selP[i]->pos)]++; }
    std::stable_sort(selIdx.begin(), selIdx.end(), [&](int a, int b) { int pa = selP[a] ? selP[a]->pos : 9, pb = selP[b] ? selP[b]->pos : 9; if (pa != pb) return pa < pb; return (selP[a] ? selP[a]->overall() : 0) > (selP[b] ? selP[b]->overall() : 0); });
    auto clubName = [](int c) { return c < 0 ? std::string("Championnat local") : g_world.teams[c].name; };
    // vivier
    drawSection(8, 48, 330, fmt("JOUEURS SÉLECTIONNABLES (%d)", (int)pool.size()));
    int sa = listRun(g_callPoolLW, (int)pool.size(), 8, 60, 330, 21, 13, [&](int i, int x, int y, bool s) {
        const PoolPlayer& pp = g_poolCache[pool[i]];
        const Player& p = *pp.p;
        bool in = isCalled(p.id);
        Color c = s ? BLACK : in ? C_GOOD : C_TXT;
        drawTextPx(posName(p.pos), x + 3, y + 1, 10, s ? BLACK : C_DIM);
        drawTextPx(fitText(p.name, 120, 10), x + 22, y + 1, 10, c);
        drawTextPx(fmt("%d", p.age), x + 146, y + 1, 10, s ? BLACK : C_DIM);
        drawTextPx(fitText(clubName(pp.club), 120, 10), x + 166, y + 1, 10, s ? BLACK : C_DIM);
        drawTextPx(fmt("%d", p.overall()), x + 300, y + 1, 10, s ? BLACK : C_HI);
        if (in) DrawRectangle(x + 322, y + 3, 5, 5, C_GOOD);
        if (p.injured > 0) DrawRectangle(x + 314, y + 3, 5, 5, C_BAD);
    }, g_callCol == 0);
    // sélection
    drawSection(346, 48, 286, fmt("SÉLECTION (%d/%d)  G%d D%d M%d A%d", (int)K.coachCalls.size(), K.coachSquadMax(), cnt[0], cnt[1], cnt[2], cnt[3]));
    int sb = listRun(g_callSelLW, (int)selIdx.size(), 346, 60, 286, 21, 13, [&](int i, int x, int y, bool s) {
        const CoachCall& cc = K.coachCalls[selIdx[i]];
        const Player* p = selP[selIdx[i]];
        if (!p) { drawTextPx("(joueur introuvable)", x + 4, y + 1, 10, C_BAD); return; }
        drawTextPx(posName(p->pos), x + 3, y + 1, 10, s ? BLACK : C_DIM);
        drawTextPx(fitText(p->name, 110, 10), x + 22, y + 1, 10, s ? BLACK : C_TXT);
        drawTextPx(fitText(clubName(selClub[selIdx[i]]), 90, 10), x + 136, y + 1, 10, s ? BLACK : C_DIM);
        drawTextPx(fmt("%d sél.", cc.caps), x + 230, y + 1, 10, s ? BLACK : C_DIM);
        drawTextPx(fmt("%d", p->overall()), x + 268, y + 1, 10, s ? BLACK : C_HI);
    }, g_callCol == 1);
    if (IN.mouse.x < 340 && IN.click) g_callCol = 0;
    if (IN.mouse.x >= 346 && IN.click) g_callCol = 1;
    if (sa >= 0 && sa < (int)pool.size()) {
        const PoolPlayer& pp = g_poolCache[pool[sa]];
        if (isCalled(pp.p->id)) { K.coachCalls.erase(std::remove_if(K.coachCalls.begin(), K.coachCalls.end(), [&](const CoachCall& c) { return c.pid == pp.p->id; }), K.coachCalls.end()); }
        else if ((int)K.coachCalls.size() >= K.coachSquadMax()) toast(fmt("%d joueurs maximum : retirez d'abord un joueur", K.coachSquadMax()));
        else if (K.coachTeamCat() == 4 && pp.p->age > 23 && [&]() { int o = 0; for (auto& c : K.coachCalls) { const Player* q = coachFind(c); if (q && q->age > 23) o++; } return o >= 3; }())
            toast("Tournoi olympique : 3 joueurs de plus de 23 ans au maximum");
        else { CoachCall c; c.pid = pp.p->id; c.club = pp.club; K.coachCalls.push_back(c); }
    }
    if (sb >= 0 && sb < (int)selIdx.size()) K.coachCalls.erase(K.coachCalls.begin() + selIdx[sb]);
    int by = VH - 34;
    if (button(8, by, 150, 16, "Sélection automatique")) { K.coachAutoSelect(); toast("Sélection automatique"); }
    if (button(162, by, 120, 16, "Tout retirer")) askConfirm("Retirer tous les joueurs de la sélection ?", []() { g_career.coachCalls.clear(); });
    bool okN = K.coachCalls.size() >= 18 && cnt[0] >= 2;
    if (button(VW - 208, by, 200, 16, okN ? "Valider la liste" : "Liste incomplète (18 dont 2 gardiens)", false, okN) || (IN.start && okN)) {
        K.coachApplySquad(); toast("Liste validée"); openHub(); return;
    }
    drawFooter("Gauche/Droite : colonne   OK : ajouter / retirer   LB/RB : poste   Entrée : valider   Retour");
    if (IN.back) { if (okN) K.coachApplySquad(); g_screen = g_callBack; }
}

// ---------------- organisateurs et stades
static int g_hostSel = 0;
static ListW g_hostLW;
static void screenHosts() {
    Career& K = g_career;
    drawBackground("Organisation des compétitions");
    auto plan = K.coachPlan();
    int y = 34;
    drawSection(8, y, VW - 16, "CALENDRIER DES CAMPAGNES"); y += 12;
    for (int i = std::max(0, K.coachCamp - 1); i < (int)plan.size() && i < K.coachCamp + 4; i++) {
        const CoachCampaign& c = plan[i];
        std::vector<int> h = i == K.coachCamp ? K.intlHosts : i == K.coachCamp + 1 ? K.coachNextHosts : K.campaignHosts(c);
        std::string hs; for (int x : h) hs += (hs.empty() ? "" : ", ") + g_world.teams[x].name;
        std::string st = i < K.coachCamp ? "terminée" : i == K.coachCamp ? "EN COURS" : i == K.coachCamp + 1 ? "organisateur désigné" : "attribution à venir";
        DrawRectangle(8, y, VW - 16, 11, i == K.coachCamp ? Color{ 90, 80, 30, 255 } : C_ITEM);
        drawTextPx(fmt("%s %d", INTL_NAMES[c.type], c.year), 12, y, 10, i == K.coachCamp ? C_HI : C_TXT);
        drawTextPx(fitText(hs.empty() ? std::string("-") : hs, 300, 10), 200, y, 10, C_TXT);
        drawTextPx(st, VW - 14 - textWidth(st, 10), y, 10, C_DIM);
        y += 12;
    }
    y += 4;
    // organisateurs de la prochaine campagne : modifiables
    if (K.coachCamp + 1 < (int)plan.size()) {
        const CoachCampaign& nx = plan[K.coachCamp + 1];
        drawSection(8, y, VW - 16, fmt("%s %d : ORGANISATEURS (modifiables)", INTL_NAMES[nx.type], nx.year)); y += 13;
        int conf = nx.type == IT_EURO || nx.type == IT_EURO21 || nx.type == IT_EURO19 || nx.type == IT_EURO17 ? UEFA : nx.type == IT_CAN ? CAF : nx.type == IT_ASIA ? AFC : nx.type == IT_GOLD ? CONCACAF : nx.type == IT_COPA ? CONMEBOL : nx.type == IT_OFC ? OFC : -1;
        std::vector<int> cand; for (int i = 0; i < NUM_NATIONS; i++) if (nationEligible(i) && (conf < 0 || NATIONS[i].conf == conf)) cand.push_back(i);
        sortTeamsByName(cand);
        auto& H = K.coachNextHosts;
        if (H.empty() && !cand.empty()) H.push_back(cand[0]);
        std::string hs; for (int x : H) hs += (hs.empty() ? "" : ", ") + g_world.teams[x].name;
        drawTextPx("Organisation : " + hs, 12, y, 10, C_HI); y += 14;
        int cur = (int)(std::find(cand.begin(), cand.end(), H[0]) - cand.begin());
        if (cur >= (int)cand.size()) cur = 0;
        if (button(12, y, 150, 14, "< Pays principal") || IN.left) { H[0] = cand[(cur + (int)cand.size() - 1) % cand.size()]; }
        if (button(166, y, 150, 14, "Pays principal >") || IN.right) { H[0] = cand[(cur + 1) % cand.size()]; }
        if (button(320, y, 150, 14, "+ Co-organisateur", false, H.size() < 3)) {
            for (int k = 1; k < (int)cand.size(); k++) { int c = cand[(cur + k) % cand.size()]; if (std::find(H.begin(), H.end(), c) == H.end()) { H.push_back(c); break; } }
        }
        if (button(474, y, 150, 14, "- Co-organisateur", false, H.size() > 1)) H.pop_back();
        y += 20;
        // stades du pays principal
        auto v = nationVenues(H[0]);
        drawSection(8, y, VW - 16, "STADES DE " + g_world.teams[H[0]].name); y += 12;
        for (int i = 0; i < (int)v.size() && y < VH - 30; i++) { drawTextPx(fitText(v[i], VW / 2 - 20, 10), 12 + (i % 2) * (VW / 2), y, 10, i == 0 ? C_HI : C_TXT); if (i % 2) y += 11; }
    }
    drawFooter("Gauche/Droite : pays organisateur   Y : boutons   Retour");
    if (IN.back) openHub();
}

// ---------------- bilan du sélectionneur
static void screenCoachLog() {
    Career& K = g_career;
    const Team& N = g_world.teams[K.coachTeam()];
    drawBackground("Bilan du sélectionneur");
    drawKitIcon(N.home, 16, 36, 3);
    drawTextPx(N.name, 48, 36, 20, C_HI);
    drawTextPx("Objectif en cours : " + K.coachObjectiveText(), 48, 58, 10, C_TXT);
    int y = 76;
    drawSection(8, y, VW - 16, "CAMPAGNES"); y += 12;
    if (K.coachLog.empty()) { drawTextPx("Première campagne en cours.", 12, y, 10, C_DIM); y += 12; }
    for (auto& l : K.coachLog) { drawTextPx(fitText(l, VW - 24, 10), 12, y, 10, C_TXT); y += 11; }
    y += 6;
    drawSection(8, y, VW - 16, "MEILLEURS SÉLECTIONNÉS ET BUTEURS"); y += 12;
    std::vector<std::pair<int, int>> caps;
    for (int i = 0; i < (int)K.coachCalls.size(); i++) caps.push_back({ -(K.coachCalls[i].caps * 100 + K.coachCalls[i].goals), i });
    std::sort(caps.begin(), caps.end());
    for (int k = 0; k < (int)caps.size() && k < 12 && y < VH - 20; k++) {
        const CoachCall& c = K.coachCalls[caps[k].second];
        const Player* p = coachFind(c);
        drawTextPx(p ? p->name : std::string("?"), 12, y, 10, C_TXT);
        drawTextPx(fmt("%d sélection%s, %d but%s", c.caps, c.caps > 1 ? "s" : "", c.goals, c.goals > 1 ? "s" : ""), 220, y, 10, C_DIM);
        y += 11;
    }
    drawFooter("Retour");
    if (IN.back || IN.ok) openHub();
}

// ---------------- fin de campagne et offres
static std::vector<std::string> g_coachMsgs;
static std::vector<int> g_coachOffers;
static ListW g_coachJobLW;
static void coachNextCampaign() {
    g_coachMsgs.clear();
    g_career.endCoachCampaign(g_coachMsgs);
    if (g_career.coachSacked) {
        // offres d'autres fédérations : sélections un peu moins bien classées
        g_coachOffers.clear();
        std::vector<std::pair<float, int>> v;
        float r0 = g_world.teams[g_career.coachNation].rating;
        for (int i = 0; i < NUM_NATIONS; i++) if (nationEligible(i) && (g_career.coachCat < 2 || NATIONS[i].conf == UEFA) && i != g_career.coachNation && g_world.teams[i].rating < r0 - 2) v.push_back({ std::fabs(g_world.teams[i].rating - (r0 - 8)) + g_rng.frange(0, 6), i });
        std::sort(v.begin(), v.end());
        for (int k = 0; k < 5 && k < (int)v.size(); k++) g_coachOffers.push_back(v[k].second);
        g_coachJobLW = ListW();
        g_screen = SC_COACHJOBS;
        return;
    }
    g_career.startCoachCampaign();
    g_needAdvance = true;
    auto plan = g_career.coachPlan();
    g_hubNotice = fmt("Nouvelle campagne : %s %d", INTL_NAMES[g_career.intlType], g_career.year);
    openHub(); autosave();
}

static void screenCoachJobs() {
    drawBackground("Offres de sélections");
    int y = 34;
    for (auto& m : g_coachMsgs) { drawTextCentered(m, VW / 2, y, 10, C_TXT); y += 12; }
    y += 6;
    std::vector<std::string> items;
    for (int n : g_coachOffers) items.push_back(fmt("%s  (note %d)", g_world.teams[n].name.c_str(), (int)g_world.teams[n].rating));
    items.push_back("Prendre une année sabbatique (menu principal)");
    int s = menuRun(g_coachJobLW, items, y + 10, 400);
    drawFooter("OK : accepter l'offre");
    if (s < 0) return;
    if (s >= (int)g_coachOffers.size()) { g_careerActive = false; g_screen = SC_MAIN; return; }
    Career& K = g_career;
    K.coachNation = g_coachOffers[s];
    K.coachCalls.clear();
    K.coachLog.push_back("Nouveau poste : sélectionneur de " + g_world.teams[K.coachTeam()].name);
    g_world.ensureSquad(K.coachNation);
    K.coachAutoSelect();
    K.coachNextHosts.clear();
    K.startCoachCampaign();
    g_needAdvance = true;
    g_hubNotice = "Nouveau poste : sélectionneur de " + g_world.teams[K.coachTeam()].name;
    openHub(); autosave();
}
