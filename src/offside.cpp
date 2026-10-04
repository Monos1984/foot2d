// Hors-jeu et arbitres assistants (build 19)
//
// 1. Vérité : quand un joueur joue volontairement le ballon, on photographie la position de ses partenaires
//    (ligne = max(milieu de terrain, avant-dernier adversaire, ballon)). Marge en mètres : > 0 hors-jeu, < 0 en jeu.
// 2. Perception : l'arbitre assistant responsable de la moitié de terrain juge chaque marge. Une erreur n'est possible
//    que sur une position serrée (|marge| < OFFSIDE_OBVIOUS), surtout sous OFFSIDE_TIGHT ; jamais sur une position flagrante.
// 3. Participation : un joueur signalé n'est sanctionné que s'il joue le ballon, dispute le ballon à un adversaire,
//    gêne le gardien ou profite d'un rebond (parade, poteau, déviation). Un joueur passif loin de l'action n'est pas sanctionné.
// 4. La phase se termine par : nouveau jeu d'un partenaire, jeu volontaire d'un adversaire, sortie, faute, but, coup de pied arrêté.
//    Une parade ou une déviation d'un adversaire ne la termine PAS.
#include "match.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <algorithm>

uint64_t offsideHash(uint64_t a, uint64_t b) {
    uint64_t h = a * 0x9E3779B97F4A7C15ULL ^ (b + 0x7F4A7C159E3779B9ULL + (a << 6) + (a >> 2));
    h ^= h >> 33; h *= 0xff51afd7ed558ccdULL; h ^= h >> 33; h *= 0xc4ceb9fe1a85ec53ULL; h ^= h >> 33;
    return h;
}
static inline float clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }
static float hashUnit(uint64_t h) { return (float)((h >> 11) & 0xFFFFFF) / (float)0x1000000; }   // [0,1)
static float hashGauss(uint64_t h) {                                                                 // loi normale centrée réduite
    float u1 = std::max(1e-6f, hashUnit(h)), u2 = hashUnit(offsideHash(h, 77));
    return std::sqrt(-2.f * std::log(u1)) * std::cos(6.2831853f * u2);
}

bool offsideDecision(const AssistantReferee& ar, float margin, float lineSpeed, float clockMin, uint64_t seed, float* perceived) {
    bool truth = margin > OFFSIDE_ENGINE_EPSILON;
    float am = std::fabs(margin);
    if (am >= OFFSIDE_OBVIOUS) { if (perceived) *perceived = margin; return truth; }     // flagrant : toujours juste
    // amplitude de l'erreur de perception (m) : précision, placement (parallaxe), expérience
    float sigma = 0.02f + (100 - ar.offsideAccuracy) * 0.0009f + (100 - ar.positioning) * 0.0004f + (100 - ar.experience) * 0.0002f;
    sigma *= 1.f + std::min(0.4f, std::max(0.f, lineSpeed) * 0.05f);                     // ligne qui se croise vite
    if (clockMin > 75.f && ar.concentration < 60) sigma *= 1.f + std::min(0.15f, (60 - ar.concentration) * 0.004f);   // fin de match : effet très faible
    float p = margin + sigma * hashGauss(seed);
    if (perceived) *perceived = p;
    bool flag = p > 0.f;
    // zone serrée (15 à 40 cm) : une erreur reste exceptionnelle, même pour un assistant faible
    if (flag != truth && am >= OFFSIDE_TIGHT && hashUnit(offsideHash(seed, 31)) > 0.25f) flag = truth;
    return flag;
}

