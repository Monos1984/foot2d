// France Foot 2D - point d'entrée
#include "render.h"
#include "input.h"
#include "audio.h"
#include "icon_data.h"

void appInit();
void appFrame(float dt);
void appTestStart(const char* mode);
#include <cstdlib>
#include <cstdio>
#include <cstring>
extern bool g_quit;

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, "France Foot 2D");
    {
        Image ic = GenImageColor(32, 32, BLANK);
        memcpy(ic.data, ICON32, sizeof ICON32);
        SetWindowIcon(ic);
        UnloadImage(ic);
    }
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    SetWindowMinSize(640, 360);
    audioInit();
    appInit();
    if (g_settings.fullscreen) ToggleBorderlessWindowed();
    RenderTexture2D target = LoadRenderTexture(VW, VH);
    const char* test = getenv("FOOT_TEST");
    int frame = 0;
    if (test) appTestStart(test);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);
    while (!WindowShouldClose() && !g_quit) {
        float dt = GetFrameTime();
        int sw = GetScreenWidth(), sh = GetScreenHeight();
        float sc = std::min((float)sw / VW, (float)sh / VH);
        if (sc >= 1.0f) { float isc = std::floor(sc); if (isc >= 1 && sc - isc < 0.25f) sc = isc; }
        g_viewScale = sc;
        g_viewX = (sw - VW * sc) * 0.5f; g_viewY = (sh - VH * sc) * 0.5f;
        BeginTextureMode(target);
        appFrame(dt);
        EndTextureMode();
        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(target.texture, Rectangle{ 0, 0, (float)VW, -(float)VH }, Rectangle{ g_viewX, g_viewY, VW * sc, VH * sc }, Vector2{ 0, 0 }, 0, WHITE);
        EndDrawing();
        frame++;
        if (test) {
            int every = atoi(getenv("FOOT_EVERY") ? getenv("FOOT_EVERY") : "150");
            int maxf = atoi(getenv("FOOT_FRAMES") ? getenv("FOOT_FRAMES") : "900");
            if (frame % every == 0) { char b[64]; snprintf(b, sizeof b, "shot_%05d.png", frame); TakeScreenshot(b); }
            if (frame >= maxf) break;
        }
    }
    UnloadRenderTexture(target);
    renderShutdown();
    audioShutdown();
    CloseWindow();
    return 0;
}
