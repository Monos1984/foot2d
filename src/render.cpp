// Rendu 2D façon Amiga : terrain vu de dessus, petits sprites en pixel-art
#include "render.h"
#include "rlgl.h"
#include <cstring>
#include <cstdlib>

static Texture2D g_pitchTex[5];
static bool g_pitchBuilt[5] = { false };
static uint64_t g_pitchKey[5] = { 0 };
// description du stade du club qui reçoit (profondeur des tribunes, couverture, remplissage)
struct StadiumLook { float depth[4] = { 9, 9, 9, 9 }; int kind[4] = { 2, 2, 2, 2 }; float fill = 0.8f; bool small = false; int turf = 100; std::string sponsor, logo; };
std::string (*g_sponsorImagePath)(const std::string&) = nullptr;   // fourni par l'interface (sponsors.txt)
static StadiumLook g_look;
static std::vector<std::string> g_boardAds; static int g_boardAdsVer = 0;
void setCustomBoardAds(const std::vector<std::string>& paths) { if (paths != g_boardAds) { g_boardAds = paths; g_boardAdsVer++; } }
static const float HZ = 0.75f; // facteur de projection de la hauteur

Color hexc(unsigned rgb, unsigned char a) { return Color{ (unsigned char)((rgb >> 16) & 255), (unsigned char)((rgb >> 8) & 255), (unsigned char)(rgb & 255), a }; }

// ------------------------------------------------------------------ texte
void drawTextPx(const std::string& s, int x, int y, int size, Color c) {
    Font f = GetFontDefault();
    DrawTextEx(f, s.c_str(), Vector2{ (float)x, (float)y }, (float)size, (float)(size / 10), c);
}
int textWidth(const std::string& s, int size) {
    return (int)MeasureTextEx(GetFontDefault(), s.c_str(), (float)size, (float)(size / 10)).x;
}
void drawTextShadow(const std::string& s, int x, int y, int size, Color c) {
    int o = size >= 20 ? 2 : 1;
    drawTextPx(s, x + o, y + o, size, Color{ 0, 0, 0, (unsigned char)(c.a * 0.8f) });
    drawTextPx(s, x, y, size, c);
}
void drawTextCentered(const std::string& s, int cx, int y, int size, Color c, bool shadow) {
    int w = textWidth(s, size);
    if (shadow) drawTextShadow(s, cx - w / 2, y, size, c); else drawTextPx(s, cx - w / 2, y, size, c);
}
std::string fitText(const std::string& s, int maxw, int size) {
    if (textWidth(s, size) <= maxw) return s;
    std::string r = s;
    while (!r.empty() && textWidth(r + ".", size) > maxw) {
        // retire un caractère UTF-8 complet
        size_t n = r.size() - 1;
        while (n > 0 && ((unsigned char)r[n] & 0xC0) == 0x80) n--;
        r.erase(n);
    }
    return r + ".";
}

// ------------------------------------------------------------------ terrain
static unsigned grassColors[5][2] = {
    { 0x3E9A3E, 0x348A34 },  // normal
    { 0x7FA24A, 0x74953F },  // sec
    { 0x2F8A45, 0x277A3B },  // humide
    { 0x6E7A34, 0x5E6A2C },  // boueux
    { 0x9DC7B8, 0x8FB9AA },  // gelé
};

static void buildPitch(int type, uint64_t key) {
    if (g_pitchBuilt[type]) UnloadTexture(g_pitchTex[type]);
    g_pitchKey[type] = key;
    int W = (int)((PITCH_W + 2 * MARGIN) * PPM), H = (int)((PITCH_L + 2 * MARGIN) * PPM);
    Image img = GenImageColor(W, H, BLACK);
    Rng r(1234 + type);
    auto P = [&](float x, float y) { return Vector2{ (x + MARGIN) * PPM, (y + MARGIN) * PPM }; };
    // gazon avec bandes
    for (int y = 0; y < H; y++) {
        float wy = y / PPM - MARGIN;
        int band = (int)std::floor((wy + 100) / 5.25f) % 2;
        unsigned base = grassColors[type][band];
        for (int x = 0; x < W; x++) {
            unsigned c = base;
            float n = r.f();
            Color col = hexc(c);
            if (n < 0.08f) { col.r = (unsigned char)std::max(0, col.r - 10); col.g = (unsigned char)std::max(0, col.g - 12); }
            else if (n > 0.95f) { col.r = (unsigned char)std::min(255, col.r + 8); col.g = (unsigned char)std::min(255, col.g + 10); }
            ImageDrawPixel(&img, x, y, col);
        }
    }
    // boue : taches au centre et devant les buts
    if (type == 3) {
        for (int k = 0; k < 900; k++) {
            float x = PITCH_W / 2 + r.frange(-14, 14), y;
            float w = r.f();
            if (w < 0.4f) y = PITCH_L / 2 + r.frange(-12, 12); else if (w < 0.7f) y = r.frange(0, 14); else y = PITCH_L - r.frange(0, 14);
            Vector2 p = P(x, y);
            ImageDrawCircle(&img, (int)p.x, (int)p.y, r.range(2, 6), hexc(0x6B5230, 180));
        }
    }
    // pelouse usée : zones pelées devant les buts, au centre et dans les couloirs ; « champ de patates » : trous partout
    if (g_look.turf < 70) {
        Rng rt(777 + g_look.turf / 5);
        float bad = (70 - g_look.turf) / 70.f;
        int n = (int)(bad * bad * 2600) + 80;
        for (int k = 0; k < n; k++) {
            float x, y, w = rt.f();
            if (w < 0.3f) { x = PITCH_W / 2 + rt.frange(-10, 10); y = rt.chance(0.5f) ? rt.frange(0, 13) : PITCH_L - rt.frange(0, 13); }
            else if (w < 0.5f) { x = PITCH_W / 2 + rt.frange(-12, 12); y = PITCH_L / 2 + rt.frange(-12, 12); }
            else if (w < 0.75f) { x = rt.chance(0.5f) ? rt.frange(2, 12) : PITCH_W - rt.frange(2, 12); y = rt.frange(10, PITCH_L - 10); }
            else { if (bad < 0.6f) continue; x = rt.frange(0, PITCH_W); y = rt.frange(0, PITCH_L); }
            Vector2 p = P(x, y);
            unsigned col = rt.chance(0.5f) ? 0x8A7A4A : 0x6E5A34;
            ImageDrawCircle(&img, (int)p.x, (int)p.y, rt.range(1, 2 + (int)(bad * 4)), hexc(col, (unsigned char)(90 + bad * 140)));
            if (g_look.turf < 12 && rt.chance(0.15f)) ImageDrawCircle(&img, (int)p.x + 1, (int)p.y + 1, 1, hexc(0x3A2A18, 220));   // trous
        }
    }
    if (type == 4) {
        for (int k = 0; k < 3000; k++) { Vector2 p = P(r.frange(-MARGIN, PITCH_W + MARGIN), r.frange(-MARGIN, PITCH_L + MARGIN)); ImageDrawPixel(&img, (int)p.x, (int)p.y, hexc(0xF0F8FF)); }
    }
    Color L = hexc(0xF4F4F4);
    auto line = [&](float x1, float y1, float x2, float y2) {
        Vector2 a = P(x1, y1), b = P(x2, y2);
        ImageDrawLine(&img, (int)a.x, (int)a.y, (int)b.x, (int)b.y, L);
    };
    auto rect = [&](float x1, float y1, float x2, float y2) { line(x1, y1, x2, y1); line(x2, y1, x2, y2); line(x2, y2, x1, y2); line(x1, y2, x1, y1); };
    auto arc = [&](float cx, float cy, float rad, float a0, float a1, bool outsideBox, float boxY, bool top) {
        int n = std::max(60, (int)(rad * PPM * 8));
        for (int i = 0; i <= n; i++) {
            float a = a0 + (a1 - a0) * i / n;
            float x = cx + std::cos(a) * rad, y = cy + std::sin(a) * rad;
            if (outsideBox) { if (top && y <= boxY) continue; if (!top && y >= boxY) continue; }
            Vector2 p = P(x, y);
            ImageDrawPixel(&img, (int)p.x, (int)p.y, L);
        }
    };
    rect(0, 0, PITCH_W, PITCH_L);
    line(0, PITCH_L / 2, PITCH_W, PITCH_L / 2);
    arc(PITCH_W / 2, PITCH_L / 2, 9.15f, 0, 6.2832f, false, 0, false);
    for (int g = 0; g < 2; g++) {
        float y0 = g == 0 ? 0 : PITCH_L, s = g == 0 ? 1.f : -1.f;
        rect(PITCH_W / 2 - BOX_W / 2, y0, PITCH_W / 2 + BOX_W / 2, y0 + s * BOX_L);
        rect(PITCH_W / 2 - SIX_W / 2, y0, PITCH_W / 2 + SIX_W / 2, y0 + s * SIX_L);
        Vector2 sp = P(PITCH_W / 2, y0 + s * 11);
        ImageDrawRectangle(&img, (int)sp.x - 1, (int)sp.y - 1, 2, 2, L);
        arc(PITCH_W / 2, y0 + s * 11, 9.15f, 0, 6.2832f, true, y0 + s * BOX_L, g == 0);
    }
    Vector2 c = P(PITCH_W / 2, PITCH_L / 2);
    ImageDrawRectangle(&img, (int)c.x - 1, (int)c.y - 1, 2, 2, L);
    arc(0, 0, 1, 0, 1.5708f, false, 0, false); arc(PITCH_W, 0, 1, 1.5708f, 3.1416f, false, 0, false);
    arc(0, PITCH_L, 1, 4.7124f, 6.2832f, false, 0, false); arc(PITCH_W, PITCH_L, 1, 3.1416f, 4.7124f, false, 0, false);
    // panneaux publicitaires
    static const char* ADS[] = { "SSW", "BROKE STUDIO", "SUPER BUT", "ALEKMAUL", "GOAL FM", "SGDK", "PIXEL COLA", "PVSNESLIB",
                                 "OFFGAME", "DOUBLE SIDE", "BEURTON", "ELEC. DREAMS", "BALLON D'OR", "CRAMPONS+", "STADE TV", "AMIGOAL",
                                 "GAMOPAT", "ONIROMANCIE", "ATARI", "SEGA", "NINTENDO", "SONY", "SIGAMES", "EA", "AMSTRAD", "COMMODORE", "ORIC", "CAPCOM" };
    static const unsigned ADC[] = { 0xD62828, 0x1D3557, 0xF77F00, 0x2A9D8F, 0x6A4C93, 0x3A7D44, 0x264653, 0xE63946,
                                    0xB02020, 0x5A189A, 0x8C6D1F, 0x0077B6, 0x1D3557, 0x2A9D8F, 0xE9C46A, 0xE63946,
                                    0x3D5A80, 0x6A0572, 0xC1121F, 0x0B3D91, 0xE60012, 0x111111, 0x2B2D42, 0x1B1B1B, 0x8D0801, 0x33415C, 0x7B2CBF, 0x1F4E9E };
    const int NADS = (int)(sizeof ADS / sizeof ADS[0]);
    float bd = 4.0f; // distance du terrain
    int adi = 0;
    // images de l'éditeur de publicités : chargées une fois pour cette construction du terrain
    std::vector<Image> custom;
    for (auto& path : g_boardAds) if (FileExists(path.c_str())) { Image im = LoadImage(path.c_str()); if (im.data) custom.push_back(im); }
    int cadi = 0;
    auto board = [&](float x1, float y1, float x2, float y2, bool horiz) {
        Vector2 a = P(x1, y1), b = P(x2, y2);
        int w = (int)(b.x - a.x), h = (int)(b.y - a.y);
        if (g_look.small && (adi++ % 4) != 0) {
            // petit stade : main courante blanche au lieu des panneaux
            if (horiz) { ImageDrawRectangle(&img, (int)a.x, (int)a.y + h / 2, w, 1, hexc(0xE8E8E8)); for (int k = 0; k < w; k += 8) ImageDrawRectangle(&img, (int)a.x + k, (int)a.y + h / 2 - 1, 1, 3, hexc(0xB0B0B0)); }
            else { ImageDrawRectangle(&img, (int)a.x + w / 2, (int)a.y, 1, h, hexc(0xE8E8E8)); for (int k = 0; k < h; k += 8) ImageDrawRectangle(&img, (int)a.x + w / 2 - 1, (int)a.y + k, 3, 1, hexc(0xB0B0B0)); }
            return;
        }
        // publicités de l'éditeur : un panneau sur deux
        if (!custom.empty() && adi % 2 == 1) {
            ImageDrawRectangle(&img, (int)a.x, (int)a.y, w, h, hexc(0xF4F4F4));
            Image lg = ImageCopy(custom[cadi++ % custom.size()]);
            if (!horiz) ImageRotateCW(&lg);       // panneaux le long des lignes de touche : image tournée
            float k = std::min((float)(w - 2) / lg.width, (float)(h - 2) / lg.height);
            ImageResize(&lg, std::max(1, (int)(lg.width * k)), std::max(1, (int)(lg.height * k)));
            ImageDraw(&img, lg, Rectangle{ 0, 0, (float)lg.width, (float)lg.height }, Rectangle{ a.x + (w - lg.width) / 2.f, a.y + (h - lg.height) / 2.f, (float)lg.width, (float)lg.height }, WHITE);
            UnloadImage(lg);
            ImageDrawRectangleLines(&img, Rectangle{ a.x, a.y, (float)w, (float)h }, 1, hexc(0x111111));
            adi++;
            return;
        }
        // sponsor du club qui reçoit : un panneau sur trois (logo si une image est fournie)
        if (!g_look.sponsor.empty() && adi % 3 == 0) {
            ImageDrawRectangle(&img, (int)a.x, (int)a.y, w, h, hexc(0xF4F4F4));
            ImageDrawRectangleLines(&img, Rectangle{ a.x, a.y, (float)w, (float)h }, 1, hexc(0x111111));
            bool drawn = false;
            if (!g_look.logo.empty() && FileExists(g_look.logo.c_str())) {
                Image lg = LoadImage(g_look.logo.c_str());
                if (lg.data) {
                    int bw = horiz ? w - 2 : w - 2, bh = horiz ? h - 2 : h - 2;
                    float k = std::min((float)bw / lg.width, (float)bh / lg.height);
                    ImageResize(&lg, std::max(1, (int)(lg.width * k)), std::max(1, (int)(lg.height * k)));
                    ImageDraw(&img, lg, Rectangle{ 0, 0, (float)lg.width, (float)lg.height }, Rectangle{ a.x + (w - lg.width) / 2.f, a.y + (h - lg.height) / 2.f, (float)lg.width, (float)lg.height }, WHITE);
                    UnloadImage(lg); drawn = true;
                }
            }
            if (!drawn && horiz) ImageDrawText(&img, g_look.sponsor.c_str(), (int)a.x + 3, (int)a.y, 10, hexc(0x1B2A63));
            adi++;
            return;
        }
        ImageDrawRectangle(&img, (int)a.x, (int)a.y, w, h, hexc(ADC[adi % NADS]));
        ImageDrawRectangleLines(&img, Rectangle{ a.x, a.y, (float)w, (float)h }, 1, hexc(0x111111));
        if (horiz) ImageDrawText(&img, ADS[adi % NADS], (int)a.x + 3, (int)a.y, 10, WHITE);
        adi++;
    };
    for (float x = -8; x < PITCH_W + 8; x += 14) { board(x, -bd - 2.0f, x + 13.8f, -bd, true); board(x, PITCH_L + bd, x + 13.8f, PITCH_L + bd + 2.0f, true); }
    for (float y = -4; y < PITCH_L + 4; y += 14) { board(-bd - 2.0f, y, -bd, y + 13.8f, false); board(PITCH_W + bd, y, PITCH_W + bd + 2.0f, y + 13.8f, false); }
    // bancs de touche (côté gauche, de part et d'autre de la ligne médiane) et zone technique
    for (int k = 0; k < 2; k++) {
        float yc = PITCH_L / 2 + (k ? 7.5f : -7.5f);
        Vector2 a = P(-3.6f, yc - 4.0f);
        int w = (int)(2.4f * PPM), h = (int)(8.0f * PPM);
        ImageDrawRectangle(&img, (int)a.x + 2, (int)a.y + 2, w, h, hexc(0x0A0A0A, 90));
        ImageDrawRectangle(&img, (int)a.x, (int)a.y, w, h, hexc(0x3A4A5A));
        ImageDrawRectangle(&img, (int)a.x, (int)a.y, 3, h, hexc(0x9FB4C8));          // toit transparent
        for (int j = 3; j < h - 2; j += 4) ImageDrawRectangle(&img, (int)a.x + 5, (int)a.y + j, w - 7, 2, hexc(k ? 0xC03030 : 0x3050C0));
        Vector2 z1 = P(-1.0f, yc - 5.0f), z2 = P(-1.0f, yc + 5.0f);
        for (int yy = (int)z1.y; yy < (int)z2.y; yy += 3) ImageDrawPixel(&img, (int)z1.x, yy, hexc(0xE0E0E0));
    }
    // table du 4e arbitre
    { Vector2 a = P(-2.6f, PITCH_L / 2 - 1.0f); ImageDrawRectangle(&img, (int)a.x, (int)a.y, 6, 10, hexc(0xE8E8E8)); ImageDrawRectangle(&img, (int)a.x + 1, (int)a.y + 2, 4, 6, hexc(0x404040)); }
    // tribunes selon le stade qui reçoit : 0 principale (bas, x<0 côté gauche = tribune latérale), on associe :
    // côté gauche = tribune principale, côté droit = tribune face, haut = virage nord, bas = virage sud
    static const unsigned CROWD[] = { 0x2B2D42, 0x8D99AE, 0xB23A48, 0xD9D9D9, 0x1D3557, 0xC98A4B, 0x3A3A3A, 0x6D597A, 0xC9A94F, 0x3F8F7A };
    const StadiumLook& L0 = g_look;
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
        float wx = x / PPM - MARGIN, wy = y / PPM - MARGIN;
        float edge = bd + 2.2f;
        bool left = wx < -edge, right = wx > PITCH_W + edge, top = wy < -edge, bot = wy > PITCH_L + edge;
        if (!(left || right || top || bot)) continue;
        int side; float dist;
        if (left && !(top || bot)) { side = 0; dist = -edge - wx; }
        else if (right && !(top || bot)) { side = 1; dist = wx - PITCH_W - edge; }
        else if (top && !(left || right)) { side = 2; dist = -edge - wy; }
        else if (bot && !(left || right)) { side = 3; dist = wy - PITCH_L - edge; }
        else { side = -1; dist = 0; }   // coins
        uint32_t h0 = (uint32_t)(x * 2654435761u) ^ (uint32_t)(y * 40503u); h0 ^= h0 >> 13; h0 *= 0x5bd1e995; h0 ^= h0 >> 15;
        Color base;
        float depth = side >= 0 ? L0.depth[side] : std::min(std::min(L0.depth[0], L0.depth[1]), std::min(L0.depth[2], L0.depth[3]));
        if (side < 0 && depth < 6) depth = 0;
        if (dist > depth) {
            // hors tribune : pelouse, arbres ou parking pour les petits stades, coursive sombre pour les grands
            if (L0.small) {
                base = (h0 % 7 == 0) ? hexc(0x2E6B2E) : hexc(0x3A7F3A);
                if ((x / 9 + y / 11) % 5 == 0 && h0 % 3 == 0) base = hexc(0x1F4F24);   // arbres
            } else base = hexc(0x23263A);
            ImageDrawPixel(&img, x, y, base);
            continue;
        }
        int kind = side >= 0 ? L0.kind[side] : 2;
        bool vert = side == 0 || side == 1;
        int row = vert ? x / 3 : y / 3;
        int col = vert ? y / 3 : x / 3;
        if (kind == 0) base = (row % 2) ? hexc(0x4A6B3A) : hexc(0x44633A);          // talus / places debout
        else base = (row % 2) ? hexc(0x3A3F58) : hexc(0x323650);
        int cx = vert ? y % 3 : x % 3, cy = vert ? x % 3 : y % 3;
        if (cx < 2 && cy < 2) {
            uint32_t h = (uint32_t)(row * 7919 + col * 104729);
            h ^= h >> 13; h *= 0x5bd1e995; h ^= h >> 15;
            float f = L0.fill * (kind == 0 ? 0.6f : 1.f);
            if ((h % 1000) < (uint32_t)(f * 1000)) base = hexc(CROWD[(h / 10) % 10]);
            if (cy == 0 && h % 3 == 0 && (h % 1000) < (uint32_t)(f * 1000)) base = hexc(0x2A1E14);
        }
        // toit des tribunes couvertes (bord extérieur)
        if (kind == 2 && dist > depth - 2.2f) { base = hexc((row % 2) ? 0x8A90A8 : 0x7C8298); if ((col % 8) == 0) base = hexc(0x5A607A); }
        ImageDrawPixel(&img, x, y, base);
    }
    for (auto& im : custom) UnloadImage(im);
    g_pitchTex[type] = LoadTextureFromImage(img);
    SetTextureFilter(g_pitchTex[type], TEXTURE_FILTER_POINT);
    UnloadImage(img);
    g_pitchBuilt[type] = true;
}

void renderInit() {}
void renderShutdown() { for (int i = 0; i < 5; i++) if (g_pitchBuilt[i]) UnloadTexture(g_pitchTex[i]); }

// ------------------------------------------------------------------ sprites
// sprites 7 x 12 : h cheveux, H cheveux (ombre), s peau, t peau (ombre), e yeux, S maillot, c col / liseré (2e couleur),
// n numéro dans le dos, P short, k chaussettes, K bande des chaussettes, b chaussures
// course en 4 temps : 0 et 2 appuis (jambes serrées), 1 et 3 foulées (bras et jambes opposés)
static const char* SPR_DOWN[4][12] = {
    { "..HhH..", ".hhhhh.", ".heseh.", "..tst..", "SSScSSS", "sSSSSSs", "sSSSSSs", ".SSSSS.", ".PPPPP.", ".PP.PP.", ".kk.kk.", ".bb.bb." },
    // foulée : jambe gauche en avant (pied plus bas), jambe droite levée ; bras opposés
    { "..HhH..", ".hhhhh.", ".heseh.", "..tst..", "SSScSSS", "sSSSSSs", ".SSSSSs", ".SSSSS.", ".PPPPP.", ".PP.kk.", ".kk.bb.", ".bb...." },
    { "..HhH..", ".hhhhh.", ".heseh.", "..tst..", "SSScSSS", "sSSSSSs", "sSSSSSs", ".SSSSS.", ".PPPPP.", ".PP.PP.", ".kk.kk.", ".bb.bb." },
    { "..HhH..", ".hhhhh.", ".heseh.", "..tst..", "SSScSSS", "sSSSSSs", "sSSSSS.", ".SSSSS.", ".PPPPP.", ".kk.PP.", ".bb.kk.", "....bb." },
};
static const char* SPR_UP[4][12] = {
    { "..HhH..", ".hhhhh.", ".hhhhh.", "..hhh..", "SSScSSS", "sSSSSSs", "sSSnSSs", ".SSnSS.", ".PPPPP.", ".PP.PP.", ".kk.kk.", ".bb.bb." },
    { "..HhH..", ".hhhhh.", ".hhhhh.", "..hhh..", "SSScSSS", "sSSSSSs", "sSSnSS.", ".SSnSS.", ".PPPPP.", ".kk.PP.", ".bb.kk.", "....bb." },
    { "..HhH..", ".hhhhh.", ".hhhhh.", "..hhh..", "SSScSSS", "sSSSSSs", "sSSnSSs", ".SSnSS.", ".PPPPP.", ".PP.PP.", ".kk.kk.", ".bb.bb." },
    { "..HhH..", ".hhhhh.", ".hhhhh.", "..hhh..", "SSScSSS", "sSSSSSs", ".SSnSSs", ".SSnSS.", ".PPPPP.", ".PP.kk.", ".kk.bb.", ".bb...." },
};
static const char* SPR_RIGHT[4][12] = {
    { "..hhH..", ".hhhss.", ".hhses.", "..tss..", "..SSc..", "..SsS..", "..SsS..", "..SSS..", "..PPP..", "..PPP..", "..kk...", "..bbb.." },
    { "..hhH..", ".hhhss.", ".hhses.", "..tss..", "..SSc..", ".sSSSs.", ".sSSS.s", "..SSS..", "..PPP..", ".PP.PP.", ".k...k.", "bb...bb" },
    { "..hhH..", ".hhhss.", ".hhses.", "..tss..", "..SSc..", "..SsS..", "..SsS..", "..SSS..", "..PPP..", "..PPkk.", "..k..b.", "..bb..." },
    { "..hhH..", ".hhhss.", ".hhses.", "..tss..", "..SSc..", ".sSSSs.", "s.SSSs.", "..SSS..", "..PPP..", ".PP.PP.", ".k...k.", "bb...bb" },
};
// frappe : jambe tendue vers l'avant
static const char* SPR_KICK_DOWN[12] = { "..HhH..", ".hhhhh.", ".heseh.", "..tst..", "SSScSSS", "sSSSSSs", ".SSSSS.", ".SSSSS.", ".PPPPP.", ".PP.PP.", ".kk..kk", ".bb..bb" };
static const char* SPR_KICK_UP[12] = { "..HhH..", ".hhhhh.", ".hhhhh.", "..hhh..", "SSScSSS", "sSSSSSs", ".SSnSS.", ".SSnSS.", ".PPPPP.", ".PP.PP.", "kk..kk.", "bb..bb." };
static const char* SPR_KICK_RIGHT[12] = { "..hhH..", ".hhhss.", ".hhses.", "..tss..", "..SSc..", ".sSSSs.", "s.SSS..", "..SSS..", "..PPPP.", ".PP..kk", ".k...bb", "b......" };
// tête : bras écartés
static const char* SPR_HEAD[12] = { "..HhH..", ".hhhhh.", ".heseh.", "s.tst.s", "SSScSSS", ".SSSSS.", ".SSSSS.", ".SSSSS.", ".PPPPP.", ".PP.PP.", ".kk.kk.", ".bb.bb." };
// gardien : ballon serré contre la poitrine
static const char* SPR_HOLD[12] = { "..HhH..", ".hhhhh.", ".heseh.", "..tst..", "SSScSSS", "SsSSSsS", ".sssss.", ".SSSSS.", ".PPPPP.", ".PP.PP.", ".kk.kk.", ".bb.bb." };
// célébration : « l'avion », bras à l'horizontale ; bras levés
static const char* SPR_CELEB2[12] = { "..HhH..", ".hhhhh.", ".heseh.", "..tst..", "sSScSSs", "sSSSSSs", ".SSSSS.", ".SSSSS.", ".PPPPP.", ".PP.PP.", ".kk.kk.", ".bb.bb." };
static const char* SPR_CELEB[12] = { "s.HhH.s", "s.hhh.s", "S.ese.S", "SStstSS", ".SScSS.", ".SSSSS.", ".SSSSS.", ".SSSSS.", ".PPPPP.", ".PP.PP.", ".kk.kk.", ".bb.bb." };
static const char* SPR_LYING[5] = { ".hh.........", "hss.SSS.....", ".sSSSSSPPkkb", "...SSSSPPkkb", "............" };
// plongeon du gardien : corps à l'horizontale, bras tendus
static const char* SPR_DIVE[5] = { ".....SSS....", "bkkPPSSSShss", "bkkPPSSSShss", "......SSS...", "............" };