// ------------------------------------------------------------------ équipe arbitrale
void Match::makeAssistants() {
    static const char* FN[] = { "Julien", "Nicolas", "Karim", "Thomas", "Mehdi", "Sophie", "Claire", "Antoine", "Benoît", "Yannick", "Hervé", "Laura", "Stéphane", "Rachid", "Marion", "Quentin" };
    static const char* LN[] = { "Martin", "Roux", "Benali", "Garnier", "Lefèvre", "Mercier", "Fontaine", "Chevalier", "Perrot", "Hamel", "Guillou", "Ferrand", "Brunet", "Tanguy", "Maillard", "Ollivier" };
    uint64_t h = 1469598103934665603ULL;
    auto mixStr = [&](const std::string& s) { for (unsigned char c : s) h = (h ^ c) * 1099511628211ULL; };
    mixStr(S.kickoffDate); mixStr(S.kickoffTime); mixStr(S.title);
    offSeed = offsideHash(offsideHash(h, (uint64_t)(unsigned)S.home * 7919u + (unsigned)S.away), (uint64_t)(unsigned)(S.referee + 1));
    int level = S.referee >= 0 && S.referee < NUM_REFEREES ? REFEREES[S.referee].level : 3;          // 1 à 5
    int cons = S.referee >= 0 && S.referee < NUM_REFEREES ? REFEREES[S.referee].consistency : 60;
    for (int a = 0; a < 2; a++) {
        AssistantReferee& A = assistants[a];
        uint64_t s = offsideHash(offSeed, 1000 + a);
        auto stat = [&](int k, int base) {          // niveau de la compétition + grande variété individuelle (un excellent assistant de district reste possible)
            float g = hashGauss(offsideHash(s, k));
            return std::max(25, std::min(99, (int)(base + level * 5 + g * 11)));
        };
        A.id = (int)(s % 100000);
        snprintf(A.name, sizeof A.name, "%s %s", FN[s % 16], LN[(s >> 8) % 16]);
        A.offsideAccuracy = stat(1, 50 + (cons - 60) / 4);
        A.positioning = stat(2, 50);
        A.concentration = stat(3, 52);
        A.experience = stat(4, 40);
        A.communication = stat(5, 50);
        A.decisiveness = stat(6, 50);
        A.reputation = std::max(5, std::min(99, (A.offsideAccuracy + A.experience) / 2 + level * 3 - 10));
    }
}
int Match::offsideAssistantFor(int t) const { return goalCenter(t).y > PITCH_L / 2 ? 0 : 1; }

// ------------------------------------------------------------------ géométrie
float Match::offsideLine(int t) const {
    int ot = 1 - t;
    float b1 = 0, b2 = 0;
    for (int j = ot * 11; j < ot * 11 + 11; j++) {
        if (!pl[j].onPitch) continue;
        float pr = progress(t, pl[j].pos);
        if (pr > b1) { b2 = b1; b1 = pr; } else if (pr > b2) b2 = pr;
    }
    return std::max(0.5f, std::max(b2, progress(t, ball.pos)));
}
float Match::offsideMarginMeters(int j) const {
    int t = pl[j].team;
    return (progress(t, pl[j].pos) - offsideLine(t)) * PITCH_L;
}

// ------------------------------------------------------------------ journal (OFFSIDE DEBUG)
void Match::offsideLog(const char* event, int player, const OffsideCand* c, int reason) {
    static int on = -1;
    if (on < 0) on = getenv("FOOT_OFFSIDE_DEBUG") ? 1 : 0;
    if (!on) return;
    FILE* f = fopen("offside_debug.log", "a");
    if (!f) return;
    const AssistantReferee& A = assistants[off.assistant & 1];
    fprintf(f, "[%5.1f'] seq=%d %s team=%d source=%d player=%d ball=(%.1f,%.1f) line=%.3f", clock, off.seq, event, off.team, off.source, player, off.ballAtPlay.x, off.ballAtPlay.y, off.line * PITCH_L);
    if (c) fprintf(f, " margin=%+.3f truth=%d perceived=%+.3f flagged=%d speed=%.1f", c->margin, c->truth, c->perceived, c->flagged, c->lineSpeed);
    fprintf(f, " AR%d=%s acc=%d pos=%d reason=%d\n", (off.assistant & 1) + 1, A.name, A.offsideAccuracy, A.positioning, reason);
    fclose(f);
}

// ------------------------------------------------------------------ phases
void Match::offsideReset(int reason) {
    if (off.active) offsideLog("reset", -1, nullptr, reason);
    off.active = false; off.cands.clear(); off.rebound = false;
}

