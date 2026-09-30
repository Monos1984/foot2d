#pragma once
#include "raylib.h"
#include "match.h"
#include <string>

const int VW = 640, VH = 360;       // résolution virtuelle
const int MW = 320, MH = 180;       // résolution du match (style Amiga, affichée x2)
const float PPM = 5.0f;             // pixels par mètre
const float MARGIN = 12.0f;         // bordure autour du terrain (m)

Color hexc(unsigned rgb, unsigned char a = 255);
void renderInit();
void renderShutdown();
void renderMatch(const Match& m, bool radar);
void drawTextPx(const std::string& s, int x, int y, int size, Color c);
void drawTextShadow(const std::string& s, int x, int y, int size, Color c);
void drawTextCentered(const std::string& s, int cx, int y, int size, Color c, bool shadow = true);
int textWidth(const std::string& s, int size);
void drawKitIcon(const Kit& k, int x, int y, int scale);
void drawPlayerSprite(int x, int y, const Kit& kit, int skin, int hair, int dir, int frame, int state, bool gk, unsigned gkShirt, int scale = 1);
const int NUM_SKINS = 6, NUM_HAIRS = 7;
extern const char* SKIN_NAMES[NUM_SKINS];
extern const char* HAIR_NAMES[NUM_HAIRS];
void drawPortrait(int x, int y, int size, int skin, int hair, int gender, const Kit& kit, unsigned seed);
std::string fitText(const std::string& s, int maxw, int size);
