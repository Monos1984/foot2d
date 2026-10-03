// ------------------------------------------------------------------ éditeur de compétitions de carrière
// Hors carrière : modèle des nouvelles carrières (competitions_perso.txt). En carrière : règles de la carrière en cours.
static ListW g_ceLW, g_ceLW2;
// 0 pays, 1 divisions, 2 règles division, 3 clubs, 4 déplacer club, 5 ajouter club,
// 6 coupe nationale, 7 régions, 8 région+d districts, 9 district, 10 coupes perso,
// 11 coupe perso, 13 choix de poule régionale/district
static int g_ceMode = 0;
static int g_cePyr = -1, g_ceTier = -1, g_ceClub = -1, g_cePool = -1;
static int g_ceGeoRegion = -1, g_ceGeoDistrict = -1, g_ceCupId = -1;
static int g_ceRenameKind = 0; // 0 division, 1 région, 2 district, 3 coupe
static bool g_ceTyping = false;

static bool ceInCareer() { return g_careerActive && g_career.kind == CK_CLUB; }
static std::vector<Pyramid>& cePyrs() { return ceInCareer() ? g_career.pyramids : g_basePyramids; }
static void openCompEditor() {
    g_ceMode = 0; g_ceLW = ListW(); g_ceLW2 = ListW(); g_cePyr = -1; g_ceTier = -1;
    g_cePool = -1; g_ceTyping = false; g_ceRenameKind = 0; g_textMode = false; g_screen = SC_COMPEDIT;
}
static void ceChanged(Pyramid& P) {
    if (ceInCareer()) return;           // carrière : règles conservées dans la sauvegarde
    markPyrEdited(P.country); saveCompEdits();
}
static const char* TB_NAMES[] = { "différence de buts", "règlement LFP", "règlement FFF (particulière)", "confrontations directes", "buts marqués (Angleterre)", "règlement FIFA" };
static const char* LEGS_NAMES[] = { "", "aller simple", "aller-retour", "3 matchs", "4 matchs" };
static const char* SCOPE_NAMES[] = { "National", "Zone", "Régional", "District" };
static const char* CUP_SCOPE_NAMES[] = { "Nationale", "Régionale", "District" };
static const char* CUP_QUAL_NAMES[] = { "Aucune", "Qualification en coupe nationale", "Place européenne de la coupe", "Ligue des champions", "Ligue Europa", "Ligue Conférence" };

static std::vector<int> cePoolsForTier(const Pyramid& P, int tier) {
    std::vector<int> v; for (int i = 0; i < (int)P.pools.size(); i++) if (P.pools[i].tier == tier) v.push_back(i); return v;
}
static std::string cePoolName(const Pyramid& P, int q) {
    if (q < 0 || q >= (int)P.pools.size()) return "Poule";
    const Pool& pl = P.pools[q];
    int sc = P.tiers[pl.tier].scope;
    if (sc == SC_REGION) return geoRegionName(P.country, pl.key);
    if (sc == SC_DEPT) return geoDistrictName(P.country, pl.key);
    return poolLabel(P, pl, 0);
}
static int ceFirstGeoKey(const std::string& country, int scope) {
    if (scope == ECS_REGION) { auto v = geoRegions(country); return v.empty() ? -1 : v[0].id; }
    if (scope == ECS_DISTRICT) { auto v = geoDistricts(country); return v.empty() ? -1 : v[0].id; }
    return -1;
}

