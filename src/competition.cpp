// Moteur de compétitions : championnats (règles de départage), coupes, phases de groupes,
// Ligue des champions / Coupe UEFA format 2000-2001, barrages
#include "game.h"
#include "crashlog.h"
#include <functional>
#include <cstring>
#include <map>
#include <set>

float teamStrength(int team, RuleProfile rules) { return g_world.teams[team].rating + teamBonus(team) + formFactor(team,rules); }

const char* tieBreakText(int tb) {
    switch (tb) {
    case TB_LFP: return "Départage LFP : différence de buts générale, puis points et différence de buts particuliers, buts marqués, victoires.";
    case TB_FFF: return "Départage FFF : points des matchs particuliers, différence de buts particulière, puis différence de buts générale et buts marqués.";
    case TB_H2H: return "Départage : confrontations directes (points puis différence de buts), puis différence de buts générale et buts marqués.";
    case TB_ENG: return "Départage : différence de buts, buts marqués, puis confrontations directes.";
    case TB_FIFA: return "Départage : différence de buts, buts marqués, puis confrontations directes.";
    default: return "Départage : différence de buts, puis buts marqués.";
    }
}

void sortStandings(std::vector<Standing>& v) {
    std::stable_sort(v.begin(), v.end(), [](const Standing& a, const Standing& b) {
        if (a.pts != b.pts) return a.pts > b.pts;
        if (a.gd() != b.gd()) return a.gd() > b.gd();
        if (a.gf != b.gf) return a.gf > b.gf;
        return g_world.teams[a.team].rating > g_world.teams[b.team].rating;
    });
}

static int poisson(Rng& r, double lam) {
    double L = std::exp(-lam), p = 1; int k = 0;
    do { k++; p *= r.f(); } while (p > L && k < 15);
    return k - 1;
}

static void penalties(MatchRes& m, Rng& r) {
    int a = 0, b = 0;
    for (int i = 0; i < 5; i++) { if (r.chance(0.76f)) a++; if (r.chance(0.76f)) b++; }
    while (a == b) { bool x = r.chance(0.75f), y = r.chance(0.75f); a += x; b += y; }
    m.ph = (int16_t)a; m.pa = (int16_t)b;
}

static bool firstLegScore(const Competition* c, const MatchRes& m, int& aggHome, int& aggAway) {
    aggHome = aggAway = 0;
    if (!c || m.leg != 1 || m.tie < 0) return false;
    int si = c->stageOfMatch((int)(&m - &c->matches[0]));
    if (si < 0) return false;
    const Stage& st = c->stages[si];
    if (m.tie >= (int)st.ties.size()) return false;
    const Tie& t = st.ties[m.tie];
    if (t.m1 < 0) return false;
    const MatchRes& a = c->matches[t.m1];
    if (!a.played) return false;
    aggHome = a.ag; aggAway = a.hg;
    return true;
}

void simulateMatch(MatchRes& m, const Competition* c) {
    if(g_career.sportingMode()){g_career.sportingPrepare(m.home);g_career.sportingPrepare(m.away);}
    Rng& r = g_rng;
    RuleProfile rules=c&&careerRules()?RULESET_CAREER:RULESET_SIMPLE;
    double rh = teamStrength(m.home,rules), ra = teamStrength(m.away,rules);
    if(rules==RULESET_CAREER){rh*=personalityTeamMultiplier(g_career,m.home,m,c);ra*=personalityTeamMultiplier(g_career,m.away,m,c);}
    double d = rh - ra + (m.neutral ? 0 : 4.0) + lifeBribeDelta(m);
    // écart de niveau « tassé » : un gros écart donne une large victoire, rarement un score fleuve (les favoris gèrent)
    double dd = 30.0 * std::tanh(d / 30.0);
    double lh = 1.38 * std::exp(dd / 22.0), la = 1.13 * std::exp(-dd / 22.0);
    if(rules==RULESET_CAREER){
     auto attacking=[&](int team){const auto& T=g_world.teams[team];float risk=0;for(int s=1;s<11;s++){int dp=teamSlotPosition(T,T.formation,s);auto tac=T.tactical.customized?T.tactical.slot[s]:defaultSlotTactic(dp,T.seed%2,s);auto e=roleEffects(dp,tac);risk+=e.advance+(.15f*(e.shot-1))+.05f*(e.risk-1);}return std::clamp(risk/10.f,-.12f,.18f);};
     float home=attacking(m.home),away=attacking(m.away);lh*=1+home+.45f*away;la*=1+away+.45f*home;
    }
    lh = std::max(0.12, std::min(lh, 4.2)); la = std::max(0.12, std::min(la, 4.2));
    if(c && c->kind==150){lh*=40.0/90;la*=40.0/90;}
    m.hg = (int16_t)poisson(r, lh);
    m.ag = (int16_t)poisson(r, la);
    if (m.hg > 6) m.hg = (int16_t)(6 + (m.hg - 6) / 3);
    if (m.ag > 6) m.ag = (int16_t)(6 + (m.ag - 6) / 3);
    m.aet = false; m.ph = m.pa = -1;
    if (m.decisive) {
        int fh = 0, fa = 0;
        bool leg2 = firstLegScore(c, m, fh, fa);
        bool ag = leg2 && c && c->awayGoals;
        // fh = buts marqués à l'extérieur par l'équipe qui reçoit ce match retour
        auto level = [&]() { return m.hg + fh == m.ag + fa && (!ag || m.ag == fh); };
        if (level()) {
            if (!m.noET) {
                m.hg += (int16_t)poisson(r, lh / 3); m.ag += (int16_t)poisson(r, la / 3);
                m.aet = true;
            }
            if (level()) penalties(m, r);
        }
    }
    m.played = true;
}

// ------------------------------------------------------------------ calendriers
// calendrier « à la Berger » : aucune équipe ne joue plus de 2 matchs de suite à domicile ou à l'extérieur
static std::vector<std::vector<std::pair<int, int>>> roundRobin(std::vector<int> teams, int legs, Rng& r) {
    r.shuffle(teams);
    if (teams.size() % 2) teams.push_back(-1);
    int n = (int)teams.size(), m = n - 1;
    std::vector<std::vector<std::pair<int, int>>> rounds;
    for (int rd = 0; rd < m; rd++) {
        std::vector<std::pair<int, int>> ms;
        auto add = [&](int h, int a) { if (teams[h] >= 0 && teams[a] >= 0) ms.push_back({ teams[h], teams[a] }); };
        if (rd % 2 == 0) add(n - 1, rd); else add(rd, n - 1);
        for (int k = 1; k < n / 2; k++) {
            int a = (rd + k) % m, b = (rd - k + m) % m;
            if (k % 2 == 1) add(b, a); else add(a, b);
        }
        rounds.push_back(ms);
    }
    if (legs >= 2) {
        // matchs retour : ordre décalé d'une journée pour éviter trois matchs de suite au même endroit ;
        // 3 ou 4 confrontations (éditeur de compétitions) : cycles supplémentaires, domicile alterné
        int R = (int)rounds.size();
        for (int cyc = 1; cyc < std::min(4, legs); cyc++)
            for (int i = 0; i < R; i++) {
                std::vector<std::pair<int, int>> ms;
                for (auto& p : rounds[(i + cyc) % R]) ms.push_back(cyc % 2 ? std::make_pair(p.second, p.first) : p);
                rounds.push_back(ms);
            }
    }
    return rounds;
}

