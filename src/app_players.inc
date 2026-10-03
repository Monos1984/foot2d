// Fiche joueur (infos personnelles, caractéristiques, historique), éditeur de joueurs, consignes tactiques
static int g_plTeam = -1, g_plIdx = -1;
static Screen g_plBack = SC_MAIN;
static ListW g_plLW;static bool g_psychArchive=false;

static void openPlayer(int team, int idx, Screen back) {
    if (team < 0 || team >= (int)g_world.teams.size()) return;
    g_world.ensureSquad(team);
    if (idx < 0 || idx >= (int)g_world.teams[team].squad.size()) return;
    g_plTeam = team; g_plIdx = idx;g_psychArchive=false; g_plBack = back; g_plLW = ListW();
    g_screen = SC_PLAYER;
}

static const char* MONTHS_FR[] = { "", "janvier", "février", "mars", "avril", "mai", "juin", "juillet", "août", "septembre", "octobre", "novembre", "décembre" };
static int curYear() { return g_careerActive ? g_career.year : 2026; }

static void attrBar(int x, int y, const char* lbl, int v) {
    drawTextPx(lbl, x, y, 10, C_DIM);
    DrawRectangle(x + 72, y + 3, 60, 5, Color{ 0, 0, 0, 120 });
    DrawRectangle(x + 72, y + 3, v * 60 / 100, 5, v >= 75 ? C_GOOD : v >= 50 ? C_HI : C_BAD);
    drawTextPx(fmt("%d", v), x + 138, y, 10, C_TXT);
}

static bool playerNavigation(int selected){
 if(!g_careerActive||!careerRules())return false;
 const char* labels[]={"Fiche générale","Postes / apprentissage","Personnalité"};
 int count=g_career.personalityEnabled()?3:2,w=(VW-32)/count;
 for(int i=0;i<count;i++){if(i==selected){DrawRectangle(12+i*(w+4),32,w,19,C_SEL);drawTextCentered(labels[i],12+i*(w+4)+w/2,37,10,BLACK);continue;}if(button(12+i*(w+4),32,w,19,labels[i])&&i!=selected){g_rolePlayerTab=i;g_psychArchive=false;return true;}}
 return false;
}

#include "app_roles.inc"
#include "app_personality.inc"

