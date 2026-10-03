// Sons synthétisés (aucun fichier externe)
#include "audio.h"
#include "raylib.h"
#include "match.h"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>

static Sound g_sfx[NUM_SFX];
static Sound g_crowd;
static const int NUM_CHANTS = 12;
static Sound g_fanVoice[NUM_CHANTS];
static Sound g_music;
static bool g_musicOk = false, g_musicOn = true;
static bool g_ok = false, g_enabled = true;
static const int SR = 22050;

static Sound makeSound(const std::vector<float>& s) {
    Wave w;
    w.frameCount = (unsigned)s.size();
    w.sampleRate = SR; w.sampleSize = 16; w.channels = 1;
    short* d = (short*)MemAlloc((unsigned)(s.size() * sizeof(short)));
    for (size_t i = 0; i < s.size(); i++) { float v = s[i]; if (v > 1) v = 1; if (v < -1) v = -1; d[i] = (short)(v * 32000); }
    w.data = d;
    Sound snd = LoadSoundFromWave(w);
    UnloadWave(w);
    return snd;
}

static float frand() { return (float)rand() / RAND_MAX * 2 - 1; }

static std::vector<float> noise(float sec, float lp, float gain, float (*env)(float)) {
    int n = (int)(sec * SR);
    std::vector<float> v(n);
    float y = 0, y2 = 0;
    for (int i = 0; i < n; i++) {
        float t = (float)i / n;
        y += (frand() - y) * lp; y2 += (y - y2) * lp;
        v[i] = y2 * gain * env(t);
    }
    return v;
}

