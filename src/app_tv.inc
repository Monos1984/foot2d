// ------------------------------------------------------------------ contexte d'un match : date, météo, horaire, télévision ; présentation TV ; article de presse
struct TvInfo { std::string channel, com1, com2; };
static TvInfo g_tv;
static void launchMatch();
static void finishMatchToResult();
static std::vector<std::string> wrapText(const std::string& s, int maxw);

// affluence et influence du stade (public, taille, remplissage) sur l'équipe qui reçoit
static void crowdContext(MatchSetup& s) {
    const Team& H = g_world.teams[s.home];
    int cap = H.sta.init && H.sta.capacity() > 0 ? H.sta.capacity() : H.kind == TK_NATION ? 60000 : 5000;
    if (s.neutral) cap = std::max(cap, 40000);
    s.attendance = (int)(cap * s.crowdFill);
    s.homeBoost = s.neutral || s.training ? 0.f : 0.008f + 0.03f * s.crowdFill * std::min(1.f, 0.35f + cap / 60000.f);
}

static void matchContext(MatchSetup& s, bool career, int comp, int match) {
    s.tv = false; s.night = false; s.channel.clear(); s.kickoffTime = "20h45"; s.kickoffDate.clear(); s.weather = 0;
    const Team& H = g_world.teams[s.home];
    s.anthems = H.kind == TK_NATION && g_world.teams[s.away].kind == TK_NATION;   // hymnes entre sélections nationales
    ensureStadium(s.home);
    float fill = 0.35f + H.rating / 160.f;
    if (!career || comp < 0) {
        s.referee = (int)(hashStr(g_world.teams[s.home].name + g_world.teams[s.away].name) % NUM_REFEREES);
        s.crowdFill = std::max(0.2f, std::min(1.f, fill));
        s.tv = g_world.teams[s.home].kind == TK_NATION || H.rating >= 70;
        s.proMedia = s.tv || H.rating >= 62;
        if (s.tv) { s.channel = "FRANCE SPORT"; g_tv = { "FRANCE SPORT", "Michel Larrieu", "Bernard Castagnet" }; }
        s.night = true;
        static const int PM[6] = { 2, 1, 3, 3, 5, -1 };        // météo cohérente avec le terrain choisi
        s.meteo = s.pitch >= 0 && s.pitch < 5 ? PM[s.pitch] : meteoForMonth(0, 0, (uint64_t)g_rng.range(1, 1 << 30));
        if (s.pitch == 3 && g_rng.chance(0.4f)) s.meteo = 4;
        s.weather = s.meteo == 3 || s.meteo == 4 ? 2 : s.meteo == 5 ? 3 : 0;
        crowdContext(s);
        supportersApplySetup(career?&g_career:nullptr,s,comp,match);
        return;
    }
    const Season& S = g_career.season;
    const Competition& C = S.comps[comp];
    double t = 0; for (auto& st : C.stages) for (auto& R : st.rounds) for (int x : R.m) if (x == match) t = R.time;
    int month = std::max(0, std::min(10, (int)(t / 4.2)));          // 0 août ... 10 juin
    bool winter = month >= 3 && month <= 7;
    int lvl = std::min(teamLevel(s.home), teamLevel(s.away));
    Rng r((uint64_t)(comp * 7919 + match * 31 + g_career.year));
    // horaire
    int ko = kickoffMinutes(comp, match);                      // calendrier réel de la journée
    int hour = (ko % 1440) / 60, minute = ko % 60;
    s.kickoffTime = fmt("%dh%02d", hour, minute);
    s.kickoffDate = kickoffText(comp, match, true);
    s.night = hour >= 20 || (winter && hour >= 17) || C.kind == 3 || C.kind == 7;
    // carrière : la météo dépend de la date et du climat du pays ; le terrain dépend de la météo et de la qualité de la pelouse
    {
        bool heated = lvl <= 1;       // pelouses chauffées en pro
        int turf = s.neutral ? 90 : turfQualityFor(s.home, month);
        int kind = s.neutral || !H.sta.init ? 0 : H.sta.pitch;   // 0 naturelle, 1 hybride, 2 synthétique
        int climate = 0;
        int mth = month;
        const char* cc = H.nation >= 0 && H.nation < NUM_NATIONS ? NATIONS[H.nation].code : "FRA";
        static const char* HOT[] = { "QAT", "KSA", "UAE", "EGY", "MAR", "ALG", "TUN", "MEX", "BRA", "NGA", "SEN", "CIV", "CMR", "GHA", "IRQ", "JOR", "IRN", "IND", "THA", "VIE", "IDN", "MAS", "COL", "VEN", "ECU", "PER", "CRC", "PAN", "HON", "JAM", "HAI", "CUW", "CPV", "COD", "ANG", "KUW", "BHR", "OMA", "LBY", "SUD", "MLI", "BFA", "GUI", "CAM", "TRI", "SLV", "GUA", "NCA", "DOM", "CUB", "PUR" };
        static const char* SOUTH[] = { "ARG", "URU", "CHI", "RSA", "AUS", "NZL", "PAR", "BOL" };
        static const char* NORTH[] = { "NOR", "SWE", "FIN", "RUS", "ISL", "EST", "LVA", "LTU", "CAN", "FRO", "UKR", "BLR", "KAZ" };
        for (auto c : HOT) if (!strcmp(c, cc)) climate = 1;
        for (auto c : SOUTH) if (!strcmp(c, cc)) climate = 2;
        for (auto c : NORTH) if (!strcmp(c, cc)) climate = 3;
        bool summerTourney = g_career.kind == CK_INTL || (g_career.kind == CK_CUSTOM && C.kind == 50);
        if (summerTourney) mth = 10;                                          // phases finales : en juin-juillet
        if (g_career.kind == CK_CUSTOM && C.kind == 50 && C.tag >= 0 && C.tag < NUM_LEGENDS) {
            int y = LEGENDS[C.tag].year;
            const char* hc = LEGENDS[C.tag].hostCode;
            climate = 0;
            for (auto c : HOT) if (!strcmp(c, hc)) climate = 1;
            if (y == 1994 || y == 2026) climate = 1;                         // étés américains
            if (y == 2022) { mth = 4; climate = 1; }                         // Qatar : novembre-décembre
            if (y == 1978 || y == 2010) climate = 2;                         // Argentine, Afrique du Sud : hiver austral
            if (y == 1950 || y == 2014) climate = 1;
        }
        s.meteo = meteoForMonth(mth, climate, (uint64_t)(comp * 7919 + match * 31 + g_career.year * 13));
        meteoApply(s, heated || summerTourney, turf);
        if (kind == 1 && s.pitch == 3 && r.chance(0.6f)) s.pitch = 2;   // hybride : draine mieux
        if (kind == 2) { if (s.pitch == 3) s.pitch = 2; if (s.pitch == 4) s.pitch = 0; }   // synthétique : ni boue ni gel
        s.turf = kind == 2 ? std::max(turf, 70) : turf;
        // phases finales internationales et Coupe du monde des légendes : pelouses de grande qualité
        if (summerTourney || s.neutral) s.turf = std::max(s.turf, 96);
        if (summerTourney && s.pitch == 3) s.pitch = 2;
    }
    // télévision : L1, L2, Coupe de France dès le 9e tour (32es), Coupe de la Ligue, Europe, supercoupes
    int st = C.stageOfMatch(match);
    bool cdf32 = C.name == "Coupe de France" && st >= 2;
    if (C.kind == 1 && lvl == 0 && teamLevel(s.home) == 0) { s.tv = true; g_tv = { "CANAL FOOT+", "Stéphane Guy-Durand", "Patrice Vannier" }; }
    else if (C.kind == 1 && teamLevel(s.home) == 1) { s.tv = true; g_tv = { "FOOT 2 TV", "Julien Morvan", "Éric Castel" }; }
    else if (cdf32) { s.tv = true; g_tv = { "FRANCE SPORT 2", "Michel Larrieu", "Bernard Castagnet" }; }
    else if (C.kind == 11) { s.tv = true; g_tv = { "FRANCE SPORT 3", "Laurent Maréchal", "Didier Roche" }; }
    else if (C.kind == 3 || C.kind == 8 || C.kind == 7) { s.tv = true; g_tv = { "TF SPORT", "Thierry Delorme", "Jean-Michel Lavaud" }; }
    else if (C.kind == 6) { s.tv = true; g_tv = { "CANAL FOOT+", "Stéphane Guy-Durand", "Patrice Vannier" }; }
    else if (C.kind == 50 || C.name.find("Coupe du monde") != std::string::npos) { s.tv = true; g_tv = { "TF SPORT", "Thierry Delorme", "Jean-Michel Lavaud" }; }   // Coupe du monde : toujours télévisée
    else if (g_career.kind == CK_INTL) { s.tv = true; g_tv = { "TF SPORT", "Thierry Delorme", "Jean-Michel Lavaud" }; }
    if (s.tv) s.channel = g_tv.channel;
    // médias pro (caméras, photographes) : football professionnel et grandes coupes ; plateau TV en Ligue des champions et phases finales
    s.proMedia = s.tv || lvl <= 2 || C.kind == 3 || C.kind == 8 || C.kind == 7 || C.kind == 6 || C.kind == 11 || C.kind == 14 || cdf32 || g_career.kind == CK_INTL;
    s.studio = C.kind == 3 || C.kind == 7 || (g_career.kind == CK_INTL && comp == g_career.finalComp) || C.kind == 50;
    if (C.kind == 50) s.proMedia = true;
    // photo officielle avant les matchs de coupe
    s.cupPhoto = C.format != FMT_LEAGUE && C.format != FMT_QUAL_GROUPS && C.kind != 12;
    // affluence prévue
    const StadiumInfo& SI = H.sta;
    if (SI.init && SI.capacity() > 0) fill = std::min(1.f, SI.fans * 0.85f / SI.capacity() + (C.format != FMT_LEAGUE ? 0.1f : 0) + (s.tv ? 0.05f : 0));
    s.crowdFill = std::max(0.15f, std::min(1.f, fill));
    crowdContext(s);
    supportersApplySetup(&g_career,s,comp,match);
}



