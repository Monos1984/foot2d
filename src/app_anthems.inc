// ------------------------------------------------------------------ éditeur d'hymnes : écouter, associer un fichier musical à une sélection
// hymnes.txt : CODE|chemin du fichier (à côté du jeu)
static ListW g_anLW, g_anFileLW;
static int g_anSel = -1;                  // sélection choisie (index de nation)
static bool g_anBrowse = false;           // explorateur de fichiers ouvert
static std::string g_anDir;
static bool g_anLoaded = false;

static void loadAnthemFiles() {
    if (g_anLoaded) return;
    g_anLoaded = true;
    FILE* f = fopen("hymnes.txt", "r");
    if (!f) return;
    char buf[1200];
    while (fgets(buf, sizeof buf, f)) {
        std::string l = buf;
        while (!l.empty() && (l.back() == '\n' || l.back() == '\r')) l.pop_back();
        size_t p = l.find('|');
        if (p == std::string::npos || p == 0) continue;
        audioSetAnthemFile(l.substr(0, p), l.substr(p + 1));
    }
    fclose(f);
}
static void saveAnthemFiles() {
    FILE* f = fopen("hymnes.txt", "w");
    if (!f) { toast("Impossible d'enregistrer hymnes.txt"); return; }
    for (auto& kv : audioAnthemFiles()) fprintf(f, "%s|%s\n", kv.first.c_str(), kv.second.c_str());
    fclose(f);
}
static bool isMusicFile(const std::string& p) {
    std::string e = GetFileExtension(p.c_str()) ? GetFileExtension(p.c_str()) : "";
    for (auto& c : e) c = (char)tolower((unsigned char)c);
    return e == ".ogg" || e == ".mp3" || e == ".wav" || e == ".flac" || e == ".qoa" || e == ".xm" || e == ".mod";
}
static std::vector<int> anthemNations() {
    std::vector<int> v;
    for (int i = 0; i < NUM_NATIONS; i++) if (nationEligible(i)) v.push_back(i);
    std::stable_sort(v.begin(), v.end(), [](int a, int b) { return g_world.teams[a].name < g_world.teams[b].name; });
    return v;
}