// ------------------------------------------------------------------ public : chœurs, tambours, clappements, sifflets
// Une voix de supporter = fondamentale + harmoniques pondérées par deux formants (voyelle chantée).
struct Vowel { float f1, f2; };
static const Vowel V_A = { 750, 1250 }, V_E = { 420, 2000 }, V_O = { 480, 850 }, V_I = { 300, 2300 }, V_OU = { 330, 750 };
static float formantGain(float f, const Vowel& v) {
    float a = (f - v.f1) / 170.f, b = (f - v.f2) / 300.f;
    return std::exp(-a * a) + 0.6f * std::exp(-b * b) + 0.06f;
}
struct Syll { float t0, len; int semi; const Vowel* v; float accent; };
// chœur de nv voix légèrement désaccordées et décalées, sur une suite de syllabes ; base = fréquence du demi-ton 0
static void addChoir(std::vector<float>& out, const std::vector<Syll>& song, float base, int nv, float gain, unsigned seed) {
    const float PI2 = 6.2831853f;
    srand(seed);
    for (int v = 0; v < nv; v++) {
        float det = std::pow(2.f, (frand() * 22.f) / 1200.f) * (v % 3 == 0 ? 0.5f : 1.f);   // un tiers chante à l'octave basse
        float onset = (frand() * 0.5f + 0.5f) * 0.07f, vibF = 4.5f + frand() * 1.2f, vibA = 0.004f + 0.004f * std::fabs(frand());
        float loud = 0.7f + 0.3f * std::fabs(frand()), ph = std::fabs(frand()) * PI2;
        for (const Syll& sy : song) {
            int i0 = (int)((sy.t0 + onset) * SR), n = (int)(sy.len * SR);
            float f0 = base * det * std::pow(2.f, sy.semi / 12.f);
            float scoop = 1.f - 0.04f * std::fabs(frand());                     // attaque un peu en dessous de la note
            float hg[10] = {}; int nh = 0;                                      // poids des harmoniques (voyelle), calculés une fois par syllabe
            for (int h = 1; h <= 9 && f0 * h < 4500; h++, nh++) hg[h] = formantGain(f0 * h, *sy.v) / (0.6f + h * 0.4f);
            for (int i = 0; i < n && i0 + i < (int)out.size(); i++) {
                if (i0 + i < 0) continue;
                float t = (float)i / SR, u = (float)i / n;
                float env = std::min(1.f, t * 28.f) * (u > 0.7f ? (1 - u) / 0.3f : 1.f) * sy.accent * loud;
                float f = f0 * (scoop + (1 - scoop) * std::min(1.f, t * 12.f)) * (1 + vibA * std::sin(PI2 * vibF * t));
                ph += PI2 * f / SR; if (ph > PI2 * 64) ph -= PI2 * 64;
                float smp = 0;
                float s1 = std::sin(ph), c1 = std::cos(ph), sh = s1, ch = c1;     // sin(h·ph) par récurrence
                for (int h = 1; h <= nh; h++) { smp += hg[h] * sh; float ns = sh * c1 + ch * s1; ch = ch * c1 - sh * s1; sh = ns; }
                smp += frand() * 0.05f;                                             // souffle
                out[i0 + i] += smp * env * gain / nv;
            }
        }
    }
}
// grosse caisse du kop
static void addDrum(std::vector<float>& out, float t0, float amp) {
    const float PI2 = 6.2831853f;
    int i0 = (int)(t0 * SR), n = (int)(0.35f * SR);
    for (int i = 0; i < n && i0 + i < (int)out.size(); i++) { float t = (float)i / SR; out[i0 + i] += (std::sin(PI2 * (52 + 70 * std::exp(-t * 25)) * t) * 0.9f + frand() * 0.15f * std::exp(-t * 60)) * std::exp(-t * 9) * amp; }
}
// clappements de milliers de mains : bruit filtré à attaque franche, légèrement étalé
static void addClap(std::vector<float>& out, float t0, float amp) {
    int i0 = (int)(t0 * SR), n = (int)(0.16f * SR);
    float y = 0, y2 = 0;
    for (int i = 0; i < n && i0 + i < (int)out.size(); i++) {
        float t = (float)i / SR;
        float x = frand();
        y += (x - y) * 0.55f; y2 += (y - y2) * 0.55f;
        float hp = y - y2;                                                          // bande médium
        float env = std::min(1.f, t * 300.f) * std::exp(-t * 26.f) * (1.f + 0.3f * std::exp(-t * 90.f));
        out[i0 + i] += hp * env * amp * 2.2f;
    }
}
// rumeur de fond (foule)
static void addMurmur(std::vector<float>& out, float amp, float lp) {
    float y = 0, y2 = 0;
    for (size_t i = 0; i < out.size(); i++) { y += (frand() - y) * lp; y2 += (y - y2) * lp; out[i] += y2 * amp; }
}
// sifflets du public : des dizaines de sifflements aigus, glissés, qui démarrent et s'arrêtent au hasard
static void addCrowdWhistles(std::vector<float>& out, int nw, float amp, unsigned seed) {
    const float PI2 = 6.2831853f;
    srand(seed);
    float total = (float)out.size() / SR;
    for (int w = 0; w < nw; w++) {
        float f0 = 1900.f + std::fabs(frand()) * 1700.f, ph = 0;
        float start = std::fabs(frand()) * total * 0.25f, len = total * (0.55f + 0.4f * std::fabs(frand()));
        float glide = frand() * 300.f, wob = 3.f + std::fabs(frand()) * 6.f, finger = std::fabs(frand()) > 0.5f ? 1.f : 0.f;
        int i0 = (int)(start * SR), n = (int)(len * SR);
        for (int i = 0; i < n && i0 + i < (int)out.size(); i++) {
            float t = (float)i / SR, u = (float)i / n;
            float f = f0 + glide * u + 60.f * std::sin(PI2 * wob * t) * finger;
            ph += PI2 * f / SR; if (ph > PI2) ph -= PI2;
            float env = std::min(1.f, t * 15.f) * std::min(1.f, (1 - u) * 8.f) * (0.75f + 0.25f * std::sin(PI2 * 0.7f * t + w));
            out[i0 + i] += (std::sin(ph) + 0.12f * std::sin(ph * 2)) * env * amp / std::sqrt((float)nw);
        }
    }
}
static void normalize(std::vector<float>& v, float peak, float fade) {
    float m = 0.0001f; for (float x : v) m = std::max(m, std::fabs(x));
    int nf = (int)(fade * SR);
    for (size_t i = 0; i < v.size(); i++) {
        float g = peak / m;
        if ((int)i < nf) g *= (float)i / nf;
        if ((int)(v.size() - i) < nf) g *= (float)(v.size() - i) / nf;
        v[i] *= g;
    }
}
// chants complets (boucles) : 0 « Allez ! », 1 sifflets, 2 contestation, 3 encouragements rythmés, 4 « Olé ! »
static std::vector<float> renderChant(int kind) {
    float len = kind == 1 ? 5.f : kind == 5 ? 12.f : kind == 9 ? 9.6f : kind == 10 ? 8.f : 6.4f;
    std::vector<float> v((size_t)(len * SR), 0.f);
    const float b = 60.f / 132.f;                    // une noire à 132 bpm
    std::vector<Syll> song;
    switch (kind) {
    case 0: {   // « Al-lez, al-lez, al-lez... » sur une mélodie montante, grosse caisse sur les temps
        static const int MEL[12] = { 0, 4, 7, 7, 5, 4, 2, 4, 5, 4, 2, 0 };
        for (int r = 0; r < 2; r++) for (int k = 0; k < 6; k++) {
            float t0 = r * 6 * b * 2 + k * b * 2;
            song.push_back({ t0, b * 0.8f, MEL[(r * 6 + k) % 12], &V_A, 0.85f });
            song.push_back({ t0 + b, b * 0.95f, MEL[(r * 6 + k) % 12] + 2, &V_E, 1.f });
        }
        addChoir(v, song, 196.f, 26, 1.2f, 11);
        for (float t = 0; t < len; t += b) addDrum(v, t, (int)(t / b + 0.5f) % 2 ? 0.25f : 0.45f);
        addMurmur(v, 0.18f, 0.06f);
        break;
    }
    case 1:     // sifflets nourris et huées
        addCrowdWhistles(v, 46, 1.1f, 23);
        song.push_back({ 0.2f, len - 0.5f, 0, &V_OU, 0.55f });
        addChoir(v, song, 120.f, 14, 0.9f, 29);
        addMurmur(v, 0.25f, 0.09f);
        break;
    case 2: {   // « Dé-mis-sion ! » scandé, clappements entre les mots
        for (int r = 0; r < 4; r++) {
            float t0 = r * b * 3.f;
            song.push_back({ t0, b * 0.45f, 0, &V_E, 0.9f });
            song.push_back({ t0 + b * 0.5f, b * 0.45f, 0, &V_I, 0.9f });
            song.push_back({ t0 + b, b * 0.9f, -3, &V_O, 1.f });
            addClap(v, t0 + b * 2.f, 0.8f); addClap(v, t0 + b * 2.5f, 0.8f);
        }
        addChoir(v, song, 150.f, 24, 1.3f, 31);
        for (int r = 0; r < 4; r++) addDrum(v, r * b * 3.f + b, 0.5f);
        addMurmur(v, 0.2f, 0.07f);
        break;
    }
    case 3: {   // clap clap / clap-clap-clap + « Al-lez ! » et tambour
        for (int r = 0; r < 3; r++) {
            float t0 = r * b * 4.f + 0.1f;
            addClap(v, t0, 1.f); addClap(v, t0 + b, 1.f);
            addClap(v, t0 + b * 2.f, 1.f); addClap(v, t0 + b * 2.5f, 1.f); addClap(v, t0 + b * 3.f, 1.f);
            addDrum(v, t0, 0.5f); addDrum(v, t0 + b * 2.f, 0.4f);
            song.push_back({ t0 + b * 3.4f, b * 0.35f, 5, &V_A, 0.9f });
            song.push_back({ t0 + b * 3.8f, b * 0.5f, 7, &V_E, 1.f });
        }
        addChoir(v, song, 196.f, 22, 1.1f, 37);
        addMurmur(v, 0.2f, 0.07f);
        break;
    }
    case 4: {   // « O-lé, o-lé o-lé o-lééé » : mélodie classique des stades, chœur fourni
        static const float T[8] = { 0, 0.5f, 1.5f, 2.f, 2.5f, 3.f, 3.5f, 4.f };
        static const int N[8] = { 0, 4, 4, 7, 4, 7, 9, 7 };
        static const float L[8] = { 0.45f, 0.9f, 0.45f, 0.45f, 0.45f, 0.45f, 0.45f, 1.6f };
        for (int r = 0; r < 2; r++) for (int k = 0; k < 8; k++) {
            float t0 = r * b * 6.f + T[k] * b;
            song.push_back({ t0, L[k] * b, N[k], k % 2 ? &V_E : &V_O, k == 7 ? 1.f : 0.9f });
        }
        addChoir(v, song, 220.f, 30, 1.3f, 41);
        for (float t = 0; t < len; t += b) addDrum(v, t, 0.35f);
        addMurmur(v, 0.2f, 0.07f);
        break;
    }
    case 5: {   // chant du club : lent, en mineur, repris par tout le stade (écharpes tendues)
        static const int N[12] = { 0, 3, 7, 8, 7, 5, 3, 5, 7, 3, 2, 0 };
        static const float L[12] = { 2, 1, 1, 2, 1, 1, 2, 1, 1, 1.5f, 0.5f, 3 };
        float t0 = 0.1f;
        for (int k = 0; k < 12; k++) { song.push_back({ t0, L[k] * b * 1.5f * 0.95f, N[k], k % 3 == 0 ? &V_O : k % 3 == 1 ? &V_A : &V_E, 1.f }); t0 += L[k] * b * 1.5f; }
        addChoir(v, song, 175.f, 30, 1.3f, 43);
        addMurmur(v, 0.15f, 0.06f);
        break;
    }
    case 6: {   // clapping viking : « HOU ! » et clappement, de plus en plus rapprochés
        float t0 = 0.1f, gap = 1.5f;
        while (t0 < len - 0.3f) {
            addDrum(v, t0, 0.6f); addClap(v, t0 + 0.02f, 1.2f);
            song.push_back({ t0 + 0.04f, 0.28f, 0, &V_OU, 1.f });
            t0 += gap; gap = std::max(0.24f, gap * 0.8f);
        }
        addChoir(v, song, 130.f, 26, 1.4f, 47);
        addMurmur(v, 0.12f, 0.06f);
        break;
    }
    case 7: {   // tambours (batucada) et cris « Al-lez ! »
        for (float t = 0.05f; t < len; t += b / 2) {
            int step = (int)std::lround(t / (b / 2));
            if (step % 4 == 0) addDrum(v, t, 0.55f); else if (step % 4 == 3) addDrum(v, t, 0.3f);
            addClap(v, t, step % 2 ? 0.25f : 0.45f);
        }
        for (int r = 0; r < 3; r++) { float t0 = r * b * 4.f + b * 2.f; song.push_back({ t0, b * 0.4f, 7, &V_A, 0.9f }); song.push_back({ t0 + b * 0.5f, b * 0.7f, 5, &V_E, 1.f }); }
        addChoir(v, song, 196.f, 20, 1.0f, 53);
        addMurmur(v, 0.15f, 0.07f);
        break;
    }
    case 8: {   // « Et un, et deux... » : comptine moqueuse quand l'équipe mène, rythmée par les mains
        static const float T[9] = { 0, 0.5f, 1.f, 1.5f, 2.f, 2.5f, 3.f, 3.5f, 4.f };
        static const int N[9] = { 4, 4, 5, 5, 7, 7, 9, 7, 12 };
        for (int r = 0; r < 2; r++) for (int k = 0; k < 9; k++) { float t0 = r * b * 6.f + T[k] * b; song.push_back({ t0, (k == 8 ? 1.4f : 0.42f) * b, N[k], k % 2 ? &V_E : &V_A, k == 8 ? 1.f : 0.85f }); addClap(v, t0, 0.6f); }
        addChoir(v, song, 220.f, 28, 1.25f, 59);
        addMurmur(v, 0.18f, 0.07f);
        break;
    }
    case 9: {   // « AUX ARMES ! » : un virage lance, l'autre répond, puis « Nous sommes les... et nous allons gagner ! »
        for (int r = 0; r < 2; r++) {
            float t0 = 0.1f + r * b * 2.2f;
            std::vector<Syll> call = { { t0, b * 0.42f, 0, &V_O, 1.f }, { t0 + b * 0.5f, b * 0.42f, 0, &V_A, 1.f }, { t0 + b, b * 0.9f, -2, &V_E, 1.f } };
            addChoir(v, call, r ? 150.f : 165.f, r ? 22 : 18, r ? 1.35f : 1.1f, 61 + r);   // réponse plus forte
            addDrum(v, t0, 0.6f); addDrum(v, t0 + b, 0.6f);
        }
        static const float T[10] = { 0, 0.5f, 1.f, 1.5f, 2.f, 2.5f, 3.5f, 4.f, 4.5f, 5.f };
        static const int N[10] = { 0, 0, 2, 4, 4, 2, 4, 5, 7, 7 };
        float s0 = 0.1f + b * 4.6f;
        for (int k = 0; k < 10; k++) song.push_back({ s0 + T[k] * b, (k == 9 ? 1.4f : 0.45f) * b, N[k], k % 3 == 0 ? &V_O : k % 3 == 1 ? &V_A : &V_E, k == 9 ? 1.f : 0.9f });
        addChoir(v, song, 175.f, 30, 1.3f, 67);
        for (float t = s0; t < len - 0.2f; t += b) addDrum(v, t, (int)((t - s0) / b + 0.5f) % 2 ? 0.25f : 0.5f);
        addMurmur(v, 0.15f, 0.07f);
        break;
    }
    case 10: {  // « Oh oh oh oh oh oh ohhh » : le riff repris par tout le stade
        static const float T[7] = { 0, 1.5f, 2.f, 2.75f, 3.5f, 4.f, 6.f };
        static const int N[7] = { 0, 0, 3, 0, -2, -4, -5 };
        static const float L[7] = { 1.4f, 0.45f, 0.7f, 0.7f, 0.45f, 1.9f, 1.9f };
        for (int r = 0; r < 2; r++) for (int k = 0; k < 7; k++) song.push_back({ 0.1f + r * b * 8.f + T[k] * b, L[k] * b, N[k], &V_O, k == 0 ? 1.f : 0.9f });
        addChoir(v, song, 165.f, 32, 1.35f, 71);
        for (float t = 0.1f; t < len; t += b) addDrum(v, t, 0.4f);
        addMurmur(v, 0.15f, 0.07f);
        break;
    }
    case 11: {  // « Qui ne saute pas... » : chant sauté, piétinement des tribunes
        for (int r = 0; r < 4; r++) {
            float t0 = 0.1f + r * b * 3.f;
            static const int N[6] = { 7, 7, 5, 7, 9, 7 };
            for (int k = 0; k < 6; k++) song.push_back({ t0 + k * b * 0.5f, b * 0.38f, N[k], k % 2 ? &V_I : &V_A, k == 5 ? 1.f : 0.85f });
            for (int k = 0; k < 6; k++) addDrum(v, t0 + k * b * 0.5f, 0.35f);       // les tribunes tremblent
        }
        addChoir(v, song, 196.f, 28, 1.25f, 73);
        addMurmur(v, 0.22f, 0.08f);
        break;
    }
    }
    normalize(v, 0.85f, 0.04f);
    return v;
}

// ------------------------------------------------------------------ musique (synthèse chiptune)
// Notes en demi-tons depuis la note de base de la mélodie ; -99 silence, -98 note tenue (prolonge la précédente)
struct Track {
    const char* name;
    float bpm;
    const int* mel; int n;          // une valeur par croche
    const int* bass; int nb;        // fondamentale par mesure (demi-tons depuis do2)
    float melBase;                  // fréquence de la note 0
    float duty;                     // rapport cyclique de la carrée (0.125, 0.25, 0.5)
    int bassStyle;                  // 0 octaves, 1 arpège, 2 funk syncopé, 3 notes tenues
    int drums;                      // 0 rock, 1 léger, 2 fanfare (roulements), 3 aucune
    float gain;
};

