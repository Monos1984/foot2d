#pragma once
#include <vector>
#include <cstdint>
struct Career;struct Writer;struct Reader;
enum { SD_RECRUIT,SD_NEGOTIATE,SD_SELL,SD_CONTRACT,SD_LOAN,SD_MARKET,SD_NATIONAL,SD_INTERNATIONAL,SD_YOUTH,SD_ANALYSIS,SD_AGENTS,SD_STAFF,SD_COACH,SD_DIPLOMACY,SD_VISION,SD_SKILLS };
struct SportingProfile {char first[32]="Jean",last[32]="Martin";int age=35,nation=0,experience=0,reputation=20,specialty=0,philosophy=2,start=0;uint8_t skills[SD_SKILLS]={};uint16_t xp[SD_SKILLS]={};};
struct SportingCoach {char name[48]={};int club=-1,age=45,reputation=30,philosophy=2,formation=0,style=1,youth=50,discipline=50,adaptation=50,dressing=50,demand=50,wage=0,years=2,lastYear=0;};
struct SportingPolicy {int averageAge=25,youth=50,local=50,youthBudgetPct=20,over30=5,loanStrategy=1,saleStrategy=1,wages=1;};
struct SportingPlayerPlan {int pid=0,status=2,position=-1,successor=0,path=0,minPrice=0,wantedPrice=0,quick=0;};
struct SportingScouting {int nation=-1,region=-1,position=-1,minAge=16,maxAge=30,minPotential=0;};
struct SportingNetwork {int nation=-1,knowledge=20;};
struct SportingAgent {int id=0,reputation=30,relation=50,demand=50;char name[48]={};};
struct SportingHistory {int club=-1,from=0,to=0,seasons=0,promotions=0,trophies=0,coaches=0,young=0;int64_t spent=0,sales=0;};
struct SportingTrade {int pid=0,club=-1,boughtYear=0,soldYear=0,fee=0,salary=0,sale=0;char name[48]={};int64_t wageTotal=0;};
struct SportingMeeting {int year=0,month=0,kind=0;char text[220]={};};
struct SportingJob {int club=-1,wage=0,years=3,power=0,interested=0,refused=0,negotiated=0;};
struct SportingPartner {int club=-1,type=0,year=0;};
struct SportingCareer {
 uint8_t active=0,employed=0;SportingProfile profile;SportingPolicy policy;SportingScouting scouting;
 int contractYears=3,wage=0,trust=50,coachTrust=50,power=0,objective=0,relationMonth=-1,relationYear=0,seasonYear=0,departure=0,applicationMonth=-1;
 int requestPosition=-1,requestState=0,requestYear=0,requestMonth=-1,approvedPid=0,approvedFee=0,approvedWage=0,approvedCoach=0;
 int plan[3]={0,1,2},planStart=2026,planApproved=0;int64_t transferBudget=0,staffBudget=0,scoutBudget=0,youthBudget=0,wageStart=0,salesGoal=0;
 char reason[220]={};std::vector<SportingCoach> coaches;std::vector<SportingPlayerPlan> players;std::vector<SportingNetwork> network;std::vector<SportingAgent> agents;std::vector<SportingHistory> history;std::vector<SportingTrade> trades;std::vector<SportingMeeting> meetings;std::vector<SportingJob> jobs;std::vector<SportingPartner> partners;std::vector<uint64_t> matchKeys;
};
const char* sportingReputation(int rep);const char* sportingPower(int p);const char* sportingObjective(int o);const char* sportingStatus(int i);
SportingCoach sportingCoachCandidate(int club,int level,int seed);int sportingCompatibility(const SportingCareer& s,const SportingCoach& c);
void sportingSave(Writer& w,const Career& k);void sportingLoad(Reader& r,Career& k);
