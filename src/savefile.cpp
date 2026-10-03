// Conteneur des sauvegardes (version 35) : compression LZ rapide par blocs, somme de contrôle CRC32,
// écriture atomique (fichier temporaire vérifié, puis l'ancienne sauvegarde devient .bak).
//
// Format :
//   u32 magic 'SSWZ'   u32 version du conteneur (1)   u32 drapeaux (bit 0 : compressé)
//   u64 taille brute   u32 CRC32 des données brutes   u32 nombre de blocs
//   pour chaque bloc : u32 taille brute, u32 taille compressée, octets compressés
// Les données brutes sont la sauvegarde habituelle (Career::saveRaw), lue telle quelle par Career::loadRaw.
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>

static const uint32_t SSW_MAGIC = 0x5A575353;    // « SSWZ »
static const uint32_t SSW_VERSION = 2;   // 2 : compression LZ rapide
static const size_t SSW_BLOCK = 16u << 20;       // 16 Mo par bloc

// ---- compression LZ rapide (format de bloc de type LZ4) : jetons littéraux + correspondances (décalage 16 bits)
static size_t lzBound(size_t n) { return n + n / 255 + 16; }
static size_t lzCompress(const unsigned char* src, size_t n, unsigned char* dst) {
    // chaînes de hachage (8 candidats au plus), fenêtre de 64 Ko, une étape d'évaluation paresseuse
    const int HB = 16; const size_t WIN = 65536; const int DEPTH = 8;
    std::vector<int32_t> head(1u << HB, -1), chain(WIN, -1);
    size_t ip = 0, anchor = 0, op = 0;
    auto hash = [&](size_t p) { uint32_t v; memcpy(&v, src + p, 4); return (v * 2654435761u) >> (32 - HB); };
    auto insert = [&](size_t p) { uint32_t h = hash(p); chain[p & (WIN - 1)] = head[h]; head[h] = (int32_t)p; };
    auto putLen = [&](size_t len) { while (len >= 255) { dst[op++] = 255; len -= 255; } dst[op++] = (unsigned char)len; };
    auto best = [&](size_t p, size_t& off) {
        size_t bl = 0; int32_t c = head[hash(p)]; int d = 0;
        size_t maxL = n - 5 - p;
        while (c >= 0 && d++ < DEPTH && p - (size_t)c <= 65535) {
            if ((size_t)c < p && src[c + bl] == src[p + bl] && memcmp(src + c, src + p, 4) == 0) {
                size_t l = 4; while (l < maxL && src[c + l] == src[p + l]) l++;
                if (l > bl) { bl = l; off = p - c; if (l >= 255) break; }
            }
            int32_t nx = chain[c & (WIN - 1)];
            if (nx >= c) break;
            c = nx;
        }
        return bl;
    };
    if (n >= 13) {
        size_t limit = n - 12;
        while (ip < limit) {
            size_t off = 0, ml = best(ip, off);
            if (ml >= 4 && ip + 1 < limit) {      // paresseux : une meilleure correspondance commence-t-elle juste après ?
                insert(ip);
                size_t off2 = 0, ml2 = best(ip + 1, off2);
                if (ml2 > ml + 1) { ip++; ml = ml2; off = off2; }
            }
            if (ml >= 4) {
                size_t ll = ip - anchor;
                unsigned char* tok = dst + op++;
                *tok = (unsigned char)((ll >= 15 ? 15 : ll) << 4);
                if (ll >= 15) putLen(ll - 15);
                memcpy(dst + op, src + anchor, ll); op += ll;
                dst[op++] = (unsigned char)(off & 255); dst[op++] = (unsigned char)(off >> 8);
                size_t m = ml - 4; *tok |= (unsigned char)(m >= 15 ? 15 : m); if (m >= 15) putLen(m - 15);
                size_t end = ip + ml;
                for (size_t q = ip + 1; q < end && q < limit; q++) insert(q);
                ip = end; anchor = ip;
            } else { insert(ip); ip++; }
        }
    }
    size_t ll = n - anchor;                     // derniers littéraux
    dst[op++] = (unsigned char)((ll >= 15 ? 15 : ll) << 4);
    if (ll >= 15) putLen(ll - 15);
    memcpy(dst + op, src + anchor, ll); op += ll;
    return op;
}
static bool lzDecompress(const unsigned char* src, size_t n, unsigned char* dst, size_t outN) {
    size_t ip = 0, op = 0;
    while (ip < n) {
        unsigned tok = src[ip++];
        size_t ll = tok >> 4;
        if (ll == 15) { unsigned c; do { if (ip >= n) return false; c = src[ip++]; ll += c; } while (c == 255); }
        if (ip + ll > n || op + ll > outN) return false;
        memcpy(dst + op, src + ip, ll); ip += ll; op += ll;
        if (ip >= n) break;                     // fin : derniers littéraux
        if (ip + 2 > n) return false;
        size_t off = src[ip] | (src[ip + 1] << 8); ip += 2;
        if (off == 0 || off > op) return false;
        size_t ml = (tok & 15);
        if (ml == 15) { unsigned c; do { if (ip >= n) return false; c = src[ip++]; ml += c; } while (c == 255); }
        ml += 4;
        if (op + ml > outN) return false;
        for (size_t k = 0; k < ml; k++) { dst[op] = dst[op - off]; op++; }   // recouvrement possible
    }
    return op == outN;
}

