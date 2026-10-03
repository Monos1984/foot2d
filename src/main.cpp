// Super Soccer World - point d'entrée
#include "render.h"
#include "input.h"
#include "audio.h"
#include "icon_data.h"
#include "crashlog.h"

void appInit();
void appFrame(float dt);
void appTestStart(const char* mode);
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
extern bool g_quit;

// capture d'écran (F12 ou Impr. écran) : image du jeu agrandie x2, enregistrée en PNG dans « captures » à côté de l'exécutable
static float g_shotMsgT = 0; static std::string g_shotMsg;
static void saveScreenshot(const RenderTexture2D& target) {
    std::string dir = std::string(GetApplicationDirectory()) + "captures";
    if (!DirectoryExists(dir.c_str())) MakeDirectory(dir.c_str());
    Image img = LoadImageFromTexture(target.texture);
    ImageFlipVertical(&img);
    ImageResizeNN(&img, VW * 2, VH * 2);
    time_t now = time(nullptr); struct tm* lt = localtime(&now);
    char name[64]; strftime(name, sizeof name, "capture_%Y%m%d_%H%M%S", lt);
    std::string path = dir + "/" + name + ".png";
    for (int k = 2; FileExists(path.c_str()) && k < 100; k++) path = dir + "/" + name + "_" + std::to_string(k) + ".png";
    bool ok = ExportImage(img, path.c_str());
    UnloadImage(img);
    g_shotMsg = ok ? std::string("Capture enregistrée : captures/") + GetFileName(path.c_str()) : std::string("Échec de la capture d'écran");
    g_shotMsgT = 2.5f;
}

int main(int argc, char** argv) {
    if (argc > 1 && !strcmp(argv[1], "--validate-france")) {
        g_world.build();
        for (const auto& p : g_basePyramids) if (p.country == "FRA" && p.dom < 0) {
            auto errors = validateFrancePyramid(p, true);
            for (const auto& error : errors) std::printf("ERROR %s\n", error.c_str());
            std::printf("%s: %d imported senior teams\n", errors.empty() ? "PASS" : "FAIL", NUM_FR_OFFICIAL_2627);
            return errors.empty() ? 0 : 1;
        }
        return 2;
    }
    crashLogInit();
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, "Super Soccer World");
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
        if (IsKeyPressed(KEY_F12) || IsKeyPressed(KEY_PRINT_SCREEN)) saveScreenshot(target);
        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(target.texture, Rectangle{ 0, 0, (float)VW, -(float)VH }, Rectangle{ g_viewX, g_viewY, VW * sc, VH * sc }, Vector2{ 0, 0 }, 0, WHITE);
        if (g_shotMsgT > 0) {
            g_shotMsgT -= dt;
            int fs = std::max(10, (int)(10 * sc));
            int w = MeasureText(g_shotMsg.c_str(), fs);
            unsigned char a = (unsigned char)(255 * std::min(1.f, g_shotMsgT * 2.f));
            DrawRectangle((int)g_viewX + 8, (int)g_viewY + 8, w + 16, fs + 10, Color{ 0, 0, 0, (unsigned char)(a * 0.7f) });
            DrawText(g_shotMsg.c_str(), (int)g_viewX + 16, (int)g_viewY + 13, fs, Color{ 255, 225, 90, a });
        }
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
