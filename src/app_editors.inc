// ------------------------------------------------------------------ messagerie, éditeurs de sponsors et de managers, à propos, mode championnat
static const char* GAME_VERSION = "00.02.00";

// ---------------- messagerie : lire, effacer, non lus
static int g_newsOpen = -1;           // index (dans season.news) du message ouvert
static bool newsIsRead(const std::string& s) {
    auto& r = g_career.newsRead;
    return std::find(r.begin(), r.end(), s) != r.end();
}
static void newsMarkRead(const std::string& s) {
    if (newsIsRead(s)) return;
    g_career.newsRead.push_back(s);
    if (g_career.newsRead.size() > 600) g_career.newsRead.erase(g_career.newsRead.begin(), g_career.newsRead.begin() + 200);
}
static int newsUnread() {
    int n = 0; for (auto& s : g_career.season.news) if (!newsIsRead(s)) n++;
    return n;
}
static void newsCategory(const std::string& s, std::string& cat, Color& c) {
    if (s.rfind("Presse", 0) == 0) { cat = "PRESSE"; c = Color{ 200, 200, 210, 255 }; }
    else if (s.rfind("Prime", 0) == 0 || s.find("Budget") != std::string::npos || s.find("EUR") != std::string::npos) { cat = "FINANCES"; c = Color{ 110, 230, 120, 255 }; }
    else if (s.find("Objectif") != std::string::npos || s.find("président") != std::string::npos || s.find("Président") != std::string::npos) { cat = "PRÉSIDENT"; c = Color{ 255, 210, 90, 255 }; }
    else if (s.find("transf") != std::string::npos || s.find("rejoint") != std::string::npos || s.find("quitte") != std::string::npos || s.find("recrut") != std::string::npos) { cat = "MERCATO"; c = Color{ 120, 190, 255, 255 }; }
    else if (s.find("bless") != std::string::npos || s.find("retraite") != std::string::npos || s.find("formation") != std::string::npos) { cat = "EFFECTIF"; c = Color{ 255, 150, 120, 255 }; }
    else { cat = "INFO"; c = Color{ 170, 180, 220, 255 }; }
}
static void screenNews() {
    auto& n = g_career.season.news;
    if (g_newsOpen >= (int)n.size()) g_newsOpen = -1;
    if (g_newsOpen >= 0) {
        // lecture d'un message
        drawBackground("Message");
        const std::string& m = n[g_newsOpen];
        newsMarkRead(m);
        std::string cat; Color cc; newsCategory(m, cat, cc);
        DrawRectangle(30, 40, VW - 60, VH - 80, Color{ 236, 232, 220, 255 });
        DrawRectangle(30, 40, VW - 60, 18, cc);
        drawTextPx("De : " + (cat == "PRESSE" ? std::string("Revue de presse") : cat == "PRÉSIDENT" ? std::string("Le président du club") : cat == "FINANCES" ? std::string("Direction financière") : cat == "MERCATO" ? std::string("Cellule de recrutement") : cat == "EFFECTIF" ? std::string("Staff médical et sportif") : std::string("Secrétariat du club")), 38, 44, 10, BLACK);
        drawTextPx(cat, VW - 38 - textWidth(cat, 10), 44, 10, BLACK);
        int y = 66;
        for (auto& l : wrapText(m, VW - 90)) { drawTextPx(l, 42, y, 10, Color{ 30, 30, 30, 255 }); y += 13; }
        bool del = button(VW / 2 - 160, VH - 60, 150, 16, "Effacer ce message");
        bool back = button(VW / 2 + 10, VH - 60, 150, 16, "Retour à la liste");
        drawFooter("Suppr : effacer   Gauche/Droite : message précédent/suivant   Retour");
        if (del || IsKeyPressed(KEY_DELETE)) { n.erase(n.begin() + g_newsOpen); g_newsOpen = -1; return; }
        if (IN.left && g_newsOpen + 1 < (int)n.size()) g_newsOpen++;
        if (IN.right && g_newsOpen > 0) g_newsOpen--;
        if (back || IN.back || IN.ok) g_newsOpen = -1;
        return;
    }
    drawBackground(fmt("Messages (%d non lu%s)", newsUnread(), newsUnread() > 1 ? "s" : ""));
    int sel = listRun(g_newsLW, (int)n.size(), 12, 50, VW - 24, 22, 12, [&](int i, int x, int y, bool s) {
        const std::string& m = n[n.size() - 1 - i];
        bool rd = newsIsRead(m);
        std::string cat; Color cc; newsCategory(m, cat, cc);
        DrawRectangle(x + 2, y + 2, 6, 7, rd ? Color{ 90, 100, 130, 255 } : cc);      // enveloppe
        if (!rd) DrawRectangle(x + 3, y + 3, 4, 1, BLACK);
        drawTextPx(cat, x + 12, y + 1, 10, s ? BLACK : cc);
        drawTextPx(fitText(m, VW - 130, 10), x + 82, y + 1, 10, s ? BLACK : rd ? C_DIM : C_TXT);
    });
    if (n.empty()) drawTextCentered("Aucun message", VW / 2, 80, 10, C_DIM);
    if (button(12, 32, 150, 14, "Tout marquer comme lu")) for (auto& m : n) newsMarkRead(m);
    if (button(166, 32, 150, 14, "Effacer les messages lus")) {
        n.erase(std::remove_if(n.begin(), n.end(), [](const std::string& m) { return newsIsRead(m); }), n.end());
        g_newsLW = ListW();
    }
    if (button(320, 32, 120, 14, "Tout effacer")) { askConfirm("Effacer tous les messages ?", []() { g_career.season.news.clear(); g_newsLW = ListW(); }); }
    if (IsKeyPressed(KEY_DELETE) && !n.empty() && g_newsLW.cur < (int)n.size()) { n.erase(n.begin() + (n.size() - 1 - g_newsLW.cur)); return; }
    drawFooter("OK / clic : lire   Suppr : effacer   Retour");
    if (sel >= 0) { g_newsOpen = (int)n.size() - 1 - sel; return; }
    if (IN.back) openHub();
}

