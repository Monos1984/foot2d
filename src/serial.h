// Sérialisation binaire simple
#pragma once
#include <cstdio>
#include <string>
#include <vector>
#include <type_traits>

struct Writer {
    FILE* f;
    template <class T> void pod(const T& v) { static_assert(std::is_trivially_copyable<T>::value, "pod"); fwrite(&v, sizeof(T), 1, f); }
    void str(const std::string& s) { unsigned n = (unsigned)s.size(); pod(n); fwrite(s.data(), 1, n, f); }
    template <class T> void vpod(const std::vector<T>& v) { unsigned n = (unsigned)v.size(); pod(n); if (n) fwrite(v.data(), sizeof(T), n, f); }
    void vstr(const std::vector<std::string>& v) { unsigned n = (unsigned)v.size(); pod(n); for (auto& s : v) str(s); }
    void vvi(const std::vector<std::vector<int>>& v) { unsigned n = (unsigned)v.size(); pod(n); for (auto& x : v) vpod(x); }
};

struct Reader {
    FILE* f;
    bool ok = true;
    template <class T> void pod(T& v) { if (fread(&v, sizeof(T), 1, f) != 1) ok = false; }
    void str(std::string& s) { unsigned n = 0; pod(n); if (!ok || n > 100000000) { ok = false; return; } s.resize(n); if (n && fread(&s[0], 1, n, f) != n) ok = false; }
    template <class T> void vpod(std::vector<T>& v) { unsigned n = 0; pod(n); if (!ok || n > 100000000) { ok = false; return; } v.resize(n); if (n && fread(v.data(), sizeof(T), n, f) != n) ok = false; }
    void vstr(std::vector<std::string>& v) { unsigned n = 0; pod(n); if (!ok || n > 10000000) { ok = false; return; } v.resize(n); for (auto& s : v) str(s); }
    void vvi(std::vector<std::vector<int>>& v) { unsigned n = 0; pod(n); if (!ok || n > 10000000) { ok = false; return; } v.resize(n); for (auto& x : v) vpod(x); }
};