uint32_t saveCrc32(const unsigned char* d, size_t n, uint32_t crc = 0) {
    static uint32_t T[256]; static bool init = false;
    if (!init) { for (uint32_t i = 0; i < 256; i++) { uint32_t c = i; for (int k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1; T[i] = c; } init = true; }
    crc = ~crc;
    for (size_t i = 0; i < n; i++) crc = T[(crc ^ d[i]) & 0xff] ^ (crc >> 8);
    return ~crc;
}
static bool readAll(const char* path, std::vector<unsigned char>& out) {
    FILE* f = fopen(path, "rb"); if (!f) return false;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    if (n < 0) { fclose(f); return false; }
    out.resize((size_t)n);
    bool ok = n == 0 || fread(out.data(), 1, (size_t)n, f) == (size_t)n;
    fclose(f); return ok;
}
double g_saveTimes[4];   // compression, écriture, vérification, total (secondes) : affiché par le profil

// compresse rawPath vers finalPath de façon atomique ; supprime rawPath
bool saveContainerWrite(const char* rawPath, const char* finalPath, bool compress) {
    auto t0 = std::chrono::steady_clock::now();
    std::vector<unsigned char> raw;
    if (!readAll(rawPath, raw)) return false;
    uint32_t crc = saveCrc32(raw.data(), raw.size());
    std::string tmp = std::string(finalPath) + ".tmp";
    FILE* f = fopen(tmp.c_str(), "wb");
    if (!f) return false;
    uint32_t nblocks = (uint32_t)((raw.size() + SSW_BLOCK - 1) / SSW_BLOCK);
    uint32_t flags = compress ? 1u : 0u; uint64_t rawSize = raw.size();
    fwrite(&SSW_MAGIC, 4, 1, f); fwrite(&SSW_VERSION, 4, 1, f); fwrite(&flags, 4, 1, f);
    fwrite(&rawSize, 8, 1, f); fwrite(&crc, 4, 1, f); fwrite(&nblocks, 4, 1, f);
    double tc = 0;
    bool ok = true;
    for (uint32_t b = 0; b < nblocks && ok; b++) {
        size_t off = (size_t)b * SSW_BLOCK, n = std::min(SSW_BLOCK, raw.size() - off);
        uint32_t rn = (uint32_t)n, cn = 0;
        if (compress) {
            auto c0 = std::chrono::steady_clock::now();
            std::vector<unsigned char> c(lzBound(n));
            cn = (uint32_t)lzCompress(raw.data() + off, n, c.data());
            tc += std::chrono::duration<double>(std::chrono::steady_clock::now() - c0).count();
            fwrite(&rn, 4, 1, f); fwrite(&cn, 4, 1, f); ok = fwrite(c.data(), 1, cn, f) == cn;
        } else { cn = rn; fwrite(&rn, 4, 1, f); fwrite(&cn, 4, 1, f); ok = fwrite(raw.data() + off, 1, n, f) == n; }
    }
    ok = ok && fflush(f) == 0;
    ok = (fclose(f) == 0) && ok;
    auto t1 = std::chrono::steady_clock::now();
    // vérification : en-tête relu et taille attendue
    if (ok) {
        FILE* v = fopen(tmp.c_str(), "rb");
        uint32_t m = 0; uint64_t rs = 0;
        ok = v && fread(&m, 4, 1, v) == 1 && m == SSW_MAGIC && fseek(v, 12, SEEK_SET) == 0 && fread(&rs, 8, 1, v) == 1 && rs == rawSize;
        if (v) fclose(v);
    }
    if (!ok) { remove(tmp.c_str()); return false; }
    // l'ancienne sauvegarde devient .bak, la nouvelle prend sa place
    std::string bak = std::string(finalPath) + ".bak";
    FILE* old = fopen(finalPath, "rb");
    if (old) { fclose(old); remove(bak.c_str()); rename(finalPath, bak.c_str()); }
    if (rename(tmp.c_str(), finalPath) != 0) { rename(bak.c_str(), finalPath); remove(tmp.c_str()); return false; }
    remove(rawPath);
    auto t2 = std::chrono::steady_clock::now();
    g_saveTimes[0] = tc; g_saveTimes[1] = std::chrono::duration<double>(t1 - t0).count() - tc; g_saveTimes[2] = std::chrono::duration<double>(t2 - t1).count();
    g_saveTimes[3] = std::chrono::duration<double>(t2 - t0).count();
    return true;
}

// 0 : ancien format brut (lire path directement) ; 1 : décompressé dans rawOut ; -1 : fichier corrompu ou illisible
int saveContainerOpen(const char* path, const std::string& rawOut) {
    FILE* f = fopen(path, "rb");
    if (!f) return -1;
    uint32_t m = 0;
    if (fread(&m, 4, 1, f) != 1) { fclose(f); return -1; }
    if (m != SSW_MAGIC) { fclose(f); return 0; }
    uint32_t ver = 0, flags = 0, crc = 0, nb = 0; uint64_t rawSize = 0;
    bool ok = fread(&ver, 4, 1, f) == 1 && fread(&flags, 4, 1, f) == 1 && fread(&rawSize, 8, 1, f) == 1 && fread(&crc, 4, 1, f) == 1 && fread(&nb, 4, 1, f) == 1;
    if (!ok || ver != SSW_VERSION || rawSize > (4ull << 30) || nb > 4096 || (uint64_t)nb * SSW_BLOCK < rawSize) { fclose(f); return -1; }
    std::vector<unsigned char> raw; raw.reserve((size_t)rawSize);
    std::vector<unsigned char> comp;
    for (uint32_t b = 0; b < nb && ok; b++) {
        uint32_t rn = 0, cn = 0;
        if (fread(&rn, 4, 1, f) != 1 || fread(&cn, 4, 1, f) != 1 || rn > SSW_BLOCK || cn > lzBound(SSW_BLOCK)) { ok = false; break; }
        comp.resize(cn);
        if (cn && fread(comp.data(), 1, cn, f) != cn) { ok = false; break; }
        if (flags & 1) {
            size_t at = raw.size(); raw.resize(at + rn);
            if (!lzDecompress(comp.data(), cn, raw.data() + at, rn)) { ok = false; break; }
        } else { if (cn != rn) { ok = false; break; } raw.insert(raw.end(), comp.begin(), comp.end()); }
    }
    fclose(f);
    if (!ok || raw.size() != rawSize || saveCrc32(raw.data(), raw.size()) != crc) return -1;
    FILE* o = fopen(rawOut.c_str(), "wb");
    if (!o) return -1;
    ok = raw.empty() || fwrite(raw.data(), 1, raw.size(), o) == raw.size();
    ok = (fclose(o) == 0) && ok;
    return ok ? 1 : -1;
}
