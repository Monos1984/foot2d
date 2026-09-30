// Moteur de match (suite) : bagarres (jauge d'énervement, combat), tour d'honneur, tirages au sort de la prolongation
// et des tirs au but, match arrêté, marquage sur coups de pied arrêtés, reprises de volée et retournés
#include "match.h"
#include <cmath>
#include <algorithm>

static Rng R2(0xF16A7);
static const float GRAV2 = 10.8f;
static float clampf2(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }

float Match::refSeverity() const {
    if (S.sevOverride >= 0) return (float)S.sevOverride;
    return S.referee >= 0 && S.referee < NUM_REFEREES ? (float)REFEREES[S.referee].severity : 60.f;
}

// ------------------------------------------------------------------ jauge d'énervement
void Match::addAnger(int i, float a) {
    if (i < 0 || i >= 22 || S.training) return;
    MPlayer& p = pl[i];
    if (!p.onPitch) return;
    p.anger = clampf2(p.anger + a, 0.f, 1.f);
    if (p.anger >= 0.5f && p.angerLvl < 1) { p.angerLvl = 1; say(playerName(i) + " commence à perdre son calme...", 2.8f); }
    if (p.anger >= 0.8f && p.angerLvl < 2) { p.angerLvl = 2; say(playerName(i) + " est très nerveux, il pourrait craquer !", 3.f, true); }
    if (p.anger < 0.4f) p.angerLvl = 0;
}

// escalade : bousculade si les esprits s'échauffent, bagarre quand la jauge est pleine
void Match::escalate(int a, int b, bool provoked) {
    if (S.training || fightT > 0 || a < 0 || b < 0 || !pl[a].onPitch || !pl[b].onPitch || pl[a].gk || pl[b].gk) return;
    float m = std::max(pl[a].anger, pl[b].anger) + (provoked ? 0.35f : 0.f);
    if (m >= 1.0f) fightLevel = 2;
    else if (m >= 0.6f) fightLevel = 1;
    else {
        if (provoked) { addAnger(b, 0.15f); say(playerName(a) + " va au contact de " + playerName(b) + ", l'arbitre calme le jeu.", 2.6f, true); }
        return;
    }
    startFight(a, b);
}

// bagarre en cours : vrai tant que le jeu est interrompu
bool Match::updateFight(float dt) {
    if (fightT <= 0) return false;
    // les coéquipiers accourent (bagarre générale évitée de justesse)
    for (int i = 0; i < 22; i++) {
        if (!pl[i].onPitch || i == fightA || i == fightB || pl[i].gk) continue;
        V2 d = pl[fightA].pos - pl[i].pos;
        if (d.len() < 14 && d.len() > 2.5f) { pl[i].vel = d.norm() * 3.5f; pl[i].pos += pl[i].vel * dt; pl[i].anim += dt * 3; }
    }
    for (int k : { fightA, fightB }) if (k >= 0) { pl[k].st += dt; pl[k].anim += dt * 8; }
    if (duel) {
        duelT += dt;
        int f[2] = { fightA, fightB };
        for (int k = 0; k < 2; k++) {
            if (duelCool[k] > 0) duelCool[k] -= dt;
            if (duelAtkT[k] > 0) { duelAtkT[k] -= dt; if (duelAtkT[k] <= 0) duelAtk[k] = 0; }
            if (duelHitFx[k] > 0) duelHitFx[k] -= dt;
            int atk = 0;
            const MPlayer& me = pl[f[k]];
            if (duelCtl[k] >= 0) {
                const Controls& c = ctl[duelCtl[k]];
                duelGuard[k] = c.dir.y < -0.5f ? 1 : c.dir.y > 0.5f ? 2 : 0;
                if (duelCool[k] <= 0) { if (c.f1p) atk = 1; else if (c.f2p || c.f3p) atk = 2; }
            } else {
                // ordinateur : garde et coups selon son agressivité
                if (R2.chance(dt * 1.6f)) duelGuard[k] = R2.range(0, 2);
                float aggr = 1.6f + me.tackle / 60.f;
                if (duelCool[k] <= 0 && R2.chance(dt * aggr)) atk = R2.chance(0.5f) ? 1 : 2;
            }
            if (atk) {
                duelAtk[k] = atk; duelAtkT[k] = 0.22f; duelCool[k] = 0.42f;
                int o = 1 - k;
                bool blocked = duelGuard[o] == atk;
                float dmg = blocked ? 2.f : 8.f + me.tackle * 0.06f + me.stamina * 3.f + R2.frange(0, 4);
                duelHp[o] = std::max(0.f, duelHp[o] - dmg);
                if (!blocked) { duelHitFx[o] = 0.2f; playSfx(SFX_PUNCH); }
            }
        }
        if (duelHp[0] <= 0 || duelHp[1] <= 0 || duelT > 15.f) {
            duelLoser = duelHp[0] < duelHp[1] ? 0 : 1;
            endFight();
        }
        return true;
    }
    fightT -= dt;
    if (fightT <= 0 && !fightDone) endFight();
    return true;
}

