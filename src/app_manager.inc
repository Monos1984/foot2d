// ------------------------------------------------------------------ mode manager : sauvegarde auto, mercato, finances, messages
static void autosave() {
    if (!g_careerActive) return;
#ifdef _WIN32
    mkdir("saves");
#else
    mkdir("saves", 0755);
#endif
    if (g_career.save("saves/auto.sav")) {
        FILE* f = fopen("saves/auto.txt", "w");
        if (f) {
            std::string d = g_career.kind == 0 ? g_world.teams[g_career.userTeam].name + fmt(" - saison %d-%02d - %s", g_career.year, (g_career.year + 1) % 100, dateOf(g_career.season.now, g_career.year).c_str())
                          : g_career.kind == CK_CUSTOM ? g_career.custom.name : fmt("%s %d", INTL_NAMES[g_career.intlType], g_career.year);
            fprintf(f, "%s\n", d.c_str()); fclose(f);
        }
    }
}

// ---------------- mercato
struct MkRow { int team; int pid; int idx; };
static int g_mkTab = 0, g_mkPos = -1;
static bool g_mkAfford = false, g_mkReal = false, g_mkFree = false;
static int g_mkSort = 0;                // 0 note, 1 âge, 2 prix, 3 salaire, 4 potentiel
// négociation en cours
struct Nego { bool active = false; int team = -1, pid = -1, fee = 0, wage = 0, years = 3, ask = 0, demand = 0; int row = 0; std::string last; };
static Nego g_nego;
static std::vector<MkRow> g_mkList;
static ListW g_mkLW;

static void buildMarket() {
    g_mkList.clear();
    int u = g_career.userTeam;
    int p, q, g;
    int ut = g_career.tierOfTeam(u, &p, &q, &g);
    std::set<int> clubs;
    for (int t = 0; t < (int)g_world.teams.size(); t++) if (g_world.teams[t].kind == TK_CLUB && g_world.teams[t].dbClub >= 0) clubs.insert(t);
    if (ut >= 0) {
        const Pyramid& P = g_career.pyramids[p];
        int reg = g_world.teams[u].region;
        for (auto& pl : P.pools) {
            if (std::abs(pl.tier - ut) > 1) continue;
            for (int t : pl.clubs) if (pl.tier <= 4 || g_world.teams[t].region == reg) clubs.insert(t);
        }
    }
    for (int t = 0; t < (int)g_world.teams.size(); t++) if (g_world.teams[t].parent == u) clubs.insert(t);   // réserves du club
    clubs.insert(freeAgentsTeam());                                                                      // joueurs libres
    if (g_mkFree) { clubs.clear(); clubs.insert(freeAgentsTeam()); }
    clubs.erase(u);
    // niveau de l'effectif
    g_world.ensureSquad(u);
    std::vector<int> ovs; for (auto& pl : g_world.teams[u].squad) ovs.push_back(pl.overall());
    std::sort(ovs.rbegin(), ovs.rend());
    int ref = ovs.size() > 10 ? ovs[10] : 40;
    for (int t : clubs) {
        g_world.ensureSquad(t);
        const Team& T = g_world.teams[t];
        for (int i = 0; i < (int)T.squad.size(); i++) {
            const Player& pl = T.squad[i];
            int ov = pl.overall();
            if (!T.freeAgents && (ov < ref - 10 || ov > ref + 12 + 2 * g_career.staffLevel(SR_SCOUT))) continue;
            if (T.freeAgents && ov < ref - 14) continue;
            if (g_mkPos >= 0 && pl.pos != g_mkPos) continue;
            if ((pl.gender == 1) != (g_world.teams[u].youth == 6)) continue;     // séparation stricte hommes / femmes
            g_mkList.push_back({ t, pl.id, i });
        }
    }
    std::sort(g_mkList.begin(), g_mkList.end(), [](const MkRow& a, const MkRow& b) {
        const Player& A = g_world.teams[a.team].squad[a.idx]; const Player& B = g_world.teams[b.team].squad[b.idx];
        switch (g_mkSort) {
        case 1: if (A.age != B.age) return A.age < B.age; break;
        case 2: if (A.value() != B.value()) return A.value() < B.value(); break;
        case 3: if (A.wage() != B.wage()) return A.wage() < B.wage(); break;
        case 4: if (A.pot != B.pot) return A.pot > B.pot; break;
        default: break;
        }
        return A.overall() > B.overall(); });
    if (g_mkList.size() > 600) g_mkList.resize(600);
    g_mkLW = ListW();
}

