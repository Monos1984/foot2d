#pragma once
#include <vector>
#include <string>
#include <cstdint>
struct Career;struct Writer;struct Reader;struct Player;
enum DirectorDomain { DD_SEARCH,DD_SHORTLIST,DD_OFFERS,DD_FEES,DD_CONTRACTS,DD_RENEWALS,DD_SALES,DD_LIST,DD_LOAN_IN,DD_LOAN_OUT,DD_LOAN_CLUB,DD_AGENTS,DD_YOUTH,DD_STAFF,DD_STAFF_RENEW,DD_COUNT };
enum DirectorMode { DM_MANUAL,DM_ADVICE,DM_NEGOTIATE,DM_AUTO };
enum DirectorSkill { DK_RECRUIT,DK_NEGOTIATE,DK_SELL,DK_CONTRACT,DK_LOAN,DK_MARKET,DK_NATIONAL,DK_INTERNATIONAL,DK_YOUTH,DK_ANALYSIS,DK_AGENTS,DK_STAFF,DK_COUNT };
enum DirectorPhilosophy { DP_YOUNG,DP_EXPERIENCED,DP_LOCAL,DP_NATIONAL,DP_INTERNATIONAL,DP_FREE,DP_TRADING,DP_KEEP,DP_LOW_WAGE,DP_ACADEMY,DP_VERSATILE,DP_TACTICAL,DPH_COUNT };
enum DirectorStatus { DS_UNTOUCHABLE,DS_IMPORTANT,DS_ROTATION,DS_AVAILABLE,DS_FOR_SALE,DS_FOR_LOAN };
enum DirectorPhase { DN_QUEUED,DN_CLUB,DN_PLAYER,DN_APPROVAL,DN_DONE,DN_FAILED,DN_PAUSED,DN_CANCELLED };
struct DirectorProfile {char name[32]={};int age=0,nation=-1,wage=0,years=3,reputation=0,experience=0,level=0;uint8_t skill[DK_COUNT]={};uint8_t philosophy=DP_LOCAL;};
struct DirectorLimits {int maxFee=0,maxWage=0,minAge=16,maxAge=34,budgetPct=35;uint8_t configured=0;int64_t budgetBase=0,used=0,boardFees=0,wageCap=0,boardWages=0;};
struct DirectorRequest {int position=-1,role=-1,minAge=16,maxAge=34,minLevel=0,profile=-1,loanWagePct=50;};
struct DirectorNeed {int position=0,required=0,available=0,academy=0,priority=0;char reason[128]={};};
struct DirectorTarget {int pid=0,club=-1,age=0,position=0,level=0,potential=0,fee=0,wage=0,score=0,interest=0,knowledge=0;uint8_t freePlayer=0,loan=0;char reason[160]={};};
struct DirectorNegotiation {int pid=0,from=-1,phase=DN_QUEUED,attempts=0,fee=0,wage=0,years=3,kind=0,wagePct=50;double next=0,started=0;uint8_t chosen=0,approved=0;char message[180]={};};
struct DirectorPlayerPolicy {int pid=0,status=DS_ROTATION;uint8_t noSell=0;};
struct DirectorContract {int pid=0,months=0,priority=0;char reason[120]={};};
struct DirectorLoanTerms {int pid=0,from=-1,to=-1,year=0,option=0,wagePct=50;};
struct DirectorStaffContract {int role=0,years=3;char name[32]={};};
struct DirectorState {
 int club=-1,policyYear=0,staffYear=0,announcedYear=0;double lastResearch=-100,lastTick=-100;uint64_t rng=0xD1AEC70;
 DirectorProfile profile;DirectorLimits limits;DirectorRequest request;uint8_t mode[DD_COUNT]={};
 std::vector<DirectorNeed> needs;std::vector<DirectorTarget> targets;std::vector<DirectorNegotiation> negotiations;
 std::vector<DirectorPlayerPolicy> policies;std::vector<DirectorContract> contracts;std::vector<DirectorLoanTerms> loanTerms;std::vector<DirectorStaffContract> staffContracts;
 std::vector<std::string> report;
};
const char* directorDomainName(int i);const char* directorModeName(int i);const char* directorSkillName(int i);const char* directorPhilosophyName(int i);const char* directorStatusName(int i);const char* directorPhaseName(int i);
void directorSave(Writer& w,const Career& K);void directorLoad(Reader& r,Career& K);
