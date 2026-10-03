#include "game.h"
bool careerRules(){return g_career.coach || (g_career.kind==CK_CLUB && !g_career.opts.lite && !g_career.euroOnly);}
int positionFamily(int p){return p==DP_GB?POS_GK:p<=DP_PIS_G?POS_DF:p<=DP_MOC?POS_MF:POS_FW;}
const char* detailedPositionName(int p){static const char* n[]={"GB","DD","DC","DG","PIS D","PIS G","MDC","MC","MD","MG","MOC","AD","AG","SA","BU"};return n[std::clamp(p,0,DP_COUNT-1)];}
const char* familiarityName(int v){return v>=90?"Naturel":v>=75?"Très bon":v>=55?"Correct":v>=30?"Dépannage":"Inadapté";}
int slotPosition(int f,int s){if(s==0)return DP_GB;const auto& F=FORMATIONS[std::clamp(f,0,NUM_FORMATIONS-1)];s=std::clamp(s,1,10)-1;float x=F.x[s],y=F.y[s];int nd=0;for(int k=0;k<10;k++)nd+=F.role[k]==1;
 if(F.role[s]==1){if(nd>=4&&(x<.22f||x>.78f))return nd>=5?(x<.5f?DP_PIS_G:DP_PIS_D):(x<.5f?DP_DG:DP_DD);return DP_DC;}
 if(y>=.5f&&x>.3f&&x<.7f&&y<.6f)return DP_MOC;
 if(y>=.5f&&(x<.25f||x>.75f))return x<.5f?DP_AG:DP_AD;
 if(F.role[s]==2){if(x<.22f||x>.78f)return nd==3?(x<.5f?DP_PIS_G:DP_PIS_D):(x<.5f?DP_MG:DP_MD);return y<=.36f?DP_MDC:DP_MC;}
 return DP_BU;
}
static float closeness(int a,int b){if(a==b)return 1; if(a==DP_GB||b==DP_GB)return 0;
 bool leftA=a==DP_DG||a==DP_PIS_G||a==DP_MG||a==DP_AG,leftB=b==DP_DG||b==DP_PIS_G||b==DP_MG||b==DP_AG;
 bool wideA=a==DP_DD||leftA||a==DP_PIS_D||a==DP_MD||a==DP_AD,wideB=b==DP_DD||leftB||b==DP_PIS_D||b==DP_MD||b==DP_AD;
 float row[]={0,1,1,1,1.5f,1.5f,2,2.5f,2.5f,2.5f,3,3.5f,3.5f,3.5f,4};
 return std::clamp(1.f-std::abs(row[a]-row[b])*.22f-(wideA!=wideB?.18f:0.f)-(wideA&&wideB&&leftA!=leftB?.18f:0.f),.08f,.92f);}
void initPositions(Player& p){if(p.positions.initialized&&positionFamily(p.positions.primary)==p.pos)return;p.positions=PositionKnowledge();auto& q=p.positions;unsigned h=(unsigned)p.id*2654435761u;q.initialized=1;q.foot=h%7==0?2:h%3==0?1:0;
 int df[]={DP_DC,DP_DC,DP_DD,DP_DG,DP_PIS_D,DP_PIS_G},mf[]={DP_MC,DP_MC,DP_MDC,DP_MD,DP_MG,DP_MOC},fw[]={DP_BU,DP_BU,DP_AD,DP_AG,DP_SA};q.primary=p.pos==0?DP_GB:p.pos==1?df[h%6]:p.pos==2?mf[h%6]:fw[h%5];
 for(int i=0;i<DP_COUNT;i++)q.familiarity[i]=(uint8_t)(i==q.primary?95:std::clamp((int)(closeness(q.primary,i)*80-10),0,85));}
