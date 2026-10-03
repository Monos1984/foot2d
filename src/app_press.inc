// ------------------------------------------------------------------ carrière : forme des équipes et conférence de presse d'avant-match

// forme : derniers résultats d'une équipe dans toutes les compétitions de la saison (du plus ancien au plus récent) ; 'V', 'N', 'D'
static std::string teamForm(int team, int n = 5) {
    static int cTeam = -1; static double cNow = -1; static int cCount = -1; static std::string cRes;
    const Season& S = g_career.season;
    int played = 0; for (auto& C : S.comps) for (auto& m : C.matches) if (m.played) played++;
    if (team == cTeam && S.now == cNow && played == cCount) return cRes.substr(cRes.size() > (size_t)n ? cRes.size() - n : 0);
    std::vector<std::pair<double, char>> v;
    for (auto& C : S.comps) {
        for (auto& st : C.stages) for (auto& R : st.rounds) for (int x : R.m) {
            if (x < 0 || x >= (int)C.matches.size()) continue;
            const MatchRes& m = C.matches[x];
            if (!m.played || (m.home != team && m.away != team)) continue;
            int gf = m.home == team ? m.hg : m.ag, ga = m.home == team ? m.ag : m.hg;
            char r = gf > ga ? 'V' : gf < ga ? 'D' : 'N';
            if (gf == ga && m.ph >= 0) r = ((m.home == team) == (m.ph > m.pa)) ? 'V' : 'D';
            v.push_back({ R.time, r });
        }
    }
    std::stable_sort(v.begin(), v.end(), [](const std::pair<double, char>& a, const std::pair<double, char>& b) { return a.first < b.first; });
    std::string s; for (auto& e : v) s += e.second;
    cTeam = team; cNow = S.now; cCount = played; cRes = s;
    return s.substr(s.size() > (size_t)n ? s.size() - n : 0);
}
static void drawForm(const std::string& f, int x, int y) {
    for (int i = 0; i < (int)f.size(); i++) {
        Color c = f[i] == 'V' ? Color{ 60, 170, 80, 255 } : f[i] == 'D' ? Color{ 200, 60, 60, 255 } : Color{ 120, 120, 135, 255 };
        DrawRectangle(x + i * 12, y, 11, 11, c);
        drawTextCentered(std::string(1, f[i]), x + i * 12 + 6, y + 1, 10, WHITE, false);
    }
}

