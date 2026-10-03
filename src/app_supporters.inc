static int g_supClub=-1,g_supPage=0,g_supParty=-1;static Screen g_supBack=SC_HUB,g_supPartyBack=SC_HUB;static ListW g_supRows,g_supLines;
static const char* SUP_TABS[]={"Résumé","Abonnés","Affluence","Ambiance","Rivalités","Confiance","Événements","Historique"};
static void openSupporters(int club,Screen back){g_supClub=club;g_supBack=back;g_supPage=0;g_supRows=ListW();g_supLines=ListW();g_screen=SC_SUPPORTERS;}
static void supText(const std::string& text,int y=84,int rows=15){auto lines=wrapText(text,600);listRun(g_supLines,(int)lines.size(),12,y,616,rows,15,[&](int i,int x,int yy,bool sel){drawTextPx(lines[i],x+5,yy+2,10,sel?BLACK:C_TXT);});}
static void supBar(const char* label,int value,int x,int y){drawTextPx(label,x,y,10,C_DIM);DrawRectangle(x+145,y+2,105,7,Color{12,22,42,255});DrawRectangle(x+145,y+2,105*value/100,7,value<30?C_BAD:value>70?C_GOOD:C_HI);drawTextPx(fmt("%d",value),x+260,y,10,C_TXT);}
static void screenSupporters(){auto& K=g_career;if(g_supClub<0||g_supClub>=(int)g_world.teams.size()){g_screen=g_supBack;return;}drawBackground("Supporters : "+fitText(g_world.teams[g_supClub].name,430,20));if(!K.supportersEnabled()){supText("Mode Simple : affluence, déplacements, rivalité, ambiance et chants sont calculés pour le match.\nLa gestion des abonnements, la confiance et les événements persistants appartiennent aux carrières complètes.");if(IN.back)g_screen=g_supBack;drawFooter("Mode Simple : public et ambiance sans gestion");return;}if(!K.supporters.initialized){K.supportersInit();for(auto& p:K.supporters.profiles)K.supportersCampaign(p.first,false);}auto& P=K.supportersProfile(g_supClub);auto& T=g_world.teams[g_supClub];for(int i=0;i<8;i++)if(button(12+i*77,32,74,20,SUP_TABS[i],g_supPage==i)){g_supPage=i;g_supRows=ListW();g_supLines=ListW();}if(IN.tab){g_supPage=(g_supPage+1)%8;g_supRows=ListW();g_supLines=ListW();}int cap=std::max(1,T.sta.capacity());
 if(g_supPage==0){drawTextPx(fmt("%d supporters | %d abonnés | noyau actif %d",P.totalSupporters,P.seasonTicketHolders,P.activeCore),17,72,16,C_HI);drawTextPx(fmt("Occasionnels %d | historiques %d | internationaux %d",P.casual,P.historical,P.international),17,98,10,C_TXT);supBar("Fidélité",P.loyalty,18,128);supBar("Ferveur",P.fervor,18,155);supBar("Exigence",P.expectations,18,182);supBar("Patience",P.patience,18,209);supBar("Identité locale",P.localIdentity,18,236);drawTextPx("Dernier match :",334,129,10,C_DIM);drawTextPx(fmt("%d spectateurs",P.lastAttendance),334,148,16,C_TXT);drawTextPx(fmt("%d visiteurs",P.lastVisitors),334,172,10,C_TXT);drawTextPx(supporterAtmosphereName(P.atmosphereIndex,P.protestLevel),334,197,16,C_HI);drawTextPx(supporterChantName(P.lastChant),334,222,10,C_TXT);drawTextPx(fmt("Contestation %d / 5 | boycott : %d match(s)",P.protestLevel,P.boycottMatches),18,280,10,P.protestLevel?C_BAD:C_DIM);}
 else if(g_supPage==1){supText(fmt("Campagne saison %d/%d\nAbonnés : %d\nCapacité réservée : %d\nListe d'attente : %d\nRecettes brutes abonnements : %s\n",P.campaignYear,P.campaignYear+1,P.seasonTicketHolders,P.seasonTicketCapacity,P.waitingList,money(P.subscriptionEuros/1000).c_str())+"Les abonnements sont encaissés une seule fois à l'intersaison. Les abonnés présents ne paient pas une seconde fois leur place ordinaire.\nRésultats, prix, fidélité, ferveur et stade influencent les demandes.",82,13);if(g_supClub==K.userTeam&&(!K.sportingMode()||K.sporting.employed)&&button(13,297,610,21,"Prix et équipement du stade")){g_stTab=0;g_screen=SC_STADIUM;}}
 else if(g_supPage==2){std::string text=fmt("Matchs à domicile : %d\nMoyenne : %d spectateurs\nMeilleure affluence : %d | plus faible : %d\nRemplissage moyen : %d %%\n",P.seasonMatches,P.seasonMatches?P.seasonAttendance/P.seasonMatches:0,P.seasonBest,P.seasonWorst,P.seasonMatches?(int)((int64_t)P.seasonAttendance*100/P.seasonMatches/cap):0);text+="RECORD PERSISTANT : "+std::to_string(P.recordAttendance)+"\n";if(P.recordYear>0)text+=std::string(P.recordOpponentName)+" | "+P.recordCompetition+"\n"+P.recordDateText+"\n";else text+="Record antérieur aux archives détaillées, date non disponible.\n";text+="Dernier déplacement reçu : "+std::to_string(P.lastVisitors)+" visiteurs.\nDistances et demandes de public sont des estimations de simulation.";supText(text,82,16);}
 else if(g_supPage==3){supText("Ambiance : "+std::string(supporterAtmosphereName(P.atmosphereIndex,P.protestLevel))+fmt(" (%d / 100)\n",P.atmosphereIndex)+"État du public : "+supporterChantName(P.lastChant)+fmt("\nDernier tifo : %s\nContestation : %d / 5\nBoycott : %d rencontre(s) à domicile restantes\n",P.lastTifo<0?"aucun":"préparé",P.protestLevel,P.boycottMatches)+"Le score modifie chants et encouragements pendant le match. Les sifflets demandent une forte déception ; le boycott exceptionnel exige plusieurs facteurs graves.\nEffets sportifs plafonnés : stimulation légère à domicile, pression limitée sur les jeunes et les penalties.",82,15);}
 else if(g_supPage==4){listRun(g_supRows,(int)P.rivalries.size(),12,81,616,14,17,[&](int i,int x,int y,bool sel){auto& R=P.rivalries[i];static const char* types[]={"locale","historique","sportive","récente"};drawTextPx(fitText(g_world.teams[R.clubId].name+fmt(" | %s | %d / 100 | ~%d km",types[R.type],R.intensity,supporterDistance(g_supClub,R.clubId)),597,10),x+5,y+4,10,sel?BLACK:C_TXT);});drawTextPx("Rivalités de simulation ; proximité estimée, intensité évolutive.",16,324,10,C_DIM);}
 else if(g_supPage==5){supBar("Satisfaction",P.sportingSatisfaction,18,88);supBar("Entraîneur",P.managerTrust,18,119);supBar("Direction",P.boardTrust,18,150);supBar("Mercato / DS",P.transferTrust,18,181);supText(fmt("Série : %d victoire(s) / %d défaite(s)\nCrise prolongée : %d match(s)\n",P.winStreak,P.lossStreak,P.crisisMatches)+"Les jeunes locaux, les cadres conservés et les bons recrutements peuvent rassurer le public. Une vente sensible est jugée selon l'âge, le prix, la volonté du joueur et la succession.",226,7);}
 else if(g_supPage==6){int choice=listRun(g_supRows,(int)P.supporterEvents.size(),12,82,616,12,17,[&](int i,int x,int y,bool sel){auto& e=P.supporterEvents[P.supporterEvents.size()-1-i];drawTextPx(fitText(fmt("%d | %s | ",e.year,supporterEventName(e.type))+e.reason,597,10),x+5,y+4,10,sel?BLACK:C_TXT);});if(choice>=0){int at=(int)P.supporterEvents.size()-1-choice;auto& e=P.supporterEvents[at];if(g_supClub==K.userTeam&&(e.type==SU_TITLE_PARTY||e.type==SU_PROMOTION_PARTY)){g_supParty=at;g_supPartyBack=SC_SUPPORTERS;g_screen=SC_SUPPORTPARTY;}}drawTextPx("Cliquez une fête du titre ou de montée pour revoir la cérémonie.",16,309,10,C_DIM);}
 else if(g_supPage==7){listRun(g_supRows,(int)P.attendanceHistory.size(),12,82,616,14,17,[&](int i,int x,int y,bool sel){auto& a=P.attendanceHistory[P.attendanceHistory.size()-1-i];drawTextPx(fitText(fmt("%d/%d | %d matches | moy. %d | max. %d | min. %d | %d abonnés",a.year,a.year+1,a.matches,a.matches?a.total/a.matches:0,a.best,a.worst,a.subscribers),597,10),x+5,y+4,10,sel?BLACK:C_TXT);});}
 if(IN.back)g_screen=g_supBack;drawFooter("Tab : rubrique suivante | Retour : club");}
