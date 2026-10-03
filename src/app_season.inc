// ------------------------------------------------------------------ bilans de début et de fin de saison (toutes les sections du club), archives des compétitions
// équipes du club : fanion, réserves, jeunes, section féminine
static std::vector<int> clubFamily(int club) {
    std::vector<int> v;
    if (club < 0) return v;
    v.push_back(club);
    for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].parent == club) v.push_back(i);
    auto w = g_world.womenOf.find(club);          // section féminine (club lié)
    if (w != g_world.womenOf.end()) v.push_back(w->second);
    return v;
}
static std::string sectionName(int t, int club) {
    if (t == club) return "Équipe première";
    const Team& T = g_world.teams[t];
    if (T.youth == 6) return "Section féminine";
    if (T.name.find("U19") != std::string::npos) return "U19";
    if (T.name.find("U17") != std::string::npos) return "U17";
    if (T.name.find("U15") != std::string::npos) return "U15";
    return T.resLevel >= 2 ? "Équipe C" : "Réserve";
}

struct SeasonLine { std::string comp, info; Color c; };
// engagements : championnat (division) et coupes (tour d'entrée)
static std::vector<SeasonLine> seasonEntries(int t) {
    std::vector<SeasonLine> out;
    const Season& S = g_career.season;
    for (int c = 0; c < (int)S.comps.size(); c++) {
        const Competition& C = S.comps[c];
        if (C.kind == 12) continue;
        std::string info;
        for (int k = 0; k < (int)C.entrants.size() && info.empty(); k++)
            for (int e : C.entrants[k]) if (e == t) { info = "entrée : " + (k < (int)C.koNames.size() && !C.koNames[k].empty() ? C.koNames[k] : fmt("%de tour", k + 1)); break; }
        if (info.empty()) for (auto& st : C.stages) {
            bool in = false;
            for (auto& g : st.groups) for (int x : g) if (x == t) in = true;
            for (auto& ti : st.ties) if (ti.a == t || ti.b == t) in = true;
            if (in) { info = C.kind == 1 ? std::string("championnat") : "entrée : " + st.name; break; }
        }
        if (info.empty()) continue;
        out.push_back({ C.name, info, C.kind == 1 ? C_HI : C_TXT });
    }
    // coupes dont la phase nationale est créée en cours de saison : Coupe de France (L1 et L2), Gambardella (National U19)
    auto has = [&](const char* pre) { for (auto& l : out) if (l.comp.rfind(pre, 0) == 0) return true; return false; };
    const Team& T = g_world.teams[t];
    bool fr = T.nation >= 0 && strcmp(NATIONS[T.nation].code, "FRA") == 0;
    if (fr && T.parent < 0 && !T.youth && !has("Coupe de France") && g_career.cdfNational < 0) {
        int lv = teamLevel(t);
        if (lv == 0 || t == g_career.cdfHolderDirect) out.push_back({ "Coupe de France", "entrée : 32es de finale (janvier)", C_TXT });
        else if (lv == 1) out.push_back({ "Coupe de France", "entrée : 7e tour (phase nationale)", C_TXT });
    }
    if (T.youth == 1 && !has("Coupe Gambardella") && g_career.gambNational < 0) {
        int yp = g_career.u19Pyramid(), p, q, g;
        if (yp >= 0 && g_career.tierOfTeam(t, &p, &q, &g) == 0 && p == yp) out.push_back({ "Coupe Gambardella", "entrée : 1er tour fédéral", C_TXT });
    }
    std::stable_sort(out.begin(), out.end(), [](const SeasonLine& a, const SeasonLine& b) { return (a.info == "championnat") > (b.info == "championnat"); });
    return out;
}
// bilan : classement final, parcours en coupe
static std::vector<SeasonLine> seasonResults(int t) {
    std::vector<SeasonLine> out;
    const Season& S = g_career.season;
    for (int c = 0; c < (int)S.comps.size(); c++) {
        const Competition& C = S.comps[c];
        if (C.kind == 12) continue;
        int last = -1; std::string lastName;
        for (int s = 0; s < (int)C.stages.size(); s++) {
            const Stage& st = C.stages[s];
            bool in = false;
            for (auto& g : st.groups) for (int x : g) if (x == t) in = true;
            for (auto& ti : st.ties) if (ti.a == t || ti.b == t) in = true;
            if (in) { last = s; lastName = st.name; }
        }
        if (last < 0) continue;
        if (C.kind == 1) {
            int pos = 0; for (int i = 0; i < (int)C.result.size(); i++) if (C.result[i] == t) pos = i + 1;
            if (!pos) { auto tb = C.table(0, 0); for (int g = 0; g < (int)C.stages[0].groups.size(); g++) { auto tg = C.table(0, g); for (int i = 0; i < (int)tg.size(); i++) if (tg[i].team == t) pos = i + 1; } }
            std::string mv;
            int p, q, g;
            if (g_career.tierOfTeam(t, &p, &q, &g) >= 0 && pos > 0) {
                const Pool& pl = g_career.pyramids[p].pools[q];
                const TierConf& T = g_career.pyramids[p].tiers[pl.tier];
                int n = pl.groups.empty() ? 0 : (int)pl.groups[std::min(g, (int)pl.groups.size() - 1)].size();
                if (pl.tier > 0 && pos <= T.up) mv = " - montée";
                else if (!T.flexible && !pl.terminal && T.down && n && pos > n - T.down) mv = " - relégation";
            }
            out.push_back({ C.name, (pos == 1 ? std::string("CHAMPION !") : fmt("%de", pos)) + mv, pos == 1 || mv == " - montée" ? C_GOOD : mv.empty() ? C_TXT : C_BAD });
        } else {
            std::string r = C.winner == t ? "VAINQUEUR !" : (last == (int)C.stages.size() - 1 && C.stages.back().ties.size() == 1 && C.done) ? "finaliste" : "éliminé en " + lastName;
            if (!C.done && C.winner != t) r = "en course : " + lastName;
            out.push_back({ C.name, r, C.winner == t ? C_GOOD : C_TXT });
        }
    }
    return out;
}

