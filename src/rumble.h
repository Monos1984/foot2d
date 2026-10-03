#pragma once
// Vibrations des manettes (XInput sous Windows ; sans effet ailleurs)
void rumbleStart(int pad, float strength, float seconds);   // pad 0..3, force 0..1
void rumbleUpdate(float dt);                                   // à appeler à chaque image (arrêt en fin de durée)
void rumbleStopAll();