static const int MEL1[128] = {
    0, 4, 7, 12, 11, 7, 4, 7,     2, 7, 11, 14, 12, 11, 7, -99,
    9, 12, 16, 12, 9, 12, 16, 19, 17, 16, 14, 12, 9, 5, 9, -99,
    0, 4, 7, 12, 14, 12, 11, 7,   2, 7, 11, 7, 14, 11, 7, 11,
    9, 7, 5, 9, 12, 9, 5, 4,      5, 7, 9, 11, 12, -99, 12, -99,
    16, 16, 14, 12, 14, 14, 12, 11, 12, 12, 11, 9, 7, -99, 7, 9,
    12, 12, 11, 9, 11, 11, 9, 7,  9, 11, 12, 14, 16, -99, 16, -99,
    17, 16, 14, 12, 14, 12, 11, 9, 12, 11, 9, 7, 9, 7, 5, 4,
    5, 4, 2, 5, 7, 11, 14, 17,    19, -99, 16, -99, 12, -99, -99, -99 };
static const int BASS1[16] = { 0, 7, 9, 5, 0, 7, 9, 5, 5, 0, 9, 7, 5, 0, 5, 7 };

// « Vestiaire » : la mineur, plus calme
static const int MEL2[128] = {
    0, -98, 3, 7, 5, -98, 3, 0,    -2, -98, 0, 3, 2, -98, -99, -99,
    0, -98, 3, 7, 10, -98, 8, 7,   5, -98, 3, 5, 7, -98, -99, -99,
    8, -98, 7, 5, 3, -98, 5, 7,    3, -98, 2, 0, -2, -98, 0, 2,
    3, -98, 5, 7, 8, 7, 5, 3,      2, -98, -98, -98, -99, -99, -99, -99,
    12, -98, 10, 8, 7, -98, 8, 10, 12, -98, 15, 12, 10, -98, -99, -99,
    8, -98, 7, 8, 10, -98, 12, 10, 8, -98, 7, 5, 3, -98, -99, -99,
    5, 7, 8, 10, 12, -98, 10, 8,   7, 8, 10, 12, 14, -98, 12, 10,
    12, -98, -98, -98, 7, -98, 3, -98, 0, -98, -98, -98, -99, -99, -99, -99 };
static const int BASS2[16] = { 9, 9, 5, 7, 9, 9, 5, 4, 5, 5, 2, 4, 9, 5, 7, 9 };

// « Nuit européenne » : hymne majestueux
static const int MEL3[128] = {
    0, -98, -98, 4, 7, -98, 12, -98,  11, -98, 9, -98, 7, -98, -98, -98,
    5, -98, -98, 9, 12, -98, 14, -98, 12, -98, 11, -98, 9, -98, -98, -98,
    7, -98, -98, 11, 14, -98, 17, -98, 16, -98, 14, -98, 12, -98, 11, -98,
    9, -98, 11, -98, 12, -98, 14, -98, 12, -98, -98, -98, -99, -99, -99, -99,
    12, -98, -98, 16, 19, -98, 24, -98, 23, -98, 21, -98, 19, -98, -98, -98,
    17, -98, -98, 21, 24, -98, 26, -98, 24, -98, 23, -98, 21, -98, 19, -98,
    17, -98, 16, -98, 14, -98, 12, -98, 14, -98, 16, -98, 17, -98, 19, -98,
    24, -98, -98, -98, -98, -98, -98, -98, 12, -98, -98, -98, -99, -99, -99, -99 };
static const int BASS3[16] = { 0, 7, 5, 0, 7, 7, 5, 0, 0, 7, 5, 0, 5, 7, 0, 0 };

// « Mercato » : funk syncopé
static const int MEL4[128] = {
    7, -99, 7, 10, -99, 12, -99, 10,  7, -99, 5, -99, 3, 5, -99, -99,
    7, -99, 7, 10, -99, 12, -99, 15,  14, -99, 12, -99, 10, 12, -99, -99,
    7, -99, 7, 10, -99, 12, -99, 10,  7, -99, 5, -99, 3, 5, -99, 7,
    10, -99, 10, 12, 10, 7, 5, 3,     5, -99, -99, -99, 0, -99, -99, -99,
    15, -99, 14, 12, -99, 10, 12, -99, 15, -99, 17, -99, 15, 14, 12, -99,
    10, -99, 12, 14, -99, 15, 17, -99, 19, -98, 17, -99, 15, -99, 14, -99,
    12, 12, -99, 10, 10, -99, 7, 7,   -99, 5, 7, -99, 10, -99, 12, -99,
    15, -99, 14, -99, 12, -99, 10, -99, 7, -98, -98, -98, -99, -99, -99, -99 };
static const int BASS4[16] = { 0, 0, 5, 7, 0, 0, 5, 3, 8, 8, 10, 7, 5, 5, 7, 0 };

// jingles
static const int MELW[24] = { 0, 4, 7, 12, -98, -98, 7, 12, 16, -98, -98, -98, 19, -98, -98, -98, -98, -98, -99, -99, -99, -99, -99, -99 };
static const int BASSW[3] = { 0, 7, 0 };
static const int MELT[32] = { 7, -99, 7, 7, 12, -98, -98, -98, 11, -99, 11, 11, 14, -98, -98, -98, 12, 14, 16, 17, 19, -98, -98, -98, 24, -98, -98, -98, -98, -98, -99, -99 };
static const int BASST[4] = { 0, 7, 0, 0 };
static const int MELC[32] = { 0, -98, 4, -98, 7, -98, 12, -98, 11, -98, 12, -98, 14, -98, -98, -98, 12, -98, 16, -98, 19, -98, 24, -98, -98, -98, -98, -98, -98, -98, -99, -99 };
static const int BASSC[4] = { 0, 5, 7, 0 };

// « Hymne des étoiles » : hymne original façon grande soirée européenne (composition maison)
static const int MELA[64] = { 0, -98, 4, -98, 7, -98, -98, -98, 5, -98, 4, -98, 2, -98, -98, -98,
                              4, -98, 7, -98, 12, -98, -98, -98, 11, -98, 9, -98, 7, -98, -98, -98,
                              9, -98, 11, -98, 12, -98, 14, -98, 12, -98, -98, -98, 11, -98, 9, -98,
                              7, -98, 4, -98, 5, -98, 7, -98, 12, -98, -98, -98, -98, -98, -99, -99 };
static const int BASSA[8] = { 2, 7, 9, 2, 7, 9, 2, 2 };
// « Derby » : rock énergique en mi mineur
static const int MEL5[128] = {
    0, -99, 0, 3, 5, -99, 7, -99,     5, 3, 0, -99, -2, 0, -99, -99,
    0, -99, 0, 3, 5, -99, 7, 10,      12, -98, 10, 7, 5, -98, -99, -99,
    7, 7, 8, 7, 5, -99, 3, 5,         7, -98, -98, 5, 3, -99, 0, -99,
    3, 3, 5, 3, 2, -99, -2, 2,        0, -98, -98, -98, -99, -99, -99, -99,
    12, -99, 12, 15, 17, -99, 15, 12, 10, -99, 12, 10, 7, -98, -99, -99,
    8, -99, 8, 10, 12, -99, 10, 8,    7, -98, 5, -98, 3, -98, 2, -98,
    0, 3, 7, 12, 10, 7, 3, 7,         8, 7, 5, 3, 5, -98, -99, -99,
    3, 5, 7, 8, 10, 12, 14, 15,       12, -98, -98, -98, -99, -99, -99, -99 };
static const int BASS5[16] = { 4, 4, 4, 7, 0, 0, 2, 4, 4, 7, 0, 11, 4, 0, 2, 4 };
// « Copacabana » : samba ensoleillée
static const int MEL6[128] = {
    7, -99, 9, 7, -99, 4, -99, 7,     9, -99, 12, 9, -99, 7, 4, -99,
    5, -99, 7, 5, -99, 2, -99, 5,     7, -98, 4, -98, -99, -99, -99, -99,
    7, -99, 9, 7, -99, 4, -99, 7,     9, -99, 12, 14, -99, 12, 9, -99,
    11, -99, 9, 7, -99, 5, 4, 2,      0, -98, -98, -98, -99, -99, -99, -99,
    12, 12, -99, 11, 12, -99, 14, -99, 12, 11, 9, -99, 7, -99, 9, -99,
    11, 11, -99, 9, 11, -99, 12, -99, 11, 9, 7, -99, 4, -99, -99, -99,
    7, 9, 11, 12, 14, -99, 12, -99,   9, -99, 7, -99, 9, 11, 12, -99,
    14, -99, 12, -99, 11, -99, 9, 7,  12, -98, -98, -98, -99, -99, -99, -99 };
static const int BASS6[16] = { 9, 9, 2, 4, 9, 9, 4, 9, 2, 2, 4, 4, 9, 2, 4, 9 };
// « Tifo » : hymne de tribune, grand et fédérateur
static const int MEL7[128] = {
    0, -98, 4, -98, 7, -98, 12, -98,  11, -98, 7, -98, 9, -98, -98, -98,
    5, -98, 9, -98, 12, -98, 14, -98, 12, -98, 11, -98, 7, -98, -98, -98,
    4, -98, 7, -98, 12, -98, 16, -98, 14, -98, 12, -98, 11, -98, 12, -98,
    9, -98, 11, -98, 12, -98, 14, -98, 12, -98, -98, -98, -99, -99, -99, -99,
    16, -98, 16, 14, 12, -98, 14, -98, 16, -98, 19, -98, 17, -98, 16, -98,
    14, -98, 14, 12, 11, -98, 12, -98, 14, -98, 17, -98, 16, -98, 14, -98,
    12, -98, 16, -98, 19, -98, 24, -98, 23, -98, 21, -98, 19, -98, 17, -98,
    16, -98, 14, -98, 12, -98, 11, -98, 12, -98, -98, -98, -98, -98, -99, -99 };
