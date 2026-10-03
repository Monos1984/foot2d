// Moteur de match : règles du football, physique du ballon, IA
#include "match.h"
#include <cstring>

static Rng R(0xBA11);
static const float GRAV = 10.8f;
static const float PI = 3.14159265f;

static float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
static V2 rot(V2 v, float a) { float c = std::cos(a), s = std::sin(a); return { v.x * c - v.y * s, v.x * s + v.y * c }; }

static float colorDist(unsigned a, unsigned b) {
    int dr = (int)((a >> 16) & 255) - (int)((b >> 16) & 255);
    int dg = (int)((a >> 8) & 255) - (int)((b >> 8) & 255);
    int db = (int)(a & 255) - (int)(b & 255);
    return std::sqrt((float)(dr * dr * 2 + dg * dg * 4 + db * db * 3));
}

// ------------------------------------------------------------------ repères
V2 Match::goalCenter(int t) const { return attackDir[t] < 0 ? V2(PITCH_W / 2, 0) : V2(PITCH_W / 2, PITCH_L); }
V2 Match::ownGoal(int t) const { return attackDir[t] < 0 ? V2(PITCH_W / 2, PITCH_L) : V2(PITCH_W / 2, 0); }
float Match::progress(int t, V2 p) const { return attackDir[t] < 0 ? (PITCH_L - p.y) / PITCH_L : p.y / PITCH_L; }
V2 Match::fromTeamFrame(int t, float u, float v) const {
    if (attackDir[t] < 0) return { v * PITCH_W, PITCH_L * (1 - u) };
    return { (1 - v) * PITCH_W, PITCH_L * u };
}
// tenues du match : conflit de couleurs -> tenue extérieure ; tenues imposées avant le match
void matchKits(const MatchSetup& S, Kit kit[2]) {
    const Team& H = g_world.teams[S.home]; const Team& A = g_world.teams[S.away];
    kit[0] = H.home;
    kit[1] = A.home;
    if (colorDist(H.home.shirt, A.home.shirt) < 160 || colorDist(H.home.shorts, A.home.shorts) + colorDist(H.home.shirt, A.home.shirt) < 250) kit[1] = A.away;
    if (colorDist(kit[0].shirt, kit[1].shirt) < 120) { kit[1] = A.away; kit[1].shirt = kit[0].shirt == 0xFFFFFF ? 0x111111 : 0xFFFFFF; kit[1].shorts = kit[1].shirt; }
    if (S.kitSel[0] >= 0) kit[0] = teamKit(H, S.kitSel[0]);
    if (S.kitSel[1] >= 0) kit[1] = teamKit(A, S.kitSel[1]);
}

// troisième tenue : une couleur éloignée des deux tenues habituelles
Kit teamKit(const Team& T, int sel) {
    if (sel <= 0) return T.home;
    if (sel == 1) return T.away;
    if (T.hasThird) return T.third;
    static const unsigned C[] = { 0xFFD700, 0x6CABDD, 0x111111, 0xF47920, 0x00843D, 0x7A1438, 0xFFFFFF, 0x6A2C91, 0x2BB5B0 };
    unsigned best = C[0]; float bd = -1;
    for (unsigned c : C) { float d = std::min(colorDist(c, T.home.shirt), colorDist(c, T.away.shirt)); if (d > bd) { bd = d; best = c; } }
    Kit k; k.shirt = best; k.shirt2 = T.home.shirt; k.shorts = best == 0xFFFFFF ? 0x111111 : best; k.socks = best; k.pattern = 0;
    return k;
}

bool Match::inOwnBox(int t, V2 p) const {
    V2 g = ownGoal(t);
    return std::fabs(p.x - PITCH_W / 2) < BOX_W / 2 && std::fabs(p.y - g.y) < BOX_L;
}
bool Match::humanSide(int t) const {
    for (int i = 0; i < NUM_INPUTS; i++) if (S.side[i] == t) return true;
    return false;
}
std::string Match::playerName(int i) const {
    const Team& T = team(pl[i].team);
    if (pl[i].squad < 0 || pl[i].squad >= (int)T.squad.size()) return "?";
    return T.squad[pl[i].squad].name;
}
int Match::teamPlayersCount(int t) const {
    int n = 0; for (int i = t * 11; i < t * 11 + 11; i++) if (pl[i].onPitch) n++;
    return n;
}

// ------------------------------------------------------------------ initialisation
static void loadAttrs(MPlayer& p, const Player& P, float mult, int teamId, RuleProfile rules) {
    if(rules==RULESET_CAREER){mult *= .97f+.06f*P.morale/100.f;float fit=.8f+.2f*positionFamiliarity(P,p.detailedPosition)/100.f;mult*=fit;}       // moral
    if (P.gender) mult *= 0.95f;                    // football féminin : physique adapté (vitesse et puissance de frappe)
    if(rules==RULESET_CAREER&&g_career.personalityEnabled()){auto traits=personalityTraits(g_career,P);p.personalityStress=personalityStressMultiplier(traits);p.personalityFoul=personalityFoulMultiplier(traits);p.personalityEngagement=std::clamp(1.f+(traits.aggression-50)*.001f,.95f,1.05f);}
    p.dribble = std::min(99.f, P.drib() * mult);
    p.heading = std::min(99.f, P.head() * mult);
    p.endur = P.stamina / 100.f;
    p.cond0 = (float)playerCond(teamId, P);
    p.stamina = p.stam0 = 0.5f + 0.5f * p.cond0 / 100.f;   // condition physique au coup d'envoi
    p.speed = 5.0f + (P.speed * mult) / 100.f * 3.1f;     // allure plus « simulation »
    p.shoot = std::min(99.f, P.shoot * mult);
    p.pass = std::min(99.f, P.pass * mult);
    p.tackle = std::min(99.f, P.tackle * mult);
    p.keep = std::min(99.f, P.keep * mult);
    p.posi = std::min(99.f, P.posi() * mult);
    p.compo = std::min(99.f, P.comp() * mult);
}

void Match::setFormation(int t, int f) {
    formation[t] = f;
    const Formation& F = FORMATIONS[f];
    for(int s=0;s<11;s++){auto& p=pl[t*11+s];p.detailedPosition=S.rules==RULESET_CAREER?teamSlotPosition(team(t),f,s):slotPosition(f,s);if(s)p.role=F.role[s-1];
      if(S.rules==RULESET_CAREER){const auto& T=team(t);p.tactic=T.tactical.customized?T.tactical.slot[s]:defaultSlotTactic(p.detailedPosition,T.seed%2,s);p.effects=roleEffects(p.detailedPosition,p.tactic);}else {p.tactic=SlotTactic();p.effects=RoleEffects();}}
    applyMentality(t);
}

const char* mentalityName(int m) {
    static const char* N[] = { "Ultra défensive", "Défensive", "Neutre", "Offensive", "Ultra offensive" };
    return N[std::max(0, std::min(4, m))];
}

// mentalité : fait passer des joueurs d'une ligne à l'autre (nombre de défenseurs / milieux / attaquants)
void Match::applyMentality(int t) {
    const Formation& F = FORMATIONS[formation[t]];
    for (int s = 1; s < 11; s++) pl[t * 11 + s].role = F.role[s - 1];
    if(S.rules==RULESET_CAREER)return;
    int m = mentality[t];
    auto count = [&](int r) { int n = 0; for (int s = 1; s < 11; s++) if (pl[t * 11 + s].role == r) n++; return n; };
    auto pickSlot = [&](int r, bool advanced, bool wide) {
        int best = -1; float bv = -1e9f;
        for (int s = 1; s < 11; s++) {
            if (pl[t * 11 + s].role != r) continue;
            float v = wide ? std::fabs(F.x[s - 1] - 0.5f) : (advanced ? F.y[s - 1] : -F.y[s - 1]);
            if (v > bv) { bv = v; best = s; }
        }
        return best;
    };
    auto setRole = [&](int s, int r) { if (s > 0) pl[t * 11 + s].role = (uint8_t)r; };
    if (m <= 1) {
        if (count(3) > 1) setRole(pickSlot(3, false, false), 2);
        else if (count(2) > 2) setRole(pickSlot(2, false, false), 1);
        if (m == 0 && count(2) > 2) setRole(pickSlot(2, false, false), 1);
    } else if (m >= 3) {
        if (count(2) > 1) setRole(pickSlot(2, true, false), 3);
        if (m == 4 && count(1) > 3) setRole(pickSlot(1, false, true), 2);
    }
}

void Match::setMentality(int t, int m, bool announce) {
    m = std::max(0, std::min(4, m));
    if (m == mentality[t]) return;
    mentality[t] = m;
    applyMentality(t);
    if (announce) {
        static const char* SAY[] = { "%s ferme le jeu et passe en ultra défensive.", "%s recule d'un cran : consigne défensive.", "%s revient à une organisation équilibrée.",
                                     "%s se projette vers l'avant : consigne offensive.", "%s met le paquet, tout le monde à l'attaque !" };
        say(fmt(SAY[m], team(t).name.c_str()), 3.f);
    }
}

// capitaine : celui désigné par l'entraîneur, puis le vice-capitaine, sinon le joueur le plus expérimenté sur le terrain
void Match::updateCaptain(int t) {
    const Team& T = team(t);
    int cap = -1, vice = -1;
    for (int i = t * 11; i < t * 11 + 11; i++) {
        if (!pl[i].onPitch || pl[i].squad < 0 || pl[i].squad >= (int)T.squad.size()) continue;
        int id = T.squad[pl[i].squad].id;
        if (T.captainPid && id == T.captainPid) cap = i;
        if (T.vicePid && id == T.vicePid) vice = i;
    }
    if (cap < 0) cap = vice;
    if (cap < 0) {
        float bo = -1;
        for (int i = t * 11 + 1; i < t * 11 + 11; i++) {
            if (!pl[i].onPitch || pl[i].squad < 0 || pl[i].squad >= (int)T.squad.size()) continue;
            float o = pl[i].shoot + pl[i].pass + pl[i].tackle + T.squad[pl[i].squad].age * 3.f;
            if (o > bo) { bo = o; cap = i; }
        }
    }
    captain[t] = cap >= 0 ? cap : t * 11 + 1;
}

// l'ordinateur adapte sa mentalité au score et au temps restant
void Match::aiTactics() {
    for (int t = 0; t < 2; t++) {
        if (coachSide(t)) continue;
        int diff = score[t] - score[1 - t];
        if (S.hasFirstLeg) diff = (score[t] + (t == 0 ? S.aggHome : S.aggAway)) - (score[1 - t] + (t == 0 ? S.aggAway : S.aggHome));
        int base = team(t).mentality;
        int m = base;
        if (diff < 0) m = clock > 80 || diff <= -2 ? 4 : clock > 60 ? 3 : base;
        else if (diff > 0) m = clock > 85 && diff == 1 ? 0 : clock > 70 ? std::min(base, 1) : base;
        int on = 0; for (int i = t * 11; i < t * 11 + 11; i++) if (pl[i].onPitch) on++;
        if (on <= 9 && diff >= 0) m = std::min(m, 1);          // à neuf : on protège le résultat
        setMentality(t, m, true);
    }
}

void Match::init(const MatchSetup& setup) {
    S = setup;
    for(int t=0;t<2;t++)pst[t].clear();for(int t=0;t<2;t++){shots[t]=onTarget[t]=passes[t]=completedPasses[t]=saves[t]=blockedShots[t]=woodwork[t]=throws[t]=freeKicks[t]=goalKicks[t]=penalties[t]=corners[t]=fouls[t]=offsides[t]=0;possTime[t]=0;}pendingPass=pendingShot=-1;kickingPass=shotHitWoodwork=false;
    positionMinutes.clear();
    R = Rng(g_rng.next());
    off = OffsidePhase(); offSeq = 0; offPend = -1; offFlagT = 0; offFlagAR = -1; offDecisions = offClose = offMistakes = 0;
    makeAssistants();
    int ids[2] = { S.home, S.away };
    for (int t = 0; t < 2; t++) {
        g_world.ensureSquad(ids[t]);
        const Team& T = g_world.teams[ids[t]];
        formation[t] = S.formation[t] >= 0 ? S.formation[t] : T.formation;
        auto lu = g_world.pickLineup(ids[t], formation[t],S.rules);
        onField[t].assign(lu.begin(), lu.begin() + 11);
        bench[t].assign(lu.begin() + 11, lu.end());
        bool human = coachSide(t);
        float mult = human ? 1.0f : (S.difficulty == 0 ? 0.86f : S.difficulty == 2 ? 1.1f : 1.0f);
        for (int s = 0; s < 11; s++) {
            MPlayer& p = pl[t * 11 + s];
            p = MPlayer();
            p.team = t; p.slot = s; p.gk = (s == 0);
            p.squad = onField[t][s];p.detailedPosition=S.rules==RULESET_CAREER?teamSlotPosition(T,formation[t],s):slotPosition(formation[t],s);
            loadAttrs(p, T.squad[p.squad], mult * supporterPlayerMultiplier(S,t,T.squad[p.squad].age)*(S.personalityActive?personalityMatchMultiplier(g_career,T.squad[p.squad],t==0?S.home:S.away,S.personalityKey,S.personalityImportant,S.supporterPressure):1.f) * (t == 0 ? 1.f + S.homeBoost : 1.f) * S.bribeMult[t] * (p.squad == S.bribeSquad[t] ? 0.6f : 1.f), ids[t],S.rules);
        }
        mentality[t] = std::max(0, std::min(4, T.mentality));
        S.tac[t][0] = T.pressing; S.tac[t][1] = T.defLine; S.tac[t][2] = T.width; S.tac[t][3] = T.tempo; S.tac[t][4] = T.passStyle;
        for (auto& v : S.tac[t]) v = (uint8_t)std::min(2, (int)v);
        if (!coachSide(t) && S.rules==RULESET_CAREER) {   // ordinateur : consignes selon le niveau et le style du club
            uint32_t h = T.seed * 2654435761u;
            S.tac[t][0] = (uint8_t)(T.rating >= 72 ? 2 : T.rating < 40 && (h & 1) ? 0 : 1);
            S.tac[t][3] = (uint8_t)((h >> 3) % 3);
            S.tac[t][4] = (uint8_t)(T.rating < 45 ? 2 : (h >> 5) % 3);
            S.tac[t][2] = (uint8_t)((h >> 7) % 3);
        }
        if(S.rules==RULESET_CAREER&&!coachSide(t)){bool possession=T.seed%2==0;S.tac[t][0]=possession?2:0;S.tac[t][1]=possession?2:0;S.tac[t][4]=possession?0:2;S.tac[t][3]=possession?0:2;}
        if(S.rules==RULESET_SIMPLE){S.tac[t][1]=1;S.tac[t][3]=1;}
        lateSubDone[t] = false;
        setFormation(t, formation[t]);
        subsLeft[t] = S.maxSubs >= 0 ? S.maxSubs : 5;
        if (S.benchSize >= 0 && (int)bench[t].size() > S.benchSize) bench[t].resize(S.benchSize);
        if (S.rolling) { if (S.benchSize < 0 && bench[t].size() > 3) bench[t].resize(3); subsLeft[t] = 99; }
    }
    matchKits(S, kit);
    static const unsigned GKC[] = { 0x2E9E4F, 0xF2D21B, 0x222222, 0x8A8A8A, 0xE85DA8, 0x19C3C8, 0xF47920 };
    for (int t = 0; t < 2; t++) {
        float best = -1;
        for (unsigned c : GKC) {
            float d = std::min(colorDist(c, kit[0].shirt), colorDist(c, kit[1].shirt));
            if (t == 1) d = std::min(d, colorDist(c, gkShirt[0]));
            if (d > best) { best = d; gkShirt[t] = c; }
        }
    }
    switch (S.pitch) {
    case 1: pitchFriction = 0.8f; pitchBounce = 0.62f; break;   // sec
    case 2: pitchFriction = 0.62f; pitchBounce = 0.35f; break;  // humide
    case 3: pitchFriction = 1.9f; pitchBounce = 0.25f; break;   // boueux
    case 4: pitchFriction = 0.45f; pitchBounce = 0.7f; break;   // gelé
    default: pitchFriction = 1.0f; pitchBounce = 0.5f; break;
    }
    if (S.meteo == 5 && S.pitch == 4) { pitchFriction = 1.35f; pitchBounce = 0.3f; }   // terrain enneigé : le ballon freine
    if (S.meteo == 4) { float a = R.frange(0.f, 6.2831f); wind = V2(std::cos(a), std::sin(a)) * R.frange(1.2f, 2.6f); }
    turfBad = clampf((75.f - S.turf) / 75.f, 0.f, 1.f);
    pitchFriction *= 1.f + turfBad * 0.55f;
    rp.assign(REPLAY_N * 22, Snap());
    rb.assign(REPLAY_N, BallSnap());
    rpHead = rpCount = 0;
    score[0] = score[1] = 0;
    events.clear();
    finished = false;
    kickoffTeam = firstKickoff = R.range(0, 1);
    cam = V2(S.training ? PITCH_W / 2 : 4.f, PITCH_L / 2);
    refPos = V2(PITCH_W / 2 - 6, PITCH_L / 2 + 4);
    // cérémonie : les deux équipes alignées au milieu du terrain, poignées de main
    ceremony = S.training == 0; state = MS_INTRO; stateT = 0;
    cerPhase = S.cupPhoto ? 0 : 2; cerT = 0; htWaiting = htGo = false; sideFlip = false; tossCall = tossResult = tossWinner = tossChoice = tossSideTeam = -1; tossAsked = false;
    for (int t = 0; t < 2; t++) {
        // capitaine : le joueur de champ le mieux noté
        updateCaptain(t);
    }
    for (int t = 0; t < 2; t++) for (int s2 = 0; s2 < 11; s2++) {
        MPlayer& p = pl[t * 11 + s2];
        if (cerPhase == 0) p.pos = V2(PITCH_W / 2 + (t ? 1.f : -1.f) * (6.f + s2 * 1.1f), PITCH_L / 2 + 14.f + s2 * 0.3f);   // sortie du tunnel
        else p.pos = V2(PITCH_W / 2 - 13 + s2 * 2.4f + (t ? 30.f : 0), PITCH_L / 2 + (t ? 1.4f : -1.4f));
        p.face = V2(0, t ? -1.f : 1.f); p.vel = V2(); p.state = PS_NORMAL;
    }
    ball.pos = V2(PITCH_W / 2, PITCH_L / 2); ball.vel = V2(); ball.owner = -1;
    refPos = V2(PITCH_W / 2 - 16, PITCH_L / 2);
    if (ceremony) {   // les équipes sortent du tunnel, l'arbitre en tête
        cerPhase = -1;
        for (int t = 0; t < 2; t++) for (int s2 = 0; s2 < 11; s2++) { MPlayer& p = pl[t * 11 + s2]; p.pos = tunnelSpot(t, s2 + 1); p.face = V2(1, 0); }
        refPos = V2(-3.f, PITCH_L / 2);
        ballStage = 1; pedestal = V2(1.4f, PITCH_L / 2 + 2.4f);
        ball.pos = pedestal; ball.z = 0.95f;
    }
    msg = team(0).name + " - " + team(1).name; msg2 = S.stadium.empty() ? S.title : S.stadium; msgT = 3.0f;
    {
        int hh = atoi(S.kickoffTime.c_str());
        bool neutralSt = S.stadium.empty() || S.stadium == "Terrain neutre";
        if (!S.training) say(fmt("%s à tous et bienvenue %s pour %s - %s !", hh >= 18 || S.kickoffTime.empty() ? "Bonsoir" : "Bonjour", neutralSt ? "sur terrain neutre" : ("au " + S.stadium).c_str(), team(0).name.c_str(), team(1).name.c_str()), 4.5f, true);
    }
    if (S.training) {
        S.commentary = false;
        attackDir[0] = -1; attackDir[1] = 1;
        for (int t = 0; t < 2; t++) for (int s2 = 0; s2 < 11; s2++) { V2 tg; formationTarget(t * 11 + s2, tg); pl[t * 11 + s2].pos = tg; }
        trainTries = 0; trainGoals = 0;
        trainingNext();
    }
    if (S.pensOnly && !S.training) {
        // séance de tirs au but seule : pas de cérémonie ni de jeu, tirage au sort puis tirs
        S.decisive = true; ceremony = false; cerPhase = 99; ballStage = 0; ball.z = 0;
        for (int t = 0; t < 2; t++) for (int s2 = 0; s2 < 11; s2++) { V2 tg; formationTarget(t * 11 + s2, tg); pl[t * 11 + s2].pos = tg; pl[t * 11 + s2].vel = V2(); }
        period = 1; state = MS_BREAK; stateT = 0; nextSp = 4;
        msg = "SÉANCE DE TIRS AU BUT"; msg2 = team(0).shortName + " - " + team(1).shortName; msgT = 3;
        say("Place à une séance de tirs au but entre " + team(0).name + " et " + team(1).name + " !", 3.5f, true);
    }
}

// ------------------------------------------------------------------ entraînement
void Match::trainingNext() {
    trainTries++;
    trainT = 0;
    score[0] = trainGoals; score[1] = std::max(0, trainTries - 1 - trainGoals);
    ball = Ball();
    for (auto& p : pl) { if (p.state != PS_OFF) p.state = PS_NORMAL; p.stamina = std::max(p.stamina, 0.9f); p.cool = 0; p.z = 0; }
    V2 g = goalCenter(0);
    float sg = g.y == 0 ? 1.f : -1.f;
    switch (S.training) {
    case 1: beginSetPiece(SP_PENALTY, 0, V2(PITCH_W / 2, g.y + sg * 11.f)); msg = "PENALTY"; break;
    case 5: { V2 og = ownGoal(0); beginSetPiece(SP_PENALTY, 1, V2(PITCH_W / 2, og.y + (og.y == 0 ? 11.f : -11.f))); msg = "ARRÊTEZ LE PENALTY"; break; }
    case 2: {
        float dist = R.frange(18.f, 30.f), ang = R.frange(-0.7f, 0.7f);
        V2 spot(PITCH_W / 2 + std::sin(ang) * dist, g.y + sg * std::cos(ang) * dist);
        beginSetPiece(SP_FREEKICK, 0, spot); msg = "COUP FRANC"; break;
    }
    case 3: beginSetPiece(SP_CORNER, 0, V2(R.chance(0.5f) ? 0.3f : PITCH_W - 0.3f, g.y == 0 ? 0.3f : PITCH_L - 0.3f)); msg = "CORNER"; break;
    case 6: case 7: {
        // face-à-face (6) : l'attaquant part seul de 25 m contre le gardien ; frappes de loin (7) : seul face au but, de 22 à 30 m
        int shooter = 9;
        for (int i = 0; i < 22; i++) if (i != shooter && i != 11) { pl[i].onPitch = false; pl[i].pos = V2(-20, -20); }
        float dist = S.training == 6 ? R.frange(24.f, 28.f) : R.frange(22.f, 30.f), side = R.frange(-12.f, 12.f);
        pl[shooter].onPitch = true; pl[shooter].state = PS_NORMAL; pl[shooter].pos = V2(PITCH_W / 2 + side, g.y + sg * dist); pl[shooter].vel = V2();
        pl[11].onPitch = true; pl[11].pos = V2(PITCH_W / 2, g.y + sg * 1.f); pl[11].vel = V2(); pl[11].state = PS_NORMAL;
        ball.pos = pl[shooter].pos; state = MS_PLAY; stateT = 0; takePossession(shooter);
        msg = S.training == 6 ? "FACE-À-FACE" : "FRAPPE DE LOIN"; break;
    }
    default: beginSetPiece(SP_INDIRECT, 0, V2(PITCH_W / 2 + R.frange(-12, 12), g.y + sg * R.frange(36.f, 48.f))); msg = "ATTAQUE"; break;
    }
    msg2 = fmt("Essai %d", trainTries); msgT = 1.2f;
}

// renvoie vrai si la mise à jour normale doit être court-circuitée
bool Match::trainingUpdate(float dt) {
    if (state == MS_PLAY) {
        trainT += dt;
        int att = S.training == 5 ? 1 : 0;
        bool over = false;
        float limit = S.training == 1 || S.training == 5 ? 3.5f : S.training == 4 ? 25.f : S.training == 6 ? 9.f : 8.f;
        if (S.training == 7 && ball.owner == 9 && progress(0, pl[9].pos) > 0.83f) over = true;      // frappes de loin : tirer avant la surface
        if (trainT > limit) over = true;
        if (ball.owner >= 0 && pl[ball.owner].team != att && trainT > 0.3f) over = true;   // défense ou gardien récupère
        if ((S.training == 1 || S.training == 5) && ball.owner < 0 && ball.vel.len() < 0.6f && trainT > 1.0f) over = true;
        if (over) {
            msg = S.training == 5 ? "ARRÊT !" : "RATÉ"; msg2 = ""; msgT = 1.0f;
            if (S.training == 5) { trainGoals++; }
            state = MS_STOP; stateT = 0.8f; nextSp = -2;
            playSfx(SFX_WHISTLE);
        }
    }
    if (state == MS_STOP && stateT > 1.6f) { trainingNext(); return true; }
    if (state == MS_GOAL && stateT > 1.8f) {
        for (auto& p : pl) if (p.state == PS_CELEB) p.state = PS_NORMAL;
        trainingNext(); return true;
    }
    if (state == MS_BREAK || state == MS_REPLAY) { trainingNext(); return true; }
    return false;
}

// avant-match : photo officielle (coupes), poignées de main, pile ou face avec le choix du coup d'envoi ou du côté
static V2 lineSpot(int t, int s2) { return V2(PITCH_W / 2 - 13 + s2 * 2.4f, PITCH_L / 2 + (t ? 2.6f : -2.6f)); }
// hymnes : les deux équipes côte à côte sur une seule ligne, face aux caméras (tribune principale), l'arbitre au centre
V2 anthemSpot(int t, int s2) { return V2(PITCH_W / 2 + (t ? 1.8f + s2 * 2.2f : -1.8f - (10 - s2) * 2.2f), PITCH_L / 2 - 1.5f); }
void Match::updateCeremony(float dt) {
    cerT += dt;
    if (msgT > 0) msgT -= dt;
    {   // caméra : suit la sortie du tunnel puis se recentre
        V2 f(PITCH_W / 2, PITCH_L / 2);
        if (cerPhase < 0) { float sx = 0; int n = 0; for (auto& p : pl) if (p.pos.x > -1.f) { sx += p.pos.x; n++; } f.x = std::max(4.f, n ? sx / n : 0.f); }
        bool anth = cerPhase == 10 || cerPhase == 11 || cerPhase == 12;
        if (cerPhase == 12) { f = V2(vipOn ? vipPos.x : PITCH_W / 2, anthemSpot(0, 0).y + 1.f); }
        else if (anth) {   // hymnes : travelling le long de la ligne des joueurs, caméra rapprochée
            int tt = cerPhase - 10;
            float u = std::min(1.f, std::max(0.f, (cerT - 0.8f) / 8.5f));
            float xa = anthemSpot(tt, 0).x, xb = anthemSpot(tt, 10).x;
            f = V2(xa + u * (xb - xa), anthemSpot(tt, 0).y + 1.f);
        }
        camZoom = camZoom + ((anth ? 2.0f : 1.f) - camZoom) * std::min(1.f, dt * 1.8f);
        cam = cam + (f - cam) * std::min(1.f, dt * (anth ? 3.f : 1.5f));
    }
    float t = cerT;
    if (ballStage == 2) { ball.pos = refPos + V2(0.4f, 0.15f); ball.z = 0.5f; ball.vel = V2(); ball.owner = -1; }
    else if (ballStage == 1) { ball.pos = pedestal; ball.z = 0.95f; ball.vel = V2(); ball.owner = -1; }
    auto walkTo = [&](MPlayer& p, V2 tg, float spd) {
        V2 d = tg - p.pos;
        p.vel = d.len() > 0.08f ? d.norm() * std::min(spd, d.len() * 4) : V2();
        p.pos += p.vel * dt; p.anim += p.vel.len() * dt;
        if (d.len() > 0.3f) p.face = d.norm();
        return d.len() < 0.3f;
    };
    bool skip = false, pauseSkip = false;
    for (int i = 0; i < NUM_INPUTS; i++) { if (ctl[i].f1p || ctl[i].f2p) skip = true; if (ctl[i].pause) pauseSkip = true; }
    auto toToss = [&]() {
        if (cerPhase == 10 || cerPhase == 11) anthemReq = -2;
        vipOn = false;
        for (int k = 0; k < 22; k++) { int tt = k / 11, s2 = k % 11; pl[k].pos = lineSpot(tt, s2); pl[k].state = PS_NORMAL; pl[k].face = V2(0, tt ? -1.f : 1.f); }
        cerPhase = 3; cerT = 0;
    };
    if (pauseSkip && cerPhase < 5) {           // tout passer : tirage au sort automatique
        tossWinner = R.range(0, 1); tossChoice = R.range(0, 1);
        if (tossChoice == 1) { sideFlip = R.chance(0.5f); firstKickoff = 1 - tossWinner; } else firstKickoff = tossWinner;
        cerPhase = 5; cerT = 99; tossUI = 0;
    }
    auto photoSpot = [&](int k) {
        int tt = k / 11, s2 = k % 11; bool back = s2 < 6;
        float cx = PITCH_W / 2 + (tt ? 14.f : -14.f);
        return back ? V2(cx + (s2 - 2.5f) * 2.1f, PITCH_L / 2 - 1.9f) : V2(cx + (s2 - 6 - 2.f) * 2.1f + 1.f, PITCH_L / 2 + 0.3f);
    };
    switch (cerPhase) {
    case -1: {  // sortie du tunnel : deux files côte à côte
        bool allIn = true;
        for (int k = 0; k < 22; k++) {
            int tt = k / 11, s2 = k % 11;
            if (t < 2.6f + s2 * 0.45f) { allIn = false; continue; }
            V2 tg = S.anthems ? anthemSpot(tt, s2) : S.cupPhoto ? photoSpot(k) : lineSpot(tt, s2) + V2(tt ? 30.f : 0.f, 0);
            V2 mid(1.5f + s2 * 0.2f, PITCH_L / 2 + (tt ? 1.f : -1.f));
            MPlayer& p = pl[k];
            bool out = p.pos.x > 0.8f;
            if (!walkTo(p, out ? tg : mid, out ? 4.f : 3.2f)) allIn = false;
            p.state = PS_NORMAL;
        }
        // l'arbitre sort le premier et prend le ballon du match sur son présentoir
        if (ballStage == 1) {
            V2 tg = pedestal - V2(0.6f, 0.2f), d = tg - refPos;
            if (d.len() > 0.15f) refMove(tg, 3.2f, refFace, dt);
            else { ballStage = 2; say("L'arbitre récupère le ballon du match sur son présentoir.", 2.5f, true); }
        } else refMove(V2(PITCH_W / 2 - 16, PITCH_L / 2), 3.4f, V2(1, 0), dt);
        if (t > 2.5f && t < 2.6f) { msg = "ENTRÉE DES JOUEURS"; msg2 = team(0).name + " - " + team(1).name; msgT = 3.f; }
        if (allIn || t > 16.f) { cerPhase = S.anthems ? 10 : S.cupPhoto ? 0 : 1; cerT = 0; }
        if (skip && t > 0.3f) toToss();
        break;
    }
    case 10: case 11: {   // hymnes nationaux : chaque équipe en ligne, travelling sur les joueurs
        int tt = cerPhase - 10;
        for (int k = 0; k < 22; k++) { int t2 = k / 11; walkTo(pl[k], anthemSpot(t2, k % 11), 3.f); pl[k].face = V2(0, 1.f); pl[k].state = PS_NORMAL; }
        refMove(V2(PITCH_W / 2, PITCH_L / 2 - 1.5f), 3.4f, V2(0, 1), dt);
        if (t < dt * 1.5f) {
            anthemReq = tt; anthemDur = 10.5f; msg = "HYMNE NATIONAL"; msgT = 3.f;
            if (!S.anthemOnly.empty()) { msg2 = "La Marseillaise"; say("Le stade se lève : les deux équipes et le public entonnent La Marseillaise.", 4.5f, true); }
            else { msg2 = team(tt).name; say("Les joueurs de " + team(tt).name + " entonnent leur hymne national.", 4.f, true); }
        }
        if (t > 1.f && t < 1.f + dt * 1.5f && !anthemName.empty()) { msg2 = (S.anthemOnly.empty() ? team(tt).name + " - " : std::string()) + anthemName; msgT = 3.f; }
        if (t > anthemDur) {
            cerT = 0;
            if (tt == 0 && S.anthemOnly.empty()) cerPhase = 11;
            else { anthemReq = -2; cerPhase = S.president ? 12 : S.cupPhoto ? 0 : 1; }
        }
        if (skip && t > 0.3f) toToss();
        break;
    }
    case 12: {  // le président de la République sort du tunnel et serre la main de chaque joueur
        for (int k = 0; k < 22; k++) { int t2 = k / 11; walkTo(pl[k], anthemSpot(t2, k % 11), 3.f); pl[k].face = V2(0, 1.f); pl[k].state = PS_NORMAL; }
        refMove(V2(PITCH_W / 2, PITCH_L / 2 - 1.5f), 3.4f, V2(0, 1), dt);
        if (t < dt * 1.5f) {
            vipOn = true; vipPos = V2(-2.5f, PITCH_L / 2);
            msg = "LE PRÉSIDENT DE LA RÉPUBLIQUE"; msg2 = "salue les joueurs"; msgT = 3.f;
            say("Le président de la République descend sur la pelouse pour saluer les deux équipes, comme le veut la tradition de la finale.", 5.f, true);
        }
        float xa = anthemSpot(0, 0).x - 1.2f, xb = anthemSpot(1, 10).x + 1.2f, yl = anthemSpot(0, 0).y + 1.1f;
        float walkIn = 3.2f, per = 0.75f;
        V2 tg;
        if (t < walkIn) tg = V2(xa, yl);
        else if (t < walkIn + 22 * per) tg = V2(xa + (xb - xa) * (t - walkIn) / (22 * per), yl);
        else tg = V2(-2.5f, PITCH_L / 2);
        V2 d = tg - vipPos; float l = d.len();
        if (l > 0.05f) vipPos += d * (std::min(l, (t < walkIn ? 4.5f : t < walkIn + 22 * per ? 6.f : 4.f) * dt) / l);
        for (int k = 0; k < 22; k++) if (std::fabs(pl[k].pos.x - vipPos.x) < 0.7f && t >= walkIn && t < walkIn + 22 * per) pl[k].state = PS_HAND;
        if (t > walkIn + 22 * per + 2.5f || (skip && t > 0.3f)) { vipOn = false; cerPhase = S.cupPhoto ? 0 : 1; cerT = 0; }
        break;
    }
    case 0: {   // photo officielle : deux rangées par équipe, flashs des photographes
        bool allIn = true;
        for (int k = 0; k < 22; k++) {
            int tt = k / 11, s2 = k % 11;
            bool back = s2 < 6;
            float cx = PITCH_W / 2 + (tt ? 14.f : -14.f);
            V2 tg = back ? V2(cx + (s2 - 2.5f) * 2.1f, PITCH_L / 2 - 1.9f) : V2(cx + (s2 - 6 - 2.f) * 2.1f + 1.f, PITCH_L / 2 + 0.3f);
            if (!walkTo(pl[k], tg, 4.5f)) allIn = false;
            if (allIn || t > 3.f) pl[k].face = V2(0, 1);
            pl[k].state = PS_NORMAL;
        }
        refMove(V2(PITCH_W / 2, PITCH_L / 2 - 3.f), 3.6f, V2(0, 1), dt);
        if (t > 1.2f && t < 1.3f) { msg = "PHOTO OFFICIELLE"; msg2 = team(0).name + " - " + team(1).name; msgT = 3.f; say("Les deux équipes posent pour la photo officielle avant cette rencontre de coupe.", 3.5f, true); }
        if (t > 5.5f) { cerPhase = 1; cerT = 0; }
        if (skip && t > 0.3f) toToss();
        break;
    }
    case 1: {   // retour en ligne pour les poignées de main
        bool ok = true;
        for (int k = 0; k < 22; k++) {
            int tt = k / 11, s2 = k % 11;
            V2 tg = lineSpot(tt, s2); if (tt == 1) tg.x += 30.f;
            if (!walkTo(pl[k], tg, 5.f)) ok = false;
        }
        if (ok || t > 4.f) { cerPhase = 2; cerT = 0; }
        if (skip && t > 0.3f) toToss();
        break;
    }
    case 2: {   // l'équipe visiteuse longe la ligne des locaux et serre les mains
        // les visiteurs défilent à pas réguliers devant la ligne des locaux et serrent chaque main
        for (int s2 = 0; s2 < 11; s2++) { MPlayer& h = pl[s2]; walkTo(h, lineSpot(0, s2) + V2(0, 0.5f), 3.f); h.face = V2(0, 1); h.state = PS_NORMAL; }
        for (int s2 = 0; s2 < 11; s2++) {
            MPlayer& a = pl[11 + s2];
            float tx = PITCH_W / 2 + 13.f + s2 * 2.4f - std::max(0.f, t - 0.3f) * 5.0f;
            V2 tg(tx, PITCH_L / 2 + 0.9f);
            V2 d = tg - a.pos;
            a.vel = d.len() > 0.05f ? d.norm() * std::min(9.f, d.len() * 8.f) : V2();
            a.pos += a.vel * dt; a.anim += a.vel.len() * dt;
            a.face = V2(-1, 0); a.state = PS_NORMAL;
        }
        for (int s2 = 0; s2 < 11; s2++) for (int k = 0; k < 11; k++) {
            MPlayer& h = pl[k]; MPlayer& a = pl[11 + s2];
            if (std::fabs(h.pos.x - a.pos.x) < 0.8f) { h.state = PS_HAND; a.state = PS_HAND; a.face = V2(0, -1); }
        }
        refMove(V2(PITCH_W / 2 - 16, PITCH_L / 2), 4.f, V2(1, 0), dt);
        if (t > 1.2f && t < 1.3f) { msg = "LES JOUEURS SE SERRENT LA MAIN"; msg2 = ""; msgT = 2.5f; }
        if (t > 10.5f || (skip && t > 0.3f)) toToss();
        break;
    }
    case 3: {   // pile ou face : les capitaines rejoignent l'arbitre au rond central ; le capitaine visiteur annonce
        for (auto& p : pl) p.state = PS_NORMAL;
        V2 c(PITCH_W / 2, PITCH_L / 2);
        for (int k = 0; k < 22; k++) if (k != captain[0] && k != captain[1]) {   // les autres s'écartent dans leur moitié
            int tt = k / 11; V2 tg = lineSpot(tt, k % 11); tg.y += tt ? 7.f : -7.f; walkTo(pl[k], tg, 3.f); pl[k].face = V2(0, tt ? -1.f : 1.f);
        }
        walkTo(pl[captain[0]], c + V2(-1.4f, 0.4f), 4.f); pl[captain[0]].face = V2(1, 0);
        walkTo(pl[captain[1]], c + V2(1.4f, 0.4f), 4.f); pl[captain[1]].face = V2(-1, 0);
        refMove(c + V2(0, -0.6f), 3.6f, V2(0, 1), dt);
        if (t > 1.5f && tossCall < 0) {
            if (humanSide(1)) {
                tossUI = 1;
                if (!tossAsked) { tossAsked = true; tossSel = 0; say("Choisissez pile ou face avec les flèches, puis validez.", 6.f, true); }
                if (tossNav(1, tossSel, 2, dt)) tossCall = tossSel;
                if (t > 20.f && tossCall < 0) tossCall = tossSel;
            } else tossCall = R.range(0, 1);
            if (tossCall >= 0) { say(playerName(captain[1]) + (tossCall == 0 ? " annonce PILE." : " annonce FACE."), 2.f, true); cerT = 0; cerPhase = 4; tossResult = R.range(0, 1); tossUI = 2; playSfx(SFX_WHISTLE); }
        }
        break;
    }
    case 4: {   // la pièce tourne en l'air puis retombe sur pile ou face
        const float FLIP = 2.4f;
        if (t > FLIP && tossWinner < 0) {
            tossWinner = tossResult == tossCall ? 1 : 0;
            say(std::string(tossResult == 0 ? "PILE ! " : "FACE ! ") + team(tossWinner).name + " remporte le tirage au sort.", 2.5f, true);
        }
        if (tossWinner >= 0 && t > FLIP + 1.3f) {
            if (tossChoice < 0) {
                if (humanSide(tossWinner)) {
                    if (tossUI != 3) { tossUI = 3; tossSel = 0; }
                    if (tossNav(tossWinner, tossSel, 2, dt)) tossChoice = tossSel;
                    if (t > 30.f && tossChoice < 0) tossChoice = 0;
                } else tossChoice = R.chance(0.5f) ? 0 : 1;
                if (tossChoice >= 0) {
                    // ballon : le perdant choisit son côté ; côté : le perdant donne le coup d'envoi
                    tossSideTeam = tossChoice == 0 ? 1 - tossWinner : tossWinner;
                    firstKickoff = tossChoice == 0 ? tossWinner : 1 - tossWinner;
                    if (tossChoice == 0) say(team(tossWinner).shortName + " prend le ballon : " + team(1 - tossWinner).shortName + " choisit son côté.", 2.8f, true);
                    tossDir = -1; tossSel = 0; sideT = t;
                    tossUI = humanSide(tossSideTeam) ? 4 : 6;
                }
            } else {
                // choix du côté (vainqueur s'il a choisi le côté, sinon l'adversaire)
                if (tossDir < 0) {
                    if (humanSide(tossSideTeam)) { if (tossNav(tossSideTeam, tossSel, 2, dt)) tossDir = tossSel; if (t - sideT > 25.f && tossDir < 0) tossDir = 0; }
                    else if (t - sideT > 1.2f) tossDir = R.range(0, 1);
                }
                if (tossDir >= 0) {
                    bool up = tossDir == 0;          // sans inversion, l'équipe 0 attaque vers le haut en 1re période
                    sideFlip = (tossSideTeam == 0) ? !up : up;
                    tossUI = 5;
                    say(team(tossSideTeam).shortName + " choisit son côté ; " + team(firstKickoff).shortName + " donnera le coup d'envoi.", 2.8f, true);
                    cerPhase = 5; cerT = 0;
                }
            }
        }
        break;
    }
    default: break;
    }
    if (cerPhase == 5 && cerT > 1.8f) {
        ceremony = false; tossUI = 0; ballStage = 0; ball.z = 0;
        for (auto& p : pl) p.state = PS_NORMAL;
        startPeriod(0);
        const char* refName = S.referee >= 0 && S.referee < NUM_REFEREES ? REFEREES[S.referee].name : "";
        say(std::string("Coup d'envoi ! L'arbitre de la rencontre : ") + refName + ", assisté de " + assistants[0].name + " et " + assistants[1].name + ".", 4.0f, true);
    }
}

