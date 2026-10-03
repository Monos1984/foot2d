// Hymnes : rendu de chaque hymne réel, durée, niveau, et export WAV de quelques-uns (build/anthem_XX.wav) pour écoute
#include "../src/audio.h"
#include <cstdio>
#include <cmath>
#include <cstdint>
#include <string>
static void wav(const char* path, const std::vector<float>& v, int sr) {
    FILE* f = fopen(path, "wb"); if (!f) return;
    uint32_t n = (uint32_t)v.size(), bytes = n * 2, r = sr, br = sr * 2, sz = 36 + bytes; uint16_t fmt = 1, ch = 1, ba = 2, bits = 16;
    fwrite("RIFF", 1, 4, f); fwrite(&sz, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f); uint32_t sixteen = 16; fwrite(&sixteen, 4, 1, f);
    fwrite(&fmt, 2, 1, f); fwrite(&ch, 2, 1, f); fwrite(&r, 4, 1, f); fwrite(&br, 4, 1, f); fwrite(&ba, 2, 1, f); fwrite(&bits, 2, 1, f);
    fwrite("data", 1, 4, f); fwrite(&bytes, 4, 1, f);
    for (float x : v) { int16_t s = (int16_t)std::max(-32767.f, std::min(32767.f, x * 32767.f)); fwrite(&s, 2, 1, f); }
    fclose(f);
}
int main(int argc, char** argv) {
    int n = audioAnthemListCount(), bad = 0;
    for (int i = 0; i < n; i++) {
        int sr = 0; auto v = audioRenderAnthem(audioAnthemListIndex(i), &sr);
        float peak = 0; for (float x : v) peak = std::max(peak, std::fabs(x));
        float sec = (float)v.size() / sr;
        printf("%2d %-70s %5.1f s peak %.2f\n", i, audioAnthemListName(i).c_str(), sec, peak);
        if (sec < 8 || sec > 90 || peak < 0.3f || peak > 1.f) bad++;
        if (argc > 1) { std::string p = "build/anthem_" + std::to_string(i) + ".wav"; wav(p.c_str(), v, sr); }
    }
    if (bad) { printf("FAIL anthems: %d\n", bad); return 1; }
    printf("PASS anthems: %d rendered\n", n);
}