void Competition::setupLeague(const std::vector<int>& teams, int lg, double a, double b, int numGroups) {
    legs = lg; t0 = a; t1 = b;
    Stage st; st.type = ST_LEAGUE; st.legs = lg; st.name = numGroups > 1 ? "Phase de groupes" : "Championnat";
    st.groups.assign(numGroups, {});
    for (int i = 0; i < (int)teams.size(); i++) st.groups[i % numGroups].push_back(teams[i]);
    int maxR = 0;
    std::vector<std::vector<std::vector<std::pair<int, int>>>> sched;
    for (int g = 0; g < numGroups; g++) {
        sched.push_back(roundRobin(st.groups[g], lg, g_rng));
        maxR = std::max(maxR, (int)sched.back().size());
    }
    for (int rd = 0; rd < maxR; rd++) {
        Round R;
        R.time = maxR > 1 ? a + (b - a) * rd / (maxR - 1) : a;
        R.name = fmt("Journée %d", rd + 1);
        for (int g = 0; g < numGroups; g++) {
            if (rd >= (int)sched[g].size()) continue;
            for (auto& p : sched[g][rd]) {
                MatchRes m; m.home = p.first; m.away = p.second; m.group = (int16_t)g;
                matches.push_back(m);
                R.m.push_back((int)matches.size() - 1);
            }
        }
        st.rounds.push_back(R);
    }
    stages.push_back(st);
}

void Competition::addGroupStage(const std::vector<std::vector<int>>& groups, int lg, const std::vector<double>& times, const std::string& nm) {
    Stage st; st.type = ST_LEAGUE; st.legs = lg; st.name = nm; st.groups = groups;
    std::vector<std::vector<std::vector<std::pair<int, int>>>> sched;
    int maxR = 0;
    for (auto& g : groups) { sched.push_back(roundRobin(g, lg, g_rng)); maxR = std::max(maxR, (int)sched.back().size()); }
    for (int rd = 0; rd < maxR; rd++) {
        Round R;
        R.time = times.empty() ? rd : times[std::min(rd, (int)times.size() - 1)] + (rd >= (int)times.size() ? rd - (int)times.size() + 1 : 0);
        R.name = nm + fmt(" - J%d", rd + 1);
        if(kind==50 && tag>=0 && tag<22) R.time=legendGroupTime(tag,(int)stages.size(),rd,maxR);
        for (int g = 0; g < (int)groups.size(); g++) {
            if (rd >= (int)sched[g].size()) continue;
            for (auto& p : sched[g][rd]) {
                MatchRes m; m.home = p.first; m.away = p.second; m.group = (int16_t)g;
                if (format == FMT_TOURNAMENT && kind != 21 && kind != 42 && kind != KIND_CONTINENTS && !isContinentalKind(kind)) {
                    m.neutral = true;
                    if (host >= 0 && (m.away == host)) std::swap(m.home, m.away);
                    if (host >= 0 && m.home == host) m.neutral = false;
                }
                matches.push_back(m);
                R.m.push_back((int)matches.size() - 1);
            }
        }
        st.rounds.push_back(R);
    }
    stages.push_back(st);
}

void Competition::setupSwiss(const std::vector<int>& teams, int nrounds, const std::vector<double>& times) {
    Stage st; st.type = ST_SWISS; st.name = "Phase de ligue"; st.groups.push_back(teams);
    int n = (int)teams.size();
    std::set<std::pair<int, int>> met;
    std::map<int, int> homes;
    for (int rd = 0; rd < nrounds; rd++) {
        std::vector<std::pair<int, int>> pairs;
        bool ok = false;
        for (int attempt = 0; attempt < 400 && !ok; attempt++) {
            bool relax = attempt > 200;
            std::vector<int> order = teams;
            g_rng.shuffle(order);
            std::vector<bool> used(n, false);
            pairs.clear();
            ok = true;
            for (int i = 0; i < n; i++) {
                if (used[i]) continue;
                int a = order[i]; int found = -1;
                for (int j = i + 1; j < n; j++) {
                    if (used[j]) continue;
                    int b = order[j];
                    if (met.count({ std::min(a, b), std::max(a, b) })) continue;
                    if (!relax && g_world.teams[a].nation == g_world.teams[b].nation) continue;
                    found = j; break;
                }
                if (found < 0) { ok = false; break; }
                used[i] = used[found] = true;
                pairs.push_back({ a, order[found] });
            }
        }
        Round R; R.time = times[std::min(rd, (int)times.size() - 1)]; R.name = fmt("Journée %d", rd + 1);
        for (auto& p : pairs) {
            int a = p.first, b = p.second;
            met.insert({ std::min(a, b), std::max(a, b) });
            if (homes[a] > homes[b] || (homes[a] == homes[b] && g_rng.chance(0.5f))) std::swap(a, b);
            homes[a]++;
            MatchRes m; m.home = a; m.away = b; m.group = 0;
            matches.push_back(m);
            R.m.push_back((int)matches.size() - 1);
        }
        st.rounds.push_back(R);
    }
    stages.push_back(st);
}

Stage& Competition::addKOStage(const std::vector<std::pair<int, int>>& pairs, int lg, double time, const std::string& nm, bool neutralFinal) {
    if(kind==50 && tag>=0 && tag<22) time=legendRoundTime(tag,(int)pairs.size()*2,nm);
    Stage st; st.type = ST_KO; st.legs = lg; st.name = nm;
    Round r1; r1.time = time; r1.name = lg == 2 ? nm + " (aller)" : nm;
    double gap = (format == FMT_UCL2000 || format == FMT_UEFA2000) ? 2.0 : format == FMT_NEWEURO ? 1.0 : format == FMT_INTERTOTO ? 1.0 : 0.5;   // coupes d'Europe : retour 2 semaines plus tard
    Round r2; r2.time = time + gap; r2.name = nm + " (retour)";
    for (auto& p : pairs) {
        Tie t; t.a = p.first; t.b = p.second;
        if (t.b < 0) { t.winner = t.a; st.ties.push_back(t); continue; }
        MatchRes m; m.home = t.a; m.away = t.b; m.tie = (int16_t)st.ties.size();
        if(marneCupPart(*this)>=0 || kind==150) m.noET=1;
        m.neutral = neutralFinal || (format == FMT_TOURNAMENT && kind != 21 && kind != 42 && !isContinentalKind(kind));
        if (format == FMT_SINGLE || (penaltiesOnly() && (!neutralFinal || kind == 26 || kind == 27))) { m.noET = 1; }
        if (kind == 72 && nm != "Finale") {
            if (EditorCupDef* d = editorCupById(tag)) if (!d->et) m.noET = 1;
        }
        if (kind == 2 && nm != "Finale" && g_career.kind == CK_CLUB) {     // prolongation : option Coupe de France / règle de la coupe nationale (éditeur)
            bool et = g_career.opts.cdfET;
            int self = (int)(this - g_career.season.comps.data());
            if (self >= 0 && self < (int)g_career.season.comps.size() && std::find(g_career.nationalCups.begin(), g_career.nationalCups.end(), self) != g_career.nationalCups.end()
                && tag >= 0 && tag < (int)g_career.pyramids.size()) et = cupRuleFor(g_career.pyramids[tag].country).et;
            if (!et) m.noET = 1;
        }   // option : Coupe de France sans prolongation (sauf la finale)   // supercoupes de région et Méga Coupe : TAB directs, finale comprise
        if (format == FMT_TOURNAMENT && host >= 0 && (t.a == host || t.b == host)) {
            m.neutral = false; if (t.b == host) { m.home = t.b; m.away = t.a; }
        }
        if (lg == 1) {
            m.decisive = 1;
            matches.push_back(m); t.m1 = (int)matches.size() - 1; r1.m.push_back(t.m1);
        } else {
            m.leg = 0; matches.push_back(m); t.m1 = (int)matches.size() - 1; r1.m.push_back(t.m1);
            MatchRes m2; m2.home = t.b; m2.away = t.a; m2.tie = m.tie; m2.leg = 1; m2.decisive = 1;
            matches.push_back(m2); t.m2 = (int)matches.size() - 1; r2.m.push_back(t.m2);
        }
        st.ties.push_back(t);
    }
    // un tour sans aucun match (que des exemptés) garde une date
    st.rounds.push_back(r1);
    if (lg == 2) st.rounds.push_back(r2);
    stages.push_back(st);
    return stages.back();
}

