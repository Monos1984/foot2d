"""Rebuild the audited first import lot from the preserved official extracts.
No generated team is accepted as a parent. Unknown aliases stop the import.
"""
import json, re, unicodedata
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXTRACTS = ROOT / 'data' / 'official'
def key(s):
    return re.sub(r'[^A-Z0-9]', '', unicodedata.normalize('NFKD', s.upper()).encode('ascii', 'ignore').decode())
def lines(doc):
    out = {}
    for n, p, p2, text in re.findall(r'^L(\d+)@P(\d+)(?:-(\d+))?: (.*)$', (EXTRACTS / (doc+'-extracted.txt')).read_text(encoding='utf-8'), re.M):
        text = re.sub(r'^.*?SAISON 2026-2027', '', text)
        text = re.sub(r'^Descente de Départemental \d', '', text)
        text = re.split(r'GROUPES 2026/2027|Sous réserve', text)[0].strip()
        out[int(n)] = (int(p2 or p), text)
    return out

aliases = {}
def alias(target, *variants):
    for v in (target,)+variants: aliases[key(v)] = target
alias('VFC La-Roche-sur-Yon', 'VENDÉE FC LA ROCHE')
alias('Vendée Poiré-sur-Vie Football', 'LE POIRE SUR VIE VF')
alias('Voltigeurs Châteaubriant', 'CHATEAUBRIANT VOLT.')
alias('Les Herbiers VF', 'LES HERBIERS VF')
alias('Sablé', 'SABLÉ SUR SARTHE FC')
alias('Vertou', 'VERTOU USSA')
alias('Saint-Philbert-de-Grand-Lieu', 'ST PHILBERT GD LIEU')
alias('Olympique Saumur FC', 'SAUMUR OFC')
alias('Fontenay-le-Comte', 'FONTENAY VENDÉE')
alias('La Châtaigneraie', 'LA CHATAIGNERAIE AS')
alias('Challans', 'CHALLANS FC')
alias('Les Sables Vendée', 'LES SABLES VF')
alias('ST SEBASTIEN SUR LOIRE FC', 'ST SEBASTIEN /LOIRE FC')
alias('ORVAULT SPORT', 'ORVAULT SF')
alias('LA SUZE ROEZE FC', 'LA SUZE ROEZÉ')
alias('LE MANS VILLARET AS', 'LE MANS VILLARET')
alias('ST ANDRE ST MACAIRE FC', 'ST ANDRE ST MAC. FC')
alias('MAREUIL SUR LAY SPC', 'MAREUIL SC')
alias('LA ROCHE ESO VENDÉE', 'LA ROCHE /YON ESOFV')
alias('ST PIERRE MONTREV. AS', 'ST PIERRE MONTREVAULT AS')
alias('CHALONNES CHAUDEF.', 'CHALONNES CHAUDEFONDS FC', 'CHALONNES CHAUD')
alias('CHEMILLÉ MELAY O.', 'CHEMILLÉ MELAY')
alias('ST SYLVAIN ANJOU AS', "ST SYLVAIN D'ANJOU AS")
alias('LA POMMERAYE POMJ.', 'LA POMMERAYE POMJ JA', 'LA POMMERAYE POMJ')
alias('TOUT MAULEVRIER SP', 'TOUTMAULÉVRIER US', 'TOUT MAULÉVRIER US')
alias('ANGERS VAILLANTE', 'ANGERS VAILLANTE FC')
alias('ANGERS CBAF', 'ANGERS CROIX BLANCHE')
alias('ANGERS HAUTS ST AUBIN AC', 'ANGERS HSA')
alias('BÉCON VILLEMOISANT OBVA', 'BÉCON VILLEMOISAN OBVA')
alias('CANTENAY ÉPINARD ES', 'CANTENAY ÉPINARD US')
alias('CHAMPTOCÉ INGRANDES LFA', 'CHAMPTOCÉ INGRANDES')
alias('ST GERMAIN VAL DE MOINE FC', 'ST GERMAIN VAL MOINE FC')
alias('LA ROMAGNE ROUSSAY ES', 'LA ROMAGNE ROUSSAY')
alias('VILLEDIEU RENAUDIÈRE FC', 'VILLEDIEU RENAUDIÈRE')
alias('ST GEORGES TRÉMENTINES FC', 'ST GEORGES TREM FC')
alias('ANDREZE JUB JALLAIS FC', 'ANDREZÉ JUB JALLAIS')
alias('LE LONGERON TORFOU AS', 'LE LONGERON TORFOU')
alias('LE FUILET CHAUSSAIRE FC', 'LE FUILET CHAUSSAIRE')
alias('MAUGES SUR LOIRE FCEL', 'MAUGES SUR LOIRE FCLE')
alias('ANGRIE AS ST PIERRE', 'ANGRIE AS.ST P')
alias('ST MATH MÉNITRÉ', 'ST MATHURIN MÉNITRÉ')
alias('CHAMPTEUSSÉ ANJ BAC FC', 'CHAMPTEUSSÉ ANJ BAC')
alias('GENNES LES ROSIERS ES', 'GENNES LES ROSIERS')
alias('SAUMUR BAYARD AS', 'SAUMUR BAYARD')
alias('ST LAMBERT LEVÉES', 'ST LAMBERT DES LEV”ES')
alias('LES HAUTS ANJOU FC', "LES HAUTS D'ANJOU FC")
alias('CHRISTOPHESEGUINIERE', 'CHRISTOPHESÉGUINIÈRE')

