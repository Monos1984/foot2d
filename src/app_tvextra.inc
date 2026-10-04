// ------------------------------------------------------------------ mode TV (build 29) : générique de la chaîne, vue du stade, plan sur les supporters,
// interview de mi-temps devant le panneau des partenaires, tunnel des sélections (enfants, attente du signal de l'arbitre), note du match

// ---------------- générique, vue aérienne du stade, plan sur les supporters (avant les compositions)
static int g_tvOpenPhase = 0; static float g_tvOpenT = 0;
static void tvOpeningReset() { g_tvOpenPhase = 0; g_tvOpenT = 0; }

static void tvGenericShot(float pt, float t) {
    ClearBackground(BLACK);
    std::string ch = g_tv.channel.empty() ? std::string("FOOT TV") : g_tv.channel;
    // traits lumineux qui balaient l'écran
    for (int k = 0; k < 14; k++) {
        float sp = 260.f + k * 37.f, y = 20.f + (k * 53) % (VH - 40);
        float x = std::fmod(pt * sp + k * 97.f, (float)VW + 300) - 150;
        DrawRectangleGradientH((int)x - 120, (int)y, 120, 2 + k % 3, Color{ 190, 30, 40, 0 }, Color{ 255, 90, 90, (unsigned char)(120 + k * 8) });
    }
    float pop = std::min(1.f, std::max(0.f, (pt - 0.4f) * 2.5f)), e = 1 - (1 - pop) * (1 - pop) * (1 - pop);
    int bw = (int)((textWidth(ch, 40) + 60) * e), bh = 58;
    DrawRectangle(VW / 2 - bw / 2 + 4, VH / 2 - 40 + 4, bw, bh, Color{ 0, 0, 0, 140 });
    DrawRectangleGradientV(VW / 2 - bw / 2, VH / 2 - 40, bw, bh, Color{ 220, 40, 50, 255 }, Color{ 140, 16, 24, 255 });
    DrawRectangle(VW / 2 - bw / 2, VH / 2 - 40, bw, 2, Color{ 255, 255, 255, 160 });
    if (pop >= 1.f) {
        drawTextCentered(ch, VW / 2, VH / 2 - 30, 40, WHITE, true);
        int sx = (int)std::fmod(pt * 300.f, (float)bw + 80) - 40;
        for (int k = 0; k < 16; k++) if (sx + k > 0 && sx + k < bw) DrawRectangle(VW / 2 - bw / 2 + sx + k, VH / 2 - 38, 1, bh - 4, Color{ 255, 255, 255, (unsigned char)(40 - std::abs(k - 8) * 4) });
    }
    if (pt > 1.2f) {
        float a = std::min(1.f, (pt - 1.2f) * 3.f);
        drawTextCentered("LE FOOTBALL EN DIRECT", VW / 2, VH / 2 + 28, 10, Color{ 255, 220, 90, (unsigned char)(255 * a) }, false);
        drawTextCentered(fitText(g_setup.title, VW - 60, 10), VW / 2, VH / 2 + 44, 10, Color{ 220, 225, 240, (unsigned char)(255 * a) }, false);
    }
    for (int k = 0; k < 24; k++) {   // étincelles
        float u = std::fmod(pt * 0.8f + k * 0.13f, 1.f);
        int x = VW / 2 + (int)(std::cos(k * 2.4f) * (40 + u * 260)), y = VH / 2 - 12 + (int)(std::sin(k * 2.4f) * (20 + u * 120));
        DrawRectangle(x, y, 2, 2, Color{ 255, 230, 140, (unsigned char)(220 * (1 - u)) });
    }
}