// ---------------- présentation télévisée
// cabine des commentateurs : vue sur le stade, les deux voix de la chaîne présentent l'affiche
static void drawCommentaryBooth(float t) {
    const Team& H = g_world.teams[g_setup.home]; const Team& A = g_world.teams[g_setup.away];
    MatchSetup tmp = g_setup; Kit kits[2]; matchKits(tmp, kits);
    // la baie vitrée : tribunes, pelouse, joueurs à l'échauffement
    DrawRectangle(0, 0, VW, VH, Color{ 30, 30, 40, 255 });
    int wx = 20, wy = 30, ww = VW - 40, wh = 170;
    DrawRectangleGradientV(wx, wy, ww, 40, g_setup.night ? Color{ 10, 14, 40, 255 } : Color{ 120, 170, 230, 255 }, g_setup.night ? Color{ 30, 30, 70, 255 } : Color{ 180, 210, 240, 255 });
    fxCrowd(wx, wy + 24, ww, 50, hexc(kits[0].shirt), hexc(kits[1].shirt), t, 0.5f, (unsigned)(g_setup.home + g_setup.away), 4);
    DrawRectangle(wx, wy + 84, ww, wh - 84, Color{ 50, 140, 60, 255 });
    for (int k = 0; k < 8; k++) if (k % 2) DrawRectangle(wx + k * ww / 8, wy + 84, ww / 8, wh - 84, Color{ 56, 150, 66, 255 });
    DrawLine(wx, wy + 92, wx + ww, wy + 92, WHITE); DrawLine(wx + ww / 2, wy + 92, wx + ww / 2, wy + wh, WHITE);
    for (int k = 0; k < 8; k++) { float u = std::fmod(t * 0.2f + k * 0.13f, 1.f); int side = k % 2; drawPlayerSprite(wx + (side ? ww / 2 + 20 : 30) + (int)(u * (ww / 2 - 60)), wy + 110 + (k * 13) % 50, kits[side], k % 6, k % 7, side ? 3 : 2, (int)(t * 6) + k, PS_NORMAL, false, 0, 1); }
    if (g_setup.night) for (int k = 0; k < 2; k++) DrawCircleGradient(wx + (k ? ww - 30 : 30), wy + 10, 50, Color{ 255, 250, 210, 70 }, Color{ 255, 250, 210, 0 });
    for (int k = 0; k < 4; k++) DrawLine(wx + 60 + k * 150, wy, wx + 120 + k * 150, wy + wh, Color{ 255, 255, 255, 30 });   // reflets de la vitre
    DrawRectangleLines(wx, wy, ww, wh, Color{ 80, 80, 90, 255 }); DrawRectangle(wx + ww / 2 - 2, wy, 4, wh, Color{ 60, 60, 70, 255 });
    // pupitre, écrans, micros
    DrawRectangle(0, 214, VW, 146, Color{ 50, 40, 36, 255 }); DrawRectangle(0, 214, VW, 6, Color{ 90, 72, 60, 255 });
    for (int k = 0; k < 2; k++) {
        int cx = k ? VW - 190 : 190;
        Kit suit; suit.shirt = k ? 0x2A2F44 : 0x3A2A24; suit.shirt2 = 0xE8E8E8; suit.shorts = 0x1A1C22; suit.socks = 0x111111;
        drawPortrait(cx - 40, 140, 80, k ? 3 : 0, k ? 5 : 4, 0, suit, 31u + k);
        // casque et micro
        DrawRectangle(cx - 30, 138, 60, 5, Color{ 30, 30, 34, 255 }); DrawRectangle(cx - 34, 160, 8, 18, Color{ 30, 30, 34, 255 }); DrawRectangle(cx + 26, 160, 8, 18, Color{ 30, 30, 34, 255 });
        DrawLine(cx + 30, 176, cx + 10, 196, Color{ 30, 30, 34, 255 }); DrawCircle(cx + 9, 197, 4, Color{ 40, 40, 44, 255 });
        bool speaking = ((int)(t / 2.2f)) % 2 == k;
        if (speaking) DrawRectangle(cx - 6, 196, 12, 3 + (int)(std::fabs(std::sin(t * 14)) * 3), Color{ 60, 20, 20, 255 });
        DrawRectangle(cx - 50, 222, 100, 30, Color{ 20, 24, 34, 255 }); DrawRectangle(cx - 46, 226, 92, 22, Color{ 40, 90, 160, 255 });
        drawTextCentered(k ? fmt("%s", A.shortName.c_str()) : fmt("%s", H.shortName.c_str()), cx, 232, 10, WHITE, false);
        // bandeau du nom
        DrawRectangle(cx - 90, 262, 180, 18, speaking ? Color{ 190, 30, 40, 255 } : Color{ 60, 60, 80, 255 });
        drawTextCentered(fitText(k ? g_tv.com2 : g_tv.com1, 176, 10), cx, 266, 10, WHITE, false);
        drawTextCentered(k ? "Consultant" : "Commentaires", cx, 284, 10, C_DIM, false);
    }
    DrawRectangle(0, 0, VW, 24, Color{ 180, 20, 30, 240 });
    drawTextShadow(g_tv.channel, 10, 3, 20, WHITE);
    drawTextPx("EN DIRECT DE LA CABINE", VW - 160, 8, 10, ((int)(t * 2)) % 2 ? WHITE : Color{ 255, 200, 200, 255 });
    // phrase d'ouverture
    std::string l1 = "Bonsoir à tous et bienvenue " + std::string(g_setup.stadium.empty() ? "au stade" : "au " + g_setup.stadium) + " pour " + H.name + " - " + A.name + " !";
    std::string l2 = (g_setup.title.empty() ? std::string("Une belle affiche") : g_setup.title) + fmt(" : %d spectateurs attendus, ambiance %s.", g_setup.attendance, g_setup.supporterAtmosphere >= 70 ? "électrique" : g_setup.supporterAtmosphere >= 45 ? "chaleureuse" : "calme");
    std::string line = ((int)(t / 2.2f)) % 2 == 0 ? l1 : l2;
    DrawRectangle(20, 304, VW - 40, 34, Color{ 0, 0, 0, 170 });
    int ly = 308; for (auto& l : wrapText(line, VW - 60)) { if (ly > 330) break; drawTextPx(l, 30, ly, 10, WHITE); ly += 12; }
}