static unsigned SKIN[NUM_SKINS] = { 0xF2C9A0, 0xD9A066, 0xA86B3C, 0x6B4226, 0xFFE0C8, 0x8A5A34 };
static unsigned HAIR[NUM_HAIRS] = { 0x1A1A1A, 0x5A3A1A, 0xE8C35A, 0xB5502A, 0x3B2A1A, 0x9A9A9A, 0xEDEDED };
const char* SKIN_NAMES[NUM_SKINS] = { "claire", "mate", "hâlée", "foncée", "très claire", "brune" };
const char* HAIR_NAMES[NUM_HAIRS] = { "noirs", "châtains", "blonds", "roux", "bruns", "gris", "blancs" };

static float colorBright(unsigned c) { return (((c >> 16) & 255) * 0.3f + ((c >> 8) & 255) * 0.59f + (c & 255) * 0.11f) / 255.f; }
static int g_bootCol = 0;          // couleur des chaussures du joueur en cours de dessin
static Color pixelColor(char ch, int x, int y, const Kit& k, int skin, int hair, bool gk, unsigned gkShirt) {
    auto dim = [](Color c, float f) { return Color{ (unsigned char)(c.r * f), (unsigned char)(c.g * f), (unsigned char)(c.b * f), 255 }; };
    switch (ch) {
    case 'h': return hexc(HAIR[hair % NUM_HAIRS]);
    case 'H': return dim(hexc(HAIR[hair % NUM_HAIRS]), 0.7f);
    case 's': return hexc(SKIN[skin % NUM_SKINS]);
    case 't': return dim(hexc(SKIN[skin % NUM_SKINS]), 0.82f);
    case 'e': return Color{ 25, 20, 20, 255 };
    case 'c': case 'n': case 'K': {
        if (gk) return ch == 'K' ? hexc(gkShirt) : dim(hexc(gkShirt), 0.75f);
        unsigned c2 = k.shirt2 != k.shirt ? k.shirt2 : (colorBright(k.shirt) > 0.55f ? 0x202020 : 0xF2F2F2);
        if (ch == 'K') return hexc(c2);
        return hexc(c2);
    }
    case 'S': {
        if (gk) return hexc(gkShirt);
        unsigned c = k.shirt;
        switch (k.pattern) {
        case KP_VSTRIPES: if (x % 2 == 0) c = k.shirt2; break;
        case KP_HOOPS: if (y % 2 == 1) c = k.shirt2; break;
        case KP_HALVES: if (x >= 4) c = k.shirt2; break;
        case KP_SASH: if ((x + y) % 5 == 0 || (x + y) % 5 == 1) c = k.shirt2; break;
        case KP_CHECK: if ((x + y) % 2) c = k.shirt2; break;
        case KP_SLEEVES: if (x <= 1 || x >= 5) c = k.shirt2; break;
        case KP_PINSTRIPES: if (x % 3 == 1) c = k.shirt2; break;                               // fines rayures
        case KP_QUARTERS: if ((x >= 4) != ((y % 4) >= 2)) c = k.shirt2; break;                 // quartiers
        case KP_BAND: if (y % 4 == 2) c = k.shirt2; break;                                      // bande pectorale
        case KP_SHOULDERS: if (y % 4 == 0) c = k.shirt2; break;                                 // épaules contrastées
        case KP_CHEVRON: if (y % 4 == std::abs(x - 3)) c = k.shirt2; break;                     // chevron (V)
        case KP_CROSS: if (x == 3 || y % 4 == 2) c = k.shirt2; break;                           // croix (style nordique)
        case KP_DIAGHALF: if (x > (y % 4) * 2) c = k.shirt2; break;                             // moitiés en diagonale
        }
        return hexc(c);
    }
    case 'P': return hexc(gk ? 0x222222 : k.shorts);
    case 'k': return hexc(gk ? gkShirt : k.socks);
    case 'b': { static const unsigned BOOT[5] = { 0x111111, 0x111111, 0xF2F2F2, 0xE8B830, 0xD83A2A }; return hexc(BOOT[g_bootCol % 5]); }
    case 'g': return hexc(0xD8F060);                     // gants du gardien
    default: return BLANK;
    }
}

static bool g_sprLongHair = false;      // joueuse / arbitre : cheveux longs (queue de cheval)
// poses particulières (bancs, célébrations) : modifient le sprite debout
static int g_sprPose = POSE_NONE;
// dir : 0 bas, 1 haut, 2 droite, 3 gauche
void drawPlayerSprite(int x, int y, const Kit& kit, int skin, int hair, int dir, int frame, int state, bool gk, unsigned gkShirt, int scale) {
    // (x,y) = pieds
    if (state == PS_BIKE) state = PS_DIVE;          // retourné acrobatique : corps à l'horizontale en l'air
    if (state == PS_SLIDE || state == PS_DOWN || state == PS_DIVE) {
        bool flip = dir == 3;
        bool vertical = dir == 0 || dir == 1;
        const char* const* LY = state == PS_DIVE && !vertical ? SPR_DIVE : SPR_LYING;
        if (state == PS_DIVE && LY == SPR_LYING) flip = !flip;
        for (int r = 0; r < 5; r++) for (int c = 0; c < 12; c++) {
            char ch = LY[r][c];
            if (ch == '.' || ch == ' ') continue;
            if (gk && state == PS_DIVE && ch == 's' && c >= 10) ch = 'g';     // gants
            int cc = flip ? 11 - c : c;
            Color col = pixelColor(ch, c % 7, r, kit, skin, hair, gk, gkShirt);
            int px, py;
            if (vertical) { px = x + (r - 2) * scale; py = y - 6 * scale + (dir == 0 ? cc : 11 - cc) * scale; }
            else { px = x + (cc - 6) * scale; py = y - 4 * scale + r * scale; }
            DrawRectangle(px, py, scale, scale, col);
        }
        return;
    }
    const char* const* src;
    bool mirror = false;
    int f = ((frame % 4) + 4) % 4;
    // léger rebond du corps pendant la course

    if (g_sprPose == POSE_ARMSOUT) src = SPR_CELEB2;
    else if (g_sprPose == POSE_CRADLE) src = SPR_HOLD;
    else if (g_sprPose == POSE_KNEEL) src = SPR_CELEB;
    else if (state == PS_CELEB) src = (frame / 3) % 2 ? SPR_CELEB2 : SPR_CELEB;
    else if (state == PS_THROW) src = SPR_CELEB;
    else if (state == PS_GKHOLD) src = SPR_HOLD;
    else if (state == PS_HEAD) src = SPR_HEAD;
    else if (state == PS_KICK) { if (dir == 0) src = SPR_KICK_DOWN; else if (dir == 1) src = SPR_KICK_UP; else { src = SPR_KICK_RIGHT; mirror = dir == 3; } }
    else if (dir == 0) src = SPR_DOWN[f];
    else if (dir == 1) src = SPR_UP[f];
    else { src = SPR_RIGHT[f]; mirror = dir == 3; }
    // coiffure : courte, rasée ou longue (déterminée par le joueur)
    char buf[13][8] = {};
    for (int r = 0; r < 12; r++) { strncpy(buf[r], src[r], 7); buf[r][7] = 0; }
    int style = (hair * 7 + skin * 3) % 4;
    if (style == 1 && dir != 1) { for (int c = 0; c < 7; c++) if (buf[0][c] == 'h') buf[0][c] = (c == 2 || c == 4) ? 's' : 'h'; }
    else if (style == 2) { if (buf[2][1] == '.') buf[2][1] = 'h'; if (buf[2][5] == '.') buf[2][5] = 'h'; }
    else if (style == 3 && dir == 1) { buf[3][2] = 'h'; buf[3][3] = 'h'; buf[3][4] = 'h'; }
    if (g_sprLongHair && state != PS_CELEB) {
        auto H = [&](int r, int c) { if (buf[r][c] == '.' || buf[r][c] == ' ' || buf[r][c] == 0) buf[r][c] = 'h'; };
        if (dir == 1) { buf[3][2] = buf[3][3] = buf[3][4] = 'h'; buf[4][3] = 'h'; }
        else if (dir == 0) { H(2, 1); H(2, 5); H(3, 1); H(3, 5); }
        else { H(1, 1); H(2, 1); H(3, 1); H(3, 2); }
    }
    // gardien : gants (mains)
    if (gk) for (int r = 6; r < 9; r++) for (int c = 0; c < 7; c++) if (buf[r][c] == 's') buf[r][c] = 'g';
    if (state == PS_CELEB && gk) for (int r = 0; r < 3; r++) for (int c = 0; c < 7; c++) if ((c == 0 || c == 6) && buf[r][c] == 's') buf[r][c] = 'g';
    g_bootCol = (skin * 5 + hair * 3) % 5;
    int NR = 12;
    // poses : assis (bancs), à genoux, « chut », mains sur la tête, bras tendu, applaudissements
    bool front = dir == 0;
    if (g_sprPose == POSE_SIT || g_sprPose == POSE_KNEEL) { strcpy(buf[9], g_sprPose == POSE_SIT ? ".kk.kk." : "bkk.kkb"); NR = 10; }
    else if (g_sprPose == POSE_SHUSH && front) { buf[3][4] = 's'; buf[4][5] = 's'; buf[5][6] = '.'; buf[6][6] = '.'; }
    else if (g_sprPose == POSE_HEAD && (front || dir == 1)) { buf[0][0] = buf[0][6] = buf[1][0] = buf[1][6] = 's'; buf[2][0] = buf[2][6] = 'S'; buf[5][0] = buf[5][6] = buf[6][0] = buf[6][6] = '.'; }
    else if (g_sprPose == POSE_POINT) { int c = dir == 3 ? 0 : 6; buf[1][c] = 's'; buf[2][c] = 's'; buf[3][c] = 'S'; buf[5][c] = '.'; buf[6][c] = '.'; }
    else if (g_sprPose == POSE_CLAP && front) { buf[5][0] = buf[5][6] = '.'; buf[6][0] = buf[6][6] = '.'; buf[4][(frame & 1) ? 2 : 3] = 's'; buf[5][(frame & 1) ? 4 : 3] = 's'; }
    // course : le haut du corps rebondit d'un pixel pendant les foulées
    bool bob = state == PS_NORMAL && (f == 1 || f == 3) && scale == 1 && g_sprPose == POSE_NONE;
    if (bob) { for (int r = 12; r >= 10; r--) memcpy(buf[r], buf[r - 1], 8); NR = 13; }     // jambes en extension : corps relevé d'un pixel
    const char* spr[13]; for (int r = 0; r < 13; r++) spr[r] = buf[r];
    auto filled = [&](int r, int c) {
        if (r < 0 || r >= NR || c < 0 || c >= 7) return false;
        const char* row = spr[r];
        if (c >= (int)strlen(row)) return false;
        return row[c] != '.' && row[c] != ' ';
    };
    // contour sombre (style 16 bits)
    Color outline = Color{ 10, 10, 20, 110 };
    for (int r = 0; r < NR; r++) for (int c = -1; c <= 7; c++) {
        if (filled(r, c)) continue;
        if (!(filled(r, c - 1) || filled(r, c + 1) || filled(r - 1, c) || filled(r + 1, c))) continue;
        int cc = mirror ? 6 - c : c;
        DrawRectangle(x + (cc - 3) * scale, y - (NR - r) * scale, scale, scale, outline);
    }
    for (int r = 0; r < NR; r++) {
        const char* row = spr[r];
        int n = (int)strlen(row);
        for (int c = 0; c < n && c < 7; c++) {
            char ch = row[c];
            if (ch == '.' || ch == ' ') continue;
            int cc = mirror ? 6 - c : c;
            Color col = pixelColor(ch, c, r, kit, skin, hair, gk, gkShirt);
            // ombrage : côté droit (lumière venant de la gauche) et bas du maillot
            bool shade = (ch == 'S' || ch == 'P' || ch == 'k') && (!filled(r, c + 1) || (ch == 'S' && !filled(r + 1, c)));
            if (mirror) shade = (ch == 'S' || ch == 'P' || ch == 'k') && (!filled(r, c - 1) || (ch == 'S' && !filled(r + 1, c)));
            auto lift = [&](int v) { col.r = (unsigned char)std::min(255, col.r + v); col.g = (unsigned char)std::min(255, col.g + v); col.b = (unsigned char)std::min(255, col.b + v); };
            // revers des manches dans la 2e couleur du maillot (maillots unis)
            if (ch == 'S' && !gk && r == 4 && (c == 0 || c == 6) && kit.shirt2 != kit.shirt && kit.shirt2 != 0 && kit.pattern == KP_PLAIN && state != PS_CELEB) col = hexc(kit.shirt2);
            if (shade) { col.r = (unsigned char)(col.r * 0.72f); col.g = (unsigned char)(col.g * 0.72f); col.b = (unsigned char)(col.b * 0.72f); }
            else if (ch == 'S' && r == 3 && scale >= 2) lift(25);
            else {
                // lumière venant de la gauche : reflet sur le bord éclairé du maillot, du short et dans les cheveux
                bool litEdge = mirror ? !filled(r, c + 1) : !filled(r, c - 1);
                if ((ch == 'S' || ch == 'P') && litEdge) lift(22);
                else if (ch == 'h' && (r == 0 || litEdge)) lift(30);
                else if (ch == 's' && r <= 2 && litEdge) lift(14);
                else if (ch == 'b' && litEdge) lift(60);                                  // reflet sur les chaussures
            }
            DrawRectangle(x + (cc - 3) * scale, y - (NR - r) * scale, scale, scale, col);
        }
    }
}

void drawPosedSprite(int x, int y, const Kit& kit, int skin, int hair, int dir, int frame, int state, int pose, int scale) {
    g_sprPose = pose;
    drawPlayerSprite(x, y, kit, skin, hair, dir, frame, state, false, 0, scale);
    g_sprPose = POSE_NONE;
}

void drawKitIcon(const Kit& k, int x, int y, int scale) {
    // petit maillot
    static const char* SH[8] = { ".SS..SS.", "SSSSSSSS", "SSSSSSSS", ".SSSSSS.", ".SSSSSS.", ".SSSSSS.", ".PPPPPP.", ".PP..PP." };
    for (int r = 0; r < 8; r++) for (int c = 0; c < 8; c++) {
        char ch = SH[r][c];
        if (ch == '.') continue;
        Color col = pixelColor(ch, c, r, k, 0, 0, false, 0);
        DrawRectangle(x + c * scale, y + r * scale, scale, scale, col);
    }
}

static int dirOf(V2 f) {
    if (std::fabs(f.x) > std::fabs(f.y) * 1.1f) return f.x > 0 ? 2 : 3;
    return f.y > 0 ? 0 : 1;
}

// ------------------------------------------------------------------ rendu du match
struct DrawItem { float y; int kind; int idx; };

static void drawGoal(float gy, bool top, int ox, int oy) {
    float x1 = (PITCH_W / 2 - GOAL_W / 2 + MARGIN) * PPM - ox, x2 = (PITCH_W / 2 + GOAL_W / 2 + MARGIN) * PPM - ox;
    float y0 = (gy + MARGIN) * PPM - oy;
    float h = GOAL_H * PPM * HZ;
    float d = GOAL_DEPTH * PPM * (top ? -1 : 1);
    Color net = Color{ 235, 235, 235, 150 };
    Color post = Color{ 250, 250, 250, 255 };
    if (top) {
        // filet (derrière la ligne), vu de dessus
        for (float yy = y0 + d - h * 0.6f; yy <= y0 - h + 1; yy += 2) DrawLine((int)x1, (int)yy, (int)x2, (int)yy, net);
        for (float xx = x1; xx <= x2; xx += 2) DrawLine((int)xx, (int)(y0 + d - h * 0.6f), (int)xx, (int)(y0 - h), net);
        DrawRectangle((int)x1 - 1, (int)(y0 - h), 2, (int)h + 1, post);
        DrawRectangle((int)x2 - 1, (int)(y0 - h), 2, (int)h + 1, post);
        DrawRectangle((int)x1 - 1, (int)(y0 - h) - 1, (int)(x2 - x1) + 2, 2, post);
    } else {
        for (float yy = y0 - h; yy <= y0 + d - h * 0.6f; yy += 2) DrawLine((int)x1, (int)yy, (int)x2, (int)yy, net);
        for (float xx = x1; xx <= x2; xx += 2) DrawLine((int)xx, (int)(y0 - h), (int)xx, (int)(y0 + d - h * 0.6f), net);
        DrawRectangle((int)x1 - 1, (int)(y0 - h), 2, (int)h + 1, post);
        DrawRectangle((int)x2 - 1, (int)(y0 - h), 2, (int)h + 1, post);
        DrawRectangle((int)x1 - 1, (int)(y0 - h) - 1, (int)(x2 - x1) + 2, 2, post);
    }
}

static const Color CTRL_COL[NUM_INPUTS] = { { 255, 230, 40, 255 }, { 60, 220, 255, 255 }, { 255, 90, 200, 255 }, { 110, 255, 110, 255 }, { 255, 150, 40, 255 }, { 200, 160, 255, 255 } };


