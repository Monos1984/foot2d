// Décors animés des cérémonies (Ballon d'or, soirée du championnat, fête des supporters, salle des trophées)
// Tout est dessiné par le jeu, en gros pixels, sans fichier externe.
#include "render.h"
#include <cmath>
#include <algorithm>

static uint32_t hash32(uint32_t h) { h ^= h >> 16; h *= 0x7feb352d; h ^= h >> 15; h *= 0x846ca68b; h ^= h >> 16; return h; }
static float hf(uint32_t h) { return (hash32(h) & 0xFFFF) / 65535.f; }
static Color shade(Color c, float f) { return Color{ (unsigned char)std::min(255.f, c.r * f), (unsigned char)std::min(255.f, c.g * f), (unsigned char)std::min(255.f, c.b * f), c.a }; }

// Public vu de face, en gradins : rangs du fond plus petits et plus sombres. excite 0 assis ... 1 debout, bras levés, écharpes.
void fxCrowd(int x, int y, int w, int h, Color c1, Color c2, float t, float excite, unsigned seed, int cell) {
    static const unsigned SK[6] = { 0xF2C9A0, 0xD9A066, 0xA86B3C, 0x6B4226, 0xFFE0C8, 0x8A5A34 };
    static const unsigned HR[5] = { 0x1A1A1A, 0x5A3A1A, 0xE8C35A, 0xB5502A, 0x3B2A1A };
    static const unsigned MISC[6] = { 0x2B2D42, 0x8D99AE, 0xD9D9D9, 0x3A3A3A, 0x6D597A, 0x1D3557 };
    int rows = std::max(1, h / cell);
    for (int r = 0; r < rows; r++) {
        float depth = rows > 1 ? (float)r / (rows - 1) : 1.f;          // 0 fond ... 1 premier rang
        int cs = std::max(2, (int)std::round(cell * (0.65f + 0.45f * depth)));
        float light = 0.55f + 0.45f * depth;
        int ry = y + r * cell;
        int n = w / cs + 1;
        for (int k = 0; k < n; k++) {
            uint32_t id = seed * 7919u + r * 104729u + k * 1299709u;
            int fx = x + k * cs + ((r % 2) ? cs / 2 : 0);
            if (fx > x + w - cs) continue;
            float ph = hf(id) * 6.28f;
            bool active = hf(id + 1) < excite;
            int jump = active ? (int)std::round(std::max(0.f, std::sin(t * (6.f + hf(id + 2) * 3.f) + ph)) * cs * 0.45f) : 0;
            float pick = hf(id + 3);
            Color body = pick < 0.45f ? c1 : pick < 0.75f ? c2 : hexc(MISC[hash32(id) % 6]);
            body = shade(body, light);
            Color skin = shade(hexc(SK[hash32(id + 4) % 6]), light), hair = shade(hexc(HR[hash32(id + 5) % 5]), light);
            int bw = std::max(2, cs - 1), bh = std::max(2, cs);
            int by = ry + cs / 2 - jump;
            DrawRectangle(fx, by, bw, bh, body);                                       // torse
            int hs = std::max(1, bw * 2 / 3);
            DrawRectangle(fx + (bw - hs) / 2, by - hs, hs, hs, skin);                  // tête
            DrawRectangle(fx + (bw - hs) / 2, by - hs, hs, std::max(1, hs / 3), hair);
            if (active && excite > 0.3f) {                                             // bras levés
                int arm = std::max(1, cs / 3);
                bool up = std::sin(t * 5.f + ph) > -0.3f;
                DrawRectangle(fx - arm / 2, by - (up ? hs + arm : 0), arm, up ? hs + arm : bh / 2, skin);
                DrawRectangle(fx + bw - arm / 2, by - (up ? hs + arm : 0), arm, up ? hs + arm : bh / 2, skin);
            }
            if (excite > 0.5f && hf(id + 6) < 0.18f) {                                 // écharpe tendue
                int sw = cs * 2, sy = by - hs - std::max(2, cs / 2);
                for (int q = 0; q < sw; q += 2) DrawRectangle(fx - cs / 2 + q, sy, 2, std::max(1, cs / 3), ((q / 2) % 2) ? shade(c1, light) : shade(c2, light));
            }
        }
    }
    // drapeaux géants qui flottent au-dessus du public
    if (excite > 0.35f) for (int k = 0; k < std::max(1, w / 120); k++) {
        int fx = x + 30 + k * 120 + (int)(hf(seed + k * 31) * 50), fy = y + (int)(hf(seed + k * 17) * h * 0.4f);
        DrawLine(fx, fy, fx, fy - 26, Color{ 210, 210, 210, 255 });
        for (int c = 0; c < 22; c++) {
            int wave = (int)std::round(std::sin(t * 5.f + c * 0.45f + k) * 2.f);
            DrawRectangle(fx + 1 + c, fy - 26 + wave, 1, 7, c1);
            DrawRectangle(fx + 1 + c, fy - 19 + wave, 1, 6, c2);
        }
    }
}