static void screenTvIntro(float dt) {
    static float t = 0; t += dt;
    static int phase = 0; static float pt = 0;
    const Team& H = g_world.teams[g_setup.home]; const Team& A = g_world.teams[g_setup.away];
    if (phase == 0) {   // la cabine des commentateurs, puis les compositions
        pt += dt;
        drawCommentaryBooth(t);
        drawFooter("OK : compositions   Retour : avant-match");
        if (pt > 6.5f || IN.ok || IN.start || IN.click) { phase = 1; pt = 0; IN.ok = IN.start = IN.click = false; }
        if (IN.back) { phase = 0; pt = 0; g_screen = SC_SETUP; }
        return;
    }
    // plateau TV
    for (int y = 0; y < VH; y += 4) { unsigned char k = (unsigned char)(y * 40 / VH); DrawRectangle(0, y, VW, 4, Color{ (unsigned char)(8 + k / 3), (unsigned char)(10 + k / 3), (unsigned char)(30 + k), 255 }); }
    for (int i = 0; i < 12; i++) { float a = t * 0.3f + i * 0.52f; DrawLine(VW / 2, -40, VW / 2 + (int)(std::cos(a) * 700), VH, Color{ 60, 90, 200, 30 }); }
    DrawRectangle(0, 18, VW, 26, Color{ 180, 20, 30, 230 });
    drawTextShadow(g_tv.channel, 14, 22, 20, WHITE);
    drawTextPx("EN DIRECT", VW - 90, 26, 10, ((int)(t * 2)) % 2 ? WHITE : Color{ 255, 200, 200, 255 });
    drawTextCentered(fitText(g_setup.title, VW - 40, 10), VW / 2, 52, 10, C_HI);
    drawTextCentered("En direct du " + g_setup.stadium + "  -  " + (g_setup.kickoffDate.empty() ? "coup d'envoi " + g_setup.kickoffTime : g_setup.kickoffDate) + "  -  " + METEO_NAMES[g_setup.meteo], VW / 2, 66, 10, C_TXT);
    // équipes
    drawKitIcon(H.home, 40, 86, 4); drawKitIcon(A.home, VW - 72, 86, 4);
    drawTextCentered(fitText(H.name, 220, 20), 190, 90, 20, WHITE);
    drawTextCentered(fitText(A.name, 220, 20), VW - 190, 90, 20, WHITE);
    drawTextCentered(shortLevel(g_setup.home), 190, 112, 10, C_DIM, false);
    drawTextCentered(shortLevel(g_setup.away), VW - 190, 112, 10, C_DIM, false);
    drawTextCentered("VS", VW / 2, 94, 20, C_HI);
    if (g_setup.hasFirstLeg) drawTextCentered(fmt("Match aller : %d - %d", g_setup.aggAway, g_setup.aggHome), VW / 2, 116, 10, C_HI, false);
    // compositions
    for (int side = 0; side < 2; side++) {
        int team = side ? g_setup.away : g_setup.home;
        g_world.ensureSquad(team);
        const Team& T = g_world.teams[team];
        int f = g_setup.formation[side] >= 0 ? g_setup.formation[side] : T.formation;
        auto lu = g_world.pickLineup(team, f);
        int x = side ? VW / 2 + 20 : 30, y = 132;
        DrawRectangle(x - 6, y - 4, VW / 2 - 44, 12 * 11 + 18, Color{ 0, 0, 0, 120 });
        drawTextPx(std::string("Composition (") + FORMATIONS[f].name + ")", x, y, 10, C_HI); y += 12;
        for (int k = 0; k < 11 && k < (int)lu.size(); k++) {
            const Player& p = T.squad[lu[k]];
            drawTextPx(fmt("%2d  %s  ", p.num, posName(p.pos)) + fitText(p.name, 180, 10), x, y, 10, C_TXT);
            y += 11;
        }
    }
    const char* refName = g_setup.referee >= 0 && g_setup.referee < NUM_REFEREES ? REFEREES[g_setup.referee].name : "";
    drawTextPx(std::string("Arbitre : ") + refName, 30, VH - 32, 10, C_DIM);
    drawTextPx("Commentaires : " + g_tv.com1 + " et " + g_tv.com2, VW / 2 + 20, VH - 32, 10, C_DIM);
    drawFooter("OK : coup d'envoi   Retour : avant-match");
    if (IN.ok || IN.start || IN.click) {
        t = 0; phase = 0; pt = 0;
        if (g_setup.studio) { g_studioPhase = 0; g_studioT = 0; g_screen = SC_STUDIO; audioJingle(3); }
        else launchMatch();
    }
    if (IN.back) { phase = 0; pt = 0; g_screen = SC_SETUP; }
}