void Match::endFight() {
    fightDone = true;
    int a = fightA, b = fightB;
    if (a < 0 || b < 0) { fightT = 0; duel = false; return; }
    pl[a].state = PS_NORMAL; pl[b].state = PS_NORMAL;
    float sev = refSeverity() / 100.f;
    if (fightLevel == 1) {
        // bousculade : l'arbitre sépare les joueurs, avertissements possibles
        msg = "L'ARBITRE SÉPARE LES JOUEURS"; msg2 = ""; msgT = 2.f;
        if (R2.chance(0.25f + sev * 0.25f)) giveCard(a, 1);
        if (pl[b].onPitch && R2.chance(0.2f + sev * 0.2f)) giveCard(b, 1);
        pl[a].anger = std::max(0.f, pl[a].anger - 0.3f); pl[b].anger = std::max(0.f, pl[b].anger - 0.3f);
    } else if (duel) {
        // combat : le perdant est sonné (fatigue, blessure possible) ; les deux sont expulsés
        int lo = duelLoser == 0 ? a : b, wi = duelLoser == 0 ? b : a;
        pl[lo].stamina = std::max(0.25f, pl[lo].stamina - 0.35f); pl[lo].state = PS_DOWN; pl[lo].st = 0;
        pl[wi].stamina = std::max(0.25f, pl[wi].stamina - 0.1f);
        say(playerName(wi) + " a pris le dessus sur " + playerName(lo) + "... Scène lamentable, l'arbitre sort le rouge !", 4.f, true);
        if (R2.chance(0.3f) && !pl[lo].injured) {
            pl[lo].injured = true; pl[lo].speed *= 0.8f;
            MatchEvent e; e.type = 4; e.team = pl[lo].team; e.minute = clock; e.player = playerName(lo);
            const Team& TL = team(pl[lo].team);
            if (pl[lo].squad >= 0 && pl[lo].squad < (int)TL.squad.size()) e.pid = TL.squad[pl[lo].squad].id;
            events.push_back(e);
        }
        playSfx(SFX_BOO);
        giveCard(a, 2);
        if (pl[b].onPitch) giveCard(b, 2);
    } else {
        // bagarre (sans combat) : l'initiateur (a) rouge probable ; l'autre jaune ou rouge
        if (R2.chance(0.25f + sev * 0.3f)) giveCard(a, 2); else giveCard(a, 1);
        if (pl[b].onPitch) { float r2 = R2.f(); if (r2 < 0.1f + sev * 0.15f) giveCard(b, 2); else if (r2 < 0.65f) giveCard(b, 1); }
    }
    for (int k : { a, b }) if (pl[k].onPitch) { pl[k].anger = std::min(pl[k].anger, 0.35f); pl[k].angerLvl = 0; }
    fightT = 0; duel = false; fightLevel = 0;
    state = MS_STOP; stateT = 0.2f;
    checkAbandon();
}

// ------------------------------------------------------------------ tour d'honneur (qualification en coupe)
V2 Match::lapPoint(float s) const {
    const float m = 2.2f, w = PITCH_W - 2 * m, h = PITCH_L - 2 * m, per = 2 * (w + h);
    s = std::fmod(s, per); if (s < 0) s += per;
    // départ au bord de touche (côté tunnel), vers le haut, puis sens des aiguilles d'une montre
    float y0 = PITCH_L / 2 - m;
    if (s < y0) return V2(m, PITCH_L / 2 - s);
    s -= y0;
    if (s < w) return V2(m + s, m);
    s -= w;
    if (s < h) return V2(PITCH_W - m, m + s);
    s -= h;
    if (s < w) return V2(PITCH_W - m - s, PITCH_L - m);
    s -= w;
    return V2(m, PITCH_L - m - s);
}

void Match::startLap(int t) {
    lapActive = true; lapTeam = t; lapT = 0;
    int k = 0;
    for (int i = t * 11; i < t * 11 + 11; i++) lapS[i] = -2.f * (k++);
    msg = "TOUR D'HONNEUR"; msg2 = team(t).name + " est qualifié !"; msgT = 4.f;
    say(team(t).name + " fête sa qualification avec ses supporters !", 4.f, true);
    playSfx(SFX_GOAL);
}