groups = []
starts = [3,19,33,49,61,77,93,109,121,137,151,167,181,197,211,227]
depts = [
 '44 53 85 85 85 44 49 53 44 85 49 44',
 '44 44 72 72 85 44 49 72 85 85 85 49',
 '49 44 53 72 44 72 53 53 44 49 72 49',
 '44 72 49 44 44 72 85 72 85 49 49 44',
 '49 44 72 85 85 44 49 85 44 44 72 49',
 '49 49 85 49 85 44 85 49 44 44 44 44',
 '72 44 72 44 72 53 44 44 53 53 49 53',
 '44 85 53 53 53 44 44 85 44 44 85 44',
 '53 49 49 49 72 72 53 53 72 72 53 49',
 '53 49 49 49 72 53 72 53 53 72 72 49',
 '72 53 44 53 72 53 44 44 53 72 44 44',
 '49 53 72 53 72 49 72 49 53 49 53 72',
 '85 85 49 44 85 85 44 85 44 44 49 49',
 '49 85 49 85 44 44 44 44 85 49 85 44',
 '85 44 49 85 85 49 44 49 85 44 85 44',
 '85 85 85 44 85 85 44 44 44 44 44 85']
raw = lines('pdl')
for i, start in enumerate(starts):
    tier = 5 if i < 2 else 6 if i < 6 else 7
    gi = i if i < 2 else i-2 if i < 6 else i-6
    groups.append(dict(tier=tier, region=6, district='', group=chr(65+gi), source='pdl', page=raw[start][0]+1,
                       entries=[dict(published_name=raw[n][1], dept=d) for n,d in zip(range(start,start+12),depts[i].split())]))
for level in range(1,6):
    raw = lines('d'+str(level))
    for page in sorted({p for p,t in raw.values()}):
        texts = [t for n,(p,t) in sorted(raw.items()) if p==page]
        # The 12 team lines precede the page's group heading; EXEMPT is a bye.
        teams = texts[:texts.index('GROUPES SENIORS M')]
        assert len(teams)==12, (level,page,teams)
        groups.append(dict(tier=7+level, region=6, district='Maine-et-Loire', group=chr(65+page), source='d'+str(level), page=page+1,
                           entries=[dict(published_name=t,dept='49') for t in teams if t!='EXEMPT']))

