// Sérialisation compacte explicite (modules Musée, Supporters, Personnalité, postes) : toujours en mode packed,
// quel que soit le mode du Writer / Reader sous-jacent (voir serial.h).
#pragma once
#include "serial.h"

struct PackW {
    Writer& w;
    template <class T> void pod(const T& v) { static_assert(std::is_trivially_copyable<T>::value, "pod"); packBytesWF(w.f, &v, sizeof(T)); }
    template <size_t N> void pod(const char (&a)[N]) { packCharsWF(w.f, a, N); }
    void vpod(const std::vector<uint64_t>& v) { packLedgerWF(w.f, v); }
    void count(size_t n) { packPutVF(w.f, n); }
    template <class T> void vpod(const std::vector<T>& v) { count(v.size()); for (auto& x : v) pod(x); }
    void str(const std::string& s) { count(s.size()); if (!s.empty()) fwrite(s.data(), 1, s.size(), w.f); }
};
struct PackR {
    Reader& r;
    bool& ok;
    explicit PackR(Reader& rr) : r(rr), ok(rr.ok) {}
    template <class T> void pod(T& v) { static_assert(std::is_trivially_copyable<T>::value, "pod"); packBytesRF(r.f, &v, sizeof(T), r.ok); }
    template <size_t N> void pod(char (&a)[N]) { packCharsRF(r.f, a, N, r.ok); }
    void vpod(std::vector<uint64_t>& v, size_t maxN = 3000000) { packLedgerRF(r.f, v, maxN, r.ok); }
    size_t count(size_t maxN) { uint64_t n = packGetVF(r.f, r.ok); if (!r.ok || n > maxN) { r.ok = false; return 0; } return (size_t)n; }
    template <class T> void vpod(std::vector<T>& v, size_t maxN = 2000000) { size_t n = count(maxN); v.clear(); if (!r.ok) return; v.resize(n); for (size_t i = 0; i < n && r.ok; i++) pod(v[i]); }
    void str(std::string& s, size_t maxN = 1000000) { size_t n = count(maxN); s.clear(); if (!r.ok || !n) return; s.resize(n); if (fread(&s[0], 1, n, r.f) != n) r.ok = false; }
};

// écart à une valeur de référence (OU exclusif octet par octet) : les champs restés à leur valeur par défaut deviennent des zéros
template <class T> inline void packDeltaW(FILE* f, const T& v, const T& def) {
    unsigned char a[sizeof(T)], b[sizeof(T)]; memcpy(a, &v, sizeof(T)); memcpy(b, &def, sizeof(T));
    for (size_t i = 0; i < sizeof(T); i++) a[i] ^= b[i];
    packBytesWF(f, a, sizeof(T));
}
template <class T> inline void packDeltaR(FILE* f, T& v, const T& def, bool& ok) {
    unsigned char a[sizeof(T)], b[sizeof(T)]; packBytesRF(f, a, sizeof(T), ok); memcpy(b, &def, sizeof(T));
    for (size_t i = 0; i < sizeof(T); i++) a[i] ^= b[i];
    memcpy(&v, a, sizeof(T));
}
