// ------------------------------------------------------------------ carrière de joueur / joueuse, vie privée, paris, « valise à l'arbitre »
// g_lifeNew : déclaré dans app.cpp
static int g_lifeRow = 0; static bool g_lifeTyping = false; static ListW g_lifeLW;
static int g_lifeNat = -1;
static int g_betSel = 0, g_betPick = 0, g_betStake = 50;

static void openLifeNew() {
    g_lifeNew = Player();
    g_lifeNew.name = "Nouveau joueur"; g_lifeNew.age = 17; g_lifeNew.pos = POS_FW; g_lifeNew.skin = 0; g_lifeNew.hair = 0; g_lifeNew.gender = 0;
    g_lifeNat = g_world.nationIndex("FRA");
    g_lifeRow = 0; g_lifeTyping = false;
    g_screen = SC_LIFENEW;
}

// création du joueur incarné : niveau selon l'âge, points forts selon le poste
static Player lifeBuildPlayer(const Team& T) {
    Player p = g_lifeNew;
    int a = p.age;
    int base = a <= 17 ? 48 : a <= 19 ? 53 : a <= 22 ? 58 : 62;
    if (T.status != CS_PRO) base -= 3;
    auto set = [&](uint8_t& v, int d) { v = (uint8_t)std::max(20, std::min(90, base + d + g_rng.range(-3, 3))); };
    switch (p.pos) {
    case POS_GK: set(p.keep, 10); set(p.speed, -8); set(p.shoot, -20); set(p.pass, -5); set(p.tackle, -15); break;
    case POS_DF: set(p.tackle, 8); set(p.speed, 0); set(p.pass, -2); set(p.shoot, -12); p.keep = 15; break;
    case POS_MF: set(p.pass, 8); set(p.speed, 0); set(p.shoot, -2); set(p.tackle, -2); p.keep = 15; break;
    default: set(p.shoot, 8); set(p.speed, 5); set(p.pass, -3); set(p.tackle, -15); p.keep = 12; break;
    }
    set(p.stamina, 4);
    p.pot = (uint8_t)std::min(95, (a <= 17 ? 82 : a <= 20 ? 78 : a <= 24 ? 72 : 66) + g_rng.range(-2, 3));
    p.nation = (int16_t)g_lifeNat;
    p.contract = T.status == CS_PRO && a >= 18 ? 0 : T.status == CS_AMATEUR ? 2 : 1;
    p.years = 3; p.wageK = 0; p.morale = 70; p.cond = 100;
    p.id = g_world.nextPid++;
    return p;
}

static void lifeStartCareer(int club) {
    g_lifePick = false;
    g_career.opts = Career::Opts();
    g_career.newClubCareer(club, 2026);
    g_career.mgr.managerMode = false; g_career.mgr.noSack = true; g_career.mgr.fullManager = false;
    // les plus jeunes commencent en réserve (ou en U19), les autres dans l'équipe fanion
    int dest = club;
    if (g_lifeNew.age <= 19) {
        // U15 (-15 ans), U17 (15-16 ans), U19 (17-18 ans), sinon réserve ; à défaut la catégorie existante la plus proche
        int a = g_lifeNew.age, want = a <= 14 ? 3 : a <= 16 ? 2 : a <= 18 ? 1 : 0;
        int best = -1, bestD = 99;
        for (int i = 0; i < (int)g_world.teams.size(); i++) {
            const Team& t = g_world.teams[i];
            if (t.parent != club || t.youth > 3) continue;
            int d = std::abs(t.youth - want) * 2 + (t.youth < want ? 1 : 0);
            if (t.youth == 0 && want > 0) d = 2 * want + 1;
            if (d < bestD) { bestD = d; best = i; }
        }
        if (best >= 0) dest = best;
    }
    g_world.ensureSquad(dest);
    Team& D = g_world.teams[dest];
    Player p = lifeBuildPlayer(g_world.teams[club]);
    std::vector<bool> used(100, false);
    for (auto& q : D.squad) if (q.num < 100) used[q.num] = true;
    int want = p.pos == POS_GK ? 16 : p.pos == POS_DF ? 4 : p.pos == POS_MF ? 8 : 9;
    p.num = (uint8_t)want; while (p.num < 99 && used[p.num]) p.num++;
    D.squad.push_back(p);
    g_world.teams[club].xi.clear();
    g_career.lifeStart(true, p.id);
    g_careerActive = true; g_needAdvance = true;
    g_hubNotice = dest == club ? p.name + " signe dans l'équipe première de " + g_world.teams[club].name + " !"
                               : p.name + " commence en " + D.name + " : il faudra gagner sa place en équipe première.";
    g_career.season.news.push_back(g_hubNotice);
    levelCacheDirty();
    openHub();
    autosave();
}