// conférence de presse : une question selon le contexte, trois tons de réponse, des effets annoncés
static int g_pressQ = 0, g_pressA = -1, g_pressComp = -1, g_pressMatch = -1;
static float g_pressT = 0;
static std::string g_pressEffect, g_pressEffect2;
// rivalité connue des supporters entre deux clubs (type RV_*), sinon -1 ; repli sur la même ville sans le système de supporters
static int pressRivalry(int u, int o) {
    if (g_career.supportersEnabled()) {
        auto& P = g_career.supportersProfile(u);
        int best = -1, bi = 0;
        for (auto& R : P.rivalries) if (R.clubId == o && R.intensity >= 20 && R.intensity > bi) { bi = R.intensity; best = R.type; }
        if (best >= 0) return best;
    }
    if (g_world.teams[u].town == g_world.teams[o].town && !g_world.teams[u].town.empty()) return RV_LOCAL;
    return -1;
}
static int pressQuestion(int comp, int match) {
    const Season& S = g_career.season;
    const Competition& C = S.comps[comp];
    const MatchRes& m = C.matches[match];
    int u = g_career.userTeam, o = m.home == u ? m.away : m.home;
    std::string f = teamForm(u, 3);
    if (C.format != FMT_LEAGUE) return 5;                                                   // coupe
    int rv = pressRivalry(u, o);
    if (rv == RV_LOCAL) return 0;                                                            // derby local
    if (rv == RV_HISTORICAL) return 7;                                                       // rivalité historique
    if (rv == RV_SPORTING) return 8;                                                         // rivalité sportive
    if (rv == RV_RECENT) return 9;                                                           // rivalité récente
    if (f.size() == 3 && f == "DDD") return 1;
    if (f.size() == 3 && f.find('D') == std::string::npos && f.find('N') == std::string::npos) return 2;
    if (g_world.teams[o].rating > g_world.teams[u].rating + 6) return 3;
    if (g_world.teams[o].rating + 6 < g_world.teams[u].rating) return 4;
    return 6;
}
static bool pressWanted(int comp, int match) {
    if (g_career.kind != CK_CLUB || g_career.life.isPlayer || g_career.sportingMode()) return false;
    if (getenv("FOOT_TEST") && !getenv("FOOT_PRESS")) return false;
    auto& MS = g_career.monthly;
    if (MS.pressYear == g_career.year && MS.pressComp == comp && MS.pressMatch == match) return false;   // déjà faite pour ce match (sauvegardé)
    int q = pressQuestion(comp, match);
    if (q == 0 || q == 1 || q == 5 || q == 7) return true;                      // derby, crise, coupe, rivalité historique : toujours
    return ((comp * 7919 + match * 104729) >> 3) % 3 == 0 || getenv("FOOT_PRESS");   // sinon environ un match sur trois
}
static void openPress(int comp, int match) {
    g_pressComp = comp; g_pressMatch = match; g_career.monthly.pressYear = g_career.year; g_career.monthly.pressComp = comp; g_career.monthly.pressMatch = match; g_pressQ = pressQuestion(comp, match); g_pressA = -1; g_pressT = 0; g_pressEffect.clear();
    g_screen = SC_PRESS;
}
static void pressApply(int a) {
    Team& U = g_world.teams[g_career.userTeam];
    int morale = 0, conf = 0, fans = 0;
    switch (g_pressQ * 3 + a) {
    case 0: morale = 1; fans = 1; break;      case 1: morale = 3; fans = 3; break;      case 2: morale = 4; fans = 5; conf = -2; break;   // derby
    case 3: morale = 2; conf = 1; break;      case 4: morale = 1; conf = -1; break;     case 5: morale = -2; conf = -3; fans = 2; break;  // crise
    case 6: morale = 2; break;                case 7: morale = 3; fans = 2; conf = 1; break; case 8: morale = 1; fans = 3; conf = -1; break; // bonne forme
    case 9: morale = 1; break;                case 10: morale = 3; fans = 2; break;     case 11: morale = 2; fans = 3; conf = -1; break;  // outsider
    case 12: morale = 2; conf = 1; break;     case 13: morale = 2; break;               case 14: morale = -1; fans = 1; conf = -2; break; // favori
    case 15: morale = 1; conf = 1; break;     case 16: morale = 3; fans = 3; break;     case 17: morale = 2; fans = 4; conf = -1; break;  // coupe
    case 21: morale = 1; fans = 2; break;     case 22: morale = 3; fans = 4; break;     case 23: morale = 4; fans = 6; conf = -2; break;  // rivalité historique
    case 24: morale = 2; break;               case 25: morale = 3; fans = 2; break;     case 26: morale = 3; fans = 3; conf = -1; break;  // rivalité sportive
    case 27: morale = 2; conf = 1; break;     case 28: morale = 2; fans = 2; break;     case 29: morale = 2; fans = 3; conf = -2; break;  // rivalité récente
    default: morale = a == 0 ? 1 : a == 1 ? 2 : 2; fans = a == 2 ? 2 : 0; conf = a == 2 ? -1 : 0; break;
    }
    // réaction individuelle légèrement modulée par la personnalité (susceptibilité, leadership, sang-froid, grands matchs)
    bool big = g_pressQ == 0 || g_pressQ == 5 || g_pressQ >= 7;
    int hyped = 0, tense = 0;
    for (auto& p : U.squad) {
        int d = morale;
        if (g_career.personalityEnabled()) {
            auto Q = personalityTraits(g_career, p);
            if (a == 2) { if (Q.sensitivity >= 75 && Q.composure < 45) { d -= 2; tense++; } else if (big && Q.bigMatchTemperament >= 78) { d += 1; hyped++; } }
            if (a == 0 && Q.leadership >= 70) d += 1;                    // les cadres apprécient le discours mesuré
            if (morale < 0 && (Q.leadership >= 75 || Q.composure >= 75)) d = std::min(0, d + 1);
            d = std::max(morale - 2, std::min(morale + 2, d));
        }
        p.morale = (uint8_t)std::max(10, std::min(100, p.morale + d));
    }
    g_career.mgr.confidence = std::max(0, std::min(100, g_career.mgr.confidence + conf));
    if (fans && g_career.supportersEnabled()) { auto& P = g_career.supportersProfile(g_career.userTeam); P.managerTrust = std::max(0, std::min(100, P.managerTrust + fans)); }
    auto sg = [](int v) { return v > 0 ? fmt("+%d", v) : fmt("%d", v); };
    g_pressEffect = "Moral du groupe " + sg(morale) + "   Confiance du président " + sg(conf) + (g_career.supportersEnabled() ? "   Supporters " + sg(fans) : std::string());
    g_pressEffect2.clear();
    if (hyped) g_pressEffect2 = fmt("%d joueur%s habitué%s aux grands rendez-vous s'en nourri%s.", hyped, hyped > 1 ? "s" : "", hyped > 1 ? "s" : "", hyped > 1 ? "ssent" : "t");
    if (tense) g_pressEffect2 += std::string(g_pressEffect2.empty() ? "" : "  ") + fmt("%d joueur%s plus susceptible%s se crispe%s.", tense, tense > 1 ? "s" : "", tense > 1 ? "s" : "", tense > 1 ? "nt" : "");
}
static void screenPress() {
    const Season& S = g_career.season;
    if (g_pressComp < 0 || g_pressComp >= (int)S.comps.size()) { g_screen = SC_HUB; return; }
    const Competition& C = S.comps[g_pressComp];
    const MatchRes& m = C.matches[g_pressMatch];
    int u = g_career.userTeam, o = m.home == u ? m.away : m.home;
    const Team& U = g_world.teams[u]; const Team& O = g_world.teams[o];
    float t = (float)GetTime(); g_pressT += GetFrameTime();
    drawBackground("Conférence de presse");
    // mur des partenaires aux couleurs du club
    Color c1 = hexc(U.home.shirt), c2 = hexc(U.home.shirt2 ? U.home.shirt2 : U.home.shorts);
    DrawRectangle(0, 26, VW, 170, Color{ 230, 232, 238, 255 });
    for (int r = 0; r < 6; r++) for (int k = 0; k < 7; k++) {
        int x = k * 96 + (r % 2) * 48 - 20, y = 30 + r * 27;
        std::string lab = (k + r) % 2 ? U.shortName : (U.sponsor.empty() ? std::string("PIXEL COLA") : U.sponsor);
        drawTextCentered(fitText(lab, 90, 10), x + 45, y + 8, 10, (k + r) % 2 ? c1 : Color{ 120, 120, 140, 255 }, false);
    }
    // table, micros, entraîneur
    Kit suit; suit.shirt = 0x22252E; suit.shirt2 = 0xE8E8E8; suit.shorts = 0x1A1C22; suit.socks = 0x111111;
    drawPortrait(VW / 2 - 40, 98, 80, 1, 0, 0, suit, 99u + (unsigned)u);
    DrawRectangle(60, 178, VW - 120, 24, c1); DrawRectangle(60, 178, VW - 120, 3, c2);
    drawTextCentered(U.name, VW / 2, 185, 10, colorBrightUI(U.home.shirt) > 0.55f ? BLACK : WHITE, false);
    for (int k = 0; k < 5; k++) { int mx = VW / 2 - 50 + k * 25; DrawLine(mx, 178, mx + (k - 2) * 2, 166, Color{ 40, 40, 44, 255 }); DrawRectangle(mx + (k - 2) * 2 - 3, 160, 7, 7, k % 2 ? Color{ 200, 40, 40, 255 } : Color{ 40, 80, 200, 255 }); }
    // journalistes de dos et flashs
    DrawRectangle(0, 202, VW, 120, Color{ 30, 30, 40, 255 });
    for (int k = 0; k < 14; k++) {
        int x = 20 + k * 44 + (k % 2) * 10, y = 214 + (k % 2) * 6;
        DrawCircle(x, y, 9, Color{ 20, 18, 24, 255 }); DrawCircle(x, y - 1, 8, hexc(k % 3 ? 0x2A1E14 : 0x5A5A5A));
        DrawRectangle(x - 11, y + 7, 22, 30, Color{ 40, 44, 60, 255 });
        if (((int)(t * 4) + k * 3) % 11 == 0) { DrawCircle(x + 6, y - 12, 9, Color{ 255, 255, 255, 170 }); DrawRectangle(x + 2, y - 14, 8, 5, Color{ 20, 20, 20, 255 }); }
    }
    // question, puis réponse
    static const char* Q[10] = {
        "C'est un derby : comment abordez-vous ce match face à %s ?",
        "Trois défaites de suite. Votre place est-elle menacée avant d'affronter %s ?",
        "Votre équipe est en grande forme. Jusqu'où peut-elle aller, en commençant par %s ?",
        "Face à %s, sur le papier plus fort que vous, quelles sont vos chances ?",
        "Vous êtes favoris contre %s. La pression est-elle sur vos épaules ?",
        "En coupe tout est possible. Allez-vous faire tourner face à %s ?",
        "Quel est votre objectif pour le match contre %s ?",
        "Une rivalité historique : que représente ce match face à %s pour le club ?",
        "%s est devenu votre grand rival sportif ces dernières saisons. Qui a l'ascendant ?",
        "Le dernier match contre %s a laissé des traces. Comment gérer la tension ?" };
    static const char* A[10][3] = {
        { "Un match comme un autre, on reste concentrés.", "C'est NOTRE ville, on va le prouver sur le terrain.", "Ils vont souffrir. Ce soir, on va les écraser." },
        { "Je fais confiance au groupe, on va travailler.", "On va s'en sortir, j'en suis convaincu.", "Ce sont les joueurs qui doivent se regarder dans la glace." },
        { "On prend match après match, sans s'enflammer.", "Pourquoi pas viser plus haut ? On a les moyens.", "Personne ne peut nous arrêter cette saison." },
        { "Ils sont favoris, nous jouerons sans pression.", "On a nos chances, on y croit.", "Les étiquettes ne gagnent pas les matchs. On va les surprendre." },
        { "Il n'y a pas de petit adversaire.", "Oui, et c'est normal : on veut gagner.", "On doit les balayer, rien d'autre." },
        { "On respecte cette compétition, l'équipe sera solide.", "On joue chaque match de coupe pour aller au bout.", "On veut ce trophée, et on ira le chercher." },
        { "Les trois points, en jouant juste.", "Gagner et faire plaisir à nos supporters.", "Montrer qui commande dans ce championnat." },
        { "On respecte l'histoire, mais seul le terrain compte.", "Nos supporters attendent ce match toute l'année, on sera prêts.", "Notre histoire est plus belle que la leur, et on va le rappeler." },
        { "Chaque saison repart de zéro.", "On a progressé, on peut prendre le dessus.", "Ils courent derrière nous depuis des années." },
        { "On tourne la page, on reste calmes.", "On a retenu les leçons, on sera prêts.", "S'ils veulent la bagarre, ils l'auront... sur le terrain." } };
    static const char* TONE[3] = { "Prudent", "Confiant", "Provocateur" };
    DrawRectangle(20, 222, VW - 40, 30, Color{ 250, 250, 245, 245 }); DrawRectangleLines(20, 222, VW - 40, 30, Color{ 40, 40, 50, 255 });
    drawTextPx("Journaliste :", 28, 225, 10, Color{ 120, 30, 30, 255 });
    std::string q = fmt(Q[g_pressQ], O.name.c_str());
    int ly = 237; for (auto& l : wrapText(q, VW - 60)) { if (ly > 247) break; drawTextPx(l, 28, ly - 1, 10, Color{ 30, 30, 40, 255 }); ly += 11; }
    if (g_pressA < 0) {
        for (int k = 0; k < 3; k++) {
            int y = 258 + k * 22;
            bool sel = button(20, y, VW - 40, 19, "", false) || (IsKeyPressed(KEY_ONE + k) && g_pressT > 0.3f);
            DrawRectangle(24, y + 3, 76, 13, k == 0 ? Color{ 60, 120, 200, 255 } : k == 1 ? Color{ 60, 160, 80, 255 } : Color{ 200, 70, 50, 255 });
            drawTextCentered(TONE[k], 62, y + 4, 10, WHITE, false);
            drawTextPx(fitText(std::string("« ") + A[g_pressQ][k] + " »", VW - 150, 10), 108, y + 4, 10, C_TXT);
            if (sel) { g_pressA = k; pressApply(k); g_career.season.news.push_back("Presse : " + g_world.teams[u].name + " - " + TONE[k] + " avant " + O.name + " : « " + A[g_pressQ][k] + " »"); }
        }
        drawFooter("Choisissez votre réponse (1/2/3)   Retour : passer la conférence");
        if (IN.back) { g_pressA = 3; startSetup(m.home, m.away, true, g_pressComp, g_pressMatch); }
        return;
    }
    DrawRectangle(20, 258, VW - 40, 44, Color{ 6, 12, 28, 230 });
    drawTextPx(fitText(std::string("Vous : « ") + A[g_pressQ][std::min(2, g_pressA)] + " »", VW - 60, 10), 30, 264, 10, C_HI);
    drawTextPx(g_pressEffect, 30, 278, 10, C_GOOD);
    if (!g_pressEffect2.empty()) drawTextPx(fitText(g_pressEffect2, VW - 60, 10), 30, 290, 10, C_DIM);
    bool go = button(VW / 2 - 100, 310, 200, 16, "Aller à l'avant-match");
    drawFooter("OK : continuer");
    if (go || ((IN.ok || IN.start) && g_pressT > 0.3f)) startSetup(m.home, m.away, true, g_pressComp, g_pressMatch);
}