static void drawClubSeason(int y, bool results) {
    int club = g_career.userTeam;
    auto fam = clubFamily(club);
    int col = 0, x0 = 16, yy = y;
    std::vector<std::pair<int, std::vector<SeasonLine>>> all;
    int total = 0;
    for (int t : fam) {
        auto lines = results ? seasonResults(t) : seasonEntries(t);
        if (lines.empty() && t != club) continue;
        total += 14 + 11 * (int)lines.size() + 4;
        all.push_back({ t, lines });
    }
    bool wide = y + total <= VH - 20;          // tout tient sur une colonne : pleine largeur
    int w = wide ? VW - 32 : VW / 2 - 22, infoW = wide ? 330 : 150;
    for (auto& e : all) {
        int t = e.first; auto& lines = e.second;
        int need = 14 + 11 * (int)lines.size() + 4;
        if (!wide && yy + need > VH - 20 && col == 0) { col = 1; x0 = VW / 2 + 6; yy = y; }
        drawSection(x0, yy, w, sectionName(t, club) + "  -  " + fitText(g_career.teamLevelName(t), w - 140, 10)); yy += 13;
        for (auto& l : lines) {
            if (yy > VH - 20) break;
            drawTextPx(fitText(l.comp, w - infoW - 10, 10), x0 + 4, yy, 10, C_DIM);
            drawTextPx(fitText(l.info, infoW, 10), x0 + w - infoW + 2, yy, 10, l.c);
            yy += 11;
        }
        yy += 4;
    }
}

