// Persistent club museum: no scan of past competitions when opening a page.
static int g_muClub=-1,g_muPage=0,g_muYear=0,g_muCategory=0,g_muComp=0,g_muDetail=-1;
static bool g_muReverse=true;
static Screen g_muBack=SC_CLUBMENU;
static ListW g_muList,g_muText;
static const char* MU_PAGES[]={"Accueil","Saisons","Trophées","Finales","Montées / descentes","Records","Matchs de légende","Légendes / Hall of Fame","Entraîneurs","Stades","Chronologie","Maillots"};
struct MuseumRowUI {int year=0; std::string competition,title,detail;};
static void openMuseum(int t,Screen back){g_muClub=t;g_muBack=back;g_muPage=g_muYear=g_muCategory=g_muComp=0;g_muDetail=-1;g_muList=ListW();g_muText=ListW();g_career.museumState(t);g_screen=SC_MUSEUM;}
static std::string museumScoreUI(const MuseumMatch& m){return fmt("%s %d - %d %s",m.homeName,m.hg,m.ag,m.awayName)+(m.ph>=0?fmt(" (TAB %d-%d)",m.ph,m.pa):m.extraTime?" (après prolongation)":"");}
static std::string museumMatchUI(const MuseumMatch& m){
 std::string text=fmt("%d/%d | %s\n",m.year,m.year+1,m.competition)+museumScoreUI(m)+"\nStade : "+m.venue+"\n";
 text+=m.attendance>=0?fmt("Affluence : %d\n",m.attendance):"Affluence non renseignée\n";
 text+=m.winner>=0?(m.winner==g_muClub?"Victoire du club\n":"Victoire adverse\n"):"Match nul\n";
 text+=m.exactLineup?"Composition enregistrée lors du match joué\n":"Composition indicative du match simulé\n";
 for(int side=0;side<2;side++){text+=std::string(side?m.awayName:m.homeName)+" | Capitaine : "+(m.captain[side][0]?m.captain[side]:"non renseigné")+"\n";for(int s=0;s<11;s++)if(m.lineupNames[side][s][0])text+=std::string(m.lineupNames[side][s])+"; ";text+="\n";}return text;
}
static void screenMuseum(){
 if(g_muClub<0||g_muClub>=(int)g_world.teams.size()){g_screen=g_muBack;return;}
 auto it=g_career.clubHistories.find(g_muClub);if(it==g_career.clubHistories.end()){g_screen=g_muBack;return;}
 auto& H=it->second;const Team& T=g_world.teams[g_muClub];drawBackground("Musée du club");
 if(button(12,32,450,18,"Club : "+fitText(T.name,385,10)+" >")){std::vector<int> visited;for(auto& history:g_career.clubHistories)if(history.first==g_career.userTeam||!history.second.managers.empty())visited.push_back(history.first);auto current=std::find(visited.begin(),visited.end(),g_muClub);if(!visited.empty())openMuseum(visited[(current==visited.end()?0:current-visited.begin()+1)%visited.size()],g_muBack);return;}
 int level=std::max(0,std::min(2,(int)T.sta.museum));drawTextPx(level==0?"Archives du club":level==1?"Salle des trophées":"Musée du club",470,34,10,C_DIM);
 for(int page=0;page<12;page++){if(button(10,57+page*20,156,18,MU_PAGES[page],page==g_muPage)){g_muPage=page;g_muDetail=-1;g_muList=ListW();g_muText=ListW();}}
 if(IN.left||IN.right){g_muPage=(g_muPage+(IN.right?1:11))%12;g_muDetail=-1;g_muList=ListW();}
 if(button(10,305,156,18,"Archives antérieures")){openFiche(g_muClub,SC_MUSEUM);g_fichePage=3;return;}
 if(button(10,327,156,18,"Retour")||IN.back){if(g_muDetail>=0){g_muDetail=-1;return;}g_screen=g_muBack;return;}
 std::vector<MuseumRowUI> rows;
 if(g_muPage==1||g_muPage==4)for(auto& s:H.seasons){if(g_muPage==4&&!s.promoted&&!s.relegated)continue;std::string move=s.promoted?"MONTÉE":s.relegated?"DESCENTE":s.finished?"Maintien":"En cours";
 std::string detail=fmt("Saison %d/%d\n%s\n%s | Place : %d\n%d matchs | %d V / %d N / %d D\n%d points | %d buts pour / %d contre\n",s.year,s.year+1,s.competition,s.division,s.position,s.played,s.wins,s.draws,s.losses,s.points,s.gf,s.ga)+move+" -> "+s.newDivision+"\nEntraîneur : "+s.manager+"\n";
 detail+=s.budgetKnown?"Budget début : "+money(s.budgetStart)+" | fin : "+money(s.budgetEnd)+"\n":"Budget non renseigné\n";
 detail+=fmt("Affluence moyenne : %d | maximum : %d\nButeur : %s (%d) | Passeur : %s (%d)\nJoueur marquant : %s\n%d trophées | %d récompenses individuelles\n",s.averageAttendance,s.maxAttendance,s.scorer,s.scorerGoals,s.assister,s.assisterCount,s.bestPlayer,s.trophies,s.awards);if(s.historic)detail+="Une saison historique du club\n";
 rows.push_back({s.year,s.competition,fmt("%d/%d | %s | %d pts | %s",s.year,s.year+1,s.division,s.points,move.c_str()),detail});}
 if(g_muPage==2)for(auto& t:H.trophies)rows.push_back({t.year,t.name,fmt("%d | %s",t.year,t.name),std::string(t.name)+fmt("\nSaison %d/%d\n",t.year,t.year+1)+(t.finalMatch.final?museumMatchUI(t.finalMatch):"Titre de championnat ou compétition sans finale unique\n")});
 if(g_muPage==3||g_muPage==6)for(auto& m:g_muPage==3?H.finals:H.legendaryMatches)rows.push_back({m.year,m.competition,fmt("%d | ",m.year)+museumScoreUI(m),museumMatchUI(m)});
 if(g_muPage==5)for(auto& r:H.records)rows.push_back({r.year,r.competition,std::string(museumRecordName(r.kind))+fmt(" : %lld",(long long)r.value),std::string(museumRecordName(r.kind))+fmt("\nValeur : %lld\nSaison %d/%d\n",(long long)r.value,r.year,r.year+1)+r.holder+"\n"+r.competition});
 if(g_muPage==7){std::vector<const MuseumPlayer*> players;for(auto& p:H.players)if(p.legendScore()>=250||p.hall)players.push_back(&p);std::sort(players.begin(),players.end(),[](auto a,auto b){return a->legendScore()>b->legendScore();});for(auto p:players)rows.push_back({0,"",std::string(p->hall?"[HOF] ":"")+p->name+fmt(" | %d pts",p->legendScore()),std::string(p->name)+fmt("\n%d matchs | %d buts | %d passes\n%d saisons | %d trophées | %d récompenses\nButs : championnat %d / coupes %d / Europe %d\nScore de légende : %d\n",p->apps,p->goals,p->assists,p->seasons,p->trophies,p->awards,p->leagueGoals,p->cupGoals,p->euroGoals,p->legendScore())+"Personnalité / fidélité : +"+std::to_string(personalityLegendBonus(g_career,p->pid,H.clubId))+" points historiques.\n"+"Le score récompense la contribution au club.\nCapitanat et matchs sans but encaissé : estimations en simulation.\n"});}
 if(g_muPage==8)for(auto& m:H.managers)rows.push_back({m.fromYear,"",fmt("%s | %d - %s",m.name,m.fromYear,m.toYear?std::to_string(m.toYear).c_str():"actuel"),fmt("%s\nArrivée : %d | Départ : %s\n%d matchs | %d victoires / %d nuls / %d défaites\n%d trophées | %d promotions\n%s",m.name,m.fromYear,m.toYear?std::to_string(m.toYear).c_str():"en poste",m.apps,m.wins,m.draws,m.losses,m.trophies,m.promotions,m.departure)});
 if(g_muPage==9)for(auto& s:H.stadiums)rows.push_back({s.fromYear,"",fmt("%d | %s | %d places",s.fromYear,s.name,s.capacityEnd),fmt("%s\n%s\nDepuis %d | %s\nCapacité initiale : %d | actuelle / finale : %d\nAffluence record : %d\n",s.name,s.town,s.fromYear,s.toYear?std::to_string(s.toYear).c_str():"stade actuel",s.capacityStart,s.capacityEnd,s.recordAttendance)});
 if(g_muPage==10){std::vector<const MuseumEvent*> events;for(auto& e:H.timeline)if(!g_muCategory||e.category==g_muCategory)events.push_back(&e);std::stable_sort(events.begin(),events.end(),[](auto a,auto b){return a->year!=b->year?a->year<b->year:a->time<b->time;});for(auto e:events)rows.push_back({e->year,"",fmt("%d | %s | %s",e->year,museumCategoryName(e->category),e->text),e->text});}
 if(g_muPage==11)for(auto& s:H.shirts)rows.push_back({s.year,"",fmt("%d/%d | %s",s.year,s.year+1,s.sponsor),fmt("Maillots %d/%d\nSponsor : %s\n",s.year,s.year+1,s.sponsor)});
 std::vector<int> years{0};for(auto& s:H.seasons)if(std::find(years.begin(),years.end(),s.year)==years.end())years.push_back(s.year);
 std::vector<std::string> comps{"Toutes compétitions"};for(auto& r:rows)if(!r.competition.empty()&&std::find(comps.begin(),comps.end(),r.competition)==comps.end())comps.push_back(r.competition);if(g_muPage==10){for(auto& season:H.seasons)if(season.competition[0]&&std::find(comps.begin(),comps.end(),season.competition)==comps.end())comps.push_back(season.competition);for(auto& trophy:H.trophies)if(std::find(comps.begin(),comps.end(),trophy.name)==comps.end())comps.push_back(trophy.name);}if(g_muComp>=(int)comps.size())g_muComp=0;
 if(button(178,57,127,18,g_muYear?fmt("Saison %d",g_muYear):"Toutes saisons")){auto y=std::find(years.begin(),years.end(),g_muYear);g_muYear=years[(y==years.end()?0:(y-years.begin()+1))%years.size()];g_muList=ListW();g_muDetail=-1;}
 if(button(310,57,202,18,fitText(comps[g_muComp],190,10))){g_muComp=(g_muComp+1)%comps.size();g_muList=ListW();g_muDetail=-1;}
 if(button(517,57,111,18,g_muReverse?"Récent d'abord":"Ancien d'abord")){g_muReverse=!g_muReverse;g_muList=ListW();g_muDetail=-1;}
 if(g_muPage==10&&button(178,80,202,18,museumCategoryName(g_muCategory))){g_muCategory=(g_muCategory+1)%8;g_muList=ListW();g_muDetail=-1;}
 std::vector<MuseumRowUI> filtered;for(auto& r:rows)if((!g_muYear||r.year==g_muYear)&&(g_muComp==0||r.competition==comps[g_muComp]||g_muPage==10&&r.detail.find(comps[g_muComp])!=std::string::npos))filtered.push_back(r);if(g_muReverse&&g_muPage!=7)std::reverse(filtered.begin(),filtered.end());
 if(g_muPage==0){
 // salle des trophées : boiseries, vitrine éclairée, une étagère par rangée de quatre trophées (regroupés par compétition)
 float t=(float)GetTime();
 DrawRectangleGradientV(178,84,450,218,Color{70,44,30,255},Color{40,26,20,255});
 for(int x=178;x<628;x+=30)DrawRectangle(x,84,1,218,Color{30,18,12,255});
 DrawRectangle(178,84,450,3,Color{120,84,50,255});
 const char* hall=level==0?"ARCHIVES DU CLUB":level==1?"SALLE DES TROPHÉES":"MUSÉE DU CLUB";
 DrawRectangle(318,88,170,15,Color{30,22,16,255});DrawRectangleLines(318,88,170,15,C_HI);drawTextCentered(hall,403,91,10,C_HI,false);
 struct Group{std::string name;int count=0,last=0,style=0;};std::vector<Group> groups;
 for(auto& tr:H.trophies){std::string nm=tr.name;auto g=std::find_if(groups.begin(),groups.end(),[&](const Group& x){return x.name==nm;});if(g==groups.end()){Group ng;ng.name=nm;ng.style=tr.kind==3?1:tr.finalMatch.final?0:2;groups.push_back(ng);g=groups.end()-1;}g->count++;g->last=std::max(g->last,tr.year);}
 std::stable_sort(groups.begin(),groups.end(),[](const Group& x,const Group& y){return x.count!=y.count?x.count>y.count:x.last>y.last;});
 // vitrine
 int cx0=190,cy0=108,cw=426,ch=150;
 DrawRectangle(cx0-4,cy0-4,cw+8,ch+8,Color{26,16,10,255});
 DrawRectangleGradientV(cx0,cy0,cw,ch,Color{24,30,46,255},Color{14,18,30,255});
 for(int k=0;k<4;k++){int sx=cx0+53+k*106;DrawTriangle(Vector2{(float)sx,(float)cy0},Vector2{(float)sx-28,(float)cy0+ch},Vector2{(float)sx+28,(float)cy0+ch},Color{255,240,200,18});DrawRectangle(sx-5,cy0,10,3,Color{230,230,240,255});}
 for(int shelf=0;shelf<2;shelf++){
  int sy=cy0+70+shelf*75;
  DrawRectangle(cx0,sy,cw,3,Color{180,200,220,160});DrawRectangle(cx0,sy+3,cw,2,Color{0,0,0,80});
  for(int k=0;k<4;k++){
   int gi=shelf*4+k,x=cx0+53+k*106;
   if(gi<(int)groups.size()){
    const Group& G=groups[gi];
    DrawEllipse(x,sy,18,3,Color{0,0,0,90});
    fxBigTrophy(x,sy,G.style,2,t+gi*0.4f);
    if(G.count>1){DrawCircle(x+28,sy-50,8,Color{190,30,40,255});drawTextCentered(fmt("x%d",G.count),x+28,sy-55,10,WHITE,false);}
    DrawRectangle(x-46,sy+6,92,11,Color{200,170,90,255});DrawRectangleLines(x-46,sy+6,92,11,Color{120,90,40,255});
    drawTextCentered(fitText(G.name,88,10),x,sy+7,10,Color{40,26,10,255},false);
   }else{
    DrawRectangleLines(x-14,sy-30,28,30,Color{70,80,100,120});
    if(gi==0)drawTextCentered("En attente du premier trophée",x+150,sy-24,10,C_DIM,false);
   }
  }
 }
 // reflets sur la vitre
 for(int k=0;k<3;k++){int gx=cx0+60+k*150+(int)(std::sin(t*0.4f+k)*6);DrawLine(gx,cy0+4,gx+30,cy0+60,Color{255,255,255,40});DrawLine(gx+6,cy0+4,gx+36,cy0+60,Color{255,255,255,22});}
 if((int)groups.size()>8)drawTextCentered(fmt("+ %d autres compétitions remportées (rubrique Trophées)",(int)groups.size()-8),403,262,10,C_DIM,false);
 drawTextCentered(fmt("%d saisons  |  %d trophées  |  %d finales",(int)H.seasons.size(),(int)H.trophies.size(),(int)H.finals.size()),403,(int)groups.size()>8?274:266,12,C_TXT,false);
 drawTextCentered(fitText(fmt("Archives depuis %d  |  %d souvenirs  |  ",H.firstYear,(int)H.timeline.size())+(T.founded?fmt("Fondation : %d | %s",T.founded,T.town.c_str()):std::string("Fondation non renseignée")),440,10),403,288,10,C_DIM,false);
 }else if(g_muDetail>=0&&g_muDetail<(int)filtered.size()){
 auto lines=wrapText(filtered[g_muDetail].detail,432);listRun(g_muText,(int)lines.size(),178,104,450,14,15,[&](int i,int x,int y,bool selected){drawTextPx(lines[i],x+5,y+2,10,selected?BLACK:C_TXT);});if(button(390,80,238,18,"Revenir à la liste")){g_muDetail=-1;g_muText=ListW();}
 }else{
 g_muDetail=-1;if(filtered.empty())drawTextPx("Aucune archive pour cette sélection.",185,117,10,C_DIM);
 int selected=listRun(g_muList,(int)filtered.size(),178,104,450,g_muPage==11?5:11,18,[&](int i,int x,int y,bool sel){drawTextPx(fitText(filtered[i].title,432,10),x+6,y+4,10,sel?BLACK:C_TXT);});if(selected>=0){g_muDetail=selected;g_muText=ListW();}
 if(g_muPage==11&&!H.shirts.empty()){const MuseumShirt* chosen=&H.shirts.back();if(!filtered.empty()){int row=std::clamp(g_muList.cur,0,(int)filtered.size()-1);for(auto& shirt:H.shirts)if(shirt.year==filtered[row].year)chosen=&shirt;}const auto& sh=*chosen;for(int k=0;k<(sh.third?3:2);k++){Kit kit;kit.shirt=sh.shirt[k];kit.shirt2=sh.shirt2[k];kit.shorts=sh.shorts[k];kit.socks=sh.socks[k];kit.pattern=sh.pattern[k];drawPlayerSprite(290+k*135,306,kit,2,k,0,0,PS_NORMAL,false,0,4);}}
 }
 drawTextPx("Données observées dans cette carrière ; historique réel non inventé.",180,334,10,C_DIM);
}