// ------------------------------------------------------------------ tirage au sort : pièce, ballon, terrain (écran de choix)
static void drawCoin(int cx, int cy, float r, float w, int face) {
    // pièce en or avec relief : tranche visible pendant la rotation, listel cannelé, dégradé métallique, reflet mobile
    // w : largeur apparente (0..1) ; face 0 = PILE (valeur), 1 = FACE (effigie)
    float aw = std::fabs(w);
    int rx = std::max(1, (int)(r * aw));
    Color edgeD = { 120, 84, 16, 255 }, dark = { 150, 108, 22, 255 }, mid = { 214, 168, 48, 255 }, gold = { 238, 196, 70, 255 }, light = { 255, 236, 150, 255 };
    // ombre portée
    DrawEllipse(cx + 2, cy + 3, (float)rx + 1, r, Color{ 0, 0, 0, 80 });
    // tranche (épaisseur) : visible quand la pièce est presque de profil
    float thick = std::max(0.f, r * 0.16f * (1.f - aw));
    if (thick >= 1.f) {
        // tranche : ellipses décalées (épouse la forme de la pièce, pas de barre qui dépasse)
        int tw = (int)thick + 1;
        for (int o = tw; o >= 1; o--) DrawEllipse(cx - o, cy, (float)rx, r, (o & 1) ? edgeD : dark);
    }
    // disque : dégradé concentrique
    DrawEllipse(cx, cy, (float)rx, r, edgeD);
    DrawEllipse(cx, cy, (float)std::max(1, rx - 1), r - 1, dark);
    DrawEllipse(cx, cy, (float)std::max(1, rx - 2), r - 2, mid);
    DrawEllipse(cx - (int)(r * 0.08f * aw), cy - (int)(r * 0.08f), (float)std::max(1, (int)((r - 4) * aw)), r - 4, gold);
    if (rx < r * 0.3f) return;
    float k = (float)rx / r;
    // listel cannelé
    for (int i = 0; i < 36; i++) {
        float a = i * 6.2832f / 36;
        int px = cx + (int)(std::cos(a) * (r - 2.5f) * k), py = cy + (int)(std::sin(a) * (r - 2.5f));
        DrawPixel(px, py, (i & 1) ? light : dark);
    }
    DrawEllipseLines(cx, cy, std::max(1.f, (r - 5) * k), r - 5, dark);
    if (face == 0) {
        // PILE : grand « 1 » en relief entouré de 12 étoiles
        int h = (int)(r * 0.8f);
        int sw = std::max(1, (int)(4 * k));
        DrawRectangle(cx - sw / 2 + 1, cy - h / 2 + 1, sw, h, dark);                     // ombre du relief
        DrawRectangle(cx - sw / 2, cy - h / 2, sw, h, light);
        DrawRectangle(cx - (int)(5 * k), cy - h / 2 + 2, std::max(1, (int)(5 * k)), 2, light);
        DrawRectangle(cx - (int)(6 * k), cy + h / 2 - 2, std::max(2, (int)(12 * k)), 2, light);
        DrawRectangle(cx - (int)(6 * k) + 1, cy + h / 2, std::max(2, (int)(12 * k)), 1, dark);
        for (int i = 0; i < 12; i++) {
            float a = i * 0.5236f - 1.5708f;
            int sx = cx + (int)(std::cos(a) * r * 0.68f * k), sy = cy + (int)(std::sin(a) * r * 0.68f);
            DrawPixel(sx, sy, light); DrawPixel(sx - 1, sy, mid); DrawPixel(sx + 1, sy, mid); DrawPixel(sx, sy - 1, mid); DrawPixel(sx, sy + 1, mid);
        }
    } else {
        // FACE : effigie de profil tournée vers la droite (chevelure, visage, nez, œil, cou, épaules), couronne de laurier
        float s = r * 0.30f;
        int hx = cx - (int)(r * 0.04f * k), hy = cy - (int)(r * 0.18f);
        DrawEllipse(hx - (int)(s * 0.15f * k) + 1, hy + 1, s * 1.05f * k + 1, s * 1.05f, dark);                 // ombre du relief
        DrawEllipse(hx - (int)(s * 0.15f * k), hy, s * 1.05f * k + 1, s * 1.05f, mid);                          // chevelure (arrière)
        DrawEllipse(hx + (int)(s * 0.22f * k), hy + (int)(s * 0.15f), s * 0.72f * k + 1, s * 0.88f, light);     // visage
        DrawTriangle(Vector2{ (float)hx + s * 0.9f * k, (float)hy + s * 0.05f }, Vector2{ (float)hx + s * 0.9f * k, (float)hy + s * 0.45f },
                     Vector2{ (float)hx + s * 1.25f * k, (float)hy + s * 0.38f }, light);                                   // nez
        DrawPixel(hx + (int)(s * 0.55f * k), hy - (int)(s * 0.05f), dark);                                                // œil
        DrawRectangle(hx - (int)(s * 0.05f * k), hy + (int)(s * 0.85f), std::max(2, (int)(s * 0.6f * k)), (int)(s * 0.7f), light);   // cou
        DrawTriangle(Vector2{ (float)hx - s * 1.1f * k, (float)hy + s * 2.05f }, Vector2{ (float)hx + s * 1.2f * k, (float)hy + s * 2.05f },
                     Vector2{ (float)hx + s * 0.25f * k, (float)hy + s * 1.35f }, light);                                   // épaules
        DrawRectangle(hx - (int)(s * 1.1f * k), hy + (int)(s * 2.05f), std::max(2, (int)(s * 2.3f * k)), 1, dark);
        for (int i = 0; i < 7; i++) {   // laurier
            float a = 2.2f + i * 0.28f;
            DrawPixel(cx + (int)(std::cos(a) * r * 0.66f * k), cy + (int)(std::sin(a) * r * 0.66f), light);
            DrawPixel(cx - (int)(std::cos(a) * r * 0.66f * k), cy + (int)(std::sin(a) * r * 0.66f), light);
        }
    }
    // reflet qui glisse sur la pièce
    float gl = std::fmod((float)GetTime() * 0.7f, 1.6f) - 0.3f;
    if (gl > 0 && gl < 1) {
        int gx = cx - rx + (int)(gl * 2 * rx);
        for (int d = -2; d <= 2; d++) {
            float yy = 1.f - std::pow((gx + d - cx) / (float)std::max(1, rx), 2.f);
            if (yy <= 0) continue;
            int hh = (int)(r * std::sqrt(yy) * 0.85f);
            DrawRectangle(gx + d, cy - hh, 1, hh * 2, Color{ 255, 255, 230, (unsigned char)(90 - std::abs(d) * 30) });
        }
    }
}
static void drawMiniBall(int cx, int cy, int r) {
    DrawCircle(cx + 1, cy + 2, (float)r, Color{ 0, 0, 0, 80 });
    DrawCircle(cx, cy, (float)r, WHITE);
    DrawCircleLines(cx, cy, (float)r, Color{ 40, 40, 40, 255 });
    DrawPoly(Vector2{ (float)cx, (float)cy }, 5, r * 0.35f, 0, Color{ 30, 30, 30, 255 });
    for (int i = 0; i < 5; i++) { float a = i * 1.2566f - 1.57f; DrawPoly(Vector2{ cx + std::cos(a) * r * 0.78f, cy + std::sin(a) * r * 0.78f }, 5, r * 0.2f, a * 57.3f, Color{ 30, 30, 30, 255 }); }
}
static void drawMiniPitch(int x, int y, int w, int h, int arrow, Color kitc) {
    // arrow : 0 aucune, 1 flèche vers le haut, 2 vers le bas, 3 double flèche
    for (int k = 0; k < 6; k++) DrawRectangle(x, y + k * h / 6, w, h / 6 + 1, (k % 2) ? Color{ 46, 130, 46, 255 } : Color{ 56, 146, 56, 255 });
    Color L = { 235, 235, 235, 255 };
    DrawRectangleLines(x, y, w, h, L);
    DrawLine(x, y + h / 2, x + w, y + h / 2, L);
    DrawCircleLines(x + w / 2, y + h / 2, (float)w / 6, L);
    DrawRectangleLines(x + w / 4, y, w / 2, h / 7, L); DrawRectangleLines(x + w / 4, y + h - h / 7, w / 2, h / 7, L);
    auto arrowUp = [&](int ax, int ay, int len) { DrawRectangle(ax - 2, ay - len + 6, 5, len - 6, kitc); DrawTriangle(Vector2{ (float)ax, (float)(ay - len - 2) }, Vector2{ (float)ax - 7, (float)(ay - len + 7) }, Vector2{ (float)ax + 7, (float)(ay - len + 7) }, kitc); };
    auto arrowDn = [&](int ax, int ay, int len) { DrawRectangle(ax - 2, ay, 5, len - 6, kitc); DrawTriangle(Vector2{ (float)ax, (float)(ay + len + 2) }, Vector2{ (float)ax + 7, (float)(ay + len - 7) }, Vector2{ (float)ax - 7, (float)(ay + len - 7) }, kitc); };
    if (arrow == 1) { DrawRectangle(x + 1, y + 1, w - 2, h / 2 - 1, Color{ 255, 255, 255, 40 }); arrowUp(x + w / 2, y + h * 3 / 4, h / 2); }
    if (arrow == 2) { DrawRectangle(x + 1, y + h / 2, w - 2, h / 2 - 1, Color{ 255, 255, 255, 40 }); arrowDn(x + w / 2, y + h / 4, h / 2); }
    if (arrow == 3) { arrowUp(x + w / 2 - 8, y + h / 2 + 4, h / 3); arrowDn(x + w / 2 + 8, y + h / 2 - 4, h / 3); }
}
// séance de tirs au but : écarter des joueurs, liste des 5 tireurs, tireur suivant (6e à 11e)
static void drawShootUI(const Match& m) {
    if (m.shootUI <= 0 || m.shootUITeam < 0) return;
    int t = m.shootUITeam;
    std::vector<int> ch = m.shootChoices();
    int pw = 250, rows = (int)ch.size(), ph = 30 + rows * 10 + 4, px = MW / 2 - pw / 2, py = std::max(14, MH / 2 - ph / 2 - 6);
    DrawRectangle(px + 3, py + 3, pw, ph, Color{ 0, 0, 0, 120 });
    DrawRectangle(px, py, pw, ph, Color{ 14, 22, 48, 240 });
    DrawRectangleLines(px, py, pw, ph, Color{ 240, 200, 60, 255 });
    DrawRectangle(px, py, pw, 13, hexc(m.kit[t].shirt));
    std::string title = m.shootUI == 1 ? fmt("%s : ÉCARTER %d JOUEUR%s", m.team(t).shortName.c_str(), m.shootExcl[t], m.shootExcl[t] > 1 ? "S" : "")
                      : m.shootUI == 2 ? fmt("%s : TIREUR N°%d (liste des 5)", m.team(t).shortName.c_str(), (int)m.shootList[t].size() + 1)
                      : fmt("%s : TIREUR SUIVANT (n°%d)", m.team(t).shortName.c_str(), m.penTaken[t] + 1);
    int tw = textWidth(title, 10);
    DrawRectangle(px + pw / 2 - tw / 2 - 3, py + 1, tw + 6, 11, Color{ 0, 0, 0, 150 });
    drawTextPx(title, px + pw / 2 - tw / 2, py + 1, 10, WHITE);
    std::string sub = m.shootUI == 1 ? "Supériorité numérique : même nombre de tireurs" : m.shootUI == 2 ? "Ordre des 5 premiers tireurs" : "Joueurs qui n'ont pas encore tiré";
    drawTextPx(sub, px + 6, py + 15, 10, Color{ 170, 185, 220, 255 });
    for (int k = 0; k < rows; k++) {
        int i = ch[k], y = py + 27 + k * 10;
        bool on = k == m.shootSel;
        if (on) DrawRectangle(px + 3, y, pw - 6, 10, Color{ 240, 200, 60, 255 });
        Color c = on ? BLACK : WHITE;
        drawTextPx(fitText(m.playerName(i) + (m.pl[i].gk ? " (G)" : ""), pw - 60, 10), px + 8, y, 10, c);
        std::string r = fmt("tir %d", (int)m.pl[i].shoot);
        drawTextPx(r, px + pw - 8 - textWidth(r, 10), y, 10, on ? BLACK : Color{ 170, 185, 220, 255 });
    }
    if (m.shootUI == 2 && !m.shootList[t].empty()) {
        std::string l = "Liste :";
        for (size_t k = 0; k < m.shootList[t].size(); k++) l += fmt(" %d.", (int)k + 1) + m.playerName(m.shootList[t][k]);
        drawTextPx(fitText(l, MW - 8, 10), 4, std::min(MH - 22, py + ph + 3), 10, WHITE);
    }
    std::string h = "Haut/Bas : choisir    Tir : valider";
    drawTextPx(h, MW / 2 - textWidth(h, 10) / 2, MH - 11, 10, WHITE);
}

static void drawTossUI(const Match& m) {
    if ((!m.ceremony && m.tossKind == 0) || m.tossUI <= 0) return;
    float t = (float)GetTime();
    int pw = 236, ph = 118, px = MW / 2 - pw / 2, py = 30;
    int caller = 1, win = m.tossWinner;
    Color kc[2] = { hexc(m.kit[0].shirt), hexc(m.kit[1].shirt) };
    DrawRectangle(px + 3, py + 3, pw, ph, Color{ 0, 0, 0, 120 });
    DrawRectangle(px, py, pw, ph, Color{ 14, 22, 48, 235 });
    DrawRectangleLines(px, py, pw, ph, Color{ 240, 200, 60, 255 });
    DrawRectangle(px, py, pw, 13, Color{ 240, 200, 60, 255 });
    auto title = [&](const std::string& s) { int w = textWidth(s, 10); drawTextPx(s, px + pw / 2 - w / 2, py + 1, 10, BLACK); };
    auto card = [&](int i, int sel, const std::string& l1, const std::string& l2) {
        int cw = 100, ch = 86, cx = px + 12 + i * (cw + 12), cy = py + 18;
        bool on = sel == i;
        DrawRectangle(cx, cy, cw, ch, on ? Color{ 60, 70, 110, 255 } : Color{ 30, 40, 72, 255 });
        DrawRectangleLines(cx, cy, cw, ch, on ? Color{ 255, 225, 90, 255 } : Color{ 80, 100, 150, 255 });
        if (on) { DrawRectangleLines(cx - 1, cy - 1, cw + 2, ch + 2, Color{ 255, 225, 90, 160 }); int by = (int)(std::sin(t * 6) * 2); DrawTriangle(Vector2{ (float)cx + cw / 2 - 5, (float)cy - 7 + by }, Vector2{ (float)cx + cw / 2, (float)cy - 1 + by }, Vector2{ (float)cx + cw / 2 + 5, (float)cy - 7 + by }, Color{ 255, 225, 90, 255 }); }
        int w1 = textWidth(l1, 10), w2 = textWidth(l2, 10);
        drawTextPx(l1, cx + cw / 2 - w1 / 2, cy + ch - 22, 10, on ? Color{ 255, 225, 90, 255 } : WHITE);
        drawTextPx(l2, cx + cw / 2 - w2 / 2, cy + ch - 12, 10, Color{ 170, 185, 220, 255 });
        return V2((float)(cx + cw / 2), (float)(cy + 30));
    };
    auto hint = [&]() { std::string h = "< > : choisir    Tir : valider"; drawTextPx(h, px + pw / 2 - textWidth(h, 10) / 2, py + ph + 4, 10, WHITE); };
    switch (m.tossUI) {
    case 1: {
        title("PILE OU FACE ? - " + m.team(caller).shortName + " annonce");
        for (int i = 0; i < 2; i++) {
            V2 c = card(i, m.tossSel, i == 0 ? "PILE" : "FACE", i == 0 ? "le chiffre" : "le profil");
            float w = m.tossSel == i ? std::cos(t * 3.f) : 1.f;
            drawCoin((int)c.x, (int)c.y, 20, w < 0 ? -w : w, i);
        }
        hint();
        break;
    }
    case 2: {
        title("L'ARBITRE LANCE LA PIÈCE...");
        const float FLIP = 2.4f;
        float ft = std::min(m.cerT, FLIP);
        float hgt = ft < FLIP ? std::sin(ft / FLIP * 3.14159f) * 46.f : 0.f;
        float spin = ft < FLIP ? m.cerT * (22.f - ft * 6.f) : 0.f;
        float w = std::cos(spin);
        int face = ft < FLIP ? (w >= 0 ? 0 : 1) : m.tossResult;
        int cx = MW / 2, cy = py + 78 - (int)hgt;
        // rebond à l'atterrissage
        if (m.cerT > FLIP && m.cerT < FLIP + 0.35f) cy -= (int)(std::sin((m.cerT - FLIP) / 0.35f * 3.14159f) * 5);
        DrawEllipse(cx, py + 102, std::max(4.f, 18 - hgt * 0.25f), 3, Color{ 0, 0, 0, (unsigned char)std::max(30, 110 - (int)hgt) });
        if (ft < FLIP) for (int k = 1; k <= 3; k++) {    // traînée
            float t2 = std::max(0.f, ft - k * 0.04f);
            float h2 = std::sin(t2 / FLIP * 3.14159f) * 46.f;
            DrawEllipse(cx, py + 78 - (int)h2, 20.f * std::fabs(std::cos(t2 * (22.f - t2 * 6.f))) + 1, 22, Color{ 240, 200, 70, (unsigned char)(50 - k * 12) });
        }
        drawCoin(cx, cy, 24, ft < FLIP ? std::fabs(w) : 1.f, face);
        if (m.cerT > FLIP && m.cerT < FLIP + 0.6f) {   // éclat
            float e = (m.cerT - FLIP) / 0.6f;
            for (int i = 0; i < 8; i++) { float a = i * 0.785f; int l = (int)(10 + e * 14); DrawLine(cx + (int)(std::cos(a) * 28), cy + (int)(std::sin(a) * 28), cx + (int)(std::cos(a) * (28 + l)), cy + (int)(std::sin(a) * (28 + l)), Color{ 255, 230, 120, (unsigned char)(200 * (1 - e)) }); }
        }
        std::string call = std::string("Annonce : ") + (m.tossCall == 0 ? "PILE" : "FACE");
        drawTextPx(call, px + 8, py + 18, 10, Color{ 170, 185, 220, 255 });
        if (m.cerT > FLIP) {
            std::string r = m.tossResult == 0 ? "PILE !" : "FACE !";
            drawTextPx(r, cx + 35, py + 69, 20, BLACK);
            drawTextPx(r, cx + 34, py + 68, 20, Color{ 255, 225, 90, 255 });
            if (win >= 0) {
                std::string g = m.team(win).name + " gagne le tirage";
                int gw = std::min(pw - 20, textWidth(g, 10)), gx = px + pw / 2 - gw / 2 + 4;
                DrawRectangle(gx - 8, py + ph - 13, 5, 9, kc[win]);
                drawTextPx(fitText(g, pw - 20, 10), gx, py + ph - 14, 10, WHITE);
            }
        }
        break;
    }
    case 3: {
        title(m.team(win >= 0 ? win : 0).shortName + " A GAGNÉ : VOTRE CHOIX");
        if (m.tossKind == 2) {
            V2 a0 = card(0, m.tossSel, "EN PREMIER", "tirer le 1er");
            drawMiniBall((int)a0.x, (int)a0.y, 16 + (m.tossSel == 0 ? (int)(std::fabs(std::sin(t * 5)) * 2) : 0));
            V2 a1 = card(1, m.tossSel, "EN SECOND", "tirer le 2e");
            drawMiniBall((int)a1.x - 8, (int)a1.y, 10); drawMiniBall((int)a1.x + 8, (int)a1.y, 10);
            hint();
            break;
        }
        V2 c0 = card(0, m.tossSel, "LE BALLON", "coup d'envoi");
        drawMiniBall((int)c0.x, (int)c0.y, 16 + (m.tossSel == 0 ? (int)(std::fabs(std::sin(t * 5)) * 2) : 0));
        V2 c1 = card(1, m.tossSel, "LE CÔTÉ", "choisir le sens");
        drawMiniPitch((int)c1.x - 22, (int)c1.y - 26, 44, 54, 3, kc[win >= 0 ? win : 0]);
        hint();
        break;
    }
    case 4: {
        int st = m.tossSideTeam >= 0 ? m.tossSideTeam : (win >= 0 ? win : 0);
        title(m.team(st).shortName + " : DANS QUEL SENS ATTAQUER ?");
        V2 c0 = card(0, m.tossSel, "VERS LE HAUT", "1re période");
        drawMiniPitch((int)c0.x - 22, (int)c0.y - 26, 44, 54, 1, kc[st]);
        V2 c1 = card(1, m.tossSel, "VERS LE BAS", "1re période");
        drawMiniPitch((int)c1.x - 22, (int)c1.y - 26, 44, 54, 2, kc[st]);
        hint();
        break;
    }
    case 6: {   // l'adversaire (ordinateur) réfléchit au choix du côté
        int st = m.tossSideTeam >= 0 ? m.tossSideTeam : 0;
        title("CHOIX DU CÔTÉ");
        std::string a = m.team(st).name, b = "choisit son côté...";
        drawMiniPitch(MW / 2 - 22, py + 22, 44, 54, ((int)(t * 2)) % 2 ? 1 : 2, kc[st]);
        drawTextPx(fitText(a, pw - 10, 10), px + pw / 2 - std::min(pw - 10, textWidth(a, 10)) / 2, py + ph - 26, 10, Color{ 255, 225, 90, 255 });
        drawTextPx(b, px + pw / 2 - textWidth(b, 10) / 2, py + ph - 14, 10, WHITE);
        break;
    }
    case 5: {
        title(m.tossKind == 1 ? "TIRAGE AU SORT - PROLONGATION" : m.tossKind == 2 ? "TIRAGE AU SORT - TIRS AU BUT" : "TIRAGE AU SORT");
        if (win >= 0 && m.tossKind == 2) {
            std::string a = m.team(m.shootTeam).name + " tire en premier", b = std::string("Tirs dans le but ") + (m.shootGoal == 0 ? "du haut" : "du bas");
            drawMiniPitch(MW / 2 - 22, py + 22, 44, 54, m.shootGoal == 0 ? 1 : 2, kc[m.shootTeam]);
            drawTextPx(fitText(a, pw - 10, 10), px + pw / 2 - std::min(pw - 10, textWidth(a, 10)) / 2, py + ph - 26, 10, Color{ 255, 225, 90, 255 });
            drawTextPx(b, px + pw / 2 - textWidth(b, 10) / 2, py + ph - 14, 10, WHITE);
        } else if (win >= 0 && m.tossKind == 1) {
            std::string a = m.team(win).name + (m.tossChoice == 0 ? " : le ballon" : " : le côté"), b = "Coup d'envoi de la prolongation : " + m.team(m.etKickoff >= 0 ? m.etKickoff : 0).shortName;
            drawMiniBall(MW / 2, py + 50, 14);
            drawTextPx(fitText(a, pw - 10, 10), px + pw / 2 - std::min(pw - 10, textWidth(a, 10)) / 2, py + ph - 26, 10, Color{ 255, 225, 90, 255 });
            drawTextPx(fitText(b, pw - 10, 10), px + pw / 2 - std::min(pw - 10, textWidth(b, 10)) / 2, py + ph - 14, 10, WHITE);
        } else if (win >= 0) {
            int st = m.tossSideTeam >= 0 ? m.tossSideTeam : win;
            std::string a = m.team(win).name + (m.tossChoice == 0 ? " : le ballon" : " : le côté"), b;
            b = m.team(st).shortName + " attaque " + std::string(m.tossDir == 0 ? "vers le haut" : "vers le bas") + ", engagement " + m.team(m.firstKickoff).shortName;
            if (m.tossChoice == 0) drawMiniBall(MW / 2 - 30, py + 50, 14);
            drawMiniPitch(MW / 2 + (m.tossChoice == 0 ? 0 : -22), py + 22, 44, 54, m.tossDir == 0 ? 1 : 2, kc[st]);
            drawTextPx(fitText(a, pw - 10, 10), px + pw / 2 - std::min(pw - 10, textWidth(a, 10)) / 2, py + ph - 26, 10, Color{ 255, 225, 90, 255 });
            drawTextPx(fitText(b, pw - 10, 10), px + pw / 2 - std::min(pw - 10, textWidth(b, 10)) / 2, py + ph - 14, 10, WHITE);
        }
        break;
    }
    }
}


// trophée (x,y = pied du socle) : 0 coupe, 1 « grandes oreilles », 2 trophée de championnat
static void drawTrophy(int x, int y, int style, int s) {
    Color gold{ 255, 205, 60, 255 }, gold2{ 200, 140, 30, 255 }, silv{ 225, 230, 240, 255 }, silv2{ 150, 158, 175, 255 }, base{ 50, 40, 36, 255 };
    auto R = [&](int dx, int dy, int w, int h, Color c) { DrawRectangle(x + dx * s, y + dy * s, w * s, h * s, c); };
    R(-3, -2, 6, 2, base);
    if (style == 1) {
        Color a = silv, b = silv2;
        R(-1, -4, 2, 2, b); R(-2, -9, 4, 5, a); R(-1, -11, 2, 2, a); R(1, -9, 1, 5, b);
        R(-5, -10, 1, 5, a); R(-4, -10, 2, 1, a); R(-4, -6, 2, 1, a);        // oreilles
        R(4, -10, 1, 5, a); R(2, -10, 2, 1, a); R(2, -6, 2, 1, a);
    } else if (style == 2) {
        R(-2, -3, 4, 1, silv2); R(-1, -10, 3, 7, gold); R(1, -10, 1, 7, gold2); R(-2, -11, 5, 1, silv); R(-1, -13, 3, 2, silv);
        R(-3, -8, 1, 3, silv); R(3, -8, 1, 3, silv);
    } else {
        R(-1, -4, 2, 2, gold2); R(-3, -8, 6, 4, gold); R(1, -8, 2, 4, gold2); R(-3, -9, 6, 1, Color{ 255, 240, 150, 255 });
        R(-4, -8, 1, 3, gold); R(3, -8, 1, 3, gold);
    }
}

// ------------------------------------------------------------------ stade vivant
static uint32_t mix32(uint32_t h) { h ^= h >> 13; h *= 0x5bd1e995; h ^= h >> 15; return h; }
// excitation du public de chaque camp (0 calme ... 1 explosion de joie), selon le score, l'action et les chants
static float crowdExcite(const Match& m, int side, int chant) {
    if (m.S.training) return 0;
    if (m.state == MS_GOAL || (m.state == MS_REPLAY && m.lastScorerTeam >= 0)) return m.lastScorerTeam == side ? 1.f : 0.f;
    if (m.trophyActive || m.lapActive) return (m.trophyActive ? m.trophyTeam : m.lapTeam) == side ? 0.9f : 0.05f;
    if (m.finished) { int d = m.score[side] - m.score[1 - side]; return d > 0 ? 0.8f : 0.03f; }
    float e = 0.12f + m.S.supporterAtmosphere * 0.002f;
    if (side == 0) {
        if (chant == CH_CELEBRATE) e = 0.6f; else if (chant == CH_LOUD) e = 0.4f; else if (chant == CH_ENCOURAGE) e = 0.32f;
        else if (chant == CH_TENSE) e = 0.05f; else if (chant == CH_WHISTLES || chant == CH_PROTEST) e = 0.02f;
    } else e *= 0.8f + 0.1f * m.S.supporterVisitors / 1000.f;
    // occasion : le public se lève quand son équipe approche du but adverse
    if (m.state == MS_PLAY && m.possTeam == side && m.progress(side, m.ball.pos) > 0.78f) e = std::max(e, 0.5f);
    return std::min(1.f, e);
}

// la ola : quand l'ambiance est bonne, une vague fait le tour du stade environ toutes les 19 minutes de jeu
float crowdWavePhase(const Match& m) {
    if (m.S.training || m.S.supporterAtmosphere < 45 || g_look.fill < 0.25f || m.finished || m.trophyActive) return -1;
    if (m.state != MS_PLAY && m.state != MS_SETPIECE && m.state != MS_STOP) return -1;
    if (m.clock < 10.f) return -1;
    float c = std::fmod(m.clock + 7.f, 19.f);
    return c < 2.2f ? c / 2.2f : -1.f;
}
// position d'un spectateur sur le tour du stade (0..1, sens horaire depuis le coin haut gauche)
static float perimeterPos(float wx, float wy) {
    const float W = PITCH_W, L = PITCH_L, P = 2 * (W + L);
    if (wy < 0 && wx >= 0 && wx <= W) return wx / P;
    if (wx > W) return (W + std::max(0.f, std::min(L, wy))) / P;
    if (wy > L) return (W + L + (W - std::max(0.f, std::min(W, wx)))) / P;
    return (2 * W + L + (L - std::max(0.f, std::min(L, wy)))) / P;
}

