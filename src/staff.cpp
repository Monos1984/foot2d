// Staff technique, arbitres, équipes réserves du joueur, matchs amicaux
#include "game.h"
#include <cstring>

const char* staffRoleName(int r) {
    static const char* N[NUM_SR] = { "Entraîneur adjoint", "Préparateur physique", "Entraîneur des gardiens", "Recruteur", "Médecin / kiné", "Directeur du centre de formation", "Entraîneur de la réserve" };
    return r >= 0 && r < NUM_SR ? N[r] : "?";
}
const char* staffRoleDesc(int r) {
    static const char* D[NUM_SR] = {
        "Tactique et préparation des matchs : l'équipe joue mieux (matchs simulés et note de l'équipe).",
        "Condition physique : endurance en match, moins de blessures, progression à l'entraînement.",
        "Progression des gardiens à l'intersaison.",
        "Élargit la liste du mercato et révèle le potentiel des joueurs.",
        "Blessures moins longues.",
        "Jeunes du centre de formation plus nombreux et plus talentueux.",
        "Dirige les équipes réserves quand vous ne les contrôlez pas : meilleurs résultats." };
    return r >= 0 && r < NUM_SR ? D[r] : "";
}

int Career::staffLevel(int role) const {
    int best = 0;
    for (auto& s : mgr.staff) if (s.role == role) best = std::max(best, (int)s.level);
    return best;
}

static std::string staffName(Rng& r) {
    static const char* F[] = { "Patrick", "Jean-Marc", "Didier", "Laurent", "Christophe", "Frédéric", "Stéphane", "Olivier", "Bruno", "Philippe", "Éric", "Thierry", "Franck",
                               "Sébastien", "Nicolas", "Karim", "Mohamed", "Julien", "Sylvain", "Pascal", "Yannick", "Rachid", "David", "Hervé", "Cédric", "Joël" };
    static const char* L[] = { "Martin", "Bernard", "Dubois", "Durand", "Lefebvre", "Moreau", "Laurent", "Garnier", "Faure", "Rousseau", "Blanc", "Guérin", "Muller",
                               "Henry", "Roussel", "Nicolas", "Perrin", "Morin", "Mathieu", "Clément", "Gauthier", "Dumont", "Lopez", "Fontaine", "Chevalier", "Robin",
                               "Masson", "Sanchez", "Gérard", "Nguyen", "Boyer", "Denis", "Lemaire", "Duval", "Joly", "Gautier", "Roger", "Roche", "Roy", "Noël", "Benali", "Traoré" };
    return sanitize(F[r.range(0, (int)(sizeof(F) / sizeof(F[0])) - 1)]) + " " + sanitize(L[r.range(0, (int)(sizeof(L) / sizeof(L[0])) - 1)]);
}

// salaire annuel (k€) selon le niveau et le statut du club
int staffWage(int level, int status) {
    static const int BASE[6] = { 0, 20, 45, 90, 180, 350 };
    int w = BASE[std::max(0, std::min(5, level))];
    if (status == CS_SEMIPRO) w = w * 30 / 100;
    if (status == CS_AMATEUR) w = level <= 2 ? 0 : w * 8 / 100;   // bénévoles en amateur
    return w;
}

StaffMember makeStaff(int role, int level, int status, Rng& r) {
    StaffMember s;
    std::string n = staffName(r);
    strncpy(s.name, n.c_str(), sizeof s.name - 1);
    s.role = role; s.level = level; s.age = r.range(32, 64);
    s.wage = staffWage(level, status);
    return s;
}

void Career::initStaff() {
    mgr.staff.clear();
    const Team& U = g_world.teams[userTeam];
    int base = U.status == CS_PRO ? 3 : U.status == CS_SEMIPRO ? 2 : 1;
    Rng r(g_rng.next());
    mgr.staff.push_back(makeStaff(SR_ADJOINT, base, U.status, r));
    if (U.status != CS_AMATEUR) { mgr.staff.push_back(makeStaff(SR_PHYSIO_PREP, base, U.status, r)); mgr.staff.push_back(makeStaff(SR_MEDIC, base, U.status, r)); }
    if (U.status == CS_PRO) { mgr.staff.push_back(makeStaff(SR_GK, base, U.status, r)); mgr.staff.push_back(makeStaff(SR_SCOUT, base - 1, U.status, r)); mgr.staff.push_back(makeStaff(SR_YOUTH, base, U.status, r)); }
    // staff créé avec l'éditeur et rattaché au club
    for (auto& c : customStaff()) {
        if (c.club.empty() || c.club != U.name) continue;
        StaffMember m = makeStaff(c.role, c.level, U.status, r);
        snprintf(m.name, sizeof m.name, "%s", c.name.c_str()); m.age = c.age;
        mgr.staff.erase(std::remove_if(mgr.staff.begin(), mgr.staff.end(), [&](const StaffMember& x) { return x.role == c.role; }), mgr.staff.end());
        mgr.staff.push_back(m);
    }
}