static void screenPlayer() {
    if (g_plTeam < 0 || g_plTeam >= (int)g_world.teams.size() || g_plIdx >= (int)g_world.teams[g_plTeam].squad.size()) { g_screen = g_plBack; return; }
    Team& T = g_world.teams[g_plTeam];
    Player& p = T.squad[g_plIdx];
    if(g_careerActive&&careerRules()){if(IN.tab)g_rolePlayerTab=(g_rolePlayerTab+1)%(g_career.personalityEnabled()?3:2);if(g_rolePlayerTab==2){screenPlayerPersonality(p,T);return;}if(g_rolePlayerTab==1){screenPlayerPositions(p);return;}}
    drawBackground("Fiche joueur");
    bool tabs=g_careerActive&&careerRules();if(playerNavigation(0))return;int head=tabs?64:31;
    drawKitIcon(T.home, 12, head+1, 2);
    drawTextPx(fitText(fmt("%d. ", p.num) + p.name, 490, 20), 34, head, 20, C_HI);
    drawPortrait(VW - 76, head, 64, p.skin, p.hair, p.gender, T.home, (unsigned)p.id);
    drawTextPx(T.name + "  -  " + (p.pos == POS_GK ? "Gardien" : p.pos == POS_DF ? "Défenseur" : p.pos == POS_MF ? "Milieu" : "Attaquant"), 34, head+21, 10, C_TXT);
    int d, m, y; std::string place;
    playerBirth(p, curYear(), d, m, y, place, T.culture, T.dept);
    int x = 12, yy = head+37;
    auto line = [&](const std::string& k, const std::string& v) { drawTextPx(k, x, yy, 10, C_DIM); drawTextPx(fitText(v, 200, 10), x + 96, yy, 10, C_TXT); yy += 12; };
    line("Âge", fmt("%d ans", p.age));
    line("Genre", p.gender ? "Femme" : "Homme");
    line("Né le", fmt("%d %s %d", d, MONTHS_FR[std::max(1, std::min(12, m))], y));
    line("Lieu de naissance", place);
    int nat = p.nation >= 0 ? p.nation : T.nation;
    line("Nationalité", nat >= 0 && nat < NUM_NATIONS ? sanitize(NATIONS[nat].name) : std::string("-"));
    static const char* CT[] = { "Professionnel", "Semi-professionnel", "Licence amateur" };
    if(g_careerActive&&careerRules()){line("Contrat", CT[p.contract % 3]);
    line("Valeur", money(p.value()));
    if (p.contract < 2) line("Salaire", money(playerWage(p)) + " / an");
    line("Potentiel", fmt("%d", p.pot));
    line("Condition / moral", fmt("%d / %d", playerCond(g_plTeam, p), p.morale));}else line("Énergie",fmt("%d / 100",playerCond(g_plTeam,p)));
    // caractéristiques
    int ax = 320, ay = head+76;
    drawTextPx(fmt("NOTE %d", p.overall()), ax, head+60, 10, C_HI);
    struct A { const char* n; int v; } at[] = { { "Vitesse", p.speed }, { "Tir", p.shoot }, { "Passe", p.pass }, { "Tacle", p.tackle }, { "Dribble", p.drib() },
                                                 { "Jeu de tête", p.head() }, { "Placement", p.posi() }, { "Sang-froid", p.comp() }, { "Endurance", p.stamina }, { "Gardien", p.keep } };
    for (int i = 0; i < 10; i++) attrBar(ax + (i / 5) * 160, ay + (i % 5) * 12, at[i].n, at[i].v);
    // historique
    int hy = tabs?228:202;
    drawTextPx("Carrière", 12, hy, 10, C_HI);
    hy += 12;
    const int cx[] = { 16, 84, 320, 370, 416, 470, 520 };
    const char* hd[] = { "Saison", "Club", "Matchs", "Buts", "P. déc.", "Jaunes", "Rouges" };
    for (int k = 0; k < 7; k++) drawTextPx(hd[k], cx[k], hy, 10, C_DIM);
    hy += 12;
    struct HL { std::string s, club; int a, b, pd, j, r; bool cur; };
    std::vector<HL> H;
    int ta = 0, tb = 0, tp = 0, tj = 0, tr = 0;
    for (auto& h : p.hist) {
        std::string cn = h.team >= 0 && h.team < (int)g_world.teams.size() ? g_world.teams[h.team].name : std::string("?");
        H.push_back({ fmt("%d-%02d", h.year, (h.year + 1) % 100), cn + (h.moved ? " (transféré)" : ""), h.apps, h.goals, h.assists, h.yel, h.red, false });
        ta += h.apps; tb += h.goals; tp += h.assists; tj += h.yel; tr += h.red;
    }
    if (g_careerActive) {
        H.push_back({ fmt("%d-%02d", g_career.year, (g_career.year + 1) % 100), T.name + " (en cours)", p.apps, p.goals, p.assists, p.sYel, p.sRed, true });
        ta += p.apps; tb += p.goals; tp += p.assists; tj += p.sYel; tr += p.sRed;
    }
    int rows = (VH - 44 - hy) / 12;
    listRun(g_plLW, (int)H.size(), 12, hy, VW - 24, rows, 12, [&](int i, int xx, int y2, bool sel) {
        const HL& h = H[i];
        Color c = sel ? BLACK : h.cur ? C_HI : C_TXT;
        drawTextPx(h.s, cx[0], y2 + 1, 10, c);
        drawTextPx(fitText(h.club, 240, 10), cx[1], y2 + 1, 10, c);
        int v[] = { h.a, h.b, h.pd, h.j, h.r };
        for (int k = 0; k < 5; k++) drawTextPx(fmt("%d", v[k]), cx[k + 2] + 8, y2 + 1, 10, c);
        (void)xx;
    }, false);
    if (H.empty()) drawTextPx("Pas encore d'historique (les saisons jouées en carrière s'ajoutent ici).", 16, hy + 2, 10, C_DIM);
    drawTextPx(fitText(fmt("Total : %d matchs, %d buts, %d passes décisives, %d jaunes, %d rouges", ta, tb, tp, tj, tr), g_careerActive ? 290 : VW - 180, 10), 16, VH - 32, 10, C_GOOD);
    if (!g_careerActive) {   // l'éditeur de joueurs n'est accessible qu'avant de lancer une carrière
        if (button(VW - 150, VH - 34, 138, 14, "Modifier (éditeur)")) { extern void openPlayerEditorFwd(); openPlayerEditorFwd(); return; }
        drawFooter("Tab : modifier le joueur   Retour");
        if (IN.tab) { extern void openPlayerEditorFwd(); openPlayerEditorFwd(); return; }
    } else {
        // carrière : discussion individuelle avec un joueur de son effectif (une fois par mois)
        if (careerRules() && g_plTeam == g_career.userTeam && !g_career.life.isPlayer && g_career.kind == CK_CLUB) {
            static std::map<int, int> talked;     // joueur -> mois de la dernière discussion
            int month = (int)(g_career.season.now / 4.345) + g_career.year * 12;
            bool can = talked.count(p.id) == 0 || talked[p.id] != month;
            static const char* TALK[3] = { "Féliciter", "Encourager", "Recadrer" };
            for (int k = 0; k < 3; k++) if (button(VW - 330 + k * 106, VH - 34, 100, 14, TALK[k], false, can)) {
                int d = 0; std::string r;
                bool inForm = p.goals + p.assists >= std::max(2, p.apps / 3) || p.morale >= 75;
                if (k == 0) { d = inForm ? 6 : 1; r = inForm ? p.name + " apprécie vos félicitations, il est gonflé à bloc." : p.name + " est surpris : il sait qu'il peut mieux faire."; }
                else if (k == 1) { d = p.morale < 50 ? 7 : 2; r = p.morale < 50 ? p.name + " reprend confiance grâce à votre soutien." : p.name + " vous remercie, il était déjà serein."; }
                else { bool lazy = p.morale >= 70 && !inForm; d = lazy ? 1 : -7; r = lazy ? p.name + " a compris le message et redouble d'efforts." : p.name + " le prend mal, son moral baisse."; }
                p.morale = (uint8_t)std::max(10, std::min(100, p.morale + d));
                talked[p.id] = month;
                toast(r + fmt(" (moral %+d)", d));
                g_career.season.news.push_back("Vestiaire : " + r);
            }
        }
        drawFooter(careerRules()?"Tab : postes puis personnalité   Retour":"Retour");
    }
    if (IN.back) g_screen = g_plBack;
}

