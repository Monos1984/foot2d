// Championship-specific ceremony, next-season club planning, and simulated prospects.
static int g_awardEdition=-1,g_awardPage=0;
static bool g_awardReplay=false,g_plan[4]={};
static float g_awardTime=0;
static int g_planCursor=4;
static ListW g_poleLW;
static int managedRoot(){int u=g_career.userTeam;return g_world.teams[u].parent>=0?g_world.teams[u].parent:u;}
static int youthSection(int root,int youth){for(int t=0;t<(int)g_world.teams.size();t++)if(g_world.teams[t].parent==root&&g_world.teams[t].youth==youth)return t;return -1;}
static void openSeasonAwards(bool replay){
 g_awardReplay=replay;g_awardEdition=g_career.prepareSeasonAwards();
 if(g_awardEdition<0 && replay && !g_career.seasonAwards.empty())g_awardEdition=(int)g_career.seasonAwards.size()-1;
 g_awardPage=0;g_awardTime=0;g_screen=SC_SEASONAWARDS;
 if(g_awardEdition>=0)audioJingle(1);
}
// podium final du championnat de l'équipe dirigée (3 premiers), si la saison est encore disponible
static bool seasonPodium(int out[3],std::string& comp){
 int p,q,g,u=g_career.userTeam;
 if(g_career.tierOfTeam(u,&p,&q,&g)<0)return false;
 const Competition& C=g_career.season.comps[g_career.pyramids[p].pools[q].comps[g]];
 if(C.result.size()<3)return false;
 for(int i=0;i<3;i++){out[i]=C.result[i];}
 comp=C.name;return true;
}
static void screenSeasonAwards(){
 drawBackground("La soirée du championnat");
 if(g_awardEdition<0 || g_awardEdition>=(int)g_career.seasonAwards.size()){
  drawTextCentered("Les récompenses seront remises à la fin de votre championnat.",VW/2,130,10,C_HI);
  drawTextCentered("Joueurs, buteur, passeur, gardien, espoir et entraîneur.",VW/2,152,10,C_TXT);
  if(button(220,280,200,16,"Retour à la carrière")||IN.back||IN.ok)openHub();return;
 }
 auto& a=g_career.seasonAwards[g_awardEdition];g_awardTime+=GetFrameTime();
 float t=(float)GetTime();
 static const char* labels[]={"Joueur de la saison","Meilleur buteur","Meilleur passeur","Gardien de la saison","Espoir de la saison (21 ans et moins)","Entraîneur de la saison","Podium du championnat"};
 int podium[3]={-1,-1,-1};std::string podComp;
 bool hasPodium=!g_awardReplay&&seasonPodium(podium,podComp);
 int last=hasPodium?6:5;
 if(g_awardPage>last)g_awardPage=last;
 drawTextCentered(fitText(a.competition,VW-70,10),VW/2,36,10,C_HI);
 drawTextCentered(fmt("Saison %d / %d  -  Distinction %d sur %d",a.year,a.year+1,g_awardPage+1,last+1),VW/2,50,10,C_DIM);
 bool reveal=g_awardReplay||g_awardTime>=1.5f;
 const int X0=20,X1=VW-20,Y0=62,Y1=304,FLOOR=250;
 stageBackdrop(X0,Y0,X1,Y1,t,C_HI,reveal);
 int sy=Y0+6;
 drawTextCentered(labels[g_awardPage],VW/2,sy+6,10,C_HI,false);
 stageFloor(X0+34,FLOOR,X1-34,Y1-FLOOR,C_HI);
 static const Color CONF[5]={{255,214,80,255},{255,255,255,255},{90,200,255,255},{255,90,110,255},{140,255,120,255}};
 if(g_awardPage==6){
  // podium : maillots des trois premiers, champion avec le trophée
  drawTextCentered(fitText(podComp,220,10),VW/2,sy+22,10,C_TXT,false);
  for(int rank=2;rank>=0;--rank){
   if(!reveal&&g_awardTime<(3-rank)*0.45f)continue;
   int team=podium[rank];if(team<0)continue;
   int x=rank==0?VW/2:rank==1?200:440,top=rank==0?196:rank==1?212:222;
   podiumBlock(x,top,FLOOR,rank==0?110:96,rank);
   const Team& T=g_world.teams[team];
   for(int k=0;k<3;k++){int px=x-22+k*22;int sq=k<(int)T.squad.size()?k*3:0;int skin=sq<(int)T.squad.size()?T.squad[sq].skin:0,hair=sq<(int)T.squad.size()?T.squad[sq].hair:0;
    if(rank==0&&reveal)drawPlayerSprite(px,top-1-(int)(std::fabs(std::sin(t*6+k))*3),T.home,skin,hair,0,(int)(t*4)+k,PS_CELEB,false,0,2);
    else drawPosedSprite(px,top-1,T.home,skin,hair,0,(int)(t*3)+k,PS_NORMAL,reveal?POSE_CLAP:POSE_NONE,2);}
   if(rank==0&&reveal)fxBigTrophy(x,top-30-(int)(std::fabs(std::sin(t*3))*2),2,1,t);
   drawTextCentered(fitText(T.name,rank==0?190:150,10),x,top-(rank==0?58:40),10,team==g_career.userTeam?C_GOOD:rank==0?C_HI:C_TXT);
  }
  if(reveal){fxConfetti(X0+34,Y0+6,X1-X0-68,FLOOR-Y0,t,CONF,5,120,3);fxFireworks(X0+40,Y0+60,X1-X0-80,80,t,5,3);}
  stageAudience(X0,Y1-30,X1,32,t,reveal);
 }else{
  // statuette sur son pupitre, enveloppe, puis lauréat sous le projecteur
  int style=g_awardPage==5?0:g_awardPage==3?1:4;
  DrawRectangle(VW/2+92,FLOOR-34,30,34,Color{40,30,26,255});DrawRectangle(VW/2+90,FLOOR-36,34,4,C_HI);
  fxBigTrophy(VW/2+107,FLOOR-36,style,1,t);
  if(!reveal){
   // l'enveloppe s'ouvre
   float u=std::min(1.f,g_awardTime/1.5f);
   int ex=VW/2-30,ey=FLOOR-70;
   DrawRectangle(ex,ey,60,38,Color{240,232,210,255});DrawRectangleLines(ex,ey,60,38,Color{180,150,90,255});
   DrawTriangle(Vector2{(float)ex,(float)ey},Vector2{(float)ex+30,(float)ey+(int)(20*(1-u))-4},Vector2{(float)ex+60,(float)ey},Color{214,200,170,255});
   DrawRectangle(ex+26,ey+14,8,8,Color{190,30,40,255});
   drawTextCentered("Le jury révèle le lauréat...",VW/2,FLOOR-96,10,C_TXT);
  }else if(a.valid[g_awardPage]){
   const auto& n=a.winners[g_awardPage];
   DrawTriangle(Vector2{(float)VW/2,(float)Y0+8},Vector2{(float)VW/2-40,(float)FLOOR},Vector2{(float)VW/2+40,(float)FLOOR},Color{255,245,200,46});
   if(n.team>=0&&n.team<(int)g_world.teams.size()){
    int skin=(int)(n.pid%NUM_SKINS),hair=(int)((n.pid/7)%NUM_HAIRS);playerLook(n.team,n.pid,skin,hair);
    if(g_awardPage==5){Kit suit;suit.shirt=0x22252E;suit.shirt2=0xE8E8E8;suit.shorts=0x1A1C22;suit.socks=0x111111;drawPlayerSprite(VW/2,FLOOR-2,suit,skin,hair,0,0,PS_CELEB,false,0,3);}
    else drawPlayerSprite(VW/2,FLOOR-2,g_world.teams[n.team].home,skin,hair,0,0,PS_CELEB,false,0,3);
    fxBigTrophy(VW/2,FLOOR-40-(int)(std::fabs(std::sin(t*3))*2),style,1,t);
    drawTextCentered(fitText(g_world.teams[n.team].name,240,10),VW/2,sy+38,10,C_DIM,false);
   }
   drawTextCentered(fitText(n.name,220,20),VW/2,sy+20,20,C_HI);
   fxConfetti(X0+34,Y0+6,X1-X0-68,FLOOR-Y0,t,CONF,5,70,g_awardPage+1);
  }else drawTextCentered("Aucun joueur éligible cette saison.",VW/2,FLOOR-80,10,C_TXT);
  stageAudience(X0,Y1-30,X1,32,t,reveal&&a.valid[g_awardPage]);
  if(reveal&&a.valid[g_awardPage]){
   const auto& n=a.winners[g_awardPage];
   if(g_awardPage!=5)drawTextCentered(fmt("%d buts  /  %d passes  /  %d matchs",n.goals,n.assists,n.apps),VW/2,308,10,C_DIM);
   else drawTextCentered("Récompense attribuée à l'entraîneur du champion.",VW/2,308,10,C_DIM);
  }
 }
 if(button(28,322,98,16,"Précédent",false,g_awardPage>0)|| (IN.left&&g_awardPage>0)){g_awardPage--;g_awardTime=0;}
 bool next=button(390,322,222,16,g_awardPage<last?"Récompense suivante":"Continuer");
 if(next||IN.ok||IN.right||IN.start){
  if(!reveal){g_awardTime=2;return;}
  if(g_awardPage<last){g_awardPage++;g_awardTime=0;if(g_awardPage==6)audioJingle(4);}
  else {if(!g_awardReplay){a.presented=1;autosave();g_screen=SC_SEASONEND;}else openHub();}
 }
 if(g_awardReplay && g_career.seasonAwards.size()>1 && button(154,322,212,16,"Édition précédente")){g_awardEdition=(g_awardEdition+(int)g_career.seasonAwards.size()-1)%(int)g_career.seasonAwards.size();g_awardPage=0;g_awardTime=0;}
 if(IN.back){if(!g_awardReplay){a.presented=1;autosave();g_screen=SC_SEASONEND;}else openHub();}
 drawFooter("Jury du jeu : performances dans ce championnat   OK : suite   Retour");
}
static void finishNextSeason(){
 int u=g_career.userTeam,tb=g_career.tierOfTeam(u);g_career.endSeason();int ta=g_career.tierOfTeam(u);
 g_hubNotice=(ta<tb?"PROMOTION ! Nouvelle saison en ":ta>tb?"Relégation... Nouvelle saison en ":"Nouvelle saison en ")+g_career.teamLevelName(u);
 g_needAdvance=true;openHub();autosave();showSeasonArticle();
}
static void openTeamPlans(){for(bool& v:g_plan)v=false;g_planCursor=4;g_screen=SC_TEAMPLANS;}
static void screenTeamPlans(){
 drawBackground("Préparer la nouvelle saison");
 int root=managedRoot(),res=0;for(auto& t:g_world.teams)if(t.parent==root&&!t.youth)res++;
 drawTextCentered(fmt("%s  -  Saison %d / %d",g_world.teams[root].name.c_str(),g_career.year+1,g_career.year+2),VW/2,38,10,C_HI);
 drawTextPx("Souhaitez-vous créer des équipes pour la saison qui commence ?",32,64,10,C_TXT);
 drawTextPx("Les nouvelles équipes seront inscrites dans leur district.",32,80,10,C_DIM);
 if(IN.up)g_planCursor=(g_planCursor+4)%5;if(IN.down)g_planCursor=(g_planCursor+1)%5;
 static const char* labels[]={"Une équipe réserve supplémentaire","Une équipe U15","Une équipe U17","Une équipe U19"};
 for(int k=0;k<4;k++){
  int y=112+k*36;int youth=k==1?3:k==2?2:1;
  bool available=k==0?(root==g_career.userTeam&&res<3):youthSection(root,youth)<0;
  if(g_planCursor==k)DrawRectangleLines(30,y-2,424,26,C_HI);
  if(button(32,y,420,22,std::string(available?(g_plan[k]?"[X] ":"[ ] "):"     ")+labels[k],g_plan[k],available)){g_plan[k]=!g_plan[k];g_planCursor=k;}
  if(available&&g_planCursor==k&&(IN.ok||IN.left||IN.right))g_plan[k]=!g_plan[k];
  drawTextPx(available?(g_plan[k]?"Création demandée":"Au choix"):(k==0?"Déjà 3 / équipe fanion":"Existe déjà"),466,y+6,10,available?C_DIM:C_GOOD);
 }
 int pending=0;for(int t:g_career.pendingNewClubs)if(g_world.teams[t].parent==root)pending++;
 drawTextPx(fmt("%d équipe(s) déjà créée(s) seront également intégrées.",pending),32,272,10,C_DIM);
 if(button(290,306,320,20,"Valider et commencer la saison",g_planCursor==4)||(IN.ok&&g_planCursor==4)||IN.start){
  std::string err;for(int k=0;k<4;k++)if(g_plan[k]){int t=k==0?g_career.createReserve(err):g_career.createYouth(root,k==1?3:k==2?2:1,err);if(t<0){toast(err);g_plan[k]=false;return;}g_plan[k]=false;}
  finishNextSeason();return;
 }
 if(button(32,306,200,20,"Retour au bilan")||IN.back)g_screen=SC_SEASONEND;
 drawFooter("Haut/Bas : choix   OK : cocher / valider   Les créations sont facultatives.");
}
static void screenPoleScout(){
 drawBackground("Pôles Espoirs - tournoi et recrutement");
 int ci=g_career.poleCup;
 if(ci<0||ci>=(int)g_career.season.comps.size()){drawTextCentered("Disponible en carrière de club masculin en France.",VW/2,130,10,C_TXT);if(IN.back||IN.ok)openHub();return;}
 auto ranking=g_career.poleRanking();bool done=g_career.season.comps[ci].done;
 int root=managedRoot(),dest=youthSection(root,3);
 drawTextPx(done?"Classement final : choisissez un joueur pour vos U15 de la saison prochaine.":"Classement provisoire : recrutement ouvert après la finale.",16,34,10,C_HI);
 drawTextPx("Joueurs simulés, 13-14 ans. 4 groupes puis quarts, demies et finale.",16,50,10,C_DIM);
 drawTextPx("2 x 20 minutes ; TAB directs. Un site unique de Pôle Espoirs.",16,64,10,C_DIM);
 drawTextPx("Rg / Joueur",20,84,10,C_DIM);drawTextPx("Pôle",240,84,10,C_DIM);drawTextPx("Âge  B  P  Note",442,84,10,C_DIM);
 int sel=listRun(g_poleLW,(int)ranking.size(),16,98,VW-32,12,15,[&](int i,int x,int y,bool selected){
  const auto& n=ranking[i];int ix=-1,t=n.team;if(t<0||t>=(int)g_world.teams.size())return;for(int k=0;k<(int)g_world.teams[t].squad.size();k++)if(g_world.teams[t].squad[k].id==n.pid){ix=k;break;}if(ix<0)return;const auto& p=g_world.teams[t].squad[ix];
  bool reserved=false;for(auto& r:g_career.poleRecruits)reserved|=r.pid==n.pid;
  drawTextPx(fitText(fmt("%d. %s",i+1,n.name),210,10),x+4,y+2,10,selected?BLACK:reserved?C_GOOD:C_TXT);
  drawTextPx(fitText(g_world.teams[t].town,185,10),x+224,y+2,10,selected?BLACK:C_DIM);
  drawTextPx(fmt("%d  %d  %d  %d%s",p.age,n.goals,n.assists,n.rating,reserved?" *":""),x+426,y+2,10,selected?BLACK:C_HI);
 });
 if(sel>=0 && sel<(int)ranking.size()){
  std::string err;if(dest<0)toast("Créez d'abord votre équipe U15.");else if(g_career.recruitPole(ranking[sel].pid,dest,err)){toast("Joueur réservé pour vos U15 de la saison prochaine");autosave();}else toast(err);
 }
 if(button(16,294,204,18,"Voir le tournoi"))openCompView(ci,-1,-1);
 if(dest<0 && button(232,294,222,18,"Créer mes U15 pour l'an prochain")){std::string err;if(g_career.createYouth(root,3,err)<0)toast(err);else {toast("Équipe U15 créée pour la saison prochaine");autosave();}}
 if(button(478,294,146,18,"Retour carrière")||IN.back)openHub();
 drawTextPx(fitText("Site : "+g_career.cupVenue[ci],VW-32,10),16,320,10,C_DIM);
 drawFooter("OK / clic : réserver   * : joueur réservé   Intégration à la nouvelle saison");
}