// ---------------- sponsors (sponsors.txt : nom|chemin de l'image)
struct SponsorDef { std::string name, image; };
static std::vector<SponsorDef> g_sponsors;
static bool g_sponsorsLoaded = false;
static std::map<std::string, Texture2D> g_logoTex;
static void loadSponsors() {
    if (g_sponsorsLoaded) return;
    g_sponsorsLoaded = true;
    FILE* f = fopen("sponsors.txt", "r");
    if (!f) return;
    char buf[1024];
    while (fgets(buf, sizeof buf, f)) {
        std::string l = buf; while (!l.empty() && (l.back() == '\n' || l.back() == '\r')) l.pop_back();
        if (l.empty() || l[0] == '#') continue;
        size_t p = l.find('|');
        SponsorDef s; s.name = l.substr(0, p); if (p != std::string::npos) s.image = l.substr(p + 1);
        if (!s.name.empty()) g_sponsors.push_back(s);
    }
    fclose(f);
}
static void saveSponsors() {
    FILE* f = fopen("sponsors.txt", "w");
    if (!f) return;
    fprintf(f, "# sponsors : nom|chemin d'une image (PNG ou JPG)\n");
    for (auto& s : g_sponsors) fprintf(f, "%s|%s\n", s.name.c_str(), s.image.c_str());
    fclose(f);
}
// logo d'un sponsor (texture en cache) ; id 0 si pas d'image
static Texture2D* sponsorLogo(const std::string& name) {
    loadSponsors();
    for (auto& s : g_sponsors) if (s.name == name && !s.image.empty()) {
        auto it = g_logoTex.find(s.image);
        if (it == g_logoTex.end()) {
            Texture2D t{}; 
            if (FileExists(s.image.c_str())) { Image im = LoadImage(s.image.c_str()); if (im.data) { t = LoadTextureFromImage(im); UnloadImage(im); } }
            it = g_logoTex.insert({ s.image, t }).first;
        }
        return it->second.id ? &it->second : nullptr;
    }
    return nullptr;
}
static void drawLogo(Texture2D* t, int x, int y, int w, int h) {
    if (!t || !t->id) return;
    float k = std::min((float)w / t->width, (float)h / t->height);
    int dw = (int)(t->width * k), dh = (int)(t->height * k);
    DrawTexturePro(*t, Rectangle{ 0, 0, (float)t->width, (float)t->height }, Rectangle{ (float)(x + (w - dw) / 2), (float)(y + (h - dh) / 2), (float)dw, (float)dh }, Vector2{ 0, 0 }, 0, WHITE);
}
static ListW g_spLW;
static int g_spEdit = -1, g_spField = 0; static bool g_spTyping = false;
static void screenSponsors() {
    loadSponsors();
    drawBackground("Éditeur de sponsors");
    if (g_spEdit >= 0 && g_spEdit < (int)g_sponsors.size()) {
        SponsorDef& S = g_sponsors[g_spEdit];
        drawTextPx("Nom du sponsor :", 40, 50, 10, g_spField == 0 ? C_HI : C_DIM);
        DrawRectangle(40, 62, 360, 16, g_spField == 0 ? C_SEL : C_ITEM);
        drawTextPx(S.name + (g_spTyping && g_spField == 0 && ((int)(GetTime() * 2)) % 2 ? "_" : ""), 44, 65, 10, g_spField == 0 ? BLACK : C_TXT);
        drawTextPx("Image (chemin d'un fichier PNG ou JPG, ex. C:\\\\logos\\\\sponsor.png) :", 40, 88, 10, g_spField == 1 ? C_HI : C_DIM);
        DrawRectangle(40, 100, 560, 16, g_spField == 1 ? C_SEL : C_ITEM);
        drawTextPx(fitText(S.image + (g_spTyping && g_spField == 1 && ((int)(GetTime() * 2)) % 2 ? "_" : ""), 550, 10), 44, 103, 10, g_spField == 1 ? BLACK : C_TXT);
        // aperçu
        DrawRectangle(420, 40, 180, 56, Color{ 240, 240, 240, 255 });
        Texture2D* t = sponsorLogo(S.name);
        if (t) drawLogo(t, 424, 44, 172, 48);
        else drawTextCentered(S.image.empty() ? "(pas d'image)" : "(image introuvable)", 510, 62, 10, Color{ 90, 90, 90, 255 }, false);
        if (g_spTyping) {
            textEdit(g_spField == 0 ? S.name : S.image, g_spField == 0 ? 40 : 300);
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_ESCAPE)) { g_spTyping = false; g_textMode = false; g_logoTex.erase(S.image); saveSponsors(); }
            drawFooter("Tapez le texte   Entrée : valider");
            return;
        }
        bool b1 = button(40, 130, 180, 16, "Modifier le nom"), b2 = button(230, 130, 180, 16, "Modifier l'image");
        bool b3 = g_careerActive && g_career.kind == CK_CLUB && button(420, 130, 180, 16, "Sponsor de mon club");
        bool b4 = button(40, 154, 180, 16, "Supprimer ce sponsor"), b5 = button(230, 154, 180, 16, "Terminé");
        if (IN.up || IN.down) g_spField ^= 1;
        if (b1 || (IN.ok && g_spField == 0)) { g_spField = 0; g_spTyping = true; g_textMode = true; while (GetCharPressed() > 0) {} }
        if (b2 || (IN.ok && g_spField == 1)) { g_spField = 1; g_spTyping = true; g_textMode = true; while (GetCharPressed() > 0) {} }
        if (b3) { g_world.teams[g_career.userTeam].sponsor = S.name; if (g_career.mgr.sponsorIncome == 0) g_career.mgr.sponsorIncome = std::max<int64_t>(2, g_career.mgr.incomeBase / 12); toast("Nouveau sponsor maillot : " + S.name); }
        if (b4) { g_sponsors.erase(g_sponsors.begin() + g_spEdit); g_spEdit = -1; saveSponsors(); return; }
        drawTextPx("Le logo s'affiche sur la fiche du club, dans l'accueil de la carrière et sur les panneaux du stade.", 40, 184, 10, C_DIM);
        drawFooter("Haut/Bas : champ   OK : modifier   Retour");
        if (b5 || IN.back) { saveSponsors(); g_spEdit = -1; }
        return;
    }
    int n = (int)g_sponsors.size();
    int s = listRun(g_spLW, n + 1, 40, 40, VW - 80, 20, 14, [&](int i, int x, int y, bool sel) {
        if (i == n) { drawTextPx("+ Nouveau sponsor", x + 6, y + 2, 10, sel ? BLACK : C_HI); return; }
        drawTextPx(g_sponsors[i].name, x + 6, y + 2, 10, sel ? BLACK : C_TXT);
        drawTextPx(fitText(g_sponsors[i].image.empty() ? "-" : g_sponsors[i].image, 300, 10), x + 240, y + 2, 10, sel ? BLACK : C_DIM);
        Texture2D* t = sponsorLogo(g_sponsors[i].name);
        if (t) drawLogo(t, x + VW - 80 - 40, y, 36, 12);
    });
    drawFooter("OK : modifier / créer   Retour");
    if (s == n) { g_sponsors.push_back({ "Nouveau sponsor", "" }); g_spEdit = n; g_spField = 0; g_spTyping = true; g_textMode = true; while (GetCharPressed() > 0) {} }
    else if (s >= 0) { g_spEdit = s; g_spField = 0; }
    if (IN.back) g_screen = SC_EDITMENU;
}