static void tvStadiumShot(float pt, float t) {
    const Team& H = g_world.teams[g_setup.home]; const Team& A = g_world.teams[g_setup.away];
    bool night = g_setup.night;
    if (night) DrawRectangleGradientV(0, 0, VW, VH, Color{ 8, 12, 34, 255 }, Color{ 40, 26, 60, 255 });
    else DrawRectangleGradientV(0, 0, VW, VH, Color{ 110, 170, 235, 255 }, Color{ 200, 225, 245, 255 });
    if (night) for (int k = 0; k < 60; k++) DrawPixel((k * 131) % VW, (k * 71) % 120, Color{ 255, 255, 255, (unsigned char)(80 + (k * 37) % 120) });
    else for (int k = 0; k < 5; k++) { int cx = (int)std::fmod(k * 160 + t * 8.f, (float)VW + 120) - 60; DrawEllipse(cx, 40 + k * 13 % 50, 44, 12, Color{ 255, 255, 255, 170 }); }
    float z = 1.f + pt * 0.05f;                      // lent travelling avant de l'hélicoptère
    int cx = VW / 2, cy = VH / 2 + 50;
    float rx = 280 * z, ry = 120 * z;
    // toit et tribunes
    DrawEllipse(cx, cy + 10, rx + 10, ry + 10, Color{ 0, 0, 0, 90 });
    DrawEllipse(cx, cy, rx, ry, night ? Color{ 70, 74, 90, 255 } : Color{ 190, 196, 210, 255 });
    DrawEllipse(cx, cy, rx - 12 * z, ry - 6 * z, night ? Color{ 40, 44, 58, 255 } : Color{ 120, 126, 140, 255 });
    DrawEllipse(cx, cy, rx - 18 * z, ry - 10 * z, Color{ 34, 30, 40, 255 });
    // foule : points aux couleurs des deux équipes (parcage visiteur dans un coin)
    Color hc = hexc(H.home.shirt), hc2 = hexc(H.home.shirt2), ac = hexc(A.home.shirt);
    for (int k = 0; k < 2600; k++) {
        unsigned h = (unsigned)k * 2654435761u;
        float a = (h % 6283) / 1000.f, r = 0.62f + ((h >> 13) % 1000) / 1000.f * 0.33f;
        float px = cx + std::cos(a) * (rx - 18 * z) * r, py = cy + std::sin(a) * (ry - 10 * z) * r;
        bool away = a > 0.3f && a < 0.9f;
        Color c = away ? ac : ((h >> 7) % 3 == 0 ? hc2 : (h >> 9) % 5 == 0 ? Color{ 230, 220, 200, 255 } : hc);
        int flick = ((int)(t * 6) + k) % 9 == 0 ? 40 : 0;
        DrawRectangle((int)px, (int)py, 2, 1, Color{ (unsigned char)std::min(255, c.r * 3 / 4 + flick), (unsigned char)std::min(255, c.g * 3 / 4 + flick), (unsigned char)std::min(255, c.b * 3 / 4 + flick), 255 });
    }
    // pelouse
    float pw = rx * 0.9f, ph = ry * 0.9f;
    int x0 = cx - (int)(pw * 0.5f), y0 = cy - (int)(ph * 0.5f);
    DrawRectangle(x0 - 6, y0 - 4, (int)pw + 12, (int)ph + 8, Color{ 40, 110, 50, 255 });
    for (int k = 0; k < 12; k++) DrawRectangle(x0 + (int)(k * pw / 12), y0, (int)(pw / 12) + 1, (int)ph, k % 2 ? Color{ 58, 146, 64, 255 } : Color{ 66, 158, 72, 255 });
    Color L = Color{ 235, 245, 235, 200 };
    DrawRectangleLines(x0, y0, (int)pw, (int)ph, L); DrawLine(cx, y0, cx, y0 + (int)ph, L);
    DrawCircleLines(cx, cy, ph * 0.18f, L);
    DrawRectangleLines(x0, cy - (int)(ph * 0.22f), (int)(pw * 0.13f), (int)(ph * 0.44f), L);
    DrawRectangleLines(x0 + (int)pw - (int)(pw * 0.13f), cy - (int)(ph * 0.22f), (int)(pw * 0.13f), (int)(ph * 0.44f), L);
    // pylônes d'éclairage
    for (int k = 0; k < 4; k++) {
        int px = cx + (k % 2 ? 1 : -1) * (int)(rx * 0.93f), py = cy + (k / 2 ? 1 : -1) * (int)(ry * 0.8f) - 40;
        DrawRectangle(px - 1, py, 3, 46, Color{ 90, 94, 110, 255 });
        DrawRectangle(px - 7, py - 6, 15, 7, Color{ 220, 225, 235, 255 });
        if (night) DrawCircleGradient(px, py - 3, 46, Color{ 255, 250, 210, 90 }, Color{ 255, 250, 210, 0 });
    }
    if (night) DrawEllipse(cx, cy, pw * 0.6f, ph * 0.6f, Color{ 255, 250, 220, 18 });
    // bandeau du stade
    float a = std::min(1.f, pt * 2.f);
    int bx = 20 - (int)((1 - a) * 300), by = VH - 74;
    DrawRectangle(bx + 3, by + 3, 330, 40, Color{ 0, 0, 0, 120 });
    DrawRectangleGradientH(bx, by, 330, 40, Color{ 190, 30, 40, 240 }, Color{ 20, 24, 50, 240 });
    DrawRectangle(bx, by, 4, 40, C_HI);
    drawTextPx(fitText(g_setup.stadium, 310, 20), bx + 10, by + 3, 20, WHITE);
    std::string sub = (H.town.empty() ? std::string() : H.town + "  -  ") + fmt("%d spectateurs attendus", g_setup.attendance) + "  -  " + METEO_NAMES[std::max(0, std::min(5, g_setup.meteo))];
    drawTextPx(fitText(sub, 314, 10), bx + 10, by + 26, 10, Color{ 230, 230, 245, 255 });
    tvLogo(10, 8, g_tv.channel, t); tvLive(VW - 10, 12, t);
}