// petit pictogramme de compétition : championnat (classement), coupe (trophée), Europe (étoiles)
static void drawCompGlyph(int x, int y, const std::string& comp, const std::string& info) {
    bool league = info == "championnat";
    bool euro = comp.find("champions") != std::string::npos || comp.find("Europa") != std::string::npos || comp.find("Conférence") != std::string::npos || comp.find("UEFA") != std::string::npos;
    if (league) { for (int k = 0; k < 3; k++) { DrawRectangle(x, y + 1 + k * 3, 9, 2, k == 0 ? Color{ 240, 200, 60, 255 } : Color{ 150, 170, 210, 255 }); } return; }
    if (euro) { for (int k = 0; k < 3; k++) { int sx = x + 1 + k * 3, sy = y + 2 + (k == 1 ? -1 : 1); DrawRectangle(sx, sy, 1, 3, WHITE); DrawRectangle(sx - 1, sy + 1, 3, 1, WHITE); } DrawCircleLines(x + 4, y + 4, 5, Color{ 120, 160, 255, 255 }); return; }
    DrawRectangle(x + 1, y, 7, 4, Color{ 240, 200, 60, 255 }); DrawRectangle(x + 3, y + 4, 3, 2, Color{ 200, 150, 40, 255 }); DrawRectangle(x + 2, y + 6, 5, 2, Color{ 120, 90, 50, 255 });
    DrawRectangle(x, y + 1, 1, 2, Color{ 240, 200, 60, 255 }); DrawRectangle(x + 8, y + 1, 1, 2, Color{ 240, 200, 60, 255 });
}
static void screenSeasonStart() {
    drawBackground(fmt("Saison %d-%02d : engagements du club", g_career.year, (g_career.year + 1) % 100));
    int club = g_career.userTeam;
    const Team& U = g_world.teams[club];
    float t = (float)GetTime();
    // bandeau du club : couleurs, nom, stade, objectif
    Color c1 = hexc(U.home.shirt), c2 = hexc(U.home.shirt2 != U.home.shirt ? U.home.shirt2 : U.home.shorts);
    DrawRectangleGradientH(10, 30, VW - 20, 40, Color{ (unsigned char)(c1.r / 2), (unsigned char)(c1.g / 2), (unsigned char)(c1.b / 2), 255 }, Color{ 10, 16, 34, 255 });
    DrawRectangle(10, 30, 4, 40, c2); DrawRectangle(10, 69, VW - 20, 1, c2);
    for (int k = 0; k < 6; k++) DrawRectangle(VW - 150 + k * 22 + (int)(std::sin(t + k) * 2), 30, 10, 40, Color{ 255, 255, 255, 8 });
    drawKitIcon(U.home, 22, 36, 3);
    drawTextPx(fitText(U.name, 330, 20), 64, 34, 20, WHITE);
    drawTextPx(fitText((U.stadium.empty() ? std::string() : U.stadium + "  -  ") + g_career.teamLevelName(club), 400, 10), 64, 56, 10, Color{ 220, 225, 240, 255 });
    drawTextPx(fmt("SAISON %d-%02d", g_career.year, (g_career.year + 1) % 100), VW - 110, 36, 10, Color{ 255, 220, 120, 255 });
    // cartes : une par équipe du club (équipe première en pleine largeur, puis réserves et jeunes sur deux colonnes)
    auto fam = clubFamily(club);
    std::vector<std::pair<int, std::vector<SeasonLine>>> all;
    for (int tm : fam) { auto lines = seasonEntries(tm); if (lines.empty() && tm != club) continue; all.push_back({ tm, lines }); }
    int y = 76, col = 0, colY[2] = { 0, 0 };
    for (size_t i = 0; i < all.size(); i++) {
        int tm = all[i].first; auto& lines = all[i].second;
        bool first = i == 0;
        int w = first ? VW - 20 : (VW - 26) / 2, x = first ? 10 : 10 + col * (w + 6);
        int h = 16 + 12 * (int)lines.size() + 4;
        int cy = first ? y : y + colY[col];
        if (cy + h > VH - 22) { if (!first && col == 0) { col = 1; x = 10 + w + 6; cy = y + colY[1]; } if (cy + h > VH - 22) continue; }
        const Team& T = g_world.teams[tm];
        DrawRectangle(x + 2, cy + 2, w, h, Color{ 0, 0, 0, 70 });
        DrawRectangle(x, cy, w, h, Color{ 16, 26, 52, 235 });
        DrawRectangle(x, cy, w, 14, first ? Color{ 40, 60, 110, 255 } : Color{ 30, 44, 84, 255 });
        DrawRectangle(x, cy, 3, h, hexc(T.home.shirt));
        drawKitIcon(T.home, x + 6, cy + 2, 1);
        drawTextPx(fitText(sectionName(tm, club), w / 2, 10), x + 20, cy + 2, 10, C_HI);
        std::string lv = fitText(g_career.teamLevelName(tm), w / 2 - 20, 10);
        drawTextPx(lv, x + w - 6 - textWidth(lv, 10), cy + 2, 10, C_DIM);
        int ly = cy + 16;
        for (auto& l : lines) {
            DrawRectangle(x + 4, ly - 1, w - 8, 11, (int)(&l - &lines[0]) % 2 ? Color{ 255, 255, 255, 6 } : Color{ 255, 255, 255, 14 });
            drawCompGlyph(x + 7, ly, l.comp, l.info);
            int infoW = first ? 300 : w / 2 - 10;
            drawTextPx(fitText(l.comp, w - infoW - 30, 10), x + 20, ly, 10, C_TXT);
            std::string inf = fitText(l.info, infoW, 10);
            bool champ = l.info == "championnat";
            int iw = textWidth(inf, 10) + 8;
            DrawRectangle(x + w - 8 - iw, ly - 1, iw, 11, champ ? Color{ 90, 74, 20, 255 } : Color{ 30, 60, 100, 255 });
            drawTextPx(inf, x + w - 4 - iw, ly, 10, champ ? Color{ 255, 220, 120, 255 } : l.c);
            ly += 12;
        }
        if (lines.empty()) drawTextPx("Aucune compétition officielle", x + 20, ly, 10, C_DIM);
        if (first) y += h + 6; else { colY[col] += h + 6; col = colY[0] <= colY[1] ? 0 : 1; }
    }
    drawFooter("OK / Retour : continuer");
    if (IN.back || IN.ok || IN.start) openHub();
}

