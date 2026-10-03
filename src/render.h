#pragma once
#include "raylib.h"
#include "match.h"
#include <string>
#include <vector>

const int VW = 640, VH = 360;       // résolution virtuelle
const int MW = 320, MH = 180;       // résolution du match (style Amiga, affichée x2)
const float PPM = 5.0f;             // pixels par mètre
const float MARGIN = 12.0f;         // bordure autour du terrain (m)

Color hexc(unsigned rgb, unsigned char a = 255);
void renderInit();
void setCustomBoardAds(const std::vector<std::string>& paths);   // images de l'éditeur de publicités (panneaux du stade)
void renderShutdown();
void renderMatch(const Match& m, bool radar);
float crowdWavePhase(const Match& m);   // la ola : -1 si inactive, sinon position (0..1) du tour du stade
void drawTextPx(const std::string& s, int x, int y, int size, Color c);
void drawTextShadow(const std::string& s, int x, int y, int size, Color c);
void drawTextCentered(const std::string& s, int cx, int y, int size, Color c, bool shadow = true);
int textWidth(const std::string& s, int size);
void drawKitIcon(const Kit& k, int x, int y, int scale);
void drawPlayerSprite(int x, int y, const Kit& kit, int skin, int hair, int dir, int frame, int state, bool gk, unsigned gkShirt, int scale = 1);
// poses particulières (bancs, célébrations, cérémonies)
enum SprPose { POSE_NONE = 0, POSE_SIT, POSE_KNEEL, POSE_SHUSH, POSE_HEAD, POSE_POINT, POSE_ARMSOUT, POSE_CRADLE, POSE_CLAP };
void drawPosedSprite(int x, int y, const Kit& kit, int skin, int hair, int dir, int frame, int state, int pose, int scale = 1);
// décors animés communs aux cérémonies (scenes.cpp)
void fxCrowd(int x, int y, int w, int h, Color c1, Color c2, float t, float excite, unsigned seed, int cell = 4);
void fxConfetti(int x, int y, int w, int h, float t, const Color* cols, int ncol, int n, unsigned seed);
void fxFireworks(int x, int y, int w, int h, float t, unsigned seed, int n = 4);
void fxSpotlights(int x, int y, int w, int h, float t, int n, Color c);
void fxBigTrophy(int x, int y, int style, int s, float t);
void fxCurtains(int x, int y, int w, int h, Color c1, Color c2);
void fxFlare(int x, int y, float t, unsigned seed);
const int NUM_SKINS = 6, NUM_HAIRS = 7;
extern const char* SKIN_NAMES[NUM_SKINS];
extern const char* HAIR_NAMES[NUM_HAIRS];
void drawPortrait(int x, int y, int size, int skin, int hair, int gender, const Kit& kit, unsigned seed);
std::string fitText(const std::string& s, int maxw, int size);