known = {}
for g in groups:
    for e in g['entries']:
        m = re.search(r' (\d+)$',e['published_name'])
        squad = int(m[1]) if m else 1
        base = e['published_name'][:m.start()] if m else e['published_name']
        canonical = aliases.get(key(base),base)
        e.update(club=canonical, squad=squad, parent=canonical if squad>1 else '',
                 name=canonical+(' '+str(squad) if squad>1 else ''))
        if squad==1: known[key(canonical)] = canonical
for canonical in ['VFC La-Roche-sur-Yon','Vendée Poiré-sur-Vie Football','Voltigeurs Châteaubriant','Les Herbiers VF','Sablé','Vertou','Saint-Philbert-de-Grand-Lieu','Olympique Saumur FC','Fontenay-le-Comte','La Châtaigneraie','Challans','Les Sables Vendée']:
    known[key(canonical)] = canonical
# Accent/punctuation variations are resolved against a unique first team.
for g in groups:
    for e in g['entries']:
        c=known.get(key(e['club']),e['club'])
        e.update(club=c,parent=c if e['squad']>1 else '',name=c+(' '+str(e['squad']) if e['squad']>1 else ''))

pending=[]
for g in groups:
    missing=[e['published_name'] for e in g['entries'] if e['parent'] and key(e['parent']) not in known]
    if missing: pending.append(dict(source=g['source'],group=g['group'],unresolved=missing))
print('Unresolved groups:', json.dumps(pending, ensure_ascii=True))
# D5 stays pending until entente affiliations and all first teams are resolved.
active=[g for g in groups if g['tier']<12]
assert not any(e['parent'] and key(e['parent']) not in known for g in active for e in g['entries'])
sources = {
 'pdl': 'https://lfpl.fff.fr/wp-content/uploads/sites/20/2026/07/Groupes-Seniors-20262027.pdf',
 'd1': 'https://foot49.fff.fr/wp-content/uploads/sites/36/2026/07/Groupes-Seniors-D1-2026-2027.pdf',
 'd2': 'https://foot49.fff.fr/wp-content/uploads/sites/36/2026/07/Groupes-Seniors-D2-2026-2027-1.pdf',
 'd3': 'https://foot49.fff.fr/wp-content/uploads/sites/36/2026/07/Groupes-Seniors-D3-2026-2027-1.pdf',
 'd4': 'https://foot49.fff.fr/wp-content/uploads/sites/36/2026/07/Groupes-Seniors-D4-2026-2027-1.pdf',
 'd5': 'https://foot49.fff.fr/wp-content/uploads/sites/36/2026/09/Groupes-Seniors-D5-2026-2027.pdf',
}
manifest=dict(season='2026/2027', checked_on='2026-10-02', sources=sources, groups=active, pending_groups=[g for g in groups if g['tier']==12], issues=pending)
from official_expansion import extend
extend(manifest)
active=manifest['groups']
dest=ROOT/'data'; dest.mkdir(exist_ok=True)
(dest/'france_2627_official.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def q(s): return json.dumps(s,ensure_ascii=False)
rows=[]
for g in active:
    for e in g['entries']:
        rows.append('    {'+', '.join([q(e['name']),q(e['parent']),q(e['dept']),q(g['district']),str(g['region']),str(g['tier']),str(ord(g['group'])-65),str(e['squad'])])+ '},')
(ROOT/'src'/'data_france_official.cpp').write_text('#include "data.h"\n// Generated from data/france_2627_official.json; official publication labels.\nconst FrOfficialTeam FR_OFFICIAL_2627[] = {\n'+'\n'.join(rows)+'\n};\nconst int NUM_FR_OFFICIAL_2627 = sizeof(FR_OFFICIAL_2627)/sizeof(FrOfficialTeam);\n',encoding='utf-8')
print('Active:',len(active),'groups,',len(rows),'teams')