// ------------------------------------------------------------------ classements (règles de départage)
std::vector<Standing> Competition::table(int s, int g) const {
    std::vector<Standing> v;
    if (s < 0 || s >= (int)stages.size()) return v;
    const Stage& st = stages[s];
    if (g < 0 || g >= (int)st.groups.size()) return v;
    std::map<int, int> idx;
    for (int t : st.groups[g]) { Standing x; x.team = t; idx[t] = (int)v.size(); v.push_back(x); }
    std::vector<const MatchRes*> ms;
    for (auto& R : st.rounds) for (int mi : R.m) {
        const MatchRes& m = matches[mi];
        if (!m.played || (st.type == ST_LEAGUE && m.group != g)) continue;
        auto ih = idx.find(m.home), ia = idx.find(m.away);
        if (ih == idx.end() || ia == idx.end()) continue;
        ms.push_back(&m);
        Standing& H = v[ih->second]; Standing& A = v[ia->second];
        H.p++; A.p++; H.gf += m.hg; H.ga += m.ag; A.gf += m.ag; A.ga += m.hg;
        if (m.hg > m.ag) { H.w++; A.l++; H.pts += ptsWin; A.pts += ptsLoss; }
        else if (m.hg < m.ag) { A.w++; H.l++; A.pts += ptsWin; H.pts += ptsLoss; A.aw++; }
        else if (kind == 42 && m.ph >= 0 && m.pa >= 0) { H.d++; A.d++; if (m.ph > m.pa) { H.pts += 2; A.pts++; } else { A.pts += 2; H.pts++; } }   // Coupe LFFP : tirs au but directs
        else { H.d++; A.d++; H.pts += ptsDraw; A.pts += ptsDraw; }
    }
    // mini-classement entre équipes à égalité
    auto h2h = [&](const std::vector<int>& teams, std::map<int, std::pair<int, int>>& out) {
        std::set<int> T(teams.begin(), teams.end());
        for (int t : teams) out[t] = { 0, 0 };
        for (auto m : ms) {
            if (!T.count(m->home) || !T.count(m->away)) continue;
            auto& H = out[m->home]; auto& A = out[m->away];
            H.second += m->hg - m->ag; A.second += m->ag - m->hg;
            if (m->hg > m->ag) { H.first += ptsWin; A.first += ptsLoss; }
            else if (m->hg < m->ag) { A.first += ptsWin; H.first += ptsLoss; }
            else if (kind == 42 && m->ph >= 0 && m->pa >= 0) { if (m->ph > m->pa) { H.first += 2; A.first += 1; } else { A.first += 2; H.first += 1; } }
            else { H.first += ptsDraw; A.first += ptsDraw; }
        }
    };
    std::stable_sort(v.begin(), v.end(), [](const Standing& a, const Standing& b) { return a.pts > b.pts; });
    // départage par blocs de points égaux
    for (size_t i = 0; i < v.size();) {
        size_t j = i;
        while (j < v.size() && v[j].pts == v[i].pts) j++;
        if (j - i > 1) {
            std::vector<int> tied; for (size_t k = i; k < j; k++) tied.push_back(v[k].team);
            std::map<int, std::pair<int, int>> hh; h2h(tied, hh);
            int rule = tb;
            std::stable_sort(v.begin() + i, v.begin() + j, [&](const Standing& a, const Standing& b) {
                auto ha = hh[a.team], hb = hh[b.team];
                switch (rule) {
                case TB_LFP:
                    if (a.gd() != b.gd()) return a.gd() > b.gd();
                    if (ha.first != hb.first) return ha.first > hb.first;
                    if (ha.second != hb.second) return ha.second > hb.second;
                    if (a.gf != b.gf) return a.gf > b.gf;
                    if (a.w != b.w) return a.w > b.w;
                    return a.aw > b.aw;
                case TB_FFF: case TB_H2H:
                    if (ha.first != hb.first) return ha.first > hb.first;
                    if (ha.second != hb.second) return ha.second > hb.second;
                    if (a.gd() != b.gd()) return a.gd() > b.gd();
                    return a.gf > b.gf;
                case TB_ENG: case TB_FIFA:
                    if (a.gd() != b.gd()) return a.gd() > b.gd();
                    if (a.gf != b.gf) return a.gf > b.gf;
                    return ha.first > hb.first;
                default:
                    if (a.gd() != b.gd()) return a.gd() > b.gd();
                    return a.gf > b.gf;
                }
            });
        }
        i = j;
    }
    return v;
}

std::vector<Standing> Competition::swissTable(int s) const { return table(s, 0); }

bool Competition::stageComplete(int s) const {
    for (auto& R : stages[s].rounds) if (!R.done) return false;
    return true;
}

int Competition::tieWinner(const Tie& t) const {
    if (t.b < 0) return t.a;
    if (t.m2 < 0) {
        const MatchRes& m = matches[t.m1];
        if (m.hg != m.ag) return m.hg > m.ag ? m.home : m.away;
        return m.ph > m.pa ? m.home : m.away;
    }
    const MatchRes& a = matches[t.m1]; const MatchRes& b = matches[t.m2];
    int ga = a.hg + b.ag, gb = a.ag + b.hg;
    if (ga != gb) return ga > gb ? t.a : t.b;
    if (awayGoals && b.ag != a.ag) return b.ag > a.ag ? b.away : a.away;   // buts à l'extérieur
    return b.ph > b.pa ? b.home : b.away;
}

const char* Competition::roundName(int s, int r) const { return stages[s].rounds[r].name.c_str(); }

std::string koName(int nteams) {
    switch (nteams) {
    case 2: return "Finale";
    case 4: return "Demi-finales";
    case 8: return "Quarts de finale";
    case 16: return "Huitièmes de finale";
    case 32: return "Seizièmes de finale";
    case 64: return "32es de finale";
    default: return fmt("Tour à %d", nteams);
    }
}

static std::vector<std::pair<int, int>> bracketPairs(const std::vector<int>& w) {
    std::vector<std::pair<int, int>> p;
    for (size_t i = 0; i + 1 < w.size(); i += 2) p.push_back({ w[i], w[i + 1] });
    if (w.size() % 2) p.push_back({ w.back(), -1 });
    return p;
}


