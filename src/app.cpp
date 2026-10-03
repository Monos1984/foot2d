// Interface : menus, sélection d'équipes, carrière, classements, déroulement des matchs
#include "render.h"
#include "input.h"
#include "audio.h"
#include "game.h"
#include "match.h"
#include "crashlog.h"
#include "rumble.h"
#include <memory>
#include <functional>
#include <map>
#include <set>
#include <cstring>
#include <sys/stat.h>


enum Screen { SC_MAIN = 0, SC_PICK, SC_SETUP, SC_MATCH, SC_POST, SC_INTL, SC_HUB, SC_COMPS, SC_COMPVIEW, SC_FIXTURES,
              SC_SQUAD, SC_HISTORY, SC_OPTIONS, SC_SLOTS, SC_SEASONEND, SC_HELP, SC_QUIT, SC_FICHE, SC_EDITMENU, SC_CLUBEDIT,
              SC_CUSTOM, SC_COEFF, SC_CONTROLS, SC_DEPTPICK, SC_MATCHINFO, SC_MARKET, SC_FINANCE, SC_NEWS, SC_JOBS, SC_CAREEROPT, SC_STATUS, SC_STADIUM, SC_CUSTOMLIST, SC_STAFF, SC_RESERVES, SC_FRIENDLIES, SC_REFEREES, SC_CLUBMENU, SC_TVINTRO, SC_ARTICLE, SC_TRAINMODE, SC_TRAINING, SC_DRAW, SC_TROPHIES, SC_STUDIO, SC_ABOUT, SC_SPONSORS, SC_MANAGERS, SC_LEAGUEMODE, SC_HALFTIME, SC_OFFERS, SC_ACADEMY, SC_PLAYER, SC_PLAYEREDIT, SC_TACTICS, SC_EDITDB, SC_MATCHDAY, SC_CALLUP, SC_HOSTS, SC_COACHLOG, SC_COACHJOBS, SC_ARCHIVE, SC_KITS, SC_SPLASH, SC_LIFENEW, SC_LIFE, SC_BRIBE, SC_SEASONSTART, SC_COMPARCH, SC_LEGENDS, SC_ANTHEMS, SC_COMPEDIT, SC_BALLON, SC_SEASONAWARDS, SC_TEAMPLANS, SC_POLESCOUT, SC_MUSEUM, SC_DIRECTOR, SC_SPORTNEW, SC_SPORTING, SC_SUPPORTERS, SC_SUPPORTPARTY, SC_LOCKER, SC_WALKMUSIC, SC_TVADS, SC_ADSEDIT, SC_PRESS, SC_MEDIA, SC_CUPNEWS, SC_SOUNDTEST };
static void openPlayer(int team, int idx, Screen back);
static bool g_lifePick = false;            // choix du club pour une carrière de joueur
static int g_lifeTab = 0;
static void lifeStartCareer(int club);
static Player g_lifeNew;                 // joueur / joueuse en création (app_life.inc)
static void lifeCheckPromotion();
static bool requireManager(Screen back);
static int g_caSel = -1;
static void drawClubSeason(int y, bool results);
static void openBallon(bool replay=false);
static void screenBallon();
static void openSeasonAwards(bool replay=false);
static void openTeamPlans();
static void finishNextSeason();
static void screenSeasonAwards();
static void screenTeamPlans();
static void screenPoleScout();

static Screen g_screen = SC_SPLASH;
static void openMuseum(int t,Screen back);
static void openDirector();
static bool g_sdPicking;
static void openSporting(int page);
static void openSportingNew();
static void sdBegin(int club);
static void openSupporters(int club,Screen back);
static void screenSupporters();
static void screenSupporterParty(float dt);
static bool showSupporterParty(Screen back);
static MenuInput IN;
static Controls CTL[NUM_INPUTS];
static std::unique_ptr<Match> g_match;
static bool g_careerActive = false;
static std::string g_toast; static float g_toastT = 0;
bool g_quit = false;
static const char* TIPEEE_URL = "https://fr.tipeee.com/le-bazar-de-monos";

// contexte du match en cours
struct MatchCtx { bool career = false; int comp = -1, match = -1; Screen back = SC_MAIN; };
static MatchCtx g_mctx;
static MatchSetup g_setup;
static int g_studioPhase = 0; static float g_studioT = 0; static bool g_htStudio = false, g_ftStudio = false;
static PendingMatch g_pending;
static bool g_needAdvance = true;
static std::string g_hubNotice;

static bool g_slotSave = true;
static int g_stTab = 0;
static void toast(const std::string& s) { g_toast = s; g_toastT = 2.5f; }

// ------------------------------------------------------------------ primitives graphiques
static const Color C_BG = { 20, 34, 64, 255 }, C_BG2 = { 26, 44, 82, 255 }, C_ITEM = { 44, 72, 124, 255 },
                   C_SEL = { 240, 200, 60, 255 }, C_TXT = { 235, 240, 255, 255 }, C_DIM = { 150, 170, 210, 255 },
                   C_HI = { 255, 225, 90, 255 }, C_GOOD = { 110, 230, 120, 255 }, C_BAD = { 255, 110, 100, 255 };

// boutons à la manette : Y fait passer d'un bouton à l'autre (dans l'ordre d'affichage), X active le bouton choisi
static int g_btnFocus = -1, g_btnCount = 0, g_btnCountPrev = 0;
static bool g_btnPress = false;
// bouton cliquable ; renvoie vrai si cliqué (ou activé à la manette)
static bool button(int x, int y, int w, int h, const std::string& label, bool selected = false, bool enabled = true) {
    int idx = g_btnCount++;
    bool focus = idx == g_btnFocus;
    if (focus) {
        DrawRectangleLines(x - 2, y - 2, w + 4, h + 4, C_SEL);
        DrawRectangleLines(x - 1, y - 1, w + 2, h + 2, Color{ 255, 255, 255, 160 });
    }
    if (focus && g_btnPress && enabled) { g_btnPress = false; audioPlay(SFX_UI_OK); return true; }
    bool hover = enabled && IN.mouse.x >= x && IN.mouse.x < x + w && IN.mouse.y >= y && IN.mouse.y < y + h;
    Color bg = !enabled ? Color{ 40, 50, 70, 255 } : selected ? C_SEL : hover ? Color{ 70, 104, 170, 255 } : C_ITEM;
    auto lift = [](Color c, int d) { return Color{ (unsigned char)std::max(0, std::min(255, c.r + d)), (unsigned char)std::max(0, std::min(255, c.g + d)), (unsigned char)std::max(0, std::min(255, c.b + d)), c.a }; };
    DrawRectangleGradientV(x, y, w, h, lift(bg, 22), lift(bg, -18));
    if (hover && enabled) DrawRectangle(x, y + h, w, 1, Color{ 0, 0, 0, 120 });
    DrawRectangle(x, y, w, 1, Color{ 255, 255, 255, (unsigned char)(selected ? 110 : 45) });     // biseau clair
    DrawRectangle(x, y + h - 1, w, 1, Color{ 0, 0, 0, 90 });
    if (hover && !selected) DrawRectangleLines(x, y, w, h, Color{ 255, 225, 90, 120 });
    drawTextCentered(fitText(label, w - 4, 10), x + w / 2, y + (h - 10) / 2, 10, selected ? BLACK : enabled ? C_TXT : C_DIM, false);
    if (hover && IN.click) audioPlay(SFX_UI_OK);
    return hover && IN.click;
}

static bool g_noBackBtn = false;
// bouton rapide : musique marche / arrêt (aussi touche F9)
static void toggleMusic();
void applyVibration();
static void drawMusicButton(int x, int y) {
    bool on = g_settings.music;
    bool hover = IN.mouse.x >= x && IN.mouse.x < x + 20 && IN.mouse.y >= y && IN.mouse.y < y + 18;
    DrawRectangle(x, y, 20, 18, hover ? Color{ 70, 90, 140, 255 } : Color{ 40, 56, 96, 255 });
    DrawRectangleLines(x, y, 20, 18, Color{ 120, 150, 210, 255 });
    Color c = on ? WHITE : Color{ 140, 150, 170, 255 };
    // note de musique
    DrawCircle(x + 7, y + 13, 2.5f, c); DrawCircle(x + 13, y + 11, 2.5f, c);
    DrawRectangle(x + 9, y + 4, 1, 9, c); DrawRectangle(x + 15, y + 2, 1, 9, c);
    DrawRectangle(x + 9, y + 3, 7, 2, c);
    if (!on) DrawLine(x + 3, y + 16, x + 17, y + 2, Color{ 230, 60, 60, 255 });
    if (hover && IN.click) { toggleMusic(); IN.click = false; }
}
static void drawBackground(const std::string& title) {
    ClearBackground(C_BG);
    DrawRectangleGradientV(0, 26, VW, VH - 26, Color{ 24, 40, 76, 255 }, Color{ 12, 22, 44, 255 });
    {   // lignes diagonales qui défilent lentement, halo lumineux
        int off = (int)(GetTime() * 6) % 16;
        for (int i = -VH; i < VW + 16; i += 16) DrawLine(i + off, VH, i + off + VH, 0, Color{ C_BG2.r, C_BG2.g, C_BG2.b, 150 });
        DrawCircleGradient(VW / 2, 40, 260, Color{ 80, 120, 200, 26 }, Color{ 80, 120, 200, 0 });
        DrawRectangleGradientV(0, VH - 60, VW, 60, Color{ 0, 0, 0, 0 }, Color{ 0, 0, 0, 70 });
    }
    // filigrane : rond central et surface de réparation
    DrawCircleLines(VW - 70, VH - 60, 46, Color{ 255, 255, 255, 10 });
    DrawCircleLines(VW - 70, VH - 60, 45, Color{ 255, 255, 255, 10 });
    DrawRectangleLines(VW - 200, VH - 150, 1, 180, Color{ 255, 255, 255, 8 });
    DrawRectangleGradientV(0, 0, VW, 26, Color{ 18, 30, 58, 255 }, Color{ 8, 14, 30, 255 });
    DrawRectangle(0, 26, VW, 2, C_SEL);
    // liseré tricolore
    DrawRectangle(0, 28, 40, 1, Color{ 0, 85, 164, 255 }); DrawRectangle(40, 28, 40, 1, WHITE); DrawRectangle(80, 28, 40, 1, Color{ 226, 0, 26, 255 });
    drawTextShadow(fitText(title, VW - 134, 20), 10, 4, 20, C_HI);
    drawMusicButton(VW - 116, 4);
    if (g_noBackBtn) {
        DrawRectangle(VW - 88, 6, 80, 14, Color{ 200, 30, 40, 255 });
        drawTextPx("SUPER SOCCER", VW - 86, 8, 10, WHITE);
    } else {
        // bouton « page précédente »
        bool hover = IN.mouse.x >= VW - 92 && IN.mouse.x < VW - 6 && IN.mouse.y >= 4 && IN.mouse.y < 22;
        DrawRectangle(VW - 92, 4, 86, 18, hover ? Color{ 230, 60, 60, 255 } : Color{ 200, 30, 40, 255 });
        DrawTriangle(Vector2{ (float)VW - 76, 8 }, Vector2{ (float)VW - 84, 13 }, Vector2{ (float)VW - 76, 18 }, WHITE);
        drawTextPx("RETOUR", VW - 70, 8, 10, WHITE);
        if (hover && IN.click) { IN.back = true; IN.click = false; }
    }
}

// ------------------------------------------------------------------ boîte de confirmation
struct Confirm { bool active = false; bool backCancel = false; std::string text, yes, no; std::function<void()> onYes, onNo; int sel = 0; };
static Confirm g_confirm;
static float g_confirmT = 0;     // anti-rebond (manette) à l'ouverture
static void askConfirm(const std::string& text, std::function<void()> onYes, const std::string& yes = "Oui", const std::string& no = "Non", std::function<void()> onNo = nullptr) {
    g_confirm = Confirm(); g_confirm.active = true; g_confirm.text = text; g_confirm.onYes = onYes; g_confirm.onNo = onNo; g_confirm.yes = yes; g_confirm.no = no; g_confirm.sel = 1; g_confirmT = 0.35f;
}

static bool anyPad() { for (int p = 0; p < 4; p++) if (IsGamepadAvailable(p)) return true; return false; }
static void drawFooter(const std::string& s) {
    DrawRectangle(0, VH - 14, VW, 14, Color{ 12, 20, 40, 235 });
    DrawRectangle(0, VH - 14, VW, 1, Color{ 240, 200, 60, 90 });
    drawTextPx(s, 6, VH - 12, 10, C_DIM);
    if (g_btnCountPrev > 0 && anyPad()) { std::string h = g_btnFocus >= 0 ? "X : activer  Y : bouton suivant" : "Y : boutons"; drawTextPx(h, VW - 8 - textWidth(h, 10), VH - 12, 10, C_HI); }
}

struct ListW { int cur = 0, top = 0; float hy = -1; };   // hy : position animée de la barre de sélection (en lignes)
static ListW g_statusLW;
static Screen g_optBack = SC_MAIN; static ListW g_optLW;   // retour de l'écran Options (menu principal ou carrière)
static ListW g_ctlLW; static int g_ctlCapture = -1; static Screen g_ctlBack = SC_OPTIONS;   // configuration des commandes (options ou pause)

// liste générique ; renvoie l'index activé (OK / clic) ou -1
static int listRun(ListW& w, int n, int x, int y, int wd, int rows, int rowH, std::function<void(int, int, int, bool)> row, bool active = true) {
    if (n <= 0) { w.cur = 0; w.top = 0; return -1; }
    int act = -1;
    int prevCur = w.cur;
    if (active) {
        if (IN.up) w.cur = (w.cur - 1 + n) % n;
        if (IN.down) w.cur = (w.cur + 1) % n;
        if (IN.wheel != 0) w.top = std::max(0, std::min(std::max(0, n - rows), w.top - (int)IN.wheel * 3));
        // souris
        if (IN.mouse.x >= x && IN.mouse.x < x + wd && IN.mouse.y >= y && IN.mouse.y < y + rows * rowH) {
            int i = w.top + (int)((IN.mouse.y - y) / rowH);
            if (i < n && (IN.mouseMoved || IN.click)) w.cur = i;
            if (i < n && IN.click) act = i;
        }
        if (IN.ok) act = w.cur;
        if (w.cur != prevCur && w.cur < n) audioPlay(SFX_UI_MOVE);
        if (act >= 0) audioPlay(SFX_UI_OK);
    }
    if (w.cur >= n) w.cur = n - 1;
    if ((IN.wheel == 0 || !active) && w.cur >= 0) {
        if (w.cur < w.top) w.top = w.cur;
        if (w.cur >= w.top + rows) w.top = w.cur - rows + 1;
    }
    if (w.top > std::max(0, n - rows)) w.top = std::max(0, n - rows);
    if (w.top < 0) w.top = 0;
    // fond des lignes, puis barre de sélection qui glisse vers la ligne choisie, puis contenu
    for (int i = w.top; i < n && i < w.top + rows; i++) {
        int yy = y + (i - w.top) * rowH;
        Color base = (i % 2) ? C_ITEM : Color{ 38, 62, 108, 255 };
        DrawRectangle(x, yy, wd, rowH - 1, base);
        DrawRectangle(x, yy, wd, 1, Color{ (unsigned char)std::min(255, base.r + 18), (unsigned char)std::min(255, base.g + 18), (unsigned char)std::min(255, base.b + 22), 255 });
    }
    {
        float target = (float)(w.cur - w.top);
        float dtf = std::min(0.05f, GetFrameTime());
        if (w.hy < 0 || std::fabs(w.hy - target) > rows) w.hy = target;
        else w.hy += (target - w.hy) * std::min(1.f, dtf * 22.f);
        if (std::fabs(w.hy - target) < 0.02f) w.hy = target;
        if (target >= 0 && target < rows) {
            int yy = y + (int)std::lround(w.hy * rowH);
            int hh = rowH - 1;
            DrawRectangleGradientH(x, yy, wd, hh, Color{ 255, 214, 80, 255 }, C_SEL);
            // reflet qui balaie la barre
            float tt = (float)GetTime();
            int sx = (int)(std::fmod(tt * 260.f, (float)wd + 160.f)) - 80;
            for (int k = 0; k < 18; k++) {
                int cx = x + sx + k * 2;
                if (cx >= x && cx < x + wd - 1) DrawRectangle(cx, yy + 1, 2, hh - 2, Color{ 255, 255, 255, (unsigned char)(70 - std::abs(k - 9) * 7) });
            }
            DrawRectangle(x, yy, wd, 1, Color{ 255, 255, 255, 140 });
            DrawRectangle(x, yy + hh - 1, wd, 1, Color{ 150, 100, 10, 255 });
            DrawRectangle(x, yy, 3, hh, Color{ 200, 140, 20, 255 });
            DrawRectangle(x + wd - 3, yy, 3, hh, Color{ 200, 140, 20, 255 });
            // curseur ballon qui pulse à gauche
            if (x >= 8 && active) {
                float pz = 0.5f + 0.5f * std::sin(tt * 8.f);
                int cx = x - 6 - (int)(pz * 2), cy = yy + hh / 2;
                DrawTriangle(Vector2{ (float)cx - 3, (float)cy - 4 }, Vector2{ (float)cx - 3, (float)cy + 4 }, Vector2{ (float)cx + 2, (float)cy }, C_HI);
            }
        }
    }
    for (int i = w.top; i < n && i < w.top + rows; i++) {
        int yy = y + (i - w.top) * rowH;
        bool sel = i == w.cur;
        row(i, x, yy, sel);
    }
    if (n > rows) {
        int bh = rows * rowH;
        int th = std::max(8, bh * rows / n);
        int ty = y + (bh - th) * w.top / std::max(1, n - rows);
        DrawRectangle(x + wd + 2, y, 3, bh, Color{ 0, 0, 0, 100 });
        DrawRectangle(x + wd + 2, ty, 3, th, C_DIM);
    }
    return act;
}

static int menuRun(ListW& w, const std::vector<std::string>& items, int y, int wd = 300) {
    int x = VW / 2 - wd / 2;
    return listRun(w, (int)items.size(), x, y, wd, (int)items.size(), 20, [&](int i, int xx, int yy, bool sel) {
        drawTextCentered(fitText(items[i], wd - 12, 10), xx + wd / 2, yy + 5, 10, sel ? Color{ 20, 20, 40, 255 } : C_TXT, false);
    });
}

static std::string sortKey(const std::string& s) {
    std::string o;
    const unsigned char* p = (const unsigned char*)s.c_str();
    static const char* MAP = "AAAAAAACEEEEIIIIDNOOOOOxOUUUUYTsaaaaaaaceeeeiiiidnooooo/ouuuuyty";
    while (*p) {
        unsigned cp; int n = 1;
        if (*p < 0x80) cp = *p; else if ((*p & 0xE0) == 0xC0 && p[1]) { cp = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F); n = 2; } else { cp = '?'; }
        char c = cp >= 0xC0 && cp <= 0xFF ? MAP[cp - 0xC0] : (char)cp;
        o += (char)tolower((unsigned char)c);
        p += n;
    }
    return o;
}

static void sortTeamsByName(std::vector<int>& v) {
    std::stable_sort(v.begin(), v.end(), [](int a, int b) { return sortKey(g_world.teams[a].name) < sortKey(g_world.teams[b].name); });
}

static std::vector<Pyramid>& curPyr() { return g_careerActive && g_career.kind == 0 ? g_career.pyramids : g_basePyramids; }

// ------------------------------------------------------------------ division courte d'une équipe (cache)
static std::vector<std::string> g_lvlCache;
static int g_lvlStamp = -1;
static int g_lvlStampCounter = 0;
static void levelCacheDirty() { g_lvlStampCounter++; }
static std::string tierShort(const Pyramid& P, int tier) {
    const std::string& n = P.tiers[tier].name;
    if (P.country == "FRA") {
        char d = 0; for (char c : n) if (c >= '0' && c <= '9') d = c;
        std::string pre;
        if (n.rfind("Ligue", 0) == 0) pre = "L";
        else if (n.rfind("National", 0) == 0) pre = "N";
        else if (n.find("gional") != std::string::npos) pre = "R";
        else if (n.find("partemental") != std::string::npos) pre = "D";
        if (!pre.empty() && d) return (P.dom >= 0 ? "DOM " : "") + pre + d;
        return n;
    }
    if (P.country == "U19" || P.country == "U17" || P.country == "U15") {
        if (tier == 0 && P.country != "U15") return "Nat. " + P.country;
        char d = 0; for (char c : n) if (c >= '0' && c <= '9') d = c;
        return std::string(n.find("gional") != std::string::npos ? "R" : "D") + (d ? std::string(1, d) : std::string()) + " " + P.country;
    }
    return P.country + fmt(" %d", tier + 1);
}
// titres en capitales sans accent (COMPETITIONS, PALMARES...) : plus lisibles avec la police pixel
static std::string stripUpperAccents(const std::string& in) {
    static const char* MAP[][2] = { { "É", "E" }, { "È", "E" }, { "Ê", "E" }, { "À", "A" }, { "Â", "A" }, { "Î", "I" }, { "Ô", "O" }, { "Û", "U" }, { "Ù", "U" }, { "Ç", "C" } };
    std::string s = in;
    for (auto& m : MAP) { size_t p; while ((p = s.find(m[0])) != std::string::npos) s.replace(p, strlen(m[0]), m[1]); }
    return s;
}
static const std::string& shortLevel(int team) {
    static const std::string empty;
    if (team < 0) return empty;
    int stamp = g_lvlStampCounter * 10000 + (g_careerActive ? g_career.year - 2000 : 0) + (g_careerActive && g_career.kind == CK_CLUB ? 5000 : 0);
    if (stamp != g_lvlStamp || (int)g_lvlCache.size() != (int)g_world.teams.size()) {
        g_lvlStamp = stamp;
        g_lvlCache.assign(g_world.teams.size(), std::string());
        auto& P = curPyr();
        for (auto& Y : P) for (auto& pl : Y.pools) { std::string sl = tierShort(Y, pl.tier); for (int t : pl.clubs) if (t < (int)g_lvlCache.size()) g_lvlCache[t] = sl; }
    }
    return team < (int)g_lvlCache.size() ? g_lvlCache[team] : empty;
}
// nom + division entre parenthèses
static std::string nameLvl(int team) {
    const std::string& l = shortLevel(team);
    return l.empty() ? g_world.teams[team].name : g_world.teams[team].name + " (" + l + ")";
}

static void drawTeamRow(int team, int x, int y, int w, bool sel, const std::string& right = "") {
    const Team& t = g_world.teams[team];
    drawKitIcon(t.home, x + 3, y + 1, 1);
    Color c = sel ? Color{ 20, 20, 40, 255 } : C_TXT;
    drawTextPx(fitText(t.name, w - 70, 10), x + 16, y + 1, 10, c);
    std::string r = right.empty() ? fmt("%d", (int)t.rating) : right;
    drawTextPx(r, x + w - textWidth(r, 10) - 4, y + 1, 10, c);
}

static const char* MONTHS[] = { "Août", "Septembre", "Octobre", "Novembre", "Décembre", "Janvier", "Février", "Mars", "Avril", "Mai", "Juin", "Juillet" };
static std::string dateOf(double t, int year) {
    (void)year;
    int y0 = g_career.season.year;
    if(g_career.kind==CK_CUSTOM && g_career.custom.format>=10 && g_career.custom.format<32) return legendDateText(t,y0);
    if (t < 0) return fmt("%s %d", t < -4.3 ? "juin" : "juillet", y0);
    int mm = (int)(t / 4.345);
    int m = mm % 12, yo = mm / 12;
    return fmt("%s %d", MONTHS[m], y0 + yo + (m >= 5 ? 1 : 0));
}

// ------------------------------------------------------------------ navigation / choix d'équipes
enum NodeKind { K_ROOT, K_NATROOT, K_CONF, K_CLUBROOT, K_PYR, K_TIER, K_TIERREG, K_POOL, K_GROUP, K_EUROPOOL, K_DOMROOT, K_TEAM, K_DONE,
                K_INTLLIST, K_COMP, K_WORLDPOOL, K_CREATE, K_EUROCOUNTRY, K_COACHCAT, K_CLUBCONF, K_WOMENROOT, K_WNATROOT, K_MENROOT, K_MYCLUBS, K_INFO };
enum PickMode { PM_FRIENDLY_HOME, PM_FRIENDLY_AWAY, PM_CAREER, PM_INTL, PM_BROWSE, PM_CUSTOM, PM_EDIT, PM_FICHE, PM_INVITE, PM_TRAIN };
static int g_trainTeam = -1;
static int g_intlFormat = 0;
static int seTab = 0;   // onglet du bilan de fin de saison
static int g_legendEd = -1;              // légendes de la Coupe du monde : édition en cours de sélection
static bool g_leagueModePick = false;   // choix des clubs du mode Championnat
static bool g_euroModePick = false;     // choix des clubs du mode Coupes d'Europe
static bool g_coachModePick = false;    // carrière de sélectionneur : choix de la sélection
static bool g_contPick = false;         // Coupe des Continents : choix des sélections continentales
static int g_coachCatPick = 0;          // catégorie : 0 A, 1 Espoirs / olympique, 2 U19, 3 U17
static void autosave();
static void openCallup(Screen back);
static void coachNextCampaign();
static std::vector<int> g_inviteSel;

struct Node { int kind = K_ROOT; int a = 0, b = 0, c = 0; std::string title; ListW lw; };
struct Item { std::string label, right; int kind; int a = 0, b = 0, c = 0; int team = -1; };

static PickMode g_pickMode;
static std::vector<Node> g_stack;
static int g_friendlyHome = -1;
static std::vector<int> g_intlSel;
static int g_intlType = 0;
static bool g_intlQual = true;
static std::vector<int> g_intlCandidates;
static std::vector<int> g_intlHosts;
static std::vector<int> g_customSel;
static int g_ficheTeam = -1;
static Screen g_ficheBack = SC_MAIN;
static int g_editTeam = -1;          // -1 = création
static bool g_editForCareer = false;

static const char* CONF_NAMES[] = { "UEFA (Europe)", "CONMEBOL (Amérique du Sud)", "CONCACAF (Amérique du Nord et centrale)", "CAF (Afrique)", "AFC (Asie)", "OFC (Océanie)" };

static void openFiche(int team, Screen back);
static void startCareerWith(int team, Screen back);
static void openClubEditor(int team, bool forCareer, Screen back);
static Screen g_editBack = SC_MAIN;

static std::vector<Item> buildItems(const Node& n) {
    std::vector<Item> it;
    auto& P = curPyr();
    auto clubsOf = [&](std::vector<int> v) { sortTeamsByName(v); for (int t : v) { Item x; x.kind = K_TEAM; x.team = t; x.label = g_world.teams[t].name; it.push_back(x); } };
    switch (n.kind) {
    case K_ROOT:
        if (g_pickMode == PM_CUSTOM) {
            Item d{ fmt(">>> VALIDER (%d équipe%s) <<<", (int)g_customSel.size(), g_customSel.size() > 1 ? "s" : ""), "", K_DONE }; it.push_back(d);
            int nc = 0; for (int i = g_world.baseCount; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].custom) nc++;
            Item cc{ "Mes clubs créés", fmt("%d", nc), K_MYCLUBS }; it.push_back(cc);
        }
        it.push_back({ "Sélections nationales", "", K_NATROOT });
        it.push_back({ "Clubs", "", K_CLUBROOT });
        it.push_back({ "Sélections nationales féminines", "", K_WNATROOT });
        break;
    case K_MYCLUBS: {
        std::vector<int> v; for (int i = g_world.baseCount; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].custom) v.push_back(i);
        clubsOf(v);
        if (v.empty()) { Item x{ "Aucun club créé (Éditeur > Créer un nouveau club)", "", K_INFO }; it.push_back(x); }
        break; }
    case K_WNATROOT:
        for (int c = 0; c < NUM_CONFEDS; c++) { Item x{ CONF_NAMES[c], "", K_CONF }; x.a = c; x.b = 1; it.push_back(x); }
        break;
    case K_WOMENROOT:
        for (int i = 0; i < (int)P.size(); i++) if (isWomenPyramid(P[i])) { Item x{ P[i].name, P[i].tiers.empty() ? std::string() : P[i].tiers[0].name, K_PYR }; x.a = i; it.push_back(x); }
        break;
    case K_NATROOT:
        for (int c = 0; c < NUM_CONFEDS; c++) { Item x{ CONF_NAMES[c], "", K_CONF }; x.a = c; int cnt = 0; for (int i = 0; i < NUM_NATIONS; i++) if (NATIONS[i].conf == c) cnt++; x.right = fmt("%d", cnt); it.push_back(x); }
        break;
    case K_CONF: {
        std::vector<int> v; for (int i = 0; i < NUM_NATIONS; i++) if (NATIONS[i].conf == n.a && (!g_coachModePick || nationEligible(i))) v.push_back(n.b == 1 ? youthNationTeam(i, 5) : i);
        if (g_coachModePick) {
            // sélections classées par niveau
            std::stable_sort(v.begin(), v.end(), [](int a, int b) { return g_world.teams[a].rating > g_world.teams[b].rating; });
            for (int k = 0; k < (int)v.size(); k++) { Item x; x.kind = K_TEAM; x.team = v[k]; x.label = fmt("%2d. ", k + 1) + g_world.teams[v[k]].name; x.right = fmt("niveau %d", (int)g_world.teams[v[k]].rating); it.push_back(x); }
            break;
        }
        clubsOf(v); break; }
    case K_INTLLIST: {
        if (g_coachModePick) {
            // carrière de sélectionneur : choix par confédération
            static const char* CATN[4] = { "Sélection A (Coupe du monde, championnat continental)", "Espoirs (Euro Espoirs et tournoi olympique)", "U19 (Euro U19, Europe uniquement)", "U17 (Euro U17, Europe uniquement)" };
            Item d{ g_intlSel.empty() ? std::string(">>> VALIDER (aucune sélection choisie) <<<") : ">>> COMMENCER AVEC : " + g_world.teams[g_intlSel[0]].name + " <<<", "", K_DONE }; it.push_back(d);
            Item ct{ std::string("Catégorie : ") + CATN[g_coachCatPick & 3], "changer", K_COACHCAT }; it.push_back(ct);
            for (int c = 0; c < NUM_CONFEDS; c++) {
                int cnt = 0; for (int i : g_intlCandidates) if (NATIONS[i].conf == c) cnt++;
                if (!cnt) continue;
                Item x{ CONF_NAMES[c], fmt("%d sélections", cnt), K_CONF }; x.a = c; it.push_back(x);
            }
            break;
        }
        Item d{ fmt(">>> VALIDER (%d sélectionnée%s) <<<", (int)g_intlSel.size(), g_intlSel.size() > 1 ? "s" : ""), "", K_DONE }; it.push_back(d);
        std::vector<int> v = g_intlCandidates; clubsOf(v);
        break;
    }
    case K_CLUBROOT: {
        if (g_pickMode == PM_INVITE) { Item d{ fmt(">>> VALIDER (%d club%s invité%s) <<<", (int)g_inviteSel.size(), g_inviteSel.size() > 1 ? "s" : "", g_inviteSel.size() > 1 ? "s" : ""), "", K_DONE }; it.push_back(d); }
        if (g_pickMode == PM_CAREER) it.push_back({ "+ Créer mon propre club (débute en dernière division de son district)", "", K_CREATE });
        it.push_back({ "Football masculin", "France, puis par fédération", K_MENROOT });
        it.push_back({ "Football féminin", g_pickMode == PM_CAREER ? "carrière manager ou joueuse" : "", K_WOMENROOT });
        if (g_pickMode != PM_BROWSE) {
            std::vector<int> cust; for (int i = g_world.baseCount; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].custom) cust.push_back(i);
            if (!cust.empty()) { Item x{ "Clubs créés avec l'éditeur", fmt("%d", (int)cust.size()), K_COMP }; it.push_back(x); }
        }
        break;
    }
    case K_MENROOT: {
        int fr = -1;
        for (int i = 0; i < (int)P.size(); i++) if (P[i].country == "FRA" && P[i].dom < 0) fr = i;
        if (fr >= 0) { Item x{ "France (pyramide complète)", "Ligue 1 au district", K_PYR }; x.a = fr; it.push_back(x); }
        it.push_back({ "France - Outre-mer", "", K_DOMROOT });
        static const char* CFN[NUM_CONFEDS] = { "Europe (UEFA)", "Amérique du Sud (CONMEBOL)", "Amérique du Nord et centrale (CONCACAF)", "Afrique (CAF)", "Asie (AFC)", "Océanie (OFC)" };
        int cnt[NUM_CONFEDS] = {};
        for (int i = 0; i < (int)P.size(); i++) if (P[i].country != "FRA" && P[i].dom < 0) {
            int nat = g_world.nationIndex(P[i].country.c_str());
            if (nat >= 0) cnt[NATIONS[nat].conf]++;
            else if (!isWomenPyramid(P[i]) && g_pickMode != PM_CAREER) { Item x{ P[i].name, "", K_PYR }; x.a = i; it.push_back(x); }     // pyramides de jeunes
        }
        for (int c = 0; c < NUM_CONFEDS; c++) if (cnt[c]) { Item x{ CFN[c], fmt("%d championnats", cnt[c]), K_CLUBCONF }; x.a = c; it.push_back(x); }
        if (g_pickMode != PM_CAREER && g_pickMode != PM_BROWSE) {
            it.push_back({ "Autres clubs européens (par pays)", "", K_EUROPOOL });
            it.push_back({ "Autres clubs du monde", "", K_WORLDPOOL });
        }
        break;
    }
    case K_CLUBCONF: {
        for (int i = 0; i < (int)P.size(); i++) if (P[i].country != "FRA" && P[i].dom < 0) {
            int nat = g_world.nationIndex(P[i].country.c_str());
            if (nat < 0 || NATIONS[nat].conf != n.a) continue;
            Item x{ P[i].name, P[i].tiers.empty() ? std::string() : fitText(P[i].tiers[0].name, 200, 10) + (P[i].tiers.size() > 1 ? fmt(" (+%d)", (int)P[i].tiers.size() - 1) : std::string()), K_PYR }; x.a = i; it.push_back(x);
        }
        std::stable_sort(it.begin(), it.end(), [](const Item& a, const Item& b) { return sortKey(a.label) < sortKey(b.label); });
        break;
    }
    case K_COMP: { std::vector<int> cust; for (int i = g_world.baseCount; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].custom) cust.push_back(i); clubsOf(cust); break; }
    case K_DOMROOT:
        for (int i = 0; i < (int)P.size(); i++) if (P[i].dom >= 0) { Item x{ P[i].name, "", K_PYR }; x.a = i; it.push_back(x); }
        break;
    case K_PYR: {
        const Pyramid& Y = P[n.a];
        for (int t = 0; t < (int)Y.tiers.size(); t++) {
            int cnt = 0, grp = 0; for (auto& pl : Y.pools) if (pl.tier == t) { cnt += (int)pl.clubs.size(); grp += (int)pl.groups.size(); }
            if (!cnt) continue;
            Item x{ Y.tiers[t].name, grp > 1 ? fmt("%d équipes / %d poules", cnt, grp) : fmt("%d équipes", cnt), K_TIER }; x.a = n.a; x.b = t; it.push_back(x);
        }
        break;
    }
    case K_TIER: {
        const Pyramid& Y = P[n.a];
        int scope = Y.tiers[n.b].scope;
        if (scope == SC_DEPT) {
            std::set<int> regs;
            for (auto& pl : Y.pools) if (pl.tier == n.b) regs.insert(districtRegion(pl.key));
            for (int r : regs) { Item x{ sanitize(REGIONS[r].name), "", K_TIERREG }; x.a = n.a; x.b = n.b; x.c = r; it.push_back(x); }
        } else {
            for (int q = 0; q < (int)Y.pools.size(); q++) {
                const Pool& pl = Y.pools[q];
                if (pl.tier != n.b) continue;
                if (scope == SC_NATIONAL || Y.dom >= 0) {
                    if (pl.groups.size() > 1) for (int g = 0; g < (int)pl.groups.size(); g++) { Item x{ poolLabel(Y, pl, g), fmt("%d équipes", (int)pl.groups[g].size()), K_GROUP }; x.a = n.a; x.b = q; x.c = g; it.push_back(x); }
                    else if (g_pickMode == PM_BROWSE) { Item x{ poolLabel(Y, pl, 0), "", K_GROUP }; x.a = n.a; x.b = q; x.c = 0; it.push_back(x); }
                    else clubsOf(pl.clubs);
                } else {
                    std::string lbl = scope == SC_ZONE ? zoneName(pl.key) : sanitize(REGIONS[pl.key].name);
                    Item x{ lbl, fmt("%d équipes", (int)pl.clubs.size()), K_POOL }; x.a = n.a; x.b = q; it.push_back(x);
                }
            }
        }
        break;
    }
    case K_TIERREG: {
        const Pyramid& Y = P[n.a];
        for (int q = 0; q < (int)Y.pools.size(); q++) {
            const Pool& pl = Y.pools[q];
            if (pl.tier != n.b || districtRegion(pl.key) != n.c) continue;
            Item x{ districtFullName(pl.key), fmt("%d équipes", (int)pl.clubs.size()), K_POOL }; x.a = n.a; x.b = q; it.push_back(x);
        }
        break;
    }
    case K_POOL: {
        const Pool& pl = P[n.a].pools[n.b];
        if (pl.groups.size() > 1 || g_pickMode == PM_BROWSE) for (int g = 0; g < (int)pl.groups.size(); g++) { Item x{ poolLabel(P[n.a], pl, g), fmt("%d équipes", (int)pl.groups[g].size()), K_GROUP }; x.a = n.a; x.b = n.b; x.c = g; it.push_back(x); }
        else clubsOf(pl.clubs);
        break;
    }
    case K_GROUP: clubsOf(P[n.a].pools[n.b].groups[n.c]); break;
    case K_EUROPOOL: {
        for (auto& kv : g_world.countryClubs) {
            int nat = g_world.nationIndex(kv.first.c_str());
            if (nat < 0 || NATIONS[nat].conf != UEFA) continue;
            Item x{ g_world.teams[nat].name, fmt("%d clubs", (int)kv.second.size()), K_EUROCOUNTRY }; x.a = nat; it.push_back(x);
        }
        std::stable_sort(it.begin(), it.end(), [](const Item& a, const Item& b) { return sortKey(a.label) < sortKey(b.label); });
        break;
    }
    case K_EUROCOUNTRY: { auto f = g_world.countryClubs.find(NATIONS[n.a].code); if (f != g_world.countryClubs.end()) clubsOf(f->second); break; }
    case K_WORLDPOOL: clubsOf(g_world.worldPool); break;
    }
    return it;
}

static void openPick(PickMode m) {
    g_pickMode = m;
    g_stack.clear();
    Node root;
    if (m == PM_CAREER || m == PM_BROWSE) { root.kind = K_CLUBROOT; root.title = m == PM_CAREER ? "Choisissez votre club" : "Championnats"; }
    else if (m == PM_INTL) { root.kind = K_INTLLIST; root.title = g_contPick ? "Coupe des Continents : choisissez vos sélections (1 à 4)" : g_coachModePick ? "Carrière de sélectionneur : choisissez votre sélection" : g_euroModePick ? "Coupes d'Europe : choisissez vos clubs (1 à 4)" : g_leagueModePick ? "Choisissez vos clubs (1 à 4)" : "Choisissez vos sélections"; }
    else if (m == PM_CUSTOM) { root.kind = K_ROOT; root.title = "Équipes de la compétition"; }
    else if (m == PM_EDIT) { root.kind = K_CLUBROOT; root.title = "Club à modifier"; }
    else if (m == PM_FICHE) { root.kind = K_ROOT; root.title = "Fiches des clubs et sélections"; }
    else if (m == PM_INVITE) { root.kind = K_CLUBROOT; root.title = "Clubs à inviter (1 : match amical, 3 : tournoi)"; }
    else if (m == PM_TRAIN) { root.kind = K_ROOT; root.title = "Équipe à entraîner"; }
    else { root.kind = K_ROOT; root.title = "Équipe à domicile"; }
    g_stack.push_back(root);
    g_screen = SC_PICK;
}

static void startSetup(int home, int away, bool career, int comp, int match);
static void matchContext(MatchSetup& s, bool career, int comp, int match);
static void openCompView(int comp, int stage = -1, int group = -1);
static void openHub();
static int newsUnread();
static void managerCycle(int d);
static std::string managerChoiceName();
static void applyChosenManager();
static Texture2D* sponsorLogoFwd(const std::string& name);
static void drawLogo2(Texture2D* t, int x, int y, int w, int h);
static void startLeagueMode();
static bool collectDraws();
static void primeDraws();

static std::vector<std::string> wrapText(const std::string& s, int maxw);
static void screenPick() {
    Node& n = g_stack.back();
    std::string title = n.title;
    if (g_pickMode == PM_FRIENDLY_AWAY) title = "Équipe à l'extérieur";
    drawBackground(title);
    std::string crumbs;
    for (size_t i = 1; i < g_stack.size(); i++) crumbs += (i > 1 ? " > " : "") + g_stack[i].title;
    drawTextPx(fitText(crumbs, VW - 20, 10), 10, 32, 10, C_DIM);
    auto items = buildItems(n);
    if (g_pickMode == PM_FRIENDLY_AWAY && g_friendlyHome >= 0) drawTextPx("Domicile : " + g_world.teams[g_friendlyHome].name, 10, 44, 10, C_HI);
    const int LW = VW - 40 - 196;      // liste à gauche, fiche rapide à droite
    int sel = listRun(n.lw, (int)items.size(), 20, 58, LW, 19, 14, [&](int i, int x, int y, bool s) {
        const Item& it = items[i];
        if (it.kind == K_TEAM) {
            bool picked = (g_pickMode == PM_INTL && std::find(g_intlSel.begin(), g_intlSel.end(), it.team) != g_intlSel.end()) ||
                          (g_pickMode == PM_CUSTOM && std::find(g_customSel.begin(), g_customSel.end(), it.team) != g_customSel.end()) ||
                          (g_pickMode == PM_INVITE && std::find(g_inviteSel.begin(), g_inviteSel.end(), it.team) != g_inviteSel.end());
            drawTeamRow(it.team, x, y + 1, LW, s);
            if (picked) { DrawRectangle(x + LW - 44, y + 3, 16, 8, C_GOOD); drawTextPx("OK", x + LW - 42, y + 2, 10, BLACK); }
        } else {
            Color c = s ? Color{ 20, 20, 40, 255 } : (it.kind == K_CREATE || it.kind == K_DONE ? C_HI : C_TXT);
            drawTextPx(fitText(it.label, LW - 120, 10), x + 8, y + 2, 10, c);
            if (!it.right.empty()) drawTextPx(fitText(it.right, 110, 10), x + LW - std::min(110, textWidth(it.right, 10)) - 18, y + 2, 10, s ? c : C_DIM);
            if (it.kind != K_DONE && it.kind != K_CREATE && it.kind != K_INFO) drawTextPx(">", x + LW - 10, y + 2, 10, c);
        }
    });
    {   // fiche rapide de l'élément choisi
        int px = 20 + LW + 12, pw = VW - px - 14, py = 58, ph = 19 * 14;
        DrawRectangleGradientV(px, py, pw, ph, Color{ 18, 26, 60, 230 }, Color{ 6, 10, 28, 235 });
        DrawRectangle(px, py, 3, ph, C_HI);
        int cur = n.lw.cur;
        if (cur >= 0 && cur < (int)items.size() && items[cur].kind == K_TEAM) {
            int t = items[cur].team; const Team& T = g_world.teams[t];
            DrawRectangleGradientV(px + 3, py, pw - 3, 60, Color{ (unsigned char)(hexc(T.home.shirt).r / 2), (unsigned char)(hexc(T.home.shirt).g / 2), (unsigned char)(hexc(T.home.shirt).b / 2), 200 }, Color{ 6, 10, 28, 0 });
            drawKitIcon(T.home, px + 12, py + 10, 4);
            drawKitIcon(T.away, px + 52, py + 18, 2);
            int ty = py + 10;
            for (auto& l : wrapText(T.name, pw - 90)) { if (ty > py + 34) break; drawTextPx(l, px + 78, ty, 10, WHITE); ty += 11; }
            drawTextPx(T.kind == TK_NATION ? "Sélection nationale" : T.custom ? "Club créé" : "Club", px + 78, py + 40, 10, T.custom ? C_GOOD : C_DIM);
            int yy = py + 66;
            drawTextPx(fmt("Niveau %d", (int)T.rating), px + 10, yy, 10, C_TXT);
            DrawRectangle(px + 70, yy + 2, pw - 82, 7, Color{ 0, 0, 0, 140 });
            DrawRectangle(px + 70, yy + 2, (int)((pw - 82) * std::min(1.f, T.rating / 100.f)), 7, T.rating >= 80 ? C_GOOD : T.rating >= 65 ? C_HI : C_BAD);
            yy += 14;
            if (!T.stadium.empty()) { drawTextPx(fitText("Stade : " + T.stadium, pw - 20, 10), px + 10, yy, 10, C_DIM); yy += 12; }
            if (!T.town.empty()) { drawTextPx(fitText("Ville : " + T.town, pw - 20, 10), px + 10, yy, 10, C_DIM); yy += 12; }
            if (T.founded > 0) { drawTextPx(fmt("Fondé en %d", T.founded), px + 10, yy, 10, C_DIM); yy += 12; }
            yy += 4;
            g_world.ensureSquad(t);
            if (!T.squad.empty()) {
                drawTextPx("JOUEURS VEDETTES", px + 10, yy, 10, C_HI); yy += 13;
                std::vector<int> ord(T.squad.size()); for (size_t k = 0; k < ord.size(); k++) ord[k] = (int)k;
                std::partial_sort(ord.begin(), ord.begin() + std::min<size_t>(5, ord.size()), ord.end(), [&](int a, int b) { return T.squad[a].overall() > T.squad[b].overall(); });
                for (int k = 0; k < 5 && k < (int)ord.size(); k++) {
                    const Player& P = T.squad[ord[k]];
                    drawTextPx(posName(P.pos), px + 10, yy, 10, C_DIM);
                    drawTextPx(fitText(P.name, pw - 70, 10), px + 30, yy, 10, C_TXT);
                    std::string ov = fmt("%d", P.overall()); drawTextPx(ov, px + pw - 10 - textWidth(ov, 10), yy, 10, C_HI);
                    yy += 11;
                }
            }
        } else if (cur >= 0 && cur < (int)items.size()) {
            const Item& it = items[cur];
            int yy = py + 10;
            for (auto& l : wrapText(it.label, pw - 20)) { if (yy > py + 50) break; drawTextPx(l, px + 10, yy, 10, WHITE); yy += 12; }
            if (!it.right.empty()) { drawTextPx(fitText(it.right, pw - 20, 10), px + 10, yy + 2, 10, C_HI); yy += 14; }
            const char* hint = it.kind == K_DONE ? "Valider la sélection et continuer." : it.kind == K_CREATE ? "Créer votre propre club (nom, couleurs, stade)." : it.kind == K_MYCLUBS ? "Les clubs que vous avez créés dans l'éditeur." : "OK : ouvrir";
            for (auto& l : wrapText(hint, pw - 20)) { drawTextPx(l, px + 10, yy + 6, 10, C_DIM); yy += 11; }
            // ballon qui rebondit pour égayer le panneau
            float b = std::fabs(std::sin((float)GetTime() * 3.f));
            int bx = px + pw / 2, by = py + ph - 20 - (int)(b * 26);
            DrawEllipse(bx, py + ph - 12, 7 - b * 3, 2, Color{ 0, 0, 0, 90 });
            DrawCircle(bx, by, 7, WHITE); DrawCircleLines(bx, by, 7, Color{ 40, 40, 40, 255 });
            DrawPoly(Vector2{ (float)bx, (float)by }, 5, 2.6f, (float)GetTime() * 90, Color{ 30, 30, 30, 255 });
        }
    }
    bool onTeam = n.lw.cur >= 0 && n.lw.cur < (int)items.size() && items[n.lw.cur].kind == K_TEAM;
    if (onTeam) {
        int t = items[n.lw.cur].team;
        std::string lvl = g_world.teams[t].kind == TK_CLUB && g_careerActive && g_career.kind == CK_CLUB ? g_career.teamLevelName(t) : "";
        drawFooter(fmt("%s - note %d  %s   [Tab : fiche]", g_world.teams[t].name.c_str(), (int)g_world.teams[t].rating, lvl.c_str()));
        if (IN.tab) { openFiche(t, SC_PICK); return; }
    } else drawFooter("OK : choisir    Retour : revenir");
    if (IN.back) {
        if (g_stack.size() > 1) g_stack.pop_back();
        else if (g_pickMode == PM_FRIENDLY_AWAY) { g_pickMode = PM_FRIENDLY_HOME; }
        else if (g_pickMode == PM_BROWSE) g_screen = SC_COMPS;
        else if (g_pickMode == PM_CAREER && g_lifePick) g_screen = SC_LIFENEW;
        else if (g_pickMode == PM_INTL && g_legendEd >= 0) { g_screen = SC_LEGENDS; g_legendEd = -1; }
        else if (g_pickMode == PM_INTL) { g_screen = g_euroModePick || g_coachModePick || g_contPick ? SC_MAIN : g_leagueModePick ? SC_LEAGUEMODE : SC_INTL; g_leagueModePick = false; g_euroModePick = false; g_coachModePick = false; g_contPick = false; }
        else if (g_pickMode == PM_CUSTOM) g_screen = SC_CUSTOM;
        else if (g_pickMode == PM_EDIT) g_screen = SC_EDITMENU;
        else if (g_pickMode == PM_INVITE) g_screen = SC_FRIENDLIES;
        else g_screen = SC_MAIN;
        return;
    }
    if (sel < 0) return;
    const Item& it = items[sel];
    if (it.kind == K_CREATE) { openClubEditor(-1, true, SC_PICK); return; }
    if (it.kind == K_INFO) return;
    if (it.kind == K_COACHCAT) { g_coachCatPick = (g_coachCatPick + 1) % 4; return; }
    if (it.kind == K_TEAM) {
        switch (g_pickMode) {
        case PM_FRIENDLY_HOME: g_friendlyHome = it.team; g_pickMode = PM_FRIENDLY_AWAY; break;
        case PM_FRIENDLY_AWAY:
            if (it.team == g_friendlyHome) { toast("Choisissez une autre équipe"); break; }
            if (isWomenTeam(it.team) != isWomenTeam(g_friendlyHome)) { toast(isWomenTeam(g_friendlyHome) ? "Séparation stricte : choisissez un club ou une sélection féminine" : "Séparation stricte : choisissez une équipe masculine"); break; }
            startSetup(g_friendlyHome, it.team, false, -1, -1);
            break;
        case PM_CAREER:
            if(g_sdPicking){sdBegin(it.team); }
            else if (g_lifePick) {
                int t = it.team;
                if (g_lifeNew.gender && !isWomenTeam(t)) { auto f = g_world.womenOf.find(t); if (f == g_world.womenOf.end()) { toast("Une joueuse évolue dans un club féminin (Football féminin)"); break; } t = f->second; }
                if (!g_lifeNew.gender && isWomenTeam(t)) { toast("Un joueur évolue dans un club masculin"); break; }
                lifeStartCareer(t);
            } else startCareerWith(it.team, SC_PICK);
            break;
        case PM_INTL: {
            if (g_coachModePick) { g_intlSel = { it.team }; toast("Sélection choisie : " + g_world.teams[it.team].name); if (g_stack.size() > 1) g_stack.pop_back(); break; }
            auto f = std::find(g_intlSel.begin(), g_intlSel.end(), it.team);
            if (f != g_intlSel.end()) g_intlSel.erase(f);
            else if (g_intlSel.size() < 4) g_intlSel.push_back(it.team);
            else toast("4 sélections maximum");
            break;
        }
        case PM_CUSTOM: {
            auto f = std::find(g_customSel.begin(), g_customSel.end(), it.team);
            if (f != g_customSel.end()) g_customSel.erase(f);
            else if (g_customSel.size() < 64) g_customSel.push_back(it.team);
            else toast("64 équipes maximum");
            break;
        }
        case PM_EDIT: openClubEditor(it.team, false, SC_PICK); break;
        case PM_FICHE: openFiche(it.team, SC_PICK); break;
        case PM_TRAIN: g_trainTeam = it.team; g_screen = SC_TRAINMODE; break;
        case PM_BROWSE: openFiche(it.team, SC_PICK); break;
        case PM_INVITE: {
            if (it.team == g_career.userTeam || g_world.teams[it.team].parent == g_career.userTeam) { toast("Choisissez un autre club"); break; }
            if (isWomenTeam(it.team) != isWomenTeam(g_career.userTeam)) { toast("Séparation stricte hommes / femmes : choisissez un autre club"); break; }
            auto f = std::find(g_inviteSel.begin(), g_inviteSel.end(), it.team);
            if (f != g_inviteSel.end()) g_inviteSel.erase(f);
            else if (g_inviteSel.size() < 3) g_inviteSel.push_back(it.team);
            else toast("3 clubs maximum");
            break;
        }
        default: break;
        }
        return;
    }
    if (it.kind == K_DONE) {
        if (g_pickMode == PM_CUSTOM) { g_screen = SC_CUSTOM; return; }
        if (g_pickMode == PM_INVITE) { g_screen = SC_FRIENDLIES; return; }
        if (g_intlSel.empty()) { toast("Sélectionnez au moins une équipe"); return; }
        if (g_legendEd >= 0) {
            std::vector<int> ctrl = g_intlSel; if (ctrl.size() > 4) ctrl.resize(4);
            int ed = g_legendEd; g_legendEd = -1;
            legendStart(g_career, ed, ctrl);
            g_careerActive = true; g_needAdvance = true;
            g_hubNotice = g_career.custom.name;
            openHub();
            return;
        }
        if (g_euroModePick) {
            std::vector<int> ctrl = g_intlSel; if (ctrl.size() > 4) ctrl.resize(4);
            auto go = [ctrl](int fmt) {
                g_euroModePick = false; g_leagueModePick = false;
                g_career.opts = Career::Opts(); g_career.opts.euroFormat = (uint8_t)fmt; g_career.opts.awayGoals = fmt ? 0 : 1;
                g_career.newEuroCareer(ctrl, 2026);
                g_careerActive = true; g_needAdvance = true;
                g_hubNotice = g_career.opts.euroFormat ? "Coupes d'Europe 2026-27 : Ligue des champions, Ligue Europa et Ligue Conférence" : "Coupes d'Europe 2026-27 : Ligue des champions et Coupe UEFA";
                openHub();
            };
            askConfirm("Quelle formule pour les coupes d'Europe ? Formule 2003 : Ligue des champions (groupes) et Coupe UEFA. Nouvelle formule : C1, C3 et C4 avec phase de ligue à 36 clubs.",
                       [go]() { go(1); }, "Nouvelle formule", "Formule 2003", [go]() { go(0); });
            return;
        }
        if (g_leagueModePick) { startLeagueMode(); return; }
        if (g_contPick) {
            std::vector<int> ctrl = g_intlSel; if (ctrl.size() > 4) ctrl.resize(4);
            g_contPick = false;
            g_career.coach = false; g_career.nlLeague.clear();
            continentsStart(g_career, ctrl);
            g_careerActive = true; g_needAdvance = true;
            g_hubNotice = "Coupe des Continents : groupe unique de 6, matchs aller-retour, puis Final Four";
            openHub();
            return;
        }
        if (g_coachModePick) {
            if (g_coachCatPick >= 2 && NATIONS[g_intlSel[0]].conf != UEFA) { toast("Catégorie U19 / U17 : réservée aux sélections européennes (Euro U19 / U17)"); return; }
            if (!requireManager(SC_PICK)) return;
            g_coachModePick = false;
            applyChosenManager();
            g_career.coachCat = g_coachCatPick;
            g_career.newCoachCareer(g_intlSel[0]);
            g_careerActive = true; g_needAdvance = true;
            g_hubNotice = fmt("Sélectionneur de %s : %s %d", g_world.teams[g_career.coachTeam()].name.c_str(), INTL_NAMES[g_career.intlType], g_career.year);
            openHub(); autosave();
            return;
        }
        g_career.coach = false; g_career.nlLeague.clear();
        g_career.newInternational(g_intlType, g_intlQual, g_intlSel, 0, g_intlHosts, g_intlFormat);
        g_careerActive = true;
        g_needAdvance = true;
        g_hubNotice = fmt("%s %d", INTL_NAMES[g_intlType], g_career.year) + (g_intlQual && g_intlType != IT_COPA ? " - début des qualifications" : " - phase finale");
        openHub();
        return;
    }
    if (g_pickMode == PM_BROWSE && it.kind == K_GROUP) {
        int comp = curPyr()[it.a].pools[it.b].comps.size() > (size_t)it.c ? curPyr()[it.a].pools[it.b].comps[it.c] : -1;
        if (comp >= 0) openCompView(comp, 0, 0);
        return;
    }
    Node nn; nn.kind = it.kind; nn.a = it.a; nn.b = it.b; nn.c = it.c; nn.title = it.label;
    g_stack.push_back(nn);
}

// ------------------------------------------------------------------ préparation du match
static int g_setupRow = 0;
static void startSetup(int home, int away, bool career, int comp, int match) {
    g_setup = MatchSetup();
    g_setup.home = home; g_setup.away = away;
    g_setup.rules=career&&careerRules()?RULESET_CAREER:RULESET_SIMPLE;
    for (int i = 0; i < NUM_INPUTS; i++) g_setup.side[i] = -1;
    g_mctx = MatchCtx(); g_mctx.career = career; g_mctx.comp = comp; g_mctx.match = match;
    g_mctx.back = career ? SC_HUB : SC_MAIN;
    if (career) {
        Season& S = g_career.season;
        Competition& C = S.comps[comp];
        MatchRes& m = C.matches[match];
        g_setup.decisive = m.decisive;
        g_setup.goalAssist = C.format == FMT_UCL2000 || C.format == FMT_UEFA2000 || C.format == FMT_NEWEURO || C.kind == 43;   // arbitres de surface
        g_setup.neutral = m.neutral;
        g_setup.noET = m.noET;
        g_setup.awayGoals = C.awayGoals;
        g_setup.yellowLimit = C.yellowLimit;
        g_setup.referee = refereeFor(comp, match);
        if (m.leg == 1 && m.tie >= 0) {
            int st = C.stageOfMatch(match);
            const Tie& t = C.stages[st].ties[m.tie];
            const MatchRes& a = C.matches[t.m1];
            if (a.played) { g_setup.hasFirstLeg = true; g_setup.aggHome = a.ag; g_setup.aggAway = a.hg; }
        }
        int st = C.stageOfMatch(match);
        std::string rn;
        if (st >= 0) for (auto& R : C.stages[st].rounds) for (int x : R.m) if (x == match) rn = R.name;
        g_setup.title = C.name + (rn.empty() ? "" : " - " + rn);
        bool hc = S.isControlled(home), ac = S.isControlled(away);
        // un seul contrôleur par équipe humaine (manette en priorité) ; les autres restent sur « ordinateur »
        std::vector<int> devs;
        for (int p = 0; p < 4; p++) if (IsGamepadAvailable(p)) devs.push_back(IN_PAD1 + p);
        devs.push_back(IN_KB1); devs.push_back(IN_KB2);
        int k = 0;
        bool fm = g_career.kind == CK_CLUB && g_career.mgr.fullManager && (home == g_career.userTeam || away == g_career.userTeam);
        if (fm) { g_setup.managed = home == g_career.userTeam ? 0 : 1; g_setup.highlights = false; }   // match joué en entier (durée choisie par le joueur), plus de temps forts
        else {
            if (hc) g_setup.side[devs[k++]] = 0;
            if (ac) g_setup.side[devs[k++]] = 1;
        }
        g_setup.delegSubs = g_career.kind == CK_CLUB && g_career.mgr.delegSubs;
        if (g_career.kind == CK_CLUB) {
            const PlayerLife& L = g_career.life;
            // carrière de joueur : on ne contrôle que son joueur, l'entraîneur choisit la composition
            if (L.isPlayer) {
                g_world.teams[g_career.userTeam].xi.clear();
                int t = -1, idx = -1; Player* me = g_career.lifePlayer(&t);
                if (me && (t == home || t == away)) for (int i = 0; i < (int)g_world.teams[t].squad.size(); i++) if (g_world.teams[t].squad[i].id == me->id) idx = i;
                if (idx >= 0) g_setup.lockSquad[t == home ? 0 : 1] = idx;
                else for (int d = 0; d < NUM_INPUTS; d++) g_setup.side[d] = -1;     // pas concerné : on regarde le match
            }
            // valise : l'adversaire est diminué
            if (L.bribeComp == comp && L.bribeMatch == match && L.bribeTeam >= 0) {
                int s = L.bribeTeam == home ? 0 : 1;
                static const float MUL[3] = { 0.93f, 0.97f, 0.85f };
                g_setup.bribeMult[s] = MUL[L.bribeKind % 3];
                if (L.bribeKind == 1) {
                    const Team& O = g_world.teams[L.bribeTeam];
                    int best = -1, bo = -1;
                    for (int i = 0; i < (int)O.squad.size(); i++) if (O.squad[i].pos != POS_GK && O.squad[i].overall() > bo) { bo = O.squad[i].overall(); best = i; }
                    g_setup.bribeSquad[s] = best;
                }
            }
        }
    } else {
        g_setup.title = "Match amical";
        g_setup.side[IsGamepadAvailable(0) ? IN_PAD1 : IN_KB1] = 0;
        g_setup.decisive = false;
    }
    g_setup.halfSeconds = g_settings.halfSeconds();
    g_setup.difficulty = g_settings.difficulty;
    g_setup.pitch = g_settings.pitch;
    g_setup.stadium = g_setup.neutral ? std::string("Terrain neutre") : g_world.teams[home].stadium;
    if (g_setup.neutral && career && g_career.kind == CK_CLUB) {
        const Competition& C0 = g_career.season.comps[comp];
        if (C0.kind == 2 || C0.kind == 6 || C0.kind == 11 || C0.kind == 15) g_setup.stadium = "Stade de France";
        if (C0.kind == 6 && !g_career.tdcVenue.empty() && C0.tag >= 0 && C0.tag < (int)g_career.pyramids.size() && g_career.pyramids[C0.tag].country == "FRA") g_setup.stadium = g_career.tdcVenue;
        if (C0.kind == 3) g_setup.stadium = g_career.uclFinalVenue.empty() ? "Stade de la finale européenne" : g_career.uclFinalVenue;
        if (C0.kind == 8) g_setup.stadium = g_career.uefaFinalVenue.empty() ? "Stade de la finale européenne" : g_career.uefaFinalVenue;
        if (C0.kind == 7) g_setup.stadium = "Stade Louis-II (Monaco)";
        auto cv = g_career.cupVenue.find(comp);
        if (cv != g_career.cupVenue.end() && !cv->second.empty()) g_setup.stadium = cv->second;
    }
    if (career && g_career.kind == CK_INTL) { std::string v = matchVenue(comp, match); if (!v.empty()) g_setup.stadium = v; }
    if (career && g_career.kind == CK_CUSTOM && g_career.season.comps[comp].kind == 50) {       // légendes : stades de l'époque, but en or (1998, 2002)
        std::string v = legendVenue(comp, match); if (!v.empty()) g_setup.stadium = v;
        const Competition& LC = g_career.season.comps[comp];
        if (LC.tag >= 0 && LC.tag < NUM_LEGENDS && legendGoldenGoal(LC.tag) && LC.matches[match].decisive) g_setup.goldenGoal = true;
        g_setup.title = LC.name;
    }
    if(career && g_career.season.comps[comp].kind==150){g_setup.halfMinutes=20;g_setup.noET=true;g_setup.neutral=true;}
    if (career) { int sh, sb; bool rl; g_career.sheetRules(comp, sh, sb, rl); g_setup.benchSize = sh - 11; g_setup.maxSubs = rl ? -1 : sb; g_setup.rolling = rl; }
    ensureStadium(home);
    if (g_world.teams[home].sta.namingIncome > 0 && !g_setup.neutral) g_setup.stadium = g_world.teams[home].sta.sponsor;
    matchContext(g_setup, career, comp, match);
    if (career && g_career.kind == CK_CLUB) {       // finale de la Coupe de France : La Marseillaise et le président de la République
        const Competition& FC = g_career.season.comps[comp];
        int st = FC.stageOfMatch(match);
        std::string rn; if (st >= 0) for (auto& R : FC.stages[st].rounds) for (int x : R.m) if (x == match) rn = R.name;
        bool fr = FC.name.find("Coupe de France") != std::string::npos;
        if ((FC.kind == 2 || FC.kind == 41) && rn == "Finale" && fr) {
            g_setup.anthems = true; g_setup.anthemOnly = "FRA"; g_setup.president = FC.kind == 2;
        }
    }
    g_setupRow = 0;
    g_screen = SC_SETUP;
}

static const char* PITCH_NAMES[] = { "Normal", "Sec", "Humide", "Boueux", "Gelé", "Aléatoire" };
static const char* DIFF_NAMES[] = { "Facile", "Normal", "Difficile" };

static bool g_trophyChecked = false, g_trophyShown = false, g_podiumMusic = false, g_htShown = false;
static void openTvAds(int phase, Screen next);
static void loadCustomAds();
static std::string walkMusicFor(const std::string& comp);
static std::vector<std::string> wrapText(const std::string& s, int maxw);
#include "app_locker.inc"
static int g_matchSerial = 0;
static void launchMatch() {
    g_matchSerial++;
    loadCustomAds();
    MatchSetup s = g_setup;
    if(g_mctx.career&&g_career.sportingMode()){g_career.sportingPrepare(s.home);g_career.sportingPrepare(s.away);for(auto& side:s.side)side=-1;s.managed=-1;s.delegSubs=true;s.highlights=false;}
    s.snes = g_settings.controlStyle == 1;
    s.commentary = g_settings.commentary;
    s.refKit = g_settings.refKit;
    if (s.pitch == 5) { s.meteo = g_rng.range(0, 5); meteoApply(s, false, s.turf); }
    s.halfSeconds = HALF_MINUTES[g_settings.halfIdx] * 60.f;
    s.halftimeScreen = true;
    g_match.reset(new Match());
    g_match->init(s);
    g_screen = SC_MATCH;
    g_htStudio = g_ftStudio = false;
    g_trophyChecked = false; g_trophyShown = false; g_podiumMusic = false; g_htShown = false;
    openLocker(0);
}

// options des matchs amicaux
static int g_frSev = 0, g_frEnd = 0, g_frBench = 7, g_frSubs = 5, g_frStadium = 0, g_frTime = 4, g_frTv = 0;   // g_frTv : 0 selon l'affiche, 1..4 chaîne, 5 sans TV
static const char* FR_CHANNELS[4] = { "FRANCE SPORT", "CANAL FOOT+", "TF SPORT", "FOOT 2 TV" };
static void setFriendlyTv(int chan);
static void crowdContext(MatchSetup& s);
static void applyFriendly(bool friendly, const std::vector<std::string>& stadiums) {
    if (!friendly) return;
    static const int SEVV[] = { -1, 35, 60, 80, 95 };
    static const char* TIMEN[] = { "13h00", "15h00", "17h00", "18h00", "20h45", "21h00" };
    g_setup.sevOverride = SEVV[g_frSev];
    g_setup.decisive = g_frEnd != 0; g_setup.noET = g_frEnd == 3 || g_frEnd == 5; g_setup.goldenGoal = g_frEnd == 2; g_setup.etNoPens = g_frEnd == 1; g_setup.pensOnly = g_frEnd == 5;
    g_setup.benchSize = g_frBench;
    g_setup.rolling = g_frSubs >= 12; g_setup.maxSubs = g_frSubs >= 12 ? -1 : g_frSubs;
    int si = g_frStadium % (int)stadiums.size();
    g_setup.stadium = stadiums[si]; g_setup.neutral = si >= 2;
    g_setup.kickoffTime = TIMEN[g_frTime];
    g_setup.night = g_frTime >= 4;
    crowdContext(g_setup);
    // retransmission : chaîne choisie (générique, plateau, publicités, homme du match, caméras) ou match sans TV
    if (g_frTv >= 1 && g_frTv <= 4) { g_setup.tv = true; g_setup.studio = true; g_setup.proMedia = true; g_setup.channel = FR_CHANNELS[g_frTv - 1]; setFriendlyTv(g_frTv - 1); }
    else if (g_frTv == 5) { g_setup.tv = false; g_setup.studio = false; g_setup.proMedia = false; g_setup.channel.clear(); }
}

// petites icônes de l'avant-match (12 x 12)
static void drawDeviceIcon(int dev, int x, int y, Color c) {
    if (dev <= IN_KB2) {   // clavier
        DrawRectangle(x, y + 2, 16, 9, c); DrawRectangle(x + 1, y + 3, 14, 7, Color{ 30, 30, 40, 255 });
        for (int r = 0; r < 2; r++) for (int k = 0; k < 6; k++) DrawRectangle(x + 2 + k * 2, y + 4 + r * 2, 1, 1, c);
        DrawRectangle(x + 4, y + 8, 8, 1, c);
    } else {               // manette
        DrawRectangleRounded(Rectangle{ (float)x, (float)y + 2, 16, 9 }, 0.6f, 4, c);
        DrawRectangle(x + 3, y + 5, 4, 1, Color{ 30, 30, 40, 255 }); DrawRectangle(x + 4, y + 4, 1, 3, Color{ 30, 30, 40, 255 }); DrawRectangle(x + 4, y + 4, 2, 3, Color{ 30, 30, 40, 0 });
        DrawRectangle(x + 11, y + 4, 2, 2, Color{ 220, 50, 50, 255 }); DrawRectangle(x + 12, y + 7, 2, 2, Color{ 50, 110, 230, 255 });
    }
}
static void drawOptIcon(int kind, int x, int y, Color c) {
    Color d{ 20, 24, 40, 255 };
    switch (kind) {
    case 0: DrawCircle(x + 6, y + 6, 5, c); DrawCircle(x + 6, y + 6, 4, d); DrawRectangle(x + 6, y + 3, 1, 4, c); DrawRectangle(x + 6, y + 6, 3, 1, c); break;        // durée
    case 1: DrawRectangle(x + 1, y + 2, 10, 8, Color{ 50, 150, 60, 255 }); DrawRectangle(x + 6, y + 2, 1, 8, WHITE); DrawRectangleLines(x + 1, y + 2, 10, 8, WHITE); break;   // terrain
    case 2: DrawPoly(Vector2{ x + 6.f, y + 6.f }, 5, 5.5f, -90, c); break;                                                                                         // difficulté
    case 3: DrawRectangle(x + 1, y + 1, 10, 10, Color{ 230, 232, 236, 255 }); DrawCircleLines(x + 4, y + 4, 2, Color{ 40, 80, 220, 255 }); DrawLine(x + 6, y + 6, x + 10, y + 10, Color{ 220, 40, 40, 255 }); DrawLine(x + 10, y + 6, x + 6, y + 10, Color{ 220, 40, 40, 255 }); break;   // tactique
    case 4: DrawRectangle(x + 2, y + 2, 8, 9, c); DrawRectangle(x, y + 2, 12, 3, c); DrawRectangle(x + 4, y + 2, 4, 1, d); break;                                    // maillot
    case 5: DrawRectangle(x + 2, y + 1, 6, 9, Color{ 255, 220, 0, 255 }); DrawRectangle(x + 5, y + 3, 6, 9, Color{ 230, 40, 40, 255 }); break;                      // arbitre
    case 6: DrawCircle(x + 5, y + 7, 4, c); DrawRectangle(x + 5, y + 3, 6, 3, c); DrawCircle(x + 5, y + 7, 2, d); break;                                           // sifflet
    case 7: for (int k = 0; k < 4; k++) DrawRectangle(x + 1, y + 1 + k * 3, 10, 2, k == 0 ? c : Color{ 180, 190, 210, 255 }); break;                             // feuille de match
    case 8: DrawTriangle(Vector2{ x + 1.f, y + 4.f }, Vector2{ x + 6.f, y + 8.f }, Vector2{ x + 6.f, y + 0.f }, Color{ 230, 60, 50, 255 }); DrawTriangle(Vector2{ x + 11.f, y + 8.f }, Vector2{ x + 6.f, y + 4.f }, Vector2{ x + 6.f, y + 12.f }, Color{ 60, 200, 80, 255 }); break;   // remplacements
    case 9: DrawRectangle(x, y + 6, 12, 5, Color{ 160, 170, 190, 255 }); DrawRectangle(x + 1, y + 2, 2, 4, c); DrawRectangle(x + 9, y + 2, 2, 4, c); DrawRectangle(x + 3, y + 7, 6, 2, Color{ 50, 150, 60, 255 }); break;   // stade
    case 10: DrawCircle(x + 6, y + 6, 5, Color{ 240, 230, 160, 255 }); DrawCircle(x + 8, y + 4, 4, d); break;                                                     // heure (lune)
    case 11: DrawCircle(x + 4, y + 4, 3, Color{ 255, 200, 50, 255 }); DrawCircle(x + 7, y + 7, 3, Color{ 220, 225, 235, 255 }); DrawCircle(x + 4, y + 8, 3, Color{ 220, 225, 235, 255 }); break;   // météo
    case 12: DrawRectangle(x, y + 2, 12, 8, c); DrawRectangle(x + 1, y + 3, 10, 6, Color{ 60, 120, 200, 255 }); DrawLine(x + 3, y, x + 6, y + 2, c); DrawLine(x + 9, y, x + 6, y + 2, c); break;   // TV
    default: DrawCircle(x + 6, y + 6, 5, WHITE); DrawCircle(x + 5, y + 5, 2, Color{ 30, 30, 30, 255 }); break;                                                  // ballon
    }
}

static void screenSetup() {
    drawBackground("Avant-match");
    const Team& H = g_world.teams[g_setup.home]; const Team& A = g_world.teams[g_setup.away];
    float tnow = (float)GetTime();
    MatchSetup tmpK = g_setup; Kit kits[2]; matchKits(tmpK, kits);
    // ---- affiche du match : bandeau stade, joueurs dans leur tenue, « VS »
    {
        int y0 = 30, h = 70;
        DrawRectangleGradientV(8, y0, VW - 16, h, Color{ 18, 30, 70, 255 }, Color{ 40, 26, 60, 255 });
        fxCrowd(8, y0 + 8, VW - 16, 30, hexc(kits[0].shirt), hexc(kits[1].shirt), tnow, 0.35f, (unsigned)(g_setup.home * 7 + g_setup.away), 5);
        DrawRectangle(8, y0 + 44, VW - 16, h - 44, Color{ 40, 120, 50, 255 });
        for (int x = 8; x < VW - 8; x += 40) DrawRectangle(x, y0 + 44, 20, h - 44, Color{ 46, 132, 56, 255 });
        DrawRectangle(8, y0 + 44, VW - 16, 1, Color{ 230, 240, 230, 255 });
        DrawRectangle(8, y0, VW - 16, h, Color{ 0, 0, 0, 70 });
        DrawRectangleLines(8, y0, VW - 16, h, C_HI);
        for (int t = 0; t < 2; t++) {
            const Team& T = t ? A : H;
            int cx = t ? VW - 150 : 150;
            int skin = T.squad.empty() ? 0 : T.squad[0].skin, hair = T.squad.empty() ? 0 : T.squad[0].hair;
            drawPlayerSprite(t ? VW - 46 : 46, y0 + h - 4, kits[t], skin, hair, t ? 3 : 2, (int)(tnow * 2) % 2 ? 0 : 0, PS_NORMAL, false, 0, 3);
            drawTextCentered(fitText(T.name, 200, 20), cx, y0 + 6, 20, WHITE);
            int fm = g_setup.formation[t] >= 0 ? g_setup.formation[t] : T.formation;
            drawTextCentered(fmt("%s  -  note %d", FORMATIONS[fm].name, (int)T.rating), cx, y0 + 30, 10, Color{ 220, 230, 255, 255 });
            int bw = 120, bx = cx - bw / 2, by = y0 + 46;
            DrawRectangle(bx, by, bw, 6, Color{ 10, 14, 30, 200 }); DrawRectangle(bx, by, (int)(bw * std::min(1.f, T.rating / 100.f)), 6, T.rating >= 80 ? C_GOOD : T.rating >= 65 ? C_HI : C_BAD);
        }
        DrawCircle(VW / 2, y0 + 30, 19, Color{ 20, 20, 30, 230 }); DrawCircleLines(VW / 2, y0 + 30, 19, C_HI);
        drawTextCentered("VS", VW / 2, y0 + 21, 20, C_HI, false);
        std::string sub = g_setup.hasFirstLeg ? fmt("Match aller : %s %d - %d %s", A.shortName.c_str(), g_setup.aggAway, g_setup.aggHome, H.shortName.c_str())
                                              : g_setup.stadium + (g_setup.kickoffDate.empty() ? std::string() : "  -  " + g_setup.kickoffDate);
        drawTextCentered(fitText(g_setup.title, 200, 10), VW / 2, y0 + 52, 10, C_HI, false);
        drawTextCentered(fitText(sub, 230, 10), VW / 2, y0 + h + 2, 10, C_DIM, false);
    }
    // ---- contrôleurs : un jeton par manette/clavier, qui glisse entre DOMICILE, ORDINATEUR et EXTÉRIEUR
    int y = 114;
    std::vector<int> devs;
    for (int d = 0; d < NUM_INPUTS; d++) if (inputAvailable(d)) devs.push_back(d);
    {
        static const Color CC[NUM_INPUTS] = { { 255, 230, 40, 255 }, { 60, 220, 255, 255 }, { 255, 90, 200, 255 }, { 110, 255, 110, 255 }, { 255, 150, 40, 255 }, { 200, 160, 255, 255 } };
        static float slide[NUM_INPUTS] = { 1, 1, 1, 1, 1, 1 };
        int rowH = devs.size() > 4 ? 12 : 15;
        int cw = VW - 40, colw = cw / 3;
        bool selRow = g_setupRow == 0;
        DrawRectangle(16, y - 2, VW - 32, 14 + (int)devs.size() * (rowH + 1) + 4, Color{ 6, 12, 28, 200 });
        if (selRow) DrawRectangleLines(16, y - 2, VW - 32, 14 + (int)devs.size() * (rowH + 1) + 4, C_HI);
        int nh = 0, na = 0; for (int d = 0; d < NUM_INPUTS; d++) { if (inputAvailable(d) && g_setup.side[d] == 0) nh++; if (inputAvailable(d) && g_setup.side[d] == 1) na++; }
        for (int c = 0; c < 3; c++) {
            int hx = 20 + c * colw;
            Color hc = c == 0 ? hexc(kits[0].shirt) : c == 2 ? hexc(kits[1].shirt) : Color{ 90, 96, 120, 255 };
            DrawRectangle(hx + 2, y, colw - 4, 11, hc);
            std::string lab = c == 0 ? fmt("DOMICILE - %s (%d)", H.shortName.c_str(), nh) : c == 2 ? fmt("EXTÉRIEUR - %s (%d)", A.shortName.c_str(), na) : std::string("ORDINATEUR");
            float br = (hc.r * 0.3f + hc.g * 0.59f + hc.b * 0.11f) / 255.f;
            drawTextCentered(fitText(lab, colw - 10, 10), hx + colw / 2, y + 1, 10, br > 0.55f ? BLACK : WHITE, false);
        }
        y += 14;
        for (int k = 0; k < (int)devs.size(); k++) {
            int d = devs[k];
            int cy = y + k * (rowH + 1);
            for (int c = 0; c < 3; c++) DrawRectangle(20 + c * colw + 2, cy, colw - 4, rowH, c == 1 ? Color{ 30, 36, 56, 255 } : c == 0 ? Color{ 30, 44, 90, 255 } : Color{ 80, 30, 40, 255 });
            int col = g_setup.side[d] == 0 ? 0 : g_setup.side[d] == 1 ? 2 : 1;
            slide[d] += (col - slide[d]) * std::min(1.f, GetFrameTime() * 12.f);
            int bx = 20 + (int)(slide[d] * colw) + 6, bwid = colw - 12;
            Color tc = col == 1 ? Color{ 110, 116, 140, 255 } : CC[d];
            DrawRectangle(bx + 2, cy + 2, bwid, rowH - 2, Color{ 0, 0, 0, 90 });
            DrawRectangle(bx, cy + 1, bwid, rowH - 2, tc);
            DrawRectangle(bx, cy + 1, bwid, 2, Color{ 255, 255, 255, 70 });
            drawDeviceIcon(d, bx + 4, cy + (rowH - 13) / 2, Color{ 20, 20, 30, 255 });
            drawTextCentered(inputName(d), bx + bwid / 2 + 8, cy + (rowH - 10) / 2, 10, BLACK, false);
            if (selRow && ((int)(tnow * 3) % 2)) {   // flèches de déplacement
                if (col > 0) drawTextPx("<", bx - 7, cy + (rowH - 10) / 2, 10, C_HI);
                if (col < 2) drawTextPx(">", bx + bwid + 2, cy + (rowH - 10) / 2, 10, C_HI);
            }
            if (g_setupRow == 0) {
                if (IN.devLeft[d]) g_setup.side[d] = g_setup.side[d] == 1 ? -1 : 0;
                if (IN.devRight[d]) g_setup.side[d] = g_setup.side[d] == 0 ? -1 : 1;
            }
            if (IN.click && IN.mouse.y >= cy && IN.mouse.y < cy + rowH && IN.mouse.x >= 20 && IN.mouse.x < 20 + cw) {
                int c = (int)((IN.mouse.x - 20) * 3 / cw);
                g_setup.side[d] = c == 0 ? 0 : c == 2 ? 1 : -1;
                g_setupRow = 0;
            }
        }
        y += (int)devs.size() * (rowH + 1) + 8;
    }
    // options
    bool humanH = false, humanA = false;
    for (int d = 0; d < NUM_INPUTS; d++) { if (g_setup.side[d] == 0) humanH = true; if (g_setup.side[d] == 1) humanA = true; }
    static const char* KITN[] = { "automatique", "domicile", "extérieur", "troisième" };
    static const char* SEVN[] = { "selon l'arbitre", "clément", "normal", "sévère", "très sévère" };
    static const int SEVV[] = { -1, 35, 60, 80, 95 };
    static const char* ENDN[] = { "match nul possible", "prolongation (nul possible)", "prolongation, but en or, puis TAB", "tirs au but directs", "prolongation puis tirs au but", "séance de tirs au but directe" };
    static const char* TIMEN[] = { "13h00", "15h00", "17h00", "18h00", "20h45", "21h00" };
    bool friendly = !g_mctx.career || (g_mctx.comp >= 0 && g_career.season.comps[g_mctx.comp].kind == 12);
    std::vector<std::string> stadiums = { H.stadium, A.stadium, "Stade de France (Saint-Denis)", "Parc des Princes (Paris)", "Stade Vélodrome (Marseille)", "Wembley (Londres)",
                                          "Santiago Bernabéu (Madrid)", "Maracanã (Rio de Janeiro)", "Terrain municipal" };
    std::vector<std::string> rows = {
        fmt("Durée d'une mi-temps : %d min", HALF_MINUTES[g_settings.halfIdx]),
        g_mctx.career ? fmt("Météo : %s  -  terrain %s  (pelouse %d %%)", METEO_NAMES[g_setup.meteo], PITCH_NAMES[g_setup.pitch], g_setup.turf) : fmt("Terrain : %s", PITCH_NAMES[g_setup.pitch]),
        fmt("Difficulté : %s", DIFF_NAMES[g_setup.difficulty]),
        fmt("Tactique %s : %s", H.shortName.c_str(), FORMATIONS[g_setup.formation[0] >= 0 ? g_setup.formation[0] : H.formation].name),
        fmt("Tactique %s : %s", A.shortName.c_str(), FORMATIONS[g_setup.formation[1] >= 0 ? g_setup.formation[1] : A.formation].name),
        fmt("Maillot %s : %s", H.shortName.c_str(), KITN[g_setup.kitSel[0] + 1]),
        fmt("Maillot %s : %s", A.shortName.c_str(), KITN[g_setup.kitSel[1] + 1]) };
    if (friendly) {
        rows.push_back(fmt("Arbitre : %s", SEVN[g_frSev]));
        rows.push_back(fmt("Fin du match : %s", ENDN[g_frEnd]));
        rows.push_back(fmt("Feuille de match : %d joueurs (11 + %d)", 11 + g_frBench, g_frBench));
        rows.push_back(g_frSubs >= 12 ? std::string("Remplacements : illimités (retour possible)") : fmt("Remplacements autorisés : %d", g_frSubs));
        rows.push_back("Stade : " + fitText(stadiums[g_frStadium % stadiums.size()], 200, 10));
        rows.push_back(fmt("Heure du coup d'envoi : %s", TIMEN[g_frTime]));
        rows.push_back(std::string("Météo : ") + METEO_NAMES[g_setup.meteo]);
        rows.push_back(g_frTv == 0 ? std::string("Retransmission : selon l'affiche (") + (g_setup.tv ? "télévisé)" : "sans TV)") : g_frTv == 5 ? std::string("Retransmission : match sans TV") : std::string("Retransmission : en direct sur ") + FR_CHANNELS[g_frTv - 1]);
    }
    static const char* REFKN[5] = { "noire", "jaune", "verte", "rouge", "bleue" };
    int refRow = (int)rows.size();
    rows.push_back(std::string("Tenue des arbitres : ") + REFKN[std::max(0, std::min(4, g_settings.refKit))]);
    rows.push_back(">>> COUP D'ENVOI <<<");
    int nOpt = (int)rows.size();
    int nrows = 1 + nOpt;
    if (IN.up) g_setupRow = (g_setupRow - 1 + nrows) % nrows;
    if (IN.down) g_setupRow = (g_setupRow + 1) % nrows;
    // ---- options en cartes sur deux colonnes ; le coup d'envoi en gros bouton
    static const int ICON[15] = { 0, 1, 2, 3, 3, 4, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
    int nCards = nOpt - 1;
    int perCol = (nCards + 1) / 2;
    int avail = VH - 18 - y - 24;
    int rh = std::max(12, std::min(19, avail / std::max(1, perCol)));
    int colW = (VW - 40) / 2;
    for (int i = 0; i < nCards; i++) {
        int c = i / perCol, rr = i % perCol;
        int x = 16 + c * (colW + 8), yy = y + rr * rh;
        bool sl = g_setupRow == i + 1;
        DrawRectangle(x, yy, colW, rh - 2, sl ? C_SEL : Color{ 34, 56, 100, 255 });
        DrawRectangle(x, yy, 3, rh - 2, sl ? Color{ 255, 255, 255, 255 } : Color{ 90, 130, 200, 255 });
        int ik = i < 15 ? ICON[i] : 13;
        Color ic = sl ? Color{ 40, 30, 10, 255 } : C_HI;
        if ((i == 5 || i == 6) && ik == 4) { Kit kk = kits[i - 5]; drawKitIcon(kk, x + 6, yy + (rh - 10) / 2, 1); }
        else drawOptIcon(ik, x + 6, yy + (rh - 14) / 2 + 1, ic);
        std::string txt = rows[i];
        size_t colon = txt.find(" : ");
        std::string lab = colon == std::string::npos ? txt : txt.substr(0, colon), val = colon == std::string::npos ? std::string() : txt.substr(colon + 3);
        drawTextPx(fitText(lab, colW / 2 - 24, 10), x + 22, yy + (rh - 12) / 2, 10, sl ? BLACK : C_DIM);
        int vx = x + colW / 2 - 2;
        if (sl) { drawTextPx("<", vx, yy + (rh - 12) / 2, 10, BLACK); drawTextPx(">", x + colW - 10, yy + (rh - 12) / 2, 10, BLACK); }
        drawTextCentered(fitText(val, colW / 2 - 22, 10), vx + (colW / 2 - 4) / 2, yy + (rh - 12) / 2, 10, sl ? BLACK : C_TXT, false);
        if (IN.click && IN.mouse.x >= x && IN.mouse.x < x + colW && IN.mouse.y >= yy && IN.mouse.y < yy + rh - 2) {
            if (g_setupRow == i + 1) { if (IN.mouse.x < x + colW * 3 / 4) IN.left = true; else IN.right = true; }   // clic sur la carte choisie : valeur précédente / suivante
            g_setupRow = i + 1;
        }
    }
    {   // bouton du coup d'envoi
        int by = y + perCol * rh + 2, bw = 240, bx = VW / 2 - bw / 2;
        bool sl = g_setupRow == nOpt;
        float pulse = sl ? 0.5f + 0.5f * std::sin(tnow * 6) : 0.f;
        DrawRectangle(bx + 3, by + 3, bw, 18, Color{ 0, 0, 0, 100 });
        DrawRectangle(bx, by, bw, 18, sl ? Color{ (unsigned char)(60 + 40 * pulse), 200, 80, 255 } : Color{ 40, 130, 60, 255 });
        DrawRectangleLines(bx, by, bw, 18, sl ? WHITE : Color{ 120, 220, 140, 255 });
        drawOptIcon(13, bx + 8, by + 3, WHITE);
        drawTextCentered("COUP D'ENVOI", VW / 2, by + 4, 10, sl ? BLACK : WHITE, false);
        if (IN.click && IN.mouse.x >= bx && IN.mouse.x < bx + bw && IN.mouse.y >= by && IN.mouse.y < by + 18) { applyFriendly(friendly, stadiums); if (g_setup.tv) openTvAds(0, SC_TVINTRO); else launchMatch(); return; }
    }
    int r = g_setupRow - 1;
    int dl = IN.left ? -1 : IN.right ? 1 : 0;
    if (dl && r == refRow) { g_settings.refKit = (g_settings.refKit + dl + 5) % 5; g_setup.refKit = g_settings.refKit; g_settings.save(); dl = 0; }
    if (dl && r >= 0) {
        switch (r) {
        case 0: g_settings.halfIdx = (g_settings.halfIdx + dl + NUM_HALF) % NUM_HALF; break;
        case 1: if (g_mctx.career) toast("Carrière : le terrain dépend de la météo, de la saison et de la pelouse"); else g_setup.pitch = (g_setup.pitch + dl + 6) % 6; break;
        case 2: g_setup.difficulty = (g_setup.difficulty + dl + 3) % 3; break;
        case 3: case 4: {
            int t = r - 3; int team = t == 0 ? g_setup.home : g_setup.away;
            int f = g_setup.formation[t] >= 0 ? g_setup.formation[t] : g_world.teams[team].formation;
            g_setup.formation[t] = (f + dl + NUM_FORMATIONS) % NUM_FORMATIONS;
            if ((t == 0 && humanH) || (t == 1 && humanA)) g_world.teams[team].formation = g_setup.formation[t];
            break;
        }
        case 5: case 6: { int t = r - 5; g_setup.kitSel[t] = (g_setup.kitSel[t] + 1 + dl + 4) % 4 - 1; break; }
        default:
            if (friendly) switch (r) {
            case 7: g_frSev = (g_frSev + dl + 5) % 5; break;
            case 8: g_frEnd = (g_frEnd + dl + 6) % 6; break;
            case 9: g_frBench = std::max(1, std::min(12, g_frBench + dl)); break;
            case 10: g_frSubs = std::max(0, std::min(12, g_frSubs + dl)); break;
            case 11: g_frStadium = (g_frStadium + dl + (int)stadiums.size()) % (int)stadiums.size(); break;
            case 12: g_frTime = (g_frTime + dl + 6) % 6; break;
            case 13: g_setup.meteo = (g_setup.meteo + dl + 6) % 6; meteoApply(g_setup, false, g_setup.turf); break;
            case 14: g_frTv = (g_frTv + dl + 6) % 6; break;
            }
            break;
        }
    }
    (void)SEVV;
    if (((IN.ok || IN.start) && g_setupRow == nOpt) || (IN.start && g_setupRow != nOpt)) { applyFriendly(friendly, stadiums); if (g_setup.tv) openTvAds(0, SC_TVINTRO); else launchMatch(); return; }
    drawFooter("Entrée : coup d'envoi    Gauche/Droite : modifier    Retour : annuler");
    if (IN.back) g_screen = g_mctx.back;
    (void)humanH; (void)humanA;
}

// ------------------------------------------------------------------ match
static std::string thousands(int v) { std::string r = std::to_string(std::max(0, v)); for (int i = (int)r.size() - 3; i > 0; i -= 3) r.insert(i, " "); return r; }
// statistiques d'un match joué
struct MStats { int onTarget[2]={},passes[2]={},completed[2]={},saves[2]={},blocked[2]={},woodwork[2]={},throws[2]={},freeKicks[2]={},goalKicks[2]={},penalties[2]={}; int poss = 50, shots[2] = { 0, 0 }, corners[2] = { 0, 0 }, fouls[2] = { 0, 0 }, offs[2] = { 0, 0 }, yel[2] = { 0, 0 }, red[2] = { 0, 0 }, inj[2] = { 0, 0 }, subs[2] = { 0, 0 }; };
static MStats statsOf(const Match& m) {
    MStats st;
    float tot = m.possTime[0] + m.possTime[1];
    st.poss = tot > 0 ? (int)std::lround(m.possTime[0] / tot * 100) : 50;
    for (int t = 0; t < 2; t++) { st.onTarget[t]=m.onTarget[t];st.passes[t]=m.passes[t];st.completed[t]=m.completedPasses[t];st.saves[t]=m.saves[t];st.blocked[t]=m.blockedShots[t];st.woodwork[t]=m.woodwork[t];st.throws[t]=m.throws[t];st.freeKicks[t]=m.freeKicks[t];st.goalKicks[t]=m.goalKicks[t];st.penalties[t]=m.penalties[t];st.shots[t] = m.shots[t]; st.corners[t] = m.corners[t]; st.fouls[t] = m.fouls[t]; st.offs[t] = m.offsides[t]; }
    for (auto& e : m.events) {
        int t = std::max(0, std::min(1, e.team));
        if (e.type == 1) st.yel[t]++;
        if (e.type == 2) st.red[t]++;
        if (e.type == 4) st.inj[t]++;
    }
    for (int t = 0; t < 2; t++) st.subs[t] = std::max(0, (m.S.maxSubs >= 0 ? m.S.maxSubs : 5) - m.subsLeft[t]);
    if (m.S.rolling) st.subs[0] = st.subs[1] = -1;
    return st;
}
// tableau des statistiques (barres comparées)
static void drawStatsTable(const MStats& st, int x, int y, int w,int page=0) {
    struct Row { const char* n; int a, b; bool pct; };
    std::vector<Row> rows;
    if(page==0)rows={{"Possession",st.poss,100-st.poss,true},{"Tirs tentés",st.shots[0],st.shots[1],false},{"Tirs cadrés",st.onTarget[0],st.onTarget[1],false},{"Tirs bloqués",st.blocked[0],st.blocked[1],false},{"Montants",st.woodwork[0],st.woodwork[1],false},{"Passes tentées",st.passes[0],st.passes[1],false},{"Passes réussies",st.completed[0],st.completed[1],false},{"Précision passes",st.passes[0]?100*st.completed[0]/st.passes[0]:0,st.passes[1]?100*st.completed[1]/st.passes[1]:0,true},{"Arrêts gardien",st.saves[0],st.saves[1],false}};
    else rows={{"Corners",st.corners[0],st.corners[1],false},{"Touches",st.throws[0],st.throws[1],false},{"Coups francs",st.freeKicks[0],st.freeKicks[1],false},{"Sorties de but",st.goalKicks[0],st.goalKicks[1],false},{"Penalties accordés",st.penalties[0],st.penalties[1],false},{"Fautes",st.fouls[0],st.fouls[1],false},{"Hors-jeu",st.offs[0],st.offs[1],false},{"Cartons jaunes",st.yel[0],st.yel[1],false},{"Cartons rouges",st.red[0],st.red[1],false},{"Blessés",st.inj[0],st.inj[1],false},{"Remplacements",st.subs[0],st.subs[1],false}};
    if(page&&st.subs[0]<0)rows.pop_back();
    DrawRectangle(x - 4, y - 4, w + 8, (int)rows.size() * 17 + 6, Color{ 6, 12, 28, 210 });
    int cx = x + w / 2, half = w / 2 - 34;
    for (auto& r : rows) {
        drawTextCentered(r.n, cx, y, 10, C_DIM, false);
        std::string as = r.pct ? fmt("%d%%", r.a) : fmt("%d", r.a), bs = r.pct ? fmt("%d%%", r.b) : fmt("%d", r.b);
        drawTextPx(as, x + 2, y + 5, 10, r.a > r.b ? C_HI : C_TXT);
        drawTextPx(bs, x + w - 2 - textWidth(bs, 10), y + 5, 10, r.b > r.a ? C_HI : C_TXT);
        int tot = std::max(1, r.a + r.b);
        int la = half * r.a / tot, lb = half * r.b / tot;
        DrawRectangle(cx - half, y + 12, half, 3, Color{ 40, 50, 80, 255 }); DrawRectangle(cx + 1, y + 12, half, 3, Color{ 40, 50, 80, 255 });
        DrawRectangle(cx - la, y + 12, la, 3, Color{ 90, 160, 255, 230 });
        DrawRectangle(cx + 1, y + 12, lb, 3, Color{ 255, 120, 90, 230 });
        y += 17;
    }
}
static bool g_paused = false;
static int g_pauseMenu = 0; // 0 principal, 1 remplacements
static ListW g_pauseLW, g_subLW;
static int g_subTeam = 0, g_subOut = -1;
static float g_acc = 0;
static float g_hlHold = 0, g_hlCut = 0;     // mode temps forts
static int g_fmSpeed = 1;                   // Full Manager : vitesse du match regardé (x1, x2, x4)

static void finishMatchToResult();

static void simulateRest(Match& m) {
    // termine le match rapidement en simulant le temps restant
    float minutes=2.f*m.S.halfMinutes;
    float remain = std::max(0.f, minutes - m.clock) / 90.f;
    Rng& r = g_rng;
    double rh = g_world.teams[m.S.home].rating, ra = g_world.teams[m.S.away].rating;
    double d = rh - ra;
    auto pois = [&](double lam) { double L = std::exp(-lam), p = 1; int k = 0; do { k++; p *= r.f(); } while (p > L && k < 10); return k - 1; };
    // cartons pendant la fin simulée
    for (int t = 0; t < 2; t++) {
        int ny = pois(1.3 * remain);
        for (int k = 0; k < ny; k++) {
            std::vector<int> on; for (int i = t * 11; i < t * 11 + 11; i++) if (m.pl[i].onPitch && !m.pl[i].gk) on.push_back(i);
            if (on.empty()) break;
            m.clock = std::max(m.clock, minutes - 90 * remain * (float)r.f());
            m.giveCard(on[r.range(0, (int)on.size() - 1)], r.chance(0.06f) ? 2 : 1);
        }
    }
    m.score[0] += pois(1.4 * std::exp(d / 22) * remain);
    m.score[1] += pois(1.15 * std::exp(-d / 22) * remain);
    int th = m.score[0] + m.S.aggHome, ta = m.score[1] + m.S.aggAway;
    if (m.S.decisive && th == ta) {
        m.aet = !m.S.noET;
        if (!m.S.noET && r.chance(0.3f)) { if (r.chance(0.5f)) m.score[0]++; else m.score[1]++; }
        th = m.score[0] + m.S.aggHome; ta = m.score[1] + m.S.aggAway;
        if (th == ta) { m.shootout = true; m.pens[0] = r.range(2, 5); m.pens[1] = r.range(2, 5); if (m.pens[0] == m.pens[1]) m.pens[r.range(0, 1)]++; }
    }
    m.clock=m.aet?minutes+30:minutes;
    m.finishMatch();
}

// remise du trophée : finale de coupe (ou supercoupe) gagnée, ou titre de champion acquis lors de la dernière journée
// niveau de la cérémonie selon la compétition (et le style du trophée)
static int trophyTierFor(const Competition& C, int team, int& style) {
    switch (C.kind) {
    case 3: style = 1; return 4;
    case 7: case 8: case 9: case 14: style = 5; return 4;
    case 4: case 16: case 17: case 26: case 27: style = 4; return 1;
    case 5: case 18: style = 3; return 0;
    default: break;
    }
    if (C.format == FMT_LEAGUE) {
        int p, q, g;
        if (g_career.tierOfTeam(team, &p, &q, &g) >= 0) {
            const Pyramid& P = g_career.pyramids[p];
            int tier = P.pools[q].tier;
            int sc = tier >= 0 && tier < (int)P.tiers.size() ? P.tiers[tier].scope : SC_NATIONAL;
            if (sc == SC_DEPT) { style = 3; return 0; }
            if (sc == SC_REGION) { style = 4; return 1; }
        }
        style = 2; return 3;
    }
    if (C.name.find("istrict") != std::string::npos) { style = 3; return 0; }
    if (C.name.find("égional") != std::string::npos) { style = 4; return 1; }
    return 2;
}
static int g_trophyTier = 2;
static bool trophyCheck(const Match& m, int& side, int& kind, int& style, std::string& title) {
    if (!g_mctx.career || g_mctx.comp < 0 || m.S.training) return false;
    Season& S = g_career.season;
    Competition& C = S.comps[g_mctx.comp];
    MatchRes& r = C.matches[g_mctx.match];
    int st = C.stageOfMatch(g_mctx.match);
    if (st < 0) return false;
    std::string rn;
    for (auto& R : C.stages[st].rounds) for (int x : R.m) if (x == g_mctx.match) rn = R.name;
    if (C.format != FMT_LEAGUE && C.format != FMT_QUAL_GROUPS) {
        if (rn != "Finale" || !r.decisive || C.format == FMT_INTERTOTO) return false;
        int th = m.score[0] + m.S.aggHome, ta = m.score[1] + m.S.aggAway;
        if (th != ta) side = th > ta ? 0 : 1;
        else if (m.S.hasFirstLeg && m.S.awayGoals && m.score[1] != m.S.aggHome) side = m.score[1] > m.S.aggHome ? 1 : 0;
        else if (m.shootout) side = m.pens[0] > m.pens[1] ? 0 : 1;
        else return false;
        kind = 0; style = C.kind == 3 ? 1 : 0;
        g_trophyTier = trophyTierFor(C, side ? m.S.away : m.S.home, style);
        title = C.name;
        return true;
    }
    if (C.format != FMT_LEAGUE || st != (int)C.stages.size() - 1) return false;
    const Stage& SG = C.stages[st];
    if (SG.type != ST_LEAGUE || SG.rounds.empty()) return false;
    bool last = false; for (int x : SG.rounds.back().m) if (x == g_mctx.match) last = true;
    if (!last) return false;
    for (int x : SG.rounds.back().m) if (x != g_mctx.match && !C.matches[x].played) return false;   // autre match de la journée non joué
    // classement final avec ce résultat
    MatchRes save = r;
    r.hg = (int16_t)m.score[0]; r.ag = (int16_t)m.score[1]; r.played = true;
    auto tb = C.table(st, std::max(0, (int)r.group));
    r = save;
    if (tb.empty()) return false;
    int champ = tb[0].team;
    if (champ == m.S.home) side = 0; else if (champ == m.S.away) side = 1; else return false;
    kind = 1; style = 2; title = C.name;
    g_trophyTier = trophyTierFor(C, champ, style);
    return true;
}

// dernier match à domicile d'un club qui termine (ou est assuré de terminer) à une place de montée :
// les supporters peuvent envahir le terrain au coup de sifflet final (une fois sur deux environ)
static bool invasionCheck(const Match& m) {
    if (!g_mctx.career || g_mctx.comp < 0 || m.S.training || m.S.neutral || (getenv("FOOT_TEST") && !getenv("FOOT_INVASION"))) return false;
    Season& S = g_career.season;
    Competition& C = S.comps[g_mctx.comp];
    if (C.format != FMT_LEAGUE) return false;
    int home = m.S.home, p, q, g;
    if (g_career.tierOfTeam(home, &p, &q, &g) < 0) return false;
    const Pyramid& P = g_career.pyramids[p];
    if (P.pools[q].comps[g] != g_mctx.comp) return false;
    int tier = P.pools[q].tier;
    int up = tier > 0 ? std::max(1, P.tiers[tier].up) : 0;
    if (up <= 0) return false;
    for (int x = 0; x < (int)C.matches.size(); x++) if (x != g_mctx.match && !C.matches[x].played && C.matches[x].home == home) return false;   // ce n'est pas le dernier match à domicile
    MatchRes& r = C.matches[g_mctx.match];
    int st = C.stageOfMatch(g_mctx.match);
    if (st < 0) return false;
    MatchRes save = r;
    r.hg = (int16_t)m.score[0]; r.ag = (int16_t)m.score[1]; r.played = true;
    auto tb = C.table(st, std::max(0, (int)r.group));
    r = save;
    int pos = -1; for (int i = 0; i < (int)tb.size(); i++) if (tb[i].team == home) pos = i;
    if (pos < 0 || pos >= up) return false;
    return getenv("FOOT_INVASION") || GetRandomValue(0, 99) < 55;
}

// tour de coupe passé (hors finale) : l'équipe qualifiée fait un tour d'honneur
static bool qualifyCheck(const Match& m, int& side) {
    if (!g_mctx.career || g_mctx.comp < 0 || m.S.training || !m.S.decisive) return false;
    Competition& C = g_career.season.comps[g_mctx.comp];
    if (C.format == FMT_LEAGUE || C.format == FMT_QUAL_GROUPS || C.format == FMT_SINGLE) return false;
    int st = C.stageOfMatch(g_mctx.match);
    if (st < 0 || C.stages[st].type != ST_KO) return false;
    std::string rn;
    for (auto& R : C.stages[st].rounds) for (int x : R.m) if (x == g_mctx.match) rn = R.name;
    if (rn == "Finale") return false;
    int th = m.score[0] + m.S.aggHome, ta = m.score[1] + m.S.aggAway;
    if (th != ta) side = th > ta ? 0 : 1;
    else if (m.S.hasFirstLeg && m.S.awayGoals && m.score[1] != m.S.aggHome) side = m.score[1] > m.S.aggHome ? 1 : 0;
    else if (m.shootout) side = m.pens[0] > m.pens[1] ? 0 : 1;
    else return false;
    return true;
}

// menu pause : tableau d'affichage, mini-terrain en direct et statistiques clés
static MStats statsOf(const Match& m);
static void drawPauseBoard(const Match& m) {
    float t = (float)GetTime();
    // titre
    DrawRectangle(0, 28, VW, 24, Color{ 10, 18, 40, 230 }); DrawRectangle(0, 51, VW, 2, C_HI);
    drawTextShadow("PAUSE", 18, 31, 20, C_HI);
    drawTextPx(fitText(m.S.title + (m.S.stadium.empty() ? std::string() : "  -  " + m.S.stadium), VW - 140, 10), 120, 37, 10, C_DIM);
    int px = 320, pw = VW - px - 14;
    // tableau d'affichage
    DrawRectangle(px, 58, pw, 48, Color{ 6, 10, 24, 235 }); DrawRectangleLines(px, 58, pw, 48, Color{ 90, 130, 200, 255 });
    drawKitIcon(m.kit[0], px + 8, 66, 3); drawKitIcon(m.kit[1], px + pw - 32, 66, 3);
    drawTextCentered(fitText(m.team(0).shortName, 90, 10), px + 80, 64, 10, C_TXT, false);
    drawTextCentered(fitText(m.team(1).shortName, 90, 10), px + pw - 80, 64, 10, C_TXT, false);
    DrawRectangle(px + pw / 2 - 36, 62, 72, 26, Color{ 0, 0, 0, 200 });
    drawTextCentered(fmt("%d - %d", m.score[0], m.score[1]), px + pw / 2, 65, 20, C_HI, false);
    static const char* PER[4] = { "1re mi-temps", "2e mi-temps", "prolongation 1", "prolongation 2" };
    drawTextCentered(m.S.training ? std::string("Entraînement") : fmt("%d'  -  %s", std::max(0, (int)m.clock), PER[std::max(0, std::min(3, m.period))]), px + pw / 2, 92, 10, C_DIM, false);
    // mini-terrain : position des joueurs et du ballon
    int mx = px, my = 112, mw = pw, mh = 104;
    DrawRectangle(mx, my, mw, mh, Color{ 40, 120, 50, 255 });
    for (int k = 0; k < 8; k++) if (k % 2) DrawRectangle(mx + k * mw / 8, my, mw / 8, mh, Color{ 46, 132, 56, 255 });
    DrawRectangleLines(mx + 4, my + 4, mw - 8, mh - 8, Color{ 230, 240, 230, 200 });
    DrawLine(mx + mw / 2, my + 4, mx + mw / 2, my + mh - 4, Color{ 230, 240, 230, 200 });
    DrawCircleLines(mx + mw / 2, my + mh / 2, 12, Color{ 230, 240, 230, 200 });
    DrawRectangleLines(mx + 4, my + mh / 2 - 22, 22, 44, Color{ 230, 240, 230, 200 }); DrawRectangleLines(mx + mw - 26, my + mh / 2 - 22, 22, 44, Color{ 230, 240, 230, 200 });
    auto MP = [&](V2 p) { float ux = std::max(0.f, std::min(1.f, p.y / PITCH_L)), uy = std::max(0.f, std::min(1.f, p.x / PITCH_W)); return Vector2{ mx + 4 + ux * (mw - 8), my + 4 + uy * (mh - 8) }; };   // terrain couché (but à gauche et à droite)
    for (int i = 0; i < 22; i++) if (m.pl[i].onPitch) {
        Vector2 q = MP(m.pl[i].pos);
        Color c = hexc(m.pl[i].gk ? m.gkShirt[m.pl[i].team] : m.kit[m.pl[i].team].shirt);
        DrawCircle((int)q.x, (int)q.y, 3, BLACK); DrawCircle((int)q.x, (int)q.y, 2, c);
        if (m.pl[i].human >= 0 && (int)(t * 3) % 2) DrawCircleLines((int)q.x, (int)q.y, 5, WHITE);
    }
    { Vector2 b = MP(m.ball.pos); DrawCircle((int)b.x, (int)b.y, 2, WHITE); }
    // statistiques clés
    MStats st = statsOf(m);
    float tot = m.possTime[0] + m.possTime[1];
    int poss = tot > 0 ? (int)std::lround(m.possTime[0] / tot * 100) : 50;
    int sy = my + mh + 8;
    DrawRectangle(px, sy - 4, pw, 98, Color{ 6, 10, 24, 235 });
    auto bar = [&](const char* lab, int a, int b, int y) {
        int tw = std::max(1, a + b);
        drawTextCentered(lab, px + pw / 2, y, 10, C_DIM, false);
        drawTextPx(fmt("%d", a), px + 8, y, 10, C_TXT); std::string bs = fmt("%d", b); drawTextPx(bs, px + pw - 8 - textWidth(bs, 10), y, 10, C_TXT);
        DrawRectangle(px + 30, y + 11, pw - 60, 3, hexc(m.kit[1].shirt)); DrawRectangle(px + 30, y + 11, (pw - 60) * a / tw, 3, hexc(m.kit[0].shirt));
    };
    bar("Possession %", poss, 100 - poss, sy);
    bar("Tirs", m.shots[0], m.shots[1], sy + 18);
    bar("Tirs cadrés", m.onTarget[0], m.onTarget[1], sy + 36);
    bar("Corners", m.corners[0], m.corners[1], sy + 54);
    bar("Fautes", m.fouls[0], m.fouls[1], sy + 72);
    (void)st;
}

static std::vector<std::string> wrapText(const std::string& s, int maxw);
// match télévisé : désignation de l'homme du match à la fin de la rencontre (bandeau de la chaîne)
static void drawManOfMatch(const Match& m) {
    static const Match* done = nullptr; static int sq = -1, tm = -1; static float rating = 0;
    if (done != &m) { done = &m; sq = m.manOfMatch(tm); rating = sq >= 0 ? m.playerRating(tm, sq) : 0; }
    if (sq < 0 || m.stateT < 1.5f) return;
    const Team& T = m.team(tm);
    if (sq >= (int)T.squad.size()) return;
    const Player& P = T.squad[sq];
    float u = std::min(1.f, (m.stateT - 1.5f) * 3.f);
    int w = 360, h = 112, x = VW / 2 - w / 2, y = VH - 132 + (int)((1 - u) * 140);
    float t = (float)GetTime();
    DrawRectangle(x + 4, y + 4, w, h, Color{ 0, 0, 0, 120 });
    DrawRectangleGradientV(x, y, w, h, Color{ 18, 28, 64, 240 }, Color{ 8, 12, 30, 240 });
    DrawRectangle(x, y, w, 18, Color{ 190, 30, 40, 255 });
    drawTextPx(m.S.channel.empty() ? std::string("TV") : m.S.channel, x + 6, y + 4, 10, WHITE);
    drawTextCentered("HOMME DU MATCH", x + w / 2 + 30, y + 4, 10, C_HI, false);
    DrawRectangle(x, y + h - 3, w, 3, C_HI);
    // projecteur et joueur dans sa tenue
    DrawTriangle(Vector2{ (float)x + 48, (float)y + 18 }, Vector2{ (float)x + 22, (float)y + h - 6 }, Vector2{ (float)x + 74, (float)y + h - 6 }, Color{ 255, 245, 200, 40 });
    int pskin = P.skin, phair = P.hair;
    drawPlayerSprite(x + 48, y + h - 8, m.kit[tm], pskin, phair, 0, (int)(t * 4), PS_CELEB, false, 0, 3);
    fxBigTrophy(x + 78, y + h - 8, 4, 1, t);
    int tx = x + 100;
    drawTextPx(fitText(P.name, w - 110, 20), tx, y + 24, 20, C_HI);
    drawTextPx(fitText(T.name, w - 110, 10), tx, y + 46, 10, C_DIM);
    // note sur 10
    DrawRectangle(x + w - 58, y + 24, 50, 30, rating >= 8 ? Color{ 40, 140, 60, 255 } : rating >= 7 ? Color{ 90, 130, 40, 255 } : Color{ 140, 110, 30, 255 });
    drawTextCentered(fmt("%.1f", rating), x + w - 33, y + 30, 20, WHITE, false);
    // statistiques marquantes
    int goals = 0, assists = 0;
    for (auto& e : m.events) { if (e.type == 0 && e.team == tm && e.pid == P.id) goals++; if (e.type == 0 && e.team == tm && e.aid == P.id) assists++; }
    std::string st;
    auto add = [&](int v, const char* one, const char* many) { if (v <= 0) return; if (!st.empty()) st += "  -  "; st += fmt("%d %s", v, v > 1 ? many : one); };
    auto it = m.pst[tm].find(sq);
    add(goals, "but", "buts"); add(assists, "passe décisive", "passes décisives");
    if (it != m.pst[tm].end()) {
        const auto& ps = it->second;
        if (ps.gk) add(ps.save, "arrêt", "arrêts");
        add(ps.tackle, "tacle réussi", "tacles réussis"); add(ps.inter, "interception", "interceptions");
        if (st.size() < 40) add(ps.passOk, "passe réussie", "passes réussies");
    }
    if (st.empty()) st = "Un match plein, présent dans tous les duels.";
    for (auto& l : wrapText(st, w - 110)) { drawTextPx(l, tx, y + 62, 10, C_TXT); y += 12; }
}

static void trainSeriesDone(int drill, int score);
static void screenMatch(float dt) {
    Match& m = *g_match;
    if (m.S.training && m.trainTries > 10 && !getenv("FOOT_TEST")) {     // série de 10 essais terminée : bilan dans le menu d'entraînement
        trainSeriesDone(m.S.training, m.trainGoals); audioCrowd(false, 0); g_screen = SC_TRAINMODE; return;
    }
    if(g_mctx.career&&g_career.sportingMode()){for(auto& side:m.S.side)side=-1;m.S.managed=-1;m.S.delegSubs=true;}
    m.S.snes = g_settings.controlStyle == 1;     // style de commandes modifiable depuis la pause
    for (int i = 0; i < NUM_INPUTS; i++) m.ctl[i] = CTL[i];
    bool pausePressed = false;
    for (int i = 0; i < NUM_INPUTS; i++) if (CTL[i].pause) pausePressed = true;
    bool justPaused = false;
    if (!g_paused && pausePressed && !m.finished) { g_paused = true; g_pauseMenu = 0; g_pauseLW = ListW(); pausePressed = false; justPaused = true; }
    if (!g_paused) {
        if (m.state == MS_REPLAY) for (int i = 0; i < NUM_INPUTS; i++) if (CTL[i].f1p || CTL[i].f2p) m.stateT = 99;
        const float step = 1.f / 60.f;
        bool skipOk = m.S.highlights && !m.finished && !m.ceremony && !m.htWaiting && !m.trophyActive && !m.lapActive && m.state != MS_WALKOUT;
        if (skipOk && m.hotPhase()) g_hlHold = 2.0f;
        if (skipOk && !m.hotPhase() && g_hlHold <= 0) {
            // mode Full Manager : on saute directement au prochain temps fort (pas de jeu accéléré)
            int k = 0;
            for (; k < 6000 && !m.hotPhase() && !m.finished && !m.htWaiting && !m.ceremony; k++) { m.update(step); m.sfxN = 0; m.rumbleN = 0; }
            if (k > 30) g_hlCut = 1.1f;
            g_acc = 0;
        } else {
            if (g_hlHold > 0) g_hlHold -= dt;
            if (m.S.managed >= 0 && IsKeyPressed(KEY_TAB)) g_fmSpeed = g_fmSpeed >= 4 ? 1 : g_fmSpeed * 2;
            bool calm = m.state == MS_PLAY && !m.hotPhase();
            g_acc += std::min(dt, 0.1f) * (m.S.managed >= 0 && calm ? g_fmSpeed : 1);
            while (g_acc >= step) { m.update(step); g_acc -= step; }
        }
        if (g_settings.vibration) {   // vibrations : manette du joueur concerné, ou toutes les manettes d'une équipe
            for (int i = 0; i < m.rumbleN; i++) {
                int who = m.rumbleQ[i][0], kind = m.rumbleQ[i][1];
                for (int c = IN_PAD1; c < IN_PAD1 + 4; c++) {
                    bool hit = who >= 0 ? who == c : m.S.side[c] == -1 - who;
                    if (hit && m.S.side[c] >= 0) rumblePlay(c - IN_PAD1, kind);
                }
            }
            // penalty d'un joueur humain : battements de cœur pendant l'attente
            static float beat = 0;
            if (m.state == MS_SETPIECE && m.sp == SP_PENALTY && m.spKicker >= 0 && m.pl[m.spKicker].human >= IN_PAD1) { beat -= dt; if (beat <= 0) { beat = 0.85f; rumblePlay(m.pl[m.spKicker].human - IN_PAD1, RB_HEARTBEAT); } }
        }
        m.rumbleN = 0;
        for (int i = 0; i < m.sfxN; i++) {
            audioPlay(m.sfxQueue[i]);
            if (m.sfxQueue[i] == SFX_GOAL && m.lastScorerTeam == 0 && !m.S.neutral && !m.S.training && !m.invasion && (m.S.proMedia || m.S.supporterAtmosphere >= 60)) audioJingle(8);   // la sono du stade

        }
        m.sfxN = 0;
        // musique d'entrée des joueurs (éditeur) : sortie du tunnel, effacée avant les hymnes et le coup d'envoi
        {
            static int walkDone = -1;
            if (m.ceremony && m.cerPhase < 0 && walkDone != g_matchSerial) {
                walkDone = g_matchSerial;
                std::string comp = g_mctx.career && g_mctx.comp >= 0 && g_mctx.comp < (int)g_career.season.comps.size() ? g_career.season.comps[g_mctx.comp].name : m.S.title;
                std::string file = walkMusicFor(comp);
                if (!file.empty()) audioWalkoutPlay(file);
            }
            if (audioWalkoutPlaying() && (!m.ceremony || m.anthemReq >= 0 || m.cerPhase >= 10)) audioWalkoutFade();
        }
        // hymnes nationaux
        if (m.anthemReq >= 0) {
            std::string code = m.S.anthemOnly;
            if (code.empty()) { const Team& AT = g_world.teams[m.anthemReq == 0 ? m.S.home : m.S.away]; code = AT.nation >= 0 && AT.nation < NUM_NATIONS ? NATIONS[AT.nation].code : "FRA"; }
            audioAnthemCode(code);
            m.anthemDur = std::max(8.f, std::min(32.f, audioAnthemSeconds() + 0.5f));
            m.anthemName = audioAnthemName(code);
            if (!audioAnthemFile(code).empty()) m.anthemName = GetFileNameWithoutExt(audioAnthemFile(code).c_str());
            m.anthemReq = -1;
        }
        else if (m.anthemReq == -2) { audioStopAnthem(); m.anthemReq = -1; }
        // entraînement : pas de public
        bool anth = m.ceremony && (m.cerPhase == 10 || m.cerPhase == 11);
        {   // rumeur du stade : enfle quand une équipe approche du but, explose sur les buts ; la ola
            float danger = m.state == MS_PLAY && m.possTeam >= 0 ? std::max(0.f, m.progress(m.possTeam, m.ball.pos) - 0.62f) / 0.38f : 0.f;
            if (m.possTeam == 1 && !m.S.neutral) danger *= 0.6f;
            static float swell = 0; swell += (danger - swell) * std::min(1.f, dt * 2.5f);
            float goalBoost = m.state == MS_GOAL ? 0.12f : 0.f;
            audioCrowd(!m.S.training, anth ? 0.08f : .10f + m.S.supporterAtmosphere * .003f + swell * 0.12f + goalBoost);
            static float lastWave = -1; float wv = crowdWavePhase(m);
            if (wv >= 0 && lastWave < 0) audioPlay(SFX_OLA);
            lastWave = wv;
        }
        if(!m.S.training&&!anth){int chant=supportersLiveChant(m.S,m.score[0],m.score[1],m.clock);if((m.state==MS_GOAL||m.state==MS_REPLAY)&&m.lastScorerTeam==0&&!m.S.neutral)chant=CH_CELEBRATE;else if(m.trophyActive||m.lapActive)chant=CH_CELEBRATE;audioSupporters(chant,std::max(.35f,m.S.supporterAtmosphere/100.f));}
    } else audioCrowd(false, 0);
    if (m.finished && !g_trophyChecked && !g_paused) {
        g_trophyChecked = true;
        if (g_mctx.career && g_mctx.comp >= 0) g_career.season.finishRoundOthers(g_mctx.comp, g_mctx.match);   // matchs simultanés de la journée
        int side = -1, kind = 0, style = 0; std::string title;
        if (trophyCheck(m, side, kind, style, title)) { m.trophyTier = g_trophyTier; m.startTrophy(side, kind, style, title); g_trophyShown = true; }
        else if (!m.abandoned && qualifyCheck(m, side)) m.startLap(side);     // qualification en coupe : tour d'honneur
        else if (!m.abandoned && invasionCheck(m)) m.startInvasion(0);        // montée : envahissement de terrain (aléatoire)
    }
    if (m.lapActive) {
        if (IN.start || IN.back) m.lapActive = false;
        renderMatch(m, g_settings.radar);
        return;
    }
    if (m.trophyActive && m.trLift && !g_podiumMusic) { g_podiumMusic = true; audioJingle(4); }     // hymne de la victoire
    // mi-temps : écran récapitulatif, puis plateau TV éventuel, puis reprise
    if (m.htWaiting && !m.htGo && !g_paused) {
        if (!g_htShown) { g_htShown = true; if (m.S.tv && !m.S.training) openTvAds(1, SC_HALFTIME); else g_screen = SC_HALFTIME; return; }
        if (m.S.studio && !g_htStudio) { g_htStudio = true; g_studioPhase = 1; g_studioT = 0; g_screen = SC_STUDIO; return; }
        m.htGo = true;
    }
    if (m.trophyActive && m.trTotal > 2.5f && IN.start) m.endTrophy();          // Entrée uniquement (la touche de tir ne coupe plus la cérémonie)
    renderMatch(m, g_settings.radar);
    {   // aide des commandes : au début du jeu, puis à la demande (F1) ; rappelle les actions avec et sans ballon
        static const Match* hintM = nullptr; static float hintT = 99;
        if (hintM != &m) { hintM = &m; hintT = 0; }
        if (IsKeyPressed(KEY_F1)) hintT = 0;
        if (!g_paused && m.state == MS_PLAY && !m.ceremony) hintT += dt;
        std::vector<int> devs; for (int c = 0; c < NUM_INPUTS; c++) if (m.S.side[c] >= 0 && inputAvailable(c)) devs.push_back(c);
        if (hintT < 8.f && !devs.empty() && !m.finished && m.S.managed < 0 && (m.state == MS_PLAY || m.state == MS_SETPIECE)) {
            float a = std::min(1.f, std::min(hintT * 3.f + 0.3f, (8.f - hintT) * 1.5f));
            int n = std::min(2, (int)devs.size());
            int h = 14 + n * 24, y = VH - 30 - h;
            DrawRectangle(8, y, VW - 16, h, Color{ 6, 10, 28, (unsigned char)(200 * a) });
            DrawRectangle(8, y, 3, h, Color{ 240, 200, 60, (unsigned char)(255 * a) });
            Color hc = C_HI; hc.a = (unsigned char)(255 * a); Color tc = C_TXT; tc.a = hc.a; Color dc = C_DIM; dc.a = hc.a;
            drawTextPx("COMMANDES  (F1 : afficher à nouveau)", 16, y + 2, 10, hc);
            for (int k = 0; k < n; k++) {
                int c = devs[k];
                drawTextPx(fitText(std::string(inputName(c)) + " : " + inputHelp(c), VW - 36, 10), 16, y + 14 + k * 24, 10, tc);
                std::string off = g_settings.controlStyle == 1 ? "Sans ballon : passe = changer de joueur (stick = direction), tir = tacle glissé, lob = pressing. Passe : le contrôle suit le receveur."
                                                              : "Sans ballon : bouton 1 = changer de joueur (près du porteur : tacle), bouton 2 = tacle glissé. Passe : le contrôle suit le receveur.";
                drawTextPx(fitText(off, VW - 36, 10), 16, y + 25 + k * 24, 10, dc);
            }
        }
    }
    if (m.S.managed >= 0 && !m.S.highlights && !m.trophyActive && !g_paused && !m.finished) {
        DrawRectangle(VW - 170, VH - 20, 162, 14, Color{ 0, 0, 0, 150 });
        drawTextPx(fmt("FULL MANAGER  x%d  (Tab : vitesse)", g_fmSpeed), VW - 166, VH - 18, 10, C_HI);
    }
    if (m.S.highlights && !m.trophyActive && !g_paused) {
        DrawRectangle(VW - 150, VH - 20, 142, 14, Color{ 0, 0, 0, 150 });
        drawTextPx("FULL MANAGER : temps forts", VW - 146, VH - 18, 10, C_HI);
        if (g_hlCut > 0) {      // transition entre deux temps forts
            g_hlCut -= dt;
            float a = std::min(1.f, g_hlCut * 2.f);
            DrawRectangle(0, 0, VW, VH, Color{ 0, 0, 0, (unsigned char)(200 * a) });
            DrawRectangle(0, VH / 2 - 22, VW, 44, Color{ 16, 30, 64, (unsigned char)(235 * a) });
            DrawRectangle(0, VH / 2 - 22, VW, 2, Color{ 240, 200, 60, (unsigned char)(255 * a) });
            DrawRectangle(0, VH / 2 + 20, VW, 2, Color{ 240, 200, 60, (unsigned char)(255 * a) });
            Color c = C_HI; c.a = (unsigned char)(255 * a); Color c2 = C_TXT; c2.a = c.a;
            drawTextCentered(fmt("TEMPS FORT  -  %d'", std::max(1, (int)m.clock + 1)), VW / 2, VH / 2 - 16, 20, c, false);
            drawTextCentered(fmt("%s %d - %d %s", g_world.teams[m.S.home].shortName.c_str(), m.score[0], m.score[1], g_world.teams[m.S.away].shortName.c_str()), VW / 2, VH / 2 + 6, 10, c2, false);
        }
    }
    if (m.trophyActive) return;
    // plateau TV à la mi-temps et en fin de match
    if (m.S.studio && !g_paused) {

        if (!g_ftStudio && m.finished && m.stateT > (m.S.tv && !m.abandoned && !g_trophyShown ? 8.5f : 1.2f)) { g_ftStudio = true; g_studioPhase = 2; g_studioT = 0; g_screen = SC_STUDIO; return; }
    }
    if (m.finished) {
        bool motm = m.S.tv && !m.abandoned && !m.S.training && !g_trophyShown;
        if (motm) drawManOfMatch(m);
        if (m.invasion && !(IN.start || IN.back)) { m.stateT = 0; if (m.invT < 1.f) audioSupporters(CH_CELEBRATE, 1.f); return; }
        if (m.invasion) m.invasion = false;
        if (m.stateT > (g_trophyShown ? 2.5f : motm ? 9.0f : 5.0f) || IN.ok || IN.click) { audioCrowd(false, 0); finishMatchToResult(); }
        m.stateT += dt;
        return;
    }
    if (!g_paused) return;
    if (justPaused) { IN.back = IN.ok = IN.up = IN.down = false; }
    // ---- menu pause
    DrawRectangle(0, 0, VW, VH, Color{ 0, 0, 20, 160 });
    if (g_pauseMenu == 0) {
        int ht = m.coachSide(0) ? 0 : m.coachSide(1) ? 1 : -1;
        drawPauseBoard(m);
        std::vector<std::string> items = { "Reprendre", "Remplacements", fmt("Radar : %s", g_settings.radar ? "oui" : "non"),
                                           fmt("Son : %s", g_settings.sound ? "oui" : "non"), m.S.training ? "Changer d'exercice" : "Terminer le match (simuler la fin)" };
        if (!g_mctx.career) items.push_back(m.S.training ? "Quitter l'entraînement" : "Quitter le match");
        if(g_mctx.career&&g_career.sportingMode())items[1]="Coach IA : remplacements automatiques";
        int mentIdx = -1;
        if (ht >= 0 && !m.S.training) { mentIdx = (int)items.size(); items.push_back(std::string("Mentalité : < ") + mentalityName(m.mentality[ht]) + " >"); }
        if (mentIdx >= 0 && g_pauseLW.cur == mentIdx && (IN.left || IN.right)) {
            m.setMentality(ht, m.mentality[ht] + (IN.left ? -1 : 1), false);
            g_world.teams[ht == 0 ? m.S.home : m.S.away].mentality = m.mentality[ht];
        }
        // consignes rapides : pressing et hauteur de la ligne défensive
        static const char* LV3[3] = { "faible", "normal", "fort" };
        static const char* LN3[3] = { "basse", "normale", "haute" };
        int pressIdx = -1, lineIdx = -1;
        if (ht >= 0 && !m.S.training) {
            pressIdx = (int)items.size(); items.push_back(std::string("Pressing : < ") + LV3[std::min(2, (int)m.S.tac[ht][0])] + " >");
            lineIdx = (int)items.size(); items.push_back(std::string("Ligne défensive : < ") + LN3[std::min(2, (int)m.S.tac[ht][1])] + " >");
            for (int k = 0; k < 2; k++) {
                int idx = k ? lineIdx : pressIdx;
                if (g_pauseLW.cur == idx && (IN.left || IN.right)) { int v = (int)m.S.tac[ht][k] + (IN.left ? -1 : 1); m.S.tac[ht][k] = (uint8_t)std::max(0, std::min(2, v)); }
            }
        }
        int statIdx = -1;
        if (!m.S.training) { statIdx = (int)items.size(); items.push_back("Statistiques du match (buteurs, cartons, blessés)"); }
        int ctlIdx = (int)items.size(); items.push_back("Commandes et configuration des touches");
        int s = listRun(g_pauseLW, (int)items.size(), 18, 58, 290, (int)items.size(), 20, [&](int i, int xx, int yy, bool sel) {
            DrawRectangle(xx + 4, yy + 6, 6, 8, sel ? BLACK : i == 0 ? C_GOOD : (i == pressIdx || i == lineIdx || i == mentIdx) ? C_HI : Color{ 90, 130, 200, 255 });
            drawTextPx(fitText(items[i], 266, 10), xx + 16, yy + 5, 10, sel ? Color{ 20, 20, 40, 255 } : C_TXT);
        });
        if (pressIdx >= 0 && (s == pressIdx || s == lineIdx)) { int k = s == lineIdx; m.S.tac[ht][k] = (uint8_t)((m.S.tac[ht][k] + 1) % 3); return; }
        if (statIdx >= 0 && s == statIdx) { g_pauseMenu = 2; return; }
        if (s == ctlIdx) { g_ctlBack = SC_MATCH; g_ctlLW = ListW(); g_ctlCapture = -1; g_screen = SC_CONTROLS; return; }
        if (mentIdx >= 0 && s == mentIdx) {
            m.setMentality(ht, (m.mentality[ht] + 1) % 5, false);
            g_world.teams[ht == 0 ? m.S.home : m.S.away].mentality = m.mentality[ht];
            return;
        }
        if (IN.back || pausePressed) { g_paused = false; return; }
        if (s == 0) g_paused = false;
        else if (s == 1) {
            if(g_mctx.career&&g_career.sportingMode()){toast("Les remplacements appartiennent au coach IA.");return;}
            g_pauseMenu = 1; g_subOut = -1; g_subLW = ListW();
            g_subTeam = m.coachSide(0) ? 0 : 1;
        } else if (s == 2) { g_settings.radar = !g_settings.radar; g_settings.save(); }
        else if (s == 3) { g_settings.sound = !g_settings.sound; audioSetEnabled(g_settings.sound); g_settings.save(); }
        else if (s == 4 && m.S.training) { g_paused = false; g_match.reset(); g_screen = SC_TRAINMODE; }
        else if (s == 4) { simulateRest(m); g_paused = false; }
        else if (s == 5) { g_paused = false; g_match.reset(); g_screen = SC_MAIN; }
        return;
    }
    if (g_pauseMenu == 2) {     // statistiques du match en cours
        drawTextCentered(fmt("%s  %d - %d  %s", m.team(0).name.c_str(), m.score[0], m.score[1], m.team(1).name.c_str()), VW / 2, 26, 10, C_HI);
        drawTextCentered(fmt("%d'  -  %s  -  %s", std::max(0, (int)m.clock), m.S.stadium.c_str(), METEO_NAMES[std::max(0, std::min(5, m.S.meteo))]), VW / 2, 38, 10, C_DIM);
        drawStatsTable(statsOf(m), 20, 58, VW / 2 - 36);
        // feuille de match : buteurs et passeurs, cartons, blessés
        int x2 = VW / 2 + 8, w2 = VW / 2 - 24, y2 = 54;
        DrawRectangle(x2 - 4, y2 - 2, w2 + 8, VH - 80, Color{ 6, 12, 28, 210 });
        for (int t2 = 0; t2 < 2; t2++) {
            drawTextPx(fitText(m.team(t2).name, w2, 10), x2, y2, 10, C_HI); y2 += 12;
            int n = 0;
            std::vector<MatchEvent> evs = m.events;
            std::stable_sort(evs.begin(), evs.end(), [](const MatchEvent& a, const MatchEvent& b) { return a.minute < b.minute; });
            for (auto& e : evs) {
                if (e.team != t2 || y2 > VH - 40) continue;
                static const char* TY[] = { "But", "Jaune", "Rouge", "csc", "Blessé" };
                Color c = e.type == 0 || e.type == 3 ? C_TXT : e.type == 1 ? Color{ 255, 220, 0, 255 } : e.type == 2 ? C_BAD : C_DIM;
                std::string l = fmt("%d' %s : ", (int)e.minute + 1, TY[std::max(0, std::min(4, e.type))]) + e.player + (e.pen ? " (pen.)" : "") + (e.assist.empty() ? "" : " - passe : " + e.assist);
                drawTextPx(fitText(l, w2 - 6, 10), x2 + 6, y2, 10, c); y2 += 11; n++;
            }
            if (!n) { drawTextPx("-", x2 + 6, y2, 10, C_DIM); y2 += 11; }
            y2 += 6;
        }
        drawFooter("Retour : menu pause");
        if (IN.back || IN.ok || pausePressed) g_pauseMenu = 0;
        return;
    }
    // remplacements
    int t = g_subTeam;
    const Team& T = m.team(t);
    drawTextCentered(m.S.rolling ? fmt("REMPLACEMENTS - %s (illimités, un joueur remplacé peut revenir)", T.name.c_str()) : fmt("REMPLACEMENTS - %s (%d restant%s)", T.name.c_str(), m.subsLeft[t], m.subsLeft[t] > 1 ? "s" : ""), VW / 2, 36, 10, C_HI);
    if (m.humanSide(0) && m.humanSide(1) && (IN.left || IN.right)) { g_subTeam = 1 - g_subTeam; g_subOut = -1; }
    std::vector<int> rows; // >=0 : joueur sur le terrain (index pl), <0 : -(1+index banc)
    if (g_subOut < 0) { for (int i = t * 11; i < t * 11 + 11; i++) if (m.pl[i].onPitch) rows.push_back(i); }
    else for (int k = 0; k < (int)m.bench[t].size(); k++) rows.push_back(-(1 + k));
    drawTextCentered(g_subOut < 0 ? "Joueur qui sort :" : "Joueur qui entre :", VW / 2, 50, 10, C_TXT);
    int s = listRun(g_subLW, (int)rows.size(), VW / 2 - 170, 64, 340, 18, 13, [&](int i, int x, int y, bool sel) {
        Color c = sel ? BLACK : C_TXT;
        int r = rows[i];
        const Player* P;
        std::string extra;
        if (r >= 0) {
            P = &T.squad[m.pl[r].squad];
            extra = fmt("forme %d%%", (int)(m.pl[r].stamina * 100));
            if (m.pl[r].injured) extra += " BLESSÉ";
            if (m.pl[r].yellow) extra += " (J)";
        } else P = &T.squad[m.bench[t][-r - 1]];
        static const char* PN[] = { "G", "D", "M", "A" };
        drawTextPx(fmt("%2d %s %s", P->num, PN[P->pos], P->name.c_str()), x + 4, y + 1, 10, c);
        drawTextPx(fmt("%d %s", P->overall(), extra.c_str()), x + 220, y + 1, 10, c);
    });
    if (s >= 0) {
        if (g_subOut < 0) { if (m.subsLeft[t] > 0) { g_subOut = rows[s]; g_subLW = ListW(); } else toast("Plus de remplacement possible"); }
        else {
            int bi = -rows[s] - 1;
            m.substitute(t, m.pl[g_subOut].slot, bi);
            g_subOut = -1; g_subLW = ListW();
        }
    }
    drawFooter("OK : choisir   Retour : revenir" + std::string(m.humanSide(0) && m.humanSide(1) ? "   Gauche/Droite : équipe" : ""));
    if (IN.back) { if (g_subOut >= 0) g_subOut = -1; else g_pauseMenu = 0; }
}

// ------------------------------------------------------------------ écran de mi-temps
static void screenHalftime() {
    if (!g_match) { g_screen = SC_MAIN; return; }
    Match& m = *g_match;
    audioCrowd(false, 0);
    drawBackground("Mi-temps");
    const Team& H = m.team(0); const Team& A = m.team(1);
    {   // bandeau du stade à la pause : tribunes, couleurs des deux équipes
        DrawRectangleGradientV(8, 45, VW - 16, 57, Color{ 18, 30, 70, 255 }, Color{ 40, 26, 60, 255 });
        fxCrowd(8, 48, VW - 16, 36, hexc(m.kit[0].shirt), hexc(m.kit[1].shirt), (float)GetTime(), 0.15f, (unsigned)(m.S.home * 3 + m.S.away), 5);
        DrawRectangle(8, 45, VW - 16, 57, Color{ 0, 0, 0, 110 });
        DrawRectangle(8, 45, (VW - 16) / 2, 3, hexc(m.kit[0].shirt)); DrawRectangle(VW / 2, 45, (VW - 16) / 2, 3, hexc(m.kit[1].shirt));
    }
    drawTextCentered(fitText(m.S.title, VW - 40, 10), VW / 2, 32, 10, C_DIM);
    // score
    drawKitIcon(m.kit[0], VW / 2 - 150, 48, 3);
    drawKitIcon(m.kit[1], VW / 2 + 126, 48, 3);
    drawTextCentered(fitText(H.name, 180, 10), VW / 2 - 138, 90, 10, C_TXT);
    drawTextCentered(fitText(A.name, 180, 10), VW / 2 + 138, 90, 10, C_TXT);
    std::string sc = fmt("%d - %d", m.score[0], m.score[1]);
    DrawRectangle(VW / 2 - 50, 50, 100, 34, Color{ 0, 0, 0, 150 });
    drawTextCentered(sc, VW / 2, 56, 20, C_HI);
    drawTextCentered("MI-TEMPS", VW / 2, 76, 10, C_DIM);
    if (m.S.hasFirstLeg) drawTextCentered(fmt("Cumul %d - %d", m.score[0] + m.S.aggHome, m.score[1] + m.S.aggAway), VW / 2, 90, 10, C_HI);
    // buteurs et cartons
    int y0 = 108;
    for (int t = 0; t < 2; t++) {
        int x = t == 0 ? 30 : VW / 2 + 20, y = y0;
        DrawRectangle(x - 6, y - 4, VW / 2 - 44, 150, Color{ 0, 0, 0, 90 });
        DrawRectangle(x - 6, y - 4, 3, 150, hexc(m.kit[t].shirt));
        drawTextPx(t == 0 ? H.shortName : A.shortName, x, y, 10, C_HI); y += 14;
        int n = 0;
        for (auto& e : m.events) {
            bool mine = e.type == 3 ? e.team != t : e.team == t;
            if (!mine || e.type == 1 || e.type == 2 || e.type == 4) continue;
            std::string l = fmt("%d'  ", (int)e.minute + 1) + e.player + (e.pen ? " (pen.)" : "") + (e.type == 3 ? " (csc)" : "");
            DrawCircle(x + 3, y + 5, 3, WHITE);
            drawTextPx(fitText(l, VW / 2 - 70, 10), x + 10, y, 10, C_TXT); y += 12; n++;
        }
        for (auto& e : m.events) {
            if (e.team != t || (e.type != 1 && e.type != 2 && e.type != 4)) continue;
            if (e.type == 4) { DrawRectangle(x, y + 3, 8, 3, Color{ 230, 40, 40, 255 }); DrawRectangle(x + 3, y, 3, 9, Color{ 230, 40, 40, 255 }); }
            else DrawRectangle(x + 1, y + 1, 5, 8, e.type == 2 ? Color{ 230, 40, 40, 255 } : Color{ 255, 220, 0, 255 });
            drawTextPx(fitText(fmt("%d'  ", (int)e.minute + 1) + e.player, VW / 2 - 70, 10), x + 10, y, 10, C_DIM); y += 12; n++;
        }
        if (!n) drawTextPx("Rien à signaler", x + 10, y, 10, C_DIM);
    }
    // statistiques
    float tot = m.possTime[0] + m.possTime[1];
    int poss = tot > 0 ? (int)std::lround(m.possTime[0] / tot * 100) : 50;
    int sy = y0 + 156;
    drawTextCentered("Possession", VW / 2, sy, 10, C_DIM);
    DrawRectangle(VW / 2 - 120, sy + 12, 240, 7, hexc(m.kit[1].shirt));
    DrawRectangle(VW / 2 - 120, sy + 12, 240 * poss / 100, 7, hexc(m.kit[0].shirt));
    drawTextPx(fmt("%d %%", poss), VW / 2 - 160, sy + 10, 10, C_TXT);
    drawTextPx(fmt("%d %%", 100 - poss), VW / 2 + 128, sy + 10, 10, C_TXT);
    drawTextCentered(fmt("Tirs  %d - %d", m.shots[0], m.shots[1]), VW / 2, sy + 24, 10, C_TXT);
    // consignes pour la seconde période
    int ht = m.coachSide(0) ? 0 : m.coachSide(1) ? 1 : -1;
    if (ht >= 0) {
        std::string ml = std::string("Mentalité : < ") + mentalityName(m.mentality[ht]) + " >";
        if (button(VW / 2 - 110, sy + 38, 220, 13, ml)) m.setMentality(ht, (m.mentality[ht] + 1) % 5, false);
        if (IN.left || IN.right) m.setMentality(ht, m.mentality[ht] + (IN.left ? -1 : 1), false);
        g_world.teams[ht == 0 ? m.S.home : m.S.away].mentality = m.mentality[ht];
        // changement de maillot pour la seconde période
        static const char* KN[3] = { "domicile", "extérieur", "troisième" };
        const Team& T = ht == 0 ? H : A;
        int cur = m.S.kitSel[ht] < 0 ? (m.kit[ht].shirt == T.home.shirt ? 0 : m.kit[ht].shirt == T.away.shirt ? 1 : 2) : m.S.kitSel[ht];
        drawKitIcon(m.kit[ht], VW / 2 - 128, sy + 55, 1);
        if (button(VW / 2 - 110, sy + 54, 220, 13, std::string("Maillot 2e période : < ") + KN[cur] + " >")) {
            int nx = (cur + 1) % 3; m.S.kitSel[ht] = nx; m.kit[ht] = teamKit(T, nx);
            auto cd = [](unsigned a, unsigned b) { float dr = (float)((a >> 16) & 255) - ((b >> 16) & 255), dg = (float)((a >> 8) & 255) - ((b >> 8) & 255), db = (float)(a & 255) - (b & 255); return std::sqrt(dr * dr * 2 + dg * dg * 4 + db * db * 3); };
            if (cd(m.kit[ht].shirt, m.kit[1 - ht].shirt) < 120) toast("Attention : couleurs proches de l'adversaire");
            else toast(std::string("Les joueurs enfileront le maillot ") + KN[nx] + " pour la seconde période");
        }
    }
    drawFooter("Gauche/Droite : mentalité    OK : seconde période    (remplacements : menu pause)");
    if (IN.ok || IN.start) { g_screen = SC_MATCH; openLocker(1); }
}

// ------------------------------------------------------------------ après-match
static std::string roundNameOf(const Competition& C, int mi);
static std::vector<std::string> wrapText(const std::string& s, int maxw);
static std::vector<std::string> wrapTextSz(const std::string& s, int maxw, int fs);
static const MatchRes* firstLegOf(const Competition& C, int mi);
static void autosave();
static void openHub();

struct EvLine { int side; int minute; int type; std::string txt; };   // type 0 but, 1 csc, 2 jaune, 3 rouge, 4 blessure
struct PostInfo { int home, away, hg, ag, ph = -1, pa = -1; bool aet = false; std::vector<EvLine> ev; float poss = 50; int shots[2] = { 0, 0 }; std::string title, extra; int comp = -1, match = -1;
                  bool hasStats = false; MStats st; int att = 0; int64_t gate = -1; bool realGate = false; std::string stadium, meteo, pitch; int turf = -1; float boost = 0; bool neutral = false; std::string motm; };
static int g_postPage = 0;
static PostInfo g_post;
#include "app_tv.inc"
#include "app_adsedit.inc"
#include "app_tvads.inc"


// jingle après un match : victoire, ou trophée si une finale est gagnée
static void postJingle() {
    bool career = g_careerActive;
    int me = -1;
    if (career) { if (g_career.season.isControlled(g_post.home)) me = 0; else if (g_career.season.isControlled(g_post.away)) me = 1; }
    else { bool h = false, a2 = false; for (int d = 0; d < NUM_INPUTS; d++) { if (g_setup.side[d] == 0) h = true; if (g_setup.side[d] == 1) a2 = true; } me = h ? 0 : a2 ? 1 : -1; }
    if (me < 0) return;
    int a = me == 0 ? g_post.hg : g_post.ag, b = me == 0 ? g_post.ag : g_post.hg;
    bool win = a > b || (a == b && g_post.ph >= 0 && (me == 0 ? g_post.ph > g_post.pa : g_post.pa > g_post.ph));
    if (!win) return;
    bool trophy = g_articleTitle.rfind("SACRE", 0) == 0;
    if (trophy && g_trophyShown) return;          // déjà joué pendant la cérémonie
    audioJingle(trophy ? 2 : 0);
}

static std::string playerNameById(int pid) {
    if (pid <= 0) return "";
    int idx; int t = g_world.findPlayer(pid, &idx);
    return t >= 0 ? g_world.teams[t].squad[idx].name : std::string("?");
}

static std::vector<EvLine> eventsFromComp(const Competition& C, int mi) {
    std::vector<EvLine> v;
    for (const MEv& e : C.events) {
        if (e.match != mi) continue;
        EvLine l; l.side = e.side; l.minute = e.minute; l.type = e.type;
        l.txt = playerNameById(e.pid);
        if (e.type == 0 && e.pen) l.txt += " (pen.)";
        if (e.type == 0 && e.aid > 0) l.txt += " (passe : " + playerNameById(e.aid) + ")";
        if (e.type == 1) l.txt += " (csc)";
        if (e.type == 4) l.txt += " (blessé)";
        v.push_back(l);
    }
    std::stable_sort(v.begin(), v.end(), [](const EvLine& a, const EvLine& b) { return a.minute < b.minute; });
    return v;
}

static void applyCardsAndStats(Match& m) {
    for (int t = 0; t < 2; t++) {
        Team& T = g_world.teams[t == 0 ? m.S.home : m.S.away];
        for (int s : m.onField[t]) if (s < (int)T.squad.size()) T.squad[s].apps++;
    }
}

static void finishMatchToResult() {
    Match& m = *g_match;
    audioWalkoutStop();
    g_post = PostInfo();
    g_post.home = m.S.home; g_post.away = m.S.away;
    g_post.hg = m.score[0]; g_post.ag = m.score[1];
    g_post.aet = m.aet;
    if (m.shootout) { g_post.ph = m.pens[0]; g_post.pa = m.pens[1]; }
    for (auto& e : m.events) {
        EvLine l; l.side = e.team; l.minute = (int)e.minute + 1;
        l.type = e.type == 0 ? 0 : e.type == 3 ? 1 : e.type == 1 ? 2 : e.type == 4 ? 4 : 3;
        l.txt = e.player + (e.pen ? " (pen.)" : "") + (e.assist.empty() ? "" : " (passe : " + e.assist + ")") + (e.type == 3 ? " (csc)" : "") + (e.type == 4 ? " (blessé)" : "");
        g_post.ev.push_back(l);
    }
    float tot = m.possTime[0] + m.possTime[1];
    g_post.poss = tot > 0 ? m.possTime[0] / tot * 100 : 50;
    g_post.shots[0] = m.shots[0]; g_post.shots[1] = m.shots[1];
    g_post.title = m.S.title;
    g_post.hasStats = true; g_post.st = statsOf(m); g_postPage = 0;
    if (!m.abandoned && !m.S.training) { int tm = -1, sq = m.manOfMatch(tm); if (sq >= 0 && sq < (int)m.team(tm).squad.size()) g_post.motm = fmt("Homme du match : %s (%s) - note %.1f", m.team(tm).squad[sq].name.c_str(), m.team(tm).shortName.c_str(), m.playerRating(tm, sq)); }
    g_post.att = m.S.attendance; g_post.stadium = m.S.stadium; g_post.meteo = METEO_NAMES[std::max(0, std::min(5, m.S.meteo))];
    g_post.pitch = PITCH_NAMES[std::max(0, std::min(4, m.S.pitch))]; g_post.turf = m.S.turf; g_post.boost = m.S.homeBoost; g_post.neutral = m.S.neutral;
    {   // recette estimée (billetterie) ; remplacée par la recette réelle du club du joueur
        const Team& HT = g_world.teams[m.S.home];
        int cap = HT.sta.init ? HT.sta.capacity() : 0;
        int price = HT.kind == TK_NATION || m.S.neutral ? 60 : cap >= 20000 ? 25 : cap >= 8000 ? 15 : cap >= 3000 ? 8 : cap >= 1000 ? 5 : 3;
        g_post.gate = (int64_t)g_post.att * price;
    }
    g_lastGate.att = -1;
    applyCardsAndStats(m);
    if (m.S.hasFirstLeg) {
        int th = g_post.hg + m.S.aggHome, ta = g_post.ag + m.S.aggAway;
        int w = th != ta ? (th > ta ? 0 : 1) : (m.S.awayGoals && g_post.ag != m.S.aggHome) ? (g_post.ag > m.S.aggHome ? 1 : 0) : (g_post.ph > g_post.pa ? 0 : 1);
        bool byAway = th == ta && m.S.awayGoals && g_post.ag != m.S.aggHome;
        g_post.extra = fmt("Cumul : %d - %d  -  %s qualifié%s", th, ta, g_world.teams[w == 0 ? m.S.home : m.S.away].name.c_str(), byAway ? " (buts à l'extérieur)" : "");
    }
    if (g_mctx.career) {
        Season& S = g_career.season;
        Competition& C = S.comps[g_mctx.comp];
        MatchRes& r = C.matches[g_mctx.match];
        r.hg = (int16_t)g_post.hg; r.ag = (int16_t)g_post.ag; r.aet = g_post.aet;
        r.ph = (int16_t)g_post.ph; r.pa = (int16_t)g_post.pa;
        if (r.decisive && r.leg == 0 && r.hg == r.ag && g_post.ph < 0) { r.ph = 5; r.pa = 4; } // sécurité
        r.played = true;
        for (auto& e : m.events) {
            MEv x; x.match = g_mctx.match; x.minute = (uint8_t)std::min(125, (int)e.minute + 1); x.pid = e.pid; x.aid = e.aid; x.pen = e.pen;
            x.type = (uint8_t)(e.type == 0 ? 0 : e.type == 3 ? 1 : e.type == 1 ? 2 : e.type == 4 ? 4 : 3);
            x.side = (uint8_t)e.team;
            x.team = x.type == 1 ? (e.team == 0 ? r.away : r.home) : (e.team == 0 ? r.home : r.away);
            C.events.push_back(x);
        }
        g_post.comp = g_mctx.comp; g_post.match = g_mctx.match;
        S.recordResult(g_mctx.comp, g_mctx.match,true);
        g_career.museumEvents(g_mctx.comp,g_mctx.match);
        MuseumMatch actual;actual.exactLineup=1;actual.attendance=m.S.attendance;snprintf(actual.venue,sizeof actual.venue,"%s",m.S.stadium.c_str());
        for(int side=0;side<2;side++){const auto& T=g_world.teams[side?m.S.away:m.S.home];for(int slot=0;slot<11;slot++){const auto& mp=m.pl[side*11+slot];if(mp.onPitch&&mp.squad>=0&&mp.squad<(int)T.squad.size()){actual.xi[side][slot]=T.squad[mp.squad].id;snprintf(actual.lineupNames[side][slot],48,"%s",T.squad[mp.squad].name.c_str());}}int cap=m.captain[side];if(cap>=0&&cap<22)snprintf(actual.captain[side],64,"%s",m.playerName(cap).c_str());}
        g_career.museumEnrich(g_mctx.comp,g_mctx.match,actual);
        if(m.S.rules==RULESET_CAREER)for(auto& entry:m.positionMinutes)for(int ti:{m.S.home,m.S.away})for(auto& p:g_world.teams[ti].squad)if(p.id==entry.first.first)learnPosition(p,entry.first.second,(int)entry.second);
        // condition physique réelle : d'après l'endurance restante en fin de match
        for (int i = 0; i < 22; i++) {
            const MPlayer& p = m.pl[i];
            Team& T = g_world.teams[p.team == 0 ? m.S.home : m.S.away];
            if (p.squad < 0 || p.squad >= (int)T.squad.size() || p.sentOff) continue;
            float drop = std::max(0.f, p.stam0 - p.stamina) * 80.f * (p.gk ? 0.5f : 1.f);
            T.squad[p.squad].cond = (uint8_t)std::max(25, std::min(100, (int)(p.cond0 - drop)));
        }
        g_career.mgrAfterMatch(g_mctx.comp, g_mctx.match);
        if (g_lastGate.att >= 0) { g_post.att = g_lastGate.att; g_post.gate = g_lastGate.brut; g_post.realGate = true; }
        S.checkRound(g_mctx.comp);
        g_pending = PendingMatch();
        g_needAdvance = true;
    }
    g_match.reset();
    bool cupNews = makeArticle(g_post);
    if (g_mctx.career) g_career.season.news.push_back("Presse : " + g_articleTitle);
    g_screen = cupNews && g_mctx.career ? SC_ARTICLE : SC_POST;
    postJingle();
}

static void simulateUserMatch() {
    Season& S = g_career.season;
    Competition& C = S.comps[g_pending.comp];
    MatchRes& r = C.matches[g_pending.match];
    simulateMatch(r, &C);
    S.recordResult(g_pending.comp, g_pending.match);
    genMatchEvents(C, g_pending.match);
    S.finishRoundOthers(g_pending.comp, g_pending.match);
    g_post = PostInfo();
    g_post.home = r.home; g_post.away = r.away; g_post.hg = r.hg; g_post.ag = r.ag; g_post.ph = r.ph; g_post.pa = r.pa; g_post.aet = r.aet;
    g_post.ev = eventsFromComp(C, g_pending.match);
    g_post.comp = g_pending.comp; g_post.match = g_pending.match;
    int st = C.stageOfMatch(g_pending.match);
    std::string rn;
    if (st >= 0) for (auto& R : C.stages[st].rounds) for (int x : R.m) if (x == g_pending.match) rn = R.name;
    g_post.title = C.name + " - " + rn + " (simulé)";
    g_post.poss = -1;
    {
        int mi = g_pending.match;
        if (r.leg == 1 && r.tie >= 0 && st >= 0) {
            const Tie& t = C.stages[st].ties[r.tie];
            if (t.m1 >= 0 && t.m1 != mi) {
                const MatchRes& a = C.matches[t.m1];
                int th = r.hg + a.ag, ta = r.ag + a.hg;
                int w = th != ta ? (th > ta ? 0 : 1) : (C.awayGoals && r.ag != a.ag) ? (r.ag > a.ag ? 1 : 0) : (r.ph > r.pa ? 0 : 1);
                g_post.extra = fmt("Cumul : %d - %d  -  %s qualifié", th, ta, g_world.teams[w == 0 ? r.home : r.away].name.c_str());
            }
        }
    }
    g_career.mgrAfterMatch(g_pending.comp, g_pending.match);
    S.checkRound(g_pending.comp);
    g_pending = PendingMatch();
    g_needAdvance = true;
    g_mctx = MatchCtx(); g_mctx.career = true; g_mctx.back = SC_HUB;
    bool cupNews = makeArticle(g_post);
    S.news.push_back("Presse : " + g_articleTitle);
    g_screen = cupNews ? SC_ARTICLE : SC_POST;
    postJingle();
}

// feuille de match (buts, passes, cartons) en deux colonnes
static int drawEventColumns(const std::vector<EvLine>& ev, int y, int maxY) {
    int yl = y, yr = y;
    for (auto& e : ev) {
        int& yy = e.side == 0 ? yl : yr;
        if (yy > maxY) continue;
        int xx = e.side == 0 ? 24 : VW / 2 + 14;
        Color c = C_TXT;
        switch (e.type) {
        case 0: case 1:
            DrawCircle(xx + 3, yy + 5, 3, WHITE); DrawCircle(xx + 3, yy + 5, 1, BLACK);
            break;
        case 2: DrawRectangle(xx + 1, yy + 1, 5, 8, Color{ 255, 220, 0, 255 }); c = C_DIM; break;
        case 3: DrawRectangle(xx + 1, yy + 1, 5, 8, Color{ 230, 30, 30, 255 }); c = C_BAD; break;
        default: DrawRectangle(xx, yy + 4, 7, 2, C_BAD); DrawRectangle(xx + 2, yy + 2, 2, 6, C_BAD); c = C_DIM; break;
        }
        drawTextPx(fmt("%d'", e.minute), xx + 10, yy, 10, C_HI);
        drawTextPx(fitText(e.txt, VW / 2 - 70, 10), xx + 36, yy, 10, c);
        yy += 11;
    }
    return std::max(yl, yr);
}

static bool openMatchday(int comp, int mi, bool endOfDay);
static void postContinue() { if (g_mctx.career) { autosave(); if (!openMatchday(g_mctx.comp, g_mctx.match, true)) openHub(); } else g_screen = SC_MAIN; }
static void postExit() {
    static int shown = -1;     // une seule scène de vestiaire par match
    if (g_match && shown != g_matchSerial) { shown = g_matchSerial; openLocker(2); if (g_screen == SC_LOCKER) return; }
    postContinue();
}
static void screenPost() {
    drawBackground("Résultat");
    const Team& H = g_world.teams[g_post.home]; const Team& A = g_world.teams[g_post.away];
    // animation d'entrée : remise à zéro quand l'écran réapparaît
    static float postT = 0; static double postSeen = -10;
    if (GetTime() - postSeen > 0.5) postT = 0;
    postSeen = GetTime(); postT += GetFrameTime();
    int win = g_post.hg > g_post.ag || (g_post.hg == g_post.ag && g_post.ph > g_post.pa) ? 0 : g_post.ag > g_post.hg || (g_post.hg == g_post.ag && g_post.pa > g_post.ph) ? 1 : -1;
    {   // bandeau TV de fin de match : tribunes, panneaux aux couleurs des clubs, score en grand
        float t = (float)GetTime();
        int by = 44, bh = 36;
        DrawRectangleGradientV(8, by, VW - 16, bh, Color{ 18, 30, 70, 255 }, Color{ 40, 26, 60, 255 });
        fxCrowd(8, by + 2, VW - 16, bh - 4, hexc(H.home.shirt), hexc(A.home.shirt), t, win >= 0 ? 0.6f : 0.2f, (unsigned)(g_post.home + g_post.away * 3), 4);
        DrawRectangle(8, by, VW - 16, bh, Color{ 0, 0, 0, 120 });
        float in = std::min(1.f, postT * 3.f); float e = 1 - (1 - in) * (1 - in);
        for (int side = 0; side < 2; side++) {
            Color c = hexc(side ? A.home.shirt : H.home.shirt);
            int pw = VW / 2 - 56, px = side ? VW / 2 + 48 + (int)((1 - e) * 200) : 8 - (int)((1 - e) * 200);
            if (side == 0) DrawRectangleGradientH(px, by, pw, bh, Color{ c.r, c.g, c.b, 200 }, Color{ c.r, c.g, c.b, 40 });
            else DrawRectangleGradientH(px, by, pw, bh, Color{ c.r, c.g, c.b, 40 }, Color{ c.r, c.g, c.b, 200 });
            DrawRectangle(px, by, pw, 1, Color{ 255, 255, 255, 70 });
        }
        if (win >= 0) { Color cc[3] = { hexc(win ? A.home.shirt : H.home.shirt), WHITE, C_HI }; fxConfetti(win ? VW / 2 : 8, 30, VW / 2 - 8, 60, t, cc, 3, 40, 21); }
        // score : deux cases, celle du vainqueur en or
        for (int side = 0; side < 2; side++) {
            int sx = VW / 2 + (side ? 4 : -36), sy = by + 3;
            bool w = win == side;
            DrawRectangle(sx + 2, sy + 2, 32, 30, Color{ 0, 0, 0, 120 });
            DrawRectangleGradientV(sx, sy, 32, 30, w ? Color{ 255, 230, 110, 255 } : Color{ 245, 246, 250, 255 }, w ? Color{ 220, 170, 40, 255 } : Color{ 190, 196, 210, 255 });
            float pop = std::min(1.f, std::max(0.f, (postT - 0.25f - side * 0.15f) * 5.f));
            if (pop > 0) drawTextCentered(fmt("%d", side ? g_post.ag : g_post.hg), sx + 16, sy + 5, 20, Color{ 16, 20, 34, 255 }, false);
        }
        DrawRectangle(VW / 2 - 60, by + bh + 2, 120, 12, Color{ 200, 30, 40, 255 });
        drawTextCentered(g_post.ph >= 0 ? "TIRS AU BUT" : g_post.aet ? "APRÈS PROLONGATION" : "FIN DU MATCH", VW / 2, by + bh + 3, 10, WHITE, false);
        DrawRectangle(8, by + bh, (VW - 16) / 2, 2, hexc(H.home.shirt)); DrawRectangle(VW / 2, by + bh, (VW - 16) / 2, 2, hexc(A.home.shirt));
    }
    drawTextCentered(fitText(g_post.title, VW - 20, 10), VW / 2, 30, 10, C_DIM);
    drawKitIcon(H.home, 16, 49, 3); drawKitIcon(A.home, VW - 40, 49, 3);
    drawTextShadow(fitText(H.name, VW / 2 - 110, 20), 46, 47, 20, WHITE);
    { std::string an = fitText(A.name, VW / 2 - 110, 20); drawTextShadow(an, VW - 46 - textWidth(an, 20), 47, 20, WHITE); }
    drawTextPx(shortLevel(g_post.home) + (win == 0 ? "   VICTOIRE" : win == 1 ? "   DÉFAITE" : "   NUL"), 46, 68, 10, win == 0 ? C_GOOD : win == 1 ? C_BAD : C_DIM);
    { std::string al = std::string(win == 1 ? "VICTOIRE   " : win == 0 ? "DÉFAITE   " : "NUL   ") + shortLevel(g_post.away); drawTextPx(al, VW - 46 - textWidth(al, 10), 68, 10, win == 1 ? C_GOOD : win == 0 ? C_BAD : C_DIM); }
    int y = 98;
    if (g_post.ph >= 0) { drawTextCentered(fmt("Tirs au but : %d - %d", g_post.ph, g_post.pa), VW / 2, y, 10, C_HI); y += 12; }
    if (!g_post.extra.empty()) { drawTextCentered(g_post.extra, VW / 2, y, 10, C_GOOD); y += 12; }
    y += 4;
    drawTextCentered(g_post.hasStats?fmt("%s spectateurs",thousands(g_post.att).c_str()):"Affluence non archivée",VW/2,y,10,C_GOOD);y+=12;
    if(!g_post.motm.empty()){drawTextCentered(fitText(g_post.motm,VW-40,10),VW/2,y,10,C_HI);y+=12;}
    y+=4;
    if(g_post.hasStats){if(IN.left)g_postPage=(g_postPage+2)%3;if(IN.right)g_postPage=(g_postPage+1)%3;
      const char* tabs[]={"Feuille de match","Jeu et attaque","Discipline / reprises"};for(int i=0;i<3;i++){bool clicked=button(12+i*210,y,204,18,tabs[i]);if(i==g_postPage){DrawRectangle(12+i*210,y,204,18,C_SEL);drawTextCentered(tabs[i],114+i*210,y+4,10,BLACK);}if(clicked){g_postPage=i;return;}}y+=25;}

    if (g_post.hasStats && g_postPage > 0) {
        // rapport : statistiques, stade, météo, recette
        drawStatsTable(g_post.st, 24, y, VW / 2 - 40,g_postPage-1);
        int x2 = VW / 2 + 12, yy = y, w2 = VW / 2 - 32;
        DrawRectangle(x2 - 4, yy - 2, w2 + 8, 150, Color{ 6, 12, 28, 200 });
        drawTextPx("LE STADE", x2, yy, 10, C_HI); yy += 13;
        drawTextPx(fitText(g_post.stadium.empty() ? std::string("Terrain neutre") : g_post.stadium, w2, 10), x2, yy, 10, C_TXT); yy += 12;
        drawTextPx(fmt("Affluence : %s spectateurs", thousands(g_post.att).c_str()), x2, yy, 10, C_TXT); yy += 12;
        drawTextPx("Météo : " + g_post.meteo, x2, yy, 10, C_TXT); yy += 12;
        drawTextPx(fitText(fmt("Terrain : %s", g_post.pitch.c_str()) + (g_post.turf >= 0 ? fmt(", pelouse %d %%", g_post.turf) : std::string()), w2, 10), x2, yy, 10, C_TXT); yy += 14;
        drawTextPx("INFLUENCE DU STADE", x2, yy, 10, C_HI); yy += 13;
        std::string inf = g_post.neutral ? std::string("Terrain neutre : aucun avantage pour l'une ou l'autre équipe.")
                        : fmt("Le public a porté %s : +%.1f %% sur les qualités des joueurs.", H.name.c_str(), g_post.boost * 100.f);
        for (auto& l : wrapText(inf, w2)) { drawTextPx(l, x2, yy, 10, g_post.neutral ? C_DIM : C_GOOD); yy += 11; }
        yy += 4;
        drawTextPx("RECETTE DU MATCH", x2, yy, 10, C_HI); yy += 13;
        if (g_post.gate >= 0) drawTextPx(fmt("%s (%s)", money(g_post.gate / 1000).c_str(), g_post.realGate ? "billetterie du club" : "estimation"), x2, yy, 10, C_TXT);
        drawFooter("OK : continuer   Gauche/Droite : feuille de match   Tab : lire l'article");
        if (IN.tab) { g_screen = SC_ARTICLE; return; }
        if (IN.ok || IN.back || IN.click || IN.start) postExit();
        return;
    }
    DrawRectangle(VW / 2, y, 1, 150, Color{ 255, 255, 255, 40 });
    y = drawEventColumns(g_post.ev, y, VH - 100) + 8;
    if (g_post.poss >= 0) {   // barres comparatives façon TV
        int hp = (int)std::lround(g_post.poss);
        auto bar = [&](const char* lab, int a, int b, int yy) {
            int bw = 150, cx = VW / 2;
            float tot = std::max(1, a + b), ga = std::min(1.f, postT * 1.5f);
            int wa = (int)(bw * a / tot * ga), wb = (int)(bw * b / tot * ga);
            DrawRectangle(cx - 40 - bw, yy + 2, bw, 7, Color{ 0, 0, 0, 120 }); DrawRectangle(cx + 40, yy + 2, bw, 7, Color{ 0, 0, 0, 120 });
            DrawRectangle(cx - 40 - wa, yy + 2, wa, 7, hexc(H.home.shirt)); DrawRectangle(cx + 40, yy + 2, wb, 7, hexc(A.home.shirt));
            DrawRectangle(cx - 40 - wa, yy + 2, wa, 1, Color{ 255, 255, 255, 90 }); DrawRectangle(cx + 40, yy + 2, wb, 1, Color{ 255, 255, 255, 90 });
            drawTextCentered(lab, cx, yy, 10, C_DIM, false);
            std::string sa = strcmp(lab, "Possession") == 0 ? fmt("%d%%", a) : fmt("%d", a), sb = strcmp(lab, "Possession") == 0 ? fmt("%d%%", b) : fmt("%d", b);
            drawTextPx(sa, cx - 46 - bw - textWidth(sa, 10), yy, 10, C_TXT); drawTextPx(sb, cx + 46 + bw, yy, 10, C_TXT);
        };
        bar("Possession", hp, 100 - hp, y); y += 12;
        bar("Tirs", g_post.shots[0], g_post.shots[1], y);
    }
    // gros titre de la presse
    DrawRectangle(20, VH - 36, VW - 40, 18, Color{ 238, 232, 214, 255 });
    drawTextPx(fitText("PRESSE : " + g_articleTitle, VW - 60, 10), 28, VH - 32, 10, Color{ 30, 30, 30, 255 });
    drawFooter(g_post.hasStats ? "OK : continuer   Gauche/Droite : statistiques, stade et recette   Tab : article" : "OK : continuer   Tab : lire l'article");
    if (IN.tab) { g_screen = SC_ARTICLE; return; }
    if (IN.ok || IN.back || IN.click || IN.start) postExit();
}

// ------------------------------------------------------------------ détail d'un match (depuis les résultats)
static int g_miComp = -1, g_miMatch = -1; static Screen g_miBack = SC_HUB;
static std::vector<EvLine> g_miEv;
static void openMatchInfo(int comp, int match, Screen back) {
    g_miComp = comp; g_miMatch = match; g_miBack = back;
    g_miEv = eventsFromComp(g_career.season.comps[comp], match);
    g_screen = SC_MATCHINFO;
}
static void screenMatchInfo() {
    const Competition& C = g_career.season.comps[g_miComp];
    const MatchRes& m = C.matches[g_miMatch];
    drawBackground("Feuille de match");
    std::string rn = roundNameOf(C, g_miMatch);
    double tm = 0; for (auto& st : C.stages) for (auto& R : st.rounds) for (int x : R.m) if (x == g_miMatch) tm = R.time;
    drawTextCentered(fitText(C.name + (rn.empty() ? "" : " - " + rn) + "  -  " + dateOf(tm, g_career.year), VW - 20, 10), VW / 2, 34, 10, C_DIM);
    for (int side = 0; side < 2; side++) {
        int t = side ? m.away : m.home;
        int cx = side ? VW - 170 : 170;
        bool hover = IN.mouse.x >= cx - 110 && IN.mouse.x < cx + 110 && IN.mouse.y >= 46 && IN.mouse.y < 80;
        if (hover) DrawRectangle(cx - 110, 46, 220, 34, Color{ 60, 90, 150, 120 });
        drawKitIcon(g_world.teams[t].home, side ? VW - 64 : 40, 50, 3);
        drawTextCentered(fitText(g_world.teams[t].name, 200, 10), cx, 54, 10, C_TXT);
        drawTextCentered(shortLevel(t), cx, 66, 10, C_DIM, false);
        if (hover && IN.click) { openFiche(t, SC_MATCHINFO); return; }
    }
    drawTextCentered(m.played ? fmt("%d - %d", m.hg, m.ag) : std::string("-"), VW / 2, 52, 20, C_HI);
    int y = 84;
    if (m.played && m.aet) { drawTextCentered("après prolongation", VW / 2, y, 10, C_DIM); y += 12; }
    if (m.played && m.ph >= 0) { drawTextCentered(fmt("Tirs au but : %d - %d", m.ph, m.pa), VW / 2, y, 10, C_HI); y += 12; }
    const MatchRes* a = firstLegOf(C, g_miMatch);
    if (a && a->played) {
        drawTextCentered(fmt("Match aller : %s %d - %d %s", g_world.teams[a->home].shortName.c_str(), a->hg, a->ag, g_world.teams[a->away].shortName.c_str()), VW / 2, y, 10, C_TXT);
        y += 12;
        if (m.played) { drawTextCentered(fmt("Cumul : %d - %d", m.hg + a->ag, m.ag + a->hg), VW / 2, y, 10, C_GOOD); y += 12; }
    }
    std::string st = m.neutral ? "Terrain neutre" : g_world.teams[m.home].stadium;
    if (g_careerActive && g_career.kind == CK_INTL) { std::string v = matchVenue(g_miComp, g_miMatch); if (!v.empty()) st = v; }
    if (!st.empty()) { drawTextCentered(st, VW / 2, y, 10, C_DIM, false); y += 12; }
    y += 4;
    if (!m.played) drawTextCentered("Match pas encore joué", VW / 2, y + 10, 10, C_DIM);
    else if (g_miEv.empty()) drawTextCentered("Aucun événement enregistré", VW / 2, y + 10, 10, C_DIM);
    else { DrawRectangle(VW / 2, y, 1, 150, Color{ 255, 255, 255, 40 }); drawEventColumns(g_miEv, y, VH - 30); }
    drawFooter("Clic sur une équipe : fiche du club   G / D : fiche domicile / extérieur   Retour");
    if (IN.left) { openFiche(m.home, SC_MATCHINFO); return; }
    if (IN.right) { openFiche(m.away, SC_MATCHINFO); return; }
    if (IN.back || IN.ok) g_screen = g_miBack;
}

// ------------------------------------------------------------------ carrière : écran principal
static ListW g_hubLW;
static ListW g_slotLW;
static void openHub() { g_screen = SC_HUB; g_hubLW = ListW(); levelCacheDirty(); if (g_career.coach && g_career.kind == CK_INTL) g_career.coachApplySquad(); }

// ------------------------------------------------------------------ programme de la journée / fin de journée (calendrier réel)
struct MdRow { int mi; int ko; };
static std::vector<MdRow> g_mdRows;
static int g_mdComp = -1, g_mdRef = -1; static bool g_mdEnd = false; static ListW g_mdLW;
static std::map<int, int> g_mdPrevRank;     // position de chaque équipe avant la journée
static bool openMatchday(int comp, int mi, bool endOfDay) {
    Season& S = g_career.season;
    if (comp < 0 || comp >= (int)S.comps.size() || mi < 0) return false;
    const Competition& C = S.comps[comp];
    const Round* rd = nullptr;
    for (auto& st : C.stages) for (auto& R : st.rounds) for (int x : R.m) if (x == mi) rd = &R;
    if (!rd || rd->m.size() < 2) return false;
    g_mdRows.clear();
    for (int x : rd->m) g_mdRows.push_back({ x, kickoffMinutes(comp, x) });
    std::stable_sort(g_mdRows.begin(), g_mdRows.end(), [&](const MdRow& a, const MdRow& b) {
        const MatchRes& A = C.matches[a.mi]; const MatchRes& B = C.matches[b.mi];
        if (a.ko != b.ko) return a.ko < b.ko;
        return A.group < B.group; });
    g_mdComp = comp; g_mdRef = mi; g_mdEnd = endOfDay; g_mdLW = ListW();
    // classement avant la journée (flèches d'évolution)
    g_mdPrevRank.clear();
    {
        int st = C.stageOfMatch(mi);
        if (st >= 0 && C.stages[st].type != ST_KO && !C.stages[st].groups.empty()) {
            Competition cp;
            cp.tb = C.tb; cp.ptsWin = C.ptsWin; cp.ptsDraw=C.ptsDraw; cp.ptsLoss=C.ptsLoss; cp.ptsForfeit=C.ptsForfeit; cp.stages = C.stages; cp.matches = C.matches;
            for (int x : rd->m) cp.matches[x].played = false;
            int g = std::max(0, (int)C.matches[mi].group);
            if (C.stages[st].type == ST_SWISS) g = 0;
            auto tb = cp.table(st, std::min(g, (int)C.stages[st].groups.size() - 1));
            for (int i = 0; i < (int)tb.size(); i++) g_mdPrevRank[tb[i].team] = i;
        }
    }
    g_screen = SC_MATCHDAY;
    return true;
}

static void screenMatchday() {
    Season& S = g_career.season;
    if (g_mdComp < 0 || g_mdComp >= (int)S.comps.size()) { openHub(); return; }
    const Competition& C = S.comps[g_mdComp];
    drawBackground(g_mdEnd ? "Fin de journée" : "Programme de la journée");
    int st = C.stageOfMatch(g_mdRef);
    std::string rn;
    if (st >= 0) for (auto& R : C.stages[st].rounds) for (int x : R.m) if (x == g_mdRef) rn = R.name;
    drawTextCentered(fitText(C.name + (rn.empty() ? "" : "  -  " + rn), VW - 40, 10), VW / 2, 34, 10, C_HI);
    int refKo = kickoffMinutes(g_mdComp, g_mdRef);
    // lignes : en-têtes de créneau + rencontres
    struct L { int kind; int mi; int ko; };
    std::vector<L> lines;
    int lastKo = -1;
    for (auto& r : g_mdRows) { if (r.ko != lastKo) { lines.push_back({ 0, -1, r.ko }); lastKo = r.ko; } lines.push_back({ 1, r.mi, r.ko }); }
    // classement du championnat (ou du groupe) à droite, s'il y en a un
    bool withTable = st >= 0 && C.stages[st].type != ST_KO && !C.stages[st].groups.empty();
    if (withTable) {
        int g = C.stages[st].type == ST_SWISS ? 0 : std::max(0, std::min((int)C.matches[g_mdRef].group, (int)C.stages[st].groups.size() - 1));
        auto tb = C.table(st, g);
        int tx = 352, tw = VW - tx - 10, ty = 50, th = 11;
        DrawRectangle(tx - 4, ty - 4, tw + 8, VH - 24 - ty + 8, Color{ 6, 12, 28, 170 });
        std::string head = C.stages[st].type == ST_SWISS ? "CLASSEMENT DE LA PHASE DE LIGUE" : C.stages[st].groups.size() > 1 ? fmt("CLASSEMENT - GROUPE %c", 'A' + g) : "CLASSEMENT";
        DrawRectangle(tx, ty - 2, tw, 11, Color{ 30, 50, 100, 255 }); DrawRectangle(tx, ty + 8, tw, 1, C_SEL); drawTextPx(head, tx + 4, ty - 2, 10, C_HI);
        ty += 12;
        drawTextPx("J", tx + tw - 76, ty, 10, C_DIM); drawTextPx("Diff", tx + tw - 58, ty, 10, C_DIM); drawTextPx("Pts", tx + tw - 22, ty, 10, C_DIM);
        ty += 11;
        int n = (int)tb.size(), rows = (VH - 26 - ty) / th;
        int me = -1; for (int i = 0; i < n; i++) if (S.isControlled(tb[i].team)) me = i;
        int first = 0; if (n > rows) first = std::max(0, std::min(n - rows, me - rows / 2));
        for (int i = first; i < n && i < first + rows; i++) {
            const Standing& sd = tb[i];
            bool mine = S.isControlled(sd.team);
            DrawRectangle(tx, ty, tw, th - 1, mine ? Color{ 110, 90, 20, 255 } : (i % 2 ? C_ITEM : Color{ 38, 62, 108, 255 }));
            drawTextPx(fmt("%2d", i + 1), tx + 2, ty, 10, mine ? C_HI : C_DIM);
            auto pr = g_mdPrevRank.find(sd.team);
            if (pr != g_mdPrevRank.end() && pr->second != i) {
                bool up = i < pr->second;
                Color ac = up ? C_GOOD : C_BAD;
                int ax = tx + 20, ay = ty + 5;
                if (up) DrawTriangle(Vector2{ (float)ax, (float)ay - 3 }, Vector2{ (float)ax - 3, (float)ay + 2 }, Vector2{ (float)ax + 3, (float)ay + 2 }, ac);
                else DrawTriangle(Vector2{ (float)ax - 3, (float)ay - 2 }, Vector2{ (float)ax, (float)ay + 3 }, Vector2{ (float)ax + 3, (float)ay - 2 }, ac);
            }
            drawKitIcon(g_world.teams[sd.team].home, tx + 26, ty + 1, 1);
            drawTextPx(fitText(g_world.teams[sd.team].name, tw - 118, 10), tx + 38, ty, 10, mine ? C_HI : C_TXT);
            drawTextPx(fmt("%d", sd.p), tx + tw - 76, ty, 10, C_DIM);
            drawTextPx(fmt("%+d", sd.gd()), tx + tw - 58, ty, 10, C_DIM);
            drawTextPx(fmt("%d", sd.pts), tx + tw - 22, ty, 10, mine ? C_HI : C_TXT);
            ty += th;
        }
    }
    int y0 = 50, lh = 12, vis = (VH - 20 - y0) / lh;
    int maxTop = std::max(0, (int)lines.size() - vis);
    if (IN.down) g_mdLW.top = std::min(maxTop, g_mdLW.top + 1);
    if (IN.up) g_mdLW.top = std::max(0, g_mdLW.top - 1);
    if (IN.pgDn) g_mdLW.top = std::min(maxTop, g_mdLW.top + vis);
    if (IN.pgUp) g_mdLW.top = std::max(0, g_mdLW.top - vis);
    if (GetMouseWheelMove() != 0) g_mdLW.top = std::max(0, std::min(maxTop, g_mdLW.top - (int)GetMouseWheelMove() * 3));
    int y = y0;
    int rx = withTable ? 10 : 60, rw = withTable ? 332 : VW - 120;
    for (int i = g_mdLW.top; i < (int)lines.size() && i < g_mdLW.top + vis; i++) {
        const L& l = lines[i];
        if (l.kind == 0) {
            std::string lab = kickoffText(g_mdComp, [&]() { for (auto& r : g_mdRows) if (r.ko == l.ko) return r.mi; return g_mdRef; }(), true);
            std::string tag = l.ko < refKo ? "avant votre match" : l.ko == refKo ? "en même temps que votre match" : "après votre match";
            DrawRectangle(rx, y + 1, rw, lh - 1, Color{ 16, 30, 64, 255 });
            DrawRectangle(rx, y + lh - 1, rw, 1, Color{ 240, 200, 60, 120 });
            drawTextPx(lab, rx + 6, y + 1, 10, C_HI);
            drawTextPx(tag, rx + rw - 6 - textWidth(tag, 10), y + 1, 10, C_DIM);
        } else {
            const MatchRes& m = C.matches[l.mi];
            bool mine = S.isControlled(m.home) || S.isControlled(m.away);
            bool hover = IN.mouse.x >= rx && IN.mouse.x < rx + rw && IN.mouse.y >= y && IN.mouse.y < y + lh;
            DrawRectangle(rx, y, rw, lh - 1, mine ? Color{ 110, 90, 20, 255 } : hover ? Color{ 60, 90, 150, 255 } : (i % 2 ? C_ITEM : Color{ 38, 62, 108, 255 }));
            if (m.group >= 0 && C.stages.size() && st >= 0 && C.stages[st].groups.size() > 1) drawTextPx(fmt("%c", 'A' + m.group % 26), rx + 4, y + 1, 10, C_DIM);
            drawKitIcon(g_world.teams[m.home].home, rx + 16, y + 2, 1);
            std::string a = fitText(withTable ? g_world.teams[m.home].name : nameLvl(m.home), rw / 2 - 40, 10), b = fitText(withTable ? g_world.teams[m.away].name : nameLvl(m.away), rw / 2 - 40, 10);
            drawTextPx(a, rx + rw / 2 - 30 - textWidth(a, 10), y + 1, 10, mine ? C_HI : C_TXT);
            std::string sc = m.played ? fmt("%d - %d", m.hg, m.ag) : (mine ? "à jouer" : "à venir");
            if (m.played && m.ph >= 0) sc += fmt(" (%d-%d tab)", m.ph, m.pa);
            else if (m.played && m.aet) sc += " ap";
            drawTextCentered(sc, rx + rw / 2, y + 1, 10, m.played ? C_GOOD : C_DIM, false);
            drawTextPx(b, rx + rw / 2 + 30, y + 1, 10, mine ? C_HI : C_TXT);
            drawKitIcon(g_world.teams[m.away].home, rx + rw - 24, y + 2, 1);
            if (hover && IN.click) { openMatchInfo(g_mdComp, l.mi, SC_MATCHDAY); return; }
        }
        y += lh;
    }
    if (maxTop > 0) drawTextPx(fmt("%d/%d", g_mdLW.top + 1, maxTop + 1), VW - 50, 36, 10, C_DIM);
    drawFooter(g_mdEnd ? "Haut/Bas : défiler   Clic : détails du match   Entrée : continuer" : "Haut/Bas : défiler   Clic : détails du match   Retour");
    if (IN.ok || IN.back || IN.start) openHub();
}

static std::vector<std::pair<int, int>> userMatches(bool playedOnly) {
    std::vector<std::pair<int, int>> v;
    Season& S = g_career.season;
    for (int c = 0; c < (int)S.comps.size(); c++) {
        const Competition& C = S.comps[c];
        for (int mi = 0; mi < (int)C.matches.size(); mi++) {
            const MatchRes& m = C.matches[mi];
            if (!S.isControlled(m.home) && !S.isControlled(m.away)) continue;
            if (playedOnly && !m.played) continue;
            v.push_back({ c, mi });
        }
    }
    auto timeOf = [&](const std::pair<int, int>& p) {
        const Competition& C = S.comps[p.first];
        for (auto& st : C.stages) for (auto& R : st.rounds) for (int x : R.m) if (x == p.second) return R.time;
        return 0.0;
    };
    std::stable_sort(v.begin(), v.end(), [&](const std::pair<int, int>& a, const std::pair<int, int>& b) { return timeOf(a) < timeOf(b); });
    return v;
}

static std::string roundNameOf(const Competition& C, int mi) {
    for (auto& st : C.stages) for (auto& R : st.rounds) for (int x : R.m) if (x == mi) return R.name;
    return "";
}

static void openMatchInfo(int comp, int match, Screen back);
static void openMarket();
static void autosave();
static int g_hubSel = 0, g_hubCategory=0;
static ListW g_hubMenu;

// premier match d'une confrontation aller-retour (score à rappeler)
static const MatchRes* firstLegOf(const Competition& C, int mi) {
    const MatchRes& m = C.matches[mi];
    if (m.leg != 1 || m.tie < 0) return nullptr;
    int st = C.stageOfMatch(mi);
    if (st < 0 || m.tie >= (int)C.stages[st].ties.size()) return nullptr;
    const Tie& t = C.stages[st].ties[m.tie];
    if (t.m1 < 0 || t.m1 == mi) return nullptr;
    return &C.matches[t.m1];
}

static void drawSection(int x, int y, int w, const std::string& t) {
    drawTextPx(t, x + 2, y, 10, C_HI);
    DrawRectangle(x + textWidth(t, 10) + 6, y + 5, w - textWidth(t, 10) - 8, 1, Color{ 240, 200, 60, 120 });
}

#include "app_press.inc"
#include "app_media.inc"
static void screenHub() {
    if(showSupporterParty(SC_HUB))return;
    if(g_career.sportingMode()){openSporting(0);return;}
    Season& S = g_career.season;
    if(!g_confirm.active && g_career.ballonPending()>=0){openBallon();return;}
    // début de saison : inscription (facultative en district) à la Coupe de France
    if (g_career.kind == CK_CLUB && !g_career.euroOnly && g_career.cdfAskYear != g_career.year && !g_confirm.active) {
        g_career.cdfAskYear = g_career.year;
        int c = g_career.userCdfComp();
        if (c >= 0) {
            std::string club = g_world.teams[g_career.userTeam].name;
            askConfirm(fmt("Coupe de France %d-%02d : en division de district, l'inscription est facultative. Inscrire %s ?", g_career.year, (g_career.year + 1) % 100, club.c_str()),
                [club]() { toast(club + " est inscrit à la Coupe de France"); }, "Inscrire", "Ne pas inscrire",
                [c, club]() { g_career.withdrawFromCup(c, g_career.userTeam); g_career.season.news.push_back(club + " ne s'inscrit pas à la Coupe de France cette saison."); toast("Pas d'inscription à la Coupe de France"); });
        }
    }
    {   // début de saison : récapitulatif des engagements de toutes les équipes du club
        static int shownYear = -1;
        if (g_career.kind == CK_CLUB && !g_career.euroOnly && !g_confirm.active && g_career.year != shownYear && S.now < 0.6 && g_career.userTeam >= 0) { shownYear = g_career.year; g_screen = SC_SEASONSTART; return; }
    }
    if (g_needAdvance && !g_confirm.active) {
        crashMark("carrière : avance du calendrier (saison %d)", g_career.year);
        g_pending = S.advance(false);
        g_needAdvance = false;
        g_career.mercatoPress();
        monthlyAwards();
        lifeCheckPromotion();
        g_career.mgrTick();
        g_career.directorTick();
        if(g_career.ballonPending()>=0){openBallon();return;}
        if (collectDraws()) { g_screen = SC_DRAW; return; }
    }
    if (!g_confirm.active) {   // coupures de journal (vainqueurs de coupes), puis invitation dans les médias
        int cn = cupNewsPending();
        if (cn >= 0) { openCupNews(cn); return; }
        if (mediaInviteDue()) { openMediaInvite(); return; }
    }
    bool euro = g_career.kind == 0 && g_career.euroOnly;
    bool club = g_career.kind == 0 && !euro;
    if (club && g_career.mgr.sacked) { g_screen = SC_JOBS; return; }
    if (club && g_career.mgr.managerMode && g_career.mgr.needStatus && g_career.forcedStatus(g_career.userTeam) < 0) { g_statusLW = ListW(); g_statusLW.cur = g_world.teams[g_career.userTeam].status == CS_AMATEUR ? 1 : 0; g_screen = SC_STATUS; return; }
    std::string title = club ? g_world.teams[g_career.userTeam].name : euro ? fmt("Coupes d'Europe %d-%02d", g_career.year, (g_career.year + 1) % 100) : g_career.kind == CK_CUSTOM ? g_career.custom.name : fmt("%s %d", INTL_NAMES[g_career.intlType], g_career.year);
    drawBackground(title);
    int y = 31;
    // ---- bandeau
    if (club) {
        const Team& U = g_world.teams[g_career.userTeam];
        drawKitIcon(U.home, 10, y + 2, 2);
        drawTextPx(fitText(g_career.teamLevelName(g_career.userTeam), 300, 10), 32, y, 10, C_TXT);
        drawTextPx(fmt("Saison %d-%02d  -  %s", g_career.year, (g_career.year + 1) % 100, dateOf(S.now, g_career.year).c_str()), 32, y + 11, 10, C_DIM);
        const ManagerState& M = g_career.mgr;
        drawTextPx("Budget : " + money(M.budget), 360, y, 10, M.budget >= 0 ? C_GOOD : C_BAD);
        drawTextPx(fmt("Confiance %d%%", M.confidence), 520, y, 10, M.confidence >= 50 ? C_GOOD : M.confidence >= 25 ? C_HI : C_BAD);
        {
            std::string ob = "Objectif : " + g_career.objectiveText();
            int pp, qq, gg;
            if (g_career.tierOfTeam(g_career.userTeam, &pp, &qq, &gg) >= 0) {
                const Competition& LC = S.comps[g_career.pyramids[pp].pools[qq].comps[gg]];
                auto tb = LC.table(0, 0);
                for (int i = 0; i < (int)tb.size(); i++) if (tb[i].team == g_career.userTeam && tb[i].p > 0) {
                    ob += fmt(" - actuel : %d%s", i + 1, i == 0 ? "er" : "e");
                }
            }
            bool okc = true; { size_t k = ob.find("actuel : "); if (k != std::string::npos) okc = atoi(ob.c_str() + k + 9) <= M.objTarget; }
            drawTextPx(fitText(ob, 270, 10), 360, y + 11, 10, okc ? C_DIM : C_BAD);
        }
        drawTextPx(fitText(g_career.windowText(), 300, 10), 32, y + 22, 10, g_career.transferWindow() ? C_GOOD : C_DIM);
        if (!g_career.managerName.empty() && g_hubNotice.empty()) drawTextPx(fitText("Manager : " + g_career.managerName, 270, 10), 360, y + 22, 10, C_DIM);
        if (!U.sponsor.empty()) { Texture2D* lg = sponsorLogoFwd(U.sponsor); if (lg) drawLogo2(lg, VW - 70, y + 12, 60, 18); }
    } else {
        std::string sel;
        for (int t : S.controlled) sel += g_world.teams[t].name + "  ";
        drawTextPx(fitText("Vos équipes : " + sel, VW - 24, 10), 12, y, 10, C_TXT);
        drawTextPx(euro ? std::string(g_career.opts.euroFormat ? "C1, C3 et C4 (nouvelle formule)  -  " : "Ligue des champions et Coupe UEFA  -  ") + dateOf(S.now, g_career.year) : g_career.kind == CK_CUSTOM ? "Compétition personnalisée" : g_career.finalComp >= 0 ? "Phase finale" : "Qualifications", 12, y + 11, 10, C_DIM);
    }
    if (!g_hubNotice.empty()) drawTextPx(fitText(g_hubNotice, club ? 270 : 600, 10), club ? 360 : 12, y + 22, 10, C_HI);
    y = 66;
    // ---- prochain match
    DrawRectangle(8, y, VW - 16, 48, Color{ 10, 18, 36, 220 });
    DrawRectangleLines(8, y, VW - 16, 48, Color{ 240, 200, 60, 90 });
    if (g_pending.comp >= 0) {
        const Competition& C = S.comps[g_pending.comp];
        const MatchRes& m = C.matches[g_pending.match];
        std::string rn = roundNameOf(C, g_pending.match);
        double tm = 0; for (auto& st : C.stages) for (auto& R : st.rounds) for (int x : R.m) if (x == g_pending.match) tm = R.time;
        drawTextPx(fitText("PROCHAIN MATCH  -  " + C.name + (rn.empty() ? "" : " - " + rn) + "  -  " + (g_career.kind == CK_CLUB ? kickoffText(g_pending.comp, g_pending.match, true) : dateOf(tm, g_career.year)), VW - 40, 10), 14, y + 3, 10, C_DIM);
        for (int side = 0; side < 2; side++) {
            int t = side ? m.away : m.home;
            int bx = side ? VW / 2 + 24 : 14, bw = VW / 2 - 40;
            bool hover = IN.mouse.x >= bx && IN.mouse.x < bx + bw && IN.mouse.y >= y + 15 && IN.mouse.y < y + 44;
            if (hover) DrawRectangle(bx, y + 15, bw, 29, Color{ 60, 90, 150, 120 });
            drawKitIcon(g_world.teams[t].home, side ? VW - 36 : bx + 2, y + 18, 2);
            int tx = side ? bx + 4 : bx + 24;
            drawTextPx(fitText(g_world.teams[t].name, bw - 30, 10), tx, y + 18, 10, S.isControlled(t) ? C_HI : C_TXT);
            std::string lv = shortLevel(t);
            if (!lv.empty()) drawTextPx(fitText(lv, bw - 100, 10), tx, y + 30, 10, C_DIM);
            if (g_career.kind == CK_CLUB) { std::string f = teamForm(t); if (!f.empty()) drawForm(f, side ? bx + bw - 66 - 30 : bx + bw - 62, y + 30); }   // forme : 5 derniers matchs
            if (hover && IN.click) { openFiche(t, SC_HUB); return; }
        }
        const MatchRes* a = firstLegOf(C, g_pending.match);
        if (a && a->played) {
            drawTextCentered("RETOUR", VW / 2, y + 16, 10, C_HI, false);
            drawTextCentered(fmt("Aller : %d-%d", a->ag, a->hg), VW / 2, y + 30, 10, C_TXT, false);
        } else drawTextCentered(m.leg == 1 ? "retour" : "contre", VW / 2, y + 22, 10, C_HI, false);
    } else drawTextPx(S.finished ? "Saison terminée" : "Aucun match", 16, y + 18, 10, C_TXT);
    y += 54;
    // ---- menu (sections)
    struct HI { std::string label; int id; };
    std::vector<std::pair<std::string, std::vector<HI>>> secs;
    std::vector<HI> m1;
    bool lifeP = club && g_career.life.isPlayer;
    if (g_pending.comp >= 0) {
        m1.push_back({ "Jouer le match", 0 }); m1.push_back({ "Simuler le match", 1 }); m1.push_back({ "Programme de la journée", 22 });
        if (club && !g_career.opts.disableBribes) m1.push_back({ "Valise à l'arbitre...", 41 });
    }
    else if (S.finished) m1.push_back({ club ? "Bilan de fin de saison" : "Bilan du tournoi", 2 });
    if (!m1.empty()) secs.push_back({ "MATCH", m1 });
    if (club) secs.push_back({ "COMPÉTITIONS", { { "Classements, résultats, buteurs", 3 }, { "Calendrier de l'équipe", 4 }, { "Engagements de la saison", 44 }, { "Archives des compétitions", 43 }, { "Coefficients UEFA", 10 }, { "Arbitres", 19 } } });
    else if (euro) secs.push_back({ "COMPÉTITIONS", { { "Classements, résultats, buteurs", 3 }, { "Calendrier de l'équipe", 4 }, { "Coefficients UEFA", 10 } } });
    else secs.push_back({ "COMPÉTITIONS", { { "Classements, résultats, buteurs", 3 }, { "Calendrier de l'équipe", 4 } } });
    if (lifeP) secs.push_back({ "MA CARRIÈRE", { { "Ma fiche", 42 }, { "Vie du joueur (argent, couple, paris)", 40 }, { "Effectif du club", 5 }, { fmt("Messages (%d non lu%s)", newsUnread(), newsUnread() > 1 ? "s" : ""), 14 } } });
    else if (club) secs.push_back({ "CLUB", { { "Effectif, tactique et contrats", 5 }, { "Gestion du club", 20 }, { "Vie privée du manager", 40 }, { fmt("Messages (%d non lu%s)", newsUnread(), newsUnread() > 1 ? "s" : ""), 14 } } });
    else if (g_career.coach) secs.push_back({ "SÉLECTION", { { "Convocations", 30 }, { "Effectif et tactique", 5 }, { "Organisation et stades", 31 }, { "Bilan du sélectionneur", 32 }, { fmt("Messages (%d non lu%s)", newsUnread(), newsUnread() > 1 ? "s" : ""), 14 } } });
    else secs.push_back({ "ÉQUIPE", { { "Effectif et tactique", 5 }, { "Fiche de l'équipe", 9 } } });
    if(club && !lifeP && !g_career.managerLifeEnabled()) { auto& v=secs.back().second; v.erase(std::remove_if(v.begin(),v.end(),[](const HI& x){return x.id==40;}),v.end()); }
    if(club) {int n=g_world.nationIndex(g_career.selectedCountryCode().c_str());if(n>=0 && NATIONS[n].conf!=UEFA)for(auto& sec:secs)sec.second.erase(std::remove_if(sec.second.begin(),sec.second.end(),[](const HI& x){return x.id==10;}),sec.second.end());}
    std::vector<HI> honours={{"Palmarès des compétitions",6}};
    if(g_career.ballonEnabled())honours.push_back({"Ballon d'or : cérémonie et palmarès",45});
    if(g_career.awardsEnabled())honours.push_back({"Récompenses du championnat",46});
    if(g_career.museumEnabled())honours.push_back({"Musée et histoire du club",48});
    secs.push_back({"PALMARÈS & RÉCOMPENSES",honours});
    if(g_career.polesEnabled())secs.push_back({"FORMATION",{{"Pôles Espoirs : tournoi et recrutement",47}}});
    secs.push_back({ "PARTIE", { { "Sauvegarder", 7 }, { "Options", 21 }, { "Menu principal", 8 } } });
    std::vector<HI> flat;
    for (auto& sc : secs) for (auto& it : sc.second) flat.push_back(it);
    // Categories occupy a fixed grid. Only the selected category's options are
    // rendered; long future categories scroll inside the available panel height.
    g_hubCategory=std::clamp(g_hubCategory,0,(int)secs.size()-1);
    if(IN.left||IN.right||IN.tab){g_hubCategory=(g_hubCategory+(IN.left?(int)secs.size()-1:1))%secs.size();g_hubMenu=ListW();}
    const int mw=230;
    for(int c=0;c<(int)secs.size();c++){
        std::string title=secs[c].first=="PALMARÈS & RÉCOMPENSES"?"PALMARES":stripUpperAccents(secs[c].first);
        if(button(8+(c%2)*118,y+(c/2)*20,112,18,fitText(title,102,10),c==g_hubCategory)){g_hubCategory=c;g_hubMenu=ListW();}
    }
    int menuY=y+((int)secs.size()+1)/2*20+6;
    drawSection(8,menuY,mw,fitText(stripUpperAccents(secs[g_hubCategory].first),mw-12,10));menuY+=14;
    int offset=0;for(int c=0;c<g_hubCategory;c++)offset+=(int)secs[c].second.size();
    auto& options=secs[g_hubCategory].second;
    int rows=std::max(1,(VH-24-menuY)/18);
    int selected=listRun(g_hubMenu,(int)options.size(),8,menuY,mw,rows,18,[&](int i,int x,int yy,bool sel){drawTextPx(fitText(options[i].label,mw-16,10),x+8,yy+4,10,sel?BLACK:C_TXT);});
    g_hubSel=offset+g_hubMenu.cur;
    int act=selected<0?-1:offset+selected;
    if(getenv("FOOT_TEST")&&menuY+std::min(rows,(int)options.size())*18>VH-24){fprintf(stderr,"FAIL hub menu outside footer bounds\n");exit(2);}
    // ---- colonne droite : classement adapté au prochain match (championnat, groupe de coupe ou tour de coupe)
    int rx = 250, rw = VW - rx - 8, ry = y;
    int showComp = -1, showStage = 0, showGroup = 0; bool showKO = false;
    if (g_pending.comp >= 0) {
        const Competition& C = S.comps[g_pending.comp];
        int st = C.stageOfMatch(g_pending.match);
        const MatchRes& m = C.matches[g_pending.match];
        if (st >= 0 && C.stages[st].type != ST_KO) { showComp = g_pending.comp; showStage = st; showGroup = std::max(0, (int)m.group); }
        else if (st >= 0) { showComp = g_pending.comp; showStage = st; showKO = true; }
    }
    int p, q, g;
    if (showComp < 0 && club && g_career.tierOfTeam(g_career.userTeam, &p, &q, &g) >= 0) showComp = g_career.pyramids[p].pools[q].comps[g];
    if (showComp >= 0 && !showKO) {
        const Competition& C = S.comps[showComp];
        const Stage& stg = C.stages[showStage];
        if (showGroup >= (int)stg.groups.size()) showGroup = 0;
        for (int gg = 0; gg < (int)stg.groups.size(); gg++) for (int t : stg.groups[gg]) if (S.isControlled(t)) showGroup = gg;
        std::string ttl = C.kind == 1 ? "CLASSEMENT" : fitText(C.shortName + " - " + stg.name + (stg.groups.size() > 1 ? fmt(" - Groupe %c", 'A' + showGroup) : ""), rw - 20, 10);
        drawSection(rx, ry, rw, ttl); ry += 11;
        auto tb = stg.type == ST_SWISS ? C.swissTable(showStage) : C.table(showStage, showGroup);
        int me = 0; for (int i = 0; i < (int)tb.size(); i++) if (S.isControlled(tb[i].team)) me = i;
        int from = std::max(0, std::min((int)tb.size() - 9, me - 4));
        drawTextPx("J", rx + rw - 88, ry, 10, C_DIM); drawTextPx("Diff", rx + rw - 66, ry, 10, C_DIM); drawTextPx("Pts", rx + rw - 26, ry, 10, C_DIM); ry += 11;
        for (int i = from; i < (int)tb.size() && i < from + 9; i++) {
            const Standing& st = tb[i];
            bool mine = S.isControlled(st.team);
            bool hover = IN.mouse.x >= rx && IN.mouse.x < rx + rw && IN.mouse.y >= ry && IN.mouse.y < ry + 11;
            DrawRectangle(rx, ry, rw, 10, mine ? Color{ 110, 90, 20, 255 } : hover ? Color{ 60, 90, 150, 255 } : (i % 2 ? C_ITEM : Color{ 38, 62, 108, 255 }));
            drawTextPx(fmt("%2d", i + 1), rx + 3, ry, 10, C_TXT);
            drawKitIcon(g_world.teams[st.team].home, rx + 20, ry + 1, 1);
            drawTextPx(fitText(C.kind == 1 ? g_world.teams[st.team].name : nameLvl(st.team), rw - 130, 10), rx + 32, ry, 10, mine ? C_HI : C_TXT);
            drawTextPx(fmt("%d", st.p), rx + rw - 88, ry, 10, C_TXT);
            drawTextPx(fmt("%+d", st.gd()), rx + rw - 66, ry, 10, C_TXT);
            drawTextPx(fmt("%d", st.pts), rx + rw - 26, ry, 10, C_HI);
            if (hover && IN.click) { openFiche(st.team, SC_HUB); return; }
            ry += 11;
        }
        ry += 4;
    } else if (showComp >= 0 && showKO) {
        // tour de coupe : les rencontres du tour
        const Competition& C = S.comps[showComp];
        const Stage& stg = C.stages[showStage];
        drawSection(rx, ry, rw, fitText(C.shortName + " - " + stg.name, rw - 20, 10)); ry += 11;
        std::vector<int> ms;
        for (auto& R : stg.rounds) for (int mi : R.m) if (C.matches[mi].leg == 0 || stg.legs == 1) ms.push_back(mi);
        std::stable_sort(ms.begin(), ms.end(), [&](int a, int b) { const MatchRes& A = C.matches[a]; const MatchRes& B = C.matches[b];
            return (S.isControlled(A.home) || S.isControlled(A.away)) > (S.isControlled(B.home) || S.isControlled(B.away)); });
        int shown = 0;
        for (int mi : ms) {
            if (shown++ >= 10) break;
            const MatchRes& m = C.matches[mi];
            bool mine = S.isControlled(m.home) || S.isControlled(m.away);
            bool hover = IN.mouse.x >= rx && IN.mouse.x < rx + rw && IN.mouse.y >= ry && IN.mouse.y < ry + 11;
            DrawRectangle(rx, ry, rw, 10, mine ? Color{ 110, 90, 20, 255 } : hover ? Color{ 60, 90, 150, 255 } : (shown % 2 ? C_ITEM : Color{ 38, 62, 108, 255 }));
            std::string l = fitText(nameLvl(m.home), rw / 2 - 30, 10), r = fitText(nameLvl(m.away), rw / 2 - 30, 10);
            drawTextPx(l, rx + rw / 2 - 18 - textWidth(l, 10), ry, 10, mine ? C_HI : C_TXT);
            drawTextCentered(m.played ? fmt("%d-%d", m.hg, m.ag) : "-", rx + rw / 2, ry, 10, C_TXT, false);
            drawTextPx(r, rx + rw / 2 + 18, ry, 10, mine ? C_HI : C_TXT);
            if (hover && IN.click) { openMatchInfo(showComp, mi, SC_HUB); return; }
            ry += 11;
        }
        if ((int)ms.size() > 10) { drawTextPx(fmt("... et %d autres rencontres", (int)ms.size() - 10), rx + 4, ry, 10, C_DIM); ry += 11; }
        ry += 4;
    }
    drawSection(rx, ry, rw, "DERNIERS RÉSULTATS"); ry += 11;
    auto um = userMatches(true);
    for (int kk = (int)um.size() - 1, shown = 0; kk >= 0 && shown < (club ? 6 : 12) && ry < VH - 28; kk--, shown++) {
        const Competition& C = S.comps[um[kk].first];
        const MatchRes& m = C.matches[um[kk].second];
        int me = S.isControlled(m.home) ? m.home : m.away;
        int gf = me == m.home ? m.hg : m.ag, ga = me == m.home ? m.ag : m.hg;
        Color c = gf > ga ? C_GOOD : gf < ga ? C_BAD : C_HI;
        if (gf == ga && m.ph >= 0) c = ((me == m.home) == (m.ph > m.pa)) ? C_GOOD : C_BAD;
        std::string sres = fmt("%s %d-%d %s", g_world.teams[m.home].shortName.c_str(), m.hg, m.ag, g_world.teams[m.away].shortName.c_str());
        if (m.ph >= 0) sres += fmt(" (%d-%d tab)", m.ph, m.pa);
        bool hover = IN.mouse.x >= rx && IN.mouse.x < rx + rw && IN.mouse.y >= ry && IN.mouse.y < ry + 11;
        if (hover) DrawRectangle(rx, ry, rw, 10, Color{ 60, 90, 150, 160 });
        DrawRectangle(rx + 2, ry + 2, 4, 7, c);
        drawTextPx(fitText(C.shortName, 70, 10), rx + 10, ry, 10, C_DIM);
        drawTextPx(fitText(sres, rw - 90, 10), rx + 84, ry, 10, C_TXT);
        if (hover && IN.click) { openMatchInfo(um[kk].first, um[kk].second, SC_HUB); return; }
        ry += 11;
    }
    drawFooter("Gauche/droite/Tab : rubrique | Haut/bas : choix | OK : ouvrir | Retour : menu");
    if (IN.back) { act = -1; askConfirm("Revenir au menu principal ? (la partie est sauvegardée automatiquement)", []() { autosave(); g_screen = SC_MAIN; }); return; }
    if (act < 0) return;
    switch (flat[act].id) {
    case 0: {
        const MatchRes& m = S.comps[g_pending.comp].matches[g_pending.match];
        if (club && pressWanted(g_pending.comp, g_pending.match)) { openPress(g_pending.comp, g_pending.match); break; }   // conférence de presse
        startSetup(m.home, m.away, true, g_pending.comp, g_pending.match);
        break;
    }
    case 1: simulateUserMatch(); break;
    case 2: g_screen = SC_SEASONEND; break;
    case 3: g_screen = SC_COMPS; break;
    case 4: g_screen = SC_FIXTURES; break;
    case 5: g_screen = SC_SQUAD; break;
    case 6: g_screen = SC_HISTORY; break;
    case 7: g_slotSave = true; g_slotLW = ListW(); g_screen = SC_SLOTS; break;
    case 8: askConfirm("Revenir au menu principal ? (la partie est sauvegardée automatiquement)", []() { autosave(); g_screen = SC_MAIN; }); break;
    case 9: openFiche(club ? g_career.userTeam : S.controlled[0], SC_HUB); break;
    case 10: g_screen = SC_COEFF; break;
    case 11: openClubEditor(-1, false, SC_HUB); break;
    case 12: openMarket(); break;
    case 13: g_screen = SC_FINANCE; break;
    case 14: g_screen = SC_NEWS; break;
    case 15: g_stTab = 0; g_screen = SC_STADIUM; break;
    case 19: g_screen = SC_REFEREES; break;
    case 20: g_screen = SC_CLUBMENU; break;
    case 30: openCallup(SC_HUB); break;
    case 31: g_screen = SC_HOSTS; break;
    case 32: g_screen = SC_COACHLOG; break;
    case 40: g_lifeTab = 0; g_screen = SC_LIFE; break;
    case 43: g_caSel = -1; g_screen = SC_COMPARCH; break;
    case 44: g_screen = SC_SEASONSTART; break;
    case 45: openBallon(true);break;
    case 46: openSeasonAwards(true);break;
    case 47: g_screen=SC_POLESCOUT;break;
    case 48: openMuseum(g_career.userTeam,SC_HUB);break;
    case 41: g_screen = SC_BRIBE; break;
    case 42: { int t = -1; int idx = -1; Player* me = g_career.lifePlayer(&t); if (me) { for (int i = 0; i < (int)g_world.teams[t].squad.size(); i++) if (g_world.teams[t].squad[i].id == me->id) idx = i; openPlayer(t, idx, SC_HUB); } break; }
    case 22: if (!openMatchday(g_pending.comp, g_pending.match, false)) toast("Un seul match pour ce tour"); break;
    case 21: g_optBack = SC_HUB; g_optLW = ListW(); g_screen = SC_OPTIONS; break;
    }
}

// ------------------------------------------------------------------ fin de saison
static void screenSeasonEnd() {
    Season& S = g_career.season;
    int award=g_career.prepareSeasonAwards();
    if(award>=0&&!g_career.seasonAwards[award].presented){openSeasonAwards();return;}
    if (g_career.kind == 0 && g_career.euroOnly) {
        drawBackground("Fin de saison européenne");
        int y = 40;
        int nk = g_career.uecl >= 0 ? 3 : 2;
        for (int k = 0; k < nk; k++) {
            int c = k == 0 ? g_career.ucl : k == 1 ? g_career.uel : g_career.uecl;
            if (c < 0 || c >= (int)S.comps.size()) continue;
            const Competition& C = S.comps[c];
            int x = nk == 3 ? VW * (1 + 2 * k) / 6 : (k == 0 ? VW / 4 : 3 * VW / 4);
            drawTextCentered(C.name, x, y, 10, C_DIM);
            if (C.winner >= 0) {
                drawKitIcon(g_world.teams[C.winner].home, x - 12, y + 14, 3);
                drawTextCentered(fitText(g_world.teams[C.winner].name, VW / 2 - 20, 20), x, y + 44, 20, C_HI);
                if (!C.stages.empty() && C.stages.back().ties.size() == 1) { const Tie& t = C.stages.back().ties[0]; int f = t.a == C.winner ? t.b : t.a; if (f >= 0) drawTextCentered("Finaliste : " + g_world.teams[f].name, x, y + 66, 10, C_TXT); }
            }
        }
        y = 130;
        drawSection(20, y, VW - 40, "VOS CLUBS"); y += 14;
        for (int t : S.controlled) {
            std::string res = "non qualifié";
            for (int c : { g_career.ucl, g_career.uel, g_career.uecl }) {
                if (c < 0) continue;
                const Competition& C = S.comps[c];
                for (auto& st : C.stages) {
                    bool in = false;
                    for (auto& g : st.groups) for (int x2 : g) if (x2 == t) in = true;
                    for (auto& ti : st.ties) if (ti.a == t || ti.b == t) in = true;
                    if (in) res = C.shortName + " : " + st.name;
                }
                if (C.winner == t) res = C.name + " : VAINQUEUR !";
            }
            drawTextPx(g_world.teams[t].name, 30, y, 10, C_HI); drawTextPx(res, 260, y, 10, C_TXT); y += 12;
        }
        drawTextPx("Les qualifications de la saison prochaine sont simulées (championnats nationaux non joués).", 20, VH - 44, 10, C_DIM);
        bool nextBtn = button(VW - 170, VH - 32, 150, 14, "Saison suivante >>", false);
        drawFooter("OK : saison suivante   Retour : menu");
        if (IN.ok || IN.start || nextBtn) { g_career.endSeason(); g_needAdvance = true; g_hubNotice = fmt("Coupes d'Europe %d-%02d", g_career.year, (g_career.year + 1) % 100); openHub(); autosave(); }
        if (IN.back) openHub();
        return;
    }
    bool club = g_career.kind == 0;
    const std::string country=g_career.selectedCountryName(), code=g_career.selectedCountryCode();
    drawBackground(club ? "Fin de saison - " + country : "Fin du tournoi");
    int y = 36;
    if (club) {
        int p, q, g;
        int nat=g_world.nationIndex(code.c_str());
        bool women=isWomenTeam(g_career.userTeam);
        bool europe=nat>=0 && NATIONS[nat].conf==UEFA && !women;
        int u = g_career.userTeam;
        if (g_career.tierOfTeam(u, &p, &q, &g) >= 0) {
            const Competition& C = S.comps[g_career.pyramids[p].pools[q].comps[g]];
            int pos = 1; for (int i = 0; i < (int)C.result.size(); i++) if (C.result[i] == u) pos = i + 1;
            drawTextCentered(fmt("%s termine %d%s de %s", g_world.teams[u].name.c_str(), pos, pos == 1 ? "er" : "e", C.name.c_str()), VW / 2, y, 10, C_HI);
            y += 16;
        }
        auto champ = [&](int comp) { return comp >= 0 && comp < (int)S.comps.size() && S.comps[comp].winner >= 0 ? g_world.teams[S.comps[comp].winner].name : std::string("-"); };
        auto finalist = [&](int comp) -> int {
            if (comp < 0 || comp >= (int)S.comps.size()) return -1;
            const Competition& C = S.comps[comp];
            if (C.stages.empty() || C.winner < 0) return -1;
            const Stage& st = C.stages.back();
            if (st.ties.size() != 1) return -1;
            return st.ties[0].a == C.winner ? st.ties[0].b : st.ties[0].a;
        };
        if (button(20, y - 2, 180, 14, "Palmarès de la saison", seTab == 0)) seTab = 0;
        if (button(204, y - 2, 180, 14, europe ? "Qualifiés pour l'Europe" : "Coupes continentales", seTab == 2)) seTab = 2;
        if (button(388, y - 2, 120, 14, "Votre saison", seTab == 1)) seTab = 1;
        if (button(512, y - 2, 110, 14, "Tout le club", seTab == 3)) seTab = 3;
        if (IN.tab) seTab = seTab == 0 ? 2 : seTab == 2 ? 1 : seTab == 1 ? 3 : 0;
        y += 16;
        if (seTab == 3) { drawClubSeason(y, true); }
        else
        if(seTab==2 && !europe) {
            int yy=y;drawSection(16,yy,VW-32,country+" - CONTINENT");yy+=18;
            for(int c=0;c<(int)S.comps.size();++c)if((women && S.comps[c].kind==43) || (!women && isContinentalKind(S.comps[c].kind)&&nat>=0&&S.comps[c].tag==NATIONS[nat].conf)){drawTextPx(S.comps[c].name,24,yy,10,C_HI);yy+=14;drawTextPx("Vainqueur : "+champ(c),24,yy,10,C_TXT);yy+=18;}
        } else
        if (seTab == 2) {
            // clubs qualifiés pour les coupes d'Europe de la saison prochaine (France en priorité)
            static int pvYear = -1, pvUser=-1; static EuroSpots pv;
            if (pvYear != g_career.year || pvUser!=g_career.userTeam) { pv = g_career.previewEuro(); pvYear = g_career.year; pvUser=g_career.userTeam; }
            struct Cat { const char* name; const std::vector<int>* v; };
            bool nf = g_career.opts.euroFormat != 0;
            Cat cats[] = { { nf ? "Ligue des champions - phase de ligue" : "Ligue des champions - phase de groupes", &pv.uclGS }, { nf ? "Ligue des champions - 3e tour de qualification ou barrage" : "Ligue des champions - 3e tour de qualification", &pv.uclQ3 },
                           { "Ligue des champions - 2e tour de qualification", &pv.uclQ2 }, { "Ligue des champions - 1er tour de qualification", &pv.uclQ1 },
                           { nf ? "Ligue Europa - phase de ligue" : "Coupe UEFA - 1er tour", &pv.uefaR1 }, { nf ? "Ligue Europa - 3e tour de qualification" : "Coupe UEFA - tour de qualification", &pv.uefaQR },
                           { "Coupe Intertoto - 3e tour", &pv.itR3 }, { "Coupe Intertoto - 2e tour", &pv.itR2 }, { nf ? "Ligue Conférence" : "Coupe Intertoto - 1er tour", &pv.itR1 } };
            int yl = y;
            drawSection(14, yl, VW - 28, country); yl += 13;
            for (auto& c : cats) {
                std::string names;
                for (int t : *c.v) if (g_world.teams[t].nation >= 0 && std::string(NATIONS[g_world.teams[t].nation].code) == code) names += (names.empty() ? "" : ", ") + g_world.teams[t].name;
                if (names.empty()) continue;
                drawTextPx(c.name, 20, yl, 10, C_DIM); yl += 11;
                for (auto& l : wrapText(names, VW - 60)) { drawTextPx(l, 34, yl, 10, names.find(g_world.teams[u].name) != std::string::npos && l.find(g_world.teams[u].name) != std::string::npos ? C_GOOD : C_TXT); yl += 11; }
            }
            yl += 4;
            drawSection(14, yl, VW - 28, "EUROPE - TÊTES D'AFFICHE (phase de groupes)"); yl += 13;
            std::string all;
            for (int t : pv.uclGS) if (!(g_world.teams[t].nation >= 0 && std::string(NATIONS[g_world.teams[t].nation].code) == code)) all += (all.empty() ? "" : ", ") + g_world.teams[t].shortName;
            for (auto& l : wrapText(all, VW - 60)) { if (yl > VH - 46) break; drawTextPx(l, 20, yl, 10, C_TXT); yl += 11; }
        } else
        if (seTab == 0) {
            int frp = g_career.selectedPyramid();
            // colonne gauche : championnats de France
            int yl = y;
            drawSection(14, yl, 300, country + " - CHAMPIONS"); yl += 13;
            if (frp >= 0) {
                const Pyramid& P = g_career.pyramids[frp];
                for (int tier = 0; tier < (int)P.tiers.size(); tier++)
                    for (auto& pl : P.pools) if (pl.tier == tier) for (int k = 0; k < (int)pl.comps.size(); k++) {
                        if(yl>VH-46) continue;
                        int c = pl.comps[k];
                        std::string nm = P.tiers[tier].name + (pl.comps.size() > 1 ? fmt(" %c", 'A' + k) : "");
                        bool me = S.comps[c].winner == u;
                        if (S.comps[c].winner >= 0) drawKitIcon(g_world.teams[S.comps[c].winner].home, 20, yl, 1);
                        drawTextPx(fitText(nm, 110, 10), 34, yl, 10, tier <= 1 ? C_HI : C_DIM);
                        drawTextPx(fitText(champ(c), 170, 10), 146, yl, 10, me ? C_GOOD : C_TXT);
                        yl += tier <= 2 ? 11 : 10;
                    }
            }
            // colonne droite : coupes françaises puis européennes, puis étranger
            int yr = y;
            auto cupLine = [&](const std::string& n, int c, bool fin) {
                if (c < 0 || c >= (int)S.comps.size()) return;
                bool me = S.comps[c].winner == u;
                if (S.comps[c].winner >= 0) drawKitIcon(g_world.teams[S.comps[c].winner].home, 330, yr, 1);
                drawTextPx(fitText(n, 116, 10), 344, yr, 10, C_DIM);
                std::string w = champ(c);
                int f = fin ? finalist(c) : -1;
                if (f >= 0) w += "  (fin. " + g_world.teams[f].shortName + ")";
                drawTextPx(fitText(w, 170, 10), 466, yr, 10, me ? C_GOOD : C_TXT);
                yr += 11;
            };
            drawSection(324, yr, 302, country + " - COUPES"); yr += 13;
            int cup=g_career.domesticCup();
            if(cup>=0) cupLine(S.comps[cup].shortName, cup, true);
            if(code=="FRA" && !women) cupLine("Coupe de la Ligue", g_career.cdl, true);
            for (int c : g_career.superCups) if (S.comps[c].tag == frp) cupLine(S.comps[c].shortName, c, true);
            yr += 3;
            if(code=="FRA" && !women) { drawSection(324, yr, 302, "COUPES RÉGIONALES"); yr += 13;
            cupLine("Méga Coupe des Régions", g_career.superRegions, true); }
            yr += 3;
            drawSection(324, yr, 302, europe ? "EUROPE" : "CONTINENT"); yr += 13;
            if(europe) {
            cupLine("Ligue des champions", g_career.ucl, true);
            cupLine(g_career.uecl >= 0 ? "Ligue Europa" : "Coupe UEFA", g_career.uel, true);
            if (g_career.uecl >= 0) cupLine("Ligue Conférence", g_career.uecl, true);
            cupLine("Supercoupe de l'UEFA", g_career.uefaSuper, false);
            if (g_career.intertoto >= 0 && g_career.intertoto < (int)S.comps.size() && S.comps[g_career.intertoto].done) {
                std::string w; for (int t : S.comps[g_career.intertoto].result) w += (w.empty() ? "" : ", ") + g_world.teams[t].shortName;
                drawTextPx("Intertoto", 344, yr, 10, C_DIM); drawTextPx(fitText(w, 170, 10), 466, yr, 10, C_TXT); yr += 11;
            }
            yr += 3;
            } else for(int c=0;c<(int)S.comps.size();++c)if((women && S.comps[c].kind==43) || (!women && isContinentalKind(S.comps[c].kind)&&nat>=0&&S.comps[c].tag==NATIONS[nat].conf))cupLine(S.comps[c].shortName,c,true);
            drawSection(324, yr, 302, "AUTRES PAYS - CHAMPIONS"); yr += 13;
            for (int pi = 0; pi < (int)g_career.pyramids.size(); pi++) {
                const Pyramid& P = g_career.pyramids[pi];
                if (P.dom >= 0 || pi == frp || P.country=="U19" || P.country=="U17" || P.country=="U15" || isWomenPyramid(P)!=women) continue;
                int q0 = P.poolIndex(0, 0);
                if (q0 < 0 || P.pools[q0].comps.empty()) continue;
                if (yr > VH - 46) break;
                cupLine(P.tiers[0].name, P.pools[q0].comps[0], false);
            }
        } else {
            // actualités de la saison regroupées par catégorie (comme « Tout le club »)
            static const char* CATN[10] = { "VOTRE CLUB", "COMPETITIONS", "RESERVES", "U19", "U17", "U15", "EFFECTIF & STAFF", "PRESSE", "VIE PRIVEE", "AILLEURS" };
            static const int NCAT = 10;
            auto has = [](const std::string& s, const char* k) { return s.find(k) != std::string::npos; };
            std::vector<std::pair<int, std::string>> rows;   // (catégorie ou -1 = titre, texte)
            std::vector<std::string> byCat[NCAT];
            struct Agg { int v = 0, n = 0, d = 0; std::string last; };
            std::vector<std::pair<std::string, Agg>> agg[NCAT];   // résultats des équipes réserves / jeunes, cumulés par compétition
            for (auto& n0 : S.news) {
                std::string n = n0; int c = 9;
                int bt = -1;
                if (n.rfind("[Réserve] ", 0) == 0) bt = 2; else if (n.rfind("[U19] ", 0) == 0) bt = 3; else if (n.rfind("[U17] ", 0) == 0) bt = 4; else if (n.rfind("[U15] ", 0) == 0) bt = 5;
                if (bt >= 0) {
                    n = n.substr(n.find("] ") + 2);
                    size_t cp = n.find(" : "), rp = n.rfind(" (");
                    if (cp != std::string::npos && rp != std::string::npos && rp > cp) {
                        std::string comp = n.substr(0, cp), res = n.substr(rp + 2);
                        Agg* A = nullptr;
                        for (auto& e : agg[bt]) if (e.first == comp) A = &e.second;
                        if (!A) { agg[bt].push_back({ comp, Agg() }); A = &agg[bt].back().second; }
                        if (res.rfind("victoire", 0) == 0 || res.rfind("qualifié", 0) == 0) A->v++; else if (res.rfind("défaite", 0) == 0 || res.rfind("éliminé", 0) == 0) A->d++; else A->n++;
                        A->last = n.substr(cp + 3, rp - cp - 3);
                        continue;
                    }
                    byCat[bt].push_back(n); continue;
                }
                if (n.rfind("[Votre club] ", 0) == 0) { n = n.substr(13); c = has(n, "U19") ? 3 : has(n, "U17") ? 4 : has(n, "Gambardella") ? 3 : 0; }
                else if (n.rfind("Presse : ", 0) == 0) { n = n.substr(9); c = 7; }
                else if (n.rfind("Vie privée : ", 0) == 0) { n = n.substr(13); c = 8; }
                else if (has(n, "U19") || has(n, "Gambardella")) c = 3;
                else if (has(n, "U17")) c = 4;
                else if (has(n, "U15")) c = 5;
                else if (has(n, "retraite") || has(n, "raccroche") || has(n, "Reconversion") || has(n, "se reconvertit") || has(n, "quitte les") || has(n, "centre de formation")) c = 6;
                else if (has(n, g_world.teams[u].name.c_str())) c = 0;
                else if (has(n, "AFFAIRE") || has(n, "Sanction") || has(n, "président") || has(n, "ligue prononce") || has(n, "Statut du club") || has(n, "Droits TV") || has(n, "DNCG") || has(n, "Sponsor") || has(n, "Objectif")) c = 0;
                else if (has(n, " : ") || has(n, "Coupe") || has(n, "Ligue") || has(n, "Trophée")) c = 1;
                byCat[c].push_back(n);
            }
            for (int c = 2; c <= 5; c++) {
                std::vector<std::string> sum;
                for (auto& e : agg[c]) sum.push_back(fmt("%s : %d match%s - %d V / %d N / %d D   (dernier : %s)", e.first.c_str(), e.second.v + e.second.n + e.second.d, e.second.v + e.second.n + e.second.d > 1 ? "s" : "", e.second.v, e.second.n, e.second.d, e.second.last.c_str()));
                byCat[c].insert(byCat[c].begin(), sum.begin(), sum.end());
            }
            for (int c = 0; c < NCAT; c++) {
                if (byCat[c].empty()) continue;
                rows.push_back({ -1, fmt("%s (%d)", CATN[c], (int)byCat[c].size()) });
                for (auto& n : byCat[c]) {
                    auto ls = wrapText(n, VW - 60);
                    for (size_t k = 0; k < ls.size(); k++) rows.push_back({ k == 0 ? c : c + 100, ls[k] });
                }
            }
            static int seScroll = 0;
            int vis = (VH - 40 - y) / 11, maxS = std::max(0, (int)rows.size() - vis);
            if (IN.down || IN.wheel < 0) seScroll += IN.wheel < 0 ? 3 : 1;
            if (IN.up || IN.wheel > 0) seScroll -= IN.wheel > 0 ? 3 : 1;
            seScroll = std::max(0, std::min(maxS, seScroll));
            int yy = y;
            for (int i = seScroll; i < (int)rows.size() && yy <= VH - 40 - 11; i++) {
                auto& r = rows[i];
                if (r.first < 0) { if (i > seScroll) yy += 3; drawSection(14, yy, VW - 28, r.second); yy += 13; continue; }
                int c = r.first % 100;
                Color col = c == 0 ? C_GOOD : c == 1 ? C_HI : c == 8 || c == 9 ? C_DIM : C_TXT;
                if (r.first < 100) drawTextPx("-", 22, yy, 10, C_DIM);
                drawTextPx(r.second, 32, yy, 10, col); yy += 11;
            }
            if (rows.empty()) drawTextCentered("Aucun fait marquant cette saison.", VW / 2, y + 20, 10, C_DIM);
            if (maxS > 0) drawTextPx(fmt("Haut/Bas : défiler (%d/%d)", seScroll + 1, maxS + 1), VW - 190, VH - 44, 10, C_DIM);
        }
        drawFooter("Tab : onglet   OK : saison suivante");
        bool nextBtn = button(VW - 170, VH - 32, 150, 14, "Saison suivante >>", false);
        if (IN.ok || IN.start || nextBtn) {
            if(g_career.awardsEnabled()){openTeamPlans();return;}
            std::string before = g_career.teamLevelName(u);
            int tb = g_career.tierOfTeam(u);
            g_career.endSeason();
            int ta = g_career.tierOfTeam(u);
            std::string after = g_career.teamLevelName(u);
            if (ta < tb) g_hubNotice = "PROMOTION ! Nouvelle saison en " + after;
            else if (ta > tb) g_hubNotice = "Relégation... Nouvelle saison en " + after;
            else g_hubNotice = "Nouvelle saison en " + after;
            g_needAdvance = true;
            openHub();
            autosave();
            showSeasonArticle();
        }
        if (IN.back) openHub();
    } else {
        int fc = g_career.kind == CK_CUSTOM ? 0 : g_career.finalComp;
        if (fc >= 0 && fc < (int)S.comps.size() && S.comps[fc].winner < 0 && !S.comps[fc].result.empty()) S.comps[fc].winner = S.comps[fc].result[0];
        if (fc >= 0 && fc < (int)S.comps.size() && S.comps[fc].winner >= 0) {
            int w = S.comps[fc].winner;
            drawKitIcon(g_world.teams[w].home, VW / 2 - 12, 50, 3);
            drawTextCentered("VAINQUEUR", VW / 2, 80, 10, C_DIM);
            drawTextCentered(g_world.teams[w].name, VW / 2, 94, 20, C_HI);
        }
        y = 130;
        for (int t : S.controlled) {
            std::string res = "non qualifiée";
            if (fc >= 0) {
                const Competition& C = S.comps[fc];
                bool in = false;
                for (auto& g : C.stages[0].groups) for (int x : g) if (x == t) in = true;
                for (auto& ti : C.stages[0].ties) if (ti.a == t || ti.b == t) in = true;
                if (g_career.kind == CK_CUSTOM && C.format == FMT_LEAGUE) {
                    int pos = 0; for (int i = 0; i < (int)C.result.size(); i++) if (C.result[i] == t) pos = i + 1;
                    res = pos ? fmt("%de place", pos) : "-"; if (pos == 1) res = "CHAMPION !";
                } else if (in) {
                    res = "phase de groupes";
                    for (size_t s = 1; s < C.stages.size(); s++) for (auto& ti : C.stages[s].ties) if (ti.a == t || ti.b == t) res = C.stages[s].name;
                    if (C.winner == t) res = "VAINQUEUR !";
                }
            }
            drawTextCentered(g_world.teams[t].name + " : " + res, VW / 2, y, 10, C_TXT); y += 12;
        }
        if (g_career.coach) {
            int res = g_career.coachResult();
            drawTextCentered(std::string("Objectif de la fédération : ") + g_career.coachObjectiveText(), VW / 2, y + 6, 10, C_DIM);
            drawTextCentered(std::string("Résultat : ") + resultLevelName(res), VW / 2, y + 20, 10, res <= g_career.coachObjective ? C_GOOD : C_BAD);
            drawFooter("OK : campagne suivante");
            if (button(VW - 190, VH - 34, 180, 16, "Campagne suivante >>") || IN.ok || IN.start) { coachNextCampaign(); return; }
            if (IN.back) openHub();
            return;
        }
        drawFooter("OK : menu principal");
        if (IN.ok || IN.click || IN.start) { g_careerActive = false; g_screen = SC_MAIN; }
        if (IN.back) openHub();
    }
}

// ------------------------------------------------------------------ classements
static ListW g_compsLW;
static int g_cv = -1, g_cvStage = 0, g_cvGroup = 0, g_cvMode = 0, g_cvRound = 0;
static ListW g_cvLW;

static void openCompView(int comp, int stage, int group) {
    g_cv = comp;
    const Competition& C = g_career.season.comps[comp];
    g_cvStage = stage >= 0 ? stage : C.cur;
    if (g_cvStage >= (int)C.stages.size()) g_cvStage = (int)C.stages.size() - 1;
    g_cvGroup = std::max(0, group);
    // groupe contenant une équipe contrôlée
    if (group < 0 && g_cvStage >= 0) {
        const Stage& st = C.stages[g_cvStage];
        for (int g = 0; g < (int)st.groups.size(); g++) for (int t : st.groups[g]) if (g_career.season.isControlled(t)) g_cvGroup = g;
    }
    g_cvMode = 0; g_cvRound = -1; g_cvLW = ListW();
    g_screen = SC_COMPVIEW;
}

static int g_compsCat = 0, g_compsSub = 0;
static int g_compsRegion = -1, g_compsDept = -1;
static void screenComps() {
    drawBackground("Classements et résultats");
    Season& S = g_career.season;
    struct CI { std::string label; int comp; int kind; };
    std::vector<CI> items;
    std::set<int> mine;
    for (int c = 0; c < (int)S.comps.size(); c++) {
        const Competition& C = S.comps[c];
        bool in = false;
        for (auto& m : C.matches) if (S.isControlled(m.home) || S.isControlled(m.away)) { in = true; break; }
        if (!in) for (auto& st : C.stages) for (auto& g : st.groups) for (int t : g) if (S.isControlled(t)) in = true;
        if (in) mine.insert(c);
    }
    int y = 34;
    if (g_career.kind == 0) {
        const std::string country=g_career.selectedCountryName();
        const std::string code=g_career.selectedCountryCode();
        const bool french=code=="FRA" && !isWomenTeam(g_career.userTeam);
        int countryCup=g_career.domesticCup();
        const std::string CATS[] = { "Mes compétitions", country, french ? "Coupe de France" : countryCup>=0 ? S.comps[countryCup].shortName : "Coupes nationales", "Continent", "Autres pays", "Féminin", "Parcourir" };
        const int NC = 7;
        int bw = (VW - 20) / NC;
        if (g_compsCat >= NC) g_compsCat = 0;
        for (int i = 0; i < NC; i++) if (button(10 + i * bw, y, bw - 2, 16, fitText(CATS[i],bw-8,10), g_compsCat == i)) { g_compsCat = i; g_compsLW = ListW(); }
        if (IN.pgUp) { g_compsCat = (g_compsCat + NC - 1) % NC; g_compsLW = ListW(); }
        if (IN.pgDn) { g_compsCat = (g_compsCat + 1) % NC; g_compsLW = ListW(); }
        y += 20;
        if (g_compsCat == 1 || g_compsCat == 2) {
            static const char* SUBF[] = { "National", "Régional", "District", "Jeunes" };
            static const char* SUBC[] = { "Coupes nationales", "Coupes régionales", "Coupes de district", "Coupes jeunes" };
            int sw = (VW - 40) / 4;
            DrawRectangle(10, y - 2, VW - 20, 18, Color{ 16, 26, 52, 255 });
            for (int i = 0; i < 4; i++) if (button(20 + i * sw, y, sw - 4, 14, g_compsCat == 1 ? SUBF[i] : SUBC[i], g_compsSub == i)) { g_compsSub = i; g_compsLW = ListW(); }
            y += 18;
        }
        // catégorie interne
        int L = g_compsCat == 0 ? 0 : g_compsCat == 3 ? 3 : g_compsCat == 4 ? 4 : g_compsCat == 5 ? 9 : g_compsCat == 6 ? 8 : 0;
        bool champOnly = false, cupsOnly = false;
        if (g_compsCat == 1) { static const int M[4] = { 1, 5, 6, 7 }; L = M[g_compsSub & 3]; champOnly = true; }
        if (g_compsCat == 2) { static const int M[4] = { 2, 5, 6, 7 }; L = M[g_compsSub & 3]; cupsOnly = true; }
        std::vector<int> v;
        int frp = g_career.selectedPyramid();
        std::map<int, std::string> cpre;      // préfixe (pays) des compétitions listées
        switch (L) {
        case 0: for (int c : mine) v.push_back(c); break;
        case 1:
            if (frp >= 0) for (auto& pl : g_career.pyramids[frp].pools) if (pl.tier <= 4 || !french) for (int c : pl.comps) v.push_back(c);
            { int p, q, g; if (g_career.tierOfTeam(g_career.userTeam, &p, &q, &g) > 4) v.push_back(g_career.pyramids[p].pools[q].comps[g]); }
            for (int c = 0; c < (int)S.comps.size(); c++) if (S.comps[c].kind == 10 && S.comps[c].tag / 100 == frp) v.push_back(c);
            break;
        case 2:
            if (french) {
                if (g_career.cdfNational >= 0) v.push_back(g_career.cdfNational);
                if (g_career.cdl >= 0) v.push_back(g_career.cdl);
            } else { int c=g_career.domesticCup(); if(c>=0) v.push_back(c); }
            for (int c : g_career.superCups) if (S.comps[c].tag == frp) v.push_back(c);
            if (french && g_career.cdfNational < 0) for (int c : g_career.cdfRegional) v.push_back(c);
            break;
        case 3: {
            int nat=g_world.nationIndex(code.c_str());
            if(nat>=0 && NATIONS[nat].conf!=UEFA) {for(int c=0;c<(int)S.comps.size();++c)if(isContinentalKind(S.comps[c].kind)&&S.comps[c].tag==NATIONS[nat].conf)v.push_back(c);}
            else if(frp>=0 && isWomenPyramid(g_career.pyramids[frp])) {for(int c=0;c<(int)S.comps.size();++c)if(S.comps[c].kind==43)v.push_back(c);}
            else v={g_career.ucl,g_career.uel,g_career.uecl,g_career.intertoto,g_career.uefaSuper,g_career.youthPrelim,g_career.youthUcl};
            break;
        }
        case 4: {
            // championnats étrangers par confédération, puis par pays (toutes divisions, coupe et supercoupe)
            static const char* CF[NUM_CONFEDS] = { "UEFA", "CONMEBOL", "CONCACAF", "CAF", "AFC", "OFC" };
            static int cfSel = 0, cfCountry = 0;
            int cw = (VW - 40) / NUM_CONFEDS;
            DrawRectangle(10, y - 2, VW - 20, 18, Color{ 16, 26, 52, 255 });
            for (int i = 0; i < NUM_CONFEDS; i++) if (button(20 + i * cw, y, cw - 4, 14, CF[i], cfSel == i)) { cfSel = i; cfCountry = 0; g_compsLW = ListW(); }
            y += 18;
            std::vector<int> pys;
            for (int p = 0; p < (int)g_career.pyramids.size(); p++) {
                const Pyramid& P = g_career.pyramids[p];
                if (P.dom >= 0 || p == frp || P.country == "U19" || P.country == "U17" || P.country == "U15") continue;
                int n = g_world.nationIndex(P.country.c_str());
                if (n < 0 || NATIONS[n].conf != cfSel) continue;
                pys.push_back(p);
            }
            std::stable_sort(pys.begin(), pys.end(), [](int a, int b) { return sortKey(g_career.pyramids[a].name) < sortKey(g_career.pyramids[b].name); });
            int nOpt = (int)pys.size() + 1;          // 0 : toutes les premières divisions de la confédération
            if (cfCountry >= nOpt) cfCountry = 0;
            int nw = 300, nx = VW / 2 - nw / 2;
            if (button(nx - 26, y, 22, 14, "<", false) || IN.left) cfCountry = (cfCountry + nOpt - 1) % nOpt, g_compsLW = ListW();
            if (button(nx + nw + 4, y, 22, 14, ">", false) || IN.right) cfCountry = (cfCountry + 1) % nOpt, g_compsLW = ListW();
            DrawRectangle(nx, y, nw, 14, C_ITEM);
            drawTextCentered(cfCountry == 0 ? fmt("%s : toutes les premières divisions (%d pays)", CF[cfSel], (int)pys.size()) : g_career.pyramids[pys[cfCountry - 1]].name,
                             VW / 2, y + 2, 10, C_HI, false);
            y += 18;
            if (!cfCountry) for (int c = 0; c < (int)S.comps.size(); c++) if (isContinentalKind(S.comps[c].kind) && S.comps[c].tag == cfSel) v.push_back(c);
            for (int k = 0; k < (int)pys.size(); k++) {
                if (cfCountry && k != cfCountry - 1) continue;
                int p = pys[k];
                for (auto& pl : g_career.pyramids[p].pools) if (pl.tier == 0 || cfCountry) for (int c : pl.comps) { v.push_back(c); if (!cfCountry) cpre[c] = g_career.pyramids[p].name + " - "; }
                if (!cfCountry) continue;
                for (int c : g_career.nationalCups) if (S.comps[c].tag == p) v.push_back(c);
                for (int c : g_career.superCups) if (S.comps[c].tag == p) v.push_back(c);
                for (int c = 0; c < (int)S.comps.size(); c++) if (S.comps[c].kind == 10 && S.comps[c].tag / 100 == p) v.push_back(c);
            }
            break;
        }
        case 9: {
            // football féminin : un pays à la fois (toutes les divisions, coupes, play-offs), Ligue des champions féminine
            std::vector<int> pys;
            for (int p = 0; p < (int)g_career.pyramids.size(); p++) if (isWomenPyramid(g_career.pyramids[p])) pys.push_back(p);
            static int wSel = 0;
            int nOpt = (int)pys.size() + 1;
            if (wSel >= nOpt) wSel = 0;
            int nw = 300, nx = VW / 2 - nw / 2;
            if (button(nx - 26, y, 22, 14, "<", false) || IN.left) wSel = (wSel + nOpt - 1) % nOpt, g_compsLW = ListW();
            if (button(nx + nw + 4, y, 22, 14, ">", false) || IN.right) wSel = (wSel + 1) % nOpt, g_compsLW = ListW();
            DrawRectangle(nx, y, nw, 14, C_ITEM);
            drawTextCentered(wSel == 0 ? std::string("Europe (Ligue des champions féminine)") : g_career.pyramids[pys[wSel - 1]].name, VW / 2, y + 2, 10, C_HI, false);
            y += 18;
            if (wSel == 0) { for (int c = 0; c < (int)S.comps.size(); c++) if (S.comps[c].kind == 43) v.push_back(c); break; }
            int p = pys[wSel - 1];
            for (auto& pl : g_career.pyramids[p].pools) for (int c : pl.comps) v.push_back(c);
            for (int c = 0; c < (int)S.comps.size(); c++) {
                const Competition& C = S.comps[c];
                if ((C.kind == 41 || C.kind == 42 || C.kind == 44) && C.tag == p) v.push_back(c);
                if (C.kind == 40 && C.tag >= 0 && C.tag < (int)S.comps.size() && S.comps[C.tag].tag / 100000 == p) v.push_back(c);
            }
            break;
        }
        case 5: case 6: {
            if(!french) {
                if(frp>=0 && !cupsOnly) for(const auto& pl:g_career.pyramids[frp].pools) if(g_career.pyramids[frp].tiers[pl.tier].scope==(L==5?SC_REGION:SC_DEPT)) for(int c:pl.comps) v.push_back(c);
                break;
            }
            // sélecteur de ligue régionale / district (par défaut : celui du club)
            const int NMR = 13;
            int ur = g_career.userTeam >= 0 ? g_world.teams[g_career.userTeam].region : -1;
            int ud = g_career.userTeam >= 0 ? g_world.teams[g_career.userTeam].district : -1;
            std::vector<int> depts;     // districts, classés par ligue puis par nom
            for (int d = 0; d < numDistricts(); d++) if (districtRegion(d) >= 0 && districtRegion(d) < NMR) depts.push_back(d);
            std::stable_sort(depts.begin(), depts.end(), [](int a, int b) { if (districtRegion(a) != districtRegion(b)) return districtRegion(a) < districtRegion(b); return sortKey(districtName(a)) < sortKey(districtName(b)); });
            if (g_compsRegion < 0) g_compsRegion = ur >= 0 && ur < NMR ? ur : 0;
            if (g_compsDept < 0 || g_compsDept >= numDistricts()) g_compsDept = ud >= 0 ? ud : depts[0];
            bool reg = L == 5;
            int nOpt = reg ? NMR + 1 : (int)depts.size();
            int curI = reg ? g_compsRegion : (int)(std::find(depts.begin(), depts.end(), g_compsDept) - depts.begin());
            if (curI >= nOpt) curI = 0;
            auto label = [&](int i) { return reg ? (i < NMR ? sanitize(REGIONS[i].name) : std::string("Outre-mer")) : districtFullName(depts[i]) + " (" + sanitize(REGIONS[districtRegion(depts[i])].name) + ")"; };
            int nw = 300, nx = VW / 2 - nw / 2;
            if (button(nx - 26, y, 22, 14, "<", false) || IN.left) curI = (curI + nOpt - 1) % nOpt, g_compsLW = ListW();
            if (button(nx + nw + 4, y, 22, 14, ">", false) || IN.right) curI = (curI + 1) % nOpt, g_compsLW = ListW();
            DrawRectangle(nx, y, nw, 14, C_ITEM);
            drawTextCentered(fitText(label(curI), nw - 10, 10), VW / 2, y + 2, 10, C_HI, false);
            if (reg) g_compsRegion = curI; else g_compsDept = depts[curI];
            y += 18;
            if (frp >= 0 && !cupsOnly && (!reg || curI < NMR)) {
                const Pyramid& P = g_career.pyramids[frp];
                for (auto& pl : P.pools) {
                    int sc = P.tiers[pl.tier].scope;
                    if (reg && sc == SC_REGION && pl.key == curI) for (int c : pl.comps) v.push_back(c);
                    if (!reg && sc == SC_DEPT && pl.key == g_compsDept) for (int c : pl.comps) v.push_back(c);
                }
            }
            if (reg && curI == NMR) {
                if (!cupsOnly) for (auto& P : g_career.pyramids) if (P.dom >= 0) for (auto& pl : P.pools) for (int c : pl.comps) v.push_back(c);
                if (!champOnly) for (int c : g_career.cdfRegional) if (S.comps[c].tag >= 100) v.push_back(c);
            }
            if (champOnly) {}
            else if (reg) {
                for (int c : g_career.regionalCups) if (S.comps[c].tag == curI) v.push_back(c);
                for (int c : g_career.cdfRegional) if (S.comps[c].tag == curI) v.push_back(c);
                for (int c : g_career.regSuperCups) if (c < (int)S.comps.size() && S.comps[c].tag == curI) v.push_back(c);
                if (g_career.superRegions >= 0) v.push_back(g_career.superRegions);
            } else for (int c : g_career.deptCups) if (S.comps[c].tag == g_compsDept) v.push_back(c);
            break;
        }
        }
        if (L == 7 && french) {
            if(!champOnly && g_career.poleCup>=0)v.push_back(g_career.poleCup);
            // jeunes (U19 / U17) : national, puis chaque ligue (régional + districts de la ligue) avec ses coupes
            static int ageSel = 0;
            if (button(12, y, 40, 14, "U19", ageSel == 0)) { ageSel = 0; g_compsLW = ListW(); }
            if (button(56, y, 40, 14, "U17", ageSel == 1)) { ageSel = 1; g_compsLW = ListW(); }
            if (button(100, y, 40, 14, "U15", ageSel == 2)) { ageSel = 2; g_compsLW = ListW(); }
            int yp = ageSel == 2 ? g_career.u15Pyramid() : ageSel == 1 ? g_career.u17Pyramid() : g_career.u19Pyramid();
            static int u19Sel = 0;
            const int NMR = 13;
            int nOpt = NMR + 1;
            if (u19Sel == 0 && g_compsLW.cur == 0 && g_career.userTeam >= 0) {}
            int nw = 300, nx = VW / 2 - nw / 2;
            if (button(nx - 26, y, 22, 14, "<", false) || IN.left) u19Sel = (u19Sel + nOpt - 1) % nOpt, g_compsLW = ListW();
            if (button(nx + nw + 4, y, 22, 14, ">", false) || IN.right) u19Sel = (u19Sel + 1) % nOpt, g_compsLW = ListW();
            DrawRectangle(nx, y, nw, 14, C_ITEM);
            if (ageSel == 2 && u19Sel == 0) u19Sel = 1;         // U15 : pas de niveau national
            drawTextCentered(u19Sel == 0 ? std::string(ageSel == 1 ? "National (National U17)" : "National (National U19, Gambardella, Ligue des champions U19)") : "Ligue " + sanitize(REGIONS[u19Sel - 1].name), VW / 2, y + 2, 10, C_HI, false);
            y += 18;
            if (yp >= 0) {
                const Pyramid& Y = g_career.pyramids[yp];
                if (u19Sel == 0) {
                    if (!cupsOnly) for (auto& pl : Y.pools) if (pl.tier == 0) for (int c : pl.comps) v.push_back(c);
                    if (ageSel == 1) { if (g_career.u17Final >= 0 && !cupsOnly) v.push_back(g_career.u17Final); }
                    else {
                        if (g_career.u19Final >= 0 && !cupsOnly) v.push_back(g_career.u19Final);
                        if (g_career.gambNational >= 0 && !champOnly) v.push_back(g_career.gambNational);
                        if (g_career.youthPrelim >= 0 && !champOnly) v.push_back(g_career.youthPrelim);
                        if (g_career.youthUcl >= 0 && !champOnly) v.push_back(g_career.youthUcl);
                    }
                } else {
                    int rg = u19Sel - 1;
                    if (!cupsOnly) for (int tier = (ageSel == 2 ? 0 : 1); tier < (int)Y.tiers.size(); tier++)
                        for (auto& pl : Y.pools) {
                            if (pl.tier != tier) continue;
                            bool ok = Y.tiers[tier].scope == SC_REGION ? pl.key == rg : districtRegion(pl.key) == rg;
                            if (ok) for (int c : pl.comps) v.push_back(c);
                        }
                    if (ageSel == 0 && !champOnly) for (int c : g_career.gambRegional) if (S.comps[c].tag == rg) v.push_back(c);
                    if (ageSel == 0 && !champOnly) for (int c : g_career.u19Cups) {
                        const Competition& C = S.comps[c];
                        if ((C.kind == 17 && C.tag == rg) || (C.kind == 18 && districtRegion(C.tag) == rg)) v.push_back(c);
                    }
                }
            }
        }
        if (L == 8) items.push_back({ "Parcourir les championnats de toutes les nations", -1, 1 });
        std::set<int> seen;
        for (int c : v) if (c >= 0 && c<(int)S.comps.size() && seen.insert(c).second) items.push_back({ (mine.count(c) ? "* " : "") + (cpre.count(c) ? cpre[c] : std::string()) + S.comps[c].name, c, 0 });
    } else {
        for (int c : mine) items.push_back({ "* " + S.comps[c].name, c, 0 });
        for (int c = 0; c < (int)S.comps.size(); c++) if (!mine.count(c)) items.push_back({ S.comps[c].name, c, 0 });
    }
    int rows = (VH - 18 - y) / 13;
    // sous-groupes (National 1 groupes A/B/C, National 2 A à D...) : famille = nom avant « - Groupe » / « - Poule »
    auto family = [](const std::string& l, std::string& grp) {
        size_t k = l.find(" - Groupe "); if (k == std::string::npos) k = l.find(" - Poule ");
        if (k == std::string::npos) { grp.clear(); return l; }
        grp = l.substr(k + 3); return l.substr(0, k);
    };
    int s = listRun(g_compsLW, (int)items.size(), 20, y, VW - 40, rows, 13, [&](int i, int x, int yy, bool sel) {
        std::string g0, g1, f0 = family(items[i].label, g0);
        std::string f1 = i > 0 ? family(items[i - 1].label, g1) : std::string();
        if (!g0.empty()) {
            // bande de couleur par groupe et séparateur en tête de famille
            static const Color GCOL[6] = { { 80, 150, 240, 255 }, { 240, 160, 60, 255 }, { 110, 210, 120, 255 }, { 220, 100, 200, 255 }, { 240, 220, 80, 255 }, { 120, 220, 230, 255 } };
            int gi = 0; size_t sp = g0.rfind(' '); if (sp != std::string::npos && sp + 1 < g0.size()) gi = (g0[sp + 1] - 'A' + 6000) % 6;
            DrawRectangle(x, yy, 4, 12, GCOL[gi]);
            if (f0 != f1) DrawRectangle(x, yy - 1, VW - 40, 1, Color{ 240, 200, 60, 160 });
        } else if (i > 0 && !g1.empty()) DrawRectangle(x, yy - 1, VW - 40, 1, Color{ 240, 200, 60, 160 });
        drawTextPx(fitText(items[i].label, VW - 140, 10), x + 8, yy + 1, 10, sel ? BLACK : (items[i].label[0] == '*' ? C_HI : C_TXT));
        if (items[i].comp >= 0) {
            const Competition& C = S.comps[items[i].comp];
            std::string st = C.done ? (C.winner >= 0 ? "Vainqueur : " + g_world.teams[C.winner].shortName : "terminé") : C.stages.empty() ? "à venir" : "en cours";
            drawTextPx(st, x + VW - 40 - textWidth(st, 10) - 6, yy + 1, 10, sel ? BLACK : C_DIM);
        }
    });
    if (items.empty()) drawTextCentered("Aucune compétition dans cette catégorie", VW / 2, y + 20, 10, C_DIM);
    drawFooter("OK : afficher   PgPréc/PgSuiv (LB/RB) : catégorie   Retour");
    if (IN.back) { openHub(); return; }
    if (s < 0) return;
    if (items[s].kind == 1) { openPick(PM_BROWSE); return; }
    openCompView(items[s].comp);
}

static std::vector<std::string> wrapTextSz(const std::string& s, int maxw, int fs);
static std::vector<std::string> wrapText(const std::string& s, int maxw) { return wrapTextSz(s, maxw, 10); }
static std::vector<std::string> wrapTextSz(const std::string& s, int maxw, int fs) {
    std::vector<std::string> out;
    std::string para;
    auto flush = [&](const std::string& p) {
        std::string line, word;
        for (size_t i = 0; i <= p.size(); i++) {
            char ch = i < p.size() ? p[i] : ' ';
            if (ch == ' ') {
                std::string cand = line.empty() ? word : line + " " + word;
                if (textWidth(cand, fs) > maxw && !line.empty()) { out.push_back(line); line = word; }
                else line = cand;
                word.clear();
            } else word += ch;
        }
        if (!line.empty()) out.push_back(line);
    };
    for (char ch : s) { if (ch == '\n') { flush(para); para.clear(); } else para += ch; }
    if (!para.empty()) flush(para);
    return out;
}

// classement des buteurs / passeurs d'une compétition
struct ScorerRow { int pid, team, n, pens; std::string name; };
static std::vector<ScorerRow> g_scorers[2];
static int g_scorersComp = -1; static size_t g_scorersN = 0;
static void buildScorers(const Competition& C, int comp) {
    if (g_scorersComp == comp && g_scorersN == C.events.size()) return;
    g_scorersComp = comp; g_scorersN = C.events.size();
    for (int k = 0; k < 2; k++) {
        std::map<int, ScorerRow> m;
        for (const MEv& e : C.events) {
            if (e.type != 0) continue;
            int pid = k == 0 ? e.pid : e.aid;
            if (pid <= 0) continue;
            ScorerRow& r = m[pid];
            r.pid = pid; r.team = e.team; r.n++; if (k == 0 && e.pen) r.pens++;
        }
        std::vector<ScorerRow> v;
        for (auto& kv : m) v.push_back(kv.second);
        std::sort(v.begin(), v.end(), [](const ScorerRow& a, const ScorerRow& b) { return a.n > b.n; });
        if (v.size() > 100) v.resize(100);
        // noms
        std::map<int, std::string> names;
        for (auto& r : v) names[r.pid] = "";
        for (auto& T : g_world.teams) if (T.squadGen) for (auto& p : T.squad) { auto it = names.find(p.id); if (it != names.end()) it->second = p.name; }
        for (auto& r : v) r.name = names[r.pid].empty() ? "?" : names[r.pid];
        g_scorers[k] = v;
    }
}

// tableau d'une coupe à élimination directe (à partir des 8es de finale)
static void drawCupBracket(const Competition& C, int y0) {
    std::vector<int> ks;
    for (int s = 0; s < (int)C.stages.size(); s++) if (C.stages[s].type == ST_KO && !C.stages[s].ties.empty() && C.stages[s].name.find("3e place") == std::string::npos) ks.push_back(s);
    if (ks.empty()) return;
    int first = -1;
    for (int s : ks) if (C.stages[s].ties.size() <= 8) { first = s; break; }
    if (first < 0) first = ks.back();
    int N = (int)C.stages[first].ties.size();
    int cols = 1; for (int n = N; n > 1; n /= 2) cols++;
    int cw = std::min(150, (VW - 20) / cols), x0 = 10;
    int top = y0 + 16, bottom = VH - 26;
    auto slotY = [&](int col, int i) { int n = N >> col; float h = (float)(bottom - top) / std::max(1, n); return top + (int)(h * (i + 0.5f)); };
    auto tieText = [&](const Tie& t, int side) { int team = side ? t.b : t.a; return team >= 0 ? fitText(g_world.teams[team].shortName, cw - 34, 10) : std::string("exempt"); };
    for (int c = 0; c < cols; c++) {
        int si = -1;
        for (size_t k = 0; k < ks.size(); k++) if (ks[k] == first) si = (int)k;
        int stg = si >= 0 && si + c < (int)ks.size() ? ks[si + c] : -1;
        int n = N >> c; if (n < 1) n = 1;
        std::string title = stg >= 0 ? C.stages[stg].name : (n == 1 ? std::string("Finale") : n == 2 ? std::string("Demi-finales") : n == 4 ? std::string("Quarts de finale") : std::string("8es de finale"));
        drawTextCentered(fitText(title, cw - 6, 10), x0 + c * cw + cw / 2, y0 + 1, 10, C_HI, false);
        for (int i = 0; i < n; i++) {
            int cy = slotY(c, i), bx = x0 + c * cw + 3, bw2 = cw - 10;
            DrawRectangle(bx, cy - 11, bw2, 22, Color{ 20, 32, 64, 230 });
            DrawRectangleLines(bx, cy - 11, bw2, 22, Color{ 80, 110, 170, 255 });
            if (c > 0) { int py1 = slotY(c - 1, 2 * i), py2 = slotY(c - 1, 2 * i + 1); int lx = bx - 4; DrawLine(lx, py1, lx, py2, Color{ 120, 140, 190, 255 }); DrawLine(lx, cy, bx, cy, Color{ 120, 140, 190, 255 }); }
            if (stg >= 0 && i < (int)C.stages[stg].ties.size()) {
                const Tie& t = C.stages[stg].ties[i];
                int w = t.winner;
                for (int sd = 0; sd < 2; sd++) {
                    int team = sd ? t.b : t.a;
                    bool mine = team >= 0 && g_career.season.isControlled(team);
                    Color col = w >= 0 && team == w ? C_GOOD : mine ? C_HI : C_TXT;
                    if (team >= 0) drawKitIcon(g_world.teams[team].home, bx + 2, cy - 10 + sd * 10, 1);
                    drawTextPx(tieText(t, sd), bx + 14, cy - 11 + sd * 10, 10, col);
                }
                // score (match unique ou cumul aller-retour)
                std::string sc;
                if (t.m1 >= 0 && C.matches[t.m1].played) {
                    const MatchRes& a = C.matches[t.m1];
                    int ga = a.hg, gb = a.ag;
                    if (t.m2 >= 0 && C.matches[t.m2].played) { ga += C.matches[t.m2].ag; gb += C.matches[t.m2].hg; }
                    sc = fmt("%d-%d", ga, gb);
                    const MatchRes& last = t.m2 >= 0 && C.matches[t.m2].played ? C.matches[t.m2] : a;
                    if (last.ph >= 0) sc += "*";
                }
                if (!sc.empty()) drawTextPx(sc, bx + bw2 - textWidth(sc, 10) - 3, cy - 6, 10, C_DIM);
            } else {
                std::string a = fmt("Vainq. %d", 2 * i + 1), b = fmt("Vainq. %d", 2 * i + 2);
                if (c == 0) { a = "?"; b = "?"; }
                drawTextPx(a, bx + 14, cy - 11, 10, C_DIM); drawTextPx(b, bx + 14, cy - 1, 10, C_DIM);
            }
        }
    }
    drawTextPx("* : tirs au but", 10, VH - 24, 10, C_DIM);
}

static void screenCompView() {
    Season& S = g_career.season;
    const Competition& C = S.comps[g_cv];
    drawBackground(C.name);
    if (C.stages.empty()) { drawTextCentered("En attente du tirage", VW / 2, 80, 10, C_DIM); drawFooter("Retour"); if (IN.back) g_screen = SC_COMPS; return; }
    if (g_cvStage < 0 || g_cvStage >= (int)C.stages.size()) g_cvStage = 0;
    int nst = (int)C.stages.size();
    auto setStage = [&](int st) { g_cvStage = st; g_cvGroup = 0; g_cvRound = -1; g_cvLW = ListW();
        for (int g = 0; g < (int)C.stages[st].groups.size(); g++) for (int t : C.stages[st].groups[g]) if (S.isControlled(t)) g_cvGroup = g; };
    if (IN.pgUp) setStage((g_cvStage - 1 + nst) % nst);
    if (IN.pgDn) setStage((g_cvStage + 1) % nst);
    int y = 31;
    // ---- onglets des phases
    if (nst > 1) {
        int bw = std::min(150, (VW - 20) / nst);
        auto shortSt = [](std::string n) {
            const char* R[][2] = { { "tour de qualification", "tour qualif." }, { "phase de groupes", "phase" }, { "Quarts de finale", "Quarts" }, { "Demi-finales", "Demies" },
                                   { "de finale", "" }, { "Tour préliminaire", "Tour prélim." } };
            for (auto& r : R) { size_t k = n.find(r[0]); if (k != std::string::npos) n = n.substr(0, k) + r[1] + n.substr(k + strlen(r[0])); }
            while (!n.empty() && n.back() == ' ') n.pop_back();
            return n; };
        for (int i = 0; i < nst; i++) if (button(10 + i * bw, y, bw - 2, 14, shortSt(C.stages[i].name), i == g_cvStage)) setStage(i);
        y += 16;
    }
    const Stage& st = C.stages[g_cvStage];
    bool league = st.type != ST_KO;
    // ---- onglets de mode
    static const char* MODES[] = { "Classement", "Résultats", "Buteurs", "Passeurs", "Règles" };
    {
        int bw = 90;
        for (int i = 0; i < 5; i++) {
            if (button(10 + i * (bw + 2), y, bw, 14, i == 0 && !league ? "Tableau" : MODES[i], g_cvMode == i)) { g_cvMode = i; g_cvLW = ListW(); }
        }
        if (IN.tab) { g_cvMode = (g_cvMode + 1) % 5; g_cvLW = ListW(); }
        y += 17;
    }
    if (!league && g_cvMode == 0) {
        drawCupBracket(C, y);
        drawFooter("Tab : onglet   LB/RB : phase   Retour");
        if (IN.back) { g_screen = g_pickMode == PM_BROWSE && !g_stack.empty() ? SC_PICK : SC_COMPS; }
        return;
    }
    std::string help = "Tab : onglet";
    if (nst > 1) help += "   LB/RB : phase";
    auto backTo = [&]() { g_screen = g_pickMode == PM_BROWSE && !g_stack.empty() ? SC_PICK : SC_COMPS; };
    if (g_cvMode == 4) {
        // règlement mis en forme : titre, sous-titres, puces ; défilement
        static int scroll = 0, lastComp = -1;
        if (lastComp != g_cv) { scroll = 0; lastComp = g_cv; }
        std::string rules = g_career.divisionRules(g_cv);
        struct RL { int type; std::string t; };      // 0 texte, 1 titre, 2 sous-titre, 3 puce, 4 suite de puce
        std::vector<RL> lines;
        std::string cur;
        for (size_t i = 0; i <= rules.size(); i++) {
            if (i < rules.size() && rules[i] != '\n') { cur += rules[i]; continue; }
            if (cur.rfind("## ", 0) == 0) lines.push_back({ 2, cur.substr(3) });
            else if (cur.rfind("# ", 0) == 0) lines.push_back({ 1, cur.substr(2) });
            else if (cur.rfind("- ", 0) == 0) { auto w = wrapText(cur.substr(2), VW - 80); for (size_t k = 0; k < w.size(); k++) lines.push_back({ k ? 4 : 3, w[k] }); }
            else if (!cur.empty()) for (auto& l : wrapText(cur, VW - 60)) lines.push_back({ 0, l });
            cur.clear();
        }
        int top = y + 2, bottom = VH - 22;
        int total = 0; for (auto& l : lines) total += l.type == 1 ? 18 : l.type == 2 ? 16 : 11;
        int maxScroll = std::max(0, total - (bottom - top));
        if (IN.down || IN.wheel < 0) scroll = std::min(maxScroll, scroll + (IN.wheel < 0 ? 33 : 11));
        if (IN.up || IN.wheel > 0) scroll = std::max(0, scroll - (IN.wheel > 0 ? 33 : 11));
        BeginScissorMode(0, top, VW, bottom - top);
        int yy = top - scroll;
        for (auto& l : lines) {
            int h = l.type == 1 ? 18 : l.type == 2 ? 16 : 11;
            if (yy + h >= top - 20 && yy <= bottom) {
                if (l.type == 1) { DrawRectangle(16, yy, VW - 32, 15, Color{ 30, 50, 100, 255 }); DrawRectangle(16, yy + 14, VW - 32, 1, C_SEL); drawTextPx(fitText(l.t, VW - 50, 10), 22, yy + 3, 10, C_HI); }
                else if (l.type == 2) { DrawRectangle(20, yy + 3, 4, 9, C_SEL); drawTextPx(l.t, 30, yy + 3, 10, Color{ 170, 210, 255, 255 }); DrawRectangle(30, yy + 14, textWidth(l.t, 10), 1, Color{ 70, 100, 160, 255 }); }
                else if (l.type == 3) { DrawRectangle(40, yy + 4, 3, 3, C_HI); drawTextPx(l.t, 50, yy, 10, C_TXT); }
                else if (l.type == 4) drawTextPx(l.t, 50, yy, 10, C_TXT);
                else drawTextPx(l.t, 26, yy, 10, C_TXT);
            }
            yy += h;
        }
        EndScissorMode();
        if (maxScroll > 0) {
            int bh = std::max(12, (bottom - top) * (bottom - top) / std::max(1, total));
            int by = top + (bottom - top - bh) * scroll / std::max(1, maxScroll);
            DrawRectangle(VW - 10, top, 4, bottom - top, Color{ 30, 40, 70, 255 }); DrawRectangle(VW - 10, by, 4, bh, C_SEL);
        }
        drawFooter(help + (maxScroll > 0 ? "   Haut/Bas : défiler" : "") + "   Retour");
        if (IN.back) backTo();
        return;
    }
    if (g_cvMode == 2 || g_cvMode == 3) {
        buildScorers(C, g_cv);
        auto& v = g_scorers[g_cvMode - 2];
        drawTextPx("Rang  Joueur", 16, y, 10, C_DIM); drawTextPx("Club", 300, y, 10, C_DIM); drawTextPx(g_cvMode == 2 ? "Buts" : "Passes", VW - 80, y, 10, C_DIM);
        y += 12;
        int rows = (VH - 18 - y) / 12;
        int sel = listRun(g_cvLW, (int)v.size(), 12, y, VW - 24, rows, 12, [&](int i, int x, int yy, bool s2) {
            const ScorerRow& r = v[i];
            Color c = s2 ? BLACK : (S.isControlled(r.team) ? C_HI : C_TXT);
            int rank = i + 1; while (rank > 1 && v[rank - 2].n == r.n) rank--;
            drawTextPx(fmt("%3d", rank), x + 4, yy + 1, 10, c);
            drawTextPx(fitText(r.name, 230, 10), x + 40, yy + 1, 10, c);
            drawKitIcon(g_world.teams[r.team].home, x + 276, yy + 1, 1);
            drawTextPx(fitText(nameLvl(r.team), 230, 10), x + 290, yy + 1, 10, s2 ? BLACK : C_DIM);
            drawTextPx(r.pens ? fmt("%d (%d pen.)", r.n, r.pens) : fmt("%d", r.n), x + VW - 92, yy + 1, 10, c);
        });
        if (v.empty()) drawTextCentered("Aucun but marqué pour l'instant", VW / 2, y + 20, 10, C_DIM);
        if (sel >= 0) { openFiche(v[sel].team, SC_COMPVIEW); return; }
        drawFooter(help + "   OK : fiche du club   Retour");
        if (IN.back) backTo();
        return;
    }
    // zones de montée / descente
    int upN = 0, downN = 0, barN = 0, barDown = 0; bool terminal = false;
    int eu1 = 0, eu3 = 0, eu4 = 0;       // places européennes (C1, C3, C4) de la 1re division
    bool wPO = false;                     // Arkema Première Ligue : play-offs des 4 premières
    if (C.kind == 1 && C.tag >= 0 && g_career.kind == CK_CLUB) {
        int p = C.tag / 100000, q = (C.tag / 100) % 1000;
        if (p < (int)g_career.pyramids.size() && q < (int)g_career.pyramids[p].pools.size()) {
            const Pyramid& PY = g_career.pyramids[p];
            const Pool& pl = PY.pools[q];
            const TierConf& T = PY.tiers[pl.tier];
            upN = pl.tier > 0 ? T.up : 0; downN = T.flexible || pl.terminal ? 0 : T.down; barN = T.barrageUp ? (T.barrageUp == 2 ? 4 : T.barrageUp == 3 ? 3 : 1) : 0;
            if (pl.tier + 1 < (int)PY.tiers.size() && PY.tiers[pl.tier + 1].barrageUp && downN) barDown = 1;     // barrage de maintien
            terminal = pl.terminal;
            if (pl.tier == 0 && PY.country == "F:FRA") { wPO = true; eu1 = 4; }
            else if (pl.tier == 0 && PY.dom < 0 && !g_career.euroOnly && !isWomenPyramid(PY)) {
                int rk = g_career.uefaRank(PY.country);
                if (rk <= 55) {
                    // accès selon le rang UEFA de l'association (formule actuelle, simplifiée)
                    eu1 = rk <= 4 ? 4 : rk <= 6 ? 3 : rk <= 10 ? 2 : 1;
                    eu3 = rk <= 6 ? 2 : rk <= 15 ? 1 : 0;
                    eu4 = rk <= 33 ? 1 : 2;
                    if (rk > 33) eu3 = 0;
                    if (!g_career.opts.euroFormat) { eu3 += eu4; eu4 = 0; }      // formule 2003 : pas de Ligue Conférence
                }
            }
        }
    }
    // ---- onglets des groupes
    int ng = (int)st.groups.size();
    if (league && ng > 1) {
        int bw = std::max(22, std::min(80, (VW - 20) / ng));
        int perRow = (VW - 20) / bw;
        for (int g = 0; g < ng; g++) {
            int bx = 10 + (g % perRow) * bw, by = y + (g / perRow) * 15;
            bool mine = false; for (int t : st.groups[g]) if (S.isControlled(t)) mine = true;
            std::string lb = ng <= 26 ? fmt("%s%c", bw >= 70 ? "Groupe " : bw >= 50 ? "Gr. " : "", 'A' + g) : fmt("%d", g + 1);
            if (button(bx, by, bw - 2, 13, lb + (mine ? "*" : ""), g == g_cvGroup)) { g_cvGroup = g; g_cvLW = ListW(); }
        }
        y += 15 * ((ng + perRow - 1) / perRow) + 2;
        if (g_cvMode == 0) {
            if (IN.left) { g_cvGroup = (g_cvGroup - 1 + ng) % ng; g_cvLW = ListW(); }
            if (IN.right) { g_cvGroup = (g_cvGroup + 1) % ng; g_cvLW = ListW(); }
        }
    }
    if (g_cvGroup >= ng) g_cvGroup = 0;
    if (league && g_cvMode == 0) {
        auto tb = st.type == ST_SWISS ? C.swissTable(g_cvStage) : C.table(g_cvStage, g_cvGroup);
        int x0 = 16;
        drawTextPx("Pos  Équipe", x0, y, 10, C_DIM);
        const char* cols[] = { "J", "G", "N", "P", "BP", "BC", "Diff", "Pts" };
        int cx[] = { 380, 405, 430, 455, 480, 510, 540, 580 };
        for (int k = 0; k < 8; k++) drawTextPx(cols[k], cx[k], y, 10, C_DIM);
        y += 12;
        int rows = (VH - 32 - y) / 12;
        int n = (int)tb.size();
        int sel = listRun(g_cvLW, n, x0 - 4, y, VW - 24, rows, 12, [&](int i, int x, int yy, bool sel2) {
            const Standing& s = tb[i];
            bool me = S.isControlled(s.team);
            Color c = sel2 ? BLACK : (me ? C_HI : C_TXT);
            Color zone = BLANK;
            static const Color Z_C1 = { 70, 140, 255, 255 }, Z_C3 = { 255, 140, 40, 255 }, Z_C4 = { 60, 200, 120, 255 }, Z_BAR = { 250, 220, 60, 255 };
            if (i < upN) zone = C_GOOD;
            else if (barN && i >= upN && i < upN + barN && C.kind == 1) zone = Z_BAR;
            if (eu1 && i < eu1) zone = Z_C1;
            else if (eu1 && i < eu1 + eu3) zone = Z_C3;
            else if (eu1 && i < eu1 + eu3 + eu4) zone = Z_C4;
            if (downN && i >= n - downN) zone = C_BAD;
            else if (barDown && i == n - downN - 1) zone = Z_BAR;
            if (zone.a && C.kind == 1) DrawRectangle(x + 3, yy, VW - 30, 11, Color{ zone.r, zone.g, zone.b, 28 });   // bande de couleur sur toute la ligne
            if (C.kind != 1 && i < C.groupsAdvance) zone = C_GOOD;
            if (st.type == ST_SWISS) zone = i < 8 ? C_GOOD : i < 24 ? Color{ 255, 170, 60, 255 } : C_BAD;
            if (zone.a) DrawRectangle(x, yy, 3, 11, zone);
            drawTextPx(fmt("%2d", i + 1), x + 4, yy + 1, 10, c);
            drawKitIcon(g_world.teams[s.team].home, x + 24, yy + 1, 1);
            drawTextPx(fitText(C.kind == 1 ? g_world.teams[s.team].name : nameLvl(s.team), 320, 10), x + 36, yy + 1, 10, c);
            int v[] = { s.p, s.w, s.d, s.l, s.gf, s.ga, s.gd(), s.pts };
            for (int k = 0; k < 8; k++) drawTextPx(k == 6 ? fmt("%+d", v[k]) : fmt("%d", v[k]), cx[k] - x0 + x + 4, yy + 1, 10, c);
        });
        if (sel >= 0) { openFiche(tb[sel].team, SC_COMPVIEW); return; }
        std::string info;
        if (C.kind == 1) {
            // légende en couleur
            int lx = 12;
            auto leg = [&](Color c, const std::string& t) { DrawRectangle(lx, VH - 26, 8, 8, c); drawTextPx(t, lx + 11, VH - 28, 10, C_DIM); lx += 20 + textWidth(t, 10); };
            if (eu1) { leg(Color{ 70, 140, 255, 255 }, wPO ? "Play-offs (titre)" : "Ligue des champions"); if (eu3) leg(Color{ 255, 140, 40, 255 }, g_career.opts.euroFormat ? "Ligue Europa" : "Coupe UEFA"); if (eu4) leg(Color{ 60, 200, 120, 255 }, "Ligue Conférence"); }
            if (upN) leg(C_GOOD, "Montée");
            if (barN || barDown) leg(Color{ 250, 220, 60, 255 }, "Barrages");
            if (downN) leg(C_BAD, "Relégation");
            if (terminal) drawTextPx("Dernier niveau (pas de descente)", lx, VH - 28, 10, C_DIM);
        } else drawTextPx(fmt("Vert : qualifiés   Victoire : %d pts / Nul : %d pt",C.ptsWin,C.ptsDraw), 12, VH - 27, 10, C_DIM);
        help += ng > 1 ? "   G/D : groupe   OK : fiche" : "   OK : fiche du club";
    } else {
        int nr = (int)st.rounds.size();
        if (nr == 0) { drawFooter("Retour"); if (IN.back) backTo(); return; }
        if (g_cvRound < 0 || g_cvRound >= nr) { g_cvRound = 0; for (int r = 0; r < nr; r++) if (st.rounds[r].done) g_cvRound = std::min(nr - 1, r + 1); }
        if (IN.left) { g_cvRound = (g_cvRound - 1 + nr) % nr; g_cvLW = ListW(); }
        if (IN.right) { g_cvRound = (g_cvRound + 1) % nr; g_cvLW = ListW(); }
        const Round& R = st.rounds[g_cvRound];
        if (button(10, y, 20, 13, "<")) { g_cvRound = (g_cvRound - 1 + nr) % nr; g_cvLW = ListW(); }
        if (button(VW - 30, y, 20, 13, ">")) { g_cvRound = (g_cvRound + 1) % nr; g_cvLW = ListW(); }
        drawTextCentered(R.name + "   (" + dateOf(R.time, g_career.year) + ")" + (nr > 1 ? fmt("   %d/%d", g_cvRound + 1, nr) : ""), VW / 2, y + 2, 10, C_TXT, false);
        y += 16;
        std::vector<int> ms;
        for (int mi : R.m) {
            // en phase de groupes : filtre sur le groupe affiché
            if (league && ng > 1 && C.matches[mi].group >= 0 && C.matches[mi].group != g_cvGroup) continue;
            ms.push_back(mi);
        }
        int rows = (VH - 32 - y) / 12;
        bool cup = C.kind != 1;
        // coupes d'Europe (nouvelle formule) : qualifications séparées en voie des champions / voie de la ligue
        bool paths = !league && !C.extra2.empty() && std::find(C.koNames.begin(), C.koNames.begin() + std::min((int)C.koNames.size(), C.regionalRounds), st.name) != C.koNames.begin() + std::min((int)C.koNames.size(), C.regionalRounds);
        std::set<int> champs(C.extra2.begin(), C.extra2.end());
        if (paths) std::stable_sort(ms.begin(), ms.end(), [&](int a, int b) { bool ca = champs.count(C.matches[a].home) || champs.count(C.matches[a].away), cb = champs.count(C.matches[b].home) || champs.count(C.matches[b].away); return ca > cb; });
        int sel = listRun(g_cvLW, (int)ms.size(), 12, y, VW - 24, rows, 12, [&](int i, int x, int yy, bool sel2) {
            const MatchRes& m = C.matches[ms[i]];
            if (paths) {
                bool cp = champs.count(m.home) || champs.count(m.away);
                DrawRectangle(x + 1, yy + 1, 3, 10, cp ? Color{ 240, 200, 60, 255 } : Color{ 90, 160, 240, 255 });
                drawTextPx(cp ? "Voie champions" : "Voie ligue", x + 8, yy + 1, 10, sel2 ? BLACK : cp ? C_HI : Color{ 150, 190, 250, 255 });
            }
            bool me = S.isControlled(m.home) || S.isControlled(m.away);
            Color c = sel2 ? BLACK : (me ? C_HI : C_TXT);
            std::string hn = cup ? nameLvl(m.home) : g_world.teams[m.home].name, an = cup ? nameLvl(m.away) : g_world.teams[m.away].name;
            hn = fitText(hn, 250, 10); an = fitText(an, 230, 10);
            drawTextPx(hn, x + VW / 2 - 52 - textWidth(hn, 10), yy + 1, 10, c);
            std::string sc = m.played ? fmt("%d - %d", m.hg, m.ag) : "  -  ";
            drawTextCentered(sc, x + VW / 2 - 24, yy + 1, 10, c, false);
            drawTextPx(an, x + VW / 2 + 6, yy + 1, 10, c);
            std::string ex;
            if (m.played && m.aet) ex += "ap";
            if (m.played && m.ph >= 0) ex += fmt(" %d-%d tab", m.ph, m.pa);
            if (!ex.empty()) drawTextPx(ex, x + VW - 84, yy + 1, 10, sel2 ? BLACK : C_DIM);
        });
        if (ms.empty()) drawTextCentered("Aucun match", VW / 2, y + 20, 10, C_DIM);
        if (sel >= 0) { openMatchInfo(g_cv, ms[sel], SC_COMPVIEW); return; }
        if (!league) {
            int byes = 0; for (auto& t : st.ties) if (t.b < 0) byes++;
            if (byes) drawTextPx(fmt("%d équipe(s) exemptée(s)", byes), 12, VH - 27, 10, C_DIM);
        }
        help += "   G/D : journée / tour   OK : feuille de match";
    }
    drawFooter(fitText(help + "   Retour", VW - 12, 10));
    if (IN.back) backTo();
}

// ------------------------------------------------------------------ calendrier
static ListW g_fixLW;
static void screenFixtures() {
    drawBackground("Calendrier de l'équipe");
    Season& S = g_career.season;
    auto v = userMatches(false);
    static bool first = true;
    if (first) { first = false; }
    int s = listRun(g_fixLW, (int)v.size(), 12, 36, VW - 24, 24, 12, [&](int i, int x, int y, bool sel) {
        const Competition& C = S.comps[v[i].first];
        const MatchRes& m = C.matches[v[i].second];
        Color c = sel ? BLACK : C_TXT;
        double tm = 0; for (auto& st : C.stages) for (auto& R : st.rounds) for (int xx : R.m) if (xx == v[i].second) tm = R.time;
        drawTextPx(fitText(dateOf(tm, g_career.year), 90, 10), x + 4, y + 1, 10, sel ? BLACK : C_DIM);
        drawTextPx(fitText(C.shortName + " " + roundNameOf(C, v[i].second), 130, 10), x + 96, y + 1, 10, sel ? BLACK : C_DIM);
        std::string s2 = nameLvl(m.home) + "  " + (m.played ? fmt("%d-%d", m.hg, m.ag) : "-") + "  " + nameLvl(m.away);
        if (m.played && m.ph >= 0) s2 += fmt(" (%d-%d tab)", m.ph, m.pa);
        if (m.played) {
            int me = S.isControlled(m.home) ? m.home : m.away;
            int gf = me == m.home ? m.hg : m.ag, ga = me == m.home ? m.ag : m.hg;
            Color rc = gf > ga ? C_GOOD : gf < ga ? C_BAD : C_HI;
            if (gf == ga && m.ph >= 0) rc = ((me == m.home) == (m.ph > m.pa)) ? C_GOOD : C_BAD;
            DrawRectangle(x + 230, y + 2, 4, 7, rc);
        }
        drawTextPx(fitText(s2, VW - 280, 10), x + 238, y + 1, 10, c);
    });
    if (s >= 0) { openMatchInfo(v[s].first, v[s].second, SC_FIXTURES); return; }
    drawFooter("OK : feuille de match   Retour");
    if (IN.back) openHub();
}

// ------------------------------------------------------------------ effectif (titulaires, tactique)
static int g_squadPick = 0;
static int g_squadTeamIdx = 0;
static ListW g_squadLW;
static void screenSquad() {
    if(g_career.sportingMode()){openSporting(8);return;}
    Season& S = g_career.season;
    std::vector<int> teams = g_career.kind == 0 ? std::vector<int>{ g_career.userTeam } : S.controlled;
    if (g_career.kind == 0) for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].parent == g_career.userTeam) teams.push_back(i);
    if (g_squadTeamIdx >= (int)teams.size()) g_squadTeamIdx = 0;
    int ti = teams[g_squadTeamIdx];
    g_world.ensureSquad(ti);
    Team& T = g_world.teams[ti];
    drawBackground("Effectif et tactique");
    drawKitIcon(T.home, 12, 32, 2);
    drawTextPx(T.name, 34, 32, 10, C_TXT);
    bool manual = T.xi.size() == 11;
    // boutons
    if (button(34, 44, 150, 13, "Tactique : " + std::string(FORMATIONS[T.formation].name))) T.formation = (T.formation + 1) % NUM_FORMATIONS;
    if (button(188, 44, 170, 13, manual ? "Titulaires : choix manuel" : "Titulaires : automatique", manual)) { if (manual) T.xi.clear(); else { auto lu = g_world.pickLineup(ti, T.formation); T.xi.clear(); for (int k = 0; k < 11; k++) T.xi.push_back(T.squad[lu[k]].id); } }
    if (g_career.kind == CK_CLUB && careerRules()) {
        drawTextPx(std::string("Statut ") + statusName(T.status) + "   Salaires : " + money(g_career.wageBill(ti)) + " / an", 364, 46, 10, C_DIM);
        int cur = g_squadLW.cur;
        if (T.status == CS_SEMIPRO && cur >= 0 && cur < (int)T.squad.size()) {
            Player& P = T.squad[cur];
            if (button(364, 31, 262, 13, P.contract == 1 ? "Passer " + P.name + " en licence amateur" : "Offrir un contrat semi-pro à " + P.name)) {
                P.contract = P.contract == 1 ? 2 : 1;
                toast(P.contract == 1 ? "Contrat semi-pro signé" : "Joueur repassé sous licence amateur");
            }
        }
    }
    // mentalité, capitaine et vice-capitaine
    if (button(34, 58, 150, 13, std::string("Mentalité : ") + mentalityName(T.mentality))) T.mentality = careerRules()?(T.mentality+1)%5:1+T.mentality%3;
    {
        int cur = g_squadLW.cur;
        auto nameOf = [&](int pid) { for (auto& q : T.squad) if (q.id == pid) return q.name; return std::string("auto"); };
        if (button(188, 58, 200, 13, "Capitaine : " + fitText(nameOf(T.captainPid), 120, 10)) && cur >= 0 && cur < (int)T.squad.size()) {
            if(g_career.personalityEnabled())g_career.personalityCaptain((int)(&T-&g_world.teams[0]),T.squad[cur].id);else T.captainPid = T.squad[cur].id; if (T.vicePid == T.captainPid) T.vicePid = 0; toast(T.squad[cur].name + " est le capitaine");
        }
        if (button(392, 58, 230, 13, "Vice-capitaine : " + fitText(nameOf(T.vicePid), 120, 10)) && cur >= 0 && cur < (int)T.squad.size()) {
            T.vicePid = T.squad[cur].id; if (T.captainPid == T.vicePid) T.captainPid = 0; toast(T.squad[cur].name + " est vice-capitaine");
        }
    }
    {
        int cur = g_squadLW.cur;
        if ((g_career.kind == CK_CLUB||g_career.coach) && ti == g_career.userTeam && button(290, VH - 32, 140, 13, careerRules()?"Consignes / adjoint":"Consignes")) { g_screen = SC_TACTICS; return; }
        if (cur >= 0 && cur < (int)T.squad.size() && (button(12, VH - 32, 150, 13, "Fiche de " + fitText(T.squad[cur].name, 90, 10)) || IsKeyPressed(KEY_F))) { openPlayer(ti, cur, SC_SQUAD); return; }
    }
    if (IN.tab) T.formation = (T.formation + 1) % NUM_FORMATIONS;
    if (teams.size() > 1 && (IN.pgUp || IN.pgDn)) { g_squadTeamIdx = (g_squadTeamIdx + (IN.pgUp ? -1 : 1) + (int)teams.size()) % (int)teams.size(); return; }
    auto lu = g_world.pickLineup(ti, T.formation,careerRules()?RULESET_CAREER:RULESET_SIMPLE);
    std::set<int> starters(lu.begin(), lu.begin() + 11);
    int y = 76;
    const char* cols[] = { "ÂGE", "VIT", "TIR", "PAS", "TAC", "DRI", "TÊT", "GAR", "NOTE", "COND", "MOR", "B", "PD", "CONTRAT" };
    int cx[] = { 214, 238, 262, 286, 310, 334, 358, 382, 408, 440, 474, 504, 522, 546 };
    for (int k = 0; k < 14; k++) if(careerRules()||k!=10&&k!=13)drawTextPx(cols[k], cx[k], y, 10, C_DIM);
    drawTextPx("N°  P   Joueur", 16, y, 10, C_DIM);
    y += 12;
    int sel = listRun(g_squadLW, (int)T.squad.size(), 12, y, VW - 24, 19, 12, [&](int i, int x, int yy, bool s2) {
        const Player& p = T.squad[i];
        bool tit = starters.count(i) > 0;
        Color c = s2 ? BLACK : (tit ? C_TXT : C_DIM);
        if (tit) DrawRectangle(x, yy, 3, 11, C_GOOD);
        drawTextPx(fmt("%2d  %s", p.num, posName(p.pos)), x + 6, yy + 1, 10, c);
        std::string badge = p.id == T.captainPid ? " (C)" : p.id == T.vicePid ? " (VC)" : "";
        drawTextPx(fitText(p.name, 112 - textWidth(badge, 10), 10) + badge, x + 52, yy + 1, 10, c);
        if (p.suspended > 0) drawTextPx("SUSP", x + 168, yy + 1, 10, C_BAD);
        else if (p.injured > 0) drawTextPx("BLES", x + 168, yy + 1, 10, C_BAD);
        int v[] = { p.age, p.speed, p.shoot, p.pass, p.tackle, p.drib(), p.head(), p.keep, p.overall() };
        for (int k = 0; k < 9; k++) drawTextPx(fmt("%d", v[k]), cx[k] - 12 + x + 4, yy + 1, 10, k == 8 ? (s2 ? BLACK : C_HI) : c);
        int cd = playerCond(ti, p);
        Color cc = cd >= 80 ? C_GOOD : cd >= 60 ? C_HI : C_BAD;
        DrawRectangle(cx[9] - 12 + x, yy + 3, 14, 5, Color{ 0, 0, 0, 120 });
        DrawRectangle(cx[9] - 12 + x, yy + 3, cd * 14 / 100, 5, cc);
        drawTextPx(fmt("%d", cd), cx[9] + 4 + x, yy + 1, 10, s2 ? BLACK : cc);
        Color mc = p.morale >= 70 ? C_GOOD : p.morale >= 45 ? C_TXT : C_BAD;
        if(careerRules())drawTextPx(p.morale >= 80 ? "++" : p.morale >= 62 ? "+" : p.morale >= 45 ? "=" : p.morale >= 30 ? "-" : "--", cx[10] - 6 + x, yy + 1, 10, s2 ? BLACK : mc);
        drawTextPx(fmt("%d", p.goals), cx[11] - 12 + x + 4, yy + 1, 10, c);
        drawTextPx(fmt("%d", p.assists), cx[12] - 12 + x + 4, yy + 1, 10, c);
        static const char* CT[] = { "Pro", "Semi", "Ama" };
        if(careerRules())drawTextPx(fitText(std::string(CT[p.contract % 3]) + (p.contract < 2 ? " " + money(playerWage(p)) : ""), 84, 10), cx[13] - 12 + x + 4, yy + 1, 10, c);
    });
    // clic / OK : bascule titulaire (passe en mode manuel)
    if (sel >= 0) {
        int id = T.squad[sel].id;
        if (T.xi.size() != 11) { T.xi.clear(); for (int k = 0; k < 11; k++) T.xi.push_back(T.squad[lu[k]].id); }
        auto it = std::find(T.xi.begin(), T.xi.end(), id);
        if (it != T.xi.end()) toast("Choisissez un remplaçant pour prendre sa place");
        if (it != T.xi.end()) { g_squadPick = id; }
        else if (g_squadPick > 0) {
            auto jt = std::find(T.xi.begin(), T.xi.end(), g_squadPick);
            if (jt != T.xi.end()) *jt = id;
            g_squadPick = 0;
            toast("Composition modifiée");
        } else {
            // remplace le titulaire le plus faible au même poste
            int wi = -1, wv = 999;
            for (int k = 0; k < 11; k++) for (auto& q : T.squad) if (q.id == T.xi[k] && q.pos == T.squad[sel].pos && q.overall() < wv) { wv = q.overall(); wi = k; }
            if (wi >= 0) { T.xi[wi] = id; toast("Titulaire remplacé"); } else toast("Sélectionnez d'abord le titulaire à remplacer");
        }
    }
    // mouvements entre l'équipe première et les réserves
    if (g_career.kind == 0 && teams.size() > 1) {
        int cur = g_squadLW.cur;
        if (cur >= 0 && cur < (int)T.squad.size()) {
            int dest = ti == g_career.userTeam ? teams[1] : g_career.userTeam;
            std::string lbl = ti == g_career.userTeam ? "Envoyer en " + g_world.teams[dest].shortName : "Rappeler en équipe première";
            if (button(VW - 200, VH - 32, 188, 13, lbl)) {
                int minSrc = ti == g_career.userTeam ? 16 : 14;
                if ((int)T.squad.size() <= minSrc) toast("Effectif trop réduit");
                else if (g_world.teams[dest].youth == 1 && T.squad[cur].age > 19) toast("Trop âgé pour les U19 (19 ans maximum)");
                else if (g_world.teams[dest].youth == 2 && T.squad[cur].age > 16) toast("Trop âgé pour les U17 (16 ans maximum)");
                else if (g_world.teams[dest].youth == 3 && T.squad[cur].age > 14) toast("Trop âgé pour les U15 (14 ans maximum)");
                else {
                    g_world.ensureSquad(dest);
                    Player p = T.squad[cur];
                    Team& D = g_world.teams[dest];
                    std::vector<bool> used(100, false); for (auto& q : D.squad) if (q.num < 100) used[q.num] = true;
                    if (p.num >= 100 || used[p.num]) { p.num = 1; while (p.num < 99 && used[p.num]) p.num++; }
                    T.xi.erase(std::remove(T.xi.begin(), T.xi.end(), p.id), T.xi.end());
                    T.squad.erase(T.squad.begin() + cur);
                    D.squad.push_back(p);
                    toast(p.name + " rejoint " + D.name);
                }
            }
        }
        drawTextPx("PgPréc/PgSuiv : équipe", 170, VH - 30, 10, C_DIM);
    }
    std::string foot = "Clic/OK : titulaire/remplaçant   Tab : système   F : fiche du joueur   Capitaine : joueur puis bouton";
    if (g_squadPick > 0) foot = "Choisissez le joueur qui entre dans le onze   (Retour : annuler)";
    drawFooter(foot);
    if (IN.back) { if (g_squadPick > 0) g_squadPick = 0; else openHub(); }
}

// ------------------------------------------------------------------ palmarès
static ListW g_histLW;
static int g_histTab = 0;
static void screenHistory() {
    drawBackground("Palmarès");
    int countryP=g_career.selectedPyramid();
    if(g_career.kind==CK_CLUB && countryP>=0 && g_career.pyramids[countryP].country!="FRA") {
        std::vector<std::string> competitions;
        auto add=[&](int c){if(c>=0 && c<(int)g_career.season.comps.size()){auto name=compFamily(g_career.season.comps[c].name).substr(0,47);if(std::find(competitions.begin(),competitions.end(),name)==competitions.end())competitions.push_back(name);}};
        for(const auto& pool:g_career.pyramids[countryP].pools) for(int c:pool.comps)add(c);
        add(g_career.domesticCup());for(int c:g_career.superCups)if(g_career.season.comps[c].tag==countryP)add(c);
        int nat=g_world.nationIndex(g_career.selectedCountryCode().c_str());
        if(nat>=0) {if(NATIONS[nat].conf==UEFA){add(g_career.ucl);add(g_career.uel);add(g_career.uecl);} else for(int c=0;c<(int)g_career.season.comps.size();++c)if(isContinentalKind(g_career.season.comps[c].kind)&&g_career.season.comps[c].tag==NATIONS[nat].conf)add(c);}
        static int index=0; if(index>=(int)competitions.size())index=0;
        int n=std::max(1,(int)competitions.size());
        if(IN.left || IN.pgUp) {index=(index+n-1)%n;g_histLW=ListW();}
        if(IN.right || IN.pgDn || IN.tab) {index=(index+1)%n;g_histLW=ListW();}
        drawTextCentered(g_career.selectedCountryName(),VW/2,34,10,C_HI);
        drawTextCentered(competitions.empty()?"Palmarès national":competitions[index],VW/2,50,10,C_TXT);
        std::vector<const CompArchRec*> records;
        for(auto& a:g_career.compArch)if(!competitions.empty() && competitions[index]==a.comp)records.push_back(&a);
        std::stable_sort(records.begin(),records.end(),[](auto a,auto b){return a->year>b->year;});
        if(records.empty())drawTextCentered("Aucune saison terminée pour cette compétition.",VW/2,86,10,C_DIM);
        listRun(g_histLW,(int)records.size(),18,72,VW-36,21,12,[&](int i,int x,int y,bool sel){auto& a=*records[i];drawTextPx(fmt("%d",a.year),x+4,y+1,10,sel?BLACK:C_DIM);if(a.winner>=0)drawTextPx(fitText(g_world.teams[a.winner].name,270,10),x+54,y+1,10,sel?BLACK:C_HI);if(a.runner>=0)drawTextPx(fitText("2e : "+g_world.teams[a.runner].name,260,10),x+334,y+1,10,sel?BLACK:C_TXT);});
        drawFooter("Palmarès de votre carrière   Gauche/Droite / Tab : compétition   Retour");
        if(IN.back)openHub();return;
    }
    // onglets : compétitions (historique réel + saisons jouées) puis journal de la carrière
    static const int TABS[] = { HC_L1, HC_L2, HC_CDF, HC_CDL, HC_TDC, HC_UCL, HC_UEFA, HC_SUPERUEFA, HC_INTERTOTO, -2, -1 };
    static const char* TL[] = { "Ligue 1", "Ligue 2", "C. France", "C. Ligue", "Trophée", "Ligue ch.", "C. UEFA", "Supercoupe", "Intertoto", "Reconv.", "Carrière" };
    const int NT = 11;
    bool club = g_career.kind == CK_CLUB;
    if (!club) g_histTab = NT - 1;
    int bw = (VW - 20) / NT;
    if (club) for (int i = 0; i < NT; i++) if (button(10 + i * bw, 32, bw - 2, 14, TL[i], g_histTab == i)) { g_histTab = i; g_histLW = ListW(); }
    if (club && IN.pgUp) { g_histTab = (g_histTab + NT - 1) % NT; g_histLW = ListW(); }
    if (club && (IN.pgDn || IN.tab)) { g_histTab = (g_histTab + 1) % NT; g_histLW = ListW(); }
    int y = club ? 50 : 36;
    int hc = TABS[g_histTab];
    if (hc < 0) {
        auto& h = hc == -2 ? g_career.reconversions : g_career.history;
        if (h.empty()) drawTextCentered(hc == -2 ? "Aucune reconversion d'ancien joueur pour l'instant" : "Aucune saison terminée pour l'instant", VW / 2, 80, 10, C_DIM);
        listRun(g_histLW, (int)h.size(), 12, y, VW - 24, (VH - 20 - y) / 12, 12, [&](int i, int x, int yy, bool sel) {
            drawTextPx(fitText(h[h.size() - 1 - i], VW - 40, 10), x + 4, yy + 1, 10, sel ? BLACK : C_TXT);
        });
        drawFooter("PgPréc/PgSuiv / Tab : onglet   Retour");
        if (IN.back) openHub();
        return;
    }
    struct Row { int year; std::string w, r; int team; bool career; std::string venue; };
    std::vector<Row> rows;
    for (size_t k = 0; k < g_career.honourLog.size(); k++) {
        const HonourRec& e = g_career.honourLog[k];
        if (e.comp != hc) continue;
        rows.push_back({ e.year, e.winner >= 0 ? g_world.teams[e.winner].name : "?", e.runner >= 0 ? g_world.teams[e.runner].name : "", e.winner, true,
                         k < g_career.honourVenue.size() ? g_career.honourVenue[k] : std::string() });
    }
    int lastHist = 0;
    for (int i = histCount(hc) - 1; i >= 0; i--) {
        int yr; const char* w; const char* r; histEntry(hc, i, yr, w, r);
        lastHist = std::max(lastHist, yr);
        rows.push_back({ yr, w ? w : "", r ? r : "", resolveHistWinner(w), false, histVenue(hc, yr) });
    }
    std::stable_sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) { return a.year > b.year; });
    // classement des vainqueurs
    std::map<std::string, int> cnt;
    for (auto& r : rows) if (!r.w.empty() && r.w[0] != '(') cnt[r.w]++;
    std::vector<std::pair<int, std::string>> top;
    for (auto& kv : cnt) top.push_back({ -kv.second, kv.first });
    std::sort(top.begin(), top.end());
    bool single = hc == HC_TDC || hc == HC_SUPERUEFA || hc == HC_INTERTOTO;
    bool league = hc == HC_L1 || hc == HC_L2;
    drawTextPx("Saison", 16, y, 10, C_DIM); drawTextPx("Vainqueur", 80, y, 10, C_DIM);
    if (!league && hc != HC_INTERTOTO) drawTextPx("Finaliste", 222, y, 10, C_DIM);
    if (!league) drawTextPx("Lieu de la finale", 346, y, 10, C_DIM);
    int lw = league ? 400 : 480;
    drawSection(lw + 20, y, VW - lw - 30, "TITRES");
    y += 12;
    int u = g_career.userTeam;
    listRun(g_histLW, (int)rows.size(), 12, y, lw, (VH - 20 - y) / 12, 12, [&](int i, int x, int yy, bool sel) {
        const Row& r = rows[i];
        bool mine = r.team >= 0 && r.team == u;
        Color c = sel ? BLACK : mine ? C_GOOD : r.career ? C_HI : C_TXT;
        drawTextPx(single ? fmt("%d", r.year) : fmt("%d-%02d", r.year - 1, r.year % 100), x + 4, yy + 1, 10, c);
        if (r.team >= 0) drawKitIcon(g_world.teams[r.team].home, x + 62, yy + 1, 1);
        drawTextPx(fitText(r.w, league ? 300 : 132, 10), x + 76, yy + 1, 10, c);
        if (!league) {
            drawTextPx(fitText(r.r, 118, 10), x + 212, yy + 1, 10, sel ? BLACK : C_DIM);
            drawTextPx(fitText(r.venue.empty() ? "-" : r.venue, lw - 340, 10), x + 336, yy + 1, 10, sel ? BLACK : C_DIM);
        }
    });
    int yy = y + 2;
    for (int i = 0; i < (int)top.size() && yy < VH - 30; i++, yy += 11) {
        drawTextPx(fmt("%2d", -top[i].first), lw + 24, yy, 10, C_HI);
        drawTextPx(fitText(top[i].second, VW - lw - 56, 10), lw + 44, yy, 10, C_TXT);
    }
    drawFooter(fmt("Historique réel jusqu'en %d puis saisons de votre carrière (en jaune)   Tab : onglet   Retour", lastHist));
    if (IN.back) openHub();
}

// ------------------------------------------------------------------ sauvegardes
static std::string slotPath(int i) { return fmt("saves/partie%d.sav", i + 1); }
static std::string slotInfo(int i) {
    FILE* f = fopen(fmt("saves/partie%d.txt", i + 1).c_str(), "r");
    if (!f) return "(vide)";
    char buf[256] = { 0 };
    if (!fgets(buf, sizeof buf, f)) buf[0] = 0;
    fclose(f);
    std::string s = buf; while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
    return s;
}
static void screenSlots() {
    drawBackground(g_slotSave ? "Sauvegarder" : "Charger une partie");
    std::vector<std::string> items;
    int off = g_slotSave ? 0 : 1;
    if (!g_slotSave) {
        FILE* f = fopen("saves/auto.txt", "r");
        std::string info = "(vide)";
        if (f) { char buf[256] = { 0 }; if (fgets(buf, sizeof buf, f)) { info = buf; while (!info.empty() && (info.back() == '\n' || info.back() == '\r')) info.pop_back(); } fclose(f); }
        items.push_back("Sauvegarde automatique : " + info);
    }
    for (int i = 0; i < 5; i++) items.push_back(fmt("Emplacement %d : %s", i + 1, slotInfo(i).c_str()));
    int n = (int)items.size();
    int s = listRun(g_slotLW, n, 40, 50, VW - 80, n, 24, [&](int i, int x, int y, bool sel) {
        drawTextPx(fitText(items[i], VW - 100, 10), x + 8, y + 7, 10, sel ? BLACK : (i == 0 && off ? C_HI : C_TXT));
    });
    if (g_slotSave) drawTextCentered("La partie est aussi sauvegardée automatiquement après chaque match.", VW / 2, 50 + n * 24 + 10, 10, C_DIM, false);
    drawFooter("OK : valider   Retour");
    if (IN.back) { g_screen = g_slotSave ? SC_HUB : SC_MAIN; return; }
    if (s < 0) return;
#ifdef _WIN32
    mkdir("saves");
#else
    mkdir("saves", 0755);
#endif
    if (g_slotSave) {
        auto doSave = [s]() {
            if (g_career.save(slotPath(s).c_str())) {
                FILE* f = fopen(fmt("saves/partie%d.txt", s + 1).c_str(), "w");
                if (f) {
                    std::string d = g_career.kind == 0 ? g_world.teams[g_career.userTeam].name + fmt(" - saison %d-%02d", g_career.year, (g_career.year + 1) % 100)
                                                       : g_career.kind == CK_CUSTOM ? g_career.custom.name : fmt("%s %d", INTL_NAMES[g_career.intlType], g_career.year);
                    fprintf(f, "%s\n", d.c_str()); fclose(f);
                }
                toast("Partie sauvegardée");
                openHub();
            } else toast("Erreur de sauvegarde");
        };
        if (slotInfo(s) != "(vide)") askConfirm("Écraser la sauvegarde de l'emplacement " + fmt("%d", s + 1) + " ?", doSave, "Écraser", "Annuler");
        else doSave();
    } else {
        std::string path = (off && s == 0) ? std::string("saves/auto.sav") : slotPath(s - off);
        if (g_career.load(path.c_str())) {
            g_careerActive = true;
            g_needAdvance = true;
            g_hubNotice = "Partie chargée";
            levelCacheDirty();
            primeDraws();
            openHub();
        } else toast("Impossible de charger cet emplacement (sauvegarde absente ou d'une ancienne version)");
    }
}

// ------------------------------------------------------------------ compétitions internationales
static ListW g_intlLW;
static int g_hostChoice = 0;   // 0 = organisateurs officiels, sinon index+1 dans la liste des nations éligibles
static std::vector<int> intlHostCandidates(int type) {
    int conf = -1;
    switch (type) { case IT_EURO: case IT_EURO21: case IT_EURO19: case IT_EURO17: case IT_EURO_W: conf = UEFA; break; case IT_CAN: conf = CAF; break; case IT_ASIA: conf = AFC; break;
                    case IT_GOLD: conf = CONCACAF; break; case IT_OFC: conf = OFC; break; case IT_COPA: conf = CONMEBOL; break; default: break; }
    std::vector<int> v;
    for (int i = 0; i < NUM_NATIONS; i++) if (nationEligible(i) && (conf < 0 || NATIONS[i].conf == conf)) v.push_back(i);
    sortTeamsByName(v);
    return v;
}
static void screenIntl() {
    drawBackground("Compétitions internationales");
    static int type = 0;
    std::vector<std::string> items;
    auto hosts = intlHostCandidates(type);
    if (g_hostChoice > (int)hosts.size()) g_hostChoice = 0;
    std::string hostTxt;
    if (g_hostChoice == 0) { for (int h : defaultHosts(type)) hostTxt += (hostTxt.empty() ? "" : " / ") + g_world.teams[h].name; hostTxt += type == IT_EURO19 || type == IT_EURO17 ? " (désigné)" : " (officiel)"; }
    else hostTxt = g_world.teams[hosts[g_hostChoice - 1]].name;
    static int bigFmt = 0;
    bool canBig = type == IT_WORLDCUP || type == IT_EURO;
    if (!canBig) bigFmt = 0;
    std::string fmtTxt = type == IT_WORLDCUP ? (bigFmt ? "48 équipes, format USA 2026 (12 groupes, 16es)" : "32 équipes (8 groupes, 8es de finale)") :
                         type == IT_EURO ? (bigFmt ? "24 équipes (6 groupes + 4 meilleurs 3es, 8es)" : "16 équipes (4 groupes, quarts)") :
                         type == IT_CAN || type == IT_ASIA ? "24 équipes" : type == IT_OFC ? "8 équipes" :
                         type == IT_OLYMPICS ? "16 équipes U23 (+3 de plus de 23 ans), 4 groupes, quarts" :
                         type == IT_OLY_W ? "12 sélections féminines, 3 groupes (+2 meilleures 3es), quarts" :
                         type == IT_EURO_W ? "16 sélections féminines, 4 groupes, quarts" :
                         type == IT_WC_W ? "32 sélections féminines, 8 groupes, 8es de finale" :
                         type == IT_EURO21 ? "16 équipes Espoirs (U21), 4 groupes, quarts" :
                         type == IT_EURO19 || type == IT_EURO17 ? "8 équipes, 2 groupes, demi-finales" : "16 équipes";
    if (canBig) fmtTxt += "  < >";
    items.push_back(fmt("Compétition : %s  (%d)", INTL_NAMES[type], intlYear(type)));
    items.push_back("Format de la phase finale : " + fmtTxt);
    bool womenT = type == IT_OLY_W || type == IT_EURO_W || type == IT_WC_W;
    items.push_back(fmt("Qualifications : %s", g_intlQual && type != IT_COPA && !womenT ? "OUI (toutes les éliminatoires)" : "NON (phase finale directe)"));
    items.push_back("Pays organisateur : " + hostTxt);
    items.push_back(">>> Choisir mes sélections <<<");
    int s = menuRun(g_intlLW, items, 60, 600);
    drawFooter("Gauche/Droite : modifier   OK : valider   Retour");
    int d = IN.left ? -1 : IN.right ? 1 : 0;
    if (d) {
        switch (g_intlLW.cur) {
        case 0: type = (type + d + NUM_INTL) % NUM_INTL; g_hostChoice = 0; break;
        case 1: if (canBig) bigFmt ^= 1; break;
        case 2: g_intlQual = !g_intlQual; break;
        case 3: g_hostChoice = (g_hostChoice + d + (int)hosts.size() + 1) % ((int)hosts.size() + 1); break;
        }
    }
    if (IN.back) { g_screen = SC_MAIN; return; }
    if (s < 0) return;
    if (s == 0) { type = (type + 1) % NUM_INTL; g_hostChoice = 0; return; }
    if (s == 1) { if (canBig) bigFmt ^= 1; return; }
    if (s == 2) { g_intlQual = !g_intlQual; return; }
    if (s == 3) { g_hostChoice = (g_hostChoice + 1) % ((int)hosts.size() + 1); return; }
    if (s != 4) return;
    g_intlType = type;
    g_intlFormat = bigFmt;
    g_intlHosts = g_hostChoice == 0 ? std::vector<int>() : std::vector<int>{ hosts[g_hostChoice - 1] };
    g_intlSel.clear();
    g_intlCandidates.clear();
    int conf = -1;
    switch (type) { case IT_EURO: case IT_EURO21: case IT_EURO19: case IT_EURO17: case IT_EURO_W: conf = UEFA; break; case IT_CAN: conf = CAF; break; case IT_ASIA: conf = AFC; break; case IT_GOLD: conf = CONCACAF; break; case IT_OFC: conf = OFC; break; default: break; }
    for (int i = 0; i < NUM_NATIONS; i++) {
        if (!nationEligible(i)) continue;
        if (type == IT_COPA) { if (NATIONS[i].conf == CONMEBOL) g_intlCandidates.push_back(i); continue; }
        if (conf < 0 || NATIONS[i].conf == conf) g_intlCandidates.push_back(i);
    }
    if (type == IT_COPA) {
        std::vector<int> c; for (int i = 0; i < NUM_NATIONS; i++) if (NATIONS[i].conf == CONCACAF) c.push_back(i);
        std::stable_sort(c.begin(), c.end(), [](int a, int b) { return NATIONS[a].rating > NATIONS[b].rating; });
        for (int i = 0; i < 6; i++) g_intlCandidates.push_back(c[i]);
    }
    openPick(PM_INTL);
}

static void openCompEditor();
static int g_roleSlot=0,g_roleInstr=0,g_rolePlayerTab=0;
#include "app_anthems.inc"
#include "app_walkmusic.inc"
#include "app_screens.inc"
#include "app_manager.inc"
#include "app_club.inc"
#include "app_club2.inc"
#include "app_stage.inc"
#include "app_museum.inc"
#include "app_director.inc"
#include "app_supporters.inc"
#include "app_sporting.inc"
#include "app_draw.inc"
#include "app_editors.inc"
#include "app_compedit.inc"
#include "app_players.inc"
#include "app_coach.inc"
#include "app_life.inc"
#include "app_season.inc"
#include "app_ballondor.inc"
#include "app_season_features.inc"

// ------------------------------------------------------------------ test des sons : bruitages, chants, musiques, jingles, hymnes, vibrations
static void screenSoundTest() {
    drawBackground("Son et musique : tout écouter");
    static int cat = 0; static ListW lw[6]; static int playing = -1, playCat = -1;
    static const char* CATS[6] = { "BRUITAGES", "AMBIANCE", "MUSIQUES", "JINGLES", "HYMNES", "VIBRATIONS" };
    // onglets
    int tw = (VW - 24) / 6;
    for (int c = 0; c < 6; c++) {
        int x = 12 + c * tw;
        bool sel = c == cat;
        bool hov = IN.mouse.x >= x && IN.mouse.x < x + tw - 4 && IN.mouse.y >= 32 && IN.mouse.y < 50;
        DrawRectangleGradientV(x, 32, tw - 4, 18, sel ? Color{ 255, 220, 90, 255 } : hov ? Color{ 70, 104, 170, 255 } : Color{ 40, 64, 116, 255 }, sel ? C_SEL : Color{ 24, 40, 76, 255 });
        drawTextCentered(CATS[c], x + (tw - 4) / 2, 36, 10, sel ? BLACK : C_TXT, false);
        if (hov && IN.click) { cat = c; IN.click = false; audioPlay(SFX_UI_TICK); }
    }
    if (IN.left) cat = (cat + 5) % 6;
    if (IN.right) cat = (cat + 1) % 6;
    int n = cat == 5 ? NUM_RUMBLE : audioPreviewCount(cat);
    int rows = 13;
    bool nowPlaying = playCat >= 0 && playCat < 5 && audioPreviewPlaying();
    if (playCat >= 0 && playCat < 5 && !nowPlaying) { playing = -1; playCat = -1; }
    int act = listRun(lw[cat], n, 20, 58, VW - 230, rows, 18, [&](int i, int x, int y, bool sel) {
        std::string nm = cat == 5 ? std::string(rumbleName(i)) : audioPreviewName(cat, i);
        bool pl = playCat == cat && playing == i;
        drawTextPx(fmt("%2d", i + 1), x + 6, y + 4, 10, sel ? Color{ 60, 40, 10, 255 } : C_DIM);
        drawTextPx(fitText(nm, VW - 300, 10), x + 28, y + 4, 10, sel ? BLACK : pl ? C_HI : C_TXT);
        if (pl) {   // petit égaliseur animé
            float t = (float)GetTime();
            for (int k = 0; k < 4; k++) { int h = 3 + (int)(std::fabs(std::sin(t * (6 + k * 1.7f) + k)) * 9); DrawRectangle(x + VW - 262 + k * 4, y + 14 - h, 3, h, sel ? BLACK : C_GOOD); }
        }
    });
    if (act >= 0) {
        if (playCat == cat && playing == act && cat < 5) { audioPreviewStop(); playing = playCat = -1; }
        else if (cat == 5) { for (int p = 0; p < 4; p++) if (IsGamepadAvailable(p)) rumblePlay(p, act); playing = act; playCat = 5; }
        else { audioPreview(cat, act); playing = act; playCat = cat; }
    }
    // panneau de droite : ce qui est en lecture
    int px = VW - 200, py = 58, pw = 186, ph = rows * 18;
    DrawRectangleGradientV(px, py, pw, ph, Color{ 18, 26, 60, 230 }, Color{ 6, 10, 28, 235 });
    DrawRectangle(px, py, 3, ph, C_HI);
    drawTextPx("EN LECTURE", px + 10, py + 6, 10, C_HI);
    if (playCat >= 0 && playing >= 0) {
        std::string nm = playCat == 5 ? std::string(rumbleName(playing)) : audioPreviewName(playCat, playing);
        int yy = py + 22;
        for (auto& l : wrapText(nm, pw - 20)) { if (yy > py + 70) break; drawTextPx(l, px + 10, yy, 10, WHITE); yy += 12; }
        if (playCat == 4) { std::string cr = audioAnthemListCredit(playing); int y2 = py + 80; for (auto& l : wrapText(cr, pw - 20)) { if (y2 > py + 140) break; drawTextPx(l, px + 10, y2, 10, C_DIM); y2 += 11; } }
        float t = (float)GetTime();
        if (playCat == 5) {   // niveaux des deux moteurs
            for (int m = 0; m < 2; m++) {
                float lv = 0; for (int p = 0; p < 4; p++) lv = std::max(lv, rumbleLevel(p, m));
                drawTextPx(m ? "Moteur droit" : "Moteur gauche", px + 10, py + 90 + m * 26, 10, C_DIM);
                DrawRectangle(px + 10, py + 102 + m * 26, pw - 20, 8, Color{ 0, 0, 0, 140 });
                DrawRectangle(px + 10, py + 102 + m * 26, (int)((pw - 20) * lv), 8, m ? C_HI : C_GOOD);
            }
            if (!anyPad()) drawTextPx("Aucune manette branchée", px + 10, py + 160, 10, C_BAD);
            if (!g_settings.vibration) drawTextPx("Vibrations coupées (Options)", px + 10, py + 174, 10, C_BAD);
        } else {
            for (int k = 0; k < 20; k++) { int h = nowPlaying ? 4 + (int)(std::fabs(std::sin(t * (3 + k * 0.6f) + k * 1.3f)) * 34) : 2; DrawRectangle(px + 12 + k * 8, py + ph - 14 - h, 6, h, k % 3 == 0 ? C_HI : C_GOOD); }
        }
    } else drawTextPx("OK : écouter", px + 10, py + 24, 10, C_DIM);
    std::string info = fmt("%d éléments dans cette rubrique", n);
    drawTextPx(info, 20, 58 + rows * 18 + 6, 10, C_DIM);
    drawFooter("Gauche/Droite : rubrique   Haut/Bas : choisir   OK : écouter / arrêter   Retour : options");
    if (IN.back) { audioPreviewStop(); playing = playCat = -1; g_screen = SC_OPTIONS; }
}

// ------------------------------------------------------------------ options / aide
static void screenOptions() {
    drawBackground("Options");
    std::vector<std::string> items = {
        fmt("Durée d'une mi-temps : %d min", HALF_MINUTES[g_settings.halfIdx]),
        fmt("Difficulté : %s", DIFF_NAMES[g_settings.difficulty]),
        fmt("Terrain : %s", PITCH_NAMES[g_settings.pitch]),
        fmt("Radar : %s", g_settings.radar ? "oui" : "non"),
        fmt("Plein écran : %s  (F11)", g_settings.fullscreen ? "oui" : "non"),
        fmt("Son : %s", g_settings.sound ? "oui" : "non"),
        "Configurer le clavier et les manettes",
        std::string("Musique des menus : ") + (!g_settings.music ? "non" : g_settings.musicTrack == 0 ? "tous les thèmes" : audioTrackName(g_settings.musicTrack - 1)),
        fmt("Commentaires : %s", g_settings.commentary ? "oui" : "non"),
        std::string("Vibrations des manettes : ") + (!g_settings.vibration ? "non" : g_settings.vibForce <= 1 ? "faibles" : g_settings.vibForce == 2 ? "moyennes" : "fortes"),
        fmt("Scènes de vestiaire : %s", g_settings.lockerRoom ? "oui" : "non"),
        "Aide des commandes  (F12 = capture d'écran)",
        "Retour" };
    static const char* REFKN[5] = { "noire", "jaune", "verte", "rouge", "bleue" };
    items.push_back(std::string("Tenue des arbitres : ") + REFKN[std::max(0, std::min(4, g_settings.refKit))]);   // 13
    items.push_back("Tester les sons, musiques, hymnes et vibrations");                                          // 14
    // ---- présentation en cartes par rubrique (MATCH, AFFICHAGE, SON, COMMANDES)
    struct Sec { const char* name; std::vector<int> ids; int icon; };
    static const Sec SECS[4] = { { "MATCH", { 0, 1, 2, 3, 8, 10, 13 }, 13 }, { "AFFICHAGE", { 4 }, 6 }, { "SON & MUSIQUE", { 5, 7, 9, 14 }, 9 }, { "COMMANDES", { 6, 11 }, 3 } };
    static const int ICO[15] = { 0, 2, 1, 6, 6, 9, 3, 9, 10, 3, 11, 12, 13, 5, 9 };
    std::vector<int> order; for (auto& sc : SECS) for (int id : sc.ids) order.push_back(id); order.push_back(12);
    static int vpos = 0;
    vpos = std::max(0, std::min((int)order.size() - 1, vpos));
    if (IN.up) { vpos = (vpos + (int)order.size() - 1) % (int)order.size(); audioPlay(SFX_UI_MOVE); }
    if (IN.down) { vpos = (vpos + 1) % (int)order.size(); audioPlay(SFX_UI_MOVE); }
    int s = -1;
    float tt = (float)GetTime();
    int colX[2] = { 14, VW / 2 + 4 }, colW = VW / 2 - 18, colY[2] = { 32, 32 };
    int k = 0;
    for (int si = 0; si < 4; si++) {
        int col = si == 0 ? 0 : 1;
        int x = colX[col], y = colY[col];
        const Sec& sc = SECS[si];
        DrawRectangle(x, y, colW, 15, Color{ 30, 50, 100, 255 }); DrawRectangle(x, y + 14, colW, 1, C_HI);
        drawOptIcon(sc.icon, x + 4, y + 1, C_HI);
        drawTextPx(sc.name, x + 22, y + 2, 10, C_HI);
        y += 18;
        for (int id : sc.ids) {
            bool sel = order[vpos] == id;
            std::string txt = items[id];
            size_t colon = txt.find(" : ");
            std::string lab = colon == std::string::npos ? txt : txt.substr(0, colon), val = colon == std::string::npos ? std::string() : txt.substr(colon + 3);
            int h = 17;
            DrawRectangle(x + 2, y + 2, colW, h - 2, Color{ 0, 0, 0, 60 });
            DrawRectangle(x, y, colW, h - 2, sel ? C_SEL : Color{ 24, 40, 76, 255 });
            DrawRectangle(x, y, 3, h - 2, sel ? WHITE : Color{ 70, 110, 180, 255 });
            drawOptIcon(ICO[id], x + 6, y + 1, sel ? Color{ 40, 30, 10, 255 } : C_HI);
            drawTextPx(fitText(lab, val.empty() ? colW - 30 : colW / 2 - 6, 10), x + 22, y + 3, 10, sel ? BLACK : C_TXT);
            if (!val.empty()) {
                bool on = val == "oui", off = val == "non";
                std::string v = fitText(val, colW / 2 - 30, 10);
                int vw = textWidth(v, 10) + 10, vx = x + colW - vw - 14;
                DrawRectangle(vx, y + 2, vw, 11, on ? Color{ 40, 140, 60, 255 } : off ? Color{ 110, 40, 40, 255 } : Color{ 14, 22, 44, 255 });
                drawTextPx(v, vx + 5, y + 3, 10, WHITE);
                if (sel && ((int)(tt * 3)) % 2) { drawTextPx("<", vx - 8, y + 3, 10, BLACK); drawTextPx(">", x + colW - 10, y + 3, 10, BLACK); }
            }
            if (IN.click && IN.mouse.x >= x && IN.mouse.x < x + colW && IN.mouse.y >= y && IN.mouse.y < y + h - 2) { if (order[vpos] == id) s = id; for (int q = 0; q < (int)order.size(); q++) if (order[q] == id) vpos = q; }
            y += h; k++;
        }
        colY[col] = y + 8;
    }
    {   // bouton retour
        bool sel = order[vpos] == 12;
        int bw = 160, bx = VW / 2 - bw / 2, by = VH - 40;
        DrawRectangle(bx, by, bw, 16, sel ? C_SEL : Color{ 120, 30, 40, 255 });
        drawTextCentered("Retour", VW / 2, by + 3, 10, sel ? BLACK : WHITE, false);
        if (IN.click && IN.mouse.x >= bx && IN.mouse.x < bx + bw && IN.mouse.y >= by && IN.mouse.y < by + 16) s = 12;
    }
    if (IN.ok) s = order[vpos];
    g_optLW.cur = order[vpos];
    if (s == 13 || (g_optLW.cur == 13 && (IN.left || IN.right))) { g_settings.refKit = (g_settings.refKit + (IN.left ? 4 : 1)) % 5; g_settings.save(); }
    if (s == 6) { g_ctlLW = ListW(); g_ctlCapture = -1; g_ctlBack = SC_OPTIONS; g_screen = SC_CONTROLS; return; }
    if (s == 14) { g_screen = SC_SOUNDTEST; return; }
    if (s == 7 || (g_optLW.cur == 7 && (IN.left || IN.right))) {
        // non -> tous les thèmes -> thème 1 ... thème N -> non
        int nt = audioTrackCount();
        int st = !g_settings.music ? 0 : 1 + g_settings.musicTrack;
        st = (st + (IN.left ? nt + 1 : 1)) % (nt + 2);
        g_settings.music = st != 0;
        if (st > 0) g_settings.musicTrack = st - 1;
        audioMusicEnabled(g_settings.music);
        audioSetTrackMode(g_settings.musicTrack);
        g_settings.save();
    }
    if (s == 8 || (g_optLW.cur == 8 && (IN.left || IN.right))) { g_settings.commentary = !g_settings.commentary; g_settings.save(); }
    if (s == 9 || (g_optLW.cur == 9 && (IN.left || IN.right))) {
        // non -> faibles -> moyennes -> fortes -> non
        if (!g_settings.vibration) { g_settings.vibration = true; g_settings.vibForce = 1; }
        else if (g_settings.vibForce < 3) g_settings.vibForce++;
        else g_settings.vibration = false;
        g_settings.save(); applyVibration();
        if (g_settings.vibration) for (int p = 0; p < 4; p++) if (IsGamepadAvailable(p)) rumblePlay(p, RB_GOAL_FOR);   // essai
    }
    if (s == 10 || (g_optLW.cur == 10 && (IN.left || IN.right))) { g_settings.lockerRoom = !g_settings.lockerRoom; g_settings.save(); }
    int d = IN.left ? -1 : IN.right ? 1 : 0;
    int c = g_optLW.cur;
    if (s >= 0 && s < 6) d = 1;
    if (d) {
        switch (c) {
        case 0: g_settings.halfIdx = (g_settings.halfIdx + d + NUM_HALF) % NUM_HALF; break;
        case 1: g_settings.difficulty = (g_settings.difficulty + d + 3) % 3; break;
        case 2: g_settings.pitch = (g_settings.pitch + d + 6) % 6; break;
        case 3: g_settings.radar = !g_settings.radar; break;
        case 4: g_settings.fullscreen = !g_settings.fullscreen; ToggleBorderlessWindowed(); break;
        case 5: g_settings.sound = !g_settings.sound; audioSetEnabled(g_settings.sound); break;
        }
        g_settings.save();
    }
    drawFooter("Gauche/Droite : modifier   Retour");
    if (s == 11) { g_screen = SC_HELP; return; }
    if (IN.back || s == 12) { g_settings.save(); g_screen = g_optBack; }
}

static void screenHelp() {
    drawBackground("Commandes");
    // cartes par thème : titre coloré, puis lignes « action : touche »
    struct Card { const char* title; Color c; std::vector<std::string> l; };
    std::vector<Card> cards = {
        { "AVEC LE BALLON (style Super Nintendo)", { 60, 190, 90, 255 }, {
            "Passe : X (clavier 1) / F (clavier 2) / A (manette)", "Tir : C / G / B   appui long = puissance", "Lob / centre : V / H / X", "Passe en profondeur : B / J / Y", "Sprint : N / T / RB",
            "Après une passe, vous contrôlez le receveur" } },
        { "SANS LE BALLON", { 230, 120, 60, 255 }, {
            "Passe : changer de joueur", "  stick orienté = partenaire dans cette direction", "  au contact du porteur = tacle debout", "Tir : tacle glissé (par derrière = faute)", "Lob maintenu : presser le porteur" } },
        { "STYLE CLASSIQUE (2 boutons)", { 80, 150, 240, 255 }, {
            "Bouton 1 : passe (court) ou tir (long)", "Bouton 2 : lob / tacle glissé", "Sans ballon, loin du porteur :", "  bouton 1 = changer de joueur" } },
        { "EFFETS ET COUPS DE PIED ARRÊTÉS", { 200, 120, 230, 255 }, {
            "Après une frappe, orientez : côté = brossé,", "  arrière = levé, avant = tendu", "Coup franc : effet L / R, flèche courbée", "Penalty : direction + appui long", "  gardien : gauche / droite / centre" } },
        { "GARDIEN ET BAGARRE", { 240, 200, 60, 255 }, {
            "Gardien : plonge seul ; avec le ballon,", "  bouton 1 = relance, bouton 2 = dégagement", "Bagarre : tir ou lob près du fautif", "  ... gare au carton rouge !" } },
        { "PENDANT LE MATCH", { 200, 60, 70, 255 }, {
            "Échap / P / Start : pause", "F1 : rappel des commandes", "F9 : musique   F11 : plein écran", "F12 : capture d'écran", "Les manettes vibrent (Options : intensité)" } },
    };
    int cw = (VW - 30) / 2, chh = 86;
    float t = (float)GetTime();
    for (int i = 0; i < (int)cards.size(); i++) {
        int col = i % 2, row = i / 2;
        int x = 12 + col * (cw + 6), y = 32 + row * (chh + 4);
        float in = std::min(1.f, std::max(0.f, (t * 4.f) - i * 0.15f));   // simple fondu : pas de dépendance à l'entrée sur l'écran
        (void)in;
        const Card& C = cards[i];
        DrawRectangle(x + 2, y + 2, cw, chh, Color{ 0, 0, 0, 80 });
        DrawRectangleGradientV(x, y, cw, chh, Color{ 22, 34, 72, 240 }, Color{ 8, 14, 34, 240 });
        DrawRectangle(x, y, cw, 14, Color{ C.c.r, C.c.g, C.c.b, 230 });
        drawTextPx(fitText(C.title, cw - 8, 10), x + 4, y + 2, 10, BLACK);
        int yy = y + 18;
        for (auto& l : C.l) { drawTextPx(fitText(l, cw - 10, 10), x + 5, yy, 10, l.size() > 1 && l[0] == ' ' ? C_DIM : C_TXT); yy += 11; }
    }
    int ry = 32 + 3 * (chh + 4) + 2;
    DrawRectangle(12, ry, VW - 24, 26, Color{ 6, 12, 28, 200 }); DrawRectangle(12, ry, 3, 26, C_HI);
    drawTextPx("RÈGLES : hors-jeu, touches, corners, 6 mètres, fautes, cartons (2e jaune = rouge), penalties, prolongations,", 20, ry + 2, 10, C_TXT);
    drawTextPx("tirs au but, matchs aller-retour, passe en retrait au gardien interdite, 5 remplacements.", 20, ry + 14, 10, C_TXT);
    drawFooter("Les touches se changent dans Options > Configurer le clavier et les manettes   Retour");
    if (IN.back || IN.ok || IN.click) g_screen = SC_OPTIONS;
}

// ------------------------------------------------------------------ menu principal
static ListW g_mainLW;
static float g_titleT = 0;
static int g_titleVar = -1;          // écran titre tiré au hasard à chaque lancement
static const int NUM_TITLE_VARS = 4;
static uint32_t titleHash(uint32_t h) { h ^= h >> 13; h *= 0x5bd1e995; h ^= h >> 15; return h; }

// écrans de démarrage : Offgame puis mention légale
static float g_splashT = 0; static int g_splashPhase = 0;
static void screenSplash(float dt) {
    g_splashT += dt;
    const float DUR[2] = { 2.6f, 6.0f };
    bool skip = IN.ok || IN.back || IN.click || IN.start || (IN.anyKey && g_splashT > 0.4f);
    if (skip || g_splashT >= DUR[g_splashPhase]) {
        g_splashPhase++; g_splashT = 0;
        if (g_splashPhase >= 2) { g_titleVar = -1; g_titleT = 0; g_screen = SC_MAIN; return; }
    }
    float t = g_splashT, d = DUR[g_splashPhase];
    float a = std::min(1.f, std::min(t / 0.5f, (d - t) / 0.5f)); a = std::max(0.f, a);
    unsigned char al = (unsigned char)(255 * a);
    ClearBackground(BLACK);
    if (g_splashPhase == 0) {
        // logo Offgame : manette stylisée et nom
        int cx = VW / 2, cy = VH / 2 - 26;
        DrawRectangleRounded(Rectangle{ (float)cx - 44, (float)cy - 20, 88, 40 }, 0.6f, 8, Color{ 230, 60, 60, al });
        DrawRectangle(cx - 30, cy - 3, 18, 6, Color{ 255, 255, 255, al }); DrawRectangle(cx - 24, cy - 9, 6, 18, Color{ 255, 255, 255, al });
        DrawCircle(cx + 20, cy - 5, 5, Color{ 255, 220, 60, al }); DrawCircle(cx + 30, cy + 5, 5, Color{ 60, 140, 255, al });
        const char* O = "OFFGAME";
        int w = textWidth(O, 40);
        drawTextPx(O, cx - w / 2 + 2, cy + 34, 40, Color{ 80, 20, 20, al });
        drawTextPx(O, cx - w / 2, cy + 32, 40, Color{ 255, 255, 255, al });
        drawTextCentered("présente", cx, cy + 82, 10, Color{ 180, 190, 220, al }, false);
    } else {
        const char* L[] = { "Jeu gratuit non licencié par la FIFA, l'UEFA, la FFF",
                            "ou toute autre instance officielle.", "",
                            "Sigames, EA Sports, Konami et Capcom produisent",
                            "de très bons jeux de football (parmi d'autres)." };
        int y = VH / 2 - 44;
        drawTextCentered("AVERTISSEMENT", VW / 2, y - 24, 10, Color{ 255, 225, 90, al }, false);
        for (auto l : L) { drawTextCentered(l, VW / 2, y, 10, Color{ 235, 240, 255, al }, false); y += 16; }
    }
    drawTextCentered("OK / clic : passer", VW / 2, VH - 16, 10, Color{ 90, 100, 130, al }, false);
}

// décor de l'écran titre : 0 nuit, 1 plein jour, 2 coucher de soleil, 3 nuit enneigée
static void drawTitleScene(int var) {
    struct Sky { Color top, bot; };
    static const Sky SK[NUM_TITLE_VARS] = { { { 10, 16, 36, 255 }, { 40, 46, 96, 255 } }, { { 70, 140, 230, 255 }, { 170, 210, 250, 255 } },
                                            { { 60, 30, 90, 255 }, { 250, 140, 60, 255 } }, { { 16, 20, 34, 255 }, { 60, 66, 90, 255 } } };
    ClearBackground(SK[var].top);
    DrawRectangleGradientV(0, 0, VW, 120, SK[var].top, SK[var].bot);
    if (var == 1) {          // soleil et nuages
        DrawCircle(VW - 110, 40, 18, Color{ 255, 240, 160, 255 }); DrawCircle(VW - 110, 40, 26, Color{ 255, 240, 160, 60 });
        for (int k = 0; k < 5; k++) {
            float cx = std::fmod(k * 150.f + g_titleT * (8 + k * 2), (float)VW + 120) - 60; int cy = 20 + (k * 37) % 50;
            DrawEllipse((int)cx, cy, 26, 7, Color{ 255, 255, 255, 220 }); DrawEllipse((int)cx + 14, cy - 4, 16, 7, Color{ 255, 255, 255, 220 });
        }
    } else if (var == 2) {   // soleil couchant
        DrawCircle(VW / 2 + 150, 104, 34, Color{ 255, 200, 90, 255 });
        for (int i = 0; i < 4; i++) DrawRectangle(0, 70 + i * 9, VW, 2, Color{ 255, 190, 120, 60 });
    } else {                 // étoiles
        for (int k = 0; k < 40; k++) { uint32_t h = titleHash(k * 7919u + 13); if ((h >> 20) % 5 || std::sin(g_titleT * 2 + k) > -0.6f) DrawPixel((int)(h % VW), (int)((h >> 10) % 86), Color{ 255, 255, 230, 200 }); }
        if (var == 0) { DrawCircle(90, 30, 10, Color{ 240, 240, 220, 255 }); DrawCircle(94, 27, 9, SK[0].top); }
    }
    // tribunes : gradins et public
    Color stand = var == 1 ? Color{ 90, 96, 116, 255 } : var == 2 ? Color{ 60, 40, 60, 255 } : Color{ 28, 30, 46, 255 };
    DrawRectangle(0, 92, VW, 34, stand);
    for (int row = 0; row < 8; row++) {
        int yy = 94 + row * 4;
        DrawRectangle(0, yy + 3, VW, 1, Color{ (unsigned char)(stand.r * 3 / 4), (unsigned char)(stand.g * 3 / 4), (unsigned char)(stand.b * 3 / 4), 255 });
        for (int x = (row % 2) * 2; x < VW; x += 4) {
            uint32_t h = titleHash((uint32_t)(x * 2654435761u) ^ (uint32_t)(row * 40503u));
            static const Color CR[] = { { 180, 40, 50, 255 }, { 220, 220, 230, 255 }, { 40, 70, 160, 255 }, { 200, 170, 60, 255 }, { 90, 90, 110, 255 }, { 60, 40, 30, 255 } };
            int bob = std::sin(g_titleT * 5 + (h % 17)) > 0.8f ? 1 : 0;
            DrawRectangle(x, yy - bob, 2, 2, CR[h % 6]);
        }
    }
    if (var != 1) for (int k = 0; k < 3; k++) {   // flashs d'appareils photo
        uint32_t h = titleHash((uint32_t)((int)(g_titleT * 7) * 7919 + k * 104729));
        if (h % 3 == 0) DrawRectangle((int)(h % VW), 94 + (int)((h >> 8) % 30), 2, 2, WHITE);
    }
    // projecteurs (allumés la nuit)
    for (int k = 0; k < 2; k++) {
        int px = k ? VW - 40 : 28;
        DrawRectangle(px + 5, 40, 2, 56, Color{ 70, 74, 90, 255 });
        DrawRectangle(px - 4, 30, 20, 12, Color{ 50, 54, 70, 255 });
        bool lit = var != 1;
        for (int i = 0; i < 4; i++) for (int j = 0; j < 2; j++) DrawRectangle(px - 2 + i * 5, 32 + j * 5, 4, 4, lit ? Color{ 255, 250, 210, 255 } : Color{ 150, 150, 140, 255 });
        if (lit) {
            float pulse = 0.85f + 0.15f * std::sin(g_titleT * 3 + k);
            DrawCircle(px + 6, 36, 20, Color{ 255, 250, 200, (unsigned char)(40 * pulse) });
            DrawTriangle(Vector2{ (float)px + 6, 40 }, Vector2{ (float)(k ? px - 160 : px - 40), 360 }, Vector2{ (float)(k ? px + 40 : px + 160), 360 }, Color{ 255, 250, 210, 14 });
        }
    }
    // pelouse rayée (enneigée sur les bords en variante 3)
    Color g1 = var == 2 ? Color{ 60, 110, 40, 255 } : Color{ 40, 116, 40, 255 }, g2 = var == 2 ? Color{ 52, 96, 34, 255 } : Color{ 34, 102, 34, 255 };
    for (int i = 0; i < 12; i++) { DrawRectangle(0, 126 + i * 20, VW, 10, g1); DrawRectangle(0, 136 + i * 20, VW, 10, g2); }
    DrawRectangle(0, 126, VW, 2, Color{ 240, 240, 240, 200 });
    DrawLine(0, 240, VW, 240, Color{ 240, 240, 240, 180 });
    DrawCircleLines(VW / 2, 240, 40, Color{ 240, 240, 240, 180 });
    if (var == 3) {
        for (int k = 0; k < 90; k++) {
            uint32_t h = titleHash(k * 104729u + 7);
            float fx = std::fmod((float)(h % VW) + std::sin(g_titleT + k) * 10.f, (float)VW);
            float fy = std::fmod((float)((h >> 9) % VH) + g_titleT * (20 + (h % 30)), (float)VH);
            DrawRectangle((int)fx, (int)fy, 1 + (int)(h % 2), 1 + (int)(h % 2), Color{ 255, 255, 255, 210 });
        }
    }
    // panneaux publicitaires
    static const char* ADS[] = { "SUPER SOCCER WORLD", "OFFGAME", "PIXEL COLA", "AMIGOAL", "STADE TV", "GOAL FM" };
    static const Color ADC[] = { { 214, 40, 40, 255 }, { 247, 127, 0, 255 }, { 42, 157, 143, 255 }, { 106, 76, 147, 255 }, { 29, 53, 87, 255 }, { 38, 70, 83, 255 } };
    int off = (int)(g_titleT * 20) % 110;
    for (int i = -1; i < VW / 110 + 2; i++) {
        int bx = i * 110 - off;
        DrawRectangle(bx, 116, 108, 10, ADC[(i + 12) % 6]);
        drawTextPx(fitText(ADS[(i + 12) % 6], 100, 10), bx + 4, 116, 10, WHITE);
    }
    // titre
    static const Color TC[NUM_TITLE_VARS][2] = { { { 255, 220, 60, 255 }, { 220, 40, 40, 255 } }, { { 255, 255, 255, 255 }, { 20, 60, 160, 255 } },
                                                 { { 255, 230, 120, 255 }, { 150, 30, 60, 255 } }, { { 200, 240, 255, 255 }, { 40, 90, 170, 255 } } };
    {
        const char* F = "SUPER SOCCER";
        int fw = textWidth(F, 20);
        int fx = VW / 2 - fw / 2;
        drawTextPx(F, fx + 2, 6, 20, Color{ 0, 0, 0, 200 });
        drawTextPx(F, fx, 4, 20, WHITE);
        DrawRectangle(fx, 26, fw / 3, 3, Color{ 0, 85, 164, 255 });
        DrawRectangle(fx + fw / 3, 26, fw / 3, 3, WHITE);
        DrawRectangle(fx + 2 * fw / 3, 26, fw - 2 * fw / 3, 3, Color{ 226, 0, 26, 255 });
    }
    const char* T = "WORLD";
    int w = textWidth(T, 60);
    for (int k = 0; k < 3; k++) drawTextPx(T, VW / 2 - w / 2 + 3 - k, 24 + 3 - k, 60, k == 2 ? TC[var][0] : (k == 1 ? TC[var][1] : Color{ 0, 0, 0, 200 }));
    drawTextCentered("Jouez. Gérez. Vivez le football", VW / 2, 84, 10, var == 1 ? WHITE : C_DIM);
}

static void screenMain(float dt) {
    g_titleT += dt;
    if (g_titleVar < 0) g_titleVar = getenv("FOOT_TITLE") ? atoi(getenv("FOOT_TITLE")) % NUM_TITLE_VARS : GetRandomValue(0, NUM_TITLE_VARS - 1);
    drawTitleScene(g_titleVar);
    // joueurs qui courent
    Kit k1; k1.shirt = 0x1B2A63; k1.shorts = 0xFFFFFF; k1.socks = 0xE2001A;
    Kit k2; k2.shirt = 0xFFFFFF; k2.shirt2 = 0xE2001A; k2.shorts = 0xE2001A; k2.pattern = KP_VSTRIPES; k2.socks = 0xFFFFFF;
    int fr = (int)(g_titleT * 8);
    float px = std::fmod(g_titleT * 60, (float)VW + 80) - 40;
    drawPlayerSprite((int)px, 138, k1, 1, 0, 2, fr, PS_NORMAL, false, 0, 2);
    drawPlayerSprite((int)px - 40, 138, k2, 3, 0, 2, fr + 1, PS_NORMAL, false, 0, 2);
    DrawEllipse((int)px + 12, 138, 4, 1.5f, Color{ 0, 0, 0, 80 });
    DrawCircle((int)px + 12, 135, 3, WHITE);
    // menu principal à sous-menus : chaque entrée = (libellé, description, action, famille) ; action < 0 : ouvre le sous-menu -action
    struct MI { const char* label; const char* desc; int act; int grp; };
    static int sub = 0;
    static const MI ROOT[] = {
        { "Match amical", "Un match entre deux équipes au choix : clubs ou sélections (masculins ou féminins), stade, météo, durée, prolongation, tirs au but.", 1, 0 },
        { "Championnat  >", "Mode foot : une saison de championnat avec plusieurs clubs contrôlés, sans gestion, et les Coupes d'Europe.", -1, 0 },
        { "International  >", "Coupe du monde, Euro, CAN, Copa América, compétitions de jeunes et féminines, carrière de sélectionneur.", -2, 0 },
        { "Carrière  >", "Carrière de club (jeu + manager, manager seul ou jeu seul), carrière de joueur / joueuse, carrière de sélectionneur.", -3, 1 },
        { "Compétition personnalisée", "Créez votre tournoi : championnat, coupe ou groupes et phase finale, avec les équipes de votre choix.", 20, 0 },
        { "Entraînement", "Tirs au but, coups francs, penalties et matchs d'entraînement pour prendre en main les commandes.", 21, 0 },
        { "Éditeur  >", "Modifier ou créer des clubs, joueurs, sponsors et managers ; fiches complètes des clubs et sélections.", -4, 2 },
        { "Options", "Durée des matchs, difficulté, terrain, son, musique, commentaires, clavier et manettes.", 30, 3 },
        { "Charger une partie", "Reprendre une partie sauvegardée (emplacements et sauvegarde automatique).", 31, 2 },
        { "À propos", "Crédits et informations sur le jeu.", 32, 3 },
        { "Soutenir le jeu (Tipeee)", "Soutenir le développement du jeu sur Tipeee : fr.tipeee.com/le-bazar-de-monos (ouvre le navigateur).", 33, 3 },
        { "Quitter", "Quitter Super Soccer World.", 34, 3 } };
    static const MI SUB1[] = {
        { "Championnat (mode foot)", "Choisissez un championnat (masculin ou féminin, toutes fédérations) : saison complète, 1 à 4 clubs contrôlés, sans gestion.", 2, 0 },
        { "Coupes d'Europe (C1 et C3)", "Ligue des champions et Coupe UEFA (formule 2003) ou C1, Ligue Europa et Ligue Conférence (nouvelle formule).", 3, 0 },
        { "< Retour", "Revenir au menu principal.", 0, 3 } };
    static const MI SUB2[] = {
        { "Compétitions internationales", "Coupe du monde, Euro, CAN, Copa América, Gold Cup... ; Espoirs, U19, U17, tournois olympiques ; Euro et Coupe du monde féminins.", 4, 0 },
        { "Carrière de sélectionneur", "Dirigez une sélection (A, Espoirs, U19 ou U17) : convocations, Ligue des nations, qualifications, phases finales.", 5, 1 },
        { "Légendes de la Coupe du monde", "Mode hors-série : rejouez les phases finales de 1930 à 2026 avec les joueurs, la formule, les stades et les règles de l'époque.", 12, 0 },
        { "Coupe des Continents", "Chaque confédération (UEFA, CONMEBOL, CONCACAF, CAF, AFC, OFC) envoie ses meilleurs joueurs : un groupe de 6 en matchs aller-retour, puis Final Four pour les 4 premiers.", 13, 0 },
        { "< Retour", "Revenir au menu principal.", 0, 3 } };
    static const MI SUB3[] = {
        { "Carrière club : JEU + MANAGER", "Vous gérez le club (effectif, mercato, finances, stade) et vous jouez les matchs. Championnats masculins et féminins du monde entier.", 6, 1 },
        { "Carrière club : MANAGER", "Vous gérez le club, les matchs se déroulent sans vous (match complet, durée au choix). Idéal pour enchaîner les saisons.", 7, 1 },
        { "Carrière club : JEU", "Vous jouez les matchs de votre club saison après saison, sans la gestion.", 8, 1 },
        { "Carrière Directeur sportif", "Construisez le club : effectif, mercato, contrats, entraîneur IA, président, emplois et projet sportif.", 35, 1 },
        { "Carrière de joueur / joueuse", "Créez votre joueur ou joueuse, choisissez un club et vivez sa carrière : temps de jeu, salaire, vie privée, paris...", 9, 1 },
        { "Carrière de sélectionneur", "Dirigez une sélection (A, Espoirs, U19 ou U17) : convocations, Ligue des nations, qualifications, phases finales.", 5, 1 },
        { "< Retour", "Revenir au menu principal.", 0, 3 } };
    static const MI SUB4[] = {
        { "Éditeurs (clubs, joueurs, sponsors, managers)", "Modifier ou créer des clubs, des joueurs et joueuses, des sponsors et des managers.", 10, 2 },
        { "Fiches des clubs et sélections", "Fiches complètes des clubs et sélections : effectifs, stades, palmarès, archives.", 11, 2 },
        { "< Retour", "Revenir au menu principal.", 0, 3 } };
    const MI* M = sub == 1 ? SUB1 : sub == 2 ? SUB2 : sub == 3 ? SUB3 : sub == 4 ? SUB4 : ROOT;
    int NM = sub == 1 ? 3 : sub == 2 ? 5 : sub == 3 ? 7 : sub == 4 ? 3 : 12;
    static const Color GC[4] = { { 60, 190, 90, 255 }, { 240, 190, 50, 255 }, { 80, 150, 240, 255 }, { 150, 160, 180, 255 } };
    static const char* GN[4] = { "JOUER", "CARRIÈRES", "DONNÉES", "SYSTÈME" };
    static const char* SUBN[5] = { "MENU PRINCIPAL", "CHAMPIONNAT", "INTERNATIONAL", "CARRIÈRE", "ÉDITEUR" };
    int mx = 34, mw = 300, my = 136, rh = 16;
    int boxH = 12 * rh + 12;
    DrawRectangle(mx - 6, my - 18, mw + 12, boxH + 12, Color{ 6, 12, 28, 190 });
    DrawRectangleLines(mx - 6, my - 18, mw + 12, boxH + 12, Color{ 240, 200, 60, 180 });
    drawTextPx(SUBN[sub], mx, my - 14, 10, C_HI);
    int s = listRun(g_mainLW, NM, mx, my, mw, NM, rh, [&](int i, int x, int y, bool sel) {
        Color gc = GC[M[i].grp];
        DrawRectangle(x + 4, y + 4, 6, 7, sel ? Color{ 40, 30, 10, 255 } : gc);
        drawTextPx(M[i].label, x + 16, y + 3, 10, sel ? BLACK : C_TXT);
    });
    {   // panneau d'information sur l'entrée sélectionnée
        int cur = std::max(0, std::min(NM - 1, g_mainLW.cur));
        int px = mx + mw + 22, pw = VW - px - 20, py = my - 18, ph = boxH + 12;
        Color gc = GC[M[cur].grp];
        DrawRectangle(px, py, pw, ph, Color{ 6, 12, 28, 200 });
        DrawRectangle(px, py, pw, 16, Color{ gc.r, gc.g, gc.b, 230 });
        drawTextPx(GN[M[cur].grp], px + 6, py + 3, 10, BLACK);
        DrawRectangleLines(px, py, pw, ph, gc);
        drawTextPx(fitText(M[cur].label, pw - 12, 10), px + 8, py + 24, 10, C_HI);
        DrawRectangle(px + 8, py + 37, pw - 16, 1, Color{ gc.r, gc.g, gc.b, 140 });
        int yy = py + 44;
        for (auto& l : wrapText(M[cur].desc, pw - 18)) { drawTextPx(l, px + 8, yy, 10, C_TXT); yy += 12; }
        float b = std::fabs(std::sin(g_titleT * 3.f));
        int bx = px + pw / 2, by = py + ph - 18 - (int)(b * 22);
        DrawEllipse(bx, py + ph - 10, 7 - b * 3, 2, Color{ 0, 0, 0, 90 });
        DrawCircle(bx, by, 7, WHITE); DrawCircleLines(bx, by, 7, Color{ 40, 40, 40, 255 });
        DrawPoly(Vector2{ (float)bx, (float)by }, 5, 2.6f, g_titleT * 90, Color{ 30, 30, 30, 255 });
    }
    drawFooter(sub ? "Flèches / souris / manette   OK : valider   Retour : menu principal   F9 : musique" : "Flèches / souris / manette   OK : valider   F9 : musique");
    { std::string v = std::string("v") + GAME_VERSION + fmt(" build %d", GAME_BUILD); drawTextPx(v, VW - 8 - textWidth(v, 10), VH - 14, 10, Color{ 200, 210, 240, 200 }); }
    if (sub && IN.back) { int from = sub; sub = 0; g_mainLW = ListW(); for (int i = 0; i < 12; i++) if (ROOT[i].act == -from) g_mainLW.cur = i; return; }
    if (s < 0) return;
    int act = M[s].act;
    if (act < 0) { sub = -act; g_mainLW = ListW(); return; }
    if (act == 0) { int from = sub; sub = 0; g_mainLW = ListW(); for (int i = 0; i < 12; i++) if (ROOT[i].act == -from) g_mainLW.cur = i; return; }
    switch (act) {
    case 1:
        g_careerActive = false;
        askConfirm("Type de match amical :", []() { g_frEnd = 0; openPick(PM_FRIENDLY_HOME); }, "Match complet", "Séance de tirs au but directe",
                   []() { g_frEnd = 5; openPick(PM_FRIENDLY_HOME); });
        g_confirm.backCancel = true;
        break;
    case 2: g_careerActive = false; g_lmLW = ListW(); g_screen = SC_LEAGUEMODE; break;
    case 3: {
        g_careerActive = false;
        g_intlCandidates = Career::euroCandidates();
        g_intlSel.clear(); g_euroModePick = true; g_leagueModePick = true;
        openPick(PM_INTL);
        break;
    }
    case 4: g_screen = SC_INTL; break;
    case 5:
        g_careerActive = false;
        g_intlCandidates.clear();
        for (int i = 0; i < NUM_NATIONS; i++) if (nationEligible(i)) g_intlCandidates.push_back(i);
        g_intlSel.clear(); g_coachModePick = true; g_euroModePick = false; g_leagueModePick = false;
        openPick(PM_INTL);
        break;
    case 6: case 7: case 8: g_sdPicking=false;g_optMode = act == 6 ? 2 : act == 7 ? 1 : 0; g_careerActive = false; openPick(PM_CAREER); break;
    case 35:g_careerActive=false;openSportingNew();break;
    case 9: g_sdPicking=false;g_careerActive = false; openLifeNew(); break;
    case 10: g_edMenuLW = ListW(); g_screen = SC_EDITMENU; break;
    case 11: openPick(PM_FICHE); break;
    case 12: g_screen = SC_LEGENDS; break;
    case 13:
        g_careerActive = false;
        g_intlCandidates = continentTeams();
        g_intlSel.clear(); g_contPick = true; g_coachModePick = false; g_euroModePick = false; g_leagueModePick = false;
        openPick(PM_INTL);
        break;
    case 20: g_customLW = ListW(); g_screen = SC_CUSTOM; break;
    case 21: g_careerActive = false; openPick(PM_TRAIN); break;
    case 30: g_optBack = SC_MAIN; g_screen = SC_OPTIONS; break;
    case 31: g_slotSave = false; g_screen = SC_SLOTS; break;
    case 32: g_screen = SC_ABOUT; break;
    case 33: OpenURL(TIPEEE_URL); toast("Merci pour votre soutien !"); break;
    case 34: g_quit = true; break;
    }
}

static void toggleMusic() {
    g_settings.music = !g_settings.music;
    audioMusicEnabled(g_settings.music);
    g_settings.save();
    toast(g_settings.music ? "Musique activée" : "Musique coupée");
}

// ------------------------------------------------------------------ boucle
extern std::string (*g_sponsorImagePath)(const std::string&);
void applyVibration() { rumbleSetStrength(!g_settings.vibration ? 0.f : g_settings.vibForce <= 1 ? 0.45f : g_settings.vibForce == 2 ? 0.75f : 1.f); }
void appInit() {
    g_sponsorImagePath = sponsorImagePath;
    g_settings.load();
    applyVibration();
    g_world.build();
    audioSetEnabled(g_settings.sound);
    audioMusicEnabled(g_settings.music);
    audioSetTrackMode(g_settings.musicTrack);
    loadAnthemFiles();
}

void appFrame(float dt) {
    audioUpdate();
    inputPoll(IN, CTL);
    rumbleUpdate(dt);
    {   // navigation des boutons à la manette
        static int lastScreen = -1;
        if ((int)g_screen != lastScreen) { g_btnFocus = -1; lastScreen = (int)g_screen; }
        if (g_btnCountPrev <= 0) g_btnFocus = -1;
        if (IN.btnNext && g_btnCountPrev > 0) g_btnFocus = (g_btnFocus + 1) % g_btnCountPrev;
        g_btnPress = false;
        if (IN.btnPress && !g_confirm.active) { if (g_btnFocus >= 0) g_btnPress = true; else IN.tab = true; }
        if (IN.back && g_btnFocus >= 0 && g_screen != SC_MATCH) { g_btnFocus = -1; IN.back = false; }   // B : quitte d'abord le choix des boutons
        g_btnCount = 0;
    }
    MenuInput saved = IN;
    if (g_confirm.active) { IN = MenuInput(); IN.mouse = saved.mouse; }
    if (IsKeyPressed(KEY_F11) || ((IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) && IsKeyPressed(KEY_ENTER))) {
        g_settings.fullscreen = !g_settings.fullscreen; ToggleBorderlessWindowed(); g_settings.save();
        IN.ok = IN.start = false;
    }
    if (IsKeyPressed(KEY_F9)) toggleMusic();
    if (g_screen != SC_MATCH && g_screen != SC_SPLASH) {   // bruitages de l'interface : retour, réglage gauche / droite
        if (IN.back) audioPlay(SFX_UI_BACK);
        else if (IN.left || IN.right) audioPlay(SFX_UI_TICK);
    }
    {   // fil d'Ariane pour debug.log : changement d'écran
        static int markScreen = -1;
        if ((int)g_screen != markScreen) { crashMark("écran %d", (int)g_screen); markScreen = (int)g_screen; }
    }
    g_noBackBtn = g_screen == SC_MAIN || g_screen == SC_JOBS || g_screen == SC_SPLASH;
    audioMusic(g_screen != SC_MATCH && g_screen != SC_HALFTIME && g_screen != SC_LOCKER && g_screen != SC_TVADS && g_screen != SC_TVINTRO && g_screen != SC_SETUP && g_screen != SC_STUDIO);
    switch (g_screen) {
    case SC_SPLASH: screenSplash(dt); break;
    case SC_LIFENEW: screenLifeNew(); break;
    case SC_LIFE: screenLife(); break;
    case SC_BRIBE: screenBribe(); break;
    case SC_SEASONSTART: screenSeasonStart(); break;
    case SC_LEGENDS: screenLegends(); break;
    case SC_COMPARCH: screenCompArch(); break;
    case SC_MAIN: screenMain(dt); break;
    case SC_PICK: screenPick(); break;
    case SC_SETUP: if(g_mctx.career&&g_career.sportingMode())launchMatch();else screenSetup(); break;
    case SC_MATCH: screenMatch(dt); break;
    case SC_POST: screenPost(); break;
    case SC_INTL: screenIntl(); break;
    case SC_HUB: screenHub(); break;
    case SC_COMPS: screenComps(); break;
    case SC_COMPVIEW: screenCompView(); break;
    case SC_FIXTURES: screenFixtures(); break;
    case SC_SQUAD: screenSquad(); break;
    case SC_HISTORY: screenHistory(); break;
    case SC_OPTIONS: screenOptions(); break;
    case SC_SLOTS: g_slotSave = g_slotSave; screenSlots(); break;
    case SC_SEASONEND: screenSeasonEnd(); break;
    case SC_BALLON: screenBallon();break;
    case SC_SEASONAWARDS:screenSeasonAwards();break;
    case SC_TEAMPLANS:screenTeamPlans();break;
    case SC_POLESCOUT:screenPoleScout();break;
    case SC_MUSEUM:screenMuseum();break;
    case SC_DIRECTOR:screenDirector();break;
    case SC_SPORTNEW:screenSportingNew();break;
    case SC_SPORTING:screenSporting();break;
    case SC_SUPPORTERS:screenSupporters();break;
    case SC_SUPPORTPARTY:screenSupporterParty(dt);break;
    case SC_LOCKER: screenLocker(); break;
    case SC_WALKMUSIC: screenWalkMusic(); break;
    case SC_TVADS: screenTvAds(); break;
    case SC_ADSEDIT: screenAdsEdit(); break;
    case SC_PRESS: screenPress(); break;
    case SC_MEDIA: screenMedia(); break;
    case SC_CUPNEWS: screenCupNews(); break;
    case SC_SOUNDTEST: screenSoundTest(); break;
    case SC_HELP: screenHelp(); break;
    case SC_FICHE: screenFiche(dt); break;
    case SC_EDITMENU: screenEditMenu(); break;
    case SC_ANTHEMS: screenAnthems(); break;
    case SC_COMPEDIT: if (g_ceMode == 6) screenCompEditCup(); else screenCompEdit(); break;
    case SC_CLUBEDIT: screenClubEdit(); break;
    case SC_DEPTPICK: screenDeptPick(); break;
    case SC_CUSTOM: screenCustom(); break;
    case SC_COEFF: screenCoeff(); break;
    case SC_CONTROLS: screenControls(dt); break;
    case SC_MATCHINFO: screenMatchInfo(); break;
    case SC_MARKET: screenMarket(); break;
    case SC_FINANCE: screenFinance(); break;
    case SC_NEWS: screenNews(); break;
    case SC_JOBS: screenJobs(); break;
    case SC_CAREEROPT: screenCareerOpt(); break;
    case SC_STATUS: screenStatus(); break;
    case SC_STADIUM:if(g_career.sportingMode()){openSporting(2);}else screenStadium();break;
    case SC_CUSTOMLIST: screenCustomList(); break;
    case SC_STAFF:if(g_career.sportingMode()){openSporting(12);}else screenStaff();break;
    case SC_RESERVES: screenReserves(); break;
    case SC_OFFERS: screenOffers(); break;
    case SC_ACADEMY: screenAcademy(); break;
    case SC_PLAYER: screenPlayer(); break;
    case SC_PLAYEREDIT: screenPlayerEdit(); break;
    case SC_TACTICS:if(g_career.sportingMode()){openSporting(5);}else screenTactics();break;
    case SC_EDITDB: screenEditDb(); break;
    case SC_MATCHDAY: screenMatchday(); break;
    case SC_CALLUP: screenCallup(); break;
    case SC_HOSTS: screenHosts(); break;
    case SC_COACHLOG: screenCoachLog(); break;
    case SC_COACHJOBS: screenCoachJobs(); break;
    case SC_KITS: screenKits(); break;
    case SC_FRIENDLIES: screenFriendlies(); break;
    case SC_REFEREES: screenReferees(); break;
    case SC_CLUBMENU:if(g_career.sportingMode()){openSporting(0);}else screenClubMenu();break;
    case SC_TRAINMODE: screenTrainMode(); break;
    case SC_TRAINING:if(g_career.sportingMode()){openSporting(5);}else screenTraining();break;
    case SC_DRAW: screenDraw(dt); break;
    case SC_STUDIO: screenStudio(dt); break;
    case SC_HALFTIME: screenHalftime(); break;
    case SC_ABOUT: screenAbout(); break;
    case SC_SPONSORS: screenSponsors(); break;
    case SC_MANAGERS: screenManagers(); break;
    case SC_LEAGUEMODE: screenLeagueMode(); break;
    case SC_TVINTRO: screenTvIntro(dt); break;
    case SC_ARTICLE: screenArticle(); break;
    default: break;
    }
    if (g_confirm.active) {
        IN = saved;
        if (g_confirmT > 0) {                   // la touche qui a ouvert la fenêtre ne doit pas la refermer
            g_confirmT -= dt;
            IN.ok = IN.back = IN.start = IN.left = IN.right = IN.up = IN.down = IN.click = false;
        }
        DrawRectangle(0, 0, VW, VH, Color{ 0, 0, 20, 170 });
        int w = 440, h = 90, x = VW / 2 - w / 2, y = VH / 2 - h / 2;
        DrawRectangle(x, y, w, h, Color{ 20, 34, 64, 255 });
        DrawRectangleLines(x, y, w, h, C_SEL);
        auto lines = wrapText(g_confirm.text, w - 30);
        int ty = y + 10;
        for (auto& l : lines) { drawTextCentered(l, VW / 2, ty, 10, C_TXT, false); ty += 12; }
        if (IN.left || IN.right || IN.up || IN.down) g_confirm.sel = 1 - g_confirm.sel;
        bool yes = button(VW / 2 - 150, y + h - 24, 140, 16, g_confirm.yes, g_confirm.sel == 0);
        bool no = button(VW / 2 + 10, y + h - 24, 140, 16, g_confirm.no, g_confirm.sel == 1);
        if (IN.ok) { if (g_confirm.sel == 0) yes = true; else no = true; }
        if (IN.back) { if (g_confirm.backCancel) { g_confirm.active = false; IN.back = false; } else no = true; }
        if (yes) { auto f = g_confirm.onYes; g_confirm.active = false; if (f) f(); }
        else if (no) { auto f = g_confirm.onNo; g_confirm.active = false; if (f) f(); }
    }
    if (g_screen == SC_HUB && IN.back) {}
    g_btnCountPrev = g_btnCount;
    if (getenv("FOOT_SPRITES")) {   // test : planche des sprites de course (4 directions x 4 temps)
        ClearBackground(Color{ 40, 110, 40, 255 });
        Kit k; k.shirt = 0x1B2A63; k.shorts = 0xFFFFFF; k.socks = 0xE2001A;
        int fr = (int)(GetTime() * 8);
        for (int d = 0; d < 4; d++) for (int f = 0; f < 4; f++) drawPlayerSprite(60 + f * 70, 90 + d * 70, k, 1, 0, d, f, PS_NORMAL, false, 0, 5);
        for (int d = 0; d < 4; d++) drawPlayerSprite(400 + d * 50, 200, k, 1, 0, d, fr, PS_NORMAL, false, 0, 3);
        { const Team& F = g_world.teams[g_world.nationIndex("FRA")]; const Team& B = g_world.teams[g_world.nationIndex("ARG")];
          for (int d = 0; d < 4; d++) { drawPlayerSprite(400 + d * 50, 300, F.home, d + 1, d, d, fr, PS_NORMAL, false, 0, 3); drawPlayerSprite(400 + d * 50, 350, B.home, d, d + 2, d, fr, PS_NORMAL, false, 0, 3); } }
    }
    if (g_toastT > 0) {
        g_toastT -= dt;
        int w = textWidth(g_toast, 10) + 20;
        DrawRectangle(VW / 2 - w / 2, VH - 40, w, 16, Color{ 0, 0, 0, 200 });
        drawTextCentered(g_toast, VW / 2, VH - 37, 10, C_HI, false);
    }
}

// accès pour le mode sauvegarde depuis le hub
void appSetSlotSave(bool s) { g_slotSave = s; }

// ------------------------------------------------------------------ mode test (captures automatiques)
void appTestStart(const char* mode) {
    std::string m = mode;
    if (m == "officialreg" || m == "officialdist" || m == "bretagne" || m == "finistere" || m == "normandie") {
        int tier = m == "officialreg" ? 5 : 8;
        int key = m == "officialreg" ? 6 : districtIndex("Maine-et-Loire");
        if (m == "bretagne") { tier = 5; key = 7; }
        if (m == "normandie") { tier = 5; key = 3; }
        if (m == "finistere") { tier = 9; key = districtIndex("Finistère"); }
        for (const auto& p : g_basePyramids) if (p.country == "FRA" && p.dom < 0) {
            int q = p.poolIndex(tier, key);
            if (q < 0 || p.pools[q].clubs.empty()) return;
            g_career.newClubCareer(p.pools[q].clubs[0], 2026); g_careerActive = true;
            for (const auto& careerP : g_career.pyramids) if (careerP.country == "FRA" && careerP.dom < 0) {
                int cq = careerP.poolIndex(tier,key);
                if (cq >= 0 && !careerP.pools[cq].comps.empty()) openCompView(careerP.pools[cq].comps[0],0,0);
            }
            return;
        }
    }
    if (m == "anthem" || m == "duel" || m == "lap" || m == "ettoss" || m == "reds" || m == "setupfr" || m == "hl" || m == "medic" || m == "card" || m == "pstats" || m == "fk" || m == "brawl" || m == "corner" || m == "throwin" || m == "pen" || m == "offside") {
        int a = g_world.nationIndex("FRA"), b = g_world.nationIndex("ARG");
        startSetup(a, b, false, -1, -1);
        if (m == "setupfr") { g_setupRow = 9; return; }
        for (int i = 0; i < NUM_INPUTS; i++) g_setup.side[i] = -1;
        if (m == "duel" || m == "fk") g_setup.side[IN_KB1] = 0;
        launchMatch();
        Match& M = *g_match;
        if(getenv("FOOT_TV_HUD")){M.S.tv=true;M.S.channel="TF SPORT";}
        if (m == "anthem" && getenv("FOOT_CER")) return;
        if (m == "anthem" && getenv("FOOT_ASSIST")) { M.S.goalAssist = true; M.ceremony = false; M.startPeriod(0); M.state = MS_PLAY; return; }
        if (m == "anthem" && getenv("FOOT_PRES")) { M.S.anthemOnly = "FRA"; M.S.president = true; M.S.anthems = true; M.cerPhase = 12; M.cerT = 0; for (int k = 0; k < 22; k++) M.pl[k].pos = anthemSpot(k / 11, k % 11); return; }
        if (m == "anthem") { M.cerPhase = 10; M.cerT = 0; for (int k = 0; k < 22; k++) M.pl[k].pos = anthemSpot(k / 11, k % 11); return; }
        M.ceremony = false; M.startPeriod(0); M.state = MS_PLAY;
        if (m == "duel") { M.fightLevel = 2; M.startFight(5, 16); M.duelHp[1] = 55; }
        if (m == "brawl") { for (int k = 0; k < 22; k++) { V2 o; M.formationTarget(k, o); M.pl[k].pos = o; } M.pl[16].pos = M.pl[5].pos + V2(0.8f, 0.3f); M.fightLevel = 2; M.startFight(5, 16); }
        if (m == "corner" || m == "throwin" || m == "pen") {
            for (int k = 0; k < 22; k++) { V2 o; M.formationTarget(k, o); M.pl[k].pos = o; }
            V2 g = M.goalCenter(0); bool top = g.y < 1;
            if (m == "corner") M.beginSetPiece(SP_CORNER, 0, V2(0.3f, top ? 0.3f : PITCH_L - 0.3f));
            else if (m == "throwin") M.beginSetPiece(SP_THROWIN, 0, V2(0.f, top ? 30.f : PITCH_L - 30.f));
            else M.beginSetPiece(SP_PENALTY, 0, V2(PITCH_W / 2, top ? 11.f : PITCH_L - 11.f));
        }
        if (m == "lap") { M.startPeriod(1); M.clock = 90; M.finishMatch(); g_trophyChecked = true; M.startLap(0); }
        if (m == "ettoss") { M.S.decisive = true; M.startPeriod(1); M.state = MS_BREAK; M.stateT = 1.9f; M.nextSp = 2; }
        if (m == "hl") { M.S.highlights = true; }
        if (m == "offside") {   // passe vers un attaquant hors-jeu de 2 m : drapeau de l'arbitre assistant
            for (int k = 0; k < 22; k++) { V2 o; M.formationTarget(k, o); M.pl[k].pos = o; M.pl[k].vel = V2(); }
            M.ball.pos = M.pl[6].pos; M.takePossession(6);
            float line = M.offsideLine(0); M.pl[9].pos = M.fromTeamFrame(0, line + 2.f / PITCH_L, 0.45f);
            M.passTo(6, 9, false);
        }
        if (m == "fk") { for (int k = 0; k < 22; k++) { V2 o; M.formationTarget(k, o); M.pl[k].pos = o; } V2 g = M.goalCenter(0); M.beginSetPiece(SP_FREEKICK, 0, V2(PITCH_W / 2 - 8, g.y + (g.y < 1 ? 24.f : -24.f))); M.spCurl = 0.7f; }
        if (m == "medic" || m == "card") for (int k = 0; k < 22; k++) { V2 o; M.formationTarget(k, o); M.pl[k].pos = o; }
        if (m == "medic") { M.pl[16].injured = true; M.pl[16].state = PS_DOWN; M.state = MS_STOP; M.stateT = 0; M.nextSp = SP_FREEKICK; M.nextSpTeam = 1; M.nextSpPos = M.pl[16].pos; }
        if (m == "card") { M.state = MS_STOP; M.stateT = 0; M.nextSp = SP_FREEKICK; M.nextSpTeam = 1; M.nextSpPos = M.pl[5].pos; M.pendCardOff = 5; M.pendCardType = 1; M.pendCardT = 0; }
        if (m == "pstats") { M.clock = 30; M.giveCard(4, 1); M.score[0] = 1; MatchEvent e; e.type = 0; e.team = 0; e.minute = 12; e.player = "K. Mbappé"; e.assist = "A. Griezmann"; M.events.push_back(e); M.shots[0]=12;M.shots[1]=8;M.onTarget[0]=5;M.onTarget[1]=3;M.passes[0]=340;M.passes[1]=265;M.completedPasses[0]=289;M.completedPasses[1]=211;M.saves[0]=3;M.saves[1]=4;M.woodwork[0]=1;M.blockedShots[1]=2;M.throws[0]=12;M.freeKicks[1]=7;M.S.attendance=12345;M.corners[0] = 3; M.fouls[1] = 4; g_paused = true; g_pauseMenu = 2;
            if (getenv("FOOT_POST")) { g_paused = false; M.clock = 90; M.finishMatch(); finishMatchToResult(); g_postPage = atoi(getenv("FOOT_POST")); } }
        if (m == "reds") { M.giveCard(3, 2); M.giveCard(4, 2); M.giveCard(14, 2); M.state = MS_PLAY; }
        return;
    }
    if (m == "match" || m == "match2" || m == "board" || m == "subtest" || m == "goaltest" || m == "cardtest" || m == "photo" || m == "toss" || m == "trophy" || m == "trophy2" || m == "motm" || m == "locker" || m == "invasion") {
        int a = g_world.nationIndex("FRA"), b = g_world.nationIndex("ARG");
        startSetup(a, b, false, -1, -1);
        for (int i = 0; i < NUM_INPUTS; i++) g_setup.side[i] = -1;
        if (m == "match2" || m == "locker") g_setup.side[IN_KB1] = 0;
        g_setup.pitch = 1;
        if (getenv("FOOT_TURF")) g_setup.turf = atoi(getenv("FOOT_TURF"));
        if (getenv("FOOT_METEO")) { g_setup.meteo = atoi(getenv("FOOT_METEO")); meteoApply(g_setup, false, g_setup.turf); }
        if (m == "photo") { g_setup.cupPhoto = true; g_setup.side[IN_KB1] = 1; }
        if (m == "toss") g_setup.side[IN_KB1] = 1;
        launchMatch();
        if (m == "toss") { g_match->cerPhase = 3; g_match->cerT = 0; }
        if (m == "trophy" || m == "trophy2") {
            Match& M = *g_match; M.ceremony = false; M.startPeriod(1); M.clock = 90; M.score[0] = 2; M.score[1] = 1;
            for (int i = 0; i < 22; i++) M.pl[i].pos = V2(10.f + (i * 7) % 50, 30.f + (i * 13) % 50);
            M.finishMatch(); g_trophyChecked = true;
            static const char* TN[5] = { "District 1", "Régional 1", "Coupe de France", "Ligue 1", "Ligue des champions" };
            static const int TS[5] = { 3, 4, 0, 2, 1 };
            int tier = getenv("FOOT_TROPHY_TIER") ? std::max(0, std::min(4, atoi(getenv("FOOT_TROPHY_TIER")))) : (m == "trophy" ? 2 : 3);
            M.trophyTier = tier;
            bool cup = tier == 2 || tier == 4;
            M.startTrophy(cup ? 1 : 0, cup ? 0 : 1, TS[tier], TN[tier]);
        }
        if (m == "invasion") { Match& M = *g_match; M.ceremony = false; M.startPeriod(1); M.clock = 90; M.score[0] = 3; M.score[1] = 1; for (int i = 0; i < 22; i++) { V2 o; M.formationTarget(i, o); M.pl[i].pos = o; } M.finishMatch(); g_trophyChecked = true; M.startInvasion(0); }
        if (m == "locker") {   // FOOT_LOCKER=1 FOOT_LOCKER_PHASE=0/1/2 FOOT_SCORE=ab
            Match& M = *g_match; M.ceremony = false; const char* sc = getenv("FOOT_SCORE"); if (sc && strlen(sc) >= 2) { M.score[0] = sc[0] - '0'; M.score[1] = sc[1] - '0'; }
            int ph = getenv("FOOT_LOCKER_PHASE") ? atoi(getenv("FOOT_LOCKER_PHASE")) : 0; g_screen = SC_MATCH; openLocker(ph);
        }
        if (m == "motm") {   // fin d'un match télévisé : homme du match
            Match& M = *g_match; M.ceremony = false; M.S.tv = true; M.S.channel = "FRANCE SPORT"; M.startPeriod(1); M.state = MS_PLAY;
            for (int k = 0; k < 2400 && M.clock < 89.5f; k++) { M.update(1.f / 60); M.sfxN = 0; }
            M.clock = 90; M.finishMatch(); g_trophyChecked = true;
        }
        if (m == "board") { g_match->ceremony = false; g_match->startPeriod(0); g_match->clock = getenv("FOOT_CLOCK") ? (float)atof(getenv("FOOT_CLOCK")) : 43.7f; if (getenv("FOOT_PAUSE")) { g_paused = true; g_pauseMenu = 0; g_setup.side[IN_KB1] = 0; g_match->S.side[IN_KB1] = 0; } }
        if (m == "subtest" || m == "goaltest" || m == "cardtest") {
            Match& M = *g_match; M.ceremony = false; M.startPeriod(0);
            M.state = MS_PLAY; M.clock = 20;
            if (m == "subtest") { M.substitute(0, 6, 0); M.state = MS_STOP; M.stateT = 0; M.nextSp = SP_THROWIN; M.nextSpTeam = 0; M.nextSpPos = V2(0.2f, 45.f); M.cam = V2(10, 50); }
            if (m == "goaltest") { int f = 10; M.ball.pos = V2(PITCH_W / 2, 3); M.pl[f].pos = V2(PITCH_W / 2 + 3, 8); M.ball.lastTouch = f; M.ball.lastTeam = 0; M.attackDir[0] = -1; M.goalScored(0); if (getenv("FOOT_CELEB")) M.setCelebration(atoi(getenv("FOOT_CELEB"))); }
            if (m == "cardtest") { M.pl[14].pos = M.pl[3].pos + V2(0.5f, 0); M.foul(14, 3, true); M.pendCardOff = 14; M.pendCardType = 1; if (getenv("FOOT_SECOND")) M.pl[14].yellow = 1; }
        }
    } else if (m == "tvads") {
        startSetup(g_world.nationIndex("FRA"), g_world.nationIndex("ARG"), false, -1, -1); setFriendlyTv(0);
        openTvAds(0, SC_TVINTRO); if (getenv("FOOT_AD")) { g_adsSpots[0] = atoi(getenv("FOOT_AD")); g_adsSpots[1] = (g_adsSpots[0] + 1) % NUM_AD_SPOTS; g_adsSpots[2] = (g_adsSpots[0] + 2) % NUM_AD_SPOTS; }
    } else if (m == "walkmusic") { g_screen = SC_WALKMUSIC;
    } else if (m == "adsedit") { g_screen = SC_ADSEDIT; if (getenv("FOOT_AE_ROW")) g_aeLW.cur = atoi(getenv("FOOT_AE_ROW"));
    } else if (m == "setup") {
        startSetup(g_world.nationIndex("FRA"), g_world.nationIndex("BRA"), false, -1, -1);
    } else if (m == "soundtest") { g_screen = SC_SOUNDTEST;
    } else if (m == "customcomp") { g_cdefPreset = getenv("FOOT_PRESET") ? atoi(getenv("FOOT_PRESET")) : 1; customApplyPreset(g_cdefPreset); g_screen = SC_CUSTOM;
    } else if (m == "continents" || m == "contmatch") {
        auto T = continentTeams(); continentsStart(g_career, { T[0] }); g_careerActive = true; g_needAdvance = true; openHub();
        if (m == "contmatch") { g_pending = g_career.season.advance(false); g_needAdvance = false; const MatchRes& mr = g_career.season.comps[g_pending.comp].matches[g_pending.match]; startSetup(mr.home, mr.away, true, g_pending.comp, g_pending.match); for (int i = 0; i < NUM_INPUTS; i++) g_setup.side[i] = -1; launchMatch(); }
    } else if (m == "hub") {
        int user = -1;
        for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Stade Brestois") user = i;
        g_career.newClubCareer(user, 2026); g_careerActive = true; g_needAdvance = true;
        if (getenv("FOOT_SIM")) {   // test : quelques journées jouées (matchs du joueur simulés)
            for (int n = 0; n < atoi(getenv("FOOT_SIM")); n++) { PendingMatch pm = g_career.season.advance(false); if (pm.comp < 0) break; Competition& C = g_career.season.comps[pm.comp]; simulateMatch(C.matches[pm.match], &C); g_career.season.recordResult(pm.comp, pm.match); g_career.season.finishRoundOthers(pm.comp, pm.match); }
            g_career.ballonTick(); for (auto& e : g_career.ballonEditions) e.presented = 1; g_needAdvance = true;
        }
        openHub();
        if (getenv("FOOT_MEDIA")) { g_needAdvance = false; openMediaInvite(); if (getenv("FOOT_MEDIA_A")) { g_mediaA = std::max(0, std::min(2, atoi(getenv("FOOT_MEDIA_A")))); mediaApply(g_mediaA); } if (getenv("FOOT_MEDIA_END")) g_mediaStep = 3; }
        if (getenv("FOOT_CUPNEWS")) {
            int want = atoi(getenv("FOOT_CUPNEWS"));
            int guard = 0; while (guard++ < 20000 && !g_career.season.finished) { int c = cupNewsPending(); if (c >= 0 && (want == 0 || g_career.season.comps[c].kind == want)) break; if (c >= 0) { Career::KeyVal kv; kv.k = c; kv.v = g_career.year; g_career.monthly.cupNews.push_back(kv); } g_career.season.advance(true); }
            int c = cupNewsPending(); g_needAdvance = false; if (c >= 0) openCupNews(c);
        }
        if (getenv("FOOT_PRESS")) { g_pending = g_career.season.advance(false); g_needAdvance = false; if (g_pending.comp >= 0) { openPress(g_pending.comp, g_pending.match); if (getenv("FOOT_PRESS_Q")) g_pressQ = std::max(0, std::min(9, atoi(getenv("FOOT_PRESS_Q")))); if (getenv("FOOT_PRESS_A")) { g_pressA = std::max(0, std::min(2, atoi(getenv("FOOT_PRESS_A")))); pressApply(g_pressA); } } }
    } else if (m == "coach" || m == "callup" || m == "hosts" || m == "coachlog" || m == "coachend") {
        g_career.newCoachCareer(g_world.nationIndex("FRA")); g_careerActive = true; g_needAdvance = true;
        if (m == "coach") { openHub(); return; }
        if (m == "callup") { openCallup(SC_HUB); return; }
        if (m == "hosts") { g_screen = SC_HOSTS; return; }
        if (m == "coachlog") { g_screen = SC_COACHLOG; return; }
        while (!g_career.season.finished) { g_career.season.advance(true); }
        g_screen = SC_SEASONEND; return;
    } else if(m=="role-plan"||m=="role-player"||m=="simple-tactics"||m=="role-scout"){
        int ti=g_world.nationIndex("FRA");for(int i=g_world.firstClub;i<(int)g_world.teams.size();i++)if(g_world.teams[i].name=="Stade Brestois")ti=i;
        g_career.newClubCareer(ti,2026);g_careerActive=true;g_career.opts.lite=m=="simple-tactics";g_rolePlayerTab=m=="simple-tactics"?0:1;
        if(m=="role-scout"){g_mkDetailed=DP_MC;g_mkRole=1;g_mkSort=5;openMarket();}else if(m=="role-player"){openPlayer(ti,5,SC_SQUAD);g_rolePlayerTab=1;}else g_screen=SC_TACTICS;
    } else if (m == "matchday" || m == "matchday2") {
        int user = -1;
        for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Stade Brestois") user = i;
        g_career.newClubCareer(user, 2026); g_careerActive = true;
        PendingMatch pm;
        for (int k = 0; k < (m == "matchday2" ? 12 : 3); k++) { pm = g_career.season.advance(false); if (pm.comp < 0) break; if (k < (m == "matchday2" ? 11 : 2)) { MatchRes& r = g_career.season.comps[pm.comp].matches[pm.match]; simulateMatch(r, &g_career.season.comps[pm.comp]); g_career.season.recordResult(pm.comp, pm.match); g_career.season.finishRoundOthers(pm.comp, pm.match); g_career.season.checkRound(pm.comp); } }
        if (pm.comp >= 0) openMatchday(pm.comp, pm.match, false);
    } else if (m == "kits") {
        int user = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Stade Brestois") user = i;
        g_career.newClubCareer(user, 2026); g_careerActive = true; openKits();
        g_kitThird = true; g_kitEd[2].pattern = KP_CHEVRON; g_kitEd[0].pattern = KP_QUARTERS; g_kitEd[1].pattern = KP_BAND;
    } else if (m == "editmenu") {
        g_edMenuLW = ListW(); g_screen = SC_EDITMENU;
    } else if (m == "clubedit") {
        int t = g_world.nationIndex("FRA"); for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Stade Brestois") t = i;
        openClubEditor(t, false, SC_MAIN); g_ed.hasThird = 1; g_ed.third.shirt = 0x111111; g_ed.third.shirt2 = 0xFFD700; g_ed.third.pattern = KP_SHOULDERS; g_edRow = 5;
    } else if (m == "fichecal") {
        int user = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Stade Brestois") user = i;
        g_career.newClubCareer(user, 2026); g_careerActive = true;
        for (int k = 0; k < 60; k++) { auto pm = g_career.season.advance(true); if (pm.comp < 0) break; }
        int res = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].parent == user) { res = i; break; }
        openFiche(res >= 0 ? res : user, SC_MAIN); g_fichePage = 4;
    } else if (m == "lifenew") {
        openLifeNew();
    } else if (m == "lifehub" || m == "life" || m == "life2" || m == "life3" || m == "lifesim" || m == "bribe" || m == "lifematch") {
        int club = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Stade Lavallois") club = i;
        openLifeNew(); g_lifeNew.name = "Kylian Testeur"; g_lifeNew.age = m == "lifehub" ? 17 : 22;
        lifeStartCareer(club);
        if (m == "lifesim") {
            Season& S = g_career.season;
            for (int k = 0; k < 400; k++) {
                auto pm = S.advance(false); if (pm.comp < 0) break;
                g_career.mgrTick();
                if (k == 5) { std::string e; g_career.bribe(pm.comp, pm.match, 2, e); g_career.life.cash += 500000; g_career.lifeBuy(0, 3, e); g_career.lifeDate(e); }
                auto& C = S.comps[pm.comp]; simulateMatch(C.matches[pm.match], &C); S.recordResult(pm.comp, pm.match); g_career.mgrAfterMatch(pm.comp, pm.match); S.finishRoundOthers(pm.comp, pm.match); S.checkRound(pm.comp);
                lifeCheckPromotion();
            }
            g_screen = SC_LIFE; g_lifeTab = 0;
        }
        if (m == "life") { g_screen = SC_LIFE; g_lifeTab = 0; }
        if (m == "lifehub") { g_needAdvance = true; g_screen = SC_HUB; }
        if (m == "life2") { g_screen = SC_LIFE; g_lifeTab = 3; }
        if (m == "lifematch") {
            g_pending = g_career.season.advance(false); g_needAdvance = false;
            const MatchRes& mm = g_career.season.comps[g_pending.comp].matches[g_pending.match];
            startSetup(mm.home, mm.away, true, g_pending.comp, g_pending.match); launchMatch();
            g_match->ceremony = false; g_match->startPeriod(0); g_match->state = MS_PLAY;
        }
        if (m == "life3") { g_screen = SC_LIFE; g_lifeTab = 1; }
        if (m == "bribe") { g_career.life.isPlayer = 0; g_needAdvance = true; screenHub(); g_screen = SC_BRIBE; }
    } else if (m == "penonly" || m == "penpick") {
        startSetup(g_world.nationIndex("FRA"), g_world.nationIndex("ENG"), false, -1, -1);
        for (int i = 0; i < NUM_INPUTS; i++) g_setup.side[i] = -1;
        if (m == "penpick") g_setup.side[IN_KB1] = 0;     // équipe humaine : choix des tireurs
        g_setup.pensOnly = true; g_setup.decisive = true; g_setup.noET = true;
        launchMatch();
        if (m == "penpick") { g_match->pl[15].onPitch = false; g_match->pl[15].sentOff = true; }   // l'adversaire a pris un rouge : il faut écarter un joueur
    } else if (m == "pick") {
        openPick(PM_CAREER);
    } else if (m == "fk" || m == "corner" || m == "pen" || m == "throw" || m == "rain" || m == "snow") {
        startSetup(g_world.nationIndex("FRA"), g_world.nationIndex("ENG"), false, -1, -1);
        for (int i = 0; i < NUM_INPUTS; i++) g_setup.side[i] = -1;
        g_setup.pitch = m == "rain" ? 2 : m == "snow" ? 4 : 0;
        launchMatch();
        Match& M = *g_match;
        V2 g = M.goalCenter(0);
        float s = g.y == 0 ? 1.f : -1.f;
        if (m == "fk") M.beginSetPiece(SP_FREEKICK, 0, V2(PITCH_W / 2 - 6, g.y + s * 22));
        if (m == "corner") M.beginSetPiece(SP_CORNER, 0, V2(0.3f, g.y + s * 0.3f));
        if (m == "pen") M.beginSetPiece(SP_PENALTY, 0, V2(PITCH_W / 2, g.y + s * 11));
        if (m == "throw") M.beginSetPiece(SP_THROWIN, 0, V2(0, 60));
        M.msgT = 0;
    } else if (m == "trainfk" || m == "trainpen" || m == "trainmode") {
        int t = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Olympique de Marseille") t = i;
        g_trainTeam = t < 0 ? g_world.firstClub : t;
        g_screen = SC_TRAINMODE;
        if (m != "trainmode") {
            MatchSetup ms; ms.home = ms.away = g_trainTeam;
            for (int d = 0; d < NUM_INPUTS; d++) ms.side[d] = -1;
            ms.training = getenv("FOOT_TRAIN_DRILL") ? atoi(getenv("FOOT_TRAIN_DRILL")) : m == "trainfk" ? 2 : 1; ms.title = "Entraînement"; ms.stadium = "Centre d'entraînement"; ms.halfSeconds = 600; ms.crowdFill = 0.02f;
            g_match.reset(new Match()); g_match->init(ms); g_screen = SC_MATCH;
        }
    } else if (m == "drawall") {   // test : saisons complètes, chaque tirage affiché à l'écran (équipes de jeunes contrôlées)
        std::vector<int> users;
        for (auto& P : g_basePyramids) if (P.country == "FRA" && P.dom < 0) { std::vector<int> tiers; for (auto& pool : P.pools) if (!pool.clubs.empty() && std::find(tiers.begin(), tiers.end(), pool.tier) == tiers.end()) { tiers.push_back(pool.tier); users.push_back(pool.clubs[pool.clubs.size() / 3]); } }
        int screens = 0;
        for (int u : users) {
            g_career.opts = Career::Opts(); g_career.newClubCareer(u, 2026); g_careerActive = true;
            g_career.mgr.ctrlReserves.clear(); for (int t = 0; t < (int)g_world.teams.size(); t++) if (g_world.teams[t].parent == u) g_career.mgr.ctrlReserves.push_back(t); g_career.syncControlled();
            Season& S = g_career.season;
            g_drawQueue.clear(); g_drawSeen.clear(); g_drawSeenYear = g_career.year;
            for (int k = 0; k < 6000; k++) {
                auto pm = S.advance(false);
                if (pm.comp < 0) break;
                if (collectDraws()) {
                    int guard = 0;
                    while (!g_drawQueue.empty() && guard++ < 20000) {
                        IN = MenuInput(); if (guard % 7 == 0) IN.ok = true;
                        BeginDrawing(); screenDraw(0.1f); EndDrawing(); screens++;
                    }
                }
                auto& C = S.comps[pm.comp]; simulateMatch(C.matches[pm.match], &C); S.recordResult(pm.comp, pm.match); genMatchEvents(C, pm.match); S.checkRound(pm.comp);
            }
            fprintf(stderr, "drawall %s ok (%d frames)\n", g_world.teams[u].name.c_str(), screens);
        }
        fprintf(stderr, "PASS drawall\n"); exit(0);
    } else if (m == "draw" || m == "drawgrp" || m == "seasonend" || m == "drawbig" || m == "drawlp") {
        const char* nm = m == "draw" ? "Stade Lavallois" : m == "drawbig" ? "Stade Briochin" : "Paris Saint-Germain";
        int user = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == nm) user = i;
        g_career.opts = Career::Opts(); if (m == "drawlp") g_career.opts.euroFormat = 1;
        g_career.newClubCareer(user, 2026); g_careerActive = true;
        Season& S = g_career.season;
        g_drawQueue.clear(); g_drawSeen.clear(); g_drawSeenYear = g_career.year;
        for (int k = 0; k < 3000; k++) {
            auto pm = S.advance(false);
            if (pm.comp < 0) break;
            if (m != "seasonend" && collectDraws()) {
                if (m == "drawlp") { while (!g_drawQueue.empty() && S.comps[g_drawQueue[0].first].stages[g_drawQueue[0].second].type != ST_SWISS) g_drawQueue.erase(g_drawQueue.begin()); if (g_drawQueue.empty()) continue; g_pending = pm; g_needAdvance = false; g_screen = SC_DRAW; return; }
                if (m == "drawgrp") { bool grp = false; for (auto& q : g_drawQueue) if (!S.comps[q.first].stages[q.second].groups.empty()) grp = true; if (!grp) { g_drawQueue.clear(); } else { while (S.comps[g_drawQueue[0].first].stages[g_drawQueue[0].second].groups.empty()) g_drawQueue.erase(g_drawQueue.begin()); g_pending = pm; g_needAdvance = false; g_screen = SC_DRAW; return; } }
                else { g_pending = pm; g_needAdvance = false; g_screen = SC_DRAW; return; }
            }
            auto& C = S.comps[pm.comp]; simulateMatch(C.matches[pm.match], &C); S.recordResult(pm.comp, pm.match); genMatchEvents(C, pm.match); S.checkRound(pm.comp);
        }
        g_screen = SC_SEASONEND; seTab = 2;
    } else if (m == "euromode") {
        int psg = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Paris Saint-Germain") psg = i;
        g_career.newEuroCareer({ psg }, 2026); g_careerActive = true; g_needAdvance = true; openHub();
    } else if (m == "offers" || m == "academy" || m == "clubmenu2") {
        int user = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Stade Rennais") user = i;
        g_career.newClubCareer(user, 2026); g_careerActive = true; g_needAdvance = true;
        g_career.genOffers(3);
        g_screen = m == "offers" ? SC_OFFERS : m == "academy" ? SC_ACADEMY : SC_CLUBMENU; return;
    } else if (m == "u19res" || m == "u19comps") {
        int user = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Stade Brestois") user = i;
        g_career.newClubCareer(user, 2026); g_careerActive = true; g_needAdvance = true;
        if (m == "u19res") { g_screen = SC_RESERVES; return; }
        int yp = g_career.u19Pyramid(); openCompView(g_career.pyramids[yp].pools[0].comps[0], 0, 0);
    } else if (m == "player" || m == "playeredit" || m == "tactics" || m == "youthucl" || m == "clubedit2") {
        int user = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Stade Brestois") user = i;
        g_career.newClubCareer(user, 2026); g_careerActive = true; g_needAdvance = true;
        if (m == "youthucl") {
            Season& S = g_career.season;
            while (g_career.youthUcl < 0 || S.now < 13) { auto pm = S.advance(true); if (pm.comp < 0) break; }
            openCompView(g_career.youthUcl, 0, 0); return;
        }
        if (m == "tactics") { g_screen = SC_TACTICS; return; }
        if (m == "clubedit2") { openClubEditor(user, false, SC_MAIN); return; }
        g_career.recordSeasonHistory();
        openPlayer(user, 0, SC_MAIN);
        if (m == "playeredit") openPlayerEditorFwd();
    } else if (m == "careeropt") { int u = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Stade Brestois") u = i; startCareerWith(u, SC_MAIN); }
    else if (m == "editdb") { openEditDb(); g_dbFilter = "brest"; }
    else if (m == "deptpick") { openClubEditor(-1, false, SC_MAIN); g_screen = SC_DEPTPICK; }
    else if (m == "coeffpts" || m == "jeunes" || m == "spots") {
        int user = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Stade Brestois") user = i;
        g_career.newClubCareer(user, 2026); g_careerActive = true; g_needAdvance = true;
        if (m == "jeunes") { g_screen = SC_COMPS; g_compsCat = 1; g_compsSub = 3; return; }
        g_screen = SC_COEFF; g_coeffTab = m == "spots" ? 0 : 3;
    }
    else if (m == "wcareer" || m == "wcomps" || m == "wtable" || m == "wmatch" || m == "wpick") {
        int u = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "OL Lyonnes") u = i;
        if (m == "wpick") { openPick(PM_CAREER); Node nn; nn.kind = K_WOMENROOT; nn.title = "Football féminin"; g_stack.push_back(nn); return; }
        if (m == "wmatch") { int b2 = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "PSG Féminines") b2 = i; startSetup(u, b2, false, -1, -1); for (int i = 0; i < NUM_INPUTS; i++) g_setup.side[i] = -1; launchMatch(); g_match->ceremony = false; g_match->startPeriod(0); g_match->state = MS_PLAY; return; }
        g_career.newClubCareer(u, 2026); g_careerActive = true;
        if (m == "wcomps") { g_screen = SC_COMPS; g_compsCat = 5; return; }
        if (m == "wtable") { auto& S = g_career.season; for (int k = 0; k < 400; k++) { auto pm = S.advance(true); if (pm.comp < 0) break; } int p, q, g; g_career.tierOfTeam(u, &p, &q, &g); openCompView(g_career.pyramids[p].pools[q].comps[g]); return; }
        openHub();
    }
    else if (m == "legends") g_screen = SC_LEGENDS;
    else if (m == "legend82") { auto t = legendTeams(12); legendStart(g_career, 12, { t[0] }); g_careerActive = true; auto& S = g_career.season; for (int k = 0; k < 20; k++) { auto pm = S.advance(true); if (pm.comp < 0) break; } openCompView(0); }
    else if(m=="hub-menu"){
        const char* country=getenv("FOOT_COUNTRY");int user=-1;
        for(int t=0;t<(int)g_world.teams.size();t++)if(g_world.teams[t].name=="Cuchery FC")user=t;
        if(country&&std::string(country)!="FRA")for(auto& P:g_basePyramids)if(P.country==country&&P.dom<0&&!P.pools.empty()&&!P.pools[0].clubs.empty()){user=P.pools[0].clubs[0];break;}
        if(user<0)for(auto& P:g_basePyramids)if(P.country=="FRA"&&P.dom<0)for(auto& pool:P.pools)if(pool.tier>=8)for(int t:pool.clubs)if(g_world.teams[t].dept==51){user=t;break;}
        if(user<0)user=g_world.firstClub;
        g_career.opts=Career::Opts();g_career.newClubCareer(user,2026);g_careerActive=true;g_career.cdfAskYear=g_career.year;
        g_pending=g_career.season.advance(false);g_career.season.now=std::max(0.7,g_career.season.now);g_career.mgr.needStatus=0;g_needAdvance=false;g_screen=SC_HUB;
        g_hubCategory=getenv("FOOT_HUB_CATEGORY")?atoi(getenv("FOOT_HUB_CATEGORY")):0;g_hubMenu=ListW();
        if(getenv("FOOT_HUB_SIMPLE"))g_career.opts.lite=1;
        if(getenv("FOOT_HUB_FINISHED")){g_career.season.finished=true;g_pending=PendingMatch();}
        if(!getenv("FOOT_HUB_OPEN")){screenHub();if(g_screen!=SC_HUB){fprintf(stderr,"FAIL test did not reach career hub\n");exit(4);}}
        if(getenv("FOOT_HUB_OPEN")){g_hubMenu.cur=atoi(getenv("FOOT_HUB_OPEN"));IN.ok=true;screenHub();IN.ok=false;int expected=-1;if(g_hubCategory==3){Screen screens[]={SC_HISTORY,SC_BALLON,SC_SEASONAWARDS,SC_MUSEUM};if(g_hubMenu.cur<4)expected=screens[g_hubMenu.cur];}else if(g_hubCategory==5)expected=g_hubMenu.cur==0?SC_SLOTS:SC_OPTIONS;if(expected>=0&&g_screen!=expected){fprintf(stderr,"FAIL hub menu action routed to wrong screen\n");exit(3);}}
        return;
    }
    else if(m.rfind("personality",0)==0){
        int user=-1,target=-1;for(auto& P:g_basePyramids)if(P.dom<0)for(auto& pool:P.pools){if(P.country=="FRA"&&pool.tier==(m=="personality-amateur"?8:0)&&!pool.clubs.empty())user=pool.clubs[0];if(P.country=="GER"&&pool.tier==0&&!pool.clubs.empty())target=pool.clubs[0];}
        g_career.opts=Career::Opts();if(m=="personality-simple")g_career.opts.lite=1;g_career.newClubCareer(user,2026);g_careerActive=true;g_needAdvance=false;g_career.cdfAskYear=g_career.year;g_career.mgr.needStatus=0;
        bool scout=m=="personality-scout-low"||m=="personality-scout-high";int team=scout?target:user;g_world.ensureSquad(team);auto& T=g_world.teams[team];auto& p=T.squad[0];
        if(!scout&&m!="personality-simple"){p.age=18;auto& P=g_career.personalityPlayer(p,team);P.traits.professionalism=92;P.traits.ambition=88;auto& R=g_career.personalityRelation(P,team);if(m=="personality-foreign")R.adaptationProgress=35;if(m=="personality-history"){g_career.personalityEvent(P,team,2,p.name+" suit un mentor expérimenté.");g_career.personalityEvent(P,team,5,p.name+" s'est pleinement adapté au club.");g_psychArchive=true;}}
        if(scout)g_career.personalityScout(p,team,m=="personality-scout-high"?95:20,m=="personality-scout-high"?95:40);
        openPlayer(team,0,SC_HUB);g_rolePlayerTab=m=="personality-simple"?0:2;g_psychArchive=m=="personality-history";
        if(m=="personality-mentor"){auto& q=T.squad[1];q.age=32;auto& P=g_career.personalityPlayer(q,team);P.traits.professionalism=P.traits.leadership=95;std::string msg;if(!g_career.personalityMentor(team,p.id,q.id,msg)){fprintf(stderr,"FAIL personality mentor UI fixture\n");exit(11);}}
        if(m=="personality-navigation"){g_rolePlayerTab=0;IN.click=true;IN.mouse={300,40};screenPlayer();if(g_rolePlayerTab!=1)exit(11);IN.mouse={540,40};screenPlayer();if(g_rolePlayerTab!=2)exit(11);IN.mouse={100,40};screenPlayer();if(g_rolePlayerTab!=0)exit(11);IN.click=false;}
        if(m=="personality-tabs"){g_rolePlayerTab=0;IN.tab=true;screenPlayer();if(g_rolePlayerTab!=1){fprintf(stderr,"FAIL personality role tab\n");exit(11);}screenPlayer();if(g_rolePlayerTab!=2){fprintf(stderr,"FAIL personality third tab\n");exit(11);}screenPlayer();if(g_rolePlayerTab!=0){fprintf(stderr,"FAIL personality tab cycle\n");exit(11);}IN.tab=false;g_rolePlayerTab=2;}
        return;
    }
    else if(m.rfind("supporters",0)==0){
        int user=-1;for(auto& P:g_basePyramids)if(P.country=="FRA"&&P.dom<0)for(auto& pool:P.pools)if(pool.tier==(m=="supporters-major"?0:8)&&!pool.clubs.empty()){user=pool.clubs[0];break;}
        g_career.opts=Career::Opts();g_career.newClubCareer(user,2026);g_careerActive=true;g_needAdvance=false;g_career.cdfAskYear=g_career.year;g_career.mgr.needStatus=0;
        auto& P=g_career.supportersProfile(user);P.lastAttendance=g_world.teams[user].sta.capacity()*8/10;P.lastVisitors=P.lastAttendance/10;P.atmosphereIndex=77;
        if(m=="supporters-crisis"){P.protestLevel=5;P.boycottMatches=2;P.boardTrust=10;P.transferTrust=15;P.sportingSatisfaction=12;P.lastChant=CH_PROTEST;g_career.supportersEvent(user,SU_BOYCOTT,"Boycott exceptionnel : deux rencontres à domicile après une crise prolongée.",90,2);}
        for(int i=0;i<10;i++){SupporterAttendance a;a.year=2016+i;a.matches=P.homeGames;a.total=P.lastAttendance*a.matches;a.best=P.lastAttendance;a.worst=P.lastAttendance*7/10;a.subscribers=P.seasonTicketHolders;P.attendanceHistory.push_back(a);}
        openSupporters(user,SC_HUB);if(getenv("FOOT_SUPPORTER_PAGE"))g_supPage=std::clamp(atoi(getenv("FOOT_SUPPORTER_PAGE")),0,7);
        if(m=="supporters-party"){g_career.supportersOnPromotion(user,P.lastTier-1);showSupporterParty(SC_SUPPORTERS);}
        if(m=="supporters-match"||m=="supporters-whistles"){g_pending=g_career.season.advance(false);auto& M=g_career.season.comps[g_pending.comp].matches[g_pending.match];startSetup(M.home,M.away,true,g_pending.comp,g_pending.match);launchMatch();g_match->S.supporterTifo=1;g_match->S.supporterAtmosphere=90;if(m=="supporters-whistles"){g_match->S.supporterChant=CH_PROTEST;g_match->clock=65;g_match->score[1]=4;}}
        return;
    }
    else if(m.rfind("sporting",0)==0){
        int user=-1;for(auto& P:g_basePyramids)if(P.country=="FRA"&&P.dom<0)for(auto& pool:P.pools)if(pool.tier==(m=="sporting-amateur"?8:0)&&!pool.clubs.empty()){user=pool.clubs[0];break;}
        SportingProfile profile;profile.nation=g_world.nationIndex("FRA");profile.reputation=m=="sporting-amateur"?15:65;profile.experience=15;profile.start=3;
        if(m=="sporting-new"){openSportingNew();return;}g_career.sportingStart(m=="sporting-jobs"?-1:user,2026,profile);g_careerActive=true;g_career.season.now=.7;g_career.sportingTick();g_needAdvance=false;g_pending=g_career.season.advance(false);openSporting(0);
        if(getenv("FOOT_SPORTING_PAGE"))g_sdPage=std::clamp(atoi(getenv("FOOT_SPORTING_PAGE")),0,20);if(m=="sporting-jobs")g_sdPage=17;if(m=="sporting-supporters"){g_sdPage=20;screenSporting();if(g_screen!=SC_SUPPORTERS){fprintf(stderr,"FAIL DS supporter navigation\n");exit(10);}IN.back=true;screenSupporters();IN.back=false;screenSporting();if(g_screen!=SC_SPORTING){fprintf(stderr,"FAIL DS supporter return loop\n");exit(10);}}
        if(m=="sporting-match"||m=="sporting-pause"||m=="sporting-half"){auto& M=g_career.season.comps[g_pending.comp].matches[g_pending.match];startSetup(M.home,M.away,true,g_pending.comp,g_pending.match);launchMatch();for(auto side:g_match->S.side)if(side>=0){fprintf(stderr,"FAIL DS human controller\n");exit(9);}if(g_match->S.managed>=0){fprintf(stderr,"FAIL DS managed match\n");exit(9);}if(m=="sporting-pause"){g_paused=true;g_pauseMenu=0;}if(m=="sporting-half"){g_screen=SC_HALFTIME;}}
        return;
    }
    else if(m.rfind("director",0)==0){
        int user=-1;for(auto& P:g_basePyramids)if(P.country=="FRA"&&P.dom<0)for(auto& pool:P.pools)if(pool.tier==(m=="director-amateur"?8:0)&&!pool.clubs.empty()){user=pool.clubs[0];break;}
        if(user<0)user=g_world.firstClub;g_career.opts=Career::Opts();g_career.newClubCareer(user,2026);g_careerActive=true;g_needAdvance=false;g_career.season.now=.7;g_career.cdfAskYear=g_career.year;g_career.mgr.needStatus=0;
        g_career.directorAnalyse(true);openDirector();if(getenv("FOOT_DIRECTOR_PAGE"))g_dsPage=std::clamp(atoi(getenv("FOOT_DIRECTOR_PAGE")),0,9);
        if(m=="director-detail")g_dsDetail=0;
        if(m=="director-negotiation"&&!g_career.director.targets.empty()){for(auto& mode:g_career.director.mode)mode=DM_NEGOTIATE;std::string msg;g_career.directorStart(g_career.director.targets[0].pid,false,msg);if(!g_career.director.negotiations.empty())g_career.director.negotiations[0].phase=DN_APPROVAL;g_dsPage=5;}
        if(m=="director-club")g_screen=SC_CLUBMENU;if(m=="director-staff"){g_stfRole=SR_DIRECTOR;g_screen=SC_STAFF;}
        return;
    }
    else if(m.rfind("museum",0)==0){
        const char* save=getenv("FOOT_MUSEUM_SAVE");
        if(save&&m!="museum-empty")g_career.load(save);else g_career.newClubCareer(g_world.firstClub,2026);
        g_careerActive=true;g_needAdvance=false;openMuseum(g_career.userTeam,SC_MAIN);
        if(getenv("FOOT_MUSEUM_PAGE"))g_muPage=std::clamp(atoi(getenv("FOOT_MUSEUM_PAGE")),0,11);
        if(m=="museum-detail")g_muDetail=0;
        if(getenv("FOOT_MUSEUM_DEMO")){auto& H=g_career.clubHistories[g_career.userTeam];const char* N[]={"Ligue 1","Coupe de France","Ligue des champions","Trophée des champions","Ligue 1","Coupe de France","Ligue Europa","Coupe régionale - Grand Est","District 1 - Marne"};int K[]={0,1,3,1,0,1,8,4,0};bool F[]={false,true,true,true,false,true,true,true,false};for(int i=0;i<9;i++){MuseumTrophy tr;tr.year=2027+i;tr.kind=K[i];tr.finalMatch.final=F[i];snprintf(tr.name,sizeof tr.name,"%s",N[i]);H.trophies.push_back(tr);}}
        if(m=="museum-filter"){g_muPage=10;g_muYear=2028;g_muCategory=MH_TROPHY;g_muComp=1;}
        if(m=="museum-switch"){int old=g_career.userTeam,next=-1;for(auto& pair:g_career.clubHistories)if(pair.first!=old){next=pair.first;break;}if(next>=0){g_career.museumLeave(old);g_career.userTeam=next;g_career.museumState(next);openMuseum(next,SC_MAIN);IN.click=true;IN.mouse=Vector2{40,40};screenMuseum();IN.click=false;g_muPage=8;}}
        return;
    }
    else if (m == "about") g_screen = SC_ABOUT;
    else if (m == "sponsors") g_screen = SC_SPONSORS;
    else if (m == "managers") g_screen = SC_MANAGERS;
    else if (m == "leaguemode") g_screen = SC_LEAGUEMODE;
    else if (m == "intlopt") g_screen = SC_INTL;
    else if (m == "studio" || m == "history" || m == "comparch" || m == "clubend" || m == "seasonnews" || m == "news" || m == "newslist" || m == "seasonart" || m == "seasonart2" || m == "cdlbracket" || m == "finance2") {
        int user = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Paris Saint-Germain") user = i;
        g_career.newClubCareer(user, 2026); g_careerActive = true;
        Season& S = g_career.season;
        if (m == "comparch" || m == "clubend" || m == "seasonnews") { while (true) { auto pm = S.advance(true); if (pm.comp < 0) break; } if (m != "comparch") { g_screen = SC_SEASONEND; seTab = m == "clubend" ? 3 : 1; return; } g_career.endSeason(); g_caSel = -1; g_screen = SC_COMPARCH; return; }
        if (m == "history") { while (true) { auto pm = S.advance(true); if (pm.comp < 0) break; } g_career.endSeason(); g_histTab = 5; g_screen = SC_HISTORY; return; }
        if (m == "seasonart") { showSeasonArticle(); return; }
        if (m == "cdlbracket") { while (true) { auto pm = S.advance(true); if (pm.comp < 0) break; } openCompView(g_career.cdl, -1, -1); g_cvMode = 0; for (int k = 0; k < (int)S.comps[g_career.cdl].stages.size(); k++) if (S.comps[g_career.cdl].stages[k].ties.size() == 8) g_cvStage = k; return; }
        if (m == "seasonart2") { while (true) { auto pm = S.advance(true); if (pm.comp < 0) break; } g_career.endSeason(); showSeasonArticle(); return; }
        if (m == "finance2") { g_screen = SC_FINANCE; return; }
        for (int k = 0; k < 8; k++) { auto pm = S.advance(false); if (pm.comp < 0) break; auto& C = S.comps[pm.comp]; simulateMatch(C.matches[pm.match], &C); S.recordResult(pm.comp, pm.match); g_career.mgrAfterMatch(pm.comp, pm.match); S.checkRound(pm.comp); }
        if (m == "news") { g_screen = SC_NEWS; if (!S.news.empty()) g_newsOpen = (int)S.news.size() - 1; return; }
        if (m == "newslist") { for (int k = 0; k < 30; k++) { auto pm = S.advance(false); if (pm.comp < 0) break; auto& C = S.comps[pm.comp]; simulateMatch(C.matches[pm.match], &C); S.recordResult(pm.comp, pm.match); S.finishRoundOthers(pm.comp, pm.match); S.checkRound(pm.comp); } g_screen = SC_NEWS; g_newsOpen = -1; return; }
        int rm = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Real Madrid") rm = i;
        startSetup(user, rm, false, -1, -1);
        g_setup.studio = true; g_setup.stadium = "Parc des Princes"; g_setup.title = "Ligue des Champions - Phase de groupes";
        g_tv = { "TF SPORT", "Thierry Delorme", "Jean-Michel Lavaud" }; tvCrew();
        g_studioPhase = 0; g_studioT = 0; g_screen = SC_STUDIO;
    } else if (m == "help") g_screen = SC_HELP;
    else if (m == "main") g_screen = SC_MAIN;
    else if (m == "tv" || m == "tvmatch" || m == "tdcmatch" || m == "amateur" || m == "clubmenu" || m == "staff" || m == "reserves" || m == "referees" || m == "article" || m == "post" || m == "controls2" || m == "training") {
        const char* nm = m == "amateur" ? "" : "Paris Saint-Germain";
        int user = -1;
        if (m == "amateur") { for (auto& pl : g_basePyramids[0].pools) if (pl.tier == 9 && !pl.clubs.empty()) { user = pl.clubs[0]; break; } }
        else for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == nm) user = i;
        g_career.newClubCareer(user, 2026); g_careerActive = true;
        Season& S = g_career.season;
        g_pending = S.advance(false); g_needAdvance = false;
        for (int k = 0; k < 4 && m == "tv"; k++) { if (g_pending.comp < 0) break; auto& C = S.comps[g_pending.comp]; if (C.kind == 1) break; simulateMatch(C.matches[g_pending.match], &C); S.recordResult(g_pending.comp, g_pending.match); S.checkRound(g_pending.comp); g_pending = S.advance(false); }
        if (m == "clubmenu") { g_screen = SC_CLUBMENU; return; }
        if (m == "training") { for (int k = 0; k < 6 && g_pending.comp >= 0; k++) { auto& C = S.comps[g_pending.comp]; simulateMatch(C.matches[g_pending.match], &C); S.recordResult(g_pending.comp, g_pending.match); S.checkRound(g_pending.comp); g_pending = S.advance(false); } g_screen = SC_TRAINING; return; }
        if (m == "staff") { g_screen = SC_STAFF; return; }
        if (m == "reserves") { g_screen = SC_RESERVES; return; }
        if (m == "referees") { g_screen = SC_REFEREES; return; }
        if (m == "controls2") { g_screen = SC_CONTROLS; return; }
        if (m == "article") { simulateUserMatch(); g_screen = SC_ARTICLE; return; }
        if (m == "post") { simulateUserMatch(); g_screen = SC_POST; return; }
        const MatchRes& mr = S.comps[g_pending.comp].matches[g_pending.match];
        startSetup(mr.home, mr.away, true, g_pending.comp, g_pending.match);
        for (int i = 0; i < NUM_INPUTS; i++) g_setup.side[i] = -1;
        if (m == "tv") { g_screen = SC_TVINTRO; audioJingle(1); } else launchMatch();
        if (m == "tdcmatch") { fprintf(stderr, "TDC test: comp %s kind %d\n", S.comps[g_pending.comp].name.c_str(), S.comps[g_pending.comp].kind); g_match->ceremony = false; g_match->startPeriod(1); g_match->clock = 89.9f; g_match->state = MS_PLAY; g_match->score[0] = 1; }
    }
    else if(m=="ballondor") {int team=g_world.firstClub;g_career.opts=Career::Opts();g_career.newClubCareer(team,2026);g_careerActive=true;g_career.season.now=ballonDate(2026);g_career.ballonTick();g_pending=PendingMatch();g_needAdvance=true;openBallon();}
    else if(m=="season-awards" || m=="team-plans" || m=="pole-scout" || m=="pole-match" || m=="club-create-bottom"){
        int user=g_world.firstClub;for(const auto& P:g_basePyramids)if(P.country=="FRA"&&P.dom<0){int q=P.poolIndex(8,districtIndex("Marne"));if(q>=0)user=P.pools[q].clubs[0];}
        g_career.opts=Career::Opts();g_career.opts.disableManagerLife=1;g_career.opts.disableBribes=1;g_career.newClubCareer(user,2026);g_careerActive=true;g_needAdvance=false;
        if(m=="team-plans")openTeamPlans();
        else if(m=="club-create-bottom")openClubEditor(-1,false,SC_HUB);
        else if(m=="pole-match"){int ci=g_career.poleCup;g_career.season.controlled={g_career.poleTeams[0]};startSetup(g_career.season.comps[ci].matches[0].home,g_career.season.comps[ci].matches[0].away,true,ci,0);launchMatch();g_match->ceremony=false;g_match->startPeriod(1);fprintf(stderr,"Pole match: halfMinutes=%d clock=%.0f periodEnd=%.0f noET=%d site=%s\n",g_setup.halfMinutes,g_match->clock,g_match->periodEnd,g_setup.noET,g_setup.stadium.c_str());}
        else {
            int pi,qi,gi;g_career.tierOfTeam(user,&pi,&qi,&gi);int league=g_career.pyramids[pi].pools[qi].comps[gi];
            for(int i=0;i<(int)g_career.season.comps.size();i++)if(i!=g_career.poleCup && (m!="season-awards"||i!=league)){auto& c=g_career.season.comps[i];c.done=true;for(auto& st:c.stages)st.finished=true;}
            for(int n=0;n<500&&!g_career.season.finished;n++)g_career.season.advance(true);
            if(m=="pole-scout")g_screen=SC_POLESCOUT;else{openSeasonAwards();if(getenv("FOOT_AWARD_PAGE"))g_awardPage=atoi(getenv("FOOT_AWARD_PAGE"));}
        }
    }
    else if(m=="aboutthanks")g_screen=SC_ABOUT;
    else if(m=="legend1930" || m=="legend2022") {
        int ed=m=="legend1930"?0:21;auto teams=legendTeams(ed);legendStart(g_career,ed,{teams[0]});g_careerActive=true;
        g_career.season.now=legendCalendarTime(ed,0);g_needAdvance=false;g_pending=PendingMatch();g_screen=SC_COMPS;g_compsCat=1;g_compsSub=0;
    }
    else if(m=="marne-cups") {
        int user=-1;for(const auto& P:g_basePyramids)if(P.country=="FRA"&&P.dom<0){int q=P.poolIndex(8,districtIndex("Marne"));if(q>=0)user=P.pools[q].clubs[0];}
        g_career.newClubCareer(user,2026);g_careerActive=true;g_needAdvance=false;g_compsCat=2;g_compsSub=2;g_screen=SC_COMPS;
    }
    else if(m=="abouttech") {g_screen=SC_ABOUT;IN.tab=true;screenAbout();}
    else if(m.rfind("nation-",0)==0) {
        std::string code=getenv("FOOT_COUNTRY") ? getenv("FOOT_COUNTRY") : "GER";
        int team=-1;for(const auto& P:g_basePyramids) if(P.country==code) for(const auto& pl:P.pools) if(pl.tier==0 && !pl.clubs.empty()){team=pl.clubs[0];break;}
        if(team<0)return;
        if(m=="nation-options"){startCareerWith(team,SC_MAIN);return;}
        g_career.opts=Career::Opts();g_career.opts.disableManagerLife=1;g_career.opts.disableBribes=1;g_career.newClubCareer(team,2026);g_careerActive=true;g_needAdvance=false;
        if(m=="nation-end"){while(!g_career.season.finished)g_career.season.advance(true);g_screen=SC_SEASONEND;seTab=0;}
        else if(m=="nation-hub") {openHub();g_needAdvance=false;}
        else {g_screen=SC_COMPS;g_compsCat=m=="nation-cup"?2:1;g_compsSub=0;}
    }
    else if (m == "copt") startCareerWith(g_world.firstClub + 3, SC_MAIN);
    else if (m == "status") { int u = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_career.tierOfTeam(i) < 0 && i >= g_world.firstClub) {} for (auto& pl : g_basePyramids[0].pools) if (pl.tier == 6 && !pl.clubs.empty()) { u = pl.clubs[0]; break; } if (u < 0) u = g_world.firstClub; g_career.newClubCareer(u, 2026); g_careerActive = true; g_needAdvance = true; openHub(); }
    else if (m == "fiche") { int u = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Stade Brestois") u = i; openFiche(u, SC_MAIN); }
    else if (m == "fiche2") { int u = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Stade Rennais") u = i; openFiche(u, SC_MAIN); g_fichePage = 2; }
    else if (m == "editor") openClubEditor(-1, false, SC_MAIN);
    else if (m == "editmenu") g_screen = SC_EDITMENU;
    else if (m == "compedit") { openCompEditor(); if (getenv("FOOT_PYR")) { for (int p = 0; p < (int)g_basePyramids.size(); p++) if (g_basePyramids[p].country == getenv("FOOT_PYR") && g_basePyramids[p].dom < 0 && g_cePyr < 0) g_cePyr = p; g_ceMode = getenv("FOOT_CEMODE") ? atoi(getenv("FOOT_CEMODE")) : 1; g_ceTier = 0; } }
    else if (m == "anthems") { g_screen = SC_ANTHEMS; if (getenv("FOOT_SEL")) g_anSel = g_world.nationIndex(getenv("FOOT_SEL")); }
    else if (m == "custom") { for (int i = 0; i < 12; i++) g_customSel.push_back(i); g_screen = SC_CUSTOM; }
    else if (m == "controls") { g_screen = SC_CONTROLS; if (getenv("FOOT_CTL_ROW")) g_ctlLW.cur = atoi(getenv("FOOT_CTL_ROW")); }
    else if (m == "coeff" || m == "coeffclub" || m == "eurospots") { g_career.newClubCareer(0 + g_world.firstClub, 2026); g_careerActive = true; g_screen = SC_COEFF; g_coeffTab = m == "coeffclub" ? 1 : m == "eurospots" ? 2 : 0; }
    else if (m == "compsreg" || m == "compsdist") { int u = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Stade Briochin") u = i; g_career.newClubCareer(u, 2026); g_careerActive = true; g_compsCat = 2; g_compsSub = m == "compsreg" ? 1 : 2; g_screen = SC_COMPS; }
    else if (m == "options") g_screen = SC_OPTIONS;
    else if (m == "careeropt") { int u = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Stade Lavallois") u = i; startCareerWith(u, SC_MAIN); g_coptLW.cur = 7; }
    else if (m == "coachpick") { g_intlCandidates.clear(); for (int i = 0; i < NUM_NATIONS; i++) if (nationEligible(i)) g_intlCandidates.push_back(i); g_intlSel = { g_world.nationIndex("FRA") }; g_coachModePick = true; g_coachCatPick = 1; openPick(PM_INTL); }
    else if (m == "comps2") { int u = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Stade Lavallois") u = i; g_career.newClubCareer(u, 2026); g_careerActive = true; g_compsCat = 1; g_compsSub = 0; g_screen = SC_COMPS; }
    else if (m == "rules2" || m == "rulesucl" || m == "rulesnl" || m == "rulescdf") {
        int user = -1;
        for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Stade Lavallois") user = i;
        if (m == "rulesnl") { g_career.coach = false; g_career.newInternational(IT_EURO, true, { g_world.nationIndex("FRA") }, 0, {}, 1); g_careerActive = true; openCompView(0); g_cvMode = 4; }
        else {
            g_career.opts = Career::Opts(); if (m == "rulesucl") g_career.opts.euroFormat = 1;
            g_career.newClubCareer(user, 2026); g_careerActive = true;
            int p, q, gg; g_career.tierOfTeam(user, &p, &q, &gg);
            openCompView(m == "rulesucl" ? g_career.ucl : m == "rulescdf" ? (g_career.cdfNational >= 0 ? g_career.cdfNational : g_career.cdfRegional[0]) : g_career.pyramids[p].pools[q].comps[gg]); g_cvMode = 4;
        }
    }
    else if (m == "youthintl") { g_career.coach = false; g_career.newInternational(IT_OLYMPICS, true, { g_world.nationIndex("FRA") }, 0, {}, 0); g_careerActive = true; g_needAdvance = true; openHub(); }
    else if (m == "rules") {
        int user = -1;
        for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Stade Lavallois") user = i;
        g_career.newClubCareer(user, 2026); g_careerActive = true;
        int p, q, gg; g_career.tierOfTeam(user, &p, &q, &gg);
        openCompView(g_career.pyramids[p].pools[q].comps[gg]); g_cvMode = 2;
    }
    else if (m == "table" || m == "cup" || m == "hub2" || m == "squad" || m == "fixtures" || m == "end" || m == "scorers" || m == "compsfr" || m == "compsworld" || m == "table1" || m == "market" || m == "market4" || m == "market5" || m == "finance" || m == "minfo" || m == "groups" || m == "confirm" || m == "fiche3" || m == "stadium" || m == "stadium2" || m == "stadium3" || m == "stadium4" || m == "stadium6" || m == "stadium7" || m == "stadcrash" || m == "cuphub" || m == "nego" || m == "archive" || m == "finance3") {
        int user = -1;
        for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Stade Lavallois") user = i;
        g_career.newClubCareer(user, 2026); g_careerActive = true;
        Season& S = g_career.season;
        for (int k = 0; k < (m == "end" ? 100 : 14); k++) { auto pm = S.advance(false); if (pm.comp < 0) break; simulateMatch(S.comps[pm.comp].matches[pm.match], &S.comps[pm.comp]); S.recordResult(pm.comp, pm.match); S.checkRound(pm.comp); }
        g_needAdvance = true; openHub();
        int p, q, gg; g_career.tierOfTeam(user, &p, &q, &gg);
        if (m == "table") openCompView(g_career.pyramids[p].pools[q].comps[gg]);
        if (m == "cup") { openCompView(g_career.cdf); g_cvStage = 2; }
        if (m == "squad") g_screen = SC_SQUAD;
        if (m == "fixtures") g_screen = SC_FIXTURES;
        if (m == "end") { S.advance(true); g_screen = SC_SEASONEND; }
        if (m == "scorers") { openCompView(g_career.pyramids[p].pools[q].comps[gg]); g_cvMode = 2; }
        if (m == "market" || m == "nego") openMarket();
        if (m == "compsfr") { g_screen = SC_COMPS; g_compsCat = 1; g_compsSub = 0; }
        if (m == "compsworld") { g_screen = SC_COMPS; g_compsCat = 4; }
        if (m == "table1") { int p, q, g; for (int i = 0; i < (int)g_career.pyramids.size(); i++) if (g_career.pyramids[i].country == "FRA" && g_career.pyramids[i].dom < 0) { int qq = g_career.pyramids[i].poolIndex(0, 0); openCompView(g_career.pyramids[i].pools[qq].comps[0], 0, 0); g_cvMode = 0; } (void)p; (void)q; (void)g; }
        if (m == "market4" || m == "market5") { openMarket(); g_mkTab = m == "market4" ? 3 : 4; std::string e; g_career.loanIn(g_world.teams[g_world.nationIndex("FRA")].squad.empty() ? -1 : -1, e); }
        if (m == "nego" && !g_mkList.empty()) { MkRow r = g_mkList[0]; g_nego = Nego(); g_nego.active = true; g_nego.team = r.team; g_nego.pid = r.pid; g_nego.ask = askingPrice(r.team, r.idx); g_nego.fee = g_nego.ask * 8 / 10; g_nego.demand = wageDemand(g_world.teams[r.team].squad[r.idx], g_career.userTeam); g_nego.wage = g_nego.demand; g_nego.last = "Test : refus du club"; }
        if (m == "archive") { while (!S.finished) S.advance(true); g_career.endSeason(); openFiche(user, SC_HUB); g_fichePage = 3; }
        if (m == "finance3") g_screen = SC_FINANCE;
        if (m == "finance") g_screen = SC_FINANCE;
        if (m == "minfo") { auto um = userMatches(true); if (!um.empty()) openMatchInfo(um.back().first, um.back().second, SC_HUB); }
        if (m == "groups") { for (int k = 0; k < 400; k++) { auto pm = S.advance(true); (void)pm; if (S.comps[g_career.ucl].stages.size() >= 4) break; } openCompView(g_career.ucl, 3, 0); g_cvMode = 0; }
        if (m == "stadium") { g_screen = SC_STADIUM; }
        if (m == "stadium2") { g_screen = SC_STADIUM; g_stTab = 1; }
        if (m == "stadium3") { g_screen = SC_STADIUM; g_stTab = 2; }
        if (m == "stadium4") { g_screen = SC_STADIUM; g_stTab = 3; }
        if (m == "stadium6") { g_screen = SC_STADIUM; g_stTab = 5; g_world.teams[user].sta.turf = 18; }
        if (m == "stadium7") { g_screen = SC_STADIUM; g_stTab = 6; }
        if (m == "stadcrash") { g_screen = SC_STADIUM; g_stTab = 0; g_stSel = 8; }
        if (m == "cuphub") { for (int k = 0; k < 3000; k++) { auto pm = S.advance(false); if (pm.comp < 0) break; if (S.comps[pm.comp].format != FMT_LEAGUE) { g_pending = pm; g_needAdvance = false; break; } simulateMatch(S.comps[pm.comp].matches[pm.match], &S.comps[pm.comp]); S.recordResult(pm.comp, pm.match); S.checkRound(pm.comp); } g_screen = SC_HUB; }
        if (m == "confirm") askConfirm("Revenir au menu principal ? (la partie est sauvegardée automatiquement)", []() {});
        if (m == "fiche3") { for (int k = 0; k < 3; k++) { while (true) { auto pm = S.advance(true); if (pm.comp < 0) break; } g_career.endSeason(); } openFiche(g_world.nationIndex("FRA") >= 0 ? g_career.ucl >= 0 && S.comps.size() ? user : user : user, SC_HUB); for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Paris Saint-Germain") openFiche(i, SC_HUB); g_fichePage = 1; }
    }
}
bool appInMatch() { return g_screen == SC_MATCH; }