// ---------------- managers (managers.txt : nom|code pays|peau|cheveux|âge)
static const char* MONTHS_FR2[] = { "", "janvier", "février", "mars", "avril", "mai", "juin", "juillet", "août", "septembre", "octobre", "novembre", "décembre" };
struct ManagerDef { std::string name; std::string nation = "FRA"; int skin = 0, hair = 0, age = 45; int bday = 1, bmonth = 1; std::string place; int diploma = 0; };
static const char* DIPLOMAS[] = { "UEFA Pro", "UEFA A (DES)", "UEFA B (BEF)", "Sans diplôme" };
static std::vector<ManagerDef> g_managers;
static bool g_managersLoaded = false;
static int g_managerSel = -1;         // manager choisi pour la prochaine carrière
static void loadManagers() {
    if (g_managersLoaded) return;
    g_managersLoaded = true;
    FILE* f = fopen("managers.txt", "r");
    if (!f) return;
    char buf[512];
    while (fgets(buf, sizeof buf, f)) {
        std::string l = buf; while (!l.empty() && (l.back() == '\n' || l.back() == '\r')) l.pop_back();
        if (l.empty() || l[0] == '#') continue;
        std::vector<std::string> p; size_t a = 0;
        while (true) { size_t b = l.find('|', a); p.push_back(l.substr(a, b == std::string::npos ? std::string::npos : b - a)); if (b == std::string::npos) break; a = b + 1; }
        ManagerDef m; m.name = p[0];
        if (p.size() > 1) m.nation = p[1];
        if (p.size() > 2) m.skin = atoi(p[2].c_str()) % 4;
        if (p.size() > 3) m.hair = atoi(p[3].c_str()) % 5;
        if (p.size() > 4) m.age = std::max(25, std::min(85, atoi(p[4].c_str())));
        if (p.size() > 8) { m.bday = std::max(1, std::min(31, atoi(p[5].c_str()))); m.bmonth = std::max(1, std::min(12, atoi(p[6].c_str()))); m.place = p[7]; m.diploma = atoi(p[8].c_str()) & 3; }
        if (!m.name.empty()) g_managers.push_back(m);
    }
    fclose(f);
}
static void saveManagers() {
    FILE* f = fopen("managers.txt", "w");
    if (!f) return;
    fprintf(f, "# managers : nom|pays (code)|peau (0-3)|cheveux (0-4)|âge|jour|mois|lieu de naissance|diplôme (0 Pro, 1 A, 2 B, 3 aucun)\n");
    for (auto& m : g_managers) fprintf(f, "%s|%s|%d|%d|%d|%d|%d|%s|%d\n", m.name.c_str(), m.nation.c_str(), m.skin, m.hair, m.age, m.bday, m.bmonth, m.place.c_str(), m.diploma);
    fclose(f);
}
static void applyManager(int i) {
    loadManagers();
    if (i < 0 || i >= (int)g_managers.size()) { g_career.managerName.clear(); g_career.managerNation = -1; return; }
    const ManagerDef& m = g_managers[i];
    g_career.managerName = m.name; g_career.managerNation = g_world.nationIndex(m.nation.c_str());
    g_career.managerSkin = (uint8_t)m.skin; g_career.managerHair = (uint8_t)m.hair; g_career.managerAge = m.age;
    g_career.managerBday = (uint8_t)m.bday; g_career.managerBmonth = (uint8_t)m.bmonth; g_career.managerPlace = m.place;
}
static ListW g_mgLW;
static int g_mgEdit = -1; static bool g_mgTyping = false; static int g_mgField = 0;
static void screenManagers() {
    loadManagers();
    drawBackground("Éditeur de managers");
    if (g_mgEdit >= 0 && g_mgEdit < (int)g_managers.size()) {
        ManagerDef& M = g_managers[g_mgEdit];
        int ni = g_world.nationIndex(M.nation.c_str());
        std::vector<std::string> rows = {
            "Nom : " + M.name + (g_mgTyping && g_mgField == 0 && ((int)(GetTime() * 2)) % 2 ? "_" : ""),
            "Nationalité : " + (ni >= 0 ? g_world.teams[ni].name : M.nation),
            fmt("Teint : %d", M.skin + 1), fmt("Cheveux : %d", M.hair + 1), fmt("Âge : %d ans", M.age),
            fmt("Jour de naissance : %d", M.bday), std::string("Mois de naissance : ") + MONTHS_FR2[std::max(1, std::min(12, M.bmonth))],
            "Lieu de naissance : " + (M.place.empty() && !(g_mgTyping && g_mgField == 7) ? std::string("(non renseigné)") : M.place) + (g_mgTyping && g_mgField == 7 && ((int)(GetTime() * 2)) % 2 ? "_" : ""),
            std::string("Diplôme d'entraîneur : ") + DIPLOMAS[M.diploma & 3],
            g_mgEdit == g_managerSel ? "Manager choisi pour la carrière (OK pour retirer)" : "Choisir ce manager pour la carrière",
            "Supprimer ce manager", "Terminé" };
        // portrait
        Kit suit; suit.shirt = 0x1B2A63; suit.shirt2 = 0x1B2A63; suit.shorts = 0x202020; suit.socks = 0x101010;
        DrawRectangle(470, 50, 120, 130, Color{ 30, 50, 90, 255 });
        drawPlayerSprite(530, 170, suit, M.skin, M.hair, 0, 0, PS_NORMAL, false, 0, 8);
        if (ni >= 0) drawKitIcon(g_world.teams[ni].home, 476, 56, 2);
        if (g_mgTyping) {
            textEdit(g_mgField == 7 ? M.place : M.name, 40);
            menuRun(g_mgLW, rows, 50, 400);
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_ESCAPE)) { g_mgTyping = false; g_textMode = false; saveManagers(); }
            drawFooter("Tapez le nom   Entrée : valider");
            return;
        }
        int s = listRun(g_mgLW, (int)rows.size(), 40, 50, 400, (int)rows.size(), 18, [&](int i, int x, int y, bool sel) { drawTextPx(rows[i], x + 6, y + 4, 10, sel ? BLACK : C_TXT); });
        int d = IN.left ? -1 : IN.right ? 1 : 0;
        if (d) {
            switch (g_mgLW.cur) {
            case 1: { std::vector<int> nat; for (int i = 0; i < NUM_NATIONS; i++) nat.push_back(i);
                      int k = 0; for (int i = 0; i < (int)nat.size(); i++) if (nat[i] == ni) k = i;
                      k = (k + d + (int)nat.size()) % (int)nat.size(); M.nation = NATIONS[nat[k]].code; break; }
            case 2: M.skin = (M.skin + d + 4) % 4; break;
            case 3: M.hair = (M.hair + d + 5) % 5; break;
            case 4: M.age = std::max(25, std::min(85, M.age + d)); break;
            case 5: M.bday = (M.bday - 1 + d + 31) % 31 + 1; break;
            case 6: M.bmonth = (M.bmonth - 1 + d + 12) % 12 + 1; break;
            case 8: M.diploma = (M.diploma + d + 4) % 4; break;
            }
            saveManagers();
        }
        if (s == 0 || s == 7) { g_mgField = s; g_mgTyping = true; g_textMode = true; while (GetCharPressed() > 0) {} }
        if (s == 9) { g_managerSel = g_managerSel == g_mgEdit ? -1 : g_mgEdit; }
        if (s == 10) { g_managers.erase(g_managers.begin() + g_mgEdit); if (g_managerSel == g_mgEdit) g_managerSel = -1; g_mgEdit = -1; saveManagers(); return; }
        drawFooter("Gauche/Droite : modifier   OK : valider   Retour");
        if (s == 11 || IN.back) { saveManagers(); g_mgEdit = -1; g_mgLW = ListW(); }
        return;
    }
    int n = (int)g_managers.size();
    int s = listRun(g_mgLW, n + 1, 40, 40, VW - 80, 20, 14, [&](int i, int x, int y, bool sel) {
        if (i == n) { drawTextPx("+ Nouveau manager", x + 6, y + 2, 10, sel ? BLACK : C_HI); return; }
        const ManagerDef& m = g_managers[i];
        int ni = g_world.nationIndex(m.nation.c_str());
        if (ni >= 0) drawKitIcon(g_world.teams[ni].home, x + 4, y + 2, 1);
        drawTextPx(m.name + (i == g_managerSel ? "   (choisi pour la carrière)" : ""), x + 18, y + 2, 10, sel ? BLACK : i == g_managerSel ? C_GOOD : C_TXT);
        drawTextPx(fmt("%d ans", m.age), x + VW - 150, y + 2, 10, sel ? BLACK : C_DIM);
    });
    drawFooter("OK : modifier / créer   Le manager choisi est utilisé au lancement d'une carrière   Retour");
    if (s == n) { g_managers.push_back(ManagerDef()); g_managers.back().name = "Nouveau manager"; g_mgEdit = n; g_mgField = 0; g_mgTyping = true; g_textMode = true; g_mgLW = ListW(); while (GetCharPressed() > 0) {} saveManagers(); }
    else if (s >= 0) { g_mgEdit = s; g_mgLW = ListW(); }
    if (IN.back) g_screen = SC_EDITMENU;
}

