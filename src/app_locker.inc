// Scènes de vestiaire : avant le match, à la mi-temps et après le match (équipe dirigée par le joueur).
// Le discours choisi a un effet léger et annoncé (mentalité, récupération) ; après le match, il est purement narratif.
static int g_lockerPhase = 0, g_lockerTalk = -1, g_lockerSide = -1;
static float g_lockerT = 0;
static void postContinue();
static float colorBrightUI(unsigned c) { return (((c >> 16) & 255) * 0.3f + ((c >> 8) & 255) * 0.59f + (c & 255) * 0.11f) / 255.f; }

static bool lockerWanted(int& side) {
    side = -1;
    if (!g_settings.lockerRoom || !g_match) return false;
    if (getenv("FOOT_TEST") && !getenv("FOOT_LOCKER")) return false;
    const Match& m = *g_match;
    if (m.S.training || m.S.pensOnly) return false;
    side = m.coachSide(0) ? 0 : m.coachSide(1) ? 1 : -1;
    return side >= 0;
}
static void openLocker(int phase) {
    int side; if (!lockerWanted(side)) return;
    g_lockerPhase = phase; g_lockerSide = side; g_lockerTalk = -1; g_lockerT = 0;
    g_screen = SC_LOCKER;
}

static const char* LOCKER_TALKS[3][3] = {
    { "Calme et concentration", "Se battre sur chaque ballon", "Mettre le feu d'entrée" },
    { "Garder le cap", "Remobiliser les troupes", "Hausser le ton" },
    { "Féliciter le groupe", "Rassurer les joueurs", "Recadrer l'équipe" } };

static std::string lockerSpeech(const Match& m, int side) {
    const Team& U = m.team(side); const Team& O = m.team(1 - side);
    int gf = m.score[side], ga = m.score[1 - side];
    bool home = side == 0 && !m.S.neutral;
    switch (g_lockerPhase) {
    case 0: return "Les gars, en face c'est " + O.name + ". " + (m.S.title.empty() ? std::string("") : m.S.title + ". ") +
                   (home ? "On est chez nous, le public va nous pousser : je veux de l'intensité dès la première minute."
                         : "Le stade sera contre nous : restez groupés, gagnez vos duels et faites taire les tribunes.");
    case 1:
        if (gf > ga) return fmt("On mène %d-%d, c'est bien, mais rien n'est fait. ", gf, ga) + O.shortName + " va pousser : on reste solides et on tue le match en contre.";
        if (gf == ga) return fmt("%d-%d à la pause. Tout reste à faire. ", gf, ga) + "Il faut plus de mouvement devant et plus d'agressivité sur les seconds ballons.";
        return fmt("On est menés %d-%d. Ce n'est pas le niveau qu'on attend de ", gf, ga) + U.name + ". Je veux une réaction dès la reprise !";
    default:
        if (g_trophyShown && m.trophyTeam == side) return "CHAMPIONS ! Profitez-en, vous l'avez mérité. Ce trophée restera dans l'histoire de " + U.name + " !";
        if (gf > ga) return fmt("Bravo à tous ! Victoire %d-%d, ", gf, ga) + "vous avez respecté le plan jusqu'au bout. Ce soir, on savoure.";
        if (gf == ga) return fmt("Un nul %d-%d. ", gf, ga) + "Il y a du bon et du moins bon. On analysera la vidéo demain, à tête reposée.";
        return fmt("Défaite %d-%d. ", gf, ga) + "Ça fait mal, mais on relève la tête. Le prochain match arrive vite, on se remet au travail.";
    }
}
static std::string lockerReply(int phase, int talk, int gf, int ga) {
    static const char* PRE[3] = { "Concentrés, coach. On connaît le plan.", "On va tout donner sur le terrain !", "Allez les gars, on les étouffe dès le coup d'envoi !" };
    static const char* HALF[3] = { "On continue comme ça, on ne lâche rien.", "On a encore de l'énergie, coach. On y retourne !", "Message reçu. On va montrer autre chose." };
    if (phase == 0) return PRE[talk];
    if (phase == 1) return HALF[talk];
    if (gf > ga) return talk == 2 ? "D'accord coach... mais ce soir, on fête ça !" : "Merci coach ! Allez, on chante !";
    if (gf == ga) return "On fera mieux la prochaine fois.";
    return talk == 2 ? "Vous avez raison. On n'a pas été au niveau." : "Merci coach. On va se racheter.";
}
static std::string lockerEffect(Match& m, int side, int phase, int talk) {
    if (phase == 0) {
        if (talk == 0 && m.mentality[side] > 0) { m.setMentality(side, m.mentality[side] - 1, false); return std::string("Mentalité de départ : ") + mentalityName(m.mentality[side]); }
        if (talk == 2 && m.mentality[side] < 4) { m.setMentality(side, m.mentality[side] + 1, false); return std::string("Mentalité de départ : ") + mentalityName(m.mentality[side]); }
        return "Les joueurs abordent le match avec engagement.";
    }
    if (phase == 1) {
        if (talk == 1) { for (int i = side * 11; i < side * 11 + 11; i++) if (m.pl[i].onPitch) m.pl[i].stamina = std::min(m.pl[i].stam0, m.pl[i].stamina + 0.03f); return "Récupération : +3 % d'endurance pour la seconde période."; }
        if (talk == 2 && m.mentality[side] < 4) { m.setMentality(side, m.mentality[side] + 1, false); return std::string("Mentalité en seconde période : ") + mentalityName(m.mentality[side]); }
        return "Pas de changement : l'équipe garde son organisation.";
    }
    return "";
}

