// Décors communs des cérémonies (salle, scène, public, podium) : Ballon d'or, soirée du championnat, musée, fête des supporters.
// apparence d'un joueur retrouvé dans son effectif (peau, cheveux) ; garde les valeurs par défaut sinon
static void playerLook(int team,int pid,int& skin,int& hair){
    if(team<0||team>=(int)g_world.teams.size())return;
    for(auto& p:g_world.teams[team].squad)if(p.id==pid){skin=p.skin;hair=p.hair;return;}
}

// ---- décor commun des cérémonies en salle (Ballon d'or, soirée du championnat)
// salle sombre, écran géant, projecteurs, rideaux, scène éclairée ; renvoie la hauteur du plancher
static void stageBackdrop(int x0,int y0,int x1,int y1,float t,Color accent,bool festive){
    DrawRectangleGradientV(x0,y0,x1-x0,y1-y0,Color{10,8,30,255},Color{30,14,44,255});
    // lumières de la salle (points qui scintillent dans le fond)
    for(int i=0;i<46;i++){int x=x0+12+(i*97)%(x1-x0-24),y=y0+6+(i*53)%70;bool on=((int)(t*2.5f)+i*7)%5;DrawRectangle(x,y,1,1,on?Color{accent.r,accent.g,accent.b,160}:Color{80,70,110,160});}
    // écran géant à l'arrière
    int sw=230,sx=(x0+x1)/2-sw/2,sy=y0+6;
    DrawRectangle(sx-3,sy-3,sw+6,58,Color{40,34,30,255});
    DrawRectangleGradientV(sx,sy,sw,52,Color{20,24,60,255},Color{60,20,70,255});
    for(int k=0;k<52;k+=2)DrawRectangle(sx,sy+k,sw,1,Color{0,0,0,40});
    for(int k=0;k<12;k++){float a=std::fmod(t*0.3f+k*0.083f,1.f);DrawRectangle(sx+(int)(a*sw),sy+4+(k*13)%44,2,1,Color{accent.r,accent.g,accent.b,(unsigned char)(140*(1-a))});}
    DrawRectangleLines(sx-1,sy-1,sw+2,54,accent);
    // projecteurs et rideaux
    fxSpotlights(x0+40,y0,x1-x0-80,y1-y0-40,t,festive?5:3,festive?Color{255,240,180,255}:Color{200,210,255,255});
    fxCurtains(x0,y0,34,y1-y0-30,Color{130,22,44,255},Color{94,16,35,255});
    fxCurtains(x1-34,y0,34,y1-y0-30,Color{130,22,44,255},Color{94,16,35,255});
    DrawRectangle(x0,y0,x1-x0,6,Color{94,16,35,255});for(int i=x0;i<x1;i+=3)DrawRectangle(i,y0+5,2,3,Color{230,180,60,255});
}
// scène : plancher brillant avec reflets
static void stageFloor(int x0,int y,int x1,int h,Color edge){
    DrawRectangleGradientV(x0,y,x1-x0,h,Color{58,44,40,255},Color{26,20,22,255});
    DrawRectangle(x0,y,x1-x0,2,edge);
    for(int i=x0;i<x1;i+=24)DrawRectangle(i,y+3,12,1,Color{255,255,255,24});
}
// public au premier plan, de dos : têtes, mains qui applaudissent, flashs des téléphones
static void stageAudience(int x0,int y,int x1,int h,float t,bool cheer){
    static const unsigned HAIR[5]={0x141414,0x2A1E14,0x3A2814,0x5A5A5A,0x6A3018};
    for(int row=0;row<3;row++){
        int ry=y+row*(h/3),r=5+row*2;
        for(int k=0;k*(r*2+3)<x1-x0+r*2;k++){
            int x=x0+k*(r*2+3)+(row%2?r:0);
            unsigned id=(unsigned)(row*131+k*977);
            int bob=cheer&&((id>>3)%3)?(int)(std::fabs(std::sin(t*6+k))*2):0;
            DrawCircle(x,ry-bob,(float)r,Color{14,12,22,255});
            DrawCircle(x,ry-1-bob,(float)r-1,hexc(HAIR[(id*7)%5],255));
            DrawCircle(x-1,ry-2-bob,(float)r-3,Color{255,255,255,14});
            DrawRectangle(x-r-1,ry+r-2-bob,r*2+2,h/3,Color{18,16,28,255});
            if(cheer&&id%4==0){bool up=std::sin(t*8+k)>0;DrawRectangle(x-r,ry-r-(up?6:3)-bob,3,4,Color{224,180,140,255});DrawRectangle(x+r-3,ry-r-(up?3:6)-bob,3,4,Color{224,180,140,255});}
            if(cheer&&id%9==2&&((int)(t*4)+k)%6==0){DrawRectangle(x-2,ry-r-10,4,6,Color{40,40,50,255});DrawRectangle(x-1,ry-r-9,2,3,Color{255,255,255,255});DrawCircle(x,ry-r-8,4,Color{255,255,255,90});}
        }
    }
}
// bloc de podium : métal, chiffre, liseré
static void podiumBlock(int cx,int top,int bottom,int w,int rank){
    Color c=rank==0?Color{240,196,60,255}:rank==1?Color{196,206,222,255}:Color{205,132,70,255};
    DrawRectangle(cx-w/2+3,top+3,w,bottom-top,Color{0,0,0,80});
    DrawRectangleGradientV(cx-w/2,top,w,bottom-top,c,Color{(unsigned char)(c.r*0.55f),(unsigned char)(c.g*0.55f),(unsigned char)(c.b*0.55f),255});
    DrawRectangle(cx-w/2,top,w,3,Color{255,255,255,120});
    DrawRectangle(cx-w/2,top,4,bottom-top,Color{255,255,255,50});
    drawTextCentered(fmt("%d",rank+1),cx,top+8,20,Color{30,24,30,255},false);
}