// ---------------- à propos
static void screenAbout() {
    drawBackground("À propos");
    int y = 50;
    drawTextCentered("FRANCE FOOT 2D", VW / 2, y, 20, C_HI); y += 26;
    drawTextCentered(std::string("Version ") + GAME_VERSION, VW / 2, y, 10, C_DIM); y += 20;
    const char* L[] = { "Jeu gratuit réalisé par l'IA Claude,", "piloté par Jean Monos pour Offgame.", "",
                        "Jeu non licencié : fan jeu, sans lien avec les ligues, fédérations et clubs cités.",
                        "Musiques et sons originaux synthétisés par le jeu.", "",
                        "Données : EA SPORTS FC 26 DataHub (joueurs), INSEE (communes), Wikipédia (palmarès)." };
    for (auto l : L) { drawTextCentered(l, VW / 2, y, 10, C_TXT); y += 14; }
    // petit joueur qui jongle
    static float t = 0; t += GetFrameTime();
    Kit k; k.shirt = 0x1B2A63; k.shorts = 0xFFFFFF; k.socks = 0xE2001A;
    drawPlayerSprite(VW / 2, VH - 50, k, 1, 0, 0, (int)(t * 6), PS_NORMAL, false, 0, 3);
    DrawCircle(VW / 2 + 4, VH - 60 - (int)(std::fabs(std::sin(t * 5)) * 18), 3, WHITE);
    drawFooter("Retour");
    if (IN.back || IN.ok) g_screen = SC_MAIN;
}