void Match::updateLap(float dt) {
    lapT += dt;
    if (msgT > 0) msgT -= dt;
    int lead = -1;
    for (int i = 0; i < 22; i++) {
        MPlayer& p = pl[i];
        if (!p.onPitch) continue;
        if (p.team != lapTeam) {   // les adversaires rentrent au vestiaire
            V2 door(-3.f, PITCH_L / 2 + (p.team ? 1.f : -1.f));
            V2 d = door - p.pos;
            if (d.len() > 0.3f) { p.vel = d.norm() * 3.f; p.pos += p.vel * dt; p.anim += 3.f * dt; p.face = d.norm(); } else p.vel = V2();
            p.state = PS_NORMAL;
            continue;
        }
        lapS[i] += 3.4f * dt;
        V2 tg = lapPoint(std::max(0.f, lapS[i]));
        V2 d = tg - p.pos;
        float sp = d.len() > 3.f ? 5.5f : 3.4f;
        if (d.len() > 0.1f) { p.vel = d.norm() * std::min(sp, d.len() * 4.f); p.pos += p.vel * dt; p.anim += p.vel.len() * dt; p.face = d.norm(); }
        // salut aux supporters : bras levés par intermittence
        p.state = ((int)(lapT * 2.5f) + i) % 4 == 0 ? PS_CELEB : PS_NORMAL;
        if (lead < 0 || lapS[i] > lapS[lead]) lead = i;
    }
    if (lead >= 0) cam = cam + (pl[lead].pos - cam) * std::min(1.f, dt * 2.f);
    if (lapT > 18.f) { lapActive = false; for (auto& p : pl) if (p.state == PS_CELEB) p.state = PS_NORMAL; walkT = 0; }
}

// ------------------------------------------------------------------ tirage au sort avant la prolongation / les tirs au but
bool Match::updateMiniToss(float dt) {
    if (tossKind == 0) return false;
    cerT += dt;
    V2 c(PITCH_W / 2, PITCH_L / 2);
    auto walk = [&](int k, V2 tg) {
        if (k < 0 || !pl[k].onPitch) return;
        V2 d = tg - pl[k].pos;
        if (d.len() > 0.2f) { pl[k].vel = d.norm() * std::min(5.f, d.len() * 3); pl[k].pos += pl[k].vel * dt; pl[k].anim += pl[k].vel.len() * dt; pl[k].face = d.norm(); }
        else pl[k].vel = V2();
    };
    walk(captain[0], c + V2(-1.4f, 0.4f)); walk(captain[1], c + V2(1.4f, 0.4f));
    refMove(c + V2(0, -0.6f), 3.6f, V2(0, 1), dt);
    cam = cam + (c - cam) * std::min(1.f, dt * 2);
    const float FLIP = 2.4f;
    if (cerT > FLIP && tossWinner < 0) {
        tossWinner = tossResult == tossCall ? 1 : 0;
        say(std::string(tossResult == 0 ? "PILE ! " : "FACE ! ") + team(tossWinner).name + " remporte le tirage au sort.", 2.5f, true);
    }
    if (tossWinner >= 0 && cerT > FLIP + 1.3f && tossChoice < 0) {
        if (humanSide(tossWinner)) {
            if (tossUI != 3) { tossUI = 3; tossSel = 0; }
            if (tossNav(tossWinner, tossSel, 2, dt)) tossChoice = tossSel;
            if (cerT > 30.f && tossChoice < 0) tossChoice = 0;
        } else tossChoice = tossKind == 2 ? (R2.chance(0.8f) ? 0 : 1) : R2.range(0, 1);
        if (tossChoice >= 0) {
            int w = tossWinner;
            if (tossKind == 1) {
                // prolongation : le ballon (coup d'envoi) ou le côté
                etKickoff = tossChoice == 0 ? w : 1 - w;
                etFlip = R2.range(0, 1);
                tossSideTeam = tossChoice == 0 ? 1 - w : w;
                tossDir = etFlip;
                say(team(w).shortName + (tossChoice == 0 ? " prend le ballon pour la prolongation." : " choisit son côté pour la prolongation.") + " Coup d'envoi : " + team(etKickoff).shortName + ".", 3.f, true);
            } else {
                // tirs au but : le vainqueur choisit de tirer en premier ou en second ; le but est tiré au sort
                shootTeam = tossChoice == 0 ? w : 1 - w;
                shootGoal = R2.range(0, 1);
                say(team(w).shortName + (tossChoice == 0 ? " choisit de tirer en premier." : " choisit de laisser tirer l'adversaire en premier.") + " Les tirs se dérouleront " + (shootGoal == 0 ? "dans le but du haut." : "dans le but du bas."), 3.5f, true);
            }
            tossUI = 5; sideT = cerT;
        }
    }
    if (tossChoice >= 0 && cerT > sideT + 2.4f) { tossKind = 0; tossUI = 0; return false; }
    return true;
}