// décor : carrelage, casiers avec les maillots, banc, tableau tactique, sol
static void drawLockerRoom(const Match& m, int side, float t, int mood) {
    const Team& U = m.team(side);
    const Kit& kit = m.kit[side];
    Color club = hexc(kit.shirt), club2 = hexc(kit.shirt2 != kit.shirt ? kit.shirt2 : kit.shorts);
    // murs carrelés et bandeau aux couleurs du club
    DrawRectangle(0, 26, VW, 210, Color{ 196, 204, 212, 255 });
    for (int x = 0; x < VW; x += 14) DrawRectangle(x, 26, 1, 210, Color{ 170, 178, 188, 255 });
    for (int y = 26; y < 236; y += 14) DrawRectangle(0, y, VW, 1, Color{ 170, 178, 188, 255 });
    DrawRectangle(0, 30, VW, 16, club); DrawRectangle(0, 46, VW, 3, club2);
    drawTextCentered(fitText(U.name, VW - 40, 10), VW / 2, 33, 10, colorBrightUI(kit.shirt) > 0.55f ? BLACK : WHITE, false);
    // néons
    for (int k = 0; k < 3; k++) { DrawRectangle(90 + k * 220, 52, 80, 4, Color{ 250, 252, 240, 255 }); DrawRectangle(80 + k * 220, 56, 100, 30, Color{ 255, 255, 230, 18 }); }
    // tableau tactique : terrain, croix et ronds, flèche qui se dessine
    int bx = 18, by = 62, bw = 132, bh = 92;
    DrawRectangle(bx - 3, by - 3, bw + 6, bh + 6, Color{ 90, 90, 100, 255 });
    DrawRectangle(bx, by, bw, bh, Color{ 245, 248, 245, 255 });
    DrawRectangleLines(bx + 8, by + 8, bw - 16, bh - 16, Color{ 40, 140, 70, 255 });
    DrawLine(bx + bw / 2, by + 8, bx + bw / 2, by + bh - 8, Color{ 40, 140, 70, 255 });
    DrawCircleLines(bx + bw / 2, by + bh / 2, 10, Color{ 40, 140, 70, 255 });
    static const int OX[6] = { 20, 30, 30, 48, 52, 58 }, OY[6] = { 46, 24, 66, 34, 58, 46 };
    for (int k = 0; k < 6; k++) { DrawCircleLines(bx + OX[k], by + OY[k], 3, Color{ 30, 60, 200, 255 }); int xx = bx + bw - OX[k], yy = by + OY[k]; DrawLine(xx - 3, yy - 3, xx + 3, yy + 3, Color{ 210, 30, 30, 255 }); DrawLine(xx - 3, yy + 3, xx + 3, yy - 3, Color{ 210, 30, 30, 255 }); }
    float a = std::fmod(t * 0.5f, 1.f);
    for (int k = 0; k < (int)(a * 20); k++) { float u = k / 20.f; int px = bx + 52 + (int)(u * 50), py = by + 58 - (int)(std::sin(u * 3.14f) * 22); DrawRectangle(px, py, 2, 2, Color{ 30, 60, 200, 255 }); }
    // casiers avec les maillots des titulaires
    int nL = 7, lx0 = 166, lw = 64;
    for (int k = 0; k < nL; k++) {
        int x = lx0 + k * lw, y = 58;
        DrawRectangle(x, y, lw - 4, 124, Color{ 70, 52, 38, 255 });
        DrawRectangle(x + 3, y + 14, lw - 10, 106, Color{ 34, 26, 22, 255 });
        int slot = side * 11 + (k * 11) / nL;
        int sq = m.pl[slot].squad;
        std::string nm = sq >= 0 && sq < (int)U.squad.size() ? U.squad[sq].name : "";
        int num = sq >= 0 && sq < (int)U.squad.size() ? U.squad[sq].num : k + 1;
        DrawRectangle(x + 4, y + 2, lw - 12, 10, Color{ 230, 220, 190, 255 });
        drawTextCentered(fitText(nm, lw - 14, 10), x + lw / 2 - 2, y + 2, 10, Color{ 40, 30, 20, 255 }, false);
        DrawRectangle(x + lw / 2 - 12, y + 18, 20, 2, Color{ 180, 180, 190, 255 });   // cintre
        Kit k2 = kit; if (m.pl[slot].gk) { k2.shirt = m.gkShirt[side]; k2.shirt2 = m.gkShirt[side]; }
        drawKitIcon(k2, x + lw / 2 - 18, y + 22, 4);
        drawTextCentered(fmt("%d", num), x + lw / 2 - 2, y + 34, 10, colorBrightUI(k2.shirt) > 0.55f ? BLACK : WHITE, false);
        DrawRectangle(x + 8, y + 96, 18, 10, Color{ 20, 20, 20, 255 }); DrawRectangle(x + 30, y + 96, 18, 10, Color{ 20, 20, 20, 255 });   // chaussures
        DrawRectangle(x + 10, y + 104, 14, 2, hexc(0xE8B830)); DrawRectangle(x + 32, y + 104, 14, 2, hexc(0xE8B830));
    }
    // banc en bois
    DrawRectangle(150, 186, 480, 10, Color{ 150, 108, 64, 255 }); DrawRectangle(150, 186, 480, 2, Color{ 190, 146, 96, 255 });
    for (int k = 0; k < 6; k++) DrawRectangle(160 + k * 90, 196, 6, 30, Color{ 90, 64, 40, 255 });
    // sol
    DrawRectangleGradientV(0, 226, VW, VH - 226, Color{ 60, 66, 80, 255 }, Color{ 36, 40, 52, 255 });
    for (int x = 0; x < VW; x += 40) DrawRectangle(x, 226, 1, VH - 226, Color{ 50, 54, 66, 255 });
    // accessoires : glacière, bouteilles, serviettes, sac de ballons
    DrawRectangle(24, 250, 40, 26, Color{ 30, 80, 170, 255 }); DrawRectangle(24, 250, 40, 5, WHITE);
    for (int k = 0; k < 4; k++) { DrawRectangle(72 + k * 8, 258, 5, 16, Color{ 120, 200, 255, 200 }); DrawRectangle(73 + k * 8, 255, 3, 3, club); }
    DrawRectangle(590, 252, 34, 22, Color{ 20, 20, 30, 255 }); for (int k = 0; k < 3; k++) DrawCircle(596 + k * 10, 250, 5, WHITE);
    for (int k = 0; k < 3; k++) DrawRectangle(240 + k * 140, 189, 30, 5, k % 2 ? WHITE : club2);
    (void)mood;
}