// public animé par-dessus les tribunes précalculées : supporters qui sautent, écharpes, drapeaux, parcage visiteur, fumigènes
static void drawCrowdLive(const Match& m, int ox, int oy, int MWZ, int MHZ) {
    if (m.S.training || g_look.fill <= 0.01f) return;
    const StadiumLook& L0 = g_look;
    const float edge = 4.0f + 2.2f;
    const float t = (float)GetTime();
    int chant = supportersLiveChant(m.S, m.score[0], m.score[1], m.clock);
    float ex[2] = { crowdExcite(m, 0, chant), crowdExcite(m, 1, chant) };
    Color hk1 = hexc(m.kit[0].shirt), hk2 = hexc(m.kit[0].shirt2 != m.kit[0].shirt ? m.kit[0].shirt2 : m.kit[0].shorts);
    Color ak1 = hexc(m.kit[1].shirt), ak2 = hexc(m.kit[1].shirt2 != m.kit[1].shirt ? m.kit[1].shirt2 : m.kit[1].shorts);
    // parcage visiteur : coin de la tribune sud (bas, côté droit), proportionnel aux supporters adverses
    float vf = m.S.attendance > 0 ? std::min(0.3f, std::max(m.S.neutral ? 0.45f : 0.04f, m.S.supporterVisitors / (float)std::max(1, m.S.attendance))) : 0.08f;
    if (m.S.neutral) vf = 0.5f;
    float awayX0 = PITCH_W + edge - (PITCH_W + 2 * edge) * vf;
    bool whistles = chant == CH_WHISTLES || chant == CH_PROTEST;
    float wave = crowdWavePhase(m);
    int x0 = std::max(0, ox) / 3 * 3, y0 = std::max(0, oy) / 3 * 3;
    for (int Y = y0; Y < oy + MHZ + 3; Y += 3) for (int X = x0; X < ox + MWZ + 3; X += 3) {
        float wx = X / PPM - MARGIN, wy = Y / PPM - MARGIN;
        bool left = wx < -edge, right = wx > PITCH_W + edge, top = wy < -edge, bot = wy > PITCH_L + edge;
        if (!(left || right || top || bot)) continue;
        int side; float dist;
        if (left && !(top || bot)) { side = 0; dist = -edge - wx; }
        else if (right && !(top || bot)) { side = 1; dist = wx - PITCH_W - edge; }
        else if (top && !(left || right)) { side = 2; dist = -edge - wy; }
        else if (bot && !(left || right)) { side = 3; dist = wy - PITCH_L - edge; }
        else continue;
        float depth = L0.depth[side];
        int kind = L0.kind[side];
        if (dist > depth || (kind == 2 && dist > depth - 2.2f)) continue;
        bool vert = side == 0 || side == 1;
        int row = vert ? X / 3 : Y / 3, col = vert ? Y / 3 : X / 3;
        uint32_t h = mix32((uint32_t)(row * 7919 + col * 104729));
        float f = L0.fill * (kind == 0 ? 0.6f : 1.f);
        bool away = side == 3 && wx > awayX0;
        bool buffer = side == 3 && !m.S.neutral && wx > awayX0 - 2.4f && wx <= awayX0;   // zone tampon vide entre les deux publics
        Color seat = kind == 0 ? ((row % 2) ? hexc(0x4A6B3A) : hexc(0x44633A)) : ((row % 2) ? hexc(0x3A3F58) : hexc(0x323650));
        int sx = X - ox, sy = Y - oy;
        if (buffer) { DrawRectangle(sx, sy, 2, 2, seat); if (col % 2 == 0) DrawRectangle(sx, sy + 2, 3, 1, hexc(0x9A9AA8)); continue; }
        bool occupied = (h % 1000) < (uint32_t)(f * 1000);
        if (away && !occupied && (h % 1000) < (uint32_t)(std::min(1.f, f + 0.25f) * 1000)) occupied = true;   // parcage plein
        if (!occupied) continue;
        int camp = away ? 1 : 0;
        float e = ex[camp];
        bool kop = (side == 2 && !m.S.neutral) || away;        // virage nord : kop des supporters locaux
        Color body = hexc(0x2B2D42);
        {
            static const unsigned CROWD[] = { 0x2B2D42, 0x8D99AE, 0xB23A48, 0xD9D9D9, 0x1D3557, 0xC98A4B, 0x3A3A3A, 0x6D597A, 0xC9A94F, 0x3F8F7A };
            body = hexc(CROWD[(h / 10) % 10]);
        }
        if (away) body = (h >> 5) % 3 ? ak1 : ak2;
        else if (kop && (h >> 5) % 3 != 0) body = (h >> 7) % 2 ? hk1 : hk2;
        // profils : ultras en noir dans le kop, torse nu par forte chaleur, familles en tenue claire dans les latérales
        if (kop && !away && (h >> 25) % 10 == 0) body = hexc(0x141414);
        else if (kop && m.S.meteo <= 1 && (h >> 25) % 7 == 1) body = hexc(SKIN[(h >> 3) % NUM_SKINS]);
        else if (!kop && !away && (h >> 25) % 9 == 2) body = hexc(0xE8E4D8);
        uint32_t ph = (h >> 11) & 63;
        bool active = ((h >> 17) % 100) < (uint32_t)(e * 100);
        int lift = 0;
        if (active) lift = std::sin(t * (7.f + (ph % 5)) + ph) > 0.f ? 1 : 0;
        bool inWave = false;
        if (wave >= 0) { float d = std::fabs(perimeterPos(wx, wy) - wave); d = std::min(d, 1.f - d); if (d < 0.035f) { inWave = true; active = true; e = 1.f; lift = d < 0.02f ? 2 : 1; } }
        bool flash = m.S.night && ((h >> 23) % 211) == (uint32_t)((int)(t * 3) % 211);   // flashs des téléphones en nocturne
        if (!active && !(kop && e > 0.25f) && !(whistles && camp == 0) && !flash) continue;   // rien à animer : le fond précalculé suffit
        Color skin = hexc(SKIN[(h >> 3) % NUM_SKINS]);
        DrawRectangle(sx, sy, 2, 2, seat);
        DrawRectangle(sx, sy - lift, 2, 2, body);
        if ((h % 3) == 0) DrawRectangle(sx, sy - lift, 2, 1, hexc(0x2A1E14));
        else DrawRectangle(sx, sy - lift, 2, 1, skin);
        // bras levés / écharpe tendue au-dessus de la tête
        if (active && e > 0.45f && ((h >> 9) % 2 || inWave)) { DrawRectangle(sx + ((h >> 13) & 1), sy - 1 - lift, 1, 1, skin); if (inWave) DrawRectangle(sx + 1 - ((h >> 13) & 1), sy - 1 - lift, 1, 1, skin); }
        if (flash) { DrawRectangle(sx, sy - 1, 2, 2, WHITE); DrawCircle(sx + 1, sy, 3, Color{ 255, 255, 255, 70 }); }
        if (kop && e > 0.25f && (h >> 19) % 4 == 0) {
            bool up = std::sin(t * 3.f + ph * 0.2f) > -0.2f;
            Color sc = away ? ((col % 2) ? ak1 : ak2) : ((col % 2) ? hk1 : hk2);
            if (up) DrawRectangle(sx - 1, sy - 2 - lift, 4, 1, sc);
        }
        // sifflets : mains portées à la bouche (pixel blanc qui clignote)
        if (whistles && camp == 0 && !kop && (h >> 21) % 3 == 0 && ((int)(t * 6) + ph) % 2) DrawRectangle(sx + 1, sy, 1, 1, Color{ 240, 240, 240, 255 });
    }
    // bâches accrochées devant le kop et le parcage, capo au mégaphone et tambours
    {
        auto SXW = [&](float x) { return (int)std::round((x + MARGIN) * PPM) - ox; };
        auto SYW = [&](float y) { return (int)std::round((y + MARGIN) * PPM) - oy; };
        if (!m.S.neutral && L0.depth[2] > 2.f) {
            int y0 = SYW(-edge - 0.2f) - 4;
            std::string ban = "ALLEZ " + m.team(0).shortName + " !";
            int bx0 = SXW(PITCH_W / 2 - 16.f), bx1 = SXW(PITCH_W / 2 + 16.f);
            if (y0 > -10 && y0 < MHZ + 10 && bx1 > -10 && bx0 < MWZ + 10) {
                DrawRectangle(bx0, y0, bx1 - bx0, 6, hk1); DrawRectangle(bx0, y0, bx1 - bx0, 1, hk2); DrawRectangle(bx0, y0 + 5, bx1 - bx0, 1, hk2);
                int tw = textWidth(ban, 5); drawTextPx(ban, (bx0 + bx1) / 2 - tw / 2, y0, 5, hk2);
                for (int k = 0; k < 2; k++) { int cx = SXW(PITCH_W / 2 + (k ? 18.5f : -18.5f)); DrawRectangle(cx - 6, y0, 12, 6, hk2); DrawRectangle(cx - 5, y0 + 1, 10, 4, hk1); }
                // capo dos au terrain, mégaphone levé ; deux tambours
                int cx = SXW(PITCH_W / 2), cy = SYW(-edge + 0.6f);
                Kit ck = m.kit[0];
                drawPosedSprite(cx, cy, ck, 2, 0, 1, (int)(t * 4), PS_NORMAL, ((int)(t * 2)) % 2 ? POSE_POINT : POSE_NONE, 1);
                for (int k = 0; k < 2; k++) {
                    int dx = SXW(PITCH_W / 2 + (k ? 3.f : -3.f)), dy = SYW(-edge + 0.5f);
                    int hit = ((int)(t * (ex[0] > 0.4f ? 8 : 3)) + k) % 2;
                    DrawEllipse(dx, dy - 2, 3, 2, Color{ 200, 40, 40, 255 }); DrawEllipse(dx, dy - 3, 3, 1, Color{ 240, 236, 220, 255 });
                    DrawLine(dx - 3, dy - 6 + hit * 2, dx - 1, dy - 3, Color{ 120, 80, 40, 255 });
                }
            }
        }
        if (!m.S.neutral) {   // bâche du parcage visiteur
            int y0 = SYW(PITCH_L + edge + 0.2f);
            int bx0 = SXW(awayX0 + 1.f), bx1 = SXW(PITCH_W + edge - 1.f);
            if (y0 > -10 && y0 < MHZ + 10 && bx1 - bx0 > 20) {
                DrawRectangle(bx0, y0, bx1 - bx0, 6, ak1); DrawRectangle(bx0, y0, bx1 - bx0, 1, ak2);
                std::string ban = m.team(1).shortName; int tw = textWidth(ban, 5); drawTextPx(ban, (bx0 + bx1) / 2 - tw / 2, y0, 5, ak2);
            }
        }
    }
    // grands drapeaux qui flottent dans les virages et le parcage
    for (int k = 0; k < 10; k++) {
        bool aw = k >= 7;
        if (aw && m.S.neutral) continue;
        float fx = aw ? awayX0 + 2.f + (k - 7) * 4.5f : -edge + 6.f + k * (PITCH_W + 2 * edge - 12.f) / 6.f;
        float fy = aw ? PITCH_L + edge + 2.2f + (k % 2) * 2.f : -edge - 1.6f - (k % 3) * 2.2f;
        int side = aw ? 3 : 2;
        if ((aw ? fy - PITCH_L - edge : -edge - fy) > L0.depth[side] - (L0.kind[side] == 2 ? 2.4f : 0.4f)) continue;
        int x = (int)std::round((fx + MARGIN) * PPM) - ox, y = (int)std::round((fy + MARGIN) * PPM) - oy;
        if (x < -20 || x > MWZ + 20 || y < -20 || y > MHZ + 20) continue;
        float e = ex[aw ? 1 : 0];
        if (e < 0.08f) continue;
        Color c1 = aw ? ak1 : hk1, c2 = aw ? ak2 : hk2;
        DrawLine(x, y, x, y - 9, Color{ 200, 200, 200, 255 });
        for (int c = 0; c < 9; c++) {
            int wave = (int)std::round(std::sin(t * (4.f + e * 4.f) + c * 0.7f + k) * 1.2f);
            DrawRectangle(x + 1 + c, y - 9 + wave, 1, 3, c1);
            DrawRectangle(x + 1 + c, y - 6 + wave, 1, 2, c2);
        }
    }
    // fumigènes : but, fête, ou grosse ambiance dans le kop
    for (int camp = 0; camp < 2; camp++) {
        if (ex[camp] < 0.85f || m.S.neutral) continue;
        for (int k = 0; k < 4; k++) {
            float fx = camp ? awayX0 + 1.5f + k * 2.6f : PITCH_W / 2 - 14.f + k * 9.f;
            float fy = camp ? PITCH_L + edge + 1.f : -edge - 1.f;
            int x = (int)std::round((fx + MARGIN) * PPM) - ox, y = (int)std::round((fy + MARGIN) * PPM) - oy;
            if (x < -30 || x > MWZ + 30 || y < -40 || y > MHZ + 30) continue;
            fxFlare(x, y, t, (unsigned)(k * 31 + camp * 7));
        }
    }
}

// fumigène : flamme rouge vif, halo et fumée qui dérive
void fxFlare(int x, int y, float t, unsigned seed) {
    for (int k = 0; k < 7; k++) {
        float a = std::fmod(t * 0.35f + k / 7.f + (seed % 13) * 0.07f, 1.f);
        int r = 2 + (int)(a * 6);
        DrawCircle(x + (int)(a * 10.f) + (int)(std::sin(t + k) * 2), y - 3 - (int)(a * 16.f), (float)r, Color{ 210, 205, 215, (unsigned char)(110 * (1 - a)) });
    }
    DrawCircle(x, y, 4, Color{ 255, 60, 30, 70 });
    bool fl = ((int)(t * 15) + seed) % 2;
    DrawRectangle(x - 1, y - 1, 2, 2, fl ? Color{ 255, 240, 200, 255 } : Color{ 255, 90, 40, 255 });
    DrawRectangle(x, y - 2, 1, 1, Color{ 255, 200, 80, 255 });
}

// bancs de touche : remplaçants assis, entraîneur et adjoint dans la zone technique, kiné ; échauffement le long de la ligne
static void drawBenchesLive(const Match& m, int ox, int oy, int MWZ, int MHZ) {
    if (m.S.training) return;
    const float t = (float)GetTime();
    auto SX = [&](float x) { return (int)std::round((x + MARGIN) * PPM) - ox; };
    auto SY = [&](float y) { return (int)std::round((y + MARGIN) * PPM) - oy; };
    if (SX(-0.2f) < -30 || SX(-4.f) > MWZ + 30) return;
    bool goal = m.state == MS_GOAL || (m.state == MS_REPLAY && m.lastScorerTeam >= 0);
    for (int tm = 0; tm < 2; tm++) {
        float yc = PITCH_L / 2 + (tm ? 7.5f : -7.5f);
        if (SY(yc + 9) < -20 || SY(yc - 9) > MHZ + 20) continue;
        const Team& T = m.team(tm);
        bool joy = (goal && m.lastScorerTeam == tm) || (m.finished && m.score[tm] > m.score[1 - tm]) || (m.trophyActive && m.trophyTeam == tm);
        bool sad = (goal && m.lastScorerTeam == 1 - tm) || (m.finished && m.score[tm] < m.score[1 - tm]);
        Kit bib; bib.shirt = tm ? 0x30B050 : 0xF07A20; bib.shirt2 = bib.shirt; bib.shorts = m.kit[tm].shorts; bib.socks = m.kit[tm].socks;
        Kit track; track.shirt = m.kit[tm].shorts == 0xFFFFFF ? 0x1C2A55 : m.kit[tm].shorts; track.shirt2 = m.kit[tm].shirt; track.shorts = track.shirt; track.socks = 0x151515; track.pattern = KP_SHOULDERS;
        Kit suit; suit.shirt = 0x22252E; suit.shirt2 = 0xE8E8E8; suit.shorts = 0x1A1C22; suit.socks = 0x111111;
        // remplaçants : assis sur le banc, debout et sautant lors d'un but
        int nb = std::min(7, (int)m.bench[tm].size());
        for (int k = 0; k < nb; k++) {
            int sq = m.bench[tm][k];
            int skin = 0, hair = 0;
            if (sq >= 0 && sq < (int)T.squad.size()) { skin = T.squad[sq].skin; hair = T.squad[sq].hair; g_sprLongHair = T.squad[sq].gender; }
            int x = SX(-2.2f), y = SY(yc - 3.3f + k * 1.0f) + 3;
            if (joy) { y -= (int)(std::fabs(std::sin(t * 7.f + k)) * 3.f) + 2; drawPlayerSprite(x + 2, y, bib, skin, hair, 2, (int)(t * 4) + k, PS_CELEB, false, 0, 1); }
            else drawPosedSprite(x, y, bib, skin, hair, 2, 0, PS_NORMAL, sad && k % 2 ? POSE_HEAD : POSE_SIT, 1);
            g_sprLongHair = false;
        }
        // entraîneur : fait les cent pas, donne des consignes, exulte ou se prend la tête
        {
            float pace = std::sin(t * 0.45f + tm * 2.f);
            float cy = yc + (tm ? -1.f : 1.f) * 1.5f + pace * 2.6f;
            int x = SX(-0.6f), y = SY(cy);
            int pose = POSE_NONE, state = PS_NORMAL, dir = std::cos(t * 0.45f + tm * 2.f) > 0 ? 0 : 1, fr = (int)(t * 6);
            if (joy) { state = PS_CELEB; dir = 0; y -= (int)(std::fabs(std::sin(t * 6.f)) * 4.f); }
            else if (sad) { pose = POSE_HEAD; dir = 0; fr = 0; }
            else if (((int)(t * 0.5f) + tm) % 3 == 0) { pose = POSE_POINT; dir = 2; fr = 0; }
            drawPosedSprite(x, y, suit, (tm * 3 + 1) % NUM_SKINS, tm ? 5 : 0, dir, fr, state, pose, 1);
            // adjoint : debout au bout du banc, bras croisés ou applaudissements
            drawPosedSprite(SX(-1.5f), SY(yc + (tm ? 4.6f : -4.6f)), track, (tm * 2 + 2) % NUM_SKINS, tm + 1, 2, (int)(t * 5), joy ? PS_CELEB : PS_NORMAL, joy ? POSE_NONE : POSE_CLAP, 1);
            // kiné assis avec sa sacoche
            int kx = SX(-2.2f), ky = SY(yc + 3.9f) + 3;
            drawPosedSprite(kx, ky, track, (tm + 4) % NUM_SKINS, 4, 2, 0, PS_NORMAL, POSE_SIT, 1);
            DrawRectangle(kx + 3, ky - 3, 4, 3, WHITE); DrawRectangle(kx + 4, ky - 3, 2, 3, Color{ 220, 30, 30, 255 }); DrawRectangle(kx + 3, ky - 2, 4, 1, Color{ 220, 30, 30, 255 });
        }
        // échauffement : deux remplaçants trottinent derrière la ligne, entre le banc et le poteau de corner
        if (m.state == MS_PLAY || m.state == MS_SETPIECE || m.state == MS_STOP) {
            for (int k = 0; k < 2 && k + nb - 2 >= 0 && nb >= 3; k++) {
                int sq = m.bench[tm][nb - 1 - k];
                int skin = sq >= 0 && sq < (int)T.squad.size() ? T.squad[sq].skin : 0, hair = sq >= 0 && sq < (int)T.squad.size() ? T.squad[sq].hair : 0;
                float u = std::sin(t * 0.35f + k * 0.9f + tm);
                float wy = tm ? PITCH_L / 2 + 22.f + u * 10.f : PITCH_L / 2 - 22.f - u * 10.f;
                float vy = std::cos(t * 0.35f + k * 0.9f + tm) * (tm ? 1.f : -1.f);
                drawPlayerSprite(SX(-2.6f - k * 0.9f), SY(wy), bib, skin, hair, vy > 0 ? 0 : 1, (int)(t * 7) + k, PS_NORMAL, false, 0, 1);
            }
        }
    }
    g_sprLongHair = false;
}

// panneaux LED animés le long des lignes de but (stades professionnels) : affiche, score, « BUT ! » qui clignote
static void drawLedBoards(const Match& m, int ox, int oy, int MWZ, int MHZ) {
    if (m.S.training || g_look.small || !m.S.proMedia) return;
    const float t = (float)GetTime();
    auto SX = [&](float x) { return (int)std::round((x + MARGIN) * PPM) - ox; };
    auto SY = [&](float y) { return (int)std::round((y + MARGIN) * PPM) - oy; };
    bool goal = m.state == MS_GOAL && m.lastScorerTeam >= 0;
    std::string msg;
    Color fg = Color{ 255, 220, 80, 255 }, bg = Color{ 8, 10, 18, 255 };
    if (goal) { msg = "  BUT !  BUT !  BUT !  " + m.team(m.lastScorerTeam).shortName + "  "; fg = ((int)(t * 6) % 2) ? Color{ 255, 220, 80, 255 } : WHITE; bg = ((int)(t * 6) % 2) ? Color{ 40, 10, 10, 255 } : Color{ 10, 10, 40, 255 }; }
    else if (m.ceremony) msg = "  BIENVENUE  -  " + m.team(0).name + "  -  " + m.team(1).name + "  -  " + (m.S.title.empty() ? std::string("") : m.S.title + "  ");
    else msg = fmt("  %s %d - %d %s   PIXEL COLA   CRAMPONS+   GOAL FM 98.5   SUPER BUT   ", m.team(0).shortName.c_str(), m.score[0], m.score[1], m.team(1).shortName.c_str());
    int tw = std::max(40, textWidth(msg, 10));
    for (int g = 0; g < 2; g++) {
        float wy = g ? PITCH_L + 4.0f : -6.0f;
        int y0 = SY(wy), y1 = SY(wy + 2.0f);
        if (y1 < -4 || y0 > MHZ + 4) continue;
        int x0 = SX(-8.f), x1 = SX(PITCH_W + 8.f);
        x0 = std::max(x0, -4); x1 = std::min(x1, MWZ + 4);
        if (x1 <= x0) continue;
        DrawRectangle(x0, y0, x1 - x0, y1 - y0, bg);
        int scroll = (int)std::fmod(t * 28.f, (float)tw);
        int base = SX(-8.f) - scroll + (g ? tw / 2 : 0);
        for (int x = base; x < x1; x += tw) if (x + tw > x0) drawTextPx(msg, x, y0, 10, fg);
        DrawRectangle(x0, y0, x1 - x0, 1, Color{ 255, 255, 255, 40 });
        // trame des diodes
        for (int x = x0; x < x1; x += 2) DrawRectangle(x, y0, 1, y1 - y0, Color{ 0, 0, 0, 60 });
    }
}

// pylônes d'éclairage dans les coins du stade
static void drawFloodlights(const Match& m, int ox, int oy, int MWZ, int MHZ) {
    if (m.S.training) return;
    auto SX = [&](float x) { return (int)std::round((x + MARGIN) * PPM) - ox; };
    auto SY = [&](float y) { return (int)std::round((y + MARGIN) * PPM) - oy; };
    for (int k = 0; k < 4; k++) {
        float fx = (k & 1) ? PITCH_W + 9.5f : -9.5f, fy = (k & 2) ? PITCH_L + 9.5f : -9.5f;
        int x = SX(fx), y = SY(fy);
        if (x < -30 || x > MWZ + 30 || y < -40 || y > MHZ + 40) continue;
        DrawRectangle(x - 1, y - 18, 3, 20, Color{ 70, 74, 86, 255 });
        DrawRectangle(x - 8, y - 26, 17, 9, Color{ 50, 54, 66, 255 });
        for (int r = 0; r < 2; r++) for (int c = 0; c < 4; c++) DrawRectangle(x - 7 + c * 4, y - 25 + r * 4, 3, 3, m.S.night ? Color{ 255, 252, 220, 255 } : Color{ 190, 196, 210, 255 });
        if (m.S.night) DrawCircleGradient(x, y - 22, 16, Color{ 255, 250, 210, 80 }, Color{ 255, 250, 210, 0 });
    }
}

// entrée des joueurs : flammes à la sortie du tunnel, serpentins et confettis lancés des tribunes ; fumée après un but
static void drawShowEffects(const Match& m, int ox, int oy, int MWZ, int MHZ) {
    if (m.S.training) return;
    const float t = (float)GetTime();
    auto SX = [&](float x) { return (int)std::round((x + MARGIN) * PPM) - ox; };
    auto SY = [&](float y) { return (int)std::round((y + MARGIN) * PPM) - oy; };
    if (m.ceremony && m.cerPhase < 0) {
        if (m.S.proMedia) for (int k = 0; k < 2; k++) {   // jets de flammes de part et d'autre du tunnel
            int x = SX(-0.4f), y = SY(PITCH_L / 2 + (k ? 3.6f : -3.6f));
            float cyc = std::fmod(t * 0.8f + k * 0.5f, 2.f);
            if (cyc < 0.7f) {
                int h = (int)(8 + std::sin(cyc * 4.5f) * 14);
                DrawRectangle(x - 2, y - h, 5, h, Color{ 255, 110, 30, 220 }); DrawRectangle(x - 1, y - h + 3, 3, h - 3, Color{ 255, 230, 120, 240 });
                DrawCircle(x, y - h / 2, 9, Color{ 255, 150, 40, 60 });
            }
            DrawRectangle(x - 2, y - 1, 5, 3, Color{ 40, 40, 46, 255 });
        }
        // serpentins de papier lancés depuis le kop (haut) et le parcage
        Color c1 = hexc(m.kit[0].shirt), c2 = WHITE;
        for (int k = 0; k < 60; k++) {
            float u = std::fmod(t * 0.35f + k * 0.0173f, 1.f);
            float wx = -4.f + (k * 7.3f) - std::floor((k * 7.3f) / (PITCH_W + 8)) * (PITCH_W + 8);
            float wy = -9.f + u * 7.f;
            int x = SX(wx), y = SY(wy);
            if (x < -4 || x > MWZ + 4 || y < -4 || y > MHZ + 4) continue;
            int wob = (int)(std::sin(t * 4 + k) * 2);
            DrawLine(x + wob, y, x - wob, y + 4, k % 2 ? c1 : c2);
        }
    }
    // fumée des fumigènes qui dérive sur le terrain après un but du club local
    if ((m.state == MS_GOAL || m.state == MS_REPLAY) && m.lastScorerTeam == 0 && !m.S.neutral && m.stateT > 0.8f) {
        float a = std::min(1.f, (m.stateT - 0.8f) * 0.5f);
        for (int k = 0; k < 10; k++) {
            float wx = 4.f + k * 6.5f + std::sin(t * 0.3f + k) * 3.f, wy = -4.f + std::fmod(t * 0.9f + k * 0.7f, 9.f);
            int x = SX(wx), y = SY(wy);
            if (x < -30 || x > MWZ + 30 || y < -30 || y > MHZ + 30) continue;
            DrawCircle(x, y, 10.f + (k % 3) * 4.f, Color{ 215, 210, 220, (unsigned char)(28 * a) }); DrawCircle(x + 4, y - 3, 7.f + (k % 2) * 3.f, Color{ 230, 226, 236, (unsigned char)(24 * a) });
        }
    }
}