static const int BASS7[16] = { 7, 2, 0, 7, 4, 2, 2, 7, 7, 4, 2, 2, 7, 0, 2, 7 };
// « Nuit au stade » : thème calme, notes tenues
static const int MEL8[128] = {
    0, -98, -98, 3, 7, -98, -98, -98, 5, -98, 3, -98, 2, -98, -98, -98,
    0, -98, -98, 3, 7, -98, 10, -98,  8, -98, 7, -98, -98, -98, -99, -99,
    3, -98, -98, 7, 10, -98, -98, -98, 8, -98, 7, -98, 5, -98, -98, -98,
    3, -98, 5, -98, 7, -98, 8, -98,   7, -98, -98, -98, -99, -99, -99, -99,
    12, -98, -98, 10, 8, -98, 7, -98, 8, -98, -98, 7, 5, -98, 3, -98,
    5, -98, -98, 3, 2, -98, 0, -98,   2, -98, 3, -98, 5, -98, -98, -98,
    12, -98, -98, 15, 14, -98, 12, -98, 10, -98, -98, 12, 10, -98, 8, -98,
    7, -98, 5, -98, 3, -98, 2, -98,   0, -98, -98, -98, -98, -98, -99, -99 };
static const int BASS8[16] = { 2, 10, 2, 0, 5, 10, 0, 9, 2, 10, 7, 9, 2, 0, 9, 2 };
// jingles des pages de publicité (compositions originales)
static const int MELP1[16] = { 0, 4, 7, 12, -98, 11, 12, -98, 16, -98, -98, -98, -99, -99, -99, -99 };
static const int MELP2[16] = { 7, 7, 9, 7, 12, -98, 11, -98, 7, 9, 12, 14, 16, -98, -98, -98 };
static const int MELP3[16] = { 12, -98, 9, -98, 5, -98, 9, -98, 12, 14, 16, -98, -98, -98, -99, -99 };
static const int BASSP1[2] = { 0, 7 }, BASSP2[2] = { 0, 5 }, BASSP3[2] = { 5, 0 };
static const Track JINGLE_AD1 = { "Pub 1", 140.f, MELP1, 16, BASSP1, 2, 523.25f, 0.25f, 0, 1, 0.9f };
static const Track JINGLE_AD2 = { "Pub 2", 150.f, MELP2, 16, BASSP2, 2, 523.25f, 0.125f, 2, 0, 0.9f };
static const Track JINGLE_AD3 = { "Pub 3", 120.f, MELP3, 16, BASSP3, 2, 440.00f, 0.5f, 1, 1, 0.9f };
// musique de but de la sono du stade (composition originale)
static const int MELG[32] = { 0, 0, 4, 7, -98, 4, 7, 12, -98, -98, 11, 12, 14, 16, -98, -98, 12, -98, 7, -98, 12, -98, 16, -98, 19, -98, -98, -98, -99, -99, -99, -99 };
static const int BASSG[4] = { 0, 5, 7, 0 };
static const Track JINGLE_GOAL = { "Sono : but", 150.f, MELG, 32, BASSG, 4, 392.00f, 0.25f, 0, 0, 0.85f };
static const Track TRACKS[] = {
    { "Super Soccer World", 138.f, MEL1, 128, BASS1, 16, 523.25f, 0.25f, 0, 0, 1.0f },
    { "Vestiaire", 112.f, MEL2, 128, BASS2, 16, 440.00f, 0.5f, 1, 1, 0.95f },
    { "Nuit européenne", 96.f, MEL3, 128, BASS3, 16, 392.00f, 0.125f, 3, 2, 0.9f },
    { "Mercato", 124.f, MEL4, 128, BASS4, 16, 392.00f, 0.25f, 2, 0, 0.95f },
    { "Derby", 152.f, MEL5, 128, BASS5, 16, 329.63f, 0.25f, 0, 0, 0.9f },
    { "Copacabana", 118.f, MEL6, 128, BASS6, 16, 440.00f, 0.5f, 2, 1, 0.9f },
    { "Tifo", 128.f, MEL7, 128, BASS7, 16, 392.00f, 0.125f, 1, 0, 0.9f },
    { "Nuit au stade", 92.f, MEL8, 128, BASS8, 16, 293.66f, 0.5f, 3, 1, 0.9f },
};
static const int NUM_TRACKS = 8;
static const Track JINGLE_WIN = { "Victoire", 150.f, MELW, 24, BASSW, 3, 523.25f, 0.25f, 3, 2, 1.0f };
static const Track JINGLE_TV = { "Générique", 132.f, MELT, 32, BASST, 4, 392.00f, 0.125f, 3, 2, 1.0f };
static const Track JINGLE_ANTHEM = { "Hymne des étoiles", 76.f, MELA, 64, BASSA, 8, 293.66f, 0.5f, 3, 2, 1.0f };
// hymne du podium (composition originale) : fanfare en do majeur
static const int MELP[64] = { 0, -98, 0, 4, 7, -98, 4, 7,     12, -98, -98, 11, 9, -98, 7, -98,
                              5, -98, 5, 9, 12, -98, 9, 12,    14, -98, -98, -98, 11, -98, -99, -99,
                              0, -98, 0, 4, 7, -98, 4, 7,      12, -98, -98, 14, 16, -98, 14, 12,
                              17, -98, 16, 14, 12, -98, 11, 14, 12, -98, -98, -98, -98, -98, -99, -99 };
static const int BASSP[8] = { 0, 4, 5, 7, 0, 9, 7, 0 };
static const Track JINGLE_PODIUM = { "Podium", 126.f, MELP, 64, BASSP, 8, 523.25f, 0.25f, 3, 2, 1.0f };
static const Track JINGLE_TROPHY = { "Trophée", 100.f, MELC, 32, BASSC, 4, 523.25f, 0.25f, 3, 2, 1.0f };
static Sound g_tracks[NUM_TRACKS];
static Sound g_jingles[9];
static int g_curTrack = 0, g_trackMode = 0;   // 0 = enchaînement automatique, 1..N = thème fixe

static std::vector<float> renderTrack(const Track& T) {
    const float PI2 = 6.2831853f;
    const float eighth = 60.f / T.bpm / 2.f;
    int n = (int)(T.n * eighth * SR) + SR / 2;
    std::vector<float> v(n, 0.f);
    // mélodie : notes tenues regroupées
    for (int k = 0; k < T.n; k++) {
        if (T.mel[k] <= -98) continue;
        int len8 = 1; while (k + len8 < T.n && T.mel[k + len8] == -98) len8++;
        float fm = T.melBase * std::pow(2.f, T.mel[k] / 12.f);
        int i0 = (int)(k * eighth * SR), len = (int)(len8 * eighth * SR);
        float dur = len8 * eighth;
        for (int i = 0; i < len && i0 + i < n; i++) {
            float t = (float)i / SR;
            float vib = t > 0.25f ? 1.f + 0.006f * std::sin(PI2 * 5.5f * t) : 1.f;
            float ph = std::fmod(t * fm * vib, 1.f);
            float sq = ph < T.duty ? 1.f : -1.f;
            float env = std::min(1.f, t * 200) * (0.7f + 0.3f * std::exp(-t * 6)) * std::min(1.f, (dur - t) * 40);
            v[i0 + i] += sq * 0.10f * env;
            // écho léger
            int e = i0 + i + (int)(eighth * 1.5f * SR);
            if (e < n) v[e] += sq * 0.03f * env;
        }
    }
    // basse
    int bars = T.n / 8;
    for (int k = 0; k < T.n; k++) {
        int bar = std::min(k / 8, T.nb - 1);
        int root = T.bass[bar % T.nb];
        int i0 = (int)(k * eighth * SR), len = (int)(eighth * SR);
        int note = root; bool play = true; float sustain = 3.f;
        switch (T.bassStyle) {
        case 0: note = root + ((k % 2) ? 12 : 0); break;
        case 1: { static const int AR[8] = { 0, 7, 12, 7, 0, 7, 12, 15 }; note = root + AR[k % 8]; break; }
        case 2: { static const int FK[8] = { 1, 0, 1, 1, 0, 1, 0, 1 }; play = FK[k % 8] != 0; note = root + ((k % 8) == 3 ? 12 : (k % 8) == 7 ? 10 : 0); sustain = 9.f; break; }
        default: play = (k % 4) == 0; len = (int)(eighth * 4 * SR); sustain = 0.8f; break;
        }
        if (!play) continue;
        float fb = 65.41f * std::pow(2.f, note / 12.f);
        for (int i = 0; i < len && i0 + i < n; i++) {
            float t = (float)i / SR;
            float ph = std::fmod(t * fb, 1.f);
            float tri = 4 * std::fabs(ph - 0.5f) - 1;
            v[i0 + i] += tri * 0.22f * std::exp(-t * sustain);
        }
    }
    (void)bars;
    // batterie
    if (T.drums != 3) for (int k = 0; k < T.n; k++) {
        int i0 = (int)(k * eighth * SR), len = (int)(eighth * SR);
        for (int i = 0; i < len && i0 + i < n; i++) {
            float t = (float)i / SR;
            float d = 0;
            if (T.drums == 0) {
                if (k % 4 == 0) d += std::sin(PI2 * (60 + 90 * std::exp(-t * 30)) * t) * std::exp(-t * 18) * 0.35f;
                if (k % 4 == 2) d += frand() * std::exp(-t * 22) * 0.16f;
                d += frand() * std::exp(-t * 90) * 0.05f;
            } else if (T.drums == 1) {
                if (k % 8 == 0) d += std::sin(PI2 * (55 + 70 * std::exp(-t * 30)) * t) * std::exp(-t * 16) * 0.28f;
                if (k % 8 == 4) d += frand() * std::exp(-t * 30) * 0.08f;
                if (k % 2 == 1) d += frand() * std::exp(-t * 120) * 0.03f;
            } else {
                // fanfare : timbales et roulements de caisse claire
                if (k % 4 == 0) d += std::sin(PI2 * (80 + 40 * std::exp(-t * 20)) * t) * std::exp(-t * 8) * 0.3f;
                if (k % 8 >= 6) d += frand() * (0.5f + 0.5f * std::sin(PI2 * 28 * t)) * std::exp(-t * 6) * 0.07f;
            }
            v[i0 + i] += d;
        }
    }
    for (auto& x : v) x *= T.gain;
    return v;
}

