// ------------------------------------------------------------------ éditeur : musique d'entrée des joueurs par compétition
// musiques_entree.txt : Nom de la compétition|chemin du fichier (à côté du jeu). Jouée quand les équipes entrent sur le terrain.
static std::map<std::string, std::string> g_walkFiles;
static bool g_walkLoaded = false;
static ListW g_wmLW, g_wmFileLW, g_wmMenuLW;
static std::string g_wmName; static bool g_wmBrowse = false; static std::string g_wmDir;
static bool isMusicFile(const std::string& p);
static void loadWalkMusic() {
    if (g_walkLoaded) return;
    g_walkLoaded = true;
    FILE* f = fopen("musiques_entree.txt", "r");
    if (!f) return;
    char buf[1400];
    while (fgets(buf, sizeof buf, f)) {
        std::string l = buf;
        while (!l.empty() && (l.back() == '\n' || l.back() == '\r')) l.pop_back();
        size_t p = l.find('|');
        if (p != std::string::npos && p > 0) g_walkFiles[l.substr(0, p)] = l.substr(p + 1);
    }
    fclose(f);
}
static void saveWalkMusic() {
    FILE* f = fopen("musiques_entree.txt", "w");
    if (!f) { toast("Impossible d'enregistrer musiques_entree.txt"); return; }
    for (auto& kv : g_walkFiles) fprintf(f, "%s|%s\n", kv.first.c_str(), kv.second.c_str());
    fclose(f);
}
// fichier associé à une compétition : nom exact, sinon le plus long nom enregistré qui commence le nom de la compétition
// (« Ligue 1 » couvre « Ligue 1 - Journée 3 », « National 2 » couvre « National 2 - Groupe B »)
static std::string lowerAscii(std::string s) { for (auto& c : s) c = (char)tolower((unsigned char)c); return s; }
static std::string walkMusicFor(const std::string& comp) {
    loadWalkMusic();
    std::string lc = lowerAscii(comp), best; size_t bl = 0;
    for (auto& kv : g_walkFiles) {
        std::string k = lowerAscii(kv.first);
        if (k == lc) return kv.second;
        if (k.size() > bl && lc.compare(0, k.size(), k) == 0) { best = kv.second; bl = k.size(); }
    }
    return best;
}
// compétitions proposées : celles de la carrière en cours, sinon les divisions de chaque pays et les grandes coupes
static std::vector<std::string> walkCompetitions() {
    std::vector<std::string> v;
    auto add = [&](const std::string& n) { if (!n.empty() && std::find(v.begin(), v.end(), n) == v.end()) v.push_back(n); };
    for (auto& kv : g_walkFiles) add(kv.first);
    if (g_careerActive) for (auto& c : g_career.season.comps) add(c.name);
    static const char* CUPS[] = { "Match amical", "Coupe de France", "Coupe de la Ligue", "Trophée des champions", "Ligue des champions", "Ligue Europa", "Ligue Conférence",
                                  "Supercoupe de l'UEFA", "Coupe du monde", "Championnat d'Europe", "Coupe du monde des clubs", "Ligue des nations" };
    for (auto c : CUPS) add(c);
    for (auto& P : g_basePyramids) for (int t = 0; t < (int)P.tiers.size() && t < 3; t++) add(P.tiers[t].name);
    std::stable_sort(v.begin() + 0, v.end(), [](const std::string& a, const std::string& b) { bool fa = g_walkFiles.count(a), fb = g_walkFiles.count(b); return fa != fb ? fa : false; });
    return v;
}

