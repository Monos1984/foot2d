import json
c=json.load(open('/tmp/package/data/communes.json'))
d=json.load(open('/tmp/package/data/departements.json'))
r=json.load(open('/tmp/package/data/regions.json'))
regions=['11','24','27','28','32','44','52','53','75','76','84','93','94','01','02','03','04','06']
rname={x['code']:x['nom'] for x in r}
out=[]
out.append('// Généré automatiquement depuis les données officielles des communes (INSEE / etalab)\n#include "data.h"\n')
out.append('const RegionDef REGIONS[] = {')
for code in regions: out.append('  {"%s", "%s"},' % (code, rname[code].replace('"','\\"')))
out.append('};\nconst int NUM_REGIONS = %d;\n' % len(regions))
out.append('const DeptDef DEPTS[] = {')
n=0
for dep in d:
    if dep['region'] not in regions: continue
    if dep['code']=='75':
        towns=[(x['nom'].replace(' Arrondissement',''),x['population']) for x in c if x['type']=='arrondissement-municipal' and x['nom'].startswith('Paris')]
    else:
        towns=[(x['nom'],x.get('population',0)) for x in c if x['type']=='commune-actuelle' and x['departement']==dep['code']]
    total=sum(t[1] for t in towns) if dep['code']!='75' else 2100000
    towns.sort(key=lambda t:-t[1]); towns=towns[:160]
    s=';'.join('%s|%d'%(t[0].replace('"',''),t[1]) for t in towns)
    out.append('  {"%s", "%s", %d, %d, "%s"},' % (dep['code'], dep['nom'], regions.index(dep['region']), total, s))
    n+=1
out.append('};\nconst int NUM_DEPTS = %d;\n' % n)
open('/home/claude/foot/src/data_france_geo.cpp','w',encoding='utf-8').write('\n'.join(out))
print(n)
