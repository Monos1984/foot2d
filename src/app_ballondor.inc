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
    bool revealed=g_ballonReplay || g_ballonTime>=4.0f;
    if(button(205,58,110,14,"Masculin",g_ballonGender==0)){g_ballonGender=0;g_ballonTime=0;}
    if(button(325,58,110,14,"Féminin",g_ballonGender==1)){g_ballonGender=1;g_ballonTime=0;}
    drawTextCentered(fmt("28 octobre %d  -  %s",E.year,g_ballonGender ? "Ballon d'or féminin" : "Ballon d'or"),VW/2,36,10,C_HI);
    if(g_ballonReplay){
        if(button(22,36,24,14,"<") || IN.left){g_ballonEdition=(g_ballonEdition+(int)g_career.ballonEditions.size()-1)%(int)g_career.ballonEditions.size();g_ballonTime=0;return;}
        if(button(VW-46,36,24,14,">") || IN.right){g_ballonEdition=(g_ballonEdition+1)%(int)g_career.ballonEditions.size();g_ballonTime=0;return;}
    }
    // Scène, rideaux et projecteurs en gros pixels, adaptés au rendu 16 bits.
    DrawRectangle(36,82,VW-72,218,Color{6,10,24,255});
    for(int side=0;side<2;side++)for(int x=0;x<7;x++)DrawRectangle(side ? VW-70+x*4 : 38+x*4,82,4,174,x%2 ? Color{94,16,35,255} : Color{130,22,44,255});
    for(int i=0;i<20;i++){
        int x=75+(i*83)%490,y=92+(i*37)%60;
        DrawRectangle(x,y,2,2,((int)(g_ballonTime*3)+i)%3 ? Color{122,106,46,255} : C_HI);
    }
    DrawRectangle(68,254,VW-136,42,Color{38,31,26,255});
    DrawRectangle(68,254,VW-136,3,C_HI);
    // Trophée doré, dessiné par le jeu.
    DrawRectangle(VW/2-8,136,16,8,Color{154,109,28,255});
    DrawRectangle(VW/2-4,122,8,14,C_HI);
    DrawRectangle(VW/2-12,103,24,20,C_HI);
    DrawRectangle(VW/2-8,99,16,28,Color{242,193,59,255});
    DrawRectangle(VW/2-8,104,7,6,Color{255,240,144,255});
    DrawRectangle(VW/2+2,114,6,6,Color{155,103,25,255});
    int n=E.count[g_ballonGender];
    for(int rank=2;rank>=0;--rank){
        if(rank>=n || (!revealed && g_ballonTime<(3-rank)*1.15f))continue;
        const BallonNominee& P=E.podium[g_ballonGender][rank];
        int x=rank==0 ? VW/2 : rank==1 ? 176 : 464;
        int y=rank==0 ? 164 : 186;
        Color medal=rank==0 ? C_HI : rank==1 ? Color{206,220,234,255} : Color{218,150,83,255};
        DrawRectangle(x-52,y+39,104,255-(y+39),medal);
        drawTextCentered(fmt("%d",rank+1),x,y+46,20,Color{24,24,32,255});
        if(P.team>=0 && P.team<(int)g_world.teams.size())drawPlayerSprite(x,y+8,g_world.teams[P.team].home,1,0,0,0,PS_NORMAL,false,0,3);
        drawTextCentered(fitText(P.name,170,10),x,rank==0 ? 145 : 163,10,rank==0 ? C_HI : C_TXT);
    }
    if(revealed && n){
        const auto& winner=E.podium[g_ballonGender][0];
        drawTextCentered(std::string("LAURÉAT")+(g_ballonGender ? "E : " : " : ")+winner.name,VW/2,275,10,C_HI);
        if(winner.team>=0 && winner.team<(int)g_world.teams.size())drawTextCentered(fitText(g_world.teams[winner.team].name,VW-40,10),VW/2,298,10,C_TXT);
        drawTextCentered(fmt("%d buts  /  %d passes  /  %d matchs",winner.goals,winner.assists,winner.apps),VW/2,311,10,C_DIM);
    }else drawTextCentered("Le jury va révéler le podium...",VW/2,275,10,C_TXT);
    bool next=button(VW/2-104,328,208,14,revealed ? "Continuer la carrière" : "Révéler le podium");
    drawFooter(g_ballonReplay ? "Jury simulé   G/D : édition   Tab : masculin/féminin   Retour" : "Jury simulé   Tab : masculin/féminin   OK : révéler / continuer");
    if(IN.tab){g_ballonGender=1-g_ballonGender;g_ballonTime=0;}
    if(IN.ok || IN.start || next || IN.back){
        if(!revealed && !IN.back){g_ballonTime=5;return;}
        if(!g_ballonReplay){g_career.ballonEditions[g_ballonEdition].presented=1;if(g_pending.comp<0)g_needAdvance=true;autosave();}
        openHub();
    }
}