// confettis qui tombent en virevoltant
void fxConfetti(int x, int y, int w, int h, float t, const Color* cols, int ncol, int n, unsigned seed) {
    for (int k = 0; k < n; k++) {
        uint32_t id = seed * 31u + k * 2654435761u;
        float sp = 18.f + hf(id) * 34.f, ph = hf(id + 1) * 6.28f;
        float yy = std::fmod(hf(id + 2) * (h + 20) + t * sp, (float)(h + 20)) - 10;
        float xx = std::fmod(hf(id + 3) * w + std::sin(t * 1.8f + ph) * 9.f + w, (float)w);
        bool flat = std::sin(t * 7.f + ph) > 0;
        DrawRectangle(x + (int)xx, y + (int)yy, flat ? 3 : 1, flat ? 1 : 2, cols[k % ncol]);
    }
}

// feux d'artifice : fusée, gerbe circulaire, retombée scintillante
void fxFireworks(int x, int y, int w, int h, float t, unsigned seed, int n) {
    static const Color PAL[6] = { { 255, 210, 70, 255 }, { 255, 80, 90, 255 }, { 90, 200, 255, 255 }, { 140, 255, 120, 255 }, { 255, 255, 255, 255 }, { 230, 120, 255, 255 } };
    for (int k = 0; k < n; k++) {
        float period = 2.4f + hf(seed + k * 13) * 1.6f;
        float lt = std::fmod(t + hf(seed + k * 7) * period, period);
        int cycle = (int)((t + hf(seed + k * 7) * period) / period);
        uint32_t id = seed + k * 977 + cycle * 7919;
        float cx = x + w * (0.1f + 0.8f * hf(id)), cy = y + h * (0.15f + 0.4f * hf(id + 1));
        Color c = PAL[hash32(id + 2) % 6];
        if (lt < 0.6f) {                                                  // montée de la fusée
            float u = lt / 0.6f;
            int ry = (int)(y + h - (y + h - cy) * u);
            DrawRectangle((int)cx, ry, 1, 3, Color{ 255, 230, 160, 255 });
            DrawRectangle((int)cx, ry + 3, 1, 3, Color{ 255, 160, 60, 140 });
        } else {
            float u = (lt - 0.6f) / (period - 0.6f);
            if (u > 1) continue;
            int parts = 26;
            float rad = 6.f + 30.f * std::sqrt(u);
            unsigned char a = (unsigned char)(255 * (1 - u));
            for (int p = 0; p < parts; p++) {
                float an = p * 6.2831f / parts + hf(id + p) * 0.2f;
                float px = cx + std::cos(an) * rad, py = cy + std::sin(an) * rad + 18.f * u * u;
                bool twinkle = ((int)(t * 20) + p) % 3 != 0 || u < 0.6f;
                if (twinkle) DrawRectangle((int)px, (int)py, 2, 2, Color{ c.r, c.g, c.b, a });
                if (u < 0.3f) DrawRectangle((int)(cx + std::cos(an) * rad * 0.6f), (int)(cy + std::sin(an) * rad * 0.6f), 1, 1, Color{ 255, 255, 255, a });
            }
            if (u < 0.08f) DrawCircle((int)cx, (int)cy, 8, Color{ 255, 255, 230, 120 });
        }
    }
}