static void tvFansShot(float pt, float t) {
    const Team& H = g_world.teams[g_setup.home];
    Color c1 = hexc(H.home.shirt), c2 = hexc(H.home.shirt2 != H.home.shirt ? H.home.shirt2 : 0xF2F2F2);
    fxCrowd(0, 0, VW, VH, c1, c2, t, 0.7f, (unsigned)(g_setup.home * 13 + 5), 6);
    DrawRectangle(0, 0, VW, VH, Color{ 0, 0, 0, 60 });
    // drapeaux géants qui ondulent
    for (int f = 0; f < 3; f++) {
        int fx = 40 + f * 220, fy = 30 + (f % 2) * 16;
        for (int k = 0; k < 120; k += 4) {
            int wob = (int)(std::sin(t * 4.f + k * 0.08f + f) * 5);
            DrawRectangle(fx + k, fy + wob, 4, 26, (k / 40) % 2 ? c2 : c1);
        }
        DrawRectangle(fx - 2, fy - 4, 2, 70, Color{ 120, 90, 60, 255 });
    }
    // fumigènes
    for (int k = 0; k < 18; k++) {
        float u = std::fmod(t * 0.25f + k * 0.17f, 1.f);
        int x = 80 + (k * 113) % (VW - 160) + (int)(std::sin(t + k) * 10), y = VH - 40 - (int)(u * 180);
        DrawCircle(x, y, 10 + u * 30, Color{ (unsigned char)(k % 3 ? 230 : c1.r), (unsigned char)(k % 3 ? 70 : c1.g), (unsigned char)(k % 3 ? 50 : c1.b), (unsigned char)(70 * (1 - u)) });
    }
    // tifo : bâche géante avec le nom du club
    int tw = std::min(VW - 60, textWidth(H.shortName, 40) + 80);
    DrawRectangle(VW / 2 - tw / 2, 108, tw, 52, c1); DrawRectangle(VW / 2 - tw / 2, 108, tw, 6, c2); DrawRectangle(VW / 2 - tw / 2, 154, tw, 6, c2);
    drawTextCentered(H.shortName, VW / 2, 116, 40, c2, true);
    // une supportrice au premier plan, écharpe tendue
    drawFanGirl(VW - 150, VH - 150, 128, (int)(g_setup.home % 6), (int)(g_setup.away % 5), (int)(g_setup.home % 4), c1, c2, t, 0);
    float a = std::min(1.f, pt * 2.f);
    int bx = 20 - (int)((1 - a) * 300), by = VH - 60;
    DrawRectangle(bx, by, 380, 26, Color{ 10, 14, 34, 230 }); DrawRectangle(bx, by, 4, 26, c1);
    drawTextPx(fitText("Les supporters de " + H.name + " donnent de la voix !", 366, 10), bx + 10, by + 8, 10, WHITE);
    tvLogo(10, 8, g_tv.channel, t); tvLive(VW - 10, 12, t);
}

// renvoie vrai tant que la séquence d'ouverture est à l'écran
static bool tvOpening(float dt) {
    if (g_tvOpenPhase >= 3) return false;
    static float t = 0; t += dt;
    g_tvOpenT += dt;
    static const float LEN[3] = { 3.0f, 4.8f, 4.0f };
    if (g_tvOpenPhase == 0) tvGenericShot(g_tvOpenT, t);
    else if (g_tvOpenPhase == 1) tvStadiumShot(g_tvOpenT, t);
    else tvFansShot(g_tvOpenT, t);
    if (g_tvOpenPhase >= 1) audioCrowd(true, 0.08f + (g_tvOpenPhase == 2 ? 0.06f : 0.f));
    // fondu enchaîné entre les plans
    float fin = std::min(1.f, g_tvOpenT * 4.f), fout = std::min(1.f, (LEN[g_tvOpenPhase] - g_tvOpenT) * 4.f);
    float f = std::min(fin, fout);
    if (f < 1.f) DrawRectangle(0, 0, VW, VH, Color{ 0, 0, 0, (unsigned char)(255 * (1 - f)) });
    drawFooter("OK : plan suivant   Entrée : passer la présentation");
    if (IN.start || IN.back) { g_tvOpenPhase = 3; IN.start = IN.back = false; audioCrowd(false, 0); return false; }
    if (g_tvOpenT > LEN[g_tvOpenPhase] || IN.ok || IN.click) { g_tvOpenPhase++; g_tvOpenT = 0; IN.ok = IN.click = false; if (g_tvOpenPhase >= 3) audioCrowd(false, 0); }
    return true;
}

