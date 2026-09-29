// Interface : menus, sélection d'équipes, carrière, classements, déroulement des matchs
#include "render.h"
#include "input.h"
#include "audio.h"
#include "game.h"
#include "match.h"
#include <memory>
#include <functional>
#include <map>
#include <set>
#include <cstring>
#include <sys/stat.h>


enum Screen { SC_MAIN = 0, SC_PICK, SC_SETUP, SC_MATCH, SC_POST, SC_INTL, SC_HUB, SC_COMPS, SC_COMPVIEW, SC_FIXTURES,
              SC_SQUAD, SC_HISTORY, SC_OPTIONS, SC_SLOTS, SC_SEASONEND, SC_HELP, SC_QUIT, SC_FICHE, SC_EDITMENU, SC_CLUBEDIT,
              SC_CUSTOM, SC_COEFF, SC_CONTROLS, SC_DEPTPICK, SC_MATCHINFO, SC_MARKET, SC_FINANCE, SC_NEWS, SC_JOBS, SC_CAREEROPT, SC_STATUS, SC_STADIUM, SC_CUSTOMLIST, SC_STAFF, SC_RESERVES, SC_FRIENDLIES, SC_REFEREES, SC_CLUBMENU, SC_TVINTRO, SC_ARTICLE, SC_TRAINMODE, SC_TRAINING, SC_DRAW, SC_TROPHIES, SC_STUDIO, SC_ABOUT, SC_SPONSORS, SC_MANAGERS, SC_LEAGUEMODE, SC_HALFTIME, SC_OFFERS, SC_ACADEMY, SC_PLAYER, SC_PLAYEREDIT, SC_TACTICS, SC_EDITDB };
static void openPlayer(int team, int idx, Screen back);

static Screen g_screen = SC_MAIN;
static MenuInput IN;
static Controls CTL[NUM_INPUTS];
static std::unique_ptr<Match> g_match;
static bool g_careerActive = false;
static std::string g_toast; static float g_toastT = 0;
bool g_quit = false;

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

// bouton cliquable ; renvoie vrai si cliqué
static bool button(int x, int y, int w, int h, const std::string& label, bool selected = false, bool enabled = true) {
    bool hover = enabled && IN.mouse.x >= x && IN.mouse.x < x + w && IN.mouse.y >= y && IN.mouse.y < y + h;
    Color bg = !enabled ? Color{ 40, 50, 70, 255 } : selected ? C_SEL : hover ? Color{ 70, 104, 170, 255 } : C_ITEM;
    DrawRectangle(x, y, w, h, bg);
    DrawRectangle(x, y, w, 1, Color{ 255, 255, 255, (unsigned char)(selected ? 110 : 45) });     // biseau clair
    DrawRectangle(x, y + h - 1, w, 1, Color{ 0, 0, 0, 90 });
    if (hover && !selected) DrawRectangleLines(x, y, w, h, Color{ 255, 225, 90, 120 });
    drawTextCentered(fitText(label, w - 4, 10), x + w / 2, y + (h - 10) / 2, 10, selected ? BLACK : enabled ? C_TXT : C_DIM, false);
    return hover && IN.click;
}