// curseur de sélection (gauche/droite ou haut/bas) pour l'équipe humaine ; vrai quand le choix est validé
bool Match::tossNav(int team, int& sel, int n, float dt) {
    if (tossStickT > 0) tossStickT -= dt;
    bool ok = false;
    for (int i = 0; i < NUM_INPUTS; i++) {
        if (S.side[i] != team) continue;
        const Controls& c = ctl[i];
        float ax = std::fabs(c.dir.x) > std::fabs(c.dir.y) ? c.dir.x : c.dir.y;
        if (std::fabs(ax) > 0.5f && tossStickT <= 0) { sel = (sel + (ax > 0 ? 1 : n - 1)) % n; tossStickT = 0.25f; playSfx(SFX_BOUNCE); }
        if (std::fabs(ax) < 0.2f && tossStickT > 0.12f) tossStickT = 0.12f;
        if (c.f1p || c.f2p || c.f3p) ok = true;
    }
    if (ok) playSfx(SFX_KICK);
    return ok;
}

void Match::say(const std::string& s, float dur, bool force) {
    if (!force && comCool > 0) return;
    comLine = s; comT = dur; comCool = 1.2f;
}

// ------------------------------------------------------------------ arbitre
void Match::updateReferee(float dt) {
    if (refCardT > 0) {
        float before = refCardT;
        refCardT -= dt;
        // 2e avertissement : après le jaune, l'arbitre sort le rouge
        if (refCardSecond && before > 1.7f && refCardT <= 1.7f && refCardFor >= 0) {
            cardShow = 2; playSfx(SFX_CARD);
            int nOff = 0; for (int j = pl[refCardFor].team * 11; j < pl[refCardFor].team * 11 + 11; j++) if (pl[j].sentOff) nOff++;
            msg = nOff >= 2 ? fmt("%dE ROUGE POUR %s !", nOff, team(pl[refCardFor].team).shortName.c_str()) : std::string("CARTON ROUGE !");
            msg2 = playerName(refCardFor); msgT = 2.2f;
        }
        if (refCardT <= 0) refCardSecond = false;
    }
    if (comT > 0) comT -= dt;
    if (comCool > 0) comCool -= dt;
    for (int i = 0; i < NUM_INPUTS; i++) { if (lockSwitch[i] > 0) lockSwitch[i] -= dt; if (passRecvT[i] > 0) { passRecvT[i] -= dt; if (passRecvT[i] <= 0) passRecv[i] = -1; } }
    if (ceremony) return;
    V2 target;
    if (refCardT > 0 && refCardFor >= 0) target = pl[refCardFor].pos + V2(1.2f, 0.8f);
    else if (pendCardOff >= 0 && state != MS_PLAY && pl[pendCardOff].onPitch) target = pl[pendCardOff].pos + V2(1.2f, 0.8f);
    else if (fightT > 0 && fightA >= 0) target = pl[fightA].pos + V2(1.5f, 1.5f);
    else if (state == MS_STOP && (nextSp == SP_FREEKICK || nextSp == SP_INDIRECT) && stateT < 3.f) {
        // coup franc : l'arbitre se rend sur place et recule de 9,15 m pour placer le mur
        V2 g = goalCenter(nextSpTeam);
        target = nextSpPos + (g - nextSpPos).norm() * 6.5f + V2(nextSpPos.x < PITCH_W / 2 ? 2.f : -2.f, 0);
    } else if ((state == MS_STOP && nextSp == SP_CORNER) || (state == MS_SETPIECE && sp == SP_CORNER)) {
        // corner : l'arbitre ne colle pas le tireur, il se poste à l'angle opposé de la surface pour voir les duels
        V2 c = state == MS_SETPIECE ? spPos : nextSpPos;
        float gy = c.y < PITCH_L / 2 ? 0.f : PITCH_L, in = c.y < PITCH_L / 2 ? 1.f : -1.f;
        float far = c.x < PITCH_W / 2 ? PITCH_W / 2 + 13.f : PITCH_W / 2 - 13.f;
        target = V2(far, gy + in * 15.f);
    } else if (state == MS_STOP && nextSp == SP_PENALTY) {
        target = nextSpPos + V2(nextSpPos.x < PITCH_W / 2 ? 3.f : -3.f, nextSpPos.y < PITCH_L / 2 ? 4.f : -4.f);   // désigne le point puis se place à l'entrée de la surface
    } else {
        // se place en diagonale, à distance du ballon, côté opposé ; anticipe la trajectoire du ballon et du jeu
        V2 b = ball.pos + (ball.owner < 0 ? ball.vel * 0.5f : pl[ball.owner].vel * 0.6f);
        float side = b.x < PITCH_W / 2 ? 1.f : -1.f;
        float ahead = possTeam >= 0 ? (attackDir[possTeam] < 0 ? -3.f : 3.f) : 0.f;     // en avance sur le jeu, vers le but attaqué
        target = V2(b.x + side * 9.f, b.y + (b.y < PITCH_L / 2 ? 7.f : -7.f) + ahead);
        // ne pas rester sur la trajectoire d'une passe
        if (ball.owner < 0 && ball.vel.len() > 6.f) { V2 bd = ball.vel.norm(); V2 rp = refPos - ball.pos; float u = rp.dot(bd); if (u > 0 && u < 20.f && (rp - bd * u).len() < 2.f) target = refPos + V2(-bd.y, bd.x) * 3.f; }
    }
    // jamais collé au tireur d'un coup de pied arrêté
    if (state == MS_SETPIECE && sp != SP_KICKOFF && sp != SP_PENALTY && sp != SP_SHOOTOUT) {
        V2 o = target - spPos; float ol = o.len();
        if (ol < 9.f) target = spPos + (ol > 0.1f ? o * (1.f / ol) : (V2(PITCH_W / 2, PITCH_L / 2) - spPos).norm()) * 9.f;
    }
    target.x = clampf(target.x, 2.f, PITCH_W - 2.f); target.y = clampf(target.y, 2.f, PITCH_L - 2.f);
    V2 d = target - refPos;
    float dl = d.len();
    // allure naturelle : petites foulées près de l'action, course soutenue quand le jeu s'éloigne, sprint pour un carton ou une bagarre ;
    // zone morte autour de la cible (pas de piétinement), accélération et freinage progressifs
    bool urgent = refCardT > 0 || fightT > 0 || (pendCardOff >= 0 && state != MS_PLAY);
    static const float DEAD = 2.2f;
    float sp = 0;
    if (dl > DEAD) sp = urgent ? 6.8f : dl > 14.f ? 6.f : dl > 7.f ? 4.4f : 2.8f;
    V2 want = sp > 0 ? d * (std::min(sp, (dl - DEAD * 0.5f) * 1.6f) / dl) : V2();
    float acc = want.len() > refVel.len() ? 3.2f : 4.5f;
    refVel = refVel + (want - refVel) * std::min(1.f, dt * acc);
    refPos += refVel * dt;
    refAnim += refVel.len() * dt;
    V2 f = refVel.len() > 0.6f ? refVel.norm() : (ball.pos - refPos).norm();
    refFace = (refFace + (f - refFace) * std::min(1.f, dt * (refVel.len() > 0.6f ? 8.f : 4.f))).norm();
    if (refFace.len2() < 0.5f) refFace = f.len2() > 0.5f ? f : V2(0, 1);
}

void Match::refMove(V2 tg, float spd, V2 faceIdle, float dt) {
    V2 d = tg - refPos; float l = d.len();
    if (l > 0.08f) {
        float v = std::min(spd, 0.6f + l * 2.2f);             // freine en arrivant
        V2 dir = d * (1.f / l);
        refVel = dir * v;
        refPos += dir * std::min(l, v * dt);
        refAnim += std::min(l, v * dt);
        refFace = (refFace + (dir - refFace) * std::min(1.f, dt * 10.f)).norm();
        if (refFace.len2() < 0.5f) refFace = dir;
    } else {
        refVel = V2();
        if (faceIdle.len2() > 0.5f) { refFace = (refFace + (faceIdle - refFace) * std::min(1.f, dt * 6.f)).norm(); if (refFace.len2() < 0.5f) refFace = faceIdle; }
    }
}

// carton : l'arbitre court vers le joueur et le montre
void Match::giveCard(int off, int type) {
    MPlayer& o = pl[off];
    Team& T = g_world.teams[o.team == 0 ? S.home : S.away];
    const Team& T0 = team(o.team);
    bool second = type == 1 && o.yellow >= 1;
    if (!S.neutral && !S.training) playSfx(o.team == 0 ? SFX_FANS_WHISTLE : SFX_CLAPS);   // carton : sifflets du public local, ou applaudissements ironiques
    MatchEvent e; e.team = o.team; e.minute = clock; e.player = playerName(off);
    if (o.squad >= 0 && o.squad < (int)T0.squad.size()) e.pid = T0.squad[o.squad].id;
    refCardFor = off; refCardT = 2.2f; refCardSecond = false;
    rumbleFx(-1 - o.team, 8);                                       // RB_CARD
    if (second) { refCardT = 3.8f; refCardSecond = true; }
    if (type == 2 || second) {
        cardShow = 2; refCardType = 2;
        int nOff = 1; for (int j = o.team * 11; j < o.team * 11 + 11; j++) if (pl[j].sentOff) nOff++;
        static const char* LEFTW[] = { "onze", "dix", "neuf", "huit", "sept", "six", "cinq" };
        const char* left = LEFTW[std::min(6, nOff)];
        const std::string tn = T0.name, pn = playerName(off);
        msg = second ? "2E CARTON JAUNE" : "CARTON ROUGE !"; msg2 = pn; msgT = second ? 1.9f : 2.4f;
        if (nOff >= 2 && !second) { msg = fmt("%dE ROUGE POUR %s !", nOff, T0.shortName.c_str()); msg2 = pn + fmt(" - %s à %s", T0.shortName.c_str(), left); }
        if (second) cardShow = 1;          // le rouge suivra (voir l'arbitre dans update)
        if (second) { MatchEvent y = e; y.type = 1; events.push_back(y); }
        e.type = 2; events.push_back(e);
        if (o.squad < (int)T.squad.size()) { T.squad[o.squad].suspended = (int8_t)(second ? 2 : 2 + R.range(0, 2)); T.squad[o.squad].sRed++; }
        playSfx(SFX_CARD);
        std::string line;
        if (nOff == 1) line = second ? pn + " prend un deuxième jaune : il est expulsé ! " + tn + " va devoir finir à dix." : "Carton rouge pour " + pn + " ! Son équipe va devoir finir à dix.";
        else if (nOff == 2) line = second ? "Deuxième jaune pour " + pn + " et deuxième expulsion pour " + tn + " ! Les voilà réduits à neuf." : "Et de deux ! Nouveau carton rouge pour " + tn + " : " + pn + " est expulsé, ils ne sont plus que neuf !";
        else if (nOff == 3) line = "Incroyable, un troisième carton rouge pour " + tn + " ! " + pn + " quitte à son tour le terrain, ils finiront à huit.";
        else line = fmt("C'est la débandade : %s est le %de joueur expulsé de %s, qui n'est plus qu'à %s !", pn.c_str(), nOff, tn.c_str(), left);
        say(line, 4.f, true);
        sendOff(off);
        checkAbandon();
    } else {
        cardShow = 1; refCardType = 1;
        o.yellow++;
        {   // avertissement jugé injuste : le joueur conteste et s'énerve
            float cons = S.referee >= 0 && S.referee < NUM_REFEREES ? REFEREES[S.referee].consistency / 100.f : 0.6f;
            if (R.chance(0.12f + (1 - cons) * 0.3f)) { addAnger(off, 0.35f); say(playerName(off) + " conteste vivement cet avertissement !", 2.6f); }
            else addAnger(off, 0.1f);
        }
        msg = "CARTON JAUNE"; msg2 = playerName(off); msgT = 2.2f;
        e.type = 1; events.push_back(e);
        if (o.squad < (int)T.squad.size()) {
            Player& P = T.squad[o.squad];
            P.yellows++; P.sYel++;
            if (P.yellows >= S.yellowLimit) { P.suspended = 2; P.yellows = 0; }
        }
        playSfx(SFX_CARD);
        say("Avertissement pour " + playerName(off) + ".", 3.f, true);
    }
}

// bagarre : les deux joueurs s'accrochent, l'arbitre sanctionne
void Match::startFight(int a, int b) {
    if (S.training || fightT > 0 || a < 0 || b < 0 || !pl[a].onPitch || !pl[b].onPitch) return;
    if (fightLevel == 0) fightLevel = 2;
    fightA = a; fightB = b; fightT = fightLevel == 1 ? 1.6f : 2.6f; fightDone = false;
    duel = false;
    if (fightLevel == 2) {
        // combat au corps à corps quand un joueur humain est impliqué
        int ca = -1, cb = -1;
        for (int c = 0; c < NUM_INPUTS; c++) { if (S.side[c] == pl[a].team && ca < 0) ca = c; if (S.side[c] == pl[b].team && cb < 0) cb = c; }
        if (ca >= 0 || cb >= 0) {
            duel = true; duelHp[0] = duelHp[1] = 100; duelCool[0] = duelCool[1] = 0.6f; duelGuard[0] = duelGuard[1] = 0; duelAtk[0] = duelAtk[1] = 0;
            duelAtkT[0] = duelAtkT[1] = 0; duelHitFx[0] = duelHitFx[1] = 0; duelCtl[0] = ca; duelCtl[1] = cb; duelT = 0; duelLoser = -1; fightT = 99.f;
        }
    }
    pl[a].state = PS_FIGHT; pl[a].st = 0; pl[b].state = PS_FIGHT; pl[b].st = 0;
    pl[a].vel = V2(); pl[b].vel = V2();
    V2 mid = (pl[a].pos + pl[b].pos) * 0.5f;
    // échauffourée générale : parfois, un joueur de chaque camp s'en mêle
    fightExtra[0] = fightExtra[1] = -1;
    if (fightLevel == 2 && !duel && R.chance(0.35f)) for (int k = 0; k < 2; k++) {
        int tm = pl[k ? b : a].team, best = -1; float bd = 12.f;
        for (int j = tm * 11; j < tm * 11 + 11; j++) if (pl[j].onPitch && !pl[j].gk && j != a && j != b) { float d = (pl[j].pos - mid).len(); if (d < bd) { bd = d; best = j; } }
        fightExtra[k] = best;
    }
    pl[a].pos = mid + V2(-0.4f, 0); pl[b].pos = mid + V2(0.4f, 0);
    state = MS_STOP; stateT = 0;
    msg = fightLevel == 1 ? "ÉCHAUFFOURÉE !" : "BAGARRE !"; msg2 = playerName(a) + " / " + playerName(b); msgT = 2.6f;
    if (fightLevel == 1) say("Bousculade entre " + playerName(a) + " et " + playerName(b) + " ! Les esprits s'échauffent...", 3.f, true);
    else say("Ça dégénère entre " + playerName(a) + " et " + playerName(b) + " ! Ils en viennent aux mains !", 3.5f, true);
    if (fightLevel == 2) playSfx(SFX_BOO);
    playSfx(SFX_CROWD_OOH);
}

void Match::startPeriod(int p) {
    period = p;
    clock = p < 2 ? p * (float)S.halfMinutes : 2.f*S.halfMinutes + (p - 2) * 15.f;
    periodEnd = clock + (p < 2 ? (float)S.halfMinutes : 15.f);
    added = 0; boardDone = false; boardT = 0; subsPeriod = 0;   // temps additionnel annoncé à la dernière minute
    bool swap = (p == 1 || p == 3);
    if (sideFlip) swap = !swap;                    // côté choisi au tirage au sort
    if (p >= 2 && etFlip >= 0) swap = (p == 3) != (etFlip == 1);   // prolongation : côté tiré au sort
    attackDir[0] = swap ? 1 : -1;
    attackDir[1] = -attackDir[0];
    if (p == 0) kickoffTeam = firstKickoff;
    else if (p == 1) kickoffTeam = 1 - firstKickoff;
    else if (p == 2) kickoffTeam = etKickoff >= 0 ? etKickoff : R.range(0, 1);
    else kickoffTeam = 1 - kickoffTeam;
    for (auto& q : pl) if (q.onPitch) { q.state = PS_NORMAL; q.vel = V2(); }
    beginSetPiece(SP_KICKOFF, kickoffTeam, V2(PITCH_W / 2, PITCH_L / 2));
    if (p == 1 && !S.training && htWaiting) {
        // retour des vestiaires : les joueurs ressortent du tunnel et regagnent leur place
        htWaiting = htGo = false;
        for (int i = 0; i < 22; i++) if (pl[i].onPitch) { pl[i].pos = tunnelSpot(pl[i].team, pl[i].slot + 1); pl[i].face = V2(1, 0); }
        refPos = V2(-3.f, PITCH_L / 2);
        state = MS_WALKOUT; stateT = 0; walkT = 0;
        msg = "RETOUR DES VESTIAIRES"; msg2 = ""; msgT = 2.5f;
        return;
    }
    playSfx(SFX_WHISTLE);
}

// les joueurs regagnent le tunnel (mi-temps, fin du match)
void Match::walkToTunnel(float dt, float spd) {
    for (int i = 0; i < 22; i++) {
        MPlayer& p = pl[i];
        if (!p.onPitch) continue;
        V2 door(-3.f, PITCH_L / 2 + (p.team ? 1.f : -1.f));
        V2 tg = p.pos.x > 1.f ? V2(1.2f, PITCH_L / 2 + (p.team ? 1.f : -1.f)) : door;
        if (p.pos.x > 1.f && (p.pos - tg).len() > 20.f) tg = door;
        V2 d = tg - p.pos;
        float sp = spd * (0.8f + (i % 5) * 0.08f);
        if (d.len() > 0.2f) { p.vel = d.norm() * std::min(sp, d.len() * 3); p.pos += p.vel * dt; p.anim += p.vel.len() * dt; p.face = d.norm(); }
        else { p.vel = V2(); if (tg.x > 0) p.pos = V2(0.f, tg.y); }
        p.state = PS_NORMAL;
    }
    V2 rd = V2(-3.f, PITCH_L / 2) - refPos;
    if (rd.len() > 0.2f) { refVel = rd.norm() * std::min(spd, rd.len() * 3); refPos += refVel * dt; refFace = rd.norm(); } else refVel = V2();
}

// fin du match : les joueurs des deux équipes se cherchent sur le terrain pour se serrer la main (pas en ligne),
// certains échangent leur maillot ; l'arbitre ramasse le ballon et rentre au vestiaire avec, sous le bras
void Match::updateEndScene(float dt) {
    walkT += dt;
    if (msgT > 0) msgT -= dt;
    auto walk = [&](V2& pos, V2& vel, float& anim, V2& face, V2 tg, float spd) {
        V2 d = tg - pos;
        if (d.len() > 0.12f) { vel = d.norm() * std::min(spd, d.len() * 3.f); pos += vel * dt; anim += vel.len() * dt; face = d.norm(); return false; }
        vel = V2(); return true;
    };
    // l'arbitre va chercher le ballon, puis le porte
    if (!refCarry) {
        V2 bp = ball.inNet ? V2(clampf(ball.pos.x, 1.f, PITCH_W - 1.f), clampf(ball.pos.y, 0.5f, PITCH_L - 0.5f)) : ball.pos;
        float dummyAnim = refAnim;
        if (walk(refPos, refVel, dummyAnim, refFace, bp, 3.f) || (refPos - bp).len() < 0.7f) { refCarry = true; ball.inNet = false; }
        refAnim = dummyAnim;
    }
    if (refCarry) { ball.owner = -1; ball.vel = V2(); ball.vz = 0; }
    // appariements : deux tours de poignées de main avec des adversaires tirés au hasard
    if (endPairs.empty() && walkT > 0.6f) {
        std::vector<int> A, B;
        for (int i = 0; i < 22; i++) if (pl[i].onPitch) (pl[i].team ? B : A).push_back(i);
        if (!A.empty() && !B.empty()) {
            for (int r = 0; r < 2; r++) {
                std::vector<int> a = A, b = B;
                for (int k = (int)b.size() - 1; k > 0; k--) std::swap(b[k], b[R.range(0, k)]);
                for (size_t k = 0; k < a.size(); k++) {
                    EndPair e; e.a = a[k]; e.b = b[k % b.size()]; e.round = r;
                    e.swap = r == 0 && R.chance(0.22f) && pl[e.a].shirtOf < 0 && pl[e.b].shirtOf < 0 && !pl[e.a].gk && !pl[e.b].gk;
                    endPairs.push_back(e);
                }
            }
        }
    }
    bool allDone = !endPairs.empty();
    std::vector<int> busy(22, -1);
    for (int pi = 0; pi < (int)endPairs.size(); pi++) {
        EndPair& e = endPairs[pi];
        if (e.done) continue;
        allDone = false;
        if (busy[e.a] >= 0 || busy[e.b] >= 0) continue;           // un joueur ne fait qu'une poignée de main à la fois
        busy[e.a] = busy[e.b] = pi;
        if (!e.active) { e.active = true; e.meet = (pl[e.a].pos + pl[e.b].pos) * 0.5f; e.meet.x = clampf(e.meet.x, 2.f, PITCH_W - 2.f); e.meet.y = clampf(e.meet.y, 2.f, PITCH_L - 2.f); }
        MPlayer& a = pl[e.a]; MPlayer& b = pl[e.b];
        bool ia = walk(a.pos, a.vel, a.anim, a.face, e.meet + V2(-0.45f, 0), 2.6f + (e.a % 3) * 0.3f);
        bool ib = walk(b.pos, b.vel, b.anim, b.face, e.meet + V2(0.45f, 0), 2.6f + (e.b % 3) * 0.3f);
        a.state = b.state = PS_NORMAL;
        if (ia && ib) {
            e.t += dt;
            a.face = V2(1, 0); b.face = V2(-1, 0);
            if (e.t < 1.2f) { a.state = b.state = PS_HAND; }
            else if (e.swap && e.t < 2.6f) {                         // échange de maillots
                if (e.t > 1.6f && a.shirtOf < 0) { a.shirtOf = b.team; b.shirtOf = a.team; if (R.chance(0.6f)) say(playerName(e.a) + " et " + playerName(e.b) + " échangent leur maillot.", 2.2f); }
                a.state = b.state = PS_CELEB;
            } else e.done = true;
        }
    }
    // les autres attendent leur tour en marchant tranquillement
    for (int i = 0; i < 22; i++) if (pl[i].onPitch && busy[i] < 0 && !allDone) { pl[i].vel = pl[i].vel * 0.9f; pl[i].state = PS_NORMAL; }
    if (allDone || walkT > 16.f || endPairs.empty()) walkToTunnel(dt, 3.2f);
    if (refCarry) { ball.pos = refPos + V2(refFace.x >= 0 ? 0.35f : -0.35f, 0.05f); ball.z = 1.f; }   // le ballon sous le bras de l'arbitre
    // caméra : suit les poignées de main, puis le retour au tunnel
    V2 f(PITCH_W / 2, PITCH_L / 2);
    if (allDone || walkT > 16.f) f = V2(14.f, PITCH_L / 2);
    else { float sx = 0, sy = 0; int n = 0; for (auto& p : pl) if (p.onPitch) { sx += p.pos.x; sy += p.pos.y; n++; } if (n) f = V2(sx / n, sy / n); }
    cam = cam + (f - cam) * std::min(1.f, dt * 1.2f);
    camZoom = camZoom + ((allDone || walkT > 16.f ? 1.2f : 1.7f) - camZoom) * std::min(1.f, dt * 1.5f);   // caméra rapprochée sur les poignées de main
}