// ---------------- buste en gros plan (interviews) : maillot du club, sueur, micro tendu
static void tvBust(int x, int y, int u, const Player& P, const Kit& k, float t, bool sweat) {
    auto R = [&](int a, int b, int w, int h, Color c) { DrawRectangle(x + a * u, y + b * u, w * u, h * u, c); };
    Color sk = hexc(skinColorOf(P.skin)), skd = Color{ (unsigned char)(sk.r * 0.84f), (unsigned char)(sk.g * 0.84f), (unsigned char)(sk.b * 0.84f), 255 };
    Color hc = hexc(hairColorOf(P.hair)), sh = hexc(k.shirt), sh2 = hexc(k.shirt2 != k.shirt ? k.shirt2 : 0xF2F2F2);
    bool fem = P.gender == 1;
    if (fem) R(7, 4, 18, 22, hc);
    R(2, 26, 28, 10, sh); R(2, 26, 28, 1, sh2); R(12, 26, 8, 3, sh2);   // épaules, col
    R(13, 21, 6, 6, skd);
    R(9, 6, 14, 16, sk); R(8, 11, 1, 4, skd); R(23, 11, 1, 4, skd);
    if (fem) { R(9, 4, 14, 4, hc); R(9, 7, 2, 10, hc); R(21, 7, 2, 10, hc); } else { R(9, 4, 14, 4, hc); R(9, 7, 1, 3, hc); R(22, 7, 1, 3, hc); }
    bool blink = std::fmod(t + P.num * 0.3f, 3.3f) < 0.12f;
    if (blink) { R(12, 13, 3, 1, Color{ 40, 30, 30, 255 }); R(18, 13, 3, 1, Color{ 40, 30, 30, 255 }); }
    else { R(12, 12, 3, 2, WHITE); R(13, 12, 2, 2, Color{ 40, 30, 30, 255 }); R(18, 12, 3, 2, WHITE); R(18, 12, 2, 2, Color{ 40, 30, 30, 255 }); }
    R(12, 10, 3, 1, Color{ (unsigned char)(hc.r * 0.8f), (unsigned char)(hc.g * 0.8f), (unsigned char)(hc.b * 0.8f), 255 }); R(18, 10, 3, 1, Color{ (unsigned char)(hc.r * 0.8f), (unsigned char)(hc.g * 0.8f), (unsigned char)(hc.b * 0.8f), 255 });
    R(15, 14, 2, 3, skd);
    bool talk = ((int)(t * 7)) % 2;
    R(13, 18, 6, talk ? 2 : 1, Color{ 120, 50, 50, 255 });
    if (sweat) { int ph = (int)(t * 3) % 6; R(22, 9 + ph, 1, 2, Color{ 190, 220, 255, 220 }); R(10, 12 + (ph + 3) % 6, 1, 2, Color{ 190, 220, 255, 200 }); }
    std::string num = fmt("%d", P.num);
    drawTextPx(num, x + 16 * u - textWidth(num, 10) / 2, y + 29 * u, 10, sh2);
}

