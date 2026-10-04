// ------------------------------------------------------------------ match télévisé : pages de publicité avant le match et à la mi-temps
// Marques imaginaires, spots animés en pixel art (environ 4 s chacun), passables avec OK.
static int g_adsSpots[3] = { 0, 1, 2 }, g_adsIdx = 0, g_adsCount = 3;
static float g_adsT = 0;
static Screen g_adsNext = SC_MATCH;
static const int NUM_AD_SPOTS = 18;
static const float AD_LEN = 4.2f;
static void openTvAds(int phase, Screen next) {
    g_adsNext = next; g_adsIdx = 0; g_adsT = 0; g_adsCount = phase == 0 ? 3 : 2;
    // tirage sans répétition, différent selon le match et la période
    unsigned seed = (unsigned)(g_setup.home * 131 + g_setup.away * 17 + phase * 7 + (unsigned)GetTime());
    // spots de la chaîne + pages de l'éditeur de publicités (une page perso passe en priorité)
    int nCustom = (int)customTvAds().size();
    std::vector<int> pool; for (int i = 0; i < NUM_AD_SPOTS + nCustom; i++) pool.push_back(i);
    for (int i = (int)pool.size() - 1; i > 0; i--) { seed = seed * 1103515245u + 12345u; int j = (int)((seed >> 16) % (unsigned)(i + 1)); std::swap(pool[i], pool[j]); }
    if (nCustom > 0) { seed = seed * 1103515245u + 12345u; int c = NUM_AD_SPOTS + (int)((seed >> 16) % (unsigned)nCustom); auto it = std::find(pool.begin(), pool.end(), c); std::iter_swap(pool.begin(), it); }
    for (int i = 0; i < 3; i++) g_adsSpots[i] = pool[i];
    g_screen = SC_TVADS;
    audioJingleForce(5 + g_adsSpots[0] % 3);
}

