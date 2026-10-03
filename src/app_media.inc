// ------------------------------------------------------------------ carrière : invitations dans les médias et coupures de journal
// Une ou deux fois par saison, l'entraîneur est invité sur un plateau TV ou à la radio, selon la division du club :
// émission nationale (élite), chaîne de la 2e division, télé régionale, puis radio locale pour les petites divisions.
// Trois questions, trois réponses chacune ; chaque réponse a un effet (président, supporters, vestiaire) et la chaîne verse un cachet.

static int g_mediaKind = 0;             // 0 émission nationale, 1 chaîne de 2e division, 2 télé régionale, 3 radio locale
static int g_mediaQ[3] = { 0, 1, 2 }, g_mediaStep = 0, g_mediaA = -1, g_mediaHost = 0;
static float g_mediaT = 0;
static std::string g_mediaEffect, g_mediaShow, g_mediaChannel;
static int g_mediaTot[3] = { 0, 0, 0 };   // cumul : confiance du président, supporters, moral

static int mediaRank(int* nTeams = nullptr) {
    int pp, qq, gg;
    if (g_career.tierOfTeam(g_career.userTeam, &pp, &qq, &gg) < 0) return -1;
    const Competition& LC = g_career.season.comps[g_career.pyramids[pp].pools[qq].comps[gg]];
    auto tb = LC.table(0, 0);
    if (nTeams) *nTeams = (int)tb.size();
    for (int i = 0; i < (int)tb.size(); i++) if (tb[i].team == g_career.userTeam) return i + 1;
    return -1;
}
static bool mediaInviteDue() {
    if (g_career.kind != CK_CLUB || g_career.euroOnly || g_career.life.isPlayer || g_career.sportingMode() || g_career.userTeam < 0) return false;
    if (getenv("FOOT_TEST") && !getenv("FOOT_MEDIA")) return false;
    auto& M = g_career.monthly;
    if (M.mediaYear != g_career.year) { M.mediaYear = g_career.year; M.mediaCount = 0; }
    double now = g_career.season.now;
    if (getenv("FOOT_MEDIA")) return M.mediaCount == 0;
    // première invitation à l'automne ; la seconde au printemps, seulement si le club fait parler de lui (ou s'il est dans l'élite)
    if (M.mediaCount == 0 && now >= 13) return true;
    if (M.mediaCount == 1 && now >= 29) {
        int tier = g_career.tierOfTeam(g_career.userTeam);
        int n = 0, r = mediaRank(&n);
        bool talk = tier <= 1 || (r > 0 && (r <= 3 || r >= n - 2)) || ((g_career.year * 31 + g_career.userTeam) % 2 == 0);
        if (!talk) { M.mediaCount = 2; return false; }
        return true;
    }
    return false;
}

