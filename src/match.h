// Moteur de match (physique, IA, règles)
#pragma once
#include "game.h"
#include <string>
#include <vector>

struct V2 {
    float x = 0, y = 0;
    V2() {}
    V2(float a, float b) : x(a), y(b) {}
    V2 operator+(V2 o) const { return { x + o.x, y + o.y }; }
    V2 operator-(V2 o) const { return { x - o.x, y - o.y }; }
    V2 operator*(float k) const { return { x * k, y * k }; }
    V2& operator+=(V2 o) { x += o.x; y += o.y; return *this; }
    V2& operator-=(V2 o) { x -= o.x; y -= o.y; return *this; }
    float len() const { return std::sqrt(x * x + y * y); }
    float len2() const { return x * x + y * y; }
    V2 norm() const { float l = len(); return l > 1e-5f ? V2(x / l, y / l) : V2(0, 0); }
    float dot(V2 o) const { return x * o.x + y * o.y; }
    V2 perp() const { return { -y, x }; }
};

// Dimensions (mètres)
const float PITCH_W = 68.f, PITCH_L = 105.f;
const float GOAL_W = 7.32f, GOAL_H = 2.44f, GOAL_DEPTH = 2.0f;
const float BOX_W = 40.32f, BOX_L = 16.5f, SIX_W = 18.32f, SIX_L = 5.5f;
const float BALL_R = 0.12f;

enum InputId { IN_KB1 = 0, IN_KB2, IN_PAD1, IN_PAD2, IN_PAD3, IN_PAD4, NUM_INPUTS };

struct Controls {
    V2 dir;
    bool f1 = false, f2 = false;          // maintenu
    bool f1p = false, f2p = false;        // pressé cette frame
    bool f1r = false, f2r = false;        // relâché cette frame
    bool f3 = false, f4 = false, sprint = false;
    bool f3p = false, f4p = false;
    bool pause = false;
};

enum PState { PS_NORMAL = 0, PS_SLIDE, PS_DOWN, PS_DIVE, PS_GKHOLD, PS_THROW, PS_CELEB, PS_OFF, PS_KICK, PS_HEAD, PS_WAIT, PS_FIGHT, PS_HAND, PS_BIKE };

struct MPlayer {
    int team = 0;
    int slot = 0;           // 0 = gardien, 1..10
    int squad = 0;          // index dans l'effectif
    bool gk = false;
    V2 pos, vel;
    float z = 0, vz = 0;
    V2 face{ 0, -1 };
    int state = PS_NORMAL;
    float st = 0;           // temps dans l'état
    float cool = 0;         // délai avant de pouvoir retoucher le balle
    float anim = 0;
    float speed = 7, shoot = 50, pass = 50, tackle = 50, keep = 50;
    float stamina = 1;
    float dribble = 50, heading = 50, endur = 0.6f;
    float posi = 50, compo = 50;   // placement, sang-froid
    float personalityStress=1,personalityFoul=1,personalityEngagement=1;
    float cond0 = 100, stam0 = 1;   // condition avant le match / endurance au coup d'envoi
    int yellow = 0;
    bool sentOff = false, injured = false, onPitch = true;
    int human = -1;         // index du contrôleur humain
    V2 target;              // position visée (IA)
    float charge = 0;       // durée d'appui (tir)
    bool charging = false;
    int detailedPosition=DP_MC; SlotTactic tactic; RoleEffects effects;
    int role = 2;           // 1 déf, 2 mil, 3 att
    float thinkT = 0;
    V2 slideDir;
    bool fouledInSlide = false;
    bool touchedBallInSlide = false;
    bool smother = false;          // gardien : plongeon dans les pieds de l'attaquant en cours
    float anger = 0;               // jauge d'énervement (0 calme ... 1 hors de lui)
    int angerLvl = 0;              // dernier palier annoncé (1 énervé, 2 très énervé)
    float runT = 0; V2 runTarget;  // appel de balle après une passe (une-deux)
    int wallFrom = -1; float wallT = 0;   // vient de recevoir une passe de wallFrom : remise en une-deux possible
};