static void adSlogan(const std::string& brand, const std::string& slogan, Color bc, Color sc, float t) {
    // bandeau de marque : cartouche sombre qui s'ouvre, reflet qui balaie, nom en grand, slogan en dessous
    float pop = std::min(1.f, t * 3.f);
    int size = pop < 1.f ? 20 : 40;
    int bw = (int)((std::min(VW - 40, textWidth(brand, 40) + 60)) * (1 - (1 - pop) * (1 - pop)));
    int bx = VW / 2 - bw / 2, by = 226;
    DrawRectangle(bx + 4, by + 4, bw, 50, Color{ 0, 0, 0, 110 });
    DrawRectangleGradientV(bx, by, bw, 50, Color{ 20, 20, 30, 210 }, Color{ 0, 0, 0, 230 });
    DrawRectangle(bx, by, bw, 2, bc); DrawRectangle(bx, by + 48, bw, 2, bc);
    int sx = (int)std::fmod(t * 260.f, (float)bw + 80) - 40;
    for (int k = 0; k < 18; k++) if (sx + k > 0 && sx + k < bw) DrawRectangle(bx + sx + k, by + 2, 1, 46, Color{ 255, 255, 255, (unsigned char)(34 - std::abs(k - 9) * 3) });
    drawTextCentered(brand, VW / 2, by + (size == 40 ? 6 : 16), size, bc);
    if (t > 0.8f) {
        float a = std::min(1.f, (t - 0.8f) * 3.f);
        int sw = textWidth(slogan, 10) + 16;
        DrawRectangle(VW / 2 - sw / 2, 282, sw, 14, Color{ 0, 0, 0, (unsigned char)(150 * a) });
        drawTextCentered(slogan, VW / 2, 284, 10, Color{ sc.r, sc.g, sc.b, (unsigned char)(255 * a) });
    }
    // vignettage façon écran cathodique
    for (int k = 0; k < 6; k++) { DrawRectangle(0, k * 3, VW, 3, Color{ 0, 0, 0, (unsigned char)(60 - k * 10) }); DrawRectangle(0, VH - (k + 1) * 3, VW, 3, Color{ 0, 0, 0, (unsigned char)(60 - k * 10) }); }
}
static void drawAdSpot(int spot, float t) {
    const Team& H = g_world.teams[g_setup.home]; const Team& A = g_world.teams[g_setup.away];
    if (spot >= NUM_AD_SPOTS) {     // page de l'éditeur de publicités
        DrawRectangle(0, 0, VW, VH, BLACK);
        auto tv = customTvAds();
        int i = spot - NUM_AD_SPOTS;
        if (i < (int)tv.size()) drawFitted(adTexture(tv[i]), 0, 0, VW, VH);
        float a = std::min(1.f, t * 3.f); DrawRectangle(0, 0, VW, VH, Color{ 0, 0, 0, (unsigned char)(255 * (1 - a)) });
        return;
    }
    switch (spot) {
    case 0: {   // PIXEL COLA : canette géante, rayons, bulles
        DrawRectangle(0, 0, VW, VH, Color{ 170, 20, 30, 255 });
        for (int k = 0; k < 16; k++) { float a = k * 0.3927f + t * 0.3f; DrawTriangle(Vector2{ VW / 2.f, 130.f }, Vector2{ VW / 2.f + std::cos(a + 0.18f) * 600, 130.f + std::sin(a + 0.18f) * 600 }, Vector2{ VW / 2.f + std::cos(a) * 600, 130.f + std::sin(a) * 600 }, Color{ 220, 40, 50, 255 }); }
        int cx = VW / 2, cy = 130 - (int)(std::sin(t * 3) * 3);
        DrawRectangle(cx - 30, cy - 70, 60, 140, Color{ 230, 230, 236, 255 }); DrawRectangle(cx - 30, cy - 50, 60, 100, Color{ 200, 16, 30, 255 });
        DrawRectangle(cx - 26, cy - 76, 52, 6, Color{ 180, 180, 190, 255 }); DrawRectangle(cx - 20, cy + 70, 40, 5, Color{ 160, 160, 170, 255 });
        DrawRectangle(cx - 24, cy - 50, 6, 100, Color{ 255, 255, 255, 70 });
        drawTextCentered("PIXEL", cx, cy - 26, 10, WHITE, false); drawTextCentered("COLA", cx, cy - 12, 20, WHITE, false);
        for (int k = 0; k < 30; k++) { float a = std::fmod(t * 0.8f + k * 0.0333f, 1.f); int bx = cx - 50 + (k * 37) % 100, by = cy + 60 - (int)(a * 200); DrawCircleLines(bx, by, 2 + k % 3, Color{ 255, 255, 255, (unsigned char)(200 * (1 - a)) }); }
        adSlogan("PIXEL COLA", "Le goût pétillant du 16 bits. À consommer avec modération... de pixels.", WHITE, Color{ 255, 230, 120, 255 }, t);
        break;
    }
    case 1: {   // CRAMPONS+ : chaussure géante, lignes de vitesse, frappe
        DrawRectangleGradientV(0, 0, VW, VH, Color{ 20, 90, 40, 255 }, Color{ 10, 40, 20, 255 });
        for (int k = 0; k < 24; k++) { int y = 30 + (k * 53) % 200, x = (int)std::fmod(VW - t * 600 - k * 97, (float)VW + 200) - 100; DrawRectangle(x, y, 60 + k % 4 * 20, 2, Color{ 255, 255, 255, 60 }); }
        float kx = std::min(1.f, t / 1.2f);
        int bx = 120 + (int)(kx * 140), by = 150;
        DrawRectangle(bx - 60, by - 20, 120, 36, Color{ 245, 245, 245, 255 }); DrawRectangle(bx + 40, by - 10, 40, 26, Color{ 245, 245, 245, 255 });
        DrawRectangle(bx - 60, by + 16, 140, 6, Color{ 30, 30, 30, 255 }); for (int k = 0; k < 6; k++) DrawRectangle(bx - 50 + k * 24, by + 22, 6, 6, Color{ 200, 160, 40, 255 });
        DrawRectangle(bx - 40, by - 14, 70, 4, Color{ 230, 50, 40, 255 }); DrawRectangle(bx - 60, by - 30, 30, 12, Color{ 230, 50, 40, 255 });
        if (t > 1.2f) { float u = (t - 1.2f) * 1.5f; int ballx = bx + 90 + (int)(u * 400), bally = by - (int)(u * 120); DrawCircle(ballx, bally, 12, WHITE); DrawCircle(ballx - 3, bally - 3, 4, Color{ 30, 30, 30, 255 }); for (int k = 1; k < 5; k++) DrawCircle(ballx - k * 18, bally + k * 5, 8 - k, Color{ 255, 255, 255, (unsigned char)(120 - k * 25) }); }
        adSlogan("CRAMPONS+", "Frappez plus fort. Courez plus vite. Jouez plus pixel.", Color{ 255, 230, 60, 255 }, WHITE, t);
        break;
    }
    case 2: {   // GOAL FM : poste de radio, ondes, égaliseur
        DrawRectangleGradientV(0, 0, VW, VH, Color{ 40, 20, 90, 255 }, Color{ 10, 10, 40, 255 });
        int rx = VW / 2 - 90, ry = 70;
        DrawRectangleRounded(Rectangle{ (float)rx, (float)ry, 180, 110 }, 0.2f, 6, Color{ 230, 140, 40, 255 });
        DrawCircle(rx + 50, ry + 60, 34, Color{ 50, 40, 40, 255 }); DrawCircle(rx + 50, ry + 60, 34 - (int)(std::fabs(std::sin(t * 12)) * 4), Color{ 80, 70, 70, 255 });
        for (int k = 0; k < 8; k++) { int h = 6 + (int)(std::fabs(std::sin(t * 7 + k * 1.3f)) * 40); DrawRectangle(rx + 100 + k * 9, ry + 90 - h, 6, h, k < 5 ? Color{ 90, 230, 120, 255 } : Color{ 255, 90, 80, 255 }); }
        DrawLine(rx + 150, ry, rx + 170, ry - 40, Color{ 200, 200, 210, 255 });
        for (int k = 0; k < 4; k++) { float a = std::fmod(t + k * 0.25f, 1.f); DrawCircleLines(rx + 170, ry - 40, 10 + a * 70, Color{ 255, 255, 255, (unsigned char)(150 * (1 - a)) }); }
        drawTextCentered("98.5", rx + 130, ry + 10, 20, WHITE, false);
        adSlogan("GOAL FM", "Tout le foot, tout le temps. Les multiplex en direct sur le 98.5.", Color{ 255, 170, 60, 255 }, WHITE, t);
        break;
    }
    case 3: {   // SUPER BUT : magasin de sport, maillots des deux équipes en promotion
        DrawRectangle(0, 0, VW, VH, Color{ 20, 60, 140, 255 });
        DrawRectangle(60, 40, VW - 120, 150, Color{ 240, 236, 226, 255 }); DrawRectangle(60, 40, VW - 120, 22, Color{ 230, 40, 40, 255 });
        drawTextCentered("SUPER BUT - RAYON FOOTBALL", VW / 2, 46, 10, WHITE, false);
        for (int k = 0; k < 4; k++) { const Kit& kk = k % 2 ? A.home : H.home; int x = 110 + k * 110, y = 80 + (int)(std::sin(t * 2 + k) * 2); DrawRectangle(x + 14, y - 6, 20, 2, Color{ 120, 120, 130, 255 }); drawKitIcon(kk, x, y, 6); }
        float p = std::min(1.f, t * 2.f);
        DrawCircle(VW - 110, 160, 26 * p, Color{ 255, 220, 0, 255 }); if (p >= 1) { drawTextCentered("-30%", VW - 110, 150, 20, Color{ 200, 20, 20, 255 }, false); }
        adSlogan("SUPER BUT", "Les maillots de " + H.shortName + " et de " + A.shortName + " à prix cassés !", WHITE, Color{ 255, 230, 120, 255 }, t);
        break;
    }
    case 4: {   // AMIGOAL : console de salon, petit match sur l'écran
        DrawRectangle(0, 0, VW, VH, Color{ 16, 16, 24, 255 });
        int sx = VW / 2 - 110, sy = 40;
        DrawRectangle(sx - 8, sy - 8, 236, 152, Color{ 60, 60, 70, 255 }); DrawRectangle(sx, sy, 220, 136, Color{ 50, 140, 60, 255 });
        DrawRectangleLines(sx + 10, sy + 10, 200, 116, WHITE); DrawLine(sx + 110, sy + 10, sx + 110, sy + 126, WHITE);
        for (int k = 0; k < 5; k++) { int px = sx + 40 + (int)(std::sin(t * 2 + k) * 30) + k * 30, py = sy + 30 + k * 18; DrawRectangle(px, py, 4, 6, k % 2 ? Color{ 230, 40, 40, 255 } : Color{ 40, 80, 230, 255 }); }
        DrawRectangle(sx + 110 + (int)(std::sin(t * 3) * 80), sy + 68 + (int)(std::cos(t * 4) * 40), 3, 3, WHITE);
        DrawRectangleRounded(Rectangle{ (float)VW / 2 - 40, 196, 80, 26 }, 0.5f, 6, Color{ 200, 200, 210, 255 });
        DrawCircle(VW / 2 + 22, 205, 4, Color{ 230, 60, 60, 255 }); DrawCircle(VW / 2 + 30, 213, 4, Color{ 60, 120, 230, 255 }); DrawRectangle(VW / 2 - 30, 207, 14, 4, Color{ 60, 60, 70, 255 }); DrawRectangle(VW / 2 - 25, 202, 4, 14, Color{ 60, 60, 70, 255 });
        adSlogan("AMIGOAL", "La console qui met le stade dans ton salon. 2 manettes incluses !", Color{ 120, 220, 255, 255 }, WHITE, t);
        break;
    }
    case 5: {   // FRITES DU STADE : cornet de frites fumant
        DrawRectangleGradientV(0, 0, VW, VH, Color{ 250, 200, 40, 255 }, Color{ 230, 120, 20, 255 });
        int cx = VW / 2, cy = 170;
        for (int k = 0; k < 14; k++) DrawRectangle(cx - 46 + k * 7, cy - 90 + (k * 13) % 20, 6, 80, Color{ 255, 220, 90, 255 });
        DrawTriangle(Vector2{ (float)cx - 60, (float)cy - 30 }, Vector2{ (float)cx, (float)cy + 50 }, Vector2{ (float)cx + 60, (float)cy - 30 }, Color{ 220, 30, 30, 255 });
        DrawRectangle(cx - 60, cy - 34, 120, 8, WHITE);
        for (int k = 0; k < 6; k++) { float a = std::fmod(t * 0.7f + k / 6.f, 1.f); DrawCircle(cx - 30 + k * 12 + (int)(std::sin(t * 2 + k) * 6), cy - 100 - (int)(a * 60), 6 + a * 6, Color{ 255, 255, 255, (unsigned char)(100 * (1 - a)) }); }
        adSlogan("FRITES DU STADE", "Croustillantes à la mi-temps, irrésistibles après les prolongations.", Color{ 180, 20, 20, 255 }, Color{ 60, 20, 10, 255 }, t);
        break;
    }
    case 6: {   // MUTUELLE DU SUPPORTER : parapluie au-dessus d'une tribune sous la pluie
        DrawRectangleGradientV(0, 0, VW, VH, Color{ 60, 80, 110, 255 }, Color{ 30, 40, 60, 255 });
        for (int k = 0; k < 120; k++) { float y = std::fmod(k * 37.f + t * 300.f, (float)VH), x = (float)((k * 89) % VW); if (x > VW / 2 - 140 && x < VW / 2 + 140 && y > 110 && y < 200) continue; DrawLine((int)x, (int)y, (int)x - 2, (int)y + 6, Color{ 180, 200, 240, 160 }); }
        fxCrowd(60, 150, VW - 120, 40, hexc(H.home.shirt), hexc(H.home.shorts), t, 0.4f, 5, 6);
        int ux = VW / 2, uy = 110;
        DrawCircleSector(Vector2{ (float)ux, (float)uy }, 140, 180, 360, 24, Color{ 40, 160, 120, 255 });
        for (int k = 0; k < 6; k++) DrawLine(ux, uy, ux - 140 + k * 56, uy, Color{ 20, 100, 80, 255 });
        DrawRectangle(ux - 2, uy, 4, 60, Color{ 80, 60, 40, 255 });
        adSlogan("MUTUELLE DU SUPPORTER", "Qu'il pleuve ou qu'on perde 3-0, on vous couvre.", WHITE, Color{ 200, 240, 220, 255 }, t);
        break;
    }
    case 7: {   // TURBO FIBRE : traits de lumière, barre de téléchargement instantanée, box qui clignote
        DrawRectangleGradientV(0, 0, VW, VH, Color{ 6, 16, 50, 255 }, Color{ 0, 60, 90, 255 });
        for (int k = 0; k < 40; k++) {
            float sp = 300 + (k * 53) % 400; int y = 20 + (k * 37) % 180;
            int x = (int)std::fmod(k * 97.f + t * sp, (float)VW + 200) - 100;
            DrawRectangleGradientH(x, y, 90, 2, Color{ 0, 220, 255, 0 }, Color{ 120, 255, 255, 220 });
        }
        int bx = VW / 2 - 70, by = 140;
        DrawRectangle(bx, by, 140, 40, Color{ 230, 235, 245, 255 }); DrawRectangle(bx, by + 34, 140, 6, Color{ 170, 180, 200, 255 });
        for (int k = 0; k < 5; k++) DrawCircle(bx + 20 + k * 25, by + 18, 4, ((int)(t * 10) + k) % 3 ? Color{ 0, 230, 120, 255 } : Color{ 20, 80, 50, 255 });
        float pr = std::min(1.f, t * 2.5f);
        DrawRectangle(VW / 2 - 150, 196, 300, 12, Color{ 0, 0, 0, 140 }); DrawRectangle(VW / 2 - 150, 196, (int)(300 * pr), 12, Color{ 0, 230, 255, 255 });
        drawTextCentered(pr < 1 ? fmt("Téléchargement du match : %d %%", (int)(pr * 100)) : std::string("Match téléchargé en 0,4 s !"), VW / 2, 182, 10, WHITE, false);
        adSlogan("TURBO FIBRE", "Le replay avant même la fin de l'action. Ping garanti plus bas que votre défense.", Color{ 0, 230, 255, 255 }, WHITE, t);
        break;
    }
    case 8: {   // VOLT AUTO : voiture électrique sur une route, ville la nuit, éclairs
        DrawRectangleGradientV(0, 0, VW, VH, Color{ 20, 10, 60, 255 }, Color{ 230, 90, 120, 255 });
        for (int k = 0; k < 16; k++) { int h = 40 + (k * 41) % 90; DrawRectangle(k * 42, 200 - h, 38, h, Color{ 30, 20, 60, 255 }); for (int w = 0; w < h - 10; w += 10) if ((k + w) % 3) DrawRectangle(k * 42 + 8, 206 - h + w, 6, 4, Color{ 255, 220, 120, 200 }); }
        DrawRectangle(0, 200, VW, 30, Color{ 40, 40, 50, 255 });
        for (int k = 0; k < 12; k++) { float lx = std::fmod(k * 60 - t * 300, (float)VW + 60); if (lx < 0) lx += VW + 60; DrawRectangle((int)lx - 30, 214, 30, 3, Color{ 240, 240, 240, 255 }); }
        int cx = (int)(std::fmin(1.f, t * 0.7f) * (VW / 2 + 60)) - 60, cy = 186;
        DrawRectangle(cx - 50, cy, 100, 16, Color{ 230, 240, 255, 255 }); DrawRectangle(cx - 30, cy - 12, 56, 14, Color{ 230, 240, 255, 255 });
        DrawRectangle(cx - 24, cy - 9, 22, 9, Color{ 60, 120, 200, 255 }); DrawRectangle(cx + 2, cy - 9, 20, 9, Color{ 60, 120, 200, 255 });
        DrawRectangle(cx - 50, cy + 6, 100, 3, Color{ 0, 200, 255, 255 });
        for (int w = 0; w < 2; w++) { DrawCircle(cx - 30 + w * 60, cy + 16, 8, Color{ 20, 20, 24, 255 }); DrawCircle(cx - 30 + w * 60, cy + 16, 3, Color{ 180, 180, 190, 255 }); }
        if (((int)(t * 5)) % 2) { DrawTriangle(Vector2{ (float)cx + 60, (float)cy - 30 }, Vector2{ (float)cx + 52, (float)cy - 10 }, Vector2{ (float)cx + 66, (float)cy - 14 }, Color{ 255, 240, 80, 255 }); DrawTriangle(Vector2{ (float)cx + 66, (float)cy - 14 }, Vector2{ (float)cx + 52, (float)cy - 10 }, Vector2{ (float)cx + 58, (float)cy + 6 }, Color{ 255, 240, 80, 255 }); }
        adSlogan("VOLT AUTO", "Zéro émission, cent pour cent contre-attaque. 0 à 100 km/h plus vite qu'un ailier.", WHITE, Color{ 255, 230, 160, 255 }, t);
        break;
    }
    case 9: {   // PIZZA PENALTY : une pizza tirée au but
        DrawRectangle(0, 0, VW, VH, Color{ 30, 120, 50, 255 });
        for (int k = 0; k < 10; k++) if (k % 2) DrawRectangle(k * VW / 10, 0, VW / 10, 220, Color{ 36, 132, 58, 255 });
        int gx = VW - 220, gy = 50;
        DrawRectangle(gx, gy, 160, 6, WHITE); DrawRectangle(gx, gy, 6, 120, WHITE); DrawRectangle(gx + 154, gy, 6, 120, WHITE);
        for (int k = 0; k < 12; k++) DrawLine(gx + 6 + k * 13, gy + 6, gx + 6 + k * 13, gy + 120, Color{ 230, 230, 230, 110 });
        for (int k = 0; k < 9; k++) DrawLine(gx + 6, gy + 6 + k * 13, gx + 154, gy + 6 + k * 13, Color{ 230, 230, 230, 110 });
        float u = std::min(1.f, t * 0.8f);
        int px = 80 + (int)(u * (gx + 80 - 80)), py = 190 - (int)(std::sin(u * 3.14159f) * 90) - (int)(u * 40);
        DrawCircle(px, py, 26, Color{ 230, 170, 70, 255 }); DrawCircle(px, py, 22, Color{ 220, 60, 30, 255 });
        for (int k = 0; k < 7; k++) { float a = k * 0.9f + t * 6; DrawCircle(px + (int)(std::cos(a) * 12), py + (int)(std::sin(a) * 12), 4, Color{ 170, 30, 30, 255 }); DrawCircle(px + (int)(std::cos(a + 0.4f) * 6), py + (int)(std::sin(a + 0.4f) * 6), 2, Color{ 250, 240, 200, 255 }); }
        if (u >= 1.f) drawTextCentered("BUUUT !", gx + 80, 20, 20, Color{ 255, 230, 90, 255 });
        adSlogan("PIZZA PENALTY", "Livrée en 11 mètres chrono. Jamais arrêtée par le gardien.", Color{ 255, 230, 90, 255 }, WHITE, t);
        break;
    }
    case 10: {  // 8-BIT ENERGY : canette, étoiles en pixels, fond qui pulse
        float pl = 0.5f + 0.5f * std::sin(t * 6);
        DrawRectangleGradientV(0, 0, VW, VH, Color{ (unsigned char)(60 + 60 * pl), 10, 120, 255 }, Color{ 10, 0, 40, 255 });
        for (int k = 0; k < 60; k++) { int x = (k * 131) % VW, y = (k * 71) % 220; if (((int)(t * 8) + k) % 4) DrawRectangle(x, y, 3, 3, Color{ 255, 255, 120, 220 }); }
        int cx = VW / 2, cy = 120;
        DrawRectangle(cx - 30, cy - 60, 60, 120, Color{ 30, 30, 36, 255 }); DrawRectangle(cx - 30, cy - 60, 60, 8, Color{ 180, 180, 190, 255 }); DrawRectangle(cx - 30, cy + 52, 60, 8, Color{ 180, 180, 190, 255 });
        DrawRectangle(cx - 30, cy - 20, 60, 30, Color{ 120, 255, 60, 255 });
        DrawTriangle(Vector2{ (float)cx + 6, (float)cy - 40 }, Vector2{ (float)cx - 12, (float)cy + 2 }, Vector2{ (float)cx + 2, (float)cy + 2 }, Color{ 255, 230, 40, 255 });
        DrawTriangle(Vector2{ (float)cx + 2, (float)cy - 6 }, Vector2{ (float)cx - 6, (float)cy + 40 }, Vector2{ (float)cx + 14, (float)cy - 6 }, Color{ 255, 230, 40, 255 });
        drawTextCentered("8-BIT", cx, cy - 16, 10, BLACK, false);
        adSlogan("8-BIT ENERGY", "Recharge tes 3 vies avant la seconde période. Saveur cartouche soufflée.", Color{ 120, 255, 60, 255 }, WHITE, t);
        break;
    }
    case 11: {  // BANQUE DU BUT : pièces qui tombent dans un filet
        DrawRectangleGradientV(0, 0, VW, VH, Color{ 10, 50, 40, 255 }, Color{ 4, 20, 16, 255 });
        int gx = VW / 2 - 90, gy = 70;
        DrawRectangle(gx, gy, 180, 5, WHITE); DrawRectangle(gx, gy, 5, 130, WHITE); DrawRectangle(gx + 175, gy, 5, 130, WHITE);
        for (int k = 0; k < 14; k++) DrawLine(gx + 5 + k * 13, gy + 5, gx + 5 + k * 13, gy + 130, Color{ 220, 220, 220, 80 });
        int pile = (int)std::min(8.f, t * 3);
        for (int k = 0; k < pile; k++) DrawRectangle(gx + 30, gy + 118 - k * 6, 120, 6, k % 2 ? Color{ 230, 190, 50, 255 } : Color{ 250, 215, 80, 255 });
        for (int k = 0; k < 14; k++) { float y = std::fmod(t * 160 + k * 37, 160.f); int x = gx + 20 + (k * 47) % 140; DrawCircle(x, gy - 40 + (int)y, 5, Color{ 250, 210, 70, 255 }); DrawCircleLines(x, gy - 40 + (int)y, 5, Color{ 160, 120, 20, 255 }); }
        adSlogan("BANQUE DU BUT", "Votre épargne au fond des filets. Taux garanti même en cas de défaite.", Color{ 250, 215, 80, 255 }, WHITE, t);
        break;
    }
    case 12: {  // PIXEL BURGER : burger qui s'empile étage par étage
        DrawRectangleGradientV(0, 0, VW, VH, Color{ 255, 190, 40, 255 }, Color{ 230, 90, 20, 255 });
        for (int k = 0; k < 12; k++) { float a = k * 0.5236f + t * 0.4f; DrawTriangle(Vector2{ VW / 2.f, 120.f }, Vector2{ VW / 2.f + std::cos(a + 0.2f) * 500, 120.f + std::sin(a + 0.2f) * 500 }, Vector2{ VW / 2.f + std::cos(a) * 500, 120.f + std::sin(a) * 500 }, Color{ 255, 210, 80, 120 }); }
        int cx = VW / 2, base = 190;
        struct L { int h, w; Color c; } LY[] = { { 14, 120, Color{ 200, 130, 50, 255 } }, { 10, 130, Color{ 110, 60, 30, 255 } }, { 5, 134, Color{ 250, 210, 40, 255 } }, { 6, 128, Color{ 80, 180, 60, 255 } }, { 6, 120, Color{ 220, 50, 40, 255 } }, { 26, 124, Color{ 215, 140, 55, 255 } } };
        int n = std::min(6, (int)(t * 4.f) + 1), yy = base;
        for (int k = 0; k < n; k++) {
            int drop = k == n - 1 ? (int)(std::max(0.f, 1.f - std::fmod(t * 4.f, 1.f)) * 30) : 0;
            yy -= LY[k].h;
            if (k == 5) DrawEllipse(cx, yy + 14 - drop, LY[k].w / 2.f, 16, LY[k].c); else DrawRectangle(cx - LY[k].w / 2, yy - drop, LY[k].w, LY[k].h, LY[k].c);
            if (k == 5) for (int q = 0; q < 9; q++) DrawRectangle(cx - 40 + q * 10, yy + 4 - drop + (q % 2) * 3, 3, 2, Color{ 255, 240, 200, 255 });
        }
        adSlogan("PIXEL BURGER", "Double steak, triple fromage, zéro polygone. Menu Mi-temps à 4,99.", Color{ 255, 240, 120, 255 }, WHITE, t);
        break;
    }
    case 13: {  // HEXA MOBILE : smartphone, notification « BUT ! », réseau 16G
        DrawRectangleGradientV(0, 0, VW, VH, Color{ 30, 10, 70, 255 }, Color{ 0, 60, 110, 255 });
        for (int k = 0; k < 5; k++) { float r = std::fmod(t * 90.f + k * 40.f, 200.f); DrawCircleLines(VW / 2 + 60, 60, r, Color{ 120, 200, 255, (unsigned char)(160 * (1 - r / 200.f)) }); }
        int px = VW / 2 - 40, py = 30 + (int)(std::sin(t * 2) * 3);
        DrawRectangleRounded(Rectangle{ (float)px, (float)py, 80, 150 }, 0.15f, 6, Color{ 20, 20, 26, 255 });
        DrawRectangle(px + 6, py + 12, 68, 124, Color{ 40, 140, 70, 255 });
        DrawRectangle(px + 6, py + 12, 68, 2, WHITE); DrawRectangle(px + 38, py + 12, 2, 124, Color{ 255, 255, 255, 120 });
        if (t > 0.6f) { float a = std::min(1.f, (t - 0.6f) * 4); DrawRectangle(px + 8, py + 20, 64, 22, Color{ 250, 250, 250, (unsigned char)(240 * a) }); drawTextPx("BUT !", px + 14, py + 22, 10, Color{ 200, 30, 40, (unsigned char)(255 * a) }); drawTextPx("en direct", px + 14, py + 31, 5, Color{ 60, 60, 60, (unsigned char)(255 * a) }); }
        adSlogan("HEXA MOBILE", "Le réseau 16G : vous voyez le but avant même qu'il soit marqué.", Color{ 120, 220, 255, 255 }, WHITE, t);
        break;
    }
    case 14: {  // SPRITE VOYAGES : avion au-dessus de la mer, soleil couchant
        DrawRectangleGradientV(0, 0, VW, 150, Color{ 250, 120, 80, 255 }, Color{ 255, 210, 120, 255 });
        DrawCircle(VW / 2, 150, 50, Color{ 255, 240, 160, 255 });
        DrawRectangleGradientV(0, 150, VW, VH - 150, Color{ 30, 110, 170, 255 }, Color{ 10, 40, 90, 255 });
        for (int k = 0; k < 12; k++) DrawRectangle((int)std::fmod(k * 83 + t * 30, (float)VW), 160 + k * 6, 40 - k * 2, 1, Color{ 255, 230, 160, 120 });
        int ax = (int)std::fmod(t * 160.f, (float)VW + 200) - 100, ay = 70 - (int)(t * 6);
        DrawRectangle(ax, ay, 60, 10, WHITE); DrawTriangle(Vector2{ (float)ax + 60, (float)ay }, Vector2{ (float)ax + 60, (float)ay + 10 }, Vector2{ (float)ax + 72, (float)ay + 6 }, WHITE);
        DrawTriangle(Vector2{ (float)ax + 22, (float)ay + 4 }, Vector2{ (float)ax + 40, (float)ay + 4 }, Vector2{ (float)ax + 18, (float)ay + 26 }, Color{ 220, 220, 230, 255 });
        DrawTriangle(Vector2{ (float)ax, (float)ay }, Vector2{ (float)ax + 8, (float)ay }, Vector2{ (float)ax, (float)ay - 12 }, Color{ 200, 40, 50, 255 });
        for (int k = 0; k < 6; k++) DrawRectangle(ax + 12 + k * 7, ay + 3, 3, 3, Color{ 60, 120, 200, 255 });
        adSlogan("SPRITE VOYAGES", "Suivez votre équipe à l'extérieur : vols charters, hôtel 3 étoiles... et demie.", Color{ 255, 250, 200, 255 }, WHITE, t);
        break;
    }
    case 15: {  // GAZON PRO : tondeuse qui trace des bandes sur la pelouse
        for (int k = 0; k < 12; k++) DrawRectangle(0, k * 30, VW, 30, k % 2 ? Color{ 60, 150, 60, 255 } : Color{ 70, 168, 70, 255 });
        float mx = std::fmod(t * 180.f, (float)VW + 120) - 60;
        DrawRectangle(0, 120, (int)mx, 30, Color{ 110, 200, 90, 255 });
        int x = (int)mx, y = 112;
        DrawRectangle(x - 30, y, 50, 26, Color{ 220, 40, 30, 255 }); DrawRectangle(x - 26, y + 4, 42, 6, Color{ 255, 120, 90, 255 });
        DrawCircle(x - 22, y + 30, 7, Color{ 30, 30, 30, 255 }); DrawCircle(x + 12, y + 30, 7, Color{ 30, 30, 30, 255 });
        DrawLine(x - 30, y + 2, x - 60, y - 30, Color{ 60, 60, 60, 255 }); DrawRectangle(x - 66, y - 34, 14, 4, Color{ 60, 60, 60, 255 });
        for (int k = 0; k < 10; k++) DrawRectangle(x - 34 - (k * 7) % 30, y + 20 - (k * 5) % 16, 2, 2, Color{ 140, 230, 110, 220 });
        adSlogan("GAZON PRO", "Le gazon des pros, pour votre jardin. Rayures de stade garanties.", Color{ 230, 255, 200, 255 }, WHITE, t);
        break;
    }
    case 16: {  // CAFÉ CORNER : tasse fumante en forme de drapeau de corner
        DrawRectangleGradientV(0, 0, VW, VH, Color{ 70, 40, 25, 255 }, Color{ 25, 14, 8, 255 });
        int cx = VW / 2, cy = 140;
        DrawRectangle(cx - 46, cy - 40, 92, 70, Color{ 240, 236, 228, 255 }); DrawRectangle(cx - 46, cy - 40, 92, 8, Color{ 90, 50, 25, 255 });
        DrawRing(Vector2{ (float)cx + 50, (float)cy - 8 }, 10, 16, -90, 90, 12, Color{ 240, 236, 228, 255 });
        DrawEllipse(cx, cy + 34, 70, 8, Color{ 220, 216, 206, 255 });
        for (int k = 0; k < 3; k++) for (int q = 0; q < 8; q++) { float yy = cy - 50 - q * 9 - std::fmod(t * 20, 9.f); DrawCircle(cx - 20 + k * 20 + (int)(std::sin(yy * 0.1f + k + t * 2) * 5), (int)yy, 3, Color{ 255, 255, 255, (unsigned char)(90 - q * 10) }); }
        DrawRectangle(cx - 2, cy - 30, 2, 50, Color{ 200, 200, 200, 255 }); DrawTriangle(Vector2{ (float)cx, (float)cy - 30 }, Vector2{ (float)cx, (float)cy - 16 }, Vector2{ (float)cx + 18, (float)cy - 23 }, Color{ 230, 40, 40, 255 });
        adSlogan("CAFÉ CORNER", "Le petit noir des supporters. Serré comme un score de finale.", Color{ 255, 220, 160, 255 }, WHITE, t);
        break;
    }
    default: {  // auto-promotion de la chaîne : prochain grand rendez-vous
        DrawRectangle(0, 0, VW, VH, Color{ 10, 14, 30, 255 });
        fxSpotlights(40, 0, VW - 80, 200, t, 4, Color{ 200, 220, 255, 255 });
        std::string ch = g_tv.channel.empty() ? std::string("FOOT TV") : g_tv.channel;
        DrawRectangle(VW / 2 - 120, 60, 240, 40, Color{ 190, 30, 40, 255 }); drawTextCentered(ch, VW / 2, 70, 20, WHITE, false);
        drawKitIcon(H.home, VW / 2 - 140, 120, 6); drawKitIcon(A.home, VW / 2 + 92, 120, 6);
        bool ht = g_adsNext == SC_HALFTIME;
        drawTextCentered(ht ? "À SUIVRE" : "CE SOIR", VW / 2, 130, 20, C_HI); drawTextCentered(ht ? "LA SECONDE PÉRIODE" : "EN DIRECT", VW / 2, 156, 10, WHITE, false);
        adSlogan(fitText(H.shortName + " - " + A.shortName, VW - 40, 20), "Avant-match, mi-temps et après-match : toute l'émission sur " + ch + ".", C_HI, WHITE, t);
        break;
    }
    }
}