// questions : texte, puis trois réponses (prudente, confiante, audacieuse) et leurs effets {président, supporters, moral}
struct MediaQ { const char* q; const char* a[3]; int fx[3][3]; };
static const MediaQ MEDIA_Q[] = {
    { "Quel bilan tirez-vous du début de saison de %s ?", { "Il y a du bon, mais tout reste à faire.", "Je suis fier de mes joueurs, on est dans les clous.", "On peut viser beaucoup plus haut que prévu." }, { { 1, 0, 1 }, { 1, 2, 2 }, { -1, 3, 2 } } },
    { "Les supporters de %s rêvent-ils trop grand ?", { "Ils ont le droit de rêver, nous on travaille.", "Leur soutien nous porte, on veut les rendre heureux.", "Qu'ils rêvent : on va leur offrir quelque chose cette saison !" }, { { 1, 1, 0 }, { 0, 3, 1 }, { -2, 5, 1 } } },
    { "Votre président vous a fixé un objectif. Est-il réaliste ?", { "Il est exigeant mais atteignable.", "Oui, et on va l'atteindre.", "On fera même mieux, je le garantis." }, { { 2, 0, 0 }, { 2, 1, 1 }, { -1, 2, 1 } } },
    { "Comment gérez-vous un vestiaire avec autant de caractères ?", { "Avec du dialogue, chacun a son rôle.", "Le groupe vit bien, c'est notre force.", "Celui qui ne joue pas pour l'équipe sortira du groupe." }, { { 1, 0, 2 }, { 0, 1, 3 }, { 1, 0, -2 } } },
    { "Le mercato approche : allez-vous recruter ?", { "Seulement si une vraie opportunité se présente.", "On cherche un ou deux profils précis.", "Il me faut des renforts, le président le sait." }, { { 2, 0, 1 }, { 1, 1, 0 }, { -2, 2, -1 } } },
    { "Quelle place donnez-vous aux jeunes du centre de formation ?", { "Ils doivent mériter leur place comme les autres.", "Ils sont l'avenir du club, je leur fais confiance.", "Je lancerai un gamin dès le prochain match." }, { { 1, 0, 0 }, { 1, 2, 1 }, { 0, 3, -1 } } },
    { "Votre style de jeu est parfois critiqué. Que répondez-vous ?", { "Le résultat compte avant tout.", "On progresse, le jeu viendra avec la confiance.", "Les critiques, je ne les écoute pas." }, { { 1, -1, 0 }, { 1, 1, 1 }, { -1, 1, 1 } } },
    { "Le prochain derby se prépare déjà dans les têtes ?", { "Un match après l'autre.", "Évidemment, c'est le match de l'année pour nos supporters.", "On va leur faire vivre un enfer chez nous." }, { { 1, 0, 0 }, { 0, 3, 1 }, { -1, 5, 2 } } },
    { "Petite question pour finir : votre idole quand vous étiez enfant ?", { "Un défenseur, j'ai toujours aimé l'abnégation.", "Un numéro 10, pour le beau jeu.", "Moi-même ! (rires sur le plateau)" }, { { 0, 1, 0 }, { 0, 2, 0 }, { 0, 1, 1 } } },
    { "Votre équipe est en difficulté. Votre poste est-il menacé ?", { "Je ne pense qu'au travail.", "Le président me fait confiance, on va s'en sortir.", "Si on doit juger quelqu'un, c'est les joueurs." }, { { 2, 0, 1 }, { 1, 1, 1 }, { 0, -1, -4 } } },
    { "Vous êtes en tête : la pression monte. Comment la gérez-vous ?", { "On reste humbles, rien n'est fait.", "On savoure, mais on garde les pieds sur terre.", "Personne ne nous rattrapera." }, { { 2, 0, 1 }, { 1, 2, 1 }, { -1, 4, 2 } } } };
static const int NUM_MEDIA_Q = (int)(sizeof MEDIA_Q / sizeof MEDIA_Q[0]);

static void openMediaInvite() {
    auto& M = g_career.monthly;
    M.mediaCount++;
    int tier = g_career.tierOfTeam(g_career.userTeam);
    g_mediaKind = tier <= 0 ? 0 : tier == 1 ? 1 : tier <= 3 ? 2 : 3;
    if (getenv("FOOT_MEDIA_KIND")) g_mediaKind = std::max(0, std::min(3, atoi(getenv("FOOT_MEDIA_KIND"))));
    static const char* SHOW[4][2] = { { "Le Grand Débrief", "TF SPORT" }, { "Le Club du Lundi", "FOOT 2 TV" }, { "Stade Région", "FRANCE SPORT RÉGION" }, { "Allô Foot", "RADIO STADE FM" } };
    static const char* ITW[4][2] = { { "L'Entretien du dimanche", "CANAL FOOT+" }, { "Face au coach", "FOOT 2 TV" }, { "Le Mag des clubs", "TÉLÉ PAYS" }, { "La Voix du Club", "RADIO PAYS 98.4" } };
    bool itw = (g_career.year + M.mediaCount + g_career.userTeam) % 2 == 0;
    g_mediaShow = itw ? ITW[g_mediaKind][0] : SHOW[g_mediaKind][0];
    g_mediaChannel = itw ? ITW[g_mediaKind][1] : SHOW[g_mediaKind][1];
    g_mediaHost = GetRandomValue(0, NUM_TV_HOSTS - 1);
    // questions selon le contexte : classement, objectif, crise ou tête du championnat, puis deux autres et une question légère
    int n = 0, r = mediaRank(&n);
    std::vector<int> pool = { 0, 1, 2, 3, 4, 5, 6, 7 };
    int first = r == 1 ? 10 : (r > 0 && r >= n - 2) ? 9 : 0;
    unsigned h = (unsigned)(g_career.year * 7919 + M.mediaCount * 131 + g_career.userTeam) * 2654435761u;
    for (int i = (int)pool.size() - 1; i > 0; i--) { h = h * 1103515245u + 12345u; std::swap(pool[i], pool[(h >> 16) % (i + 1)]); }
    g_mediaQ[0] = first; int k = 1;
    for (int q : pool) { if (k >= 2) break; if (q != first) g_mediaQ[k++] = q; }
    g_mediaQ[2] = (h >> 8) % 3 == 0 ? 8 : pool[2] != first ? pool[2] : pool[3];
    if (getenv("FOOT_MEDIA_Q")) g_mediaQ[0] = std::max(0, std::min(NUM_MEDIA_Q - 1, atoi(getenv("FOOT_MEDIA_Q"))));
    g_mediaStep = 0; g_mediaA = -1; g_mediaT = 0; g_mediaEffect.clear();
    g_mediaTot[0] = g_mediaTot[1] = g_mediaTot[2] = 0;
    g_screen = SC_MEDIA;
}