static int askingPrice(int team, int idx) {
    const Team& T = g_world.teams[team];
    const Player& p = T.squad[idx];
    if (T.parent == g_career.userTeam || T.freeAgents) return 0;   // joueur de la réserve ou libre : gratuit
    auto lu = g_world.pickLineup(team, T.formation);
    bool starter = false; for (int k = 0; k < 11 && k < (int)lu.size(); k++) if (lu[k] == idx) starter = true;
    return (int)(p.value() * (starter ? 1.3 : 1.0));
}

static void openMarket() { g_mkTab = 0; buildMarket(); g_screen = SC_MARKET; }

// négociation : indemnité, salaire, durée du contrat
static void marketNegotiation() {
    Nego& N = g_nego;
    int idx = -1; int t = g_world.findPlayer(N.pid, &idx);
    if (t < 0) { N.active = false; return; }
    const Player& p = g_world.teams[t].squad[idx];
    const Team& U = g_world.teams[g_career.userTeam];
    int x = 60, y = 84, w = VW - 120;
    DrawRectangle(x, y, w, 220, Color{ 16, 28, 56, 250 });
    DrawRectangleLines(x, y, w, 220, C_SEL);
    drawTextPx(fmt("Négociation : %s (%s, %d ans, note %d)", p.name.c_str(), posName(p.pos), p.age, p.overall()), x + 10, y + 6, 10, C_HI);
    drawTextPx("Club : " + g_world.teams[t].name + (g_world.teams[t].freeAgents ? " - pas d'indemnité" : fmt(" - prix demandé %s", money(N.ask).c_str())), x + 10, y + 20, 10, C_DIM);
    drawTextPx(U.status == CS_AMATEUR ? std::string("Licence amateur : pas de salaire.") : fmt("Salaire demandé par le joueur : %s / an   (réputation du club : %d)", money(N.demand).c_str(), teamReputation(g_career.userTeam)), x + 10, y + 32, 10, C_DIM);
    bool freeP = g_world.teams[t].freeAgents || g_world.teams[t].parent == g_career.userTeam;
    int bonus = U.status == CS_AMATEUR ? 0 : N.wage * N.years / 10;
    std::vector<std::string> rows = {
        freeP ? std::string("Indemnité de transfert : aucune") : "Indemnité proposée : " + money(N.fee) + fmt("  (%d %% du prix)", N.ask ? N.fee * 100 / N.ask : 100),
        U.status == CS_AMATEUR ? std::string("Salaire : licence amateur") : "Salaire annuel proposé : " + money(N.wage),
        fmt("Durée du contrat : %d an%s", N.years, N.years > 1 ? "s" : ""),
        "Prime à la signature (10 % du contrat) : " + money(bonus),
        ">>> ENVOYER L'OFFRE <<<", "Abandonner" };
    if (IN.up) N.row = (N.row + 5) % 6;
    if (IN.down) N.row = (N.row + 1) % 6;
    for (int i = 0; i < 6; i++) {
        int yy = y + 50 + i * 18;
        bool sel = N.row == i;
        bool hover = IN.mouse.x >= x + 10 && IN.mouse.x < x + w - 10 && IN.mouse.y >= yy && IN.mouse.y < yy + 16;
        if (hover && IN.click) N.row = i;
        DrawRectangle(x + 10, yy, w - 20, 16, sel ? C_SEL : C_ITEM);
        drawTextPx((i < 3 ? "< " : "") + rows[i] + (i < 3 ? " >" : ""), x + 16, yy + 3, 10, sel ? BLACK : C_TXT);
    }
    int d = IN.left ? -1 : IN.right ? 1 : 0;
    if (d) {
        if (N.row == 0 && !freeP) N.fee = std::max(0, N.fee + d * std::max(1, N.ask / 20));
        if (N.row == 1 && U.status != CS_AMATEUR) N.wage = std::max(1, N.wage + d * std::max(1, N.demand / 20));
        if (N.row == 2) N.years = std::max(1, std::min(5, N.years + d));
    }
    if (!N.last.empty()) drawTextPx(fitText(N.last, w - 20, 10), x + 10, y + 204, 10, C_BAD);
    drawFooter("Haut/Bas : ligne   Gauche/Droite : modifier   OK : valider   Retour : abandonner");
    bool go = (IN.ok || (IN.click && IN.mouse.y >= y + 50 + 4 * 18 && IN.mouse.y < y + 50 + 5 * 18)) && N.row == 4;
    if (go) {
        std::string msg;
        int r = g_career.signPlayer(N.pid, freeP ? 0 : N.fee, U.status == CS_AMATEUR ? 0 : N.wage, N.years, msg);
        if (r == 1) { toast(msg); N.active = false; buildMarket(); return; }
        N.last = msg;
        if (r == 0 && msg.find("demande ") != std::string::npos) {}     // le club a fait une contre-proposition
    }
    if ((IN.ok && N.row == 5) || IN.back) { N.active = false; IN.back = false; }
}