// ---------------- plateau TV (avant-match, mi-temps, fin de match)
static std::string formOf(int team) {
    if (!g_careerActive) return "";
    struct R { double t; char c; };
    std::vector<R> v;
    for (auto& C : g_career.season.comps) for (auto& st : C.stages) for (auto& Rd : st.rounds) for (int mi : Rd.m) {
        const MatchRes& m = C.matches[mi];
        if (!m.played || (m.home != team && m.away != team)) continue;
        int a = m.home == team ? m.hg : m.ag, b = m.home == team ? m.ag : m.hg;
        v.push_back({ Rd.time, a > b ? 'V' : a < b ? 'D' : 'N' });
    }
    std::sort(v.begin(), v.end(), [](const R& x, const R& y) { return x.t < y.t; });
    std::string f;
    for (size_t i = v.size() > 5 ? v.size() - 5 : 0; i < v.size(); i++) f += v[i].c;
    return f;
}
static std::string starOf(int team) {
    g_world.ensureSquad(team);
    const Team& T = g_world.teams[team];
    int b = -1, bv = -1;
    for (auto& p : T.squad) if (p.pos != POS_GK && p.overall() > bv && p.injured <= 0 && p.suspended <= 0) { bv = p.overall(); b = (int)(&p - &T.squad[0]); }
    return b >= 0 ? T.squad[b].name : "";
}
static void screenStudio(float dt) {
    g_studioT += dt;
    float t = g_studioT;
    const Team& H = g_world.teams[g_setup.home]; const Team& A = g_world.teams[g_setup.away];
    const Match* M = g_match.get();
    Color hc = hexc(H.home.shirt), ac = hexc(A.home.shirt);
    // ---- décor du plateau : fond, projecteurs, mur vidéo, écrans latéraux
    for (int y = 0; y < 200; y += 2) { unsigned char k = (unsigned char)(y * 40 / 200); DrawRectangle(0, y, VW, 2, Color{ (unsigned char)(8 + k / 3), (unsigned char)(10 + k / 3), (unsigned char)(30 + k), 255 }); }
    for (int i = 0; i < 7; i++) {   // arches lumineuses du décor
        int x = 20 + i * 100; DrawRectangle(x, 24, 6, 150, Color{ 30, 40, 90, 255 }); DrawRectangle(x + 2, 24, 2, 150, Color{ 80, 120, 255, (unsigned char)(90 + 60 * std::sin(t * 2 + i)) });
    }
    for (int i = 0; i < 5; i++) {   // faisceaux des projecteurs
        float a = std::sin(t * 0.6f + i * 1.7f) * 60.f;
        Vector2 top{ (float)(70 + i * 125), 18.f };
        DrawTriangle(top, Vector2{ top.x - 40 + a, 200.f }, Vector2{ top.x + 40 + a, 200.f }, Color{ 150, 180, 255, 14 });
        DrawRectangle((int)top.x - 5, 16, 10, 5, Color{ 200, 210, 230, 255 });
    }
    // mur vidéo : vue du stade animée
    int wx = VW / 2 - 170, wy = 28, ww = 340, wh = 92;
    DrawRectangle(wx - 4, wy - 4, ww + 8, wh + 8, Color{ 15, 15, 25, 255 });
    DrawRectangleLines(wx - 4, wy - 4, ww + 8, wh + 8, Color{ 90, 120, 200, 255 });
    DrawRectangle(wx, wy, ww, 34, Color{ 25, 25, 45, 255 });
    for (int k = 0; k < 420; k++) {   // tribunes : public qui scintille
        uint32_t h = (uint32_t)(k * 2654435761u); int px = wx + (int)(h % ww), py = wy + (int)((h >> 12) % 34);
        bool flash = ((int)(t * 8) + k) % 53 == 0;
        Color cc = flash ? WHITE : (k % 3 == 0 ? hc : k % 3 == 1 ? ac : Color{ 150, 150, 170, 255 });
        DrawRectangle(px, py, 2, 2, cc);
    }
    DrawRectangle(wx, wy + 34, ww, wh - 34, Color{ 40, 130, 50, 255 });
    for (int k = 0; k < 6; k++) DrawRectangle(wx, wy + 34 + k * 10, ww, 5, Color{ 48, 145, 58, 255 });
    DrawRectangleLines(wx + 20, wy + 40, ww - 40, wh - 46, Color{ 230, 240, 230, 200 });
    DrawLine(wx + ww / 2, wy + 40, wx + ww / 2, wy + wh - 6, Color{ 230, 240, 230, 200 });
    DrawCircleLines(wx + ww / 2, wy + 40 + (wh - 46) / 2, 14, Color{ 230, 240, 230, 200 });
    // incrustation : équipes et score
    DrawRectangle(wx + ww / 2 - 120, wy + wh - 30, 240, 22, Color{ 5, 10, 40, 220 });
    DrawRectangle(wx + ww / 2 - 120, wy + wh - 30, 4, 22, hc); DrawRectangle(wx + ww / 2 + 116, wy + wh - 30, 4, 22, ac);
    drawTextPx(H.shortName, wx + ww / 2 - 110, wy + wh - 25, 10, WHITE);
    drawTextPx(A.shortName, wx + ww / 2 + 110 - textWidth(A.shortName, 10), wy + wh - 25, 10, WHITE);
    std::string mid = g_studioPhase > 0 && M ? fmt("%d - %d", M->score[0], M->score[1]) : std::string("VS");
    drawTextCentered(mid, wx + ww / 2, wy + wh - 30, 20, C_HI, false);
    // écrans latéraux : maillots
    for (int side = 0; side < 2; side++) {
        int x = side == 0 ? 18 : VW - 118;
        const Team& T = side == 0 ? H : A;
        DrawRectangle(x, 40, 100, 70, Color{ 14, 20, 50, 255 });
        DrawRectangleLines(x, 40, 100, 70, side == 0 ? hc : ac);
        drawKitIcon(T.home, x + 38, 46, 3);
        drawTextCentered(fitText(T.name, 96, 10), x + 50, 94, 10, WHITE, false);
    }
    // ---- bureau des consultants
    Kit suit; suit.shirt = 0x222244; suit.shirt2 = 0x222244; suit.shorts = 0x1A1A30; suit.socks = 0x111111;
    Kit suit2 = suit; suit2.shirt = 0x3A3A3A; suit2.shirt2 = 0x3A3A3A;
    Kit dress = suit; dress.shirt = 0x8A1D3A; dress.shirt2 = 0x8A1D3A;
    int dy = 172;
    int speaker = std::max(0, (int)(t / 1.1f)) % 3;
    int bob[3]; for (int k = 0; k < 3; k++) bob[k] = k == speaker ? (int)(std::fabs(std::sin(t * 9)) * 2) : 0;
    drawPlayerSprite(VW / 2 - 120, dy + 8 - bob[0], suit2, 2, 1, 0, 0, PS_NORMAL, false, 0, 4);
    drawPlayerSprite(VW / 2, dy + 4 - bob[1], dress, 0, 2, 0, 0, PS_NORMAL, false, 0, 4);
    drawPlayerSprite(VW / 2 + 120, dy + 8 - bob[2], suit2, 1, 0, 0, 0, PS_NORMAL, false, 0, 4);
    // bureau en verre avec bandeau lumineux
    DrawRectangle(VW / 2 - 200, dy - 14, 400, 30, Color{ 180, 200, 235, 200 });
    DrawRectangle(VW / 2 - 200, dy - 14, 400, 3, Color{ 240, 245, 255, 255 });
    for (int x = 0; x < 400; x += 4) { float ph = std::sin(t * 3 + x * 0.05f); DrawRectangle(VW / 2 - 200 + x, dy + 10, 4, 3, ph > 0 ? hc : ac); }
    DrawRectangle(wx + 6, wy + 4, 110, 12, Color{ 200, 30, 40, 230 });
    drawTextPx(g_studioPhase == 2 ? "LE DÉBRIEF" : g_studioPhase == 1 ? "LA PAUSE" : "LE GRAND DIRECT", wx + 10, wy + 5, 10, WHITE);
    // sol brillant (reflet du bureau)
    DrawRectangle(0, dy + 16, VW, 200 - dy - 16, Color{ 12, 16, 44, 255 });
    DrawRectangle(VW / 2 - 200, dy + 17, 400, 6, Color{ 120, 150, 220, 40 });
    // plaques nominatives
    const char* NM[3] = { g_tv.com1.c_str(), "Claire Vasseur", g_tv.com2.c_str() };
    for (int k = 0; k < 3; k++) {
        int x = VW / 2 - 120 + k * 120;
        DrawRectangle(x - 56, dy - 8, 112, 12, k == speaker ? Color{ 255, 205, 60, 255 } : Color{ 20, 30, 70, 255 });
        drawTextCentered(fitText(NM[k], 108, 10), x, dy - 7, 10, k == speaker ? BLACK : C_TXT, false);
    }
    // ---- public du plateau (premier plan)
    for (int row = 0; row < 2; row++) {
        for (int k = 0; k < 24; k++) {
            int x = 8 + k * 27 + (row ? 13 : 0), y = 204 + row * 8;
            uint32_t h = (uint32_t)((k + row * 31) * 2246822519u);
            float jump = std::fabs(std::sin(t * (2.f + (h % 7) * 0.3f) + k)) * ((h % 4 == 0) ? 3.f : 1.f);
            Color body = (h % 5 == 0) ? hc : (h % 5 == 1) ? ac : Color{ (unsigned char)(40 + h % 40), (unsigned char)(40 + (h >> 8) % 40), (unsigned char)(60 + (h >> 16) % 50), 255 };
            DrawRectangle(x - 7, y - (int)jump, 14, 10, body);
            DrawCircle(x, y - 4 - (int)jump, 4.5f, Color{ (unsigned char)(150 + h % 90), (unsigned char)(110 + h % 60), (unsigned char)(80 + h % 50), 255 });
            DrawCircle(x, y - 6 - (int)jump, 4.5f, Color{ 30, 22, 18, 255 });
            if (h % 6 == 0) {   // écharpe levée
                int sx = x + 5, sy = y - 16 - (int)jump;
                DrawRectangle(sx, sy, 3, 12, (h & 8) ? hc : ac); DrawRectangle(sx, sy + 3, 3, 2, WHITE);
            }
        }
    }
    // caméra au premier plan
    DrawRectangle(VW - 64, 176, 30, 16, Color{ 25, 25, 30, 255 }); DrawRectangle(VW - 72, 180, 8, 8, Color{ 60, 60, 70, 255 });
    DrawRectangle(VW - 50, 192, 3, 20, Color{ 40, 40, 45, 255 }); DrawRectangle(VW - 36, 178, 2, 2, ((int)(t * 2)) % 2 ? RED : Color{ 90, 20, 20, 255 });
    // bandeau supérieur
    DrawRectangle(0, 0, VW, 14, Color{ 10, 20, 70, 240 });
    drawTextPx(g_tv.channel + "  -  " + (g_studioPhase == 0 ? "AVANT-MATCH" : g_studioPhase == 1 ? "LA MI-TEMPS" : "APRÈS-MATCH"), 8, 2, 10, WHITE);
    DrawCircle(VW - 76, 7, 3, ((int)(t * 2)) % 2 ? RED : Color{ 120, 20, 20, 255 });
    drawTextPx("EN DIRECT", VW - 68, 2, 10, WHITE);
    // texte du plateau (apparition progressive, ligne par ligne)
    std::vector<std::string> L;
    if (g_studioPhase == 0) {
        L.push_back("Bonsoir à tous ! Bienvenue pour " + g_setup.title + ".");
        std::string fh = formOf(g_setup.home), fa = formOf(g_setup.away);
        if (!fh.empty() || !fa.empty()) L.push_back("Forme du moment : " + H.shortName + " " + (fh.empty() ? "-" : fh) + ", " + A.shortName + " " + (fa.empty() ? "-" : fa) + ".");
        L.push_back("Joueurs à suivre : " + starOf(g_setup.home) + " côté " + H.shortName + ", " + starOf(g_setup.away) + " côté " + A.shortName + ".");
        if (g_setup.hasFirstLeg) L.push_back(fmt("L'enjeu : au match aller, %s s'était imposé... ou pas : %d - %d. Tout reste possible !", A.shortName.c_str(), g_setup.aggAway, g_setup.aggHome));
        else L.push_back("L'enjeu est énorme ce soir, chaque point compte dans cette compétition.");
        float d = H.rating - A.rating;
        L.push_back(g_tv.com2 + " : « " + (d > 4 ? "Je vois " + H.shortName + " s'imposer, ils ont plus de talent." : d < -4 ? A.shortName + " est favori, mais attention à l'ambiance !" : "Match très ouvert, je dirais un nul... ou un exploit ! » ") + (std::abs(d) > 4 ? " »" : ""));
        L.push_back("Arbitre : " + std::string(g_setup.referee >= 0 && g_setup.referee < NUM_REFEREES ? REFEREES[g_setup.referee].name : "-") + ". Coup d'envoi dans quelques instants au " + g_setup.stadium + ".");
    } else if (M) {
        int tot = std::max(0.001f, M->possTime[0] + M->possTime[1]) > 0 ? 1 : 1; (void)tot;
        float poss = 100.f * M->possTime[0] / std::max(0.001f, M->possTime[0] + M->possTime[1]);
        L.push_back(fmt("%s : %s %d - %d %s.", g_studioPhase == 1 ? "C'est la pause" : "Coup de sifflet final", H.shortName.c_str(), M->score[0], M->score[1], A.shortName.c_str()));
        L.push_back(fmt("Possession : %d %% - %d %%.  Tirs : %d - %d.", (int)poss, 100 - (int)poss, M->shots[0], M->shots[1]));
        std::string sc;
        std::map<std::string, int> goals;
        for (auto& e : M->events) if (e.type == 0 || e.type == 3) { sc += (sc.empty() ? "" : ", ") + e.player + fmt(" (%d')", (int)e.minute + 1); if (e.type == 0) goals[e.player]++; }
        L.push_back(sc.empty() ? "Pas de but pour l'instant." : "Buts : " + sc + ".");
        int yc = 0, rc = 0; for (auto& e : M->events) { if (e.type == 1) yc++; if (e.type == 2) rc++; }
        if (yc || rc) L.push_back(fmt("Discipline : %d carton%s jaune%s, %d rouge%s.", yc, yc > 1 ? "s" : "", yc > 1 ? "s" : "", rc, rc > 1 ? "s" : ""));
        if (g_studioPhase == 1) {
            int d = M->score[0] - M->score[1];
            L.push_back(g_tv.com1 + " : « " + (d == 0 ? "Tout est encore possible en seconde période." : "Il faudra réagir après la pause pour " + (d > 0 ? A.shortName : H.shortName) + ".") + " »");
        } else {
            std::string mom; int mg = 0;
            for (auto& kv : goals) if (kv.second > mg) { mg = kv.second; mom = kv.first; }
            if (mom.empty()) mom = starOf(M->score[0] >= M->score[1] ? g_setup.home : g_setup.away);
            L.push_back("Homme du match : " + mom + " !");
            if (g_setup.hasFirstLeg) {
                int th = M->score[0] + g_setup.aggHome, ta = M->score[1] + g_setup.aggAway;
                L.push_back(fmt("Sur l'ensemble des deux matchs : %d - %d.", th, ta));
            }
            L.push_back("Merci de nous avoir suivis, bonne soirée à tous !");
        }
    }
    int ly = 226;
    DrawRectangle(0, 214, VW, VH - 214, Color{ 8, 10, 26, 255 });
    DrawRectangle(20, ly - 6, VW - 40, VH - ly - 14, Color{ 0, 0, 0, 150 });
    DrawRectangle(20, ly - 6, 3, VH - ly - 14, C_HI);
    int shown = std::min((int)L.size(), 1 + (int)(t / 1.1f));
    for (int i = 0; i < shown; i++) {
        for (auto& l : wrapText(L[i], VW - 60)) { if (ly > VH - 30) break; drawTextPx(l, 30, ly, 10, i == shown - 1 ? WHITE : C_TXT); ly += 12; }
    }
    drawFooter(g_studioPhase == 0 ? "OK : coup d'envoi" : g_studioPhase == 1 ? "OK : seconde période" : "OK : résultat");
    if (IN.ok || IN.start || IN.click) {
        if (shown < (int)L.size()) { g_studioT = 99; return; }
        if (g_studioPhase == 0) launchMatch();
        else if (g_studioPhase == 1) g_screen = SC_MATCH;
        else { g_screen = SC_MATCH; finishMatchToResult(); }
    }
}