// ---------------- mode championnat : les premières divisions d'Europe, sans mode manager
static ListW g_lmLW;
static void screenLeagueMode() {
    drawBackground("Championnat");
    struct LI { std::string name; int pyr; };
    std::vector<LI> v;
    for (int p = 0; p < (int)g_basePyramids.size(); p++) {
        const Pyramid& P = g_basePyramids[p];
        if (P.dom >= 0) continue;
        int q = P.poolIndex(0, 0);
        if (q < 0 || P.pools[q].clubs.size() < 4) continue;
        int ni = g_world.nationIndex(P.country.c_str());
        v.push_back({ P.tiers[0].name + "  (" + (ni >= 0 ? g_world.teams[ni].name : P.country) + ")", p });
    }
    drawTextCentered("Choisissez une première division : saison complète, sans gestion (1 ou plusieurs clubs).", VW / 2, 34, 10, C_DIM);
    int s = listRun(g_lmLW, (int)v.size(), 80, 50, VW - 160, 20, 13, [&](int i, int x, int y, bool sel) {
        const Pyramid& P = g_basePyramids[v[i].pyr];
        int q = P.poolIndex(0, 0);
        drawTextPx(v[i].name, x + 6, y + 1, 10, sel ? BLACK : C_TXT);
        drawTextPx(fmt("%d clubs", (int)P.pools[q].clubs.size()), x + VW - 160 - 70, y + 1, 10, sel ? BLACK : C_DIM);
    });
    drawFooter("OK : choisir vos clubs   Retour");
    if (IN.back) { g_screen = SC_MAIN; return; }
    if (s < 0) return;
    const Pyramid& P = g_basePyramids[v[s].pyr];
    int q = P.poolIndex(0, 0);
    g_cdef = CustomCompDef();
    g_cdef.name = P.tiers[0].name + fmt(" %d-%02d", 2026, 27);
    g_cdef.format = 0; g_cdef.legs = 2;
    g_cdef.teams = P.pools[q].clubs;
    g_customSel = g_cdef.teams;
    g_leagueModePick = true;
    g_intlCandidates = g_cdef.teams;
    g_intlSel.clear();
    openPick(PM_INTL);
}