float teamBonus(int team) {
    if (g_career.kind != CK_CLUB || team < 0) return 0;
    if (team == g_career.userTeam) return g_career.staffLevel(SR_ADJOINT) * 0.4f + g_career.staffLevel(SR_PHYSIO_PREP) * 0.25f;
    if (g_world.teams[team].parent == g_career.userTeam) {
        bool ctrl = std::find(g_career.mgr.ctrlReserves.begin(), g_career.mgr.ctrlReserves.end(), team) != g_career.mgr.ctrlReserves.end();
        return ctrl ? 0 : g_career.staffLevel(SR_RESERVE) * 0.6f;
    }
    return 0;
}

int injuryReduction(int team) {
    if (g_career.kind != CK_CLUB || team != g_career.userTeam) return 0;
    return g_career.staffLevel(SR_MEDIC) / 2;
}

// ------------------------------------------------------------------ arbitres (noms fictifs)
const Referee REFEREES[] = {
    { "Olivier Marchand", 72, 45, 60, 80, 5 }, { "Julien Lecomte", 55, 50, 75, 85, 5 }, { "Thomas Vasseur", 85, 40, 40, 70, 5 },
    { "Nicolas Berthier", 48, 55, 80, 75, 5 }, { "Sébastien Carrel", 66, 60, 55, 65, 4 }, { "Mathieu Delorme", 78, 50, 50, 72, 4 },
    { "Romain Aubert", 40, 48, 85, 80, 4 }, { "Alexandre Perrier", 60, 70, 45, 55, 4 }, { "Anthony Lebrun", 90, 52, 35, 60, 4 },
    { "Fabien Collin", 52, 45, 65, 70, 4 }, { "Kévin Masselot", 62, 58, 58, 62, 3 }, { "Yoann Pichon", 70, 65, 50, 50, 3 },
    { "Damien Hérault", 45, 40, 70, 68, 3 }, { "Jérémy Faucher", 80, 55, 45, 58, 3 }, { "Cyril Bonnaud", 58, 72, 52, 48, 3 },
    { "Laurent Tessier", 67, 50, 60, 66, 3 }, { "Arnaud Lemonnier", 50, 62, 66, 57, 2 }, { "Benoît Cazenave", 75, 68, 42, 45, 2 },
    { "Grégory Barre", 42, 50, 72, 60, 2 }, { "Samir Haddad", 64, 47, 61, 63, 2 }, { "Ludovic Pasquier", 88, 60, 38, 50, 2 },
    { "Franck Rigal", 55, 75, 50, 40, 1 }, { "Mickaël Joubert", 60, 66, 55, 45, 1 }, { "Didier Galland", 70, 70, 45, 42, 1 },
    { "Hugo Renard", 47, 52, 68, 55, 1 }, { "Pascal Vidal", 82, 64, 40, 44, 1 }, { "Loïc Morvan", 53, 58, 63, 52, 1 },
};
const int NUM_REFEREES = sizeof(REFEREES) / sizeof(REFEREES[0]);

int refereeFor(int comp, int match) {
    int lvl = 1;
    if (comp >= 0 && comp < (int)g_career.season.comps.size()) {
        const Competition& C = g_career.season.comps[comp];
        int t = C.matches.empty() || match < 0 ? 99 : std::min(teamLevel(C.matches[match].home), teamLevel(C.matches[match].away));
        if (C.kind == 3 || C.kind == 8 || C.kind == 7 || t == 0 || C.kind == 11 || C.kind == 6) lvl = 5;
        else if (t <= 1 || C.name == "Coupe de France") lvl = 4;
        else if (t <= 2) lvl = 3;
        else if (t <= 4) lvl = 2;
    }
    uint32_t h = (uint32_t)(comp * 7919 + match * 104729 + 17);
    h ^= h >> 13; h *= 0x5bd1e995; h ^= h >> 15;
    std::vector<int> c;
    for (int i = 0; i < NUM_REFEREES; i++) if (REFEREES[i].level == lvl || (lvl <= 1 && REFEREES[i].level <= 2)) c.push_back(i);
    if (c.empty()) return (int)(h % NUM_REFEREES);
    return c[h % c.size()];
}