struct Ball {
    V2 pos;
    float z = 0;
    V2 vel;
    float vz = 0;
    float spin = 0;         // effet latéral
    int owner = -1;
    int lastTouch = -1;     // joueur
    int lastTeam = -1;
    float aftertouch = 0;   // temps restant d'effet (joueur humain)
    int aftertouchBy = -1;
    bool backpass = false;
    bool inNet = false;
    float spinDecay = 0;
};

enum MState { MS_INTRO = 0, MS_SETPIECE, MS_PLAY, MS_GOAL, MS_STOP, MS_BREAK, MS_END, MS_REPLAY, MS_SHOOTOUT, MS_WALKOUT };
enum SetPiece { SP_KICKOFF = 0, SP_THROWIN, SP_CORNER, SP_GOALKICK, SP_FREEKICK, SP_PENALTY, SP_INDIRECT, SP_SHOOTOUT, SP_GKBALL };

struct MatchSetup {
    RuleProfile rules=RULESET_SIMPLE;
    int home = 0, away = 1;
    int side[NUM_INPUTS] = { 0, -1, -1, -1, -1, -1 }; // -1 aucun, 0 domicile, 1 extérieur
    float halfSeconds = 180;     // durée réelle d'une mi-temps
    bool decisive = false;       // il faut un vainqueur
    bool goalAssist = false;     // arbitres assistants supplémentaires derrière les buts (compétitions qui les utilisent)
    bool hasFirstLeg = false;
    int aggHome = 0, aggAway = 0; // buts du match aller (équipe à domicile de CE match / extérieur)
    bool neutral = false;
    int pitch = 0;
    int turf = 100;              // état de la pelouse (100 parfaite ... 0 champ de patates)
    float bribeMult[2] = { 1.f, 1.f };   // « valise » : équipe adverse diminuée
    int bribeSquad[2] = { -1, -1 };      // joueur adverse acheté (index dans l'effectif)
    int lockSquad[2] = { -1, -1 };       // carrière de joueur : le joueur humain ne contrôle que ce joueur (index dans l'effectif)
    int difficulty = 1;          // 0 facile 1 normal 2 difficile
    std::string title;
    std::string stadium;
    int halfMinutes = 45;        // Pole tournament: two 20-minute halves.
    bool noET = false;
    bool pensOnly = false;       // amical : séance de tirs au but seulement
    bool awayGoals = false;
    int yellowLimit = 3;
    bool snes = true;             // commandes Super Nintendo
    int referee = 0;
    bool night = false, tv = false;
    int weather = 0;              // 0 beau, 1 couvert, 2 pluie, 3 neige
    int meteo = 2;                // 0 canicule, 1 chaud, 2 normal, 3 pluie, 4 orage, 5 neige (effets sur le match)
    float crowdFill = 0.7f;
    int supporterAtmosphere=40,supporterChant=0,supporterTifo=-1,supporterVisitors=0,supporterLoyalty=65,supporterPatience=65;
    float supporterPressure=0;
    int attendance = 0;           // spectateurs attendus
    float homeBoost = 0;          // influence du stade : bonus de l'équipe qui reçoit (public, ambiance)
    bool personalityActive=false,personalityImportant=false;int personalityKey=0;
    bool commentary = true;
    std::string channel;
    std::string kickoffTime, kickoffDate;      // buts à l'extérieur (aggHome = buts marqués à l'extérieur par l'équipe qui reçoit)
    bool awayKitHome = false, awayKitAway = true;
    int formation[2] = { -1, -1 };
    int training = 0;              // 0 match ; 1 penalties, 2 coups francs, 3 corners, 4 attaque-défense, 5 penalties (gardien)
    bool proMedia = false;         // caméras et photographes (football professionnel, grandes coupes)
    bool cupPhoto = false;         // photo officielle des équipes avant le match (coupes)
    bool studio = false;           // plateau TV avant-match, mi-temps et fin de match (Ligue des champions...)
    bool halftimeScreen = false;   // l'application affiche un écran de mi-temps (sinon reprise automatique)
    int managed = -1;              // mode Full Manager : équipe dirigée par le joueur mais jouée par l'ordinateur
    bool highlights = false;       // mode Full Manager : seuls les temps forts sont montrés à vitesse normale
    bool delegSubs = false;        // remplacements délégués à l'adjoint (équipe du joueur)
    bool rolling = false;          // coupes régionales / de district : 3 remplaçants, changements illimités, retour possible
    uint8_t tac[2][5] = { { 1, 1, 1, 1, 1 }, { 1, 1, 1, 1, 1 } };   // consignes : pressing, ligne, largeur, tempo, passes
    bool anthems = false;          // sélections nationales : hymnes avant le match
    std::string anthemOnly;        // un seul hymne joué pour les deux équipes (finale de la Coupe de France : La Marseillaise)
    bool president = false;        // le président de la République salue les joueurs (finale de la Coupe de France)
    int kitSel[2] = { -1, -1 };    // tenue choisie : -1 automatique, 0 domicile, 1 extérieur, 2 troisième
    bool goldenGoal = false;       // prolongation avec but en or
    bool etNoPens = false;         // prolongation sans tirs au but (le match peut finir nul)
    int sevOverride = -1;          // sévérité de l'arbitre imposée (amicaux)
    int benchSize = -1;            // remplaçants sur la feuille de match (-1 : règle par défaut)
    int maxSubs = -1;              // remplacements autorisés (-1 : règle par défaut)
};