static void startLeagueMode() {
    g_leagueModePick = false;
    std::vector<int> ctrl = g_intlSel;
    if (ctrl.size() > 4) ctrl.resize(4);
    g_career.newCustom(g_cdef, ctrl);
    g_careerActive = true; g_needAdvance = true;
    g_hubNotice = g_cdef.name + " - saison complète";
    openHub();
}

static void managerCycle(int d) {
    loadManagers();
    int n = (int)g_managers.size();
    g_managerSel = ((g_managerSel + 1 + d) % (n + 1) + (n + 1)) % (n + 1) - 1;
}
static std::string managerChoiceName() {
    loadManagers();
    if (g_managerSel < 0 || g_managerSel >= (int)g_managers.size()) return g_managers.empty() ? "anonyme (créez-le dans Éditeurs > managers)" : "anonyme";
    return g_managers[g_managerSel].name;
}
static void applyChosenManager() { applyManager(g_managerSel); }
static Texture2D* sponsorLogoFwd(const std::string& name) { return sponsorLogo(name); }
static void drawLogo2(Texture2D* t, int x, int y, int w, int h) { drawLogo(t, x, y, w, h); }
static std::string sponsorImagePath(const std::string& name) { loadSponsors(); for (auto& sp : g_sponsors) if (sp.name == name) return sp.image; return ""; }