// projecteurs : faisceaux qui balayent la scène depuis le haut
void fxSpotlights(int x, int y, int w, int h, float t, int n, Color c) {
    for (int k = 0; k < n; k++) {
        float sx = x + w * (k + 0.5f) / n;
        float sweep = std::sin(t * (0.6f + k * 0.17f) + k * 1.7f);
        float ex = sx + sweep * w * 0.25f;
        float hw = 22.f;
        DrawTriangle(Vector2{ sx - 3, (float)y }, Vector2{ ex - hw, (float)(y + h) }, Vector2{ ex + hw, (float)(y + h) }, Color{ c.r, c.g, c.b, 34 });
        DrawTriangle(Vector2{ sx - 1, (float)y }, Vector2{ ex - hw * 0.45f, (float)(y + h) }, Vector2{ ex + hw * 0.45f, (float)(y + h) }, Color{ c.r, c.g, c.b, 30 });
        DrawEllipse((int)ex, y + h, (int)hw, 5, Color{ c.r, c.g, c.b, 50 });
        DrawRectangle((int)sx - 4, y - 3, 8, 4, Color{ 60, 60, 70, 255 });
        DrawRectangle((int)sx - 2, y + 1, 4, 1, Color{ 255, 250, 220, 255 });
    }
}

// rideaux de scène : plis et frange dorée
void fxCurtains(int x, int y, int w, int h, Color c1, Color c2) {
    for (int i = 0; i < w; i += 2) {
        float f = 0.75f + 0.25f * std::sin(i * 0.55f);
        DrawRectangle(x + i, y, 2, h, shade((i / 6) % 2 ? c1 : c2, f));
    }
    for (int i = 0; i < w; i += 3) DrawRectangle(x + i, y + h - 2, 2, 4, Color{ 230, 180, 60, 255 });
}