// ---------------- archives des compétitions : vainqueurs, meilleurs buteurs et passeurs (saison et all-time)
static ListW g_caLW, g_caLW2; static int g_caMode = 0;
static void screenCompArch() {
    drawBackground("Archives des compétitions");
    auto& A = g_career.compArch;
    std::vector<std::string> fams;
    {
        std::set<std::string> seen;
        // grandes compétitions d'abord (Ligue 1, coupes...), puis ordre alphabétique
        for (auto& r : A) if (seen.insert(r.comp).second) fams.push_back(r.comp);
        auto rank = [](const std::string& n) { static const char* TOP[] = { "Ligue 1", "Ligue 2", "Ligue 3", "Coupe de France", "Coupe de la Ligue", "Trophée des champions", "Ligue des champions", "Ligue Europa", "Coupe UEFA", "Ligue Conférence" };
            for (int i = 0; i < 10; i++) if (n.rfind(TOP[i], 0) == 0) return i; return 99; };
        std::stable_sort(fams.begin(), fams.end(), [&](const std::string& a, const std::string& b) { int ra = rank(a), rb = rank(b); if (ra != rb) return ra < rb; return sortKey(a) < sortKey(b); });
    }
    if (fams.empty()) {
        drawTextCentered("Aucune saison archivée pour l'instant : les archives se remplissent à chaque fin de saison.", VW / 2, 90, 10, C_DIM);
        drawFooter("Retour"); if (IN.back) openHub(); return;
    }
    int lw = 220;
    int s = listRun(g_caLW, (int)fams.size(), 8, 34, lw, (VH - 52) / 12, 12, [&](int i, int x, int yy, bool sel) {
        drawTextPx(fitText(fams[i], lw - 10, 10), x + 4, yy + 1, 10, sel ? BLACK : C_TXT);
    }, g_caSel < 0);
    if (s >= 0) { g_caSel = s; g_caLW2 = ListW(); }
    int cur = std::max(0, std::min((int)fams.size() - 1, g_caLW.cur));
    const std::string& F = fams[cur];
    int x = lw + 16, w = VW - x - 8;
    if (button(x, 34, 120, 14, "Saison par saison", g_caMode == 0)) g_caMode = 0;
    if (button(x + 124, 34, 150, 14, "Buteurs all-time", g_caMode == 1)) g_caMode = 1;
    if (button(x + 278, 34, 120, 14, "Passeurs all-time", g_caMode == 2)) g_caMode = 2;
    if (IN.tab) g_caMode = (g_caMode + 1) % 3;
    int y = 52;
    drawTextPx(fitText(F, w, 10), x, y, 10, C_HI); y += 14;
    if (g_caMode == 0) {
        std::vector<const CompArchRec*> v;
        for (auto& r : A) if (F == r.comp) v.push_back(&r);
        std::stable_sort(v.begin(), v.end(), [](const CompArchRec* a, const CompArchRec* b) { return a->year > b->year; });
        drawTextPx("Saison", x, y, 10, C_DIM); drawTextPx("Vainqueur / finaliste", x + 56, y, 10, C_DIM); drawTextPx("Buteur / passeur", x + 230, y, 10, C_DIM); y += 12;
        listRun(g_caLW2, (int)v.size(), x - 4, y, w, (VH - 20 - y) / 22, 22, [&](int i, int xx, int yy, bool sel) {
            const CompArchRec& r = *v[i];
            Color c = sel ? BLACK : C_TXT;
            drawTextPx(fmt("%d-%02d", r.year - 1, r.year % 100), xx + 4, yy + 1, 10, c);
            drawTextPx(fitText(r.winner >= 0 ? g_world.teams[r.winner].name : std::string("-"), 170, 10), xx + 60, yy + 1, 10, sel ? BLACK : r.winner == g_career.userTeam ? C_GOOD : C_HI);
            if (r.runner >= 0) drawTextPx(fitText(g_world.teams[r.runner].name, 170, 10), xx + 60, yy + 11, 10, sel ? BLACK : C_DIM);
            if (r.scorer[0]) drawTextPx(fitText(fmt("%s (%d buts)", r.scorer, r.goals), w - 240, 10), xx + 234, yy + 1, 10, c);
            if (r.assister[0]) drawTextPx(fitText(fmt("%s (%d passes)", r.assister, r.assists), w - 240, 10), xx + 234, yy + 11, 10, sel ? BLACK : C_DIM);
        }, g_caSel >= 0);
    } else {
        std::vector<const CompAllTime*> v;
        for (auto& r : g_career.compAllTime) if (F == r.comp) v.push_back(&r);
        bool goals = g_caMode == 1;
        std::stable_sort(v.begin(), v.end(), [&](const CompAllTime* a, const CompAllTime* b) { return goals ? a->goals > b->goals : a->assists > b->assists; });
        drawTextPx("Rang  Joueur", x, y, 10, C_DIM); drawTextPx(goals ? "Buts" : "Passes", x + w - 110, y, 10, C_DIM); drawTextPx("Saisons", x + w - 56, y, 10, C_DIM); y += 12;
        listRun(g_caLW2, (int)v.size(), x - 4, y, w, (VH - 20 - y) / 12, 12, [&](int i, int xx, int yy, bool sel) {
            const CompAllTime& r = *v[i];
            Color c = sel ? BLACK : C_TXT;
            drawTextPx(fmt("%2d", i + 1), xx + 4, yy + 1, 10, c);
            drawTextPx(fitText(r.name, 150, 10), xx + 26, yy + 1, 10, c);
            drawTextPx(fitText(r.team >= 0 ? g_world.teams[r.team].shortName : std::string(""), 90, 10), xx + 180, yy + 1, 10, sel ? BLACK : C_DIM);
            drawTextPx(fmt("%d", goals ? r.goals : r.assists), xx + w - 106, yy + 1, 10, sel ? BLACK : C_HI);
            drawTextPx(fmt("%d", r.seasons), xx + w - 50, yy + 1, 10, c);
        }, g_caSel >= 0);
        if (v.empty()) drawTextPx("Pas de statistiques détaillées pour cette compétition.", x, y + 4, 10, C_DIM);
    }
    drawFooter(g_caSel >= 0 ? "Haut/Bas : défiler   Tab : affichage   Retour : liste des compétitions" : "OK : consulter   Tab : affichage   Retour");
    if (IN.back) { if (g_caSel >= 0) g_caSel = -1; else openHub(); }
}