static void screenTvAds() {
    float dt = GetFrameTime(); g_adsT += dt;
    audioCrowd(false, 0); audioSupporters(CH_TENSE, 0);
    int spot = g_adsSpots[g_adsIdx];
    drawAdSpot(spot, g_adsT);
    // habillage TV : bandeau « Publicité », logo de la chaîne, compte à rebours
    DrawRectangle(VW - 104, 8, 96, 14, Color{ 0, 0, 0, 150 }); drawTextPx("PUBLICITÉ", VW - 98, 10, 10, WHITE);
    if (!g_tv.channel.empty()) { DrawRectangle(8, 8, textWidth(g_tv.channel, 10) + 10, 14, Color{ 190, 30, 40, 220 }); drawTextPx(g_tv.channel, 13, 10, 10, WHITE); }
    int remain = (int)std::ceil((g_adsCount - g_adsIdx) * AD_LEN - g_adsT);
    std::string back = fmt("Retour au match dans %d s   (OK : passer)", std::max(0, remain));
    DrawRectangle(VW / 2 - textWidth(back, 10) / 2 - 6, VH - 22, textWidth(back, 10) + 12, 14, Color{ 0, 0, 0, 150 });
    drawTextCentered(back, VW / 2, VH - 20, 10, Color{ 230, 230, 240, 255 }, false);
    DrawRectangle(0, VH - 4, (int)(VW * std::min(1.f, g_adsT / AD_LEN)), 4, C_HI);
    bool skip = IN.ok || IN.start || IN.back || IN.click;
    if (g_adsT >= AD_LEN || (skip && g_adsT > 0.25f)) {
        if (skip && (IN.back || IN.start)) g_adsIdx = g_adsCount;      // Retour / Entrée : toutes les pubs
        else g_adsIdx++;
        g_adsT = 0;
        if (g_adsIdx >= g_adsCount) { g_screen = g_adsNext; if (g_adsNext == SC_TVINTRO) audioJingle(1); return; }
        audioJingleForce(5 + g_adsSpots[g_adsIdx] % 3);
    }
}

// match amical télévisé : chaîne et duo de commentateurs
static void setFriendlyTv(int chan) {
    static const TvInfo TV[4] = { { "FRANCE SPORT", "Michel Larrieu", "Bernard Castagnet" }, { "CANAL FOOT+", "Stéphane Guy-Durand", "Patrice Vannier" },
                                  { "TF SPORT", "Thierry Delorme", "Jean-Michel Lavaud" }, { "FOOT 2 TV", "Julien Morvan", "Éric Castel" } };
    g_tv = TV[std::max(0, std::min(3, chan))];
    tvCrew();
}