// ------------------------------------------------------------------ tableaux officiels des phases finales (pas de tirage après les groupes)
// Slot : position (1, 2 ou 3) et groupe ; pour un 3e, ensemble des groupes possibles.
struct BSlot { int p1; char g1; int p2; char g2; const char* thirds; };
// Euro 2016 (6 groupes + 4 meilleurs 3es) dans l'ordre du tableau
static const BSlot BR6[8] = { {2,'A',2,'C',0}, {1,'B',3,0,"ACD"}, {1,'D',3,0,"BEF"}, {1,'E',2,'D',0},
                              {1,'C',3,0,"ABF"}, {1,'A',3,0,"CDE"}, {1,'F',2,'E',0}, {2,'B',2,'F',0} };
// Coupe du monde 1998-2022 (8 groupes)
static const BSlot BR8[8] = { {1,'A',2,'B',0}, {1,'C',2,'D',0}, {1,'E',2,'F',0}, {1,'G',2,'H',0},
                              {1,'B',2,'A',0}, {1,'D',2,'C',0}, {1,'F',2,'E',0}, {1,'H',2,'G',0} };
// Coupe du monde 2026 (12 groupes + 8 meilleurs 3es) : matchs 74,77,75,76,81,85,83,86,80,87,79,82,84,88,73,78
static const BSlot BR12[16] = { {1,'E',3,0,"ABCDF"}, {1,'I',3,0,"CDFGH"}, {1,'F',2,'C',0}, {1,'C',2,'F',0},
                                {1,'D',3,0,"BEFIJ"}, {1,'B',3,0,"EFGIJ"}, {2,'K',2,'L',0}, {1,'J',2,'H',0},
                                {1,'L',3,0,"EHIJK"}, {1,'K',3,0,"DEIJL"}, {1,'A',3,0,"CEFHI"}, {1,'G',3,0,"AEHIJ"},
                                {1,'H',2,'J',0}, {2,'D',2,'G',0}, {2,'A',2,'B',0}, {2,'E',2,'I',0} };
// Euro 1996-2012 / Copa América (4 groupes) et 2 groupes
static const BSlot BR4[4] = { {1,'A',2,'B',0}, {1,'C',2,'D',0}, {1,'B',2,'A',0}, {1,'D',2,'C',0} };
static const BSlot BR2[2] = { {1,'A',2,'B',0}, {1,'B',2,'A',0} };

static bool officialBracket(const Competition& C, int cur, std::vector<std::pair<int, int>>& out) {
    const Stage& st = C.stages[cur];
    int ng = (int)st.groups.size();
    const BSlot* B = nullptr; int nb = 0;
    if (ng == 2 && C.bestThirds == 0) { B = BR2; nb = 2; }
    else if (ng == 4 && C.bestThirds == 0) { B = BR4; nb = 4; }
    else if (ng == 6 && C.bestThirds == 4) { B = BR6; nb = 8; }
    else if (ng == 8 && C.bestThirds == 0) { B = BR8; nb = 8; }
    else if (ng == 12 && C.bestThirds == 8) { B = BR12; nb = 16; }
    if (!B) return false;
    std::vector<std::vector<Standing>> tb(ng);
    for (int g = 0; g < ng; g++) { tb[g] = C.table(cur, g); if (tb[g].size() < 3) return false; }
    // meilleurs troisièmes
    std::vector<Standing> thirds;
    for (int g = 0; g < ng; g++) thirds.push_back(tb[g][2]);
    sortStandings(thirds);
    std::vector<int> qg;   // groupes des 3es qualifiés
    for (int i = 0; i < C.bestThirds && i < (int)thirds.size(); i++)
        for (int g = 0; g < ng; g++) if (tb[g][2].team == thirds[i].team) qg.push_back(g);
    // affectation des 3es aux créneaux (contraintes du règlement)
    std::vector<int> slots; for (int i = 0; i < nb; i++) if (B[i].thirds) slots.push_back(i);
    std::vector<int> asg(nb, -1);
    std::vector<bool> used(qg.size(), false);
    std::function<bool(int)> solve = [&](int k) {
        if (k == (int)slots.size()) return true;
        const char* allowed = B[slots[k]].thirds;
        for (int j = 0; j < (int)qg.size(); j++) {
            if (used[j] || !strchr(allowed, 'A' + qg[j])) continue;
            used[j] = true; asg[slots[k]] = qg[j];
            if (solve(k + 1)) return true;
            used[j] = false;
        }
        return false;
    };
    if (!solve(0)) {   // repli : dans l'ordre
        int j = 0; for (int sl : slots) asg[sl] = j < (int)qg.size() ? qg[j++] : 0;
    }
    out.clear();
    for (int i = 0; i < nb; i++) {
        int a = tb[B[i].g1 - 'A'][B[i].p1 - 1].team;
        int b = B[i].thirds ? tb[asg[i]][2].team : tb[B[i].g2 - 'A'][B[i].p2 - 1].team;
        out.push_back({ a, b });
    }
    return true;
}

// tour à élimination directe tiré au sort : têtes de série contre non têtes de série, exemptions si impair
float clubSeed(int team) {
    const Team& T = g_world.teams[team];
    float c = T.coefTotal(), nat = 0;
    // coefficient sur 5 ans : points du club + 33 % du coefficient de son association
    if (T.nation >= 0) { const char* code = NATIONS[T.nation].code; for (auto& u : g_career.uefa) if (u.code == code) nat = u.total() * 0.33f; }
    return c + nat + T.rating * 0.001f;
}

void Competition::koRound(const std::vector<int>& teams, int lg, double time, const std::string& nm, bool fin) {
    crashMark("tirage au sort : %s - %s (%d équipes)", name.c_str(), nm.c_str(), (int)teams.size());
    std::vector<int> s = teams;
    // tirage intégral (sans tête de série, sans restriction de pays) : C1 dès les quarts, C3 dès les 8es
    bool freeDraw = (format == FMT_UCL2000 || format == FMT_UEFA2000 || format == FMT_INTERTOTO) &&
                    (nm.find("Quarts") != std::string::npos || nm.find("Demi") != std::string::npos || nm.find("Finale") != std::string::npos ||
                     (format == FMT_UEFA2000 && nm.find("8es") != std::string::npos));
    if (freeDraw) {
        g_rng.shuffle(s);
        std::vector<std::pair<int, int>> pr;
        if (s.size() % 2) { pr.push_back({ s.back(), -1 }); s.pop_back(); }
        for (size_t i = 0; i + 1 < s.size(); i += 2) pr.push_back({ s[i], s[i + 1] });
        cur = (int)stages.size();
        addKOStage(pr, lg, time, nm, fin);
        return;
    }
    if (format == FMT_UCL2000 || format == FMT_UEFA2000 || format == FMT_EUROPE)
    {
        // Coupe UEFA, 3e tour : les 8 repêchés de la Ligue des champions sont obligatoirement têtes de série
        std::set<int> forced;
        if (format == FMT_UEFA2000 && (nm == "3e tour" || nm == "16es de finale") && entrants.size() > 3) forced.insert(entrants[3].begin(), entrants[3].end());
        std::stable_sort(s.begin(), s.end(), [&forced](int a, int b) {
            bool fa = forced.count(a) > 0, fb = forced.count(b) > 0;
            if (fa != fb) return fa;
            return clubSeed(a) > clubSeed(b); });
    }
    else std::stable_sort(s.begin(), s.end(), [](int a, int b) { return g_world.teams[a].rating > g_world.teams[b].rating; });
    std::vector<std::pair<int, int>> pairs;
    if (s.size() % 2) { pairs.push_back({ s[0], -1 }); s.erase(s.begin()); }
    int n = (int)s.size();
    std::vector<int> top(s.begin(), s.begin() + n / 2), bot(s.begin() + n / 2, s.end());
    g_rng.shuffle(top); g_rng.shuffle(bot);
    // éviter deux clubs d'un même pays
    // (échanges faits avant de former les paires : un échange avec une paire déjà formée dupliquait un club)
    for (int i = 0; i < n / 2; i++) {
        if (g_world.teams[top[i]].nation == g_world.teams[bot[i]].nation) {
            for (int j = 0; j < n / 2; j++)
                if (j != i && g_world.teams[top[i]].nation != g_world.teams[bot[j]].nation && g_world.teams[top[j]].nation != g_world.teams[bot[i]].nation) { std::swap(bot[i], bot[j]); break; }
        }
    }
    for (int i = 0; i < n / 2; i++) {
        // la tête de série reçoit au retour
        if (lg == 2) pairs.push_back({ bot[i], top[i] }); else pairs.push_back(g_rng.chance(0.5f) ? std::make_pair(top[i], bot[i]) : std::make_pair(bot[i], top[i]));
    }
    cur = (int)stages.size();
    addKOStage(pairs, lg, time, nm, fin);
}

