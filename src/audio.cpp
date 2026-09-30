// Sons synthétisés (aucun fichier externe)
#include "audio.h"
#include "raylib.h"
#include "match.h"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>

static Sound g_sfx[12];
static Sound g_crowd;
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
static const Track TRACKS[] = {
    { "Super Soccer World", 138.f, MEL1, 128, BASS1, 16, 523.25f, 0.25f, 0, 0, 1.0f },
    { "Vestiaire", 112.f, MEL2, 128, BASS2, 16, 440.00f, 0.5f, 1, 1, 0.95f },
    { "Nuit européenne", 96.f, MEL3, 128, BASS3, 16, 392.00f, 0.125f, 3, 2, 0.9f },
    { "Mercato", 124.f, MEL4, 128, BASS4, 16, 392.00f, 0.25f, 2, 0, 0.95f },
};
static const int NUM_TRACKS = 4;
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
static Sound g_jingles[5];
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
    g_sfx[SFX_BOO] = makeSound(noise(1.6f, 0.10f, 1.2f, [](float t) { return std::sin(t * 3.14159f) * 0.8f; }));
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
    // musiques des menus : thèmes chiptune originaux (voir renderTrack)
    for (int k = 0; k < NUM_TRACKS; k++) { g_tracks[k] = makeSound(renderTrack(TRACKS[k])); }
    g_music = g_tracks[0];
    g_musicOk = true;
    g_jingles[0] = makeSound(renderTrack(JINGLE_WIN));
    g_jingles[1] = makeSound(renderTrack(JINGLE_TV));
    g_jingles[2] = makeSound(renderTrack(JINGLE_TROPHY));
    g_jingles[3] = makeSound(renderTrack(JINGLE_ANTHEM));
    g_jingles[4] = makeSound(renderTrack(JINGLE_PODIUM));
    g_ok = true;
}

void audioShutdown() {
    if (!g_ok) return;
    for (auto& s : g_sfx) UnloadSound(s);
    UnloadSound(g_crowd);
    if (g_musicOk) { for (auto& t : g_tracks) UnloadSound(t); for (auto& j : g_jingles) UnloadSound(j); }
    CloseAudioDevice();
}

void audioSetEnabled(bool e) { g_enabled = e; if (!e && g_ok) StopSound(g_crowd); }

void audioPlay(int s) {
    if (!g_ok || !g_enabled || s < 0 || s >= 12) return;
    PlaySound(g_sfx[s]);
}

void audioCrowd(bool on, float vol) {
    if (!g_ok) return;
    if (!on || !g_enabled) { if (IsSoundPlaying(g_crowd)) StopSound(g_crowd); return; }
    SetSoundVolume(g_crowd, vol);
    if (!IsSoundPlaying(g_crowd)) PlaySound(g_crowd);
}

void audioMusic(bool on) {
    if (!g_ok || !g_musicOk) return;
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
// hymne national (mélodie solennelle originale, générée à partir de l'identifiant de l'équipe)
#include <map>
static std::map<unsigned, Sound> g_anthems;
static Sound g_anthemCur; static bool g_anthemOn = false;
void audioAnthem(unsigned seed) {
    if (!g_ok || !g_musicOn) return;
    auto it = g_anthems.find(seed);
    if (it == g_anthems.end()) {
        static int mel[32]; static int bass[4];
        unsigned r = seed * 2654435761u + 12345u;
        auto rnd = [&](int n) { r = r * 1103515245u + 12345u; return (int)((r >> 16) % (unsigned)n); };
        static const int SCALE[8] = { 0, 2, 4, 5, 7, 9, 11, 12 };
        int deg = rnd(3) == 0 ? 4 : 0;
        for (int k = 0; k < 32; k++) {
            if (k % 16 == 15 || k % 16 == 14) { mel[k] = k % 16 == 14 ? -98 : -99; continue; }
            if (k % 2 == 1 && rnd(3) > 0) { mel[k] = -98; continue; }       // notes longues
            int step = rnd(5) - 2; if (step == 0 && rnd(2)) step = 1;
            deg = std::max(0, std::min(7, deg + step));
            if (k >= 24) deg = std::max(0, deg - 1);                          // cadence vers la tonique
            mel[k] = SCALE[deg];
        }
        mel[28] = 0; mel[29] = -98; mel[30] = -98; mel[31] = -99;
        static const int PROG[4][8] = { { 0, 5, 7, 0, 5, 0, 7, 0 }, { 0, 9, 5, 7, 0, 5, 7, 0 }, { 0, 7, 9, 5, 0, 7, 5, 0 }, { 0, 4, 5, 7, 9, 5, 7, 0 } };
        int pr = rnd(4); for (int b = 0; b < 4; b++) bass[b] = PROG[pr][b * 2];
        Track T = { "Hymne", 70.f + rnd(12), mel, 32, bass, 4, 392.f * std::pow(2.f, (rnd(5) - 2) / 12.f), 0.5f, 3, 2, 1.0f };
        g_anthems[seed] = makeSound(renderTrack(T));
        it = g_anthems.find(seed);
    }
    if (g_musicOk && IsSoundPlaying(g_music)) StopSound(g_music);
    if (g_anthemOn) StopSound(g_anthemCur);
    g_anthemCur = it->second; g_anthemOn = true;
    SetSoundVolume(g_anthemCur, 0.6f);
    PlaySound(g_anthemCur);
}
void audioStopAnthem() { if (g_ok && g_anthemOn) { StopSound(g_anthemCur); g_anthemOn = false; } }

void audioJingle(int j) {
    if (!g_ok || !g_musicOn || j < 0 || j > 4) return;
    if (g_musicOk && IsSoundPlaying(g_music)) StopSound(g_music);
    SetSoundVolume(g_jingles[j], 0.6f);
    PlaySound(g_jingles[j]);
}
