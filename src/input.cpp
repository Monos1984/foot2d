#include <cstdlib>
// Clavier(s), manettes (configurables) et souris
#include "input.h"
#include "render.h"
#include <cstdio>
#include <cstring>

Settings g_settings;
const int HALF_MINUTES[] = { 2, 3, 4, 5, 7, 10, 20, 45 };
const int NUM_HALF = 8;
float g_viewScale = 2, g_viewX = 0, g_viewY = 0;
bool g_textMode = false;

Settings::Settings() { resetControls(); }

void Settings::resetControls() {
    int k1[NUM_ACTIONS] = { KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_X, KEY_C, KEY_ESCAPE, KEY_V, KEY_B, KEY_N };
    int k2[NUM_ACTIONS] = { KEY_W, KEY_S, KEY_A, KEY_D, KEY_F, KEY_G, KEY_P, KEY_H, KEY_J, KEY_T };
    memcpy(keys[0], k1, sizeof k1); memcpy(keys[1], k2, sizeof k2);
    for (int p = 0; p < 4; p++) {
        pad[p][PA_PASS] = GAMEPAD_BUTTON_RIGHT_FACE_DOWN; pad[p][PA_SHOOT] = GAMEPAD_BUTTON_RIGHT_FACE_RIGHT; pad[p][PA_PAUSE] = GAMEPAD_BUTTON_MIDDLE_RIGHT;
        pad[p][PA_LOB] = GAMEPAD_BUTTON_RIGHT_FACE_LEFT; pad[p][PA_THROUGH] = GAMEPAD_BUTTON_RIGHT_FACE_UP; pad[p][PA_SPRINT] = GAMEPAD_BUTTON_RIGHT_TRIGGER_1;
    }
    deadzone = 0.35f;
}

float Settings::halfSeconds() const { return HALF_MINUTES[halfIdx] * 60.f; }

void Settings::load() {
    FILE* f = fopen("foot2d.ini", "r");
    if (!f) return;
    char k[64]; int v;
    while (fscanf(f, "%63[^=]=%d\n", k, &v) == 2) {
        if (!strcmp(k, "half")) halfIdx = v;
        else if (!strcmp(k, "difficulty")) difficulty = v;
        else if (!strcmp(k, "pitch")) pitch = v;
        else if (!strcmp(k, "radar")) radar = v;
        else if (!strcmp(k, "fullscreen")) fullscreen = v;
        else if (!strcmp(k, "sound")) sound = v;
        else if (!strcmp(k, "deadzone")) deadzone = v / 100.f;
        else if (!strcmp(k, "style")) controlStyle = v ? 1 : 0;
        else if (!strcmp(k, "music")) music = v;
        else if (!strcmp(k, "musictrack")) musicTrack = v;
        else if (!strcmp(k, "commentary")) commentary = v;
        else if (!strcmp(k, "vibration")) vibration = v;
        else {
            int a, b;
            if (sscanf(k, "kb%d_%d", &a, &b) == 2 && a >= 0 && a < 2 && b >= 0 && b < NUM_ACTIONS) keys[a][b] = v;
            else if (sscanf(k, "pad%d_%d", &a, &b) == 2 && a >= 0 && a < 4 && b >= 0 && b < NUM_PA) pad[a][b] = v;
        }
    }
    fclose(f);
    if (halfIdx < 0 || halfIdx >= NUM_HALF) halfIdx = 1;
    if (deadzone < 0.1f || deadzone > 0.9f) deadzone = 0.35f;
}

void Settings::save() const {
    FILE* f = fopen("foot2d.ini", "w");
    if (!f) return;
    fprintf(f, "half=%d\ndifficulty=%d\npitch=%d\nradar=%d\nfullscreen=%d\nsound=%d\ndeadzone=%d\nstyle=%d\nmusic=%d\ncommentary=%d\nmusictrack=%d\nvibration=%d\n", halfIdx, difficulty, pitch, radar, fullscreen, sound, (int)(deadzone * 100), controlStyle, music, commentary, musicTrack, (int)vibration);
    for (int a = 0; a < 2; a++) for (int b = 0; b < NUM_ACTIONS; b++) fprintf(f, "kb%d_%d=%d\n", a, b, keys[a][b]);
    for (int a = 0; a < 4; a++) for (int b = 0; b < NUM_PA; b++) fprintf(f, "pad%d_%d=%d\n", a, b, pad[a][b]);
    fclose(f);
}

bool inputAvailable(int dev) {
    if (dev <= IN_KB2) return true;
    static int fake = getenv("FOOT_PADS") ? atoi(getenv("FOOT_PADS")) : -1;    // test : manettes simulées
    if (fake >= 0) return dev - IN_PAD1 < fake;
    return IsGamepadAvailable(dev - IN_PAD1);
}