static void screenAnthems() {
    loadAnthemFiles();
    drawBackground("Éditeur d'hymnes");
    auto nats = anthemNations();
    // fichier glissé-déposé sur la fenêtre : associé à la sélection choisie
    if (IsFileDropped()) {
        FilePathList fl = LoadDroppedFiles();
        if (fl.count > 0 && g_anSel >= 0 && isMusicFile(fl.paths[0])) {
            audioSetAnthemFile(NATIONS[g_anSel].code, fl.paths[0]); saveAnthemFiles(); g_anBrowse = false;
            toast(std::string("Hymne associé : ") + GetFileName(fl.paths[0]));
        } else if (fl.count > 0) toast(g_anSel < 0 ? "Choisissez d'abord une sélection" : "Format non reconnu (OGG, MP3, WAV, FLAC)");
        UnloadDroppedFiles(fl);
    }
    if (g_anSel >= 0 && g_anBrowse) {       // explorateur de fichiers
        if (g_anDir.empty()) g_anDir = DirectoryExists("hymnes") ? std::string(GetWorkingDirectory()) + "/hymnes" : std::string(GetWorkingDirectory());
        struct Ent { std::string label, path; bool dir; };
        std::vector<Ent> ents;
        ents.push_back({ ".. (dossier parent)", GetPrevDirectoryPath(g_anDir.c_str()), true });
        FilePathList fl = LoadDirectoryFiles(g_anDir.c_str());
        std::vector<Ent> dirs, files;
        for (unsigned i = 0; i < fl.count; i++) {
            std::string p = fl.paths[i], nm = GetFileName(fl.paths[i]);
            if (!nm.empty() && nm[0] == '.') continue;
            if (DirectoryExists(p.c_str())) dirs.push_back({ "[" + nm + "]", p, true });
            else if (isMusicFile(p)) files.push_back({ nm, p, false });
        }
        UnloadDirectoryFiles(fl);
        auto byName = [](const Ent& a, const Ent& b) { return a.label < b.label; };
        std::sort(dirs.begin(), dirs.end(), byName); std::sort(files.begin(), files.end(), byName);
        for (auto& e : dirs) ents.push_back(e);
        for (auto& e : files) ents.push_back(e);
        drawTextCentered(fitText("Hymne de " + g_world.teams[g_anSel].name + " : choisir un fichier musical", VW - 20, 10), VW / 2, 30, 10, C_HI);
        drawTextPx(fitText(g_anDir, VW - 40, 10), 20, 42, 10, C_DIM);
        int s = listRun(g_anFileLW, (int)ents.size(), 20, 56, VW - 40, 17, 14, [&](int i, int x, int y, bool sel) {
            drawTextPx(fitText(ents[i].label, VW - 60, 10), x + 6, y + 2, 10, sel ? BLACK : ents[i].dir ? C_HI : C_TXT);
        });
        if (files.empty()) drawTextCentered("Aucun fichier OGG, MP3, WAV ou FLAC dans ce dossier.", VW / 2, VH - 44, 10, C_DIM, false);
        drawFooter("OK : ouvrir / choisir   Retour   (vous pouvez aussi glisser-déposer un fichier sur la fenêtre)");
        if (s >= 0) {
            if (ents[s].dir) { g_anDir = ents[s].path; g_anFileLW = ListW(); }
            else { audioSetAnthemFile(NATIONS[g_anSel].code, ents[s].path); saveAnthemFiles(); g_anBrowse = false; toast("Hymne associé : " + ents[s].label); }
        }
        if (IN.back) g_anBrowse = false;
        return;
    }
    if (g_anSel >= 0) {                     // fiche d'une sélection
        const Team& T = g_world.teams[g_anSel];
        std::string code = NATIONS[g_anSel].code, file = audioAnthemFile(code);
        drawKitIcon(T.home, 40, 44, 3);
        drawTextPx(T.name, 72, 46, 20, C_HI);
        drawTextPx(std::string("Hymne intégré : ") + audioAnthemName(code), 72, 70, 10, C_TXT);
        drawTextPx(file.empty() ? std::string("Fichier musical : aucun (l'hymne intégré est joué)") : fitText("Fichier musical : " + file, VW - 100, 10), 72, 84, 10, file.empty() ? C_DIM : C_GOOD);
        if (!file.empty() && !FileExists(file.c_str())) drawTextPx("Fichier introuvable : l'hymne intégré sera joué.", 72, 96, 10, C_BAD);
        std::vector<std::string> items = { audioAnthemPlaying() ? "Arrêter l'écoute" : "Écouter l'hymne", "Choisir un fichier musical...", "Retirer le fichier (hymne intégré)", "Retour" };
        int s = menuRun(g_anLW, items, 130);
        drawTextCentered("Formats : OGG, MP3, WAV, FLAC. Les hymnes sont joués même si la musique est coupée.", VW / 2, VH - 44, 10, C_DIM, false);
        drawFooter("OK : valider   Retour   (glisser-déposer un fichier sur la fenêtre : l'associer)");
        if (s == 0) { if (audioAnthemPlaying()) audioStopAnthem(); else audioAnthemCode(code); }
        else if (s == 1) { g_anBrowse = true; g_anFileLW = ListW(); }
        else if (s == 2) { audioSetAnthemFile(code, ""); saveAnthemFiles(); toast("Hymne intégré rétabli"); }
        if (s == 3 || IN.back) { audioStopAnthem(); int keep = g_anSel; g_anSel = -1; g_anLW = ListW(); for (int i = 0; i < (int)nats.size(); i++) if (nats[i] == keep) g_anLW.cur = i; }
        return;
    }
    drawTextCentered(fitText("Hymnes réels : France, Angleterre, Allemagne, États-Unis, Russie, Canada ; 6 hymnes originaux pour les autres.", VW - 20, 10), VW / 2, 30, 10, C_DIM, false);
    int s = listRun(g_anLW, (int)nats.size(), 20, 44, VW - 40, 18, 14, [&](int i, int x, int y, bool sel) {
        const Team& T = g_world.teams[nats[i]];
        std::string code = NATIONS[nats[i]].code, file = audioAnthemFile(code);
        drawTextPx(fitText(T.name, 190, 10), x + 6, y + 2, 10, sel ? BLACK : C_TXT);
        drawTextPx(audioAnthemName(code), x + 210, y + 2, 10, sel ? BLACK : strncmp(audioAnthemName(code), "Hymne de la sélection", 21) ? C_HI : C_DIM);
        if (!file.empty()) drawTextPx(fitText(GetFileName(file.c_str()), VW - 40 - 400, 10), x + 390, y + 2, 10, sel ? BLACK : C_GOOD);
    });
    drawFooter("OK : écouter / choisir un fichier   Retour");
    if (s >= 0) { g_anSel = nats[s]; g_anLW = ListW(); }
    if (IN.back) { audioStopAnthem(); g_screen = SC_EDITMENU; }
}