void Competition::resume() {
    // relance un tour en attente d'entrées extérieures
    if (awaiting < 0) return;
    int k = awaiting;
    awaiting = -1;
    if (format == FMT_NEWEURO) { std::vector<int> pool = carry; carry.clear(); newEuroStart(*this, k, pool); return; }
    std::vector<int> pool = carry;
    if (k < (int)entrants.size()) for (int e : entrants[k]) pool.push_back(e);
    carry.clear();
    int lg = k < (int)koLegs.size() ? koLegs[k] : 2;
    std::string nm = k < (int)koNames.size() ? koNames[k] : koName((int)pool.size());
    koRound(pool, lg, k < (int)koTimes.size() ? koTimes[k] : 30, nm, false);
}

// groupes de 4 par chapeaux, en évitant deux clubs du même pays
static std::vector<std::vector<int>> drawGroups(std::vector<int> teams, int ng, int holder = -1) {
    // chapeaux : tenant du titre puis coefficient des clubs
    std::stable_sort(teams.begin(), teams.end(), [holder](int a, int b) {
        if ((a == holder) != (b == holder)) return a == holder;
        return clubSeed(a) > clubSeed(b); });
    std::vector<std::vector<int>> g(ng);
    int per = (int)teams.size() / ng;
    for (int pot = 0; pot < per; pot++) {
        std::vector<int> p(teams.begin() + pot * ng, teams.begin() + std::min((int)teams.size(), (pot + 1) * ng));
        for (int attempt = 0; attempt < 200; attempt++) {
            g_rng.shuffle(p);
            bool ok = true;
            for (int i = 0; i < (int)p.size() && ok; i++)
                for (int t : g[i]) if (g_world.teams[t].nation == g_world.teams[p[i]].nation) { ok = false; break; }
            if (ok || attempt == 199) break;
        }
        for (int i = 0; i < (int)p.size(); i++) g[i].push_back(p[i]);
    }
    return g;
}

// tirage de la Ligue des champions : pas deux clubs d'un même pays dans un groupe ; deux clubs d'un même pays sont
// répartis entre les groupes A-D et E-H (3 clubs : règle des deux premiers, le 3e au hasard ; 4 clubs : 2 et 2)
static std::vector<std::vector<int>> drawGroupsUCL(std::vector<int> teams, int ng, int holder) {
    if (ng != 8) return drawGroups(teams, ng, holder);
    std::stable_sort(teams.begin(), teams.end(), [holder](int a, int b) {
        if ((a == holder) != (b == holder)) return a == holder;
        return clubSeed(a) > clubSeed(b); });
    std::map<int, std::vector<int>> byNat;
    for (int t : teams) byNat[g_world.teams[t].nation].push_back(t);
    std::map<int, int> half;      // équipe -> 0 (A-D) ou 1 (E-H) ; absent = libre
    for (auto& kv : byNat) {
        auto& v = kv.second;
        if (v.size() < 2) continue;
        int h0 = g_rng.range(0, 1);
        half[v[0]] = h0; half[v[1]] = 1 - h0;
        if (v.size() >= 4) { int h2 = g_rng.range(0, 1); half[v[2]] = h2; half[v[3]] = 1 - h2; }
        // 3e club seul : au hasard (pas de contrainte)
    }
    // tirage complet avec retour arrière : chapeau par chapeau, groupe par groupe (A, B, C...), comme le tirage réel ;
    // un club n'est jamais placé dans un groupe qui rendrait la suite du tirage impossible
    std::vector<std::vector<int>> g(ng);
    int per = (int)teams.size() / ng;
    int n = per * ng;
    std::vector<int> slotTeam(n, -1);
    std::vector<char> used(n, 0);
    auto nat = [](int t) { return g_world.teams[t].nation; };
    long budget = 400000;
    bool useHalf = true;
    std::function<bool(int)> place = [&](int slot) -> bool {
        if (slot == n) return true;
        if (--budget < 0) return false;
        int pot = slot / ng, grp = slot % ng;
        std::vector<int> cand;
        for (int i = pot * ng; i < (pot + 1) * ng; i++) if (!used[i]) cand.push_back(i);
        g_rng.shuffle(cand);
        for (int ci : cand) {
            int t = teams[ci];
            bool ok = true;
            for (int k = 0; k < pot && ok; k++) { int o = slotTeam[k * ng + grp]; if (o >= 0 && nat(o) == nat(t)) ok = false; }
            if (ok && useHalf) { auto it = half.find(t); if (it != half.end() && it->second != grp / (ng / 2)) ok = false; }
            if (!ok) continue;
            used[ci] = 1; slotTeam[slot] = t;
            if (place(slot + 1)) return true;
            used[ci] = 0; slotTeam[slot] = -1;
        }
        return false;
    };
    bool ok = place(0);
    if (!ok) { useHalf = false; budget = 400000; std::fill(used.begin(), used.end(), 0); std::fill(slotTeam.begin(), slotTeam.end(), -1); ok = place(0); }
    if (!ok) return drawGroups(teams, ng, holder);
    for (int sl = 0; sl < n; sl++) g[sl % ng].push_back(slotTeam[sl]);
    for (int i = n; i < (int)teams.size(); i++) g[i % ng].push_back(teams[i]);
    return g;
}