void Match::offsidePhotograph(int kicker, int restart) {
    int t = pl[kicker].team;
    offsideReset(OR_NEW_TEAMMATE_PLAY);
    if (restart == SP_THROWIN || restart == SP_CORNER || restart == SP_GOALKICK) {   // réception directe : pas de hors-jeu
        offsideReset(OR_EXEMPT_RESTART);
        return;
    }
    off.active = true; off.team = t; off.source = kicker; off.seq = ++offSeq; off.restart = restart;
    off.assistant = offsideAssistantFor(t); off.ballAtPlay = ball.pos; off.line = offsideLine(t); off.rebound = false;
    offRpIdx = rpHead; offLineY = attackDir[t] < 0 ? PITCH_L * (1.f - off.line) : PITCH_L * off.line;   // pour le ralenti
    // avant-dernier défenseur (vitesse de la ligne)
    int ot = 1 - t, d1 = -1, d2 = -1; float b1 = -1, b2 = -1;
    for (int j = ot * 11; j < ot * 11 + 11; j++) {
        if (!pl[j].onPitch) continue;
        float pr = progress(t, pl[j].pos);
        if (pr > b1) { b2 = b1; d2 = d1; b1 = pr; d1 = j; } else if (pr > b2) { b2 = pr; d2 = j; }
    }
    float sgn = attackDir[t] < 0 ? -1.f : 1.f;
    for (int j = t * 11; j < t * 11 + 11; j++) {
        if (j == kicker || !pl[j].onPitch) continue;
        float pr = progress(t, pl[j].pos);
        if (pr <= 0.5f) continue;                                  // dans sa moitié : jamais hors-jeu
        float margin = (pr - off.line) * PITCH_L;
        if (margin < -OFFSIDE_OBVIOUS - 0.05f) continue;          // largement en jeu
        OffsideCand c; c.player = j; c.margin = margin; c.truth = margin > OFFSIDE_ENGINE_EPSILON;
        c.lineSpeed = d2 >= 0 ? std::fabs((pl[j].vel.y - pl[d2].vel.y) * sgn) : 0.f;
        uint64_t seed = offsideHash(offsideHash(offSeed, (uint64_t)off.seq), (uint64_t)(assistants[off.assistant].id * 31 + j));
        c.flagged = offsideDecision(assistants[off.assistant], margin, c.lineSpeed, clock, seed, &c.perceived);
        off.cands.push_back(c);
        offsideLog("photo", j, &c, 0);
    }
    (void)d1;
}

void Match::armOffside(int kicker) { offsidePhotograph(kicker, -1); }

void Match::offsideOpponentTouch(int i, int type) {
    if (!off.active || pl[i].team == off.team) return;
    if (type == OT_DELIBERATE) { offsideReset(OR_DELIBERATE_OPPONENT); return; }
    off.rebound = true;            // parade ou déviation : la phase continue, l'attaquant qui en profite reste hors-jeu
    offsideLog(type == OT_SAVE ? "save" : "deflection", i, nullptr, 0);
}

// toucher du ballon pendant le jeu (humain ou IA, mêmes règles)
void Match::checkOffsideTouch(int i, bool deliberate) {
    if (!off.active) return;
    if (pl[i].team != off.team) { offsideOpponentTouch(i, deliberate ? OT_DELIBERATE : OT_DEFLECTION); return; }
    for (auto& c : off.cands) if (c.player == i) {
        offsideLog("touch", i, &c, 0);
        if (c.truth || c.flagged) { offDecisions++; if (std::fabs(c.margin) < OFFSIDE_OBVIOUS) offClose++; if (c.truth != c.flagged) offMistakes++; }
        if (c.flagged && state == MS_PLAY) { offsideSanction(i, off.rebound ? OI_GAINING_ADVANTAGE : OI_PLAYING_BALL, c.margin); return; }
        break;
    }
    offsideReset(OR_NEW_TEAMMATE_PLAY);       // toucher d'un partenaire en jeu : la phase est close (son prochain jeu en ouvrira une)
}