// ramasseurs de balles et stadiers
static void drawPitchsideLive(const Match& m, int ox, int oy, int MWZ, int MHZ) {
    if (m.S.training) return;
    const float t = (float)GetTime();
    auto vis = [&](int x, int y) { return x > -12 && x < MWZ + 12 && y > -16 && y < MHZ + 16; };
    auto SX = [&](float x) { return (int)std::round((x + MARGIN) * PPM) - ox; };
    auto SY = [&](float y) { return (int)std::round((y + MARGIN) * PPM) - oy; };
    Kit kid; kid.shirt = 0x2A5CC8; kid.shirt2 = 0xF4F4F4; kid.shorts = 0x1A2A55; kid.socks = 0x1A2A55; kid.pattern = KP_SHOULDERS;
    static const float BB[6][2] = { { PITCH_W / 2 - 17.f, -2.6f }, { PITCH_W / 2 + 17.f, -2.6f }, { PITCH_W / 2 - 17.f, PITCH_L + 2.6f }, { PITCH_W / 2 + 17.f, PITCH_L + 2.6f },
                                    { PITCH_W + 1.9f, PITCH_L * 0.3f }, { PITCH_W + 1.9f, PITCH_L * 0.7f } };
    for (int k = 0; k < 6; k++) {
        int x = SX(BB[k][0]), y = SY(BB[k][1]);
        if (!vis(x, y)) continue;
        // le ramasseur se lève quand le ballon sort près de lui
        float bd = (m.ball.pos - V2(BB[k][0], BB[k][1])).len();
        bool up = bd < 12.f && (m.state == MS_SETPIECE || m.state == MS_STOP);
        int dir = k < 2 ? 0 : k < 4 ? 1 : 3;
        drawPosedSprite(x, y + (up ? 0 : 2), kid, (k * 2 + 1) % NUM_SKINS, (k * 3) % NUM_HAIRS, dir, (int)(t * 6), PS_NORMAL, up ? POSE_NONE : POSE_SIT, 1);
        if (!up) { DrawRectangle(x + (k < 4 ? 4 : -6), y - 1, 3, 3, WHITE); DrawRectangle(x + (k < 4 ? 5 : -5), y, 1, 1, Color{ 30, 30, 30, 255 }); }   // ballon de réserve
    }
    // stadiers en chasuble fluo, dos au terrain, face aux tribunes
    Kit vest; vest.shirt = 0xF09020; vest.shirt2 = 0xE8F040; vest.shorts = 0x22242C; vest.socks = 0x111111; vest.pattern = KP_BAND;
    const float e = 3.3f;
    static const float ST[8][3] = { { 2.5f, -1, 1 }, { 65.5f, -1, 1 }, { 2.5f, -2, 0 }, { 65.5f, -2, 0 }, { 34.f, -3, 3 }, { 72.f, -3, 3 }, { 6.f, -3, 3 }, { 99.f, -3, 3 } };
    for (int k = 0; k < 8; k++) {
        float wx, wy; int dir = (int)ST[k][2];
        if (ST[k][1] == -1) { wx = ST[k][0]; wy = -e; }
        else if (ST[k][1] == -2) { wx = ST[k][0]; wy = PITCH_L + e; }
        else { wx = PITCH_W + e; wy = ST[k][0]; dir = 2; }
        int x = SX(wx), y = SY(wy);
        if (!vis(x, y)) continue;
        drawPlayerSprite(x, y, vest, (k * 5 + 2) % NUM_SKINS, (k * 2 + 1) % NUM_HAIRS, dir, 0, PS_NORMAL, false, 0, 1);
    }
}

// ------------------------------------------------------------------ scénettes TV pendant les arrêts de jeu
// incrustations façon réalisation télé : supporters, kop, banc, échauffement, mascotte, loge, parcage, jeune supporter, presse
static void drawSceneInset(const Match& m, int kind, float t, int x, int y, int w, int h) {
    Color hk = hexc(m.kit[0].shirt), hk2 = hexc(m.kit[0].shirt2 ? m.kit[0].shirt2 : m.kit[0].shorts);
    Color ak = hexc(m.kit[1].shirt), ak2 = hexc(m.kit[1].shirt2 ? m.kit[1].shirt2 : m.kit[1].shorts);
    const Team& H = m.team(0);
    Kit suit; suit.shirt = 0x22252E; suit.shirt2 = 0xE8E8E8; suit.shorts = 0x1A1C22; suit.socks = 0x111111;
    DrawRectangle(x, y, w, h, Color{ 20, 24, 40, 255 });
    switch (kind) {
    case 0: {   // un supporter en gros plan, écharpe tendue au-dessus de la tête
        fxCrowd(x, y + 4, w, h - 4, hk, hk2, t, 0.34f, 3, 3);
        DrawRectangle(x, y, w, h, Color{ 0, 0, 0, 90 });
        Kit fk = m.kit[0]; fk.pattern = 0;
        int ps = h - 10;
        drawPortrait(x + w / 2 - ps / 2, y + 10, ps, (int)(m.S.home % 5), (int)(m.S.away % 6), 0, fk, (unsigned)m.S.home);
        int up = (int)(std::fabs(std::sin(t * 4.f)) * 4);
        for (int k = 0; k < 10; k++) DrawRectangle(x + w / 2 - ps / 2 - 6 + k * (ps + 12) / 10, y + 4 - up, (ps + 12) / 10 + 1, 5, k % 2 ? hk : hk2);
        break;
    }
    case 1: {   // le kop : capo au mégaphone, tambour, drapeaux
        fxCrowd(x, y + 2, w, h - 2, hk, hk2, t, 0.34f, 5, 4);
        DrawRectangle(x, y + h - 14, w, 14, Color{ 60, 60, 70, 255 });
        for (int k = 0; k < w; k += 6) DrawRectangle(x + k, y + h - 22, 1, 8, Color{ 150, 150, 160, 255 });
        int cx = x + 30;
        cx = x + 18;
        drawPlayerSprite(cx, y + h - 13, m.kit[0], 2, 0, 0, (int)(t * 4), PS_NORMAL, false, 0, 2);
        DrawRectangle(cx + 4, y + h - 32, 5, 3, Color{ 230, 230, 230, 255 }); DrawTriangle(Vector2{ (float)cx + 9, (float)y + h - 34 }, Vector2{ (float)cx + 9, (float)y + h - 27 }, Vector2{ (float)cx + 13, (float)y + h - 30 }, Color{ 230, 230, 230, 255 });
        int dx = x + w - 16, hit = ((int)(t * 4)) % 2;
        DrawEllipse(dx, y + h - 20, 9, 7, Color{ 200, 40, 40, 255 }); DrawEllipse(dx, y + h - 22, 9, 4, Color{ 240, 236, 220, 255 });
        DrawLine(dx - 8, y + h - 30 + hit * 4, dx - 2, y + h - 23, Color{ 120, 80, 40, 255 });
        break;
    }
    case 2: {   // sur le banc : l'entraîneur donne ses consignes
        DrawRectangle(x, y, w, h, Color{ 50, 140, 60, 255 });
        DrawRectangle(x, y, w, 18, Color{ 60, 66, 80, 255 }); DrawRectangle(x, y + 18, w, 3, Color{ 150, 160, 180, 255 });
        for (int k = 0; k < 4; k++) drawPosedSprite(x + 7 + k * 9, y + 21, m.kit[0], k, k + 1, 0, 0, PS_NORMAL, POSE_SIT, 1);
        int pose = ((int)(t * 1.5f)) % 2 ? POSE_POINT : POSE_NONE;
        drawPosedSprite(x + w - 22, y + h - 3, suit, 1, 0, 2, 0, PS_NORMAL, pose, 2);
        if (pose == POSE_POINT) drawTextPx("!", x + w - 10, y + 20, 5, Color{ 255, 225, 90, 255 });
        break;
    }
    case 3: {   // échauffement le long de la ligne
        DrawRectangle(x, y, w, h, Color{ 50, 140, 60, 255 });
        DrawRectangle(x, y + h - 20, w, 1, WHITE);
        Kit bib; bib.shirt = 0xF07A20; bib.shirt2 = 0xF07A20; bib.shorts = m.kit[0].shorts; bib.socks = m.kit[0].socks;
        for (int k = 0; k < 2; k++) { float u = std::fmod(t * 0.4f + k * 0.3f, 2.f); float px = u < 1 ? u : 2 - u; drawPlayerSprite(x + 8 + (int)(px * (w - 16)), y + h - 22 + k * 10, bib, k + 1, k + 2, u < 1 ? 2 : 3, (int)(t * 8) + k, PS_NORMAL, false, 0, 1); }
        break;
    }
    case 4: {   // la mascotte du club danse devant la tribune
        fxCrowd(x, y + 2, w, 16, hk, hk2, t, 0.34f, 9, 3);
        DrawRectangle(x, y + 18, w, h - 18, Color{ 50, 140, 60, 255 });
        int mx = x + w / 2 + (int)(std::sin(t * 3) * 10), my = y + h - 6 - (int)(std::fabs(std::sin(t * 6)) * 4);
        drawPlayerSprite(mx, my, m.kit[0], 0, 0, 0, (int)(t * 5), PS_CELEB, false, 0, 2);
        DrawCircle(mx, my - 20, 7, hk); DrawCircle(mx - 5, my - 26, 2, hk); DrawCircle(mx + 5, my - 26, 2, hk);
        DrawCircle(mx - 2, my - 21, 1.5f, WHITE); DrawCircle(mx + 2, my - 21, 1.5f, WHITE); DrawPixel(mx - 2, my - 21, BLACK); DrawPixel(mx + 2, my - 21, BLACK);
        DrawRectangle(mx - 2, my - 17, 4, 1, Color{ 200, 40, 40, 255 });
        for (int k = 0; k < 3; k++) drawPlayerSprite(x + 8 + k * 9, y + h - 2, m.kit[0], k + 2, k, 1, 0, PS_CELEB, false, 0, 1);
        break;
    }
    case 5: {   // la loge : le président derrière la vitre
        DrawRectangleGradientV(x, y, w, h, Color{ 70, 60, 50, 255 }, Color{ 40, 34, 30, 255 });
        int ps = h - 12;
        drawPortrait(x + w / 2 - ps / 2, y + 8, ps, 1, 5, 0, suit, 77u + (unsigned)m.S.home);
        for (int k = 0; k < 3; k++) DrawLine(x + 10 + k * 30, y, x + 30 + k * 30, y + h, Color{ 255, 255, 255, 40 });
        DrawRectangle(x, y + h - 8, w, 8, Color{ 30, 26, 22, 255 });
        bool tense = m.score[0] <= m.score[1];
        if (!tense && ((int)(t * 4)) % 2) { DrawRectangle(x + w / 2 - 10, y + h - 16, 6, 5, hexc(0xE0B090)); DrawRectangle(x + w / 2 + 4, y + h - 16, 6, 5, hexc(0xE0B090)); }
        break;
    }
    case 6: {   // le parcage visiteur derrière ses grilles
        fxCrowd(x, y + 2, w, h - 2, ak, ak2, t, 0.34f, 11, 4);
        for (int k = 0; k < w; k += 5) DrawRectangle(x + k, y, 1, h, Color{ 160, 160, 170, 200 });
        DrawRectangle(x, y + h / 2, w, 1, Color{ 160, 160, 170, 200 });
        break;
    }
    case 7: {   // un jeune supporter, maquillé aux couleurs du club, agite son drapeau
        fxCrowd(x, y + 4, w, h - 4, hk, hk2, t, 0.34f, 13, 3);
        DrawRectangle(x, y, w, h, Color{ 0, 0, 0, 80 });
        int ps = h - 18;
        int px = x + w / 2 - ps / 2, py = y + 16;
        drawPortrait(px, py, ps, 4, 2, 0, m.kit[0], 5u + (unsigned)m.S.away);
        int fx = x + w - 22, wv = (int)(std::sin(t * 7) * 2);
        DrawLine(fx, y + 6, fx, y + h - 4, Color{ 220, 220, 220, 255 });
        DrawRectangle(fx + 1, y + 6 + wv, 16, 5, hk); DrawRectangle(fx + 1, y + 11 + wv, 16, 5, hk2);
        break;
    }
    default: {  // tribune de presse : journalistes, ordinateurs, flashs
        DrawRectangle(x, y, w, h, Color{ 30, 34, 46, 255 });
        DrawRectangle(x, y + h - 18, w, 18, Color{ 90, 70, 50, 255 });
        for (int k = 0; k < 4; k++) {
            int jx = x + 10 + k * (w - 14) / 4;
            drawPlayerSprite(jx, y + h - 12, suit, (k * 2) % 6, k, 1, 0, PS_NORMAL, false, 0, 1);
            DrawRectangle(jx - 4, y + h - 14, 9, 4, Color{ 50, 50, 60, 255 }); DrawRectangle(jx - 3, y + h - 18, 7, 4, Color{ 120, 170, 230, 255 });
        }
        if (((int)(t * 5)) % 7 == 0) DrawCircle(x + w - 20, y + 14, 8, Color{ 255, 255, 255, 160 });
        break;
    }
    }
    (void)H;
}
static void drawMatchScenes(const Match& m) {
    static const Match* sm = nullptr; static float st = -1, wait = 0, lastClock = -100; static int kind = 0;
    if (sm != &m) { sm = &m; st = -1; wait = 0; lastClock = -100; }
    bool stop = !m.S.training && !m.ceremony && !m.finished && !m.trophyActive && !m.lapActive && m.fightT <= 0 && m.shootUI <= 0 && m.subBoardT <= 0 && m.pendCardOff < 0 &&
                (m.state == MS_STOP || (m.state == MS_SETPIECE && !m.spReady && m.sp != SP_KICKOFF));
    float dt = GetFrameTime();
    if (getenv("FOOT_SCENE")) { kind = atoi(getenv("FOOT_SCENE")) % 9; st = 1.f; stop = true; dt = 0; }
    if (st < 0) {
        if (stop) {
            wait += dt;
            if (wait > 0.6f && m.clock - lastClock > 5.f) {
                lastClock = m.clock;
                if (((int)(m.clock * 7.3f)) % 3 != 0) {
                    // le contexte oriente le choix : score, minute, équipe qui pousse
                    int base = (int)(m.clock * 13.f + m.score[0] * 5 + m.score[1] * 3) % 9;
                    if (m.score[1] > m.score[0] && base == 0) base = 6;
                    kind = base; st = 0;
                }
            }
        } else wait = 0;
        return;
    }
    st += dt;
    if (st > 3.4f || (!stop && st > 1.2f)) { st = -1; return; }
    static const char* LAB[9] = { "EN TRIBUNE", "LE KOP", "SUR LE BANC", "ÉCHAUFFEMENT", "LA MASCOTTE", "EN LOGE", "LE PARCAGE VISITEUR", "JEUNE SUPPORTER", "TRIBUNE DE PRESSE" };
    float in = std::min(1.f, st * 4.f), out = std::min(1.f, (3.4f - st) * 4.f);
    int w = 76, h = 46, x = 4 - (int)((1.f - std::min(in, out)) * 100), y = 22;
    float t = (float)GetTime();
    DrawRectangle(x + 2, y + 2, w + 4, h + 14, Color{ 0, 0, 0, 120 });
    DrawRectangle(x - 2, y - 2, w + 4, h + 4, Color{ 230, 230, 240, 255 });
    drawSceneInset(m, kind, t, x, y, w, h);
    DrawRectangle(x - 2, y + h + 2, w + 4, 10, Color{ 190, 30, 40, 255 });
    drawTextPx(LAB[kind], x + 2, y + h + 3, 5, WHITE);
    if (((int)(t * 2)) % 2) DrawCircle(x + w - 4, y + h + 7, 2, WHITE);
}