// ---------------- interview de mi-temps (avant la page de publicité)
static float g_itwT = 0; static int g_itwIdx = 0;
static int itwPlayer(const Match& m, int t, bool& scored) {
    scored = false;
    const Team& T = m.team(t);
    for (auto it = m.events.rbegin(); it != m.events.rend(); ++it) if (it->type == 0 && it->team == t) {
        for (int i = t * 11; i < t * 11 + 11; i++) if (m.pl[i].onPitch && m.pl[i].squad >= 0 && m.pl[i].squad < (int)T.squad.size() && T.squad[m.pl[i].squad].name == it->player) { scored = true; return i; }
    }
    int best = -1; float br = -1;
    for (int i = t * 11; i < t * 11 + 11; i++) {
        if (!m.pl[i].onPitch || m.pl[i].squad < 0 || m.pl[i].squad >= (int)T.squad.size()) continue;
        float r = m.playerRating(t, m.pl[i].squad) + (i == m.captain[t] ? 0.3f : 0.f);
        if (r > br) { br = r; best = i; }
    }
    return best;
}
static void openHtInterview() { g_itwT = 0; g_itwIdx = 0; g_screen = SC_HTITW; }
static void screenHtInterview(float dt) {
    if (!g_match) { g_screen = SC_MAIN; return; }
    Match& m = *g_match;
    g_itwT += dt;
    float t = (float)GetTime();
    int tm = g_itwIdx;                                   // 0 : joueur de l'équipe qui reçoit, 1 : visiteur
    bool scored = false;
    int pi = itwPlayer(m, tm, scored);
    // panneau des partenaires (« backdrop ») : logos en quinconce
    ClearBackground(Color{ 240, 242, 248, 255 });
    std::string ch = g_tv.channel.empty() ? std::string("FOOT TV") : g_tv.channel;
    const char* LG[6] = { "PIXEL COLA", nullptr, "TURBO FIBRE", nullptr, "VOLT AUTO", "HEXA MOBILE" };
    for (int r = 0; r < 9; r++) for (int c = -1; c < 7; c++) {
        int bx = c * 104 + (r % 2) * 52, by = r * 40;
        int kind = (r * 3 + c + 9) % 6;
        if (kind == 1) { DrawRectangle(bx + 8, by + 10, 88, 20, Color{ 190, 30, 40, 255 }); drawTextCentered(fitText(ch, 84, 10), bx + 52, by + 15, 10, WHITE, false); }
        else if (kind == 3) { const Team& T = m.team(((r / 2) + c + 8) % 2 ? 1 : 0); drawKitIcon(T.home, bx + 22, by + 12, 2); drawTextPx(fitText(T.shortName, 50, 10), bx + 42, by + 15, 10, Color{ 20, 24, 50, 255 }); }
        else { Color lc = kind == 0 ? Color{ 200, 20, 30, 255 } : kind == 2 ? Color{ 20, 90, 200, 255 } : kind == 4 ? Color{ 0, 150, 120, 255 } : Color{ 110, 40, 160, 255 }; drawTextCentered(LG[kind], bx + 52, by + 15, 10, lc, false); }
    }
    DrawRectangleGradientV(0, VH - 120, VW, 120, Color{ 0, 0, 0, 0 }, Color{ 0, 0, 0, 90 });
    if (pi >= 0) {
        const MPlayer& p = m.pl[pi];
        const Team& T = m.team(tm);
        const Player& P = T.squad[p.squad];
        int bxp = VW / 2 + 40;
        tvBust(bxp, VH - 36 * 5 - 22, 5, P, m.kit[tm], t, true);
        // micro de la chaîne tendu par la journaliste
        int mx = bxp - 6, my = VH - 112 + (int)(std::sin(t * 1.3f) * 2);
        DrawRectangle(mx - 40, my + 18, 60, 10, Color{ 230, 190, 150, 255 });            // main / avant-bras
        DrawRectangle(mx + 14, my + 4, 8, 30, Color{ 30, 30, 36, 255 });
        DrawCircle(mx + 18, my, 9, Color{ 60, 60, 70, 255 }); DrawCircle(mx + 18, my, 7, Color{ 100, 100, 110, 255 });
        DrawRectangle(mx + 8, my + 8, 20, 12, Color{ 190, 30, 40, 255 }); drawTextPx(fitText(ch, 18, 5), mx + 9, my + 11, 5, WHITE);
        // bandeau d'identification
        DrawRectangle(20, VH - 118, 300, 36, Color{ 10, 14, 34, 235 }); DrawRectangle(20, VH - 118, 4, 36, hexc(m.kit[tm].shirt));
        drawTextPx(fitText(P.name, 284, 20), 30, VH - 116, 20, WHITE);
        drawTextPx(fitText(T.name + "  -  interview de mi-temps", 284, 10), 30, VH - 94, 10, C_HI);
        // propos du joueur (sous-titres qui s'écrivent)
        int d = m.score[tm] - m.score[1 - tm];
        static const char* LEAD[] = { "On a fait une bonne première période, il faut continuer comme ça.", "On est devant, mais rien n'est fait. Il faut rester concentrés.", "Le coach nous a demandé de presser haut, ça paye pour l'instant." };
        static const char* LOSE[] = { "On n'est pas dedans, il va falloir se réveiller en deuxième mi-temps.", "On a encaissé sur une erreur, mais on peut encore revenir.", "Il faut garder le ballon et être plus tranchants devant." };
        static const char* DRAW[] = { "C'est un match fermé, on attend l'ouverture.", "Les deux équipes se neutralisent, ça va se jouer sur un détail.", "On doit être plus efficaces dans les trente derniers mètres." };
        int q = (int)(m.S.home * 7 + m.S.away * 3 + tm) % 3;
        std::string line = scored ? "Je suis content pour le but, mais c'est le travail de toute l'équipe. " : "";
        line += d > 0 ? LEAD[q] : d < 0 ? LOSE[q] : DRAW[q];
        size_t shown = std::min(line.size(), (size_t)(std::max(0.f, g_itwT - 0.6f) * 38.f));
        while (shown < line.size() && ((unsigned char)line[shown] & 0xC0) == 0x80) shown++;
        auto lines = wrapText(line.substr(0, shown), VW - 120);
        int sy = VH - 18 - (int)lines.size() * 12;
        for (auto& l : lines) { int w = textWidth(l, 10); DrawRectangle(VW / 2 - w / 2 - 4, sy - 1, w + 8, 12, Color{ 0, 0, 0, 170 }); drawTextCentered(l, VW / 2, sy, 10, WHITE, false); sy += 12; }
    }
    tvLogo(10, 8, ch, t); tvLive(VW - 10, 12, t, "MI-TEMPS");
    drawFooter("OK : interview suivante   Entrée : passer");
    bool next = g_itwT > 7.f || IN.ok || IN.click;
    if (IN.start || IN.back) { openTvAds(1, SC_HALFTIME); return; }
    if (next) { IN.ok = IN.click = false; g_itwT = 0; if (++g_itwIdx >= 2) openTvAds(1, SC_HALFTIME); }
}