void audioInit() {
    InitAudioDevice();
    if (!IsAudioDeviceReady()) return;
    srand(42);
    const float PI2 = 6.2831853f;
    // frappe
    {
        int n = SR * 0.09f; std::vector<float> v(n);
        for (int i = 0; i < n; i++) { float t = (float)i / SR; float e = std::exp(-t * 45); v[i] = (std::sin(PI2 * 110 * t) * 0.9f + frand() * 0.35f) * e; }
        g_sfx[SFX_KICK] = makeSound(v);
    }
    // sifflets
    for (int k = 0; k < 2; k++) {
        float len = k ? 1.1f : 0.35f;
        int n = SR * len; std::vector<float> v(n);
        for (int i = 0; i < n; i++) {
            float t = (float)i / SR;
            float trem = 0.65f + 0.35f * std::sin(PI2 * 32 * t);
            float att = std::min(1.f, t * 60) * std::min(1.f, (len - t) * 25);
            v[i] = (std::sin(PI2 * 2950 * t) * 0.5f + std::sin(PI2 * 3010 * t) * 0.3f + frand() * 0.05f) * trem * att * 0.55f;
        }
        g_sfx[k ? SFX_WHISTLE_LONG : SFX_WHISTLE] = makeSound(v);
        if (!k) g_sfx[SFX_CARD] = makeSound(v);
    }
    // coup de sifflet final : deux coups brefs puis un long
    {
        const float segs[5][2] = { { 0.f, 0.32f }, { 0.47f, 0.79f }, { 0.95f, 2.15f }, { 0, 0 }, { 0, 0 } };
        int n = SR * 2.3f; std::vector<float> v(n, 0.f);
        for (int s2 = 0; s2 < 3; s2++) {
            float a = segs[s2][0], b = segs[s2][1], len = b - a;
            for (int i = (int)(a * SR); i < (int)(b * SR) && i < n; i++) {
                float t = (float)i / SR - a;
                float trem = 0.65f + 0.35f * std::sin(PI2 * 32 * t);
                float att = std::min(1.f, t * 60) * std::min(1.f, (len - t) * 25);
                v[i] = (std::sin(PI2 * 2950 * t) * 0.5f + std::sin(PI2 * 3010 * t) * 0.3f + frand() * 0.05f) * trem * att * 0.6f;
            }
        }
        g_sfx[SFX_WHISTLE_FINAL] = makeSound(v);
    }
    // coup de poing (bagarre)
    {
        int n = SR * 0.12f; std::vector<float> v(n);
        for (int i = 0; i < n; i++) { float t = (float)i / SR; v[i] = (std::sin(PI2 * 70 * t) * 0.8f + frand() * 0.6f) * std::exp(-t * 38); }
        g_sfx[SFX_PUNCH] = makeSound(v);
    }
    // sifflets du public
    // tonnerre (orage)
    g_sfx[SFX_THUNDER] = makeSound(noise(3.0f, 0.03f, 3.2f, [](float t) { return (t < 0.04f ? t * 25 : 1.f) * std::exp(-t * 2.2f) * (0.7f + 0.3f * std::sin(t * 40)); }));
    // but : clameur
    g_sfx[SFX_GOAL] = makeSound(noise(3.5f, 0.25f, 2.2f, [](float t) { return std::min(1.f, t * 8) * (1 - t) * (1 - t) + 0.2f * (1 - t); }));
    g_sfx[SFX_CROWD_OOH] = makeSound(noise(1.2f, 0.12f, 2.5f, [](float t) { return std::sin(t * 3.14159f); }));
    {
        int n = SR * 0.05f; std::vector<float> v(n);
        for (int i = 0; i < n; i++) { float t = (float)i / SR; v[i] = std::sin(PI2 * 80 * t) * std::exp(-t * 60) * 0.8f; }
        g_sfx[SFX_BOUNCE] = makeSound(v);
    }
    {
        int n = SR * 0.4f; std::vector<float> v(n);
        for (int i = 0; i < n; i++) { float t = (float)i / SR; v[i] = (std::sin(PI2 * 880 * t) + 0.6f * std::sin(PI2 * 1370 * t)) * std::exp(-t * 9) * 0.5f; }
        g_sfx[SFX_POST] = makeSound(v);
    }
    g_crowd = makeSound(noise(4.0f, 0.08f, 1.6f, [](float t) { return 0.75f + 0.25f * std::sin(t * 6.2831f * 2); }));
    for (int kind = 0; kind < NUM_CHANTS; kind++) g_fanVoice[kind] = makeSound(renderChant(kind));
    // la ola : clameur qui monte et redescend en faisant le tour du stade
    {
        std::vector<float> v((size_t)(4.5f * SR), 0.f);
        std::vector<Syll> song = { { 0.1f, 4.1f, 0, &V_O, 1.f } };
        addChoir(v, song, 260.f, 24, 0.8f, 67);
        addMurmur(v, 1.0f, 0.12f);
        for (size_t i = 0; i < v.size(); i++) { float u = (float)i / v.size(); v[i] *= std::sin(u * 3.14159f) * (0.6f + 0.4f * std::sin(u * 3.14159f * 3)); }
        normalize(v, 0.8f, 0.2f);
        g_sfx[SFX_OLA] = makeSound(v);
    }
    // sifflets brefs du public (décision contre l'équipe locale) et applaudissements
    {
        std::vector<float> v((size_t)(2.2f * SR), 0.f);
        addCrowdWhistles(v, 34, 1.f, 53); addMurmur(v, 0.15f, 0.1f);
        normalize(v, 0.8f, 0.25f);
        g_sfx[SFX_FANS_WHISTLE] = makeSound(v);
        std::vector<float> c((size_t)(2.4f * SR), 0.f);
        srand(59);
        for (int k = 0; k < 90; k++) addClap(c, std::fabs(frand()) * 2.1f, 0.35f + 0.3f * std::fabs(frand()));
        normalize(c, 0.7f, 0.3f);
        g_sfx[SFX_CLAPS] = makeSound(c);
        // huées : « Hououou » grave, chœur d'hommes
        std::vector<float> bo((size_t)(1.8f * SR), 0.f);
        std::vector<Syll> song = { { 0.05f, 1.6f, 0, &V_OU, 1.f } };
        addChoir(bo, song, 110.f, 20, 1.2f, 61); addMurmur(bo, 0.25f, 0.08f);
        normalize(bo, 0.75f, 0.3f);
        g_sfx[SFX_BOO] = makeSound(bo);
    }
    // musiques des menus : thèmes chiptune originaux (voir renderTrack)
    for (int k = 0; k < NUM_TRACKS; k++) { g_tracks[k] = makeSound(renderTrack(TRACKS[k])); }
    g_music = g_tracks[0];
    g_musicOk = true;
    g_jingles[0] = makeSound(renderTrack(JINGLE_WIN));
    g_jingles[1] = makeSound(renderTrack(JINGLE_TV));
    g_jingles[2] = makeSound(renderTrack(JINGLE_TROPHY));
    g_jingles[3] = makeSound(renderTrack(JINGLE_ANTHEM));
    g_jingles[4] = makeSound(renderTrack(JINGLE_PODIUM));
    g_jingles[5] = makeSound(renderTrack(JINGLE_AD1));
    g_jingles[6] = makeSound(renderTrack(JINGLE_AD2));
    g_jingles[7] = makeSound(renderTrack(JINGLE_AD3));
    g_jingles[8] = makeSound(renderTrack(JINGLE_GOAL));
    g_ok = true;
}

void audioShutdown() {
    if (!g_ok) return;
    for (auto& s : g_sfx) UnloadSound(s);
    UnloadSound(g_crowd);for(auto& sound:g_fanVoice)UnloadSound(sound);
    if (g_musicOk) { for (auto& t : g_tracks) UnloadSound(t); for (auto& j : g_jingles) UnloadSound(j); }
    CloseAudioDevice();
}

void audioSetEnabled(bool e) { g_enabled = e; if (!e && g_ok){StopSound(g_crowd);for(auto& sound:g_fanVoice)StopSound(sound);} }

void audioPlay(int s) {
    if (!g_ok || !g_enabled || s < 0 || s >= NUM_SFX) return;
    PlaySound(g_sfx[s]);
}

void audioCrowd(bool on, float vol) {
    if (!g_ok) return;
    if (!on || !g_enabled) { if (IsSoundPlaying(g_crowd)) StopSound(g_crowd);for(auto& sound:g_fanVoice)StopSound(sound);return; }
    SetSoundVolume(g_crowd, vol);
    if (!IsSoundPlaying(g_crowd)) PlaySound(g_crowd);
}

