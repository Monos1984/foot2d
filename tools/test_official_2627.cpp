#include "../src/game.h"
#include <cstdlib>
#include <set>

static int checks=0;
static void require(bool ok,const char* message) { checks++; if(!ok) { std::printf("FAIL %s\n",message); std::exit(1); } }
static Pyramid& france(std::vector<Pyramid>& ps) { for(auto& p:ps) if(p.country=="FRA"&&p.dom<0) return p; std::exit(2); }
static void valid(const Pyramid& p,bool published,bool normalized=false) { auto es=validateFrancePyramid(p,published,normalized); for(auto& e:es) std::printf("ERROR %s\n",e.c_str()); require(es.empty(),"validator"); }
int main() {
    g_world.build();
    auto& base=france(g_basePyramids);
    valid(base,true);
    require(NUM_FR_OFFICIAL_2627==3761,"3761 imported teams");
    int groups=0,teams=0;
    for(auto& pool:base.pools) if(pool.officialGroups&&pool.tier>=5) { groups+=(int)pool.groups.size();teams+=(int)pool.clubs.size(); }
    require(groups==327&&teams==3761,"327 exact published groups");
    require(base.pools[base.poolIndex(6,3)].clubs.size()==49&&base.pools[base.poolIndex(7,3)].clubs.size()==95,"Normandie exceptional R2/R3 totals preserved");
    const auto& finD2=base.pools[base.poolIndex(9,districtIndex("Finistère"))];
    require(finD2.groupSizes.size()==10&&finD2.groupSizes[0]==13&&finD2.clubs.size()==121,"Finistere exceptional 13-team group preserved");
    const auto& finD3=base.pools[base.poolIndex(10,districtIndex("Finistère"))];
    require(finD3.clubs.size()==157&&finD3.nextGroupSizes==finD3.groupSizes,"Finistere legal 11/12-team capacities preserved");
    require(base.poolIndex(12,districtIndex("Finistère"))<0,"no fictitious Finistere D5");
    auto before=base;
    formGroups(base);
    for(size_t i=0;i<base.pools.size();i++) if(base.pools[i].officialGroups) require(base.pools[i].groups==before.pools[i].groups,"published draw survives regroup");
    auto broken=base;
    broken.pools[broken.poolIndex(5,6)].clubs.push_back(broken.pools[0].clubs[0]);
    require(!validateFrancePyramid(broken,true).empty(),"validator rejects duplicate/wrong region");
    broken=base; broken.pools[broken.poolIndex(5,6)].groups[0].pop_back();
    require(!validateFrancePyramid(broken,true).empty(),"validator rejects incomplete group");
    int reserve=-1;
    for(int c:base.pools[base.poolIndex(5,6)].clubs) if(g_world.teams[c].parent>=0) {reserve=c;break;}
    require(reserve>=0,"imported reserve exists");
    int parent=g_world.teams[reserve].parent;
    g_world.teams[reserve].parent=(int)g_world.teams.size()+1;
    require(!validateFrancePyramid(base,true).empty(),"validator rejects orphan reserve");
    g_world.teams[reserve].parent=parent;
    // Sizes 11/12/13, and D6/D7/D8 are represented without padding teams.
    for(int tier=13;tier<=15;tier++) {
        Pyramid sample=base; Pool p; p.tier=tier;p.key=0;p.groupSizes={11,12,13};
        p.clubs.assign(base.pools[0].clubs.begin(),base.pools[0].clubs.end());
        p.clubs.insert(p.clubs.end(),base.pools[1].clubs.begin(),base.pools[1].clubs.end());
        sample.pools={p}; formGroups(sample);
        require(poolTarget(sample,sample.pools[0])==36,"individual group capacities");
        require(sample.pools[0].groups[0].size()==11&&sample.pools[0].groups[1].size()==12&&sample.pools[0].groups[2].size()==13,"D6-D8 uneven groups");
    }
    // Controlled surplus, deficit, reserve cascade and absent lower division.
    auto sample=[&](std::vector<int> counts) {
        Pyramid p; p.country="TEST"; p.tiers.resize(counts.size());
        int at=0;
        for(int t=0;t<(int)counts.size();++t) {
            p.tiers[t].scope=SC_NATIONAL; p.tiers[t].up=0;p.tiers[t].down=0;
            Pool pool;pool.tier=t;pool.size=4;pool.nGroups=1;
            pool.terminal=t==(int)counts.size()-1 && counts.size()>1;
            for(int c=0;c<counts[t];++c) pool.clubs.push_back(base.pools[0].clubs[at++]);
            p.pools.push_back(pool);
        }
        return p;
    };
    auto conserved=[&](const Pyramid& p,int expected) {
        std::set<int> ids;int count=0;
        for(const auto& pool:p.pools) for(int c:pool.clubs){count++;ids.insert(c);}
        require(count==expected&&(int)ids.size()==expected,"cascade conserves clubs without duplicates");
    };
    Career scenario;
    scenario.pyramids={sample({6,4,6})};
    applyPyramidSeasonResults(scenario,0);
    require(scenario.pyramids[0].pools[0].clubs.size()==4&&scenario.pyramids[0].pools[1].clubs.size()==4&&scenario.pyramids[0].pools[2].clubs.size()==8,"surplus descends through two levels");
    conserved(scenario.pyramids[0],16);
    scenario.pyramids={sample({2,6,6})};applyPyramidSeasonResults(scenario,0);
    require(scenario.pyramids[0].pools[0].clubs.size()==4&&scenario.pyramids[0].pools[1].clubs.size()==4,"deficit filled by additional promotion");conserved(scenario.pyramids[0],14);
    scenario.pyramids={sample({8,4})};
    scenario.pyramids[0].pools[0].nGroups=2;scenario.pyramids[0].pools[0].groupSizes={5,3};
    applyPyramidSeasonResults(scenario,0);
    require(scenario.pyramids[0].pools[0].groups[0].size()==4&&scenario.pyramids[0].pools[0].groups[1].size()==4,"uneven groups normalized even with correct total");
    scenario.pyramids[0].pools[0].nextGroupSizes={3,5};
    applyPyramidSeasonResults(scenario,0);
    require(scenario.pyramids[0].pools[0].groups[0].size()==3&&scenario.pyramids[0].pools[0].groups[1].size()==5,"explicit per-group next-season targets");
    scenario.pyramids={sample({6})};applyPyramidSeasonResults(scenario,0);conserved(scenario.pyramids[0],6);
    require(!scenario.season.news.empty(),"unattainable target reported without inventing clubs");
    scenario.pyramids={sample({6,4,6})};
    int root=scenario.pyramids[0].pools[0].clubs[0],r=scenario.pyramids[0].pools[1].clubs[0];
    Team saved=g_world.teams[r];g_world.teams[r].parent=root;g_world.teams[r].resLevel=1;
    applyPyramidSeasonResults(scenario,0);
    require(std::find(scenario.pyramids[0].pools[2].clubs.begin(),scenario.pyramids[0].pools[2].clubs.end(),r)!=scenario.pyramids[0].pools[2].clubs.end(),"reserve follows relegated parent below its division");
    require(scenario.pyramids[0].pools[1].clubs.size()==4,"reserve cascade rebalanced");conserved(scenario.pyramids[0],16);
    g_world.teams[r]=saved;
    scenario.pyramids={sample({2,0,6})};
    scenario.pyramids[0].pools.erase(scenario.pyramids[0].pools.begin()+1);
    applyPyramidSeasonResults(scenario,0);
    require(scenario.pyramids[0].pools[0].clubs.size()==4,"promotion across absent division uses nearest existing level");
    conserved(scenario.pyramids[0],8);
    scenario.pyramids={sample({4,4})};scenario.season=Season();
    auto& rep=scenario.pyramids[0];rep.tiers[0].down=1;rep.tiers[0].noReserves=true;
    Competition result;result.result=rep.pools[0].clubs;scenario.season.comps={result};rep.pools[0].comps={0};
    std::vector<Team> originals;
    for(int c:rep.pools[1].clubs){originals.push_back(g_world.teams[c]);g_world.teams[c].parent=rep.pools[0].clubs[0];g_world.teams[c].resLevel=1;}
    applyPyramidSeasonResults(scenario,0);
    require(rep.pools[0].clubs.size()==4,"reduce relegations when lower teams cannot be promoted");conserved(rep,8);
    for(size_t i=0;i<originals.size();++i) g_world.teams[base.pools[0].clubs[i+4]]=originals[i];
    int user=base.pools[base.poolIndex(5,6)].clubs[0];
    g_career.newClubCareer(user,2026); valid(france(g_career.pyramids),true);
    auto draw=france(g_career.pyramids).pools[france(g_career.pyramids).poolIndex(5,6)].groups;
    france(g_career.pyramids).pools[france(g_career.pyramids).poolIndex(5,6)].nextGroupSizes={12,12};
    require(g_career.save("build/test-official.sav"),"save v25");
    require(g_career.load("build/test-official.sav"),"load v25");
    valid(france(g_career.pyramids),true);
    require(france(g_career.pyramids).pools[france(g_career.pyramids).poolIndex(5,6)].nextGroupSizes==std::vector<int>({12,12}),"saved next-season group targets");
    require(france(g_career.pyramids).pools[france(g_career.pyramids).poolIndex(5,6)].groups==draw,"saved official groups");
    require(g_career.load("build/test-legacy-v23.sav"),"load v23 fixture from unmodified build10 writer");
    require(g_career.load("build/test-legacy-v24.sav"),"load v24 fixture from unmodified build11 writer");
    g_world.build();
    g_career.newClubCareer(user,2026);
    int advances=0;
    while(advances++<500000) { auto m=g_career.season.advance(true);if(m.comp<0)break; }
    require(advances<500000&&g_career.season.finished,"full season simulation");
    g_career.endSeason();
    valid(france(g_career.pyramids),false,true);
    const auto& next=france(g_career.pyramids);
    require(next.pools[next.poolIndex(3,0)].clubs.size()==48,"N1 returns from 49 to 48");
    require(next.pools[next.poolIndex(4,0)].clubs.size()==112,"N2 returns from 111 to 112");
    require(next.pools[next.poolIndex(6,3)].clubs.size()==48&&next.pools[next.poolIndex(7,3)].clubs.size()==96,"Normandie R2/R3 normalize with cascading movements");
    require(next.pools[next.poolIndex(9,districtIndex("Finistère"))].clubs.size()==120,"Finistere D2 returns from 121 to 120");
    size_t oldCount=0,newCount=0;
    for(const auto& p:base.pools) oldCount+=p.clubs.size();
    for(const auto& p:next.pools) {
        newCount+=p.clubs.size();
        if(!p.terminal&&!next.tiers[p.tier].flexible) {
            if((int)p.clubs.size()!=poolTarget(next,p)) std::printf("UNBALANCED tier=%d key=%d actual=%d target=%d\n",p.tier,p.key,(int)p.clubs.size(),poolTarget(next,p));
            require((int)p.clubs.size()==poolTarget(next,p),"all fixed pools normalized after full season");
        }
    }
    require(oldCount==newCount,"season movements preserve every club");
    auto badNext=next;int nq=badNext.poolIndex(3,0);
    int shifted=badNext.pools[nq].groups[0].back();badNext.pools[nq].groups[0].pop_back();badNext.pools[nq].groups[1].push_back(shifted);
    require(!validateFrancePyramid(badNext,false,true).empty(),"validator rejects wrong per-group next-season sizes");
    while(!g_career.season.finished) g_career.season.advance(true);
    g_career.endSeason();valid(france(g_career.pyramids),false,true);
    size_t secondCount=0;for(const auto& p:france(g_career.pyramids).pools)secondCount+=p.clubs.size();
    require(secondCount==oldCount,"two successive season transitions conserve clubs");
    g_world.build();
    g_career.newClubCareer(user,2026); g_career.mgr.managerMode=true;
    valid(france(g_career.pyramids),true);
    CustomCompDef custom; custom.name="Official import smoke"; custom.format=1; custom.legs=1;
    const auto& pool=france(g_basePyramids).pools[france(g_basePyramids).poolIndex(5,6)];
    custom.teams={pool.clubs[0],pool.clubs[1],pool.clubs[2],pool.clubs[3]};
    g_career.newCustom(custom,{custom.teams[0]});
    for(int n=0;n<1000&&!g_career.season.finished;n++) g_career.season.advance(true);
    require(g_career.season.finished&&g_career.season.comps[0].winner>=0,"custom tournament imported clubs");
    std::printf("PASS %d checks, %d season advances\n",checks,advances);
}
