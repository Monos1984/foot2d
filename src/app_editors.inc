// ------------------------------------------------------------------ messagerie, éditeurs de sponsors et de managers, à propos, mode championnat
static const char* GAME_VERSION = "00.02.01";
static const int GAME_BUILD = 6;          // incrémenté à chaque livraison

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
    if (s.rfind("[U19]", 0) == 0 || s.rfind("[U17]", 0) == 0 || s.rfind("[Réserve]", 0) == 0 || s.find("Gambardella") != std::string::npos) { cat = "JEUNES"; c = Color{ 150, 230, 200, 255 }; }
    else if (s.rfind("Presse", 0) == 0) { cat = "PRESSE"; c = Color{ 200, 200, 210, 255 }; }
    else if (s.find("Ligue des champions") != std::string::npos || s.find("Coupe UEFA") != std::string::npos || s.find("Intertoto") != std::string::npos) { cat = "EUROPE"; c = Color{ 120, 150, 255, 255 }; }
    else if (s.rfind("Prime", 0) == 0 || s.find("Budget") != std::string::npos || s.find("EUR") != std::string::npos) { cat = "FINANCES"; c = Color{ 110, 230, 120, 255 }; }
    else if (s.find("Objectif") != std::string::npos || s.find("président") != std::string::npos || s.find("Président") != std::string::npos) { cat = "PRÉSIDENT"; c = Color{ 255, 210, 90, 255 }; }
    else if (s.find("transf") != std::string::npos || s.find("rejoint") != std::string::npos || s.find("quitte") != std::string::npos || s.find("recrut") != std::string::npos) { cat = "MERCATO"; c = Color{ 120, 190, 255, 255 }; }
    else if (s.find("bless") != std::string::npos || s.find("retraite") != std::string::npos || s.find("formation") != std::string::npos) { cat = "EFFECTIF"; c = Color{ 255, 150, 120, 255 }; }
    else { cat = "INFO"; c = Color{ 170, 180, 220, 255 }; }
}
static int g_newsFilter = 0;
static const char* NEWS_FILTERS[] = { "Tout", "Mon club", "Europe", "Jeunes", "Mercato", "Finances", "Président", "Infos" };
static bool newsMatch(const std::string& m, int f) {
    if (f == 0) return true;
    std::string cat; Color cc; newsCategory(m, cat, cc);
    if (f == 1) { const Team& U = g_world.teams[std::max(0, g_career.userTeam)]; return m.rfind("[Votre club]", 0) == 0 || m.find(U.name) != std::string::npos || m.find(U.shortName) != std::string::npos || cat == "PRÉSIDENT" || cat == "FINANCES"; }
    static const char* C[] = { "", "", "EUROPE", "JEUNES", "MERCATO", "FINANCES", "PRÉSIDENT", "" };
    if (f == 7) return cat == "INFO" || cat == "PRESSE" || cat == "EFFECTIF";
    return cat == C[f];
}
// page de journal : bandeau, titre (première phrase), corps
static void drawNewsPaper(int x, int y, int w, int h, const std::string& m) {
    std::string cat; Color cc; newsCategory(m, cat, cc);
    DrawRectangle(x + 3, y + 3, w, h, Color{ 0, 0, 0, 90 });
    DrawRectangle(x, y, w, h, Color{ 238, 232, 214, 255 });
    DrawRectangle(x, y, w, 24, Color{ 28, 28, 34, 255 });
    drawTextCentered("LE FIL DU CLUB", x + w / 2, y + 2, 20, Color{ 238, 232, 214, 255 }, false);
    DrawRectangle(x + 6, y + 27, w - 12, 1, Color{ 60, 60, 60, 255 });
    drawTextPx(fmt("Saison %d-%d", g_career.year, g_career.year + 1), x + 8, y + 29, 10, Color{ 60, 60, 60, 255 });
    std::string ed = std::string("Édition ") + (g_career.userTeam >= 0 ? g_world.teams[g_career.userTeam].shortName : std::string(""));
    drawTextPx(ed, x + w - 8 - textWidth(ed, 10), y + 29, 10, Color{ 60, 60, 60, 255 });
    DrawRectangle(x + 6, y + 40, w - 12, 1, Color{ 60, 60, 60, 255 });
    DrawRectangle(x + 8, y + 45, textWidth(cat, 10) + 10, 12, cc);
    drawTextPx(cat, x + 13, y + 46, 10, BLACK);
    // titre : jusqu'au premier « : » ou « . », sinon début du texte
    std::string body = m;
    if (body.rfind("[Votre club] ", 0) == 0) body = body.substr(13);
    std::string head = body; size_t cut = body.find(" : ");
    if (cut != std::string::npos && cut < 60) { head = body.substr(0, cut); body = body.substr(cut + 3); }
    else { size_t d = body.find(". "); if (d != std::string::npos && d < 70) { head = body.substr(0, d + 1); body = body.substr(d + 2); } else { head = body; body.clear(); } }
    int yy = y + 62;
    {
        int fs = 20; auto hl = wrapTextSz(head, w - 20, 20);
        if (hl.size() > 2) { fs = 10; hl = wrapText(head, w - 20); }
        for (auto& l : hl) { drawTextPx(l, x + 10, yy, fs, Color{ 10, 10, 10, 255 }); yy += fs == 20 ? 22 : 13; }
    }
    DrawRectangle(x + 10, yy + 1, 40, 2, cc); yy += 8;
    if (body.size() < 50) {
        for (auto& l : wrapText(body, w - 20)) { if (yy >= y + h - 14) break; drawTextPx(l, x + 10, yy, 10, Color{ 35, 35, 35, 255 }); yy += 13; }
    } else {
        // capitale décorée
        std::string first = body.substr(0, 1);
        if ((unsigned char)body[0] < 0x80) { drawTextPx(first, x + 10, yy, 20, Color{ 120, 30, 30, 255 }); }
        int lx = (unsigned char)body[0] < 0x80 ? x + 26 : x + 10;
        std::string rest = (unsigned char)body[0] < 0x80 ? body.substr(1) : body;
        auto ls = wrapText(rest, x + w - 10 - lx);
        for (size_t i = 0; i < ls.size() && yy < y + h - 14; i++) { drawTextPx(ls[i], i < 2 ? lx : x + 10, yy, 10, Color{ 35, 35, 35, 255 }); yy += 13; }
    }
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
        drawNewsPaper(60, 36, VW - 120, VH - 106, m);
        DrawRectangle(60, VH - 72, VW - 120, 1, cc);
        drawTextPx("De : " + (cat == "PRESSE" ? std::string("Revue de presse") : cat == "PRÉSIDENT" ? std::string("Le président du club") : cat == "FINANCES" ? std::string("Direction financière") : cat == "MERCATO" ? std::string("Cellule de recrutement") : cat == "EFFECTIF" ? std::string("Staff médical et sportif") : std::string("Secrétariat du club")), 62, VH - 68, 10, C_DIM);
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
    // onglets de filtre
    {
        int x = 8;
        for (int f = 0; f < 8; f++) {
            int w = textWidth(NEWS_FILTERS[f], 10) + 12;
            if (button(x, 30, w, 14, NEWS_FILTERS[f], g_newsFilter == f)) { g_newsFilter = f; g_newsLW = ListW(); }
            x += w + 3;
        }
        if (IsKeyPressed(KEY_TAB) || IN.right) { g_newsFilter = (g_newsFilter + 1) % 8; g_newsLW = ListW(); }
        if (IN.left) { g_newsFilter = (g_newsFilter + 7) % 8; g_newsLW = ListW(); }
    }
    std::vector<int> idx;                       // messages filtrés (du plus récent au plus ancien)
    for (int i = (int)n.size() - 1; i >= 0; i--) if (newsMatch(n[i], g_newsFilter)) idx.push_back(i);
    int lw = 330;
    int sel = listRun(g_newsLW, (int)idx.size(), 8, 50, lw, 12, 23, [&](int i, int x, int y, bool s) {
        const std::string& m = n[idx[i]];
        bool rd = newsIsRead(m);
        std::string cat; Color cc; newsCategory(m, cat, cc);
        DrawRectangle(x, y, 4, 22, cc);
        // enveloppe
        int ex = x + 8, ey = y + 4;
        DrawRectangle(ex, ey, 12, 9, rd ? Color{ 90, 100, 130, 255 } : Color{ 240, 236, 220, 255 });
        DrawLine(ex, ey, ex + 6, ey + 5, rd ? Color{ 60, 70, 100, 255 } : Color{ 120, 110, 90, 255 });
        DrawLine(ex + 12, ey, ex + 6, ey + 5, rd ? Color{ 60, 70, 100, 255 } : Color{ 120, 110, 90, 255 });
        if (!rd) DrawCircle(ex + 12, ey, 2, Color{ 230, 60, 50, 255 });
        drawTextPx(cat, x + 26, y + 1, 10, s ? BLACK : cc);
        std::string t = m; if (t.rfind("[Votre club] ", 0) == 0) t = t.substr(13);
        auto ls = wrapText(t, lw - 34);
        drawTextPx(ls.empty() ? "" : fitText(ls[0], lw - 34, 10), x + 26, y + 11, 10, s ? BLACK : rd ? C_DIM : C_TXT);
        if (ls.size() > 1) drawTextPx("...", x + lw - 22, y + 1, 10, s ? BLACK : C_DIM);
    });
    if (idx.empty()) drawTextCentered("Aucun message", 8 + lw / 2, 80, 10, C_DIM);
    // aperçu du message sélectionné
    if (!idx.empty() && g_newsLW.cur >= 0 && g_newsLW.cur < (int)idx.size()) drawNewsPaper(348, 50, VW - 356, VH - 92, n[idx[g_newsLW.cur]]);
    else { DrawRectangle(348, 50, VW - 356, VH - 92, Color{ 20, 30, 56, 200 }); drawTextCentered("Le fil du club", 348 + (VW - 356) / 2, 150, 20, C_DIM, false); }
    if (button(348, VH - 38, 96, 14, "Tout lu")) for (auto& m : n) newsMarkRead(m);
    if (button(448, VH - 38, 96, 14, "Effacer lus")) {
        n.erase(std::remove_if(n.begin(), n.end(), [](const std::string& m) { return newsIsRead(m); }), n.end());
        g_newsLW = ListW();
    }
    if (button(548, VH - 38, 84, 14, "Tout effacer")) { askConfirm("Effacer tous les messages ?", []() { g_career.season.news.clear(); g_newsLW = ListW(); }); }
    if (IsKeyPressed(KEY_DELETE) && !idx.empty() && g_newsLW.cur < (int)idx.size()) { n.erase(n.begin() + idx[g_newsLW.cur]); return; }
    drawFooter("Gauche/Droite ou Tab : rubrique   OK / clic : lire   Suppr : effacer   Retour");
    if (sel >= 0) { g_newsOpen = idx[sel]; newsMarkRead(n[idx[sel]]); return; }
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
        if (b3) { g_world.teams[g_career.userTeam].sponsor = S.name; if (g_career.mgr.sponsorIncome == 0) g_career.mgr.sponsorIncome = std::max<int64_t>(2, g_career.mgr.incomeBase / 12); if (g_career.mgr.sponsorYears == 0) g_career.mgr.sponsorYears = 3; toast("Nouveau sponsor maillot : " + S.name); }
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
struct ManagerDef { std::string name; std::string nation = "FRA"; int skin = 0, hair = 0, age = 45; int bday = 1, bmonth = 1; std::string place; int diploma = 0; int gender = 0; };
static const char* DIPLOMAS[] = { "UEFA Pro", "UEFA A (DES)", "UEFA B (BEF)", "Sans diplôme" };
static std::vector<ManagerDef> g_managers;
static bool g_managersLoaded = false;
static int g_managerSel = -1;         // manager choisi pour la prochaine carrière
static int g_mgReturn = -1;           // écran à rouvrir après la création obligatoire d'un manager
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
        if (p.size() > 2) m.skin = atoi(p[2].c_str()) % NUM_SKINS;
        if (p.size() > 3) m.hair = atoi(p[3].c_str()) % NUM_HAIRS;
        if (p.size() > 4) m.age = std::max(25, std::min(85, atoi(p[4].c_str())));
        if (p.size() > 8) { m.bday = std::max(1, std::min(31, atoi(p[5].c_str()))); m.bmonth = std::max(1, std::min(12, atoi(p[6].c_str()))); m.place = p[7]; m.diploma = atoi(p[8].c_str()) & 3; }
        if (p.size() > 9) m.gender = atoi(p[9].c_str()) ? 1 : 0;
        if (!m.name.empty()) g_managers.push_back(m);
    }
    fclose(f);
}
static void saveManagers() {
    FILE* f = fopen("managers.txt", "w");
    if (!f) return;
    fprintf(f, "# managers : nom|pays (code)|peau (0-3)|cheveux (0-4)|âge|jour|mois|lieu de naissance|diplôme (0 Pro, 1 A, 2 B, 3 aucun)|genre (0 homme, 1 femme)\n");
    for (auto& m : g_managers) fprintf(f, "%s|%s|%d|%d|%d|%d|%d|%s|%d|%d\n", m.name.c_str(), m.nation.c_str(), m.skin, m.hair, m.age, m.bday, m.bmonth, m.place.c_str(), m.diploma, m.gender);
    fclose(f);
}
static void applyManager(int i) {
    loadManagers();
    if (i < 0 || i >= (int)g_managers.size()) { g_career.managerName.clear(); g_career.managerNation = -1; return; }
    const ManagerDef& m = g_managers[i];
    g_career.managerName = m.name; g_career.managerNation = g_world.nationIndex(m.nation.c_str());
    g_career.managerSkin = (uint8_t)m.skin; g_career.managerHair = (uint8_t)m.hair; g_career.managerAge = m.age;
    g_career.managerBday = (uint8_t)m.bday; g_career.managerBmonth = (uint8_t)m.bmonth; g_career.managerPlace = m.place; g_career.managerGender = (uint8_t)m.gender;
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
            std::string("Couleur de peau : ") + SKIN_NAMES[M.skin % NUM_SKINS], std::string("Cheveux : ") + HAIR_NAMES[M.hair % NUM_HAIRS], fmt("Âge : %d ans", M.age),
            fmt("Jour de naissance : %d", M.bday), std::string("Mois de naissance : ") + MONTHS_FR2[std::max(1, std::min(12, M.bmonth))],
            "Lieu de naissance : " + (M.place.empty() && !(g_mgTyping && g_mgField == 7) ? std::string("(non renseigné)") : M.place) + (g_mgTyping && g_mgField == 7 && ((int)(GetTime() * 2)) % 2 ? "_" : ""),
            std::string("Diplôme d'entraîneur : ") + DIPLOMAS[M.diploma & 3],
            std::string("Genre : ") + (M.gender ? "Femme" : "Homme"),
            g_mgEdit == g_managerSel ? "Manager choisi pour la carrière (OK pour retirer)" : "Choisir ce manager pour la carrière",
            "Supprimer ce manager", "Terminé" };
        // portrait
        Kit suit; suit.shirt = 0x1B2A63; suit.shirt2 = 0x1B2A63; suit.shorts = 0x202020; suit.socks = 0x101010;
        drawPortrait(470, 50, 120, M.skin, M.hair, M.gender, suit, hashStr(M.name));
        if (ni >= 0) drawKitIcon(g_world.teams[ni].home, 476, 56, 2);
        if (g_mgTyping) {
            textEdit(g_mgField == 7 ? M.place : M.name, 40);
            listRun(g_mgLW, (int)rows.size(), 40, 50, 400, (int)rows.size(), 18, [&](int i, int x, int y, bool sel) { drawTextPx(rows[i], x + 6, y + 4, 10, sel ? BLACK : C_TXT); }, false);
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
            case 2: M.skin = (M.skin + d + NUM_SKINS) % NUM_SKINS; break;
            case 3: M.hair = (M.hair + d + NUM_HAIRS) % NUM_HAIRS; break;
            case 4: M.age = std::max(25, std::min(85, M.age + d)); break;
            case 5: M.bday = (M.bday - 1 + d + 31) % 31 + 1; break;
            case 6: M.bmonth = (M.bmonth - 1 + d + 12) % 12 + 1; break;
            case 8: M.diploma = (M.diploma + d + 4) % 4; break;
            case 9: M.gender = !M.gender; break;
            }
            saveManagers();
        }
        if (s == 0 || s == 7) { g_mgField = s; g_mgTyping = true; g_textMode = true; while (GetCharPressed() > 0) {} }
        if (s == 9) { M.gender = !M.gender; saveManagers(); }
        if (s == 10) { g_managerSel = g_managerSel == g_mgEdit ? -1 : g_mgEdit; }
        if (s == 11) { g_managers.erase(g_managers.begin() + g_mgEdit); if (g_managerSel == g_mgEdit) g_managerSel = -1; g_mgEdit = -1; saveManagers(); return; }
        drawFooter("Gauche/Droite : modifier   OK : valider   Retour");
        if (s == 12 || IN.back) { saveManagers(); g_mgEdit = -1; g_mgLW = ListW(); }
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
    if (IN.back) { if (g_mgReturn >= 0) { g_screen = (Screen)g_mgReturn; g_mgReturn = -1; } else g_screen = SC_EDITMENU; }
}