static float g_asVel[2] = { 0, 0 }, g_asAnim[2] = { 0, 0 };
void renderMatch(const Match& m, bool radar) {
    rlDrawRenderBatchActive();
    rlPushMatrix();
    const float Z = std::max(1.f, std::min(2.5f, m.camZoom));
    rlScalef(2 * Z, 2 * Z, 1);
    const int MWZ = (int)(MW / Z), MHZ = (int)(MH / Z);
    int type = std::max(0, std::min(4, m.S.pitch));
    {
        // aspect du stade du club qui reçoit
        const Team& HT = g_world.teams[m.S.home];
        uint64_t key = ((uint64_t)m.S.home << 8) ^ (uint64_t)(m.S.crowdFill * 100) ^ ((uint64_t)m.S.neutral << 40) ^ ((uint64_t)(m.S.training ? 1 : 0) << 41) ^ ((uint64_t)(m.S.turf / 5) << 48) ^ ((uint64_t)g_boardAdsVer << 52);
        if (!g_pitchBuilt[type] || g_pitchKey[type] != key) {
            StadiumLook L;
            if (!m.S.neutral && HT.sta.init) {
                static const int MAP[4] = { 0, 1, 2, 3 };
                for (int k = 0; k < 4; k++) {
                    const Stand& st = HT.sta.s[MAP[k]];
                    L.depth[k] = std::max(1.2f, std::min(11.f, std::sqrt((float)(st.seats + st.vip)) / 9.f));
                    L.kind[k] = st.kind;
                }
                L.small = HT.sta.capacity() < 5000;
            }
            L.fill = m.S.crowdFill;
            L.turf = m.S.turf;
            if (m.S.training) {   // terrain d'entraînement : pas de tribunes ni de public
                for (int k = 0; k < 4; k++) { L.depth[k] = 1.2f; L.kind[k] = STK_STANDING; }
                L.small = true; L.fill = 0;
            }
            if (!m.S.neutral && !m.S.training) { L.sponsor = HT.sponsor; if (g_sponsorImagePath && !L.sponsor.empty()) L.logo = g_sponsorImagePath(L.sponsor); }
            g_look = L;
            buildPitch(type, key);
        }
    }
    bool replay = m.state == MS_REPLAY;
    // position caméra
    V2 cam = m.cam;
    if (replay) { const BallSnap& b = m.rb[m.rpPos]; cam = V2(b.x, b.y); }
    float TW = (PITCH_W + 2 * MARGIN) * PPM, TH = (PITCH_L + 2 * MARGIN) * PPM;
    float cx = (cam.x + MARGIN) * PPM, cy = (cam.y + MARGIN) * PPM;
    int ox = (int)std::round(std::max(0.f, std::min(TW - MWZ, cx - MWZ / 2)));
    int oy = (int)std::round(std::max(0.f, std::min(TH - MHZ, cy - MHZ / 2)));
    if (TW < MWZ) ox = (int)(TW - MWZ) / 2;
    DrawTextureRec(g_pitchTex[type], Rectangle{ (float)ox, (float)oy, (float)MWZ, (float)MHZ }, Vector2{ 0, 0 }, WHITE);
    auto SX = [&](float x) { return (int)std::round((x + MARGIN) * PPM) - ox; };
    auto SY = [&](float y) { return (int)std::round((y + MARGIN) * PPM) - oy; };

    drawCrowdLive(m, ox, oy, MWZ, MHZ);
    drawFloodlights(m, ox, oy, MWZ, MHZ);
    drawLedBoards(m, ox, oy, MWZ, MHZ);
    drawPitchsideLive(m, ox, oy, MWZ, MHZ);
    if (!replay) drawBenchesLive(m, ox, oy, MWZ, MHZ);
    if(!m.S.training){int chant=supportersLiveChant(m.S,m.score[0],m.score[1],m.clock);if(m.S.supporterTifo>=0&&(m.clock<8||m.ceremony)){Color c1=hexc(m.kit[0].shirt),c2=hexc(m.kit[0].shorts);int tx=SX(-6.f),ty=SY(PITCH_L/2-8.f);for(int y=0;y<16;y++)for(int x=0;x<4;x++)DrawRectangle(tx+x*2,ty+y*2,2,2,(x+y)%2?c1:c2);DrawRectangleLines(tx-1,ty-1,10,34,Color{240,220,130,255});}if(chant==CH_PROTEST){int bx=SX(-5.5f),by=SY(PITCH_L/2+9.f);DrawRectangle(bx-2,by,9,31,Color{235,235,220,255});for(int y=1;y<29;y+=4)DrawRectangle(bx,by+y,5,2,Color{190,30,40,255});}}
    // ombres des buts
    for (int g = 0; g < 2; g++) {
        float gy = g == 0 ? 0 : PITCH_L;
        float s = g == 0 ? -1.f : 1.f;
        int x1 = SX(PITCH_W / 2 - GOAL_W / 2) + 2, x2 = SX(PITCH_W / 2 + GOAL_W / 2) + 3;
        int y1 = SY(gy), y2 = SY(gy + s * GOAL_DEPTH);
        DrawRectangle(x1, std::min(y1, y2), x2 - x1, std::abs(y2 - y1) + 1, Color{ 0, 0, 0, 45 });
    }
    // médias : caméras le long de la touche, photographes derrière les buts (football professionnel)
    if (m.S.proMedia && !m.S.training) {
        Kit crew; crew.shirt = 0x2A2A2A; crew.shirt2 = 0x2A2A2A; crew.shorts = 0x1A1A2A; crew.socks = 0x1A1A1A;
        static const float CY[3] = { PITCH_L / 2, 16.5f, PITCH_L - 16.5f };
        for (int k = 0; k < 3; k++) {
            int x = SX(PITCH_W + 2.6f), y = SY(CY[k]);
            if (x < -20 || x > MW + 20 || y < -20 || y > MH + 20) continue;
            DrawLine(x + 2, y, x + 5, y - 6, Color{ 60, 60, 60, 255 }); DrawLine(x + 8, y, x + 5, y - 6, Color{ 60, 60, 60, 255 });   // trépied
            drawPlayerSprite(x - 3, y, crew, (k + 1) % 4, k % 5, 3, 0, PS_NORMAL, false, 0, 1);
            DrawRectangle(x + 1, y - 10, 7, 4, Color{ 40, 40, 44, 255 });
            DrawRectangle(x - 1, y - 9, 2, 2, Color{ 120, 160, 220, 255 });   // objectif
            DrawRectangle(x + 6, y - 11, 1, 1, Color{ 230, 40, 40, 255 });     // voyant rouge
        }
        // caméra mobile sur rail derrière la ligne de touche opposée (suit le ballon)
        {
            int x = SX(-2.8f), y = SY(std::max(8.f, std::min(PITCH_L - 8.f, m.ball.pos.y)));
            if (x > -20 && x < MW + 20) { DrawRectangle(x - 1, y - 4, 3, 8, Color{ 30, 30, 30, 255 }); DrawRectangle(x - 4, y - 2, 3, 3, Color{ 120, 160, 220, 255 }); }
        }
        for (int g = 0; g < 2; g++) {
            float gy = g == 0 ? -2.2f : PITCH_L + 2.2f;
            float bnear = (g == 0 ? m.ball.pos.y : PITCH_L - m.ball.pos.y);
            for (int k = 0; k < 8; k++) {
                float fx = PITCH_W / 2 + (k < 4 ? -1.f : 1.f) * (5.2f + (k % 4) * 3.0f);
                int x = SX(fx), y = SY(gy);
                if (x < -10 || x > MW + 10 || y < -10 || y > MH + 10) continue;
                Color bib = (k % 2) ? Color{ 240, 130, 30, 255 } : Color{ 60, 170, 80, 255 };
                DrawRectangle(x - 2, y - 5, 4, 4, bib);                                    // chasuble (accroupi)
                DrawRectangle(x - 1, y - 7, 3, 2, hexc(0xD9A066));                         // tête
                DrawRectangle(x - 2, y - 1, 4, 2, Color{ 40, 40, 60, 255 });
                DrawRectangle(x + (g == 0 ? -1 : -1), y - 5 + (g == 0 ? 3 : -2), 3, 2, Color{ 20, 20, 20, 255 });   // appareil
                // flashs quand l'action est proche
                uint32_t h = (uint32_t)((int)(GetTime() * 6) * 7919 + k * 104729 + g * 31);
                h ^= h >> 13; h *= 0x5bd1e995; h ^= h >> 15;
                if (bnear < 25 && h % 9 == 0) { DrawCircle(x, y - 4, 3, Color{ 255, 255, 255, 170 }); DrawRectangle(x - 1, y - 5, 2, 2, WHITE); }
            }
        }
    }
    // drapeaux de coin
    for (int k = 0; k < 4; k++) {
        float fx = (k & 1) ? PITCH_W : 0, fy = (k & 2) ? PITCH_L : 0;
        int x = SX(fx), y = SY(fy);
        if (x < -8 || x > MW + 8 || y < -8 || y > MH + 16) continue;
        float wave = std::sin((float)GetTime() * 6 + k) > 0 ? 1.f : 0.f;
        DrawLine(x + 1, y + 1, x + 4, y + 1, Color{ 0, 0, 0, 70 });
        DrawRectangle(x, y - 7, 1, 8, Color{ 240, 240, 240, 255 });
        DrawRectangle(x + 1, y - 7, 3, 2 + (int)wave, Color{ 255, 210, 30, 255 });
        DrawRectangle(x + 1, y - 7 + 2, 3, 1, Color{ 230, 40, 40, 255 });
    }
    // tunnel des vestiaires (bord de touche, milieu de terrain)
    if (!m.S.training) {
        float c = PITCH_L / 2;
        int x0 = SX(-7.5f), x1 = SX(-0.8f), y0 = SY(c - 2.6f), y1 = SY(c + 2.6f);
        if (x1 > -10 && x0 < MW + 10 && y1 > -10 && y0 < MH + 10) {
            DrawRectangle(x0 + 2, y0 + 2, x1 - x0, y1 - y0, Color{ 0, 0, 0, 80 });
            DrawRectangle(x0, y0, x1 - x0, y1 - y0, Color{ 30, 32, 40, 255 });
            Color hc = hexc(g_world.teams[m.S.home].home.shirt);
            DrawRectangle(x0, y0, x1 - x0, 3, hc); DrawRectangle(x0, y1 - 3, x1 - x0, 3, hc);
            for (int x = x0; x < x1; x += 6) { DrawRectangle(x, y0 + 3, 1, y1 - y0 - 6, Color{ 60, 64, 78, 255 }); }
            DrawRectangle(x1 - 2, y0, 2, y1 - y0, Color{ 200, 200, 210, 255 });
        }
    }
    // podium de la remise du trophée
    if (!replay && m.trophyActive) {
        float c = m.trPod.y;
        int x0 = SX(6.0f), x1 = SX(11.2f), y0 = SY(c - 8.4f), y1 = SY(c + 8.4f);
        DrawRectangle(x0 + 2, y0 + 2, x1 - x0, y1 - y0 + 3, Color{ 0, 0, 0, 70 });
        DrawRectangle(x0, y0 - 3, x1 - x0, y1 - y0, Color{ 30, 40, 110, 255 });            // plateau
        DrawRectangle(x0, y1 - 3, x1 - x0, 3, Color{ 20, 24, 70, 255 });                    // face avant
        DrawRectangleLines(x0, y0 - 3, x1 - x0, y1 - y0, Color{ 240, 200, 70, 255 });
        for (int k = 0; k < 2; k++) {                                                        // marches
            int sy0 = k == 0 ? SY(c - 9.6f) : SY(c + 8.4f) - 3;
            DrawRectangle(SX(8.4f), sy0, SX(10.6f) - SX(8.4f), 5, Color{ 60, 70, 140, 255 });
            DrawRectangle(SX(8.4f), sy0 + 2, SX(10.6f) - SX(8.4f), 1, Color{ 40, 48, 110, 255 });
        }
        // panneau de fond
        DrawRectangle(SX(4.8f), y0 - 8, SX(6.0f) - SX(4.8f), y1 - y0 + 5, Color{ 15, 20, 60, 255 });
        for (int yy = y0 - 6; yy < y1 - 4; yy += 6) DrawRectangle(SX(4.8f) + 2, yy, 2, 2, Color{ 255, 210, 70, 255 });
        // officiels (costume) et table du trophée
        Kit suit; suit.shirt = 0x252A40; suit.shirt2 = 0xF0F0F0; suit.shorts = 0x252A40; suit.socks = 0x151515;
        for (int k = 0; k < 2; k++) drawPlayerSprite(SX(7.6f), SY(c - 0.9f + k * 1.8f) - 3, suit, k * 2, k + 1, 2, 0, PS_NORMAL, false, 0, 1);
        int tx = SX(7.4f), ty = SY(c - 3.4f) - 3;
        DrawRectangle(tx - 4, ty - 4, 9, 5, Color{ 240, 240, 245, 255 });
        DrawRectangle(tx - 4, ty + 1, 9, 2, Color{ 170, 170, 185, 255 });
        if (m.trOnTable) drawTrophy(tx, ty - 3, m.trophyStyle, 1);
        // projecteurs qui balayent le podium et colonnes de flammes au moment du sacre
        float tt = (float)GetTime();
        for (int k = 0; k < 3; k++) DrawEllipse(SX(8.6f + std::sin(tt * 0.8f + k * 2.1f) * 3.f), SY(c + std::cos(tt * 0.6f + k * 2.f) * 7.f), 16, 9, Color{ 255, 250, 210, 34 });
        if (m.trLift) for (int f = 0; f < 4; f++) {
            int fx = f < 2 ? x0 - 3 : x1 + 2, fy = f % 2 ? y1 + 2 : y0 - 4;
            int hgt = 8 + (int)(std::fabs(std::sin(tt * 9.f + f)) * 9.f);
            DrawRectangle(fx - 2, fy - hgt, 4, hgt, Color{ 255, 120, 30, 200 });
            DrawRectangle(fx - 1, fy - hgt + 2, 2, hgt - 2, Color{ 255, 230, 120, 230 });
            DrawCircle(fx, fy - hgt / 2, 6, Color{ 255, 150, 40, 50 });
        }
    }
    drawGoal(0, true, ox, oy);
    // ombres
    V2 bpos; float bz;
    if (replay) { const BallSnap& b = m.rb[m.rpPos]; bpos = V2(b.x, b.y); bz = b.z; }
    else { bpos = m.ball.pos; bz = m.ball.z; }
    if (!replay && m.ballStage == 1) {     // présentoir du ballon au bout du tunnel
        int px = SX(m.pedestal.x), py = SY(m.pedestal.y);
        DrawEllipse(px + 1, py, 4, 1.5f, Color{ 0, 0, 0, 80 });
        DrawRectangle(px - 2, py - (int)(0.95f * PPM * HZ) + 2, 4, (int)(0.95f * PPM * HZ) - 1, Color{ 190, 190, 200, 255 });
        DrawRectangle(px - 3, py - (int)(0.95f * PPM * HZ) + 1, 6, 2, Color{ 230, 200, 80, 255 });
        DrawRectangle(px - 3, py - 1, 6, 2, Color{ 120, 120, 130, 255 });
    }
    std::vector<DrawItem> items;
    bool tunnelHide = !replay && m.inTunnelPhase();
    for (int i = 0; i < 22; i++) {
        float y;
        if (replay) { const Snap& s = m.rp[m.rpPos * 22 + i]; if (s.state == PS_OFF) continue; y = s.y; }
        else { if (!m.pl[i].onPitch) continue; if (tunnelHide && m.pl[i].pos.x < -1.4f) continue; y = m.pl[i].pos.y; }
        items.push_back({ y, 0, i });
        int sx = SX(replay ? m.rp[m.rpPos * 22 + i].x : m.pl[i].pos.x), sy = SY(y);
        DrawEllipse(sx + 1, sy, 4, 1.6f, Color{ 0, 0, 0, 70 });
    }
    items.push_back({ bpos.y, 1, 0 });
    if (!replay && !m.S.training && !(tunnelHide && m.refPos.x < -1.4f)) {   // pas d'arbitre à l'entraînement
        items.push_back({ m.refPos.y, 2, 0 });
        DrawEllipse(SX(m.refPos.x) + 1, SY(m.refPos.y), 4, 1.6f, Color{ 0, 0, 0, 70 });
    }
    // arbitres assistants : un par touche (chacun sa moitié de terrain, à hauteur de l'avant-dernier défenseur),
    // et, si la compétition le prévoit, un derrière chaque but
    static const Match* asMatch = nullptr; static float asY[2] = { 0, 0 };
    std::vector<V2> assist;
    if (!m.S.training && !m.ceremony) {
        if (asMatch != &m) { asMatch = &m; asY[0] = PITCH_L * 0.75f; asY[1] = PITCH_L * 0.25f; }
        std::vector<float> ys;
        for (int i = 0; i < 22; i++) if (m.pl[i].onPitch) ys.push_back(replay ? m.rp[m.rpPos * 22 + i].y : m.pl[i].pos.y);
        std::sort(ys.begin(), ys.end());
        float lo = ys.size() > 2 ? ys[1] : PITCH_L * 0.25f, hi = ys.size() > 2 ? ys[ys.size() - 2] : PITCH_L * 0.75f;
        // les deux assistants restent à hauteur du ballon ; dans leur moitié, ils se calent sur l'avant-dernier défenseur s'il est plus près du but
        float t0 = bpos.y > PITCH_L / 2 ? std::max(bpos.y, hi) : bpos.y, t1 = bpos.y < PITCH_L / 2 ? std::min(bpos.y, lo) : bpos.y;
        t0 = std::min(t0, PITCH_L - 0.5f); t1 = std::max(t1, 0.5f);
        float k = std::min(1.f, GetFrameTime() * 4.f);
        float o0 = asY[0], o1 = asY[1];
        // course le long de la ligne : vitesse limitée (pas de glissade), foulées selon la distance parcourue
        auto stepTo = [&](float& y, float tg) { float d = tg - y, mx = 7.f * GetFrameTime(); y += std::max(-mx, std::min(mx, d * k)); };
        stepTo(asY[0], t0); stepTo(asY[1], t1);
        g_asVel[0] = (asY[0] - o0) / std::max(0.001f, GetFrameTime()); g_asVel[1] = (asY[1] - o1) / std::max(0.001f, GetFrameTime());
        g_asAnim[0] += std::fabs(asY[0] - o0); g_asAnim[1] += std::fabs(asY[1] - o1);
        assist.push_back(V2(-1.1f, asY[0])); assist.push_back(V2(PITCH_W + 1.1f, asY[1]));
        if (m.S.goalAssist) { assist.push_back(V2(PITCH_W / 2 + 6.5f, -0.9f)); assist.push_back(V2(PITCH_W / 2 - 6.5f, PITCH_L + 0.9f)); }
        for (int a = 0; a < (int)assist.size(); a++) { items.push_back({ assist[a].y, 4, a }); DrawEllipse(SX(assist[a].x) + 1, SY(assist[a].y), 4, 1.6f, Color{ 0, 0, 0, 70 }); }
    }
    // président de la République (finale de la Coupe de France)
    if (!replay && m.vipOn) { items.push_back({ m.vipPos.y, 6, 0 }); DrawEllipse(SX(m.vipPos.x) + 1, SY(m.vipPos.y), 4, 1.6f, Color{ 0, 0, 0, 70 }); }
    // soigneurs
    if (!replay && m.medicFor >= 0) for (int k = 0; k < 2; k++) { items.push_back({ m.medicPos[k].y, 5, k }); DrawEllipse(SX(m.medicPos[k].x) + 1, SY(m.medicPos[k].y), 4, 1.6f, Color{ 0, 0, 0, 70 }); }
    DrawEllipse(SX(bpos.x) + (int)(bz * 1.5f), SY(bpos.y) + 1, 2.2f, 1.2f, Color{ 0, 0, 0, 90 });
    std::sort(items.begin(), items.end(), [](const DrawItem& a, const DrawItem& b) { return a.y < b.y; });
    for (auto& it : items) {
        if (it.kind == 1) {
            int x = SX(bpos.x), y = SY(bpos.y) - (int)(bz * PPM * HZ);
            if (!replay && m.ball.owner >= 0 && m.pl[m.ball.owner].state == PS_GKHOLD) y -= 6;
            int bf = ((int)std::floor((bpos.x + bpos.y) * 2.5f)) & 3;
            if (bz > 1.5f) { DrawRectangle(x - 2, y - 3, 5, 5, Color{ 0, 0, 0, 60 }); }
            // traînée des frappes puissantes
            if (!replay && m.ball.owner < 0 && m.ball.vel.len() > 17.f) {
                V2 bv = m.ball.vel * (1.f / m.ball.vel.len());
                for (int k = 1; k <= 4; k++) {
                    int tx = SX(bpos.x - bv.x * k * 0.35f), ty = SY(bpos.y - bv.y * k * 0.35f) - (int)(bz * PPM * HZ);
                    DrawRectangle(tx - 1, ty - 1, 2, 2, Color{ 255, 255, 255, (unsigned char)(150 - k * 32) });
                }
            }
            // filet qui tremble après un but
            if (!replay && m.ball.inNet) {
                float ph = (float)GetTime() * 30.f;
                for (int k = -3; k <= 3; k++) DrawPixel(x + k * 2, y - 3 + (int)(std::sin(ph + k) * 1.5f), Color{ 240, 240, 240, 200 });
            }
            DrawRectangle(x - 1, y - 2, 3, 3, m.S.meteo == 5 ? Color{ 255, 140, 30, 255 } : WHITE);   // ballon orange sous la neige
            DrawRectangle(x - 1, y - 3, 3, 1, Color{ 20, 20, 20, 120 });
            DrawRectangle(x - 2, y - 2, 1, 3, Color{ 20, 20, 20, 120 });
            DrawRectangle(x + 2, y - 2, 1, 3, Color{ 20, 20, 20, 120 });
            DrawRectangle(x - 1, y + 1, 3, 1, Color{ 20, 20, 20, 120 });
            static const int PX[4][2] = { { -1, -2 }, { 1, -2 }, { 1, 0 }, { -1, 0 } };
            DrawRectangle(x + PX[bf][0], y + PX[bf][1], 1, 1, Color{ 40, 40, 40, 255 });
            DrawRectangle(x + PX[(bf + 2) & 3][0], y + PX[(bf + 2) & 3][1], 1, 1, Color{ 90, 90, 90, 255 });
            continue;
        }
        if (it.kind == 6) {     // costume sombre, écharpe tricolore
            Kit vk; vk.shirt = 0x23262E; vk.shirt2 = 0x23262E; vk.shorts = 0x1C1E24; vk.socks = 0x111111;
            int ax = SX(m.vipPos.x), ay = SY(m.vipPos.y);
            bool shaking = false; for (int k = 0; k < 22; k++) if (m.pl[k].state == PS_HAND) shaking = true;
            drawPlayerSprite(ax, ay, vk, 0, 4, shaking ? 0 : 3, shaking ? 0 : (int)(GetTime() * 7), shaking ? PS_HAND : PS_NORMAL, false, 0, 1);
            DrawRectangle(ax - 1, ay - 9, 1, 3, Color{ 0, 85, 164, 255 }); DrawRectangle(ax, ay - 9, 1, 3, WHITE); DrawRectangle(ax + 1, ay - 9, 1, 3, Color{ 239, 65, 53, 255 });
            continue;
        }
        if (it.kind == 5) {     // soigneur : survêtement du club, sacoche avec une croix
            const MPlayer& ip = m.pl[m.medicFor];
            Kit mk; mk.shirt = m.kit[ip.team].shorts == 0xFFFFFF ? 0x1A2A55 : m.kit[ip.team].shorts; mk.shirt2 = mk.shirt; mk.shorts = mk.shirt; mk.socks = 0x151515;
            V2 a = m.medicPos[it.idx];
            int ax = SX(a.x), ay = SY(a.y);
            V2 d = m.medicPhase == 1 ? ip.pos - a : m.medicPhase == 0 ? ip.pos - a : V2(-1, 0);
            int dir = std::fabs(d.x) > std::fabs(d.y) ? (d.x > 0 ? 3 : 2) : (d.y > 0 ? 1 : 0);
            int fr = m.medicPhase == 1 ? 0 : (int)(GetTime() * 9) + it.idx;
            drawPlayerSprite(ax, ay, mk, (it.idx * 2 + 1) % 4, (it.idx * 3 + 1) % 7, dir, fr, PS_NORMAL, false, 0, 1);
            int bx = ax + (it.idx ? -5 : 3), by = ay - 4;
            DrawRectangle(bx, by, 4, 3, WHITE); DrawRectangle(bx + 1, by, 2, 3, Color{ 220, 30, 30, 255 }); DrawRectangle(bx, by + 1, 4, 1, Color{ 220, 30, 30, 255 });
            continue;
        }
        if (it.kind == 4) {
            Kit rk; rk.shirt = 0x151515; rk.shirt2 = 0x151515; rk.shorts = 0x101010; rk.socks = 0x101010;
            V2 a = assist[it.idx];
            int ax = SX(a.x), ay = SY(a.y);
            bool line = it.idx < 2;
            int dir = line ? (it.idx == 0 ? 2 : 3) : (it.idx == 2 ? 0 : 1);        // face au terrain
            int afr = 0;
            if (line && std::fabs(g_asVel[it.idx]) > 0.5f) {                             // en mouvement : court dans le sens du jeu
                afr = (int)(g_asAnim[it.idx] * 1.8f);
                if (std::fabs(g_asVel[it.idx]) > 2.5f) dir = g_asVel[it.idx] > 0 ? 0 : 1;
            }
            bool offs = line && m.msgT > 0 && m.msg == "HORS-JEU" && ((it.idx == 0) == (bpos.y > PITCH_L / 2));
            drawPlayerSprite(ax, ay, rk, (it.idx * 3 + 1) % 4, (it.idx * 5 + 2) % 7, offs ? (it.idx == 0 ? 2 : 3) : dir, offs ? 0 : afr, offs ? PS_CELEB : PS_NORMAL, false, 0, 1);
            if (line) {     // drapeau (rouge et jaune), levé en cas de hors-jeu
                int fx = ax + (it.idx == 0 ? 3 : -5), fy = ay - (offs ? 16 : 8);
                DrawRectangle(fx + 1, fy, 1, offs ? 7 : 5, Color{ 220, 220, 220, 255 });
                DrawRectangle(fx + (it.idx == 0 ? 2 : -2), fy, 3, 2, Color{ 230, 40, 40, 255 });
                DrawRectangle(fx + (it.idx == 0 ? 2 : -2), fy + 2, 3, 1, Color{ 250, 220, 40, 255 });
            }
            continue;
        }
        if (it.kind == 2) {
            // arbitre (tenue noire)
            Kit rk; rk.shirt = 0x151515; rk.shirt2 = 0x151515; rk.shorts = 0x101010; rk.socks = 0x101010;
            int rx = SX(m.refPos.x), ry = SY(m.refPos.y);
            int rframe = m.refVel.len() > 0.4f ? (int)(m.refAnim * 1.8f) : 0;     // foulées au rythme de la course
            bool showCard = m.refCardT > 0 && m.refCardT < 1.8f;
            // coiffure : cheveux courts pour les arbitres hommes, longs (queue de cheval) pour les arbitres femmes
            bool fem = m.S.referee >= 0 && m.S.referee < NUM_REFEREES && REFEREES[m.S.referee].female;
            static const int MS[6][2] = { { 0, 0 }, { 0, 4 }, { 1, 3 }, { 2, 2 }, { 2, 6 }, { 3, 1 } };      // (peau, cheveux) donnant une coupe courte
            int rs = fem ? 1 : MS[(m.S.referee + 6) % 6][0], rh = fem ? 3 : MS[(m.S.referee + 6) % 6][1];
            g_sprLongHair = fem;
            drawPlayerSprite(rx, ry, rk, rs, rh, showCard ? 0 : dirOf(m.refFace), rframe, showCard ? PS_CELEB : PS_NORMAL, false, 0, 1);
            g_sprLongHair = false;
            if (showCard) {
                Color cc = m.refCardType == 2 ? Color{ 230, 30, 30, 255 } : Color{ 255, 220, 0, 255 };
                DrawRectangle(rx + 3, ry - 18, 3, 4, cc);
                DrawRectangleLines(rx + 2, ry - 19, 5, 6, Color{ 0, 0, 0, 120 });
            }
            continue;
        }
        int i = it.idx;
        V2 pos, face; float z; int state, frame;
        if (replay) {
            const Snap& s = m.rp[m.rpPos * 22 + i];
            pos = V2(s.x, s.y); face = V2(s.fx / 100.f, s.fy / 100.f); z = s.z; state = s.state; frame = s.frame;
        } else {
            const MPlayer& p = m.pl[i];
            pos = p.pos; face = p.face; z = p.z; state = p.state; frame = (int)(p.anim * 1.8f);
            if (p.vel.len() < 0.4f) frame = 0;
        }
        const MPlayer& p = m.pl[i];
        const Team& T = m.team(p.team);
        int skin = 0, hair = 0;
        if (p.squad >= 0 && p.squad < (int)T.squad.size()) { skin = T.squad[p.squad].skin; hair = T.squad[p.squad].hair; }
        int dir = dirOf(state == PS_SLIDE ? (replay ? face : p.slideDir) : face);
        if (state == PS_DIVE) dir = face.x >= 0 ? 2 : 3;
        if (state == PS_DIVE && !replay) dir = p.vel.x >= 0 ? 2 : 3;
        int sx = SX(pos.x), sy = SY(pos.y) - (int)(z * PPM * HZ);
        bool onPod = false;
        if (!replay && m.trophyActive) {
            float c = m.trPod.y;
            onPod = pos.x > 6.0f && pos.x < 11.2f && pos.y > c - 8.4f && pos.y < c + 8.4f;
            if (onPod) sy -= 3;
            // haie d'honneur : les joueurs applaudissent
            if (m.trPhase == 4 && p.team != m.trophyTeam && m.trRoute[i].empty() && p.vel.len() < 0.1f) state = ((int)(GetTime() * 7) + i) % 2 ? PS_GKHOLD : PS_NORMAL;
        }
        if (state == PS_FIGHT) {
            // bagarre : les joueurs se bousculent
            int jit = ((int)(GetTime() * 14) + i) % 3 - 1;
            drawPlayerSprite(sx + jit, sy, m.kit[p.team], skin, hair, dir, frame, PS_CELEB, p.gk, m.gkShirt[p.team], 1);
            if (((int)(GetTime() * 6)) % 2) { DrawRectangle(sx - 1, sy - 17, 2, 3, Color{ 255, 60, 60, 255 }); DrawRectangle(sx - 1, sy - 13, 2, 1, Color{ 255, 60, 60, 255 }); }
            continue;
        }
        // poses des célébrations de but
        int pose = POSE_NONE; bool heart = false;
        if (!replay && m.state == MS_GOAL && m.celebScorer >= 0 && p.team == m.pl[m.celebScorer].team && !p.gk) {
            bool still = p.vel.len() < 1.2f;
            bool scorer = i == m.celebScorer;
            int ct = m.celebType;
            if (scorer && ct == 9 && p.st >= 20.f && p.z <= 0.f) { pose = POSE_ARMSOUT; state = PS_CELEB; dir = 0; }
            else if (ct == 10 && still && state == PS_CELEB) { pose = POSE_CRADLE; dir = 0; sx += (int)std::round(std::sin(GetTime() * 4.f) * 1.4f); }
            else if (scorer && ct == 11 && still) { pose = POSE_SHUSH; state = PS_NORMAL; dir = m.celebTarget.y < PITCH_L / 2 ? 1 : 0; if (dir == 1) pose = POSE_POINT; }
            else if (scorer && ct == 12 && still) { pose = POSE_KNEEL; state = PS_CELEB; dir = 0; }
            else if (scorer && ct == 13 && still) { heart = true; state = PS_CELEB; dir = m.celebTarget.y < PITCH_L / 2 ? 1 : 0; }
            else if (scorer && ct == 7 && still) { pose = POSE_POINT; state = PS_NORMAL; dir = 3; }
        }
        if (state == PS_CELEB && pose == POSE_NONE && !heart) {
            // saut de joie, bras en l'air puis « l'avion »
            float ph = (float)GetTime() * 7.f + i * 1.3f;
            sy -= (int)(std::fabs(std::sin(ph)) * 3.f);
            frame = (int)(GetTime() * 4) + i;
        } else if (heart) frame = 0;
        // poussière / gerbes d'eau sous les crampons en pleine course
        if (!replay && (state == PS_NORMAL || state == PS_SLIDE) && p.vel.len() > p.speed * 0.8f && (type == 1 || type == 2 || type == 3 || state == PS_SLIDE)) {
            V2 back = p.vel.norm() * -1.f;
            int ph = (int)(p.anim * 3) % 3;
            Color dc = type == 3 ? Color{ 90, 60, 30, 150 } : type == 2 ? Color{ 200, 220, 255, 130 } : Color{ 200, 190, 150, 110 };
            for (int k = 0; k < 2; k++) {
                float d = 0.5f + (k + ph) * 0.35f;
                DrawRectangle(SX(pos.x + back.x * d) + (k ? 1 : -1), SY(pos.y + back.y * d) - k, 1, 1, dc);
            }
        }
        g_sprLongHair = p.squad >= 0 && p.squad < (int)T.squad.size() && T.squad[p.squad].gender;
        g_sprPose = pose;
        drawPlayerSprite(sx, sy, m.kit[p.team], skin, hair, dir, frame, state == PS_HAND ? PS_NORMAL : state, p.gk, m.gkShirt[p.team], 1);
        g_sprPose = POSE_NONE;
        g_sprLongHair = false;
        if (heart) {   // cœur formé avec les mains au-dessus de la tête
            int hy = sy - 17 - (int)(std::fabs(std::sin(GetTime() * 3.f)) * 2.f);
            Color hc{ 235, 40, 70, 255 };
            DrawRectangle(sx - 2, hy, 2, 1, hc); DrawRectangle(sx + 1, hy, 2, 1, hc); DrawRectangle(sx - 3, hy + 1, 7, 2, hc); DrawRectangle(sx - 2, hy + 3, 5, 1, hc); DrawRectangle(sx - 1, hy + 4, 3, 1, hc); DrawRectangle(sx, hy + 5, 1, 1, hc);
        }
        if (state == PS_HAND) {   // poignée de main : bras tendu vers l'adversaire
            unsigned sk = 0xE0B090; if (p.squad >= 0 && p.squad < (int)T.squad.size()) { static const unsigned SK[4] = { 0xF2C9A0, 0xD9A066, 0xA86B3C, 0x6B4226 }; sk = SK[T.squad[p.squad].skin % 4]; }
            DrawRectangle(sx, sy - 6 + (p.team ? -3 : 1), 1, 3, hexc(sk));
        }
        if (!replay && p.injured && state != PS_DOWN) {   // joueur blessé : croix rouge au-dessus de la tête
            int hx = sx, hy = sy - 16 + (int)(std::sin(GetTime() * 4) * 1.f);
            DrawRectangle(hx - 2, hy, 5, 5, WHITE); DrawRectangle(hx - 1, hy + 1, 3, 3, Color{ 220, 30, 30, 255 }); DrawRectangle(hx, hy, 1, 5, Color{ 220, 30, 30, 255 }); DrawRectangle(hx - 2, hy + 2, 5, 1, Color{ 220, 30, 30, 255 });
        }
        if (!replay && p.anger >= 0.5f && !m.finished && state != PS_DOWN) {   // joueur énervé : fumée au-dessus de la tête
            bool hot = p.anger >= 0.8f;
            int ph = (int)(GetTime() * (hot ? 8 : 4) + i) % 2;
            Color ac = hot ? Color{ 255, 60, 40, 255 } : Color{ 255, 150, 40, 230 };
            DrawRectangle(sx - 3 + ph, sy - 17, 2, 2, ac); DrawRectangle(sx + 2 - ph, sy - 18, 2, 2, ac);
            if (hot) DrawRectangle(sx - 1, sy - 20 - ph, 2, 2, ac);
        }
        if (i == m.captain[p.team] && !p.gk && state != PS_SLIDE && state != PS_DOWN && state != PS_DIVE) DrawRectangle(sx - 3 + (dir == 3 ? 6 : 0), sy - 8, 1, 2, Color{ 255, 220, 40, 255 });   // brassard
        if (!replay && m.trophyActive) {
            if (m.medal[i]) { bool g = p.team == m.trophyTeam; DrawRectangle(sx, sy - 10, 1, 2, Color{ 40, 90, 200, 255 }); DrawRectangle(sx - 1, sy - 8, 2, 2, g ? Color{ 255, 205, 50, 255 } : Color{ 210, 215, 225, 255 }); }
            if (i == m.trHolder) {
                if (m.trLift || state == PS_THROW) drawTrophy(sx, sy - 12 - (int)(std::fabs(std::sin(GetTime() * 5)) * 2), m.trophyStyle, 1);
                else drawTrophy(sx + 3, sy - 4, m.trophyStyle, 1);
            }
        }
        // joueur fatigué : gouttes de sueur
        if (!replay && p.onPitch && p.stamina < 0.55f && !p.gk && state == PS_NORMAL) {
            int ph = (int)(GetTime() * 3 + i) % 3;
            DrawRectangle(sx + 3, sy - 14 + ph, 1, 2, Color{ 150, 200, 255, 220 });
            if (p.stamina < 0.42f) DrawRectangle(sx - 4, sy - 13 + (ph + 1) % 3, 1, 2, Color{ 150, 200, 255, 220 });
        }
        // marqueur joueur contrôlé
        if (!replay && p.human >= 0) {
            Color c = CTRL_COL[p.human];
            int hy = sy - 16;
            DrawTriangle(Vector2{ (float)sx - 3, (float)hy }, Vector2{ (float)sx, (float)hy + 3 }, Vector2{ (float)sx + 3, (float)hy }, c);
            if (p.charging) {
                float pw = std::min(1.f, p.charge / 0.55f);
                DrawRectangle(sx - 5, hy - 4, 10, 2, Color{ 0, 0, 0, 160 });
                DrawRectangle(sx - 5, hy - 4, (int)(10 * pw), 2, Color{ 255, (unsigned char)(255 - 200 * pw), 40, 255 });
            }
        }
    }
    // envahissement de terrain : les supporters courent vers les joueurs et sautent de joie autour d'eux
    if (!replay && m.invasion) {
        std::vector<int> ord; for (int i = 0; i < (int)m.fans.size(); i++) if (m.invT >= m.fans[i].delay) ord.push_back(i);
        std::sort(ord.begin(), ord.end(), [&](int a, int b) { return m.fans[a].pos.y < m.fans[b].pos.y; });
        float tt = (float)GetTime();
        for (int i : ord) {
            const Match::Fan& f = m.fans[i];
            Kit fk; fk.shirt = f.shirt; fk.shirt2 = f.shirt2; fk.shorts = (i % 3) ? 0x34508A : 0x2A2A2A; fk.socks = 0x1A1A1A;
            V2 d = f.target - f.pos;
            bool run = d.len() > 0.4f;
            int x = SX(f.pos.x), y = SY(f.pos.y);
            if (x < -10 || x > MWZ + 10 || y < -16 || y > MHZ + 16) continue;
            DrawEllipse(x + 1, y, 3, 1.3f, Color{ 0, 0, 0, 60 });
            int jump = run ? 0 : (int)(std::fabs(std::sin(tt * 7.f + i)) * 3.f);
            drawPlayerSprite(x, y - jump, fk, f.skin, f.hair, run ? dirOf(d) : 0, run ? (int)(f.anim * 1.8f) : (int)(tt * 4) + i, run ? PS_NORMAL : PS_CELEB, false, 0, 1);
            if (f.flag) {   // drapeau aux couleurs du club
                int fx = x + 3, fy = y - 18 - jump;
                DrawLine(fx, fy, fx, fy + 10, Color{ 220, 220, 220, 255 });
                int wv = (int)(std::sin(tt * 8 + i) * 1.f);
                DrawRectangle(fx + 1, fy + wv, 7, 2, hexc(m.kit[m.invTeam].shirt)); DrawRectangle(fx + 1, fy + 2 + wv, 7, 2, hexc(m.kit[m.invTeam].shirt2 ? m.kit[m.invTeam].shirt2 : 0xFFFFFF));
            }
        }
    }
    // bagarre : nuage de poussière façon dessin animé (membres qui dépassent, étoiles, injures en symboles) ; bousculade : poussière et « !! »
    if (!replay && m.fightT > 0 && !m.duel && m.fightA >= 0 && m.fightB >= 0) {
        V2 mid = (m.pl[m.fightA].pos + m.pl[m.fightB].pos) * 0.5f;
        int cx = SX(mid.x), cy = SY(mid.y) - 7;
        float tt = (float)GetTime();
        if (m.fightLevel >= 2) {
            Color sh1 = hexc(m.kit[m.pl[m.fightA].team].shirt), sh2 = hexc(m.kit[m.pl[m.fightB].team].shirt);
            for (int k = 0; k < 4; k++) {   // bras et jambes qui dépassent
                float a = tt * 9.f + k * 1.57f;
                int lx = cx + (int)(std::cos(a) * 11), ly = cy + (int)(std::sin(a) * 7);
                DrawLine(cx, cy, lx, ly, k % 2 ? sh1 : sh2);
                DrawRectangle(lx - 1, ly - 1, 3, 3, k < 2 ? hexc(0xE0B090) : Color{ 20, 20, 20, 255 });
            }
            for (int k = 0; k < 9; k++) {
                float a = tt * 5.f + k * 0.7f;
                int r = 5 + (k % 3);
                DrawCircle(cx + (int)(std::cos(a) * 6), cy + (int)(std::sin(a * 1.3f) * 4), (float)r, k % 2 ? Color{ 215, 210, 200, 235 } : Color{ 245, 242, 236, 235 });
            }
            for (int k = 0; k < 3; k++) {   // étoiles et symboles
                float a = tt * 3.f + k * 2.1f;
                int sx2 = cx + (int)(std::cos(a) * 14), sy2 = cy - 10 + (int)(std::sin(a) * 3);
                DrawRectangle(sx2 - 2, sy2, 5, 1, Color{ 255, 230, 60, 255 }); DrawRectangle(sx2, sy2 - 2, 1, 5, Color{ 255, 230, 60, 255 });
            }
            static const char* SYM[4] = { "#", "@", "%", "!" };
            drawTextPx(SYM[(int)(tt * 6) % 4], cx - 12, cy - 18, 10, Color{ 255, 80, 60, 255 });
            drawTextPx(SYM[((int)(tt * 6) + 2) % 4], cx + 8, cy - 17, 10, Color{ 255, 80, 60, 255 });
        } else {
            for (int k = 0; k < 5; k++) { float a = tt * 4 + k; DrawCircle(cx + (int)(std::cos(a) * 6), cy + 8 + (int)(std::sin(a) * 2), 2.5f, Color{ 200, 190, 160, 140 }); }
            if ((int)(tt * 5) % 2) drawTextPx("!!", cx - 4, cy - 18, 10, Color{ 255, 220, 60, 255 });
        }
    }
    // joueurs qui quittent le terrain (remplacés, expulsés)
    if (!replay) for (auto& w : m.walkers) {
        V2 d = w.target - w.pos;
        int dir = dirOf(d.len() > 0.01f ? d : V2(-1, 0));
        drawPlayerSprite(SX(w.pos.x), SY(w.pos.y), m.kit[w.team], w.skin, w.hair, dir, (int)(w.anim * 2.2f), PS_NORMAL, w.gk, w.gkShirt, 1);
    }
    // photo officielle : photographes accroupis face aux équipes, flashs
    if (!replay && m.ceremony && m.cerPhase == 0) {
        for (int k = 0; k < 9; k++) {
            float fx = PITCH_W / 2 - 16 + k * 4.f, fy = PITCH_L / 2 + 5.5f + (k % 2) * 0.8f;
            int x = SX(fx), y = SY(fy);
            DrawRectangle(x - 2, y - 5, 4, 4, (k % 2) ? Color{ 240, 130, 30, 255 } : Color{ 60, 170, 80, 255 });
            DrawRectangle(x - 1, y - 7, 3, 2, hexc(0xD9A066));
            DrawRectangle(x - 2, y - 1, 4, 2, Color{ 40, 40, 60, 255 });
            DrawRectangle(x - 1, y - 9, 3, 2, Color{ 20, 20, 20, 255 });
            uint32_t h = (uint32_t)((int)(GetTime() * 5) * 7919 + k * 104729);
            h ^= h >> 13; h *= 0x5bd1e995; h ^= h >> 15;
            if (m.cerT > 2.f && h % 5 == 0) { DrawCircle(x, y - 9, 4, Color{ 255, 255, 255, 190 }); DrawRectangle(x - 1, y - 10, 3, 3, WHITE); }
        }
        if (m.cerT > 2.f && ((int)(GetTime() * 5)) % 7 == 0) DrawRectangle(0, 0, MW, MH, Color{ 255, 255, 255, 40 });
    }
    // 4e arbitre et panneau de remplacement le long de la touche
    if (!replay && m.subBoardT > 0) {
        Kit ok; ok.shirt = 0x151515; ok.shirt2 = 0x151515; ok.shorts = 0x101010; ok.socks = 0x101010;
        int x = SX(-1.4f), y = SY(PITCH_L / 2);
        drawPlayerSprite(x, y, ok, 1, 0, 2, 0, PS_THROW, false, 0, 1);
        DrawRectangle(x - 5, y - 20, 11, 7, Color{ 20, 20, 20, 255 });
        DrawRectangle(x - 4, y - 19, 4, 5, Color{ 230, 50, 40, 255 });
        DrawRectangle(x + 1, y - 19, 4, 5, Color{ 60, 220, 70, 255 });
    }
    drawGoal(PITCH_L, false, ox, oy);

    // visée sur coup de pied arrêté
    if (!replay && m.state == MS_SETPIECE && m.spReady && m.spKicker >= 0 && m.pl[m.spKicker].human >= 0 && m.sp != SP_KICKOFF) {
        Color cc = CTRL_COL[m.pl[m.spKicker].human];
        if (m.sp == SP_FREEKICK) {
            // flèche courbe : trajectoire prévue avec l'effet choisi
            V2 q = m.ball.pos, v = m.spAim * 26.f, prev = q; float spin = m.spCurl * 6.5f;
            float len = 0; const float dtp = 0.02f;
            for (int k = 0; k < 60; k++) {
                v += v.norm().perp() * (spin * dtp); spin *= (1 - 0.9f * dtp);
                q += v * dtp; len += (v * dtp).len();
                if (k % 2 == 0) DrawRectangle(SX(q.x), SY(q.y), k > 40 ? 2 : 1, k > 40 ? 2 : 1, cc);
                if (k == 59) {    // pointe de la flèche
                    V2 d = (q - prev).norm(), pp = d.perp();
                    V2 l = q - d * 1.4f + pp * 0.9f, r = q - d * 1.4f - pp * 0.9f;
                    DrawLine(SX(q.x), SY(q.y), SX(l.x), SY(l.y), cc); DrawLine(SX(q.x), SY(q.y), SX(r.x), SY(r.y), cc);
                }
                prev = q;
            }
            (void)len;
            if (std::fabs(m.spCurl) > 0.02f) drawTextPx(fmt("EFFET %s%d", m.spCurl > 0 ? "+" : "-", (int)std::lround(std::fabs(m.spCurl) * 100)), SX(m.ball.pos.x) + 6, SY(m.ball.pos.y) + 4, 10, cc);
        } else {
            for (int k = 1; k <= 6; k++) {
                V2 q = m.ball.pos + m.spAim * (float)k;
                DrawRectangle(SX(q.x), SY(q.y), 1, 1, cc);
            }
        }
    }

    // fête : confettis et fontaines pyrotechniques après le trophée soulevé
    if (!replay && m.trophyActive && m.trLift) {
        float tt = (float)GetTime();
        Color cc[5] = { hexc(m.kit[m.trophyTeam].shirt), hexc(m.kit[m.trophyTeam].shirt2), Color{ 255, 215, 60, 255 }, Color{ 255, 255, 255, 255 }, hexc(m.kit[m.trophyTeam].shorts) };
        Rng cr(4242);
        for (int k = 0; k < 170; k++) {
            float bx = cr.f() * MW, sp = 14 + cr.f() * 22, ph = cr.f() * 6.28f;
            float y = std::fmod(cr.f() * MH + tt * sp, (float)MH + 10) - 5, x = std::fmod(bx + std::sin(tt * 1.7f + ph) * 7 + MW, (float)MW);
            int w = ((int)(tt * 6 + k) % 3 == 0) ? 1 : 2;
            DrawRectangle((int)x, (int)y, w, 1, cc[k % 5]);
        }
        if (m.trPhase == 3 || (m.trPhase == 4 && m.trT < 3.f)) {
            float c = m.trPod.y;
            for (int f = 0; f < 4; f++) {
                int fx = SX(f < 2 ? 5.4f : 11.8f), fy = SY(c + (f % 2 ? 8.8f : -8.8f));
                for (int k = 0; k < 18; k++) {
                    float a = std::fmod(tt * 1.3f + k * 0.137f, 1.f);
                    float dx = std::sin(k * 2.3f + f) * 5.f * a, dy = -26.f * a + 30.f * a * a;
                    DrawRectangle(fx + (int)dx, fy + (int)dy, 1, 1, a < 0.5f ? Color{ 255, 250, 200, 255 } : Color{ 255, 170, 40, (unsigned char)(255 * (1 - a)) });
                }
            }
        }
    }
    if (!replay) drawShowEffects(m, ox, oy, MWZ, MHZ);
    // météo : pluie, orage (pluie battante, éclairs), neige ; chaleur (air qui ondule)
    {
        int mt = m.S.meteo;
        static float wt = 0; wt += GetFrameTime();
        Rng wr(77);
        if (mt == 3 || mt == 4) {
            int n = mt == 4 ? 260 : 140;
            float slant = mt == 4 ? 1.8f + m.wind.x * 0.4f : 0.8f;
            for (int k = 0; k < n; k++) {
                float bx = wr.f() * MW, by = wr.f() * MH, sp = 60 + wr.f() * 60;
                float y = std::fmod(by + wt * sp * (mt == 4 ? 4.f : 3.f), (float)MH), x = std::fmod(bx - wt * sp * slant + MW * 8, (float)MW);
                DrawLine((int)x, (int)y, (int)x - (mt == 4 ? 3 : 2), (int)y + (mt == 4 ? 7 : 5), Color{ 200, 220, 255, (unsigned char)(mt == 4 ? 200 : 160) });
            }
            if (mt == 4) DrawRectangle(0, 0, MW, MH, Color{ 10, 15, 30, 70 });
            if (m.flashT > 0) DrawRectangle(0, 0, MW, MH, Color{ 240, 245, 255, (unsigned char)(std::min(1.f, m.flashT * 4.f) * ((int)(m.flashT * 20) % 2 ? 150 : 90)) });
        } else if (mt == 5 || (mt < 0 && type == 4)) {
            for (int k = 0; k < 150; k++) {
                float bx = wr.f() * MW, by = wr.f() * MH, sp = 60 + wr.f() * 60;
                float y = std::fmod(by + wt * sp * 0.4f, (float)MH), x = std::fmod(bx + std::sin(wt * 2 + k) * 6 + MW, (float)MW);
                DrawRectangle((int)x, (int)y, k % 5 == 0 ? 2 : 1, k % 5 == 0 ? 2 : 1, Color{ 255, 255, 255, 220 });
            }
            DrawRectangle(0, 0, MW, MH, Color{ 220, 230, 255, 25 });
        } else if (mt == 0 || mt == 1) {
            DrawRectangle(0, 0, MW, MH, Color{ 255, 190, 60, (unsigned char)(mt == 0 ? 34 : 16) });
            if (mt == 0) for (int k = 0; k < 40; k++) {       // air brûlant qui ondule
                float bx = wr.f() * MW, by = wr.f() * MH;
                int y = (int)std::fmod(by - wt * 8.f + MH * 4, (float)MH), x = (int)(bx + std::sin(wt * 3 + k) * 3);
                DrawLine(x, y, x + 6, y, Color{ 255, 240, 200, 26 });
            }
        }
    }
    // nocturne : légère pénombre, halos des projecteurs
    if (m.S.night) {
        DrawRectangle(0, 0, MW, MH, Color{ 5, 10, 40, 60 });
        for (int k = 0; k < 4; k++) {
            float fx = (k & 1) ? PITCH_W + 6 : -6, fy = (k & 2) ? PITCH_L + 6 : -6;
            int x = SX(fx), y = SY(fy);
            if (x > -60 && x < MW + 60 && y > -60 && y < MH + 60) DrawCircleGradient(x, y, 60, Color{ 255, 250, 210, 40 }, Color{ 255, 250, 210, 0 });
        }
    }
    if (Z != 1.f) { rlDrawRenderBatchActive(); rlPopMatrix(); rlPushMatrix(); rlScalef(2, 2, 1); }
    if (!replay) drawMatchScenes(m);
    // ---------------- HUD
    const Team& H = m.team(0); const Team& A = m.team(1);
    int mins = (int)m.clock;
    std::string clk;
    float pe = m.periodEnd;
    if (m.clock > pe) clk = fmt("%d+%d'", (int)pe, std::max(1, (int)(m.clock - pe) + 1));
    else clk = fmt("%d'", std::min(mins + 1, (int)pe));
    if (m.period == 0 && m.clock <= 0.f) clk = "0'";          // avant le coup d'envoi (entrée des équipes, hymnes)
    if (m.shootout) clk = "TAB";
    if (m.S.training) {
        static const char* TN[] = { "", "PENALTIES", "COUPS FRANCS", "CORNERS", "ATTAQUE - DÉFENSE", "GARDIEN : PENALTIES" };
        int done = std::max(0, m.trainTries - 1);
        std::string t1 = std::string("ENTRAÎNEMENT - ") + TN[std::max(0, std::min(5, m.S.training))];
        std::string t2 = m.S.training == 5 ? fmt("Arrêts : %d / %d", m.trainGoals, done) : fmt("Buts : %d / %d", m.trainGoals, done);
        if (done > 0) t2 += fmt("  (%d%%)", m.trainGoals * 100 / done);
        int w = std::max(textWidth(t1, 10), textWidth(t2, 10)) + 10;
        DrawRectangle(3, 3, w, 23, Color{ 0, 0, 0, 150 });
        DrawRectangle(3, 3, 2, 23, Color{ 255, 230, 60, 255 });
        drawTextPx(t1, 8, 4, 10, WHITE);
        drawTextPx(t2, 8, 14, 10, Color{ 255, 230, 60, 255 });
    } else {
        int x = 3;
        std::string sc = fmt("%d-%d", m.score[0], m.score[1]);
        int reds[2] = { 0, 0 };
        for (int i = 0; i < 22; i++) if (m.pl[i].sentOff) reds[m.pl[i].team]++;
        for (auto& e : m.events) (void)e;
        int w = 4 + 4 + textWidth(H.shortName, 10) + 4 + textWidth(sc, 10) + 4 + 4 + textWidth(A.shortName, 10) + 6 + textWidth(clk, 10) + 4 + (reds[0] + reds[1]) * 4 + (reds[0] ? 2 : 0) + (reds[1] ? 2 : 0);
        DrawRectangle(x, 3, w, 12, Color{ 0, 0, 0, 150 });
        x += 2;
        DrawRectangle(x, 5, 2, 8, hexc(m.kit[0].shirt)); DrawRectangle(x + 2, 5, 1, 8, hexc(m.kit[0].shirt2)); x += 5;
        drawTextPx(H.shortName, x, 4, 10, WHITE); x += textWidth(H.shortName, 10) + 4;
        for (int k = 0; k < reds[0]; k++) { DrawRectangle(x - 2 + k * 4, 5, 3, 7, Color{ 230, 30, 30, 255 }); } if (reds[0]) x += reds[0] * 4 + 2;
        drawTextPx(sc, x, 4, 10, Color{ 255, 230, 60, 255 }); x += textWidth(sc, 10) + 4;
        DrawRectangle(x, 5, 2, 8, hexc(m.kit[1].shirt)); DrawRectangle(x + 2, 5, 1, 8, hexc(m.kit[1].shirt2)); x += 5;
        drawTextPx(A.shortName, x, 4, 10, WHITE); x += textWidth(A.shortName, 10) + 6;
        for (int k = 0; k < reds[1]; k++) { DrawRectangle(x - 4 + k * 4, 5, 3, 7, Color{ 230, 30, 30, 255 }); } if (reds[1]) x += reds[1] * 4 + 2;
        drawTextPx(clk, x, 4, 10, Color{ 200, 255, 200, 255 });
        // temps additionnel annoncé
        if (m.boardDone && m.boardN > 0 && !m.shootout && m.clock >= pe - 1.f) {
            int ax = x + textWidth(clk, 10) + 6;
            std::string at = fmt("+%d", m.boardN);
            DrawRectangle(ax - 2, 3, textWidth(at, 10) + 6, 12, Color{ 20, 120, 40, 230 });
            drawTextPx(at, ax + 1, 4, 10, WHITE);
        }
    }
    // panneau lumineux de remplacement (numéro sortant en rouge, entrant en vert)
    if (m.subBoardT > 0 && !replay) {
        float t = 3.4f - m.subBoardT;
        float up = std::min(1.f, t * 3.f) * std::min(1.f, m.subBoardT * 3.f);
        int bx = MW - 132, by = 34 - (int)((1.f - up) * 80);
        const Team& T = m.team(m.subBoardTeam);
        DrawRectangle(bx - 4, by - 4, 128, 56, Color{ 0, 0, 0, (unsigned char)(150 * up) });
        DrawRectangle(bx, by, 120, 30, Color{ 15, 15, 15, 255 });
        DrawRectangleLines(bx, by, 120, 30, Color{ 200, 200, 200, 255 });
        std::string o = fmt("%d", m.subBoardOut), n = fmt("%d", m.subBoardIn);
        drawTextPx(o, bx + 30 - textWidth(o, 20) / 2, by + 4, 20, Color{ 255, 60, 50, 255 });
        drawTextPx(n, bx + 90 - textWidth(n, 20) / 2, by + 4, 20, Color{ 70, 255, 90, 255 });
        DrawTriangle(Vector2{ (float)bx + 56, (float)by + 10 }, Vector2{ (float)bx + 56, (float)by + 20 }, Vector2{ (float)bx + 64, (float)by + 15 }, Color{ 240, 200, 60, 255 });
        drawTextPx(fitText("- " + m.subBoardOutName, 120, 10), bx, by + 32, 10, Color{ 255, 140, 130, 255 });
        drawTextPx(fitText("+ " + m.subBoardInName, 120, 10), bx, by + 42, 10, Color{ 140, 255, 150, 255 });
        DrawRectangle(bx - 4, by - 4, 3, 56, hexc(m.kit[m.subBoardTeam].shirt));
        (void)T;
    }
    // panneau du 4e arbitre (animation)
    if (m.boardT > 0 && !replay) {
        float t = 5.f - m.boardT;
        float up = std::min(1.f, t * 2.5f) * std::min(1.f, m.boardT * 2.5f);
        bool left = SX(m.ball.pos.x) > MW / 2;              // du côté opposé à l'action
        int bx = left ? 10 : MW - 80, by = MH / 2 - 30 - (int)((1.f - up) * 60);
        DrawRectangle(bx - 6, by - 6, 72, 52, Color{ 0, 0, 0, (unsigned char)(120 * up) });
        Kit ok; ok.shirt = 0x151515; ok.shirt2 = 0x151515; ok.shorts = 0x101010; ok.socks = 0x101010;
        drawPlayerSprite(bx + 12, by + 40, ok, 1, 0, 0, 0, PS_THROW, false, 0, 2);
        // panneau lumineux tenu à bout de bras
        DrawRectangle(bx + 24, by - 2, 36, 22, Color{ 20, 20, 20, 255 });
        DrawRectangleLines(bx + 24, by - 2, 36, 22, Color{ 200, 200, 200, 255 });
        DrawRectangle(bx + 38, by + 20, 3, 10, Color{ 90, 90, 90, 255 });
        bool blink = ((int)(t * 4)) % 2 == 0;
        std::string n = fmt("+%d", m.boardN);
        drawTextPx(n, bx + 42 - textWidth(n, 10) / 2, by + 4, 10, blink ? Color{ 80, 255, 90, 255 } : Color{ 255, 70, 60, 255 });
    }
    if (m.S.hasFirstLeg) {
        std::string ag = fmt("Cumul %d-%d", m.score[0] + m.S.aggHome, m.score[1] + m.S.aggAway);
        DrawRectangle(3, 16, textWidth(ag, 10) + 6, 11, Color{ 0, 0, 0, 120 });
        drawTextPx(ag, 6, 16, 10, Color{ 220, 220, 220, 255 });
    }
    if (m.shootout) {
        for (int t = 0; t < 2; t++) {
            int y = 30 + t * 10;
            DrawRectangle(3, y - 1, 90, 10, Color{ 0, 0, 0, 120 });
            drawTextPx(t == 0 ? H.shortName : A.shortName, 7, y, 10, WHITE);
            for (int k = 0; k < (int)m.penLog[t].size(); k++) {
                Color c = m.penLog[t][k] ? Color{ 60, 220, 60, 255 } : Color{ 230, 50, 50, 255 };
                DrawRectangle(34 + k * 6, y + 2, 4, 4, c);
            }
        }
    }
    // nom du joueur contrôlé / porteur
    int showP = -1;
    for (int c = 0; c < NUM_INPUTS; c++) if (m.ctrlPlayer[c] >= 0 && m.S.side[c] >= 0) { showP = m.ctrlPlayer[c]; break; }
    if (m.ball.owner >= 0) showP = m.ball.owner;
    if (m.trophyActive) showP = -1;
    if (showP >= 0 && !replay) {
        const MPlayer& p = m.pl[showP];
        const Team& T = m.team(p.team);
        std::string nm = p.squad < (int)T.squad.size() ? fmt("%d %s", T.squad[p.squad].num, T.squad[p.squad].name.c_str()) : "";
        int w = textWidth(nm, 10) + 8;
        bool tired = p.stamina < 0.6f;
        int extra = (p.yellow > 0 ? 8 : 0) + 22;
        DrawRectangle(3, MH - 14, w + extra, 11, Color{ 0, 0, 0, 140 });
        DrawRectangle(3, MH - 14, 2, 11, hexc(m.kit[p.team].shirt));
        drawTextPx(nm, 8, MH - 13, 10, WHITE);
        int x = 3 + w;
        if (p.yellow > 0) { DrawRectangle(x, MH - 13, 5, 8, Color{ 255, 220, 0, 255 }); DrawRectangleLines(x, MH - 13, 5, 8, Color{ 0, 0, 0, 150 }); x += 8; }
        // jauge d'endurance (rouge et clignotante quand le joueur est fatigué)
        Color sc = p.stamina > 0.75f ? Color{ 90, 220, 90, 255 } : p.stamina > 0.6f ? Color{ 240, 200, 60, 255 } : Color{ 240, 70, 60, 255 };
        if (!tired || ((int)(GetTime() * 4)) % 2) {
            DrawRectangle(x, MH - 10, 18, 4, Color{ 0, 0, 0, 180 });
            DrawRectangle(x, MH - 10, (int)(18 * std::max(0.f, std::min(1.f, p.stamina))), 4, sc);
        }
    }
    if (!replay) drawTossUI(m);
    if (!replay) drawShootUI(m);
    // bandeau de la cérémonie
    if (!replay && m.trophyActive) {
        const Team& WT = m.team(m.trophyTeam);
        static const char* PH[] = { "Cérémonie de remise du trophée", "Médailles des finalistes", "Médailles des vainqueurs", "", "Haie d'honneur", "Tour d'honneur" };
        std::string sub = m.trPhase == 3 ? (m.trophyKind == 1 ? WT.name + " champion !" : WT.name + " remporte le trophée !") : PH[std::max(0, std::min(5, m.trPhase))];
        std::string ttl = m.trophyTitle;
        if (m.trPhase == 3 && m.trLift) {
            std::string l1 = m.trophyKind == 1 ? "CHAMPION" : "VAINQUEUR", l2 = fitText(WT.name, MW - 90, 20);
            int pw = std::max(std::max(textWidth(l2, 20), textWidth(ttl, 10)), textWidth(l1, 10)) + 50, ph = 46, px = std::max(MW / 2 - pw / 2, MW - pw - 8), py = 22;
            float a = std::min(1.f, (m.trT - 1.4f) * 3.f);
            DrawRectangle(px, py, pw, ph, Color{ 10, 14, 40, (unsigned char)(215 * a) });
            DrawRectangle(px, py, pw, 2, Color{ 255, 205, 60, 255 }); DrawRectangle(px, py + ph - 2, pw, 2, Color{ 255, 205, 60, 255 });
            DrawRectangle(px, py, 3, ph, hexc(m.kit[m.trophyTeam].shirt));
            drawTrophy(px + 20, py + ph - 8, m.trophyStyle, 2);
            drawTextPx(l1, px + 40, py + 4, 10, Color{ 255, 215, 90, 255 });
            drawTextPx(l2, px + 40, py + 13, 20, WHITE);
            drawTextPx(fitText(ttl, pw - 46, 10), px + 40, py + 33, 10, Color{ 200, 210, 235, 255 });
        } else {
            int pw = std::max(textWidth(ttl, 10), textWidth(sub, 10)) + 34, px = MW / 2 - pw / 2, py = 20;
            DrawRectangle(px, py, pw, 24, Color{ 10, 14, 40, 200 });
            DrawRectangle(px, py, pw, 1, Color{ 255, 205, 60, 255 });
            drawTrophy(px + 10, py + 20, m.trophyStyle, 1);
            drawTextPx(ttl, px + 22, py + 2, 10, Color{ 255, 215, 90, 255 });
            drawTextPx(sub, px + 22, py + 12, 10, WHITE);
        }
        std::string hint = "Entrée : passer";
        drawTextPx(hint, 4, MH - 12, 10, Color{ 200, 200, 210, 170 });
    }
    // commentaires (deux lignes au plus)
    if (m.S.commentary && m.comT > 0 && !m.comLine.empty() && !replay && !((m.ceremony || m.tossKind) && m.tossUI > 0) && m.shootUI <= 0 && !(m.duel && m.fightT > 0)) {   // pas de commentaire par-dessus le pile ou face / le choix des tireurs
        int maxw = MW - 80;
        std::vector<std::string> lines; std::string cur, word;
        for (size_t i = 0; i <= m.comLine.size(); i++) {
            char ch = i < m.comLine.size() ? m.comLine[i] : ' ';
            if (ch == ' ') { std::string c2 = cur.empty() ? word : cur + " " + word; if (textWidth(c2, 10) > maxw && !cur.empty()) { lines.push_back(cur); cur = word; } else cur = c2; word.clear(); }
            else word += ch;
        }
        if (!cur.empty()) lines.push_back(cur);
        if (lines.size() > 2) { lines.resize(2); lines[1] = fitText(lines[1] + "...", maxw, 10); }
        int w = 0; for (auto& l : lines) w = std::max(w, textWidth(l, 10));
        w += 10;
        int h = 11 * (int)lines.size() + 2;
        int y = MH - 18 - h, x0 = 4;
        DrawRectangle(x0, y, w, h, Color{ 0, 0, 0, 170 });
        DrawRectangle(x0, y, 2, h, Color{ 240, 200, 60, 255 });
        for (size_t k = 0; k < lines.size(); k++) drawTextPx(lines[k], x0 + 5, y + 1 + 11 * (int)k, 10, Color{ 240, 240, 240, 255 });
    }
    // logo de la chaîne
    if (m.S.tv && !m.S.channel.empty()) {
        int w = textWidth(m.S.channel, 10) + 8;
        DrawRectangle(MW - w - 3, 3, w, 11, Color{ 180, 20, 30, 200 });
        drawTextPx(m.S.channel, MW - w + 1, 4, 10, WHITE);
    }
    // radar
    if (radar) {
        int rw = 26, rh = 40, rx = MW - rw - 4, ry = MH - rh - 4;
        DrawRectangle(rx - 1, ry - 1, rw + 2, rh + 2, Color{ 0, 0, 0, 120 });
        DrawRectangleLines(rx - 1, ry - 1, rw + 2, rh + 2, Color{ 255, 255, 255, 120 });
        DrawLine(rx, ry + rh / 2, rx + rw, ry + rh / 2, Color{ 255, 255, 255, 60 });
        for (int i = 0; i < 22; i++) {
            V2 p;
            if (replay) { const Snap& s = m.rp[m.rpPos * 22 + i]; if (s.state == PS_OFF) continue; p = V2(s.x, s.y); }
            else { if (!m.pl[i].onPitch) continue; p = m.pl[i].pos; }
            int x = rx + (int)(p.x / PITCH_W * rw), y = ry + (int)(p.y / PITCH_L * rh);
            DrawRectangle(x, y, 1, 1, hexc(m.pl[i].gk ? m.gkShirt[m.pl[i].team] : m.kit[m.pl[i].team].shirt));
        }
        DrawRectangle(rx + (int)(bpos.x / PITCH_W * rw), ry + (int)(bpos.y / PITCH_L * rh), 1, 1, WHITE);
    }
    if (replay) {
        if (((int)(GetTime() * 2)) % 2) drawTextShadow("REPLAY", MW - 44, 4, 10, Color{ 255, 80, 80, 255 });
    }
    // combat (façon hockey rétro) : ring entouré du public, deux joueurs en gros plan, jauges d'énergie, onomatopées
    if (m.duel && m.fightT > 0 && !replay && m.fightA >= 0 && m.fightB >= 0) {
        int pw = 250, ph = 120, px = MW / 2 - pw / 2, py = MH / 2 - 70;
        float tt = (float)GetTime();
        DrawRectangle(px + 3, py + 3, pw, ph, Color{ 0, 0, 0, 130 });
        DrawRectangleGradientV(px, py, pw, ph, Color{ 40, 14, 14, 245 }, Color{ 16, 6, 10, 245 });
        fxCrowd(px + 2, py + 12, pw - 4, 26, hexc(m.kit[0].shirt), hexc(m.kit[1].shirt), tt, 0.3f, 77, 3);
        DrawRectangle(px + 2, py + 12, pw - 4, 26, Color{ 0, 0, 0, 90 });
        DrawRectangle(px + 6, py + ph - 26, pw - 12, 22, Color{ 60, 120, 60, 255 });              // pelouse
        for (int k = 0; k < 3; k++) DrawLine(px + 6, py + 46 + k * 10, px + pw - 6, py + 46 + k * 10, Color{ 230, 60, 50, 180 });   // cordes
        DrawRectangleLines(px, py, pw, ph, Color{ 255, 80, 60, 255 });
        std::string ttl = "BAGARRE !";
        DrawRectangle(px + pw / 2 - 34, py - 4, 68, 13, Color{ 200, 30, 30, 255 });
        drawTextPx(ttl, px + pw / 2 - textWidth(ttl, 10) / 2, py - 3, 10, WHITE);
        int f[2] = { m.fightA, m.fightB };
        for (int k = 0; k < 2; k++) {
            const MPlayer& p = m.pl[f[k]];
            const Team& T = m.team(p.team);
            int skin = 0, hair = 0; if (p.squad >= 0 && p.squad < (int)T.squad.size()) { skin = T.squad[p.squad].skin; hair = T.squad[p.squad].hair; }
            int cx = px + (k == 0 ? 100 : pw - 100), cy = py + ph - 8;
            int shake = m.duelHitFx[k] > 0 ? (((int)(tt * 40)) % 2 ? 2 : -2) : 0;
            int st = m.duelAtk[k] ? PS_CELEB : (m.duelGuard[k] ? PS_THROW : PS_NORMAL);
            int bob = (int)(std::sin(tt * 6 + k) * 1.5f);
            DrawEllipse(cx, cy, 14, 3, Color{ 0, 0, 0, 90 });
            drawPlayerSprite(cx + shake + (m.duelAtk[k] ? (k == 0 ? 7 : -7) : 0), cy + bob, m.kit[p.team], skin, hair, k == 0 ? 2 : 3, 0, st, p.gk, m.gkShirt[p.team], 4);
            if (m.duelHitFx[k] > 0) {   // impact : étoile et onomatopée
                static const char* POW[3] = { "PAF !", "BAM !", "POW !" };
                int hx = cx + (k == 0 ? 4 : -4), hy = cy - 40;
                DrawPoly(Vector2{ (float)hx, (float)hy }, 8, 10, tt * 200, Color{ 255, 230, 80, 230 });
                drawTextPx(POW[(f[k] + (int)(m.duelT * 3)) % 3], hx - 14, hy - 22, 10, Color{ 255, 255, 255, 255 });
            }
            float hp = std::max(0.f, m.duelHp[k]) / 100.f;
            if (hp < 0.3f) for (int s2 = 0; s2 < 3; s2++) { float a = tt * 4 + s2 * 2.1f; DrawRectangle(cx + (int)(std::cos(a) * 9) - 1, cy - 54 + (int)(std::sin(a) * 2), 3, 3, Color{ 255, 230, 60, 255 }); }   // sonné
            // jauge d'énergie
            int bx = k == 0 ? px + 8 : px + pw - 8 - 100;
            DrawRectangle(bx - 1, py + 41, 102, 9, Color{ 0, 0, 0, 220 });
            DrawRectangle(bx, py + 42, (int)(100 * hp), 7, hp > 0.5f ? Color{ 90, 220, 90, 255 } : hp > 0.25f ? Color{ 240, 200, 60, 255 } : Color{ 240, 60, 50, 255 });
            DrawRectangle(bx, py + 42, (int)(100 * hp), 2, Color{ 255, 255, 255, 80 });
            drawTextPx(fitText(m.playerName(f[k]), 100, 10), bx, py + 52, 10, WHITE);
            if (m.duelGuard[k]) drawTextPx(m.duelGuard[k] == 1 ? "garde haute" : "garde basse", bx, py + 62, 10, Color{ 170, 185, 220, 255 });
        }
        std::string h = "Passe : coup haut  Tir : coup bas  Haut/Bas : garde";
        DrawRectangle(MW / 2 - textWidth(h, 10) / 2 - 4, py + ph + 3, textWidth(h, 10) + 8, 12, Color{ 0, 0, 0, 170 });
        drawTextPx(h, MW / 2 - textWidth(h, 10) / 2, py + ph + 4, 10, WHITE);
    }
    // tour d'honneur
    if (m.lapActive && !replay) {
        std::string t1 = "TOUR D'HONNEUR", t2 = m.team(m.lapTeam).name + " salue ses supporters";
        int w = std::max(textWidth(t1, 10), textWidth(t2, 10)) + 16;
        DrawRectangle(MW / 2 - w / 2, 20, w, 24, Color{ 10, 14, 40, 200 });
        DrawRectangle(MW / 2 - w / 2, 20, 3, 24, hexc(m.kit[m.lapTeam].shirt));
        drawTextCentered(t1, MW / 2, 22, 10, Color{ 255, 215, 90, 255 });
        drawTextCentered(t2, MW / 2, 32, 10, WHITE);
        drawTextPx("Entrée : passer", 4, MH - 12, 10, Color{ 200, 200, 210, 170 });
    }
    if(!m.S.training&&!replay){int chant=supportersLiveChant(m.S,m.score[0],m.score[1],m.clock);std::string label=std::string(supporterAtmosphereName(m.S.supporterAtmosphere,chant==CH_PROTEST?2:0))+" / "+supporterChantName(chant);int width=textWidth(label,5)+8;int chantY=m.S.tv&&!m.S.channel.empty()?17:3;DrawRectangle(MW-width-3,chantY,width,9,Color{8,14,28,200});drawTextPx(label,MW-width+1,chantY+1,5,chant==CH_PROTEST||chant==CH_WHISTLES?Color{255,130,80,255}:Color{255,220,100,255});if(m.S.supporterTifo>=0&&(m.clock<8||m.ceremony)){drawTextPx("TIFO : FIERTÉ DU CLUB",4,20,5,Color{255,230,130,255});}}
    // grand « BUT ! » animé au moment du but
    if (!replay && m.state == MS_GOAL && m.stateT < 1.8f && m.lastScorerTeam >= 0) {
        float u = std::min(1.f, m.stateT / 0.35f);
        int size = 10 + (int)(u * 30.f);
        float tt = (float)GetTime();
        int shake = m.stateT < 0.6f ? (((int)(tt * 30)) % 2 ? 1 : -1) : 0;
        std::string b = "BUT !";
        int w = textWidth(b, size);
        Color tc = ((int)(tt * 8)) % 2 ? Color{ 255, 225, 90, 255 } : WHITE;
        Color kc = hexc(m.kit[m.lastScorerTeam].shirt);
        drawTextPx(b, MW / 2 - w / 2 + 2 + shake, 38 + 2, size, kc);
        drawTextPx(b, MW / 2 - w / 2 + shake, 38, size, tc);
    }
    // habillage TV : bandeaux des remplacements et des cartons
    if (!replay && m.S.tv && !m.S.training) {
        auto band = [&](const std::string& top, const std::string& line, Color accent, float a) {
            int w = std::max(textWidth(top, 5), textWidth(line, 5)) + 14, x = 4 - (int)((1.f - a) * (w + 10)), y = MH - 64;
            DrawRectangle(x, y, w, 20, Color{ 10, 14, 30, 225 });
            DrawRectangle(x, y, 3, 20, accent);
            drawTextPx(top, x + 7, y + 2, 5, Color{ 255, 225, 90, 255 });
            drawTextPx(line, x + 7, y + 11, 5, WHITE);
        };
        if (m.subBoardT <= 0 && m.refCardT > 0 && m.refCardFor >= 0 && m.refCardFor < 22) {
            float a = std::min(1.f, m.refCardT * 4.f);
            const MPlayer& cp = m.pl[m.refCardFor];
            band(m.refCardType == 2 ? "CARTON ROUGE" : "CARTON JAUNE", m.playerName(m.refCardFor) + " (" + m.team(cp.team).shortName + ")", m.refCardType == 2 ? Color{ 230, 40, 40, 255 } : Color{ 255, 220, 0, 255 }, a);
        }
    }
    // messages
    if (m.msgT > 0 && !m.msg.empty() && !replay && !(m.state == MS_GOAL && m.stateT < 1.8f) && !(m.ceremony && m.tossUI > 0) && m.tossKind == 0 && !m.lapActive && !(m.duel && m.fightT > 0)) {
        int y = 40;
        int w = std::max(textWidth(m.msg, 10), textWidth(m.msg2, 10)) + 16;
        DrawRectangle(MW / 2 - w / 2, y - 4, w, m.msg2.empty() ? 18 : 30, Color{ 0, 0, 30, 170 });
        Color c = Color{ 255, 240, 80, 255 };
        if (m.cardShow == 2 && m.msg.find("ROUGE") != std::string::npos) c = Color{ 255, 70, 70, 255 };
        drawTextCentered(m.msg, MW / 2, y, 10, c);
        if (!m.msg2.empty()) drawTextCentered(m.msg2, MW / 2, y + 12, 10, WHITE);
        if (m.cardShow && (m.msg.find("CARTON") != std::string::npos || m.msg.find("ROUGE") != std::string::npos)) {
            Color cc = m.cardShow == 1 ? Color{ 255, 220, 0, 255 } : Color{ 230, 30, 30, 255 };
            DrawRectangle(MW / 2 - w / 2 - 9, y - 2, 6, 9, cc);
        }
    }
    rlDrawRenderBatchActive();
    rlPopMatrix();
}