// ---------------- article de presse
static std::vector<std::string> g_article;
static std::string g_articleTitle, g_paper;
static bool makeArticle(const PostInfo& P) {
    const Team& H = g_world.teams[P.home]; const Team& A = g_world.teams[P.away];
    Rng r((uint64_t)(P.home * 31 + P.away * 17 + P.hg * 7 + P.ag));
    int diff = P.hg - P.ag;
    int w = diff > 0 ? P.home : diff < 0 ? P.away : -1;
    if (diff == 0 && P.ph >= 0) w = P.ph > P.pa ? P.home : P.away;
    int l = w == P.home ? P.away : P.home;
    std::string wn = w >= 0 ? g_world.teams[w].name : "";
    std::string ln = w >= 0 ? g_world.teams[l].name : "";
    int lvlH = teamLevel(P.home) < 99 ? teamLevel(P.home) : 0;
    g_paper = lvlH <= 2 ? "LE JOURNAL DU FOOT" : "LA GAZETTE DE " + (H.town.empty() ? H.name : H.town);
    for (auto& ch : g_paper) ch = (char)toupper((unsigned char)ch);
    int ad = std::abs(diff);
    if (w < 0) {
        static const char* T[] = { "%s et %s se neutralisent", "Match nul entre %s et %s", "Chacun sa part pour %s et %s" };
        g_articleTitle = fmt(T[r.range(0, 2)], H.name.c_str(), A.name.c_str());
    } else if (P.ph >= 0 && diff == 0) g_articleTitle = fmt("%s passe aux tirs au but !", wn.c_str());
    else if (ad >= 3) { static const char* T[] = { "%s écrase %s", "Démonstration de %s face à %s", "%s corrige %s" }; g_articleTitle = fmt(T[r.range(0, 2)], wn.c_str(), ln.c_str()); }
    else if (ad == 2) { static const char* T[] = { "%s s'impose avec autorité contre %s", "Succès net de %s devant %s" }; g_articleTitle = fmt(T[r.range(0, 1)], wn.c_str(), ln.c_str()); }
    else { static const char* T[] = { "%s arrache la victoire face à %s", "%s s'impose de justesse contre %s", "Courte victoire de %s face à %s" }; g_articleTitle = fmt(T[r.range(0, 2)], wn.c_str(), ln.c_str()); }
    if (w == P.away && diff != 0 && g_world.teams[P.away].rating < H.rating - 8) g_articleTitle = fmt("Exploit de %s sur la pelouse de %s !", wn.c_str(), H.name.c_str());
    g_article.clear();
    std::string para = fmt("%s (%s) - %s (%s) : %d-%d", H.name.c_str(), shortLevel(P.home).c_str(), A.name.c_str(), shortLevel(P.away).c_str(), P.hg, P.ag);
    if (P.aet) para += " après prolongation";
    if (P.ph >= 0) para += fmt(" (%d-%d aux tirs au but)", P.ph, P.pa);
    para += ". " + P.title + ".";
    g_article.push_back(para);
    // buts
    std::string buts;
    for (auto& e : P.ev) if (e.type <= 1) buts += (buts.empty() ? "" : ", ") + e.txt + fmt(" (%d')", e.minute);
    if (!buts.empty()) g_article.push_back("Buts : " + buts + ".");
    else g_article.push_back("Aucun but n'a été marqué dans cette rencontre fermée.");
    std::string cards;
    int reds = 0;
    for (auto& e : P.ev) if (e.type == 2 || e.type == 3) { cards += (cards.empty() ? "" : ", ") + e.txt + (e.type == 3 ? " (rouge)" : ""); if (e.type == 3) reds++; }
    if (!cards.empty()) g_article.push_back("Avertissements et expulsions : " + cards + ".");
    // récit
    std::string rc;
    if (w < 0) rc = "Les deux équipes se sont rendu coup pour coup sans parvenir à se départager dans le temps réglementaire.";
    else if (ad >= 3) rc = wn + " a dominé la rencontre de bout en bout, sans jamais laisser " + ln + " revenir dans le match.";
    else rc = "Dans un match disputé, " + wn + " a su faire la différence au moment opportun, " + ln + " ayant manqué de réalisme.";
    if (reds) rc += fmt(" L'expulsion%s a pesé sur la physionomie de la partie.", reds > 1 ? "s" : "");
    if (P.poss >= 0) rc += fmt(" Possession : %d %% pour %s, %d tirs contre %d.", (int)std::lround(P.poss), H.shortName.c_str(), P.shots[0], P.shots[1]);
    g_article.push_back(rc);
    const StadiumInfo& SI = H.sta;
    if (SI.init) {
        int att = SI.lastAtt > 0 ? SI.lastAtt : (int)(SI.capacity() * 0.7f);
        g_article.push_back(fmt("Spectateurs : %d au ", att) + H.stadium + ".");
    }
    // ---- coupe : qualification ou élimination du club de l'utilisateur
    if (!g_careerActive || P.comp < 0 || P.comp >= (int)g_career.season.comps.size()) return false;
    const Competition& C = g_career.season.comps[P.comp];
    if (C.format == FMT_LEAGUE || C.kind == 12 || C.kind == 10) return false;
    int st = C.stageOfMatch(P.match);
    if (st < 0) return false;
    const MatchRes& m = C.matches[P.match];
    if (m.tie < 0 || m.tie >= (int)C.stages[st].ties.size()) return false;
    const Tie& tie = C.stages[st].ties[m.tie];
    if (tie.winner < 0) return false;
    int me = g_career.season.isControlled(tie.a) ? tie.a : g_career.season.isControlled(tie.b) ? tie.b : -1;
    if (me < 0) return false;
    int opp = me == tie.a ? tie.b : tie.a;
    if (opp < 0) return false;
    const Team& M = g_world.teams[me]; const Team& O = g_world.teams[opp];
    std::string rn = C.stages[st].name;
    bool fin = rn == "Finale";
    std::string next;
    if (!fin) {
        int k = -1;
        for (int i = 0; i < (int)C.koNames.size(); i++) if (C.koNames[i] == rn) k = i;
        if (k >= 0 && k + 1 < (int)C.koNames.size()) next = C.koNames[k + 1];
    }
    int lvM = teamLevel(me), lvO = teamLevel(opp);
    bool petit = lvM < 99 && lvO < 99 && lvM - lvO >= 2;       // exploit d'un club de division inférieure
    std::string cn = C.shortName.empty() ? C.name : C.name;
    g_paper = teamLevel(me) <= 2 ? "LE JOURNAL DU FOOT" : "LA GAZETTE DE " + (M.town.empty() ? M.name : M.town);
    for (auto& ch : g_paper) ch = (char)toupper((unsigned char)ch);
    std::vector<std::string> body;
    if (tie.winner == me) {
        if (fin) {
            bool fem = cn.rfind("Coupe", 0) == 0 || cn.rfind("Ligue", 0) == 0 || cn.rfind("Supercoupe", 0) == 0;
            g_articleTitle = "SACRE ! " + M.name + " remporte " + (fem ? "la " : "le ") + cn + " !";
            body.push_back(M.name + " soulève le trophée après sa victoire en finale face à " + O.name + ". Une soirée historique pour le club et ses supporters.");
        } else {
            static const char* T[] = { "%s : %s file %s !", "%s : %s poursuit l'aventure", "%s : %s au rendez-vous" };
            std::string nx = next.empty() ? "au tour suivant" : "en " + next;
            g_articleTitle = fmt(T[0], cn.c_str(), M.name.c_str(), nx.c_str());
            if (petit) g_articleTitle = cn + " : l'exploit de " + M.name + " qui sort " + O.name + " !";
            body.push_back(fmt("%s a validé son billet pour %s en éliminant %s (%s).", M.name.c_str(), next.empty() ? "le tour suivant" : next.c_str(), O.name.c_str(), shortLevel(opp).c_str()));
            if (petit) body.push_back("Le petit poucet a fait tomber un club de l'échelon supérieur : la ville entière rêve déjà du prochain tirage au sort.");
            else body.push_back("Le vestiaire savoure, mais " + (g_career.managerName.empty() ? std::string("l'entraîneur") : g_career.managerName) + " prévient : « Rien n'est fait, il faudra confirmer au prochain tour. »");
        }
    } else {
        g_articleTitle = fin ? cn + " : " + M.name + " s'incline en finale" : cn + " : fin de l'aventure pour " + M.name;
        if (lvO - lvM >= 2) g_articleTitle = cn + " : " + M.name + " tombe face au petit poucet " + O.name + " !";
        body.push_back(fmt("%s est éliminé %s par %s (%s).", M.name.c_str(), fin ? "en finale" : ("au stade des " + rn).c_str(), O.name.c_str(), shortLevel(opp).c_str()));
        body.push_back(fin ? "Une finale perdue qui laisse des regrets, mais un parcours qui restera dans les mémoires." : "Les joueurs devront vite digérer cette déception et se concentrer sur le championnat.");
    }
    g_article.insert(g_article.begin(), body.begin(), body.end());
    return true;
}