static void screenMarket() {
    drawBackground("Mercato");
    const ManagerState& M = g_career.mgr;
    Team& U = g_world.teams[g_career.userTeam];
    drawTextPx("Budget : " + money(M.budget), 12, 32, 10, M.budget >= 0 ? C_GOOD : C_BAD);
    drawTextPx(fmt("Effectif : %d joueurs", (int)U.squad.size()), 170, 32, 10, C_TXT);
    drawTextPx(fitText(g_career.windowText(), 290, 10), 330, 32, 10, g_career.transferWindow() || g_career.jokerAvailable() ? C_GOOD : C_BAD);
    static const char* TABS[] = { "Acheter", "Vendre / libérer", "Transferts récents", "Numéros", "Prêts et quotas" };
    for (int i = 0; i < 5; i++) if (button(12 + i * 124, 46, 120, 14, TABS[i], g_mkTab == i)) { g_mkTab = i; g_mkLW = ListW(); }
    if (IN.pgUp) { g_mkTab = (g_mkTab + 4) % 5; g_mkLW = ListW(); }
    if (IN.pgDn) { g_mkTab = (g_mkTab + 1) % 5; g_mkLW = ListW(); }
    int neLim = nonEuLimit(g_career.userTeam), neCnt = 0;
    for (auto& q : U.squad) if (isNonEU(q, g_career.userTeam)) neCnt++;
    int y = 64;
    if (g_mkTab == 0) {
        if (g_nego.active) { marketNegotiation(); return; }
        static const char* PF[] = { "Tous", "G", "D", "M", "A" };
        for (int i = 0; i < 5; i++) if (button(12 + i * 40, y, 38, 13, PF[i], g_mkPos == i - 1)) { g_mkPos = i - 1; buildMarket(); }
        static const char* SO[] = { "Tri : note", "Tri : âge", "Tri : prix", "Tri : salaire", "Tri : potentiel" };
        if (button(214, y, 100, 13, SO[g_mkSort])) { g_mkSort = (g_mkSort + 1) % 5; buildMarket(); }
        if (button(318, y, 96, 13, g_mkAfford ? "Abordables" : "Tous les prix", g_mkAfford)) g_mkAfford = !g_mkAfford;
        if (button(418, y, 100, 13, g_mkReal ? "Réalistes" : "Tous profils", g_mkReal)) g_mkReal = !g_mkReal;
        if (button(522, y, 106, 13, g_mkFree ? "Joueurs libres" : "Tous les clubs", g_mkFree)) { g_mkFree = !g_mkFree; buildMarket(); }
        y += 17;
        std::vector<int> rows;
        for (int i = 0; i < (int)g_mkList.size(); i++) {
            const MkRow& r = g_mkList[i];
            const Team& T = g_world.teams[r.team];
            if (r.idx >= (int)T.squad.size() || T.squad[r.idx].id != r.pid) continue;
            if (g_mkAfford && askingPrice(r.team, r.idx) > M.budget) continue;
            if (g_mkReal) {    // transfert réaliste : le joueur peut accepter le projet, le prix reste accessible
                const Player& pp = T.squad[r.idx];
                if (pp.overall() > teamReputation(g_career.userTeam) + 20) continue;
                if (askingPrice(r.team, r.idx) > std::max<int64_t>(M.budget, 0) * 3 / 2 + 5) continue;
            }
            rows.push_back(i);
        }
        drawTextPx("P   Joueur", 16, y, 10, C_DIM); drawTextPx("Âge", 190, y, 10, C_DIM); drawTextPx("Note", 222, y, 10, C_DIM);
        drawTextPx("Club", 262, y, 10, C_DIM); drawTextPx("Prix", VW - 150, y, 10, C_DIM); drawTextPx("Salaire", VW - 76, y, 10, C_DIM);
        y += 12;
        int sel = listRun(g_mkLW, (int)rows.size(), 12, y, VW - 24, (VH - 36 - y) / 12, 12, [&](int i, int x, int yy, bool s2) {
            const MkRow& r = g_mkList[rows[i]];
            const Team& T = g_world.teams[r.team];
            const Player& p = T.squad[r.idx];
            Color c = s2 ? BLACK : C_TXT;
            int price = askingPrice(r.team, r.idx);
            drawTextPx(posName(p.pos), x + 4, yy + 1, 10, c);
            drawTextPx(fitText(p.name, 150, 10), x + 22, yy + 1, 10, c);
            drawTextPx(fmt("%d", p.age), x + 182, yy + 1, 10, c);
            drawTextPx(fmt("%d", p.overall()), x + 214, yy + 1, 10, s2 ? BLACK : C_HI);
            if (g_career.staffLevel(SR_SCOUT) >= 3 && p.pot > p.overall() + 3) drawTextPx(fmt("+%d", p.pot - p.overall()), x + 230, yy - 3, 10, C_GOOD);
            drawKitIcon(T.home, x + 240, yy + 1, 1);
            drawTextPx(fitText(nameLvl(r.team), VW - 420, 10), x + 252, yy + 1, 10, s2 ? BLACK : C_DIM);
            drawTextPx(price ? money(price) : "gratuit", x + VW - 162, yy + 1, 10, price <= M.budget ? c : C_BAD);
            drawTextPx(money(wageDemand(p, g_career.userTeam)), x + VW - 88, yy + 1, 10, c);
            if (T.freeAgents) drawTextPx("libre", x + 240, yy + 1, 10, s2 ? BLACK : C_GOOD);
            if (neLim < 99 && isNonEU(p, g_career.userTeam)) DrawRectangle(x + 16, yy + 2, 3, 8, C_BAD);   // extra-communautaire
        });
        if (rows.empty()) drawTextCentered("Aucun joueur ne correspond", VW / 2, y + 20, 10, C_DIM);
        {
            int cur = g_mkLW.cur;
            if (cur >= 0 && cur < (int)rows.size()) {
                const MkRow& r = g_mkList[rows[cur]];
                if (!g_world.teams[r.team].freeAgents && button(12, VH - 32, 200, 14, "Demander un prêt (salaire partagé)")) {
                    std::string err; if (g_career.loanIn(r.pid, err)) { toast("Prêt conclu"); buildMarket(); } else toast(err);
                }
            }
            if (neLim < 99) drawTextPx(fmt("Barre rouge : extra-communautaire (%d dans l'effectif, %d sur la feuille)", neCnt, neLim), 222, VH - 30, 10, neCnt > neLim ? C_HI : C_DIM);
        }
        drawFooter("OK : négocier   LB/RB : onglet   Retour");
        if (sel >= 0) {
            MkRow r = g_mkList[rows[sel]];
            const Player& p = g_world.teams[r.team].squad[r.idx];
            g_nego = Nego(); g_nego.active = true; g_nego.team = r.team; g_nego.pid = r.pid;
            g_nego.ask = askingPrice(r.team, r.idx); g_nego.fee = g_nego.ask;
            g_nego.demand = g_world.teams[g_career.userTeam].status == CS_AMATEUR ? 0 : wageDemand(p, g_career.userTeam);
            g_nego.wage = g_nego.demand; g_nego.years = p.age >= 31 ? 1 : p.age >= 28 ? 2 : 3;
        }
    } else if (g_mkTab == 1) {
        drawTextPx("P   Joueur", 16, y, 10, C_DIM); drawTextPx("Âge", 190, y, 10, C_DIM); drawTextPx("Note", 222, y, 10, C_DIM);
        drawTextPx("Buts", 262, y, 10, C_DIM); drawTextPx("Contrat", 300, y, 10, C_DIM); drawTextPx("Valeur", VW - 200, y, 10, C_DIM); drawTextPx("Salaire", VW - 110, y, 10, C_DIM);
        y += 12;
        int sel = listRun(g_mkLW, (int)U.squad.size(), 12, y, VW - 24, (VH - 38 - y) / 12, 12, [&](int i, int x, int yy, bool s2) {
            const Player& p = U.squad[i];
            Color c = s2 ? BLACK : C_TXT;
            drawTextPx(posName(p.pos), x + 4, yy + 1, 10, c);
            drawTextPx(fitText(p.name, 150, 10), x + 22, yy + 1, 10, c);
            drawTextPx(fmt("%d", p.age), x + 182, yy + 1, 10, c);
            drawTextPx(fmt("%d", p.overall()), x + 214, yy + 1, 10, s2 ? BLACK : C_HI);
            drawTextPx(fmt("%d", p.goals), x + 254, yy + 1, 10, c);
            static const char* CS[3] = { "pro", "fédéral", "amateur" };
            drawTextPx(fmt("%s, %d an%s", CS[p.contract % 3], contractYears(p), contractYears(p) > 1 ? "s" : ""), x + 288, yy + 1, 10, contractYears(p) <= 1 && p.contract != 2 ? (s2 ? BLACK : C_BAD) : c);
            drawTextPx(money(p.value()), x + VW - 212, yy + 1, 10, c);
            drawTextPx(p.contract == 2 ? std::string("-") : money(playerWage(p)), x + VW - 122, yy + 1, 10, c);
        });
        int cur = g_mkLW.cur;
        if (cur >= 0 && cur < (int)U.squad.size()) {
            int pid = U.squad[cur].id; std::string msg;
            if (button(12, VH - 32, 110, 14, "Salaire +10 %")) { if (g_career.changeWage(pid, 10, msg)) toast(msg); else toast(msg.empty() ? "Impossible" : msg); }
            if (button(126, VH - 32, 110, 14, "Salaire -10 %")) { if (g_career.changeWage(pid, -10, msg)) toast(msg); else toast(msg.empty() ? "Impossible" : msg); }
            if (button(240, VH - 32, 130, 14, "Prolonger (+1 an)")) { if (g_career.extendContract(pid, msg)) toast(msg); else toast(msg); }
            if (button(374, VH - 32, 62, 14, "Prêter")) { if (g_career.loanOut(pid, msg)) toast("Joueur prêté"); else toast(msg); }
        }
        if (cur >= 0 && cur < (int)U.squad.size() && button(VW - 200, VH - 32, 188, 14, "Libérer le joueur sélectionné")) {
            int pid = U.squad[cur].id; std::string nm = U.squad[cur].name;
            if (U.squad.size() <= 16) toast("Effectif trop réduit (16 joueurs minimum)");
            else askConfirm("Libérer " + nm + " de son contrat (sans indemnité) ?", [pid]() { g_career.releasePlayer(pid); toast("Joueur libéré"); });
        }
        drawFooter("OK : chercher un acheteur   LB/RB : onglet   Retour");
        if (sel >= 0) {
            int pid = U.squad[sel].id;
            std::string nm = U.squad[sel].name;
            int fee = 0;
            int to = g_career.findBuyer(pid, fee);
            if (to < 0) toast("Aucun club intéressé pour l'instant");
            else askConfirm(g_world.teams[to].name + " (" + shortLevel(to) + ") propose " + (fee ? money(fee) : std::string("un transfert libre")) + " pour " + nm + ". Accepter ?",
                            [pid, to, fee]() { std::string err; if (g_career.sellPlayer(pid, to, fee, err)) toast("Joueur vendu"); else toast(err); }, "Accepter", "Refuser");
        }
    } else if (g_mkTab == 2) {
        auto& tr = g_career.mgr.transfers;
        drawTextPx("Saison  Joueur", 16, y, 10, C_DIM); drawTextPx("Transfert", 230, y, 10, C_DIM); drawTextPx("Montant", VW - 80, y, 10, C_DIM);
        y += 12;
        listRun(g_mkLW, (int)tr.size(), 12, y, VW - 24, (VH - 18 - y) / 12, 12, [&](int i, int x, int yy, bool s2) {
            const TransferRec& r = tr[tr.size() - 1 - i];
            bool mine = r.from == g_career.userTeam || r.to == g_career.userTeam;
            Color c = s2 ? BLACK : mine ? C_HI : C_TXT;
            drawTextPx(fmt("%d", r.year), x + 4, yy + 1, 10, c);
            drawTextPx(fitText(r.name, 160, 10), x + 50, yy + 1, 10, c);
            std::string mv = (r.from >= 0 ? g_world.teams[r.from].shortName : std::string("?")) + " -> " + (r.to >= 0 ? g_world.teams[r.to].name : std::string("?"));
            drawTextPx(fitText(mv, VW - 330, 10), x + 218, yy + 1, 10, s2 ? BLACK : C_DIM);
            drawTextPx(r.fee ? money(r.fee) : "libre", x + VW - 92, yy + 1, 10, c);
        });
        if (tr.empty()) drawTextCentered("Aucun transfert pour l'instant", VW / 2, y + 20, 10, C_DIM);
        drawFooter("LB/RB : onglet   Retour");
    } else if (g_mkTab == 3) {
        // ---- numéros de maillot (1 à 99)
        drawTextPx("N°   P   Joueur", 16, y, 10, C_DIM); drawTextPx("Note", 262, y, 10, C_DIM); drawTextPx("Nationalité", 310, y, 10, C_DIM);
        y += 12;
        std::vector<int> ord(U.squad.size()); for (size_t i = 0; i < ord.size(); i++) ord[i] = (int)i;
        std::stable_sort(ord.begin(), ord.end(), [&](int a, int b) { return U.squad[a].num < U.squad[b].num; });
        listRun(g_mkLW, (int)ord.size(), 12, y, VW - 24, (VH - 38 - y) / 12, 12, [&](int i, int x, int yy, bool s2) {
            const Player& p = U.squad[ord[i]];
            Color c = s2 ? BLACK : C_TXT;
            bool dup = false; for (auto& q : U.squad) if (&q != &p && q.num == p.num) dup = true;
            drawTextPx(fmt("%2d", p.num), x + 4, yy + 1, 10, dup ? C_BAD : s2 ? BLACK : C_HI);
            drawTextPx(posName(p.pos), x + 30, yy + 1, 10, c);
            drawTextPx(fitText(p.name, 190, 10), x + 48, yy + 1, 10, c);
            drawTextPx(fmt("%d", p.overall()), x + 252, yy + 1, 10, c);
            int nat = p.nation >= 0 ? p.nation : U.nation;
            drawTextPx(fitText(nat >= 0 ? sanitize(NATIONS[nat].name) : std::string("-"), 200, 10), x + 298, yy + 1, 10, c);
            if (neLim < 99 && isNonEU(p, g_career.userTeam)) drawTextPx("extra-communautaire", x + VW - 170, yy + 1, 10, s2 ? BLACK : C_BAD);
        });
        int cur = g_mkLW.cur;
        if (cur >= 0 && cur < (int)ord.size()) {
            Player& p = U.squad[ord[cur]];
            auto step = [&](int d) { std::vector<bool> used(100, false); for (auto& q : U.squad) if (&q != &p) used[q.num] = true;
                                     int n = p.num; for (int k = 0; k < 99; k++) { n = (n - 1 + d + 99) % 99 + 1; if (!used[n]) break; } p.num = (uint8_t)n; };
            if (button(12, VH - 32, 60, 14, "N° -") || IN.left) step(-1);
            if (button(76, VH - 32, 60, 14, "N° +") || IN.right) step(1);
        }
        if (button(150, VH - 32, 230, 14, "Attribution automatique des numéros")) { autoNumbers(g_career.userTeam); toast("Numéros attribués (gardiens 1-16-30, défense 2-5, milieu 6-8-10, attaque 7-9-11)"); }
        drawFooter("Gauche/Droite : changer le numéro   LB/RB : onglet   Retour");
    } else if (g_mkTab == 4) {
        // ---- prêts en cours et quotas
        int x = 16;
        drawSection(12, y, VW - 24, "QUOTAS DE LA DIVISION"); y += 14;
        drawTextPx(fmt("Prêts entrants : %d / %d      Prêts sortants : %d / %d", g_career.loansIn(g_career.userTeam), g_career.loanLimitIn(g_career.userTeam), g_career.loansOut(g_career.userTeam), g_career.loanLimitOut(g_career.userTeam)), x, y, 10, C_TXT); y += 12;
        if (neLim < 99) { drawTextPx(fmt("Extra-communautaires sur la feuille de match : %d au maximum (effectif : %d)", neLim, neCnt), x, y, 10, neCnt > neLim ? C_HI : C_TXT); y += 12; }
        if (M.wageCapK > 0) { drawTextPx("DNCG : masse salariale " + money(g_career.wageBill(g_career.userTeam)) + " / plafond " + money(M.wageCapK), x, y, 10, g_career.wageBill(g_career.userTeam) > M.wageCapK ? C_BAD : C_GOOD); y += 12; }
        if (M.fpfStrikes) { drawTextPx(fmt("Fair-play financier UEFA : %d avertissement(s)", M.fpfStrikes), x, y, 10, C_BAD); y += 12; }
        drawTextPx(std::string("Joueur joker (L1, L2, Ligue 3, hors mercato avant janvier) : ") + (M.jokerUsed ? "utilisé" : g_career.jokerAvailable() ? "DISPONIBLE" : "non disponible"), x, y, 10, g_career.jokerAvailable() ? C_GOOD : C_DIM); y += 16;
        drawSection(12, y, VW - 24, "PRÊTS EN COURS (RETOUR EN FIN DE SAISON)"); y += 14;
        int n = 0;
        for (auto& l : g_career.loans) {
            if (l.from != g_career.userTeam && l.to != g_career.userTeam) continue;
            int idx; int at = g_world.findPlayer(l.pid, &idx);
            std::string nm = at >= 0 ? g_world.teams[at].squad[idx].name : std::string("?");
            bool in = l.to == g_career.userTeam;
            drawTextPx(std::string(in ? "Arrivée : " : "Départ : ") + nm + (in ? " (prêté par " + g_world.teams[l.from].name + ")" : " (prêté à " + g_world.teams[l.to].name + ")"), x, y, 10, in ? C_GOOD : C_TXT);
            y += 12; n++;
            if (y > VH - 30) break;
        }
        if (!n) drawTextPx("Aucun prêt en cours.", x, y, 10, C_DIM);
        drawFooter("LB/RB : onglet   Retour");
    }
    if (IN.back) g_screen = SC_CLUBMENU;
}