// montée de la réserve / des jeunes vers l'équipe première quand le niveau le permet
static void lifeCheckPromotion() {
    if (!g_career.life.isPlayer) return;
    int t = -1; Player* me = g_career.lifePlayer(&t);
    if (!me || t == g_career.userTeam || t < 0 || g_world.teams[t].parent != g_career.userTeam) return;
    Team& U = g_world.teams[g_career.userTeam];
    std::vector<int> ov; for (auto& q : U.squad) ov.push_back(q.overall());
    std::sort(ov.rbegin(), ov.rend());
    int bar = ov.size() > 12 ? ov[12] : 0;           // doit valoir un titulaire en puissance
    if (me->overall() >= bar || me->age >= 21) {
        Player p = *me;
        Team& R = g_world.teams[t];
        for (size_t i = 0; i < R.squad.size(); i++) if (R.squad[i].id == p.id) { R.squad.erase(R.squad.begin() + i); break; }
        std::vector<bool> used(100, false); for (auto& q : U.squad) if (q.num < 100) used[q.num] = true;
        while (p.num < 99 && used[p.num]) p.num++;
        U.squad.push_back(p);
        g_career.season.news.push_back(p.name + " est appelé(e) en équipe première !");
        toast("Promotion en équipe première !");
    }
}

static void screenLifeNew() {
    drawBackground("Carrière de joueur / joueuse");
    static const char* PN[] = { "Gardien", "Défenseur", "Milieu", "Attaquant" };
    Player& P = g_lifeNew;
    std::vector<std::string> rows = {
        "Nom : " + P.name + (g_lifeTyping && ((int)(GetTime() * 2)) % 2 ? "_" : ""),
        std::string("Genre : ") + (P.gender ? "Femme" : "Homme"),
        std::string("Poste : ") + PN[P.pos & 3],
        fmt("Âge : %d ans", P.age),
        std::string("Couleur de peau : ") + SKIN_NAMES[P.skin % NUM_SKINS],
        std::string("Cheveux : ") + HAIR_NAMES[P.hair % NUM_HAIRS],
        "Nationalité : " + (g_lifeNat >= 0 ? sanitize(NATIONS[g_lifeNat].name) : std::string("-")),
        ">>> CHOISIR MON CLUB <<<", "Annuler" };
    Kit k; k.shirt = 0x1B2A63; k.shirt2 = 0xFFFFFF;
    drawPortrait(VW - 140, 50, 120, P.skin, P.hair, P.gender, k, hashStr(P.name));
    drawTextCentered(P.age <= 14 ? "Débute en U15, puis gravit les catégories (U17, U19, réserve) jusqu'à l'équipe première." : P.age <= 16 ? "Débute en U17, puis U19 et réserve avant l'équipe première." : P.age <= 19 ? "Débute en réserve ou en U19, puis monte en équipe première quand son niveau le permet." : "Débute directement dans l'équipe première.", VW / 2, 250, 10, C_DIM);
    if (P.gender) drawTextCentered("Joueuse : choisissez un club dans « Football féminin » (ou un club disposant d'une section féminine).", VW / 2, 264, 10, C_DIM);
    if (g_lifeTyping) {
        textEdit(P.name, 30);
        menuRun(g_lifeLW, rows, 50, 300);
        drawFooter("Tapez le nom   Entrée : valider");
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_ESCAPE)) { g_lifeTyping = false; g_textMode = false; }
        return;
    }
    int s = menuRun(g_lifeLW, rows, 50, 300);
    int d = IN.left ? -1 : IN.right ? 1 : 0;
    if (!d && s >= 1 && s <= 6) d = 1;
    if (d) switch (g_lifeLW.cur) {
    case 1: P.gender = !P.gender; if (P.name == "Nouveau joueur" || P.name == "Nouvelle joueuse") P.name = P.gender ? "Nouvelle joueuse" : "Nouveau joueur"; break;
    case 2: P.pos = (uint8_t)((P.pos + d + 4) % 4); break;
    case 3: P.age = (uint8_t)std::max(13, std::min(32, P.age + d)); break;
    case 4: P.skin = (uint8_t)((P.skin + d + NUM_SKINS) % NUM_SKINS); break;
    case 5: P.hair = (uint8_t)((P.hair + d + NUM_HAIRS) % NUM_HAIRS); break;
    case 6: g_lifeNat = (g_lifeNat + d + NUM_NATIONS) % NUM_NATIONS; break;
    }
    if (s == 0) { g_lifeTyping = true; g_textMode = true; while (GetCharPressed() > 0) {} }
    if (s == 7) {
        if (P.name.empty()) { toast("Donnez un nom à votre joueur"); return; }
        g_lifePick = true; g_careerActive = false; openPick(PM_CAREER);
        g_stack.back().title = "Choisissez le club de " + P.name;
        return;
    }
    drawFooter("Gauche/Droite : modifier   OK : valider   Retour");
    if (IN.back || s == 8) { g_lifePick = false; g_screen = SC_MAIN; }
}