// ------------------------------------------------------------------ éditeur de joueur
static Player g_pe;
static int g_peRow = 0, g_peTyping = -1;
static bool g_peNew = false;       // joueur créé : supprimé si l'édition est annulée

void openPlayerEditorFwd() {
    g_pe = g_world.teams[g_plTeam].squad[g_plIdx];
    if (!g_pe.dribble) g_pe.dribble = (uint8_t)g_pe.drib();
    if (!g_pe.heading) g_pe.heading = (uint8_t)g_pe.head();
    if (!g_pe.positioning) g_pe.positioning = (uint8_t)g_pe.posi();
    if (!g_pe.composure) g_pe.composure = (uint8_t)g_pe.comp();
    const Team& T = g_world.teams[g_plTeam];
    int d, m, y; std::string place; playerBirth(g_pe, curYear(), d, m, y, place, T.culture, T.dept);
    if (!g_pe.bday) g_pe.bday = (uint8_t)d;
    if (!g_pe.bmonth) g_pe.bmonth = (uint8_t)m;
    if (g_pe.birthPlace.empty()) g_pe.birthPlace = place;
    g_peRow = 0; g_peTyping = -1; g_textMode = false;
    g_screen = SC_PLAYEREDIT;
}

static void screenPlayerEdit() {
    Team& T = g_world.teams[g_plTeam];
    drawBackground("Éditeur de joueur");
    drawTextPx(T.name, 12, 30, 10, C_DIM);
    struct Row { std::string label, value; int kind; uint8_t* v; int lo, hi; };   // 0 texte, 1 valeur, 2 action
    static const char* PN[] = { "Gardien", "Défenseur", "Milieu", "Attaquant" };
    int nat = g_pe.nation;
    uint8_t posv = g_pe.pos;
    std::vector<Row> rows = {
        { "Nom", g_pe.name, 0, nullptr, 0, 0 },
        { "Genre", g_pe.gender ? "Femme" : "Homme", 1, &g_pe.gender, 0, 1 },
        { "Couleur de peau", SKIN_NAMES[g_pe.skin % NUM_SKINS], 1, &g_pe.skin, 0, NUM_SKINS - 1 },
        { "Cheveux", HAIR_NAMES[g_pe.hair % NUM_HAIRS], 1, &g_pe.hair, 0, NUM_HAIRS - 1 },
        { "Poste", PN[posv & 3], 1, &g_pe.pos, 0, 3 },
        { "Numéro", fmt("%d", g_pe.num), 1, &g_pe.num, 1, 99 },
        { "Âge", fmt("%d", g_pe.age), 1, &g_pe.age, 13, 45 },
        { "Jour de naissance", fmt("%d", g_pe.bday), 1, &g_pe.bday, 1, 31 },
        { "Mois de naissance", MONTHS_FR[std::max(1, std::min(12, (int)g_pe.bmonth))], 1, &g_pe.bmonth, 1, 12 },
        { "Lieu de naissance", g_pe.birthPlace, 0, nullptr, 0, 0 },
        { "Nationalité", nat >= 0 && nat < NUM_NATIONS ? sanitize(NATIONS[nat].name) : std::string("(celle du club)"), 3, nullptr, 0, 0 },
        { "Vitesse", fmt("%d", g_pe.speed), 1, &g_pe.speed, 5, 99 },
        { "Tir", fmt("%d", g_pe.shoot), 1, &g_pe.shoot, 5, 99 },
        { "Passe", fmt("%d", g_pe.pass), 1, &g_pe.pass, 5, 99 },
        { "Tacle", fmt("%d", g_pe.tackle), 1, &g_pe.tackle, 5, 99 },
        { "Dribble", fmt("%d", g_pe.dribble), 1, &g_pe.dribble, 5, 99 },
        { "Jeu de tête", fmt("%d", g_pe.heading), 1, &g_pe.heading, 5, 99 },
        { "Placement", fmt("%d", g_pe.positioning), 1, &g_pe.positioning, 5, 99 },
        { "Sang-froid", fmt("%d", g_pe.composure), 1, &g_pe.composure, 5, 99 },
        { "Endurance", fmt("%d", g_pe.stamina), 1, &g_pe.stamina, 5, 99 },
        { "Gardien", fmt("%d", g_pe.keep), 1, &g_pe.keep, 5, 99 },
        { "Potentiel", fmt("%d", g_pe.pot), 1, &g_pe.pot, 5, 99 },
        { ">>> ENREGISTRER <<<", "", 2, nullptr, 0, 0 }, { "Annuler", "", 2, nullptr, 0, 0 } };
    int n = (int)rows.size();
    if (g_peTyping < 0) {
        if (IN.up) g_peRow = (g_peRow - 1 + n) % n;
        if (IN.down) g_peRow = (g_peRow + 1) % n;
    }
    int x = 12, w = 400, y = 44;
    for (int i = 0; i < n; i++) {
        bool sel = i == g_peRow;
        if (IN.click && g_peTyping < 0 && IN.mouse.x >= x && IN.mouse.x < x + w && IN.mouse.y >= y && IN.mouse.y < y + 12) g_peRow = i;
        DrawRectangle(x, y, w, 11, sel ? C_SEL : C_ITEM);
        Color c = sel ? BLACK : C_TXT;
        if (rows[i].kind == 2) drawTextCentered(rows[i].label, x + w / 2, y + 1, 10, c, false);
        else {
            drawTextPx(rows[i].label, x + 6, y + 1, 10, c);
            std::string v = rows[i].value;
            if (g_peTyping == i && fmod(GetTime(), 1.0) < 0.5) v += "_";
            drawTextPx(rows[i].kind == 0 ? fitText(v, w - 160, 10) : "< " + v + " >", x + 150, y + 1, 10, c);
        }
        y += 12;
    }
    // aperçu
    Player pv = g_pe;
    drawTextPx(fmt("Note générale : %d", pv.overall()), 430, 50, 10, C_HI);
    drawPortrait(VW - 84, 130, 72, pv.skin, pv.hair, pv.gender, T.home, (unsigned)pv.id);
    drawTextPx(fmt("Valeur : %s", money(pv.value()).c_str()), 430, 64, 10, C_TXT);
    if (g_careerActive) { drawTextPx("Modification enregistrée", 430, 84, 10, C_DIM); drawTextPx("dans la sauvegarde de la carrière.", 430, 96, 10, C_DIM); }
    else { drawTextPx("Modification enregistrée dans", 430, 84, 10, C_DIM); drawTextPx("joueurs_perso.txt (appliquée", 430, 96, 10, C_DIM); drawTextPx("à chaque nouvelle partie).", 430, 108, 10, C_DIM); }
    if (g_peTyping >= 0) {
        std::string* dst = g_peTyping == 0 ? &g_pe.name : &g_pe.birthPlace;
        textEdit(*dst, 40);
        drawFooter("Saisissez le texte   Entrée : valider");
        if (IN.start || IN.back || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_ESCAPE)) { g_peTyping = -1; g_textMode = false; }
        return;
    }
    drawFooter("OK : modifier / valider   Gauche/Droite : changer (Maj : +/-10)   Retour : annuler");
    int dd = IN.left ? -1 : IN.right ? 1 : 0;
    if (dd && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))) dd *= 10;
    const Row& R = rows[g_peRow];
    if (R.kind == 1 && R.v && dd) *R.v = (uint8_t)std::max(R.lo, std::min(R.hi, (int)*R.v + dd));
    if (R.kind == 3 && dd) { int k = g_pe.nation + dd; if (k < -1) k = NUM_NATIONS - 1; if (k >= NUM_NATIONS) k = -1; g_pe.nation = (int16_t)k; }
    if (IN.ok || (IN.click && IN.mouse.x < x + w && IN.mouse.y >= 44 && IN.mouse.y < 44 + n * 12)) {
        if (R.kind == 0) { g_peTyping = g_peRow; g_textMode = true; while (GetCharPressed() > 0) {} }
        else if (R.kind == 2) {
            if (g_peRow == n - 2) {
                Player& P = T.squad[g_plIdx];
                for (auto& q : T.squad) if (&q != &P && q.num == g_pe.num) { toast(fmt("Le numéro %d est déjà pris", g_pe.num)); return; }
                P = g_pe;
                if (!g_careerActive) savePlayerEdit(T, g_plIdx);
                toast(P.name + (g_peNew ? " : nouveau joueur créé" : " : modifications enregistrées"));
                g_peNew = false;
            } else if (g_peNew) { T.squad.pop_back(); g_peNew = false; g_screen = g_plBack; return; }
            g_screen = SC_PLAYER;
        }
        return;
    }
    if (IN.back) { if (g_peNew) { T.squad.pop_back(); g_peNew = false; g_screen = g_plBack; return; } g_screen = SC_PLAYER; }
}

