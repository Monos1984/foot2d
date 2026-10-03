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
// personnalité visuelle des trophées : grandes oreilles, coupe d'Europe, championnat, coupe nationale, régional, district
static int museumTrophyStyle(const MuseumTrophy& tr){
 std::string n=tr.name;
 switch(tr.kind){case 3:return 1;case 7:case 8:case 9:case 14:return 7;case 4:case 16:case 17:case 26:case 27:return 6;case 5:case 18:return 5;default:break;}
 if(n.find("District")!=std::string::npos||n.find("Départemental")!=std::string::npos)return 5;
 if(n.find("Régional")!=std::string::npos||n.find("égion")!=std::string::npos)return 6;
 return tr.finalMatch.final?0:2;
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
 for(auto& tr:H.trophies){std::string nm=tr.name;auto g=std::find_if(groups.begin(),groups.end(),[&](const Group& x){return x.name==nm;});if(g==groups.end()){Group ng;ng.name=nm;ng.style=museumTrophyStyle(tr);groups.push_back(ng);g=groups.end()-1;}g->count++;g->last=std::max(g->last,tr.year);}
 std::stable_sort(groups.begin(),groups.end(),[](const Group& x,const Group& y){return x.count!=y.count?x.count>y.count:x.last>y.last;});
 // vitrine : cadre en noyer, montants en laiton, fond en velours aux couleurs du club, socles en marbre, spots et vitre
 int cx0=190,cy0=106,cw=426,ch=162;
 Color velvet=hexc(T.home.shirt);velvet=Color{(unsigned char)(velvet.r/5+14),(unsigned char)(velvet.g/5+10),(unsigned char)(velvet.b/5+16),255};
 Color brass{214,174,92,255},brass2{140,104,46,255};
 DrawRectangle(cx0-7,cy0-7,cw+14,ch+14,Color{38,22,12,255});DrawRectangle(cx0-5,cy0-5,cw+10,ch+10,Color{78,48,26,255});DrawRectangle(cx0-5,cy0-5,cw+10,2,Color{118,78,44,255});
 DrawRectangleGradientV(cx0,cy0,cw,ch,Color{(unsigned char)(velvet.r+16),(unsigned char)(velvet.g+16),(unsigned char)(velvet.b+20),255},velvet);
 for(int x=cx0+3;x<cx0+cw;x+=6)DrawRectangle(x,cy0,1,ch,Color{0,0,0,22});                               // plis du velours
 for(int k=0;k<4;k++){int px=k&1?cx0+cw+1:cx0-7,py=k&2?cy0+ch+1:cy0-7;DrawRectangle(px,py,6,6,brass);DrawRectangle(px+1,py+1,4,1,Color{250,226,150,255});}   // coins en laiton
 for(int k=1;k<4;k++){int mx=cx0+k*106;DrawRectangle(mx-1,cy0,2,ch,brass2);DrawRectangle(mx-1,cy0,1,ch,brass);}                       // montants
 DrawRectangle(cx0,cy0,cw,6,Color{20,14,10,255});
 for(int k=0;k<4;k++){int sx=cx0+53+k*106;DrawTriangle(Vector2{(float)sx,(float)cy0+5},Vector2{(float)sx-30,(float)cy0+ch},Vector2{(float)sx+30,(float)cy0+ch},Color{255,240,200,16});DrawTriangle(Vector2{(float)sx,(float)cy0+5},Vector2{(float)sx-14,(float)cy0+ch},Vector2{(float)sx+14,(float)cy0+ch},Color{255,244,214,16});DrawRectangle(sx-6,cy0+2,12,3,Color{230,230,240,255});DrawRectangle(sx-3,cy0+5,6,1,Color{255,250,200,255});
  for(int d=0;d<4;d++){float a=std::fmod(t*0.08f+d*0.29f+k*0.13f,1.f);DrawRectangle(sx-12+(int)(std::sin(t*0.6f+d*2+k)*10)+d*6,cy0+8+(int)(a*(ch-14)),1,1,Color{255,250,220,(unsigned char)(90*(1-a))});}}   // poussière dans la lumière
 auto mstyle=[](const Group& G){return G.style;};
 for(int shelf=0;shelf<2;shelf++){
  int sy=cy0+62+shelf*76;
  DrawRectangle(cx0,sy+5,cw,3,Color{200,220,235,150});DrawRectangle(cx0,sy+8,cw,2,Color{0,0,0,90});                    // étagère en verre
  for(int k=0;k<4;k++){
   int gi=shelf*4+k,x=cx0+53+k*106;
   if(gi<(int)groups.size()){
    const Group& G=groups[gi];
    // socle en marbre
    DrawRectangle(x-17,sy-1,34,6,Color{214,210,204,255});DrawRectangle(x-17,sy-1,34,1,Color{246,244,240,255});DrawRectangle(x-17,sy+4,34,1,Color{150,146,140,255});
    for(int v=0;v<3;v++)DrawLine(x-14+v*11,sy,x-9+v*11,sy+4,Color{170,168,164,150});
    DrawEllipse(x,sy-1,14,2,Color{0,0,0,70});
    DrawCircle(x,sy-26,20,Color{255,248,210,(unsigned char)(18+8*std::sin(t*1.5f+gi))});                                  // halo du spot
    fxBigTrophy(x,sy-1,mstyle(G),2,t+gi*0.4f);
    if(G.count>1){DrawRectangle(x+16,sy-60,22,11,Color{170,24,34,255});DrawTriangle(Vector2{(float)x+16,(float)sy-49},Vector2{(float)x+21,(float)sy-49},Vector2{(float)x+16,(float)sy-45},Color{120,14,24,255});drawTextCentered(fmt("x%d",G.count),x+27,sy-59,10,WHITE,false);}
    // plaque en laiton gravée : compétition et dernière victoire
    DrawRectangle(x-47,sy+8,94,11,brass);DrawRectangleLines(x-47,sy+8,94,11,brass2);DrawRectangle(x-46,sy+9,92,1,Color{250,226,150,255});
    DrawRectangle(x-45,sy+13,1,1,brass2);DrawRectangle(x+44,sy+13,1,1,brass2);
    drawTextCentered(fitText(G.name,86,10),x,sy+9,10,Color{48,30,10,255},false);
   }else{
    DrawRectangle(x-17,sy-1,34,6,Color{90,86,82,160});DrawRectangleLines(x-12,sy-30,24,29,Color{140,150,170,70});
    if(gi==0)drawTextCentered("En attente du premier trophée",x+150,sy-24,10,C_DIM,false);
   }
  }
 }
 // vitre : reflets obliques et liseré
 for(int k=0;k<3;k++){int gx=cx0+60+k*150+(int)(std::sin(t*0.4f+k)*6);DrawLine(gx,cy0+4,gx+30,cy0+60,Color{255,255,255,40});DrawLine(gx+6,cy0+4,gx+36,cy0+60,Color{255,255,255,22});DrawLine(gx+40,cy0+ch-50,gx+60,cy0+ch-6,Color{255,255,255,18});}
 DrawRectangleLines(cx0,cy0,cw,ch,Color{220,235,255,60});
 drawTextCentered(fmt("%d saisons  |  %d trophées  |  %d finales",(int)H.seasons.size(),(int)H.trophies.size(),(int)H.finals.size())+((int)groups.size()>8?fmt("  |  + %d autres (Trophées)",(int)groups.size()-8):std::string()),403,275,10,C_TXT,false);
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
