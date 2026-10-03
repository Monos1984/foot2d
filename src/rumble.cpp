// Vibrations des manettes : XInput chargé dynamiquement sous Windows (raylib/GLFW ne gère pas les vibrations)
#include "rumble.h"

static float g_left[4] = { 0, 0, 0, 0 };

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
void apply(int pad, float s) {
    load();
    if (!g_set) return;
    WORD v = (WORD)(s < 0 ? 0 : s > 1 ? 65535 : s * 65535);
    XVib x{ v, (WORD)(v * 3 / 4) };
    g_set((DWORD)pad, &x);
}
}
#else
static void apply(int, float) {}
#endif

void rumbleStart(int pad, float strength, float seconds) {
    if (pad < 0 || pad > 3) return;
    if (g_left[pad] > 0 && strength <= 0) return;
    g_left[pad] = seconds;
    apply(pad, strength);
}

void rumbleUpdate(float dt) {
    for (int p = 0; p < 4; p++) if (g_left[p] > 0) { g_left[p] -= dt; if (g_left[p] <= 0) { g_left[p] = 0; apply(p, 0); } }
}

void rumbleStopAll() { for (int p = 0; p < 4; p++) { g_left[p] = 0; apply(p, 0); } }