bool audioAnthemPlaying();
void audioMusic(bool on) {
    if (!g_ok || !g_musicOk) return;
    if (audioAnthemPlaying()) { if (IsSoundPlaying(g_music)) StopSound(g_music); return; }
    if (!on || !g_musicOn) { if (IsSoundPlaying(g_music)) StopSound(g_music); return; }
    SetSoundVolume(g_music, 0.55f);
    if (!IsSoundPlaying(g_music)) {
        for (auto& j : g_jingles) if (IsSoundPlaying(j)) return;      // on laisse finir le jingle
        // morceau terminé : enchaînement automatique sur le suivant
        if (g_trackMode == 0) { static bool first = true; if (!first) g_curTrack = (g_curTrack + 1) % NUM_TRACKS; first = false; }
        else g_curTrack = std::max(0, std::min(NUM_TRACKS - 1, g_trackMode - 1));
        g_music = g_tracks[g_curTrack];
        SetSoundVolume(g_music, 0.55f);
        PlaySound(g_music);
    }
}
void audioMusicEnabled(bool e) { g_musicOn = e; if (!e && g_musicOk) StopSound(g_music); }
void audioSetTrackMode(int m) {
    if (m == g_trackMode) return;
    g_trackMode = m;
    if (g_musicOk && IsSoundPlaying(g_music)) StopSound(g_music);
    if (m > 0) g_curTrack = std::max(0, std::min(NUM_TRACKS - 1, m - 1));
}
int audioTrackCount() { return NUM_TRACKS; }
const char* audioTrackName(int i) { return i >= 0 && i < NUM_TRACKS ? TRACKS[i].name : ""; }
int audioCurrentTrack() { return g_curTrack; }
// ------------------------------------------------------------------ hymnes nationaux
// Mélodies réelles (domaine public) pour les grandes nations, hymne générique pour les autres,
// ou fichier musical choisi par le joueur (éditeur d'hymnes). Joués même si la musique est coupée.
#include <map>
#include <string>
struct ANote { int n, d; };     // demi-tons depuis la tonique (-99 silence), durée en doubles croches
static const ANote AN_FRA[] = {    // La Marseillaise (Rouget de Lisle, 1792) : « Allons enfants... » (x2)
    { -5, 3 }, { -5, 1 }, { 0, 4 }, { 0, 4 }, { 2, 4 }, { 2, 4 }, { 7, 6 }, { 4, 2 }, { 0, 3 }, { 0, 1 }, { 4, 3 }, { 0, 1 },
    { -3, 4 }, { 5, 8 }, { 2, 3 }, { -1, 1 }, { 0, 8 }, { -99, 4 },
    { -5, 3 }, { -5, 1 }, { 0, 4 }, { 0, 4 }, { 2, 4 }, { 2, 4 }, { 7, 6 }, { 4, 2 }, { 0, 3 }, { 0, 1 }, { 4, 3 }, { 0, 1 },
    { -3, 4 }, { 5, 8 }, { 2, 3 }, { -1, 1 }, { 0, 12 }, { -99, 4 },
    // « Aux armes, citoyens ! Formez vos bataillons ! Marchons, marchons ! »
    { 4, 8 }, { 4, 3 }, { 4, 1 }, { 6, 4 }, { 7, 8 }, { -99, 4 }, { 7, 8 }, { 7, 3 }, { 7, 1 }, { 9, 4 }, { 11, 8 }, { -99, 4 },
    { 14, 8 }, { 12, 3 }, { 11, 1 }, { 9, 3 }, { 7, 1 }, { 9, 4 }, { 11, 4 }, { 7, 6 }, { 4, 2 }, { 2, 4 }, { 0, 12 } };
static const ANote AN_ENG[] = {    // God Save the King
    { 0, 4 }, { 0, 4 }, { 2, 4 }, { -1, 6 }, { 0, 2 }, { 2, 4 }, { 4, 4 }, { 4, 4 }, { 5, 4 }, { 4, 6 }, { 2, 2 }, { 0, 4 },
    { 2, 4 }, { 0, 4 }, { -1, 4 }, { 0, 12 },
    { 7, 4 }, { 7, 4 }, { 7, 4 }, { 7, 6 }, { 5, 2 }, { 4, 4 }, { 5, 4 }, { 5, 4 }, { 5, 4 }, { 5, 6 }, { 4, 2 }, { 2, 4 },
    { 4, 2 }, { 5, 2 }, { 4, 2 }, { 2, 2 }, { 0, 4 }, { 4, 6 }, { 5, 2 }, { 7, 4 }, { 9, 2 }, { 5, 2 }, { 4, 4 }, { 2, 4 }, { 0, 12 } };
static const ANote AN_GER[] = {    // Deutschlandlied (Haydn, 1797)
    { 0, 6 }, { 2, 2 }, { 4, 4 }, { 2, 4 }, { 5, 4 }, { 4, 4 }, { 2, 2 }, { -1, 2 }, { 0, 4 }, { 9, 4 }, { 7, 4 }, { 5, 4 }, { 4, 4 },
    { 2, 6 }, { 4, 2 }, { 0, 8 },
    { 0, 6 }, { 2, 2 }, { 4, 4 }, { 2, 4 }, { 5, 4 }, { 4, 4 }, { 2, 2 }, { -1, 2 }, { 0, 4 }, { 9, 4 }, { 7, 4 }, { 5, 4 }, { 4, 4 },
    { 2, 6 }, { 4, 2 }, { 0, 8 },
    { 2, 4 }, { 2, 4 }, { 4, 4 }, { 0, 4 }, { 2, 4 }, { 4, 2 }, { 5, 2 }, { 4, 4 }, { 0, 4 }, { 2, 4 }, { 4, 2 }, { 5, 2 }, { 4, 4 }, { 2, 4 },
    { 0, 4 }, { 2, 4 }, { -5, 8 },
    { 12, 6 }, { 11, 2 }, { 11, 4 }, { 9, 4 }, { 7, 8 }, { 9, 4 }, { 11, 4 }, { 12, 6 }, { 9, 2 }, { 7, 4 }, { 5, 4 }, { 4, 6 }, { 2, 2 }, { 0, 8 } };
static const ANote AN_USA[] = {    // The Star-Spangled Banner
    { 7, 3 }, { 4, 1 }, { 0, 4 }, { 4, 4 }, { 7, 4 }, { 12, 8 }, { 16, 3 }, { 14, 1 }, { 12, 4 }, { 4, 4 }, { 6, 4 }, { 7, 8 },
    { 7, 2 }, { 7, 2 }, { 16, 6 }, { 14, 2 }, { 12, 4 }, { 11, 8 }, { 9, 3 }, { 11, 1 }, { 12, 4 }, { 12, 4 }, { 7, 4 }, { 4, 4 }, { 0, 4 }, { -99, 4 },
    { 7, 3 }, { 4, 1 }, { 0, 4 }, { 4, 4 }, { 7, 4 }, { 12, 8 }, { 16, 3 }, { 14, 1 }, { 12, 4 }, { 4, 4 }, { 6, 4 }, { 7, 8 },
    { 7, 2 }, { 7, 2 }, { 16, 6 }, { 14, 2 }, { 12, 4 }, { 11, 8 }, { 9, 3 }, { 11, 1 }, { 12, 4 }, { 12, 4 }, { 7, 4 }, { 4, 4 }, { 0, 12 } };
static const ANote AN_RUS[] = {    // hymne de la Fédération de Russie (musique d'A. Alexandrov)
    { -5, 4 }, { 0, 8 }, { -5, 6 }, { -3, 2 }, { -1, 8 }, { -8, 4 }, { -8, 4 }, { -3, 8 }, { -5, 6 }, { -7, 2 }, { -5, 8 }, { -12, 4 }, { -12, 4 },
    { -10, 8 }, { -10, 6 }, { -8, 2 }, { -7, 8 }, { -7, 6 }, { -5, 2 }, { -3, 8 }, { -1, 6 }, { 0, 2 }, { 2, 12 }, { -5, 4 },
    { 4, 8 }, { 2, 6 }, { 0, 2 }, { 2, 8 }, { -1, 4 }, { -5, 4 }, { 0, 8 }, { -1, 6 }, { -3, 2 }, { -1, 8 }, { -8, 4 }, { -8, 4 },
    { -3, 8 }, { -5, 6 }, { -7, 2 }, { -5, 8 }, { -12, 4 }, { -12, 4 }, { 0, 8 }, { -1, 6 }, { -3, 2 }, { -5, 8 }, { 2, 8 }, { 0, 16 } };
static const ANote AN_CAN[] = {    // Ô Canada (Calixa Lavallée, 1880)
    { 4, 8 }, { 7, 6 }, { 7, 2 }, { 0, 12 }, { -99, 4 }, { 2, 4 }, { 4, 4 }, { 5, 4 }, { 7, 4 }, { 9, 4 }, { 2, 12 }, { -99, 4 },
    { 4, 8 }, { 6, 6 }, { 6, 2 }, { 7, 8 }, { 9, 4 }, { 11, 4 }, { 11, 8 }, { 9, 8 }, { -99, 4 },
    { 9, 4 }, { 7, 4 }, { 5, 4 }, { 4, 4 }, { 2, 4 }, { 4, 4 }, { 5, 8 }, { 7, 4 }, { 9, 4 }, { 7, 8 }, { 5, 4 }, { 4, 4 }, { 2, 8 }, { 0, 16 } };
// hymnes génériques : compositions originales, attribuées de façon stable à chaque sélection sans hymne connu
static const ANote AN_GEN[] = {    // marche majestueuse
    { 0, 4 }, { 4, 4 }, { 7, 6 }, { 5, 2 }, { 4, 4 }, { 2, 4 }, { 0, 8 }, { 5, 4 }, { 4, 4 }, { 2, 6 }, { 4, 2 }, { 7, 12 }, { -99, 4 },
    { 9, 4 }, { 7, 4 }, { 5, 6 }, { 4, 2 }, { 2, 4 }, { 4, 4 }, { 5, 8 }, { 7, 4 }, { 9, 4 }, { 11, 4 }, { 12, 4 }, { 12, 12 }, { -99, 4 },
    { 12, 4 }, { 11, 4 }, { 9, 6 }, { 7, 2 }, { 5, 4 }, { 4, 4 }, { 2, 8 }, { 4, 4 }, { 7, 4 }, { 5, 6 }, { 2, 2 }, { 0, 12 } };
