#pragma once
#include <map>
#include <vector>
#include <string>
#include <cstdint>
struct Player;struct Career;struct Writer;struct Reader;struct MatchSetup;struct Competition;struct MatchRes;
enum PersonalityTrait { PT_AMBITION,PT_PROFESSIONALISM,PT_LOYALTY,PT_AGGRESSION,PT_LEADERSHIP,PT_COMPOSURE,PT_CONSISTENCY,PT_BIG_MATCH,PT_SENSITIVITY,PT_ADAPTABILITY,PT_COUNT };
struct PlayerPersonality {int ambition=50,professionalism=50,loyalty=50,aggression=50,leadership=50,composure=50,consistency=50,bigMatchTemperament=50,sensitivity=50,foreignAdaptability=50;};
struct PlayerClubRelation {int clubId=-1,attachment=20,fanPopularity=20,managerRelationship=60,squadInfluence=30,adaptationProgress=100,joinedYear=0,seasons=0,captaincy=0,mentorPid=0,temporary=0,lastMonth=-1,lastApps=0,lastGoals=0,benchStreak=0,transferRequested=0,lastTier=-1,stagnantSeasons=0,formedHere=0,lastDecision=-1;uint32_t coachHash=0;};
struct PersonalityEvent {int year=0,club=-1,type=0;char text[160]={};};
struct PersonalityPlayer {int pid=0,activeClub=-1,retired=0,knowledge=0,scoutQuality=0,lastYear=0,lastAge=0;char name[80]={};PlayerPersonality traits;std::vector<PlayerClubRelation> relations;std::vector<PersonalityEvent> events;};
struct PersonalityState {std::map<int,PersonalityPlayer> players;std::vector<uint64_t> matches;int matchYear=0;};
const char* personalityTraitName(int trait);const char* personalityGrade(int value);int personalityTrait(const PlayerPersonality&,int trait);void personalitySetTrait(PlayerPersonality&,int trait,int value);
PlayerPersonality generatePersonality(const Player&);PlayerPersonality personalityTraits(const Career&,const Player&);std::string personalitySummary(const PlayerPersonality&,int age);
float personalityTrainingMultiplier(const Career&,const Player&);float personalityLearningMultiplier(const Career&,const Player&);int personalityAnnualGain(const Career&,const Player&,int gain);
int personalitySalary(const Career&,const Player&,int club,int base);int personalityCaptainScore(const Career&,const Player&,int club);int personalityLegendBonus(const Career&,int pid,int club);
float personalityMatchMultiplier(Career&,const Player&,int club,int matchKey,bool important,float pressure);float personalityTeamMultiplier(Career&,int club,const MatchRes&,const Competition*);
float personalityFoulMultiplier(const PlayerPersonality&);float personalityStressMultiplier(const PlayerPersonality&);
bool personalityBigMatch(const Career&,int ci,int mi);bool personalityBigMatch(const Career&,const MatchRes&,const Competition*);
std::string personalityScoutReport(const Career&,const Player&,int club);int personalityRecruitScore(const Career&,const Player&,int style);
void personalitySave(Writer&,const Career&);void personalityLoad(Reader&,Career&);
