// Journal de débogage : garde les dernières étapes du jeu et les écrit dans debug.log si l'application plante
#include "crashlog.h"
#include "raylib.h"
#include <csignal>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>

namespace {
const int N = 128, L = 200;
char g_ring[N][L];
int g_head = 0, g_count = 0;
volatile sig_atomic_t g_inCrash = 0;

void push(const char* s) {
    double t = GetTime();
    snprintf(g_ring[g_head], L, "[%9.2f] %s", t, s);
    g_head = (g_head + 1) % N;
    if (g_count < N) g_count++;
}

void traceCb(int level, const char* text, va_list args) {
    if (level < LOG_WARNING) return;             // seulement avertissements et erreurs de raylib
    char b[L - 20];
    vsnprintf(b, sizeof b, text, args);
    char m[L];
    snprintf(m, sizeof m, "raylib %s : %s", level == LOG_WARNING ? "WARN" : level == LOG_ERROR ? "ERREUR" : "FATAL", b);
    push(m);
    if (level >= LOG_ERROR) fprintf(stderr, "%s\n", m);
}

const char* sigName(int s) {
    switch (s) {
    case SIGSEGV: return "SIGSEGV (accès mémoire invalide)";
    case SIGABRT: return "SIGABRT (arrêt anormal, assertion)";
    case SIGFPE: return "SIGFPE (division par zéro / erreur arithmétique)";
    case SIGILL: return "SIGILL (instruction invalide)";
    default: return "signal inconnu";
    }
}

void onSignal(int s) {
    if (g_inCrash) _Exit(3);
    g_inCrash = 1;
    crashLogWrite(sigName(s));
    signal(s, SIG_DFL);
    raise(s);
}

void onTerminate() {
    const char* what = "exception C++ non interceptée";
    static char buf[L];
    if (auto e = std::current_exception()) {
        try { std::rethrow_exception(e); }
        catch (const std::exception& x) { snprintf(buf, sizeof buf, "exception C++ : %s", x.what()); what = buf; }
        catch (...) {}
    }
    crashLogWrite(what);
    std::abort();
}
}

void crashLogInit() {
    SetTraceLogCallback(traceCb);
    signal(SIGSEGV, onSignal);
    signal(SIGABRT, onSignal);
    signal(SIGFPE, onSignal);
    signal(SIGILL, onSignal);
    std::set_terminate(onTerminate);
    crashMark("démarrage");
}

void crashMark(const char* fmt, ...) {
    char b[L - 20];
    va_list a; va_start(a, fmt); vsnprintf(b, sizeof b, fmt, a); va_end(a);
    push(b);
}

void crashLogWrite(const char* reason) {
    FILE* f = fopen("debug.log", "w");
    if (!f) return;
    time_t now = time(nullptr);
    char d[64]; strftime(d, sizeof d, "%Y-%m-%d %H:%M:%S", localtime(&now));
    fprintf(f, "SUPER SOCCER WORLD - journal de plantage\n");
    fprintf(f, "Date : %s\nCause : %s\n\n", d, reason);
    fprintf(f, "Dernières étapes (de la plus ancienne à la plus récente) :\n");
    int start = (g_head - g_count + N) % N;
    for (int i = 0; i < g_count; i++) fprintf(f, "  %s\n", g_ring[(start + i) % N]);
    fprintf(f, "\nMerci d'envoyer ce fichier avec une description de ce que vous faisiez.\n");
    fclose(f);
}