const char* inputName(int dev) {
    static const char* N[] = { "Clavier 1", "Clavier 2", "Manette 1", "Manette 2", "Manette 3", "Manette 4" };
    return N[dev];
}

const char* keyName(int k) {
    static char bufs[8][16]; static int bi = 0;          // plusieurs appels dans une même expression
    char* buf = bufs[bi = (bi + 1) % 8];
    if (k >= KEY_A && k <= KEY_Z) { snprintf(buf, 16, "%c", 'A' + (k - KEY_A)); return buf; }
    if (k >= KEY_ZERO && k <= KEY_NINE) { snprintf(buf, 16, "%c", '0' + (k - KEY_ZERO)); return buf; }
    switch (k) {
    case KEY_UP: return "Haut"; case KEY_DOWN: return "Bas"; case KEY_LEFT: return "Gauche"; case KEY_RIGHT: return "Droite";
    case KEY_SPACE: return "Espace"; case KEY_ENTER: return "Entrée"; case KEY_ESCAPE: return "Échap"; case KEY_TAB: return "Tab";
    case KEY_LEFT_SHIFT: return "Maj G"; case KEY_RIGHT_SHIFT: return "Maj D"; case KEY_LEFT_CONTROL: return "Ctrl G"; case KEY_RIGHT_CONTROL: return "Ctrl D";
    case KEY_LEFT_ALT: return "Alt"; case KEY_RIGHT_ALT: return "Alt Gr"; case KEY_BACKSPACE: return "Retour arr.";
    case KEY_KP_0: return "Pavé 0"; case KEY_KP_1: return "Pavé 1"; case KEY_KP_2: return "Pavé 2"; case KEY_KP_3: return "Pavé 3";
    case KEY_KP_4: return "Pavé 4"; case KEY_KP_5: return "Pavé 5"; case KEY_KP_6: return "Pavé 6"; case KEY_KP_8: return "Pavé 8";
    case KEY_KP_DECIMAL: return "Pavé ."; case KEY_KP_ENTER: return "Pavé Entrée";
    case KEY_COMMA: return ","; case KEY_PERIOD: return "."; case KEY_SEMICOLON: return ";"; case KEY_SLASH: return "/";
    default: snprintf(buf, 16, "#%d", k); return buf;
    }
}

const char* padButtonName(int b) {
    switch (b) {
    case GAMEPAD_BUTTON_RIGHT_FACE_DOWN: return "A / Croix";
    case GAMEPAD_BUTTON_RIGHT_FACE_RIGHT: return "B / Rond";
    case GAMEPAD_BUTTON_RIGHT_FACE_LEFT: return "X / Carré";
    case GAMEPAD_BUTTON_RIGHT_FACE_UP: return "Y / Triangle";
    case GAMEPAD_BUTTON_LEFT_TRIGGER_1: return "LB / L1";
    case GAMEPAD_BUTTON_RIGHT_TRIGGER_1: return "RB / R1";
    case GAMEPAD_BUTTON_LEFT_TRIGGER_2: return "LT / L2";
    case GAMEPAD_BUTTON_RIGHT_TRIGGER_2: return "RT / R2";
    case GAMEPAD_BUTTON_MIDDLE_LEFT: return "Select";
    case GAMEPAD_BUTTON_MIDDLE_RIGHT: return "Start";
    case GAMEPAD_BUTTON_LEFT_THUMB: return "Stick G";
    case GAMEPAD_BUTTON_RIGHT_THUMB: return "Stick D";
    default: return "?";
    }
}

std::string inputHelp(int dev) {
    if (dev <= IN_KB2) {
        const int* k = g_settings.keys[dev];
        std::string mv = dev == 0 && k[0] == KEY_UP ? std::string("Flèches") : std::string(keyName(k[0])) + keyName(k[2]) + keyName(k[1]) + keyName(k[3]);
        if (g_settings.controlStyle == 1) return mv + " + " + keyName(k[ACT_FIRE1]) + " passe, " + keyName(k[ACT_FIRE2]) + " tir, " + keyName(k[ACT_FIRE3]) + " lob, " + keyName(k[ACT_FIRE4]) + " profondeur, " + keyName(k[ACT_SPRINT]) + " sprint";
        return mv + " + " + keyName(k[ACT_FIRE1]) + " (passe/tir) " + keyName(k[ACT_FIRE2]) + " (lob/tacle)";
    }
    int p = dev - IN_PAD1;
    if (g_settings.controlStyle == 1) return std::string("Stick/croix + ") + padButtonName(g_settings.pad[p][PA_PASS]) + " passe, " + padButtonName(g_settings.pad[p][PA_SHOOT]) + " tir, " + padButtonName(g_settings.pad[p][PA_LOB]) + " lob, " + padButtonName(g_settings.pad[p][PA_THROUGH]) + " profondeur, " + padButtonName(g_settings.pad[p][PA_SPRINT]) + " sprint";
    return std::string("Stick/croix + ") + padButtonName(g_settings.pad[p][PA_PASS]) + " (passe/tir) " + padButtonName(g_settings.pad[p][PA_SHOOT]) + " (lob/tacle)";
}

