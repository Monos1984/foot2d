static ListW g_psychLines,g_psychHistory;
static void screenPlayerPersonality(Player& player,Team& T){
 auto& K=g_career;int club=g_plTeam;bool own=club==K.userTeam||T.parent==K.userTeam;drawBackground("Personnalité du joueur");
 if(playerNavigation(2))return;
 drawTextPx(fitText(player.name,600,16),16,63,16,C_HI);
 if(!own){auto lines=wrapText(personalityScoutReport(K,player,club),595);listRun(g_psychLines,(int)lines.size(),12,89,616,15,15,[&](int i,int x,int y,bool sel){drawTextPx(lines[i],x+5,y+3,10,sel?BLACK:C_TXT);});drawFooter("Observation progressive | Tab : fiche générale | Retour");if(IN.back){g_rolePlayerTab=0;g_screen=g_plBack;}return;}
 auto& P=K.personalityPlayer(player,club);auto& R=K.personalityRelation(P,club);if(g_psychArchive&&button(12,318,300,20,"Retour aux traits et relations"))g_psychArchive=false;if(g_psychArchive){int chosen=listRun(g_psychHistory,(int)P.events.size(),12,99,616,12,17,[&](int i,int x,int y,bool sel){auto& e=P.events[P.events.size()-1-i];drawTextPx(fitText(fmt("%d | ",e.year)+e.text,599,10),x+5,y+3,10,sel?BLACK:C_TXT);});(void)chosen;drawFooter("Historique : changements importants | Retour");if(IN.back){g_psychArchive=false;return;}return;}drawTextPx(personalitySummary(P.traits,player.age),16,88,10,C_GOOD);
 for(int i=0;i<10;i++){int x=i<5?16:332,y=110+(i%5)*25;drawTextPx(personalityTraitName(i),x,y,10,C_DIM);drawTextPx(personalityGrade(personalityTrait(P.traits,i)),x,y+11,10,C_TXT);}
 std::string relation="Attachement : "+std::string(personalityGrade(R.attachment))+" | Popularité : "+personalityGrade(R.fanPopularity)+"\nEntraîneur : "+personalityGrade(R.managerRelationship)+" | Influence : "+personalityGrade(R.squadInfluence)+"\nAdaptation : "+(R.adaptationProgress==100?std::string("complète"):std::string(personalityGrade(R.adaptationProgress))+" (en cours)")+" | "+(R.temporary?"relation de prêt":"relation au club");
 auto lines=wrapText(relation,599);for(int i=0;i<(int)lines.size()&&i<4;i++)drawTextPx(lines[i],16,241+i*13,10,C_TXT);
 if(R.transferRequested)drawTextPx("Souhaite un projet plus ambitieux.",16,294,10,C_BAD);
 if(player.age<=23&&club==K.userTeam){std::vector<int> mentors{0};for(auto& p:T.squad)if(p.id!=player.id&&p.age>=25){auto traits=personalityTraits(K,p);if(traits.professionalism>=60&&traits.leadership>=60)mentors.push_back(p.id);}std::string label="Mentor : aucun";for(auto& p:T.squad)if(p.id==R.mentorPid)label="Mentor : "+p.name;if(button(12,318,300,20,fitText(label+" >",288,10))){auto at=std::find(mentors.begin(),mentors.end(),R.mentorPid);int idx=at==mentors.end()?0:(int)(at-mentors.begin());std::string msg;K.personalityMentor(club,player.id,mentors[(idx+1)%mentors.size()],msg);toast(msg);}}
 if(button(326,318,302,20,"Historique de personnalité"))g_psychArchive=true;
 drawFooter("Tab : fiche générale | Personnalité distincte de la note | Retour");if(IN.back){g_rolePlayerTab=0;g_screen=g_plBack;}
}
