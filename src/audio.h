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
#include <string>
#include <map>
void audioAnthemCode(const std::string& code);   // hymne national d'une sélection (code FIFA) : fichier choisi, mélodie réelle ou hymne générique
void audioStopAnthem();
bool audioAnthemPlaying();
float audioAnthemSeconds();                       // durée de l'hymne en cours
const char* audioAnthemName(const std::string& code);
void audioSetAnthemFile(const std::string& code, const std::string& path);
std::string audioAnthemFile(const std::string& code);
const std::map<std::string, std::string>& audioAnthemFiles();
void audioUpdate();                               // flux musicaux (hymnes en fichier externe)

void audioSupporters(int state,float strength);