// ---------------- tunnel des sélections : joueurs côte à côte avec les enfants, ils attendent le signal de l'arbitre
static float g_tunT = 0;
static void openTunnel() { g_tunT = 0; g_screen = SC_TUNNEL; }
static void screenTunnel(float dt) {
    g_tunT += dt;
    float t = (float)GetTime(), T = g_tunT;
    const float GO = 4.6f, WALK = 5.4f, END = 8.8f;
    const Team& H = g_world.teams[g_setup.home]; const Team& A = g_world.teams[g_setup.away];
    MatchSetup tmpS = g_setup; Kit kits[2]; matchKits(tmpS, kits);
    // couloir en perspective : murs, plafonnier, sortie lumineuse vers la pelouse (derrière la caméra, on voit la lumière sur eux)
    ClearBackground(Color{ 16, 18, 26, 255 });
    int vx = VW / 2, vy = 70;
    DrawTriangle(Vector2{ 0, 0 }, Vector2{ 0, (float)VH }, Vector2{ (float)vx - 60, (float)vy + 40 }, Color{ 40, 44, 58, 255 });
    DrawTriangle(Vector2{ (float)VW, 0 }, Vector2{ (float)vx + 60, (float)vy + 40 }, Vector2{ (float)VW, (float)VH }, Color{ 36, 40, 54, 255 });
    DrawTriangle(Vector2{ 0, (float)VH }, Vector2{ (float)VW, (float)VH }, Vector2{ (float)vx + 60, (float)vy + 40 }, Color{ 60, 62, 70, 255 });
    DrawTriangle(Vector2{ 0, (float)VH }, Vector2{ (float)vx + 60, (float)vy + 40 }, Vector2{ (float)vx - 60, (float)vy + 40 }, Color{ 60, 62, 70, 255 });
    DrawRectangle(vx - 60, vy - 10, 120, 50, Color{ 24, 26, 34, 255 });
    for (int k = 0; k < 6; k++) { float u = k / 6.f; int y = (int)(vy + (VH - vy) * u * u); DrawRectangle(vx - 20 - (int)(u * 200), y / 3, 40 + (int)(u * 400), 2, Color{ 255, 250, 220, (unsigned char)(60 + k * 20) }); }
    // panneau de la compétition au fond
    DrawRectangle(vx - 58, vy - 6, 116, 14, Color{ 190, 30, 40, 255 }); drawTextCentered(fitText(g_setup.title, 110, 5), vx, vy - 3, 5, WHITE, false);
    // marche : tout le monde avance vers la caméra
    float walk = T > WALK ? (T - WALK) : 0.f;
    Kit kid; kid.shirt = 0xF4F4F4; kid.shirt2 = 0xD0D0D0; kid.shorts = 0xF4F4F4; kid.socks = 0xF4F4F4;
    Kit rk; rk.shirt = 0x151515; rk.shirt2 = 0x151515; rk.shorts = 0x101010; rk.socks = 0x101010;
    for (int row = 10; row >= 0; row--) {
        float depth = row - walk * 2.2f;                                  // 0 = devant, 10 = au fond
        if (depth < -2.5f) continue;
        float u = 1.f - std::max(-2.5f, depth) / 11.f;                    // 0 au fond, ~1 devant
        int scale = u > 0.85f ? 4 : u > 0.55f ? 3 : 2;
        int y = vy + 52 + (int)((VH - vy - 112) * std::pow(std::max(0.f, u), 1.5f));
        for (int side = 0; side < 2; side++) {
            int team = side ? g_setup.away : g_setup.home;
            g_world.ensureSquad(team);
            const Team& TT = g_world.teams[team];
            auto lu = g_world.pickLineup(team, TT.formation);
            int si = row < (int)lu.size() ? lu[row] : -1;
            int skin = si >= 0 ? TT.squad[si].skin : 0, hair = si >= 0 ? TT.squad[si].hair : 0;
            int x = vx + (side ? 1 : -1) * (int)(18 + u * 120);
            int fr = walk > 0 ? (int)(walk * 7) + row : 0;
            int bob = walk > 0 ? 0 : (int)(std::sin(t * 2.f + row + side) * 0.8f);
            const Kit& kk = row == 0 && false ? kits[side] : kits[side];
            drawPlayerSprite(x, y + bob, kk, skin, hair, 0, fr, PS_NORMAL, row == 0 && si >= 0 && TT.squad[si].pos == POS_GK, 0x30A050, scale);
            // enfant qui tient la main (côté couloir)
            int kx = x + (side ? -1 : 1) * scale * 5;
            drawPlayerSprite(kx, y + bob, kid, (row * 3 + side) % 6, (row + side * 2) % 7, 0, fr, PS_NORMAL, false, 0, std::max(1, scale - 1));
            DrawRectangle(std::min(x, kx) + scale * 2, y - scale * 6, std::abs(kx - x) - scale * 3, std::max(1, scale / 2), Color{ 230, 190, 150, 255 });   // mains jointes
        }
    }
    (void)H; (void)A;
    // arbitre et assistants en tête
    {
        float depth = -0.6f - walk * 2.2f;
        float u = 1.f - std::max(-2.5f, depth) / 11.f;
        int y = vy + 52 + (int)((VH - vy - 112) * std::pow(u, 1.5f)) + 14;
        if (depth > -2.5f && y < VH + 40) {
            for (int k = -1; k <= 1; k++) drawPlayerSprite(vx + k * 40, y, rk, 1 + k, 0, 0, walk > 0 ? (int)(walk * 7) + k : 0, T > GO && T < WALK && k == 0 ? PS_THROW : PS_NORMAL, false, 0, 5);
            if (T > GO && T < WALK + 0.6f) {   // signal de l'arbitre
                const char* sig = "Messieurs, on y va !";
                int w = textWidth(sig, 10) + 12;
                DrawRectangle(vx - w / 2, y - 120, w, 18, WHITE); DrawTriangle(Vector2{ (float)vx - 4, (float)y - 102 }, Vector2{ (float)vx, (float)y - 94 }, Vector2{ (float)vx + 4, (float)y - 102 }, WHITE);
                drawTextCentered(sig, vx, y - 116, 10, Color{ 20, 20, 30, 255 }, false);
            }
        }
    }
    // la lumière du stade envahit l'écran à la sortie
    if (T > WALK) { float a = std::min(1.f, (T - WALK) / (END - WALK)); DrawRectangle(0, 0, VW, VH, Color{ 255, 255, 240, (unsigned char)(255 * a * a) }); }
    audioCrowd(true, 0.05f + std::min(0.12f, T * 0.015f));          // la rumeur du stade monte
    tvLogo(10, 8, g_tv.channel, t); tvLive(VW - 10, 12, t);
    DrawRectangle(20, VH - 46, 300, 20, Color{ 10, 14, 34, 220 }); DrawRectangle(20, VH - 46, 4, 20, C_HI);
    drawTextPx(T < GO ? "Dans le tunnel, les joueurs attendent le signal..." : "Les deux équipes entrent sur la pelouse !", 28, VH - 41, 10, WHITE);
    drawFooter("OK : passer");
    if (T > END || IN.ok || IN.start || IN.click || IN.back) { audioCrowd(false, 0); launchMatch(); }
}

