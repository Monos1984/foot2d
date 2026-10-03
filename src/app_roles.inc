// Detailed tactics are available only in long careers.
static void ensureTacticalPlan(Team& T){if(T.tactical.customized)return;for(int s=0;s<11;s++)T.tactical.slot[s]=defaultSlotTactic(slotPosition(T.formation,s),T.seed%2,s);T.tactical.model=T.seed%2;T.tactical.customized=1;}
static void screenRolePlan(Team& T){
 ensureTacticalPlan(T);drawBackground("Carrière : postes, rôles et consignes");
 if(button(12,31,180,14,"< Consignes collectives")){g_rolePlayerTab=0;return;}
 if(button(200,31,210,14,"Modèle : "+std::string(T.tactical.model==0?"Possession":"Direct compact"))){T.tactical.model=1-T.tactical.model;for(int s=0;s<11;s++)T.tactical.slot[s]=defaultSlotTactic(slotPosition(T.formation,s),T.tactical.model,s);T.passStyle=T.tactical.model?2:0;T.pressing=T.tactical.model?0:2;T.defLine=T.tactical.model?0:2;T.tempo=T.tactical.model?2:0;}
 auto lu=g_world.pickLineup((int)(&T-&g_world.teams[0]),T.formation,RULESET_CAREER);
 if(IN.up)g_roleSlot=(g_roleSlot+10)%11;if(IN.down)g_roleSlot=(g_roleSlot+1)%11;
 for(int s=0;s<11;s++){int y=53+s*18;bool selected=s==g_roleSlot;if(IN.click&&IN.mouse.x<300&&IN.mouse.y>=y&&IN.mouse.y<y+17)g_roleSlot=s;
  DrawRectangle(12,y,285,17,selected?C_SEL:C_ITEM);int dp=teamSlotPosition(T,T.formation,s);const Player& p=T.squad[lu[s]];
  drawTextPx(fmt("%2d %-5s ",s,detailedPositionName(dp))+fitText(p.name,145,10),18,y+3,10,selected?BLACK:C_TXT);
  drawTextPx(fmt("%d%%",positionFamiliarity(p,dp)),253,y+3,10,selected?BLACK:C_HI);}
 int dp=teamSlotPosition(T,T.formation,g_roleSlot);auto& t=T.tactical.slot[g_roleSlot];const Player& p=T.squad[lu[g_roleSlot]];
 if(button(315,51,300,17,"Poste du slot : "+std::string(detailedPositionName(dp)))){int candidate=dp;do{candidate=(candidate+1)%DP_COUNT;}while(positionFamily(candidate)!=positionFamily(slotPosition(T.formation,g_roleSlot)));t.position=candidate;t.role=0;}
 if(button(315,72,300,17,fitText(std::string("Rôle : ")+tacticalRoleName(dp,t.role),286,10)))t.role=(t.role+1)%tacticalRoleCount(dp);
 static const char* duty[]={"Défense","Soutien","Attaque"};if(button(315,93,300,17,std::string("Tâche : ")+duty[t.duty%3]))t.duty=(t.duty+1)%3;
 drawTextPx(fmt("Aptitude %d / 100 (caractéristiques)",roleAptitude(p,dp,t.role)),315,115,10,C_GOOD);
 drawTextPx("Vitesse / passe / placement / endurance",315,129,10,C_DIM);
 if(button(315,150,300,17,std::string("Consigne : ")+tacticalInstructionName(g_roleInstr)))g_roleInstr=(g_roleInstr+1)%TI_COUNT;
 if(button(315,172,300,17,t.instructions&(1u<<g_roleInstr)?"Activée : désactiver":"Désactivée : activer")){
  t.instructions^=1u<<g_roleInstr;
  static const int opposing[][2]={{TI_STAY,TI_ADVANCE},{TI_PRESS,TI_NO_PRESS},{TI_INSIDE,TI_WIDE},{TI_EARLY_CROSS,TI_DEEP_CROSS},{TI_FREE,TI_SIMPLE},{TI_DRIBBLE,TI_NO_DRIBBLE}};
  if(t.instructions&(1u<<g_roleInstr))for(auto& pair:opposing)if(g_roleInstr==pair[0]||g_roleInstr==pair[1])t.instructions&=~(1u<<(g_roleInstr==pair[0]?pair[1]:pair[0]));}
 if(IN.left)g_roleInstr=(g_roleInstr+TI_COUNT-1)%TI_COUNT;if(IN.right)g_roleInstr=(g_roleInstr+1)%TI_COUNT;
 if(g_roleInstr==TI_MARK){std::vector<const Player*> opponents;for(auto& C:g_career.season.comps)for(auto& m:C.matches)if(!m.played&&(m.home==g_career.userTeam||m.away==g_career.userTeam)){int other=m.home==g_career.userTeam?m.away:m.home;g_world.ensureSquad(other);for(auto& q:g_world.teams[other].squad)opponents.push_back(&q);break;}
  std::string label="Cible : à choisir";for(auto* q:opponents)if(q->id==t.markPid)label="Cible : "+q->name;
  if(button(315,195,300,16,fitText(label,280,10))&&!opponents.empty()){size_t k=0;while(k<opponents.size()&&opponents[k]->id!=t.markPid)k++;t.markPid=opponents[(k+1)%opponents.size()]->id;}}
 auto e=roleEffects(dp,t);drawTextPx(fmt("Pressing x%.2f  Tir x%.2f",e.press,e.shot),315,219,10,C_DIM);
 drawTextPx(fmt("Largeur x%.2f  Risque x%.2f",e.width,e.risk),315,233,10,C_DIM);
 {static const char* BH[]={"","Faux neuf : décroche entre les lignes, libère l'axe pour les ailiers, attaque la profondeur par moments.","Attaquant intérieur : appels vers l'axe et dans la surface, laisse le couloir au latéral.","Latéral offensif : dépasse côté ballon, appels extérieurs, repli rapide à la perte.","Latéral inversé : rentre dans le milieu à la relance, ressort sur son côté sans le ballon.","Mezzala : occupe le demi-espace, courses en diagonale vers la surface.","Stoppeur : sort au contact de l'attaquant qui décroche devant lui.","Couverture : reste juste derrière la ligne, côté ballon.","Libéro : dernier homme sans le ballon, monte d'un cran pour relancer."};
  if(e.behave>0&&e.behave<9){int ly=251;for(auto& l:wrapText(BH[e.behave],300)){if(ly>285)break;drawTextPx(l,315,ly,10,C_GOOD);ly+=12;}}}
 drawFooter("Haut/Bas : emplacement   Gauche/Droite : consigne   Retour");if(IN.back){g_rolePlayerTab=0;g_screen=SC_SQUAD;}
}
static void screenPlayerPositions(Player& p){
 initPositions(p);drawBackground("Carrière : postes et apprentissage");
 if(playerNavigation(1))return;drawTextPx(fitText(p.name+" - "+std::string(posName(p.pos)),610,16),12,63,16,C_HI);
 static const char* foot[]={"Droit","Gauche","Deux pieds"};drawTextPx(std::string("Poste naturel : ")+detailedPositionName(p.positions.primary)+"   Pied : "+foot[p.positions.foot%3],12,86,10,C_TXT);
 for(int i=0;i<DP_COUNT;i++){int x=12+(i/8)*220,y=107+(i%8)*21;int v=positionFamiliarity(p,i);drawTextPx(std::string(detailedPositionName(i))+fmt(" %3d%% ",v)+familiarityName(v),x,y,10,v>=75?C_GOOD:v>=30?C_TXT:C_DIM);}
 bool own=g_careerActive&&!g_career.sportingMode()&&(g_plTeam==g_career.userTeam||g_career.coach&&g_plTeam==g_career.coachTeam());
 if(own){if(button(450,107,174,20,p.positions.target<DP_COUNT?"Apprendre : "+std::string(detailedPositionName(p.positions.target)):"Apprentissage : aucun"))p.positions.target=p.positions.target==255?0:p.positions.target+1<DP_COUNT?p.positions.target+1:255;
  if(button(450,133,174,20,p.positions.developmentRole==255?"Rôle : aucun":fitText(tacticalRoleName(p.positions.primary,p.positions.developmentRole),160,10)))p.positions.developmentRole=p.positions.developmentRole==255?0:p.positions.developmentRole+1<tacticalRoleCount(p.positions.primary)?p.positions.developmentRole+1:255;}
 int dp=p.positions.primary;for(int k=0;k<tacticalRoleCount(dp);k++)drawTextPx(fitText(std::string(tacticalRoleName(dp,k))+fmt(" : %d",roleAptitude(p,dp,k)),178,10),450,164+k*15,10,C_TXT);
 drawTextPx("Les matchs et les séances améliorent les postes travaillés.",12,290,10,C_DIM);
 drawTextPx("Un poste proche progresse plus vite. Aptitudes calculées, sans note cachée.",12,304,10,C_DIM);
 drawFooter("Tab : personnalité / fiche   Retour");if(IN.back){g_rolePlayerTab=0;g_screen=g_plBack;}
}
