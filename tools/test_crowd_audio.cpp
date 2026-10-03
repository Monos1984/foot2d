// Ambiance : rendu des 12 chants, des applaudissements et des sifflets ; durée de calcul, niveau ; export WAV avec un argument
#include "../src/audio.h"
#include <cstdio>
#include <cmath>
#include <cstdint>
#include <chrono>
#include <string>
static void wav(const std::string& path, const std::vector<float>& v) {
    FILE* f = fopen(path.c_str(), "wb"); if (!f) return;
    uint32_t n = (uint32_t)v.size(), bytes = n * 2, r = 22050, br = 44100, sz = 36 + bytes, sixteen = 16; uint16_t fmt = 1, ch = 1, ba = 2, bits = 16;
    fwrite("RIFF", 1, 4, f); fwrite(&sz, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f); fwrite(&sixteen, 4, 1, f);
    fwrite(&fmt, 2, 1, f); fwrite(&ch, 2, 1, f); fwrite(&r, 4, 1, f); fwrite(&br, 4, 1, f); fwrite(&ba, 2, 1, f); fwrite(&bits, 2, 1, f);
    fwrite("data", 1, 4, f); fwrite(&bytes, 4, 1, f);
    for (float x : v) { int16_t s = (int16_t)std::max(-32767.f, std::min(32767.f, x * 32767.f)); fwrite(&s, 2, 1, f); }
    fclose(f);
}
int main(int argc, char**) {
    int bad = 0; double total = 0;
    for (int what = 0; what < 3; what++) for (int i = 0; i < (what == 0 ? 12 : 1); i++) {
        auto t0 = std::chrono::steady_clock::now();
        auto v = audioRenderCrowd(what, i);
        double sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count(); total += sec;
        float peak = 0, rms = 0; for (float x : v) { peak = std::max(peak, std::fabs(x)); rms += x * x; } rms = std::sqrt(rms / std::max<size_t>(1, v.size()));
        printf("%s %2d : %.1f s de son, calcul %.2f s, crête %.2f, RMS %.3f\n", what == 0 ? "chant" : what == 1 ? "applaudissements" : "sifflets", i, v.size() / 22050.0, sec, peak, rms);
        if (v.empty() || peak > 1.f || rms < 0.02f) bad++;
        if (argc > 1) wav("build/crowd_" + std::to_string(what) + "_" + std::to_string(i) + ".wav", v);
    }
    printf("calcul total %.2f s\n", total);
    if (bad) { printf("FAIL crowd audio: %d\n", bad); return 1; }
    printf("PASS crowd audio\n");
}
