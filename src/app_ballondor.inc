// Cérémonie pixelisée et consultation des éditions sauvegardées.
static int g_ballonEdition=-1,g_ballonGender=0;
static float g_ballonTime=0;
static bool g_ballonReplay=false;
static void openBallon(bool replay) {
    g_ballonReplay=replay;
    g_ballonEdition=replay ? (int)g_career.ballonEditions.size()-1 : g_career.ballonPending();
    g_ballonGender=isWomenTeam(g_career.userTeam) ? 1 : 0;
    g_ballonTime=0;g_screen=SC_BALLON;
    if(g_ballonEdition>=0)audioJingle(1);
}

static void screenBallon() {
    drawBackground("Cérémonie du Ballon d'or");
    if(g_ballonEdition<0 || g_ballonEdition>=(int)g_career.ballonEditions.size()) {
        drawTextCentered("La prochaine cérémonie aura lieu fin octobre.",VW/2,100,10,C_HI);
        drawTextCentered("Le jury du jeu récompense les performances des joueurs et des joueuses.",VW/2,122,10,C_TXT);
        drawFooter("Retour : carrière");if(IN.ok || IN.back)openHub();return;
    }
    const BallonEdition& E=g_career.ballonEditions[g_ballonEdition];
    g_ballonTime+=GetFrameTime();
    float t=(float)GetTime();
    bool revealed=g_ballonReplay || g_ballonTime>=4.0f;
    drawTextCentered(fmt("28 octobre %d  -  %s",E.year,g_ballonGender ? "Ballon d'or féminin" : "Ballon d'or"),VW/2,36,10,C_HI);
    if(g_ballonReplay){
        if(button(22,36,24,14,"<") || IN.left){g_ballonEdition=(g_ballonEdition+(int)g_career.ballonEditions.size()-1)%(int)g_career.ballonEditions.size();g_ballonTime=0;return;}
        if(button(VW-46,36,24,14,">") || IN.right){g_ballonEdition=(g_ballonEdition+1)%(int)g_career.ballonEditions.size();g_ballonTime=0;return;}
    }
    // salle, écran géant, projecteurs ; la scène s'illumine à la révélation
    const int X0=20,X1=VW-20,Y0=52,Y1=304,FLOOR=244;
    stageBackdrop(X0,Y0,X1,Y1,t,C_HI,revealed);
    int n=E.count[g_ballonGender];
    {   // écran : titre puis nom du lauréat
        int sy=Y0+6;
        if(revealed&&n){
            const auto& winner=E.podium[g_ballonGender][0];
            drawTextCentered(g_ballonGender?"LAURÉATE":"LAURÉAT",VW/2,sy+5,10,C_TXT,false);
            drawTextCentered(fitText(winner.name,220,20),VW/2,sy+19,20,C_HI);
            if(winner.team>=0&&winner.team<(int)g_world.teams.size())drawTextCentered(fitText(g_world.teams[winner.team].name,220,10),VW/2,sy+39,10,C_DIM,false);
        }else{
            drawTextCentered(g_ballonGender?"BALLON D'OR FÉMININ":"BALLON D'OR",VW/2,sy+10,20,C_HI);
            drawTextCentered(fmt("Édition %d",E.year),VW/2,sy+34,10,C_DIM,false);
        }
    }
    stageFloor(X0+34,FLOOR,X1-34,Y1-FLOOR,C_HI);
    // le Ballon d'or sur sa colonne, puis dans les mains du lauréat
    if(!revealed||!n){DrawRectangle(VW/2-6,FLOOR-30,12,30,Color{30,30,40,255});DrawRectangle(VW/2-8,FLOOR-32,16,3,C_HI);fxBigTrophy(VW/2,FLOOR-32,3,2,t);}
    for(int rank=2;rank>=0;--rank){
        if(rank>=n || (!revealed && g_ballonTime<(3-rank)*1.15f))continue;
        const BallonNominee& P=E.podium[g_ballonGender][rank];
        int x=rank==0 ? VW/2 : rank==1 ? 196 : 444;
        int top=rank==0 ? 196 : rank==1 ? 212 : 222;
        if(!revealed&&rank==0)continue;
        // cône de lumière sur la personne nommée
        DrawTriangle(Vector2{(float)x,(float)Y0+8},Vector2{(float)x-34,(float)top},Vector2{(float)x+34,(float)top},Color{255,245,200,(unsigned char)(rank==0?46:28)});
        podiumBlock(x,top,FLOOR,rank==0?96:84,rank);
        if(P.team>=0 && P.team<(int)g_world.teams.size()){
            int skin=(int)(P.pid%NUM_SKINS),hair=(int)((P.pid/7)%NUM_HAIRS);
            playerLook(P.team,P.pid,skin,hair);
            if(rank==0&&revealed){
                int bob=(int)(std::fabs(std::sin(t*3.f))*2);
                drawPlayerSprite(x,top-1,g_world.teams[P.team].home,skin,hair,0,0,PS_CELEB,false,0,3);
                fxBigTrophy(x,top-36-bob,3,1,t);
            }else drawPosedSprite(x,top-1,g_world.teams[P.team].home,skin,hair,0,(int)(t*3),PS_NORMAL,revealed?POSE_CLAP:POSE_NONE,3);
        }
        drawTextCentered(fitText(P.name,rank==0?200:150,10),x,top-(rank==0&&revealed?66:50),10,rank==0 ? C_HI : C_TXT);
    }
    if(revealed&&n){
        static const Color GOLDC[4]={{255,214,80,255},{255,244,170,255},{214,150,32,255},{255,255,255,255}};
        fxConfetti(X0+34,Y0+6,X1-X0-68,FLOOR-Y0,t,GOLDC,4,110,7);
        fxFireworks(X0+40,Y0+60,X1-X0-80,90,t,11,3);
    }
    stageAudience(X0,Y1-34,X1,36,t,revealed);
    if(revealed && n){
        const auto& winner=E.podium[g_ballonGender][0];
        drawTextCentered(fmt("%d buts  /  %d passes  /  %d matchs",winner.goals,winner.assists,winner.apps),VW/2,310,10,C_DIM);
    }else drawTextCentered("Le jury va révéler le podium...",VW/2,310,10,C_TXT);
    if(button(52,34,96,14,"Masculin",g_ballonGender==0)){g_ballonGender=0;g_ballonTime=0;}
    if(button(VW-148,34,96,14,"Féminin",g_ballonGender==1)){g_ballonGender=1;g_ballonTime=0;}
    bool next=button(VW/2-104,326,208,14,revealed ? "Continuer la carrière" : "Révéler le podium");
    drawFooter(g_ballonReplay ? "Jury simulé   G/D : édition   Tab : masculin/féminin   Retour" : "Jury simulé   Tab : masculin/féminin   OK : révéler / continuer");
    if(IN.tab){g_ballonGender=1-g_ballonGender;g_ballonTime=0;}
    if(IN.ok || IN.start || next || IN.back){
        if(!revealed && !IN.back){g_ballonTime=5;return;}
        if(!g_ballonReplay){g_career.ballonEditions[g_ballonEdition].presented=1;if(g_pending.comp<0)g_needAdvance=true;autosave();}
        openHub();
    }
}