int positionFamiliarity(const Player& p,int pos){Player copy=p;initPositions(copy);return copy.positions.familiarity[std::clamp(pos,0,DP_COUNT-1)];}
void learnPosition(Player& p,int pos,int minutes,int staff){if(pos<0||pos>=DP_COUNT||minutes<=0)return;initPositions(p);auto& q=p.positions;if(q.familiarity[pos]>=100||(q.primary==DP_GB)!=(pos==DP_GB))return;
 float rate=closeness(q.primary,pos)*(p.age<=21?1.5f:p.age>=31?.55f:1.f)*(0.5f+p.posi()/100.f)*(1.f+.08f*std::clamp(staff,0,10));
 rate*=personalityLearningMultiplier(g_career,p);
 int credit=std::min(50000,(int)q.practice[pos]+std::max(1,(int)(std::min(minutes,10000)*rate)));
 while(credit>=360&&q.familiarity[pos]<100){q.familiarity[pos]++;credit-=360;}q.practice[pos]=(uint16_t)credit;}
static const char* roles[DP_COUNT][8]={
 {"Classique","Relanceur","Libéro"},{"Latéral défensif","Latéral soutien","Latéral offensif","Latéral inversé"},{"Défenseur","Stoppeur","Couverture","Relanceur","Libéro"},{"Latéral défensif","Latéral soutien","Latéral offensif","Latéral inversé"},
 {"Piston prudent","Piston standard","Piston offensif"},{"Piston prudent","Piston standard","Piston offensif"},
 {"Sentinelle","Récupérateur","Meneur en retrait","Demi-centre"},{"Soutien","Box-to-box","Récupérateur","Meneur de jeu","Mezzala"},
 {"Milieu de couloir","Ailier","Meneur excentré"},{"Milieu de couloir","Ailier","Meneur excentré"},
 {"Meneur avancé","Numéro 10","Attaquant soutien","Trequartista"},{"Ailier","Ailier offensif","Attaquant intérieur","Meneur excentré"},{"Ailier","Ailier offensif","Attaquant intérieur","Meneur excentré"},
 {"Second attaquant","Attaquant libre","Neuf et demi"},{"Buteur","Renard","Pivot","Complet","Profondeur","Faux neuf","Attaquant pressing"}};
int tacticalRoleCount(int p){int n=0;while(n<8&&roles[std::clamp(p,0,DP_COUNT-1)][n])n++;return n;}
const char* tacticalRoleName(int p,int r){p=std::clamp(p,0,DP_COUNT-1);return roles[p][std::clamp(r,0,tacticalRoleCount(p)-1)];}
const char* tacticalInstructionName(int i){static const char* n[]={"Rester derrière","Monter","Presser davantage","Presser moins","Décrocher","Appels profondeur","Repiquer","Rester large","Centrer tôt","Centrer au fond","Marquage individuel","Liberté créative","Jouer simple","Tirer davantage","Garder le ballon","Dribbler plus","Dribbler moins"};return n[std::clamp(i,0,TI_COUNT-1)];}
int roleAptitude(const Player& p,int pos,int role){float v;
 if(pos==DP_GB)v=role==0?(p.keep*3+p.posi())/4.f:(p.keep*2+p.pass+p.speed)/4.f;
 else if(pos==DP_DC)v=role==3||role==4?(p.tackle+p.posi()+p.pass*2)/4.f:(p.tackle*2+p.posi()+p.head())/4.f;
 else if(pos==DP_MDC)v=role==2?(p.pass*2+p.posi()+p.comp())/4.f:(p.tackle*2+p.posi()+p.stamina)/4.f;
 else if(pos==DP_MC&&role==1)v=(p.stamina*2+p.speed+p.tackle+p.pass+p.posi())/6.f;
 else if(pos==DP_BU)v=role==2?(p.head()*2+p.shoot+p.comp())/4.f:role==5?(p.pass*2+p.drib()+p.comp())/4.f:(p.shoot*2+p.speed+p.posi())/4.f;
 else if(pos>=DP_AD)v=(p.drib()*2+p.speed+p.shoot+p.pass)/5.f;
 else v=(p.pass*2+p.stamina+p.posi()+p.drib())/5.f;
 return std::clamp((int)v,0,100);}