struct MatchEvent { int type; int team; float minute; std::string player; int pid = 0; std::string assist; int aid = 0; bool pen = false; }; // 0 but, 1 jaune, 2 rouge, 3 csc, 4 blessure

struct Snap { float x, y, z; int8_t fx, fy; uint8_t state, frame; };
struct BallSnap { float x, y, z; };

const char* mentalityName(int m);
Kit teamKit(const Team& T, int sel);
struct MatchSetup;
void matchKits(const MatchSetup& S, Kit kit[2]);     // 0 domicile, 1 extérieur, 2 troisième tenue


V2 anthemSpot(int t, int s2);   // hymnes : place sur la ligne (match.cpp)
struct Match {
    std::map<std::pair<int,int>,float> positionMinutes;
    MatchSetup S;
    MPlayer pl[22];
    Ball ball;
    int state = MS_INTRO;
    float stateT = 0;
    int score[2] = { 0, 0 };
    int pens[2] = { 0, 0 };
    int penTaken[2] = { 0, 0 };
    int lastPenMiss = 0;
    float penMaxZ = 0;
    int penDive = 0;                // plongeon choisi par le gardien humain : -1 gauche, 0 centre, 1 droite
    int penGk = -1;                 // gardien face à un penalty (allonge réduite)            // dernier tir au but : 0 marqué, 1 arrêté, 2 au-dessus, 3 à côté/poteau
    bool shootout = false;
    int shootTeam = 0;
    int shootKicker = -1;
    std::vector<int> shootElig[2];  // tireurs autorisés (après avoir écarté des joueurs en cas de supériorité numérique)
    std::vector<int> shootList[2];  // liste ordonnée des 5 premiers tireurs
    std::vector<int> shootDone[2];  // tireurs déjà passés dans le tour en cours
    int shootExcl[2] = { 0, 0 };    // joueurs restant à écarter
    int shootUI = 0, shootUITeam = -1, shootSel = 0; float shootUIT = 0;   // choix : 1 écarter, 2 liste des 5, 3 tireur suivant
    void shootoutSetup();
    void shootNextUI();
    void shootoutKick(int t, int kicker);
    void updateShootUI(float dt);
    std::vector<int> shootChoices() const;
    std::vector<int> shootRemaining(int t) const;
    std::vector<int> penLog[2];     // 1 marqué, 0 raté
    int period = 0;                 // 0: 1re MT, 1: 2e MT, 2-3: prolongation
    float clock = 0;                // minutes de jeu
    float periodEnd = 45;
    float added = 0;
    int attackDir[2] = { -1, 1 };   // -1 : attaque vers y=0
    int kickoffTeam = 0;
    int firstKickoff = 0;
    // coup de pied arrêté
    int sp = SP_KICKOFF;
    int spTeam = 0;
    V2 spPos;
    int spKicker = -1;
    float spT = 0;
    V2 spAim;
    float spCurl = 0;             // coup franc : effet choisi (-1 ... 1), flèche courbe
    bool spReady = false;
    int subsLeft[2] = { 5, 5 };
    std::vector<int> bench[2];      // index effectif
    std::vector<int> onField[2];    // index effectif des 11
    int formation[2] = { 0, 0 };
    Kit kit[2];
    unsigned gkShirt[2];
    // hors-jeu
    bool offsideArmed = false;
    int offsideTeam = -1;
    std::vector<int> offsideSet;
    // message
    std::string msg, msg2;
    int ballStage = 0;            // cérémonie : 1 ballon sur son présentoir au bout du tunnel, 2 porté par l'arbitre
    V2 pedestal;
    float msgT = 0;
    int cardShow = 0;               // 1 jaune 2 rouge
    // événements
    std::vector<MatchEvent> events;
    // état final
    bool finished = false;
    bool aet = false;
    int resultPH = -1, resultPA = -1;
    // replay
    static const int REPLAY_N = 480;
    std::vector<Snap> rp;           // REPLAY_N * 22
    std::vector<BallSnap> rb;
    int rpHead = 0, rpCount = 0, rpPos = 0;
    // caméra
    V2 cam;
    // contrôleurs humains -> joueur
    int ctrlPlayer[NUM_INPUTS] = { -1, -1, -1, -1, -1, -1 };
    Controls ctl[NUM_INPUTS];
    float pitchFriction = 1.0f, pitchBounce = 0.5f;
    float turfBad = 0;              // 0 pelouse correcte ... 1 champ de patates : faux rebonds, ballon freiné
    int possTeam = -1;
    float possTime[2] = { 0, 0 };
    int shots[2] = { 0, 0 };
    int onTarget[2]={},passes[2]={},completedPasses[2]={},saves[2]={},blockedShots[2]={},woodwork[2]={},throws[2]={},freeKicks[2]={},goalKicks[2]={},penalties[2]={};
    int pendingPass=-1,pendingShot=-1;bool kickingPass=false,shotHitWoodwork=false;
    void statsTouch(int player);void statsGoal(int team);void statsWoodwork();
    int corners[2] = { 0, 0 }, fouls[2] = { 0, 0 }, offsides[2] = { 0, 0 };
    float flashT = 0, thunderT = 20; V2 wind; bool coolBreak[2] = { false, false };   // météo : éclairs, vent d'orage, pauses fraîcheur
    int medicFor = -1; int medicPhase = 0; float medicT = 0; V2 medicPos[2]; bool medicDone[22] = {};   // soigneurs (blessure)
    void updateMedics(float dt);
    int lastShooter = -1; float lastShotAge = 99;   // dernier tir cadré (déviation / gardien : pas de csc)
    float stopReason = 0;
    int nextSp = -1, nextSpTeam = 0; V2 nextSpPos; // après un arrêt (faute...)
    float goalFreeze = 0;
    bool paused = false;
    int lastScorerTeam = -1;
    int touchSeen = -1, assistCand = -1;   // passeur décisif potentiel
    bool wantSwitch[NUM_INPUTS] = {};
    float lockSwitch[NUM_INPUTS] = {};
    // arbitre
    V2 refPos, refVel, refFace{ 0, 1 };
    float refCardT = 0; int refCardType = 0, refCardFor = -1;
    float refAnim = 0;           // foulées de l'arbitre (distance parcourue)
    void refMove(V2 tg, float spd, V2 faceIdle, float dt);   // l'arbitre court / marche vers un point (vitesse constante, freinage à l'arrivée)
    // bagarre
    int fightA = -1, fightB = -1; float fightT = 0; bool fightDone = false; int lastFoulOff = -1, lastFoulVic = -1; float lastFoulT = 99;
    // cérémonie d'avant-match
    bool ceremony = false;
    // cérémonie de remise du trophée (finale de coupe, titre de champion) : médailles, trophée, haie d'honneur, tour d'honneur
    struct TrStep { V2 p; int act; };
    bool trophyActive = false; int trophyTeam = -1, trophyKind = 0, trophyStyle = 0; std::string trophyTitle;
    int trPhase = 0; float trT = 0, trTotal = 0; int trHolder = -1; bool trLift = false, trOnTable = true;
    bool medal[22] = {};
    std::vector<TrStep> trRoute[22]; float trDelay[22] = {}, trWait[22] = {}, trSpeed[22] = {}; V2 trFace[22];
    V2 trPod;
    void startTrophy(int team, int kind, int style, const std::string& title);
    void updateTrophy(float dt);
    void trBegin(int ph);
    void endTrophy();
    bool trTeamDone(int t) const;
    int trCaptain(int t) const;
    bool attackOngoing() const;
    // tunnel des vestiaires : entrée des joueurs, retour à la mi-temps, sortie pour la 2e période
    bool htWaiting = false, htGo = false; float walkT = 0;
    static V2 tunnelSpot(int t, int k) { return V2(-3.f - k * 0.9f, PITCH_L / 2 + (t ? 1.0f : -1.0f)); }
    bool inTunnelPhase() const { return (ceremony && cerPhase < 0) || state == MS_WALKOUT || (state == MS_BREAK && period == 0) || (finished && !trophyActive); }
    void walkToTunnel(float dt, float spd);
    // mentalité (0 ultra défensive ... 4 ultra offensive) et capitaine
    int mentality[2] = { 2, 2 };
    float aiTacClock = 0;
    void applyMentality(int t);
    void setMentality(int t, int m, bool announce);
    void updateCaptain(int t);
    void aiTactics();
    // commentaires
    std::string comLine; float comT = 0; float comCool = 0;
    void say(const std::string& s, float dur = 3.2f, bool force = false);
    void giveCard(int off, int type);          // 1 jaune, 2 rouge direct
    void startFight(int a, int b);
    void updateReferee(float dt);
    void updateCeremony(float dt);
    void humanControlSnes(int i, const Controls& c, float dt);
    void trackTouch();
    int sfxQueue[16]; int sfxN = 0;

