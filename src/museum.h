#pragma once
#include <map>
#include <vector>
#include <cstdint>
struct Writer; struct Reader; struct Career; struct Player;
enum MuseumCategory { MH_ALL, MH_TROPHY, MH_MOVE, MH_PLAYER, MH_STADIUM, MH_RECORD, MH_EUROPE, MH_MANAGER };
enum MuseumRecordKind { MR_APPS,MR_GOALS,MR_ASSISTS,MR_CLEAN,MR_LOYALTY,MR_YOUNGEST,MR_OLDEST,MR_YOUNG_GOAL,MR_OLD_GOAL,MR_BIG_WIN,MR_BIG_LOSS,MR_TOTAL_GOALS,MR_UNBEATEN,MR_WIN_STREAK,MR_CLEAN_STREAK,MR_POINTS,MR_WINS,MR_SEASON_GF,MR_SEASON_GA,MR_GD,MR_ATTENDANCE,MR_BUY,MR_SELL,MR_BUDGET,MR_SQUAD_VALUE,MR_SEASON_GOALS,MR_COUNT };
struct MuseumRecord { int kind=0,year=0,pid=0; int64_t value=0; double time=0; char holder[64]={},competition[96]={}; };
struct MuseumEvent { int year=0,category=0; double time=0; char text[180]={}; };
struct MuseumSeason {
 int year=0,league=-1,tier=-1,newTier=-1,position=0,played=0,wins=0,draws=0,losses=0,gf=0,ga=0,points=0;
 int scorerPid=0,assisterPid=0,playerPid=0,scorerGoals=0,assisterCount=0,awards=0,trophies=0,averageAttendance=0,maxAttendance=0;
 int64_t budgetStart=0,budgetEnd=0; int attendanceTotal=0,homeMatches=0;
 uint8_t finished=0,promoted=0,relegated=0,historic=0,budgetKnown=0;
 char competition[96]={},division[96]={},newDivision[96]={},manager[64]={},scorer[64]={},assister[64]={},bestPlayer[64]={};
};
struct MuseumMatch {
 int year=0,comp=-1,match=-1,home=-1,away=-1,hg=0,ag=0,ph=-1,pa=-1,winner=-1,attendance=-1;
 double time=0; uint32_t reasons=0; uint8_t extraTime=0,final=0,exactLineup=0;
 char competition[96]={},homeName[64]={},awayName[64]={},venue[96]={},captain[2][64]={};
 int xi[2][11]={}; char lineupNames[2][11][48]={};
};
struct MuseumTrophy { int year=0,comp=-1,kind=0; double time=0; char name[96]={}; MuseumMatch finalMatch; };
struct MuseumPlayer {
 int pid=0,apps=0,goals=0,assists=0,clean=0,seasons=0,captainMatches=0,trophies=0,awards=0,leagueGoals=0,cupGoals=0,euroGoals=0;
 int observedYear=0,seenApps=0,seenGoals=0,seenAssists=0,countedYear=-1,seasonGoals=0;
 uint8_t hall=0; char name[64]={};
 int legendScore() const {return apps+goals*3+assists*2+seasons*25+captainMatches*2+trophies*50+awards*60+clean*3;}
};
struct MuseumManager { int fromYear=0,toYear=0,apps=0,wins=0,draws=0,losses=0,trophies=0,promotions=0; char name[64]={},departure[64]={}; };
struct MuseumStadium { int fromYear=0,toYear=0,capacityStart=0,capacityEnd=0,recordAttendance=0; char name[96]={},town[64]={}; };
struct MuseumShirt { int year=0; unsigned shirt[3]={},shirt2[3]={},shorts[3]={},socks[3]={}; int pattern[3]={}; uint8_t third=0; char sponsor[64]={}; };
struct ClubHistory {
 int clubId=-1,firstYear=0,seenYear=-1,unbeaten=0,winStreak=0,cleanStreak=0; uint32_t firstFlags=0;
 std::vector<MuseumSeason> seasons; std::vector<MuseumTrophy> trophies; std::vector<MuseumMatch> finals,legendaryMatches;
 std::vector<MuseumRecord> records; std::vector<MuseumEvent> timeline; std::vector<MuseumPlayer> players;
 std::vector<MuseumManager> managers; std::vector<MuseumStadium> stadiums; std::vector<MuseumShirt> shirts;
 std::vector<uint64_t> seenMatches,seenEvents,seenGates;
};
const char* museumRecordName(int kind);
const char* museumCategoryName(int kind);
void museumSave(Writer& w,const Career& K);
void museumLoad(Reader& r,Career& K);