void Competition::onStageFinished() {
    crashMark("fin de tour : %s - étape %d / %d", name.c_str(), cur, (int)stages.size());
    Stage& st = stages[cur];
    st.finished = true;
    std::vector<int> winners, losers;
    if (st.type == ST_KO) for (auto& t : st.ties) {
        int w = t.winner >= 0 ? t.winner : tieWinner(t);
        winners.push_back(w);
        if (t.b >= 0) losers.push_back(w == t.a ? t.b : t.a);
    }
    for (size_t i = 0; i < st.ties.size(); i++) st.ties[i].winner = winners[i];

    auto finish = [&](int w) { done = true; winner = w; };
    std::vector<std::pair<int, int>> pairsTmp;

    switch (format) {
    case FMT_LEAGUE: {
        result.clear();
        for (auto& s : table(0, 0)) result.push_back(s.team);
        finish(result.empty() ? -1 : result[0]);
        return;
    }
    case FMT_SINGLE: finish(winners.empty() ? -1 : winners[0]); result = winners; return;
    case FMT_CUP: {
        if(marneStageFinished(*this,winners)) return;
        if(awaiting>=0 && st.name=="Match d'ajustement du tableau"){
            int round=awaiting;awaiting=-1;std::vector<int> pool=winners;
            pool.insert(pool.end(),carry.begin(),carry.end());carry.clear();cupRound(round,pool);return;
        }
        int k = qualPlayoff>0 ? qualPlayoff : cur + 1;
        bool last = !koTargets.empty() && k >= (int)koTargets.size();
        if ((winners.size() == 1 && (k >= (int)entrants.size() || entrants[k].empty())) || last) {
            result = winners;
            finish(winners.size() == 1 ? winners[0] : -1);
            return;
        }
        std::vector<int> pool = winners;
        if (k < (int)entrants.size()) for (int e : entrants[k]) pool.push_back(e);
        cupRound(k, pool);
        return;
    }
    case FMT_EUROPE: {
        if (uwclStageFinished(*this, winners)) return;
        if (st.type == ST_SWISS) {
            auto tb2 = swissTable(cur);
            result.clear(); for (auto& s : tb2) result.push_back(s.team);
            std::vector<std::pair<int, int>> pairs;
            for (int i = 0; i < 8; i++) pairs.push_back({ tb2[23 - i].team, tb2[8 + i].team });
            cur = (int)stages.size();
            addKOStage(pairs, 2, koTimes[0], "Barrages");
            return;
        }
        if (winners.size() == 1) { finish(winners[0]); return; }
        auto pairs = bracketPairs(winners);
        bool fin = winners.size() == 2;
        int nk = (int)stages.size();
        int idx = std::min((int)koTimes.size() - 1, nk - 1);
        cur = nk; addKOStage(pairs, fin ? 1 : 2, koTimes[idx], koName((int)winners.size()), fin);
        return;
    }
    case FMT_UCL2000: {
        // format 2003-04 : 3 tours de qualification, phase de groupes (8 x 4), 8es, quarts, demies, finale
        // koTimes : 0-2 qualifications, 3-8 journées de groupes, 9 8es, 10 quarts, 11 demies, 12 finale
        int nk = (int)stages.size();
        if (st.type == ST_KO && nk <= 3) {
            if (nk == 3) extra = losers;               // éliminés du 3e tour -> 1er tour de la Coupe UEFA
            std::vector<int> pool = winners;
            int k = nk;
            if (k < (int)entrants.size()) for (int e : entrants[k]) pool.push_back(e);
            if (k < 3) { koRound(pool, 2, koTimes[k], koNames[k], false); return; }
            cur = nk;
            std::vector<double> times(koTimes.begin() + 3, koTimes.begin() + 9);
            addGroupStage(drawGroupsUCL(pool, std::max(1, (int)pool.size() / 4), host), 2, times, "Phase de groupes");
            return;
        }
        if (st.type == ST_LEAGUE) {
            // 1ers et 2es en 8es ; 3es reversés au 3e tour de la Coupe UEFA
            std::vector<int> first, second, gFirst, gSecond;
            extra2.clear();
            for (int g = 0; g < (int)st.groups.size(); g++) {
                auto tb2 = table(cur, g);
                if (tb2.size() >= 3) { first.push_back(tb2[0].team); gFirst.push_back(g); second.push_back(tb2[1].team); gSecond.push_back(g); extra2.push_back(tb2[2].team); }
            }
            // 8es : un 1er contre un 2e d'un autre groupe et d'un autre pays ; le 1er reçoit au retour
            int n = (int)first.size();
            std::vector<int> ord(n); for (int i = 0; i < n; i++) ord[i] = i;
            std::vector<int> best = ord;
            for (int attempt = 0; attempt < 500; attempt++) {
                g_rng.shuffle(ord);
                bool ok = true;
                for (int i = 0; i < n && ok; i++) {
                    int f = first[i], sc = second[ord[i]];
                    if (gFirst[i] == gSecond[ord[i]] || g_world.teams[f].nation == g_world.teams[sc].nation) ok = false;
                }
                if (ok) { best = ord; break; }
                if (attempt == 0) best = ord;
            }
            std::vector<std::pair<int, int>> pairs;
            for (int i = 0; i < n; i++) pairs.push_back({ second[best[i]], first[i] });
            cur = nk; addKOStage(pairs, 2, koTimes[9], "8es de finale");
            return;
        }
        if (winners.size() == 1) { finish(winners[0]); return; }
        bool fin = winners.size() == 2;
        std::string nm = winners.size() == 8 ? "Quarts de finale" : winners.size() == 4 ? "Demi-finales" : fin ? "Finale" : koName((int)winners.size());
        double t = winners.size() == 8 ? koTimes[10] : winners.size() == 4 ? koTimes[11] : koTimes[12];
        if (fin) { cur = nk; addKOStage({ { winners[0], winners[1] } }, 1, t, "Finale", true); return; }
        koRound(winners, 2, t, nm, false);
        return;
    }
    case FMT_UEFA2000: {
        int k = (int)stages.size();   // index du tour suivant
        if (winners.size() == 1 && k > 2) { finish(winners[0]); return; }
        // tours 1 (1er tour) et 3 (3e tour) attendent des entrées extérieures (Ligue des champions)
        if ((k == 1 || k == 3) && !(extReadyMask & (1 << k))) {
            carry = winners;
            awaiting = k;
            return;
        }
        std::vector<int> pool = winners;
        if (k < (int)entrants.size()) for (int e : entrants[k]) pool.push_back(e);
        bool fin = pool.size() == 2;
        koRound(pool, fin ? 1 : 2, k < (int)koTimes.size() ? koTimes[k] : 38, k < (int)koNames.size() ? koNames[k] : koName((int)pool.size()), fin);
        return;
    }
    case FMT_INTERTOTO: {
        int k = (int)stages.size();
        if (k >= (int)koTimes.size() || (winners.size() <= 3 && (k >= (int)entrants.size() || entrants[k].empty()) && k >= 4)) { result = winners; finish(winners.empty() ? -1 : winners[0]); return; }
        std::vector<int> pool = winners;
        if (k < (int)entrants.size()) for (int e : entrants[k]) pool.push_back(e);
        koRound(pool, 2, koTimes[k], k < (int)koNames.size() ? koNames[k] : koName((int)pool.size()), false);
        return;
    }
    case FMT_TOURNAMENT: {
        if (kind == 50 && legendStageFinished(*this, winners)) return;
        if (st.type == ST_LEAGUE && officialBracket(*this, cur, pairsTmp)) {
            cur = (int)stages.size();
            int n = (int)pairsTmp.size() * 2;
            addKOStage(pairsTmp, legs == 2 && n > 2 ? 2 : 1, koTimes.empty() ? st.rounds.back().time + 1 : koTimes[0], koName(n), n == 2);
            return;
        }
        if (st.type == ST_LEAGUE) {
            std::vector<Standing> firsts, seconds, thirds, fourths;
            for (int g = 0; g < (int)st.groups.size(); g++) {
                auto tb2 = table(cur, g);
                for (int i = 0; i < (int)tb2.size(); i++) {
                    if (i == 0) firsts.push_back(tb2[i]);
                    else if (i == 1) seconds.push_back(tb2[i]);
                    else if (i == 2) thirds.push_back(tb2[i]);
                    else if (i == 3) fourths.push_back(tb2[i]);
                }
            }
            sortStandings(firsts); sortStandings(seconds); sortStandings(thirds); sortStandings(fourths);
            std::vector<int> seeds;
            for (auto& s : firsts) seeds.push_back(s.team);
            if (groupsAdvance >= 2) for (auto& s : seconds) seeds.push_back(s.team);
            if (groupsAdvance >= 3) for (auto& s : thirds) seeds.push_back(s.team);           // Final Four : les 4 premiers du groupe
            else for (int i = 0; i < bestThirds && i < (int)thirds.size(); i++) seeds.push_back(thirds[i].team);
            if (groupsAdvance >= 4) for (auto& s : fourths) seeds.push_back(s.team);
            int n = (int)seeds.size();
            std::map<int, int> grp;
            for (int g = 0; g < (int)st.groups.size(); g++) for (int t : st.groups[g]) grp[t] = g;
            std::vector<std::pair<int, int>> pairs;
            std::vector<int> low(seeds.begin() + n / 2, seeds.end());
            std::reverse(low.begin(), low.end());
            for (int i = 0; i < n / 2; i++) {
                if (grp[seeds[i]] == grp[low[i]]) {
                    for (int j = i + 1; j < n / 2; j++)
                        if (grp[seeds[i]] != grp[low[j]] && grp[seeds[j]] != grp[low[i]]) { std::swap(low[i], low[j]); break; }
                }
            }
            for (int i = 0; i < n / 2; i++) pairs.push_back({ seeds[i], low[i] });
            std::vector<std::pair<int, int>> ord;
            std::vector<int> pos = { 0 };
            while ((int)pos.size() < n / 2) {
                std::vector<int> np; int m = (int)pos.size() * 2;
                for (int p : pos) { np.push_back(p); np.push_back(m - 1 - p); }
                pos = np;
            }
            for (int p : pos) ord.push_back(pairs[p]);
            cur = (int)stages.size();
            bool ff = groupsAdvance >= 4;     // Final Four : matchs secs
            addKOStage(ord, legs == 2 && n > 2 && !ff ? 2 : 1, koTimes.empty() ? st.rounds.back().time + 1 : koTimes[0], kind == KIND_CONTINENTS && !koNames.empty() ? koNames[0] : koName(n), n == 2);
            return;
        }
        int nk = (int)stages.size();
        if (winners.size() == 1) { finish(winners[0]); return; }
        auto pairs = bracketPairs(winners);
        int idx = std::min((int)koTimes.size() - 1, nk - 1);
        double t = koTimes.empty() ? st.rounds.back().time + 1 : koTimes[std::max(0, idx)];
        if (winners.size() == 2 && thirdPlace) {
            std::vector<int> ls;
            for (auto& ti : st.ties) ls.push_back(ti.winner == ti.a ? ti.b : ti.a);
            addKOStage({ { ls[0], ls[1] } }, 1, t - 0.2, "Match pour la 3e place", true);
        }
        cur = (int)stages.size();
        int kn = nk - (int)std::count_if(stages.begin(), stages.end(), [](const Stage& x) { return x.type == ST_LEAGUE; });
        std::string knm = kind == KIND_CONTINENTS && kn < (int)koNames.size() ? koNames[kn] : koName((int)winners.size());
        addKOStage(pairs, (legs == 2 && winners.size() > 2 && groupsAdvance < 4) ? 2 : 1, t, knm, winners.size() == 2);
        return;
    }
    case FMT_QUAL_GROUPS: done = true; return;
    case FMT_NEWEURO: newEuroStageFinished(*this, winners); return;
    case FMT_KO_ONLY: {
        int want = std::max(1, qualSpots);
        int nk = (int)stages.size();
        // chaîne de barrages : le vainqueur affronte l'équipe suivante de la file (qui reçoit)
        if (!extra.empty() && winners.size() == 1) {
            int nxt = extra.front(); extra.erase(extra.begin());
            int lg = nk < (int)koLegs.size() ? koLegs[nk] : 1;
            double t = nk < (int)koTimes.size() ? koTimes[nk] : st.rounds.back().time + 0.8;
            cur = nk;
            addKOStage({ lg == 2 ? std::make_pair(winners[0], nxt) : std::make_pair(nxt, winners[0]) }, lg, t, nk < (int)koNames.size() ? koNames[nk] : "Barrage");
            return;
        }
        if ((int)winners.size() <= want) { result = winners; finish(winners.size() == 1 ? winners[0] : -1); return; }
        auto pairs = bracketPairs(winners);
        int lg = nk < (int)koLegs.size() ? koLegs[nk] : 1;
        double t = nk < (int)koTimes.size() ? koTimes[nk] : st.rounds.back().time + 1;
        bool fin = winners.size() == 2;
        cur = nk; addKOStage(pairs, lg, t, nk < (int)koNames.size() ? koNames[nk] : koName((int)winners.size()), fin && lg == 1 && neutralFinal);
        return;
    }
    }
}

