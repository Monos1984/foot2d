#include "game.h"
#include <set>
#include <cctype>

std::vector<std::string> validateFrancePyramid(const Pyramid& P, bool published, bool normalized) {
    std::vector<std::string> errors;
    std::set<int> assigned;
    std::set<std::string> names;
    std::map<int,int> tiers;
    auto fail = [&](const std::string& s) { errors.push_back(s); };
    for (const auto& pool : P.pools) {
        if (pool.tier < 0 || pool.tier >= (int)P.tiers.size()) { fail("Invalid tier"); continue; }
        bool fixed = !P.tiers[pool.tier].flexible && !pool.terminal;
        for (int size : pool.nextGroupSizes) if (size < 1) fail("Invalid next-season group target");
        if (normalized && fixed && (int)pool.clubs.size() != poolTarget(P,pool)) fail("Next-season pool target not reached");
        std::set<int> members, drawn;
        for (int c : pool.clubs) {
            if (c < 0 || c >= (int)g_world.teams.size()) { fail("Invalid club ID"); continue; }
            if (!assigned.insert(c).second) fail("Team assigned twice: " + g_world.teams[c].name);
            members.insert(c); tiers[c] = pool.tier;
            const auto& team = g_world.teams[c];
            std::string name=team.name;
            for (char& ch : name) ch=(char)std::tolower((unsigned char)ch);
            if (name.empty() || !names.insert(name).second) fail("Empty/duplicate name: " + team.name);
            if (team.region < 0 || team.region >= NUM_REGIONS || team.district < 0 || team.district >= numDistricts()) fail("Invalid geography: " + team.name);
            else if (districtRegion(team.district) != team.region) fail("District/region mismatch: " + team.name);
            if (P.tiers[pool.tier].scope == SC_REGION && pool.key != team.region) fail("Wrong league: " + team.name);
            if (P.tiers[pool.tier].scope == SC_DEPT && pool.key != team.district) fail("Wrong district: " + team.name);
        }
        if ((int)pool.groups.size() != poolGroupCount(P,pool)) fail("Incorrect group count");
        for (size_t g=0; g<pool.groups.size(); g++) {
            if (normalized && fixed) {
                int expected = pool.groupSizes.empty() ? pool.size : (g < pool.groupSizes.size() ? pool.groupSizes[g] : -1);
                if ((int)pool.groups[g].size() != expected) fail("Next-season group target not reached");
            }
            if (pool.groups[g].empty()) fail("Empty group");
            if (!pool.groupSizes.empty() && pool.officialGroups && (int)pool.groups[g].size()!=pool.groupSizes[g]) fail("Incomplete official group");
            for (int c : pool.groups[g]) {
                if (!members.count(c) || !drawn.insert(c).second) fail("Missing/duplicate group member");
            }
        }
        if (members != drawn) fail("Club without a group");
    }
    for (int c : assigned) {
        const auto& team=g_world.teams[c];
        if (team.parent >= 0) {
            if (!assigned.count(team.parent) || team.parent==c) fail("Orphan reserve: " + team.name);
            else {
                const auto& parent=g_world.teams[team.parent];
                bool declared=false;
                for(int i=0;i<NUM_FR_OFFICIAL_2627;i++){const auto& d=FR_OFFICIAL_2627[i];
                    if(team.name==sanitize(d.name) && parent.name==sanitize(d.parent) && d.district[0] && districtIndex(sanitize(d.district))==team.district){declared=true;break;}}
                if (parent.parent>=0 || team.resLevel<1 || parent.region!=team.region || (parent.district!=team.district && !declared)) fail("Invalid reserve link: " + team.name);
            }
        }
    }
    if (published) for (int i=0;i<NUM_FR_OFFICIAL_2627;i++) {
        const auto& def=FR_OFFICIAL_2627[i];
        int key=def.tier<8?def.region:districtIndex(sanitize(def.district));
        int q=P.poolIndex(def.tier,key), found=-1;
        if (q<0) { fail("Missing official pool"); continue; }
        const auto& pool=P.pools[q];
        if (!pool.officialGroups || def.group>=(int)pool.groups.size()) { fail("Official groups lost"); continue; }
        for (int c : pool.groups[def.group]) if (g_world.teams[c].name==sanitize(def.name)) found=c;
        if (found<0) { fail(std::string("Missing published team: ")+def.name); continue; }
        const auto& team=g_world.teams[found];
        if (team.resLevel!=def.squad-1) fail("Wrong reserve number: " + team.name);
        if (def.squad>1 && (team.parent<0 || team.parent>=(int)g_world.teams.size() || g_world.teams[team.parent].name!=sanitize(def.parent))) fail("Wrong official parent: " + team.name);
        if (team.dept<0 || std::string(DEPTS[team.dept].code)!=def.dept) fail("Wrong department: " + team.name);
    }
    return errors;
}
