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
static void screenSeasonAwards(){
 drawBackground("La soirée du championnat");
 if(g_awardEdition<0 || g_awardEdition>=(int)g_career.seasonAwards.size()){
  drawTextCentered("Les récompenses seront remises à la fin de votre championnat.",VW/2,130,10,C_HI);
  drawTextCentered("Joueurs, buteur, passeur, gardien, espoir et entraîneur.",VW/2,152,10,C_TXT);
  if(button(220,280,200,16,"Retour à la carrière")||IN.back||IN.ok)openHub();return;
 }
 auto& a=g_career.seasonAwards[g_awardEdition];g_awardTime+=GetFrameTime();
 static const char* labels[]={"Joueur de la saison","Meilleur buteur","Meilleur passeur","Gardien de la saison","Espoir de la saison (21 ans et moins)","Entraîneur de la saison"};
 drawTextCentered(fitText(a.competition,VW-70,10),VW/2,36,10,C_HI);
 drawTextCentered(fmt("Saison %d / %d  -  Distinction %d sur 6",a.year,a.year+1,g_awardPage+1),VW/2,52,10,C_DIM);
 DrawRectangle(40,78,VW-80,224,Color{8,13,27,255});
 for(int side=0;side<2;side++)for(int i=0;i<6;i++)DrawRectangle(side?VW-66+i*4:42+i*4,78,4,206,i%2?Color{130,22,44,255}:Color{94,16,35,255});
 drawTextCentered(labels[g_awardPage],VW/2,96,10,C_HI);
 DrawRectangle(VW/2-18,122,36,24,C_HI);DrawRectangle(VW/2-10,146,20,7,C_HI);DrawRectangle(VW/2-4,153,8,16,C_HI);DrawRectangle(VW/2-20,169,40,6,C_HI);
 bool reveal=g_awardReplay||g_awardTime>=1.5f;
 if(reveal && a.valid[g_awardPage]){
  const auto& n=a.winners[g_awardPage];
  if(n.team>=0&&n.team<(int)g_world.teams.size()){
   drawPlayerSprite(VW/2,195,g_world.teams[n.team].home,1,0,0,0,PS_NORMAL,false,0,3);
   drawTextCentered(fitText(g_world.teams[n.team].name,480,10),VW/2,248,10,C_TXT);
  }
  drawTextCentered(fitText(n.name,480,10),VW/2,230,10,C_HI);
  if(g_awardPage!=5)drawTextCentered(fmt("%d buts  /  %d passes  /  %d matchs",n.goals,n.assists,n.apps),VW/2,268,10,C_DIM);
  else drawTextCentered("Récompense attribuée à l'entraîneur du champion.",VW/2,268,10,C_DIM);
 }else drawTextCentered(reveal?"Aucun joueur éligible cette saison.":"Le jury révèle le lauréat...",VW/2,232,10,C_TXT);
 if(button(28,312,98,16,"Précédent",false,g_awardPage>0)|| (IN.left&&g_awardPage>0)){g_awardPage--;g_awardTime=0;}
 bool next=button(390,312,222,16,g_awardPage<5?"Récompense suivante":"Continuer");
 if(next||IN.ok||IN.right||IN.start){
  if(!reveal){g_awardTime=2;return;}
  if(g_awardPage<5){g_awardPage++;g_awardTime=0;}
  else {if(!g_awardReplay){a.presented=1;autosave();g_screen=SC_SEASONEND;}else openHub();}
 }
 if(g_awardReplay && g_career.seasonAwards.size()>1 && button(154,312,212,16,"Édition précédente")){g_awardEdition=(g_awardEdition+(int)g_career.seasonAwards.size()-1)%(int)g_career.seasonAwards.size();g_awardPage=0;g_awardTime=0;}
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