// illustration « photo » du match (pixel art en demi-teintes)
static void drawPressPhoto(int x, int y, int w, int h, const Kit& k1, const Kit& k2, uint64_t seed) {
    Rng r(seed);
    // tribune floue
    for (int yy = 0; yy < h / 2; yy += 2) for (int xx = 0; xx < w; xx += 2) {
        int v = 70 + (int)(r.f() * 60) - yy / 2;
        DrawRectangle(x + xx, y + yy, 2, 2, Color{ (unsigned char)(v + 10), (unsigned char)v, (unsigned char)(v - 10), 255 });
    }
    // pelouse
    for (int yy = h / 2; yy < h; yy += 2) {
        int v = 90 + ((yy / 6) % 2) * 12;
        DrawRectangle(x, y + yy, w, 2, Color{ (unsigned char)(v - 20), (unsigned char)(v + 20), (unsigned char)(v - 30), 255 });
    }
    // deux joueurs au duel + ballon
    drawPlayerSprite(x + w / 2 - 22, y + h - 10, k1, (int)(r.next() % 4), (int)(r.next() % 5), 2, 1, PS_KICK, false, 0, 4);
    drawPlayerSprite(x + w / 2 + 22, y + h - 8, k2, (int)(r.next() % 4), (int)(r.next() % 5), 3, 2, PS_NORMAL, false, 0, 4);
    DrawCircle(x + w / 2 + 2, y + h - 22, 5, WHITE); DrawCircleLines(x + w / 2 + 2, y + h - 22, 5, Color{ 40, 40, 40, 255 });
    // grain photo et teinte « papier journal »
    for (int k = 0; k < w * h / 18; k++) DrawPixel(x + (int)(r.f() * w), y + (int)(r.f() * h), Color{ 0, 0, 0, 40 });
    DrawRectangle(x, y, w, h, Color{ 120, 100, 60, 55 });
    DrawRectangleLines(x, y, w, h, Color{ 30, 30, 30, 255 });
}