static void screenSupporterParty(float dt){auto& K=g_career;auto it=K.supporters.profiles.find(K.userTeam);if(it==K.supporters.profiles.end()||g_supParty<0||g_supParty>=(int)it->second.supporterEvents.size()){g_screen=g_supPartyBack;return;}auto& e=it->second.supporterEvents[g_supParty];auto& T=g_world.teams[K.userTeam];
 drawBackground(supporterEventName(e.type));
 float t=(float)GetTime();bool title=e.type==SU_TITLE_PARTY;
 Color c1=hexc(T.home.shirt),c2=hexc(T.home.shirt2!=T.home.shirt?T.home.shirt2:T.home.shorts);
 const int X0=12,X1=VW-12,Y0=30,Y1=282;
 // ciel de nuit, feux d'artifice au-dessus de la ville
 DrawRectangleGradientV(X0,Y0,X1-X0,Y1-Y0,Color{8,10,34,255},Color{40,26,60,255});
 for(int i=0;i<40;i++)DrawRectangle(X0+(i*137)%(X1-X0),Y0+(i*71)%80,1,1,((int)(t*2)+i)%4?Color{200,200,240,255}:Color{90,90,140,255});
 fxFireworks(X0,Y0,X1-X0,120,t,(unsigned)e.year,5);
 // immeubles, hôtel de ville et son balcon pavoisé
 for(int k=0;k<12;k++){int w=34+(k*17)%22,h=52+(k*29)%46,x=X0+k*52-10;DrawRectangle(x,Y1-70-h,w,h+40,Color{22,20,40,255});for(int wy=Y1-66-h;wy<Y1-40;wy+=9)for(int wx=x+4;wx<x+w-4;wx+=8)if((wx*7+wy*3+k)%5)DrawRectangle(wx,wy,3,4,((wx+wy+(int)(t*0.5f))%11)?Color{250,210,110,255}:Color{80,70,60,255});}
 {int hx=VW/2-70,hy=Y1-170;DrawRectangle(hx,hy,140,130,Color{70,64,80,255});DrawTriangle(Vector2{(float)hx-6,(float)hy},Vector2{(float)hx+146,(float)hy},Vector2{(float)hx+70,(float)hy-26},Color{60,54,70,255});
  DrawRectangle(hx+62,hy-44,16,22,Color{60,54,70,255});DrawCircle(hx+70,hy-36,5,Color{240,230,200,255});DrawLine(hx+70,hy-36,hx+70,hy-39,BLACK);DrawLine(hx+70,hy-36,hx+72,hy-36,BLACK);
  for(int k=0;k<5;k++)DrawRectangle(hx+10+k*26,hy+12,14,22,Color{250,214,120,255});
  for(int k=0;k<14;k++){DrawRectangle(hx+k*10,hy+40,10,3,k%2?c1:c2);DrawTriangle(Vector2{(float)hx+k*10,(float)hy+43},Vector2{(float)hx+k*10+5,(float)hy+48},Vector2{(float)hx+k*10+10,(float)hy+43},k%2?c2:c1);}}
 // bus à impériale aux couleurs du club, joueurs sur le pont supérieur
 float bx=VW/2-100+std::sin(t*0.25f)*14.f;int by=Y1-56;
 DrawRectangle((int)bx+4,by+4,200,40,Color{0,0,0,90});
 DrawRectangle((int)bx,by,200,40,c1);DrawRectangle((int)bx,by+26,200,6,c2);DrawRectangle((int)bx,by+2,200,2,Color{255,255,255,60});
 for(int k=0;k<8;k++)DrawRectangle((int)bx+10+k*23,by+8,16,12,Color{40,60,90,255});
 DrawRectangle((int)bx+178,by+8,18,24,Color{150,190,220,255});
 for(int w=0;w<2;w++){int wx=(int)bx+(w?160:34);DrawCircle(wx,by+40,7,Color{20,20,20,255});DrawCircle(wx,by+40,3,Color{150,150,160,255});}
 drawTextCentered(fitText(T.shortName,150,10),(int)bx+90,by+30,10,BLACK,false);
 DrawRectangle((int)bx,by-4,200,4,Color{230,230,230,255});
 for(int k=0;k<7;k++){
  int sq=k<(int)T.squad.size()?k*2%std::max(1,(int)T.squad.size()):0;int skin=sq<(int)T.squad.size()?T.squad[sq].skin:0,hair=sq<(int)T.squad.size()?T.squad[sq].hair:0;
  int px=(int)bx+22+k*26,py=by-4-(int)(std::fabs(std::sin(t*5+k*0.9f))*3);
  drawPlayerSprite(px,py,T.home,skin,hair,0,(int)(t*4)+k,PS_CELEB,false,0,2);
  if(k==3){if(title)fxBigTrophy(px,py-26-(int)(std::fabs(std::sin(t*3))*3),2,1,t);else{DrawRectangle(px-30,py-34,60,12,c2);drawTextCentered("MONTÉE !",px,py-33,10,c1,false);}}
 }
 // marée humaine aux couleurs du club, fumigènes et confettis
 fxCrowd(X0,Y1-36,X1-X0,30,c1,c2,t,0.9f,(unsigned)K.userTeam,6);
 for(int k=0;k<6;k++)fxFlare(X0+40+k*110,Y1-24+(k%2)*6,t,(unsigned)k*5);
 Color cc[4]={c1,c2,C_HI,WHITE};fxConfetti(X0,Y0,X1-X0,Y1-Y0,t,cc,4,140,(unsigned)e.year);
 drawTextCentered(T.name,VW/2,Y0+6,20,C_HI);
 drawTextCentered(fmt("Saison %d/%d",e.year,e.year+1),VW/2,Y0+30,10,C_TXT);
 auto lines=wrapText(e.reason,580);for(int i=0;i<(int)lines.size()&&i<3;i++)drawTextCentered(lines[i],VW/2,290+i*12,10,C_TXT);
 audioCrowd(true,.25f);audioSupporters(CH_CELEBRATE,.8f);
 if(button(155,327,330,19,"Saluer les supporters et continuer")||IN.ok||IN.back){e.presented=1;audioCrowd(false,0);audioSupporters(CH_TENSE,0);g_screen=g_supPartyBack;}
 drawFooter("Une mémoire vivante du club");
 (void)dt;
}

static bool showSupporterParty(Screen back){int event=g_career.supportersPendingEvent();if(event<0)return false;g_supParty=event;g_supPartyBack=back;g_screen=SC_SUPPORTPARTY;return true;}