// ------------------------------------------------------------------ équipes réserves du joueur
void placeInBottomPool(Pyramid& P, int team);

int Career::createReserve(std::string& err) {
    if (kind != CK_CLUB) { err = "Disponible en carrière club."; return -1; }
    const Team& U = g_world.teams[userTeam];
    int n = 0;
    for (int i = 0; i < (int)g_world.teams.size(); i++) if (g_world.teams[i].parent == userTeam && !g_world.teams[i].youth) n++;
    if (n >= 3) { err = "Trois équipes réserves maximum."; return -1; }
    Team t;
    bool pro = U.status != CS_AMATEUR;
    static const char* SFX_PRO[] = { " B", " C", " D" };
    static const char* SFX_AMA[] = { " 2", " 3", " 4" };
    t.name = U.name + (pro ? SFX_PRO[n] : SFX_AMA[n]);
    t.shortName = U.shortName.substr(0, std::min<size_t>(8, U.shortName.size())) + (pro ? SFX_PRO[n] : SFX_AMA[n]);
    t.stadium = U.stadium + " (annexe)"; t.town = U.town; t.dept = U.dept; t.region = U.region; t.district = U.district;   // dernière division du district de l'équipe fanion
    t.kind = TK_CLUB; t.nation = U.nation; t.culture = U.culture; t.home = U.home; t.away = U.away;
    t.rating = std::max(8.f, U.rating - 12.f - 4.f * n);
    t.parent = userTeam; t.resLevel = n + 1; t.status = CS_AMATEUR; t.founded = year;
    t.seed = hashStr(t.name) ^ (uint32_t)(year * 31 + n);
    int idx = (int)g_world.teams.size();
    g_world.teams.push_back(t);
    initStadium(g_world.teams[idx], 12, -1);
    pendingNewClubs.push_back(idx);
    season.news.push_back(g_world.teams[idx].name + " est créée : elle débutera la saison prochaine dans la dernière division du district.");
    return idx;
}

void Career::syncControlled() {
    std::vector<int> c = { userTeam };
    for (int t : mgr.ctrlReserves) if (t >= 0 && t < (int)g_world.teams.size() && g_world.teams[t].parent == userTeam) c.push_back(t);
    season.controlled = c;
}

// ------------------------------------------------------------------ matchs amicaux et tournoi amical
void Career::addFriendly(const std::vector<int>& invited, bool tournament) {
    Competition c;
    c.kind = 12; c.shortName = "Amical"; c.neutralFinal = false;
    double t0 = season.now + 0.02;
    if (!tournament || invited.size() < 3) {
        c.format = FMT_SINGLE; c.name = "Match amical : " + g_world.teams[userTeam].name + " - " + g_world.teams[invited[0]].name;
        c.addKOStage({ { userTeam, invited[0] } }, 1, t0, "Match amical", false);
        c.matches[0].decisive = 0; c.matches[0].noET = 1;
    } else {
        c.format = FMT_CUP; c.name = fmt("Tournoi amical de %s", g_world.teams[userTeam].town.empty() ? g_world.teams[userTeam].name.c_str() : g_world.teams[userTeam].town.c_str());
        std::vector<int> t = { userTeam, invited[0], invited[1], invited[2] };
        c.entrants = { t };
        c.koTargets = { 2, 1 }; c.koNames = { "Demi-finales", "Finale" }; c.koTimes = { t0, t0 + 0.01 };
        c.neutralFinal = false;
        season.comps.push_back(c);
        Competition& C = season.comps.back();
        C.cupRound(0, C.entrants[0]);
        for (auto& m : C.matches) m.noET = 1;
        season.news.push_back(C.name + " organisé.");
        return;
    }
    season.comps.push_back(c);
    season.news.push_back(c.name + " programmé.");
}
