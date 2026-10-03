// Vibrations des manettes : XInput chargé dynamiquement sous Windows ; ailleurs, SetGamepadVibration de raylib.
// Chaque effet est une suite d'impulsions (début, durée, moteur gauche, moteur droit, déclin) jouées en superposition.
#include "rumble.h"
#include <vector>
#include <algorithm>
#include <cmath>

namespace {
struct Pulse { float t0, dur, left, right, decay; };
struct PadState { std::vector<Pulse> pulses; float t = 0; float lastL = -1, lastR = -1; float curL = 0, curR = 0; };
PadState g_pad[4];
float g_strength = 0.8f;
}

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
namespace {
struct XVib { WORD left, right; };
typedef DWORD(WINAPI* XSetState)(DWORD, XVib*);
XSetState g_set = nullptr;
bool g_tried = false;
void load() {
    if (g_tried) return;
    g_tried = true;
    const char* dlls[] = { "xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll" };
    for (auto d : dlls) {
        HMODULE h = LoadLibraryA(d);
        if (!h) continue;
        g_set = (XSetState)(void*)GetProcAddress(h, "XInputSetState");
        if (g_set) return;
    }
}
void apply(int pad, float l, float r) {
    load();
    if (!g_set) return;
    auto w = [](float s) { return (WORD)(s <= 0 ? 0 : s >= 1 ? 65535 : s * 65535); };
    XVib x{ w(l), w(r) };
    g_set((DWORD)pad, &x);
}
}
#else
#include "raylib.h"      // pas avec windows.h (conflits de noms)
namespace { void apply(int pad, float l, float r) { if (IsWindowReady() && IsGamepadAvailable(pad)) SetGamepadVibration(pad, l, r, 0.12f); } }
#endif

static void add(int pad, float t0, float dur, float l, float r, float decay = 0) {
    PadState& P = g_pad[pad];
    P.pulses.push_back({ P.t + t0, dur, l, r, decay });
}

void rumbleSetStrength(float k) { g_strength = std::max(0.f, std::min(1.f, k)); if (g_strength <= 0) rumbleStopAll(); }

const char* rumbleName(int kind) {
    static const char* N[NUM_RUMBLE] = { "Passe", "Frappe", "Tacle", "Faute subie", "Poteau / barre", "But marqué", "But encaissé", "Arrêt du gardien",
                                         "Carton", "Coup de sifflet final", "Battements de cœur (penalty)", "Tête", "Ferveur du public", "Essai" };
    return kind >= 0 && kind < NUM_RUMBLE ? N[kind] : "";
}

void rumblePlay(int pad, int kind) {
    if (pad < 0 || pad > 3 || g_strength <= 0) return;
    switch (kind) {
    case RB_KICK: add(pad, 0, 0.07f, 0.05f, 0.35f); break;
    case RB_SHOT: add(pad, 0, 0.16f, 0.55f, 0.8f, 6.f); break;
    case RB_TACKLE: add(pad, 0, 0.2f, 0.7f, 0.25f, 4.f); break;
    case RB_FOULED: add(pad, 0, 0.12f, 0.9f, 0.6f); add(pad, 0.12f, 0.3f, 0.5f, 0.f, 5.f); break;
    case RB_POST: add(pad, 0, 0.08f, 0.4f, 1.f); add(pad, 0.08f, 0.35f, 0.f, 0.45f, 7.f); break;
    case RB_GOAL_FOR: for (int k = 0; k < 3; k++) add(pad, k * 0.22f, 0.14f, 0.8f, 0.9f); add(pad, 0.7f, 0.6f, 0.6f, 0.3f, 3.f); break;
    case RB_GOAL_AGAINST: add(pad, 0, 0.7f, 0.6f, 0.f, 3.f); break;
    case RB_SAVE: add(pad, 0, 0.2f, 0.6f, 0.45f, 5.f); break;
    case RB_CARD: add(pad, 0, 0.09f, 0.f, 0.6f); add(pad, 0.18f, 0.09f, 0.f, 0.6f); break;
    case RB_WHISTLE_END: add(pad, 0, 0.12f, 0.3f, 0.5f); add(pad, 0.25f, 0.12f, 0.3f, 0.5f); add(pad, 0.5f, 1.f, 0.35f, 0.4f, 1.5f); break;
    case RB_HEARTBEAT: add(pad, 0, 0.08f, 0.55f, 0.f); add(pad, 0.18f, 0.1f, 0.4f, 0.f); break;
    case RB_BOUNCE_HEAD: add(pad, 0, 0.06f, 0.2f, 0.5f); break;
    case RB_CROWD: add(pad, 0, 1.2f, 0.25f, 0.1f, 0.8f); break;
    default: add(pad, 0, 0.3f, 0.6f, 0.6f); break;
    }
}

void rumbleStart(int pad, float strength, float seconds) {
    if (pad < 0 || pad > 3) return;
    add(pad, 0, seconds, strength, strength * 0.75f);
}

void rumbleUpdate(float dt) {
    for (int p = 0; p < 4; p++) {
        PadState& P = g_pad[p];
        if (P.pulses.empty() && P.lastL == 0 && P.lastR == 0) continue;
        P.t += dt;
        float l = 0, r = 0;
        for (auto& u : P.pulses) {
            float a = P.t - u.t0;
            if (a < 0 || a > u.dur) continue;
            float e = u.decay > 0 ? std::exp(-a * u.decay) : 1.f;
            l = std::max(l, u.left * e); r = std::max(r, u.right * e);
        }
        P.pulses.erase(std::remove_if(P.pulses.begin(), P.pulses.end(), [&](const Pulse& u) { return P.t - u.t0 > u.dur; }), P.pulses.end());
        if (P.pulses.empty()) P.t = 0;
        l *= g_strength; r *= g_strength;
        P.curL = l; P.curR = r;
        if (std::fabs(l - P.lastL) > 0.02f || std::fabs(r - P.lastR) > 0.02f || (l == 0 && r == 0 && (P.lastL != 0 || P.lastR != 0))) { apply(p, l, r); P.lastL = l; P.lastR = r; }
    }
}

float rumbleLevel(int pad, int motor) { return pad < 0 || pad > 3 ? 0 : motor ? g_pad[pad].curR : g_pad[pad].curL; }

void rumbleStopAll() { for (int p = 0; p < 4; p++) { g_pad[p].pulses.clear(); g_pad[p].t = 0; g_pad[p].curL = g_pad[p].curR = 0; apply(p, 0, 0); g_pad[p].lastL = g_pad[p].lastR = 0; } }
