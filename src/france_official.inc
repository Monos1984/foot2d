// Imported before the fallback generator; no synthetic additions in covered pools.
for (int i = 0; i < NUM_FR_OFFICIAL_2627; i++) {
    const auto& def = FR_OFFICIAL_2627[i];
    int key = def.tier < TD1 ? def.region : districtIndex(sanitize(def.district));
    if (key < 0) throw std::runtime_error("Unknown official district");
    int q = poolOf(def.tier, key);
    if (q < 0) q = addPool(def.tier, key, 1, 12);
    auto& pool = P.pools[q];
    if (!pool.officialGroups) { pool.clubs.clear(); pool.groups.clear(); pool.groupSizes.clear(); }
    pool.officialGroups = true;
    if ((int)pool.groups.size() <= def.group) { pool.groups.resize(def.group+1); pool.groupSizes.resize(def.group+1); }
    pool.groupSizes[def.group]++;
    pool.nGroups = (int)pool.groups.size();
}
// D3 accepts 11/12 teams depending on engagements; these are legal capacities.
// D2's exceptional 13-team group instead keeps its ordinary 12-team target.
for (auto& pool : P.pools) if (pool.officialGroups && pool.tier == 10 &&
    districtName(pool.key) == "Finistère") pool.nextGroupSizes = pool.groupSizes;
for(auto& pool:P.pools) if(pool.officialGroups && pool.key==5 && pool.tier>=5 && pool.tier<=7)
    pool.nextGroupSizes.assign(pool.tier==7?13:pool.tier==6?7:3,pool.tier==5?14:12);
// First teams then reserves; parent resolution must succeed and may never fall back.
for(auto& pool:P.pools) if(pool.officialGroups && pool.tier==8) {
    const char* names[]={"Ardennes","Aube","Haute-Marne","Marne","Meurthe-et-Moselle","Meuse","Moselle","Vosges","Alsace"};
    const int quotas[]={2,1,1,2,3,1,6,2,10};
    for(int i=0;i<9;i++)if(districtName(pool.key)==names[i])pool.upCap=quotas[i];
}
for (int pass = 0; pass < 2; pass++) for (int i = 0; i < NUM_FR_OFFICIAL_2627; i++) {
    const auto& def = FR_OFFICIAL_2627[i];
    if ((def.squad > 1) != (pass == 1)) continue;
    int parent = -1;
    if (def.squad > 1) {
        auto it = byName.find(sanitize(def.parent));
        if (it == byName.end()) throw std::runtime_error(std::string("Missing official parent: ") + def.parent);
        parent = it->second;
    }
    int dept = deptIndex(def.dept);
    if (dept < 0) throw std::runtime_error("Invalid official department");
    std::string name = sanitize(def.name);
    if (g_usedNames.count(name)) throw std::runtime_error("Duplicate official team: " + name);
    int id = makeGenClub(w, {name, name}, dept, frTierBase(def.tier));
    Team& team = w.teams[id];
    // The verified league/district affiliation may cross a departmental boundary.
    team.region = def.region;
    team.stadium = sanitize("Stade municipal");
    team.parent = parent; team.resLevel = def.squad-1;
    if (parent >= 0) { team.home = w.teams[parent].home; team.away = w.teams[parent].away; }
    else byName[team.name] = id;
    g_usedNames.insert(team.name);
    int key = def.tier < TD1 ? def.region : districtIndex(sanitize(def.district));
    if (def.tier >= TD1) team.district = key;
    auto& pool = P.pools[poolOf(def.tier, key)];
    pool.clubs.push_back(id); pool.groups[def.group].push_back(id);
}