// ------------------------------------------------------------------ consignes tactiques et délégation
static int g_tacRow = 0;
static void screenTactics() {
    int ti = g_career.userTeam;
    if (ti < 0) { g_screen = SC_SQUAD; return; }
    Team& T = g_world.teams[ti];
    if(careerRules()&&g_rolePlayerTab){screenRolePlan(T);return;}
    drawBackground("Consignes tactiques");
    if(careerRules()&&button(400,18,220,14,"Postes / rôles individuels")){g_rolePlayerTab=1;return;}
    static const char* LVL3[] = { "Faible", "Normal", "Fort" };
    static const char* LINE[] = { "Basse", "Médiane", "Haute" };
    static const char* WID[] = { "Resserré", "Normal", "Large" };
    static const char* TEMPO[] = { "Lent (possession)", "Normal", "Rapide" };
    static const char* PASS[] = { "Courtes", "Mixtes", "Longues" };
    struct Row { std::string label, value, help; };
    std::vector<Row> rows = {
        { "Système", FORMATIONS[T.formation].name, "Disposition des joueurs sur le terrain." },
        { "Mentalité", mentalityName(T.mentality), "Du plus défensif au plus offensif : nombre de joueurs qui montent, prise de risque." },
        { "Pressing", LVL3[T.pressing % 3], "Intensité et distance du pressing sur le porteur du ballon (fatigue plus vite si fort)." },
        { "Ligne défensive", LINE[T.defLine % 3], "Hauteur du bloc : haute = équipe compacte mais espace dans le dos." },
        { "Largeur", WID[T.width % 3], "Écartement des joueurs : large = jeu sur les ailes, resserré = axe." },
        { "Tempo", TEMPO[T.tempo % 3], "Vitesse de circulation du ballon : rapide = plus de passes vers l'avant et de tirs." },
        { "Passes", PASS[T.passStyle % 3], "Style de passe préféré (courtes, mixtes ou longues vers les attaquants)." },
        { "Adjoint : entraînement", g_career.mgr.delegTrain ? "délégué à l'adjoint" : "géré par vous", "L'adjoint choisit chaque mois le thème et l'intensité selon l'état de l'effectif." },
        { "Adjoint : remplacements", g_career.mgr.delegSubs ? "délégués à l'adjoint" : "gérés par vous", "Pendant vos matchs, l'adjoint effectue les changements (fatigue, blessures, score)." },
        { "Mode Full Manager", g_career.mgr.fullManager ? "activé (temps forts)" : "désactivé", "Vous ne jouez plus les matchs : vous regardez les temps forts et donnez vos consignes." },
    };
    if(!careerRules()){rows.resize(7);rows.erase(rows.begin()+5);rows.erase(rows.begin()+3);}
    int n = (int)rows.size();g_tacRow%=n;
    if (IN.up) g_tacRow = (g_tacRow - 1 + n) % n;
    if (IN.down) g_tacRow = (g_tacRow + 1) % n;
    int x = 20, y = 38, w = VW - 40;
    for (int i = 0; i < n; i++) {
        bool sel = i == g_tacRow;
        if (IN.click && IN.mouse.x >= x && IN.mouse.x < x + w && IN.mouse.y >= y && IN.mouse.y < y + 24) g_tacRow = i;
        DrawRectangle(x, y, w, 23, sel ? C_SEL : C_ITEM);
        Color c = sel ? BLACK : C_TXT;
        drawTextPx(rows[i].label, x + 8, y + 1, 10, c);
        drawTextPx("< " + rows[i].value + " >", x + 200, y + 1, 10, sel ? BLACK : C_HI);
        drawTextPx(fitText(rows[i].help, w - 20, 10), x + 8, y + 12, 10, sel ? Color{ 40, 40, 40, 255 } : C_DIM);
        y += 26;
    }
    drawFooter("Haut/Bas : consigne   Gauche/Droite ou OK : changer   Retour");
    int d = IN.left ? -1 : (IN.right || IN.ok || (IN.click && IN.mouse.y >= 40 && IN.mouse.y < 40 + n * 26)) ? 1 : 0;
    if (d) {
        auto cyc = [&](uint8_t& v, int m) { v = (uint8_t)((v + d + m) % m); };
        int action=careerRules()?g_tacRow:(g_tacRow==3?4:g_tacRow==4?6:g_tacRow);
        switch (action) {
        case 0: T.formation = (T.formation + d + NUM_FORMATIONS) % NUM_FORMATIONS; T.xi.clear();T.tactical.customized=0; break;
        case 1: T.mentality = careerRules()?(T.mentality+d+5)%5:1+(T.mentality-1+d+3)%3; break;
        case 2: cyc(T.pressing, 3); break;
        case 3: cyc(T.defLine, 3); break;
        case 4: cyc(T.width, 3); break;
        case 5: cyc(T.tempo, 3); break;
        case 6: cyc(T.passStyle, 3); break;
        case 7: g_career.mgr.delegTrain = !g_career.mgr.delegTrain; break;
        case 8: g_career.mgr.delegSubs = !g_career.mgr.delegSubs; break;
        case 9: g_career.mgr.fullManager = !g_career.mgr.fullManager; break;
        }
    }
    if (IN.back) g_screen = SC_SQUAD;
}

