#pragma once
// Vibrations des manettes (XInput sous Windows, sinon l'API de raylib quand la plateforme la gère)
// Deux moteurs : gauche (grave, lourd) et droit (aigu, sec). Les effets sont des motifs d'impulsions qui se superposent.
enum RumbleKind { RB_KICK = 0, RB_SHOT, RB_TACKLE, RB_FOULED, RB_POST, RB_GOAL_FOR, RB_GOAL_AGAINST, RB_SAVE, RB_CARD, RB_WHISTLE_END,
                  RB_HEARTBEAT, RB_BOUNCE_HEAD, RB_CROWD, RB_TEST, NUM_RUMBLE };
void rumbleStart(int pad, float strength, float seconds);   // pad 0..3, force 0..1 (les deux moteurs)
void rumblePlay(int pad, int kind);                         // motif prédéfini (multiplié par l'intensité réglée)
void rumbleSetStrength(float k);                            // 0 : coupé ... 1 : fort
const char* rumbleName(int kind);
void rumbleUpdate(float dt);                                 // à appeler à chaque image
void rumbleStopAll();
float rumbleLevel(int pad, int motor);                       // niveau actuel (affichage du test des vibrations)