static void screenCompEdit() {
    auto& PY = cePyrs();
    std::string sub = ceInCareer() ? "Carrière en cours : les règles de championnat s'appliquent à la saison suivante" : "Modèle des nouvelles carrières (competitions_perso.txt)";
    drawBackground("Éditeur de compétitions");
    drawTextCentered(sub, VW / 2, 30, 10, C_DIM, false);
    if (g_cePyr >= (int)PY.size()) g_ceMode = 0;

    // ---------------- 0 : choix du pays
    if (g_ceMode == 0) {
        struct LI { std::string name; int pyr; int grp; };
        std::vector<LI> v;
        for (int p = 0; p < (int)PY.size(); p++) {
            const Pyramid& P = PY[p]; if (!pyrEditable(P)) continue;
            bool fem = isWomenPyramid(P); int ni = g_world.nationIndex(P.country.c_str());
            int grp = P.country == "FRA" ? 0 : fem ? 10 : ni >= 0 ? 1 + NATIONS[ni].conf : 9;
            v.push_back({ std::string(fem ? "[F] " : "") + P.name + "  -  " + P.tiers[0].name, p, grp });
        }
        std::stable_sort(v.begin(), v.end(), [](const LI& a, const LI& b) { if (a.grp != b.grp) return a.grp < b.grp; return sortKey(a.name) < sortKey(b.name); });
        int s = listRun(g_ceLW, (int)v.size(), 40, 44, VW - 80, 20, 14, [&](int i, int x, int y, bool sel) {
            const Pyramid& P = PY[v[i].pyr];
            drawTextPx(fitText(v[i].name, VW - 260, 10), x + 6, y + 2, 10, sel ? BLACK : v[i].grp == 10 ? Color{ 240, 150, 200, 255 } : C_TXT);
            std::string r = fmt("%d division%s", (int)P.tiers.size(), P.tiers.size() > 1 ? "s" : "") + (pyrEdited(P.country) && !ceInCareer() ? "  (modifié)" : "");
            drawTextPx(r, x + VW - 80 - 8 - textWidth(r, 10), y + 2, 10, sel ? BLACK : pyrEdited(P.country) ? C_HI : C_DIM);
        });
        drawFooter("OK : modifier ce championnat   Retour");
        if (s >= 0) { g_cePyr = v[s].pyr; g_ceMode = 1; g_ceLW2 = ListW(); }
        if (IN.back) g_screen = SC_EDITMENU;
        return;
    }

    Pyramid& P = PY[g_cePyr];
    bool structOk = !ceInCareer() && pyrStructEditable(P);

    // ---------------- 7 : régions du pays
    if (g_ceMode == 7) {
        auto regs = geoRegions(P.country);
        int n = (int)regs.size() + 2;
        drawTextCentered("Régions de " + P.name, VW / 2, 42, 10, C_HI, false);
        int s = listRun(g_ceLW, n, 60, 58, VW - 120, 17, 18, [&](int i, int x, int y, bool sel) {
            if (i < (int)regs.size()) {
                int nd = (int)geoDistricts(P.country, regs[i].id).size();
                drawTextPx(fitText(regs[i].name, 330, 10), x + 8, y + 4, 10, sel ? BLACK : C_TXT);
                drawTextPx(fmt("%d district%s", nd, nd > 1 ? "s" : ""), x + 350, y + 4, 10, sel ? BLACK : C_DIM);
            } else if (i == (int)regs.size()) drawTextPx("+ Créer une région", x + 8, y + 4, 10, sel ? BLACK : C_HI);
            else drawTextPx("Retour", x + 8, y + 4, 10, sel ? BLACK : C_HI);
        });
        drawFooter("OK : ouvrir / créer   Les régions servent aux divisions et coupes régionales");
        if (s >= 0 && s < (int)regs.size()) { g_ceGeoRegion = regs[s].id; g_ceMode = 8; g_ceLW2 = ListW(); }
        else if (s == (int)regs.size()) {
            g_ceGeoRegion = geoAddRegion(P.country, "Nouvelle région"); pyrRefreshGeo(P); ceChanged(P); saveCompEdits();
            g_ceMode = 8; g_ceTyping = true; g_ceRenameKind = 1; g_textMode = true;
        } else if (s == n - 1 || IN.back) { g_ceMode = 1; g_ceLW2 = ListW(); }
        return;
    }

    // ---------------- 8 : région et districts
    if (g_ceMode == 8) {
        if (g_ceTyping && g_ceRenameKind == 1) {
            auto regs = geoRegions(P.country); std::string name = geoRegionName(P.country, g_ceGeoRegion);
            textEdit(name, 40); geoRenameRegion(P.country, g_ceGeoRegion, name);
            drawTextCentered("Renommer la région : " + name + (fmod(GetTime(),1.0)<.5 ? "_" : ""), VW/2, 42, 10, C_HI, false);
            drawFooter("Tapez le nom   Entrée / Échap : valider");
            if (IN.start || IN.back || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_ESCAPE)) { g_ceTyping=false; g_textMode=false; saveCompEdits(); pyrRefreshGeo(P); ceChanged(P); }
            return;
        }
        auto ds = geoDistricts(P.country, g_ceGeoRegion);
        std::vector<std::string> rows; rows.push_back("Nom de la région : " + geoRegionName(P.country, g_ceGeoRegion)); rows.push_back("+ Ajouter un district");
        for (auto& d : ds) rows.push_back("District : " + d.name);
        rows.push_back("Supprimer cette région"); rows.push_back("Retour");
        drawTextCentered("Région : " + geoRegionName(P.country, g_ceGeoRegion), VW/2, 42, 10, C_HI, false);
        int s = menuRun(g_ceLW2, rows, 60, 430);
        drawFooter("OK : modifier   Les districts peuvent alimenter des divisions/coupes de district");
        if (s == 0) { g_ceTyping=true; g_ceRenameKind=1; g_textMode=true; while(GetCharPressed()>0){} }
        else if (s == 1) { g_ceGeoDistrict=geoAddDistrict(P.country,g_ceGeoRegion,"Nouveau district"); pyrRefreshGeo(P); ceChanged(P); saveCompEdits(); g_ceMode=9; g_ceTyping=true; g_ceRenameKind=2; g_textMode=true; }
        else if (s >= 2 && s < 2+(int)ds.size()) { g_ceGeoDistrict=ds[s-2].id; g_ceMode=9; g_ceLW=ListW(); }
        else if (s == 2+(int)ds.size()) {
            if (!geoDeleteRegion(P.country,g_ceGeoRegion)) toast("Cette région de base ne peut pas être supprimée");
            else { pyrRefreshGeo(P); ceChanged(P); saveCompEdits(); toast("Région supprimée"); g_ceMode=7; g_ceLW=ListW(); }
        } else if (s == 3+(int)ds.size() || IN.back) { g_ceMode=7; g_ceLW=ListW(); }
        return;
    }

    // ---------------- 9 : district
    if (g_ceMode == 9) {
        if (g_ceTyping && g_ceRenameKind == 2) {
            std::string name = geoDistrictName(P.country, g_ceGeoDistrict); textEdit(name,40); geoRenameDistrict(P.country,g_ceGeoDistrict,name);
            drawTextCentered("Renommer le district : " + name + (fmod(GetTime(),1.0)<.5 ? "_" : ""), VW/2,42,10,C_HI,false);
            drawFooter("Tapez le nom   Entrée / Échap : valider");
            if (IN.start || IN.back || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_ESCAPE)) { g_ceTyping=false; g_textMode=false; saveCompEdits(); pyrRefreshGeo(P); ceChanged(P); }
            return;
        }
        std::vector<std::string> rows = { "Nom : " + geoDistrictName(P.country,g_ceGeoDistrict), "Supprimer ce district", "Retour" };
        drawTextCentered("District de " + geoRegionName(P.country,g_ceGeoRegion),VW/2,42,10,C_HI,false);
        int s=menuRun(g_ceLW,rows,80,400);
        drawFooter("OK : modifier   Retour");
        if(s==0){g_ceTyping=true;g_ceRenameKind=2;g_textMode=true;while(GetCharPressed()>0){}}
        else if(s==1){ if(!geoDeleteDistrict(P.country,g_ceGeoDistrict)) toast("Ce district de base ne peut pas être supprimé"); else {pyrRefreshGeo(P);ceChanged(P);saveCompEdits();toast("District supprimé");g_ceMode=8;g_ceLW2=ListW();} }
        else if(s==2||IN.back){g_ceMode=8;g_ceLW2=ListW();}
        return;
    }

    // ---------------- 10 : liste des coupes personnalisées du pays
    if (g_ceMode == 10) {
        std::vector<int> ids; for(auto& c:editorCups()) if(c.country==P.country) ids.push_back(c.id);
        int n=(int)ids.size()+2;
        drawTextCentered("Coupes supplémentaires : " + P.name,VW/2,42,10,C_HI,false);
        int s=listRun(g_ceLW,n,50,58,VW-100,18,18,[&](int i,int x,int y,bool sel){
            if(i<(int)ids.size()) { EditorCupDef* c=editorCupById(ids[i]); std::string sc=c?CUP_SCOPE_NAMES[std::max(0,std::min(2,c->scope))]:""; drawTextPx(fitText(c?c->name:"?",300,10),x+8,y+4,10,sel?BLACK:C_TXT); drawTextPx(sc,x+320,y+4,10,sel?BLACK:C_DIM); }
            else if(i==(int)ids.size()) drawTextPx("+ Créer une coupe",x+8,y+4,10,sel?BLACK:C_HI); else drawTextPx("Retour",x+8,y+4,10,sel?BLACK:C_HI);
        });
        drawTextCentered("Exemple : Landespokal → qualification en coupe nationale ; coupe régionale → Europe si le règlement le prévoit.",VW/2,VH-44,10,C_DIM,false);
        drawFooter("OK : modifier / créer   Retour");
        if(s>=0&&s<(int)ids.size()){g_ceCupId=ids[s];g_ceMode=11;g_ceLW2=ListW();}
        else if(s==(int)ids.size()){g_ceCupId=addEditorCup(P.country,"Nouvelle coupe");saveCompEdits();g_ceMode=11;g_ceTyping=true;g_ceRenameKind=3;g_textMode=true;}
        else if(s==n-1||IN.back){g_ceMode=1;g_ceLW2=ListW();}
        return;
    }

    // ---------------- 11 : paramètres d'une coupe personnalisée
    if (g_ceMode == 11) {
        EditorCupDef* c=editorCupById(g_ceCupId); if(!c){g_ceMode=10;return;}
        if (!P.tiers.empty()) {
            c->minTier = std::max(0, std::min((int)P.tiers.size()-1, c->minTier));
            c->maxTier = std::max(c->minTier, std::min((int)P.tiers.size()-1, c->maxTier));
        }
        if(g_ceTyping&&g_ceRenameKind==3){textEdit(c->name,40);drawTextCentered("Nom : "+c->name+(fmod(GetTime(),1.0)<.5?"_":""),VW/2,42,10,C_HI,false);drawFooter("Tapez le nom   Entrée / Échap : valider");if(IN.start||IN.back||IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_KP_ENTER)||IsKeyPressed(KEY_ESCAPE)){g_ceTyping=false;g_textMode=false;if(c->name.empty())c->name="Nouvelle coupe";saveCompEdits();}return;}
        std::string area="-"; if(c->scope==ECS_REGION) area=geoRegionName(P.country,c->geoKey); else if(c->scope==ECS_DISTRICT) area=geoDistrictName(P.country,c->geoKey);
        std::vector<std::string> rows={
            "Nom : "+c->name,
            std::string("Active : ")+(c->enabled?"oui":"non"),
            std::string("Portée : ")+CUP_SCOPE_NAMES[std::max(0,std::min(2,c->scope))],
            "Zone : "+area,
            fmt("Division la plus haute admise : %d",c->minTier+1),
            fmt("Division la plus basse admise : %d",c->maxTier+1),
            fmt("Clubs en phase finale : %d",c->finalSize),
            std::string("Prolongation : ")+(c->et?"oui":"non, TAB directs"),
            std::string("Finale terrain neutre : ")+(c->neutralFinal?"oui":"non"),
            std::string("Qualification du vainqueur : ")+CUP_QUAL_NAMES[std::max(0,std::min(5,c->qual))],
            "Supprimer cette coupe","Retour"};
        drawTextCentered("Coupe : "+c->name,VW/2,42,10,C_HI,false);
        int s=menuRun(g_ceLW2,rows,54,455); int cur=g_ceLW2.cur; int d=IN.left?-1:IN.right?1:0; bool ch=false;
        auto act=[&](int r,int delta){
            if(r==1){c->enabled=!c->enabled;ch=true;}
            else if(r==2){c->scope=(c->scope+(delta?delta:1)+3)%3;c->geoKey=ceFirstGeoKey(P.country,c->scope);ch=true;}
            else if(r==3&&c->scope!=ECS_NATIONAL){
                std::vector<int> keys;if(c->scope==ECS_REGION)for(auto&x:geoRegions(P.country))keys.push_back(x.id);else for(auto&x:geoDistricts(P.country))keys.push_back(x.id);
                if(!keys.empty()){int k=0;for(int i=0;i<(int)keys.size();i++)if(keys[i]==c->geoKey)k=i;k=(k+(delta?delta:1)+(int)keys.size())%(int)keys.size();c->geoKey=keys[k];ch=true;}
            } else if(r==4){c->minTier=std::max(0,std::min((int)P.tiers.size()-1,c->minTier+(delta?delta:1)));if(c->maxTier<c->minTier)c->maxTier=c->minTier;ch=true;}
            else if(r==5){c->maxTier=std::max(c->minTier,std::min((int)P.tiers.size()-1,c->maxTier+(delta?delta:1)));ch=true;}
            else if(r==6){c->finalSize=std::max(2,std::min(512,c->finalSize+(delta?delta:1)*2));ch=true;}
            else if(r==7){c->et=!c->et;ch=true;} else if(r==8){c->neutralFinal=!c->neutralFinal;ch=true;}
            else if(r==9){c->qual=(c->qual+(delta?delta:1)+6)%6;ch=true;}
        };
        if(d&&cur>=1&&cur<=9)act(cur,d); if(s>=1&&s<=9)act(s,1);
        if(ch)saveCompEdits();
        drawFooter("Gauche / Droite ou OK : modifier   Retour");
        if(s==0){g_ceTyping=true;g_ceRenameKind=3;g_textMode=true;while(GetCharPressed()>0){}}
        else if(s==10){int id=c->id;askConfirm("Supprimer cette coupe ?",[id](){deleteEditorCup(id);saveCompEdits();g_ceMode=10;g_ceLW=ListW();},"Supprimer","Annuler");}
        else if(s==11||IN.back){g_ceMode=10;g_ceLW=ListW();}
        return;
    }

    // ---------------- 13 : choix de la poule géographique d'une division
    if (g_ceMode == 13) {
        auto pools=cePoolsForTier(P,g_ceTier); drawTextCentered("Poule de " + P.tiers[g_ceTier].name,VW/2,42,10,C_HI,false);
        int n=(int)pools.size()+1; int s=listRun(g_ceLW,n,60,58,VW-120,18,18,[&](int i,int x,int y,bool sel){
            if(i<(int)pools.size()){const Pool&pl=P.pools[pools[i]];drawTextPx(fitText(cePoolName(P,pools[i]),300,10),x+8,y+4,10,sel?BLACK:C_TXT);drawTextPx(fmt("%d clubs",(int)pl.clubs.size()),x+330,y+4,10,sel?BLACK:C_DIM);}else drawTextPx("Retour",x+8,y+4,10,sel?BLACK:C_HI);
        });
        drawFooter("OK : gérer les clubs de cette région / district   Retour");
        if(s>=0&&s<(int)pools.size()){g_cePool=pools[s];g_ceMode=3;g_ceLW2=ListW();}
        else if(s==n-1||IN.back){g_ceMode=2;g_ceLW=ListW();}
        return;
    }

    // ---------------- 1 : divisions du pays
    if (g_ceMode == 1) {
        drawTextCentered(P.name + (ceInCareer() ? "  -  structure figée pendant la carrière" : ""), VW / 2, 42, 10, C_HI, false);
        int T = (int)P.tiers.size(); std::vector<std::string> extra;
        if (structOk) extra.push_back("+ Ajouter une division en bas de la pyramide");
        extra.push_back("Régions et districts"); extra.push_back(P.country == "FRA" ? "Coupe de France : règles" : "Coupe nationale : règles"); extra.push_back("Coupes régionales / personnalisées");
        if (!ceInCareer() && pyrEdited(P.country)) extra.push_back("Rétablir le championnat d'origine"); extra.push_back("Retour");
        int n=T+(int)extra.size();
        int s=listRun(g_ceLW2,n,24,56,VW-48,15,17,[&](int i,int x,int y,bool sel){
            if(i>=T){drawTextPx(extra[i-T],x+6,y+3,10,sel?BLACK:C_HI);return;} const TierConf&t=P.tiers[i];int nc=0,np=0;for(auto&pl:P.pools)if(pl.tier==i){nc+=(int)pl.clubs.size();np++;}
            drawTextPx(fmt("%d.",i+1),x+6,y+3,10,sel?BLACK:C_DIM);drawTextPx(fitText(t.name,170,10),x+26,y+3,10,sel?BLACK:C_TXT);drawTextPx(fitText(SCOPE_NAMES[std::max(0,std::min(3,t.scope))],80,10),x+202,y+3,10,sel?BLACK:C_HI);drawTextPx(fmt("%d clubs%s",nc,np>1?fmt(" / %d poules",np).c_str():""),x+290,y+3,10,sel?BLACK:C_DIM);drawTextPx(fmt("%d/%d/%d pts",t.ptsWin,t.ptsDraw,t.ptsLoss),x+430,y+3,10,sel?BLACK:C_TXT);
        });
        drawFooter("OK : règles, points, portée géographique et clubs   Retour");
        if(s>=0&&s<T){g_ceTier=s;g_cePool=-1;g_ceMode=2;g_ceLW=ListW();return;}
        if(s>=T){const std::string&e=extra[s-T];if(e[0]=='+'){pyrAddTier(P,(int)P.tiers.size()-1,fmt("Division %d",(int)P.tiers.size()+1));ceChanged(P);toast("Division ajoutée");}
            else if(e.rfind("Régions",0)==0){if(ceInCareer())toast("Géographie modifiable hors carrière");else{g_ceMode=7;g_ceLW=ListW();}}
            else if(e.rfind("Coupe de",0)==0||e.rfind("Coupe nationale",0)==0){g_ceMode=6;g_ceLW=ListW();}
            else if(e.rfind("Coupes régionales",0)==0){if(ceInCareer())toast("Coupes personnalisées modifiables hors carrière");else{g_ceMode=10;g_ceLW=ListW();}}
            else if(e.rfind("Rétablir",0)==0)askConfirm("Rétablir le championnat d'origine ("+P.name+") ?",[](){auto&PY2=cePyrs();if(g_cePyr>=0&&g_cePyr<(int)PY2.size()&&pyrRestore(PY2[g_cePyr])){saveCompEdits();toast("Championnat d'origine rétabli");}},"Rétablir","Annuler");
            else{g_ceMode=0;g_ceLW=ListW();}}
        if(IN.back){g_ceMode=0;g_ceLW=ListW();} return;
    }

    if (g_ceTier < 0 || g_ceTier >= (int)P.tiers.size()) { g_ceMode = 1; return; }
    TierConf& t = P.tiers[g_ceTier]; bool clubsOk=tierClubsEditable(P,g_ceTier)&&!ceInCareer();

    // ---------------- 2 : règles d'une division
    if (g_ceMode == 2) {
        drawTextCentered(fmt("%s - division %d",P.name.c_str(),g_ceTier+1),VW/2,42,10,C_HI,false); int nc=0;for(auto&pl:P.pools)if(pl.tier==g_ceTier)nc+=(int)pl.clubs.size();
        std::vector<std::string> rows={
            "Nom : "+t.name+(g_ceTyping&&g_ceRenameKind==0&&fmod(GetTime(),1.0)<.5?"_":""),
            std::string("Portée : ")+SCOPE_NAMES[std::max(0,std::min(3,t.scope))],
            fmt("Taille cible d'un groupe : %d clubs",t.groupSize),
            fmt("Montées vers la division supérieure : %d",t.up),fmt("Relégations vers la division inférieure : %d",t.down),fmt("Barragistes pour la montée : %d",t.barrageUp),
            fmt("Points victoire : %d",t.ptsWin),fmt("Points match nul : %d",t.ptsDraw),fmt("Points défaite : %d",t.ptsLoss),fmt("Points forfait : %d",t.ptsForfeit),
            std::string("Matchs contre chaque adversaire : ")+LEGS_NAMES[std::max(1,std::min(4,t.legs))],std::string("Départage : ")+TB_NAMES[std::max(0,std::min(5,t.tb))],std::string("Équipes réserves : ")+(t.noReserves?"interdites":"autorisées"),
            clubsOk?fmt("Clubs de la division (%d) : gérer",nc):fmt("Clubs de la division (%d) : non modifiables",nc)};
        if(structOk){rows.push_back("Insérer une nouvelle division en dessous");rows.push_back("Supprimer cette division");} rows.push_back("Retour");
        if(g_ceTyping&&g_ceRenameKind==0){textEdit(t.name,40);if(IN.start||IN.back||IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_KP_ENTER)||IsKeyPressed(KEY_ESCAPE)){g_ceTyping=false;g_textMode=false;if(t.name.empty())t.name=fmt("Division %d",g_ceTier+1);ceChanged(P);}}
        int s=listRun(g_ceLW,(int)rows.size(),VW/2-220,54,440,(int)rows.size(),17,[&](int i,int x,int y,bool sel){bool val=i>=1&&i<=12;drawTextPx(fitText(rows[i],420,10),x+8,y+3,10,sel?BLACK:(i==13&&!clubsOk?C_DIM:C_TXT));if(val&&sel)drawTextPx("< >",x+404,y+3,10,BLACK);},!g_ceTyping);
        int r=g_ceLW.cur,d=g_ceTyping?0:(IN.left?-1:IN.right?1:0),T=(int)P.tiers.size(); bool ch=false;
        if(d&&r>=1&&r<=12){switch(r){
            case 1: if(structOk){int ns=t.scope;if(d>0)ns=ns==SC_NATIONAL?SC_REGION:ns==SC_REGION?SC_DEPT:SC_NATIONAL;else ns=ns==SC_NATIONAL?SC_DEPT:ns==SC_DEPT?SC_REGION:SC_NATIONAL;pyrSetTierScope(P,g_ceTier,ns);ch=true;}else toast("Portée géographique fixe pour cette pyramide");break;
            case 2:t.groupSize=std::max(2,std::min(40,t.groupSize+d));pyrRefreshGeo(P);ch=true;break;
            case 3:if(g_ceTier>0){t.up=std::max(0,std::min(10,t.up+d));pyrRelink(P,g_ceTier,false);ch=true;}break;
            case 4:if(g_ceTier+1<T){t.down=std::max(0,std::min(10,t.down+d));pyrRelink(P,g_ceTier,true);ch=true;}break;
            case 5:if(g_ceTier>0){t.barrageUp=std::max(0,std::min(4,t.barrageUp+d));ch=true;}break;
            case 6:t.ptsWin=std::max(-10,std::min(20,t.ptsWin+d));ch=true;break;case 7:t.ptsDraw=std::max(-10,std::min(20,t.ptsDraw+d));ch=true;break;case 8:t.ptsLoss=std::max(-10,std::min(20,t.ptsLoss+d));ch=true;break;case 9:t.ptsForfeit=std::max(-20,std::min(20,t.ptsForfeit+d));ch=true;break;
            case 10:t.legs=std::max(1,std::min(4,t.legs+d));ch=true;break;case 11:t.tb=(t.tb+d+6)%6;ch=true;break;case 12:t.noReserves=!t.noReserves;ch=true;break;} if(ch)ceChanged(P);}
        drawTextCentered("Points configurables : victoire / nul / défaite / forfait. La portée Régional/District crée une poule par zone.",VW/2,VH-44,10,C_DIM,false);
        drawFooter(g_ceTyping?"Tapez le nom   Entrée : valider":"Gauche / Droite : modifier   OK : ouvrir   Retour");
        if(s==0){g_ceTyping=true;g_ceRenameKind=0;g_textMode=true;while(GetCharPressed()>0){}}
        else if(s==12){t.noReserves=!t.noReserves;ceChanged(P);}
        else if(s==13){if(clubsOk){auto pools=cePoolsForTier(P,g_ceTier);if(pools.size()>1){g_ceMode=13;g_ceLW=ListW();}else if(!pools.empty()){g_cePool=pools[0];g_ceMode=3;g_ceLW2=ListW();}}else toast("Clubs non modifiables en cours de carrière");}
        else if(s>=14&&s<(int)rows.size()){const std::string&e=rows[s];if(e.rfind("Insérer",0)==0){pyrAddTier(P,g_ceTier,fmt("Division %d",g_ceTier+2));ceChanged(P);g_ceTier++;toast("Division insérée");}else if(e.rfind("Supprimer",0)==0){if(T<=1)toast("Il faut garder au moins une division");else askConfirm("Supprimer "+t.name+" ?",[](){auto&PY2=cePyrs();Pyramid&P2=PY2[g_cePyr];if(pyrDeleteTier(P2,g_ceTier)){ceChanged(P2);toast("Division supprimée");}g_ceMode=1;g_ceTier=-1;},"Supprimer","Annuler");}else g_ceMode=1;}
        if(IN.back&&!g_ceTyping)g_ceMode=1; return;
    }

    // clubs de la division : respecter la poule choisie
    auto pools=cePoolsForTier(P,g_ceTier); if(g_cePool<0||std::find(pools.begin(),pools.end(),g_cePool)==pools.end())g_cePool=pools.empty()?-1:pools[0];
    if(g_cePool<0){g_ceMode=2;return;} std::vector<int> cl=P.pools[g_cePool].clubs;sortTeamsByName(cl);
    if(g_ceMode==3){drawTextCentered(t.name+" - "+cePoolName(P,g_cePool)+fmt(" : %d clubs",(int)cl.size()),VW/2,42,10,C_HI,false);int n=(int)cl.size()+2;int s=listRun(g_ceLW2,n,40,56,VW-80,16,16,[&](int i,int x,int y,bool sel){if(i==0){drawTextPx("+ Ajouter un club d'une autre division",x+6,y+2,10,sel?BLACK:C_HI);return;}if(i==n-1){drawTextPx("Retour",x+6,y+2,10,sel?BLACK:C_HI);return;}const Team&T=g_world.teams[cl[i-1]];drawTextPx(fitText(T.name,300,10),x+6,y+2,10,sel?BLACK:C_TXT);drawTextPx(fmt("note %d",(int)T.rating),x+330,y+2,10,sel?BLACK:C_DIM);});drawFooter("OK : déplacer le club vers une autre division   Retour");if(s==0){g_ceMode=5;g_ceLW=ListW();}else if(s==n-1||IN.back)g_ceMode=pools.size()>1?13:2;else if(s>0){g_ceClub=cl[s-1];g_ceMode=4;g_ceLW=ListW();}return;}
    if(g_ceMode==4){drawTextCentered(g_world.teams[g_ceClub].name+" : nouvelle division",VW/2,42,10,C_HI,false);std::vector<int>dst;for(int k=0;k<(int)P.tiers.size();k++)if(k!=g_ceTier&&tierClubsEditable(P,k))dst.push_back(k);int s=listRun(g_ceLW,(int)dst.size(),VW/2-180,60,360,14,18,[&](int i,int x,int y,bool sel){drawTextPx(fmt("%d. ",dst[i]+1)+P.tiers[dst[i]].name,x+8,y+4,10,sel?BLACK:C_TXT);});drawFooter("OK : déplacer   Retour");if(s>=0){pyrMoveClub(P,g_ceClub,dst[s]);ceChanged(P);toast(g_world.teams[g_ceClub].name+" passe en "+P.tiers[dst[s]].name);g_ceMode=3;}if(IN.back)g_ceMode=3;return;}
    if(g_ceMode==5){drawTextCentered("Ajouter à "+t.name+" / "+cePoolName(P,g_cePool),VW/2,42,10,C_HI,false);std::vector<std::pair<int,int>>v;std::set<int>seen;for(auto&pl:P.pools)if(pl.tier!=g_ceTier)for(int c:pl.clubs)if(!seen.count(c)){v.push_back({c,pl.tier});seen.insert(c);}std::stable_sort(v.begin(),v.end(),[](auto&a,auto&b){if(a.second!=b.second)return a.second<b.second;return sortKey(g_world.teams[a.first].name)<sortKey(g_world.teams[b.first].name);});int s=listRun(g_ceLW,(int)v.size(),40,56,VW-80,16,16,[&](int i,int x,int y,bool sel){drawTextPx(fitText(g_world.teams[v[i].first].name,300,10),x+6,y+2,10,sel?BLACK:C_TXT);drawTextPx(fitText(P.tiers[v[i].second].name,220,10),x+330,y+2,10,sel?BLACK:C_DIM);});drawFooter("OK : ajouter (la région/district du club détermine sa poule)   Retour");if(s>=0){pyrMoveClub(P,v[s].first,g_ceTier);ceChanged(P);toast(g_world.teams[v[s].first].name+" rejoint "+t.name);auto pp=cePoolsForTier(P,g_ceTier);for(int q:pp)if(std::find(P.pools[q].clubs.begin(),P.pools[q].clubs.end(),v[s].first)!=P.pools[q].clubs.end())g_cePool=q;}if(IN.back)g_ceMode=3;return;}
    g_ceMode=1;
}