SlotTactic defaultSlotTactic(int p,int model,int slot){SlotTactic t;t.duty=p==DP_GB||p==DP_DC||p==DP_MDC?DUTY_DEFENSE:p>=DP_AD?DUTY_ATTACK:DUTY_SUPPORT;
 if(model==0){if(p==DP_BU)t.role=5;if(p==DP_AD||p==DP_AG)t.role=2;if(p==DP_MDC)t.role=2;if(p==DP_DD||p==DP_DG){t.role=2;t.duty=DUTY_ATTACK;}if(p==DP_DC)t.role=3;if(p==DP_MC)t.role=slot%2?1:3;}
 else {if(p==DP_BU)t.role=slot%2?2:4;if(p==DP_DD||p==DP_DG)t.role=0;if(p==DP_MC)t.role=2;if(p==DP_GB)t.role=0;}
 return t;}
RoleEffects roleEffects(int p,const SlotTactic& t){RoleEffects e;e.advance=t.duty==DUTY_ATTACK?.07f:t.duty==DUTY_DEFENSE?-.05f:0;
 int r=t.role%tacticalRoleCount(p);
 if(p==DP_DC){if(r==1){e.press=1.4f;e.advance+=.035f;}if(r==2){e.press=.7f;e.advance-=.055f;}if(r>=3)e.risk=1.2f;}
 if(p==DP_DD||p==DP_DG){e.advance+=(r==0?-.05f:r==2?.09f:0);if(r==3)e.width=.45f;}
 if(p==DP_PIS_D||p==DP_PIS_G)e.advance+=.06f+(r-1)*.04f;
 if(p==DP_MDC){if(r==0)e.advance-=.055f;if(r==1)e.press=1.4f;if(r==2)e.risk=1.25f;if(r==3)e.halfback=true;}
 if(p==DP_MC){if(r==1){e.advance+=.04f;e.press=1.25f;}if(r==2)e.press=1.4f;if(r==3)e.risk=1.3f;if(r==4)e.width=1.4f;}
 if(p==DP_MOC){e.risk=1.3f;if(r==1)e.shot=1.2f;if(r==2)e.advance+=.05f;if(r==3){e.press=.7f;e.dribble=1.25f;}}
 if(p==DP_AD||p==DP_AG){e.width=1.15f;if(r==2){e.width=.45f;e.shot=1.4f;}if(r==3){e.risk=1.3f;e.drop=true;}}
 if(p==DP_MD||p==DP_MG){e.width=1.15f;if(r==2)e.risk=1.3f;}
 if(p==DP_BU){if(r==1){e.shot=1.4f;e.advance+=.04f;}if(r==2){e.hold=2;e.dribble=.7f;}if(r==3){e.risk=1.15f;e.dribble=1.15f;}if(r==4)e.advance+=.08f;if(r==5){e.drop=true;e.advance-=.12f;e.risk=1.3f;}if(r==6)e.press=1.5f;}
 if(p==DP_SA){e.drop=true;e.risk=1.2f;if(r==1){e.dribble=1.3f;e.width=1.25f;}if(r==2){e.advance+=.05f;e.shot=1.25f;}}
 if(p==DP_GB){if(r==1)e.risk=1.25f;if(r==2)e.advance=.035f;}
 auto has=[&](int i){return (t.instructions&(1u<<i))!=0;};
 if(has(TI_STAY))e.advance-=.08f;if(has(TI_ADVANCE))e.advance+=.08f;if(has(TI_PRESS))e.press*=1.4f;if(has(TI_NO_PRESS))e.press*=.65f;if(has(TI_DROP))e.drop=true;if(has(TI_DEPTH))e.advance+=.07f;
 if(has(TI_INSIDE))e.width*=.5f;if(has(TI_WIDE))e.width*=1.4f;if(has(TI_EARLY_CROSS))e.crossAt=.65f;if(has(TI_DEEP_CROSS))e.crossAt=.9f;if(has(TI_FREE))e.risk*=1.3f;if(has(TI_SIMPLE))e.risk*=.65f;if(has(TI_SHOOT))e.shot*=1.5f;if(has(TI_HOLD))e.hold+=3;if(has(TI_DRIBBLE))e.dribble*=1.5f;if(has(TI_NO_DRIBBLE))e.dribble*=.6f;return e;}

int teamSlotPosition(const Team& T,int f,int slot){int base=slotPosition(f,slot);int requested=T.tactical.slot[std::clamp(slot,0,10)].position;return T.tactical.customized&&requested<DP_COUNT&&positionFamily(requested)==positionFamily(base)?requested:base;}