// ------------------------------------------------------------------ participation active sans toucher le ballon
bool Match::interferesWithGoalkeeper(int j) const {
    int t = pl[j].team, g = -1;
    for (int k = (1 - t) * 11; k < (1 - t) * 11 + 11; k++) if (pl[k].onPitch && pl[k].gk) g = k;
    if (g < 0) return false;
    V2 gp = pl[g].pos, pp = pl[j].pos, bp = ball.pos;
    float dg = (pp - gp).len();
    if (dg > 7.f) return false;
    // obstruction / contact : collé au gardien pendant qu'il tente de jouer le ballon
    bool gkActs = pl[g].state == PS_DIVE || (bp - gp).len() < 3.f;
    if (dg < 1.1f && gkActs) return true;
    // masque la vision : sur la trajectoire ballon-gardien, entre les deux
    V2 ab = gp - bp; float L2 = ab.len2();
    if (L2 < 1.f) return false;
    float u = (pp - bp).dot(ab) / L2;
    if (u < 0.15f || u > 0.97f) return false;
    float dist = (pp - (bp + ab * u)).len();
    return dist < 0.75f + 0.25f * (1.f - u);         // plus proche du gardien : gêne plus large
}

int Match::offsideInvolvement(int j, bool touching) const {
    if (touching) return off.rebound ? OI_GAINING_ADVANTAGE : OI_PLAYING_BALL;
    const MPlayer& p = pl[j];
    int t = p.team;
    // tir en direction du but : gêne du gardien
    bool shotLive = ball.owner < 0 && lastShotAge < 1.6f && lastShooter >= 0 && pl[lastShooter].team == t;
    if (shotLive && interferesWithGoalkeeper(j)) {
        int g = -1; for (int k = (1 - t) * 11; k < (1 - t) * 11 + 11; k++) if (pl[k].onPitch && pl[k].gk) g = k;
        return g >= 0 && (p.pos - pl[g].pos).len() < 1.1f ? OI_INTERFERING : OI_BLOCKING_VISION;
    }
    // dispute le ballon à un adversaire : proche du ballon, va vers lui, un adversaire est au contact
    float db = (ball.pos - p.pos).len();
    if (db < 1.8f && ball.z < 2.f && (ball.owner < 0 || pl[ball.owner].team != t)) {
        V2 to = (ball.pos - p.pos).norm();
        bool goes = p.vel.dot(to) > 1.5f || db < 0.9f;
        bool contest = false;
        for (int k = (1 - t) * 11; k < (1 - t) * 11 + 11; k++) if (pl[k].onPitch && (pl[k].pos - ball.pos).len() < 2.4f) contest = true;
        if (goes && contest) return OI_CHALLENGING;
    }
    return OI_NONE;
}

void Match::offsideSanction(int j, int type, float margin) {
    if (offPend >= 0) return;
    const AssistantReferee& A = assistants[off.assistant & 1];
    // petit délai humain avant le drapeau : plus long sur une position serrée, plus court pour un assistant décidé
    float delay = 0.10f + (std::fabs(margin) < OFFSIDE_OBVIOUS ? 0.22f : 0.f) + (std::fabs(margin) < OFFSIDE_TIGHT ? 0.12f : 0.f) - (A.decisiveness - 50) * 0.002f;
    offPend = j; offPendType = type; offPendMargin = margin; offPendT = std::max(0.05f, delay); offPendPos = pl[j].pos;
    offFlagAR = off.assistant & 1; offFlagT = 0.01f;
    offsideLog("involvement", j, nullptr, type);
}

