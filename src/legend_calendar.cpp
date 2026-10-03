#include "game.h"

// Historical edition boundaries supplied for the Legends mode. Intermediate
// match days are spread by stage; this is not an archive of exact fixture dates.
struct Dates { int sm,sd,em,ed; };
static const Dates D[] = {
 {7,13,7,30},{5,27,6,10},{6,4,6,19},{6,24,7,16},
 {6,16,7,4},{6,8,6,29},{5,30,6,17},{7,11,7,30},
 {5,31,6,21},{6,13,7,7},{6,1,6,25},{6,13,7,11},
 {5,31,6,29},{6,8,7,8},{6,17,7,17},{6,10,7,12},
 {5,31,6,30},{6,9,7,9},{6,11,7,11},{6,12,7,13},
 {6,14,7,15},{11,20,12,18}
};
static int ordinal(int y,int m,int d) {
 static const int days[]={31,28,31,30,31,30,31,31,30,31,30,31};
 for(int i=1;i<m;i++) d+=days[i-1]+(i==2 && y%4==0 && (y%100!=0 || y%400==0));
 return d;
}
static int firstMonday(int y) {
 int w=(y+y/4-y/100+y/400+1+1)%7;
 return ordinal(y,8,1)+(7-(w+6)%7)%7;
}
double legendCalendarTime(int ed,double fraction) {
 if(ed<0 || ed>=22) return fraction;
 int y=LEGENDS[ed].year;const Dates& d=D[ed];
 int start=ordinal(y,d.sm,d.sd),end=ordinal(y,d.em,d.ed);
 return (start+std::lround((end-start)*fraction)-firstMonday(y))/7.0;
}
double legendRoundTime(int ed,int teams,const std::string& name) {
 if(ed<0 || ed>=22) return 0;
 if(name=="Match pour la 3e place") return legendCalendarTime(ed,1)-1.0/7;
 double f=teams<=2?1:teams<=4?.86:teams<=8?.72:teams<=16?.58:.50;
 if(LEGENDS[ed].format==2) f=teams<=2?1:teams<=4?.72:teams<=8?.43:0;
 return legendCalendarTime(ed,f);
}
double legendGroupTime(int ed,int stage,int round,int count) {
 int f=LEGENDS[ed].format;double a=0,b=.45;
 if(f==1) b=.59;
 if(f==3) {a=stage? .64:0;b=stage?1:.42;}
 if(f==5 || f==6) {a=stage?.46:0;b=stage?.76:.32;}
 return legendCalendarTime(ed,a+(b-a)*round/std::max(1,count-1));
}
std::string legendDateText(double time,int y) {
 static const char* names[]={"janvier","février","mars","avril","mai","juin","juillet","août","septembre","octobre","novembre","décembre"};
 int day=firstMonday(y)+(int)std::lround(time*7),m=1;
 while(m<12 && day>ordinal(y,m+1,1)-ordinal(y,m,1)) {day-=ordinal(y,m+1,1)-ordinal(y,m,1);m++;}
 return fmt("%d %s %d",day,names[m-1],y);
}
void legendRepairCalendar(Competition& c) {
 if(c.kind!=50 || c.tag<0 || c.tag>=22) return;
 for(int s=0;s<(int)c.stages.size();s++) {
  auto& st=c.stages[s];
  for(int r=0;r<(int)st.rounds.size();r++) st.rounds[r].time=st.type==ST_LEAGUE?
   legendGroupTime(c.tag,s,r,(int)st.rounds.size()):legendRoundTime(c.tag,(int)st.ties.size()*2,st.name);
 }
 c.koTimes.clear(); // Newly created stages use the edition calendar directly.
}
