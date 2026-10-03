#pragma once
#include <map>
#include <vector>
#include <cstdint>
struct Career;struct Writer;struct Reader;struct Player;struct MatchSetup;
enum SupporterEventType { SU_TIFO,SU_PROTEST,SU_BOYCOTT,SU_TITLE_PARTY,SU_PROMOTION_PARTY,SU_TRIBUTE,SU_RECORD_ATTENDANCE,SU_LEGEND_RETURN,SU_FIRST_STADIUM,SU_HOSTILE,SU_SUBSCRIPTIONS,SU_RELEGATION };
enum SupporterChant { CH_NORMAL,CH_LOUD,CH_ENCOURAGE,CH_TENSE,CH_WHISTLES,CH_CELEBRATE,CH_PROTEST };
enum SupporterRivalryType { RV_LOCAL,RV_HISTORICAL,RV_SPORTING,RV_RECENT };
struct SupporterRivalry {int clubId=-1,intensity=0,type=RV_LOCAL,meetings=0,lastYear=0;};
struct SupporterAttendance {int year=0,matches=0,total=0,best=0,worst=0,capacity=0,subscribers=0;};
struct SupporterEvent {int type=0,year=0,intensity=0,duration=0,relatedComp=-1,relatedMatch=-1,relatedPlayer=0;double date=0;int presented=0;char reason[180]={};};
struct SupporterPopularity {int pid=0,value=0,lastYear=0,seenGoals=0,seenApps=0,seasons=0;};
struct SupporterGate {int year=0,comp=-1,match=-1,attendance=0,visitors=0,index=40,chant=0,tifo=-1;};
struct SupporterProfile {
 int clubId=-1,totalSupporters=50,seasonTicketHolders=0,waitingList=0,seasonTicketCapacity=0,activeCore=0;
 int casual=0,historical=0,international=0,loyalty=60,fervor=50,expectations=40,patience=60,localIdentity=70;
 int sportingSatisfaction=60,managerTrust=60,boardTrust=60,transferTrust=60,atmosphereIndex=40;
 int protestLevel=0,boycottMatches=0,crisisMatches=0,lossStreak=0,winStreak=0,seriousTrigger=0,triggerYear=0;
 int seasonYear=0,campaignYear=0,archiveYear=0,seasonMatches=0,seasonAttendance=0,seasonBest=0,seasonWorst=0;
 int recordAttendance=0,recordYear=0,recordComp=-1,recordMatch=-1,recordOpponent=-1;double recordDate=0;
 int lastAttendance=0,lastVisitors=0,lastChant=CH_NORMAL,lastTifo=-1,firstStadiumPending=0,celebrationPending=0;
 int homeGames=17,firstEuropean=0,knownCapacity=0,knownPrice=0,lastTier=-1;char recordCompetition[96]={},recordOpponentName[64]={},recordDateText[64]={};char knownStadium[96]={},knownTown[64]={},knownCoach[64]={};
 int64_t subscriptionEuros=0;std::vector<SupporterRivalry> rivalries;std::vector<SupporterAttendance> attendanceHistory;
 std::vector<SupporterEvent> supporterEvents;std::vector<SupporterGate> gates;std::vector<SupporterPopularity> players;std::vector<uint64_t> seenMatches,seenAttendance,preparedMatches,bookedMoney;
};
struct SupporterState {std::map<int,SupporterProfile> profiles;int initialized=0;};
struct MatchAtmosphere {int attendance=0,visitors=0,index=40,chant=CH_NORMAL,tifo=-1,rivalry=0,distance=0;float homeEffect=0,awayPressure=0;};
const char* supporterAtmosphereName(int index,int protest=0);const char* supporterChantName(int chant);const char* supporterEventName(int type);
int supporterDistance(int home,int away);int supporterPrice(int club);int supporterFanPopularity(const Career& k,int club,int pid);
MatchAtmosphere supportersMatchAtmosphere(Career* k,int home,int away,int comp=-1,int mi=-1,int weather=0,bool neutral=false);
void supportersApplySetup(Career* k,MatchSetup& setup,int comp=-1,int mi=-1);
int supportersLiveChant(const MatchSetup& setup,int homeGoals,int awayGoals,float minute);
void supportersSave(Writer& w,const Career& k);void supportersLoad(Reader& r,Career& k);

float supporterPlayerMultiplier(const MatchSetup& setup,int team,int age);float supporterPenaltyPressure(const MatchSetup& setup,int team);