void Match::offsideWhistle() {
    int i = offPend;
    if (i < 0) return;
    offPend = -1;
    float am = std::fabs(offPendMargin);
    offsideLog("whistle", i, nullptr, offPendType);
    offsideReset(OR_SANCTIONED);
    playSfx(SFX_WHISTLE);
    if (pl[i].team == 0 && !S.neutral && !S.training) playSfx(SFX_FANS_WHISTLE);   // le public local conteste le hors-jeu
    msg = "HORS-JEU"; msg2 = playerTag(i); msgT = 2.0f; offsides[pl[i].team]++;
    offFlagT = 2.0f;
    static const char* WHY[] = { "", "", " (il dispute le ballon)", " (il gêne le gardien)", " (il masque la vue du gardien)", " (il profite du rebond)" };
    std::string why = WHY[std::max(0, std::min(5, offPendType))];
    if (am < OFFSIDE_TIGHT) say("Le drapeau se lève... Hors-jeu très serré de " + playerName(i) + why + ". Décision limite de l'arbitre assistant.", 3.2f, true);
    else if (am < OFFSIDE_OBVIOUS) say("Le drapeau se lève ! Hors-jeu de " + playerName(i) + why + ", c'était serré.", 2.8f);
    else if (offPendType >= OI_CHALLENGING) say("Hors-jeu de " + playerName(i) + why + ".", 2.6f);
    state = MS_STOP; stateT = 0;
    nextSp = SP_INDIRECT; nextSpTeam = 1 - pl[i].team; nextSpPos = V2(clampf(offPendPos.x, 1.f, PITCH_W - 1.f), clampf(offPendPos.y, 1.f, PITCH_L - 1.f));
    ball.owner = -1; ball.vel = V2(); ball.vz = 0;
    // ralenti : on revoit l'action depuis un peu avant la passe, image figée sur les lignes de hors-jeu
    int back = (rpHead - offRpIdx + REPLAY_N) % REPLAY_N;
    if (!S.training && !S.highlights && offRpIdx >= 0 && back <= rpCount && back < 420) {
        int t = pl[i].team;
        offRepPlayer = i; offRepMargin = offPendMargin;
        offAttY = offLineY + (attackDir[t] < 0 ? -offPendMargin : offPendMargin);
        offReplay = true; offRepHold = 0;
        state = MS_REPLAY; stateT = 0;
        int pre = std::max(0, std::min(75, rpCount - back - 1));                         // un peu avant la passe (selon l'enregistrement disponible)
        rpPos = (offRpIdx - pre + REPLAY_N) % REPLAY_N;
    }
}

void Match::offsideUpdate(float dt) {
    if (offPend >= 0) {
        offPendT -= dt;
        if (offPendT <= 0 || state != MS_PLAY) offsideWhistle();
        return;
    }
    if (!off.active || state != MS_PLAY) return;
    for (auto& c : off.cands) {
        if (!c.flagged || !pl[c.player].onPitch) continue;
        int type = offsideInvolvement(c.player, false);
        if (type != OI_NONE) {
            offDecisions++; if (std::fabs(c.margin) < OFFSIDE_OBVIOUS) offClose++; if (!c.truth) offMistakes++;
            offsideSanction(c.player, type, c.margin);
            return;
        }
    }
}

// but marqué : drapeau en attente ou joueur signalé qui gênait le gardien -> but refusé
bool Match::offsideGoalCheck(int t) {
    int j = -1, type = OI_NONE; float margin = 0;
    if (offPend >= 0 && pl[offPend].team == t) { j = offPend; type = offPendType; margin = offPendMargin; }
    else if (off.active && off.team == t) {
        for (auto& c : off.cands) if (c.flagged && pl[c.player].onPitch && c.player != ball.lastTouch && interferesWithGoalkeeper(c.player)) { j = c.player; margin = c.margin; type = OI_BLOCKING_VISION; break; }
        if (j < 0) for (auto& c : off.cands) if (c.flagged && c.player == ball.lastTouch) { j = c.player; margin = c.margin; type = OI_PLAYING_BALL; break; }
    }
    if (j < 0) return false;
    offPend = -1;
    offsideLog("goal refused", j, nullptr, type);
    offsideReset(OR_SANCTIONED);
    playSfx(SFX_WHISTLE);
    msg = "BUT REFUSÉ"; msgT = 3.f; offsides[t]++; offFlagAR = offsideAssistantFor(t); offFlagT = 2.5f;
    msg2 = "Hors-jeu : " + playerTag(j) + (type == OI_BLOCKING_VISION || type == OI_INTERFERING ? " gêne le gardien" : "");
    if (type == OI_BLOCKING_VISION || type == OI_INTERFERING) say("Le but est refusé ! " + playerName(j) + ", en position de hors-jeu, gênait le gardien.", 4.f, true);
    else say(std::string("Le but est refusé ! Le drapeau de l'arbitre assistant était levé") + (std::fabs(margin) < OFFSIDE_TIGHT ? " : hors-jeu très serré." : " : hors-jeu de " + playerName(j) + "."), 4.f, true);
    state = MS_STOP; stateT = 0; nextSp = SP_INDIRECT; nextSpTeam = 1 - t;
    nextSpPos = V2(clampf(pl[j].pos.x, 1.f, PITCH_W - 1.f), clampf(pl[j].pos.y, 1.f, PITCH_L - 1.f));
    ball.owner = -1; ball.inNet = false;
    return true;
}