// ------------------------------------------------------------------ éditeur (hors carrière) : joueurs, présidents, staff
static int g_dbTab = 0;            // 0 joueurs et présidents, 1 staff
static int g_dbTeam = -1;          // club choisi
static std::string g_dbFilter;
static bool g_dbTyping = false;
static ListW g_dbLW, g_dbSqLW, g_dbStLW;
static int g_dbPres = -1, g_dbPresRow = 0, g_dbPresTyping = -1;
static Team g_dbPresT;
static int g_dbStaffEdit = -1, g_dbStRow = 0, g_dbStTyping = -1;
static CustomStaff g_dbSt;

static void openEditDb() { g_dbTeam = -1; g_dbPres = -1; g_dbStaffEdit = -1; g_dbTyping = false; g_textMode = false; g_screen = SC_EDITDB; }

static void newPlayerFor(int team, int gender = -1) {
    g_world.ensureSquad(team);
    Team& T = g_world.teams[team];
    Player p = g_world.makeYouth(team, POS_MF, std::max(20.f, T.rating - 3));
    if (gender < 0) gender = T.youth == 6 ? 1 : 0;          // section féminine : une joueuse par défaut
    if ((gender == 1) != (T.youth == 6)) { toast(gender ? "Une joueuse évolue dans une équipe féminine (sélection ou club féminin)" : "Équipe féminine : seules des joueuses peuvent y être créées"); return; }
    p.gender = (uint8_t)gender;
    p.name = gender ? "Nouvelle joueuse" : "Nouveau joueur"; p.age = T.youth == 3 ? 14 : T.youth == 2 ? 16 : T.youth == 1 ? 18 : 20; p.nation = (int16_t)(T.nation >= 0 ? T.nation : -1);
    T.squad.push_back(p);
    g_plTeam = team; g_plIdx = (int)T.squad.size() - 1; g_plBack = SC_EDITDB;
    openPlayerEditorFwd();
    g_peNew = true;
    g_peTyping = 0; g_textMode = true; while (GetCharPressed() > 0) {}
}