// ------------------------------------------------------------------ match arrêté : moins de 7 joueurs
void Match::checkAbandon() {
    if (abandoned || finished || S.training || shootout) return;
    for (int t = 0; t < 2; t++) {
        if (teamPlayersCount(t) >= 7) continue;
        abandoned = true; abandonTeam = t;
        int n = teamPlayersCount(t);
        score[t] = 0; score[1 - t] = std::max(3, score[1 - t]);
        say(fmt("L'arbitre arrête le match : %s n'a plus que %d joueurs sur le terrain. Défaite 3-0 sur tapis vert !", team(t).name.c_str(), n), 5.f, true);
        finishMatch();
        msg = "MATCH ARRÊTÉ"; msg2 = team(t).shortName + fmt(" à %d : défaite 3-0 sur tapis vert", n);
        return;
    }
}

// ------------------------------------------------------------------ coups de pied arrêtés : chaque attaquant dans la zone est marqué
void Match::markAttackers(int def, V2 goal, float maxDist) {
    int att = 1 - def;
    std::vector<int> attackers, defenders;
    for (int i = att * 11 + 1; i < att * 11 + 11; i++) if (pl[i].onPitch && i != spKicker && (pl[i].target - goal).len() < maxDist) attackers.push_back(i);
    for (int i = def * 11 + 1; i < def * 11 + 11; i++) if (pl[i].onPitch) defenders.push_back(i);
    // les joueurs du mur restent au mur
    std::vector<bool> used(defenders.size(), false);
    V2 wallC = spPos + (goal - spPos).norm() * 9.15f;
    for (size_t k = 0; k < defenders.size(); k++) if (sp == SP_FREEKICK && (pl[defenders[k]].target - wallC).len() < 3.f) used[k] = true;
    for (int a : attackers) {
        int best = -1; float bd = 1e9;
        for (size_t k = 0; k < defenders.size(); k++) {
            if (used[k]) continue;
            float d = (pl[defenders[k]].target - pl[a].target).len();
            if (d < bd) { bd = d; best = (int)k; }
        }
        if (best < 0) break;
        used[best] = true;
        V2 tg = pl[a].target + (goal - pl[a].target).norm() * 0.9f;   // côté but, au contact
        pl[defenders[best]].target = tg;
    }
}

// bouton de tir d'un contrôleur (Super Nintendo : bouton Tir ; classique : bouton 1)
int Match::shootBtn(int h, bool pressedOnly) const {
    if (h < 0) return 0;
    const Controls& c = ctl[h];
    if (S.snes) return pressedOnly ? c.f2p : (c.f2 || c.f2p);
    return pressedOnly ? c.f1p : (c.f1 || c.f1p);
}

