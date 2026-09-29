#pragma once
void audioInit();
void audioShutdown();
void audioPlay(int sfx);
void audioCrowd(bool on, float vol);
void audioSetEnabled(bool sfx);
void audioMusic(bool on);
void audioMusicEnabled(bool e);
void audioSetTrackMode(int mode);   // 0 enchaînement, 1..N thème fixe
int audioTrackCount();
const char* audioTrackName(int i);
int audioCurrentTrack();
void audioJingle(int j);            // 0 victoire, 1 générique TV, 2 trophée, 3 hymne (plateau européen), 4 podium
void audioAnthem(unsigned seed);   // hymne national (mélodie originale propre à l'équipe)
void audioStopAnthem();