static void screenPresEdit() {
    Team& T = g_dbPresT;
    drawBackground("Président du club");
    drawTextPx(g_world.teams[g_dbPres].name, 12, 30, 10, C_DIM);
    std::vector<std::pair<std::string, std::string>> rows = { { "Nom du président", T.president }, { "Âge", fmt("< %d ans >", T.presAge) }, { "Lieu de naissance", T.presPlace.empty() ? std::string("(non renseigné)") : T.presPlace },
                                                              { ">>> ENREGISTRER <<<", "" }, { "Annuler", "" } };
    int n = (int)rows.size();
    if (g_dbPresTyping < 0) { if (IN.up) g_dbPresRow = (g_dbPresRow + n - 1) % n; if (IN.down) g_dbPresRow = (g_dbPresRow + 1) % n; }
    int x = 60, y = 60, w = VW - 120;
    for (int i = 0; i < n; i++) {
        bool sel = i == g_dbPresRow;
        if (IN.click && g_dbPresTyping < 0 && IN.mouse.x >= x && IN.mouse.x < x + w && IN.mouse.y >= y && IN.mouse.y < y + 16) g_dbPresRow = i;
        DrawRectangle(x, y, w, 15, sel ? C_SEL : C_ITEM);
        Color c = sel ? BLACK : C_TXT;
        if (i >= 3) drawTextCentered(rows[i].first, x + w / 2, y + 3, 10, c, false);
        else { drawTextPx(rows[i].first, x + 8, y + 3, 10, c); drawTextPx(rows[i].second + (g_dbPresTyping == i && fmod(GetTime(), 1.0) < 0.5 ? "_" : ""), x + 190, y + 3, 10, c); }
        y += 18;
    }
    if (g_dbPresTyping >= 0) {
        textEdit(g_dbPresTyping == 0 ? T.president : T.presPlace, 40);
        drawFooter("Saisissez le texte   Entrée : valider");
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_ESCAPE)) { g_dbPresTyping = -1; g_textMode = false; }
        return;
    }
    drawFooter("OK : modifier / valider   Gauche/Droite : âge   Retour");
    if (g_dbPresRow == 1 && (IN.left || IN.right)) T.presAge = std::max(25, std::min(95, T.presAge + (IN.left ? -1 : 1)));
    if (IN.ok || (IN.click && IN.mouse.y >= 60 && IN.mouse.y < 60 + n * 18)) {
        if (g_dbPresRow == 0 || g_dbPresRow == 2) { g_dbPresTyping = g_dbPresRow; g_textMode = true; while (GetCharPressed() > 0) {} }
        else if (g_dbPresRow == 3) {
            Team& R = g_world.teams[g_dbPres];
            R.president = T.president; R.presAge = T.presAge; R.presPlace = T.presPlace;
            savePresident(R);
            toast("Président enregistré : " + R.president);
            g_dbPres = -1;
        } else if (g_dbPresRow == 4) g_dbPres = -1;
        return;
    }
    if (IN.back) g_dbPres = -1;
}