static void screenWalkMusic() {
    loadWalkMusic();
    drawBackground("Musiques d'entrée");
    auto comps = walkCompetitions();
    int g_wmSel = -1; for (int i = 0; i < (int)comps.size(); i++) if (!g_wmName.empty() && comps[i] == g_wmName) g_wmSel = i;
    if (IsFileDropped()) {
        FilePathList fl = LoadDroppedFiles();
        if (fl.count > 0 && g_wmSel >= 0 && g_wmSel < (int)comps.size() && isMusicFile(fl.paths[0])) { g_walkFiles[comps[g_wmSel]] = fl.paths[0]; saveWalkMusic(); g_wmBrowse = false; toast(std::string("Musique associée : ") + GetFileName(fl.paths[0])); }
        else if (fl.count > 0) toast(g_wmSel < 0 ? "Choisissez d'abord une compétition" : "Format non reconnu (OGG, MP3, WAV, FLAC)");
        UnloadDroppedFiles(fl);
    }
    if (g_wmSel >= 0 && g_wmSel < (int)comps.size() && g_wmBrowse) {      // explorateur de fichiers
        if (g_wmDir.empty()) g_wmDir = DirectoryExists("musiques") ? std::string(GetWorkingDirectory()) + "/musiques" : std::string(GetWorkingDirectory());
        struct Ent { std::string label, path; bool dir; };
        std::vector<Ent> ents, dirs, files;
        ents.push_back({ ".. (dossier parent)", GetPrevDirectoryPath(g_wmDir.c_str()), true });
        FilePathList fl = LoadDirectoryFiles(g_wmDir.c_str());
        for (unsigned i = 0; i < fl.count; i++) {
            std::string p = fl.paths[i], nm = GetFileName(fl.paths[i]);
            if (nm.empty() || nm[0] == '.') continue;
            if (DirectoryExists(p.c_str())) dirs.push_back({ "[" + nm + "]", p, true }); else if (isMusicFile(p)) files.push_back({ nm, p, false });
        }
        UnloadDirectoryFiles(fl);
        auto byName = [](const Ent& a, const Ent& b) { return a.label < b.label; };
        std::sort(dirs.begin(), dirs.end(), byName); std::sort(files.begin(), files.end(), byName);
        for (auto& e : dirs) ents.push_back(e);
        for (auto& e : files) ents.push_back(e);
        drawTextCentered(fitText(comps[g_wmSel] + " : choisir la musique d'entrée", VW - 20, 10), VW / 2, 30, 10, C_HI);
        drawTextPx(fitText(g_wmDir, VW - 40, 10), 20, 42, 10, C_DIM);
        int s = listRun(g_wmFileLW, (int)ents.size(), 20, 56, VW - 40, 17, 14, [&](int i, int x, int y, bool sel) { drawTextPx(fitText(ents[i].label, VW - 60, 10), x + 6, y + 2, 10, sel ? BLACK : ents[i].dir ? C_HI : C_TXT); });
        if (files.empty()) drawTextCentered("Aucun fichier OGG, MP3, WAV ou FLAC dans ce dossier.", VW / 2, VH - 44, 10, C_DIM, false);
        drawFooter("OK : ouvrir / choisir   Retour   (glisser-déposer un fichier sur la fenêtre)");
        if (s >= 0) { if (ents[s].dir) { g_wmDir = ents[s].path; g_wmFileLW = ListW(); } else { g_walkFiles[comps[g_wmSel]] = ents[s].path; saveWalkMusic(); g_wmBrowse = false; toast("Musique associée : " + ents[s].label); } }
        if (IN.back) g_wmBrowse = false;
        return;
    }
    if (g_wmSel >= 0 && g_wmSel < (int)comps.size()) {                    // fiche d'une compétition
        const std::string& name = comps[g_wmSel];
        auto it = g_walkFiles.find(name);
        std::string file = it == g_walkFiles.end() ? std::string() : it->second;
        DrawRectangle(20, 40, VW - 40, 64, Color{ 6, 12, 28, 200 }); DrawRectangle(20, 40, 4, 64, C_HI);
        drawTextPx(fitText(name, VW - 70, 20), 34, 48, 20, C_HI);
        drawTextPx(file.empty() ? std::string("Aucune musique : entrée des joueurs avec le bruit du stade.") : fitText("Musique : " + file, VW - 70, 10), 34, 74, 10, file.empty() ? C_DIM : C_GOOD);
        if (!file.empty() && !FileExists(file.c_str())) drawTextPx("Fichier introuvable.", 34, 88, 10, C_BAD);
        std::vector<std::string> items = { audioWalkoutPlaying() ? "Arrêter l'écoute" : "Écouter la musique", "Choisir un fichier musical...", "Retirer la musique", "Retour" };
        int s = menuRun(g_wmMenuLW, items, 124);
        drawTextCentered("La musique démarre quand les équipes sortent du tunnel et s'efface avant les hymnes ou le coup d'envoi.", VW / 2, VH - 56, 10, C_DIM, false);
        drawTextCentered("Le nom vaut aussi pour les groupes et les journées de la compétition (« Ligue 1 » couvre « Ligue 1 - Groupe A »).", VW / 2, VH - 44, 10, C_DIM, false);
        drawFooter("OK : valider   Retour   (glisser-déposer un fichier sur la fenêtre : l'associer)");
        if (s == 0) { if (audioWalkoutPlaying()) audioWalkoutStop(); else if (!file.empty()) audioWalkoutPlay(file); else toast("Aucune musique associée"); }
        else if (s == 1) { g_wmBrowse = true; g_wmFileLW = ListW(); }
        else if (s == 2) { g_walkFiles.erase(name); saveWalkMusic(); audioWalkoutStop(); toast("Musique retirée"); }
        if (s == 3 || IN.back) { audioWalkoutStop(); g_wmName.clear(); g_wmMenuLW = ListW(); }
        return;
    }
    drawTextCentered("Choisissez une compétition : sa musique sera jouée à l'entrée des joueurs.", VW / 2, 30, 10, C_DIM, false);
    int s = listRun(g_wmLW, (int)comps.size(), 20, 44, VW - 40, 18, 14, [&](int i, int x, int y, bool sel) {
        auto it = g_walkFiles.find(comps[i]);
        DrawRectangle(x + 4, y + 3, 6, 9, sel ? BLACK : it != g_walkFiles.end() ? C_GOOD : Color{ 70, 80, 100, 255 });
        drawTextPx(fitText(comps[i], 280, 10), x + 16, y + 2, 10, sel ? BLACK : C_TXT);
        if (it != g_walkFiles.end()) drawTextPx(fitText(GetFileName(it->second.c_str()), VW - 40 - 320, 10), x + 310, y + 2, 10, sel ? BLACK : C_GOOD);
    });
    drawFooter("OK : choisir   Retour");
    if (s >= 0) { g_wmName = comps[s]; g_wmMenuLW = ListW(); }
    if (IN.back) { audioWalkoutStop(); g_screen = SC_EDITMENU; }
}