static const ANote AN_GEN2[] = {   // hymne solennel en mineur
    { 0, 8 }, { 3, 4 }, { 7, 4 }, { 8, 8 }, { 7, 4 }, { 5, 4 }, { 3, 6 }, { 2, 2 }, { 0, 4 }, { 2, 4 }, { 3, 12 }, { -99, 4 },
    { 7, 8 }, { 8, 4 }, { 10, 4 }, { 12, 8 }, { 10, 4 }, { 8, 4 }, { 7, 6 }, { 5, 2 }, { 3, 4 }, { 5, 4 }, { 7, 12 }, { -99, 4 },
    { 12, 6 }, { 10, 2 }, { 8, 4 }, { 7, 4 }, { 8, 6 }, { 7, 2 }, { 5, 4 }, { 3, 4 }, { 2, 8 }, { -1, 4 }, { 2, 4 }, { 0, 16 } };
static const ANote AN_GEN3[] = {   // hymne à trois temps (choral)
    { 0, 8 }, { 4, 4 }, { 7, 12 }, { 9, 4 }, { 7, 4 }, { 5, 4 }, { 4, 12 }, { 2, 4 }, { 4, 4 }, { 5, 4 }, { 7, 12 }, { 4, 12 },
    { 5, 8 }, { 7, 4 }, { 9, 12 }, { 12, 4 }, { 11, 4 }, { 9, 4 }, { 7, 12 }, { 5, 4 }, { 4, 4 }, { 2, 4 }, { 0, 24 } };
static const ANote AN_GEN4[] = {   // fanfare vive
    { 7, 2 }, { 7, 2 }, { 12, 6 }, { 7, 2 }, { 4, 4 }, { 7, 4 }, { 12, 6 }, { 11, 2 }, { 9, 4 }, { 7, 4 }, { 5, 8 }, { -99, 4 },
    { 5, 2 }, { 5, 2 }, { 9, 6 }, { 5, 2 }, { 2, 4 }, { 5, 4 }, { 11, 6 }, { 9, 2 }, { 7, 4 }, { 5, 4 }, { 4, 8 }, { -99, 4 },
    { 4, 4 }, { 7, 4 }, { 12, 8 }, { 14, 4 }, { 16, 4 }, { 14, 6 }, { 12, 2 }, { 11, 4 }, { 14, 4 }, { 12, 16 } };
static const ANote AN_GEN5[] = {   // hymne lyrique en mineur, envolée finale en majeur
    { -5, 4 }, { 0, 6 }, { 2, 2 }, { 3, 4 }, { 0, 4 }, { 7, 8 }, { 5, 4 }, { 3, 4 }, { 2, 6 }, { 3, 2 }, { 5, 4 }, { 2, 4 }, { -5, 12 }, { -99, 4 },
    { -5, 4 }, { 0, 6 }, { 2, 2 }, { 3, 4 }, { 5, 4 }, { 7, 8 }, { 8, 4 }, { 10, 4 }, { 12, 8 }, { 7, 8 }, { -99, 4 },
    { 12, 6 }, { 11, 2 }, { 12, 4 }, { 7, 4 }, { 8, 6 }, { 7, 2 }, { 5, 4 }, { 3, 4 }, { 2, 6 }, { 3, 2 }, { 2, 4 }, { -1, 4 }, { 0, 16 } };
static const ANote AN_GEN6[] = {   // hymne populaire, mélodie simple et chantante
    { 0, 4 }, { 0, 4 }, { 4, 4 }, { 7, 4 }, { 9, 6 }, { 7, 2 }, { 4, 8 }, { 5, 4 }, { 5, 4 }, { 4, 4 }, { 2, 4 }, { 4, 12 }, { -99, 4 },
    { 0, 4 }, { 0, 4 }, { 4, 4 }, { 7, 4 }, { 12, 6 }, { 11, 2 }, { 9, 8 }, { 7, 4 }, { 5, 4 }, { 4, 4 }, { 2, 4 }, { 0, 12 }, { -99, 4 },
    { 9, 4 }, { 9, 4 }, { 11, 4 }, { 12, 4 }, { 14, 6 }, { 12, 2 }, { 9, 8 }, { 7, 4 }, { 9, 4 }, { 7, 4 }, { 4, 4 }, { 2, 8 }, { 7, 8 }, { 0, 16 } };
struct AnthemDef { const char* codes; const ANote* n; int cnt; float bpm; float base; const char* name; bool minor = false; };
#define ANL(a) a, (int)(sizeof(a) / sizeof(a[0]))
static const AnthemDef ANTHEMS[] = {
    { "FRA", ANL(AN_FRA), 88.f, 392.00f, "La Marseillaise" },
    { "ENG NIR LIE", ANL(AN_ENG), 66.f, 392.00f, "God Save the King" },
    { "GER", ANL(AN_GER), 70.f, 349.23f, "Das Lied der Deutschen" },
    { "USA", ANL(AN_USA), 80.f, 293.66f, "The Star-Spangled Banner" },
    { "RUS", ANL(AN_RUS), 76.f, 311.13f, "Hymne de la Fédération de Russie" },
    { "CAN", ANL(AN_CAN), 72.f, 349.23f, "Ô Canada" },
};
static const AnthemDef GENERIC_ANTHEMS[] = {
    { "", ANL(AN_GEN), 72.f, 349.23f, "Hymne de la sélection (marche)" },
    { "", ANL(AN_GEN2), 66.f, 329.63f, "Hymne de la sélection (solennel)", true },
    { "", ANL(AN_GEN3), 84.f, 349.23f, "Hymne de la sélection (choral)" },
    { "", ANL(AN_GEN4), 96.f, 311.13f, "Hymne de la sélection (fanfare)" },
    { "", ANL(AN_GEN5), 70.f, 349.23f, "Hymne de la sélection (lyrique)", true },
    { "", ANL(AN_GEN6), 80.f, 392.00f, "Hymne de la sélection (populaire)" },
};
static const int NUM_GENERIC = (int)(sizeof(GENERIC_ANTHEMS) / sizeof(GENERIC_ANTHEMS[0]));
// orchestre synthétique : cuivres (mélodie), cordes (accords tenus), basse, timbales, cymbale finale, réverbération de stade
static std::vector<float> renderAnthem(const ANote* N, int cnt, float bpm, float base, bool minor = false) {
    const float PI2 = 6.2831853f;
    float s16 = 60.f / bpm / 4.f;
    int total = 0; for (int i = 0; i < cnt; i++) total += N[i].d;
    int n = (int)(total * s16 * SR) + SR * 2;
    std::vector<float> v(n, 0.f);
    // accord de chaque temps (4 doubles croches) selon la note de mélodie qui y sonne : I, IV ou V
    int beats = (total + 3) / 4;
    std::vector<int> chord(beats, 0);
    { int pos = 0; for (int i = 0; i < cnt; i++) { for (int q = pos; q < pos + N[i].d; q++) if (q % 4 == 0 && N[i].n > -99) {
        int pc = ((N[i].n % 12) + 12) % 12;
        int c = (pc == 0 || pc == (minor ? 3 : 4) || pc == 7) ? 0 : (pc == 5 || pc == 9 || pc == 8) ? 5 : 7;
        chord[q / 4] = c; } pos += N[i].d; } }
    chord[beats - 1] = 0;
    int pos = 0;
    for (int i = 0; i < cnt; i++) {
        int i0 = (int)(pos * s16 * SR), len = (int)(N[i].d * s16 * SR);
        pos += N[i].d;
        if (N[i].n <= -99) continue;
        float f = base * std::pow(2.f, N[i].n / 12.f);
        float dur = (float)len / SR;
        for (int k = 0; k < len + SR / 10 && i0 + k < n; k++) {
            float t = (float)k / SR;
            float vib = t > 0.2f ? 1.f + 0.005f * std::sin(PI2 * 5.f * t) : 1.f;
            float ph = std::fmod(t * f * vib, 1.f), ph2 = std::fmod(t * f * 1.003f, 1.f);
            float brass = (ph < 0.3f ? 1.f : -0.6f) * 0.55f + (2.f * ph - 1.f) * 0.45f;           // cuivres (deux pupitres légèrement désaccordés)
            float brass2 = 2.f * ph2 - 1.f;
            float env = std::min(1.f, t * 50) * (0.75f + 0.25f * std::exp(-t * 5)) * (t < dur ? std::min(1.f, (dur - t) * 30 + 0.15f) : std::max(0.f, 0.15f - (t - dur) * 1.5f));
            v[i0 + k] += (brass * 0.08f + brass2 * 0.05f) * env;
        }
    }
    // cordes et basse : accords tenus, attaque douce
    for (int bt = 0; bt < beats; bt++) {
        int i0 = (int)(bt * 4 * s16 * SR), len = (int)(4 * s16 * SR);
        int root = chord[bt];
        int third = (root == 0 && minor) || (root == 5 && minor) ? 3 : 4;
        float fr[3] = { base * 0.5f * std::pow(2.f, root / 12.f), base * 0.5f * std::pow(2.f, (root + third) / 12.f), base * 0.5f * std::pow(2.f, (root + 7) / 12.f) };
        float fb = base * 0.25f * std::pow(2.f, (root > 6 ? root - 12 : root) / 12.f);
        bool change = bt == 0 || chord[bt - 1] != root;
        for (int k = 0; k < len && i0 + k < n; k++) {
            float t = (float)(i0 + k) / SR, tl = (float)k / SR;
            float att = change ? std::min(1.f, tl * 8.f) : 1.f;
            float pad = 0;
            for (int c = 0; c < 3; c++) { float p1 = std::fmod(t * fr[c], 1.f), p2 = std::fmod(t * fr[c] * 1.004f, 1.f); pad += (2 * p1 - 1) + (2 * p2 - 1); }
            float bass = std::sin(PI2 * fb * t) + 0.3f * std::sin(PI2 * fb * 2 * t);
            v[i0 + k] += pad * 0.018f * att + bass * 0.07f * std::min(1.f, tl * 20.f) * (0.6f + 0.4f * std::exp(-tl * 3.f));
        }
        // timbale sur les temps forts
        if (bt % 4 == 0 || bt == beats - 1) for (int k = 0; k < SR / 2 && i0 + k < n; k++) { float t = (float)k / SR; v[i0 + k] += std::sin(PI2 * (fb * 2 + 25 * std::exp(-t * 20)) * t) * std::exp(-t * 6) * 0.2f; }
    }
    // cymbale et roulement de timbales sur la dernière note
    {
        int last = (int)((total - N[cnt - 1].d) * s16 * SR);
        for (int k = 0; k < SR * 2 && last + k < n; k++) { float t = (float)k / SR; v[last + k] += frand() * std::exp(-t * 1.6f) * 0.09f + std::sin(PI2 * 65 * t) * frand() * 0.05f * std::exp(-t * 2.f); }
    }
    // réverbération (peignes en parallèle) : l'hymne résonne dans le stade
    {
        static const int D[4] = { 1557, 1617, 1491, 1422 };
        std::vector<float> wet(n, 0.f);
        for (int c = 0; c < 4; c++) { std::vector<float> buf(D[c], 0.f); int bi = 0; for (int k = 0; k < n; k++) { float o = buf[bi]; buf[bi] = v[k] + o * 0.78f; bi = (bi + 1) % D[c]; wet[k] += o * 0.25f; } }
        for (int k = 0; k < n; k++) v[k] = v[k] * 0.8f + wet[k] * 0.35f;
    }
    // roulement de caisse claire en introduction
    std::vector<float> out(SR * 1 + n, 0.f);
    for (int k = 0; k < SR; k++) { float t = (float)k / SR; out[k] = frand() * (0.5f + 0.5f * std::sin(PI2 * 30 * t)) * 0.07f * std::min(1.f, t * 3); }
    for (int k = 0; k < n; k++) out[SR + k] += v[k];
    normalize(out, 0.85f, 0.01f);
    return out;
}
static int genericAnthem(const std::string& code) { unsigned h = 2166136261u; for (char c : code) h = (h ^ (unsigned char)c) * 16777619u; return (int)(h % NUM_GENERIC); }
static std::map<std::string, std::string> g_anthemFiles;     // code de la sélection -> fichier musical (éditeur d'hymnes)
static std::map<int, Sound> g_anthemSnd;                     // index de ANTHEMS (-1 générique)
static Sound g_anthemCur; static bool g_anthemOn = false;
static Music g_anthemMus; static bool g_anthemMusOn = false;
static float g_anthemLen = 10.f;