static int g_articleBack = -1;      // écran de retour (-1 : résultat du match)
static bool g_articleNoMatch = false;
static void screenArticle() {
    const Color INK = { 25, 22, 20, 255 }, INK2 = { 70, 62, 55, 255 }, PAPER = { 240, 234, 216, 255 };
    ClearBackground(Color{ 40, 36, 32, 255 });
    int px = 16, py = 6, pw = VW - 32, ph = VH - 22;
    DrawRectangle(px + 3, py + 3, pw, ph, Color{ 0, 0, 0, 120 });
    DrawRectangle(px, py, pw, ph, PAPER);
    { Rng r(7); for (int k = 0; k < 900; k++) DrawPixel(px + (int)(r.f() * pw), py + (int)(r.f() * ph), Color{ 180, 170, 150, 90 }); }
    // ---- en-tête (manchette)
    drawTextCentered(fitText(g_paper, pw - 40, 20), VW / 2 + 1, py + 5, 20, INK, false);
    drawTextCentered(fitText(g_paper, pw - 40, 20), VW / 2, py + 5, 20, INK, false);
    DrawRectangle(px + 8, py + 27, pw - 16, 2, INK);
    std::string d = dateOf(g_careerActive ? g_career.season.now : 0, g_careerActive ? g_career.year : 2026);
    drawTextPx("Quotidien sportif - " + d, px + 10, py + 30, 10, INK2);
    std::string pr = fmt("N° %d  -  1,50", 20000 + (g_careerActive ? (int)(g_career.season.now * 7) + g_career.year : 0));
    drawTextPx(pr, px + pw - 12 - textWidth(pr, 10), py + 30, 10, INK2);
    DrawRectangle(px + 8, py + 41, pw - 16, 1, INK);
    // ---- gros titre
    int y = py + 45;
    for (auto& l : wrapText(g_articleTitle, (pw - 30) / 2)) {
        if (y > py + 70) break;
        drawTextPx(l, px + 12, y, 20, INK); drawTextPx(l, px + 13, y, 20, INK);
        y += 20;
    }
    y += 2;
    DrawRectangle(px + 8, y, pw - 16, 1, INK2); y += 4;
    int colTop = y;
    int rightW = g_articleNoMatch ? 0 : 170;
    int leftW = pw - 24 - rightW;
    // ---- photo
    const Team& H = g_world.teams[g_post.home]; const Team& A = g_world.teams[g_post.away];
    int phH = 70;
    if (!g_articleNoMatch) {
        drawPressPhoto(px + 12, y, leftW, phH, H.home, A.away, (uint64_t)(g_post.home * 131 + g_post.away * 7 + g_post.hg));
        y += phH + 2;
        drawTextPx(fitText("Photo : duel au " + (H.stadium.empty() ? H.name : H.stadium) + ".", leftW, 10), px + 12, y, 10, INK2);
        y += 12;
    }
    // ---- texte sur deux colonnes
    std::vector<std::string> lines;
    int cw = (leftW - 10) / 2;
    for (size_t i = 0; i < g_article.size(); i++) {
        for (auto& l : wrapText(g_article[i], cw)) lines.push_back(l);
        lines.push_back("");
    }
    int maxRows = (py + ph - 14 - y) / 11;
    if (g_articleNoMatch) {
        maxRows = std::min(maxRows, ((int)lines.size() + 1) / 2);
        // illustration : la coupe
        int cx0 = px + pw / 2 + 40, cy0 = y + maxRows * 11 + 14;
        if (cy0 < py + ph - 80) {
            Color G = { 200, 160, 40, 255 }, G2 = { 150, 110, 20, 255 };
            DrawRectangle(cx0, cy0, 60, 34, G); DrawRectangle(cx0 + 4, cy0 + 4, 8, 22, Color{ 240, 210, 110, 255 });
            DrawRectangle(cx0 - 10, cy0 + 4, 10, 4, G2); DrawRectangle(cx0 - 10, cy0 + 4, 4, 16, G2); DrawRectangle(cx0 - 10, cy0 + 16, 10, 4, G2);
            DrawRectangle(cx0 + 60, cy0 + 4, 10, 4, G2); DrawRectangle(cx0 + 66, cy0 + 4, 4, 16, G2); DrawRectangle(cx0 + 60, cy0 + 16, 10, 4, G2);
            DrawRectangle(cx0 + 24, cy0 + 34, 12, 10, G); DrawRectangle(cx0 + 12, cy0 + 44, 36, 8, G2);
            drawTextPx("Qui soulèvera les trophées cette saison ?", cx0 - 60, cy0 + 58, 10, INK2);
        }
    }
    for (int i = 0; i < (int)lines.size(); i++) {
        int col = i / std::max(1, maxRows);
        if (col > 1) break;
        int row = i % std::max(1, maxRows);
        int cx = px + 12 + col * (cw + 10);
        if (i == 0 && !lines[0].empty()) {   // lettrine
            std::string first = lines[0].substr(0, 1);
            DrawRectangle(cx, y - 1, 12, 12, INK);
            drawTextPx(first, cx + 3, y, 10, PAPER);
            drawTextPx(lines[0].substr(1), cx + 14, y + row * 11, 10, INK);
            continue;
        }
        drawTextPx(lines[i], cx, y + row * 11, 10, INK);
    }
    DrawRectangle(px + 12 + cw + 4, y, 1, maxRows * 11, INK2);
    // ---- encadré « fiche technique »
    if (!g_articleNoMatch) {
        int bx = px + pw - 12 - rightW, by = colTop;
        DrawRectangle(bx, by, rightW, ph - (by - py) - 12, Color{ 225, 216, 192, 255 });
        DrawRectangleLines(bx, by, rightW, ph - (by - py) - 12, INK);
        DrawRectangle(bx, by, rightW, 13, INK);
        drawTextCentered("FICHE TECHNIQUE", bx + rightW / 2, by + 2, 10, PAPER, false);
        int yy = by + 17;
        drawTextCentered(fitText(H.shortName + fmt("  %d - %d  ", g_post.hg, g_post.ag) + A.shortName, rightW - 8, 10), bx + rightW / 2, yy, 10, INK, false);
        yy += 12;
        if (g_post.ph >= 0) { drawTextCentered(fmt("(%d - %d aux t.a.b.)", g_post.ph, g_post.pa), bx + rightW / 2, yy, 10, INK2, false); yy += 11; }
        DrawRectangle(bx + 6, yy, rightW - 12, 1, INK2); yy += 4;
        if (H.sta.init) { drawTextPx(fitText(fmt("Spectateurs : %d", H.sta.lastAtt > 0 ? H.sta.lastAtt : (int)(H.sta.capacity() * 0.7f)), rightW - 10, 10), bx + 5, yy, 10, INK); yy += 11; }
        for (auto& e : g_post.ev) {
            if (yy > py + ph - 60) break;
            if (e.type > 3) continue;
            std::string ic = e.type <= 1 ? "But" : e.type == 2 ? "Av." : "Exp.";
            drawTextPx(fitText(fmt("%d' ", e.minute) + ic + " " + e.txt, rightW - 10, 10), bx + 5, yy, 10, e.type == 3 ? Color{ 150, 20, 20, 255 } : INK);
            yy += 11;
        }
        // notes des joueurs (étoiles)
        int ny = py + ph - 58;
        DrawRectangle(bx + 6, ny - 3, rightW - 12, 1, INK2);
        drawTextPx("LES NOTES", bx + 5, ny, 10, INK); ny += 11;
        std::vector<std::string> names;
        for (auto& e : g_post.ev) if (e.type == 0 && names.size() < 3) { std::string n = e.txt; size_t k = n.find(" ("); if (k != std::string::npos) n = n.substr(0, k); if (std::find(names.begin(), names.end(), n) == names.end()) names.push_back(n); }
        if (names.empty()) names.push_back("Les deux gardiens");
        Rng rr((uint64_t)(g_post.home * 3 + g_post.away * 5));
        for (auto& n : names) {
            int stars = 3 + (int)(rr.next() % 3);
            drawTextPx(fitText(n, rightW - 50, 10), bx + 5, ny, 10, INK);
            for (int k = 0; k < 5; k++) DrawRectangle(bx + rightW - 44 + k * 8, ny + 2, 6, 6, k < stars ? INK : Color{ 190, 180, 160, 255 });
            ny += 11;
        }
    }
    // ---- publicité en pied de page
    int ay = py + ph - 12;
    DrawRectangle(px + 8, ay - 2, pw - 16, 1, INK);
    static const char* ADS[] = { "PIXEL COLA : le goût de la victoire !", "CRAMPONS+ : soldes sur les chaussures moulées", "GOAL FM : tous les buts en direct", "STADE TV : abonnez-vous !" };
    drawTextPx(ADS[(g_post.home + g_post.away) % 4], px + 12, ay, 10, INK2);
    drawFooter("OK / Retour : continuer");
    if (IN.ok || IN.back || IN.click || IN.start) {
        if (g_articleBack >= 0) { g_screen = (Screen)g_articleBack; g_articleBack = -1; g_articleNoMatch = false; }
        else g_screen = SC_POST;
    }
}