static void screenStaffEdit() {
    CustomStaff& S = g_dbSt;
    drawBackground(g_dbStaffEdit >= (int)customStaff().size() ? "Nouveau membre du staff" : "Modifier un membre du staff");
    std::vector<std::pair<std::string, std::string>> rows = {
        { "Nom", S.name }, { "Poste", std::string("< ") + staffRoleName(S.role) + " >" }, { "Niveau", fmt("< %d / 5 >", S.level) }, { "Âge", fmt("< %d ans >", S.age) },
        { "Lieu de naissance", S.place.empty() ? std::string("(non renseigné)") : S.place }, { "Club (au début d'une carrière)", S.club.empty() ? std::string("(aucun : candidat pour tous les clubs)") : S.club },
        { ">>> ENREGISTRER <<<", "" }, { "Supprimer", "" }, { "Annuler", "" } };
    int n = (int)rows.size();
    if (g_dbStTyping < 0) { if (IN.up) g_dbStRow = (g_dbStRow + n - 1) % n; if (IN.down) g_dbStRow = (g_dbStRow + 1) % n; }
    int x = 40, y = 44, w = VW - 80;
    for (int i = 0; i < n; i++) {
        bool sel = i == g_dbStRow;
        if (IN.click && g_dbStTyping < 0 && IN.mouse.x >= x && IN.mouse.x < x + w && IN.mouse.y >= y && IN.mouse.y < y + 16) g_dbStRow = i;
        DrawRectangle(x, y, w, 15, sel ? C_SEL : C_ITEM);
        Color c = sel ? BLACK : C_TXT;
        if (i >= 6) drawTextCentered(rows[i].first, x + w / 2, y + 3, 10, c, false);
        else { drawTextPx(rows[i].first, x + 8, y + 3, 10, c); drawTextPx(fitText(rows[i].second, w - 230, 10) + (g_dbStTyping == i && fmod(GetTime(), 1.0) < 0.5 ? "_" : ""), x + 220, y + 3, 10, c); }
        y += 18;
    }
    drawTextPx(staffRoleDesc(S.role), x, y + 6, 10, C_DIM);
    drawTextPx("Les membres créés apparaissent parmi les candidats du poste ; avec un club, ils font partie de son staff au début de la carrière.", x, y + 20, 10, C_DIM);
    if (g_dbStTyping >= 0) {
        textEdit(g_dbStTyping == 0 ? S.name : g_dbStTyping == 4 ? S.place : S.club, 40);
        drawFooter("Saisissez le texte   Entrée : valider");
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_ESCAPE)) { g_dbStTyping = -1; g_textMode = false; }
        return;
    }
    drawFooter("OK : modifier / valider   Gauche/Droite : changer   Retour");
    int d = IN.left ? -1 : IN.right ? 1 : 0;
    if (d) {
        if (g_dbStRow == 1) S.role = (S.role + d + NUM_SR) % NUM_SR;
        if (g_dbStRow == 2) S.level = std::max(1, std::min(5, S.level + d));
        if (g_dbStRow == 3) S.age = std::max(20, std::min(85, S.age + d));
    }
    if (IN.ok || (IN.click && IN.mouse.y >= 44 && IN.mouse.y < 44 + n * 18)) {
        if (g_dbStRow == 0 || g_dbStRow == 4 || g_dbStRow == 5) { g_dbStTyping = g_dbStRow; g_textMode = true; while (GetCharPressed() > 0) {} }
        else if (g_dbStRow == 1) S.role = (S.role + 1) % NUM_SR;
        else if (g_dbStRow == 6) {
            auto& L = customStaff();
            if (g_dbStaffEdit >= (int)L.size()) L.push_back(S); else L[g_dbStaffEdit] = S;
            saveCustomStaff(); toast(S.name + " enregistré"); g_dbStaffEdit = -1;
        } else if (g_dbStRow == 7) {
            auto& L = customStaff();
            if (g_dbStaffEdit < (int)L.size()) { L.erase(L.begin() + g_dbStaffEdit); saveCustomStaff(); toast("Membre du staff supprimé"); }
            g_dbStaffEdit = -1;
        } else g_dbStaffEdit = -1;
        return;
    }
    if (IN.back) g_dbStaffEdit = -1;
}

