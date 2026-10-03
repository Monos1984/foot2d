"""Coverage tracks imports, independently of successful structural validation."""
import json,re
from pathlib import Path
root=Path(__file__).resolve().parents[1]
m=json.loads((root/'data/france_2627_official.json').read_text(encoding='utf-8'))
geo=(root/'src/data_france_geo.cpp').read_text(encoding='utf-8')
regions=re.findall(r'\{"\d+", "([^"]+)"\}',geo.split('const int NUM_REGIONS')[0])
districts=re.findall(r'\{ "([^"]+)", "[^"]+", (\d+), "[^"]+", [\d.]+f? \}',(root/'src/districts.cpp').read_text(encoding='utf-8'))
groups=m['groups']
def coverage(selected):
    return {'groups':len(selected),'teams':sum(len(g['entries']) for g in selected),'tiers':sorted({g['tier'] for g in selected})}
out={'checked_on':m['checked_on'],'season':m['season'],'all_france_completed':False,
     'regional_leagues':[dict(name=name,**coverage([g for g in groups if g['region']==i and g['tier']<8])) for i,name in enumerate(regions)],
     'districts':[dict(name=name,region=regions[int(rg)],**coverage([g for g in groups if g['district']==name])) for name,rg in districts],
     'pending':{'Maine-et-Loire':'D5: entente Combrée Pouancé 3 affiliation unresolved',
                'Loire-Atlantique':'D5 team compositions not imported',
                'Finistère':'D4 calendars observed; ententes and full groups require reconciliation',
                'others':'Remaining uncovered leagues and districts use generated fallback teams; Grand Est and its nine senior districts imported, including actual Alsace D6/D7/D8 groups'}}
(root/'data/france_2627_coverage.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(len(out['regional_leagues']),'leagues,',len(out['districts']),'districts audited for coverage')