// ---------------- finances et objectifs
static void screenFinance() {
    drawBackground("Finances et objectifs");
    const ManagerState& M = g_career.mgr;
    const Team& U = g_world.teams[g_career.userTeam];
    int y = 40;
    auto line = [&](const std::string& k, const std::string& v, Color c = C_TXT) {
        drawTextPx(k, 30, y, 10, C_DIM); drawTextPx(v, 330, y, 10, c); y += 14;
    };
    drawSection(20, y, VW - 40, "FINANCES"); y += 14;
    line("Budget disponible", money(M.budget), M.budget >= 0 ? C_GOOD : C_BAD);
    line("Revenus annuels (TV, sponsors, subventions)", money(M.incomeBase));
    if (M.tvShare > 0) line("   dont droits TV (part fixe)", money(M.tvShare));
    line("Réputation du club", fmt("%d / 100 (%s)", teamReputation(g_career.userTeam), reputationLabel(teamReputation(g_career.userTeam))), C_HI);
    if (M.dncg > 0) line("DNCG", fmt("sous surveillance (avertissement %d/2)", M.dncg), C_BAD);
    line("Masse salariale annuelle", money(g_career.wageBill(g_career.userTeam)));
    line("Revenus encaissés cette saison", money(M.seasonIncome), C_GOOD);
    line("Salaires et frais de fonctionnement versés", money(-M.seasonWages), C_BAD);
    line("Bilan des transferts cette saison", money(M.seasonTransfers), M.seasonTransfers >= 0 ? C_GOOD : C_BAD);
    int64_t val = 0; for (auto& p : U.squad) val += p.value();
    line("Valeur de l'effectif", money(val));
    line("Stade", U.stadium.empty() ? "-" : U.stadium);
    y += 6;
    drawSection(20, y, VW - 40, "PRÉSIDENT"); y += 14;
    line("Objectif de la saison", g_career.objectiveText(), C_HI);
    line("Confiance du président", fmt("%d %%", M.confidence), M.confidence >= 50 ? C_GOOD : M.confidence >= 25 ? C_HI : C_BAD);
    DrawRectangle(330, y - 2, 200, 6, Color{ 0, 0, 0, 120 });
    DrawRectangle(330, y - 2, 2 * M.confidence, 6, M.confidence >= 50 ? C_GOOD : M.confidence >= 25 ? C_HI : C_BAD);
    y += 10;
    line("Mercato", g_career.windowText(), g_career.transferWindow() ? C_GOOD : C_DIM);
    y += 4;
    const char* tips[] = { "Recettes : chaque match à domicile rapporte une recette, les victoires en coupe et en Europe des primes.",
                           "Salaires et frais prélevés chaque mois. Primes réelles : droits TV au classement, Coupe de France, Coupe de la Ligue, UEFA.",
                           "Objectif manqué ou finances dans le rouge : la confiance baisse. À 5 % ou moins, c'est le licenciement." };
    for (auto t : tips) { drawTextPx(t, 30, y, 10, C_DIM); y += 12; }
    // demandes au président (2 par saison)
    ManagerState& MM = g_career.mgr;
    y += 4;
    int left = std::max(0, 2 - MM.boardRequests);
    drawTextPx(fmt("Demandes au président (%d restante%s cette saison) :", left, left > 1 ? "s" : ""), 30, y + 3, 10, C_HI);
    bool b1 = button(300, y, 160, 15, "Rallonge budgétaire", false, left > 0);
    bool b2 = button(466, y, 150, 15, "Demander du temps", false, left > 0);
    if (b1 && left > 0) {
        MM.boardRequests++;
        if (MM.confidence >= 55 && g_rng.chance(0.4f + (MM.confidence - 55) / 100.f)) {
            int64_t add = std::max<int64_t>(10, MM.incomeBase / 10);
            MM.budget += add; MM.confidence = std::max(0, MM.confidence - 8);
            toast("Accordé : +" + money(add) + " (confiance -8 %)");
            g_career.season.news.push_back("Le président accepte une rallonge de " + money(add) + " pour renforcer l'équipe.");
        } else { MM.confidence = std::max(0, MM.confidence - 4); toast("Refusé par le président (confiance -4 %)"); g_career.season.news.push_back("Le président refuse d'augmenter le budget : « Gagnez d'abord des matchs ! »"); }
    }
    if (b2 && left > 0) {
        MM.boardRequests++;
        if (MM.confidence < 45) { MM.confidence = std::min(100, MM.confidence + 10); toast("Le président vous renouvelle sa confiance (+10 %)"); g_career.season.news.push_back("Le président vous accorde du temps : « Je crois en votre projet. »"); }
        else toast("Le président n'a aucune inquiétude pour l'instant");
    }
    drawFooter("Retour");
    if (IN.back) g_screen = SC_CLUBMENU;
}