static void screenEditDb() {
    if (g_dbPres >= 0) { screenPresEdit(); return; }
    if (g_dbStaffEdit >= 0) { screenStaffEdit(); return; }
    drawBackground("Éditeur : joueurs, présidents, staff");
    if (button(10, 30, 200, 14, "Joueurs et présidents", g_dbTab == 0)) { g_dbTab = 0; }
    if (button(214, 30, 120, 14, "Staff", g_dbTab == 1)) { g_dbTab = 1; g_dbTyping = false; g_textMode = false; }
    if (g_dbTab == 1) {
        auto& L = customStaff();
        int s = listRun(g_dbStLW, (int)L.size() + 1, 20, 52, VW - 40, 20, 13, [&](int i, int x, int y, bool sel) {
            Color c = sel ? BLACK : C_TXT;
            if (i == (int)L.size()) { drawTextPx("+ Créer un nouveau membre du staff", x + 6, y + 1, 10, sel ? BLACK : C_HI); return; }
            drawTextPx(fitText(L[i].name, 170, 10), x + 6, y + 1, 10, c);
            drawTextPx(staffRoleName(L[i].role), x + 190, y + 1, 10, c);
            drawTextPx(fmt("niv. %d - %d ans", L[i].level, L[i].age), x + 360, y + 1, 10, sel ? BLACK : C_DIM);
            drawTextPx(fitText(L[i].club, 120, 10), x + 470, y + 1, 10, sel ? BLACK : C_DIM);
        });
        drawFooter("OK : modifier / créer   Tab : onglet   Retour");
        if (s >= 0) {
            g_dbStaffEdit = s; g_dbStRow = 0; g_dbStTyping = -1;
            if (s < (int)L.size()) g_dbSt = L[s];
            else { g_dbSt = CustomStaff(); g_dbSt.name = "Nouveau membre"; g_dbStTyping = 0; g_textMode = true; while (GetCharPressed() > 0) {} }
        }
        if (IN.tab) g_dbTab = 0;
        if (IN.back) g_screen = SC_EDITMENU;
        return;
    }
    if (g_dbTeam < 0) {
        // choix du club : clubs créés d'abord, recherche par nom
        drawTextPx("Rechercher : " + g_dbFilter + (g_dbTyping && fmod(GetTime(), 1.0) < 0.5 ? "_" : ""), 350, 32, 10, g_dbTyping ? C_HI : C_DIM);
        if (!g_dbTyping) { g_dbTyping = true; g_textMode = true; }
        std::string before = g_dbFilter;
        textEdit(g_dbFilter, 30);
        if (before != g_dbFilter) g_dbLW = ListW();
        static std::vector<int> list; static std::string lastF = "\x01"; static size_t lastN = 0;
        if (lastF != g_dbFilter || lastN != g_world.teams.size()) {
            lastF = g_dbFilter; lastN = g_world.teams.size();
            list.clear();
            std::string f = sortKey(g_dbFilter);
            for (int pass = 0; pass < 2; pass++)
                for (int i = 0; i < (int)g_world.teams.size() && list.size() < 400; i++) {
                    const Team& t = g_world.teams[i];
                    if (t.kind != TK_CLUB || (pass == 0) != t.custom) continue;
                    if (!f.empty() && sortKey(t.name).find(f) == std::string::npos) continue;
                    list.push_back(i);
                }
        }
        int s = listRun(g_dbLW, (int)list.size(), 20, 48, VW - 40, 21, 13, [&](int i, int x, int y, bool sel) {
            const Team& t = g_world.teams[list[i]];
            drawKitIcon(t.home, x + 4, y + 2, 1);
            drawTextPx(fitText(t.name, 280, 10), x + 20, y + 1, 10, sel ? BLACK : t.custom ? C_GOOD : C_TXT);
            drawTextPx(fitText(t.custom ? std::string("club créé") : divisionOf(list[i]), 260, 10), x + 320, y + 1, 10, sel ? BLACK : C_DIM);
        });
        drawFooter("Tapez un nom pour filtrer   Entrée / clic : effectif du club   Échap : menu");
        if (list.empty()) drawTextCentered("Aucun club", VW / 2, 90, 10, C_DIM);
        if (s < 0 && IN.start && !list.empty()) s = std::max(0, std::min((int)list.size() - 1, g_dbLW.cur));
        if (s >= 0) { g_dbTeam = list[s]; g_world.ensureSquad(g_dbTeam); g_dbSqLW = ListW(); g_dbTyping = false; g_textMode = false; }
        if (IsKeyPressed(KEY_ESCAPE)) { g_dbTyping = false; g_textMode = false; g_screen = SC_EDITMENU; }
        return;
    }
    Team& T = g_world.teams[g_dbTeam];
    drawKitIcon(T.home, 12, 50, 2);
    drawTextPx(T.name, 36, 50, 10, C_HI);
    int root = T.parent >= 0 ? T.parent : g_dbTeam;
    drawTextPx("Président : " + presidentName(root) + fmt(" (%d ans)", presidentAge(root)), 36, 62, 10, C_TXT);
    if (button(VW - 330, 48, 150, 14, "Modifier le président")) { g_dbPres = root; g_dbPresT = g_world.teams[root]; g_dbPresT.president = presidentName(root); g_dbPresT.presAge = presidentAge(root); g_dbPresT.presPlace = presidentPlace(root); g_dbPresRow = 0; g_dbPresTyping = -1; return; }
    if (button(VW - 174, 32, 162, 14, "Créer un nouveau joueur")) { newPlayerFor(g_dbTeam, 0); return; }
    if (button(VW - 174, 48, 162, 14, "Créer une nouvelle joueuse")) { newPlayerFor(g_dbTeam, 1); return; }
    drawTextPx("N°  P   Joueur", 16, 78, 10, C_DIM);
    const char* cols[] = { "ÂGE", "VIT", "TIR", "PAS", "TAC", "GAR", "NOTE" };
    for (int k = 0; k < 7; k++) drawTextPx(cols[k], 300 + k * 44, 78, 10, C_DIM);
    int s = listRun(g_dbSqLW, (int)T.squad.size(), 12, 90, VW - 24, 19, 12, [&](int i, int x, int y, bool sel) {
        const Player& p = T.squad[i];
        Color c = sel ? BLACK : C_TXT;
        drawTextPx(fmt("%2d  %s", p.num, posName(p.pos)), x + 4, y + 1, 10, c);
        drawTextPx(fitText(p.name, 220, 10), x + 52, y + 1, 10, c);
        int v[] = { p.age, p.speed, p.shoot, p.pass, p.tackle, p.keep, p.overall() };
        for (int k = 0; k < 7; k++) drawTextPx(fmt("%d", v[k]), x + 290 + k * 44, y + 1, 10, k == 6 ? (sel ? BLACK : C_HI) : c);
    });
    drawFooter("OK : fiche du joueur (Tab : modifier)   Retour : liste des clubs");
    if (s >= 0) { openPlayer(g_dbTeam, s, SC_EDITDB); return; }
    if (IN.back) { g_dbTeam = -1; g_dbTyping = false; }
}
void openEditDbFwd() { openEditDb(); }