// portrait (fiche joueur, manager, vie privée) : visage, cheveux (courts ou longs), maillot du club
void drawPortrait(int x, int y, int size, int skin, int hair, int gender, const Kit& kit, unsigned seed) {
    float u = size / 32.f;
    auto R = [&](float a, float b, float w, float h, Color c) { DrawRectangle(x + (int)(a * u), y + (int)(b * u), std::max(1, (int)(w * u + 0.5f)), std::max(1, (int)(h * u + 0.5f)), c); };
    Color sk = hexc(SKIN[skin % NUM_SKINS]), skd = Color{ (unsigned char)(sk.r * 0.82f), (unsigned char)(sk.g * 0.82f), (unsigned char)(sk.b * 0.82f), 255 };
    Color hc = hexc(HAIR[hair % NUM_HAIRS]);
    Color sh = hexc(kit.shirt), sh2 = hexc(kit.shirt2 != kit.shirt ? kit.shirt2 : (colorBright(kit.shirt) > 0.55f ? 0x202020 : 0xF2F2F2));
    R(0, 0, 32, 32, Color{ 40, 56, 96, 255 });
    DrawRectangleGradientV(x, y, size, size, Color{ 70, 100, 160, 255 }, Color{ 24, 34, 60, 255 });
    if (gender) R(8, 7, 16, 19, hc);                                  // cheveux longs derrière le visage
    R(4, 25, 24, 7, sh); R(13, 25, 6, 3, sh2);                        // épaules, col
    R(13, 20, 6, 6, skd);                                             // cou
    R(10, 8, 12, 14, sk);                                             // visage
    R(9, 12, 1, 4, skd); R(22, 12, 1, 4, skd);                        // oreilles
    unsigned v = seed * 2654435761u;
    int style = (int)(v >> 7) % 3;
    if (gender) { R(9, 5, 14, 4, hc); R(9, 8, 2, 10, hc); R(21, 8, 2, 10, hc); }
    else if (style == 0) { R(10, 5, 12, 4, hc); }                     // courts
    else if (style == 1) { R(9, 5, 14, 5, hc); R(9, 9, 1, 3, hc); R(22, 9, 1, 3, hc); }
    else { R(11, 6, 10, 2, hc); }                                     // rasés
    R(12, 13, 2, 2, Color{ 30, 24, 24, 255 }); R(18, 13, 2, 2, Color{ 30, 24, 24, 255 });   // yeux
    R(12, 12, 2, 1, Color{ hc.r, hc.g, hc.b, 200 }); R(18, 12, 2, 1, Color{ hc.r, hc.g, hc.b, 200 });
    R(15, 15, 2, 3, skd);                                             // nez
    R(14, 19, 4, 1, gender ? Color{ 190, 70, 80, 255 } : Color{ 120, 60, 50, 255 });   // bouche
    if (!gender && (v >> 12) % 4 == 0) R(11, 19, 10, 3, Color{ hc.r, hc.g, hc.b, 170 });   // barbe
    DrawRectangleLines(x, y, size, size, Color{ 240, 200, 60, 200 });
}