void audioSetAnthemFile(const std::string& code, const std::string& path) { if (path.empty()) g_anthemFiles.erase(code); else g_anthemFiles[code] = path; }
std::string audioAnthemFile(const std::string& code) { auto it = g_anthemFiles.find(code); return it == g_anthemFiles.end() ? std::string() : it->second; }
const std::map<std::string, std::string>& audioAnthemFiles() { return g_anthemFiles; }
static int anthemIndex(const std::string& code) {
    for (int i = 0; i < (int)(sizeof(ANTHEMS) / sizeof(ANTHEMS[0])); i++) {
        std::string cs = ANTHEMS[i].codes; size_t p = 0;
        while (p < cs.size()) { size_t e = cs.find(' ', p); if (e == std::string::npos) e = cs.size(); if (cs.substr(p, e - p) == code) return i; p = e + 1; }
    }
    return -1;
}
const char* audioAnthemName(const std::string& code) { int i = anthemIndex(code); return i >= 0 ? ANTHEMS[i].name : GENERIC_ANTHEMS[genericAnthem(code)].name; }
float audioAnthemSeconds() { return g_anthemLen; }
void audioStopAnthem() {
    if (!g_ok) return;
    if (g_anthemOn) { StopSound(g_anthemCur); g_anthemOn = false; }
    if (g_anthemMusOn) { StopMusicStream(g_anthemMus); UnloadMusicStream(g_anthemMus); g_anthemMusOn = false; }
}
void audioWalkoutStop();
void audioAnthemCode(const std::string& code) {
    if (!g_ok) return;
    audioStopAnthem();
    if (g_musicOk && IsSoundPlaying(g_music)) StopSound(g_music);
    for (auto& jg : g_jingles) if (IsSoundPlaying(jg)) StopSound(jg);
    audioWalkoutStop();
    std::string file = audioAnthemFile(code);
    if (!file.empty() && FileExists(file.c_str())) {
        g_anthemMus = LoadMusicStream(file.c_str());
        if (IsMusicValid(g_anthemMus)) {
            g_anthemMus.looping = false;
            SetMusicVolume(g_anthemMus, 0.8f);
            PlayMusicStream(g_anthemMus);
            g_anthemMusOn = true;
            g_anthemLen = std::min(90.f, GetMusicTimeLength(g_anthemMus));
            return;
        }
    }
    int idx = anthemIndex(code);
    if (idx < 0) idx = -1 - genericAnthem(code);       // -1 ... -6 : hymnes génériques
    auto it = g_anthemSnd.find(idx);
    if (it == g_anthemSnd.end()) {
        const AnthemDef& A = idx >= 0 ? ANTHEMS[idx] : GENERIC_ANTHEMS[-1 - idx];
        std::vector<float> w = renderAnthem(A.n, A.cnt, A.bpm, A.base, A.minor);
        g_anthemSnd[idx] = makeSound(w);
        it = g_anthemSnd.find(idx);
        g_anthemLen = (float)w.size() / SR;
    } else g_anthemLen = (float)it->second.frameCount / SR;
    g_anthemCur = it->second; g_anthemOn = true;
    SetSoundVolume(g_anthemCur, 0.7f);
    PlaySound(g_anthemCur);
}
bool audioAnthemPlaying() { return (g_anthemOn && IsSoundPlaying(g_anthemCur)) || (g_anthemMusOn && IsMusicStreamPlaying(g_anthemMus)); }
// musique d'entrée des joueurs (fichier choisi dans l'éditeur, par compétition)
static Music g_walkMus; static bool g_walkOn = false; static float g_walkFade = -1;
void audioWalkoutPlay(const std::string& path) {
    audioWalkoutStop();
    if (!g_ok || path.empty() || !FileExists(path.c_str())) return;
    if (g_musicOk && IsSoundPlaying(g_music)) StopSound(g_music);
    for (auto& jg : g_jingles) if (IsSoundPlaying(jg)) StopSound(jg);
    audioStopAnthem();
    g_walkMus = LoadMusicStream(path.c_str());
    if (!IsMusicValid(g_walkMus)) return;
    g_walkMus.looping = false; SetMusicVolume(g_walkMus, 0.75f); PlayMusicStream(g_walkMus); g_walkOn = true; g_walkFade = -1;
}
void audioWalkoutStop() { if (g_walkOn) { StopMusicStream(g_walkMus); UnloadMusicStream(g_walkMus); g_walkOn = false; } }
void audioWalkoutFade() { if (g_walkOn && g_walkFade < 0) g_walkFade = 1.f; }
bool audioWalkoutPlaying() { return g_walkOn; }
void audioUpdate() {
    if (g_ok && g_anthemMusOn) { UpdateMusicStream(g_anthemMus); if (!IsMusicStreamPlaying(g_anthemMus)) { UnloadMusicStream(g_anthemMus); g_anthemMusOn = false; } }
    if (g_ok && g_walkOn) {
        UpdateMusicStream(g_walkMus);
        if (g_walkFade >= 0) { g_walkFade -= GetFrameTime() * 0.5f; SetMusicVolume(g_walkMus, 0.75f * std::max(0.f, g_walkFade)); if (g_walkFade <= 0) audioWalkoutStop(); }
        else if (!IsMusicStreamPlaying(g_walkMus)) audioWalkoutStop();
    }
}

void audioJingle(int j) {
    if (!g_ok || !g_musicOn || j < 0 || j > 8) return;
    // une nouvelle musique coupe la précédente (menu, autre jingle, musique d'entrée)
    if (g_musicOk && IsSoundPlaying(g_music)) StopSound(g_music);
    for (int k = 0; k < 9; k++) if (k != j && IsSoundPlaying(g_jingles[k])) StopSound(g_jingles[k]);
    audioWalkoutStop();
    SetSoundVolume(g_jingles[j], 0.6f);
    PlaySound(g_jingles[j]);
}

// chants en boucle selon l'humeur du public ; un court silence entre deux reprises, comme dans un vrai stade
void audioSupporters(int state, float strength) {
    if (!g_ok || !g_enabled) return;
    static float rest = 0; static int last = -1, playing = -1, rot = 0;
    int mood = state == CH_WHISTLES ? 1 : state == CH_PROTEST ? 2 : state == CH_ENCOURAGE ? 3 : state == CH_CELEBRATE ? 4 : 0;
    // chaque humeur a son répertoire : les chants s'enchaînent sans se répéter
    static const int REP[5][5] = { { 0, 9, 5, 10, 7 }, { 1, 1, 1, 1, 1 }, { 2, 2, 2, 2, 2 }, { 3, 9, 6, 11, 3 }, { 4, 10, 8, 11, 9 } };
    if (state == CH_TENSE || strength <= 0) { for (auto& sound : g_fanVoice) StopSound(sound); last = playing = -1; return; }
    if (mood != last) { for (auto& sound : g_fanVoice) StopSound(sound); playing = -1; rest = 0; }
    float volume = std::clamp(strength, 0.f, 1.f) * (state == CH_NORMAL ? .16f : state == CH_LOUD ? .3f : state == CH_ENCOURAGE ? .3f : state == CH_WHISTLES ? .34f : .42f);
    if (playing >= 0 && IsSoundPlaying(g_fanVoice[playing])) { SetSoundVolume(g_fanVoice[playing], volume); rest = (state == CH_NORMAL ? 2.5f : 0.6f); last = mood; return; }
    if (last == mood && rest > 0) { rest -= GetFrameTime(); return; }
    playing = REP[mood][rot++ % 5];
    SetSoundVolume(g_fanVoice[playing], volume);
    PlaySound(g_fanVoice[playing]); last = mood;
}