// carrière (club ou sélection) : un entraîneur / manager est obligatoire
static bool requireManager(Screen back) {
    loadManagers();
    if (g_managerSel >= 0 && g_managerSel < (int)g_managers.size()) return true;
    if (!g_managers.empty()) { g_managerSel = 0; toast("Manager : " + g_managers[0].name + " (modifiable dans les options de carrière)"); return true; }
    g_managers.push_back(ManagerDef()); g_managers.back().name = "Nouveau manager";
    g_managerSel = (int)g_managers.size() - 1;
    g_mgEdit = g_managerSel; g_mgField = 0; g_mgTyping = true; g_textMode = true; g_mgLW = ListW(); while (GetCharPressed() > 0) {}
    saveManagers();
    g_mgReturn = (int)back;
    g_screen = SC_MANAGERS;
    toast("Un entraîneur est obligatoire : créez votre manager");
    return false;
}

// ---------------- à propos
static void screenAbout() {
    drawBackground("À propos");
    int y = 50;
    drawTextCentered("SUPER SOCCER WORLD", VW / 2, y, 20, C_HI); y += 22;
    drawTextCentered("Jouez. Gérez. Vivez le football", VW / 2, y, 10, C_TXT); y += 14;
    drawTextCentered(std::string("Version ") + GAME_VERSION + fmt("  -  build %d", GAME_BUILD), VW / 2, y, 10, C_DIM); y += 20;
    const char* L[] = { "Jeu gratuit réalisé par l'IA Claude,", "piloté par Jean Monos pour Offgame.", "",
                        "Jeu non licencié : fan jeu, sans lien avec les ligues, fédérations et clubs cités.",
                        "Musiques et sons originaux synthétisés par le jeu.", "",
                        "Données : EA SPORTS FC 26 DataHub (joueurs), INSEE (communes), Wikipédia (palmarès).", "",
                        "Soutenir le jeu : fr.tipeee.com/le-bazar-de-monos  (touche T)" };
    for (auto l : L) { drawTextCentered(l, VW / 2, y, 10, C_TXT); y += 14; }
    // petit joueur qui jongle
    static float t = 0; t += GetFrameTime();
    Kit k; k.shirt = 0x1B2A63; k.shorts = 0xFFFFFF; k.socks = 0xE2001A;
    drawPlayerSprite(VW / 2, VH - 50, k, 1, 0, 0, (int)(t * 6), PS_NORMAL, false, 0, 3);
    DrawCircle(VW / 2 + 4, VH - 60 - (int)(std::fabs(std::sin(t * 5)) * 18), 3, WHITE);
    drawFooter("Retour    T : ouvrir la page Tipeee");
    if (IsKeyPressed(KEY_T)) OpenURL(TIPEEE_URL);
    if (IN.back || IN.ok) g_screen = SC_MAIN;
}

