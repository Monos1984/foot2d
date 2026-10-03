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
void audioJingle(int j);            // 0 victoire, 1 générique TV, 2 trophée, 3 hymne (plateau européen), 4 podium, 5-7 publicités
#include <string>
#include <vector>
#include <map>
void audioAnthemCode(const std::string& code);   // hymne national d'une sélection (code FIFA) : fichier choisi, mélodie réelle ou hymne générique
void audioStopAnthem();
// test des sons : catégories 0 bruitages, 1 ambiance et chants, 2 musiques des menus, 3 jingles, 4 hymnes
int audioPreviewCount(int cat);
std::string audioPreviewName(int cat, int i);
void audioPreview(int cat, int i);
void audioPreviewStop();
bool audioPreviewPlaying();
void audioAnthemPlayIndex(int idx);              // >= 0 hymne réel, < 0 hymne générique
int audioAnthemListCount();
int audioAnthemListIndex(int i);
std::string audioAnthemListName(int i);
std::string audioAnthemListCredit(int i);
std::vector<float> audioRenderAnthem(int idx, int* sampleRate);
std::vector<float> audioRenderCrowd(int what, int i);   // 0 chant i, 1 applaudissements, 2 sifflets
bool audioAnthemPlaying();
float audioAnthemSeconds();                       // durée de l'hymne en cours
const char* audioAnthemName(const std::string& code);
void audioSetAnthemFile(const std::string& code, const std::string& path);
std::string audioAnthemFile(const std::string& code);
const std::map<std::string, std::string>& audioAnthemFiles();
void audioUpdate();                               // flux musicaux (hymnes en fichier externe)

void audioSupporters(int state,float strength);
void audioWalkoutPlay(const std::string& path);   // musique d'entrée des joueurs (fichier)
void audioWalkoutStop();
void audioWalkoutFade();
bool audioWalkoutPlaying();