// ---------------- légendes de la Coupe du monde : choix de l'édition
static ListW g_lgLW;
static void screenLegends() {
    drawBackground("Légendes de la Coupe du monde");
    drawTextCentered("Phases finales de 1930 à 2026 : joueurs, formule, stades et règles de l'époque (points, remplacements).", VW / 2, 32, 10, C_DIM);
    int s = listRun(g_lgLW, NUM_LEGENDS, 20, 48, VW - 40, 22, 13, [&](int i, int x, int y, bool sel) {
        const LegendEdition& E = LEGENDS[i];
        drawTextPx(fmt("%d", E.year), x + 6, y + 1, 10, sel ? BLACK : C_HI);
        drawTextPx(fitText(E.host, 170, 10), x + 46, y + 1, 10, sel ? BLACK : C_TXT);
        drawTextPx(fitText(legendFormatName(i), 250, 10), x + 222, y + 1, 10, sel ? BLACK : C_DIM);
        std::string r = fmt("%d pts / vict. - %s", E.ptsWin, E.subs ? fmt("%d rempl.", E.subs).c_str() : "sans rempl.");
        drawTextPx(r, x + VW - 40 - 8 - textWidth(r, 10), y + 1, 10, sel ? BLACK : C_DIM);
    });
    drawFooter("OK : choisir vos sélections (1 à 4)   Retour");
    if (IN.back) { g_screen = SC_MAIN; return; }
    if (s < 0) return;
    g_legendEd = s;
    g_intlCandidates = legendTeams(s);
    g_intlSel.clear();
    g_euroModePick = false; g_leagueModePick = false; g_coachModePick = false;
    openPick(PM_INTL);
    g_stack.back().title = fmt("Coupe du monde %d : choisissez vos sélections (1 à 4)", LEGENDS[s].year);
}