// ---------------- mode championnat (mode foot, allégé) : moteur de la carrière sans gestion, 1 à 4 clubs contrôlés,
// toutes les divisions du pays (montées / descentes), coupes nationales et continentales, transferts simplifiés
static ListW g_lmLW;
static int g_lmPyr = -1;
static void screenLeagueMode() {
    drawBackground("Championnat (mode foot)");
    struct LI { std::string name, right; int pyr; int grp; };
    std::vector<LI> v;
    static const char* CFN[NUM_CONFEDS] = { "UEFA", "CONMEBOL", "CONCACAF", "CAF", "AFC", "OFC" };
    for (int p = 0; p < (int)g_basePyramids.size(); p++) {
        const Pyramid& P = g_basePyramids[p];
        if (P.dom >= 0 || P.country == "U19" || P.country == "U17" || P.country == "U15") continue;
        int q = P.poolIndex(0, 0);
        if (q < 0 || P.pools[q].clubs.size() < 4) continue;
        bool fem = isWomenPyramid(P);
        int ni = g_world.nationIndex(P.country.c_str());
        int grp = P.country == "FRA" ? 0 : fem ? 10 : ni >= 0 ? 1 + NATIONS[ni].conf : 9;
        std::string right = fmt("%d division%s", (int)P.tiers.size(), P.tiers.size() > 1 ? "s" : "");
        v.push_back({ std::string(fem ? "[F] " : grp >= 1 && grp <= 6 ? std::string("[") + CFN[grp - 1] + "] " : "") + P.name + "  -  " + P.tiers[0].name, right, p, grp });
    }
    std::stable_sort(v.begin(), v.end(), [](const LI& a, const LI& b) { if (a.grp != b.grp) return a.grp < b.grp; return sortKey(a.name) < sortKey(b.name); });
    drawTextCentered("Hommes (France en tête, puis par fédération) puis femmes. Saison complète, sans gestion : 1 à 4 clubs contrôlés.", VW / 2, 34, 10, C_DIM);
    int s = listRun(g_lmLW, (int)v.size(), 40, 50, VW - 80, 22, 13, [&](int i, int x, int y, bool sel) {
        drawTextPx(fitText(v[i].name, VW - 200, 10), x + 6, y + 1, 10, sel ? BLACK : v[i].grp == 10 ? Color{ 240, 150, 200, 255 } : C_TXT);
        drawTextPx(v[i].right, x + VW - 80 - 90, y + 1, 10, sel ? BLACK : C_DIM);
    });
    drawFooter("OK : choisir vos clubs   Retour");
    if (IN.back) { g_screen = SC_MAIN; return; }
    if (s < 0) return;
    const Pyramid& P = g_basePyramids[v[s].pyr];
    g_lmPyr = v[s].pyr;
    g_intlCandidates.clear();
    int maxTier = P.country == "FRA" ? 2 : 99;          // France : Ligue 1, Ligue 2, Ligue 3
    for (auto& pl : P.pools) if (pl.tier <= maxTier) for (int t : pl.clubs) if (g_world.teams[t].parent < 0) g_intlCandidates.push_back(t);
    g_cdef = CustomCompDef();
    g_cdef.name = P.tiers[0].name;
    g_leagueModePick = true;
    g_intlSel.clear();
    openPick(PM_INTL);
}

static void startLeagueMode() {
    g_leagueModePick = false;
    std::vector<int> ctrl = g_intlSel;
    if (ctrl.size() > 4) ctrl.resize(4);
    if (ctrl.empty()) return;
    g_career.opts = Career::Opts();
    g_career.opts.lite = 1;
    g_career.newClubCareer(ctrl[0], 2026);
    g_career.mgr.managerMode = false; g_career.mgr.noSack = true; g_career.mgr.fullManager = false;
    g_career.season.controlled = ctrl;
    g_careerActive = true; g_needAdvance = true;
    g_hubNotice = "Championnat : saison 2026-27 (" + std::to_string(ctrl.size()) + " club" + (ctrl.size() > 1 ? "s" : "") + " contrôlé" + (ctrl.size() > 1 ? "s" : "") + ")";
    levelCacheDirty();
    openHub();
    autosave();
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