static void mediaApply(int a) {
    const MediaQ& Q = MEDIA_Q[g_mediaQ[g_mediaStep]];
    int conf = Q.fx[a][0], fans = Q.fx[a][1], morale = Q.fx[a][2];
    g_mediaTot[0] += conf; g_mediaTot[1] += fans; g_mediaTot[2] += morale;
    Team& U = g_world.teams[g_career.userTeam];
    for (auto& p : U.squad) p.morale = (uint8_t)std::max(10, std::min(100, p.morale + morale));
    g_career.mgr.confidence = std::max(0, std::min(100, g_career.mgr.confidence + conf));
    if (fans && g_career.supportersEnabled()) { auto& P = g_career.supportersProfile(g_career.userTeam); P.managerTrust = std::max(0, std::min(100, P.managerTrust + fans)); }
    auto sg = [](int v) { return v > 0 ? fmt("+%d", v) : fmt("%d", v); };
    g_mediaEffect = "Président " + sg(conf) + "   " + (g_career.supportersEnabled() ? "Supporters " + sg(fans) + "   " : std::string()) + "Vestiaire " + sg(morale);
}

// décor : plateau TV (busts du présentateur et de l'entraîneur) ou studio de radio (micros, console, voyant « ON AIR »)
static void drawMediaSet(float t, bool talkGuest) {
    const Team& U = g_world.teams[g_career.userTeam];
    Color c1 = hexc(U.home.shirt);
    const TvPerson& HO = TV_HOSTS[g_mediaHost];
    bool radio = g_mediaKind == 3;
    if (!radio) {
        DrawRectangleGradientV(0, 26, VW, 190, g_mediaKind == 0 ? Color{ 10, 14, 50, 255 } : g_mediaKind == 1 ? Color{ 40, 10, 40, 255 } : Color{ 10, 40, 40, 255 }, Color{ 2, 4, 16, 255 });
        for (int i = 0; i < 7; i++) { int x = 20 + i * 100; DrawRectangle(x, 30, 6, 150, Color{ 30, 40, 90, 255 }); DrawRectangle(x + 2, 30, 2, 150, Color{ 80, 120, 255, (unsigned char)(90 + 60 * std::sin(t * 2 + i)) }); }
        // écran géant : écusson du club et nom de l'émission
        DrawRectangle(VW / 2 - 130, 32, 260, 64, Color{ 14, 20, 50, 255 }); DrawRectangleLines(VW / 2 - 130, 32, 260, 64, Color{ 90, 120, 200, 255 });
        drawKitIcon(U.home, VW / 2 - 122, 42, 4);
        drawTextShadow(fitText(g_mediaShow, 190, 20), VW / 2 - 82, 44, 20, WHITE);
        drawTextPx(fitText("Invité : l'entraîneur de " + U.name, 190, 10), VW / 2 - 82, 70, 10, C_HI);
        drawTextPx(fitText(g_mediaChannel, 190, 10), VW / 2 - 82, 82, 10, C_DIM);
        // bureau, deux busts face à face
        TvPerson guest{ "", 0, g_career.managerSkin % 6, g_career.managerHair % 6, 0x22252E };
        int dy = 186;
        drawBust(VW / 2 - 186, dy - 100, 84, HO.skin, HO.hair, HO.gender, HO.outfit, tvLookHash(HO.name), !talkGuest);
        drawBust(VW / 2 + 102, dy - 100, 84, guest.skin, guest.hair, 0, guest.outfit, 99u + (unsigned)g_career.userTeam, talkGuest);
        DrawRectangle(VW / 2 - 220, dy - 22, 440, 26, Color{ 180, 200, 235, 210 }); DrawRectangle(VW / 2 - 220, dy - 22, 440, 3, Color{ 240, 245, 255, 255 });
        for (int x = 0; x < 440; x += 4) DrawRectangle(VW / 2 - 220 + x, dy, 4, 3, std::sin(t * 3 + x * 0.05f) > 0 ? c1 : Color{ 240, 200, 60, 255 });
        DrawRectangle(VW / 2 - 196, dy - 16, 120, 12, Color{ 20, 30, 70, 255 }); drawTextCentered(fitText(HO.name, 116, 10), VW / 2 - 136, dy - 15, 10, C_TXT, false);
        std::string gn = g_career.managerName.empty() ? std::string("L'entraîneur") : g_career.managerName;
        DrawRectangle(VW / 2 + 76, dy - 16, 120, 12, Color{ 255, 205, 60, 255 }); drawTextCentered(fitText(gn, 116, 10), VW / 2 + 136, dy - 15, 10, BLACK, false);
    } else {
        // studio de radio : murs en mousse, console, micros sur bras articulés, voyant ON AIR
        DrawRectangle(0, 26, VW, 190, Color{ 48, 40, 44, 255 });
        for (int y = 30; y < 160; y += 12) for (int x = (y / 12 % 2) * 12; x < VW; x += 24) DrawRectangle(x, y, 10, 10, Color{ 60, 50, 56, 255 });
        bool on = ((int)(t * 1.5f)) % 4 != 3;
        DrawRectangle(VW / 2 - 50, 36, 100, 26, on ? Color{ 220, 30, 30, 255 } : Color{ 90, 20, 20, 255 });
        drawTextCentered("ON AIR", VW / 2, 42, 20, on ? WHITE : Color{ 160, 120, 120, 255 }, false);
        drawTextCentered(fitText(g_mediaChannel + "  -  " + g_mediaShow, 300, 10), VW / 2, 68, 10, C_HI, false);
        TvPerson guest{ "", 0, g_career.managerSkin % 6, g_career.managerHair % 6, 0x5A4A3A };
        drawBust(VW / 2 - 190, 92, 72, HO.skin, HO.hair, HO.gender, 0x3A5A7A, tvLookHash(HO.name), !talkGuest);
        drawBust(VW / 2 + 118, 92, 72, guest.skin, guest.hair, 0, guest.outfit, 99u + (unsigned)g_career.userTeam, talkGuest);
        for (int k = 0; k < 2; k++) {   // micros et casques
            int mx = k ? VW / 2 + 110 : VW / 2 - 110;
            DrawLine(mx, 80, mx + (k ? 30 : -30), 60, Color{ 30, 30, 34, 255 }); DrawLine(mx, 80, mx, 140, Color{ 30, 30, 34, 255 });
            DrawRectangle(mx - 5, 128, 10, 16, Color{ 40, 40, 46, 255 }); DrawRectangle(mx - 4, 129, 8, 6, Color{ 90, 90, 100, 255 });
        }
        DrawRectangle(60, 166, VW - 120, 44, Color{ 30, 30, 34, 255 }); DrawRectangle(60, 166, VW - 120, 3, Color{ 90, 90, 100, 255 });
        for (int k = 0; k < 24; k++) {   // console : faders et vumètres
            int x = 76 + k * 21;
            DrawRectangle(x, 174, 3, 28, Color{ 10, 10, 12, 255 });
            DrawRectangle(x - 3, 180 + (int)(std::fabs(std::sin(t * 0.7f + k)) * 14), 9, 5, Color{ 200, 200, 210, 255 });
            int lv = (int)(std::fabs(std::sin(t * 6 + k * 1.3f)) * 6);
            for (int j = 0; j < lv; j++) DrawRectangle(x + 6, 200 - j * 4, 4, 3, j > 4 ? Color{ 230, 60, 50, 255 } : j > 3 ? Color{ 240, 200, 60, 255 } : Color{ 60, 210, 90, 255 });
        }
    }
}

