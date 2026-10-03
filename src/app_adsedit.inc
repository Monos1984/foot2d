// ------------------------------------------------------------------ éditeur de publicités (images)
// pubs.txt : PANNEAU|chemin  ou  TV|chemin  (à côté du jeu)
// PANNEAU : panneaux publicitaires autour du terrain (image affichée sur 69 x 10 pixels du terrain) ; taille conseillée 276 x 40 px (7:1)
// TV      : page de publicité plein écran avant le match et à la mi-temps ; taille conseillée 640 x 360 px (16:9)
struct CustomAd { bool tv = false; std::string path; };
static std::vector<CustomAd> g_customAds;
static bool g_adsLoaded = false;
static ListW g_aeLW, g_aeFileLW, g_aeMenuLW;
static int g_aeSel = -1, g_aeBrowse = -1;     // g_aeBrowse : -1 non, 0 panneau, 1 TV
static std::string g_aeDir;
static std::map<std::string, Texture2D> g_adTex;
static bool isImageFile(const std::string& p) {
    std::string e = GetFileExtension(p.c_str()) ? GetFileExtension(p.c_str()) : "";
    for (auto& c : e) c = (char)tolower((unsigned char)c);
    return e == ".png" || e == ".jpg" || e == ".jpeg" || e == ".bmp";
}
static void applyCustomAds() {
    std::vector<std::string> boards;
    for (auto& a : g_customAds) if (!a.tv) boards.push_back(a.path);
    setCustomBoardAds(boards);
}
static void loadCustomAds() {
    if (g_adsLoaded) return;
    g_adsLoaded = true;
    FILE* f = fopen("pubs.txt", "r");
    if (f) {
        char buf[1400];
        while (fgets(buf, sizeof buf, f)) {
            std::string l = buf;
            while (!l.empty() && (l.back() == '\n' || l.back() == '\r')) l.pop_back();
            size_t p = l.find('|'); if (p == std::string::npos) continue;
            CustomAd a; a.tv = l.substr(0, p) == "TV"; a.path = l.substr(p + 1);
            if (!a.path.empty()) g_customAds.push_back(a);
        }
        fclose(f);
    }
    applyCustomAds();
}
static void saveCustomAds() {
    FILE* f = fopen("pubs.txt", "w");
    if (!f) { toast("Impossible d'enregistrer pubs.txt"); return; }
    for (auto& a : g_customAds) fprintf(f, "%s|%s\n", a.tv ? "TV" : "PANNEAU", a.path.c_str());
    fclose(f);
    applyCustomAds();
}
static Texture2D* adTexture(const std::string& path) {
    auto it = g_adTex.find(path);
    if (it != g_adTex.end()) return it->second.id ? &it->second : nullptr;
    Texture2D t = {};
    if (FileExists(path.c_str())) { t = LoadTexture(path.c_str()); if (t.id) SetTextureFilter(t, TEXTURE_FILTER_POINT); }
    g_adTex[path] = t;
    return t.id ? &g_adTex[path] : nullptr;
}
static std::vector<std::string> customTvAds() { loadCustomAds(); std::vector<std::string> v; for (auto& a : g_customAds) if (a.tv) v.push_back(a.path); return v; }
// image ajustée dans un cadre en conservant ses proportions
static void drawFitted(Texture2D* t, int x, int y, int w, int h) {
    if (!t) return;
    float k = std::min((float)w / t->width, (float)h / t->height);
    float dw = t->width * k, dh = t->height * k;
    DrawTexturePro(*t, Rectangle{ 0, 0, (float)t->width, (float)t->height }, Rectangle{ x + (w - dw) / 2, y + (h - dh) / 2, dw, dh }, Vector2{ 0, 0 }, 0, WHITE);
}