// journal de début de saison : lieux des finales annoncés, favoris, objectif du club
static void showSeasonArticle() {
    if (!g_careerActive || g_career.kind != CK_CLUB) return;
    g_paper = "LE JOURNAL DU FOOT";
    g_articleTitle = fmt("Saison %d-%02d : c'est reparti !", g_career.year, (g_career.year + 1) % 100);
    g_article.clear();
    int u = g_career.userTeam;
    g_article.push_back("La nouvelle saison est lancée. " + g_world.teams[u].name + " évoluera en " + g_career.teamLevelName(u) + ". Objectif fixé par le président : " + g_career.objectiveText() + ".");
    int fr = -1; for (int i = 0; i < (int)g_career.pyramids.size(); i++) if (g_career.pyramids[i].country == "FRA" && g_career.pyramids[i].dom < 0) fr = i;
    // clubs français engagés en coupe d'Europe
    if (fr >= 0) {
        std::set<int> fra; for (auto& pl : g_career.pyramids[fr].pools) fra.insert(pl.clubs.begin(), pl.clubs.end());
        const Season& S = g_career.season;
        auto names = [&](int comp, const std::vector<const char*>& rounds) {
            std::vector<std::string> out;
            if (comp < 0 || comp >= (int)S.comps.size()) return out;
            const Competition& C = S.comps[comp];
            for (int k = 0; k < (int)C.entrants.size() && k < (int)rounds.size(); k++)
                for (int t : C.entrants[k]) if (fra.count(t)) out.push_back(g_world.teams[t].name + (rounds[k][0] ? std::string(" (") + rounds[k] + ")" : ""));
            return out;
        };
        auto join = [](const std::vector<std::string>& v) { std::string r; for (size_t i = 0; i < v.size(); i++) r += (i ? (i + 1 == v.size() ? " et " : ", ") : "") + v[i]; return r; };
        bool nf = g_career.opts.euroFormat != 0;
        auto ucl = nf ? names(g_career.ucl, { "1er tour de qualification", "2e tour de qualification", "3e tour de qualification", "barrage", "phase de ligue" })
                      : names(g_career.ucl, { "1er tour préliminaire", "2e tour préliminaire", "3e tour préliminaire", "phase de groupes" });
        auto uefa = nf ? names(g_career.uel, { "3e tour de qualification", "barrage", "phase de ligue" }) : names(g_career.uel, { "tour de qualification", "1er tour", "", "" });
        auto itt = nf ? names(g_career.uecl, { "1er tour de qualification", "2e tour de qualification", "3e tour de qualification", "barrage", "phase de ligue" })
                      : names(g_career.intertoto, { "1er tour", "2e tour", "3e tour" });
        std::string eu;
        const char* n3 = nf ? "Ligue Europa" : "Coupe UEFA";
        const char* n4 = nf ? "Ligue Conférence" : "Coupe Intertoto";
        if (!ucl.empty()) { g_article.push_back("En Ligue des champions, la France sera représentée par " + join(ucl) + "."); eu += "Ligue des champions : " + join(ucl) + ". "; }
        if (!uefa.empty()) { g_article.push_back(std::string("En ") + n3 + " : " + join(uefa) + "."); eu += std::string(n3) + " : " + join(uefa) + ". "; }
        if (!itt.empty()) { g_article.push_back(std::string("En ") + n4 + " : " + join(itt) + "."); eu += std::string(n4) + " : " + join(itt) + "."; }
        if (!eu.empty()) { std::string ln = "Coupes d'Europe " + fmt("%d-%02d", g_career.year, (g_career.year + 1) % 100) + " - clubs français : " + eu;
            auto& N = g_career.season.news; if (std::find(N.begin(), N.end(), ln) == N.end()) N.push_back(ln); }
        if (ucl.empty() && uefa.empty() && itt.empty()) g_article.push_back("Aucun club français n'est engagé en coupe d'Europe cette saison.");
    }
    if (fr >= 0) {
        int q = g_career.pyramids[fr].poolIndex(0, 0);
        if (q >= 0) {
            std::vector<int> v = g_career.pyramids[fr].pools[q].clubs;
            std::sort(v.begin(), v.end(), [](int a, int b) { return g_world.teams[a].rating > g_world.teams[b].rating; });
            if (v.size() >= 3) g_article.push_back("Les favoris de la Ligue 1 : " + g_world.teams[v[0]].name + ", " + g_world.teams[v[1]].name + " et " + g_world.teams[v[2]].name + ".");
        }
    }
    if (!g_career.tdcVenue.empty()) g_article.push_back("Le Trophée des Champions se disputera au " + g_career.tdcVenue + ".");
    if (!g_career.uclFinalVenue.empty()) g_article.push_back("L'UEFA a annoncé les lieux des finales : la Ligue des champions se jouera au " + g_career.uclFinalVenue + (g_career.opts.euroFormat ? ", la Ligue Europa au " : ", la Coupe UEFA au ") + g_career.uefaFinalVenue + ".");
    if (g_career.opts.euroFormat) g_article.push_back("Nouvelle formule : phase de ligue à 36 clubs dans les trois coupes d'Europe, 8 matchs (6 en Ligue Conférence) contre des adversaires différents.");
    else g_article.push_back("Coupe Intertoto : trois billets pour le 1er tour de la Coupe UEFA sont en jeu cet été.");
    g_articleNoMatch = true; g_articleBack = SC_HUB;
    g_screen = SC_ARTICLE;
}