static bool g_noBackBtn = false;
static void drawBackground(const std::string& title) {
    ClearBackground(C_BG);
    DrawRectangleGradientV(0, 26, VW, VH - 26, Color{ 24, 40, 76, 255 }, Color{ 12, 22, 44, 255 });
    for (int i = -VH; i < VW; i += 16) DrawLine(i, VH, i + VH, 0, Color{ C_BG2.r, C_BG2.g, C_BG2.b, 150 });
    // filigrane : rond central et surface de réparation
    DrawCircleLines(VW - 70, VH - 60, 46, Color{ 255, 255, 255, 10 });
    DrawCircleLines(VW - 70, VH - 60, 45, Color{ 255, 255, 255, 10 });
    DrawRectangleLines(VW - 200, VH - 150, 1, 180, Color{ 255, 255, 255, 8 });
    DrawRectangleGradientV(0, 0, VW, 26, Color{ 18, 30, 58, 255 }, Color{ 8, 14, 30, 255 });
    DrawRectangle(0, 26, VW, 2, C_SEL);
    // liseré tricolore
    DrawRectangle(0, 28, 40, 1, Color{ 0, 85, 164, 255 }); DrawRectangle(40, 28, 40, 1, WHITE); DrawRectangle(80, 28, 40, 1, Color{ 226, 0, 26, 255 });
    drawTextShadow(fitText(title, VW - 110, 20), 10, 4, 20, C_HI);
    if (g_noBackBtn) {
        DrawRectangle(VW - 88, 6, 80, 14, Color{ 200, 30, 40, 255 });
        drawTextPx("FRANCE FOOT 2D", VW - 86, 8, 10, WHITE);
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
struct Confirm { bool active = false; std::string text, yes, no; std::function<void()> onYes, onNo; int sel = 0; };
static Confirm g_confirm;
static void askConfirm(const std::string& text, std::function<void()> onYes, const std::string& yes = "Oui", const std::string& no = "Non", std::function<void()> onNo = nullptr) {
    g_confirm = Confirm(); g_confirm.active = true; g_confirm.text = text; g_confirm.onYes = onYes; g_confirm.onNo = onNo; g_confirm.yes = yes; g_confirm.no = no; g_confirm.sel = 1;
}

static void drawFooter(const std::string& s) {
    DrawRectangle(0, VH - 14, VW, 14, Color{ 12, 20, 40, 235 });
    DrawRectangle(0, VH - 14, VW, 1, Color{ 240, 200, 60, 90 });
    drawTextPx(s, 6, VH - 12, 10, C_DIM);
}

struct ListW { int cur = 0, top = 0; };
static ListW g_statusLW;
static ListW g_ctlLW; static int g_ctlCapture = -1; static Screen g_ctlBack = SC_OPTIONS;   // configuration des commandes (options ou pause)

// liste générique ; renvoie l'index activé (OK / clic) ou -1
static int listRun(ListW& w, int n, int x, int y, int wd, int rows, int rowH, std::function<void(int, int, int, bool)> row, bool active = true) {
    if (n <= 0) { w.cur = 0; w.top = 0; return -1; }
    int act = -1;
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
    }
    if (w.cur >= n) w.cur = n - 1;
    if ((IN.wheel == 0 || !active) && w.cur >= 0) {
        if (w.cur < w.top) w.top = w.cur;
        if (w.cur >= w.top + rows) w.top = w.cur - rows + 1;
    }
    if (w.top > std::max(0, n - rows)) w.top = std::max(0, n - rows);
    if (w.top < 0) w.top = 0;
    for (int i = w.top; i < n && i < w.top + rows; i++) {
        int yy = y + (i - w.top) * rowH;
        bool sel = i == w.cur;
        DrawRectangle(x, yy, wd, rowH - 1, sel ? C_SEL : ((i % 2) ? C_ITEM : Color{ 38, 62, 108, 255 }));
        if (sel) { DrawRectangle(x, yy, wd, 1, Color{ 255, 255, 255, 120 }); DrawRectangle(x + wd - 3, yy, 3, rowH - 1, Color{ 200, 140, 20, 255 }); }
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
        drawTextCentered(items[i], xx + wd / 2, yy + 5, 10, sel ? Color{ 20, 20, 40, 255 } : C_TXT, false);
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
    if (P.country == "U19" || P.country == "U17") {
        if (tier == 0) return "Nat. " + P.country;
        char d = 0; for (char c : n) if (c >= '0' && c <= '9') d = c;
        return std::string(n.find("gional") != std::string::npos ? "R" : "D") + (d ? std::string(1, d) : std::string()) + " " + P.country;
    }
    return P.country + fmt(" %d", tier + 1);
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

static const char* MONTHS[] = { "Août", "Septembre", "Octobre", "Novembre", "Décembre", "Janvier", "Février", "Mars", "Avril", "Mai", "Juin" };
static std::string dateOf(double t, int year) {
    if (t < 0) return fmt("%s %d", t < -4.3 ? "juin" : "juillet", year);
    int m = std::max(0, std::min(10, (int)(t / 4.2)));
    return fmt("%s %d", MONTHS[m], year + (m >= 5 ? 1 : 0));
}

// ------------------------------------------------------------------ navigation / choix d'équipes
enum NodeKind { K_ROOT, K_NATROOT, K_CONF, K_CLUBROOT, K_PYR, K_TIER, K_TIERREG, K_POOL, K_GROUP, K_EUROPOOL, K_DOMROOT, K_TEAM, K_DONE,
                K_INTLLIST, K_COMP, K_WORLDPOOL, K_CREATE, K_EUROCOUNTRY };
enum PickMode { PM_FRIENDLY_HOME, PM_FRIENDLY_AWAY, PM_CAREER, PM_INTL, PM_BROWSE, PM_CUSTOM, PM_EDIT, PM_FICHE, PM_INVITE, PM_TRAIN };
static int g_trainTeam = -1;
static int g_intlFormat = 0;
static int seTab = 0;   // onglet du bilan de fin de saison
static bool g_leagueModePick = false;   // choix des clubs du mode Championnat
static bool g_euroModePick = false;     // choix des clubs du mode Coupes d'Europe
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
        if (g_pickMode == PM_CUSTOM) { Item d{ fmt(">>> VALIDER (%d équipe%s) <<<", (int)g_customSel.size(), g_customSel.size() > 1 ? "s" : ""), "", K_DONE }; it.push_back(d); }
        it.push_back({ "Sélections nationales", "", K_NATROOT });
        it.push_back({ "Clubs", "", K_CLUBROOT });
        break;
    case K_NATROOT:
        for (int c = 0; c < NUM_CONFEDS; c++) { Item x{ CONF_NAMES[c], "", K_CONF }; x.a = c; int cnt = 0; for (int i = 0; i < NUM_NATIONS; i++) if (NATIONS[i].conf == c) cnt++; x.right = fmt("%d", cnt); it.push_back(x); }
        break;
    case K_CONF: { std::vector<int> v; for (int i = 0; i < NUM_NATIONS; i++) if (NATIONS[i].conf == n.a) v.push_back(i); clubsOf(v); break; }
    case K_INTLLIST: {
        Item d{ fmt(">>> VALIDER (%d sélectionnée%s) <<<", (int)g_intlSel.size(), g_intlSel.size() > 1 ? "s" : ""), "", K_DONE }; it.push_back(d);
        std::vector<int> v = g_intlCandidates; clubsOf(v);
        break;
    }
    case K_CLUBROOT: {
        if (g_pickMode == PM_INVITE) { Item d{ fmt(">>> VALIDER (%d club%s invité%s) <<<", (int)g_inviteSel.size(), g_inviteSel.size() > 1 ? "s" : "", g_inviteSel.size() > 1 ? "s" : ""), "", K_DONE }; it.push_back(d); }
        if (g_pickMode == PM_CAREER) it.push_back({ "+ Créer mon propre club (débute en dernière division de son district)", "", K_CREATE });
        int fr = -1;
        for (int i = 0; i < (int)P.size(); i++) if (P[i].country == "FRA" && P[i].dom < 0) fr = i;
        if (fr >= 0) { Item x{ "France (pyramide complète)", "", K_PYR }; x.a = fr; it.push_back(x); }
        if (g_pickMode != PM_CAREER)   // la carrière est réservée au football français
            for (int i = 0; i < (int)P.size(); i++) if (P[i].country != "FRA") { Item x{ P[i].name, "", K_PYR }; x.a = i; it.push_back(x); }
        it.push_back({ "France - Outre-mer", "", K_DOMROOT });
        if (g_pickMode != PM_CAREER && g_pickMode != PM_BROWSE) {
            it.push_back({ "Autres clubs européens (par pays)", "", K_EUROPOOL });
            it.push_back({ "Autres clubs du monde", "", K_WORLDPOOL });
        }
        if (g_pickMode != PM_BROWSE) {
            std::vector<int> cust; for (int i = g_world.baseCount; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].custom) cust.push_back(i);
            if (!cust.empty()) { Item x{ "Clubs créés avec l'éditeur", fmt("%d", (int)cust.size()), K_COMP }; it.push_back(x); }
        }
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
    else if (m == PM_INTL) { root.kind = K_INTLLIST; root.title = g_euroModePick ? "Coupes d'Europe : choisissez vos clubs (1 à 4)" : g_leagueModePick ? "Choisissez vos clubs (1 à 4)" : "Choisissez vos sélections"; }
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
    int sel = listRun(n.lw, (int)items.size(), 20, 58, VW - 40, 22, 12, [&](int i, int x, int y, bool s) {
        const Item& it = items[i];
        if (it.kind == K_TEAM) {
            bool picked = (g_pickMode == PM_INTL && std::find(g_intlSel.begin(), g_intlSel.end(), it.team) != g_intlSel.end()) ||
                          (g_pickMode == PM_CUSTOM && std::find(g_customSel.begin(), g_customSel.end(), it.team) != g_customSel.end()) ||
                          (g_pickMode == PM_INVITE && std::find(g_inviteSel.begin(), g_inviteSel.end(), it.team) != g_inviteSel.end());
            drawTeamRow(it.team, x, y, VW - 40, s);
            if (picked) { DrawRectangle(x + VW - 40 - 44, y + 2, 16, 8, C_GOOD); drawTextPx("OK", x + VW - 40 - 42, y + 1, 10, BLACK); }
        } else {
            Color c = s ? Color{ 20, 20, 40, 255 } : (it.kind == K_CREATE || it.kind == K_DONE ? C_HI : C_TXT);
            drawTextPx(fitText(it.label, VW - 200, 10), x + 6, y + 1, 10, c);
            if (!it.right.empty()) drawTextPx(it.right, x + VW - 40 - textWidth(it.right, 10) - 16, y + 1, 10, c);
            if (it.kind != K_DONE && it.kind != K_CREATE) drawTextPx(">", x + VW - 40 - 10, y + 1, 10, c);
        }
    });
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
        else if (g_pickMode == PM_INTL) { g_screen = g_euroModePick ? SC_MAIN : g_leagueModePick ? SC_LEAGUEMODE : SC_INTL; g_leagueModePick = false; g_euroModePick = false; }
        else if (g_pickMode == PM_CUSTOM) g_screen = SC_CUSTOM;
        else if (g_pickMode == PM_EDIT) g_screen = SC_EDITMENU;
        else if (g_pickMode == PM_INVITE) g_screen = SC_FRIENDLIES;
        else g_screen = SC_MAIN;
        return;
    }
    if (sel < 0) return;
    const Item& it = items[sel];
    if (it.kind == K_CREATE) { openClubEditor(-1, true, SC_PICK); return; }
    if (it.kind == K_TEAM) {
        switch (g_pickMode) {
        case PM_FRIENDLY_HOME: g_friendlyHome = it.team; g_pickMode = PM_FRIENDLY_AWAY; break;
        case PM_FRIENDLY_AWAY:
            if (it.team == g_friendlyHome) { toast("Choisissez une autre équipe"); break; }
            startSetup(g_friendlyHome, it.team, false, -1, -1);
            break;
        case PM_CAREER: startCareerWith(it.team, SC_PICK); break;
        case PM_INTL: {
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
        if (g_euroModePick) {
            g_euroModePick = false; g_leagueModePick = false;
            std::vector<int> ctrl = g_intlSel; if (ctrl.size() > 4) ctrl.resize(4);
            g_career.newEuroCareer(ctrl, 2026);
            g_careerActive = true; g_needAdvance = true;
            g_hubNotice = "Coupes d'Europe 2026-27 : Ligue des champions et Coupe UEFA";
            openHub();
            return;
        }
        if (g_leagueModePick) { startLeagueMode(); return; }
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
    for (int i = 0; i < NUM_INPUTS; i++) g_setup.side[i] = -1;
    g_mctx = MatchCtx(); g_mctx.career = career; g_mctx.comp = comp; g_mctx.match = match;
    g_mctx.back = career ? SC_HUB : SC_MAIN;
    if (career) {
        Season& S = g_career.season;
        Competition& C = S.comps[comp];
        MatchRes& m = C.matches[match];
        g_setup.decisive = m.decisive;
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
        if (fm) { g_setup.managed = home == g_career.userTeam ? 0 : 1; g_setup.highlights = true; }
        else {
            if (hc) g_setup.side[devs[k++]] = 0;
            if (ac) g_setup.side[devs[k++]] = 1;
        }
        g_setup.delegSubs = g_career.kind == CK_CLUB && g_career.mgr.delegSubs;
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
    if (career) { int sh, sb; bool rl; g_career.sheetRules(comp, sh, sb, rl); g_setup.benchSize = sh - 11; g_setup.maxSubs = rl ? -1 : sb; g_setup.rolling = rl; }
    ensureStadium(home);
    if (g_world.teams[home].sta.namingIncome > 0 && !g_setup.neutral) g_setup.stadium = g_world.teams[home].sta.sponsor;
    matchContext(g_setup, career, comp, match);
    g_setupRow = 0;
    g_screen = SC_SETUP;
}

static const char* PITCH_NAMES[] = { "Normal", "Sec", "Humide", "Boueux", "Gelé", "Aléatoire" };
static const char* DIFF_NAMES[] = { "Facile", "Normal", "Difficile" };

static bool g_trophyChecked = false, g_trophyShown = false, g_podiumMusic = false, g_htShown = false;
static void launchMatch() {
    MatchSetup s = g_setup;
    s.snes = g_settings.controlStyle == 1;
    s.commentary = g_settings.commentary;
    if (s.pitch == 5) s.pitch = g_rng.range(0, 4);
    s.halfSeconds = HALF_MINUTES[g_settings.halfIdx] * 60.f;
    s.halftimeScreen = true;
    g_match.reset(new Match());
    g_match->init(s);
    g_screen = SC_MATCH;
    g_htStudio = g_ftStudio = false;
    g_trophyChecked = false; g_trophyShown = false; g_podiumMusic = false; g_htShown = false;
}

// options des matchs amicaux
static int g_frSev = 0, g_frEnd = 0, g_frBench = 7, g_frSubs = 5, g_frStadium = 0, g_frTime = 4;
static void applyFriendly(bool friendly, const std::vector<std::string>& stadiums) {
    if (!friendly) return;
    static const int SEVV[] = { -1, 35, 60, 80, 95 };
    static const char* TIMEN[] = { "13h00", "15h00", "17h00", "18h00", "20h45", "21h00" };
    g_setup.sevOverride = SEVV[g_frSev];
    g_setup.decisive = g_frEnd != 0; g_setup.noET = g_frEnd == 3; g_setup.goldenGoal = g_frEnd == 2; g_setup.etNoPens = g_frEnd == 1;
    g_setup.benchSize = g_frBench;
    g_setup.rolling = g_frSubs >= 12; g_setup.maxSubs = g_frSubs >= 12 ? -1 : g_frSubs;
    int si = g_frStadium % (int)stadiums.size();
    g_setup.stadium = stadiums[si]; g_setup.neutral = si >= 2;
    g_setup.kickoffTime = TIMEN[g_frTime];
    g_setup.night = g_frTime >= 4;
}

static void screenSetup() {
    drawBackground("Avant-match");
    const Team& H = g_world.teams[g_setup.home]; const Team& A = g_world.teams[g_setup.away];
    drawTextCentered(fitText(g_setup.title, VW - 20, 10), VW / 2, 34, 10, C_DIM);
    // équipes
    drawTextCentered(fitText(H.name, 250, 10), 150, 56, 10, C_TXT);
    drawTextCentered(fitText(A.name, 250, 10), VW - 150, 56, 10, C_TXT);
    drawTextCentered("CONTRE", VW / 2, 56, 10, C_HI);
    drawTextCentered(fmt("Note %d  %s", (int)H.rating, FORMATIONS[g_setup.formation[0] >= 0 ? g_setup.formation[0] : H.formation].name), 150, 68, 10, C_DIM);
    drawTextCentered(fmt("Note %d  %s", (int)A.rating, FORMATIONS[g_setup.formation[1] >= 0 ? g_setup.formation[1] : A.formation].name), VW - 150, 68, 10, C_DIM);
    if (g_setup.hasFirstLeg) drawTextCentered(fmt("Match aller : %s %d - %d %s", A.shortName.c_str(), g_setup.aggAway, g_setup.aggHome, H.shortName.c_str()), VW / 2, 80, 10, C_HI);
    else drawTextCentered(fitText(g_setup.stadium, VW - 40, 10), VW / 2, 80, 10, C_DIM);
    // contrôleurs
    int y = 96;
    drawTextPx("Contrôleurs  (gauche/droite sur chaque manette/clavier)", 20, y, 10, g_setupRow == 0 ? C_HI : C_DIM);
    y += 14;
    std::vector<int> devs;
    for (int d = 0; d < NUM_INPUTS; d++) if (inputAvailable(d)) devs.push_back(d);
    for (int d : devs) {
        DrawRectangle(20, y, VW - 40, 13, Color{ 38, 62, 108, 255 });
        int col = g_setup.side[d] == 0 ? 0 : g_setup.side[d] == 1 ? 2 : 1;
        int bx = 20 + (VW - 40) * col / 3 + 4;
        DrawRectangle(bx, y + 1, (VW - 40) / 3 - 8, 11, g_setup.side[d] < 0 ? Color{ 90, 90, 110, 255 } : C_SEL);
        drawTextPx(inputName(d), bx + 4, y + 2, 10, g_setup.side[d] < 0 ? C_TXT : BLACK);
        y += 14;
        if (g_setupRow == 0) {
            if (IN.devLeft[d]) g_setup.side[d] = g_setup.side[d] == 1 ? -1 : 0;
            if (IN.devRight[d]) g_setup.side[d] = g_setup.side[d] == 0 ? -1 : 1;
        }
    }
    {
        int nh = 0, na = 0; for (int d = 0; d < NUM_INPUTS; d++) { if (inputAvailable(d) && g_setup.side[d] == 0) nh++; if (inputAvailable(d) && g_setup.side[d] == 1) na++; }
        drawTextPx(fmt("DOMICILE (%d joueur%s)", nh, nh > 1 ? "s" : ""), 30, y, 10, C_DIM); drawTextCentered("ORDINATEUR", VW / 2, y, 10, C_DIM, false);
        std::string ea = fmt("EXTÉRIEUR (%d joueur%s)", na, na > 1 ? "s" : ""); drawTextPx(ea, VW - 30 - textWidth(ea, 10), y, 10, C_DIM);
    }
    y += 16;
    // options
    bool humanH = false, humanA = false;
    for (int d = 0; d < NUM_INPUTS; d++) { if (g_setup.side[d] == 0) humanH = true; if (g_setup.side[d] == 1) humanA = true; }
    static const char* KITN[] = { "automatique", "domicile", "extérieur", "troisième" };
    static const char* SEVN[] = { "selon l'arbitre", "clément", "normal", "sévère", "très sévère" };
    static const int SEVV[] = { -1, 35, 60, 80, 95 };
    static const char* ENDN[] = { "match nul possible", "prolongation (nul possible)", "prolongation, but en or, puis TAB", "tirs au but directs", "prolongation puis tirs au but" };
    static const char* TIMEN[] = { "13h00", "15h00", "17h00", "18h00", "20h45", "21h00" };
    bool friendly = !g_mctx.career || (g_mctx.comp >= 0 && g_career.season.comps[g_mctx.comp].kind == 12);
    std::vector<std::string> stadiums = { H.stadium, A.stadium, "Stade de France (Saint-Denis)", "Parc des Princes (Paris)", "Stade Vélodrome (Marseille)", "Wembley (Londres)",
                                          "Santiago Bernabéu (Madrid)", "Maracanã (Rio de Janeiro)", "Terrain municipal" };
    std::vector<std::string> rows = {
        fmt("Durée d'une mi-temps : %d min", HALF_MINUTES[g_settings.halfIdx]),
        fmt("Terrain : %s", PITCH_NAMES[g_setup.pitch]),
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
    }
    rows.push_back(">>> COUP D'ENVOI <<<");
    int nOpt = (int)rows.size();
    int nrows = 1 + nOpt;
    if (IN.up) g_setupRow = (g_setupRow - 1 + nrows) % nrows;
    if (IN.down) g_setupRow = (g_setupRow + 1) % nrows;
    int rh = nOpt > 9 ? 13 : 17;
    // aperçu des tenues choisies
    {
        MatchSetup tmp = g_setup; Kit k2[2]; matchKits(tmp, k2);
        drawKitIcon(k2[0], 36, 50, 3); drawKitIcon(k2[1], VW - 60, 50, 3);
    }
    for (int i = 0; i < nOpt; i++) {
        bool sl = g_setupRow == i + 1;
        DrawRectangle(VW / 2 - 170, y, 340, rh - 2, sl ? C_SEL : C_ITEM);
        drawTextCentered(fitText(rows[i], 334, 10), VW / 2, y + (rh - 12) / 2, 10, sl ? BLACK : C_TXT, false);
        if (IN.click && IN.mouse.x > VW / 2 - 170 && IN.mouse.x < VW / 2 + 170 && IN.mouse.y >= y && IN.mouse.y < y + rh - 2) { g_setupRow = i + 1; if (i == nOpt - 1) { applyFriendly(friendly, stadiums); if (g_setup.tv) { g_screen = SC_TVINTRO; audioJingle(1); } else launchMatch(); return; } }
        y += rh;
    }
    int r = g_setupRow - 1;
    int dl = IN.left ? -1 : IN.right ? 1 : 0;
    if (dl && r >= 0) {
        switch (r) {
        case 0: g_settings.halfIdx = (g_settings.halfIdx + dl + NUM_HALF) % NUM_HALF; break;
        case 1: g_setup.pitch = (g_setup.pitch + dl + 6) % 6; break;
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
            case 8: g_frEnd = (g_frEnd + dl + 5) % 5; break;
            case 9: g_frBench = std::max(1, std::min(12, g_frBench + dl)); break;
            case 10: g_frSubs = std::max(0, std::min(12, g_frSubs + dl)); break;
            case 11: g_frStadium = (g_frStadium + dl + (int)stadiums.size()) % (int)stadiums.size(); break;
            case 12: g_frTime = (g_frTime + dl + 6) % 6; break;
            }
            break;
        }
    }
    (void)SEVV;
    if (((IN.ok || IN.start) && g_setupRow == nOpt) || (IN.start && g_setupRow != nOpt)) { applyFriendly(friendly, stadiums); if (g_setup.tv) { g_screen = SC_TVINTRO; audioJingle(1); } else launchMatch(); return; }
    drawFooter("Entrée : coup d'envoi    Gauche/Droite : modifier    Retour : annuler");
    if (IN.back) g_screen = g_mctx.back;
    (void)humanH; (void)humanA;
}

// ------------------------------------------------------------------ match
static bool g_paused = false;
static int g_pauseMenu = 0; // 0 principal, 1 remplacements
static ListW g_pauseLW, g_subLW;
static int g_subTeam = 0, g_subOut = -1;
static float g_acc = 0;

static void finishMatchToResult();

static void simulateRest(Match& m) {
    // termine le match rapidement en simulant le temps restant
    float remain = std::max(0.f, 90 - m.clock) / 90.f;
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
            m.clock = std::max(m.clock, 90 - 90 * remain * (float)r.f());
            m.giveCard(on[r.range(0, (int)on.size() - 1)], r.chance(0.06f) ? 2 : 1);
        }
    }
    m.score[0] += pois(1.4 * std::exp(d / 22) * remain);
    m.score[1] += pois(1.15 * std::exp(-d / 22) * remain);
    int th = m.score[0] + m.S.aggHome, ta = m.score[1] + m.S.aggAway;
    if (m.S.decisive && th == ta) {
        m.aet = true;
        if (r.chance(0.3f)) { if (r.chance(0.5f)) m.score[0]++; else m.score[1]++; }
        th = m.score[0] + m.S.aggHome; ta = m.score[1] + m.S.aggAway;
        if (th == ta) { m.shootout = true; m.pens[0] = r.range(2, 5); m.pens[1] = r.range(2, 5); if (m.pens[0] == m.pens[1]) m.pens[r.range(0, 1)]++; }
    }
    m.finishMatch();
}

// remise du trophée : finale de coupe (ou supercoupe) gagnée, ou titre de champion acquis lors de la dernière journée
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
    return true;
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

static void screenMatch(float dt) {
    Match& m = *g_match;
    m.S.snes = g_settings.controlStyle == 1;     // style de commandes modifiable depuis la pause
    for (int i = 0; i < NUM_INPUTS; i++) m.ctl[i] = CTL[i];
    bool pausePressed = false;
    for (int i = 0; i < NUM_INPUTS; i++) if (CTL[i].pause) pausePressed = true;
    bool justPaused = false;
    if (!g_paused && pausePressed && !m.finished) { g_paused = true; g_pauseMenu = 0; g_pauseLW = ListW(); pausePressed = false; justPaused = true; }
    if (!g_paused) {
        if (m.state == MS_REPLAY) for (int i = 0; i < NUM_INPUTS; i++) if (CTL[i].f1p || CTL[i].f2p) m.stateT = 99;
        const float step = 1.f / 60.f;
        if (m.S.highlights && !m.hotPhase() && !m.finished) {
            // mode Full Manager : on accélère entre les temps forts
            for (int k = 0; k < 10 && !m.hotPhase(); k++) m.update(step);
            g_acc = 0;
        } else {
            g_acc += std::min(dt, 0.1f);
            while (g_acc >= step) { m.update(step); g_acc -= step; }
        }
        for (int i = 0; i < m.sfxN; i++) audioPlay(m.sfxQueue[i]);
        m.sfxN = 0;
        // hymnes nationaux
        if (m.anthemReq >= 0) { audioAnthem((unsigned)(m.anthemReq == 0 ? m.S.home : m.S.away) * 7919u + 17u); m.anthemReq = -1; }
        else if (m.anthemReq == -2) { audioStopAnthem(); m.anthemReq = -1; }
        // entraînement : pas de public
        bool anth = m.ceremony && (m.cerPhase == 10 || m.cerPhase == 11);
        audioCrowd(!m.S.training, anth ? 0.08f : 0.35f);
    } else audioCrowd(false, 0);
    if (m.finished && !g_trophyChecked && !g_paused) {
        g_trophyChecked = true;
        if (g_mctx.career && g_mctx.comp >= 0) g_career.season.finishRoundOthers(g_mctx.comp, g_mctx.match);   // matchs simultanés de la journée
        int side = -1, kind = 0, style = 0; std::string title;
        if (trophyCheck(m, side, kind, style, title)) { m.startTrophy(side, kind, style, title); g_trophyShown = true; }
        else if (!m.abandoned && qualifyCheck(m, side)) m.startLap(side);     // qualification en coupe : tour d'honneur
    }
    if (m.lapActive) {
        if (IN.start || IN.back) m.lapActive = false;
        renderMatch(m, g_settings.radar);
        return;
    }
    if (m.trophyActive && m.trLift && !g_podiumMusic) { g_podiumMusic = true; audioJingle(4); }     // hymne de la victoire
    // mi-temps : écran récapitulatif, puis plateau TV éventuel, puis reprise
    if (m.htWaiting && !m.htGo && !g_paused) {
        if (!g_htShown) { g_htShown = true; g_screen = SC_HALFTIME; return; }
        if (m.S.studio && !g_htStudio) { g_htStudio = true; g_studioPhase = 1; g_studioT = 0; g_screen = SC_STUDIO; return; }
        m.htGo = true;
    }
    if (m.trophyActive && m.trTotal > 2.5f && IN.start) m.endTrophy();          // Entrée uniquement (la touche de tir ne coupe plus la cérémonie)
    renderMatch(m, g_settings.radar);
    if (m.S.highlights && !m.trophyActive && !g_paused) {
        bool hot = m.hotPhase();
        DrawRectangle(VW - 150, VH - 20, 142, 14, Color{ 0, 0, 0, 150 });
        drawTextPx(hot ? "FULL MANAGER : temps fort" : "FULL MANAGER : >> accéléré", VW - 146, VH - 18, 10, hot ? C_HI : C_DIM);
    }
    if (m.trophyActive) return;
    // plateau TV à la mi-temps et en fin de match
    if (m.S.studio && !g_paused) {

        if (!g_ftStudio && m.finished && m.stateT > 1.2f) { g_ftStudio = true; g_studioPhase = 2; g_studioT = 0; g_screen = SC_STUDIO; return; }
    }
    if (m.finished) {
        if (m.stateT > (g_trophyShown ? 2.5f : 5.0f) || IN.ok || IN.click) { audioCrowd(false, 0); finishMatchToResult(); }
        m.stateT += dt;
        return;
    }
    if (!g_paused) return;
    if (justPaused) { IN.back = IN.ok = IN.up = IN.down = false; }
    // ---- menu pause
    DrawRectangle(0, 0, VW, VH, Color{ 0, 0, 20, 160 });
    if (g_pauseMenu == 0) {
        drawTextCentered("PAUSE", VW / 2, 60, 20, C_HI);
        int ht = m.coachSide(0) ? 0 : m.coachSide(1) ? 1 : -1;
        std::vector<std::string> items = { "Reprendre", "Remplacements", fmt("Radar : %s", g_settings.radar ? "oui" : "non"),
                                           fmt("Son : %s", g_settings.sound ? "oui" : "non"), m.S.training ? "Changer d'exercice" : "Terminer le match (simuler la fin)" };
        if (!g_mctx.career) items.push_back(m.S.training ? "Quitter l'entraînement" : "Quitter le match");
        int mentIdx = -1;
        if (ht >= 0 && !m.S.training) { mentIdx = (int)items.size(); items.push_back(std::string("Mentalité : < ") + mentalityName(m.mentality[ht]) + " >"); }
        if (mentIdx >= 0 && g_pauseLW.cur == mentIdx && (IN.left || IN.right)) {
            m.setMentality(ht, m.mentality[ht] + (IN.left ? -1 : 1), false);
            g_world.teams[ht == 0 ? m.S.home : m.S.away].mentality = m.mentality[ht];
        }
        int ctlIdx = (int)items.size(); items.push_back("Commandes et configuration des touches");
        int s = menuRun(g_pauseLW, items, 100);
        if (s == ctlIdx) { g_ctlBack = SC_MATCH; g_ctlLW = ListW(); g_ctlCapture = -1; g_screen = SC_CONTROLS; return; }
        if (mentIdx >= 0 && s == mentIdx) {
            m.setMentality(ht, (m.mentality[ht] + 1) % 5, false);
            g_world.teams[ht == 0 ? m.S.home : m.S.away].mentality = m.mentality[ht];
            return;
        }
        if (IN.back || pausePressed) { g_paused = false; return; }
        if (s == 0) g_paused = false;
        else if (s == 1) {
            g_pauseMenu = 1; g_subOut = -1; g_subLW = ListW();
            g_subTeam = m.coachSide(0) ? 0 : 1;
        } else if (s == 2) { g_settings.radar = !g_settings.radar; g_settings.save(); }
        else if (s == 3) { g_settings.sound = !g_settings.sound; audioSetEnabled(g_settings.sound); g_settings.save(); }
        else if (s == 4 && m.S.training) { g_paused = false; g_match.reset(); g_screen = SC_TRAINMODE; }
        else if (s == 4) { simulateRest(m); g_paused = false; }
        else if (s == 5) { g_paused = false; g_match.reset(); g_screen = SC_MAIN; }
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
    }
    drawFooter("Gauche/Droite : mentalité    OK : seconde période    (remplacements : menu pause)");
    if (IN.ok || IN.start) { g_screen = SC_MATCH; }
}

// ------------------------------------------------------------------ après-match
static std::string roundNameOf(const Competition& C, int mi);
static std::vector<std::string> wrapText(const std::string& s, int maxw);
static const MatchRes* firstLegOf(const Competition& C, int mi);
static void autosave();
static void openHub();

struct EvLine { int side; int minute; int type; std::string txt; };   // type 0 but, 1 csc, 2 jaune, 3 rouge, 4 blessure
struct PostInfo { int home, away, hg, ag, ph = -1, pa = -1; bool aet = false; std::vector<EvLine> ev; float poss = 50; int shots[2] = { 0, 0 }; std::string title, extra; int comp = -1, match = -1; };
static PostInfo g_post;
#include "app_tv.inc"


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
        S.recordResult(g_mctx.comp, g_mctx.match);
        // condition physique réelle : d'après l'endurance restante en fin de match
        for (int i = 0; i < 22; i++) {
            const MPlayer& p = m.pl[i];
            Team& T = g_world.teams[p.team == 0 ? m.S.home : m.S.away];
            if (p.squad < 0 || p.squad >= (int)T.squad.size() || p.sentOff) continue;
            float drop = std::max(0.f, p.stam0 - p.stamina) * 80.f * (p.gk ? 0.5f : 1.f);
            T.squad[p.squad].cond = (uint8_t)std::max(25, std::min(100, (int)(p.cond0 - drop)));
        }
        g_career.mgrAfterMatch(g_mctx.comp, g_mctx.match);
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

static void screenPost() {
    drawBackground("Résultat");
    const Team& H = g_world.teams[g_post.home]; const Team& A = g_world.teams[g_post.away];
    drawTextCentered(fitText(g_post.title, VW - 20, 10), VW / 2, 34, 10, C_DIM);
    drawKitIcon(H.home, 40, 52, 3); drawKitIcon(A.home, VW - 64, 52, 3);
    drawTextCentered(fitText(H.name, 200, 10), 170, 54, 10, C_TXT);
    drawTextCentered(fitText(A.name, 200, 10), VW - 170, 54, 10, C_TXT);
    drawTextCentered(shortLevel(g_post.home), 170, 66, 10, C_DIM, false);
    drawTextCentered(shortLevel(g_post.away), VW - 170, 66, 10, C_DIM, false);
    drawTextCentered(fmt("%d - %d", g_post.hg, g_post.ag), VW / 2, 52, 20, C_HI);
    int y = 80;
    if (g_post.aet) { drawTextCentered("après prolongation", VW / 2, y, 10, C_DIM); y += 12; }
    if (g_post.ph >= 0) { drawTextCentered(fmt("Tirs au but : %d - %d", g_post.ph, g_post.pa), VW / 2, y, 10, C_HI); y += 12; }
    if (!g_post.extra.empty()) { drawTextCentered(g_post.extra, VW / 2, y, 10, C_GOOD); y += 12; }
    y += 4;
    DrawRectangle(VW / 2, y, 1, 150, Color{ 255, 255, 255, 40 });
    y = drawEventColumns(g_post.ev, y, VH - 60) + 8;
    if (g_post.poss >= 0) {
        drawTextCentered("Possession", VW / 2, y, 10, C_DIM);
        drawTextPx(fmt("%d%%", (int)std::lround(g_post.poss)), 200, y, 10, C_TXT);
        drawTextPx(fmt("%d%%", 100 - (int)std::lround(g_post.poss)), VW - 230, y, 10, C_TXT);
        y += 12;
        drawTextCentered("Tirs", VW / 2, y, 10, C_DIM);
        drawTextPx(fmt("%d", g_post.shots[0]), 200, y, 10, C_TXT);
        drawTextPx(fmt("%d", g_post.shots[1]), VW - 230, y, 10, C_TXT);
    }
    // gros titre de la presse
    DrawRectangle(20, VH - 36, VW - 40, 18, Color{ 238, 232, 214, 255 });
    drawTextPx(fitText("PRESSE : " + g_articleTitle, VW - 60, 10), 28, VH - 32, 10, Color{ 30, 30, 30, 255 });
    drawFooter("OK : continuer   Tab : lire l'article");
    if (IN.tab) { g_screen = SC_ARTICLE; return; }
    if (IN.ok || IN.back || IN.click || IN.start) { if (g_mctx.career) { autosave(); openHub(); } else g_screen = SC_MAIN; }
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
static void openHub() { g_screen = SC_HUB; g_hubLW = ListW(); levelCacheDirty(); }

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
static int g_hubSel = 0;

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

static void screenHub() {
    Season& S = g_career.season;
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
    if (g_needAdvance && !g_confirm.active) {
        g_pending = S.advance(false);
        g_needAdvance = false;
        g_career.mgrTick();
        if (collectDraws()) { g_screen = SC_DRAW; return; }
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
        drawTextPx(euro ? "Ligue des champions et Coupe UEFA  -  " + dateOf(S.now, g_career.year) : g_career.kind == CK_CUSTOM ? "Compétition personnalisée" : g_career.finalComp >= 0 ? "Phase finale" : "Qualifications", 12, y + 11, 10, C_DIM);
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
        drawTextPx(fitText("PROCHAIN MATCH  -  " + C.name + (rn.empty() ? "" : " - " + rn) + "  -  " + dateOf(tm, g_career.year), VW - 40, 10), 14, y + 3, 10, C_DIM);
        for (int side = 0; side < 2; side++) {
            int t = side ? m.away : m.home;
            int bx = side ? VW / 2 + 24 : 14, bw = VW / 2 - 40;
            bool hover = IN.mouse.x >= bx && IN.mouse.x < bx + bw && IN.mouse.y >= y + 15 && IN.mouse.y < y + 44;
            if (hover) DrawRectangle(bx, y + 15, bw, 29, Color{ 60, 90, 150, 120 });
            drawKitIcon(g_world.teams[t].home, side ? VW - 36 : bx + 2, y + 18, 2);
            int tx = side ? bx + 4 : bx + 24;
            drawTextPx(fitText(g_world.teams[t].name, bw - 30, 10), tx, y + 18, 10, S.isControlled(t) ? C_HI : C_TXT);
            std::string lv = shortLevel(t);
            if (!lv.empty()) drawTextPx(lv, tx, y + 30, 10, C_DIM);
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
    if (g_pending.comp >= 0) { m1.push_back({ "Jouer le match", 0 }); m1.push_back({ "Simuler le match", 1 }); }
    else if (S.finished) m1.push_back({ club ? "Bilan de fin de saison" : "Bilan du tournoi", 2 });
    if (!m1.empty()) secs.push_back({ "MATCH", m1 });
    if (club) secs.push_back({ "COMPÉTITIONS", { { "Classements, résultats, buteurs", 3 }, { "Calendrier de l'équipe", 4 }, { "Palmarès", 6 }, { "Coefficients UEFA", 10 }, { "Arbitres", 19 } } });
    else if (euro) secs.push_back({ "COMPÉTITIONS", { { "Classements, résultats, buteurs", 3 }, { "Calendrier de l'équipe", 4 }, { "Palmarès", 6 }, { "Coefficients UEFA", 10 } } });
    else secs.push_back({ "COMPÉTITIONS", { { "Classements, résultats, buteurs", 3 }, { "Calendrier de l'équipe", 4 }, { "Palmarès", 6 } } });
    if (club) secs.push_back({ "CLUB", { { "Effectif, tactique et contrats", 5 }, { "Gestion du club", 20 }, { fmt("Messages (%d non lu%s)", newsUnread(), newsUnread() > 1 ? "s" : ""), 14 } } });
    else secs.push_back({ "ÉQUIPE", { { "Effectif et tactique", 5 }, { "Fiche de l'équipe", 9 } } });
    secs.push_back({ "PARTIE", { { "Sauvegarder", 7 }, { "Menu principal", 8 } } });
    std::vector<HI> flat;
    for (auto& sc : secs) for (auto& it : sc.second) flat.push_back(it);
    int n = (int)flat.size();
    if (g_hubSel >= n) g_hubSel = 0;
    if (IN.up) g_hubSel = (g_hubSel - 1 + n) % n;
    if (IN.down) g_hubSel = (g_hubSel + 1) % n;
    int act = -1, k = 0, my = y;
    const int mw = 230, rh = 12;
    for (auto& sc : secs) {
        drawSection(8, my, mw, sc.first); my += 11;
        for (auto& it : sc.second) {
            bool hover = IN.mouse.x >= 8 && IN.mouse.x < 8 + mw && IN.mouse.y >= my && IN.mouse.y < my + rh;
            if (hover && IN.mouseMoved) g_hubSel = k;
            bool sel = k == g_hubSel;
            DrawRectangle(8, my, mw, rh - 1, sel ? C_SEL : C_ITEM);
            if (sel) DrawTriangle(Vector2{ 12, (float)my + 2 }, Vector2{ 12, (float)my + 9 }, Vector2{ 16, (float)my + 5.5f }, BLACK);
            drawTextPx(it.label, 20, my + 1, 10, sel ? BLACK : C_TXT);
            if (hover && IN.click) act = k;
            my += rh; k++;
        }
        my += 2;
    }
    if (IN.ok) act = g_hubSel;
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
    drawFooter("OK : valider   Clic sur une équipe ou un résultat : détails   Retour : menu principal");
    if (IN.back) { act = -1; askConfirm("Revenir au menu principal ? (la partie est sauvegardée automatiquement)", []() { autosave(); g_screen = SC_MAIN; }); return; }
    if (act < 0) return;
    switch (flat[act].id) {
    case 0: {
        const MatchRes& m = S.comps[g_pending.comp].matches[g_pending.match];
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
    }
}

// ------------------------------------------------------------------ fin de saison
static void screenSeasonEnd() {
    Season& S = g_career.season;
    if (g_career.kind == 0 && g_career.euroOnly) {
        drawBackground("Fin de saison européenne");
        int y = 40;
        for (int k = 0; k < 2; k++) {
            int c = k == 0 ? g_career.ucl : g_career.uel;
            if (c < 0 || c >= (int)S.comps.size()) continue;
            const Competition& C = S.comps[c];
            int x = k == 0 ? VW / 4 : 3 * VW / 4;
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
            for (int c : { g_career.ucl, g_career.uel }) {
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
    drawBackground(club ? "Fin de saison" : "Fin du tournoi");
    int y = 36;
    if (club) {
        int p, q, g;
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
        if (button(204, y - 2, 180, 14, "Qualifiés pour l'Europe", seTab == 2)) seTab = 2;
        if (button(388, y - 2, 180, 14, "Votre saison", seTab == 1)) seTab = 1;
        if (IN.tab) seTab = seTab == 0 ? 2 : seTab == 2 ? 1 : 0;
        y += 16;
        if (seTab == 2) {
            // clubs qualifiés pour les coupes d'Europe de la saison prochaine (France en priorité)
            static int pvYear = -1; static EuroSpots pv;
            if (pvYear != g_career.year) { pv = g_career.previewEuro(); pvYear = g_career.year; }
            struct Cat { const char* name; const std::vector<int>* v; };
            Cat cats[] = { { "Ligue des champions - phase de groupes", &pv.uclGS }, { "Ligue des champions - 3e tour de qualification", &pv.uclQ3 },
                           { "Ligue des champions - 2e tour de qualification", &pv.uclQ2 }, { "Ligue des champions - 1er tour de qualification", &pv.uclQ1 },
                           { "Coupe UEFA - 1er tour", &pv.uefaR1 }, { "Coupe UEFA - tour de qualification", &pv.uefaQR },
                           { "Coupe Intertoto - 3e tour", &pv.itR3 }, { "Coupe Intertoto - 2e tour", &pv.itR2 }, { "Coupe Intertoto - 1er tour", &pv.itR1 } };
            int yl = y;
            drawSection(14, yl, VW - 28, "FRANCE"); yl += 13;
            for (auto& c : cats) {
                std::string names;
                for (int t : *c.v) if (g_world.teams[t].nation >= 0 && std::string(NATIONS[g_world.teams[t].nation].code) == "FRA") names += (names.empty() ? "" : ", ") + g_world.teams[t].name;
                if (names.empty()) continue;
                drawTextPx(c.name, 20, yl, 10, C_DIM); yl += 11;
                for (auto& l : wrapText(names, VW - 60)) { drawTextPx(l, 34, yl, 10, names.find(g_world.teams[u].name) != std::string::npos && l.find(g_world.teams[u].name) != std::string::npos ? C_GOOD : C_TXT); yl += 11; }
            }
            yl += 4;
            drawSection(14, yl, VW - 28, "EUROPE - TÊTES D'AFFICHE (phase de groupes)"); yl += 13;
            std::string all;
            for (int t : pv.uclGS) if (!(g_world.teams[t].nation >= 0 && std::string(NATIONS[g_world.teams[t].nation].code) == "FRA")) all += (all.empty() ? "" : ", ") + g_world.teams[t].shortName;
            for (auto& l : wrapText(all, VW - 60)) { if (yl > VH - 46) break; drawTextPx(l, 20, yl, 10, C_TXT); yl += 11; }
        } else
        if (seTab == 0) {
            int frp = -1; for (int i = 0; i < (int)g_career.pyramids.size(); i++) if (g_career.pyramids[i].country == "FRA" && g_career.pyramids[i].dom < 0) frp = i;
            // colonne gauche : championnats de France
            int yl = y;
            drawSection(14, yl, 300, "FRANCE - CHAMPIONS"); yl += 13;
            if (frp >= 0) {
                const Pyramid& P = g_career.pyramids[frp];
                for (int tier = 0; tier <= 4; tier++)
                    for (auto& pl : P.pools) if (pl.tier == tier) for (int k = 0; k < (int)pl.comps.size(); k++) {
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
            drawSection(324, yr, 302, "FRANCE - COUPES"); yr += 13;
            cupLine("Coupe de France", g_career.cdf, true);
            cupLine("Coupe de la Ligue", g_career.cdl, true);
            for (int c : g_career.superCups) if (S.comps[c].tag == frp) cupLine(S.comps[c].shortName, c, true);
            cupLine("Supercoupe régions", g_career.superRegions, true);
            yr += 3;
            drawSection(324, yr, 302, "EUROPE"); yr += 13;
            cupLine("Ligue des champions", g_career.ucl, true);
            cupLine("Coupe UEFA", g_career.uel, true);
            cupLine("Supercoupe de l'UEFA", g_career.uefaSuper, false);
            if (g_career.intertoto >= 0 && g_career.intertoto < (int)S.comps.size() && S.comps[g_career.intertoto].done) {
                std::string w; for (int t : S.comps[g_career.intertoto].result) w += (w.empty() ? "" : ", ") + g_world.teams[t].shortName;
                drawTextPx("Intertoto", 344, yr, 10, C_DIM); drawTextPx(fitText(w, 170, 10), 466, yr, 10, C_TXT); yr += 11;
            }
            yr += 3;
            drawSection(324, yr, 302, "ÉTRANGER - CHAMPIONS"); yr += 13;
            for (int pi = 0; pi < (int)g_career.pyramids.size(); pi++) {
                const Pyramid& P = g_career.pyramids[pi];
                if (P.dom >= 0 || pi == frp) continue;
                int q0 = P.poolIndex(0, 0);
                if (q0 < 0 || P.pools[q0].comps.empty()) continue;
                if (yr > VH - 46) break;
                cupLine(P.tiers[0].name, P.pools[q0].comps[0], false);
            }
        } else {
            int yy = y;
            for (auto& n : S.news) {
                for (auto& l : wrapText(n, VW - 40)) { if (yy > VH - 28) break; drawTextPx(l, 20, yy, 10, C_GOOD); yy += 11; }
                if (yy > VH - 28) break;
            }
        }
        drawFooter("Tab : onglet   OK : saison suivante");
        bool nextBtn = button(VW - 170, VH - 32, 150, 14, "Saison suivante >>", false);
        if (IN.ok || IN.start || nextBtn) {
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

static int g_compsCat = 0;
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
        static const char* CATS[] = { "Mes compét.", "France", "Coupes FR", "Europe", "Étranger", "Régional", "District", "Jeunes", "Parcourir" };
        const int NC = 9;
        int bw = (VW - 20) / NC;
        for (int i = 0; i < NC; i++) if (button(10 + i * bw, y, bw - 2, 16, CATS[i], g_compsCat == i)) { g_compsCat = i; g_compsLW = ListW(); }
        if (IN.pgUp) { g_compsCat = (g_compsCat + NC - 1) % NC; g_compsLW = ListW(); }
        if (IN.pgDn) { g_compsCat = (g_compsCat + 1) % NC; g_compsLW = ListW(); }
        y += 20;
        std::vector<int> v;
        int frp = -1; for (int i = 0; i < (int)g_career.pyramids.size(); i++) if (g_career.pyramids[i].country == "FRA" && g_career.pyramids[i].dom < 0) frp = i;
        switch (g_compsCat) {
        case 0: for (int c : mine) v.push_back(c); break;
        case 1:
            if (frp >= 0) for (auto& pl : g_career.pyramids[frp].pools) if (pl.tier <= 4) for (int c : pl.comps) v.push_back(c);
            { int p, q, g; if (g_career.tierOfTeam(g_career.userTeam, &p, &q, &g) > 4) v.push_back(g_career.pyramids[p].pools[q].comps[g]); }
            break;
        case 2:
            if (g_career.cdfNational >= 0) v.push_back(g_career.cdfNational);
            if (g_career.cdl >= 0) v.push_back(g_career.cdl);
            if (g_career.superRegions >= 0) v.push_back(g_career.superRegions);
            for (int c : g_career.superCups) if (S.comps[c].tag == frp) v.push_back(c);
            for (int c = 0; c < (int)S.comps.size(); c++) if (S.comps[c].kind == 10 && S.comps[c].tag / 100 == frp) v.push_back(c);
            for (int c : g_career.cdfRegional) v.push_back(c);
            break;
        case 3: v = { g_career.ucl, g_career.uel, g_career.intertoto, g_career.uefaSuper, g_career.youthPrelim, g_career.youthUcl }; break;
        case 4:
            for (int p = 0; p < (int)g_career.pyramids.size(); p++) {
                const Pyramid& P = g_career.pyramids[p];
                if (P.country == "FRA" || P.country == "U19" || P.country == "U17") continue;
                for (auto& pl : P.pools) if (pl.tier <= 1) for (int c : pl.comps) v.push_back(c);
            }
            for (int c : g_career.nationalCups) v.push_back(c);
            for (int c : g_career.superCups) if (S.comps[c].tag != frp) v.push_back(c);
            for (int c = 0; c < (int)S.comps.size(); c++) if (S.comps[c].kind == 10 && S.comps[c].tag / 100 != frp) v.push_back(c);
            break;
        case 5: case 6: {
            // sélecteur de ligue régionale / district (par défaut : celui du club)
            const int NMR = 13;
            int ur = g_career.userTeam >= 0 ? g_world.teams[g_career.userTeam].region : -1;
            int ud = g_career.userTeam >= 0 ? g_world.teams[g_career.userTeam].district : -1;
            std::vector<int> depts;     // districts, classés par ligue puis par nom
            for (int d = 0; d < numDistricts(); d++) if (districtRegion(d) >= 0 && districtRegion(d) < NMR) depts.push_back(d);
            std::stable_sort(depts.begin(), depts.end(), [](int a, int b) { if (districtRegion(a) != districtRegion(b)) return districtRegion(a) < districtRegion(b); return sortKey(districtName(a)) < sortKey(districtName(b)); });
            if (g_compsRegion < 0) g_compsRegion = ur >= 0 && ur < NMR ? ur : 0;
            if (g_compsDept < 0 || g_compsDept >= numDistricts()) g_compsDept = ud >= 0 ? ud : depts[0];
            bool reg = g_compsCat == 5;
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
            if (frp >= 0 && (!reg || curI < NMR)) {
                const Pyramid& P = g_career.pyramids[frp];
                for (auto& pl : P.pools) {
                    int sc = P.tiers[pl.tier].scope;
                    if (reg && sc == SC_REGION && pl.key == curI) for (int c : pl.comps) v.push_back(c);
                    if (!reg && sc == SC_DEPT && pl.key == g_compsDept) for (int c : pl.comps) v.push_back(c);
                }
            }
            if (reg && curI == NMR) {
                for (auto& P : g_career.pyramids) if (P.dom >= 0) for (auto& pl : P.pools) for (int c : pl.comps) v.push_back(c);
                for (int c : g_career.cdfRegional) if (S.comps[c].tag >= 100) v.push_back(c);
            }
            if (reg) {
                for (int c : g_career.regionalCups) if (S.comps[c].tag == curI) v.push_back(c);
                for (int c : g_career.cdfRegional) if (S.comps[c].tag == curI) v.push_back(c);
                if (g_career.superRegions >= 0) v.push_back(g_career.superRegions);
            } else for (int c : g_career.deptCups) if (S.comps[c].tag == g_compsDept) v.push_back(c);
            break;
        }
        }
        if (g_compsCat == 7) {
            // jeunes (U19 / U17) : national, puis chaque ligue (régional + districts de la ligue) avec ses coupes
            static int ageSel = 0;
            if (button(12, y, 40, 14, "U19", ageSel == 0)) { ageSel = 0; g_compsLW = ListW(); }
            if (button(56, y, 40, 14, "U17", ageSel == 1)) { ageSel = 1; g_compsLW = ListW(); }
            int yp = ageSel == 1 ? g_career.u17Pyramid() : g_career.u19Pyramid();
            static int u19Sel = 0;
            const int NMR = 13;
            int nOpt = NMR + 1;
            if (u19Sel == 0 && g_compsLW.cur == 0 && g_career.userTeam >= 0) {}
            int nw = 300, nx = VW / 2 - nw / 2;
            if (button(nx - 26, y, 22, 14, "<", false) || IN.left) u19Sel = (u19Sel + nOpt - 1) % nOpt, g_compsLW = ListW();
            if (button(nx + nw + 4, y, 22, 14, ">", false) || IN.right) u19Sel = (u19Sel + 1) % nOpt, g_compsLW = ListW();
            DrawRectangle(nx, y, nw, 14, C_ITEM);
            drawTextCentered(u19Sel == 0 ? std::string(ageSel == 1 ? "National (National U17)" : "National (National U19, Gambardella, Ligue des champions U19)") : "Ligue " + sanitize(REGIONS[u19Sel - 1].name), VW / 2, y + 2, 10, C_HI, false);
            y += 18;
            if (yp >= 0) {
                const Pyramid& Y = g_career.pyramids[yp];
                if (u19Sel == 0) {
                    for (auto& pl : Y.pools) if (pl.tier == 0) for (int c : pl.comps) v.push_back(c);
                    if (ageSel == 1) { if (g_career.u17Final >= 0) v.push_back(g_career.u17Final); }
                    else {
                        if (g_career.u19Final >= 0) v.push_back(g_career.u19Final);
                        if (g_career.gambNational >= 0) v.push_back(g_career.gambNational);
                        if (g_career.youthPrelim >= 0) v.push_back(g_career.youthPrelim);
                        if (g_career.youthUcl >= 0) v.push_back(g_career.youthUcl);
                    }
                } else {
                    int rg = u19Sel - 1;
                    for (int tier = 1; tier < (int)Y.tiers.size(); tier++)
                        for (auto& pl : Y.pools) {
                            if (pl.tier != tier) continue;
                            bool ok = Y.tiers[tier].scope == SC_REGION ? pl.key == rg : districtRegion(pl.key) == rg;
                            if (ok) for (int c : pl.comps) v.push_back(c);
                        }
                    if (ageSel == 0) for (int c : g_career.gambRegional) if (S.comps[c].tag == rg) v.push_back(c);
                    if (ageSel == 0) for (int c : g_career.u19Cups) {
                        const Competition& C = S.comps[c];
                        if ((C.kind == 17 && C.tag == rg) || (C.kind == 18 && districtRegion(C.tag) == rg)) v.push_back(c);
                    }
                }
            }
        }
        if (g_compsCat == 8) items.push_back({ "Parcourir tous les championnats (France : national, régional, départemental... et étranger)", -1, 1 });
        for (int c : v) if (c >= 0) items.push_back({ (mine.count(c) ? "* " : "") + S.comps[c].name, c, 0 });
    } else {
        for (int c : mine) items.push_back({ "* " + S.comps[c].name, c, 0 });
        for (int c = 0; c < (int)S.comps.size(); c++) if (!mine.count(c)) items.push_back({ S.comps[c].name, c, 0 });
    }
    int rows = (VH - 18 - y) / 13;
    int s = listRun(g_compsLW, (int)items.size(), 20, y, VW - 40, rows, 13, [&](int i, int x, int yy, bool sel) {
        drawTextPx(fitText(items[i].label, VW - 140, 10), x + 6, yy + 1, 10, sel ? BLACK : (items[i].label[0] == '*' ? C_HI : C_TXT));
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

static std::vector<std::string> wrapText(const std::string& s, int maxw) {
    std::vector<std::string> out;
    std::string para;
    auto flush = [&](const std::string& p) {
        std::string line, word;
        for (size_t i = 0; i <= p.size(); i++) {
            char ch = i < p.size() ? p[i] : ' ';
            if (ch == ' ') {
                std::string cand = line.empty() ? word : line + " " + word;
                if (textWidth(cand, 10) > maxw && !line.empty()) { out.push_back(line); line = word; }
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
    for (int s = 0; s < (int)C.stages.size(); s++) if (C.stages[s].type == ST_KO && !C.stages[s].ties.empty()) ks.push_back(s);
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
        std::string rules = g_career.divisionRules(g_cv);
        if (rules.empty()) rules = "Aucune règle particulière.";
        for (auto& l : wrapText(rules, VW - 40)) { drawTextPx(l, 20, y + 4, 10, C_TXT); y += 13; }
        drawFooter(help + "   Retour");
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
    int upN = 0, downN = 0, barN = 0; bool terminal = false;
    if (C.kind == 1 && C.tag >= 0 && g_career.kind == CK_CLUB) {
        int p = C.tag / 100000, q = (C.tag / 100) % 1000;
        if (p < (int)g_career.pyramids.size() && q < (int)g_career.pyramids[p].pools.size()) {
            const Pool& pl = g_career.pyramids[p].pools[q];
            const TierConf& T = g_career.pyramids[p].tiers[pl.tier];
            upN = pl.tier > 0 ? T.up : 0; downN = T.flexible || pl.terminal ? 0 : T.down; barN = T.barrageUp ? (T.barrageUp == 2 ? 4 : T.barrageUp == 3 ? 3 : 1) : 0;
            terminal = pl.terminal;
        }
    }
    // ---- onglets des groupes
    int ng = (int)st.groups.size();
    if (league && ng > 1) {
        int bw = std::max(22, std::min(60, (VW - 20) / ng));
        int perRow = (VW - 20) / bw;
        for (int g = 0; g < ng; g++) {
            int bx = 10 + (g % perRow) * bw, by = y + (g / perRow) * 15;
            bool mine = false; for (int t : st.groups[g]) if (S.isControlled(t)) mine = true;
            std::string lb = ng <= 26 ? fmt("%s%c", bw >= 50 ? "Gr. " : "", 'A' + g) : fmt("%d", g + 1);
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
            if (i < upN) zone = C_GOOD;
            else if (barN && i >= upN && i < upN + barN && C.kind == 1) zone = Color{ 255, 170, 60, 255 };
            if (downN && i >= n - downN) zone = C_BAD;
            if (C.kind != 1 && i < C.groupsAdvance) zone = C_GOOD;
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
            if (upN) info += "Vert : montée   ";
            if (barN) info += "Orange : barrages   ";
            if (downN) info += "Rouge : descente";
            if (terminal) info += "Dernier niveau (pas de descente)";
        } else info = "Vert : qualifiés";
        drawTextPx(info, 12, VH - 27, 10, C_DIM);
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
        int sel = listRun(g_cvLW, (int)ms.size(), 12, y, VW - 24, rows, 12, [&](int i, int x, int yy, bool sel2) {
            const MatchRes& m = C.matches[ms[i]];
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
    if (g_career.kind == CK_CLUB) {
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
    if (button(34, 58, 150, 13, std::string("Mentalité : ") + mentalityName(T.mentality))) T.mentality = (T.mentality + 1) % 5;
    {
        int cur = g_squadLW.cur;
        auto nameOf = [&](int pid) { for (auto& q : T.squad) if (q.id == pid) return q.name; return std::string("auto"); };
        if (button(188, 58, 200, 13, "Capitaine : " + fitText(nameOf(T.captainPid), 120, 10)) && cur >= 0 && cur < (int)T.squad.size()) {
            T.captainPid = T.squad[cur].id; if (T.vicePid == T.captainPid) T.vicePid = 0; toast(T.squad[cur].name + " est le capitaine");
        }
        if (button(392, 58, 230, 13, "Vice-capitaine : " + fitText(nameOf(T.vicePid), 120, 10)) && cur >= 0 && cur < (int)T.squad.size()) {
            T.vicePid = T.squad[cur].id; if (T.captainPid == T.vicePid) T.captainPid = 0; toast(T.squad[cur].name + " est vice-capitaine");
        }
    }
    {
        int cur = g_squadLW.cur;
        if (g_career.kind == CK_CLUB && ti == g_career.userTeam && button(290, VH - 32, 140, 13, "Consignes / adjoint")) { g_screen = SC_TACTICS; return; }
        if (cur >= 0 && cur < (int)T.squad.size() && (button(12, VH - 32, 150, 13, "Fiche de " + fitText(T.squad[cur].name, 90, 10)) || IsKeyPressed(KEY_F))) { openPlayer(ti, cur, SC_SQUAD); return; }
    }
    if (IN.tab) T.formation = (T.formation + 1) % NUM_FORMATIONS;
    if (teams.size() > 1 && (IN.pgUp || IN.pgDn)) { g_squadTeamIdx = (g_squadTeamIdx + (IN.pgUp ? -1 : 1) + (int)teams.size()) % (int)teams.size(); return; }
    auto lu = g_world.pickLineup(ti, T.formation);
    std::set<int> starters(lu.begin(), lu.begin() + 11);
    int y = 76;
    const char* cols[] = { "ÂGE", "VIT", "TIR", "PAS", "TAC", "DRI", "TÊT", "GAR", "NOTE", "COND", "MOR", "B", "PD", "CONTRAT" };
    int cx[] = { 214, 238, 262, 286, 310, 334, 358, 382, 408, 440, 474, 504, 522, 546 };
    for (int k = 0; k < 14; k++) drawTextPx(cols[k], cx[k], y, 10, C_DIM);
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
        drawTextPx(p.morale >= 80 ? "++" : p.morale >= 62 ? "+" : p.morale >= 45 ? "=" : p.morale >= 30 ? "-" : "--", cx[10] - 6 + x, yy + 1, 10, s2 ? BLACK : mc);
        drawTextPx(fmt("%d", p.goals), cx[11] - 12 + x + 4, yy + 1, 10, c);
        drawTextPx(fmt("%d", p.assists), cx[12] - 12 + x + 4, yy + 1, 10, c);
        static const char* CT[] = { "Pro", "Semi", "Ama" };
        drawTextPx(fitText(std::string(CT[p.contract % 3]) + (p.contract < 2 ? " " + money(playerWage(p)) : ""), 84, 10), cx[13] - 12 + x + 4, yy + 1, 10, c);
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
    switch (type) { case IT_EURO: conf = UEFA; break; case IT_CAN: conf = CAF; break; case IT_ASIA: conf = AFC; break;
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
    if (g_hostChoice == 0) { for (int h : defaultHosts(type)) hostTxt += (hostTxt.empty() ? "" : " / ") + g_world.teams[h].name; hostTxt += " (officiel)"; }
    else hostTxt = g_world.teams[hosts[g_hostChoice - 1]].name;
    static int bigFmt = 0;
    bool canBig = type == IT_WORLDCUP || type == IT_EURO;
    if (!canBig) bigFmt = 0;
    std::string fmtTxt = type == IT_WORLDCUP ? (bigFmt ? "48 équipes, format USA 2026 (12 groupes, 16es)" : "32 équipes (8 groupes, 8es de finale)") :
                         type == IT_EURO ? (bigFmt ? "24 équipes (6 groupes + 4 meilleurs 3es, 8es)" : "16 équipes (4 groupes, quarts)") :
                         type == IT_CAN || type == IT_ASIA ? "24 équipes" : type == IT_OFC ? "8 équipes" : "16 équipes";
    if (canBig) fmtTxt += "  < >";
    items.push_back(fmt("Compétition : %s  (%d)", INTL_NAMES[type], intlYear(type)));
    items.push_back("Format de la phase finale : " + fmtTxt);
    items.push_back(fmt("Qualifications : %s", g_intlQual && type != IT_COPA ? "OUI (toutes les éliminatoires)" : "NON (phase finale directe)"));
    items.push_back("Pays organisateur : " + hostTxt);
    items.push_back(">>> Choisir mes sélections <<<");
    int s = menuRun(g_intlLW, items, 60, 440);
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
    switch (type) { case IT_EURO: conf = UEFA; break; case IT_CAN: conf = CAF; break; case IT_ASIA: conf = AFC; break; case IT_GOLD: conf = CONCACAF; break; case IT_OFC: conf = OFC; break; default: break; }
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

#include "app_screens.inc"
#include "app_manager.inc"
#include "app_club.inc"
#include "app_club2.inc"
#include "app_draw.inc"
#include "app_editors.inc"
#include "app_players.inc"

// ------------------------------------------------------------------ options / aide
static ListW g_optLW;
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
        fmt("Commentaires pendant les matchs : %s", g_settings.commentary ? "oui" : "non"),
        "Aide des commandes",
        "Retour" };
    int s = menuRun(g_optLW, items, 50, 340);
    if (s == 6) { g_ctlLW = ListW(); g_ctlCapture = -1; g_ctlBack = SC_OPTIONS; g_screen = SC_CONTROLS; return; }
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
    if (s == 9) { g_screen = SC_HELP; return; }
    if (IN.back || s == 10) { g_settings.save(); g_screen = SC_MAIN; }
}

static void screenHelp() {
    drawBackground("Commandes");
    const char* lines[] = {
        "STYLE SUPER NINTENDO (par défaut, Options > Configurer) :",
        "   Clavier 1 : Flèches + X passe, C tir (appui long = puissance), V lob/centre, B passe en profondeur, N sprint",
        "   Clavier 2 : Z Q S D + F passe, G tir, H lob, J profondeur, T sprint",
        "   Manette : A passe, B tir, X lob, Y profondeur, RB sprint",
        "   En défense : passe = changer de joueur (ou tacle debout au contact), tir = tacle glissé, lob = presser le porteur",
        "STYLE CLASSIQUE (2 boutons) : bouton 1 passe (appui court) ou tir (appui long), bouton 2 lob / tacle glissé",
        "BAGARRE : juste après une faute subie, bouton tir ou lob près du fautif... attention au carton rouge !",
        "",
        "EFFET (aftertouch) : juste après une frappe, orientez la direction :",
        "   sur le côté = ballon brossé,  vers l'arrière = ballon levé,  vers l'avant = tir tendu",
        "SANS LE BALLON : bouton 1 près du porteur = tacle ;  bouton 2 = tacle glissé",
        "   Attention : un tacle par derrière = faute, carton jaune ou rouge, penalty dans la surface !",
        "GARDIEN : il plonge tout seul. Quand il a le ballon : bouton 1 = relance à la main, bouton 2 = dégagement",
        "COUPS DE PIED ARRÊTÉS : orientez la visée puis bouton 1 (court / tir) ou bouton 2 (long)",
        "PENALTY : direction + appui long pour la puissance. En défense, orientez le plongeon du gardien.",
        "",
        "Échap / P / Start : pause (remplacements, radar, son...)      F11 : plein écran",
        "RÈGLES : hors-jeu, touches, corners, 6 mètres, fautes, cartons, penalties, prolongations,",
        "tirs au but, matchs aller-retour, passe en retrait au gardien interdite, 5 remplacements.",
    };
    int y = 36;
    for (auto l : lines) { drawTextPx(l, 12, y, 10, C_TXT); y += 13; }
    drawFooter("Retour");
    if (IN.back || IN.ok || IN.click) g_screen = SC_MAIN;
}

// ------------------------------------------------------------------ menu principal
static ListW g_mainLW;
static float g_titleT = 0;
static void screenMain(float dt) {
    g_titleT += dt;
    ClearBackground(Color{ 10, 16, 36, 255 });
    // ciel nocturne en dégradé
    for (int y = 0; y < 120; y += 4) {
        unsigned char k = (unsigned char)(y / 4);
        DrawRectangle(0, y, VW, 4, Color{ (unsigned char)(10 + k / 2), (unsigned char)(16 + k), (unsigned char)(36 + k * 2), 255 });
    }
    // tribunes : gradins et public (flashs d'appareils photo)
    DrawRectangle(0, 92, VW, 34, Color{ 28, 30, 46, 255 });
    for (int row = 0; row < 8; row++) {
        int yy = 94 + row * 4;
        DrawRectangle(0, yy + 3, VW, 1, Color{ 20, 22, 34, 255 });
        for (int x = (row % 2) * 2; x < VW; x += 4) {
            uint32_t h = (uint32_t)(x * 2654435761u) ^ (uint32_t)(row * 40503u);
            h ^= h >> 13; h *= 0x5bd1e995; h ^= h >> 15;
            static const Color CR[] = { { 180, 40, 50, 255 }, { 220, 220, 230, 255 }, { 40, 70, 160, 255 }, { 200, 170, 60, 255 }, { 90, 90, 110, 255 }, { 60, 40, 30, 255 } };
            Color c = CR[h % 6];
            int bob = (int)(std::sin(g_titleT * 5 + (h % 17)) > 0.8f ? 1 : 0);
            DrawRectangle(x, yy - bob, 2, 2, c);
        }
    }
    for (int k = 0; k < 3; k++) {
        uint32_t h = (uint32_t)((int)(g_titleT * 7) * 7919 + k * 104729);
        h ^= h >> 13; h *= 0x5bd1e995;
        if (h % 3 == 0) DrawRectangle((int)(h % VW), 94 + (int)((h >> 8) % 30), 2, 2, WHITE);
    }
    // projecteurs
    for (int k = 0; k < 2; k++) {
        int px = k ? VW - 40 : 28;
        DrawRectangle(px + 5, 40, 2, 56, Color{ 70, 74, 90, 255 });
        DrawRectangle(px - 4, 30, 20, 12, Color{ 50, 54, 70, 255 });
        for (int i = 0; i < 4; i++) for (int j = 0; j < 2; j++) DrawRectangle(px - 2 + i * 5, 32 + j * 5, 4, 4, Color{ 255, 250, 210, 255 });
        float pulse = 0.85f + 0.15f * std::sin(g_titleT * 3 + k);
        DrawCircle(px + 6, 36, 20, Color{ 255, 250, 200, (unsigned char)(40 * pulse) });
        DrawTriangle(Vector2{ (float)px + 6, 40 }, Vector2{ (float)(k ? px - 160 : px - 40), 360 }, Vector2{ (float)(k ? px + 40 : px + 160), 360 }, Color{ 255, 250, 210, 14 });
    }
    // pelouse rayée avec lignes
    for (int i = 0; i < 12; i++) DrawRectangle(0, 126 + i * 20, VW, 10, Color{ 40, 116, 40, 255 });
    for (int i = 0; i < 12; i++) DrawRectangle(0, 136 + i * 20, VW, 10, Color{ 34, 102, 34, 255 });
    DrawRectangle(0, 126, VW, 2, Color{ 240, 240, 240, 200 });
    DrawLine(0, 240, VW, 240, Color{ 240, 240, 240, 180 });
    DrawCircleLines(VW / 2, 240, 40, Color{ 240, 240, 240, 180 });
    // panneaux publicitaires
    static const char* ADS[] = { "FRANCE FOOT 2D", "SUPER BUT", "PIXEL COLA", "AMIGOAL", "STADE TV", "GOAL FM" };
    static const Color ADC[] = { { 214, 40, 40, 255 }, { 247, 127, 0, 255 }, { 42, 157, 143, 255 }, { 106, 76, 147, 255 }, { 29, 53, 87, 255 }, { 38, 70, 83, 255 } };
    int off = (int)(g_titleT * 20) % 110;
    for (int i = -1; i < VW / 110 + 2; i++) {
        int bx = i * 110 - off;
        DrawRectangle(bx, 116, 108, 10, ADC[(i + 12) % 6]);
        drawTextPx(ADS[(i + 12) % 6], bx + 6, 116, 10, WHITE);
    }
    // titre
    const char* T = "FOOT 2D";
    int w = textWidth(T, 60);
    // « FRANCE » en bleu-blanc-rouge au-dessus du titre
    {
        const char* F = "FRANCE";
        int fw = textWidth(F, 20);
        int fx = VW / 2 - fw / 2;
        drawTextPx(F, fx + 2, 6, 20, Color{ 0, 0, 0, 200 });
        drawTextPx(F, fx, 4, 20, WHITE);
        DrawRectangle(fx, 26, fw / 3, 3, Color{ 0, 85, 164, 255 });
        DrawRectangle(fx + fw / 3, 26, fw / 3, 3, WHITE);
        DrawRectangle(fx + 2 * fw / 3, 26, fw - 2 * fw / 3, 3, Color{ 226, 0, 26, 255 });
    }
    for (int k = 0; k < 3; k++) drawTextPx(T, VW / 2 - w / 2 + 3 - k, 24 + 3 - k, 60, k == 2 ? Color{ 255, 220, 60, 255 } : (k == 1 ? Color{ 220, 40, 40, 255 } : Color{ 0, 0, 0, 200 }));
    drawTextCentered("le football français façon 16 bits", VW / 2, 84, 10, C_DIM);
    // joueurs qui courent
    Kit k1; k1.shirt = 0x1B2A63; k1.shorts = 0xFFFFFF; k1.socks = 0xE2001A;
    Kit k2; k2.shirt = 0xFFFFFF; k2.shirt2 = 0xE2001A; k2.shorts = 0xE2001A; k2.pattern = KP_VSTRIPES; k2.socks = 0xFFFFFF;
    int fr = (int)(g_titleT * 8);
    float px = std::fmod(g_titleT * 60, (float)VW + 80) - 40;
    drawPlayerSprite((int)px, 138, k1, 1, 0, 2, fr, PS_NORMAL, false, 0, 2);
    drawPlayerSprite((int)px - 40, 138, k2, 3, 0, 2, fr + 1, PS_NORMAL, false, 0, 2);
    DrawEllipse((int)px + 12, 138, 4, 1.5f, Color{ 0, 0, 0, 80 });
    DrawCircle((int)px + 12, 135, 3, WHITE);
    std::vector<std::string> items = { "Match amical", "Compétitions internationales", "Carrière club (saisons)", "Championnat (1res divisions)", "Coupes d'Europe (C1 et C3)", "Compétition personnalisée",
                                       "Charger une partie", "Entraînement", "Fiches des clubs", "Éditeurs (clubs, sponsors, managers)", "Options", "À propos", "Quitter" };
    int s = listRun(g_mainLW, (int)items.size(), VW / 2 - 150, 142, 300, (int)items.size(), 15, [&](int i, int x, int y, bool sel) {
        drawTextCentered(items[i], x + 150, y + 3, 10, sel ? BLACK : C_TXT, false);
    });
    drawTextPx(std::string("v") + GAME_VERSION, 6, VH - 26, 10, Color{ 200, 210, 240, 200 });
    drawFooter("Flèches / souris / manette   OK : valider");
    switch (s) {
    case 0: g_careerActive = false; openPick(PM_FRIENDLY_HOME); break;
    case 1: g_screen = SC_INTL; break;
    case 2: g_careerActive = false; openPick(PM_CAREER); break;
    case 3: g_careerActive = false; g_lmLW = ListW(); g_screen = SC_LEAGUEMODE; break;
    case 4: {
        g_careerActive = false;
        g_intlCandidates = Career::euroCandidates();
        g_intlSel.clear(); g_euroModePick = true; g_leagueModePick = true;
        openPick(PM_INTL);
        break;
    }
    case 5: g_customLW = ListW(); g_screen = SC_CUSTOM; break;
    case 6: g_slotSave = false; g_screen = SC_SLOTS; break;
    case 7: g_careerActive = false; openPick(PM_TRAIN); break;
    case 8: openPick(PM_FICHE); break;
    case 9: g_edMenuLW = ListW(); g_screen = SC_EDITMENU; break;
    case 10: g_screen = SC_OPTIONS; break;
    case 11: g_screen = SC_ABOUT; break;
    case 12: g_quit = true; break;
    }
}

// ------------------------------------------------------------------ boucle
extern std::string (*g_sponsorImagePath)(const std::string&);
void appInit() {
    g_sponsorImagePath = sponsorImagePath;
    g_settings.load();
    g_world.build();
    audioSetEnabled(g_settings.sound);
    audioMusicEnabled(g_settings.music);
    audioSetTrackMode(g_settings.musicTrack);
}

void appFrame(float dt) {
    inputPoll(IN, CTL);
    MenuInput saved = IN;
    if (g_confirm.active) { IN = MenuInput(); IN.mouse = saved.mouse; }
    if (IsKeyPressed(KEY_F11) || ((IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) && IsKeyPressed(KEY_ENTER))) {
        g_settings.fullscreen = !g_settings.fullscreen; ToggleBorderlessWindowed(); g_settings.save();
        IN.ok = IN.start = false;
    }
    g_noBackBtn = g_screen == SC_MAIN || g_screen == SC_JOBS;
    audioMusic(g_screen != SC_MATCH && g_screen != SC_HALFTIME && g_screen != SC_TVINTRO && g_screen != SC_SETUP && g_screen != SC_STUDIO);
    switch (g_screen) {
    case SC_MAIN: screenMain(dt); break;
    case SC_PICK: screenPick(); break;
    case SC_SETUP: screenSetup(); break;
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
    case SC_HELP: screenHelp(); break;
    case SC_FICHE: screenFiche(dt); break;
    case SC_EDITMENU: screenEditMenu(); break;
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
    case SC_STADIUM: screenStadium(); break;
    case SC_CUSTOMLIST: screenCustomList(); break;
    case SC_STAFF: screenStaff(); break;
    case SC_RESERVES: screenReserves(); break;
    case SC_OFFERS: screenOffers(); break;
    case SC_ACADEMY: screenAcademy(); break;
    case SC_PLAYER: screenPlayer(); break;
    case SC_PLAYEREDIT: screenPlayerEdit(); break;
    case SC_TACTICS: screenTactics(); break;
    case SC_EDITDB: screenEditDb(); break;
    case SC_FRIENDLIES: screenFriendlies(); break;
    case SC_REFEREES: screenReferees(); break;
    case SC_CLUBMENU: screenClubMenu(); break;
    case SC_TRAINMODE: screenTrainMode(); break;
    case SC_TRAINING: screenTraining(); break;
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
        if (IN.back) no = true;
        if (yes) { auto f = g_confirm.onYes; g_confirm.active = false; if (f) f(); }
        else if (no) { auto f = g_confirm.onNo; g_confirm.active = false; if (f) f(); }
    }
    if (g_screen == SC_HUB && IN.back) {}
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
    if (m == "anthem" || m == "duel" || m == "lap" || m == "ettoss" || m == "reds" || m == "setupfr") {
        int a = g_world.nationIndex("FRA"), b = g_world.nationIndex("ARG");
        startSetup(a, b, false, -1, -1);
        if (m == "setupfr") { g_setupRow = 9; return; }
        for (int i = 0; i < NUM_INPUTS; i++) g_setup.side[i] = -1;
        if (m == "duel") g_setup.side[IN_KB1] = 0;
        launchMatch();
        Match& M = *g_match;
        if (m == "anthem") { M.cerPhase = 10; M.cerT = 0; for (int k = 0; k < 22; k++) M.pl[k].pos = V2(PITCH_W / 2 - 13 + (k % 11) * 2.4f, PITCH_L / 2 + (k / 11 ? 1.4f : -1.4f)); return; }
        M.ceremony = false; M.startPeriod(0); M.state = MS_PLAY;
        if (m == "duel") { M.fightLevel = 2; M.startFight(5, 16); M.duelHp[1] = 55; }
        if (m == "lap") { M.startPeriod(1); M.clock = 90; M.finishMatch(); g_trophyChecked = true; M.startLap(0); }
        if (m == "ettoss") { M.S.decisive = true; M.startPeriod(1); M.state = MS_BREAK; M.stateT = 1.9f; M.nextSp = 2; }
        if (m == "reds") { M.giveCard(3, 2); M.giveCard(4, 2); M.giveCard(14, 2); M.state = MS_PLAY; }
        return;
    }
    if (m == "match" || m == "match2" || m == "board" || m == "subtest" || m == "goaltest" || m == "cardtest" || m == "photo" || m == "toss" || m == "trophy" || m == "trophy2") {
        int a = g_world.nationIndex("FRA"), b = g_world.nationIndex("ARG");
        startSetup(a, b, false, -1, -1);
        for (int i = 0; i < NUM_INPUTS; i++) g_setup.side[i] = -1;
        if (m == "match2") g_setup.side[IN_KB1] = 0;
        g_setup.pitch = 1;
        if (m == "photo") { g_setup.cupPhoto = true; g_setup.side[IN_KB1] = 1; }
        if (m == "toss") g_setup.side[IN_KB1] = 1;
        launchMatch();
        if (m == "toss") { g_match->cerPhase = 3; g_match->cerT = 0; }
        if (m == "trophy" || m == "trophy2") {
            Match& M = *g_match; M.ceremony = false; M.startPeriod(1); M.clock = 90; M.score[0] = 2; M.score[1] = 1;
            for (int i = 0; i < 22; i++) M.pl[i].pos = V2(10.f + (i * 7) % 50, 30.f + (i * 13) % 50);
            M.finishMatch(); g_trophyChecked = true;
            if (m == "trophy") M.startTrophy(1, 0, 0, "Coupe de France"); else M.startTrophy(0, 1, 2, "Ligue 1");
        }
        if (m == "board") { g_match->ceremony = false; g_match->startPeriod(0); g_match->clock = 43.7f; }
        if (m == "subtest" || m == "goaltest" || m == "cardtest") {
            Match& M = *g_match; M.ceremony = false; M.startPeriod(0);
            M.state = MS_PLAY; M.clock = 20;
            if (m == "subtest") { M.substitute(0, 6, 0); M.state = MS_STOP; M.stateT = 0; M.nextSp = SP_THROWIN; M.nextSpTeam = 0; M.nextSpPos = V2(0.2f, 45.f); M.cam = V2(10, 50); }
            if (m == "goaltest") { int f = 10; M.ball.pos = V2(PITCH_W / 2, 3); M.pl[f].pos = V2(PITCH_W / 2 + 3, 8); M.ball.lastTouch = f; M.ball.lastTeam = 0; M.attackDir[0] = -1; M.goalScored(0); }
            if (m == "cardtest") { M.pl[14].pos = M.pl[3].pos + V2(0.5f, 0); M.foul(14, 3, true); M.pendCardOff = 14; M.pendCardType = 1; }
        }
    } else if (m == "setup") {
        startSetup(g_world.nationIndex("FRA"), g_world.nationIndex("BRA"), false, -1, -1);
    } else if (m == "hub") {
        int user = -1;
        for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Stade Brestois") user = i;
        g_career.newClubCareer(user, 2026); g_careerActive = true; g_needAdvance = true; openHub();
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
            ms.training = m == "trainfk" ? 2 : 1; ms.title = "Entraînement"; ms.stadium = "Centre d'entraînement"; ms.halfSeconds = 600; ms.crowdFill = 0.02f;
            g_match.reset(new Match()); g_match->init(ms); g_screen = SC_MATCH;
        }
    } else if (m == "draw" || m == "drawgrp" || m == "seasonend") {
        const char* nm = m == "draw" ? "Stade Lavallois" : "Paris Saint-Germain";
        int user = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == nm) user = i;
        g_career.newClubCareer(user, 2026); g_careerActive = true;
        Season& S = g_career.season;
        g_drawQueue.clear(); g_drawSeen.clear(); g_drawSeenYear = g_career.year;
        for (int k = 0; k < 3000; k++) {
            auto pm = S.advance(false);
            if (pm.comp < 0) break;
            if (m != "seasonend" && collectDraws()) {
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
        if (m == "jeunes") { g_screen = SC_COMPS; g_compsCat = 7; return; }
        g_screen = SC_COEFF; g_coeffTab = m == "spots" ? 0 : 3;
    }
    else if (m == "about") g_screen = SC_ABOUT;
    else if (m == "sponsors") g_screen = SC_SPONSORS;
    else if (m == "managers") g_screen = SC_MANAGERS;
    else if (m == "leaguemode") g_screen = SC_LEAGUEMODE;
    else if (m == "intlopt") g_screen = SC_INTL;
    else if (m == "studio" || m == "history" || m == "news" || m == "seasonart" || m == "seasonart2" || m == "cdlbracket" || m == "finance2") {
        int user = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Paris Saint-Germain") user = i;
        g_career.newClubCareer(user, 2026); g_careerActive = true;
        Season& S = g_career.season;
        if (m == "history") { while (true) { auto pm = S.advance(true); if (pm.comp < 0) break; } g_career.endSeason(); g_histTab = 5; g_screen = SC_HISTORY; return; }
        if (m == "seasonart") { showSeasonArticle(); return; }
        if (m == "cdlbracket") { while (true) { auto pm = S.advance(true); if (pm.comp < 0) break; } openCompView(g_career.cdl, -1, -1); g_cvMode = 0; for (int k = 0; k < (int)S.comps[g_career.cdl].stages.size(); k++) if (S.comps[g_career.cdl].stages[k].ties.size() == 8) g_cvStage = k; return; }
        if (m == "seasonart2") { while (true) { auto pm = S.advance(true); if (pm.comp < 0) break; } g_career.endSeason(); showSeasonArticle(); return; }
        if (m == "finance2") { g_screen = SC_FINANCE; return; }
        for (int k = 0; k < 8; k++) { auto pm = S.advance(false); if (pm.comp < 0) break; auto& C = S.comps[pm.comp]; simulateMatch(C.matches[pm.match], &C); S.recordResult(pm.comp, pm.match); g_career.mgrAfterMatch(pm.comp, pm.match); S.checkRound(pm.comp); }
        if (m == "news") { g_screen = SC_NEWS; if (!S.news.empty()) g_newsOpen = (int)S.news.size() - 1; return; }
        int rm = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Real Madrid") rm = i;
        startSetup(user, rm, false, -1, -1);
        g_setup.studio = true; g_setup.stadium = "Parc des Princes"; g_setup.title = "Ligue des Champions - Phase de groupes";
        g_tv = { "TF SPORT", "Thierry Delorme", "Jean-Michel Lavaud" };
        g_studioPhase = 0; g_studioT = 0; g_screen = SC_STUDIO;
    } else if (m == "help") g_screen = SC_HELP;
    else if (m == "main") g_screen = SC_MAIN;
    else if (m == "tv" || m == "tvmatch" || m == "tdcmatch" || m == "amateur" || m == "clubmenu" || m == "staff" || m == "reserves" || m == "referees" || m == "article" || m == "controls2" || m == "training") {
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
        const MatchRes& mr = S.comps[g_pending.comp].matches[g_pending.match];
        startSetup(mr.home, mr.away, true, g_pending.comp, g_pending.match);
        for (int i = 0; i < NUM_INPUTS; i++) g_setup.side[i] = -1;
        if (m == "tv") { g_screen = SC_TVINTRO; audioJingle(1); } else launchMatch();
        if (m == "tdcmatch") { fprintf(stderr, "TDC test: comp %s kind %d\n", S.comps[g_pending.comp].name.c_str(), S.comps[g_pending.comp].kind); g_match->ceremony = false; g_match->startPeriod(1); g_match->clock = 89.9f; g_match->state = MS_PLAY; g_match->score[0] = 1; }
    }
    else if (m == "copt") startCareerWith(g_world.firstClub + 3, SC_MAIN);
    else if (m == "status") { int u = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_career.tierOfTeam(i) < 0 && i >= g_world.firstClub) {} for (auto& pl : g_basePyramids[0].pools) if (pl.tier == 6 && !pl.clubs.empty()) { u = pl.clubs[0]; break; } if (u < 0) u = g_world.firstClub; g_career.newClubCareer(u, 2026); g_careerActive = true; g_needAdvance = true; openHub(); }
    else if (m == "fiche") { int u = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Stade Brestois") u = i; openFiche(u, SC_MAIN); }
    else if (m == "fiche2") { int u = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Stade Rennais") u = i; openFiche(u, SC_MAIN); g_fichePage = 2; }
    else if (m == "editor") openClubEditor(-1, false, SC_MAIN);
    else if (m == "editmenu") g_screen = SC_EDITMENU;
    else if (m == "custom") { for (int i = 0; i < 12; i++) g_customSel.push_back(i); g_screen = SC_CUSTOM; }
    else if (m == "controls") g_screen = SC_CONTROLS;
    else if (m == "coeff" || m == "coeffclub" || m == "eurospots") { g_career.newClubCareer(0 + g_world.firstClub, 2026); g_careerActive = true; g_screen = SC_COEFF; g_coeffTab = m == "coeffclub" ? 1 : m == "eurospots" ? 2 : 0; }
    else if (m == "compsreg" || m == "compsdist") { int u = -1; for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Stade Briochin") u = i; g_career.newClubCareer(u, 2026); g_careerActive = true; g_compsCat = m == "compsreg" ? 5 : 6; g_screen = SC_COMPS; }
    else if (m == "options") g_screen = SC_OPTIONS;
    else if (m == "rules") {
        int user = -1;
        for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Stade Lavallois") user = i;
        g_career.newClubCareer(user, 2026); g_careerActive = true;
        int p, q, gg; g_career.tierOfTeam(user, &p, &q, &gg);
        openCompView(g_career.pyramids[p].pools[q].comps[gg]); g_cvMode = 2;
    }
    else if (m == "table" || m == "cup" || m == "hub2" || m == "squad" || m == "fixtures" || m == "end" || m == "scorers" || m == "market" || m == "finance" || m == "minfo" || m == "groups" || m == "confirm" || m == "fiche3" || m == "stadium" || m == "stadium2" || m == "stadium3" || m == "cuphub") {
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
        if (m == "market") openMarket();
        if (m == "finance") g_screen = SC_FINANCE;
        if (m == "minfo") { auto um = userMatches(true); if (!um.empty()) openMatchInfo(um.back().first, um.back().second, SC_HUB); }
        if (m == "groups") { for (int k = 0; k < 400; k++) { auto pm = S.advance(true); (void)pm; if (S.comps[g_career.ucl].stages.size() >= 4) break; } openCompView(g_career.ucl, 3, 0); g_cvMode = 0; }
        if (m == "stadium") { g_screen = SC_STADIUM; }
        if (m == "stadium2") { g_screen = SC_STADIUM; g_stTab = 1; }
        if (m == "stadium3") { g_screen = SC_STADIUM; g_stTab = 2; }
        if (m == "cuphub") { for (int k = 0; k < 3000; k++) { auto pm = S.advance(false); if (pm.comp < 0) break; if (S.comps[pm.comp].format != FMT_LEAGUE) { g_pending = pm; g_needAdvance = false; break; } simulateMatch(S.comps[pm.comp].matches[pm.match], &S.comps[pm.comp]); S.recordResult(pm.comp, pm.match); S.checkRound(pm.comp); } g_screen = SC_HUB; }
        if (m == "confirm") askConfirm("Revenir au menu principal ? (la partie est sauvegardée automatiquement)", []() {});
        if (m == "fiche3") { for (int k = 0; k < 3; k++) { while (true) { auto pm = S.advance(true); if (pm.comp < 0) break; } g_career.endSeason(); } openFiche(g_world.nationIndex("FRA") >= 0 ? g_career.ucl >= 0 && S.comps.size() ? user : user : user, SC_HUB); for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].name == "Paris Saint-Germain") openFiche(i, SC_HUB); g_fichePage = 1; }
    }
}
bool appInMatch() { return g_screen == SC_MATCH; }
