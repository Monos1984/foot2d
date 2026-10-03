// Sérialisation binaire simple
// Version 35 : mode « packed » — chaque valeur triviale est écrite comme une suite d'entiers 32 bits en varint zigzag,
// les suites de zéros étant regroupées (les champs 0..100 tiennent sur 1 octet, les chaînes de taille fixe vides presque rien).
#pragma once
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <type_traits>
#ifdef _WIN32
#define PACK_GETC(f) _fgetc_nolock(f)
#define PACK_PUTC(c, f) _fputc_nolock(c, f)
#else
#define PACK_GETC(f) getc_unlocked(f)
#define PACK_PUTC(c, f) putc_unlocked(c, f)
#endif

inline void packPutVF(FILE* f, uint64_t u) {
    while (u >= 0x80) { PACK_PUTC((int)((u & 0x7f) | 0x80), f); u >>= 7; }
    PACK_PUTC((int)u, f);
}
inline uint64_t packGetVF(FILE* f, bool& ok) {
    uint64_t u = 0; int sh = 0;
    for (int k = 0; k < 10; k++) {
        int c = PACK_GETC(f);
        if (c == EOF) { ok = false; return 0; }
        u |= (uint64_t)(c & 0x7f) << sh; sh += 7;
        if (!(c & 0x80)) return u;
    }
    ok = false; return 0;
}
inline uint32_t packZig(int32_t v) { return ((uint32_t)v << 1) ^ (uint32_t)(v >> 31); }
inline int32_t packUnzig(uint32_t u) { return (int32_t)(u >> 1) ^ -(int32_t)(u & 1); }
// chaîne de taille fixe : longueur + caractères utiles
inline void packCharsWF(FILE* f, const char* a, size_t cap) { size_t n = strnlen(a, cap); packPutVF(f, n); if (n) fwrite(a, 1, n, f); }
inline void packCharsRF(FILE* f, char* a, size_t cap, bool& ok) { uint64_t n = packGetVF(f, ok); if (!ok || n > cap) { ok = false; return; } memset(a, 0, cap); if (n && fread(a, 1, (size_t)n, f) != n) ok = false; if (n == cap && cap) a[cap - 1] = 0; }
// registre trié d'identifiants 64 bits (« déjà traité ») : écarts successifs en varint
inline void packLedgerWF(FILE* f, const std::vector<uint64_t>& v) { packPutVF(f, v.size()); uint64_t prev = 0; for (uint64_t x : v) { packPutVF(f, x - prev); prev = x; } }
inline void packLedgerRF(FILE* f, std::vector<uint64_t>& v, size_t maxN, bool& ok) { uint64_t n = packGetVF(f, ok); if (!ok || n > maxN) { ok = false; return; } v.resize((size_t)n); uint64_t prev = 0; for (auto& x : v) { prev += packGetVF(f, ok); x = prev; if (!ok) return; } }
inline void packBytesWF(FILE* f, const void* p, size_t sz) {
    size_t n = sz / 4;
    const unsigned char* c = (const unsigned char*)p;
    for (size_t i = 0; i < n;) {
        int32_t a; memcpy(&a, c + i * 4, 4);
        if (a == 0) {
            size_t j = i; while (j < n) { int32_t b; memcpy(&b, c + j * 4, 4); if (b) break; j++; }
            packPutVF(f, 0); packPutVF(f, j - i - 1); i = j;
        } else { packPutVF(f, packZig(a)); i++; }
    }
    if (sz % 4) fwrite(c + n * 4, 1, sz % 4, f);
}
inline void packBytesRF(FILE* f, void* p, size_t sz, bool& ok) {
    size_t n = sz / 4;
    unsigned char* c = (unsigned char*)p;
    for (size_t i = 0; i < n && ok;) {
        uint64_t u = packGetVF(f, ok);
        if (!ok) return;
        if (u == 0) {
            uint64_t run = packGetVF(f, ok) + 1;
            if (!ok || run > n - i) { ok = false; return; }
            memset(c + i * 4, 0, (size_t)run * 4); i += (size_t)run;
        } else { if (u > 0xffffffffULL) { ok = false; return; } int32_t a = packUnzig((uint32_t)u); memcpy(c + i * 4, &a, 4); i++; }
    }
    if (sz % 4 && ok && fread(c + n * 4, 1, sz % 4, f) != sz % 4) ok = false;
}

struct Writer {
    FILE* f;
    bool packed = false;
    template <class T> void pod(const T& v) { static_assert(std::is_trivially_copyable<T>::value, "pod"); if (packed) packBytesWF(f, &v, sizeof(T)); else fwrite(&v, sizeof(T), 1, f); }
    template <size_t N> void pod(const char (&a)[N]) { if (packed) packCharsWF(f, a, N); else fwrite(a, 1, N, f); }
    void count(unsigned n) { if (packed) packPutVF(f, n); else fwrite(&n, sizeof n, 1, f); }
    void str(const std::string& s) { unsigned n = (unsigned)s.size(); count(n); fwrite(s.data(), 1, n, f); }
    template <class T> void vpod(const std::vector<T>& v) { unsigned n = (unsigned)v.size(); count(n); if (packed) { for (auto& x : v) packBytesWF(f, &x, sizeof(T)); } else if (n) fwrite(v.data(), sizeof(T), n, f); }
    void vpod(const std::vector<uint64_t>& v) { if (packed) packLedgerWF(f, v); else { unsigned n = (unsigned)v.size(); count(n); if (n) fwrite(v.data(), 8, n, f); } }
    void vstr(const std::vector<std::string>& v) { unsigned n = (unsigned)v.size(); count(n); for (auto& s : v) str(s); }
    void vvi(const std::vector<std::vector<int>>& v) { unsigned n = (unsigned)v.size(); count(n); for (auto& x : v) vpod(x); }
};

struct Reader {
    FILE* f;
    bool ok = true;
    bool packed = false;
    template <class T> void pod(T& v) { if (packed) packBytesRF(f, &v, sizeof(T), ok); else if (fread(&v, sizeof(T), 1, f) != 1) ok = false; }
    template <size_t N> void pod(char (&a)[N]) { if (packed) packCharsRF(f, a, N, ok); else if (fread(a, 1, N, f) != N) ok = false; }
    unsigned count() { if (packed) { uint64_t n = packGetVF(f, ok); if (n > 0xffffffffULL) ok = false; return (unsigned)n; } unsigned n = 0; pod(n); return n; }
    void str(std::string& s) { unsigned n = count(); if (!ok || n > 100000000) { ok = false; return; } s.resize(n); if (n && fread(&s[0], 1, n, f) != n) ok = false; }
    template <class T> void vpod(std::vector<T>& v) { unsigned n = count(); if (!ok || n > 100000000) { ok = false; return; } v.resize(n); if (packed) { for (unsigned i = 0; i < n && ok; i++) packBytesRF(f, &v[i], sizeof(T), ok); } else if (n && fread(v.data(), sizeof(T), n, f) != n) ok = false; }
    void vpod(std::vector<uint64_t>& v) { if (packed) { packLedgerRF(f, v, 50000000, ok); return; } unsigned n = count(); if (!ok || n > 100000000) { ok = false; return; } v.resize(n); if (n && fread(v.data(), 8, n, f) != n) ok = false; }
    void vstr(std::vector<std::string>& v) { unsigned n = count(); if (!ok || n > 10000000) { ok = false; return; } v.resize(n); for (auto& s : v) str(s); }
    void vvi(std::vector<std::vector<int>>& v) { unsigned n = count(); if (!ok || n > 10000000) { ok = false; return; } v.resize(n); for (auto& x : v) vpod(x); }
};