int Competition::stageOfMatch(int mi) const {
    for (int s = (int)stages.size() - 1; s >= 0; s--)
        for (auto& R : stages[s].rounds) for (int x : R.m) if (x == mi) return s;
    return -1;
}

void Competition::cupRound(int k, const std::vector<int>& pool) {
    crashMark("tirage au sort (coupe) : %s - tour %d (%d équipes)", name.c_str(), k + 1, (int)pool.size());
    int n = (int)pool.size();
    int target = (n + 1) / 2;
    if (k < (int)koTargets.size() && koTargets[k] > 0) target = std::min(koTargets[k], n);
    if (target * 2 < n) target = (n + 1) / 2;
    if (target >= n) target = std::max(1, n / 2);
    int nm = n - target;
    std::vector<int> sorted = pool;
    std::stable_sort(sorted.begin(), sorted.end(), [](int a, int b) { if(clubFirstSeason(a)!=clubFirstSeason(b))return !clubFirstSeason(a);return g_world.teams[a].rating > g_world.teams[b].rating; });
    int nbyes = n - 2 * nm;
    int eligible=0;for(int t:pool)eligible+=!clubFirstSeason(t);
    while(nbyes>eligible && nm<n/2){nm++;nbyes-=2;}
    // An odd field consisting entirely of debut clubs cannot award a bye.
    // A played adjustment tie reduces it to an even field. Its logical round
    // is retained separately from the extra physical stage (also across saves).
    qualPlayoff=k+1;
    if(nbyes>eligible && n>2){
        g_rng.shuffle(sorted);carry.assign(sorted.begin()+2,sorted.end());awaiting=k;
        cur=(int)stages.size();double time=k<(int)koTimes.size()?koTimes[k]:t0;
        addKOStage({{sorted[0],sorted[1]}},1,time-1.0/7,"Match d'ajustement du tableau",false);return;
    }
    std::vector<int> byes, play;
    // Coupe de la Ligue : tableau fixe à partir des 8es (vainqueur du match 1 contre vainqueur du match 2...)
    bool fixedBracket = kind == 11 && k > 3 && nbyes <= 0;
    if (fixedBracket) play = pool;
    else {
        for (int i = 0; i < n; i++) (i < nbyes ? byes : play).push_back(sorted[i]);
        g_rng.shuffle(play);
    }
    if (!fixedBracket && regionalDraw && k < regionalRounds)
        std::stable_sort(play.begin(), play.end(), [](int a, int b) {
            const Team& A = g_world.teams[a]; const Team& B = g_world.teams[b];
            if (A.region != B.region) return A.region < B.region;
            if (A.district != B.district) return A.district < B.district;      // si possible, adversaire du même district
            return A.dept < B.dept; });
    std::vector<std::pair<int, int>> pairs;
    for (size_t i = 0; i + 1 < play.size(); i += 2) {
        int a = play[i], b = play[i + 1];
        if (homeRule == 1) {
            // règle FFF : avec deux divisions d'écart (ou plus), le club de la plus petite division reçoit
            int la = teamLevel(a), lb = teamLevel(b);
            if (lb - la >= 2) std::swap(a, b);
        }
        pairs.push_back({ a, b });
    }
    for (int b : byes) pairs.push_back({ b, -1 });
    bool fin = (nm == 1 && byes.empty());
    std::string nmn = k < (int)koNames.size() ? koNames[k] : koName(n);
    if (fin) nmn = "Finale";
    cur = (int)stages.size();
    double t = k < (int)koTimes.size() ? koTimes[k] : (stages.empty() ? t0 : stages.back().rounds.back().time + 2);
    addKOStage(pairs, 1, t, nmn, fin && neutralFinal);
}