// trophées mensuels du championnat de l'équipe dirigée : joueur du mois (buts marqués dans le mois), entraîneur du mois (points pris)
static void monthlyAwards() {
    if (g_career.kind != CK_CLUB || g_career.euroOnly) return;
    Season& S = g_career.season;
    if (S.now < 0) return;
    int month = (int)(S.now / 4.345);
    auto& MS = g_career.monthly;           // état sauvegardé avec la carrière
    int& lastMonth = MS.awMonth; int& lastYear = MS.awYear; int& lastTeam = MS.awTeam;
    std::map<int, int> goals0, pts0;
    for (auto& kv : MS.awGoals) goals0[kv.k] = kv.v;
    for (auto& kv : MS.awPts) pts0[kv.k] = kv.v;
    int p, q, g;
    if (g_career.tierOfTeam(g_career.userTeam, &p, &q, &g) < 0) return;
    int ci = g_career.pyramids[p].pools[q].comps[g];
    const Competition& C = S.comps[ci];
    auto snapshot = [&]() {
        goals0.clear(); pts0.clear();
        for (auto& r : C.table(0, 0)) { pts0[r.team] = r.pts; for (auto& pl : g_world.teams[r.team].squad) goals0[pl.id] = pl.goals; }
        MS.awGoals.clear(); MS.awPts.clear();
        for (auto& kv : goals0) MS.awGoals.push_back({ kv.first, kv.second });
        for (auto& kv : pts0) MS.awPts.push_back({ kv.first, kv.second });
    };
    if (lastYear != g_career.year || lastTeam != g_career.userTeam || lastMonth < 0) { lastYear = g_career.year; lastTeam = g_career.userTeam; lastMonth = month; snapshot(); return; }
    if (month == lastMonth) return;
    static const char* MN[12] = { "août", "septembre", "octobre", "novembre", "décembre", "janvier", "février", "mars", "avril", "mai", "juin", "juillet" };
    std::string mname = MN[((lastMonth % 12) + 12) % 12];
    // joueur du mois
    int bestT = -1, bestN = 0; const Player* best = nullptr;
    for (auto& r : C.table(0, 0)) for (auto& pl : g_world.teams[r.team].squad) {
        auto it = goals0.find(pl.id); int d = pl.goals - (it == goals0.end() ? pl.goals : it->second);
        if (d > bestN) { bestN = d; best = &pl; bestT = r.team; }
    }
    // entraîneur du mois
    int coachT = -1, coachP = 0;
    for (auto& r : C.table(0, 0)) { auto it = pts0.find(r.team); int d = r.pts - (it == pts0.end() ? r.pts : it->second); if (d > coachP) { coachP = d; coachT = r.team; } }
    if (best && bestN >= 2) {
        S.news.push_back(fmt("Joueur du mois de %s en %s : %s (%s), %d buts.", mname.c_str(), C.name.c_str(), best->name.c_str(), g_world.teams[bestT].name.c_str(), bestN));
        if (bestT == g_career.userTeam) {
            for (auto& pl : g_world.teams[bestT].squad) if (pl.id == best->id) pl.morale = (uint8_t)std::min(100, pl.morale + 10);
            toast("Joueur du mois : " + best->name + " !");
        }
    }
    if (coachT >= 0 && coachP >= 4) {
        S.news.push_back(fmt("Entraîneur du mois de %s en %s : l'entraîneur de %s (%d points).", mname.c_str(), C.name.c_str(), g_world.teams[coachT].name.c_str(), coachP));
        if (coachT == g_career.userTeam) { g_career.mgr.confidence = std::min(100, g_career.mgr.confidence + 3); toast("Vous êtes élu entraîneur du mois !"); }
    }
    lastMonth = month; snapshot();
}