// fin des présentations TV : tunnel pour les matchs de sélections, sinon coup d'envoi
static void tvLaunch() {
    if (g_setup.tv && g_setup.anthems && !g_setup.training) openTunnel();
    else launchMatch();
}

static void drawStar5(float cx, float cy, float r, Color c) {
    Vector2 p[10];
    for (int k = 0; k < 10; k++) { float a = -1.5708f + k * 0.6283f, rr = k % 2 ? r * 0.45f : r; p[k] = Vector2{ cx + std::cos(a) * rr, cy + std::sin(a) * rr }; }
    for (int k = 0; k < 10; k++) { Vector2 a = p[k], b = p[(k + 1) % 10]; DrawTriangle(Vector2{ cx, cy }, b, a, c); }
}
// ---------------- note du match (fin d'un match télévisé)
static void drawMatchRating(const Match& m) {
    int goals = m.score[0] + m.score[1], diff = std::abs(m.score[0] - m.score[1]);
    int shots = m.shots[0] + m.shots[1], cards = 0, late = 0;
    for (auto& e : m.events) { if (e.type == 1 || e.type == 2) cards++; if ((e.type == 0 || e.type == 3) && e.minute >= 80.f) late++; }
    float spectacle = std::min(10.f, 3.f + goals * 1.3f + shots * 0.18f);
    float suspense = std::min(10.f, (diff == 0 ? 7.5f : diff == 1 ? 8.5f : diff == 2 ? 5.f : 3.f) + late * 1.2f + (m.shootout ? 1.5f : 0.f));
    float intens = std::min(10.f, 4.f + (m.fouls[0] + m.fouls[1]) * 0.15f + cards * 0.6f + shots * 0.08f);
    float note = std::max(1.f, std::min(10.f, spectacle * 0.45f + suspense * 0.35f + intens * 0.2f));
    note = std::round(note * 2.f) / 2.f;
    float u = std::min(1.f, (m.stateT - 8.f) * 3.f);
    int w = 330, h = 104, x = VW / 2 - w / 2, y = VH - 128 + (int)((1 - u) * 140);
    DrawRectangle(x + 4, y + 4, w, h, Color{ 0, 0, 0, 120 });
    DrawRectangleGradientV(x, y, w, h, Color{ 18, 28, 64, 240 }, Color{ 8, 12, 30, 240 });
    DrawRectangle(x, y, w, 18, Color{ 190, 30, 40, 255 });
    drawTextPx(m.S.channel.empty() ? std::string("TV") : m.S.channel, x + 6, y + 4, 10, WHITE);
    drawTextCentered("NOTE DU MATCH", x + w / 2 + 30, y + 4, 10, C_HI, false);
    std::string ns = fmt("%.1f", note); for (auto& c : ns) if (c == '.') c = ',';
    drawTextPx(ns, x + 16, y + 30, 40, C_HI);
    drawTextPx("/10", x + 16 + textWidth(ns, 40) + 2, y + 52, 10, C_DIM);
    for (int k = 0; k < 5; k++) {      // étoiles
        float fill = std::max(0.f, std::min(1.f, note / 2.f - k));
        int sx = x + 18 + k * 16, sy = y + 82;
        drawStar5((float)sx, (float)sy, 7, Color{ 60, 64, 80, 255 });
        if (fill > 0) drawStar5((float)sx, (float)sy, 7 * (0.35f + 0.65f * fill), Color{ 255, 210, 60, 255 });
    }
    auto bar = [&](const char* lab, float v, int yy) {
        drawTextPx(lab, x + 112, yy, 10, C_TXT);
        DrawRectangle(x + 196, yy + 3, 120, 6, Color{ 0, 0, 0, 160 });
        DrawRectangle(x + 196, yy + 3, (int)(120 * v / 10.f), 6, v > 7 ? C_GOOD : v > 4.5f ? C_HI : C_BAD);
    };
    bar("Spectacle", spectacle, y + 26); bar("Suspense", suspense, y + 42); bar("Intensité", intens, y + 58);
    const char* verdict = note >= 8.5f ? "Un match fou, de ceux qu'on n'oublie pas !" : note >= 7 ? "Une très belle affiche." : note >= 5.5f ? "Un match correct." : note >= 4 ? "Un match fermé, peu d'occasions." : "Un match à vite oublier...";
    drawTextPx(fitText(verdict, w - 120, 10), x + 112, y + 78, 10, Color{ 220, 228, 255, 255 });
}