static void screenAdsEdit() {
    loadCustomAds();
    drawBackground("Publicités (images)");
    // glisser-déposer : ajouté selon la rubrique ouverte (panneau par défaut)
    if (IsFileDropped()) {
        FilePathList fl = LoadDroppedFiles();
        for (unsigned i = 0; i < fl.count; i++) if (isImageFile(fl.paths[i])) { CustomAd a; a.tv = g_aeBrowse == 1; a.path = fl.paths[i]; g_customAds.push_back(a); toast(std::string("Image ajoutée : ") + GetFileName(fl.paths[i])); }
        UnloadDroppedFiles(fl); saveCustomAds(); g_aeBrowse = -1;
    }
    if (g_aeBrowse >= 0) {      // explorateur de fichiers
        if (g_aeDir.empty()) g_aeDir = DirectoryExists("pubs") ? std::string(GetWorkingDirectory()) + "/pubs" : std::string(GetWorkingDirectory());
        struct Ent { std::string label, path; bool dir; };
        std::vector<Ent> ents, dirs, files;
        ents.push_back({ ".. (dossier parent)", GetPrevDirectoryPath(g_aeDir.c_str()), true });
        FilePathList fl = LoadDirectoryFiles(g_aeDir.c_str());
        for (unsigned i = 0; i < fl.count; i++) {
            std::string p = fl.paths[i], nm = GetFileName(fl.paths[i]);
            if (nm.empty() || nm[0] == '.') continue;
            if (DirectoryExists(p.c_str())) dirs.push_back({ "[" + nm + "]", p, true }); else if (isImageFile(p)) files.push_back({ nm, p, false });
        }
        UnloadDirectoryFiles(fl);
        auto byName = [](const Ent& a, const Ent& b) { return a.label < b.label; };
        std::sort(dirs.begin(), dirs.end(), byName); std::sort(files.begin(), files.end(), byName);
        for (auto& e : dirs) ents.push_back(e);
        for (auto& e : files) ents.push_back(e);
        drawTextCentered(g_aeBrowse == 1 ? "Page de pub TV : taille conseillée 640 x 360 px (16:9)" : "Panneau du stade : taille conseillée 276 x 40 px (7:1)", VW / 2, 30, 10, C_HI);
        drawTextPx(fitText(g_aeDir, 360, 10), 20, 42, 10, C_DIM);
        int s = listRun(g_aeFileLW, (int)ents.size(), 20, 56, 360, 16, 14, [&](int i, int x, int y, bool sel) { drawTextPx(fitText(ents[i].label, 340, 10), x + 6, y + 2, 10, sel ? BLACK : ents[i].dir ? C_HI : C_TXT); });
        // aperçu de l'image survolée
        int cur = std::max(0, std::min((int)ents.size() - 1, g_aeFileLW.cur));
        if (!ents.empty() && !ents[cur].dir) {
            Texture2D* t = adTexture(ents[cur].path);
            DrawRectangle(392, 56, 234, 140, Color{ 6, 12, 28, 220 });
            if (t) { drawFitted(t, 396, 60, 226, 132); drawTextCentered(fmt("%d x %d px", t->width, t->height), 509, 202, 10, C_TXT, false); }
        }
        if (files.empty()) drawTextCentered("Aucune image PNG, JPG ou BMP dans ce dossier.", VW / 2, VH - 44, 10, C_DIM, false);
        drawFooter("OK : ouvrir / choisir   Retour   (glisser-déposer une image sur la fenêtre)");
        if (s >= 0) {
            if (ents[s].dir) { g_aeDir = ents[s].path; g_aeFileLW = ListW(); }
            else { CustomAd a; a.tv = g_aeBrowse == 1; a.path = ents[s].path; g_customAds.push_back(a); saveCustomAds(); toast("Image ajoutée : " + ents[s].label); g_aeBrowse = -1; }
        }
        if (IN.back) g_aeBrowse = -1;
        return;
    }
    if (g_aeSel >= 0 && g_aeSel < (int)g_customAds.size()) {     // fiche d'une publicité
        CustomAd& a = g_customAds[g_aeSel];
        Texture2D* t = adTexture(a.path);
        drawTextPx(a.tv ? "PAGE DE PUB TV" : "PANNEAU DU STADE", 20, 34, 10, C_HI);
        drawTextPx(fitText(a.path, VW - 40, 10), 20, 48, 10, C_DIM);
        // aperçu dans les proportions réelles d'affichage
        int pw = a.tv ? 320 : 420, ph = a.tv ? 180 : 60, px = VW / 2 - pw / 2, py = 66;
        DrawRectangle(px - 2, py - 2, pw + 4, ph + 4, Color{ 230, 230, 240, 255 }); DrawRectangle(px, py, pw, ph, a.tv ? BLACK : Color{ 244, 244, 244, 255 });
        if (t) drawFitted(t, px, py, pw, ph); else drawTextCentered("Image introuvable", VW / 2, py + ph / 2 - 5, 10, C_BAD, false);
        int ty = py + ph + 10;
        if (t) {
            float ratio = (float)t->width / std::max(1, t->height), want = a.tv ? 16.f / 9.f : 6.9f;
            drawTextCentered(fmt("Image : %d x %d px  (rapport %.2f, conseillé %.2f)", t->width, t->height, ratio, want), VW / 2, ty, 10, std::fabs(ratio - want) < want * 0.2f ? C_GOOD : C_HI, false);
        }
        drawTextCentered(a.tv ? "Taille conseillée : 640 x 360 px (ou 1280 x 720). Affichée en plein écran pendant les pauses publicitaires."
                              : "Taille conseillée : 276 x 40 px (rapport 7:1). Affichée sur les panneaux autour du terrain (69 x 10 pixels).", VW / 2, ty + 14, 10, C_DIM, false);
        std::vector<std::string> items = { a.tv ? "Utiliser comme panneau du stade" : "Utiliser comme page de pub TV", "Retirer cette publicité", "Retour" };
        int s = menuRun(g_aeMenuLW, items, ty + 34);
        drawFooter("OK : valider   Retour");
        if (s == 0) { a.tv = !a.tv; saveCustomAds(); }
        else if (s == 1) { g_customAds.erase(g_customAds.begin() + g_aeSel); saveCustomAds(); g_aeSel = -1; toast("Publicité retirée"); return; }
        if (s == 2 || IN.back) { g_aeSel = -1; g_aeMenuLW = ListW(); }
        return;
    }
    drawTextCentered("Images affichées sur les panneaux du stade et pendant les pages de publicité des matchs télévisés.", VW / 2, 30, 10, C_DIM, false);
    int n = (int)g_customAds.size();
    int s = listRun(g_aeLW, n + 3, 20, 46, 340, 13, 18, [&](int i, int x, int y, bool sel) {
        if (i == 0) { drawTextPx("+ Ajouter une image de panneau (276 x 40 px)", x + 6, y + 4, 10, sel ? BLACK : C_GOOD); return; }
        if (i == 1) { drawTextPx("+ Ajouter une page de pub TV (640 x 360 px)", x + 6, y + 4, 10, sel ? BLACK : C_GOOD); return; }
        if (i == n + 2) { drawTextPx("Retour", x + 6, y + 4, 10, sel ? BLACK : C_TXT); return; }
        const CustomAd& a = g_customAds[i - 2];
        drawTextPx(a.tv ? "TV" : "PAN.", x + 6, y + 4, 10, sel ? BLACK : C_HI);
        drawTextPx(fitText(GetFileName(a.path.c_str()), 270, 10), x + 52, y + 4, 10, sel ? BLACK : C_TXT);
    });
    // aperçu de la ligne choisie
    int cur = g_aeLW.cur;
    DrawRectangle(372, 46, 254, 234, Color{ 6, 12, 28, 220 });
    if (cur >= 2 && cur < n + 2) {
        const CustomAd& a = g_customAds[cur - 2];
        Texture2D* t = adTexture(a.path);
        if (t) { drawFitted(t, 378, 52, 242, a.tv ? 136 : 36); drawTextCentered(fmt("%d x %d px", t->width, t->height), 499, a.tv ? 194 : 96, 10, C_TXT, false); }
    } else {
        int ty = 56;
        for (auto& l : wrapText("Panneaux : rapport 7:1, par exemple 276 x 40 px (affiché sur 69 x 10 pixels du terrain, un panneau sur deux). Pages TV : 640 x 360 px ou 1280 x 720 px, ajoutées aux publicités de la chaîne. Formats PNG, JPG ou BMP. Les images peuvent être rangées dans un dossier « pubs » à côté du jeu.", 236)) { drawTextPx(l, 380, ty, 10, C_TXT); ty += 12; }
    }
    drawFooter("OK : choisir   Retour   (glisser-déposer des images sur la fenêtre : panneaux)");
    if (s == 0 || s == 1) { g_aeBrowse = s; g_aeFileLW = ListW(); }
    else if (s >= 2 && s < n + 2) { g_aeSel = s - 2; g_aeMenuLW = ListW(); }
    if (s == n + 2 || IN.back) g_screen = SC_EDITMENU;
}
