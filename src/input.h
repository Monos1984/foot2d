#pragma once
#include "match.h"
#include "raylib.h"

struct MenuInput {
    bool up = false, down = false, left = false, right = false, ok = false, back = false, tab = false, start = false;
    bool pgUp = false, pgDn = false;
    bool btnNext = false, btnPress = false;   // manette : Y = bouton suivant, X = activer le bouton choisi
    bool devLeft[NUM_INPUTS] = {}, devRight[NUM_INPUTS] = {};
    bool click = false, rclick = false, mouseMoved = false;
    float wheel = 0;
    Vector2 mouse{ 0, 0 };
    bool anyKey = false;
};

enum Action { ACT_UP = 0, ACT_DOWN, ACT_LEFT, ACT_RIGHT, ACT_FIRE1, ACT_FIRE2, ACT_PAUSE, ACT_FIRE3, ACT_FIRE4, ACT_SPRINT, NUM_ACTIONS };
enum PadAction { PA_PASS = 0, PA_SHOOT, PA_PAUSE, PA_LOB, PA_THROUGH, PA_SPRINT, NUM_PA };

struct Settings {
    int halfIdx = 1;
    int difficulty = 1;
    int pitch = 5;
    bool radar = true;
    bool fullscreen = false;
    bool sound = true;
    int keys[2][NUM_ACTIONS];          // claviers 1 et 2
    int pad[4][NUM_PA];                // manettes : passe, tir, pause, lob, passe en profondeur, sprint
    int controlStyle = 1;              // 0 classique (2 boutons), 1 Super Nintendo (4 boutons + sprint)
    bool music = true, commentary = true;
    int musicTrack = 0;          // 0 enchaînement des thèmes, 1..N thème fixe
    float deadzone = 0.35f;
    bool vibration = true;       // vibrations des manettes
    int vibForce = 2;            // intensité : 1 faibles, 2 moyennes, 3 fortes
    bool lockerRoom = true;
    int refKit = 0;              // tenue des arbitres : 0 noire, 1 jaune, 2 verte, 3 rouge, 4 bleue      // scènes de vestiaire avant le match, à la mi-temps et après le match
    Settings();
    void resetControls();
    void load();
    void save() const;
    float halfSeconds() const;
};
extern Settings g_settings;
extern const int HALF_MINUTES[];
extern const int NUM_HALF;
extern float g_viewScale, g_viewX, g_viewY;
extern bool g_textMode;                // saisie de texte en cours : pas de raccourcis clavier

void inputPoll(MenuInput& mi, Controls ctl[NUM_INPUTS]);
bool inputAvailable(int dev);
const char* inputName(int dev);
std::string inputHelp(int dev);
const char* keyName(int key);
const char* padButtonName(int b);
