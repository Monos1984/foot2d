// Sons synthétisés (aucun fichier externe)
#include "audio.h"
#include <cstring>
#include "raylib.h"
#include "match.h"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>

static Sound g_sfx[NUM_SFX];
static Sound g_crowd;
static const int NUM_CHANTS = 14;
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
// Voix de la foule (build 25) : chaque supporter est une source glottale (dent de scie avec micro-variations de hauteur
// et d'intensité), toute la foule passe dans trois formants qui suivent la voyelle chantée, avec consonnes
// (sifflantes, occlusives), souffle, imprécision des attaques et de la justesse, puis l'acoustique du stade
// (passe-bas, écho de la tribune d'en face, réverbération).
struct Vowel { float f1, f2, f3; };
static const Vowel V_A = { 760, 1150, 2500 }, V_E = { 480, 1900, 2550 }, V_O = { 520, 860, 2450 }, V_I = { 310, 2150, 2950 }, V_OU = { 330, 800, 2300 };
struct Syll { float t0, len; int semi; const Vowel* v; float accent; char cons = 0; };   // cons : 's' sifflante, 'd' occlusive, 'l' attaque douce, 'h' souffle
struct Biquad {   // passe-bande RBJ (gain 0 dB au centre)
    float b0 = 0, b2 = 0, a1 = 0, a2 = 0, x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    void setBP(float f, float q) { float w = 6.2831853f * f / SR, al = std::sin(w) / (2 * q), a0 = 1 + al; b0 = al / a0; b2 = -al / a0; a1 = -2 * std::cos(w) / a0; a2 = (1 - al) / a0; }
    float run(float x) { float y = b0 * x + b2 * x2 - a1 * y1 - a2 * y2; x2 = x1; x1 = x; y2 = y1; y1 = y; return y; }
};
static float urand() { return (float)rand() / RAND_MAX; }
// acoustique du stade : passe-bas doux, écho de la tribune opposée, réverbération (peignes parallèles)
static void stadiumSpace(std::vector<float>& v, float wetAmt, float lpCoef) {
    float y = 0;
    for (auto& x : v) { y += (x - y) * lpCoef; x = y; }
    int n = (int)v.size(), echo = (int)(0.085f * SR);
    std::vector<float> dry = v;
    for (int k = echo; k < n; k++) v[k] += dry[k - echo] * 0.28f;
    static const int D[4] = { 1687, 1601, 2053, 1931 };
    std::vector<float> wet(n, 0.f);
    for (int c = 0; c < 4; c++) { std::vector<float> buf(D[c], 0.f); int bi = 0; for (int k = 0; k < n; k++) { float o = buf[bi]; buf[bi] = v[k] + o * 0.72f; bi = (bi + 1) % D[c]; wet[k] += o * 0.25f; } }
    for (int k = 0; k < n; k++) v[k] = v[k] * (1.f - wetAmt * 0.5f) + wet[k] * wetAmt;
}
// chœur de nv supporters sur une suite de syllabes ; base = fréquence du demi-ton 0
static void addChoir(std::vector<float>& out, const std::vector<Syll>& song, float base, int nv, float gain, unsigned seed) {
    const float PI2 = 6.2831853f;
    srand(seed);
    int n = (int)out.size();
    std::vector<float> src(n, 0.f), env(n, 0.f), cons(n, 0.f);
    nv = std::max(nv, 18) * 2;                                                  // une foule : beaucoup de voix
    for (int vi = 0; vi < nv; vi++) {
        float reg = vi % 4 == 0 ? 1.f : 0.5f;                                   // surtout des voix d'hommes (octave grave)
        float vibF = 4.5f + urand() * 1.5f, vibA = 0.003f + 0.006f * urand(), loud = 0.55f + 0.45f * urand();
        float rough = 0.002f + 0.004f * urand();                                // voix éraillée : instabilité de hauteur
        float ph = urand(), jit = 0;
        float lateBias = urand() * 0.06f;                                       // certains sont toujours un peu en retard
        for (const Syll& sy : song) {
            if (urand() < 0.07f) continue;                                      // il ne chante pas cette syllabe
            float onset = lateBias + urand() * 0.07f;
            int i0 = (int)((sy.t0 + onset) * SR), len = (int)(sy.len * SR * (0.85f + 0.25f * urand()));
            float err = std::pow(2.f, (urand() + urand() - 1.f) * 55.f / 1200.f); // justesse approximative (± 55 cents)
            float f0 = base * reg * err * std::pow(2.f, sy.semi / 12.f);
            float scoop = 1.f - 0.05f * urand();
            float att = sy.cons == 'l' || sy.cons == 'h' ? 14.f : 30.f;
            for (int i = 0; i < len && i0 + i < n; i++) {
                if (i0 + i < 0) continue;
                float t = (float)i / SR, u = (float)i / len;
                jit += (urand() - 0.5f - jit) * 0.02f;
                float f = f0 * (scoop + (1 - scoop) * std::min(1.f, t * 10.f)) * (1 + vibA * std::sin(PI2 * vibF * t) + jit * rough * 10.f);
                ph += f / SR; ph -= std::floor(ph);
                float e = std::min(1.f, t * att) * (u > 0.75f ? (1 - u) / 0.25f : 1.f) * sy.accent * loud * (0.9f + 0.1f * std::sin(PI2 * 11.f * t + vi));
                float saw = 2.f * ph - 1.f;
                src[i0 + i] += saw * e;
                env[i0 + i] += e;
            }
        }
    }
    // consonnes : bruit coloré juste avant la voyelle (sifflantes « s », « ch ») ou explosion brève (« d », « t »)
    for (const Syll& sy : song) {
        if (!sy.cons || sy.cons == 'l') continue;
        float dur = sy.cons == 's' ? 0.11f : sy.cons == 'h' ? 0.09f : 0.03f;
        int i0 = (int)((sy.t0 - (sy.cons == 's' ? 0.06f : 0.01f)) * SR), len = (int)(dur * SR);
        Biquad bq; bq.setBP(sy.cons == 's' ? 5200.f : sy.cons == 'h' ? 1400.f : 2200.f, sy.cons == 's' ? 1.6f : 0.8f);
        for (int i = 0; i < len + SR / 20 && i0 + i < n; i++) {
            if (i0 + i < 0) continue;
            float t = (float)i / SR;
            float e = sy.cons == 'd' ? std::exp(-t * 90.f) : std::min(1.f, t * 40.f) * std::max(0.f, 1.f - t / (dur + 0.04f));
            cons[i0 + i] += bq.run(urand() * 2.f - 1.f) * e * sy.accent * (sy.cons == 's' ? 0.5f : 0.7f);
        }
    }
    // voix moins nasillardes : la somme des dents de scie est adoucie (passe-bas ~3 kHz) avant les formants
    { float lp = 0.f; for (int k = 0; k < n; k++) { lp += (src[k] - lp) * 0.42f; src[k] = lp; } }
    // formants : ils glissent d'une voyelle à l'autre
    std::vector<float> outv(n, 0.f);
    Biquad F1, F2, F3; float cf1 = song.empty() ? 500 : song[0].v->f1, cf2 = song.empty() ? 1500 : song[0].v->f2, cf3 = 2500;
    size_t si = 0; float breath = 0, body = 0;
    for (int k = 0; k < n; k++) {
        float t = (float)k / SR;
        while (si + 1 < song.size() && song[si + 1].t0 <= t) si++;
        if (k % 32 == 0 && !song.empty()) {
            const Vowel& V = *song[si].v;
            cf1 += (V.f1 - cf1) * 0.12f; cf2 += (V.f2 - cf2) * 0.12f; cf3 += (V.f3 - cf3) * 0.12f;
            F1.setBP(cf1, 5.f); F2.setBP(cf2, 7.f); F3.setBP(cf3, 9.f);
        }
        breath += ((urand() * 2.f - 1.f) - breath) * 0.5f;
        float x = src[k] + breath * env[k] * 0.35f;                             // souffle des voix (cris)
        body += (x - body) * 0.09f;                                             // poitrine : fondamentale et premiers harmoniques
        outv[k] = F1.run(x) * 1.0f + F2.run(x) * 0.75f + F3.run(x) * 0.4f + body * 0.22f + cons[k];
    }
    stadiumSpace(outv, 0.55f, 0.5f);
    float g = gain * 2.2f / std::sqrt((float)nv);
    for (int k = 0; k < n; k++) out[k] += outv[k] * g;
}
// grosse caisse du kop
static void addDrum(std::vector<float>& out, float t0, float amp) {
    const float PI2 = 6.2831853f;
    int i0 = (int)(t0 * SR), n = (int)(0.35f * SR);
    for (int i = 0; i < n && i0 + i < (int)out.size(); i++) { if (i0 + i < 0) continue; float t = (float)i / SR; out[i0 + i] += (std::sin(PI2 * (52 + 70 * std::exp(-t * 25)) * t) * 0.9f + frand() * 0.15f * std::exp(-t * 60)) * std::exp(-t * 9) * amp; }
}
// applaudissements : banque de claquements de mains (bruit filtré à des fréquences variées), joués par des centaines de mains
static std::vector<std::vector<float>>& clapBank() {
    static std::vector<std::vector<float>> bank;
    if (!bank.empty()) return bank;
    unsigned keep = (unsigned)rand(); srand(977);
    for (int k = 0; k < 24; k++) {
        int len = (int)(0.07f * SR); std::vector<float> c(len);
        Biquad bq, bq2; float fc = 700.f + urand() * 2100.f; bq.setBP(fc, 0.9f + urand() * 1.4f); bq2.setBP(fc * (1.6f + urand() * 0.6f), 1.2f);
        float dec = 70.f + urand() * 90.f;
        for (int i = 0; i < len; i++) { float t = (float)i / SR, x = urand() * 2.f - 1.f; float e = std::min(1.f, t * 2500.f) * std::exp(-t * dec); c[i] = (bq.run(x) + bq2.run(x) * 0.5f) * e * 2.5f; }
        bank.push_back(c);
    }
    srand(keep);
    return bank;
}
static void putClap(std::vector<float>& out, int i0, int proto, float g) {
    auto& c = clapBank()[proto % clapBank().size()];
    for (size_t i = 0; i < c.size() && i0 + (int)i < (int)out.size(); i++) if (i0 + (int)i >= 0) out[i0 + i] += c[i] * g;
}
// clappement collectif en rythme (« clap clap ») : des dizaines de mains, pas tout à fait ensemble
static void addClap(std::vector<float>& out, float t0, float amp) {
    for (int k = 0; k < 40; k++) {
        float jit = (urand() + urand() + urand() - 1.5f) * 0.018f;
        putClap(out, (int)((t0 + jit) * SR), rand(), amp * (0.25f + 0.5f * urand()) * 0.35f);
    }
}
// applaudissements nourris : nClap personnes qui applaudissent chacune à son rythme, avec une enveloppe d'ensemble
static void addApplause(std::vector<float>& out, float t0, float dur, int nClap, float amp) {
    for (int p = 0; p < nClap; p++) {
        float rate = 3.2f + urand() * 3.5f, t = t0 + urand() * 0.35f, dist = 0.3f + 0.7f * urand();
        int proto = rand();
        while (t < t0 + dur) {
            float u = (t - t0) / dur;
            float e = std::min(1.f, u * 6.f) * (u > 0.65f ? std::max(0.f, (1.f - u) / 0.35f) : 1.f);
            if (urand() > 0.05f) putClap(out, (int)(t * SR), proto + (urand() < 0.3f ? 1 : 0), amp * dist * e * (0.7f + 0.3f * urand()) * 0.22f);
            t += (1.f / rate) * (0.88f + 0.24f * urand());
        }
    }
}
// rumeur de fond (foule)
static void addMurmur(std::vector<float>& out, float amp, float lp) {
    float y = 0, y2 = 0;
    for (size_t i = 0; i < out.size(); i++) { y += (frand() - y) * lp; y2 += (y - y2) * lp; out[i] += y2 * amp; }
}
// sifflets du public : des sifflements aux doigts (son pur + souffle filtré), par salves, montants, descendants ou « loup »
static void addCrowdWhistles(std::vector<float>& out, int nw, float amp, unsigned seed) {
    const float PI2 = 6.2831853f;
    srand(seed);
    float total = (float)out.size() / SR;
    std::vector<float> tmp(out.size(), 0.f);
    for (int w = 0; w < nw; w++) {
        float f0 = 1900.f + urand() * 1500.f, dist = 0.35f + 0.65f * urand();
        float t = urand() * total * 0.3f;
        int bursts = 1 + (int)(urand() * 3);
        for (int b = 0; b < bursts && t < total; b++) {
            float len = 0.35f + urand() * 1.1f;
            int shape = (int)(urand() * 4);                                     // 0 tenu, 1 montant, 2 descendant, 3 sifflet « loup »
            float wob = 4.f + urand() * 4.f, ph = 0;
            Biquad res; float lastF = -1;
            int i0 = (int)(t * SR), n = (int)(len * SR);
            for (int i = 0; i < n && i0 + i < (int)tmp.size(); i++) {
                float tt = (float)i / SR, u = (float)i / n;
                float f = f0;
                if (shape == 1) f = f0 * (0.82f + 0.18f * std::min(1.f, tt * 6.f));
                else if (shape == 2) f = f0 * (1.f - 0.15f * u);
                else if (shape == 3) f = f0 * (u < 0.4f ? 0.8f + 0.5f * (u / 0.4f) : 1.3f - 0.55f * ((u - 0.4f) / 0.6f));
                f *= 1.f + 0.008f * std::sin(PI2 * wob * tt);
                if (std::fabs(f - lastF) > 15.f) { res.setBP(f, 30.f); lastF = f; }
                ph += PI2 * f / SR; if (ph > PI2) ph -= PI2;
                float e = std::min(1.f, tt * 25.f) * std::min(1.f, (1 - u) * 12.f) * (0.8f + 0.2f * std::sin(PI2 * 9.f * tt + w));
                tmp[i0 + i] += (std::sin(ph) * 0.55f + res.run(urand() * 2.f - 1.f) * 2.2f) * e * dist;
            }
            t += len + 0.15f + urand() * 0.6f;
        }
    }
    stadiumSpace(tmp, 0.35f, 0.7f);
    for (size_t i = 0; i < out.size(); i++) out[i] += tmp[i] * amp / std::sqrt((float)nw) * 1.4f;
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
static void addBrass(std::vector<float>& v, int i0, int len, float f, float amp);
// fanfare des tribunes : trompettes qui jouent la mélodie du chant avec la foule
static void addBand(std::vector<float>& v, const std::vector<Syll>& song, float base, float amp) {
    for (auto& sy : song) { addBrass(v, (int)(sy.t0 * SR), (int)(sy.len * SR * 0.9f), base * 2.f * std::pow(2.f, sy.semi / 12.f), amp); addBrass(v, (int)(sy.t0 * SR), (int)(sy.len * SR * 0.9f), base * 2.f * std::pow(2.f, (sy.semi - 12) / 12.f), amp * 0.5f); }
}
// chants complets (boucles) : 0 « Allez ! », 1 sifflets, 2 contestation, 3 encouragements rythmés, 4 « Olé ! »
static void chantMarseillaise(std::vector<float>& v);
static std::vector<float> renderChant(int kind) {
    float len = kind == 1 ? 5.f : kind == 5 ? 12.f : kind == 9 ? 9.6f : kind == 10 ? 8.f : kind == 12 ? 26.f : kind == 13 ? 25.f : 6.4f;
    std::vector<float> v((size_t)(len * SR), 0.f);
    const float b = 60.f / 132.f;                    // une noire à 132 bpm
    std::vector<Syll> song;
    switch (kind) {
    case 0: {   // « Al-lez, al-lez, al-lez... » sur une mélodie montante, grosse caisse sur les temps
        static const int MEL[12] = { 0, 4, 7, 7, 5, 4, 2, 4, 5, 4, 2, 0 };
        for (int r = 0; r < 2; r++) for (int k = 0; k < 6; k++) {
            float t0 = r * 6 * b * 2 + k * b * 2;
            song.push_back({ t0, b * 0.8f, MEL[(r * 6 + k) % 12], &V_A, 0.85f });
            song.push_back({ t0 + b, b * 0.95f, MEL[(r * 6 + k) % 12] + 2, &V_E, 1.f, 'l' });
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
            song.push_back({ t0, b * 0.45f, 0, &V_E, 0.9f, 'd' });
            song.push_back({ t0 + b * 0.5f, b * 0.45f, 0, &V_I, 0.9f, 'l' });
            song.push_back({ t0 + b, b * 0.9f, -3, &V_O, 1.f, 's' });
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
            song.push_back({ t0 + b * 3.8f, b * 0.5f, 7, &V_E, 1.f, 'l' });
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
            song.push_back({ t0, L[k] * b, N[k], k % 2 ? &V_E : &V_O, k == 7 ? 1.f : 0.9f, k % 2 ? 'l' : (char)0 });
        }
        addChoir(v, song, 220.f, 30, 1.3f, 41);
        addBand(v, song, 220.f, 0.35f);                                         // la fanfare du virage reprend l'air
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
            song.push_back({ t0 + 0.04f, 0.28f, 0, &V_OU, 1.f, 'h' });
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
            std::vector<Syll> call = { { t0, b * 0.42f, 0, &V_O, 1.f }, { t0 + b * 0.5f, b * 0.42f, 0, &V_A, 1.f, 'l' }, { t0 + b, b * 0.9f, -2, &V_E, 1.f, 'l' } };
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
        addBand(v, song, 165.f, 0.3f);
        for (float t = 0.1f; t < len; t += b) addDrum(v, t, 0.4f);
        addMurmur(v, 0.15f, 0.07f);
        break;
    }
    case 12: {  // « When the Saints Go Marching In » (air traditionnel) repris par tout le stade, avec la fanfare
        struct N { float b; int s; float l; };
        static const N M[] = { {0,0,1},{1,4,1},{2,5,1},{3,7,4}, {7,0,1},{8,4,1},{9,5,1},{10,7,4},
                               {14,0,1},{15,4,1},{16,5,1},{17,7,2},{19,4,2},{21,0,2},{23,4,2},{25,2,4},
                               {29,4,1},{30,4,1},{31,2,1},{32,0,2},{34,0,1},{35,4,1},{36,7,1},{37,7,1},{38,5,3},
                               {41,4,1},{42,5,1},{43,7,1},{44,4,1},{45,0,1},{46,2,1},{47,0,4} };
        const float q = 0.5f;
        const Vowel* VW[4] = { &V_O, &V_E, &V_A, &V_I };
        int k = 0;
        for (auto& n : M) song.push_back({ 0.2f + n.b * q, n.l * q * 0.92f, n.s, VW[(k++) % 4], n.l >= 2 ? 1.f : 0.85f, k % 3 == 0 ? 'd' : 0 });
        addChoir(v, song, 220.f, 32, 1.3f, 81);
        addBand(v, song, 220.f, 0.28f);
        for (float t = 0.2f; t < len; t += q) addDrum(v, t, ((int)((t - 0.2f) / q + 0.5f)) % 2 ? 0.2f : 0.38f);
        addMurmur(v, 0.15f, 0.07f);
        break;
    }
    case 13: chantMarseillaise(v); addMurmur(v, 0.12f, 0.07f); break;   // le public entonne le refrain de La Marseillaise
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
    // compression douce : les chants sonnent plus pleins et plus forts sans saturer
    normalize(v, 1.f, 0.f);
    for (auto& x : v) x = std::tanh(x * 1.9f) / std::tanh(1.9f);
    normalize(v, 0.9f, 0.04f);
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
static int g_prevCat = -1, g_prevIdx = -1;     // test des sons : élément en cours d'écoute
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

// applaudissements en boucle (fin de match, ovations) : densité constante, fin fondue dans le début -> boucle sans couture
static Music g_applause; static bool g_applauseOk = false; static std::vector<unsigned char> g_applauseWav;
static void addApplauseFlat(std::vector<float>& out, float dur, int nClap, float amp) {
    for (int p = 0; p < nClap; p++) {
        float rate = 3.0f + urand() * 3.8f, t = urand() * 0.4f, dist = 0.3f + 0.7f * urand();
        int proto = rand();
        float surge = urand() * 6.2831f;                                         // chaque spectateur a ses moments d'enthousiasme
        while (t < dur) {
            float e = 0.8f + 0.2f * std::sin(t * 0.7f + surge);
            if (urand() > 0.05f) putClap(out, (int)(t * SR), proto + (urand() < 0.3f ? 1 : 0), amp * dist * e * (0.7f + 0.3f * urand()) * 0.22f);
            t += (1.f / rate) * (0.88f + 0.24f * urand());
        }
    }
}
static std::vector<unsigned char> wavBytes(const std::vector<float>& v) {
    std::vector<unsigned char> b(44 + v.size() * 2);
    auto w32 = [&](int o, uint32_t x) { for (int k = 0; k < 4; k++) b[o + k] = (unsigned char)(x >> (8 * k)); };
    auto w16 = [&](int o, uint16_t x) { b[o] = (unsigned char)x; b[o + 1] = (unsigned char)(x >> 8); };
    memcpy(&b[0], "RIFF", 4); w32(4, (uint32_t)(36 + v.size() * 2)); memcpy(&b[8], "WAVEfmt ", 8); w32(16, 16); w16(20, 1); w16(22, 1);
    w32(24, SR); w32(28, SR * 2); w16(32, 2); w16(34, 16); memcpy(&b[36], "data", 4); w32(40, (uint32_t)(v.size() * 2));
    for (size_t i = 0; i < v.size(); i++) { float x = std::max(-1.f, std::min(1.f, v[i])); int16_t q = (int16_t)(x * 32000); w16(44 + (int)i * 2, (uint16_t)q); }
    return b;
}
void audioApplause(float vol) {
    if (!g_ok || !g_applauseOk) return;
    if (!g_enabled || vol <= 0.01f) { if (IsMusicStreamPlaying(g_applause)) StopMusicStream(g_applause); return; }
    if (!IsMusicStreamPlaying(g_applause)) PlayMusicStream(g_applause);
    SetMusicVolume(g_applause, std::min(1.f, vol));
    UpdateMusicStream(g_applause);
}
void audioInit() {
    InitAudioDevice();
    if (!IsAudioDeviceReady()) return;
    srand(42);
    const float PI2 = 6.2831853f;
    // bruitages synthétisés (build 22) : filtres simples sur bruit blanc et oscillateurs à hauteur glissante
    struct Lp { float y = 0, a; Lp(float fc) { a = 1 - std::exp(-6.2831853f * fc / SR); } float f(float x) { y += a * (x - y); return y; } };
    struct Bp { Lp l1, l2; Bp(float lo, float hi) : l1(hi), l2(lo) {} float f(float x) { float h = l1.f(x); return h - l2.f(h); } };
    // impact sur le ballon : coup sourd (hauteur qui chute) + claquement du cuir
    auto kickSound = [&](float len, float f0, float f1, float drop, float thump, float crack, float crackHi, float decay) {
        int n = (int)(SR * len); std::vector<float> v(n); float ph = 0; Bp bp(900, crackHi);
        for (int i = 0; i < n; i++) {
            float t = (float)i / SR; float f = f1 + (f0 - f1) * std::exp(-t * drop); ph += PI2 * f / SR;
            float body = std::sin(ph) * std::exp(-t * decay) * thump;
            float cr = bp.f(frand()) * std::exp(-t * 140) * crack;
            v[i] = body + cr;
        }
        normalize(v, 0.9f, 0.f); return v;
    };
    g_sfx[SFX_KICK] = makeSound(kickSound(0.10f, 170, 70, 55, 0.9f, 0.9f, 3500, 38));
    g_sfx[SFX_SHOT] = makeSound(kickSound(0.18f, 260, 55, 38, 1.0f, 1.8f, 6000, 22));
    g_sfx[SFX_BOUNCE] = makeSound(kickSound(0.08f, 130, 60, 70, 0.8f, 0.25f, 2000, 55));
    // tacle : frottement sur la pelouse + choc
    {
        int n = (int)(SR * 0.22f); std::vector<float> v(n); Bp bp(400, 2500); float ph = 0;
        for (int i = 0; i < n; i++) {
            float t = (float)i / SR; ph += PI2 * (90 + 60 * std::exp(-t * 40)) / SR;
            float scuff = bp.f(frand()) * std::min(1.f, t * 60) * std::exp(-t * 14) * (0.7f + 0.3f * std::sin(t * 190));
            v[i] = scuff * 1.2f + std::sin(ph) * std::exp(-t * 35) * 0.8f;
        }
        normalize(v, 0.8f, 0.f); g_sfx[SFX_TACKLE] = makeSound(v);
    }
    // gardien : prise de balle (claquement des gants) et parade (choc sec)
    for (int k = 0; k < 2; k++) {
        int n = (int)(SR * (k ? 0.16f : 0.12f)); std::vector<float> v(n); Bp bp(k ? 600 : 1200, k ? 3000 : 5000); float ph = 0;
        for (int i = 0; i < n; i++) {
            float t = (float)i / SR; ph += PI2 * (k ? 140 : 110) / SR;
            v[i] = bp.f(frand()) * std::exp(-t * (k ? 60 : 90)) * 1.4f + std::sin(ph) * std::exp(-t * 45) * (k ? 0.9f : 0.5f);
        }
        normalize(v, k ? 0.85f : 0.7f, 0.f); g_sfx[k ? SFX_PARRY : SFX_CATCH] = makeSound(v);
    }
    // filet : froissement qui gonfle puis retombe
    {
        int n = (int)(SR * 0.6f); std::vector<float> v(n); Bp bp(700, 5500); Lp rust(18);
        for (int i = 0; i < n; i++) {
            float t = (float)i / SR;
            float env = std::min(1.f, t * 35) * std::exp(-t * 6.5f);
            float r = 0.55f + 0.45f * std::fabs(rust.f(frand()) * 6);
            v[i] = bp.f(frand()) * env * r;
        }
        normalize(v, 0.75f, 0.f); g_sfx[SFX_NET] = makeSound(v);
    }
    // poteau : son métallique (partiels inharmoniques) + impact
    {
        int n = (int)(SR * 1.1f); std::vector<float> v(n);
        const float fr[5] = { 523, 1187, 1951, 2733, 3610 }, am[5] = { 1.f, 0.7f, 0.45f, 0.3f, 0.18f }, dc[5] = { 3.2f, 4.5f, 6.f, 8.f, 11.f };
        Bp bp(1500, 7000);
        for (int i = 0; i < n; i++) {
            float t = (float)i / SR; float x = 0;
            for (int q = 0; q < 5; q++) x += std::sin(PI2 * fr[q] * t * (1 + 0.002f * std::sin(t * 30))) * am[q] * std::exp(-t * dc[q]);
            v[i] = x * 0.4f + bp.f(frand()) * std::exp(-t * 120) * 1.2f + std::sin(PI2 * 95 * t) * std::exp(-t * 40) * 0.6f;
        }
        normalize(v, 0.8f, 0.f); g_sfx[SFX_POST] = makeSound(v);
    }
    // sifflet à bille de l'arbitre : trille (modulation de fréquence par la bille), souffle, harmonique
    auto whistleSeg = [&](std::vector<float>& v, float a, float len, float amp, unsigned seed) {
        srand(seed); Bp breath(2000, 6000); float ph = 0, ph2 = 0;
        float trill = 26 + 6 * std::fabs(frand()), base = 2900 + 120 * frand();
        for (int i = (int)(a * SR); i < (int)((a + len) * SR) && i < (int)v.size(); i++) {
            float t = (float)i / SR - a;
            float att = std::min(1.f, t * 45) * std::min(1.f, (len - t) * 18);
            float bend = 1 - 0.035f * std::exp(-t * 18);                              // la note monte quand l'arbitre souffle
            float fm = std::sin(PI2 * trill * t) * 110 + std::sin(PI2 * trill * 2.03f * t) * 30;
            float f = base * bend + fm;
            ph += PI2 * f / SR; ph2 += PI2 * f * 2 / SR;
            float flut = 0.72f + 0.28f * std::sin(PI2 * trill * t + 1.3f);
            v[i] += (std::sin(ph) * 0.62f + std::sin(ph2) * 0.12f + breath.f(frand()) * 0.22f) * flut * att * amp;
        }
    };
    {
        std::vector<float> v((size_t)(SR * 0.38f), 0.f); whistleSeg(v, 0, 0.36f, 0.6f, 7); g_sfx[SFX_WHISTLE] = makeSound(v);
        std::vector<float> c((size_t)(SR * 0.62f), 0.f); whistleSeg(c, 0, 0.18f, 0.6f, 8); whistleSeg(c, 0.24f, 0.36f, 0.62f, 9); g_sfx[SFX_CARD] = makeSound(c);
        std::vector<float> l((size_t)(SR * 1.15f), 0.f); whistleSeg(l, 0, 1.12f, 0.6f, 10); g_sfx[SFX_WHISTLE_LONG] = makeSound(l);
        // coup de sifflet final : deux coups brefs puis un long
        std::vector<float> f((size_t)(SR * 2.3f), 0.f);
        whistleSeg(f, 0.f, 0.32f, 0.62f, 11); whistleSeg(f, 0.47f, 0.32f, 0.62f, 12); whistleSeg(f, 0.95f, 1.2f, 0.66f, 13);
        g_sfx[SFX_WHISTLE_FINAL] = makeSound(f);
    }
    // interface : déplacement dans un menu, validation, retour, réglage
    auto blip = [&](std::initializer_list<std::pair<float, float>> notes, float noteLen, float amp, float duty) {
        int n = (int)(SR * noteLen * notes.size() + SR * 0.03f); std::vector<float> v(n, 0.f); int k = 0; Lp lp(5000);
        for (auto nt : notes) {
            float ph = 0;
            for (int i = 0; i < (int)(SR * (noteLen + 0.03f)); i++) {
                int j = (int)(k * noteLen * SR) + i; if (j >= n) break;
                float t = (float)i / SR; float f = nt.first + (nt.second - nt.first) * std::min(1.f, t / noteLen);
                ph += f / SR; ph -= std::floor(ph);
                float sq = ph < duty ? 1.f : -1.f, tri = 4 * std::fabs(ph - 0.5f) - 1;
                v[j] += (sq * 0.35f + tri * 0.65f) * std::min(1.f, t * 400) * std::exp(-t * (4.5f / noteLen)) * amp;
            }
            k++;
        }
        for (auto& x : v) x = lp.f(x);
        return v;
    };
    g_sfx[SFX_UI_MOVE] = makeSound(blip({ { 1180, 1320 } }, 0.045f, 0.32f, 0.25f));
    g_sfx[SFX_UI_TICK] = makeSound(blip({ { 2000, 1800 } }, 0.02f, 0.25f, 0.5f));
    g_sfx[SFX_UI_OK] = makeSound(blip({ { 784, 784 }, { 1175, 1175 }, { 1568, 1568 } }, 0.05f, 0.4f, 0.25f));
    g_sfx[SFX_UI_BACK] = makeSound(blip({ { 880, 760 }, { 587, 520 } }, 0.06f, 0.36f, 0.25f));
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
        std::vector<float> c((size_t)(2.6f * SR), 0.f);
        srand(59);
        addApplause(c, 0.f, 2.5f, 220, 1.f);
        addMurmur(c, 0.08f, 0.08f);
        stadiumSpace(c, 0.3f, 0.75f);
        normalize(c, 0.7f, 0.05f);
        g_sfx[SFX_CLAPS] = makeSound(c);
        {   // boucle de 6 s : on rend 7 s et la dernière seconde est fondue dans la première
            const float L = 6.f, F = 1.f;
            std::vector<float> a((size_t)((L + F) * SR), 0.f);
            srand(61);
            addApplauseFlat(a, L + F, 240, 1.f);
            addMurmur(a, 0.06f, 0.08f);
            stadiumSpace(a, 0.3f, 0.75f);
            int nl = (int)(L * SR), nf = (int)(F * SR);
            std::vector<float> loop(a.begin(), a.begin() + nl);
            for (int i = 0; i < nf; i++) { float u = (float)i / nf; loop[i] = a[i] * std::sqrt(u) + a[nl + i] * std::sqrt(1.f - u); }
            normalize(loop, 0.7f, 0.f);
            g_applauseWav = wavBytes(loop);
            g_applause = LoadMusicStreamFromMemory(".wav", g_applauseWav.data(), (int)g_applauseWav.size());
            g_applause.looping = true;
            g_applauseOk = g_applause.frameCount > 0;
        }
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
    if (g_applauseOk) { UnloadMusicStream(g_applause); g_applauseOk = false; }
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
    if (g_prevCat >= 1 && g_prevCat <= 4) return;          // test des sons : on n'interrompt pas l'écoute
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
// hymnes réels : début de chaque hymne (temps en noires, notes MIDI), mélodie + basse + voix intérieures quand la partition les donne
struct RNote { float beat; int midi; float len; };
struct RChord { float beat; int p[2]; float len; };
struct RealAnthem { const char* codes; const char* title; const char* composer; const char* license; const char* source; float bpm; int tonic, minor; float lengthBeats, beatsPerBar;
                    const RNote* mel; int nm; const RNote* bass; int nb; const RChord* inner; int ni; };
#include "anthems_data.inc"
struct RChord4 { float beat; int p[4]; float len; };
#include "marseillaise_data.inc"
#define ANL(a) a, (int)(sizeof(a) / sizeof(a[0]))
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

// orchestre synthétique pour les partitions réelles : cuivres (mélodie), cordes (voix intérieures), basse, timbales,
// cymbale finale, réverbération de stade. Le début de l'hymne est joué deux fois (comme la reprise de la partition).
static float midiHz(float m) { return 440.f * std::pow(2.f, (m - 69.f) / 12.f); }
// timbres sans repliement (aliasing) : synthèse additive limitée sous la fréquence de Nyquist, harmoniques calculés par récurrence
// (sin(n x) = 2 cos x sin((n-1) x) - sin((n-2) x)), ce qui garde des notes justes et douces même dans l'aigu
static inline float bandSum(float x, int H, float bright, float tilt) {
    float s1 = std::sin(x), c2 = 2.f * std::cos(x), sPrev = 0.f, sCur = s1, acc = 0.f, a = 1.f, norm = 0.f;
    for (int n = 1; n <= H; n++) {
        float w = a / std::pow((float)n, tilt);
        acc += sCur * w; norm += w;
        float sNext = c2 * sCur - sPrev; sPrev = sCur; sCur = sNext;
        a *= bright;
    }
    return acc / std::max(0.001f, norm);
}
static void addBrass(std::vector<float>& v, int i0, int len, float f, float amp) {
    const float PI2 = 6.2831853f;
    float dur = (float)len / SR;
    int H = std::max(1, std::min(14, (int)(SR * 0.45f / f)));
    float ph = 0.f;
    for (int k = 0; k < len + SR / 6 && i0 + k < (int)v.size(); k++) {
        if (i0 + k < 0) continue;
        float t = (float)k / SR;
        float vib = t > 0.3f ? 1.f + 0.004f * std::sin(PI2 * 5.0f * t) * std::min(1.f, (t - 0.3f) * 3.f) : 1.f;
        ph += f * vib / SR; if (ph > 1.f) ph -= 1.f;
        float att = std::min(1.f, t * 28.f);
        float env = att * (0.8f + 0.2f * std::exp(-t * 5.f)) * (t < dur ? std::min(1.f, (dur - t) * 18.f + 0.35f) : std::max(0.f, 0.35f - (t - dur) * 2.2f));
        float bright = 0.62f + 0.22f * std::exp(-t * 6.f) + 0.08f * env;      // le cuivre s'éclaircit à l'attaque
        float x = bandSum(PI2 * ph, H, bright, 0.7f);
        v[i0 + k] += x * 0.11f * env * amp;
    }
}
static void addString(std::vector<float>& v, int i0, int len, float f, float amp) {
    const float PI2 = 6.2831853f;
    int H = std::max(1, std::min(10, (int)(SR * 0.42f / f)));
    float p1 = 0.f, p2 = 0.33f, p3 = 0.71f;
    for (int k = 0; k < len + SR / 5 && i0 + k < (int)v.size(); k++) {
        if (i0 + k < 0) continue;
        float t = (float)k / SR, dur = (float)len / SR;
        float vib = 1.f + 0.0035f * std::sin(PI2 * 5.6f * t + 1.3f);
        p1 += f * vib / SR; p2 += f * 1.004f * vib / SR; p3 += f * 0.9965f * vib / SR;
        p1 -= std::floor(p1); p2 -= std::floor(p2); p3 -= std::floor(p3);
        float env = std::min(1.f, t * 7.f) * (t < dur ? 1.f : std::max(0.f, 1.f - (t - dur) * 5.f));
        float x = bandSum(PI2 * p1, H, 0.8f, 1.f) + bandSum(PI2 * p2, H, 0.8f, 1.f) + bandSum(PI2 * p3, H, 0.8f, 1.f);
        v[i0 + k] += x * 0.022f * env * amp;
    }
}
static void addBass(std::vector<float>& v, int i0, int len, float f, float amp) {
    const float PI2 = 6.2831853f;
    for (int k = 0; k < len + SR / 10 && i0 + k < (int)v.size(); k++) {
        float t = (float)k / SR, dur = (float)len / SR;
        float env = std::min(1.f, t * 30.f) * (0.65f + 0.35f * std::exp(-t * 3.f)) * (t < dur ? 1.f : std::max(0.f, 1.f - (t - dur) * 10.f));
        v[i0 + k] += (std::sin(PI2 * f * t) + 0.35f * std::sin(PI2 * f * 2 * t) + 0.12f * std::sin(PI2 * f * 3 * t)) * 0.075f * env * amp;
    }
}
static std::vector<float> renderMarseillaise();
// God Save the King en entier (14 mesures) : « God save our gracious King... » puis « Send him victorious... »
static const RNote GSTK_FULL[] = { { 0.f, 67, 1.f }, { 1.f, 67, 1.f }, { 2.f, 69, 1.f }, { 3.f, 66, 1.5f }, { 4.5f, 67, 0.5f }, { 5.f, 69, 1.f },
    { 6.f, 71, 1.f }, { 7.f, 71, 1.f }, { 8.f, 72, 1.f }, { 9.f, 71, 1.5f }, { 10.5f, 69, 0.5f }, { 11.f, 67, 1.f },
    { 12.f, 69, 1.f }, { 13.f, 67, 1.f }, { 14.f, 66, 1.f }, { 15.f, 67, 3.f },
    { 18.f, 74, 1.f }, { 19.f, 74, 1.f }, { 20.f, 74, 1.f }, { 21.f, 74, 1.5f }, { 22.5f, 72, 0.5f }, { 23.f, 71, 1.f },
    { 24.f, 72, 1.f }, { 25.f, 72, 1.f }, { 26.f, 72, 1.f }, { 27.f, 72, 1.5f }, { 28.5f, 71, 0.5f }, { 29.f, 69, 1.f },
    { 30.f, 71, 1.f }, { 31.f, 72, 0.5f }, { 31.5f, 71, 0.5f }, { 32.f, 69, 0.5f }, { 32.5f, 67, 0.5f },
    { 33.f, 71, 1.5f }, { 34.5f, 72, 0.5f }, { 35.f, 74, 1.f },
    { 36.f, 76, 0.5f }, { 36.5f, 72, 0.5f }, { 37.f, 71, 1.f }, { 38.f, 69, 1.f }, { 39.f, 67, 3.f } };
static std::vector<float> renderReal(const RealAnthem& A0, int reps = 2) {
    if (!strcmp(A0.codes, "FRA")) return renderMarseillaise();
    RealAnthem A = A0;
    if (strstr(A0.codes, "ENG")) { A.mel = GSTK_FULL; A.nm = (int)(sizeof GSTK_FULL / sizeof GSTK_FULL[0]); A.lengthBeats = 42.f; A.bpm = 62.f; A.bass = nullptr; A.nb = 0; A.inner = nullptr; A.ni = 0; reps = 1; }
    const float PI2 = 6.2831853f;
    float bpm = A.bpm > 0 ? A.bpm : 84.f;
    if (A.lengthBeats * reps * 60.f / bpm < 14.f) reps = 3;                  // extrait très court : une reprise de plus
    float spb = 60.f / bpm;
    float tail = 2.5f;
    float totalBeats = A.lengthBeats * reps + tail;
    int n = (int)(totalBeats * spb * SR) + SR;
    std::vector<float> v(n, 0.f);
    auto at = [&](float beat) { return (int)(beat * spb * SR); };
    // accord implicite d'un temps (sans basse ni voix intérieures dans la partition) : I, IV ou V selon la note de mélodie
    auto implied = [&](float beat) {
        int best = -1;
        for (int i = 0; i < A.nm; i++) if (A.mel[i].beat <= beat + 0.01f && A.mel[i].beat + A.mel[i].len > beat + 0.01f) best = i;
        if (best < 0) return 0;
        int pc = ((A.mel[best].midi - A.tonic) % 12 + 12) % 12;
        return (pc == 0 || pc == (A.minor ? 3 : 4) || pc == 7) ? 0 : (pc == 5 || pc == 9 || pc == 8 || pc == 2 && false) ? 5 : 7;
    };
    for (int r = 0; r < reps; r++) {
        float off = r * A.lengthBeats;
        bool last = r == reps - 1;
        float amp = r == 0 ? 0.9f : 1.f;
        for (int i = 0; i < A.nm; i++) {
            float len = A.mel[i].len;
            if (last && i == A.nm - 1) len += tail;                          // dernière note tenue
            addBrass(v, at(off + A.mel[i].beat), at(len), midiHz((float)A.mel[i].midi), amp);
        }
        if (A.nb > 0) {
            for (int i = 0; i < A.nb; i++) { float len = A.bass[i].len; if (last && i == A.nb - 1) len += tail; addBass(v, at(off + A.bass[i].beat), at(len), midiHz((float)A.bass[i].midi), amp); }
        }
        if (A.ni > 0) {
            for (int i = 0; i < A.ni; i++) { float len = A.inner[i].len; if (last && i == A.ni - 1) len += tail;
                for (int q = 0; q < 2; q++) if (A.inner[i].p[q] > 0) addString(v, at(off + A.inner[i].beat), at(len), midiHz((float)A.inner[i].p[q]), amp); }
        }
        if (A.nb == 0 || A.ni == 0) {
            // harmonie déduite de la mélodie, temps par temps (cordes et/ou basse manquantes)
            int beats = (int)std::ceil(A.lengthBeats);
            for (int bt = 0; bt < beats; bt++) {
                int root = implied((float)bt);
                int third = A.minor && root != 7 ? 3 : 4;
                float blen = (last && bt == beats - 1) ? 1.f + tail : 1.f;
                int base = 48 + A.tonic + root; if (base > 55) base -= 12;
                if (A.ni == 0) { addString(v, at(off + bt), at(blen), midiHz((float)base + 12), amp * 0.8f); addString(v, at(off + bt), at(blen), midiHz((float)base + 12 + third), amp * 0.8f); addString(v, at(off + bt), at(blen), midiHz((float)base + 19), amp * 0.8f); }
                if (A.nb == 0) addBass(v, at(off + bt), at(blen), midiHz((float)base - 12), amp);
            }
        }
        // timbales sur les temps forts
        for (float b = 0; b < A.lengthBeats; b += 1.f) {
            float rel = std::fmod(b + A.beatsPerBar * 8 - std::fmod(A.mel[0].beat, 1.f), A.beatsPerBar);
            if (rel > 0.01f) continue;
            int i0 = at(off + b);
            float fb = midiHz((float)(36 + A.tonic));
            for (int k = 0; k < SR / 2 && i0 + k < n; k++) { float t = (float)k / SR; v[i0 + k] += std::sin(PI2 * (fb + 25 * std::exp(-t * 20)) * t) * std::exp(-t * 6) * 0.16f; }
        }
    }
    {   // cymbale et roulement final
        int last = at(A.lengthBeats * reps - A.mel[A.nm - 1].len);
        for (int k = 0; k < SR * 2 && last + k < n; k++) { float t = (float)k / SR; v[last + k] += frand() * std::exp(-t * 1.6f) * 0.08f + std::sin(PI2 * 65 * t) * frand() * 0.05f * std::exp(-t * 2.f); }
    }
    {   // réverbération de stade
        static const int D[4] = { 1557, 1617, 1491, 1422 };
        std::vector<float> wet(n, 0.f);
        for (int c = 0; c < 4; c++) { std::vector<float> buf(D[c], 0.f); int bi = 0; for (int k = 0; k < n; k++) { float o = buf[bi]; buf[bi] = v[k] + o * 0.78f; bi = (bi + 1) % D[c]; wet[k] += o * 0.25f; } }
        for (int k = 0; k < n; k++) v[k] = v[k] * 0.8f + wet[k] * 0.35f;
    }
    std::vector<float> out(SR + n, 0.f);
    for (int k = 0; k < SR; k++) { float t = (float)k / SR; out[k] = frand() * (0.5f + 0.5f * std::sin(PI2 * 30 * t)) * 0.07f * std::min(1.f, t * 3); }
    for (int k = 0; k < n; k++) out[SR + k] += v[k];
    normalize(out, 0.85f, 0.01f);
    return out;
}
// La Marseillaise complète (premier couplet + refrain « Aux armes, citoyens ! ») : partition piano et chant CC0
// convertie par tools/ly_to_notes.py. Orchestre de fanfare : trompettes au chant, cors et cordes sur les accords
// de la main droite, basse et tuba sur la main gauche (les trémolos deviennent un roulement de timbales).
static std::vector<float> renderMarseillaise() {
    const float PI2 = 6.2831853f;
    float bpm = 104.f, spb = 60.f / bpm, tail = 3.f;
    float total = MARS_LENGTH + tail;
    int n = (int)(total * spb * SR) + SR;
    std::vector<float> v(n, 0.f);
    auto at = [&](float b) { return (int)(b * spb * SR); };
    int nm = (int)(sizeof MARS_MEL / sizeof MARS_MEL[0]), nb = (int)(sizeof MARS_BASS / sizeof MARS_BASS[0]), nr = (int)(sizeof MARS_RH / sizeof MARS_RH[0]);
    const float REFRAIN = 76.f;           // « Aux armes, citoyens ! » (mesure 20)
    for (int i = 0; i < nm; i++) {
        float len = MARS_MEL[i].len + (i == nm - 1 ? tail : 0.f);
        float amp = MARS_MEL[i].beat >= REFRAIN ? 1.1f : 0.95f;
        addBrass(v, at(MARS_MEL[i].beat), at(len), midiHz((float)MARS_MEL[i].midi), amp);
        addBrass(v, at(MARS_MEL[i].beat), at(len), midiHz((float)MARS_MEL[i].midi - 12), amp * 0.35f);   // doublure à l'octave grave
    }
    for (int i = 0; i < nr; i++) {
        float len = MARS_RH[i].len + (i == nr - 1 ? tail : 0.f);
        int top = 0; for (int q = 0; q < 4; q++) top = std::max(top, MARS_RH[i].p[q]);
        for (int q = 0; q < 4; q++) {
            int m = MARS_RH[i].p[q];
            if (m <= 0 || m == top) continue;                 // la note du dessus double déjà le chant
            addString(v, at(MARS_RH[i].beat), at(len), midiHz((float)m), 1.1f);
        }
    }
    for (int i = 0; i < nb; i++) {
        float len = MARS_BASS[i].len, b = MARS_BASS[i].beat;
        int i0 = at(b);
        if (len < 0.2f) {                                     // trémolo : roulement de timbales
            for (int k = 0; k < at(len) + SR / 20 && i0 + k < n; k++) { float t = (float)k / SR; v[i0 + k] += std::sin(PI2 * (midiHz((float)MARS_BASS[i].midi) + 20 * std::exp(-t * 30)) * t) * std::exp(-t * 18) * 0.09f; }
            continue;
        }
        if (i == nb - 1) len += tail;
        addBass(v, i0, at(len), midiHz((float)MARS_BASS[i].midi), 1.f);
    }
    // timbales sur les temps forts, caisse claire de marche (temps 2 et 4), cymbales au refrain et à la fin
    auto snare = [&](float b, float amp) { int i0 = at(b); for (int k = 0; k < SR / 6 && i0 + k < n; k++) { float t = (float)k / SR; v[i0 + k] += (frand() * 0.8f + std::sin(PI2 * 190 * t) * 0.4f) * std::exp(-t * 22) * amp; } };
    auto cymbal = [&](float b, float amp) { int i0 = at(b); for (int k = 0; k < SR * 2 && i0 + k < n; k++) { float t = (float)k / SR; v[i0 + k] += frand() * std::exp(-t * 1.8f) * amp; } };
    for (float b = 4; b < MARS_LENGTH; b += 1.f) {
        int beatInBar = ((int)b) % 4;
        if (beatInBar == 0) { int i0 = at(b); float fb = midiHz(34.f + (b >= REFRAIN ? 0 : 0)); for (int k = 0; k < SR / 2 && i0 + k < n; k++) { float t = (float)k / SR; v[i0 + k] += std::sin(PI2 * (fb + 25 * std::exp(-t * 20)) * t) * std::exp(-t * 6) * 0.13f; } }
        if (beatInBar == 1 || beatInBar == 3) snare(b, b >= REFRAIN ? 0.05f : 0.035f);
        if (beatInBar == 3 && b >= REFRAIN) snare(b + 0.5f, 0.03f);
    }
    cymbal(REFRAIN, 0.1f);
    cymbal(MARS_LENGTH - 4.f, 0.12f);
    {   // réverbération de stade
        static const int D[4] = { 1557, 1617, 1491, 1422 };
        std::vector<float> wet(n, 0.f);
        for (int c = 0; c < 4; c++) { std::vector<float> buf(D[c], 0.f); int bi = 0; for (int k = 0; k < n; k++) { float o = buf[bi]; buf[bi] = v[k] + o * 0.78f; bi = (bi + 1) % D[c]; wet[k] += o * 0.25f; } }
        for (int k = 0; k < n; k++) v[k] = v[k] * 0.8f + wet[k] * 0.3f;
    }
    std::vector<float> out(SR + n, 0.f);
    for (int k = 0; k < SR; k++) { float t = (float)k / SR; out[k] = frand() * (0.5f + 0.5f * std::sin(PI2 * 30 * t)) * 0.07f * std::min(1.f, t * 3); }
    for (int k = 0; k < n; k++) out[SR + k] += v[k];
    normalize(out, 0.85f, 0.01f);
    return out;
}
// fanfare d'entrée des équipes (composition originale du jeu, cérémonie des matchs de sélections : tunnel)
static std::vector<float> renderEntrance() {
    const float PI2 = 6.2831853f;
    float bpm = 92.f, spb = 60.f / bpm;
    struct N { float b; int m; float l; };
    static const N MEL[] = { {0,62,1},{1,66,1},{2,69,2}, {4,71,1.5f},{5.5f,69,.5f},{6,66,2}, {8,67,1},{9,71,1},{10,74,1.5f},{11.5f,73,.5f}, {12,71,2},{14,69,2},
        {16,74,1},{17,73,1},{18,71,1},{19,69,1}, {20,67,1.5f},{21.5f,66,.5f},{22,64,2}, {24,66,1},{25,69,1},{26,74,1},{27,78,1}, {28,76,4},
        {32,78,1.5f},{33.5f,76,.5f},{34,74,2}, {36,71,1},{37,74,1},{38,69,2}, {40,67,1},{41,71,1},{42,76,1.5f},{43.5f,74,.5f}, {44,73,2},{46,69,2},
        {48,74,1},{49,76,1},{50,78,1},{51,79,1}, {52,81,1.5f},{53.5f,78,.5f},{54,74,2}, {56,71,1},{57,79,1},{58,78,1},{59,76,1}, {60,74,4} };
    // accords par demi-mesure : fondamentale (MIDI de basse) et mode (0 majeur, 1 mineur)
    static const int ROOT[32] = { 38,38, 43,38, 43,43, 38,45, 38,45, 40,45, 38,38, 45,45, 38,38, 43,38, 40,40, 45,45, 38,38, 38,38, 43,45, 38,38 };
    static const int MIN[32] = { 0,0, 0,0, 0,0, 0,0, 0,0, 1,0, 0,0, 0,0, 0,0, 0,0, 1,1, 0,0, 0,0, 0,0, 0,0, 0,0 };
    float total = 64.f + 4.f;
    int n = (int)(total * spb * SR);
    std::vector<float> v(n, 0.f);
    auto at = [&](float b) { return (int)(b * spb * SR); };
    for (auto& x : MEL) { addBrass(v, at(x.b), at(x.l), midiHz((float)x.m), 1.f); addBrass(v, at(x.b), at(x.l), midiHz((float)x.m - 12), 0.4f); }
    for (int h = 0; h < 32; h++) {
        int r = ROOT[h];
        addBass(v, at(h * 2.f), at(2.f), midiHz((float)r), 1.f);
        int third = MIN[h] ? 3 : 4;
        for (int q : { 12, 12 + third, 19 }) addString(v, at(h * 2.f), at(2.f), midiHz((float)(r + q + 12)), 0.9f);
    }
    for (int bar = 0; bar < 16; bar++) {      // timbales et caisse claire de marche
        int i0 = at(bar * 4.f);
        for (int k = 0; k < SR / 2 && i0 + k < n; k++) { float t = (float)k / SR; v[i0 + k] += std::sin(PI2 * (55.f + 25 * std::exp(-t * 20)) * t) * std::exp(-t * 6) * 0.13f; }
        for (int bt : { 1, 3 }) { int j0 = at(bar * 4.f + bt); for (int k = 0; k < SR / 6 && j0 + k < n; k++) { float t = (float)k / SR; v[j0 + k] += frand() * std::exp(-t * 24) * 0.035f; } }
    }
    for (float cb : { 48.f, 60.f }) { int i0 = at(cb); for (int k = 0; k < SR * 2 && i0 + k < n; k++) { float t = (float)k / SR; v[i0 + k] += frand() * std::exp(-t * 1.8f) * 0.09f; } }
    {   // réverbération de stade
        static const int D[4] = { 1557, 1617, 1491, 1422 };
        std::vector<float> wet(n, 0.f);
        for (int c = 0; c < 4; c++) { std::vector<float> buf(D[c], 0.f); int bi = 0; for (int k = 0; k < n; k++) { float o = buf[bi]; buf[bi] = v[k] + o * 0.78f; bi = (bi + 1) % D[c]; wet[k] += o * 0.25f; } }
        for (int k = 0; k < n; k++) v[k] = v[k] * 0.8f + wet[k] * 0.3f;
    }
    normalize(v, 0.85f, 0.02f);
    return v;
}
// le public chante « Aux armes, citoyens ! » (refrain de La Marseillaise, d'après la partition)
static void chantMarseillaise(std::vector<float>& v) {
    const float spb = 60.f / 100.f, start = 75.75f;
    const Vowel* VW[5] = { &V_O, &V_A, &V_E, &V_I, &V_OU };
    std::vector<Syll> song;
    int nm = (int)(sizeof MARS_MEL / sizeof MARS_MEL[0]), k = 0;
    for (int i = 0; i < nm; i++) {
        if (MARS_MEL[i].beat < start) continue;
        float t0 = 0.2f + (MARS_MEL[i].beat - start) * spb;
        if (t0 > (float)v.size() / SR - 1.f) break;
        song.push_back({ t0, MARS_MEL[i].len * spb * 0.95f, MARS_MEL[i].midi - 70, VW[(k++) % 5], MARS_MEL[i].len >= 2 ? 1.f : 0.9f, k % 4 == 0 ? 's' : 0 });
    }
    addChoir(v, song, midiHz(70.f), 34, 1.35f, 91);
}
static Sound g_entrance; static bool g_entranceOk = false;
void audioEntranceMusic(bool on) {
    if (!g_ok) return;
    if (on && g_enabled) {
        if (!g_entranceOk) { g_entrance = makeSound(renderEntrance()); g_entranceOk = true; }
        if (!IsSoundPlaying(g_entrance)) { SetSoundVolume(g_entrance, 0.7f); PlaySound(g_entrance); }
    } else if (g_entranceOk && IsSoundPlaying(g_entrance)) StopSound(g_entrance);
}
static int realAnthemIndex(const std::string& code) {
    for (int i = 0; i < NUM_REAL_ANTHEMS; i++) {
        std::string cs = REAL_ANTHEMS[i].codes; size_t p = 0;
        while (p < cs.size()) { size_t e = cs.find(' ', p); if (e == std::string::npos) e = cs.size(); if (cs.substr(p, e - p) == code) return i; p = e + 1; }
    }
    return -1;
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
const char* audioAnthemName(const std::string& code) { int i = realAnthemIndex(code); return i >= 0 ? REAL_ANTHEMS[i].title : GENERIC_ANTHEMS[genericAnthem(code)].name; }
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
    int idx = realAnthemIndex(code);
    if (idx < 0) idx = -1 - genericAnthem(code);       // -1 ... -6 : hymnes génériques
    audioAnthemPlayIndex(idx);
}
// hymne par index : >= 0 hymne réel, -1 ... -N hymne générique (aussi utilisé par le test des sons)
void audioAnthemPlayIndex(int idx) {
    if (!g_ok) return;
    audioStopAnthem();
    if (idx >= NUM_REAL_ANTHEMS || idx < -NUM_GENERIC) return;
    auto it = g_anthemSnd.find(idx);
    if (it == g_anthemSnd.end()) {
        std::vector<float> w = idx >= 0 ? renderReal(REAL_ANTHEMS[idx]) : renderAnthem(GENERIC_ANTHEMS[-1 - idx].n, GENERIC_ANTHEMS[-1 - idx].cnt, GENERIC_ANTHEMS[-1 - idx].bpm, GENERIC_ANTHEMS[-1 - idx].base, GENERIC_ANTHEMS[-1 - idx].minor);
        g_anthemSnd[idx] = makeSound(w);
        it = g_anthemSnd.find(idx);
        g_anthemLen = (float)w.size() / SR;
    } else g_anthemLen = (float)it->second.frameCount / SR;
    g_anthemCur = it->second; g_anthemOn = true;
    SetSoundVolume(g_anthemCur, 0.7f);
    PlaySound(g_anthemCur);
}
int audioAnthemListCount() { return NUM_REAL_ANTHEMS + NUM_GENERIC; }
// liste pour le test des sons : hymnes réels puis génériques ; renvoie l'index à passer à audioAnthemPlayIndex
int audioAnthemListIndex(int i) { return i < NUM_REAL_ANTHEMS ? i : -1 - (i - NUM_REAL_ANTHEMS); }
std::string audioAnthemListName(int i) {
    if (i < NUM_REAL_ANTHEMS) { const RealAnthem& A = REAL_ANTHEMS[i]; std::string c = A.codes; return std::string(A.title) + (c == "FRA" ? " (couplet et refrain)" : "") + "  (" + c + ")  - " + A.composer; }
    int g = i - NUM_REAL_ANTHEMS; return g < NUM_GENERIC ? GENERIC_ANTHEMS[g].name : "";
}
std::string audioAnthemListCredit(int i) {
    if (i < NUM_REAL_ANTHEMS && !strcmp(REAL_ANTHEMS[i].codes, "FRA")) return "Partition complète (piano et chant) : CC0 - github.com/jeandeaual/lilypond-piano-la-marseillaise";
    if (i < NUM_REAL_ANTHEMS) return std::string("Partition : ") + REAL_ANTHEMS[i].license + " - " + REAL_ANTHEMS[i].source; return "Composition originale du jeu"; }
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

static void jinglePlay(int j);
void audioJingle(int j) {
    if (!g_ok || !g_musicOn || j < 0 || j > 8) return;
    jinglePlay(j);
}
// pages de publicité : la musique du spot passe toujours, même musique coupée dans les options
void audioJingleForce(int j) {
    if (!g_ok || !g_enabled || j < 0 || j > 8) return;
    jinglePlay(j);
}
static void jinglePlay(int j) {
    // une nouvelle musique coupe la précédente (menu, autre jingle, musique d'entrée)
    if (g_musicOk && IsSoundPlaying(g_music)) StopSound(g_music);
    for (int k = 0; k < 9; k++) if (k != j && IsSoundPlaying(g_jingles[k])) StopSound(g_jingles[k]);
    audioWalkoutStop();
    SetSoundVolume(g_jingles[j], 0.6f);
    PlaySound(g_jingles[j]);
}

// chants en boucle selon l'humeur du public ; un court silence entre deux reprises, comme dans un vrai stade
static bool g_frenchCrowd = false;
void audioSetFrenchCrowd(bool on) { g_frenchCrowd = on; }
void audioSupporters(int state, float strength) {
    if (!g_ok || !g_enabled) return;
    static float rest = 0; static int last = -1, playing = -1, rot = 0;
    int mood = state == CH_WHISTLES ? 1 : state == CH_PROTEST ? 2 : state == CH_ENCOURAGE ? 3 : state == CH_CELEBRATE ? 4 : 0;
    // chaque humeur a son répertoire : les chants s'enchaînent sans se répéter
    static const int REP[5][6] = { { 0, 9, 5, 12, 10, 7 }, { 1, 1, 1, 1, 1, 1 }, { 2, 2, 2, 2, 2, 2 }, { 3, 9, 6, 12, 11, 3 }, { 4, 10, 8, 12, 11, 9 } };
    if (state == CH_TENSE || strength <= 0) { for (auto& sound : g_fanVoice) StopSound(sound); last = playing = -1; return; }
    if (mood != last) { for (auto& sound : g_fanVoice) StopSound(sound); playing = -1; rest = 0; }
    float volume = std::min(0.85f, std::clamp(strength, 0.f, 1.f) * 1.7f * (state == CH_NORMAL ? .16f : state == CH_LOUD ? .3f : state == CH_ENCOURAGE ? .3f : state == CH_WHISTLES ? .34f : .42f));   // chants nettement plus présents
    if (playing >= 0 && IsSoundPlaying(g_fanVoice[playing])) { SetSoundVolume(g_fanVoice[playing], volume); rest = (state == CH_NORMAL ? 2.5f : 0.6f); last = mood; return; }
    if (last == mood && rest > 0) { rest -= GetFrameTime(); return; }
    playing = REP[mood][rot++ % 6];
    if (g_frenchCrowd && mood != 1 && mood != 2 && rot % 4 == 0) playing = 13;      // match de l'équipe de France : le public entonne La Marseillaise
    SetSoundVolume(g_fanVoice[playing], volume);
    PlaySound(g_fanVoice[playing]); last = mood;
}
// rendu brut d'un hymne (outils de vérification)
std::vector<float> audioRenderAnthem(int idx, int* sampleRate) {
    if (sampleRate) *sampleRate = SR;
    if (idx >= 0 && idx < NUM_REAL_ANTHEMS) return renderReal(REAL_ANTHEMS[idx]);
    if (idx < 0 && -1 - idx < NUM_GENERIC) { const AnthemDef& G = GENERIC_ANTHEMS[-1 - idx]; return renderAnthem(G.n, G.cnt, G.bpm, G.base, G.minor); }
    return {};
}

// ------------------------------------------------------------------ test des sons (Options > Son et musique)
// Lecture de n'importe quel son du jeu, même si le son ou la musique sont coupés dans les options.
static const char* SFX_NAMES[NUM_SFX] = { "Passe", "Coup de sifflet", "Sifflet long", "Clameur du but", "Rebond", "Poteau", "« Ooh » du public", "Sifflet du carton",
    "Coup de sifflet final", "Coup de poing (bagarre)", "Huées", "Tonnerre", "Sifflets du public", "Applaudissements", "La ola",
    "Frappe", "Filet", "Tacle", "Prise du gardien", "Parade du gardien", "Menu : déplacement", "Menu : validation", "Menu : retour", "Menu : réglage" };
static const char* CHANT_NAMES[NUM_CHANTS] = { "« Allez ! »", "Sifflets et huées", "« Démission ! »", "Encouragements rythmés (clap clap)", "« Olé ! »", "Chant du club (écharpes)",
    "Clapping viking", "Batucada", "« Et un, et deux... »", "« Aux armes ! »", "« Oh oh oh oh oh ohhh »", "« Qui ne saute pas... »", "« When the Saints Go Marching In »", "La Marseillaise (refrain du public)" };
static const char* JINGLE_NAMES[9] = { "Victoire", "Générique TV", "Trophée", "Hymne des étoiles (coupe d'Europe)", "Podium", "Publicité 1", "Publicité 2", "Publicité 3", "Sono du stade : but" };
int audioPreviewCount(int cat) { switch (cat) { case 0: return NUM_SFX; case 1: return NUM_CHANTS + 1; case 2: return NUM_TRACKS; case 3: return 9; case 4: return audioAnthemListCount(); default: return 0; } }
std::string audioPreviewName(int cat, int i) {
    switch (cat) {
    case 0: return i >= 0 && i < NUM_SFX ? SFX_NAMES[i] : "";
    case 1: return i == 0 ? std::string("Rumeur du stade (boucle)") : i - 1 < NUM_CHANTS ? std::string("Chant : ") + CHANT_NAMES[i - 1] : "";
    case 2: return i >= 0 && i < NUM_TRACKS ? TRACKS[i].name : "";
    case 3: return i >= 0 && i < 9 ? JINGLE_NAMES[i] : "";
    case 4: return audioAnthemListName(i);
    }
    return "";
}
void audioPreviewStop() {
    if (!g_ok) return;
    for (auto& s : g_sfx) StopSound(s);
    StopSound(g_crowd); for (auto& s : g_fanVoice) StopSound(s);
    if (g_musicOk) { for (auto& t : g_tracks) StopSound(t); for (auto& j : g_jingles) StopSound(j); }
    audioStopAnthem();
    g_prevCat = g_prevIdx = -1;
}
void audioPreview(int cat, int i) {
    if (!g_ok) return;
    audioPreviewStop();
    audioWalkoutStop();
    g_prevCat = cat; g_prevIdx = i;
    switch (cat) {
    case 0: if (i >= 0 && i < NUM_SFX) { SetSoundVolume(g_sfx[i], 1.f); PlaySound(g_sfx[i]); } break;
    case 1: if (i == 0) { SetSoundVolume(g_crowd, 0.8f); PlaySound(g_crowd); } else if (i - 1 < NUM_CHANTS) { SetSoundVolume(g_fanVoice[i - 1], 0.6f); PlaySound(g_fanVoice[i - 1]); } break;
    case 2: if (g_musicOk && i >= 0 && i < NUM_TRACKS) { g_curTrack = i; g_music = g_tracks[i]; SetSoundVolume(g_music, 0.6f); PlaySound(g_music); } break;
    case 3: if (g_musicOk && i >= 0 && i < 9) { SetSoundVolume(g_jingles[i], 0.6f); PlaySound(g_jingles[i]); } break;
    case 4: audioAnthemPlayIndex(audioAnthemListIndex(i)); break;
    }
}
bool audioPreviewPlaying() {
    if (!g_ok || g_prevCat < 0) return false;
    int i = g_prevIdx;
    switch (g_prevCat) {
    case 0: return i >= 0 && i < NUM_SFX && IsSoundPlaying(g_sfx[i]);
    case 1: return i == 0 ? IsSoundPlaying(g_crowd) : (i - 1 < NUM_CHANTS && IsSoundPlaying(g_fanVoice[i - 1]));
    case 2: return g_musicOk && i >= 0 && i < NUM_TRACKS && IsSoundPlaying(g_tracks[i]);
    case 3: return g_musicOk && i >= 0 && i < 9 && IsSoundPlaying(g_jingles[i]);
    case 4: return audioAnthemPlaying();
    }
    return false;
}
// rendus bruts des ambiances (outils de vérification)
std::vector<float> audioRenderCrowd(int what, int i) {
    if (what == 0) return renderChant(std::max(0, std::min(NUM_CHANTS - 1, i)));
    std::vector<float> v((size_t)(2.6f * SR), 0.f);
    srand(59 + i);
    if (what == 1) { addApplause(v, 0.f, 2.5f, 220, 1.f); addMurmur(v, 0.08f, 0.08f); stadiumSpace(v, 0.3f, 0.75f); }
    else { addCrowdWhistles(v, 34, 1.f, 53); addMurmur(v, 0.15f, 0.1f); }
    normalize(v, 0.75f, 0.05f);
    return v;
}