static void screenMedia() {
    float t = (float)GetTime(); g_mediaT += GetFrameTime();
    static const char* KINDN[4] = { "Émission nationale", "Chaîne de la 2e division", "Télévision régionale", "Radio locale" };
    drawBackground(std::string(g_mediaKind == 3 ? "Invitation à la radio" : "Invitation à la télévision") + " - " + KINDN[g_mediaKind]);
    bool answered = g_mediaA >= 0;
    drawMediaSet(t, answered);
    const Team& U = g_world.teams[g_career.userTeam];
    const MediaQ& Q = MEDIA_Q[g_mediaQ[std::min(2, g_mediaStep)]];
    const TvPerson& HO = TV_HOSTS[g_mediaHost];
    if (g_mediaStep >= 3) {   // fin de l'émission : bilan et cachet
        int fee = g_mediaKind == 0 ? 25 : g_mediaKind == 1 ? 10 : g_mediaKind == 2 ? 4 : 1;
        DrawRectangle(20, 222, VW - 40, 100, Color{ 6, 12, 28, 235 }); DrawRectangle(20, 222, 3, 100, C_HI);
        drawTextPx(fitText(HO.name + std::string(" : « Merci d'être venu, et bonne fin de saison à ") + U.shortName + " ! »", VW - 60, 10), 30, 230, 10, C_TXT);
        auto sg = [](int v) { return v > 0 ? fmt("+%d", v) : fmt("%d", v); };
        drawTextPx("BILAN DE L'ÉMISSION", 30, 248, 10, C_HI);
        drawTextPx("Président " + sg(g_mediaTot[0]) + "   " + (g_career.supportersEnabled() ? "Supporters " + sg(g_mediaTot[1]) + "   " : std::string()) + "Vestiaire " + sg(g_mediaTot[2]), 30, 262, 10, C_GOOD);
        drawTextPx("Cachet versé au club : " + money(fee), 30, 276, 10, C_TXT);
        bool go = button(VW / 2 - 90, 300, 180, 16, "Retour au bureau");
        drawFooter("OK : continuer");
        if (go || ((IN.ok || IN.start || IN.back) && g_mediaT > 0.3f)) {
            g_career.mgr.budget += fee;
            g_career.season.news.push_back(fmt("Médias : l'entraîneur de %s invité de « %s » (%s).", U.name.c_str(), g_mediaShow.c_str(), g_mediaChannel.c_str()));
            g_screen = SC_HUB;
        }
        return;
    }
    // bandeau de l'émission et question
    DrawRectangle(20, 222, VW - 40, 30, Color{ 250, 250, 245, 245 }); DrawRectangle(20, 222, 4, 30, Color{ 200, 30, 40, 255 });
    drawTextPx(fmt("%s - question %d / 3", HO.name, g_mediaStep + 1), 30, 225, 10, Color{ 140, 30, 30, 255 });
    std::string q = fmt(Q.q, U.name.c_str());
    int ly = 237; for (auto& l : wrapText(q, VW - 70)) { if (ly > 247) break; drawTextPx(l, 30, ly - 1, 10, Color{ 30, 30, 40, 255 }); ly += 11; }
    static const char* TONE[3] = { "Prudent", "Confiant", "Audacieux" };
    if (!answered) {
        for (int k = 0; k < 3; k++) {
            int y = 258 + k * 22;
            bool sel = button(20, y, VW - 40, 19, "", false) || (IsKeyPressed(KEY_ONE + k) && g_mediaT > 0.3f);
            DrawRectangle(24, y + 3, 76, 13, k == 0 ? Color{ 60, 120, 200, 255 } : k == 1 ? Color{ 60, 160, 80, 255 } : Color{ 200, 70, 50, 255 });
            drawTextCentered(TONE[k], 62, y + 4, 10, WHITE, false);
            drawTextPx(fitText(std::string("« ") + Q.a[k] + " »", VW - 150, 10), 108, y + 4, 10, C_TXT);
            if (sel) { g_mediaA = k; g_mediaT = 0; mediaApply(k); }
        }
        drawFooter("Choisissez votre réponse (1/2/3)");
        return;
    }
    DrawRectangle(20, 258, VW - 40, 44, Color{ 6, 12, 28, 230 });
    drawTextPx(fitText(std::string("Vous : « ") + Q.a[g_mediaA] + " »", VW - 60, 10), 30, 264, 10, C_HI);
    drawTextPx(g_mediaEffect, 30, 280, 10, C_GOOD);
    bool go = button(VW / 2 - 90, 310, 180, 16, g_mediaStep < 2 ? "Question suivante" : "Fin de l'émission");
    drawFooter("OK : continuer");
    if (go || ((IN.ok || IN.start) && g_mediaT > 0.3f)) { g_mediaStep++; g_mediaA = -1; g_mediaT = 0; }
}