// ---------------- vie privée
static void lifeBar(int x, int y, int w, const std::string& label, int v, Color c) {
    drawTextPx(label, x, y, 10, C_DIM);
    DrawRectangle(x + 90, y + 2, w - 90, 7, Color{ 0, 0, 0, 120 });
    DrawRectangle(x + 90, y + 2, (w - 90) * std::max(0, std::min(100, v)) / 100, 7, c);
    drawTextPx(fmt("%d", v), x + w + 6, y, 10, C_TXT);
}

static std::string euros(int64_t e) {
    bool neg = e < 0; uint64_t a = (uint64_t)(neg ? -e : e);
    std::string s = std::to_string(a), o;
    for (size_t i = 0; i < s.size(); i++) { if (i && (s.size() - i) % 3 == 0) o += ' '; o += s[i]; }
    return (neg ? "-" : "") + o + " EUR";
}

static void screenLife() {
    PlayerLife& L = g_career.life;
    drawBackground(L.isPlayer ? "Vie du joueur" : "Vie privée du manager");
    int pt = -1; Player* me = g_career.lifePlayer(&pt);
    std::string who = me ? me->name : (g_career.managerName.empty() ? std::string("Le manager") : g_career.managerName);
    Kit k = g_world.teams[pt >= 0 ? pt : g_career.userTeam].home;
    drawPortrait(12, 32, 56, me ? me->skin : g_career.managerSkin, me ? me->hair : g_career.managerHair, me ? me->gender : g_career.managerGender, k, me ? (unsigned)me->id : hashStr(who));
    drawTextPx(who, 76, 32, 20, C_HI);
    drawTextPx(me ? fmt("%s - note %d - %d ans", g_world.teams[pt].name.c_str(), me->overall(), me->age) : g_world.teams[g_career.userTeam].name, 76, 54, 10, C_TXT);
    drawTextPx("Argent personnel : " + euros(L.cash), 76, 68, 10, L.cash >= 0 ? C_GOOD : C_BAD);
    drawTextPx("Salaire : " + money(L.salaryK) + " / an", 330, 68, 10, C_TXT);
    lifeBar(330, 32, 200, "Moral", L.morale, L.morale >= 60 ? C_GOOD : L.morale >= 35 ? C_HI : C_BAD);
    if (L.relation) lifeBar(330, 46, 200, "Couple", L.love, Color{ 240, 120, 160, 255 });
    static const char* TABS[] = { "Situation", "Logement et voiture", "Vie de couple", "Paris sportifs" };
    for (int i = 0; i < 4; i++) if (button(12 + i * 156, 86, 150, 14, TABS[i], g_lifeTab == i)) g_lifeTab = i;
    if (IN.pgUp) g_lifeTab = (g_lifeTab + 3) % 4;
    if (IN.pgDn) g_lifeTab = (g_lifeTab + 1) % 4;
    int y = 108;
    std::string err;
    if (g_lifeTab == 0) {
        static const char* REL[] = { "célibataire", "en couple", "fiancé(e)", "marié(e)" };
        drawSection(12, y, VW - 24, "SITUATION"); y += 16;
        drawTextPx(std::string("Logement : ") + lifeHouseName(L.house), 24, y, 10, C_TXT); y += 12;
        drawTextPx(std::string("Voiture : ") + lifeCarName(L.car), 24, y, 10, C_TXT); y += 12;
        drawTextPx(std::string("Vie sentimentale : ") + REL[L.relation & 3] + (L.relation ? std::string(" avec ") + L.partner : std::string()), 24, y, 10, C_TXT); y += 12;
        drawTextPx(fmt("Paris : %d gagnés, %d perdus (bilan ", L.betsWon, L.betsLost) + euros(L.betBalance) + ")", 24, y, 10, C_TXT); y += 18;
        for (auto& l : wrapText("Le moral dépend du confort de vie, du couple, des résultats de l'équipe et du temps de jeu. Il influence directement les performances sur le terrain" +
                                std::string(L.isPlayer ? " (votre joueur)." : " (le manager transmet son état d'esprit au groupe)."), VW - 48)) { drawTextPx(l, 24, y, 10, C_DIM); y += 12; }
        if (L.heat > 0) { y += 6; drawTextPx(fmt("Soupçons de corruption : %d / 100", L.heat), 24, y, 10, L.heat > 50 ? C_BAD : C_HI); }
        if (L.suspendedM) { y += 12; drawTextPx(fmt("Suspendu(e) pour corruption : encore %d mois", L.suspendedM), 24, y, 10, C_BAD); }
    } else if (g_lifeTab == 1) {
        for (int w = 0; w < 2; w++) {
            drawSection(12, y, VW - 24, w == 0 ? "LOGEMENT" : "VOITURE"); y += 14;
            int cur = w == 0 ? L.house : L.car;
            for (int l = 0; l < 5; l++) {
                int64_t price = w == 0 ? lifeHousePrice(l) : lifeCarPrice(l);
                std::string lbl = std::string(w == 0 ? lifeHouseName(l) : lifeCarName(l)) + (price ? "  -  " + euros(price) : std::string());
                if (button(24, y, VW - 48, 13, lbl, l == cur, l != cur)) {
                    int ww = w, ll = l;
                    askConfirm("Changer pour : " + std::string(w == 0 ? lifeHouseName(l) : lifeCarName(l)) + " ? (l'ancien bien est revendu)", [ww, ll]() { std::string e; if (g_career.lifeBuy(ww, ll, e)) toast("Achat effectué"); else toast(e); }, "Acheter", "Annuler");
                }
                y += 15;
            }
            y += 4;
        }
    } else if (g_lifeTab == 2) {
        drawSection(12, y, VW - 24, "VIE DE COUPLE"); y += 16;
        if (L.relation == 0) {
            drawTextPx("Célibataire. Sortir en ville permet de faire des rencontres (plus facile avec un bon moral, un beau logement...).", 24, y, 10, C_TXT); y += 16;
            if (button(24, y, 220, 16, std::string("Rencontrer : ") + (L.partnerGender ? "une femme" : "un homme"))) L.partnerGender ^= 1;
            if (button(260, y, 220, 16, "Sortir ce soir (150 EUR)")) { if (g_career.lifeDate(err)) toast(std::string("En couple avec ") + L.partner + " !"); else toast(err); }
        } else {
            static const char* REL[] = { "", "en couple", "fiancé(e)s", "marié(e)s" };
            drawTextPx(std::string("Vous êtes ") + REL[L.relation & 3] + " avec " + L.partner + fmt(".  Complicité : %d / 100", L.love), 24, y, 10, C_TXT); y += 16;
            static const char* G[] = { "Fleurs (60 EUR)", "Restaurant (220 EUR)", "Bijou (2 500 EUR)", "Week-end (7 000 EUR)" };
            for (int g = 0; g < 4; g++) if (button(24 + g * 150, y, 144, 16, G[g])) { if (g_career.lifeGift(g, err)) toast("Cadeau offert"); else toast(err); }
            y += 24;
            if (L.relation == 1 && button(24, y, 220, 16, "Demander en mariage (bague 2 500 EUR)")) { if (g_career.lifePropose(err)) toast("Fiançailles !"); else toast(err); }
            if (L.relation == 2 && button(24, y, 220, 16, fmt("Organiser le mariage (%s)", euros(12000 + (int64_t)L.salaryK * 20).c_str()))) { if (g_career.lifeWedding(err)) toast("Vive les mariés !"); else toast(err); }
            if (button(VW - 180, y, 156, 16, "Rompre")) askConfirm("Mettre fin à votre relation ?" + std::string(L.relation == 3 ? " (divorce : partage des biens)" : ""), []() { std::string e; g_career.lifeBreakUp(e); toast("Séparation"); }, "Rompre", "Annuler");
            y += 24;
            drawTextPx("La complicité baisse chaque mois : pensez aux petites attentions, sinon c'est la rupture !", 24, y, 10, C_DIM);
        }
    } else {
        // paris sportifs : prochains matchs non joués (un pari à la fois)
        drawSection(12, y, VW - 24, "PARIS SPORTIFS (ARGENT PERSONNEL)"); y += 14;
        Season& S = g_career.season;
        if (L.betComp >= 0) {
            const MatchRes& m = S.comps[L.betComp].matches[L.betMatch];
            static const char* PK[] = { "victoire à domicile", "match nul", "victoire à l'extérieur" };
            drawTextPx(fmt("Pari en cours : %s - %s, %s, mise %d EUR, cote %.2f", g_world.teams[m.home].name.c_str(), g_world.teams[m.away].name.c_str(), PK[L.betPick % 3], L.betStake, L.betOdds), 24, y, 10, C_HI);
            drawTextPx("Le pari est réglé dès que le match est joué.", 24, y + 14, 10, C_DIM);
        } else {
            std::vector<std::pair<int, int>> list;
            double now = S.now;
            for (int c = 0; c < (int)S.comps.size() && list.size() < 400; c++) {
                const Competition& C = S.comps[c];
                if (C.kind != 1 && C.format != FMT_LEAGUE) continue;
                for (auto& st : C.stages) for (auto& R : st.rounds) {
                    if (R.time < now || R.time > now + 4.5) continue;
                    for (int mi : R.m) if (!C.matches[mi].played && C.matches[mi].home >= 0) list.push_back({ c, mi });
                }
            }
            std::stable_sort(list.begin(), list.end(), [&](const std::pair<int, int>& a, const std::pair<int, int>& b) {
                return teamLevel(S.comps[a.first].matches[a.second].home) < teamLevel(S.comps[b.first].matches[b.second].home); });
            if (list.size() > 60) list.resize(60);
            if (list.empty()) { drawTextPx("Aucun match à venir ce mois-ci.", 24, y, 10, C_DIM); drawFooter("LB/RB : onglet   Retour"); if (IN.back) openHub(); return; }
            if (g_betSel >= (int)list.size()) g_betSel = 0;
            static ListW lw;
            lw.cur = g_betSel;
            listRun(lw, (int)list.size(), 12, y, VW - 24, 12, 12, [&](int i, int x, int yy, bool sel) {
                const MatchRes& m = S.comps[list[i].first].matches[list[i].second];
                drawTextPx(fitText(S.comps[list[i].first].shortName, 70, 10), x + 4, yy + 1, 10, sel ? BLACK : C_DIM);
                drawTextPx(fitText(g_world.teams[m.home].name + " - " + g_world.teams[m.away].name, 330, 10), x + 80, yy + 1, 10, sel ? BLACK : C_TXT);
                drawTextPx(fmt("1 : %.2f   N : %.2f   2 : %.2f", g_career.lifeOdds(list[i].first, list[i].second, 0), g_career.lifeOdds(list[i].first, list[i].second, 1), g_career.lifeOdds(list[i].first, list[i].second, 2)), x + 420, yy + 1, 10, sel ? BLACK : C_HI);
            });
            g_betSel = lw.cur;
            int by = y + 12 * 12 + 6;
            static const char* PK[] = { "1 (domicile)", "N (nul)", "2 (extérieur)" };
            for (int p = 0; p < 3; p++) if (button(12 + p * 110, by, 104, 16, PK[p], g_betPick == p)) g_betPick = p;
            if (button(352, by, 30, 16, "-")) g_betStake = std::max(10, g_betStake / 2);
            drawTextCentered(fmt("%d EUR", g_betStake), 422, by + 3, 10, C_TXT, false);
            if (button(462, by, 30, 16, "+")) g_betStake = std::min(1000000, g_betStake * 2);
            if (button(500, by, 128, 16, "PARIER")) {
                if (g_career.lifeBet(list[g_betSel].first, list[g_betSel].second, g_betPick, g_betStake, err)) toast(fmt("Pari enregistré (cote %.2f)", L.betOdds)); else toast(err);
            }
        }
    }
    drawFooter("LB/RB : onglet   Retour");
    if (IN.back) openHub();
}