// ------------------------------------------------------------------ événements des matchs simulés
static int pickPlayer(const Team& T, Rng& r, const float wpos[4], bool useShoot, int exclude) {
    float tot = 0;
    int n = std::min((int)T.squad.size(), 30);
    static float w[64];
    for (int i = 0; i < n; i++) {
        const Player& p = T.squad[i];
        float x = 0;
        if (p.id != exclude && p.suspended <= 0 && p.injured <= 0) {
            float ov = (float)p.overall();
            x = wpos[p.pos] * (useShoot ? p.shoot : p.pass) * (ov * ov) / 4000.f;
            if (i >= 18) x *= 0.3f;      // remplaçants peu utilisés
        }
        w[i] = x; tot += x;
    }
    if (tot <= 0) return -1;
    float v = r.f() * tot;
    for (int i = 0; i < n; i++) { v -= w[i]; if (v <= 0) return i; }
    return n - 1;
}

void genMatchEvents(Competition& C, int mi) {
    MatchRes& m = C.matches[mi];
    if (!m.played) return;
    Rng& r = g_rng;
    static const float WG[4] = { 0.02f, 0.5f, 1.6f, 4.0f };   // buts selon le poste
    static const float WA[4] = { 0.05f, 0.9f, 3.0f, 2.2f };   // passes décisives
    int ids[2] = { m.home, m.away };
    int goals[2] = { m.hg, m.ag };
    size_t start = C.events.size();
    for (int side = 0; side < 2; side++) {
        g_world.ensureSquad(ids[side]);
        Team& T = g_world.teams[ids[side]];
        Team& O = g_world.teams[ids[1 - side]];
        {   // matchs joués (simulation) : les 11 meilleurs disponibles (gardien compris) + 3 remplaçants
            int n = (int)T.squad.size();
            std::vector<std::pair<int, int>> ord;
            int gk = -1, gv = -1;
            for (int i = 0; i < n; i++) {
                const Player& p = T.squad[i];
                if (p.suspended > 0 || p.injured > 0) continue;
                if (p.pos == POS_GK) { if (p.keep > gv) { gv = p.keep; gk = i; } continue; }
                ord.push_back({ -(p.overall() + r.range(-4, 4)), i });
            }
            std::sort(ord.begin(), ord.end());
            if (gk >= 0) T.squad[gk].apps++;
            for (int k = 0; k < (int)ord.size() && k < 13; k++) if (k < 10 || r.chance(0.7f)) T.squad[ord[k].second].apps++;
        }
        for (int g = 0; g < goals[side]; g++) {
            MEv e; e.match = mi; e.side = (uint8_t)side; e.team = ids[side];
            e.minute = (uint8_t)(m.aet && r.chance(0.25f) ? r.range(91, 120) : r.range(1, C.kind==150?40:90));
            if (r.chance(0.035f) && !O.squad.empty()) {           // contre son camp
                int k = pickPlayer(O, r, WA, false, -1);
                if (k < 0) continue;
                e.type = 1; e.pid = O.squad[k].id; e.aid = 0; e.team = ids[1 - side];
                C.events.push_back(e);
                continue;
            }
            int k = pickPlayer(T, r, WG, true, -1);
            if (k < 0) continue;
            e.type = 0; e.pid = T.squad[k].id;
            e.pen = r.chance(0.09f) ? 1 : 0;
            T.squad[k].goals++;
            if (!e.pen && r.chance(0.72f)) {
                int a = pickPlayer(T, r, WA, false, e.pid);
                if (a >= 0) { e.aid = T.squad[a].id; T.squad[a].assists++; }
            }
            C.events.push_back(e);
        }
        // cartons (sévérité de l'arbitre)
        long ci = g_career.season.comps.empty() ? -1 : (long)(&C - g_career.season.comps.data());
        if (ci < 0 || ci >= (long)g_career.season.comps.size()) ci = 0;
        const Referee& RF = REFEREES[refereeFor((int)ci, mi)];
        float sev = RF.severity / 60.f * (side == 0 ? 1.f - (RF.homeBias - 50) / 250.f : 1.f + (RF.homeBias - 50) / 250.f);
        int yel = (int)(r.f() * 3.2f * sev);
        std::vector<int> booked;
        for (int k = 0; k < yel; k++) {
            static const float WC[4] = { 0.1f, 2.2f, 1.6f, 0.9f };
            int i = pickPlayer(T, r, WC, false, -1);
            if (i < 0) continue;
            MEv e; e.match = mi; e.side = (uint8_t)side; e.team = ids[side]; e.type = 2; e.pid = T.squad[i].id; e.minute = (uint8_t)r.range(5, C.kind==150?40:90);
            if (std::find(booked.begin(), booked.end(), i) != booked.end()) {
                // deuxième avertissement : expulsion
                e.type = 3; C.events.push_back(e);
                T.squad[i].suspended = 1; T.squad[i].sRed++;
                continue;
            }
            booked.push_back(i);
            C.events.push_back(e);
            Player& P = T.squad[i];
            P.yellows++; P.sYel++;
            if (P.yellows >= C.yellowLimit) { P.suspended = 1; P.yellows = 0; }
        }
        if (r.chance(0.035f * sev)) {
            static const float WC[4] = { 0.1f, 2.4f, 1.4f, 0.8f };
            int i = pickPlayer(T, r, WC, false, -1);
            if (i >= 0) {
                MEv e; e.match = mi; e.side = (uint8_t)side; e.team = ids[side]; e.type = 3; e.pid = T.squad[i].id; e.minute = (uint8_t)r.range(20, C.kind==150?40:90);
                C.events.push_back(e);
                T.squad[i].suspended = (int8_t)r.range(1, 3); T.squad[i].sRed++;
            }
        }
        if (r.chance(0.04f)) {
            static const float WI[4] = { 0.2f, 1, 1, 1 };
            int i = pickPlayer(T, r, WI, false, -1);
            if (i >= 0) {
                MEv e; e.match = mi; e.side = (uint8_t)side; e.team = ids[side]; e.type = 4; e.pid = T.squad[i].id; e.minute = (uint8_t)r.range(1, C.kind==150?40:90);
                C.events.push_back(e);
                T.squad[i].injured = (int8_t)std::max(1, r.range(1, 5) - injuryReduction(ids[side]));
            }
        }
    }
    std::stable_sort(C.events.begin() + start, C.events.end(), [](const MEv& a, const MEv& b) { return a.minute < b.minute; });
    for(int ci=0;ci<(int)g_career.season.comps.size();ci++)if(&g_career.season.comps[ci]==&C){g_career.museumEvents(ci,mi);break;}
}