static bool anyDown(std::initializer_list<int> keys) { for (int k : keys) if (k > 0 && IsKeyDown(k)) return true; return false; }
static bool anyPressed(std::initializer_list<int> keys) { for (int k : keys) if (k > 0 && IsKeyPressed(k)) return true; return false; }
static bool anyRepeat(std::initializer_list<int> keys) { for (int k : keys) if (k > 0 && (IsKeyPressed(k) || IsKeyPressedRepeat(k))) return true; return false; }

static float padRepeat[4][4];

void inputPoll(MenuInput& mi, Controls ctl[NUM_INPUTS]) {
    mi = MenuInput();
    Controls prev[NUM_INPUTS];
    for (int i = 0; i < NUM_INPUTS; i++) prev[i] = ctl[i];
    for (int i = 0; i < NUM_INPUTS; i++) ctl[i] = Controls();
    for (int kb = 0; kb < 2; kb++) {
        const int* k = g_settings.keys[kb];
        Controls& c = ctl[kb];
        c.dir.x = (float)(IsKeyDown(k[ACT_RIGHT]) - IsKeyDown(k[ACT_LEFT]));
        c.dir.y = (float)(IsKeyDown(k[ACT_DOWN]) - IsKeyDown(k[ACT_UP]));
        if (kb == 0) {
            c.f1 = anyDown({ k[ACT_FIRE1], KEY_RIGHT_CONTROL, KEY_KP_0 });
            c.f2 = anyDown({ k[ACT_FIRE2], KEY_KP_DECIMAL });
            c.pause = anyPressed({ k[ACT_PAUSE], KEY_ESCAPE });
        } else {
            c.f1 = anyDown({ k[ACT_FIRE1] });
            c.f2 = anyDown({ k[ACT_FIRE2] });
            c.pause = anyPressed({ k[ACT_PAUSE] });
        }
        c.f3 = anyDown({ k[ACT_FIRE3] }); c.f4 = anyDown({ k[ACT_FIRE4] }); c.sprint = anyDown({ k[ACT_SPRINT] });
        if (g_settings.controlStyle == 0) { c.f2 = c.f2 || c.f3; c.f3 = c.f4 = c.sprint = false; }
    }
    for (int p = 0; p < 4; p++) {
        Controls& c = ctl[IN_PAD1 + p];
        if (!IsGamepadAvailable(p)) continue;
        float ax = GetGamepadAxisMovement(p, GAMEPAD_AXIS_LEFT_X), ay = GetGamepadAxisMovement(p, GAMEPAD_AXIS_LEFT_Y);
        V2 d(ax, ay);
        if (d.len() < g_settings.deadzone) d = V2();
        if (IsGamepadButtonDown(p, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) d.x = -1;
        if (IsGamepadButtonDown(p, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) d.x = 1;
        if (IsGamepadButtonDown(p, GAMEPAD_BUTTON_LEFT_FACE_UP)) d.y = -1;
        if (IsGamepadButtonDown(p, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) d.y = 1;
        c.dir = d;
        c.f1 = IsGamepadButtonDown(p, g_settings.pad[p][PA_PASS]);
        c.f2 = IsGamepadButtonDown(p, g_settings.pad[p][PA_SHOOT]);
        c.f3 = IsGamepadButtonDown(p, g_settings.pad[p][PA_LOB]);
        c.f4 = IsGamepadButtonDown(p, g_settings.pad[p][PA_THROUGH]);
        c.sprint = IsGamepadButtonDown(p, g_settings.pad[p][PA_SPRINT]) || IsGamepadButtonDown(p, GAMEPAD_BUTTON_RIGHT_TRIGGER_2);
        if (g_settings.controlStyle == 0) { c.f2 = c.f2 || c.f3; c.f3 = c.f4 = c.sprint = false; }
        c.pause = IsGamepadButtonPressed(p, g_settings.pad[p][PA_PAUSE]);
    }
    for (int i = 0; i < NUM_INPUTS; i++) {
        Controls& c = ctl[i];
        if (c.dir.len2() > 1.01f) c.dir = c.dir.norm();
        c.f1p = c.f1 && !prev[i].f1; c.f2p = c.f2 && !prev[i].f2;
        c.f1r = !c.f1 && prev[i].f1; c.f2r = !c.f2 && prev[i].f2;
        c.f3p = c.f3 && !prev[i].f3; c.f4p = c.f4 && !prev[i].f4;
    }
    // ---- menus
    if (!g_textMode) {
        const int* k1 = g_settings.keys[0]; const int* k2 = g_settings.keys[1];
        mi.up = anyRepeat({ KEY_UP, k1[ACT_UP], k2[ACT_UP] });
        mi.down = anyRepeat({ KEY_DOWN, k1[ACT_DOWN], k2[ACT_DOWN] });
        mi.left = anyRepeat({ KEY_LEFT, k1[ACT_LEFT], k2[ACT_LEFT] });
        mi.right = anyRepeat({ KEY_RIGHT, k1[ACT_RIGHT], k2[ACT_RIGHT] });
        mi.ok = anyPressed({ KEY_ENTER, KEY_KP_ENTER, KEY_SPACE, k1[ACT_FIRE1], k2[ACT_FIRE1] });
        mi.back = anyPressed({ KEY_ESCAPE, KEY_BACKSPACE });
        mi.tab = anyPressed({ KEY_TAB, k1[ACT_FIRE2], k2[ACT_FIRE2] });
        mi.start = anyPressed({ KEY_ENTER, KEY_KP_ENTER });
        mi.btnNext = anyPressed({ KEY_F2 }); mi.btnPress = anyPressed({ KEY_F3 });
        mi.pgUp = anyRepeat({ KEY_PAGE_UP, KEY_Q });
        mi.pgDn = anyRepeat({ KEY_PAGE_DOWN, KEY_E });
        mi.devLeft[IN_KB1] = anyRepeat({ k1[ACT_LEFT] }); mi.devRight[IN_KB1] = anyRepeat({ k1[ACT_RIGHT] });
        mi.devLeft[IN_KB2] = anyRepeat({ k2[ACT_LEFT] }); mi.devRight[IN_KB2] = anyRepeat({ k2[ACT_RIGHT] });
    } else {
        mi.up = anyRepeat({ KEY_UP }); mi.down = anyRepeat({ KEY_DOWN });
        mi.back = anyPressed({ KEY_ESCAPE });
        mi.start = anyPressed({ KEY_ENTER, KEY_KP_ENTER });
    }
    float now = (float)GetTime();
    for (int p = 0; p < 4; p++) {
        if (!IsGamepadAvailable(p)) continue;
        float ax = GetGamepadAxisMovement(p, GAMEPAD_AXIS_LEFT_X), ay = GetGamepadAxisMovement(p, GAMEPAD_AXIS_LEFT_Y);
        bool st[4] = { ay < -0.6f || IsGamepadButtonDown(p, GAMEPAD_BUTTON_LEFT_FACE_UP),
                       ay > 0.6f || IsGamepadButtonDown(p, GAMEPAD_BUTTON_LEFT_FACE_DOWN),
                       ax < -0.6f || IsGamepadButtonDown(p, GAMEPAD_BUTTON_LEFT_FACE_LEFT),
                       ax > 0.6f || IsGamepadButtonDown(p, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) };
        bool fire[4];
        for (int k = 0; k < 4; k++) {
            fire[k] = false;
            if (st[k]) {
                if (padRepeat[p][k] == 0) { fire[k] = true; padRepeat[p][k] = now + 0.35f; }
                else if (now >= padRepeat[p][k]) { fire[k] = true; padRepeat[p][k] = now + 0.08f; }
            } else padRepeat[p][k] = 0;
        }
        mi.up |= fire[0]; mi.down |= fire[1]; mi.left |= fire[2]; mi.right |= fire[3];
        mi.devLeft[IN_PAD1 + p] = fire[2]; mi.devRight[IN_PAD1 + p] = fire[3];
        mi.ok |= IsGamepadButtonPressed(p, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
        mi.back |= IsGamepadButtonPressed(p, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
        mi.btnPress |= IsGamepadButtonPressed(p, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);     // X : onglet, ou bouton choisi
        mi.btnNext |= IsGamepadButtonPressed(p, GAMEPAD_BUTTON_RIGHT_FACE_UP);       // Y : parcourir les boutons de l'écran
        mi.start |= IsGamepadButtonPressed(p, GAMEPAD_BUTTON_MIDDLE_RIGHT);
        mi.pgUp |= IsGamepadButtonPressed(p, GAMEPAD_BUTTON_LEFT_TRIGGER_1);
        mi.pgDn |= IsGamepadButtonPressed(p, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);
    }
    Vector2 m = GetMousePosition();
    mi.mouse = Vector2{ (m.x - g_viewX) / g_viewScale, (m.y - g_viewY) / g_viewScale };
    Vector2 md = GetMouseDelta();
    mi.mouseMoved = md.x != 0 || md.y != 0;
    mi.click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    mi.rclick = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    mi.wheel = GetMouseWheelMove();
    if (mi.rclick) mi.back = true;
    mi.anyKey = mi.ok || mi.back || mi.start || mi.click;
}