// ---------------- « valise à l'arbitre » : tenter d'acheter le prochain match
static void screenBribe() {
    drawBackground("Valise à l'arbitre");
    PlayerLife& L = g_career.life;
    if (g_pending.comp < 0) { openHub(); return; }
    const MatchRes& m = g_career.season.comps[g_pending.comp].matches[g_pending.match];
    int opp = m.home == g_career.userTeam ? m.away : m.home;
    drawTextCentered("Prochain match : " + g_world.teams[m.home].name + " - " + g_world.teams[m.away].name, VW / 2, 36, 10, C_HI);
    int y = 56;
    for (auto& l : wrapText("Une enveloppe discrète peut faire basculer un match... mais chaque tentative augmente les soupçons. Une enquête peut tomber à tout moment : amende, licenciement, rétrogradation administrative du club.", VW - 60)) { drawTextCentered(l, VW / 2, y, 10, C_DIM); y += 12; }
    y += 8;
    drawTextCentered(fmt("Soupçons actuels : %d / 100", L.heat), VW / 2, y, 10, L.heat > 50 ? C_BAD : C_TXT); y += 18;
    if (L.bribeComp >= 0) {
        drawTextCentered("Une valise est déjà en route pour ce match.", VW / 2, y, 10, C_GOOD);
    } else {
        static const char* K[3] = { "Acheter l'arbitre", "Acheter un joueur adverse", "Acheter l'équipe adverse" };
        static const char* D[3] = { "Décisions favorables, adversaire gêné. Risque moyen.", "Leur meilleur joueur passe à côté de son match. Risque faible.", "L'adversaire lève le pied. Efficace mais très risqué." };
        for (int k = 0; k < 3; k++) {
            int64_t c = g_career.bribeCost(g_pending.comp, g_pending.match, k);
            if (button(VW / 2 - 200, y, 400, 18, std::string(K[k]) + " : " + money(c) + (L.isPlayer ? " (argent personnel)" : " (caisse noire du club)"))) {
                int kk = k;
                askConfirm(std::string(K[k]) + " ? Pas de retour en arrière possible.", [kk]() { std::string e; if (g_career.bribe(g_pending.comp, g_pending.match, kk, e)) toast("La valise est partie..."); else toast(e); }, "Envoyer la valise", "Renoncer");
            }
            drawTextCentered(D[k], VW / 2, y + 20, 10, C_DIM);
            y += 40;
        }
    }
    (void)opp;
    drawFooter("Retour");
    if (IN.back) openHub();
}