// ---- coupures de journal : vainqueurs des coupes nationales et des coupes d'Europe
static int g_cupNewsComp = -1;
static bool cupNewsEligible(const Competition& C) {
    if (!C.done || C.winner < 0 || C.winner >= (int)g_world.teams.size()) return false;
    if (C.regionalDraw || C.qualSpots > 0 || C.name.find(" - ") != std::string::npos) return false;   // tours régionaux, tours qualificatifs
    if (C.kind == 3 || C.kind == 8 || C.kind == 9) return true;                         // C1, C3, C4
    if (C.kind == 11) return true;                                                      // Coupe de la Ligue
    if (C.kind != 2) return false;                                                      // coupes nationales
    int un = g_world.teams[g_career.userTeam].nation, wn = g_world.teams[C.winner].nation;
    if (wn == un) return true;
    if (wn < 0 || wn >= NUM_NATIONS) return false;
    static const char* BIG[5] = { "ENG", "ESP", "ITA", "GER", "POR" };
    for (auto b : BIG) if (strcmp(NATIONS[wn].code, b) == 0) return true;
    return false;
}
static int cupNewsPending() {
    if (g_career.kind != CK_CLUB || g_career.sportingMode() || g_career.userTeam < 0) return -1;
    if (getenv("FOOT_TEST") && !getenv("FOOT_CUPNEWS")) return -1;
    auto& N = g_career.monthly.cupNews;
    N.erase(std::remove_if(N.begin(), N.end(), [](const Career::KeyVal& kv) { return kv.v != g_career.year; }), N.end());
    const Season& S = g_career.season;
    for (int c = 0; c < (int)S.comps.size(); c++) {
        if (!cupNewsEligible(S.comps[c])) continue;
        bool seen = false; for (auto& kv : N) if (kv.k == c) seen = true;
        if (!seen) return c;
    }
    return -1;
}
// finale : dernier match joué de la compétition
static const MatchRes* cupFinal(const Competition& C) {
    const MatchRes* f = nullptr; double bt = -1;
    for (auto& st : C.stages) for (auto& R : st.rounds) for (int x : R.m) if (x >= 0 && x < (int)C.matches.size() && C.matches[x].played && R.time >= bt) { bt = R.time; f = &C.matches[x]; }
    return f;
}
static void openCupNews(int c) {
    Career::KeyVal kv; kv.k = c; kv.v = g_career.year;
    g_career.monthly.cupNews.push_back(kv);
    g_cupNewsComp = c;
    const Competition& C = g_career.season.comps[c];
    g_career.season.news.push_back(fmt("Coupure de presse : %s remporte %s.", g_world.teams[C.winner].name.c_str(), C.name.c_str()));
    g_screen = SC_CUPNEWS;
}
static void screenCupNews() {
    const Season& S = g_career.season;
    if (g_cupNewsComp < 0 || g_cupNewsComp >= (int)S.comps.size()) { g_screen = SC_HUB; return; }
    const Competition& C = S.comps[g_cupNewsComp];
    const Team& W = g_world.teams[C.winner];
    const MatchRes* F = cupFinal(C);
    float t = (float)GetTime();
    static float openT = 0; static int lastComp = -1;
    if (lastComp != g_cupNewsComp) { lastComp = g_cupNewsComp; openT = 0; }
    openT += GetFrameTime();
    // panneau de liège
    ClearBackground(Color{ 150, 105, 60, 255 });
    { Rng r(11); for (int k = 0; k < 2400; k++) DrawPixel((int)(r.f() * VW), (int)(r.f() * VH), r.f() < 0.5f ? Color{ 120, 80, 44, 255 } : Color{ 175, 130, 80, 255 }); }
    DrawRectangleLines(2, 2, VW - 4, VH - 4, Color{ 90, 60, 30, 255 }); DrawRectangleLines(4, 4, VW - 8, VH - 8, Color{ 90, 60, 30, 255 });
    // la coupure : papier jauni aux bords déchirés, qui « tombe » sur le panneau
    const Color PAPER = { 236, 228, 204, 255 }, INK = { 25, 22, 20, 255 }, INK2 = { 80, 70, 60, 255 };
    float drop = std::min(1.f, openT * 3.f);
    int px = 70, pw = VW - 140, py = 18 - (int)((1 - drop) * 40), ph = VH - 50;
    DrawRectangle(px + 5, py + 5, pw, ph, Color{ 0, 0, 0, 90 });
    DrawRectangle(px, py, pw, ph, PAPER);
    { Rng r((uint64_t)g_cupNewsComp * 7 + 3);
      for (int x = px; x < px + pw; x += 6) { int a = (int)(r.f() * 4), b = (int)(r.f() * 4); DrawTriangle(Vector2{ (float)x, (float)py }, Vector2{ (float)x + 3, (float)py + a + 1 }, Vector2{ (float)x + 6, (float)py }, Color{ 150, 105, 60, 255 });
                                               DrawTriangle(Vector2{ (float)x, (float)py + ph }, Vector2{ (float)x + 6, (float)py + ph }, Vector2{ (float)x + 3, (float)py + ph - b - 1 }, Color{ 150, 105, 60, 255 }); }
      for (int k = 0; k < 600; k++) DrawPixel(px + (int)(r.f() * pw), py + (int)(r.f() * ph), Color{ 190, 175, 140, 120 }); }
    // punaise
    DrawCircle(VW / 2 + 1, py + 9, 6, Color{ 0, 0, 0, 80 }); DrawCircle(VW / 2, py + 8, 6, Color{ 210, 30, 40, 255 }); DrawCircle(VW / 2 - 2, py + 6, 2, Color{ 255, 140, 140, 255 });
    // manchette
    bool euro = C.kind == 3 || C.kind == 8 || C.kind == 9;
    std::string paper = euro ? "L'ÉCHO DU FOOT EUROPÉEN" : "LE JOURNAL DU FOOT";
    drawTextPx(paper, px + 12, py + 18, 10, INK2);
    std::string d = dateOf(S.now, g_career.year);
    drawTextPx(d, px + pw - 12 - textWidth(d, 10), py + 18, 10, INK2);
    DrawRectangle(px + 10, py + 30, pw - 20, 2, INK);
    // gros titre
    std::string cup = C.name;
    std::string head = W.shortName.empty() ? W.name : W.name;
    std::vector<std::string> lines = wrapText(head + " remporte " + (cup.find("Coupe") == 0 || cup.find("Ligue") == 0 || cup.find("Supercoupe") == 0 ? "la " : "") + cup + " !", (pw - 24) / 2);
    int y = py + 36;
    for (size_t i = 0; i < lines.size() && i < 2; i++) { drawTextPx(lines[i], px + 12, y, 20, INK); drawTextPx(lines[i], px + 13, y, 20, INK); y += 21; }
    // photo : les vainqueurs soulèvent le trophée
    int fx = px + 12, fy = y + 4, fw = pw / 2 + 20, fh = 150;
    DrawRectangleGradientV(fx, fy, fw, fh, Color{ 120, 110, 100, 255 }, Color{ 70, 64, 58, 255 });
    fxCrowd(fx, fy, fw, 60, hexc(W.home.shirt), WHITE, t, 0.8f, 5, 3);
    for (int k = 0; k < 7; k++) drawPlayerSprite(fx + 20 + k * (fw - 40) / 6, fy + fh - 6, W.home, (k * 3) % 6, k % 7, 1, (int)(t * 4) + k, PS_CELEB, false, 0, 2);
    fxBigTrophy(fx + fw / 2, fy + 60, euro ? 2 : 0, 3, t);
    for (int k = 0; k < fw * fh / 14; k++) { Rng r2((uint64_t)k * 13); DrawPixel(fx + (int)(r2.f() * fw), fy + (int)(r2.f() * fh), Color{ 0, 0, 0, 40 }); }
    DrawRectangle(fx, fy, fw, fh, Color{ 120, 100, 60, 60 });
    DrawRectangleLines(fx, fy, fw, fh, INK);
    {   // les drapeaux de la foule dépassent au-dessus de la photo : on redessine la bande du titre par-dessus
        DrawRectangle(px + 2, py + 34, pw - 4, fy - py - 34, PAPER);
        int yy = py + 36;
        for (size_t i = 0; i < lines.size() && i < 2; i++) { drawTextPx(lines[i], px + 12, yy, 20, INK); drawTextPx(lines[i], px + 13, yy, 20, INK); yy += 21; }
    }
    drawTextPx(fitText("La joie des joueurs de " + W.name + ".", fw, 10), fx, fy + fh + 2, 10, INK2);
    // texte
    int tx = fx + fw + 10, tw = px + pw - 12 - tx, ty = fy;
    std::string txt;
    if (F) {
        const Team& A = g_world.teams[F->home]; const Team& B = g_world.teams[F->away];
        txt = fmt("En finale, %s et %s se sont affrontés : %d - %d", A.name.c_str(), B.name.c_str(), F->hg, F->ag);
        if (F->ph >= 0) txt += fmt(" (%d - %d aux tirs au but)", F->ph, F->pa);
        else if (F->aet) txt += " après prolongation";
        txt += ".";
    }
    bool mine = C.winner == g_career.userTeam;
    bool lost = F && !mine && (F->home == g_career.userTeam || F->away == g_career.userTeam);
    txt += mine ? " Une soirée historique pour le club et ses supporters, qui ont fêté le trophée jusqu'au bout de la nuit."
         : lost ? " Une immense déception pour " + g_world.teams[g_career.userTeam].name + ", si près du but."
         : " Le trophée rejoint la vitrine du club, sous les applaudissements du public.";
    for (auto& l : wrapText(txt, tw)) { if (ty > py + ph - 40) break; drawTextPx(l, tx, ty, 10, INK); ty += 11; }
    // encadré
    int by = py + ph - 34;
    DrawRectangle(px + 10, by, pw - 20, 1, INK);
    drawTextPx(fitText(mine ? "VOTRE CLUB EST VAINQUEUR !" : euro ? "Coupes d'Europe : le palmarès s'enrichit." : "Coupe nationale : un nouveau nom au palmarès.", pw - 24, 10), px + 12, by + 6, 10, mine ? Color{ 150, 20, 20, 255 } : INK2);
    if (mine && ((int)(t * 3)) % 2) DrawRectangleLines(px + 8, by + 3, pw - 16, 16, Color{ 150, 20, 20, 255 });
    drawFooter("OK : continuer");
    if (openT > 0.4f && (IN.ok || IN.back || IN.click || IN.start)) g_screen = SC_HUB;
}