static void screenLocker() {
    if (!g_match || g_lockerSide < 0) { g_screen = g_lockerPhase == 2 ? SC_POST : SC_MATCH; return; }
    Match& m = *g_match;
    int side = g_lockerSide;
    const Team& U = m.team(side);
    g_lockerT += GetFrameTime();
    float t = (float)GetTime();
    int gf = m.score[side], ga = m.score[1 - side];
    int mood = g_lockerPhase == 0 ? 0 : gf > ga ? 1 : gf < ga ? -1 : 0;   // 1 joie, -1 abattement
    bool party = g_lockerPhase == 2 && mood > 0;
    static const char* TITLES[3] = { "Vestiaire - avant le match", "Vestiaire - mi-temps", "Vestiaire - après le match" };
    drawBackground(TITLES[g_lockerPhase]);
    drawLockerRoom(m, side, t, mood);
    // joueurs : assis sur le banc, ou debout qui chantent après une victoire
    for (int k = 0; k < 7; k++) {
        int slot = side * 11 + (k * 11) / 7 + (k % 2);
        if (slot >= side * 11 + 11) slot = side * 11 + 10;
        int sq = m.pl[slot].squad;
        int skin = sq >= 0 && sq < (int)U.squad.size() ? U.squad[sq].skin : 0, hair = sq >= 0 && sq < (int)U.squad.size() ? U.squad[sq].hair : 0;
        Kit kk = m.kit[side]; if (m.pl[slot].gk) { kk.shirt = m.gkShirt[side]; kk.shirt2 = kk.shirt; }
        int x = 186 + k * 66, y = 200;
        if (party) drawPlayerSprite(x, y + 22 - (int)(std::fabs(std::sin(t * 6 + k)) * 6), kk, skin, hair, 0, (int)(t * 4) + k, PS_CELEB, false, 0, 3);
        else {
            int pose = POSE_SIT;
            if (mood < 0 && k % 2 == 0) pose = POSE_HEAD;
            else if (g_lockerTalk >= 0 && g_lockerPhase < 2 && (k % 3 == 0)) pose = POSE_CLAP;
            if (pose == POSE_CLAP) drawPosedSprite(x, y + 22, kk, skin, hair, 0, (int)(t * 6), PS_NORMAL, pose, 3);
            else drawPosedSprite(x, y + 6, kk, skin, hair, 0, 0, PS_NORMAL, pose, 3);
        }
    }
    // trophée sur la table, champagne et notes de musique pour la fête
    if (g_lockerPhase == 2 && g_trophyShown && m.trophyTeam == side) {
        DrawRectangle(VW / 2 - 40, 268, 80, 8, Color{ 240, 240, 245, 255 }); DrawRectangle(VW / 2 - 34, 276, 6, 24, Color{ 120, 120, 130, 255 }); DrawRectangle(VW / 2 + 28, 276, 6, 24, Color{ 120, 120, 130, 255 });
        fxBigTrophy(VW / 2, 268, m.trophyStyle == 1 ? 1 : m.trophyStyle == 2 ? 2 : 0, 2, t);
    }
    if (party) {
        for (int b = 0; b < 2; b++) {
            int bxp = b ? 520 : 250, byp = 176;
            DrawRectangle(bxp, byp, 5, 14, Color{ 30, 90, 40, 255 }); DrawRectangle(bxp + 1, byp - 4, 3, 4, Color{ 220, 190, 80, 255 });
            for (int k = 0; k < 24; k++) { float a = std::fmod(t * 1.4f + k * 0.041f, 1.f); int px = bxp + 2 + (int)((k % 2 ? 1 : -1) * a * 30.f * (0.4f + (k % 5) * 0.15f)), py = byp - 6 - (int)(a * 40.f) + (int)(a * a * 50.f); DrawRectangle(px, py, 2, 2, Color{ 255, 250, 210, (unsigned char)(255 * (1 - a)) }); }
        }
        for (int k = 0; k < 6; k++) { float a = std::fmod(t * 0.5f + k / 6.f, 1.f); int nx = 200 + k * 70 + (int)(std::sin(t + k) * 8), ny = 160 - (int)(a * 90); Color nc{ 255, 225, 90, (unsigned char)(255 * (1 - a)) }; DrawRectangle(nx, ny, 2, 7, nc); DrawRectangle(nx - 3, ny + 6, 4, 3, nc); DrawRectangle(nx, ny, 5, 2, nc); }
        Color cc[4] = { hexc(m.kit[side].shirt), hexc(m.kit[side].shorts), C_HI, WHITE };
        fxConfetti(0, 50, VW, 180, t, cc, 4, 60, 9);
    }
    // l'entraîneur, debout devant le groupe
    Kit suit; suit.shirt = 0x22252E; suit.shirt2 = 0xE8E8E8; suit.shorts = 0x1A1C22; suit.socks = 0x111111;
    {
        int pose = g_lockerTalk < 0 ? (((int)(t * 0.8f)) % 2 ? POSE_POINT : POSE_NONE) : g_lockerPhase == 1 && g_lockerTalk == 2 ? POSE_POINT : POSE_NONE;
        if (party) drawPlayerSprite(96, 300 - (int)(std::fabs(std::sin(t * 5)) * 4), suit, 1, 0, 0, (int)(t * 4), PS_CELEB, false, 0, 3);
        else drawPosedSprite(96, 300, suit, 1, 0, pose == POSE_POINT ? 3 : 0, 0, PS_NORMAL, pose, 3);
    }
    // bulle : discours de l'entraîneur, puis réponse du capitaine
    int bx = 140, by = 236, bw = VW - 152, bh = 52;
    DrawRectangle(bx, by, bw, bh, Color{ 250, 250, 245, 245 }); DrawRectangleLines(bx, by, bw, bh, Color{ 40, 40, 50, 255 });
    DrawTriangle(Vector2{ (float)bx, (float)by + 30 }, Vector2{ (float)bx - 18, (float)by + 46 }, Vector2{ (float)bx, (float)by + 42 }, Color{ 250, 250, 245, 245 });
    std::string speech = lockerSpeech(m, side);
    int shown = std::min((int)speech.size(), (int)(g_lockerT * 70.f));   // texte qui s'affiche progressivement
    while (shown > 0 && shown < (int)speech.size() && ((unsigned char)speech[shown] & 0xC0) == 0x80) shown++;
    int ly = by + 5;
    for (auto& l : wrapText(speech.substr(0, shown), bw - 12)) { if (ly > by + bh - 12) break; drawTextPx(l, bx + 6, ly, 10, Color{ 30, 30, 40, 255 }); ly += 11; }
    static std::string effect;
    bool done = g_lockerTalk >= 0;
    if (done) {
        int cap = m.captain[side] >= 0 ? m.captain[side] : side * 11 + 10;
        std::string reply = m.playerName(cap) + " : « " + lockerReply(g_lockerPhase, g_lockerTalk, gf, ga) + " »";
        DrawRectangle(300, 126, 320, 30, Color{ 255, 248, 220, 240 }); DrawRectangleLines(300, 126, 320, 30, Color{ 120, 100, 60, 255 });
        int ry = 129; for (auto& l : wrapText(reply, 308)) { if (ry > 150) break; drawTextPx(l, 306, ry, 10, Color{ 50, 40, 20, 255 }); ry += 11; }
        if (!effect.empty()) drawTextCentered(effect, VW / 2 + 70, 296, 10, C_HI);
    }
    // choix du discours, puis retour au terrain
    if (!done) {
        for (int k = 0; k < 3; k++) if (button(140 + k * 164, 300, 158, 16, LOCKER_TALKS[g_lockerPhase][k]) || (g_lockerT > 0.3f && IsKeyPressed(KEY_ONE + k))) {
            g_lockerTalk = k; effect = lockerEffect(m, side, g_lockerPhase, k);
        }
        drawFooter("Choisissez le discours (1/2/3)   OK : passer");
        if (IN.ok || IN.start) { g_lockerTalk = 1; effect = lockerEffect(m, side, g_lockerPhase, 1); }
    } else {
        bool go = button(VW / 2 - 100, 318, 200, 16, g_lockerPhase == 2 ? "Quitter le vestiaire" : "Retour sur le terrain");
        drawFooter("OK : continuer");
        if (go || ((IN.ok || IN.start) && g_lockerT > 0.2f)) {
            effect.clear();
            audioSupporters(CH_TENSE, 0); audioCrowd(false, 0);
            if (g_lockerPhase == 2) postContinue(); else g_screen = SC_MATCH;
            return;
        }
    }
    if (IN.back) { effect.clear(); audioSupporters(CH_TENSE, 0); audioCrowd(false, 0); if (g_lockerPhase == 2) postContinue(); else g_screen = SC_MATCH; return; }
    // le stade gronde derrière les murs ; chants des joueurs après une victoire
    audioCrowd(true, g_lockerPhase == 2 ? 0.04f : 0.07f);
    audioSupporters(party ? CH_CELEBRATE : CH_NORMAL, party ? 0.3f : 0.12f);
}