// grand trophée détaillé avec reflet qui balaye le métal : 0 coupe, 1 « grandes oreilles », 2 championnat (bouclier), 3 Ballon d'or, 4 trophée individuel
void fxBigTrophy(int x, int y, int style, int s, float t) {
    Color g1{ 255, 214, 80, 255 }, g2{ 214, 150, 32, 255 }, g3{ 255, 244, 170, 255 };
    Color s1{ 232, 236, 246, 255 }, s2{ 150, 158, 178, 255 }, s3{ 255, 255, 255, 255 };
    Color base{ 52, 38, 30, 255 }, base2{ 90, 66, 44, 255 };
    auto R = [&](int dx, int dy, int w, int h, Color c) { DrawRectangle(x + dx * s, y + dy * s, w * s, h * s, c); };
    // socle
    R(-6, -3, 12, 3, base); R(-5, -5, 10, 2, base2); R(-4, -4, 8, 1, Color{ 200, 170, 90, 255 });
    int top = -20;
    switch (style) {
    case 1: {   // coupe aux grandes oreilles (argent)
        R(-1, -9, 2, 4, s2); R(-3, -10, 6, 1, s1);
        R(-4, -20, 8, 10, s1); R(-3, -21, 6, 1, s1); R(1, -20, 3, 10, s2); R(-3, -19, 1, 7, s3);
        R(-8, -20, 2, 9, s1); R(-7, -21, 3, 1, s1); R(-7, -12, 3, 1, s1);
        R(6, -20, 2, 9, s2); R(4, -21, 3, 1, s1); R(4, -12, 3, 1, s1);
        top = -21; break;
    }
    case 2: {   // trophée de championnat : coupe dorée sur colonne et plaque argentée
        R(-2, -9, 4, 4, s2); R(-1, -12, 2, 3, g2);
        R(-4, -20, 8, 8, g1); R(-5, -21, 10, 2, g3); R(1, -20, 3, 8, g2);
        R(-6, -19, 2, 4, g1); R(4, -19, 2, 4, g2);
        R(-3, -24, 6, 3, s1); R(-1, -26, 2, 2, g3);
        R(-3, -8, 6, 2, s1);
        top = -26; break;
    }
    case 3: {   // Ballon d'or : ballon doré sur une colonne
        R(-2, -11, 4, 6, g2); R(-1, -11, 1, 6, g1);
        DrawCircle(x, y - 16 * s, 5.5f * s, g2);
        DrawCircle(x - s / 2, y - 16 * s - s / 2, 4.8f * s, g1);
        R(-2, -19, 2, 2, g3); R(1, -17, 2, 2, g2); R(-3, -15, 2, 2, g2); R(1, -14, 2, 1, g2);
        top = -22; break;
    }
    case 4: {   // statuette individuelle
        R(-1, -12, 2, 7, g2); R(-3, -15, 6, 3, g1); R(-2, -18, 4, 3, g1); R(-1, -20, 2, 2, g3);
        top = -20; break;
    }
    case 5: {   // district : écusson en bois verni et coupe argentée miniature
        Color w1{ 132, 84, 44, 255 }, w2{ 92, 56, 28, 255 };
        R(-5, -16, 10, 11, w1); R(-4, -6, 8, 1, w1); R(-3, -5, 6, 1, w2); R(3, -16, 2, 11, w2); R(-5, -17, 10, 1, Color{ 170, 116, 66, 255 });
        R(-3, -11, 6, 3, s2); R(-2, -11, 4, 2, s1);                       // plaque gravée
        R(-1, -15, 2, 1, s2); R(-2, -16, 4, 1, s1);
        R(-2, -21, 4, 4, s1); R(1, -21, 1, 4, s2); R(-3, -21, 1, 2, s1); R(2, -21, 1, 2, s2); R(-2, -22, 4, 1, s3);
        top = -22; break;
    }
    case 6: {   // régional : coupe argentée à larges anses sur socle noir
        R(-5, -6, 10, 1, Color{ 26, 26, 30, 255 }); R(-1, -10, 2, 4, s2); R(-3, -11, 6, 1, s1);
        R(-4, -19, 8, 8, s1); R(1, -19, 3, 8, s2); R(-3, -18, 1, 5, s3); R(-5, -20, 10, 1, s3);
        R(-8, -19, 1, 6, s1); R(-7, -19, 3, 1, s1); R(-7, -14, 3, 1, s1);
        R(7, -19, 1, 6, s2); R(4, -19, 3, 1, s2); R(4, -14, 3, 1, s2);
        R(-2, -22, 4, 2, s1); R(-1, -23, 2, 1, g1);
        top = -23; break;
    }
    case 7: {   // coupe d'Europe : grande coupe argentée à couvercle et pierre bleue
        R(-2, -9, 4, 4, s2); R(-3, -10, 6, 1, s1); R(-1, -12, 2, 2, s2);
        R(-4, -22, 8, 10, s1); R(1, -22, 3, 10, s2); R(-3, -21, 1, 7, s3);
        R(-6, -20, 2, 6, s1); R(4, -20, 2, 6, s2);
        R(-5, -23, 10, 1, s3); R(-3, -25, 6, 2, s1); R(0, -26, 1, 1, Color{ 110, 160, 255, 255 });
        R(-1, -18, 2, 2, Color{ 70, 110, 220, 255 });
        top = -26; break;
    }
    default: {  // coupe nationale dorée
        R(-1, -9, 2, 4, g2); R(-3, -10, 6, 1, g1);
        R(-5, -19, 10, 9, g1); R(-6, -20, 12, 2, g3); R(2, -19, 3, 9, g2);
        R(-8, -18, 2, 6, g1); R(-7, -13, 2, 1, g1); R(6, -18, 2, 6, g2); R(5, -13, 2, 1, g2);
        R(-4, -18, 1, 6, g3);
        top = -20; break;
    }
    }
    // reflet lumineux qui traverse le trophée
    float sw = std::fmod(t * 0.7f, 2.2f);
    if (sw < 1.f) {
        int sx = x + (int)((-7 + sw * 14) * s);
        for (int k = 0; k < (-top - 4) * s; k += s) DrawRectangle(sx + k / 4, y + (top + 1) * s + k, s, s, Color{ 255, 255, 255, 120 });
    }
    // étincelles
    for (int k = 0; k < 3; k++) {
        float a = std::fmod(t * 1.3f + k * 0.37f, 1.f);
        if (a > 0.25f) continue;
        int px = x + (int)((k - 1) * 6 * s), py = y + (top + 3 + k * 4) * s;
        int r = (int)(a * 12) + 1;
        DrawRectangle(px - r, py, 2 * r + 1, 1, Color{ 255, 255, 230, 220 });
        DrawRectangle(px, py - r, 1, 2 * r + 1, Color{ 255, 255, 230, 220 });
    }
}