// ------------------------------------------------------------------ joueurs utilitaires
int Match::nearestToBall(int t, int exclude) const {
    int best = -1; float bd = 1e9;
    V2 bp = ball.pos + ball.vel * 0.3f;
    for (int i = t * 11; i < t * 11 + 11; i++) {
        const MPlayer& p = pl[i];
        if (!p.onPitch || i == exclude || p.gk) continue;
        if (p.state == PS_DOWN || p.state == PS_OFF) continue;
        float d = (p.pos - bp).len();
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}

void Match::formationTarget(int i, V2& out) const {
    const MPlayer& p = pl[i];
    int t = p.team;
    if (p.gk || p.slot <= 0 || p.slot > 10) {
        // gardien (slot 0) : hors des tableaux de formation (indexés slot - 1), devant sa ligne dans l'axe ballon-but
        V2 gg = ownGoal(t);
        V2 d = ball.pos - gg; float l = d.len();
        out = gg + (l > 0.01f ? d * (std::min(2.2f, 0.6f + l * 0.04f) / l) : V2(0, gg.y == 0 ? 1.0f : -1.0f));
        out.x = clampf(out.x, PITCH_W / 2 - 4.f, PITCH_W / 2 + 4.f);
        return;
    }
    const Formation& F = FORMATIONS[formation[t]];
    float u0 = F.y[p.slot - 1], v0 = F.x[p.slot - 1];
    float bu = progress(t, ball.pos);
    // position du ballon dans le repère de l'équipe (v)
    V2 tb = ball.pos;
    float bv = attackDir[t] < 0 ? tb.x / PITCH_W : 1 - tb.x / PITCH_W;
    bool attacking = (possTeam == t);
    float u = u0 * 0.62f + bu * 0.48f + (attacking ? 0.10f : -0.08f);
    if (p.role == 3 && attacking) u += 0.05f;
    if (p.role == 1 && !attacking) u -= 0.03f;
    u = clampf(u, 0.04f, 0.95f);
    {   // mentalité : bloc plus haut ou plus bas
        static const float SH[5] = { -0.08f, -0.04f, 0.f, 0.04f, 0.08f };
        u += SH[std::max(0, std::min(4, mentality[t]))] * (p.role == 3 ? 0.6f : 1.f);
        u = clampf(u, 0.04f, 0.95f);
    }
    // consigne : hauteur de la ligne défensive (bloc bas / haut)
    u = clampf(u + (S.tac[t][1] - 1) * (p.role == 1 ? 0.045f : 0.035f), 0.04f, 0.95f);
    if (p.role == 1) {
        // même en tout-attaque, la défense reste proche de sa zone : axiaux vers la ligne médiane au plus, latéraux un peu plus haut
        bool central = std::fabs(v0 - 0.5f) < 0.2f;
        float cap = (central ? 0.50f : 0.62f) + (mentality[t] >= 3 ? 0.03f : 0.f) + (S.tac[t][1] - 1) * 0.04f;
        u = std::min(u, cap);
        // sans le ballon : toujours entre le ballon et le but
        if (!attacking) u = std::min(u, std::max(0.05f, bu - (central ? 0.07f : 0.04f)));
    }
    // en phase défensive près de son but : les défenseurs restent entre le ballon et le but
    if (!attacking && p.role == 1 && bu < 0.4f) u = std::min(u, std::max(0.05f, bu - 0.02f + (u0 - 0.2f) * 0.1f));
    if (!attacking && p.role == 2 && bu < 0.25f) u = std::min(u, bu + 0.2f);
    float v = v0 * 0.72f + bv * 0.28f;
    {   // consigne : largeur (plus large en attaque)
        static const float WF[3] = { 0.9f, 1.0f, 1.08f };
        float wf = WF[S.tac[t][2] % 3];
        if (!attacking) wf = 1.f + (wf - 1.f) * 0.4f;
        v = 0.5f + (v - 0.5f) * wf;
    }
    // placement : les bons défenseurs se replacent plus près de l'axe ballon-but
    if (!attacking && p.role == 1) { float k = (p.posi - 50.f) / 400.f; v = v * (1 - k) + bv * k; }
    v = clampf(v, 0.04f, 0.96f);
    if(S.rules==RULESET_CAREER){const auto& e=p.effects;u+=e.advance*(attacking?1.f:.4f);if(e.drop&&attacking)u=std::min(u,bu+.08f);if(e.halfback&&attacking)u=.25f+.12f*bu;v=.5f+(v-.5f)*e.width;
      if(!attacking&&(p.tactic.instructions&(1u<<TI_MARK))){for(int j=(1-t)*11+1;j<(1-t)*11+11;j++){if(pl[j].onPitch&&team(1-t).squad[pl[j].squad].id==p.tactic.markPid){out=pl[j].pos+(ownGoal(t)-pl[j].pos).norm()*1.6f;return;}}}}
    // rôles tactiques détaillés (mode Carrière) : déplacements propres à chaque rôle
    if (S.rules == RULESET_CAREER && p.effects.behave != RB_NONE) {
        int bh = p.effects.behave;
        float side = v0 < 0.5f ? -1.f : 1.f;                                             // côté du joueur dans le repère de l'équipe
        float ph = std::fmod(clock * 0.5f + i * 0.137f, 1.f);                             // cycle d'appels (~2 minutes de jeu)
        bool ballSide = (bv - 0.5f) * side > 0.08f;                                       // ballon de son côté
        switch (bh) {
        case RB_FALSE9:
            // décroche entre les lignes, libère l'axe pour les ailiers, puis attaque la profondeur de temps en temps
            if (attacking) { u = ph < 0.18f && bu > 0.5f ? u + 0.2f : std::min(u, std::max(0.42f, bu + 0.02f)); v = v * 0.5f + (0.5f + (bv - 0.5f) * 0.2f) * 0.5f; }
            else u = std::max(u, std::min(0.62f, bu + 0.12f));                             // reste entre les lignes adverses pour la contre-attaque
            break;
        case RB_INSIDE_FWD:
            // appels vers l'axe et dans la surface, laisse le couloir au latéral
            if (attacking) { v = 0.5f + side * (bu > 0.62f ? 0.12f : 0.2f); if (bu > 0.6f && (ballSide || ph < 0.3f)) u += 0.12f; }
            break;
        case RB_ATT_FB:
            // dépassement côté ballon, appels extérieurs ; repli rapide à la perte
            if (attacking) { if (ballSide && bu > 0.4f) { u = std::max(u, bu + 0.07f); v = 0.5f + side * 0.45f; } else v = 0.5f + side * std::max(std::fabs(v - 0.5f), 0.4f); }
            else u = std::min(u, std::max(0.06f, bu - 0.06f));
            break;
        case RB_INV_FB:
            // rentre dans le milieu à la relance pour former un double pivot, ressort sur son côté sans le ballon
            if (attacking) { v = 0.5f + side * 0.16f; u = clampf(std::max(u, bu * 0.4f + 0.2f), 0.3f, 0.55f); }
            else v = v0 * 0.8f + v * 0.2f;
            break;
        case RB_MEZZALA:
            // occupe le demi-espace et part en diagonale vers la surface
            if (attacking) { v = 0.5f + side * 0.22f; u += bu > 0.5f ? 0.06f : 0.02f; if (ph < 0.2f && bu > 0.5f) { u += 0.12f; v = 0.5f + side * 0.15f; } }
            break;
        case RB_STOPPER:
            // sort au contact de l'attaquant qui décroche devant lui
            if (!attacking) {
                int ot = 1 - t, best = -1; float bd = 11.f;
                V2 here = fromTeamFrame(t, clampf(u, .04f, .95f), clampf(v, .04f, .96f));
                for (int j = ot * 11 + 1; j < ot * 11 + 11; j++) {
                    if (!pl[j].onPitch) continue;
                    float pr = progress(t, pl[j].pos), d = (pl[j].pos - here).len();
                    if (pr > u + 0.02f && pr < u + 0.2f && d < bd) { bd = d; best = j; }
                }
                if (best >= 0 && bu > 0.25f) { out = pl[best].pos + (ownGoal(t) - pl[best].pos).norm() * 1.3f; return; }
            }
            break;
        case RB_COVER: case RB_LIBERO: {
            // couverture : légèrement derrière la ligne, du côté du ballon ; libéro : dernier homme puis sortie de balle
            float minU = 1.f;
            for (int j = t * 11 + 1; j < t * 11 + 11; j++) if (j != i && pl[j].onPitch && pl[j].role == 1) minU = std::min(minU, progress(t, pl[j].pos));
            if (!attacking) { u = std::min(u - (bh == RB_LIBERO ? 0.04f : 0.02f), std::max(0.04f, minU - (bh == RB_LIBERO ? 0.05f : 0.03f))); v = v * 0.7f + bv * 0.3f; }
            else if (bh == RB_LIBERO && bu < 0.55f) { u = std::min(0.5f, std::max(u + 0.05f, bu + 0.08f)); v = 0.5f; }     // projection contrôlée dans le milieu
            break;
        }
        default: break;
        }
        u = clampf(u, 0.04f, 0.95f); v = clampf(v, 0.04f, 0.96f);
    }
    // l'ailier profite de l'espace libéré par le faux neuf
    if (S.rules == RULESET_CAREER && attacking && (p.detailedPosition == DP_AD || p.detailedPosition == DP_AG)) {
        for (int j = t * 11 + 1; j < t * 11 + 11; j++) if (j != i && pl[j].onPitch && pl[j].effects.behave == RB_FALSE9) { u = clampf(u + 0.04f, 0.04f, 0.95f); break; }
    }
    // ne pas se mettre hors-jeu (attaquants) : marge selon la lecture du jeu ; un attaquant peu attentif part parfois trop tôt
    if (attacking) {
        float line = offsideLine(t);
        float aw = clampf((p.posi * 0.6f + p.compo * 0.4f) / 100.f, 0.2f, 1.f);       // sens du hors-jeu
        float keep = (0.007f - aw * 0.004f);                                           // ~0,3 m (bon) à ~0,8 m (moyen) derrière la ligne
        float ph = std::fmod(clock * 0.41f + i * 0.613f, 1.f);
        if (p.role == 3 && ph < (1.f - aw) * 0.12f) keep = -0.005f * (1.f - aw) - 0.001f;   // appel lancé trop tôt
        if (u > line - keep) u = line - keep;
    }
    // piège du hors-jeu : ligne haute, la défense remonte d'un cran quand le porteur adverse lève la tête au milieu ; un défenseur peu discipliné traîne
    if (!attacking && p.role == 1 && S.tac[t][1] == 2 && possTeam == 1 - t && bu > 0.3f && bu < 0.62f) {
        u += p.posi >= 58 ? 0.008f : -0.006f;
        u = clampf(u, 0.04f, std::max(0.05f, bu - 0.03f));
    }

    out = fromTeamFrame(t, clampf(u,.04f,.95f), clampf(v,.04f,.96f));
}

// ------------------------------------------------------------------ ballon
void Match::takePossession(int i) {
    checkOffsideTouch(i);
    if (state != MS_PLAY) return;
    ball.owner = i;
    ball.lastTouch = i; ball.lastTeam = pl[i].team;
    ball.vel = V2(); ball.vz = 0; ball.z = 0; ball.spin = 0; ball.aftertouch = 0;
    ball.backpass = false;
    if (possTeam >= 0 && possTeam != pl[i].team) lostT[possTeam] = 0;
    possTeam = pl[i].team;
}

void Match::kickBall(int i, V2 dir, float speed, float vz, bool human, bool deliberate) {
    MPlayer& p = pl[i];
    statsTouch(i);pendingPass=-1;pendingShot=-1;shotHitWoodwork=false;
    dir = dir.norm();
    if (dir.len2() < 0.01f) dir = p.face;
    ball.owner = -1;
    // pluie, orage, neige : ballon glissant, frappes et passes moins précises
    if (S.meteo >= 3 && S.meteo <= 5 && !S.training) {
        static const float JIT[6] = { 0, 0, 0, 0.035f, 0.06f, 0.045f };
        float j = JIT[S.meteo] * (1.4f - p.compo / 100.f) * R.frange(-1.f, 1.f);
        dir = V2(dir.x * std::cos(j) - dir.y * std::sin(j), dir.x * std::sin(j) + dir.y * std::cos(j));
    }
    ball.vel = dir * speed;
    ball.vz = vz;
    ball.z = std::max(ball.z, 0.05f);
    ball.spin = 0;
    ball.lastTouch = i; ball.lastTeam = p.team;
    ball.backpass = deliberate && !p.gk;
    ball.aftertouch = human ? 0.5f : 0;
    ball.aftertouchBy = human ? p.human : -1;
    p.cool = 0.3f;
    p.state = p.state == PS_NORMAL ? PS_KICK : p.state; p.st = 0;
    p.charging = false; p.charge = 0;
    armOffside(i);
    playSfx(speed > 21 ? SFX_SHOT : SFX_KICK);
    if (p.human >= 0) rumbleFx(p.human, speed > 21 ? 1 : 0);       // RB_SHOT / RB_KICK
    // tir cadré ?
    V2 g = goalCenter(p.team);
    if (!shootout&&!kickingPass&&(g - ball.pos).len() < 40 && dir.dot((g - ball.pos).norm()) > 0.70f && speed > 16) { shots[p.team]++;pendingShot=i; ps(i).shot++; lastShooter = i; lastShotAge = 0; lastShotDist = (g - ball.pos).len(); }
    else if(!shootout&&(kickingPass||deliberate&&speed<23)){passes[p.team]++;pendingPass=i;}
}

// frappe en force près du but : comme un penalty manqué, le ballon peut s'envoler au-dessus de la barre
float Match::shotLift(int i, V2 d, float speed, float pw, float vz) {
    const MPlayer& p = pl[i];
    V2 g = goalCenter(p.team);
    float dist = (g - ball.pos).len();
    if (pw < 0.72f || dist > 24.f || dist < 3.f || d.norm().dot((g - ball.pos).norm()) < 0.75f) return vz;
    float skill = (p.shoot * 0.6f + p.compo * 0.4f) / 100.f;
    float close = clampf((24.f - dist) / 14.f, 0.f, 1.f);                  // 1 à 10 m et moins
    float risk = (pw - 0.72f) * 3.6f * (1.3f - skill) * close;               // frappe à pleine puissance d'un joueur moyen à 10 m : ~45 %
    if (!R.chance(clampf(risk, 0.f, 0.6f))) return vz;
    float h = GOAL_H + R.frange(0.15f, 1.6f) + (1.f - skill) * R.frange(0.f, 1.2f);   // hauteur à la ligne de but
    float tt = dist / std::max(8.f, speed);
    return std::max(vz, (h + 0.5f * GRAV * tt * tt) / tt);
}

int Match::bestPassTarget(int i, V2 dir, bool lob, float maxDist) const {
    const MPlayer& p = pl[i];
    int best = -1; float bs = 1e9;
    bool hasDir = dir.len2() > 0.01f;
    V2 d0 = hasDir ? dir.norm() : p.face;
    for (int j = p.team * 11; j < p.team * 11 + 11; j++) {
        if (j == i || !pl[j].onPitch || pl[j].state == PS_OFF) continue;
        V2 d = pl[j].pos - p.pos;
        float dist = d.len();
        if (dist < 3 || dist > maxDist) continue;
        float c = d0.dot(d * (1 / dist));
        float ang = std::acos(clampf(c, -1, 1));
        if (ang > (hasDir ? 0.62f : 0.9f)) continue;
        float score = ang * 12 + dist * 0.12f;
        if (!lob) {
            for (int k = (1 - p.team) * 11; k < (1 - p.team) * 11 + 11; k++) {
                if (!pl[k].onPitch) continue;
                V2 w = pl[k].pos - p.pos;
                float tt = clampf(w.dot(d) / (dist * dist), 0, 1);
                float dd = (p.pos + d * tt - pl[k].pos).len();
                if (dd < 1.4f) score += 6;
            }
        }
        if (pl[j].gk) score += 8;
        if (score < bs) { bs = score; best = j; }
    }
    return best;
}

void Match::passTo(int i, int target, bool lob) {
    MPlayer& p = pl[i];
    MPlayer& q = pl[target];
    V2 tp = q.pos + q.vel * (lob ? 0.9f : 0.45f);
    V2 d = tp - ball.pos;
    float dist = d.len();
    float acc = (100 - p.pass) / 100.f;
    // pression adverse : passe moins précise (sang-froid)
    float press = 1e9; for (int k = (1 - p.team) * 11; k < (1 - p.team) * 11 + 11; k++) if (pl[k].onPitch) press = std::min(press, (pl[k].pos - p.pos).len());
    if (press < 2.5f) acc += (100 - p.compo) / 300.f;
    float err = R.frange(-1, 1) * (acc * 0.10f + acc * acc * 0.12f);     // les mauvais passeurs manquent nettement plus
    V2 dir = rot(d.norm(), err);
    // une-deux : après une passe courte vers l'avant, le passeur file dans l'espace ; le receveur peut remettre en une touche
    if (!p.gk && !lob && dist < 18.f && progress(p.team, p.pos) > 0.5f && R.chance(0.35f)) {
        p.runT = 1.4f;
        V2 fwd(0, (float)attackDir[p.team]);
        p.runTarget = p.pos + fwd * 11.f + V2((q.pos.x > p.pos.x ? -1.f : 1.f) * 2.f, 0);
        p.runTarget.x = clampf(p.runTarget.x, 2.f, PITCH_W - 2.f); p.runTarget.y = clampf(p.runTarget.y, 2.f, PITCH_L - 2.f);
        q.wallFrom = i; q.wallT = 1.6f;
    }
    if (p.human >= 0 && !q.gk && S.lockSquad[p.team] < 0) { passRecv[p.human] = target; passRecvT[p.human] = std::min(2.2f, 0.6f + dist / 14.f); }
    kickingPass=true;
    if (!lob) {
        float sp = clampf(std::sqrt(2 * 4.2f * pitchFriction * dist) + 3.0f, 8, 25);
        kickBall(i, dir, sp, 0.0f, p.human >= 0);
    } else {
        float vh = clampf(dist * 0.62f + 6, 10, 24);
        float T = dist / vh;
        float vz = 0.5f * GRAV * T;
        kickBall(i, dir, vh, vz, p.human >= 0);
    }
    kickingPass=false;
}

void Match::updateBall(float dt) {
    Ball& b = ball;
    if (b.owner >= 0) {
        MPlayer& o = pl[b.owner];
        if (o.state == PS_GKHOLD) {
            o.pos.x = clampf(o.pos.x, 0.5f, PITCH_W - 0.5f);
            o.pos.y = clampf(o.pos.y, 0.6f, PITCH_L - 0.6f);
            b.pos = o.pos; b.z = 1.0f; b.vel = V2(); b.vz = 0; return;
        }
        V2 f = o.face;
        float lead = 0.45f + std::min(0.35f, o.vel.len() * 0.04f);
        b.pos = o.pos + f * lead;
        b.z = 0; b.vel = o.vel; b.vz = 0;
        // tacles "debout" automatiques des adversaires
        for (int j = (1 - o.team) * 11; j < (1 - o.team) * 11 + 11; j++) {
            MPlayer& q = pl[j];
            if (!q.onPitch || q.state != PS_NORMAL || q.cool > 0) continue;
            if ((q.pos - b.pos).len() < (q.human >= 0 ? 0.75f : 0.85f)) {
                // tacle debout : tacle du défenseur contre dribble / vitesse / conduite du porteur
                float chance = dt * 2.6f * (q.tackle / (q.tackle + o.dribble * 0.75f + o.speed * 0.25f + 18.f));
                if (q.human >= 0) chance *= 1.3f;
                else {
                    // défenseur bien placé (entre le porteur et son but, face au jeu) : il gagne plus souvent le duel
                    V2 toGoal = (ownGoal(q.team) - o.pos).norm();
                    float side = (q.pos - o.pos).norm().dot(toGoal);
                    if (side > 0.35f) chance *= 1.25f + 0.35f * side;
                    else if (side < -0.4f) chance *= 0.7f;                 // revient de l'arrière : moins efficace
                    chance *= 0.85f + q.posi / 330.f;
                }
                if (o.human >= 0 && S.difficulty == 2) chance *= 1.35f;
                if (o.human >= 0 && S.difficulty == 0) chance *= 0.7f;
                if (R.chance(chance)) {
                    if (!shootout) ps(j).tackle++;
                    b.owner = -1;
                    V2 kd = (b.pos - q.pos).norm();
                    b.vel = kd * 3.5f + q.vel * 0.7f;
                    b.lastTouch = j; b.lastTeam = q.team;
                    o.cool = 0.45f;
                    checkOffsideTouch(j, false);      // tacle : ballon détourné, pas de jeu maîtrisé
                    playSfx(SFX_TACKLE);
                    if (q.human >= 0) rumbleFx(q.human, 2);                      // RB_TACKLE (le tacleur)
                    if (o.human >= 0) rumbleFx(o.human, 3);                      // RB_FOULED (le joueur dépossédé)
                    break;
                }
            }
        }
        // gardien qui plonge dans les pieds de l'attaquant
        for (int j = (1 - o.team) * 11; j < (1 - o.team) * 11 + 11; j++) {
            MPlayer& g = pl[j];
            if (!g.onPitch || !g.gk || !g.smother || g.state != PS_DIVE) continue;
            if ((g.pos - b.pos).len() > 1.3f) continue;
            g.smother = false;
            int oi = b.owner;
            float win = 0.5f + g.keep / 280.f - (o.dribble * 0.6f + o.speed * 0.4f) / 380.f;
            // face-à-face contre un joueur humain : le gardien de l'ordinateur ferme mieux l'angle et anticipe le crochet
            if (o.human >= 0 && g.human < 0) win += S.difficulty == 0 ? 0.f : S.difficulty == 2 ? 0.12f : 0.05f;
            if (o.human >= 0 && S.difficulty == 0) win -= 0.1f;
            if (o.human >= 0 && S.difficulty == 2) win += 0.08f;
            win = clampf(win, 0.25f, 0.9f);
            float r = R.f();
            if (r < win) {
                // le gardien s'empare du ballon
                checkOffsideTouch(j);
                if (state != MS_PLAY) return;
                b.owner = j; b.vel = V2(); b.vz = 0; b.spin = 0; b.aftertouch = 0;
                b.lastTouch = j; b.lastTeam = g.team; b.backpass = false;
                possTeam = g.team;
                g.state = PS_GKHOLD; g.st = 0; g.vel = V2();
                o.cool = 0.6f;
                say("Superbe sortie de " + playerName(j) + " dans les pieds de " + playerName(oi) + " !", 2.8f, true);
                playSfx(SFX_CROWD_OOH);
                return;
            } else if (r < win + 0.07f && inOwnBox(g.team, g.pos)) {
                // le gardien fauche l'attaquant : penalty
                g.state = PS_DOWN; g.st = 0;
                foul(j, oi, false);
                return;
            } else if (r < win + 0.35f) {
                // ballon repoussé par le gardien
                b.owner = -1;
                V2 kd = (b.pos - g.pos).norm(); if (kd.len2() < 0.01f) kd = V2(1, 0);
                b.vel = V2(kd.x * 6.f + R.frange(-2, 2), kd.y * 3.f); b.lastTouch = j; b.lastTeam = g.team;
                o.cool = 0.4f;
                playSfx(SFX_KICK);
                return;
            }
            // raté : l'attaquant l'a passé
        }
        return;
    }
    // vent d'orage : dévie les ballons aériens
    if (S.meteo == 4 && b.z > 0.4f) b.vel += wind * (dt * 0.9f);
    // effet (aftertouch)
    if (b.aftertouch > 0) {
        b.aftertouch -= dt;
        if (b.aftertouchBy >= 0) {
            V2 d = ctl[b.aftertouchBy].dir;
            V2 bv = b.vel.norm();
            if (d.len2() > 0.01f && b.vel.len() > 3) {
                float side = d.dot(bv.perp());
                float along = d.dot(bv);
                b.spin = clampf(b.spin + side * 26 * dt, -9, 9);
                if (along < -0.3f) { b.vz += 9 * dt; b.vel = b.vel * (1 - 0.25f * dt); }
                if (along > 0.3f) { b.vz -= 7 * dt; }
            }
        }
    }
    float sp = b.vel.len();
    if (std::fabs(b.spin) > 0.01f && sp > 1) {
        b.vel += b.vel.norm().perp() * (b.spin * dt);
        b.spin *= (1 - 0.9f * dt);
    }
    // gravité / rebonds
    if (b.z > 0 || b.vz > 0) {
        b.vz -= GRAV * dt;
        b.z += b.vz * dt;
        if (b.z <= 0) {
            b.z = 0;
            if (b.vz < -1.8f) { b.vz = -b.vz * pitchBounce; b.vel = b.vel * 0.82f; if (b.vz > 1.2f) playSfx(SFX_BOUNCE); }
            else b.vz = 0;
        }
        b.vel = b.vel * (1 - 0.08f * dt);
    } else {
        float dec = (3.0f * pitchFriction + 0.35f * sp) * dt;
        if (sp <= dec) b.vel = V2(); else b.vel = b.vel * ((sp - dec) / sp);
        b.spin *= (1 - 3 * dt);
        // pelouse abîmée : le ballon qui roule dévie et saute sur les mottes
        if (turfBad > 0.05f && sp > 2.5f && b.owner < 0) {
            b.vel = rot(b.vel, R.frange(-1.f, 1.f) * turfBad * 1.4f * dt * std::min(1.f, sp / 10.f));
            if (R.chance(turfBad * turfBad * dt * 1.6f)) { b.vz = R.frange(1.2f, 3.2f) * turfBad; b.z = 0.02f; b.vel = rot(b.vel, R.frange(-0.35f, 0.35f) * turfBad); }
        }
    }
    V2 prev = b.pos;
    b.pos += b.vel * dt;

    // poteaux & barre
    for (int g = 0; g < 2; g++) {
        float gy = g == 0 ? 0 : PITCH_L;
        for (int s = -1; s <= 1; s += 2) {
            V2 post(PITCH_W / 2 + s * GOAL_W / 2, gy);
            V2 d = b.pos - post;
            if (b.z < GOAL_H && d.len() < BALL_R + 0.07f) {
                V2 n = d.norm();
                float vn = b.vel.dot(n);
                if (vn < 0) { b.vel = (b.vel - n * (2 * vn)) * 0.6f; b.pos = post + n * (BALL_R + 0.08f); statsWoodwork();playSfx(SFX_POST); playSfx(SFX_CROWD_OOH); rumbleFx(-1, 4); rumbleFx(-2, 4); }
            }
        }
        bool crossing = (prev.y - gy) * (b.pos.y - gy) <= 0 && prev.y != b.pos.y;
        if (crossing && std::fabs(b.pos.x - PITCH_W / 2) < GOAL_W / 2 && b.z > GOAL_H - 0.12f && b.z < GOAL_H + 0.2f) {
            b.vel.y = -b.vel.y * 0.5f; b.vz = -std::fabs(b.vz) * 0.5f; b.pos.y = prev.y;
            statsWoodwork();playSfx(SFX_POST); playSfx(SFX_CROWD_OOH); rumbleFx(-1, 4); rumbleFx(-2, 4);
        }
    }
    // filets vus de l'extérieur (petit filet latéral, fond, toit) : le ballon ne traverse pas le but
    if (!b.inNet) {
        for (int g = 0; g < 2; g++) {
            float gy = g == 0 ? 0 : PITCH_L, back = g == 0 ? -GOAL_DEPTH : PITCH_L + GOAL_DEPTH;
            float x0 = PITCH_W / 2 - GOAL_W / 2, x1 = PITCH_W / 2 + GOAL_W / 2;
            float ylo = std::min(gy, back), yhi = std::max(gy, back);
            bool inside = b.pos.x > x0 && b.pos.x < x1 && b.pos.y > ylo && b.pos.y < yhi && b.z < GOAL_H;
            bool wasIn = prev.x > x0 && prev.x < x1 && prev.y > ylo && prev.y < yhi && b.z - b.vz * dt < GOAL_H;
            if (!inside || wasIn) continue;
            bool front = g == 0 ? prev.y >= gy : prev.y <= gy;
            if (front) continue;                       // entrée par la ligne de but : c'est un but (géré ailleurs)
            if (prev.x <= x0) { b.pos.x = x0 - 0.14f; b.vel.x = -std::fabs(b.vel.x) * 0.25f; b.vel.y *= 0.5f; }
            else if (prev.x >= x1) { b.pos.x = x1 + 0.14f; b.vel.x = std::fabs(b.vel.x) * 0.25f; b.vel.y *= 0.5f; }
            else if (b.z - b.vz * dt >= GOAL_H - 0.05f) { b.z = GOAL_H + 0.05f; b.vz = std::fabs(b.vz) * 0.2f; b.vel = b.vel * 0.6f; }
            else { b.pos.y = g == 0 ? back - 0.14f : back + 0.14f; b.vel.y = -b.vel.y * 0.25f; b.vel.x *= 0.5f; }
        }
    }
    // filets
    if (b.inNet) {
        float gy = b.pos.y < PITCH_L / 2 ? 0 : PITCH_L;
        float sgn = gy == 0 ? -1 : 1;
        float depth = (b.pos.y - gy) * sgn;
        if (depth > GOAL_DEPTH - 0.2f) { b.pos.y = gy + sgn * (GOAL_DEPTH - 0.2f); b.vel = b.vel * 0.2f; }
        b.pos.x = clampf(b.pos.x, PITCH_W / 2 - GOAL_W / 2 + 0.15f, PITCH_W / 2 + GOAL_W / 2 - 0.15f);
        if (b.z > GOAL_H - 0.2f) { b.z = GOAL_H - 0.2f; b.vz = 0; }
        return;
    }
    if (state != MS_PLAY) return;

    // reprises de volée et retournés acrobatiques
    for (int i = 0; i < 22; i++) if (tryVolley(i)) return;
    // prise de balle / têtes / arrêts
    int best = -1; float bd = 1e9;
    for (int i = 0; i < 22; i++) {
        MPlayer& p = pl[i];
        if (!p.onPitch || p.cool > 0) continue;
        if (p.state != PS_NORMAL && p.state != PS_DIVE && p.state != PS_KICK && p.state != PS_SLIDE) continue;
        float d = (p.pos - b.pos).len();
        bool gkCatch = p.gk && inOwnBox(p.team, p.pos) && !(b.backpass && b.lastTeam == p.team);
        float reach = p.state == PS_DIVE ? 1.25f + p.keep / 220.f : (p.gk ? 0.85f + p.keep / 400.f : 0.7f);   // allonge du gardien selon son niveau
        // gardien de l'ordinateur face à une frappe humaine rapprochée : réflexe et placement légèrement meilleurs
        // (seulement à mi-distance : les frappes de loin et à bout portant gardent leurs chances)
        if (p.gk && p.human < 0 && b.lastTeam != p.team && b.lastTouch >= 0 && pl[b.lastTouch].human >= 0 && i != penGk && (pl[b.lastTouch].pos - ownGoal(p.team)).len() < 18.f && lastShotDist >= 9.f && lastShotDist <= 20.f)
            reach += S.difficulty == 0 ? 0.0f : S.difficulty == 2 ? 0.1f : 0.04f;
        // frappe de loin (le ballon file, le gardien doit s'étirer) ou à bout portant (réflexe) : allonge réduite, le but reste possible
        if (p.gk && i != penGk && b.lastTeam != p.team && lastShooter >= 0 && lastShotAge < 2.f && b.owner < 0) {
            if (lastShotDist > 20.f) reach *= 0.76f + std::max(0.f, 30.f - lastShotDist) * 0.006f;
            else if (lastShotDist < 9.f) reach *= 0.68f + lastShotDist * 0.02f;
        }
        if (i == penGk && p.gk) reach = p.state == PS_DIVE ? 0.7f + p.keep / 330.f : 0.55f + p.keep / 500.f;    // penalty : réflexe à 11 m
        float zmax = gkCatch ? (p.state == PS_DIVE ? 2.2f : 2.6f) : 0.95f;
        if (p.state == PS_SLIDE) { reach = 1.0f; zmax = 0.6f; }
        bool inField = b.pos.y > -BALL_R * 0.5f && b.pos.y < PITCH_L + BALL_R * 0.5f;
        if (d < reach && b.z < zmax && d < bd && inField) { bd = d; best = i; }
        // tête (détente selon le jeu de tête) ; tête plongeante sur un ballon à mi-hauteur près du but
        float headTop = 2.2f + p.heading * 0.007f;
        bool diving = !p.gk && p.state == PS_NORMAL && b.z >= 0.45f && b.z < 0.95f && progress(p.team, p.pos) > 0.8f && d < 1.1f && b.vel.len() > 5.f && b.lastTeam == p.team
                      && (p.human >= 0 ? shootBtn(p.human, false) != 0 : R.chance(0.45f));
        if (!p.gk && p.state == PS_NORMAL && ((b.z >= 0.95f && b.z < headTop && d < 0.8f + p.heading * 0.004f && b.vel.len() > 1.5f) || diving)) {
            V2 dir; float pw = 9 + (p.shoot * 0.35f + p.heading * 0.65f) / 11.f, vz = R.frange(0.5f, 2.5f);
            V2 g = goalCenter(p.team);
            float prog = progress(p.team, p.pos);
            float acc = 1.6f - p.heading / 90.f;
            if (p.human >= 0 && ctl[p.human].dir.len2() > 0.01f) dir = ctl[p.human].dir;
            else if (prog > 0.75f) {
                // tête vers le but : on vise un côté, précision selon le jeu de tête
                float side = R.chance(0.5f) ? -1.f : 1.f;
                dir = (V2(PITCH_W / 2 + side * R.frange(0.5f, 3.f) + R.frange(-3, 3) * acc, g.y) - p.pos).norm();
                vz = R.frange(-0.5f, 1.5f);   // tête piquée
            } else if (prog < 0.35f) {
                // dégagement de la tête : loin de son but, vers les côtés
                V2 tgt = fromTeamFrame(p.team, 0.55f, p.pos.x < PITCH_W / 2 ? 0.2f : 0.8f);
                dir = (tgt - p.pos).norm(); pw += 3; vz = R.frange(3.f, 5.f);
            } else dir = ((g - p.pos).norm() * 0.7f + V2(R.frange(-0.5f, 0.5f) * acc, 0)).norm();
            checkOffsideTouch(i);
            if (state != MS_PLAY) return;
            kickBall(i, dir, pw + (diving ? 3.f : 0.f), vz, false, false);
            if (diving) { p.state = PS_DIVE; p.st = 0; p.vel = dir * 4.f; say("Tête plongeante de " + playerName(i) + " !", 2.4f); }
            else { p.state = PS_HEAD; p.st = 0; p.z = 0.3f; p.vz = 2.0f + p.heading / 80.f; }
            ball.aftertouch = 0;
            return;
        }
    }
    // frappe puissante de loin ou à bout portant : le gardien la touche mais ne la retient pas toujours (elle file au fond)
    if (best >= 0 && pl[best].gk && best != penGk && b.lastTeam != pl[best].team && lastShooter >= 0 && lastShotAge < 1.5f && (lastShotDist > 20.f || lastShotDist < 9.f)) {
        MPlayer& g = pl[best];
        float rel = (b.vel - g.vel).len();
        float pf = (lastShotDist > 20.f ? 0.17f : 0.24f) * clampf((rel - 13.f) / 10.f, 0.f, 1.f) * clampf(1.25f - g.keep / 100.f, 0.3f, 1.f);
        if (R.chance(pf)) {
            g.cool = 0.6f;                                                      // il ne peut pas la reprendre tout de suite
            b.vel = rot(b.vel, R.frange(-0.08f, 0.08f)) * 0.88f;                 // légère déviation, le ballon continue
            playSfx(SFX_PARRY);
            say(lastShotDist > 20.f ? "La frappe de loin est trop puissante pour " + playerName(best) + " !" : "À bout portant, " + playerName(best) + " ne peut que l'effleurer !", 2.4f);
            best = -1;
        }
    }
    if (best >= 0) {
        MPlayer& p = pl[best];
        // hors-jeu : un partenaire de l'équipe en phase = participation ; un adversaire = jeu volontaire, parade ou déviation selon le geste
        if (p.team == off.team) { checkOffsideTouch(best); if (state != MS_PLAY) return; }
        statsTouch(best);
        float rel = (b.vel - p.vel).len();
        bool gkCatch = p.gk && inOwnBox(p.team, p.pos) && !(b.backpass && b.lastTeam == p.team);
        if (p.state == PS_SLIDE) {
            V2 d = p.slideDir;
            bool attackingSlide = !off.active || p.team == off.team;
            if (!attackingSlide) offsideOpponentTouch(best, OT_DEFLECTION);      // tacle réflexe : ne remet pas en jeu
            b.vel = d * 8.5f + V2(R.frange(-1.5f, 1.5f), R.frange(-1.5f, 1.5f));
            b.vz = 0.5f;
            b.lastTouch = best; b.lastTeam = p.team; b.backpass = false;
            p.touchedBallInSlide = true; if (!shootout) ps(best).tackle++;
            if (attackingSlide) armOffside(best);
            playSfx(SFX_TACKLE);
            if (p.human >= 0) rumbleFx(p.human, 2);
            return;
        }
        if (gkCatch) {
            float pc = 0.26f + p.keep / 215.f - std::max(0.f, rel - 8) / 20.f;
            if (b.lastTeam == p.team) pc = 1;
            pc = clampf(pc, 0.12f, 0.97f);
            if (R.chance(pc)) {
                checkOffsideTouch(best);          // prise de balle : jeu volontaire et maîtrisé du gardien
                if (state != MS_PLAY) return;
                ball.owner = best; b.vel = V2(); b.vz = 0; b.spin = 0; b.aftertouch = 0;
                b.lastTouch = best; b.lastTeam = p.team; b.backpass = false;
                possTeam = p.team;
                p.state = PS_GKHOLD; p.st = 0; p.vel = V2();
                if (rel > 4) { playSfx(SFX_CATCH); rumbleFx(-1 - p.team, 7); }
                if (rel > 18 && b.lastTeam != p.team) say("Belle prise de balle de " + playerName(best) + " !", 2.5f);
                p.pos.y = clampf(p.pos.y, 0.6f, PITCH_L - 0.6f);
                p.face = (goalCenter(p.team) - p.pos).norm();
            } else {
                // parade
                V2 n = (b.pos - p.pos).norm();
                if (n.len2() < 0.01f) n = V2(R.chance(0.5f) ? 1.f : -1.f, 0);
                float spd = std::max(4.f, b.vel.len() * 0.45f);
                V2 away;
                float side = b.pos.x < PITCH_W / 2 ? -1.f : 1.f;
                if (R.chance(0.5f)) away = V2(side * 1.2f, b.vel.norm().y * 0.5f).norm();   // détourné en corner
                else away = V2(n.x * 1.6f + R.frange(-0.5f, 0.5f), -b.vel.norm().y * 0.6f).norm();
                b.vel = away * spd; b.vz = R.frange(1, 4);
                b.lastTouch = best; b.lastTeam = p.team; b.backpass = false;
                p.cool = 0.5f;
                offsideOpponentTouch(best, OT_SAVE);
                playSfx(SFX_PARRY); rumbleFx(-1 - p.team, 7);
                playSfx(SFX_CROWD_OOH);        // une parade du gardien ne remet pas en jeu un attaquant hors-jeu
                if (p.team == 0 && !S.neutral) playSfx(SFX_CLAPS);   // le public applaudit son gardien
            }
            return;
        }
        if (!p.gk && rel > 9.f && rel <= 17.f && b.lastTeam >= 0) {
            // contrôle de balle : un ballon rapide peut échapper à un joueur à la technique moyenne
            float ctrl = (p.dribble * 0.6f + p.pass * 0.4f) / 100.f;
            float miss = clampf((rel - 9.f) * 0.045f - ctrl * 0.35f + (b.z > 0.3f ? 0.08f : 0.f) + 0.03f, 0.f, 0.45f) * (p.human >= 0 ? 0.6f : 1.f);
            if (R.chance(miss)) {
                checkOffsideTouch(best, false);   // contrôle manqué d'un adversaire : déviation, pas un jeu volontaire
                if (state != MS_PLAY) return;
                b.vel = b.vel * 0.3f + p.face * 2.5f + V2(R.frange(-1.5f, 1.5f), R.frange(-1.5f, 1.5f)); b.vz = R.frange(0.f, 1.5f);
                b.lastTouch = best; b.lastTeam = p.team; b.backpass = false;
                p.cool = 0.3f;
                return;
            }
        }
        if (rel > 17 && !p.gk) {
            // ballon trop fort : déviation (non délibérée : un défenseur qui dévie ne remet pas les attaquants en jeu)
            checkOffsideTouch(best, false);
            if (state != MS_PLAY) return;
            b.vel = b.vel * -0.25f + V2(R.frange(-3, 3), R.frange(-3, 3));
            b.lastTouch = best; b.lastTeam = p.team; b.backpass = false;
            p.cool = 0.3f;
            return;
        }
        if (p.gk && b.backpass && b.lastTeam == p.team && inOwnBox(p.team, p.pos) && comT <= 0) say("Passe en retrait : " + playerName(best) + " n'a pas le droit de la prendre à la main, il joue au pied.", 2.4f);
        takePossession(best);
    }
}

// ------------------------------------------------------------------ sorties
void Match::checkOut() {
    Ball& b = ball;
    if (state != MS_PLAY || b.inNet) return;
    int lt = b.lastTeam < 0 ? 0 : b.lastTeam;
    if (shootout) {
        // séance de tirs au but : seul un but compte, un ballon qui sort est simplement un tir manqué (pas de corner, ni de sortie de but)
        for (int g = 0; g < 2; g++) {
            bool out = g == 0 ? b.pos.y < -BALL_R : b.pos.y > PITCH_L + BALL_R;
            if (out && std::fabs(b.pos.x - PITCH_W / 2) < GOAL_W / 2 - BALL_R * 0.5f && b.z < GOAL_H) goalScored((attackDir[0] < 0) == (g == 0) ? 0 : 1);
        }
        return;
    }
    if (b.pos.x < -BALL_R || b.pos.x > PITCH_W + BALL_R) {
        playSfx(SFX_WHISTLE);
        V2 spot(clampf(b.pos.x, 0, PITCH_W), clampf(b.pos.y, 0.5f, PITCH_L - 0.5f));
        state = MS_STOP; stateT = 0.6f; nextSp = SP_THROWIN; nextSpTeam = 1 - lt; nextSpPos = spot;
        msg = "TOUCHE"; msg2 = ""; msgT = 1.0f;
        b.owner = -1;
        return;
    }
    for (int g = 0; g < 2; g++) {
        bool out = g == 0 ? b.pos.y < -BALL_R : b.pos.y > PITCH_L + BALL_R;
        if (!out) continue;
        // équipe qui attaque ce but
        int att = (attackDir[0] < 0) == (g == 0) ? 0 : 1;
        int def = 1 - att;
        if (std::fabs(b.pos.x - PITCH_W / 2) < GOAL_W / 2 - BALL_R * 0.5f && b.z < GOAL_H) {
            goalScored(att);
            return;
        }
        playSfx(SFX_WHISTLE);
        state = MS_STOP; stateT = 0.5f;
        if (lt == def) {
            nextSp = SP_CORNER; nextSpTeam = att;
            nextSpPos = V2(b.pos.x < PITCH_W / 2 ? 0.3f : PITCH_W - 0.3f, g == 0 ? 0.3f : PITCH_L - 0.3f);
            msg = "CORNER";
            say("Corner pour " + team(att).name + ".", 2.2f);
        } else {
            nextSp = SP_GOALKICK; nextSpTeam = def;
            nextSpPos = V2(PITCH_W / 2 + (b.pos.x < PITCH_W / 2 ? -1 : 1) * 6.0f, g == 0 ? SIX_L : PITCH_L - SIX_L);
            msg = "SORTIE DE BUT";
        }
        msg2 = ""; msgT = 1.0f;
        b.owner = -1;
        return;
    }
}

void Match::statsTouch(int i){
 if(i<0||i>=22||shootout)return;
 ps(i).touch++;
 if(pendingPass>=0&&i!=pendingPass){if(pl[i].team==pl[pendingPass].team){completedPasses[pl[i].team]++;ps(pendingPass).passOk++;}else ps(i).inter++;pendingPass=-1;}
 if(pendingShot>=0&&i!=pendingShot){int t=pl[pendingShot].team;if(pl[i].team!=t){
   if(pl[i].gk){V2 g=goalCenter(t);float dt=std::fabs(ball.vel.y)>.01f?(g.y-ball.pos.y)/ball.vel.y:-1;float xx=ball.pos.x+ball.vel.x*dt,zz=ball.z+ball.vz*dt-4.905f*dt*dt;
    if(dt>=0&&std::fabs(xx-PITCH_W/2)<GOAL_W/2&&zz<GOAL_H){onTarget[t]++;saves[pl[i].team]++;ps(i).save++;}}
   else {blockedShots[t]++;ps(i).block++;}
 }pendingShot=-1;}
}
// note sur 10 : buts, passes décisives, jeu (passes, récupérations, arrêts), résultat et discipline
float Match::playerRating(int t, int sq) const {
    const Team& T = team(t);
    int id = sq >= 0 && sq < (int)T.squad.size() ? T.squad[sq].id : -1;
    float r = 6.0f;
    for (auto& e : events) {
        if (e.pid == id && id >= 0) {
            if (e.type == 0 && e.team == t) r += e.pen ? 0.8f : 1.1f;
            else if (e.type == 3) r -= 0.9f;
            else if (e.type == 1 && e.team == t) r -= 0.3f;
            else if (e.type == 2 && e.team == t) r -= 1.6f;
        }
        if (e.aid == id && id >= 0 && e.type == 0 && e.team == t) r += 0.7f;
    }
    auto it = pst[t].find(sq);
    bool gk = false, def = false;
    if (it != pst[t].end()) {
        const PStat& s = it->second;
        gk = s.gk; def = s.def;
        r += std::min(1.0f, s.passOk * 0.035f) + s.tackle * 0.15f + s.inter * 0.1f + s.block * 0.25f + s.save * 0.35f + std::min(0.4f, s.touch * 0.006f) + s.shot * 0.05f;
    }
    int gf = score[t], ga = score[1 - t];
    r += gf > ga ? 0.4f : gf < ga ? -0.3f : 0.f;
    if (ga == 0 && gk) r += 0.8f; else if (ga == 0 && def) r += 0.4f;
    if (gk) r -= 0.25f * ga;
    // petite variation propre au joueur (forme du jour), stable pour un même match
    uint32_t h = (uint32_t)(id * 2654435761u) ^ (uint32_t)(S.home * 40503u + S.away * 97u);
    h ^= h >> 15; h *= 0x2c1b3c6d; h ^= h >> 12;
    r += ((h % 1000) / 1000.f - 0.5f) * 0.5f;
    return clampf(r, 3.f, 10.f);
}
int Match::manOfMatch(int& bestTeam) const {
    int best = -1; float br = -1; bestTeam = -1;
    for (int t = 0; t < 2; t++) {
        std::vector<int> cand;
        for (auto& kv : pst[t]) cand.push_back(kv.first);
        for (int i = t * 11; i < t * 11 + 11; i++) if (pl[i].onPitch || pl[i].sentOff) cand.push_back(pl[i].squad);
        for (int sq : cand) {
            float r = playerRating(t, sq) + (score[t] > score[1 - t] ? 0.05f : 0.f);   // à note égale, le vainqueur
            if (r > br) { br = r; best = sq; bestTeam = t; }
        }
    }
    return best;
}
void Match::statsGoal(int t){if(!shootout&&pendingShot>=0&&pl[pendingShot].team==t)onTarget[t]++;pendingShot=pendingPass=-1;}
void Match::statsWoodwork(){if(!shootout&&pendingShot>=0&&!shotHitWoodwork){woodwork[pl[pendingShot].team]++;shotHitWoodwork=true;}}
void Match::trackTouch() {
    int lt = ball.lastTouch;
    statsTouch(lt);
    if (lt == touchSeen) return;
    if (touchSeen >= 0 && lt >= 0 && pl[touchSeen].team == pl[lt].team) assistCand = touchSeen;
    else assistCand = -1;
    touchSeen = lt;
}

void Match::goalScored(int t) {
    statsGoal(t);
    if (shootout) {
        ball.inNet = true;
        playSfx(SFX_NET);
        return;
    }
    if (S.training) {
        bool ok = (S.training == 5) ? false : t == 0;
        if (ok) trainGoals++;
        msg = S.training == 5 ? "BUT..." : "BUT !"; msg2 = ""; msgT = 1.5f;
        ball.inNet = true; state = MS_GOAL; stateT = 0;
        playSfx(SFX_NET);
        playSfx(ok ? SFX_GOAL : SFX_CROWD_OOH);
        if (ok) for (int i = t * 11; i < t * 11 + 11; i++) if (pl[i].onPitch && !pl[i].gk && (pl[i].pos - ball.pos).len() < 25) { pl[i].state = PS_CELEB; pl[i].st = 0; }
        return;
    }
    // hors-jeu : drapeau en attente, ou joueur signalé qui gênait le gardien
    if (sp != SP_PENALTY && offsideGoalCheck(t)) return;
    trackTouch();
    score[t]++;
    lastScorerTeam = t;
    MatchEvent e; e.type = 0; e.team = t; e.minute = std::min(clock, periodEnd + added);
    int sc = ball.lastTouch;
    auto pidOf = [&](int i) { const Team& T = team(pl[i].team); return pl[i].squad >= 0 && pl[i].squad < (int)T.squad.size() ? T.squad[pl[i].squad].id : 0; };
    // tir cadré dévié ou touché par le gardien (penalty compris) : le but revient au tireur
    if (sc >= 0 && pl[sc].team != t && lastShooter >= 0 && pl[lastShooter].team == t && (lastShotAge < 3.f || (sp == SP_PENALTY && lastShotAge < 6.f))) sc = lastShooter;
    if (sc >= 0 && pl[sc].team != t) { e.type = 3; e.player = playerName(sc); e.pid = pidOf(sc); }
    else if (sc >= 0) {
        e.player = playerName(sc); e.pid = pidOf(sc);
        e.pen = sp == SP_PENALTY;
        if (!e.pen && assistCand >= 0 && assistCand != sc && pl[assistCand].team == t) {
            e.assist = playerName(assistCand); e.aid = pidOf(assistCand);
            Team& T = g_world.teams[t == 0 ? S.home : S.away];
            if (pl[assistCand].squad < (int)T.squad.size()) T.squad[pl[assistCand].squad].assists++;
        }
    }
    touchSeen = -1; assistCand = -1;
    if (sp == SP_PENALTY && stateT < 3) {}
    events.push_back(e);
    if (sc >= 0 && e.type == 0) {
        Team& T = g_world.teams[t == 0 ? S.home : S.away];
        if (pl[sc].squad < (int)T.squad.size()) T.squad[pl[sc].squad].goals++;
    }
    msg = "BUT !";
    msg2 = e.player + (e.type == 3 ? " (csc)" : "");
    {
        static const char* G1[] = { "BUUUUT ! %s trouve le chemin des filets !", "Et c'est le but ! Superbe réalisation de %s !", "%s fait trembler les filets ! Le stade explose !", "Quel but de %s !" };
        std::string sc = fmt(" %s %d - %d %s.", team(0).shortName.c_str(), score[0], score[1], team(1).shortName.c_str());
        if (e.type == 3) say("Contre son camp de " + e.player + " ! Quelle malchance !" + sc, 4.5f, true);
        else say(fmt(G1[R.range(0, 3)], e.player.c_str()) + (e.assist.empty() ? std::string("") : " Sur une passe de " + e.assist + ".") + sc, 4.5f, true);
    }
    msgT = 3.5f;
    ball.inNet = true;
    state = MS_GOAL; stateT = 0;
    addAnger(-1, 0);
    for (int i = (1 - t) * 11; i < (1 - t) * 11 + 11; i++) addAnger(i, 0.05f);
    if (S.goldenGoal && period >= 2) { msg = "BUT EN OR !"; aet = true; say("But en or ! " + team(t).name + " l'emporte en prolongation !", 4.f, true); }
    kickoffTeam = 1 - t;
    offsideReset(OR_VALID_GOAL);
    playSfx(SFX_NET);
    playSfx(SFX_GOAL);
    rumbleFx(-1 - t, 5); rumbleFx(-1 - (1 - t), 6);              // RB_GOAL_FOR / RB_GOAL_AGAINST
    // célébration : le buteur court vers le poteau de corner ou vers le public, ses coéquipiers le rejoignent
    celebScorer = (sc >= 0 && pl[sc].team == t) ? sc : -1;
    // 0 poteau de corner, 1 glissade à genoux, 2 vers le public, 3 l'avion, 4 salto, 5 la pile humaine, 6 la danse,
    // 7 course vers le banc, 8 le petit train, 9 saut et réception bras écartés, 10 le bébé, 11 « chut » au public adverse,
    // 12 à genoux, doigts vers le ciel, 13 cœur avec les mains vers les supporters
    setCelebration(R.range(0, 13));
    for (int i = t * 11; i < t * 11 + 11; i++) if (pl[i].onPitch && !pl[i].gk && i != celebScorer) { pl[i].state = PS_NORMAL; pl[i].st = 0; }
}

void Match::setCelebration(int type) {
    celebType = type; celebSfx = false;
    if (celebScorer < 0) return;
    int t = pl[celebScorer].team;
    V2 g = goalCenter(t);
    float gy = g.y == 0 ? 1.5f : PITCH_L - 1.5f;
    V2 sp = pl[celebScorer].pos;
    // « chut » : seulement vers le public adverse le plus proche (kop au nord, parcage visiteur au sud)
    if (celebType == 11 && !((t == 1 && g.y == 0) || (t == 0 && g.y != 0)) ) celebType = 12;
    if (celebType == 7 && (sp - V2(0.f, PITCH_L / 2 + (t ? 6.f : -6.f))).len() > 32.f) celebType = 0;   // banc trop loin : poteau de corner
    V2 corner(sp.x < PITCH_W / 2 ? 2.f : PITCH_W - 2.f, sp.y + (g.y == 0 ? -1.f : 1.f) * std::min(12.f, std::fabs(sp.y - gy) * 0.5f));
    switch (celebType) {
    case 2: case 6: case 13: celebTarget = V2(PITCH_W / 2 + R.frange(-8, 8), gy); break;
    case 7: celebTarget = V2(-0.2f, PITCH_L / 2 + (t ? 6.f : -6.f)); break;                       // le banc de son équipe
    case 11: celebTarget = t == 0 ? V2(PITCH_W - 9.f, PITCH_L - 1.f) : V2(PITCH_W / 2 + R.frange(-6, 6), 1.f); break;
    case 12: celebTarget = V2(corner.x < PITCH_W / 2 ? 7.f : PITCH_W - 7.f, corner.y); break;
    default: celebTarget = corner; break;
    }
    pl[celebScorer].state = PS_CELEB; pl[celebScorer].st = 0;
}

// ------------------------------------------------------------------ fautes et cartons
void Match::sendOff(int i) {
    MPlayer& p = pl[i];
    {   // l'expulsé quitte le terrain en marchant
        const Team& T0 = team(p.team);
        Walker w; w.pos = p.pos; w.target = V2(-3.2f, PITCH_L / 2 + (p.team ? 1.f : -1.f)); w.team = p.team;   // direction le tunnel des vestiaires w.gk = p.gk; w.gkShirt = gkShirt[p.team]; w.anim = 0;
        w.skin = p.squad >= 0 && p.squad < (int)T0.squad.size() ? T0.squad[p.squad].skin : 0; w.hair = p.squad >= 0 && p.squad < (int)T0.squad.size() ? T0.squad[p.squad].hair : 0;
        walkers.push_back(w);
    }
    p.sentOff = true; p.onPitch = false; p.state = PS_OFF;
    updateCaptain(p.team);
    if (ball.owner == i) ball.owner = -1;
    int t = p.team;
    if (p.gk) {
        // un joueur de champ devient gardien (ou remplaçant si possible)
        int gkBench = -1;
        const Team& T = team(t);
        for (int k = 0; k < (int)bench[t].size(); k++) if (T.squad[bench[t][k]].pos == POS_GK) gkBench = k;
        int outfield = -1;
        for (int j = t * 11 + 10; j > t * 11; j--) if (pl[j].onPitch) { outfield = j; break; }
        if (outfield < 0) return;
        MPlayer np = pl[outfield];
        pl[outfield].onPitch = false; pl[outfield].state = PS_OFF;
        np.gk = true; np.slot = 0; np.role = 0;np.detailedPosition=DP_GB;np.tactic=SlotTactic();np.effects=RoleEffects(); np.onPitch = true; np.state = PS_NORMAL;
        if (gkBench >= 0 && subsLeft[t] > 0) {
            np.squad = bench[t][gkBench];
            loadAttrs(np, T.squad[np.squad], supporterPlayerMultiplier(S,t,T.squad[np.squad].age)*(S.personalityActive?personalityMatchMultiplier(g_career,T.squad[np.squad],t==0?S.home:S.away,S.personalityKey,S.personalityImportant,S.supporterPressure):1.f), t == 0 ? S.home : S.away,S.rules);
            bench[t].erase(bench[t].begin() + gkBench);
            subsLeft[t]--;
        } else np.keep = std::max(20.f, np.tackle * 0.5f);
        np.pos = ownGoal(t) + V2(0, attackDir[t] < 0 ? -1.f : 1.f);
        MPlayer old = pl[i];
        pl[i] = np;
        pl[i].sentOff = false;
        (void)old;
    }
}

void Match::foul(int off, int vic, bool fromBehind) {
    MPlayer& o = pl[off]; MPlayer& v = pl[vic];
    fouls[o.team]++;
    if (v.human >= 0) rumbleFx(v.human, 3);                        // RB_FOULED
    v.state = PS_DOWN; v.st = 0; v.vel = o.slideDir * 3;
    if (ball.owner == vic) ball.owner = -1;
    float sev = refSeverity() / 60.f * (S.personalityActive?o.personalityFoul:1.f);
    float red = (fromBehind ? 0.01f : 0.002f) * sev, yel = (fromBehind ? 0.5f : 0.28f) * sev;
    // dernier défenseur
    float vp = progress(v.team, v.pos);
    bool lastMan = vp > 0.7f;
    if (lastMan) for (int j = o.team * 11; j < o.team * 11 + 11; j++)
        if (pl[j].onPitch && !pl[j].gk && j != off && progress(v.team, pl[j].pos) > vp) lastMan = false;
    if (lastMan) red += inOwnBox(o.team, v.pos) ? 0.015f : (fromBehind ? 0.09f : 0.04f);   // occasion nette annihilée (dans la surface : penalty, rarement rouge)
    float r = R.f();
    cardShow = 0;
    msg = "FAUTE"; msg2 = playerName(off); msgT = 2.2f;
    say(fromBehind ? "Tacle par derrière de " + playerName(off) + " sur " + playerName(vic) + " !" : "Faute de " + playerName(off) + ".", 2.6f);
    lastFoulOff = off; lastFoulVic = vic; lastFoulT = 0;
    addAnger(vic, fromBehind ? 0.32f : 0.16f);
    addAnger(off, 0.05f);
    bool fightRisk = !pl[vic].gk && !humanSide(v.team) && pl[vic].anger >= 0.6f && R.chance(0.5f);
    // la sanction n'est pas immédiate : l'arbitre va vers le joueur à l'arrêt de jeu (suspense)
    if (r < red + yel && pendCardOff < 0) { pendCardOff = off; pendCardType = r < red ? 2 : 1; pendCardT = 0; pendCardSaid = false; }
    if (!v.injured && R.chance(0.04f)) {
        // blessure : signalée au public, le joueur est remplacé au prochain arrêt de jeu
        v.injured = true; v.speed *= 0.8f;
        MatchEvent e; e.type = 4; e.team = v.team; e.minute = clock; e.player = playerName(vic);
        const Team& TV = team(v.team);
        if (v.squad >= 0 && v.squad < (int)TV.squad.size()) e.pid = TV.squad[v.squad].id;
        events.push_back(e);
        say(playerName(vic) + " reste au sol... il semble blessé et devra sortir.", 3.5f, true);
    }
    V2 spot(clampf(v.pos.x, 0.5f, PITCH_W - 0.5f), clampf(v.pos.y, 0.5f, PITCH_L - 0.5f));
    // règle de l'avantage : l'équipe victime garde le ballon hors de la surface adverse
    float adv = S.referee >= 0 && S.referee < NUM_REFEREES ? REFEREES[S.referee].advantage / 100.f : 0.6f;
    if (!inOwnBox(o.team, spot) && ball.owner >= 0 && pl[ball.owner].team == v.team && ball.owner != vic && state == MS_PLAY && R.chance(0.35f + adv * 0.6f)) {
        msg = "AVANTAGE"; msg2 = "";
        msgT = 1.8f;
        return;
    }
    playSfx(SFX_WHISTLE);
    // réaction du public local : sifflets contre l'arbitre, ou huées contre l'auteur d'une faute dure
    if (!S.neutral && !S.training) { if (o.team == 0) playSfx(SFX_FANS_WHISTLE); else if (fromBehind || inOwnBox(o.team, spot)) playSfx(SFX_BOO); }
    state = MS_STOP; stateT = 0;
    ball.vel = V2(); ball.vz = 0;
    if (inOwnBox(o.team, spot)) {
        nextSp = SP_PENALTY; nextSpTeam = v.team;
        V2 g = ownGoal(o.team);
        nextSpPos = V2(PITCH_W / 2, g.y == 0 ? 11.f : PITCH_L - 11.f);
        msg = "PENALTY !";
    } else { nextSp = SP_FREEKICK; nextSpTeam = v.team; nextSpPos = spot; }
    if (offPend >= 0) offsideWhistle();     // le hors-jeu (antérieur) prime sur la faute
    offsideReset(OR_FOUL_STOPPAGE);
    if (fightRisk && o.onPitch && v.onPitch) { std::string m0 = msg; escalate(vic, off, false); if (m0 == "PENALTY !" && fightT > 0) { msg2 = msg2 + " - PENALTY"; } }
}

// ------------------------------------------------------------------ remplacements
void Match::substitute(int t, int slot, int bi) {
    if (subsLeft[t] <= 0 || bi < 0 || bi >= (int)bench[t].size()) return;
    MPlayer& p = pl[t * 11 + slot];
    if (!p.onPitch || p.sentOff) return;
    for (auto& q : pendSubs) if (q.team == t && q.slot == slot) return;       // déjà prévu
    int incoming = bench[t][bi];
    bench[t].erase(bench[t].begin() + bi);
    subsLeft[t]--;
    if (state == MS_BREAK || state == MS_INTRO || S.training) { doSub(t, slot, incoming, false); return; }
    // remplacement effectué au prochain arrêt de jeu, avec le panneau du 4e arbitre
    pendSubs.push_back({ t, slot, incoming });
    if (humanSide(t)) { msg = "REMPLACEMENT PRÉVU"; msg2 = "au prochain arrêt de jeu"; msgT = 1.6f; }
}

void Match::doSub(int t, int slot, int incoming, bool anim) {
    MPlayer& p = pl[t * 11 + slot];
    if (!p.onPitch || p.sentOff) return;
    const Team& T = team(t);
    int outSq = p.squad;
    if(S.personalityActive&&!p.injured&&clock<70&&outSq>=0&&outSq<(int)T.squad.size())g_career.personalityDecision(g_world.teams[t==0?S.home:S.away].squad[outSq],t==0?S.home:S.away,1);
    if (anim) {
        // le joueur remplacé sort en marchant vers la ligne de touche, le remplaçant entre
        Walker w; w.pos = p.pos; w.target = V2(-1.8f, PITCH_L / 2 + (t ? 2.5f : -2.5f)); w.team = t; w.gk = p.gk; w.gkShirt = gkShirt[t]; w.anim = 0;
        w.skin = outSq >= 0 && outSq < (int)T.squad.size() ? T.squad[outSq].skin : 0; w.hair = outSq >= 0 && outSq < (int)T.squad.size() ? T.squad[outSq].hair : 0;
        walkers.push_back(w);
        if (!S.training) playSfx(SFX_CLAPS);       // le joueur qui sort est applaudi
        subBoardT = 3.4f; subBoardTeam = t;
        subBoardOut = outSq >= 0 && outSq < (int)T.squad.size() ? T.squad[outSq].num : 0; subBoardIn = T.squad[incoming].num;
        subBoardOutName = outSq >= 0 && outSq < (int)T.squad.size() ? T.squad[outSq].name : ""; subBoardInName = T.squad[incoming].name;
        V2 hold = p.pos;
        p.pos = V2(-0.8f, PITCH_L / 2 + (t ? -2.5f : 2.5f));
        p.target = hold;
    }
    if (S.rolling && outSq >= 0 && !p.injured) bench[t].push_back(outSq);   // remplacement « libre » : il pourra revenir
    p.squad = incoming;
    loadAttrs(p, T.squad[incoming], (humanSide(t) ? 1.0f : (S.difficulty == 0 ? 0.86f : S.difficulty == 2 ? 1.1f : 0.98f))*supporterPlayerMultiplier(S,t,T.squad[incoming].age)*(S.personalityActive?personalityMatchMultiplier(g_career,T.squad[incoming],t==0?S.home:S.away,S.personalityKey,S.personalityImportant,S.supporterPressure):1.f), t == 0 ? S.home : S.away,S.rules);
    p.yellow = 0; p.injured = false; p.state = PS_NORMAL; p.vel = V2();
    subsPeriod++;
    updateCaptain(t);
    msg = "REMPLACEMENT"; msg2 = T.squad[incoming].name + " (" + team(t).shortName + ")"; msgT = 2.2f;
    if (anim) say(fmt("Changement pour %s : %s cède sa place à %s.", team(t).shortName.c_str(), subBoardOutName.c_str(), subBoardInName.c_str()), 3.f);
}

// cartons en attente, remplacements, joueurs qui quittent le terrain
void Match::updatePending(float dt) {
    for (size_t k = 0; k < walkers.size();) {
        Walker& w = walkers[k];
        V2 d = w.target - w.pos;
        if (d.len() < 0.3f) { walkers.erase(walkers.begin() + k); continue; }
        w.pos += d.norm() * std::min(d.len(), 2.2f * dt);
        w.anim += 2.2f * dt;
        k++;
    }
    if (subBoardT > 0) subBoardT -= dt;
    if (pendCardOff >= 0 && !pl[pendCardOff].onPitch) pendCardOff = -1;
    bool stopped = state == MS_STOP || state == MS_GOAL;
    if (pendCardOff >= 0 && stopped && fightT <= 0) {
        pendCardT += dt;
        if (!pendCardSaid && pendCardT > 0.3f) {
            pendCardSaid = true;
            static const char* SUS[] = { "L'arbitre se dirige vers %s... Va-t-il sortir un carton ?", "%s est appelé par l'arbitre...", "L'arbitre met la main à la poche... %s craint le pire." };
            say(fmt(SUS[R.range(0, 2)], playerName(pendCardOff).c_str()), 2.4f, true);
            msg = "L'ARBITRE S'APPROCHE..."; msg2 = playerName(pendCardOff); msgT = 1.6f;
        }
        float dist = (refPos - pl[pendCardOff].pos).len();
        if ((pendCardT > 1.8f && dist < 2.6f) || pendCardT > 3.2f) {
            int off = pendCardOff; pendCardOff = -1;
            giveCard(off, pendCardType);
            if (state == MS_STOP) stateT = std::min(stateT, 0.2f);
        }
    }
    if (state == MS_STOP && pendCardOff < 0 && subBoardT <= 0 && !pendSubs.empty()) {
        PendSub q = pendSubs.front(); pendSubs.erase(pendSubs.begin());
        doSub(q.team, q.slot, q.incoming, true);
        stateT = 0;
    }
}

void Match::autoSubs(int t) {
    bool ai = !humanSide(t) || S.delegSubs;     // ordinateur ou adjoint (délégation / Full Manager)
    const Team& T = team(t);
    auto benchPick = [&](int want) {
        for (int k = 0; k < (int)bench[t].size(); k++) if (T.squad[bench[t][k]].pos == want) return k;
        for (int k = 0; k < (int)bench[t].size(); k++) if (T.squad[bench[t][k]].pos != POS_GK) return k;
        return -1;
    };
    for (int s = 1; s < 11 && subsLeft[t] > 0; s++) {
        MPlayer& p = pl[t * 11 + s];
        if (!p.onPitch) continue;
        if (medicFor == t * 11 + s && medicPhase < 2) continue;        // pendant les soins
        bool need = p.injured || (ai && p.stamina < 0.62f && clock > 58 && R.chance(0.5f));
        if (!need) continue;
        // remplaçant du même poste de préférence
        int want = p.role == 1 ? POS_DF : p.role == 2 ? POS_MF : POS_FW;
        int bi = benchPick(want);
        if (bi >= 0) substitute(t, s, bi);
    }
    // blessé sans remplaçant possible : il quitte le terrain (l'équipe joue à 10)
    for (int s = 1; s < 11; s++) {
        MPlayer& p = pl[t * 11 + s];
        if (!p.onPitch || !p.injured || S.training) continue;
        if (medicFor == t * 11 + s && medicPhase < 2) continue;
        bool pending = false; for (auto& q : pendSubs) if (q.team == t && q.slot == s) pending = true;
        if (pending || (subsLeft[t] > 0 && !bench[t].empty())) continue;
        Walker w; w.pos = p.pos; w.target = V2(-3.2f, PITCH_L / 2 + (t ? 1.f : -1.f)); w.team = t; w.gk = false; w.gkShirt = gkShirt[t]; w.anim = 0;
        w.skin = p.squad >= 0 && p.squad < (int)T.squad.size() ? T.squad[p.squad].skin : 0; w.hair = p.squad >= 0 && p.squad < (int)T.squad.size() ? T.squad[p.squad].hair : 0;
        walkers.push_back(w);
        say(playerName(t * 11 + s) + " ne peut pas continuer et aucun changement n'est possible : " + team(t).shortName + fmt(" termine à %d.", teamPlayersCount(t) - 1), 3.5f, true);
        p.onPitch = false; p.state = PS_OFF;
        if (ball.owner == t * 11 + s) ball.owner = -1;
        updateCaptain(t);
        checkAbandon();
    }
    // l'adjoint / l'ordinateur force le destin : menés après l'heure de jeu, un attaquant de plus
    if (ai && !lateSubDone[t] && subsLeft[t] > 0 && clock > 62) {
        int diff = score[t] - score[1 - t];
        if (S.hasFirstLeg) diff = (score[t] + (t == 0 ? S.aggHome : S.aggAway)) - (score[1 - t] + (t == 0 ? S.aggAway : S.aggHome));
        if (diff < 0 || (diff > 0 && clock > 78)) {
            lateSubDone[t] = true;
            int outS = -1; float worst = 9;
            for (int s = 1; s < 11; s++) {
                const MPlayer& p = pl[t * 11 + s];
                if (!p.onPitch) continue;
                if (diff < 0 ? p.role == 1 : p.role == 3) { if (p.stamina < worst) { worst = p.stamina; outS = s; } }
            }
            int bi = benchPick(diff < 0 ? POS_FW : POS_DF);
            if (outS >= 0 && bi >= 0) substitute(t, outS, bi);
        }
    }
}

// ------------------------------------------------------------------ coups de pied arrêtés
void Match::beginSetPiece(int type, int t, V2 spot) {
    penGk = -1;
    pendingPass=pendingShot=-1;
    if(!shootout&&!S.training&&t>=0&&t<2){if(type==SP_THROWIN)throws[t]++;if(type==SP_FREEKICK||type==SP_INDIRECT)freeKicks[t]++;if(type==SP_GOALKICK)goalKicks[t]++;if(type==SP_PENALTY)penalties[t]++;}
    if (type == SP_CORNER && !S.training && t >= 0 && t < 2) corners[t]++;
    state = MS_SETPIECE; stateT = 0; autoSubDone = false;
    sp = type; spTeam = t; spPos = spot; spT = 0; spReady = false; spCurl = 0;
    ball = Ball();
    ball.pos = spot;
    ball.lastTeam = t;
    possTeam = t;              // l'équipe qui joue le coup de pied arrêté a le ballon (placement des défenseurs adverses)
    offsideReset(OR_SET_PIECE);
    // tireur
    int k = -1;
    auto nearest = [&](bool allowGk) {
        int best = -1; float bd = 1e9;
        for (int i = t * 11; i < t * 11 + 11; i++) {
            if (!pl[i].onPitch || (pl[i].gk && !allowGk)) continue;
            float d = (pl[i].pos - spot).len();
            if (d < bd) { bd = d; best = i; }
        }
        return best;
    };
    auto bestShooter = [&]() {
        int best = -1; float bv = -1;
        for (int i = t * 11; i < t * 11 + 11; i++) if (pl[i].onPitch && !pl[i].gk && pl[i].shoot > bv) { bv = pl[i].shoot; best = i; }
        return best;
    };
    switch (type) {
    case SP_KICKOFF: {
        float bv = -1;
        for (int i = t * 11 + 1; i < t * 11 + 11; i++) if (pl[i].onPitch && pl[i].role == 3 && pl[i].shoot > bv) { bv = pl[i].shoot; k = i; }
        if (k < 0) k = nearest(false);
        break;
    }
    case SP_GOALKICK: k = t * 11; if (!pl[k].onPitch) k = nearest(false); break;
    case SP_PENALTY: k = bestShooter(); break;
    case SP_SHOOTOUT: k = shootKicker; break;
    default:
        k = nearest(false);
        // coup franc direct près du but : le spécialiste (tir + passe) s'en charge
        if (type == SP_FREEKICK && (goalCenter(t) - spot).len() < 32.f) {
            float bv = -1;
            for (int i = t * 11 + 1; i < t * 11 + 11; i++) if (pl[i].onPitch) { float v = pl[i].shoot * 0.6f + pl[i].pass * 0.4f; if (v > bv) { bv = v; k = i; } }
        }
        // coup franc dans sa propre surface : c'est le gardien qui le frappe
        if ((type == SP_FREEKICK || type == SP_INDIRECT) && inOwnBox(t, spot))
            for (int i = t * 11; i < t * 11 + 11; i++) if (pl[i].onPitch && pl[i].gk) { k = i; break; }
        break;
    }
    spKicker = k;
    V2 g = goalCenter(t);
    spAim = (g - spot).norm();
    if (type == SP_THROWIN) spAim = V2(spot.x < 1 ? 1.f : -1.f, 0);
    if (type == SP_CORNER) spAim = (V2(PITCH_W / 2, g.y) - spot).norm();
    for (auto& p : pl) { p.charging = false; p.charge = 0; p.cool = 0; if (p.onPitch && p.state != PS_CELEB) p.state = PS_NORMAL; if (p.state == PS_CELEB) p.state = PS_NORMAL; p.vel = V2(); p.z = 0; }
    placeForSetPiece();
}

void Match::placeForSetPiece() {
    int t = spTeam, ot = 1 - t;
    V2 g = goalCenter(t);             // but attaqué par l'équipe qui tire
    V2 og = ownGoal(t);
    // cibles par défaut : formation autour du ballon
    for (int i = 0; i < 22; i++) {
        MPlayer& p = pl[i];
        if (!p.onPitch) continue;
        if (p.gk) {
            V2 gg = ownGoal(p.team);
            p.target = gg + V2(0, gg.y == 0 ? 1.0f : -1.0f);
        } else {
            V2 tg; formationTarget(i, tg);
            p.target = tg;
        }
    }
    auto keepAway = [&](int team, V2 c, float r) {
        for (int i = team * 11; i < team * 11 + 11; i++) {
            MPlayer& p = pl[i];
            if (!p.onPitch || i == spKicker) continue;
            V2 d = p.target - c;
            if (d.len() < r) { V2 n = d.len2() > 0.01f ? d.norm() : (og - c).norm(); p.target = c + n * r; }
        }
    };
    switch (sp) {
    case SP_KICKOFF:
        for (int i = 0; i < 22; i++) {
            MPlayer& p = pl[i];
            if (!p.onPitch || p.gk) continue;
            const Formation& F = FORMATIONS[formation[p.team]];
            float u = F.y[p.slot - 1] * 0.62f, v = F.x[p.slot - 1];
            p.target = fromTeamFrame(p.team, std::min(u, 0.46f), v);
        }
        keepAway(ot, spPos, 9.6f);
        if (spKicker >= 0) {
            pl[spKicker].target = spPos + V2(0.3f, attackDir[t] < 0 ? 0.6f : -0.6f);
            // partenaire à côté
            int mate = -1; float bd = 1e9;
            for (int i = t * 11 + 1; i < t * 11 + 11; i++) if (i != spKicker && pl[i].onPitch) { float d = (pl[i].target - spPos).len(); if (d < bd) { bd = d; mate = i; } }
            if (mate >= 0) pl[mate].target = spPos + V2(-4.0f, attackDir[t] < 0 ? 1.0f : -1.0f);
        }
        break;
    case SP_CORNER: {
        int n = 0;
        static const float AX[] = { -3, 3, 0, -6, 6, 0 }, AY[] = { 6, 6, 11, 9, 9, 16 };
        float sgn = g.y == 0 ? 1 : -1;
        for (int i = t * 11 + 1; i < t * 11 + 11; i++) {
            if (!pl[i].onPitch || i == spKicker) continue;
            if (pl[i].role >= 2 && n < 6) { pl[i].target = V2(PITCH_W / 2 + AX[n] + R.frange(-1, 1), g.y + sgn * AY[n]); n++; }
        }
        int m = 0;
        for (int i = ot * 11 + 1; i < ot * 11 + 11; i++) {
            if (!pl[i].onPitch) continue;
            if (m < 7) { pl[i].target = V2(PITCH_W / 2 + (m % 4 - 1.5f) * 3.5f, g.y + sgn * (4 + (m / 4) * 5)); m++; }
        }
        keepAway(ot, spPos, 9.2f);
        break;
    }
    case SP_GOALKICK:
        for (int i = ot * 11; i < ot * 11 + 11; i++) {
            MPlayer& p = pl[i];
            if (!p.onPitch || p.gk) continue;
            if (inOwnBox(t, p.target)) p.target.y = og.y + (og.y == 0 ? BOX_L + 2 : -(BOX_L + 2));
        }
        break;
    case SP_THROWIN:
        keepAway(ot, spPos, 2.5f);
        break;
    case SP_FREEKICK: case SP_INDIRECT: {
        keepAway(ot, spPos, 9.3f);
        float dist = (g - spPos).len();
        float prog = progress(t, spPos);
        float sg = g.y == 0 ? 1.f : -1.f;
        if (prog > 0.45f && (dist >= 34 || sp == SP_INDIRECT)) {
            // coup franc lointain / indirect : attaquants au bord de la surface pour le ballon aérien, défense en ligne
            int n = 0;
            for (int i = t * 11 + 1; i < t * 11 + 11; i++) {
                if (!pl[i].onPitch || i == spKicker || pl[i].role < 2) continue;
                if (n < 5) { pl[i].target = V2(PITCH_W / 2 + (n - 2) * 5.f, g.y + sg * (BOX_L + 1.5f)); n++; }
            }
            for (int i = ot * 11 + 1; i < ot * 11 + 11; i++) if (pl[i].onPitch && pl[i].role == 1) pl[i].target.y = g.y + sg * (BOX_L - 1.f);
            markAttackers(ot, g, 30.f);
            keepAway(ot, spPos, 9.3f);
        }
        if (dist < 34 && sp == SP_FREEKICK) {
            // mur
            int wallN = dist < 22 ? 5 : dist < 28 ? 4 : 3;
            V2 dir = (g - spPos).norm();
            V2 c = spPos + dir * 9.15f;
            V2 side = dir.perp();
            int m = 0;
            for (int i = ot * 11 + 1; i < ot * 11 + 11 && m < wallN; i++) {
                if (!pl[i].onPitch) continue;
                pl[i].target = c + side * ((m - (wallN - 1) / 2.f) * 0.65f);
                m++;
            }
            // attaquants : point de penalty, premier et second poteau, un joueur au bout du mur (masque le gardien)
            static const float FX[5] = { 0.f, -4.5f, 4.5f, -1.5f, 6.5f }, FY[5] = { 11.f, 7.f, 7.f, 14.f, 13.f };
            int n = 0;
            for (int i = t * 11 + 1; i < t * 11 + 11; i++) {
                if (!pl[i].onPitch || i == spKicker || pl[i].role < 2) continue;
                if (n < 4) { pl[i].target = V2(PITCH_W / 2 + FX[n], g.y + sg * FY[n]); n++; }
                else if (n == 4) { pl[i].target = c + side * ((wallN / 2.f + 0.6f) * 0.65f) + dir * 0.3f; n++; }
            }
            markAttackers(ot, g, 20.f);
            // le mur reste en place malgré le marquage
            m = 0;
            for (int i = ot * 11 + 1; i < ot * 11 + 11 && m < wallN; i++) {
                if (!pl[i].onPitch) continue;
                pl[i].target = c + side * ((m - (wallN - 1) / 2.f) * 0.65f);
                m++;
            }
        }
        break;
    }
    case SP_PENALTY: case SP_SHOOTOUT: {
        for (int i = 0; i < 22; i++) {
            MPlayer& p = pl[i];
            if (!p.onPitch || i == spKicker) continue;
            bool defGk = p.gk && p.team == ot;
            if (defGk) { p.target = V2(PITCH_W / 2, g.y); continue; }
            if (sp == SP_SHOOTOUT) {
                // tous au rond central
                p.target = V2(PITCH_W / 2 + ((i % 11) - 5) * 1.6f, PITCH_L / 2 + (p.team == 0 ? -1.5f : 1.5f));
                if (p.gk) p.target = V2(PITCH_W / 2 + (p.team == 0 ? -16.f : 16.f), g.y + (g.y == 0 ? 14.f : -14.f));
                continue;
            }
            if (p.gk) continue;
            float edge = g.y == 0 ? BOX_L + 2.5f : PITCH_L - BOX_L - 2.5f;
            if (std::fabs(p.target.y - g.y) < BOX_L + 2) p.target.y = edge + (g.y == 0 ? R.frange(0, 6) : -R.frange(0, 6));
        }
        break;
    }
    }
    if (spKicker >= 0 && sp != SP_KICKOFF) {
        V2 back = spAim * -0.6f;
        if (sp == SP_THROWIN) back = V2(spPos.x < 1 ? -0.3f : 0.3f, 0);
        pl[spKicker].target = spPos + back;
    }
    for (auto& p : pl) {
        if (!p.onPitch) continue;
        p.target.x = clampf(p.target.x, -1.5f, PITCH_W + 1.5f);
        p.target.y = clampf(p.target.y, -1.0f, PITCH_L + 1.0f);
    }
}

void Match::updateSetPiece(float dt) {
    spT += dt;
    bool snapped = spT > 1.1f;
    for (int i = 0; i < 22; i++) {
        MPlayer& p = pl[i];
        if (!p.onPitch) continue;
        V2 d = p.target - p.pos;
        if (snapped) {
            if (!spReady) p.pos = p.target;
            p.vel = V2();
        } else {
            float l = d.len();
            float sp2 = std::min(l / dt, 10.f);
            p.vel = l > 0.05f ? d.norm() * sp2 : V2();
            p.pos += p.vel * dt;
            if (l > 0.3f) p.face = d.norm();
        }
        p.anim += p.vel.len() * dt;
        if (p.cool > 0) p.cool -= dt;
    }
    if (!snapped) return;
    if (!spReady) {
        spReady = true;
        for (int i = 0; i < 22; i++) if (pl[i].onPitch && i != spKicker) pl[i].face = (ball.pos - pl[i].pos).norm();
        if (spKicker >= 0) pl[spKicker].face = spAim;
        if (sp == SP_THROWIN && spKicker >= 0) { pl[spKicker].state = PS_THROW; }
    }
    int k = spKicker;
    if (k < 0) { state = MS_PLAY; return; }
    MPlayer& p = pl[k];
    ball.pos = sp == SP_THROWIN ? p.pos + V2(0, 0) : spPos;
    ball.z = sp == SP_THROWIN ? 1.9f : 0;
    int human = -1;
    for (int c = 0; c < NUM_INPUTS; c++) if (S.side[c] == p.team) { human = c; break; }
    if (S.lockSquad[p.team] >= 0 && p.squad != S.lockSquad[p.team]) human = -1;       // carrière de joueur : un coéquipier frappe
    // gardien adverse lors d'un penalty (humain : choisit la direction du plongeon)
    int ot = 1 - p.team;
    int gkDef = ot * 11;
    int defHuman = -1;
    for (int c = 0; c < NUM_INPUTS; c++) if (S.side[c] == ot) { defHuman = c; break; }
    if (S.lockSquad[ot] >= 0 && pl[ot * 11].squad != S.lockSquad[ot]) defHuman = -1;
    bool penalty = sp == SP_PENALTY || sp == SP_SHOOTOUT;
    // penalty contre un joueur humain : il choisit en secret le plongeon (gauche / centre / droite) ; rien n'est affiché
    if (penalty && defHuman >= 0) {
        if (spT < dt * 1.5f) penDive = 0;
        V2 dd = ctl[defHuman].dir;
        if (dd.x < -0.5f) penDive = -1;
        else if (dd.x > 0.5f) penDive = 1;
        else if (std::fabs(dd.y) > 0.5f) penDive = 0;
    }

    auto doKick = [&](int kind) { // kind 0 court, 1 long
        V2 aim = spAim;
        int t = p.team;
        switch (sp) {
        case SP_KICKOFF: {
            int tgt = bestPassTarget(k, human >= 0 && ctl[human].dir.len2() > 0.01f ? ctl[human].dir : V2(0, 0), false, 25);
            if (tgt >= 0) passTo(k, tgt, false); else kickBall(k, aim, 10, 0, human >= 0);
            break;
        }
        case SP_THROWIN: {
            // touche : lancer en cloche vers un partenaire, toujours vers l'intérieur du terrain
            int tgt = bestPassTarget(k, aim, kind == 1, kind ? 30.f : 20.f);
            V2 d = tgt >= 0 ? pl[tgt].pos + pl[tgt].vel * 0.4f - ball.pos : aim * (kind ? 16.f : 10.f);
            float in = spPos.x < PITCH_W / 2 ? 1.f : -1.f;
            if (d.x * in < 1.2f) d.x = in * std::max(1.2f, std::fabs(d.x));
            float dist = std::min(d.len(), kind ? 22.f : 16.f);
            float vz = 2.6f + dist * 0.06f, T = 2 * vz / GRAV;
            float vh = clampf(dist / T * 0.92f, 5.f, 17.f);
            kickBall(k, d, vh, vz, human >= 0, false);
            p.state = PS_NORMAL;
            break;
        }
        case SP_CORNER: case SP_GOALKICK: case SP_FREEKICK: case SP_INDIRECT: {
            if (sp == SP_CORNER && human >= 0 && std::fabs(spCurl) > 0.02f) {
                // corner brossé (L2 / R2) : centre rentrant ou sortant, le ballon suit la flèche courbe
                if (kind == 1) kickBall(k, aim, 20.f, 6.5f, true, false); else kickBall(k, aim, 15.f, 0.6f, true, false);
                ball.spin = spCurl * 5.5f;
            } else if (kind == 1) {
                int tgt = bestPassTarget(k, aim, true, 55);
                if (tgt >= 0 && human < 0) passTo(k, tgt, true);
                else if (tgt >= 0 && human >= 0) passTo(k, tgt, true);
                else kickBall(k, aim, 20, 8, human >= 0);
            } else {
                int tgt = bestPassTarget(k, aim, false, 30);
                if (tgt >= 0) passTo(k, tgt, false); else kickBall(k, aim, 16, 0.5f, human >= 0);
            }
            break;
        }
        default: break;
        }
        (void)t;
        if (sp == SP_THROWIN || sp == SP_CORNER || sp == SP_GOALKICK) offsideReset(OR_EXEMPT_RESTART);   // réception directe : pas de hors-jeu
        p.cool = sp == SP_THROWIN ? 0.8f : 0.35f;
        ball.backpass = !p.gk && sp != SP_PENALTY;     // touche ou coup de pied arrêté vers son gardien : pas de prise à la main
        state = MS_PLAY; stateT = 0;
    };
    auto doShot = [&](V2 aim, float power) {
        float sp2 = 18 + power * 14;
        float vz = 0.6f + power * 3.2f;
        V2 g = goalCenter(p.team);
        if (penalty) {
            // hauteur visée à la ligne de but : plus on frappe fort, plus le ballon monte... et peut passer au-dessus
            float dist = std::max(6.f, (g - ball.pos).len());
            float skill = (p.shoot * 0.6f + p.compo * 0.4f) / 100.f;
            float press = sp == SP_SHOOTOUT ? 0.25f + 0.1f * std::min(10, penTaken[0] + penTaken[1]) / 10.f : 0.15f;   // pression (séance de tirs au but)
            float h = 0.1f + power * 1.5f + R.frange(-0.35f, 0.35f) * (1.2f - skill) * (1.f+.02f*supporterPenaltyPressure(S,p.team));
            if (power > 0.75f) h += (power - 0.75f) * R.frange(3.f, 12.f) * (1.3f - skill);            // frappe en force : risque au-dessus
            h += R.frange(0.f, 1.2f) * press * (1.f - skill);
            h = std::max(0.05f, h);
            // précision latérale : sang-froid et pression
            aim = rot(aim, R.frange(-1.f, 1.f) * (0.02f + press * 0.05f * (S.personalityActive?p.personalityStress:1.f)) * (1.3f - skill));
            sp2 = 17 + power * 13;
            float tt = dist / sp2;
            vz = (h + 0.5f * GRAV * tt * tt) / tt;
        }
        kickBall(k, aim, sp2, vz, human >= 0);
        if (!penalty && sp == SP_FREEKICK && std::fabs(spCurl) > 0.02f) ball.spin = spCurl * 6.5f;   // effet choisi : le ballon contourne le mur
        if (!penalty && sp == SP_FREEKICK) for (int o = ot * 11 + 1; o < ot * 11 + 11; o++) {   // le mur saute
            float dd = (pl[o].pos - spPos).len();
            if (pl[o].onPitch && dd > 8.f && dd < 10.5f && R.chance(0.8f)) { pl[o].state = PS_HEAD; pl[o].st = 0; pl[o].z = 0.2f; pl[o].vz = 3.0f; }
        }
        if (penalty) {
            // plongeon du gardien
            MPlayer& G = pl[gkDef];
            penGk = gkDef;
            if (G.onPitch) {
                float side;
                if (defHuman >= 0) side = (float)penDive;
                else {
                    // l'ordinateur lit parfois la course d'élan (meilleur gardien : meilleure lecture)
                    float shotSide = (ball.pos + aim * 11.f).x - PITCH_W / 2;
                    int real = std::fabs(shotSide) < 0.9f ? 0 : (shotSide > 0 ? 1 : -1);
                    if (R.chance(0.08f + G.keep / 2000.f)) side = (float)real;
                    else { float r = R.f(); side = r < 0.42f ? -1.f : r < 0.84f ? 1.f : 0.f; }
                }
                if (std::fabs(side) > 0.2f) {
                    // plongeon au moment de la frappe : le gardien part sur un côté, un tir bien placé reste imparable
                    G.state = PS_DIVE; G.st = 0; G.smother = false;
                    G.vel = V2(side > 0 ? 1.f : -1.f, 0) * (3.1f + G.keep / 70.f);
                }
            }
            (void)g;
        }
        state = MS_PLAY; stateT = 0;
    };

    // lob de coup franc : ballon brossé qui passe au-dessus du mur et retombe derrière (effet L2 / R2 conservé)
    auto doLob = [&](float power) {
        p.charging = false; p.charge = 0; p.chargeKind = 0;
        float speed = 11.f + power * 10.f, vz = 6.2f + power * 3.8f;
        kickBall(k, spAim, speed, vz, human >= 0);
        if (std::fabs(spCurl) > 0.02f) ball.spin = spCurl * 5.f;
        p.cool = 0.35f;
        ball.backpass = !p.gk;
        state = MS_PLAY; stateT = 0;
    };
    if (human >= 0) {
        Controls& c = ctl[human];
        for (int j = 0; j < 22; j++) if (pl[j].human == human && j != k) { pl[j].human = -1; pl[j].charging = false; }
        ctrlPlayer[human] = k;
        p.human = human;
        // coup franc : effet (flèche courbe) - stick latéral pendant la prise d'élan, ou boutons L / R
        if (sp == SP_CORNER) {
            if (c.curlL) spCurl = clampf(spCurl - dt * 1.6f, -1.f, 1.f);
            if (c.curlR) spCurl = clampf(spCurl + dt * 1.6f, -1.f, 1.f);
        }
        if (sp == SP_FREEKICK) {
            if (p.charging && c.dir.len2() > 0.01f) { float side = c.dir.norm().dot(spAim.perp()); spCurl = clampf(spCurl + (side - spCurl) * std::min(1.f, dt * 5.f), -1.f, 1.f); }
            // L2 / R2 maintenus : la flèche se courbe progressivement d'un côté ou de l'autre
            if (c.curlL) spCurl = clampf(spCurl - dt * 1.6f, -1.f, 1.f);
            if (c.curlR) spCurl = clampf(spCurl + dt * 1.6f, -1.f, 1.f);
        }
        if (c.dir.len2() > 0.01f && !(sp == SP_FREEKICK && p.charging)) {
            if (penalty) {
                V2 g = goalCenter(p.team);
                float tx = PITCH_W / 2 + c.dir.x * (attackDir[p.team] < 0 ? 1 : -1) * 3.2f * (attackDir[p.team] < 0 ? 1 : -1);
                spAim = (V2(tx, g.y) - spPos).norm();
            } else {
                // rotation progressive de la visée
                V2 target = c.dir.norm();
                spAim = (spAim + (target - spAim) * std::min(1.f, dt * 6)).norm();
            }
            p.face = spAim;
        }
        if (sp == SP_KICKOFF) { if (c.f1p || c.f2p) doKick(0); return; }
        // commandes : Super Nintendo -> bouton Tir pour frapper (appui long = puissance), Passe = passe courte, Lob = ballon long ;
        //             classique -> bouton 1 frappe/passe, bouton 2 ballon long
        bool snes = S.snes;
        bool shootHeld = snes ? c.f2 : c.f1, shootRel = snes ? c.f2r : c.f1r;
        bool lobHeld = snes ? c.f3 : c.f2, lobRel = snes ? c.f3r : c.f2r;          // lob : appui long = plus fort et plus haut
        bool passP = snes && c.f1p, longP = snes && c.f4p;
        if (penalty || sp == SP_FREEKICK) {
            float cmax = penalty ? 0.6f : 0.9f;       // coup franc : élan plus long pour doser l'effet
            if (!(p.charging && p.chargeKind == 1) && shootHeld) { p.charging = true; p.chargeKind = 0; p.charge += dt; if (p.charge > cmax) { doShot(spAim, 1); return; } }
            if (shootRel && p.charging && p.chargeKind == 0) {
                if (!penalty && !snes && p.charge < 0.15f) doKick(0);
                else doShot(spAim, std::max(0.15f, std::min(1.f, p.charge / cmax)));
                return;
            }
            if (!penalty) {
                // lob par-dessus le mur (ou ballon long) : la puissance règle la distance et la hauteur
                if (!(p.charging && p.chargeKind == 0) && lobHeld) { p.charging = true; p.chargeKind = 1; p.charge += dt; if (p.charge > cmax) { doLob(1.f); return; } }
                if (lobRel && p.charging && p.chargeKind == 1) {
                    if (p.charge < 0.12f) doKick(1); else doLob(std::max(0.15f, std::min(1.f, p.charge / cmax)));
                    return;
                }
            }
            if (!penalty && passP) { doKick(0); return; }
            if (!penalty && longP) { doKick(1); return; }
            return;
        }
        if (snes) { if (c.f1p) doKick(0); else if (c.f2p || c.f3p || c.f4p) doKick(1); }
        else { if (c.f1p) doKick(0); else if (c.f2p) doKick(1); }
        return;
    }
    // IA
    float wait = sp == SP_KICKOFF ? 0.6f : 0.9f;
    if (penalty && spT > 1.1f) {
        // élan du tireur (recule puis s'avance vers le ballon) et gardien qui sautille sur sa ligne
        float total = 1.1f + wait + (sp == SP_SHOOTOUT ? 0.6f : 0);
        float u = clampf((spT - 1.1f) / (total - 1.1f), 0.f, 1.f);
        V2 back = spPos - spAim * 2.4f + spAim.perp() * 0.6f;
        p.pos = u < 0.35f ? back : back + (spPos - spAim * 0.5f - back) * ((u - 0.35f) / 0.65f);
        p.vel = u < 0.35f ? V2() : (spPos - back) * 1.2f; p.anim += p.vel.len() * dt; p.face = spAim;
        if (pl[gkDef].onPitch && pl[gkDef].state == PS_NORMAL && defHuman < 0) { pl[gkDef].pos.x = PITCH_W / 2 + std::sin(spT * 7.f) * 0.35f; pl[gkDef].anim += dt * 3; }
    }
    if (spT < 1.1f + wait + (sp == SP_SHOOTOUT ? 0.6f : 0)) return;
    V2 g = goalCenter(p.team);
    switch (sp) {
    case SP_PENALTY: case SP_SHOOTOUT: {
        float side = R.chance(0.5f) ? -1.f : 1.f;
        if (R.chance(0.15f)) side = 0;
        float acc = (100 - p.shoot) / 100.f * (1.f+.02f*supporterPenaltyPressure(S,p.team))*(S.personalityActive?p.personalityStress:1.f);
        float tx = PITCH_W / 2 + side * R.frange(1.8f, 3.3f) + R.frange(-1, 1) * acc * 2.5f;
        // l'ordinateur dose sa frappe : les meilleurs tireurs placent, les autres frappent parfois en force
        float pw = p.shoot > 75 ? R.frange(0.45f, 0.85f) : R.frange(0.45f, 1.0f);
        if (side == 0) pw = R.frange(0.55f, 0.9f);
        if (p.shoot > 80 && p.compo > 70 && R.chance(0.05f)) { side = 0; tx = PITCH_W / 2; pw = 0.22f; say(playerName(k) + " tente la panenka !", 2.5f, true); }   // panenka
        doShot((V2(tx, g.y) - spPos).norm(), pw);
        break;
    }
    case SP_FREEKICK: case SP_INDIRECT:
        if (p.gk) {   // gardien : long dégagement vers le milieu de terrain
            spAim = (fromTeamFrame(p.team, R.frange(0.5f, 0.62f), R.frange(0.2f, 0.8f)) - spPos).norm();
            kickBall(k, spAim, R.frange(24.f, 28.f), R.frange(8.f, 10.f), false);
            ball.backpass = false; state = MS_PLAY; stateT = 0;
            break;
        }
        if (progress(p.team, spPos) > 0.68f && (sp == SP_INDIRECT || std::fabs(spPos.x - PITCH_W / 2) > 18.f || (g - spPos).len() > 30.f)) {
            // coup franc excentré ou indirect près de la surface : centre au point de penalty sur le meilleur joueur de tête
            int best = -1; float bs = -1e9; V2 zone(PITCH_W / 2, g.y + (g.y == 0 ? 9.f : -9.f));
            for (int j = p.team * 11 + 1; j < p.team * 11 + 11; j++) {
                if (j == k || !pl[j].onPitch || (pl[j].pos - zone).len() > 12.f) continue;
                float sc = pl[j].heading * 0.05f - (pl[j].pos - zone).len() * 0.4f + R.frange(0, 1.5f);
                if (sc > bs) { bs = sc; best = j; }
            }
            if (best >= 0) { passTo(k, best, true); p.cool = 0.35f; ball.backpass = false; state = MS_PLAY; stateT = 0; break; }
        }
        if (sp == SP_INDIRECT) { doKick(progress(p.team, spPos) > 0.5f ? 1 : 0); break; }
        {
        float dist = (g - spPos).len();
        if (dist < 30 && std::fabs(spPos.x - PITCH_W / 2) < 20 && R.chance(0.7f)) {
            float side = spPos.x < PITCH_W / 2 ? 1.f : -1.f;
            V2 aim = (V2(PITCH_W / 2 + side * 2.5f, g.y) - spPos).norm();
            aim = rot(aim, -side * 0.08f * (attackDir[p.team] < 0 ? 1 : -1));
            kickBall(k, aim, 23, 3.4f + dist * 0.02f, false);
            ball.spin = side * (attackDir[p.team] < 0 ? -3.5f : 3.5f);
            state = MS_PLAY;
        } else doKick(progress(p.team, spPos) > 0.5f ? 1 : 0);
        break;
    }
    case SP_CORNER: {
        // corner : premier poteau, second poteau, point de penalty ou corner à deux ; on vise le meilleur joueur de tête de la zone
        float sy = g.y == 0 ? 1.f : -1.f, near = spPos.x < PITCH_W / 2 ? -1.f : 1.f;
        float r = R.f();
        int mode = r < 0.33f ? 0 : r < 0.63f ? 1 : r < 0.9f ? 2 : 3;
        if (mode == 3) {
            int mate = -1; float bd = 13.f;
            for (int j = p.team * 11 + 1; j < p.team * 11 + 11; j++) if (j != k && pl[j].onPitch) { float d = (pl[j].pos - spPos).len(); if (d < bd) { bd = d; mate = j; } }
            if (mate >= 0) { passTo(k, mate, false); p.cool = 0.35f; ball.backpass = false; offsideReset(OR_EXEMPT_RESTART); state = MS_PLAY; stateT = 0; say("Corner joué à deux.", 2.f); break; }
            mode = 2;
        }
        V2 zone = mode == 0 ? V2(PITCH_W / 2 + near * 2.6f, g.y + sy * 5.f) : mode == 1 ? V2(PITCH_W / 2 - near * 3.4f, g.y + sy * 6.5f) : V2(PITCH_W / 2, g.y + sy * 10.5f);
        int best = -1; float bs = -1e9;
        for (int j = p.team * 11 + 1; j < p.team * 11 + 11; j++) {
            if (j == k || !pl[j].onPitch) continue;
            float d = (pl[j].pos - zone).len(); if (d > 9.f) continue;
            float mark = 99.f; for (int o = ot * 11; o < ot * 11 + 11; o++) if (pl[o].onPitch) mark = std::min(mark, (pl[o].pos - pl[j].pos).len());
            float sc = pl[j].heading * 0.05f + std::min(mark, 3.f) * 1.2f - d * 0.6f + R.frange(0, 1.5f);
            if (sc > bs) { bs = sc; best = j; }
        }
        if (best >= 0) {
            V2 tgt = pl[best].pos + (zone - pl[best].pos) * 0.5f;
            V2 d = tgt - ball.pos; float L = d.len();
            kickBall(k, d, clampf(L * 0.55f + 9.f, 16.f, 26.f), clampf(L * 0.22f + 2.f, 6.f, 10.f), false);
            ball.spin = (mode == 1 ? -near : near) * (attackDir[p.team] < 0 ? 2.5f : -2.5f) * (R.chance(0.5f) ? 1.f : 0.4f);   // rentrant ou sortant
            p.cool = 0.35f; ball.backpass = false; offsideReset(OR_EXEMPT_RESTART); state = MS_PLAY; stateT = 0;
        } else { spAim = (zone - spPos).norm(); doKick(1); }
        break;
    }
    case SP_GOALKICK: {     // six mètres : long, sauf si un partenaire est totalement libre
        bool shortOk = false;
        for (int j = p.team * 11 + 1; j < p.team * 11 + 11 && !shortOk; j++) {
            if (!pl[j].onPitch || (pl[j].pos - spPos).len() > 25.f) continue;
            float open = 1e9; for (int o = ot * 11; o < ot * 11 + 11; o++) if (pl[o].onPitch) open = std::min(open, (pl[o].pos - pl[j].pos).len());
            if (open > 13.f) shortOk = true;
        }
        spAim = (fromTeamFrame(p.team, R.frange(0.6f, 0.72f), R.frange(0.25f, 0.75f)) - spPos).norm();
        doKick(shortOk && R.chance(0.3f) ? 0 : 1);
        break;
    }
    case SP_THROWIN: {
        // l'ordinateur cherche un partenaire démarqué, à distance raisonnable et vers l'intérieur
        float in = spPos.x < PITCH_W / 2 ? 1.f : -1.f;
        int tgt = -1; float bs = -1e9;
        for (int j = p.team * 11 + 1; j < p.team * 11 + 11; j++) {
            if (j == k || !pl[j].onPitch) continue;
            V2 d = pl[j].pos - spPos; float l = d.len();
            if (l < 4.f || l > 18.f || d.x * in < 1.f) continue;
            float open = 1e9; for (int o = ot * 11; o < ot * 11 + 11; o++) if (pl[o].onPitch) open = std::min(open, (pl[o].pos - pl[j].pos).len());
            float sc = std::min(open, 6.f) * 1.5f - l * 0.2f + (progress(p.team, pl[j].pos) - progress(p.team, spPos)) * 10.f;
            if (sc > bs) { bs = sc; tgt = j; }
        }
        // longue touche dans la surface quand on est proche du but adverse
        if (progress(p.team, spPos) > 0.8f && R.chance(0.35f)) {
            V2 zone(PITCH_W / 2 - in * 4.f, g.y + (g.y == 0 ? 7.f : -7.f));
            V2 d = zone - spPos; float dist = std::min(d.len(), 24.f);
            float vz = 4.2f, T2 = 2 * vz / GRAV;
            kickBall(k, d, clampf(dist / T2, 8.f, 19.f), vz, false, false);
            p.state = PS_NORMAL; p.cool = 0.8f; ball.backpass = true; offsideReset(OR_EXEMPT_RESTART); state = MS_PLAY; stateT = 0;
            say("Longue touche dans la surface !", 2.f);
            break;
        }
        spAim = tgt >= 0 ? (pl[tgt].pos - spPos).norm() : V2(in, (float)attackDir[p.team] * 0.4f).norm();
        doKick(0);
        break;
    }
    case SP_KICKOFF: {
        // coup d'envoi : passe courte au partenaire à côté, ou en retrait vers un milieu ; jamais de longue passe risquée
        int mate = -1; float bd = 1e9;
        for (int j = p.team * 11 + 1; j < p.team * 11 + 11; j++) {
            if (j == k || !pl[j].onPitch) continue;
            float d = (pl[j].pos - spPos).len();
            if (d < 9.f && d < bd) { bd = d; mate = j; }
        }
        if (mate < 0) {
            float best = -1e9;
            for (int j = p.team * 11 + 1; j < p.team * 11 + 11; j++) {
                if (j == k || !pl[j].onPitch || pl[j].role != 2) continue;
                float v = -std::fabs((pl[j].pos - spPos).len() - 12.f);
                if (v > best) { best = v; mate = j; }
            }
        }
        if (mate >= 0) passTo(k, mate, false); else kickBall(k, (ownGoal(p.team) - spPos).norm(), 9, 0, false);
        state = MS_PLAY; stateT = 0;
        break;
    }
    default: doKick(0); break;
    }
}

// ------------------------------------------------------------------ contrôle humain
void Match::assignHumans() {
    for (int t = 0; t < 2; t++) {
        std::vector<int> ctrls;
        for (int c = 0; c < NUM_INPUTS; c++) if (S.side[c] == t) ctrls.push_back(c);
        if (ctrls.empty()) continue;
        if (S.lockSquad[t] >= 0) {
            // carrière de joueur : on ne contrôle que son propre joueur (spectateur s'il est sur le banc ou sorti)
            int me = -1;
            for (int j = t * 11; j < t * 11 + 11; j++) if (pl[j].onPitch && pl[j].squad == S.lockSquad[t]) me = j;
            for (size_t k = 0; k < ctrls.size(); k++) {
                int c = ctrls[k], cur = ctrlPlayer[c];
                int want = k == 0 ? me : -1;
                if (cur >= 0 && cur != want) { pl[cur].human = -1; pl[cur].charging = false; }
                ctrlPlayer[c] = want;
                if (want >= 0) pl[want].human = c;
            }
            continue;
        }
        std::vector<int> taken;
        // propriétaire du ballon ?
        int owner = ball.owner >= 0 && pl[ball.owner].team == t ? ball.owner : -1;
        for (size_t k = 0; k < ctrls.size(); k++) {
            int c = ctrls[k];
            int cur = ctrlPlayer[c];
            if (cur >= 0 && (pl[cur].team != t || !pl[cur].onPitch)) cur = -1;
            int want = cur;
            // passe en cours : on prend directement le receveur (tant que le ballon voyage ou qu'il vient de le recevoir)
            if (passRecvT[c] > 0) {
                int r = passRecv[c];
                bool flying = ball.owner < 0 || ball.owner == r;
                if (r >= 0 && pl[r].onPitch && pl[r].team == t && flying && std::find(taken.begin(), taken.end(), r) == taken.end()) {
                    if (cur >= 0 && cur != r) { pl[cur].human = -1; pl[cur].charging = false; }
                    ctrlPlayer[c] = r; pl[r].human = c; taken.push_back(r);
                    if (ball.owner == r) { passRecvT[c] = 0; passRecv[c] = -1; lockSwitch[c] = 0.3f; }
                    continue;
                }
                if (ball.owner >= 0 && ball.owner != r) { passRecvT[c] = 0; passRecv[c] = -1; }
            }
            if (wantSwitch[c] && cur >= 0) {
                wantSwitch[c] = false;
                V2 bp = ball.pos + ball.vel * 0.35f;
                int best = -1; float bd = 1e9;
                V2 sd = switchDir[c];
                bool dirSw = sd.len2() > 0.25f;
                for (int j = t * 11 + 1; j < t * 11 + 11; j++) {
                    if (j == cur || !pl[j].onPitch || std::find(taken.begin(), taken.end(), j) != taken.end()) continue;
                    float d = (pl[j].pos - bp).len();
                    // stick orienté : on privilégie le partenaire situé dans cette direction (par rapport au joueur actuel)
                    if (dirSw) { V2 rel = pl[j].pos - pl[cur].pos; float l = rel.len(); if (l > 0.5f) d += (1.f - rel.dot(sd.norm()) / l) * 9.f + l * 0.15f; }
                    if (d < bd) { bd = d; best = j; }
                }
                if (best >= 0) { pl[cur].human = -1; pl[cur].charging = false; ctrlPlayer[c] = best; pl[best].human = c; taken.push_back(best); lockSwitch[c] = 0.6f; continue; }
            }
            if (lockSwitch[c] > 0 && cur >= 0 && pl[cur].onPitch && pl[cur].team == t && owner < 0) { taken.push_back(cur); continue; }
            if (owner >= 0 && std::find(taken.begin(), taken.end(), owner) == taken.end() && (k == 0 || cur == owner)) want = owner;
            else if (cur >= 0 && pl[cur].charging) want = cur;
            else {
                V2 bp = ball.pos + ball.vel * 0.35f;
                int best = -1; float bd = 1e9;
                for (int i = t * 11 + 1; i < t * 11 + 11; i++) {
                    if (!pl[i].onPitch || std::find(taken.begin(), taken.end(), i) != taken.end()) continue;
                    if (owner == i && k > 0) continue;
                    float d = (pl[i].pos - bp).len();
                    if (d < bd) { bd = d; best = i; }
                }
                float cd = cur >= 0 ? (pl[cur].pos - bp).len() : 1e9f;
                if (cur < 0 || cur == owner || (best >= 0 && bd + 1.5f < cd)) want = best;
                if (cur >= 0 && pl[cur].state == PS_SLIDE) want = cur;
            }
            if (want >= 0 && pl[want].gk && pl[want].state != PS_GKHOLD) want = cur >= 0 && !pl[cur].gk ? cur : -1;
            if (cur >= 0 && cur != want) { pl[cur].human = -1; pl[cur].charging = false; }
            ctrlPlayer[c] = want;
            if (want >= 0) { pl[want].human = c; taken.push_back(want); }
        }
    }
}

// lob en jeu : appui court = centre / lob vers le partenaire le mieux placé ; appui long = lob dosé dans la direction
// (près du but : lob par-dessus le gardien, plus haut quand la jauge est pleine)
void Match::doLobPlay(int i, V2 aim, float charge) {
    MPlayer& p = pl[i];
    if (charge < 0.16f) {
        int tgt = bestPassTarget(i, aim, true, 50);
        if (tgt >= 0) passTo(i, tgt, true); else kickBall(i, aim, 15, 6.5f, true);
        return;
    }
    float pw = clampf(charge / 0.6f, 0.f, 1.f);
    V2 g = goalCenter(p.team);
    float dg = (g - p.pos).len();
    bool chip = dg < 30.f && aim.dot((g - p.pos).norm()) > 0.6f;
    float speed = chip ? 10.f + pw * 8.f + dg * 0.12f : 12.f + pw * 12.f;
    float vz = chip ? 5.f + pw * 3.5f : 4.5f + pw * 5.f;
    float acc = (100 - p.shoot) / 100.f;
    kickBall(i, rot(aim, R.frange(-1, 1) * acc * 0.05f), speed, vz, true);
    if (chip) say("Lob tenté par " + playerName(i) + " !", 1.8f);
}
// commandes façon Super Nintendo : passe / tir / lob / profondeur / sprint
void Match::humanControlSnes(int i, const Controls& c, float dt) {
    MPlayer& p = pl[i];
    bool hasBall = ball.owner == i;
    bool sprint = c.sprint && c.dir.len2() > 0.01f && p.stamina > 0.15f;
    float spd = p.speed * (0.82f + 0.18f * p.stamina) * (sprint ? 1.2f : 1.0f);
    if (sprint) p.stamina = std::max(0.f, p.stamina - dt * 0.006f);
    V2 dirIn = c.dir;
    // pression : le joueur se dirige seul vers le porteur adverse
    if (!hasBall && c.f3 && ball.owner >= 0 && pl[ball.owner].team != p.team) { dirIn = (pl[ball.owner].pos - p.pos).norm(); spd *= 1.08f; }
    V2 want = dirIn.len2() > 0.01f ? dirIn.norm() * spd * (hasBall ? (sprint ? 0.9f : 0.84f) : 1.0f) : V2();
    V2 dv = want - p.vel;
    float acc = (sprint ? 26.f : 32.f) * dt;
    if (dv.len() > acc) dv = dv.norm() * acc;
    p.vel += dv;
    if (dirIn.len2() > 0.01f) {
        V2 nd = dirIn.norm();
        if (hasBall) p.face = (p.face + (nd - p.face) * std::min(1.f, dt * (sprint ? 9.f : 14.f))).norm();
        else p.face = nd;
    }
    V2 aim = c.dir.len2() > 0.01f ? c.dir.norm() : p.face;
    if (hasBall) {
        if (c.f1p) {                                    // passe courte au sol
            int tgt = bestPassTarget(i, aim, false);
            if (tgt >= 0) passTo(i, tgt, false); else kickBall(i, aim, 15, 0, true);
            return;
        }
        if (c.f3p && !p.charging) { p.charging = true; p.chargeKind = 1; p.charge = 0; }   // lob / centre : appui long = lob plus fort
        if (p.charging && p.chargeKind == 1) {
            p.charge += dt;
            if (!c.f3 || p.charge > 0.6f) {
                float ch = p.charge; p.charging = false; p.charge = 0; p.chargeKind = 0;
                doLobPlay(i, aim, ch);
                return;
            }
            return;
        }
        if (c.f4p) {                                    // passe en profondeur (dans la course du partenaire)
            int tgt = bestPassTarget(i, aim, false, 45);
            if (tgt >= 0) {
                V2 run = pl[tgt].vel.len() > 1 ? pl[tgt].vel.norm() : V2(0, (float)attackDir[p.team]);
                V2 spot = pl[tgt].pos + run * 7.f;
                spot.x = clampf(spot.x, 1.f, PITCH_W - 1.f); spot.y = clampf(spot.y, 1.f, PITCH_L - 1.f);
                V2 d = spot - p.pos;
                float dist = d.len();
                kickBall(i, d.norm(), std::min(24.f, 9.f + dist * 0.55f), 0.3f, true);
            } else kickBall(i, aim, 18, 0.2f, true);
            return;
        }
        if (c.f2p) { p.charging = true; p.charge = 0; p.chargeKind = 0; }   // tir : appui long = puissance
        if (p.charging && p.chargeKind == 0) {
            p.charge += dt;
            if (!c.f2 || p.charge > 0.55f) {
                float pw = std::min(1.f, std::max(0.25f, p.charge / 0.55f));
                p.charging = false; p.charge = 0;
                float acc2 = (100 - p.shoot) / 100.f;
                // léger guidage vers le cadre si le joueur vise le but
                V2 g = goalCenter(p.team);
                V2 tg = (g - p.pos).norm();
                V2 d = aim;
                if (d.dot(tg) > 0.55f) {
                    float side = d.x > tg.x + 0.08f ? 1.f : d.x < tg.x - 0.08f ? -1.f : 0.f;
                    V2 corner = V2(PITCH_W / 2 + side * (GOAL_W / 2 - 0.6f), g.y);
                    d = (corner - p.pos).norm();
                }
                d = rot(d, R.frange(-1, 1) * acc2 * 0.07f);
                kickBall(i, d, 17 + pw * 15, shotLift(i, d, 17 + pw * 15, pw, 0.8f + pw * 4.2f), true);
            }
        }
    } else {
        p.charging = false;
        int owner = ball.owner;
        bool oppHas = owner >= 0 && pl[owner].team != p.team;
        if (c.f2p) {                                     // tacle glissé
            p.state = PS_SLIDE; p.st = 0;
            p.slideDir = aim;
            p.vel = p.slideDir * std::max(spd * 1.25f, 9.f);
            p.fouledInSlide = false; p.touchedBallInSlide = false;
        } else if (c.f1p || c.f4p) {
            if (oppHas && (pl[owner].pos - p.pos).len() < 2.2f) {   // tacle debout
                p.state = PS_SLIDE; p.st = 0.25f;
                p.slideDir = (pl[owner].pos - p.pos).norm();
                p.vel = p.slideDir * 6.f;
                p.fouledInSlide = false; p.touchedBallInSlide = false;
            } else {
                // changer de joueur (vers la direction du stick si elle est tenue)
                for (int k = 0; k < NUM_INPUTS; k++) if (ctrlPlayer[k] == i) { wantSwitch[k] = true; switchDir[k] = c.dir; }
            }
        }
    }
}

void Match::humanControl(int i, const Controls& c, float dt) {
    MPlayer& p = pl[i];
    if (p.state == PS_SLIDE || p.state == PS_DOWN || p.state == PS_DIVE || p.state == PS_HEAD) return;
    float spd = p.speed * (0.82f + 0.18f * p.stamina);
    bool hasBall = ball.owner == i;
    if (p.state == PS_GKHOLD) {
        if (c.dir.len2() > 0.01f) p.face = c.dir.norm();
        p.vel = V2();
        if (c.f1p) {
            int tgt = bestPassTarget(i, c.dir, false, 35);
            p.state = PS_NORMAL; ball.owner = -1;
            ball.pos = p.pos + p.face * 0.6f;
            if (tgt >= 0) passTo(i, tgt, false); else kickBall(i, p.face, 14, 1.0f, true);
            ball.backpass = false;                 // relance pendant le jeu : le hors-jeu s'applique
            gkProtect(i);
        } else if (c.f2p || p.st > 6) {
            p.state = PS_NORMAL; ball.owner = -1;
            ball.pos = p.pos + p.face * 0.6f;
            kickBall(i, p.face, 25, 10, true);
            ball.backpass = false;
            gkProtect(i);
        }
        return;
    }
    if (S.snes) { humanControlSnes(i, c, dt); return; }
    V2 want = c.dir.len2() > 0.01f ? c.dir.norm() * spd * (hasBall ? 0.84f : 1.0f) : V2();
    V2 dv = want - p.vel;
    float acc = 32 * dt;
    if (dv.len() > acc) dv = dv.norm() * acc;
    p.vel += dv;
    if (c.dir.len2() > 0.01f) {
        V2 nd = c.dir.norm();
        if (hasBall) p.face = (p.face + (nd - p.face) * std::min(1.f, dt * 14)).norm();
        else p.face = nd;
    }
    if (hasBall) {
        if (c.f1p && !(p.charging && p.chargeKind == 1)) { p.charging = true; p.charge = 0; p.chargeKind = 0; }
        if (p.charging && p.chargeKind == 0) {
            p.charge += dt;
            if (!c.f1 || p.charge > 0.55f) {
                float ch = p.charge;
                p.charging = false;
                if (ch < 0.16f && c.f1r) {
                    int tgt = bestPassTarget(i, c.dir, false);
                    if (tgt >= 0) passTo(i, tgt, false); else kickBall(i, p.face, 15, 0, true);
                } else {
                    float pw = std::min(1.f, ch / 0.55f);
                    float acc2 = (100 - p.shoot) / 100.f;
                    V2 d = rot(c.dir.len2() > 0.01f ? c.dir.norm() : p.face, R.frange(-1, 1) * acc2 * 0.07f);
                    kickBall(i, d, 17 + pw * 15, shotLift(i, d, 17 + pw * 15, pw, 0.8f + pw * 4.2f), true);
                }
                p.charge = 0;
            }
        } else if (c.f2p) { p.charging = true; p.chargeKind = 1; p.charge = 0; }
        if (p.charging && p.chargeKind == 1) {        // bouton 2 : lob, appui long = plus fort
            p.charge += dt;
            if (!c.f2 || p.charge > 0.6f) { float ch = p.charge; p.charging = false; p.charge = 0; p.chargeKind = 0; doLobPlay(i, c.dir.len2() > 0.01f ? c.dir.norm() : p.face, ch); return; }
        }
    } else {
        p.charging = false;
        if (c.f2p || c.f1p) {
            // tacle glissé (bouton 2) ou tacle debout / plongeon (bouton 1)
            int owner = ball.owner;
            bool oppHas = owner >= 0 && pl[owner].team != p.team;
            if (c.f2p || (oppHas && (pl[owner].pos - p.pos).len() < 4.5f)) {
                p.state = PS_SLIDE; p.st = 0;
                p.slideDir = c.dir.len2() > 0.01f ? c.dir.norm() : p.face;
                p.vel = p.slideDir * std::max(spd * 1.25f, 9.f);
                p.fouledInSlide = false; p.touchedBallInSlide = false;
            } else if (c.f1p && S.lockSquad[p.team] < 0) {
                // loin du porteur : le bouton 1 change de joueur (vers la direction du stick si elle est tenue)
                for (int k = 0; k < NUM_INPUTS; k++) if (ctrlPlayer[k] == i) { wantSwitch[k] = true; switchDir[k] = c.dir; }
            }
        }
    }
}

// ------------------------------------------------------------------ IA
void Match::aiCarrier(int i, float dt) {
    MPlayer& p = pl[i];
    int t = p.team, ot = 1 - t;
    V2 g = goalCenter(t);
    V2 dg = g - p.pos;
    float dist = dg.len();
    float spd = p.speed * (0.82f + 0.18f * p.stamina) * 0.84f;
    // pression
    float press = 1e9; int presser = -1;
    for (int j = ot * 11; j < ot * 11 + 11; j++) {
        if (!pl[j].onPitch) continue;
        float d = (pl[j].pos - p.pos).len();
        if (d < press) { press = d; presser = j; }
    }
    // remise en une-deux : le passeur a pris de l'avance et il est libre
    if (p.wallT > 0 && p.wallFrom >= 0 && p.wallFrom != i && pl[p.wallFrom].onPitch && pl[p.wallFrom].team == t && pl[p.wallFrom].runT > 0) {
        int m = p.wallFrom;
        float gain = progress(t, pl[m].pos) - progress(t, p.pos);
        bool open = true;
        V2 dm = pl[m].pos - p.pos; float lm = dm.len();
        for (int k2 = ot * 11; k2 < ot * 11 + 11 && open; k2++) {
            if (!pl[k2].onPitch) continue;
            if ((pl[k2].pos - pl[m].pos).len() < 2.2f) open = false;
            float tt = clampf((pl[k2].pos - p.pos).dot(dm) / std::max(1.f, lm * lm), 0, 1);
            if ((p.pos + dm * tt - pl[k2].pos).len() < 1.3f) open = false;
        }
        if (gain > 0.03f && open && lm < 20.f && R.chance(0.8f)) {
            p.wallT = 0;
            if (progress(t, p.pos) > 0.55f) say("Une-deux entre " + playerName(m) + " et " + playerName(i) + " !", 2.2f);
            passTo(i, m, false);
            return;
        }
    }
    p.thinkT -= dt;
    if (p.thinkT <= 0) {
        p.thinkT = R.frange(0.15f, 0.3f);
        float prog = progress(t, p.pos);
        // tir
        if (dist < 29 && std::fabs(p.pos.x - PITCH_W / 2) < 22) {
            float pr = 0.05f + (29 - dist) / 29 * 0.45f;
            if (press < 2.5f) pr += 0.12f;
            // couloir de tir dégagé : l'attaquant ose davantage
            bool clear = true;
            for (int k = ot * 11 + 1; k < ot * 11 + 11 && clear; k++) {
                if (!pl[k].onPitch) continue;
                V2 w = pl[k].pos - p.pos; float tt = clampf(w.dot(dg) / std::max(1.f, dist * dist), 0, 1);
                if ((p.pos + dg * tt - pl[k].pos).len() < 1.4f) clear = false;
            }
            if (clear) pr += dist < 18 ? 0.25f : 0.1f;
            if (clear && dist < 15.f && (pl[ot * 11].pos - p.pos).len() < 9.f) pr += 0.3f;     // face au gardien : conclure avant qu'il ne ferme l'angle
            float ang = std::fabs(p.pos.x - PITCH_W / 2) / std::max(dist, 1.f);
            pr *= (1.2f - ang);
            if (dist > 25.f) pr *= 0.75f;                           // jeu plus construit : moins de frappes lointaines
            if (S.tac[t][3] == 0) pr *= 0.8f;
            if (R.chance(pr*p.effects.shot)) {
                int gk = ot * 11;
                float gkOut = std::fabs(pl[gk].pos.y - g.y);
                // gardien avancé : lob par-dessus
                if (gkOut > 6.f && dist < 24 && R.chance(0.5f)) {
                    V2 aim = (V2(PITCH_W / 2 + R.frange(-1.5f, 1.5f), g.y) - p.pos).norm();
                    kickBall(i, aim, clampf(dist * 0.7f + 4, 10, 20), 5.5f + dist * 0.12f, false);
                    return;
                }
                float side = pl[gk].pos.x < PITCH_W / 2 ? 1.f : -1.f;      // côté opposé au gardien
                if (R.chance(0.25f)) side = -side;
                float acc = (100 - p.shoot) / 100.f * (S.personalityActive&&((S.personalityImportant)||(clock>75&&std::abs(score[0]-score[1])<=1))?p.personalityStress:1.f);
                if (press < 2.f) acc += (100 - p.compo) / 450.f;       // sang-froid sous la pression
                float tx = PITCH_W / 2 + side * R.frange(2.2f, 3.4f) + R.frange(-1, 1) * (acc * 3.5f + acc * acc * 5.f + dist * 0.05f);
                V2 aim = (V2(tx, g.y) - p.pos).norm();
                float pw = clampf(18 + p.shoot / 6.f + R.frange(-2, 4), 18, 33);        // puissance selon la qualité de frappe
                kickBall(i, aim, pw, shotLift(i, aim, pw, (pw - 18.f) / 15.f, R.frange(0.3f, 2.0f) + dist * 0.025f), false);
                return;
            }
        }
        // passe en retrait depuis la ligne de but : un partenaire seul au point de penalty
        if (prog > 0.88f && std::fabs(p.pos.x - PITCH_W / 2) > 9 && R.chance(0.45f)) {
            int tgt = -1; float bs = -1e9;
            for (int j = t * 11 + 1; j < t * 11 + 11; j++) {
                if (j == i || !pl[j].onPitch) continue;
                float pj = progress(t, pl[j].pos);
                if (pj < 0.76f || pj > 0.9f || std::fabs(pl[j].pos.x - PITCH_W / 2) > 9) continue;
                float open = 1e9; for (int k = ot * 11; k < ot * 11 + 11; k++) if (pl[k].onPitch) open = std::min(open, (pl[k].pos - pl[j].pos).len());
                if (open > bs) { bs = open; tgt = j; }
            }
            if (tgt >= 0 && bs > 2.5f) { passTo(i, tgt, false); return; }
        }
        // passe en profondeur : un attaquant au contact de la ligne défensive, de l'espace dans son dos
        if (prog > 0.35f && prog < 0.8f && R.chance(0.25f + p.pass / 400.f)) {
            float b1 = 0, b2 = 0;
            for (int k = ot * 11; k < ot * 11 + 11; k++) if (pl[k].onPitch) { float pr = progress(t, pl[k].pos); if (pr > b1) { b2 = b1; b1 = pr; } else if (pr > b2) b2 = pr; }
            for (int j = t * 11 + 1; j < t * 11 + 11; j++) {
                if (j == i || !pl[j].onPitch || pl[j].role < 2) continue;
                float pj = progress(t, pl[j].pos);
                if (pj > b2 - 0.003f || pj < b2 - 0.08f || pj <= prog) continue;
                V2 T = pl[j].pos + V2(0.f, (float)attackDir[t]) * 8.f + V2((PITCH_W / 2 - pl[j].pos.x) * 0.12f, 0);
                if (T.y < 4.f || T.y > PITCH_L - 4.f || (T - g).len() < 9.f) continue;
                V2 d = T - p.pos; float L = d.len();
                if (L < 10.f || L > 38.f) continue;
                bool clear = true;
                for (int k = ot * 11; k < ot * 11 + 11 && clear; k++) {
                    if (!pl[k].onPitch) continue;
                    float tt = clampf((pl[k].pos - p.pos).dot(d) / (L * L), 0, 1);
                    if ((p.pos + d * tt - pl[k].pos).len() < 1.5f) clear = false;
                }
                if (!clear) continue;
                pl[j].runT = 1.6f; pl[j].runTarget = T;
                float acc = (100 - p.pass) / 100.f;
                V2 dir = rot(d.norm(), R.frange(-1, 1) * acc * 0.08f);
                kickingPass = true;
                kickBall(i, dir, clampf(std::sqrt(2 * 4.2f * pitchFriction * L) + 2.f, 10, 24), 0.f, false);
                kickingPass = false;
                if (prog > 0.5f && R.chance(0.5f)) say("Passe en profondeur de " + playerName(i) + " pour " + playerName(j) + " !", 2.2f);
                return;
            }
        }
        // centre
        if (prog > p.effects.crossAt && std::fabs(p.pos.x - PITCH_W / 2) > 17 && R.chance(0.55f)) {
            int tgt = -1; float bs = 1e9;
            for (int j = t * 11 + 1; j < t * 11 + 11; j++) {
                if (j == i || !pl[j].onPitch) continue;
                if (progress(t, pl[j].pos) > 0.8f && std::fabs(pl[j].pos.x - PITCH_W / 2) < 12) {
                    float s = std::fabs(pl[j].pos.x - PITCH_W / 2) + R.frange(0, 6);
                    if (s < bs) { bs = s; tgt = j; }
                }
            }
            if (tgt >= 0) { passTo(i, tgt, true); return; }
            V2 aimPt(PITCH_W / 2 + R.frange(-5, 5), g.y + (g.y == 0 ? 9.f : -9.f));
            V2 d = aimPt - p.pos; float dd = d.len();
            float vh = clampf(dd * 0.62f + 6, 10, 24);
            kickBall(i, d, vh, 0.5f * GRAV * dd / vh, false);
            return;
        }
        // dégagement
        if (inOwnBox(t, p.pos) && press < 4) {
            V2 d = fromTeamFrame(t, 0.6f, R.frange(0.2f, 0.8f)) - p.pos;
            kickBall(i, d, 24, 9, false, !p.gk);
            return;
        }
        // passe
        int best = -1; float bs = -1e9; bool bestLob = false;
        for (int j = t * 11; j < t * 11 + 11; j++) {
            if (j == i || !pl[j].onPitch || pl[j].state == PS_DOWN) continue;
            V2 d = pl[j].pos - p.pos;
            float dd = d.len();
            if (dd < 5 || dd > 42) continue;
            float gain = progress(t, pl[j].pos) - prog;
            float open = 1e9;
            for (int k = ot * 11; k < ot * 11 + 11; k++) if (pl[k].onPitch) open = std::min(open, (pl[k].pos - pl[j].pos).len());
            bool blocked = false;
            for (int k = ot * 11; k < ot * 11 + 11; k++) {
                if (!pl[k].onPitch) continue;
                V2 w = pl[k].pos - p.pos;
                float tt = clampf(w.dot(d) / (dd * dd), 0, 1);
                if ((p.pos + d * tt - pl[k].pos).len() < 1.6f) blocked = true;
            }
            float s = gain * p.effects.risk * (40 + S.tac[t][3] * 5) + std::min(open, 8.f) * 1.2f - dd * 0.08f - (blocked ? 12 : 0) - (pl[j].gk ? 15 : 0);
            if (S.tac[t][4] == 0) s -= std::max(0.f, dd - 20) * 0.25f;      // passes courtes
            else if (S.tac[t][4] == 2) s += std::min(dd, 35.f) * 0.1f;    // jeu long
            // hors-jeu potentiel
            float b2 = 0, b1 = 0;
            for (int k = ot * 11; k < ot * 11 + 11; k++) if (pl[k].onPitch) { float pr = progress(t, pl[k].pos); if (pr > b1) { b2 = b1; b1 = pr; } else if (pr > b2) b2 = pr; }
            float pj = progress(t, pl[j].pos);
            if (pj > 0.5f && pj > b2 && pj > prog) s -= 30;
            bool lob = blocked && dd > 15 && open > 4;
            if (lob) s += 8;
            if (s > bs) { bs = s; best = j; bestLob = lob; }
        }
        float need = press < 2.2f ? -6 : (press < 5 ? 6 : 14);
        need += (1 - S.tac[t][3]) * 1.5f;                                  // tempo lent : on garde plus le ballon... mais on fait circuler
        need+=p.effects.hold;
        float pc = (press < 3 ? 0.9f : 0.45f)/p.effects.dribble * (S.tac[t][3] == 2 ? 1.2f : S.tac[t][3] == 0 ? 1.1f : 1.f);
        if (press < 3 && p.compo > 70) pc *= 0.92f;                       // sang-froid : sait garder le ballon sous pression
        if (best >= 0 && bs > need && R.chance(std::min(0.97f, pc))) { passTo(i, best, bestLob); return; }
    }
    // dribble
    V2 dir = dg.norm();
    if (presser >= 0 && press < 5) {
        V2 away = (p.pos - pl[presser].pos).norm();
        float front = away.dot(dir * -1);
        if (front > 0.2f) dir = (dir + away.perp() * ((p.slot % 2) ? 1.f : -1.f) * 0.9f).norm();
        else dir = (dir * 0.7f + away * 0.5f).norm();
    }
    // reste dans le terrain
    if (p.pos.x < 3) dir.x = std::fabs(dir.x) + 0.3f;
    if (p.pos.x > PITCH_W - 3) dir.x = -std::fabs(dir.x) - 0.3f;
    dir = dir.norm();
    V2 want = dir * spd;
    V2 dv = want - p.vel; float acc = 24 * dt;
    if (dv.len() > acc) dv = dv.norm() * acc;
    p.vel += dv;
    p.face = (p.face + (dir - p.face) * std::min(1.f, dt * 8)).norm();
}

void Match::aiControl(int i, float dt) {
    MPlayer& p = pl[i];
    if (p.state != PS_NORMAL && p.state != PS_KICK) return;
    if (ball.owner == i) { aiCarrier(i, dt); return; }
    int t = p.team;
    float spd = p.speed * (0.82f + 0.18f * p.stamina);
    V2 target; float run = 0.75f;
    int owner = ball.owner;
    bool mateHas = owner >= 0 && pl[owner].team == t;
    bool oppHas = owner >= 0 && pl[owner].team != t;
    int chaser = nearestToBall(t, -1);
    // l'humain de l'équipe compte comme chasseur
    int humanP = -1;
    for (int c = 0; c < NUM_INPUTS; c++) if (S.side[c] == t && ctrlPlayer[c] >= 0) humanP = ctrlPlayer[c];
    if (p.runT > 0) p.runT -= dt;
    if (p.wallT > 0) p.wallT -= dt;
    // pressing faible : on laisse l'adversaire relancer, on garde sa position
    bool holdOff = oppHas && S.tac[t][0] == 0 && progress(pl[owner].team, pl[owner].pos) < 0.35f && (pl[owner].pos - p.pos).len() > 7.f;
    int second = -1;
    if (!mateHas) {
        float bd = 1e9;
        V2 bp = ball.pos + ball.vel * 0.3f;
        for (int j = t * 11 + 1; j < t * 11 + 11; j++) {
            if (j == chaser || !pl[j].onPitch) continue;
            float d = (pl[j].pos - bp).len();
            if (d < bd) { bd = d; second = j; }
        }
    }
    if (p.runT > 0 && (mateHas || (owner < 0 && ball.lastTeam == t && (ball.pos - p.pos).len() > 2.f)) && owner != i) {
        // appel de balle (une-deux) : course vers l'avant
        target = p.runTarget; run = 1.0f;
    } else if (!mateHas && !holdOff && (i == chaser || (humanP == chaser && i == second && !oppHas))) {
        // interception
        V2 bp = ball.pos; V2 bv = ball.vel;
        float tt = clampf((bp - p.pos).len() / std::max(spd, 1.f), 0, 1.2f);
        target = bp + bv * (tt * 0.8f);
        if (ball.z > 1.0f || ball.vz > 1.5f) {
            // ballon aérien : on va au point où il redescend à hauteur de tête (ou au sol)
            float hz = 1.7f, disc = ball.vz * ball.vz + 2 * GRAV * (ball.z - hz);
            float ta = disc >= 0 ? (ball.vz + std::sqrt(disc)) / GRAV : (ball.vz + std::sqrt(ball.vz * ball.vz + 2 * GRAV * ball.z)) / GRAV;
            target = bp + bv * std::max(0.f, ta);
        }
        run = 1.0f;
        // tacle
        if (oppHas && p.cool <= 0) {
            float d = (pl[owner].pos - p.pos).len();
            float aggr = S.difficulty == 2 ? 1.3f : S.difficulty == 0 ? 0.6f : 1.0f;
            aggr *= p.effects.press*(S.personalityActive?p.personalityEngagement:1.f);
            aggr *= S.tac[t][0] == 2 ? 1.25f : S.tac[t][0] == 0 ? 0.8f : 1.f;
            if (pl[owner].human >= 0 && S.difficulty >= 1) aggr *= 1.15f;
            {   // évite le tacle glissé par derrière (faute, carton) et dans sa propre surface (penalty)
                V2 ov = pl[owner].vel.len() > 0.5f ? pl[owner].vel.norm() : pl[owner].face;
                if ((pl[owner].pos - p.pos).norm().dot(ov) > 0.45f) aggr *= 0.3f;
                if (inOwnBox(t, pl[owner].pos)) aggr *= 0.55f;
            }
            if (d < 2.6f && d > 0.9f && R.chance(dt * 0.9f * aggr * (p.tackle / 70.f))) {
                p.state = PS_SLIDE; p.st = 0;
                p.slideDir = (ball.pos + ball.vel * 0.15f - p.pos).norm();
                p.vel = p.slideDir * std::max(spd * 1.2f, 8.5f);
                p.fouledInSlide = false; p.touchedBallInSlide = false;
                return;
            }
        }
    } else if (oppHas && i == second) {
        // deuxième défenseur : se place entre le porteur et le but et referme l'espace
        V2 og = ownGoal(t);
        float danger = progress(pl[owner].team, pl[owner].pos);
        float pressFrom = S.tac[t][0] == 2 ? 0.3f : S.tac[t][0] == 0 ? 0.6f : 0.45f;     // consigne de pressing
        pressFrom=clampf(pressFrom-(p.effects.press-1)*.2f,.1f,.8f);
        if (danger > pressFrom) {
            // temporise côté but : colle au porteur sans se jeter, en anticipant sa course
            float gap = danger > 0.75f ? 1.1f : 1.4f;
            target = pl[owner].pos + pl[owner].vel * 0.35f + (og - pl[owner].pos).norm() * gap; run = 1.0f;
        }
        else { target = pl[owner].pos + (og - pl[owner].pos).norm() * 2.5f; run = 0.95f; }
        // attaque le ballon s'il est à portée
        if ((ball.pos - p.pos).len() < 1.6f) { target = ball.pos; run = 1.0f; }
    } else if (oppHas && p.role == 1 && progress(pl[owner].team, pl[owner].pos) > 0.55f && (pl[owner].pos - p.pos).len() < 7.f + p.posi / 30.f && p.cool <= 0) {
        // défenseur de la zone : sort sur le porteur qui entre dans son secteur (couverture côté but)
        V2 og = ownGoal(t);
        target = pl[owner].pos + (og - pl[owner].pos).norm() * 1.2f + pl[owner].vel * 0.2f;
        run = 0.95f;
        if ((pl[owner].pos - p.pos).len() < 2.4f && p.cool <= 0 && R.chance(dt * 0.6f * (p.tackle / 70.f))) {
            p.state = PS_SLIDE; p.st = 0;
            p.slideDir = (ball.pos + ball.vel * 0.15f - p.pos).norm();
            p.vel = p.slideDir * std::max(spd * 1.2f, 8.5f);
            p.fouledInSlide = false; p.touchedBallInSlide = false;
            return;
        }
    } else {
        formationTarget(i, target);
        run = mateHas ? 0.8f : 0.72f;
        // repli défensif : un défenseur trop haut à la perte du ballon rentre au sprint
        if (!mateHas && p.role == 1 && progress(t, p.pos) > progress(t, target) + 0.04f) run = 1.0f;
        else if (!mateHas && p.role == 2 && progress(t, p.pos) > progress(t, target) + 0.12f) run = 0.9f;
        // pressing haut : attaquants et milieux proches harcèlent la relance adverse
        if (oppHas && S.tac[t][0] == 2 && p.role >= 2 && progress(pl[owner].team, pl[owner].pos) < 0.45f) {
            float d = (pl[owner].pos - p.pos).len();
            if (d < (p.role == 3 ? 12.f : 8.f)) { target = pl[owner].pos + (ownGoal(t) - pl[owner].pos).norm() * 1.6f; run = 0.95f; }
        }
        // contre-pressing : juste après la perte du ballon, les deux joueurs les plus proches harcèlent le porteur
        if (oppHas && lostT[t] < (S.tac[t][0] == 2 ? 3.0f : S.tac[t][0] == 0 ? 1.0f : 2.0f) && p.role >= 2 && progress(t, pl[owner].pos) > 0.35f) {
            float d = (pl[owner].pos - p.pos).len();
            if (d < 10.f) {
                int closer = 0;
                for (int j = t * 11 + 1; j < t * 11 + 11; j++) if (j != i && pl[j].onPitch && (pl[j].pos - pl[owner].pos).len() < d) closer++;
                if (closer < 2) { target = pl[owner].pos + pl[owner].vel * 0.25f + (ownGoal(t) - pl[owner].pos).norm() * 1.2f; run = 1.0f; }
            }
        }
        // marquage de zone : se rapprocher de l'attaquant le plus proche en défense
        if (oppHas && p.role == 1) {
            int ot = 1 - t; int m = -1; float md = 9;
            for (int j = ot * 11 + 1; j < ot * 11 + 11; j++) if (pl[j].onPitch && j != owner) { float d = (pl[j].pos - target).len(); if (d < md) { md = d; m = j; } }
            if (m >= 0) {
                // près de son but, le marquage se resserre (côté but, à un mètre) pour couper les centres et les passes en retrait
                float danger = progress(ot, pl[owner].pos);
                float w = danger > 0.7f ? 0.8f : 0.6f, gap = danger > 0.7f ? 1.0f : 1.5f;
                target = target * (1 - w) + (pl[m].pos + (ownGoal(t) - pl[m].pos).norm() * gap) * w;
            }
        }
        // milieux : en phase défensive, ils ferment la ligne de passe entre le porteur et l'attaquant le plus dangereux
        if (oppHas && p.role == 2 && progress(1 - t, pl[owner].pos) > 0.45f) {
            int ot = 1 - t, m = -1; float best = -1;
            for (int j = ot * 11 + 1; j < ot * 11 + 11; j++) if (pl[j].onPitch && j != owner) {
                float pr = progress(ot, pl[j].pos), d = (pl[j].pos - p.pos).len();
                if (d < 14.f && pr > best) { best = pr; m = j; }
            }
            if (m >= 0) { V2 lane = pl[owner].pos + (pl[m].pos - pl[owner].pos) * 0.55f; target = target * 0.5f + lane * 0.5f; }
        }
        // ballon en mouvement vers un adversaire : couper la trajectoire si on peut l'atteindre à temps
        if (owner < 0 && ball.lastTeam == 1 - t && ball.vel.len() > 7.f && ball.z < 1.6f) {
            V2 bv = ball.vel; float bs = bv.len(); V2 bd = bv * (1.f / bs);
            float u = clampf((p.pos - ball.pos).dot(bd), 0.f, bs * 1.1f);
            V2 q = ball.pos + bd * u;
            float tb = u / bs, tp = (q - p.pos).len() / std::max(spd, 1.f);
            if (tp < tb + 0.12f && (q - p.pos).len() < 9.f) { target = q; run = 1.0f; }
        }
    }
    if(S.rules==RULESET_CAREER&&oppHas&&(p.tactic.instructions&(1u<<TI_MARK)))for(int j=(1-t)*11+1;j<(1-t)*11+11;j++)if(pl[j].onPitch&&team(1-t).squad[pl[j].squad].id==p.tactic.markPid){target=pl[j].pos+(ownGoal(t)-pl[j].pos).norm()*1.6f;run=.95f;}
    V2 d = target - p.pos;
    float l = d.len();
    V2 want = l > 0.4f ? d.norm() * std::min(spd * run, l * 2.5f) : V2();
    V2 dv = want - p.vel; float acc = 22 * dt;
    if (dv.len() > acc) dv = dv.norm() * acc;
    p.vel += dv;
    if (p.vel.len() > 0.5f) p.face = p.vel.norm();
    else p.face = (ball.pos - p.pos).norm();
}

void Match::gkControl(int i, float dt) {
    MPlayer& p = pl[i];
    int t = p.team;
    V2 og = ownGoal(t);
    float sgn = og.y == 0 ? 1.f : -1.f;
    if (p.state == PS_DIVE || p.state == PS_DOWN) return;
    if (p.state == PS_GKHOLD) {
        p.vel = V2();
        if (p.human >= 0) return;
        if (p.st > 1.3f) {
            p.state = PS_NORMAL;
            ball.owner = -1;
            ball.pos = p.pos + V2(0, sgn * 0.8f);
            // distance minimale d'un adversaire au trajet du ballon (évite de relancer dans les pieds adverses)
            auto clearance = [&](V2 a, V2 b2) {
                float best = 1e9; V2 ab = b2 - a; float L2 = std::max(0.01f, ab.len2());
                for (int k = (1 - t) * 11; k < (1 - t) * 11 + 11; k++) {
                    if (!pl[k].onPitch) continue;
                    float u = clampf((pl[k].pos - a).dot(ab) / L2, 0.f, 1.f);
                    best = std::min(best, (pl[k].pos - (a + ab * u)).len());
                }
                return best;
            };
            int tgt = -1; float bs = -1e9;
            for (int j = t * 11 + 1; j < t * 11 + 11; j++) {
                if (!pl[j].onPitch) continue;
                float open = 1e9;
                for (int k = (1 - t) * 11; k < (1 - t) * 11 + 11; k++) if (pl[k].onPitch) open = std::min(open, (pl[k].pos - pl[j].pos).len());
                float dd = (pl[j].pos - p.pos).len();
                if (dd > 30) continue;
                if (clearance(p.pos, pl[j].pos) < 6.0f) continue;          // relance courte seulement si la ligne de passe est vraiment libre
                float s = open - dd * 0.1f;
                if (s > bs) { bs = s; tgt = j; }
            }
            // pressing adverse près de la surface : pas de relance courte
            int press = 0; for (int k = (1 - t) * 11; k < (1 - t) * 11 + 11; k++) if (pl[k].onPitch && (pl[k].pos - p.pos).len() < 24.f) press++;
            // relance longue vers un attaquant démarqué dans le camp adverse
            int fwd = -1; float fs = -1e9;
            for (int j = t * 11 + 1; j < t * 11 + 11; j++) {
                if (!pl[j].onPitch || progress(t, pl[j].pos) < 0.42f) continue;
                float open = 1e9;
                for (int k = (1 - t) * 11; k < (1 - t) * 11 + 11; k++) if (pl[k].onPitch) open = std::min(open, (pl[k].pos - pl[j].pos).len());
                float sc = open + progress(t, pl[j].pos) * 6.f - std::fabs(pl[j].pos.x - PITCH_W / 2) * 0.05f;
                if (sc > fs) { fs = sc; fwd = j; }
            }
            if (tgt >= 0 && bs > 11 && press <= 1 && R.chance(S.rules==RULESET_CAREER&&p.tactic.role==1?.75f:.35f)) { ball.pos = p.pos + (pl[tgt].pos - p.pos).norm() * 0.8f; passTo(i, tgt, false); }
            else if (fwd >= 0 && fs > 4.f && R.chance(0.65f)) {
                V2 aim = pl[fwd].pos + V2(0, (float)attackDir[t] * 3.f);
                ball.pos = p.pos + (aim - p.pos).norm() * 0.8f;
                float L = (aim - p.pos).len();
                kickBall(i, aim - p.pos, clampf(L * 0.52f + 6.f, 22.f, 33.f), clampf(L * 0.2f, 9.f, 15.f), false);
            } else {
                // dégagement : on choisit le côté le plus dégagé
                V2 bestD; float bc = -1;
                for (int c = 0; c < 7; c++) {
                    V2 aim = fromTeamFrame(t, R.frange(0.6f, 0.78f), 0.15f + c * 0.117f);
                    float cl = clearance(p.pos, p.pos + (aim - p.pos).norm() * 14.f) + R.frange(0, 1.5f);
                    if (cl > bc) { bc = cl; bestD = aim - p.pos; }
                }
                ball.pos = p.pos + bestD.norm() * 0.8f;
                kickBall(i, bestD, R.frange(28.f, 32.f), R.frange(12.f, 15.f), false);     // long dégagement
            }
            ball.backpass = false;                 // relance pendant le jeu : le hors-jeu s'applique
            p.cool = 0.6f;
            gkProtect(i);
        }
        return;
    }
    if (ball.owner == i) { // ballon au pied (passe en retrait)
        V2 d = fromTeamFrame(t, 0.55f, R.frange(0.2f, 0.8f)) - p.pos;
        kickBall(i, d, 24, 9, false, false);
        return;
    }
    // tir en approche ?
    Ball& b = ball;
    if (b.owner < 0 && b.vel.len() > 6 && (b.vel.y * sgn) < 0) {
        float ty = p.pos.y;
        float tt = (ty - b.pos.y) / b.vel.y;
        if (tt > 0 && tt < 1.3f) {
            float bx = b.pos.x + b.vel.x * tt;
            float bz = b.z + b.vz * tt - 0.5f * GRAV * tt * tt;
            if (std::fabs(bx - PITCH_W / 2) < GOAL_W / 2 + 1.5f && bz < GOAL_H + 0.5f) {
                float dx = bx - p.pos.x;
                float reachT = std::fabs(dx) / p.speed;
                if (std::fabs(dx) > 0.9f && tt < 0.5f + p.keep / 320.f && reachT > tt * 0.7f) {
                    p.state = PS_DIVE; p.st = 0; p.smother = false;
                    p.vel = V2(dx > 0 ? 1.f : -1.f, 0) * (3.7f + p.keep / 52.f);
                    return;
                }
                target_move: {
                    V2 tgt(bx, p.pos.y);
                    V2 d = tgt - p.pos;
                    p.vel = d.len() > 0.1f ? d.norm() * std::min(p.speed * (0.55f + p.keep / 400.f), d.len() * 6) : V2();
                    return;
                }
            }
        }
    }
    // position
    V2 bp = b.pos;
    float dist = (bp - og).len();
    V2 target;
    // sortie sur un attaquant seul
    int owner = b.owner;
    bool rush = false;
    if (owner >= 0 && pl[owner].team != t && dist < 14 && inOwnBox(t, pl[owner].pos)) rush = true;
    if (owner >= 0 && pl[owner].team != t && dist < 24 && !rush) {
        // face-à-face : plus aucun défenseur entre l'attaquant et le but -> le gardien sort
        int between = 0;
        for (int j = t * 11; j < t * 11 + 11; j++) if (pl[j].onPitch && !pl[j].gk && (pl[j].pos - og).len() < dist - 0.5f && std::fabs(pl[j].pos.x - bp.x) < 9.f) between++;
        if (between == 0) rush = true;
    }
    if (rush && owner >= 0 && pl[owner].team != t && p.cool <= 0) {
        float dd = (bp - p.pos).len();
        // 1 contre 1 : le gardien ne plonge que si l'attaquant vient sur lui (ou touche trop long) ; s'il le déborde sur le côté, il reste debout et suit
        V2 av = pl[owner].vel; V2 toGk = (p.pos - bp).norm();
        bool comesAt = av.len() < 1.5f || av.norm().dot(toGk) > 0.55f;
        float trig = 1.55f + (pl[owner].human >= 0 && p.human < 0 ? (S.difficulty == 0 ? 0.f : S.difficulty == 2 ? 0.4f : 0.25f) : 0.f) + p.keep / 250.f;
        if (!comesAt) trig = 1.05f;
        if (dd < trig && dd > 0.4f && inOwnBox(t, p.pos)) {
            // plongeon dans les pieds
            V2 dir = (bp + pl[owner].vel * 0.15f - p.pos).norm();
            p.state = PS_DIVE; p.st = 0.2f; p.smother = true;
            p.vel = dir * (5.0f + p.keep / 60.f);
            p.face = dir;
            p.cool = 1.4f;
            return;
        }
    }
    if (owner < 0 && dist < 13 && inOwnBox(t, bp) && b.vel.len() < 9 && b.z < 1) {
        int opp = nearestToBall(1 - t);
        float od = opp >= 0 ? (pl[opp].pos - bp).len() : 99;
        if ((p.pos - bp).len() < od + 1) rush = true;
    }
    float gkSet = 1.f;      // vitesse de sortie (le gardien se fige quand l'attaquant arrive sur lui)
    if (rush) {
        target = bp;
        if (owner >= 0 && pl[owner].team != t) {
            // face-à-face : sortie sur l'axe ballon-but pour fermer l'angle, sans se jeter : il garde 3 m de distance et se
            // place (appuis) dès que l'attaquant est à une dizaine de mètres, puis glisse latéralement pour rester sur l'axe
            V2 ap = bp + pl[owner].vel * 0.18f;
            float da = (ap - og).len();
            float out = clampf(da * 0.36f, 1.0f, 6.5f + p.keep / 50.f);
            out = std::min(out, std::max(1.0f, da - 2.4f - (100 - p.keep) / 80.f));
            target = og + (ap - og).norm() * out;
            if (da < 11.f) gkSet = 0.78f + p.keep / 600.f;
        }
    } else {
        float out = clampf(dist * 0.11f+p.effects.advance*100, 0.8f, 9.f);
        target = og + (bp - og).norm() * out;
        target.x = clampf(target.x, PITCH_W / 2 - GOAL_W / 2 - 1, PITCH_W / 2 + GOAL_W / 2 + 1);
    }
    target.y = sgn > 0 ? std::max(target.y, 0.3f) : std::min(target.y, PITCH_L - 0.3f);
    V2 d = target - p.pos;
    float l = d.len();
    float sp2 = rush ? p.speed * gkSet : p.speed * 0.8f;
    p.vel = l > 0.2f ? d.norm() * std::min(sp2, l * 4) : V2();
    p.face = (bp - p.pos).norm();
}

// relance du gardien : les adversaires proches ne peuvent pas intercepter immédiatement
void Match::gkProtect(int gk) {
    for (int j = (1 - pl[gk].team) * 11; j < (1 - pl[gk].team) * 11 + 11; j++)
        if (pl[j].onPitch && (pl[j].pos - pl[gk].pos).len() < 16.f) pl[j].cool = std::max(pl[j].cool, 0.75f);
}

// ------------------------------------------------------------------ mise à jour des joueurs
void Match::updatePlayers(float dt) {
    for (int i = 0; i < 22; i++) {
        MPlayer& p = pl[i];
        if (!p.onPitch) continue;
        p.st += dt;
        if (p.cool > 0) p.cool -= dt;
        switch (p.state) {
        case PS_SLIDE:
            p.vel = p.vel * (1 - 2.2f * dt);
            if (p.st > 0.6f) { p.state = PS_NORMAL; p.cool = 0.25f; p.vel = V2(); }
            break;
        case PS_DOWN:
            p.vel = p.vel * (1 - 5 * dt);
            if (p.st > 1.3f) { p.state = PS_NORMAL; }
            break;
        case PS_DIVE:
            p.vel = p.vel * (1 - (p.smother ? 3.0f : 1.2f) * dt);
            if (p.st > 0.75f) { p.state = PS_DOWN; p.st = 0.6f; p.vel = V2(); p.smother = false; }
            break;
        case PS_KICK: if (p.st > 0.2f) p.state = PS_NORMAL; break;
        case PS_HEAD:
            p.vz -= GRAV * dt; p.z += p.vz * dt;
            if (p.z <= 0) { p.z = 0; p.state = PS_NORMAL; }
            break;
        case PS_BIKE:   // retourné : le joueur retombe au sol
            p.vz -= GRAV * dt; p.z += p.vz * dt; p.vel = p.vel * (1 - 3 * dt);
            if (p.z <= 0) { p.z = 0; p.state = PS_DOWN; p.st = 0.7f; }
            break;
        default: break;
        }
        if (state == MS_PLAY) {
            if (p.human >= 0) humanControl(i, ctl[p.human], dt);
            else if (p.gk) gkControl(i, dt);
            else aiControl(i, dt);
        }
        // endurance
        float v = p.vel.len();
        static const float HEAT[6] = { 1.45f, 1.2f, 1.f, 1.f, 1.06f, 1.1f };
        p.stamina = std::max(0.25f, p.stamina - dt * HEAT[std::max(0, std::min(5, S.meteo))] * (v > p.speed * 0.85f ? 0.0022f : 0.0006f) * (1.3f - p.endur * 0.6f) * (S.tac[p.team][0] == 2 ? 1.15f : S.tac[p.team][0] == 0 ? 0.92f : 1.f) * (180.f / std::max(60.f, S.halfSeconds)));
        p.pos += p.vel * dt;
        p.pos.x = clampf(p.pos.x, -2.5f, PITCH_W + 2.5f);
        p.pos.y = clampf(p.pos.y, -2.0f, PITCH_L + 2.0f);
        p.anim += v * dt;
        // tacle glissé : contact avec un adversaire
        if (p.state == PS_SLIDE && !p.fouledInSlide && state == MS_PLAY) {
            for (int j = (1 - p.team) * 11; j < (1 - p.team) * 11 + 11; j++) {
                MPlayer& q = pl[j];
                if (!q.onPitch || q.state == PS_DOWN || q.state == PS_GKHOLD) continue;
                if ((q.pos - p.pos).len() < 0.85f) {
                    bool nearBall = ball.owner == j || (ball.pos - q.pos).len() < 1.8f;
                    if (!nearBall) continue;
                    p.fouledInSlide = true;
                    if (!p.touchedBallInSlide) {
                        // tacle glissé : réussite selon le tacle du défenseur, le dribble et l'agilité du porteur, et l'angle
                        bool behind = q.face.dot(p.slideDir) > 0.55f;
                        float side = std::fabs(q.face.x * p.slideDir.y - q.face.y * p.slideDir.x);
                        float clean = 0.28f + (p.tackle - (q.dribble * 0.6f + q.speed * 0.4f)) / 110.f + (behind ? -0.3f : 0.f) + side * 0.08f;
                        clean = clampf(clean, 0.06f, 0.85f);
                        float r = R.f();
                        if (r < clean) {
                            // ballon proprement récupéré
                            if (ball.owner == j) { ball.owner = -1; ball.vel = p.slideDir * 6.5f + V2(R.frange(-1, 1), R.frange(-1, 1)); ball.lastTouch = i; ball.lastTeam = p.team; }
                            p.touchedBallInSlide = true;
                            if (R.chance(0.35f)) { q.state = PS_DOWN; q.st = 0.6f; q.vel = p.slideDir * 1.5f; }
                            playSfx(SFX_KICK);
                            continue;
                        }
                        float fp = behind ? 0.85f : 0.55f;
                        if (r < clean + (1 - clean) * fp) { foul(i, j, behind); return; }
                        // raté : le porteur saute par-dessus le tacle
                        q.cool = 0; continue;
                    }
                    q.state = PS_DOWN; q.st = 0.4f; q.vel = p.slideDir * 2;
                    if (ball.owner == j) { ball.owner = -1; ball.vel = p.slideDir * 5; ball.lastTouch = i; ball.lastTeam = p.team; }
                }
            }
        }
    }
    // gardien en possession du ballon : les adversaires doivent s'écarter (pas de gêne sur la relance)
    for (int g = 0; g < 22; g += 11) {
        MPlayer& k = pl[g];
        if (!k.onPitch || k.state != PS_GKHOLD) continue;
        for (int j = (1 - k.team) * 11; j < (1 - k.team) * 11 + 11; j++) {
            MPlayer& q = pl[j];
            if (!q.onPitch) continue;
            V2 d = q.pos - k.pos;
            float l = d.len();
            const float R0 = 6.0f;
            if (l < R0) {
                V2 n = l > 0.01f ? d * (1.f / l) : V2(0, k.pos.y < PITCH_L / 2 ? 1.f : -1.f);
                q.pos += n * std::min(R0 - l, 9.f * dt);
                if (q.human < 0) q.vel = n * 4.f;
            }
        }
    }
    // séparation légère entre joueurs
    for (int i = 0; i < 22; i++) for (int j = i + 1; j < 22; j++) {
        if (!pl[i].onPitch || !pl[j].onPitch) continue;
        V2 d = pl[j].pos - pl[i].pos;
        float l = d.len();
        if (l < 0.55f && l > 0.001f) {
            V2 push = d * ((0.55f - l) / l * 0.5f);
            pl[i].pos -= push; pl[j].pos += push;
        }
    }
}

// ------------------------------------------------------------------ déroulement
// action offensive en cours : porteur dans la moitié adverse, ou ballon lancé vers le but (tir, centre, passe en profondeur)
bool Match::attackOngoing() const {
    if (ball.owner >= 0) {
        const MPlayer& p = pl[ball.owner];
        if (p.gk && p.state == PS_GKHOLD) return false;
        return progress(p.team, ball.pos) > 0.6f;
    }
    int t = ball.lastTeam;
    if (t < 0 || t > 1) return false;
    float pr = progress(t, ball.pos);
    float vy = ball.vel.y * (float)attackDir[t];
    if (pr > 0.55f && ball.vel.len() > 3.f) return true;
    if (pr > 0.4f && vy > 8.f) return true;
    return pr > 0.78f && (ball.pos.x > PITCH_W * 0.2f && ball.pos.x < PITCH_W * 0.8f);   // ballon qui traîne dans la surface
}

void Match::endPeriod() {
    int th = score[0] + S.aggHome, ta = score[1] + S.aggAway;
    // buts à l'extérieur : à égalité au cumul, l'équipe qui a marqué le plus à l'extérieur est qualifiée
    if (S.hasFirstLeg && S.awayGoals && th == ta && score[1] != S.aggHome) { if (score[1] > S.aggHome) ta++; else th++; }
    state = MS_BREAK; stateT = 0;
    ball.owner = -1;
    if (period == 0) { msg = "MI-TEMPS"; msgT = 3; nextSp = 1; say(fmt("C'est la mi-temps : %s %d - %d %s.", team(0).name.c_str(), score[0], score[1], team(1).name.c_str()), 4.f, true); }
    else if (period == 1) {
        if (S.decisive && th == ta && S.noET) { msg = "TIRS AU BUT"; msgT = 3; nextSp = 4; }
        else if (S.decisive && th == ta) { msg = "PROLONGATIONS"; msgT = 3; nextSp = 2; }
        else { msg = "FIN DU MATCH"; msgT = 4; nextSp = -1; }
    } else if (period == 2) { msg = "MI-TEMPS DES PROLONGATIONS"; msgT = 3; nextSp = 3; swapInit = false; }
    else {
        if (th == ta && !S.etNoPens) { msg = "TIRS AU BUT"; msgT = 3; nextSp = 4; aet = true; }
        else { msg = "FIN DU MATCH"; msgT = 4; nextSp = -1; aet = true; }
    }
    playSfx(nextSp == -1 ? SFX_WHISTLE_FINAL : SFX_WHISTLE_LONG);
    if (nextSp == -1) { rumbleFx(-1, 9); rumbleFx(-2, 9); }
    msg2 = fmt("%s %d - %d %s", team(0).shortName.c_str(), score[0], score[1], team(1).shortName.c_str());
    // remplacements automatiques à la pause
    autoSubs(0); autoSubs(1);
}

void Match::shootoutNext() {
    // fin ?
    int a = pens[0], b = pens[1];
    int ta = penTaken[0], tb = penTaken[1];
    bool done = false;
    if (ta <= 5 && tb <= 5) {
        if (a + (5 - ta) < b || b + (5 - tb) < a) done = true;
    }
    if (ta >= 5 && tb >= 5 && ta == tb && a != b) done = true;
    if (done) {
        finishMatch();
        return;
    }
    int t = shootTeam;
    // alterner
    if (penTaken[t] > penTaken[1 - t]) t = 1 - t;
    else if (penTaken[t] == penTaken[1 - t]) t = (penTaken[0] + penTaken[1]) % 2 == 0 ? shootTeam : 1 - shootTeam;
    int n = penTaken[t];
    int E = std::max(1, (int)shootElig[t].size());
    int pos = n % E;
    if (pos == 0) shootDone[t].clear();                 // tous les tireurs sont passés : on repart de la liste de départ
    if (pos < (int)shootList[t].size()) { shootoutKick(t, shootList[t][pos]); return; }
    // du 6e au dernier tireur : choix au cas par cas parmi ceux qui n'ont pas encore tiré
    if (humanSide(t)) { shootUI = 3; shootUITeam = t; shootSel = 0; shootUIT = 0; return; }
    std::vector<int> rest = shootRemaining(t);
    std::stable_sort(rest.begin(), rest.end(), [&](int a, int b) { if (pl[a].gk != pl[b].gk) return !pl[a].gk; return pl[a].shoot > pl[b].shoot; });
    shootoutKick(t, rest.empty() ? shootElig[t][0] : rest[0]);
}

void Match::shootoutKick(int t, int kicker) {
    shootKicker = kicker;
    shootDone[t].push_back(kicker);
    attackDir[t] = shootGoal == 0 ? -1 : 1; attackDir[1 - t] = -attackDir[t];
    beginSetPiece(SP_SHOOTOUT, t, V2(PITCH_W / 2, shootGoal == 0 ? 11.f : PITCH_L - 11.f));
    state = MS_SETPIECE;
}

std::vector<int> Match::shootRemaining(int t) const {
    std::vector<int> v;
    for (int i : shootElig[t]) if (std::find(shootDone[t].begin(), shootDone[t].end(), i) == shootDone[t].end()) v.push_back(i);
    return v;
}

// séance de tirs au but : tireurs autorisés (même nombre des deux côtés), liste des 5 premiers tireurs
void Match::shootoutSetup() {
    for (int t = 0; t < 2; t++) {
        shootElig[t].clear(); shootList[t].clear(); shootDone[t].clear();
        for (int i = t * 11; i < t * 11 + 11; i++) if (pl[i].onPitch && !pl[i].sentOff) shootElig[t].push_back(i);
    }
    for (int t = 0; t < 2; t++) {
        shootExcl[t] = std::max(0, (int)shootElig[t].size() - (int)shootElig[1 - t].size());
        if (shootExcl[t] > 0 && !humanSide(t)) {
            // supériorité numérique : l'ordinateur écarte ses moins bons tireurs (jamais le gardien)
            std::vector<int> v = shootElig[t];
            std::stable_sort(v.begin(), v.end(), [&](int a, int b) { if (pl[a].gk != pl[b].gk) return pl[a].gk; return pl[a].shoot > pl[b].shoot; });
            for (int k = 0; k < shootExcl[t]; k++) { int drop = v[v.size() - 1 - k]; shootElig[t].erase(std::find(shootElig[t].begin(), shootElig[t].end(), drop)); }
            shootExcl[t] = 0;
        }
        if (!humanSide(t)) {
            std::vector<int> v = shootElig[t];
            std::stable_sort(v.begin(), v.end(), [&](int a, int b) { if (pl[a].gk != pl[b].gk) return !pl[a].gk; return pl[a].shoot > pl[b].shoot; });
            for (int k = 0; k < 5 && k < (int)v.size(); k++) shootList[t].push_back(v[k]);
        }
    }
    shootNextUI();
}

// ouvre l'écran de choix suivant pour une équipe humaine, sinon lance la séance
void Match::shootNextUI() {
    for (int t = 0; t < 2; t++) {
        if (!humanSide(t)) continue;
        if (shootExcl[t] > 0) { shootUI = 1; shootUITeam = t; shootSel = 0; shootUIT = 0; return; }
        if ((int)shootList[t].size() < std::min(5, (int)shootElig[t].size())) { shootUI = 2; shootUITeam = t; shootSel = 0; shootUIT = 0; return; }
    }
    shootUI = 0; shootUITeam = -1;
    shootoutNext();
}

std::vector<int> Match::shootChoices() const {
    int t = shootUITeam;
    std::vector<int> v;
    if (t < 0) return v;
    if (shootUI == 1) return shootElig[t];
    if (shootUI == 2) { for (int i : shootElig[t]) if (std::find(shootList[t].begin(), shootList[t].end(), i) == shootList[t].end()) v.push_back(i); return v; }
    return shootRemaining(t);
}

void Match::updateShootUI(float dt) {
    shootUIT += dt;
    std::vector<int> ch = shootChoices();
    if (ch.empty()) { shootNextUI(); return; }
    if (shootSel >= (int)ch.size()) shootSel = 0;
    bool ok = tossNav(shootUITeam, shootSel, (int)ch.size(), dt);
    if (!ok || shootUIT < 0.4f) return;
    int t = shootUITeam, pick = ch[shootSel];
    shootUIT = 0; shootSel = 0;
    if (shootUI == 1) {
        shootElig[t].erase(std::find(shootElig[t].begin(), shootElig[t].end(), pick));
        shootExcl[t]--;
        shootNextUI();
    } else if (shootUI == 2) {
        shootList[t].push_back(pick);
        shootNextUI();
    } else {
        shootUI = 0; shootUITeam = -1;
        shootoutKick(t, pick);
    }
}

void Match::finishMatch() {
    if (shootout || abandoned) playSfx(SFX_WHISTLE_FINAL);
    finished = true; walkT = 0;
    state = MS_END; stateT = 0;
    if (shootout) { resultPH = pens[0]; resultPA = pens[1]; }
    msg = "FIN DU MATCH";
    msg2 = fmt("%s %d - %d %s", team(0).shortName.c_str(), score[0], score[1], team(1).shortName.c_str());
    if (shootout) msg2 += fmt(" (tab %d-%d)", pens[0], pens[1]);
    msgT = 4.5f;
}

void Match::record() {
    int h = rpHead;
    for (int i = 0; i < 22; i++) {
        Snap& s = rp[h * 22 + i];
        const MPlayer& p = pl[i];
        s.x = p.pos.x; s.y = p.pos.y; s.z = p.z;
        s.fx = (int8_t)std::lround(p.face.x * 100); s.fy = (int8_t)std::lround(p.face.y * 100);
        s.state = p.onPitch ? (uint8_t)p.state : (uint8_t)PS_OFF;
        s.frame = (uint8_t)((int)(p.anim * 2.2f) & 255);
    }
    rb[h] = { ball.pos.x, ball.pos.y, ball.z };
    rpHead = (rpHead + 1) % REPLAY_N;
    if (rpCount < REPLAY_N) rpCount++;
}

void Match::startReplay() {
    state = MS_REPLAY; stateT = 0;
    int n = std::min(rpCount, 330);
    rpPos = (rpHead - n + REPLAY_N) % REPLAY_N;
}

void Match::update(float dt) {
    if (trophyActive) { updateTrophy(dt); return; }
    if (lapActive) { updateLap(dt); return; }
    lostT[0] += dt; lostT[1] += dt;
    if (offFlagT > 0 && offPend < 0) offFlagT = std::max(0.f, offFlagT - dt);     // drapeau de l'assistant baissé après le signalement
    if (finished) {
        if (invasion) { updateInvasion(dt); return; }
        if (!S.training) updateEndScene(dt);
        return;
    }
    stateT += dt;
    lastShotAge += dt;
    updateMedics(dt);
    // orage : éclairs et tonnerre
    if (flashT > 0) flashT -= dt;
    if (S.meteo == 4 && !S.training && (thunderT -= dt) <= 0) {
        thunderT = R.frange(14.f, 34.f); flashT = 0.35f; playSfx(SFX_THUNDER);
        if (R.chance(0.3f)) say("Un violent coup de tonnerre résonne au-dessus du stade !", 2.8f);
    }
    // canicule : pause fraîcheur (vers la 30e et la 75e minute, au premier arrêt de jeu)
    if (S.meteo == 0 && !S.training && period < 2 && !coolBreak[period] && clock >= (period == 0 ? 30.f : 75.f) && state == MS_SETPIECE && stateT < 0.05f) {
        coolBreak[period] = true;
        playSfx(SFX_WHISTLE);
        msg = "PAUSE FRAÎCHEUR"; msg2 = "Les joueurs se désaltèrent"; msgT = 3.5f;
        say("Avec cette chaleur écrasante, l'arbitre accorde une pause fraîcheur : les joueurs se ruent sur les bouteilles d'eau.", 4.5f, true);
        for (auto& p : pl) if (p.onPitch) p.stamina = std::min(p.stam0, p.stamina + 0.1f);
        stateT = -2.5f;
    }
    trackTouch();
    updateReferee(dt);
    lastFoulT += dt;
    if (ceremony) { updateCeremony(dt); return; }
    if (shootUI > 0) { if (msgT > 0) msgT -= dt; updateShootUI(dt); return; }
    // bagarre en cours (bousculade, bagarre, combat)
    if (updateFight(dt)) return;
    // provocation : juste après une faute subie, le joueur humain peut aller au contact (bouton tir / lob)
    if (state == MS_STOP && lastFoulT < 1.5f && lastFoulVic >= 0 && lastFoulOff >= 0 && pl[lastFoulVic].onPitch && pl[lastFoulOff].onPitch) {
        for (int c = 0; c < NUM_INPUTS; c++) {
            if (S.side[c] != pl[lastFoulVic].team) continue;
            if (ctl[c].f2p || ctl[c].f3p) {
                int me = ctrlPlayer[c] >= 0 ? ctrlPlayer[c] : lastFoulVic;
                if ((pl[me].pos - pl[lastFoulOff].pos).len() < 6.f) { lastFoulT = 99; escalate(me, lastFoulOff, true); return; }
            }
        }
    }
    if (msgT > 0) msgT -= dt;
    if (S.training && trainingUpdate(dt)) return;
    if (!ceremony && state != MS_REPLAY) updatePending(dt);
    float rate = S.training ? 0.f : (float)S.halfMinutes / std::max(20.f, S.halfSeconds);
    switch (state) {
    case MS_SETPIECE:
        updateSetPiece(dt);
        if (!shootout){clock+=dt*rate*.5f;if(S.rules==RULESET_CAREER)for(int i=0;i<22;i++)if(pl[i].onPitch)positionMinutes[{team(pl[i].team).squad[pl[i].squad].id,pl[i].detailedPosition}]+=dt*rate*.5f;}
        updateBall(dt);
        record();
        break;
    case MS_PLAY: {
        if (!shootout){clock += dt * rate;if(S.rules==RULESET_CAREER)for(int i=0;i<22;i++)if(pl[i].onPitch)positionMinutes[{team(pl[i].team).squad[pl[i].squad].id,pl[i].detailedPosition}]+=dt*rate;}
        if (penGk >= 0 && stateT > 1.0f) penGk = -1;
        if (shootout) { if (stateT < 0.02f) penMaxZ = 0; if (ball.pos.y > 0.3f && ball.pos.y < PITCH_L - 0.3f) {} else penMaxZ = std::max(penMaxZ, ball.z); }
        assignHumans();
        updatePlayers(dt);
        if (state != MS_PLAY) break;
        updateBall(dt);
        checkOut();
        if (!shootout) offsideUpdate(dt);
        record();
        if (ball.owner >= 0) { possTime[pl[ball.owner].team] += dt; }
        if (shootout) {
            // résolution du tir au but
            bool resolved = false; bool scored = false;
            if (ball.inNet) { resolved = true; scored = true; }
            else if (ball.owner >= 0 && pl[ball.owner].gk) resolved = true;
            else if (stateT > 2.5f || ball.pos.y < -1 || ball.pos.y > PITCH_L + 1 || ball.pos.x < 0 || ball.pos.x > PITCH_W || (ball.vel.len() < 0.5f && stateT > 0.8f)) resolved = true;
            else if ((shootGoal == 0 ? ball.vel.y > 0 : ball.vel.y < 0) && stateT > 0.5f) resolved = true;     // ballon repoussé
            if (resolved) {
                int t = pl[spKicker].team;
                penTaken[t]++;
                if (scored) pens[t]++;
                penLog[t].push_back(scored ? 1 : 0);
                {
                    const char* miss = "RATÉ !";
                    float gx = std::fabs(ball.pos.x - PITCH_W / 2);
                    if (ball.lastTouch >= 0 && pl[ball.lastTouch].gk && pl[ball.lastTouch].team != t) miss = "ARRÊT DU GARDIEN !";
                    else if ((ball.pos.y < 0.5f || ball.pos.y > PITCH_L - 0.5f) && gx < GOAL_W / 2 + 0.3f && penMaxZ > GOAL_H - 0.1f) miss = gx > GOAL_W / 2 - 0.3f ? "SUR LE POTEAU !" : "AU-DESSUS !";
                    else if (gx >= GOAL_W / 2) miss = "À CÔTÉ !";
                    msg = scored ? "BUT !" : miss; msg2 = playerName(spKicker); msgT = 1.6f;
                    lastPenMiss = scored ? 0 : miss[0] == 'A' && miss[1] == 'R' ? 1 : miss[0] == 'A' && miss[1] == 'U' ? 2 : 3;
                }
                if (scored) playSfx(SFX_GOAL); else playSfx(SFX_CROWD_OOH);
                state = MS_STOP; stateT = 0; nextSp = SP_SHOOTOUT;
            }
            break;
        }
        // dernière minute : le 4e arbitre lève le panneau du temps additionnel (buts, cartons, changements, soins)
        if (!boardDone && clock >= periodEnd - 1.f) {
            float pStart = periodEnd - (period < 2 ? (float)S.halfMinutes : 15.f);
            int goals = 0, cards = 0;
            for (auto& e : events) if (e.minute >= pStart && e.minute <= periodEnd) { if (e.type == 0 || e.type == 3) goals++; else cards++; }
            float a = (period < 2 ? 1.f : 0.5f) + goals * 0.5f + cards * 0.35f + subsPeriod * 0.3f + (fightDone ? 1.f : 0.f) + R.frange(0, 1.2f) + (period == 1 ? 1.f : 0.f);
            boardN = std::max(period == 0 ? 0 : 1, std::min(period < 2 ? 8 : 3, (int)std::lround(a)));
            added = (float)boardN;
            boardDone = true; boardT = boardN > 0 ? 5.f : 0.f;
            if (boardN > 0) say(fmt("Le quatrième arbitre annonce %d minute%s de temps additionnel.", boardN, boardN > 1 ? "s" : ""), 3.5f);
        }
        if (boardT > 0) boardT -= dt;
        if (clock - aiTacClock >= 5.f) {
            aiTacClock = clock; aiTactics();
            // énervement : retombe avec le temps, monte chez l'équipe qui perd nettement en fin de match
            for (int i = 0; i < 22; i++) {
                if (!pl[i].onPitch) continue;
                int t = pl[i].team, diff = score[t] - score[1 - t];
                pl[i].anger = std::max(0.f, pl[i].anger - 0.06f);
                if (diff <= -2 && clock > 60) addAnger(i, 0.05f);
                if (pl[i].anger < 0.4f) pl[i].angerLvl = 0;
            }
        }
        // l'arbitre ne siffle pas la fin pendant une action offensive (tolérance d'une minute et demie)
        // ni tant que le ballon est dans une surface de réparation
        if (clock >= periodEnd + added && state == MS_PLAY) {
            bool inBox = std::fabs(ball.pos.x - PITCH_W / 2) < BOX_W / 2 + 0.5f && (ball.pos.y < BOX_L + 0.5f || ball.pos.y > PITCH_L - BOX_L - 0.5f);
            if (!(inBox && clock < periodEnd + added + 4.f) && (!attackOngoing() || clock > periodEnd + added + 1.5f)) endPeriod();
        }
        break;
    }
    case MS_GOAL:
        for (int i = 0; i < 22; i++) {
            MPlayer& p = pl[i];
            if (!p.onPitch) continue;
            V2 want;
            if (i == celebScorer) {
                V2 d = celebTarget - p.pos;
                bool arrived = d.len() <= 0.6f;
                if (celebType == 9 && p.st >= 20.f) arrived = true;
                if (!arrived && p.state != PS_DOWN) {
                    want = d.norm() * (celebType == 8 ? 4.6f : 6.5f); p.face = d.norm(); p.state = PS_CELEB;
                    if (celebType == 3 || celebType == 8) want += d.norm().perp() * (std::sin(stateT * (celebType == 8 ? 3.f : 5.f)) * (celebType == 8 ? 2.f : 3.5f));   // l'avion / le petit train : zigzag
                }
                else if (celebType == 8) { float a = stateT * 1.6f; want = V2(std::cos(a), std::sin(a)) * 3.2f; p.face = want.norm(); }          // le train tourne en rond
                else if (celebType == 9 && p.st < 20.f) { p.vz = 5.6f; p.z = 0.01f; p.st = 20.f; }                                                     // grand saut
                else if (celebType == 9) { if (p.z <= 0.f) p.face = V2(0, 1); }                                                                       // réception face caméra, bras écartés
                else if (celebType == 7 || celebType == 10 || celebType == 12) { p.face = V2(0, 1); }
                else if (celebType == 11 || celebType == 13) {
                    p.face = V2(0, celebTarget.y < PITCH_L / 2 ? -1.f : 1.f);
                    if (celebType == 11 && !celebSfx) { celebSfx = true; playSfx(p.team == 1 ? SFX_FANS_WHISTLE : SFX_BOO); }   // le public adverse siffle le buteur
                }
                else if (celebType == 1 && p.state != PS_SLIDE && p.st < 10.f) { p.state = PS_SLIDE; p.st = 0; p.slideDir = p.face; p.vel = p.face * 4.f; }   // glissade à genoux
                else if (p.state == PS_SLIDE) { want = p.vel * 0.9f; if (p.st > 0.8f) { p.state = PS_CELEB; p.st = 10.f; } }
                else if (celebType == 4 && p.z <= 0.f && p.vz <= 0.f && p.st < 12.f) { p.vz = 5.2f; p.z = 0.01f; p.st = 12.f; }   // salto
                else if (celebType == 5 && p.state != PS_DOWN) { p.state = PS_DOWN; }                                        // la pile humaine : le buteur se couche
                else if (celebType == 6) { float a = stateT * 6.f; p.face = V2(std::cos(a), std::sin(a)); want = V2(std::cos(a * 0.5f), 0) * 1.2f; }   // la danse
                if (p.z > 0.f || p.vz > 0.f) { p.vz -= 10.8f * dt; p.z += p.vz * dt; if (p.z <= 0.f) { p.z = 0.f; p.vz = 0.f; } float a = stateT * 14.f; p.face = V2(std::cos(a), std::sin(a)); }
                p.st += dt;
            } else if (celebScorer >= 0 && p.team == pl[celebScorer].team && !p.gk) {
                // les coéquipiers courent féliciter le buteur (pile humaine : ils se jettent dessus)
                bool pile = celebType == 5 && pl[celebScorer].state == PS_DOWN;
                float spread = pile ? 0.45f : 1.f;
                int rank = 0; for (int j = p.team * 11; j < i; j++) if (pl[j].onPitch && !pl[j].gk && j != celebScorer) rank++;
                V2 spot = pl[celebScorer].pos + V2((i % 3 - 1) * 1.1f * spread, (i % 2 ? 0.9f : -0.9f) * spread);
                const MPlayer& sc = pl[celebScorer];
                if (celebType == 8) { V2 back = sc.vel.len() > 0.3f ? sc.vel.norm() : sc.face; spot = sc.pos - back * (1.05f * (rank + 1)); }          // à la queue leu leu
                else if (celebType == 10) spot = sc.pos + V2((rank % 2 ? 1.f : -1.f) * (1.1f + (rank / 2) * 1.1f), 0.f);                              // alignés pour bercer le bébé
                else if (celebType == 11 && stateT < 2.2f) spot = sc.pos + (spot - sc.pos) * 3.f + V2(0, celebTarget.y < PITCH_L / 2 ? 3.f : -3.f);  // laissent le buteur savourer
                V2 d = spot - p.pos;
                bool lineUp = celebType == 8 || celebType == 10;
                if (d.len() > (pile ? 0.5f : lineUp ? 0.35f : 1.0f) && stateT < 5.6f) { want = d.norm() * std::min(celebType == 8 ? 7.f : 6.f, d.len() * (lineUp ? 3.f : 2.f)); p.face = celebType == 8 ? sc.face : d.norm(); if (p.state == PS_CELEB && celebType != 8) p.state = PS_NORMAL; }
                else if (pile && p.state != PS_DOWN) { p.state = PS_DOWN; p.st = 0; }
                else if (!pile && p.state != PS_CELEB) { p.state = PS_CELEB; p.st = 0; }
                if (celebType == 10 && d.len() <= 0.6f) p.face = V2(0, 1);
            } else if (p.team != (celebScorer >= 0 ? pl[celebScorer].team : -1) && !p.gk) {
                // l'équipe qui encaisse regagne tête basse son camp
                V2 home = fromTeamFrame(p.team, 0.35f, 0.5f);
                V2 d = home - p.pos;
                if (d.len() > 8.f && stateT > 1.0f) { want = d.norm() * 1.6f; p.face = d.norm(); }
            }
            p.vel = p.vel + (want - p.vel) * std::min(1.f, dt * 6);
            p.pos += p.vel * dt;
            p.anim += p.vel.len() * dt;
        }
        updateBall(dt);
        if (stateT > 6.5f) {       // célébration un peu plus longue
            for (auto& q : pl) { if (q.state == PS_DOWN && q.onPitch && !q.injured) q.state = PS_CELEB; q.z = 0; q.vz = 0; }
            startReplay();
        }
        break;
    case MS_REPLAY:
        if (offReplay) {   // ralenti du hors-jeu : demi-vitesse, arrêt sur image au moment de la passe, puis coup franc indirect
            if (stateT > 9) { offReplay = false; state = MS_STOP; stateT = 0; break; }      // passé par le joueur
            if (rpPos == offRpIdx && offRepHold < 2.2f) { offRepHold += dt; break; }
            static float halfStep = 0; halfStep += 0.5f;
            if (halfStep >= 1.f) { halfStep -= 1.f; rpPos = (rpPos + 1) % REPLAY_N; }
            if (rpPos == rpHead || stateT > 9 || (offRepHold >= 2.2f && (rpPos - offRpIdx + REPLAY_N) % REPLAY_N > 70)) { offReplay = false; state = MS_STOP; stateT = 0; }
            break;
        }
        rpPos = (rpPos + 1) % REPLAY_N;
        if (rpPos == rpHead || stateT > 7) {
            for (auto& p : pl) if (p.state == PS_CELEB) p.state = PS_NORMAL;
            if (S.goldenGoal && period >= 2 && !shootout) { aet = true; finishMatch(); break; }
            beginSetPiece(SP_KICKOFF, kickoffTeam, V2(PITCH_W / 2, PITCH_L / 2));
            playSfx(SFX_WHISTLE);
        }
        break;
    case MS_STOP:
        for (auto& p : pl) { p.vel = p.vel * (1 - 4 * dt); p.pos += p.vel * dt; p.st += dt; if (p.state == PS_SLIDE && p.st > 0.6f) p.state = PS_NORMAL; if (p.state == PS_DOWN && p.st > 1.5f) p.state = PS_NORMAL; }
        ball.vel = ball.vel * (1 - 3 * dt); ball.pos += ball.vel * dt;
        if (ball.z > 0) { ball.vz -= GRAV * dt; ball.z = std::max(0.f, ball.z + ball.vz * dt); }
        for (auto& p : pl) if (p.onPitch && p.pos.x < 0 && subBoardT > 0) { V2 d = p.target - p.pos; if (d.len() > 0.3f) { p.vel = d.norm() * 3.f; p.pos += p.vel * dt; p.anim += 3.f * dt; } }
        if (stateT > 0.6f && !autoSubDone && nextSp != SP_SHOOTOUT) { autoSubDone = true; autoSubs(0); autoSubs(1); }
        if (stateT > 1.6f && pendCardOff < 0 && subBoardT <= 0 && pendSubs.empty()) {
            if (nextSp == SP_SHOOTOUT) { shootoutNext(); break; }
            beginSetPiece(nextSp, nextSpTeam, nextSpPos);
            if (nextSp == SP_PENALTY) playSfx(SFX_CROWD_OOH);
        }
        break;
    case MS_WALKOUT: {
        walkT += dt;
        bool all = true;
        for (int i = 0; i < 22; i++) {
            MPlayer& p = pl[i];
            if (!p.onPitch) continue;
            if (walkT < 0.4f + p.slot * 0.3f) { all = false; continue; }
            V2 tg = p.pos.x < 0.8f ? V2(1.5f, PITCH_L / 2 + (p.team ? 1.f : -1.f)) : p.target;
            V2 d = tg - p.pos;
            if (d.len() > 0.25f || p.pos.x < 0.8f) { all = false; p.vel = d.norm() * std::min(5.5f, d.len() * 3 + 0.5f); p.pos += p.vel * dt; p.anim += p.vel.len() * dt; p.face = d.norm(); }
            else { p.vel = V2(); p.pos = p.target; p.face = (ball.pos - p.pos).norm(); }
            p.state = PS_NORMAL;
        }
        refMove(V2(PITCH_W / 2 - 6, PITCH_L / 2 + 4), 3.8f, (ball.pos - refPos).norm(), dt);
        if (all || walkT > 14.f) { state = MS_SETPIECE; stateT = 0; spT = 0; spReady = false; playSfx(SFX_WHISTLE); }
        break;
    }
    case MS_BREAK:
        if (period == 0 && nextSp == 1 && !S.training) {
            // mi-temps : retour au vestiaire, écran de mi-temps, puis reprise
            walkToTunnel(dt, 6.5f);
            bool gone = true; for (auto& p : pl) if (p.onPitch && p.pos.x > -1.4f) gone = false;
            if ((gone && stateT > 2.f) || stateT > 9.f) {
                htWaiting = true;
                if (!S.halftimeScreen || htGo) startPeriod(1);
            }
            break;
        }
        // mi-temps de la prolongation : les joueurs changent de côté à pied
        if (nextSp == 3 && !S.training) {
            if (!swapInit) { swapInit = true; for (int i = 0; i < 22; i++) swapTarget[i] = V2(pl[i].pos.x, PITCH_L - pl[i].pos.y); msg = "CHANGEMENT DE CÔTÉ"; msg2 = ""; msgT = 2.5f; }
            for (int i = 0; i < 22; i++) {
                MPlayer& p = pl[i];
                if (!p.onPitch) continue;
                V2 d = swapTarget[i] - p.pos;
                if (d.len() > 0.3f && stateT > 0.8f) { p.vel = d.norm() * std::min(4.5f, d.len() * 2); p.pos += p.vel * dt; p.anim += p.vel.len() * dt; p.face = d.norm(); } else p.vel = V2();
                p.state = PS_NORMAL;
            }
            if (stateT < 6.f) break;
        }
        // tirage au sort : avant la prolongation (ballon ou côté) et avant les tirs au but (qui tire en premier)
        if (stateT > 2.0f && !S.training && ((nextSp == 2 && !tossDoneET) || (nextSp == 4 && !tossDoneTAB))) {
            if (tossKind == 0) {
                tossKind = nextSp == 2 ? 1 : 2; cerT = 0; tossWinner = -1; tossChoice = -1; tossCall = R.range(0, 1); tossResult = R.range(0, 1);
                tossUI = 2; tossSideTeam = -1; tossDir = -1; sideT = 0;
                updateCaptain(0); updateCaptain(1);
                say(nextSp == 2 ? "Nouveau tirage au sort avant la prolongation." : "Tirage au sort avant la séance de tirs au but.", 2.5f, true);
                playSfx(SFX_WHISTLE);
            }
            if (updateMiniToss(dt)) break;
            if (nextSp == 2) tossDoneET = true; else tossDoneTAB = true;
            stateT = 2.9f;
        }
        if (stateT > 3.0f) {
            if (nextSp >= 1 && nextSp <= 3) startPeriod(nextSp);
            else if (nextSp == 4) {
                shootout = true;
                if (!tossDoneTAB) shootTeam = R.range(0, 1);
                pens[0] = pens[1] = 0; penTaken[0] = penTaken[1] = 0;
                shootoutSetup();
            } else finishMatch();
        }
        break;
    default: break;
    }
    // caméra
    V2 focus = ball.pos + ball.vel * 0.25f;
    if (state == MS_SETPIECE) focus = ball.pos;
    if (state == MS_WALKOUT) focus = V2(walkT < 4.f ? 12.f : PITCH_W / 2, PITCH_L / 2);
    if (state == MS_BREAK && period == 0 && stateT > 1.f) focus = V2(12.f, PITCH_L / 2);
    if (state == MS_GOAL && celebScorer >= 0 && stateT > 0.6f) focus = pl[celebScorer].pos;          // la caméra suit la célébration
    if (state == MS_STOP && subBoardT > 0) focus = V2(4.f, PITCH_L / 2);                                // et le remplacement
    bool cardZoom = refCardT > 0.f && refCardT < (refCardSecond ? 3.5f : 2.1f) && refCardFor >= 0 && state != MS_PLAY && !S.highlights;   // gros plan sur l'arbitre qui sort le carton
    if (!cardZoom && pendCardOff >= 0 && pendCardT > 0.6f && state != MS_PLAY && !S.highlights) { cardZoom = true; focus = (refPos + pl[pendCardOff].pos) * 0.5f; }
    else if (cardZoom) focus = (refPos + pl[refCardFor].pos) * 0.5f;
    if (!ceremony) camZoom = camZoom + ((cardZoom ? 2.0f : 1.f) - camZoom) * std::min(1.f, dt * 3.f);
    cam = cam + (focus - cam) * std::min(1.f, dt * (cardZoom ? 6.f : 4.f));
}

// ------------------------------------------------------------------ cérémonie de remise du trophée
static std::string trArticle(const std::string& t) {
    auto st = [&](const char* p) { return t.rfind(p, 0) == 0; };
    if (st("Coupe") || st("Ligue des") || st("Supercoupe") || st("Copa") || st("Gold")) return "la " + t;
    if (st("Trophée") || st("Championnat") || st("Mondial")) return "le " + t;
    if (!t.empty() && strchr("AEIOUaeiou", t[0])) return "l'" + t;
    return "le trophée de " + t;
}

int Match::trCaptain(int t) const {
    if (captain[t] >= 0 && pl[captain[t]].onPitch) return captain[t];
    for (int i = t * 11; i < t * 11 + 11; i++) if (pl[i].onPitch && !pl[i].gk) return i;
    for (int i = t * 11; i < t * 11 + 11; i++) if (pl[i].onPitch) return i;
    return t * 11;
}

bool Match::trTeamDone(int t) const {
    for (int i = t * 11; i < t * 11 + 11; i++) if (pl[i].onPitch && (!trRoute[i].empty() || trDelay[i] > 0 || trWait[i] > 0)) return false;
    return true;
}

void Match::startTrophy(int t, int kind, int style, const std::string& title) {
    trophyActive = true; trophyTeam = t; trophyKind = kind; trophyStyle = style; trophyTitle = title;
    trTotal = 0; trHolder = -1; trLift = false; trOnTable = true;
    trPod = V2(8.4f, PITCH_L / 2);
    for (int i = 0; i < 22; i++) {
        medal[i] = false; trRoute[i].clear(); trDelay[i] = trWait[i] = 0; trSpeed[i] = 3.f; trFace[i] = V2(-1, 0);
        if (pl[i].onPitch) { pl[i].state = PS_NORMAL; pl[i].vel = V2(); pl[i].z = 0; }
    }
    walkers.clear(); subBoardT = 0; boardT = 0; pendCardOff = -1; pendSubs.clear();
    ball.owner = -1; ball.vel = V2(); ball.vz = 0; ball.z = 0; ball.inNet = false; ball.pos = V2(-30, -30);
    refPos = V2(4.6f, trPod.y + 10.5f); refVel = V2(); refFace = V2(1, 0);
    msgT = 0;
    trBegin(0);
}

void Match::endTrophy() {
    trophyActive = false; trLift = false;
    stateT = 0; msgT = 0; comT = 0;
}

void Match::trBegin(int ph) {
    trPhase = ph; trT = 0;
    const int W = trophyTeam, L = 1 - W;
    const float c = trPod.y;
    const V2 E(9.4f, c + 7.4f), M(9.4f, c), X(9.4f, c - 7.4f);
    auto members = [&](int t, bool capLast) {
        std::vector<int> v; int cap = trCaptain(t);
        for (int i = t * 11; i < t * 11 + 11; i++) if (pl[i].onPitch && !(capLast && i == cap)) v.push_back(i);
        if (capLast && pl[cap].onPitch) v.push_back(cap);
        return v;
    };
    auto zoneL = [&](int k) { return V2(20.f + (k % 4) * 1.9f, c - 6.5f - (k / 4) * 1.9f); };
    auto set = [&](int i, std::vector<TrStep> r, float delay, float spd, V2 face) { trRoute[i] = r; trDelay[i] = delay; trSpeed[i] = spd; trFace[i] = face; };
    switch (ph) {
    case 0: {
        auto wl = members(W, true), ll = members(L, false);
        for (int k = 0; k < (int)ll.size(); k++) {
            if (trophyKind == 0) set(ll[k], { { V2(12.4f + k * 1.3f, c + 9.4f), 0 } }, k * 0.1f, 2.2f, V2(-1, 0));
            else set(ll[k], { { zoneL(k), 0 } }, k * 0.1f, 2.2f, V2(-1, 0));
        }
        for (int k = 0; k < (int)wl.size(); k++) {
            if (trophyKind == 0) set(wl[k], { { V2(30.f + (k % 4) * 2.1f, c + 6.5f + (k / 4) * 2.2f), 0 } }, 0, 4.f, V2(-1, 0));
            else set(wl[k], { { V2(12.4f + k * 1.3f, c + 9.4f), 0 } }, k * 0.1f, 3.f, V2(-1, 0));
        }
        if (trophyTier == 0) say(fmt("Champion et promu ! Le délégué du district remet la coupe à %s, au bord du terrain, entre amis et bénévoles.", team(W).name.c_str()), 5.f, true);
        else if (trophyTier == 1) say(fmt("%s est sacré au niveau régional ! La ligue a installé une petite estrade pour la remise de %s.", team(W).name.c_str(), trArticle(trophyTitle).c_str()), 5.f, true);
        else if (trophyTier == 4) say(fmt("Nuit européenne au %s ! Les projecteurs s'éteignent, place à la cérémonie : %s va soulever %s !", S.stadium.c_str(), team(W).name.c_str(), trArticle(trophyTitle).c_str()), 5.f, true);
        else if (trophyKind == 0) say(fmt("Place à la remise des trophées au %s ! %s va recevoir %s.", S.stadium.c_str(), team(W).name.c_str(), trArticle(trophyTitle).c_str()), 5.f, true);
        else say(fmt("C'est officiel, %s est champion ! Place à la remise du trophée de %s.", team(W).name.c_str(), trophyTitle.c_str()), 5.f, true);
        break;
    }
    case 1: {   // médailles des finalistes
        auto ll = members(L, false);
        for (int k = 0; k < (int)ll.size(); k++)
            set(ll[k], { { E, 0 }, { M, 1 }, { X, 0 }, { V2(14.f, c - 9.f), 0 }, { zoneL(k), 0 } }, k * 0.5f, 3.4f, V2(1, 0));
        auto wl = members(W, true);
        for (int k = 0; k < (int)wl.size(); k++) set(wl[k], { { V2(13.f + k * 1.3f, c + 11.6f), 0 } }, 2.f + k * 0.15f, 3.f, V2(-1, 0));
        say("Les joueurs de " + team(L).name + " montent chercher leur médaille de finaliste. La déception se lit sur les visages.", 5.f, true);
        break;
    }
    case 2: {   // médailles des vainqueurs puis trophée remis au capitaine
        auto wl = members(W, true);
        std::vector<V2> spots;
        for (int k = 0; k < 5; k++) spots.push_back(V2(10.4f, c - 4.f + k * 2.f));
        for (int k = 0; k < 6; k++) spots.push_back(V2(8.9f, c - 5.f + k * 2.f));
        std::vector<int> order = { 2, 1, 3, 0, 4, 5, 10, 6, 9, 7, 8 };   // le capitaine au centre du premier rang
        int cap = trCaptain(W);
        int si = 1;
        for (int k = 0; k < (int)wl.size(); k++) {
            int i = wl[k];
            V2 sp = i == cap ? spots[order[0]] : spots[order[std::min(10, si++)]];
            set(i, { { E, 0 }, { M, i == cap ? 2 : 1 }, { sp, 0 } }, k * 0.5f, 3.2f, V2(1, 0));
        }
        say("Au tour de " + team(W).name + " ! Les joueurs reçoivent leur médaille sous les acclamations.", 5.f, true);
        break;
    }
    case 3:
        say(fmt("%s reçoit le trophée des mains des officiels...", playerName(trCaptain(W)).c_str()), 3.f, true);
        break;
    case 4: {   // haie d'honneur
        auto ll = members(L, false);
        for (int k = 0; k < (int)ll.size(); k++) {
            bool up = k % 2 == 0;
            set(ll[k], { { V2(15.f + (k / 2) * 1.8f, c + (up ? -2.4f : 2.4f)), 0 } }, 0, 3.f, V2(0, up ? 1.f : -1.f));
        }
        say("Beau geste de " + team(L).name + " qui forme une haie d'honneur pour saluer le vainqueur.", 5.f, true);
        break;
    }
    case 5: {   // tour d'honneur
        auto wl = members(W, false);
        int cap = trCaptain(W);
        std::vector<int> v = { cap }; for (int i : wl) if (i != cap) v.push_back(i);
        for (int k = 0; k < (int)v.size(); k++) {
            V2 o((k % 4 - 1.5f) * 1.4f, (k / 4) * 1.5f);
            set(v[k], { { V2(PITCH_W - 3.f, c - 2.f) + o, 0 }, { V2(PITCH_W - 3.f, PITCH_L - 4.f) + o, 0 }, { V2(3.f, PITCH_L - 4.f) + o, 0 } }, 0.2f + k * 0.06f, 3.6f, V2(0, 1));
        }
        auto ll = members(L, false);
        for (int k = 0; k < (int)ll.size(); k++) set(ll[k], { { V2(-1.6f, c + 3.f + (k % 3) * 0.8f), 0 } }, 1.f + k * 0.3f, 1.8f, V2(-1, 0));
        say("Tour d'honneur pour " + team(W).name + " ! Le trophée passe de main en main devant des tribunes en fusion.", 5.f, true);
        break;
    }
    }
}

void Match::updateTrophy(float dt) {
    trT += dt; trTotal += dt; stateT += dt;
    if (comT > 0) comT -= dt;
    if (comCool > 0) comCool -= dt;
    const int W = trophyTeam;
    const float c = trPod.y;
    for (int i = 0; i < 22; i++) {
        MPlayer& p = pl[i];
        if (!p.onPitch) continue;
        bool win = p.team == W;
        if (trDelay[i] > 0) { trDelay[i] -= dt; }
        else if (trWait[i] > 0) { trWait[i] -= dt; p.vel = V2(); p.state = PS_NORMAL; p.face = V2(-1, 0); continue; }
        else if (!trRoute[i].empty()) {
            TrStep st = trRoute[i].front();
            V2 d = st.p - p.pos;
            float spd = trSpeed[i];
            if (d.len() < 0.25f) {
                p.pos = st.p; trRoute[i].erase(trRoute[i].begin());
                if (st.act >= 1) { medal[i] = true; trWait[i] = st.act == 2 ? 1.6f : 0.55f; if (st.act == 2) playSfx(SFX_GOAL); }
                if (st.act == 2) { trHolder = i; trOnTable = false; }
            } else {
                p.vel = d.norm() * std::min(spd, d.len() * 4.f);
                p.pos += p.vel * dt; p.anim += p.vel.len() * dt; p.face = d.norm();
                p.state = (trPhase == 5 && win && i == trHolder) ? PS_THROW : PS_NORMAL;
                continue;
            }
        }
        // à l'arrêt
        p.vel = V2();
        p.face = trFace[i];
        bool celeb = win && ((trPhase == 0 && trophyKind == 0) || trPhase == 3 || (trPhase == 4 && trT > 1.f) || trPhase == 5);
        if (i == trHolder && trLift) p.state = PS_THROW;
        else if (celeb) { p.state = PS_CELEB; p.anim += dt * 3; }
        else p.state = PS_NORMAL;
    }
    // enchaînement des phases
    switch (trPhase) {
    case 0: {
        bool ok = trT > 3.5f && ((trTeamDone(0) && trTeamDone(1)) || trT > 9.f);
        if (ok) trBegin(trophyKind == 0 ? 1 : 2);
        break;
    }
    case 1: {
        int n = 0, got = 0; for (int i = (1 - W) * 11; i < (1 - W) * 11 + 11; i++) if (pl[i].onPitch) { n++; if (medal[i]) got++; }
        static float allT = 0; if (trT < dt * 1.5f) allT = 0;
        if (got >= n) allT += dt;
        if (allT > 1.5f || trT > 30.f) trBegin(2);
        break;
    }
    case 2: if ((trTeamDone(W) && trT > 4.f) || trT > 32.f) trBegin(3); break;
    case 3:
        if (!trLift && trT > 1.4f) {
            trLift = true; if (trHolder < 0) { trHolder = trCaptain(W); trOnTable = false; }
            playSfx(SFX_GOAL);
            say(trophyKind == 1 ? fmt("%s soulève le trophée ! %s est sacré champion de %s !", playerName(trHolder).c_str(), team(W).name.c_str(), trophyTitle.c_str())
                                : fmt("%s soulève le trophée ! %s remporte %s !", playerName(trHolder).c_str(), team(W).name.c_str(), trArticle(trophyTitle).c_str()), 5.f, true);
        }
        if (trT > 7.5f) trBegin(4);
        break;
    case 4: {
        // les vainqueurs descendent du podium et passent entre les deux haies une fois celles-ci formées
        static bool launched = false;
        if (trT < dt * 1.5f) launched = false;
        if (!launched && (trTeamDone(1 - W) || trT > 7.f)) {
            launched = true;
            int cap = trCaptain(W);
            std::vector<int> v; if (pl[cap].onPitch) v.push_back(cap);
            for (int i = W * 11; i < W * 11 + 11; i++) if (pl[i].onPitch && i != cap) v.push_back(i);
            for (int k = 0; k < (int)v.size(); k++) {
                V2 o(28.f + (k % 4) * 1.7f, c - 2.5f + (k / 4) * 2.f);
                trRoute[v[k]] = { { V2(12.f, c), 0 }, { V2(26.f, c), 0 }, { o, 0 } };
                trDelay[v[k]] = k * 0.45f; trSpeed[v[k]] = 2.3f; trFace[v[k]] = V2(0, 1);
            }
        }
        if ((launched && trTeamDone(W) && trT > 4.f) || trT > 30.f) trBegin(5);
        break;
    }
    case 5: if (trT > 14.f) endTrophy(); break;
    }
    // caméra : le podium, puis le capitaine pendant le tour d'honneur
    V2 focus(18.f, c - 3.f);
    if (trPhase == 3) focus = V2(12.f, c);
    if (trPhase == 5 && trHolder >= 0) focus = pl[trHolder].pos;
    cam = cam + (focus - cam) * std::min(1.f, dt * 2.5f);
}

// mode Full Manager : action dangereuse, arrêt de jeu, but, tirs au but...
bool Match::hotPhase() const {
    if (state == MS_GOAL || state == MS_REPLAY || state == MS_SHOOTOUT || state == MS_END) return true;
    if (state == MS_SETPIECE) return ball.pos.y < 30.f || ball.pos.y > PITCH_L - 30.f;
    if (state != MS_PLAY) return false;
    if (ball.pos.y < 36.f || ball.pos.y > PITCH_L - 36.f) return true;
    if (ball.z > 0.3f && (ball.pos.y < 40.f || ball.pos.y > PITCH_L - 40.f)) return true;
    return false;
}

// ------------------------------------------------------------------ soigneurs : entrent sur le terrain pour soigner un joueur blessé
void Match::updateMedics(float dt) {
    if (S.training || ceremony) return;
    if (medicFor < 0) {
        if (state != MS_STOP) return;
        for (int i = 0; i < 22; i++) if (pl[i].onPitch && pl[i].injured && !medicDone[i]) {
            medicFor = i; medicPhase = 0; medicT = 0; medicDone[i] = true;
            V2 bench(-2.4f, PITCH_L / 2 + (pl[i].team ? 6.f : -6.f));
            medicPos[0] = bench; medicPos[1] = bench + V2(-0.8f, 0.6f);
            say("Les soigneurs entrent sur la pelouse pour s'occuper de " + playerName(i) + ".", 3.f, true);
            break;
        }
        return;
    }
    MPlayer& p = pl[medicFor];
    medicT += dt;
    V2 bench(-2.4f, PITCH_L / 2 + (p.team ? 6.f : -6.f));
    auto moveTo = [&](V2& m, V2 tg, float sp) { V2 d = tg - m; float l = d.len(); if (l > 0.05f) m += d * (std::min(l, sp * dt) / l); return l < 0.3f; };
    if (medicPhase == 0) {          // course vers le joueur
        bool a = moveTo(medicPos[0], p.pos + V2(-0.7f, 0.2f), 5.5f), b = moveTo(medicPos[1], p.pos + V2(0.7f, 0.3f), 5.2f);
        p.state = PS_DOWN; p.st = 0; p.vel = V2();
        if ((a && b) || medicT > 12.f) { medicPhase = 1; medicT = 0; }
    } else if (medicPhase == 1) {   // soins
        p.state = PS_DOWN; p.st = 0; p.vel = V2();
        if (medicT > 3.2f) { medicPhase = 2; medicT = 0; p.state = PS_NORMAL; }
    } else {                        // retour au banc (le joueur sort avec eux s'il doit être remplacé)
        bool a = moveTo(medicPos[0], bench, 4.f), b = moveTo(medicPos[1], bench + V2(-0.8f, 0.6f), 4.f);
        if ((a && b) || medicT > 12.f) { medicFor = -1; medicPhase = 0; }
    }
    if (medicPhase < 2 && state == MS_STOP) stateT = std::min(stateT, 0.3f);     // le jeu reprend quand les soins sont terminés
}
