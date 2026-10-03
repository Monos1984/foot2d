"""Extract the published columns without inventing missing teams."""
import re
from pathlib import Path
from pypdf import PdfReader
root=Path(__file__).resolve().parents[1]
lines=[]
for division,columns in [(1,3),(2,5)]:
    text=PdfReader(root/f'data/official/Groupes-D{division}-2026-2027.pdf').pages[0].extract_text(extraction_mode='layout')
    groups={};current=[]
    for row in text.splitlines():
        parts=re.split(r'\s{2,}',row.strip())
        if len(parts)==columns and all(re.fullmatch('[A-J]',p) for p in parts):
            current=parts
            for group in current:groups[group]=[]
            continue
        if not current:continue
        if len(parts)>columns or not all(re.fullmatch(r'.+ [1-8]',p) for p in parts):continue
        for group,label in zip(current,parts):
            name,squad=label.rsplit(' ',1)
            name=name.upper()
            if squad!='1':name+=' '+squad
            if name in ['GOURIN FC','GUISCRIFF AV','ARZANO JA','ROUDOUALLEC EC']:name+='@56'
            groups[group].append(name)
    expected={g:13 if division==2 and g=='A' else 12 for g in groups}
    assert len(groups)==(6 if division==1 else 10),groups
    assert all(len(groups[g])==n for g,n in expected.items()),[(g,len(v)) for g,v in groups.items()]
    lines.extend(f'{7+division}{group}|29|'+ ';'.join(names) for group,names in groups.items())
text=PdfReader(root/'data/official/Groupes-D3-2026-2027.pdf').pages[0].extract_text(extraction_mode='layout')
groups={};current=[]
for row in text.splitlines():
    parts=re.split(r'\s{2,}',row.strip())
    if len(parts) in [3,4] and all(re.fullmatch('[A-N]',p) for p in parts):
        current=parts
        for group in current:groups[group]=[]
        continue
    if not current:continue
    if row.strip().startswith('St Goazec Ste 1'):
        for group,label in zip(['H','J','K'],parts):groups[group].append(label)
    elif len(parts)==len(current) and all(re.fullmatch(r'.+ [1-8]',p) for p in parts):
        for group,label in zip(current,parts):groups[group].append(label)
assert len(groups)==14 and sum(map(len,groups.values()))==157,[(g,len(v)) for g,v in groups.items()]
for group,labels in groups.items():
    names=[]
    for label in labels:
        name,squad=label.rsplit(' ',1);name=name.upper()
        department='@56' if name in ['GOURIN FC','GUISCRIFF AV','ARZANO JA','ROUDOUALLEC EC'] else ''
        names.append(name+(' '+squad if squad!='1' else '')+department)
    lines.append(f'10{group}|29|'+ ';'.join(names))
(root/'data/official/finistere-2627.txt').write_text('\n'.join(lines)+'\n',encoding='utf-8')