// reprise de volée / retourné acrobatique : ballon à mi-hauteur près du but adverse
bool Match::tryVolley(int i) {
    MPlayer& p = pl[i];
    if (p.gk || !p.onPitch || p.state != PS_NORMAL || p.cool > 0 || state != MS_PLAY) return false;
    Ball& b = ball;
    float d = (p.pos - b.pos).len();
    if (d > 1.0f || b.owner >= 0 || b.vel.len() < 4.f) return false;
    float pr = progress(p.team, p.pos);
    if (pr < 0.72f || std::fabs(p.pos.x - PITCH_W / 2) > 18.f) return false;
    V2 g = goalCenter(p.team);
    V2 tg = (g - p.pos).norm();
    bool bike = b.z >= 1.05f && b.z < 2.0f && p.face.dot(tg) < -0.1f;
    bool volley = b.z >= 0.3f && b.z < 1.05f;
    if (!bike && !volley) return false;
    bool want;
    if (p.human >= 0) want = shootBtn(p.human, false) != 0;
    else want = R2.chance(bike ? 0.35f : 0.6f) && (b.lastTeam == p.team || pr > 0.82f);
    if (!want) return false;
    checkOffsideTouch(i);
    if (state != MS_PLAY) return true;
    // précision : tir, sang-froid, difficulté du geste
    float err = (100 - p.shoot) / 100.f * 0.16f + (100 - p.compo) / 100.f * 0.06f + (bike ? 0.1f : 0.04f) + std::min(0.08f, b.vel.len() * 0.004f);
    float side = R2.chance(0.5f) ? -1.f : 1.f;
    V2 aimPt(PITCH_W / 2 + side * R2.frange(0.5f, 3.2f), g.y);
    V2 aim = (aimPt - b.pos).norm();
    float a = R2.frange(-1, 1) * err;
    aim = V2(aim.x * std::cos(a) - aim.y * std::sin(a), aim.x * std::sin(a) + aim.y * std::cos(a));
    float pw = bike ? 22.f + p.shoot / 12.f : 24.f + p.shoot / 9.f;
    float vz = bike ? R2.frange(-0.5f, 1.5f) : R2.frange(-1.f, 2.2f) + (b.z > 0.7f ? 1.f : 0.f);
    kickBall(i, aim, pw, vz, p.human >= 0);
    ball.aftertouch = 0;
    if (bike) {
        p.state = PS_BIKE; p.st = 0; p.z = 0.9f; p.vz = 1.5f;
        say("Un retourné acrobatique de " + playerName(i) + " !", 3.f, true);
    } else {
        p.state = PS_KICK; p.st = 0;
        say("Reprise de volée de " + playerName(i) + " !", 2.4f);
    }
    return true;
}

// ------------------------------------------------------------------ météo
const char* METEO_NAMES[6] = { "canicule", "chaud", "temps normal", "pluie", "orage", "neige" };

int meteoForMonth(int month, int climate, uint64_t seed) {
    Rng r(seed * 2654435761ULL + 99);
    if (climate == 2) month = (month + 6) % 12;                 // hémisphère sud : saisons inversées
    if (month > 10) month = month == 11 ? 0 : 10;              // juillet ~ août
    // pondérations : canicule, chaud, normal, pluie, orage, neige
    static const int W[11][6] = {
        { 12, 30, 36, 10, 12, 0 },   // août
        { 3, 18, 52, 18, 9, 0 },     // septembre
        { 0, 4, 56, 36, 4, 0 },      // octobre
        { 0, 0, 52, 42, 1, 5 },      // novembre
        { 0, 0, 46, 38, 0, 16 },     // décembre
        { 0, 0, 44, 36, 0, 20 },     // janvier
        { 0, 0, 46, 38, 0, 16 },     // février
        { 0, 2, 54, 38, 2, 4 },      // mars
        { 0, 10, 56, 26, 8, 0 },     // avril
        { 4, 24, 46, 14, 12, 0 },    // mai
        { 14, 32, 34, 8, 12, 0 },    // juin
    };
    int w[6]; for (int k = 0; k < 6; k++) w[k] = W[std::max(0, std::min(10, month))][k];
    if (climate == 1) { w[0] = w[0] * 2 + 6; w[1] = w[1] * 2 + 14; w[3] /= 3; w[5] = 0; }            // pays chaud : pas de neige
    if (climate == 2) { w[5] /= 3; }
    if (climate == 3) { w[5] = w[5] * 2 + (month >= 3 && month <= 7 ? 12 : 0); w[0] /= 3; w[1] /= 2; }  // pays nordique : davantage de neige
    int tot = 0; for (int k = 0; k < 6; k++) tot += w[k];
    int x = r.range(0, std::max(1, tot) - 1);
    for (int k = 0; k < 6; k++) { if (x < w[k]) return k; x -= w[k]; }
    return 2;
}

void meteoApply(MatchSetup& s, bool heated, int turf) {
    Rng r((uint64_t)(s.home * 131 + s.away * 7 + s.meteo * 3 + 1));
    switch (s.meteo) {
    case 0: s.pitch = 1; break;                                            // canicule : terrain sec et dur
    case 1: s.pitch = r.chance(0.6f) ? 1 : 0; break;
    case 3: s.pitch = turf < 40 && r.chance((40 - turf) / 40.f + 0.1f) ? 3 : 2; break;
    case 4: s.pitch = r.chance(0.55f) || turf < 50 ? 3 : 2; break;         // orage : terrain gorgé d'eau
    case 5: s.pitch = heated ? 2 : 4; break;                               // neige : gelé, sauf pelouse chauffée
    default: s.pitch = r.chance(0.2f) ? 1 : 0; break;
    }
    s.weather = s.meteo == 3 || s.meteo == 4 ? 2 : s.meteo == 5 ? 3 : 0;
}