// règles de la coupe nationale (mode 6) : traité séparément dans app.cpp
static void screenCompEditCup() {
    auto& PY=cePyrs(); if(g_cePyr<0||g_cePyr>=(int)PY.size()){g_ceMode=0;return;} Pyramid&P=PY[g_cePyr]; drawBackground("Éditeur de compétitions"); CupRule r=cupRuleFor(P.country);
    drawTextCentered("Coupe nationale : "+P.name,VW/2,36,10,C_HI,false);
    std::vector<std::string> rows={std::string("Coupe disputée : ")+(r.enabled?"oui":"non"),std::string("Prolongation avant TAB : ")+(r.et?"oui":"non (TAB directs, sauf finale)"),std::string("Finale sur terrain neutre : ")+(r.neutralFinal?"oui":"non"),std::string("Le vainqueur prend la place européenne de la coupe : ")+(r.euroQual?"oui":"non"),r.finalSize?fmt("Clubs en phase finale : %d",r.finalSize):std::string("Clubs en phase finale : tous les clubs éligibles"),"Retour"};
    int s=menuRun(g_ceLW,rows,62,440),cur=g_ceLW.cur,d=IN.left?-1:IN.right?1:0;bool ch=false;
    auto act=[&](int x,int delta){if(x==0){r.enabled=!r.enabled;ch=true;}else if(x==1){r.et=!r.et;ch=true;}else if(x==2){r.neutralFinal=!r.neutralFinal;ch=true;}else if(x==3){r.euroQual=!r.euroQual;ch=true;}else if(x==4){int v=r.finalSize?r.finalSize:64;v+= (delta?delta:1)*2;if(v<2)v=0;if(v>512)v=512;r.finalSize=v;ch=true;}};
    if(d&&cur<=4)act(cur,d);if(s>=0&&s<=4)act(s,1);if(ch){setCupRule(P.country,r);saveCompEdits();}
    drawTextCentered("Les coupes régionales qui qualifient en coupe nationale sont prioritaires dans ce tableau la saison suivante.",VW/2,VH-44,10,C_DIM,false);drawFooter("OK / Gauche / Droite : modifier   Retour");if(s==5||IN.back){g_ceMode=1;g_ceLW2=ListW();}
}