    void init(const MatchSetup& setup);
    void update(float dt);
    void setControls(int input, const Controls& c) { ctl[input] = c; }
    bool humanSide(int team) const;
    bool coachSide(int team) const { return humanSide(team) || S.managed == team; }   // équipe dirigée par un joueur humain
    bool hotPhase() const;                 // temps fort (mode Full Manager)
    bool lateSubDone[2] = { false, false };
    std::string playerName(int i) const;
    const Team& team(int t) const { return g_world.teams[t == 0 ? S.home : S.away]; }
    // gestion
    void substitute(int team, int fieldSlot, int benchIdx);
    void setFormation(int team, int f);
    void playSfx(int id) { if (sfxN < 16) sfxQueue[sfxN++] = id; }

    // interne
    void startPeriod(int p);
    void beginSetPiece(int type, int team, V2 spot);
    void updateSetPiece(float dt);
    void updatePlay(float dt);
    void updateBall(float dt);
    void updatePlayers(float dt);
    void humanControl(int i, const Controls& c, float dt);
    void aiControl(int i, float dt);
    void aiCarrier(int i, float dt);
    void gkControl(int i, float dt);
    void assignHumans();
    void checkOut();
    void kickBall(int i, V2 dir, float speed, float vz, bool human, bool deliberate = true);
    void passTo(int i, int target, bool lob);
    int bestPassTarget(int i, V2 dir, bool lob, float maxDist = 40) const;
    void takePossession(int i);
    void foul(int offender, int victim, bool fromBehind);
    void goalScored(int team);
    void endPeriod();
    void checkOffsideTouch(int i, bool deliberate = true);
    float shotLift(int i, V2 d, float speed, float pw, float vz);
    void armOffside(int kicker);
    V2 goalCenter(int team) const;    // but ATTAQUÉ par l'équipe
    V2 ownGoal(int team) const;
    bool inOwnBox(int team, V2 p) const;
    float progress(int team, V2 p) const; // 0 propre but -> 1 but adverse
    V2 fromTeamFrame(int team, float u, float v) const;
    void formationTarget(int i, V2& out) const;
    int nearestToBall(int team, int exclude = -1) const;
    void record();
    void startReplay();
    void shootoutNext();
    void placeForSetPiece();
    int teamPlayersCount(int t) const;
    void finishMatch();
    void sendOff(int i);
    void autoSubs(int team);
    void gkProtect(int gk);
    // avant-match : photo, poignées de main, pile ou face
    int cerPhase = 2; float cerT = 0; bool sideFlip = false;
    int tossCall = -1, tossResult = -1, tossWinner = -1, tossChoice = -1, tossSideTeam = -1, captain[2] = { -1, -1 }; float sideT = 0; bool tossAsked = false;
    int tossSel = 0; float tossStickT = 0; int tossUI = 0;   // curseur de sélection ; 1 appel, 2 pièce, 3 choix, 4 côté
    int tossDir = -1;                                       // côté choisi (0 haut, 1 bas)
    bool tossNav(int team, int& sel, int n, float dt);        // curseur gauche/droite + validation
    // carton différé (suspense), remplacements « comme en vrai », célébrations
    int pendCardOff = -1, pendCardType = 0; float pendCardT = 0; bool pendCardSaid = false;
    struct PendSub { int team, slot, incoming; };
    std::vector<PendSub> pendSubs;
    float subBoardT = 0; int subBoardTeam = 0, subBoardOut = 0, subBoardIn = 0; std::string subBoardOutName, subBoardInName;
    bool autoSubDone = false;
    struct Walker { V2 pos, target; int team; int skin, hair; bool gk; unsigned gkShirt; float anim; };
    std::vector<Walker> walkers;
    int celebScorer = -1, celebType = 0; V2 celebTarget;
    void doSub(int team, int slot, int incoming, bool anim);
    void updatePending(float dt);
    // temps additionnel : panneau du 4e arbitre
    bool boardDone = false; float boardT = 0; int boardN = 0; int subsPeriod = 0;
    // entraînement
    int trainTries = 0, trainGoals = 0; float trainT = 0;
    void trainingNext();
    bool trainingUpdate(float dt);
    // bagarres : jauge d'énervement, bousculade, bagarre (combat au corps à corps si un joueur humain est impliqué)
    int fightLevel = 0;
    bool duel = false; float duelHp[2] = { 100, 100 }, duelCool[2] = { 0, 0 }, duelAtkT[2] = { 0, 0 }, duelHitFx[2] = { 0, 0 };
    int duelGuard[2] = { 0, 0 }, duelAtk[2] = { 0, 0 }, duelCtl[2] = { -1, -1 }; float duelT = 0; int duelLoser = -1;
    void addAnger(int i, float a);
    void escalate(int a, int b, bool provoked);
    bool updateFight(float dt);
    void endFight();
    float refSeverity() const;
    // tour d'honneur après une qualification
    bool lapActive = false; int lapTeam = -1; float lapT = 0; float lapS[22] = {};
    void startLap(int team);
    void updateLap(float dt);
    V2 lapPoint(float s) const;
    // hymnes nationaux (travelling sur les joueurs)
    int anthemReq = -1; float camZoom = 1.f; float anthemDur = 10.5f; std::string anthemName;
    V2 vipPos; bool vipOn = false;     // président de la République (finale de la Coupe de France)
    // tirage au sort avant la prolongation et avant les tirs au but
    int tossKind = 0; int etKickoff = -1; int etFlip = -1; int shootGoal = 0; bool tossDoneET = false, tossDoneTAB = false;
    bool updateMiniToss(float dt);
    // match arrêté (moins de 7 joueurs)
    bool abandoned = false; int abandonTeam = -1;
    void checkAbandon();
    // changement de côté à la mi-temps de la prolongation
    V2 swapTarget[22]; bool swapInit = false;
    // coups de pied arrêtés : marquage individuel
    void markAttackers(int defTeam, V2 goal, float maxDist);
    bool tryVolley(int i);
    int shootBtn(int human, bool pressedOnly) const;
};

enum Sfx { SFX_KICK = 0, SFX_WHISTLE, SFX_WHISTLE_LONG, SFX_GOAL, SFX_BOUNCE, SFX_POST, SFX_CROWD_OOH, SFX_CARD, SFX_WHISTLE_FINAL, SFX_PUNCH, SFX_BOO, SFX_THUNDER };
extern const char* METEO_NAMES[6];
int meteoForMonth(int month, int climate, uint64_t seed);   // month 0 août ... 10 juin ; climate 0 tempéré, 1 chaud, 2 hémisphère sud, 3 nordique
void meteoApply(MatchSetup& s, bool heated, int turf);       // terrain et affichage selon la météo