// ---------------- messages
static ListW g_newsLW;
// ---------------- licenciement : offres d'emploi
static std::vector<int> g_jobs;
static ListW g_jobsLW;
static void screenJobs() {
    if (g_jobs.empty()) {
        int u = g_career.userTeam;
        float r = g_world.teams[u].rating;
        int p, q, g; int ut = g_career.tierOfTeam(u, &p, &q, &g);
        std::vector<int> cand;
        if (ut >= 0) for (auto& pl : g_career.pyramids[p].pools) if (std::abs(pl.tier - ut) <= 1)
            for (int t : pl.clubs) if (t != u && g_world.teams[t].parent < 0 && std::fabs(g_world.teams[t].rating - r) < 8) cand.push_back(t);
        g_rng.shuffle(cand);
        if (cand.size() > 6) cand.resize(6);
        g_jobs = cand;
    }
    drawBackground("Vous êtes licencié");
    drawTextCentered("Le président a mis fin à votre contrat. Des clubs vous proposent de prendre leur équipe :", VW / 2, 40, 10, C_TXT);
    int s = listRun(g_jobsLW, (int)g_jobs.size() + 1, 60, 60, VW - 120, 8, 16, [&](int i, int x, int y, bool sel) {
        if (i == (int)g_jobs.size()) { drawTextCentered("Quitter la carrière", x + (VW - 120) / 2, y + 3, 10, sel ? BLACK : C_TXT, false); return; }
        int t = g_jobs[i];
        drawKitIcon(g_world.teams[t].home, x + 4, y + 3, 1);
        drawTextPx(fitText(g_world.teams[t].name, 260, 10), x + 18, y + 3, 10, sel ? BLACK : C_TXT);
        drawTextPx(fitText(g_career.teamLevelName(t), 220, 10), x + 290, y + 3, 10, sel ? BLACK : C_DIM);
    });
    g_noBackBtn = false;
    drawFooter("OK : accepter l'offre");
    if (s < 0) return;
    if (s == (int)g_jobs.size()) { g_jobs.clear(); g_careerActive = false; g_screen = SC_MAIN; return; }
    int t = g_jobs[s];
    g_jobs.clear();
    g_career.changeClub(t);
    g_hubNotice = "Nouveau défi : " + g_world.teams[t].name + " !";
    levelCacheDirty();
    openHub();
}
