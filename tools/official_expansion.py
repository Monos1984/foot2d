"""Audited additions from saved official images and current FFF standings.
Unknown affiliations are reported, never resolved to a generated club.
"""
import re, json, unicodedata
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def key(s):
    return re.sub('[^A-Z0-9]','',unicodedata.normalize('NFKD',s.upper()).encode('ascii','ignore').decode())
def extend(manifest):
    aliases={}
    def alias(target,*variants):
        for value in (target,)+variants: aliases[key(value)]=target
    for target,variants in {
        'US Concarneau':['US CONCARNEAU'], 'Milizac':['ST PIERRE MILIZAC'],
        'Vannes':['VANNES OC'], 'Lannion':['LANNION FC'], 'Dinan Léhon FC':['DINAN LÉHON FC'],
        'US Saint-Malo':['US SAINT MALO'], 'Vitré':['AS VITRÉ'],
        'Cesson':['OC CESSON'], 'Saint-Brieuc-Ginglin':['AS GINGLIN CESSON'],
        'Saint-Colomban Locminé':['ST CO LOCMINÉ'],
        'STADE PLABENNECOIS':['STADE PLABENNEC'], 'AS BREST':['AS BRESTOISE'],
        'PAOTRED DISPOUNT EG':['PAOTRED DISPOUNT'], 'AS VIGNOC HÉDÉ GUIPEL':['AS VIGNOC HÉDÉ GUI.'],
        'LA BAULE POULIGUEN US':['US LA BAULE LE POULIGUEN'],
        'Vertou':['USSA VERTOU'], 'LES SORINIERES ELAN':['ÉLAN SORINIÈRES FOOTBALL','SORINIERES ELAN F'],
        'ST SEBASTIEN SUR LOIRE FC':['ST-SÉBASTIEN FC','ST SEBASTIEN FC'],
        'GORGES ELAN':['ÉLAN DE GORGES'], 'GUÉRANDE MADELEINE':['AS LA MADELEINE'],
        'ST PÈRE EN RETZ SP':['LA ST-PIERRE DE RETZ','ST PERE EN RETZ'],
        'ST ANDRE DES EAUX':['LA ST-ANDRÉ'], 'PONTCHATEAU AOS':['AOS PONT-CHÂTEAU'],
        'ST MARC SUR MER FOOT':['ST-MARC FOOT'], 'MÉSANGER AS':['AS MÉSANGER','MESANGER AS'],
        'DERVAL SCNA':['SCNA DERVAL'], 'ERBRAY JEUNES':["LES JEUNES D'ERBRAY"],
        'LOIREAUXENCE VARADES':['US VARADES'], 'PLESSÉ DRESNY ES':['ES DRESNY-PLESSÉ','PLESSE DRESNY ES'],
        'NANTES BELLEVUE':['JSC BELLEVUE'], 'ORVAULT SPORT':['ORVAULT SF'],
        'VIEILLEVIGNE PLANCHE':['AS VIEILLEVIGNE LA PLANCHE'], 'ST FIACRE COTEAUX FC':['FC CÔTEAUX DU VIGNOBLE'],
        'ST JULIEN DIVATTE FC':['FC ST-JULIEN DIVATTE'], 'COUERON CHABOSSIERE':['COUËRON CHABOSSIÈRE FC'],
        'NANTES LA MELLINET':['LA MELLINET'], 'NANTES ST MED. DOULON':['ST-MÉDARD DE DOULON','NANTES ST MED DOULON'],
        'SAUTRON AS':['AS SAUTRON'], 'VIGNEUX ES':['ES VIGNEUX'], 'ST BREVIN AC':['AC ST-BREVIN'],
        'GUÉRANDE ST AUBIN':['LA ST-AUBIN DE GUÉRANDE'], 'LA CHAPELLE DES MARAIS':['FC LA CHAPELLE DES MARAIS'],
        'SAVENAY MALVILLE PFC':['SAVENAY MALVILLE PRINQUIAU FC'], 'BLAIN ES':['ES BLAIN'],
        'Voltigeurs Châteaubriant':['VOLTIGEURS CHÂTEAUBRIANT','CHATEAUBRIANT VOLTIG'],
        'CARQUEFOU USJA':['USJA CARQUEFOU'], 'ANCENIS RCASG':['RC ANCENIS ST GÉRÉON'],
        'NORT SUR ERDRE AC':['NORT AC'], 'TEILLÉ MOUZEIL LIGNÉ':['FC MOUZEIL TEILLÉ LIGNÉ','MOUZEIL TEILLE LIGNE'],
        'LA CHAPELLE / ERDRE AC':['AC CHAPELAIN','LA CHAPELLE AC CHAP'], 'REZÉ FC':['FC REZÉ'],
        'THOUARÉ US':['US THOUARÉENNE','THOUARE US'], 'SYMPHO FOOT TREILLIÈRES':['SF TREILLIÈRES'],
        'BASSE GOULAINE AC':['AC BASSE-GOULAINE'], 'LE LOROUX LANDREAU':['LOROUX LANDREAU OSC','LE LOROUX LANDREAU O'],
        'NANTES DON BOSCO':['DON BOSCO FOOTBALL NANTES'], 'Saint-Philbert-de-Grand-Lieu':['US PHILIBERTINE FOOTBALL'],
        'PONT ST MARTIN FCGL':['FC GRAND LIEU','PT ST MARTIN FCGL'], 'ENTENTE SPORTIVE DES MARAIS':['ES DES MARAIS'],
        'CLUB SPORTIF MONTOIRIN':['CS MONTOIRIN'], "L'ÉCLAIR DE CHAUVÉ":['ÉCLAIR DE CHAUVÉ'],
        'ST-JOSEPH DE PORTERIE FOOT':['ST JOSEPH DE PORTERIE FOOT'],
        'FC ENTENTE DU VIGNOBLE':['LA CHAP. HEULIN FCEV'],
        'PORNICHET ES':['ES PORNICHET'], 'BOUAYE FC':['FC BOUAYE'],
    }.items(): alias(target,*variants)
    for target,variants in {
      'PARNÉ SUR ROC AS':['PARNÉ S/ROC AS'], 'CRAON FC':['FC DU CRAONNAIS'],
      'SEVREMONT LA FLO CHA.':['SEVREMONT F.C.'], 'STE CECILE ST MARTIN':['STE-CECILE ST-MAR FC'],
      'AMBRIERES CIGNÉ FC':['AMBRIERES CIGNE F.'], 'ANDOUILLÉ AS':['ANDOUILLÉ AMS'],
      'ANGERS VAILLANTE':['ANGERS VAILLANTE FC'], 'BEAUFORT EN VALLÉE US':['BEAUFORT EN VALLEE'],
      'MARTIGNÉ /MAYENNE AS':['MARTIGNÉ S/MAY. AS'], 'ARNAGE PONLIEUE':['ARNAGE PONTLIEUE US'],
      'ÉVRON CA':['ÉVRONNAIS CA'], 'BONCHAMPS ES':['BONCHAMP ES'], 'SEGRE ESHA':['SEGRE ESHA FOOTBALL'],
      'BOURNEZEAU ST HILAIRE':['BOURNEZEAU ST HILAIR'], "L'HERMENAULT FCPB":['HERMENAULT FCPB'],
      'La Châtaigneraie':['LA CHATAIGNERAIE AS'], 'ST ANDRE ST MACAIRE FC':['ST ANDRE ST MACAIRE'],
      'ANDREZE JUB JALLAIS FC':['ANDREZE JUB-JALLAIS'], 'SALIGNY BELLEVIGNY FC':['FC SALIGNY'],
      'LE FUILET CHAUSSAIRE FC':['LE FUILET CHAUSSAIRE'], 'MAREUIL SUR LAY SPC':['MAREUIL SUR LAY'],
      'VFC La-Roche-sur-Yon':['LA ROCHE VENDEE F.'], 'ST HILAIRE VIHIERS AS':['ST HILAIRE VIHIERS'],
      'LES HERBIERS ARDELAY RS':['LES HERBIERS ARDELAY'], 'BRETIGNOLLES ESMBB':['BRETIGNOLLES BREM ES'],
      'Challans':['CHALLANS FC'], 'LES ACHARDS FC':['FC DES ACHARDS'], 'Les Sables Vendée':['LES SABLES VF'],
      'Saint-Philbert-de-Grand-Lieu':['ST PHILBERT GD LIEU'],
    }.items(): alias(target,*variants)
    for target,variants in {
      'ASPTT BREST':['BREST ASPTT'], 'AL COATAUDON':['GUIPAVAS COAT AL'],
      'Milizac':['MILIZAC ST P'], 'STADE PLABENNECOIS':['PLABENNEC ST'],
      'EA SAINT RENAN':['ST RENAN EA'], 'AS BREST':['BREST AS'],
      'GSY BOURG BLANC':['BOURG BLANC GAS'], 'DC CARHAIX':['CARHAIX DC'],
      'US MOËLAN':['MOELAN US'], 'AM ERGUÉ GABÉRIC':['ERGUE GABERIC AM'],
      "FC PONT L'ABBÉ":['PONT LABBE FC'], 'QUIMPER ERGUÉ ARMEL':['QUIMPER ERGUE ARMEL'],
      'PAOTRED DISPOUNT EG':['ERGUE GABERIC PAOT'], 'HERMINE CONCARNEAU':['CONCARNEAU HERMINE'],
      'CS PENMARCH':['PENMARCH CS'], 'PLOUZANÉ ACF':['PLOUZANE ACF'],
      'BREST SAINT LAURENT':['BREST ST LAURENT'], 'US CLÉDER':['CLEDER US'],
      'VGA BOHARS':['BOHARS VGA'], 'GDR GUIPAVAS':['GUIPAVAS GDR'],
      'ST LANDERNÉEN':['LANDERNEAU ST'], 'FC RELECQ-KERHUON':['LE RELECQ KERHUON FC'],
      'EF PLOUGOURVEST':['PLOUGOURVEST ET F'],
      'AS SIZUN LE TREHOU':['SIZUN LE TREHOU AS','AS SIZUN LE THÉHOU'],
      'GSM PLOUGUIN':['PLOUGUIN ST MAJAN'], 'AS PLOUVIEN':['PLOUVIEN AV S'],
      'AG PLOUVORN':['PLOUVORN AG'], 'ST LÉONARD KREISKER':['ST POL'],
      'ES SAINT THEGONNEC':['ST THEGONNEC ET S'], 'ES PORTSALL KERSAINT':['PORTSALL KERSAINT ES'],
      'CHÂTEAULIN FC':['CHATEAULIN FC'], 'SM DOUARNENEZ':['DOUARNENEZ STELLA'],
      'US TREGUNC':['TREGUNC US'], 'FG BANNALEC':['BANNALEC FG'],
      'QUIMPER KERF. FC':['QUIMPER KERFEUNTEUN'], 'ES MIGNONNE':['IRVILLAC ES MIGNONNE'],
      'LES GLAZIKS DE CORAY':['CORAY'], 'ST PLEYBENOIS':['PLEYBEN ST'],
      'AS PLOBANNALEC':['PLOBANNALEC AS'], 'US SAINT EVARZEC':['ST EVARZEC US'],
      'PLOÉOUR FC':['PLONEOUR FC'], 'EA SCAËR':['SCAER EA'],
      'US FOUESNANT':['FOUESNANT US'], 'LA PLOZEVETIENNE':['PLOZEVET'],
      'RC LESNEVEN':['LESNEVEN RC'], 'SC MORLAIX':['MORLAIX SC'],
      'SC LANNILIS':['LANNILIS SC'], 'JU PLOUGONVEN':['PLOUGONVEN JU'],
      'ESPE PLOUGUERNEAU':['PLOUGUERNEAU ESPE'], 'FC CAMARET PEN HIR':['CAMARET FC PEN HIR'],
      'ES CRANOU LE FAOU':['LE FAOU HANVEC ES'], 'PLOMODIERN GMH':['PLOMODIERN GMH'],
      'FC AVEN BELON':['RIEC FC AVEN BELON'],
    }.items(): alias(target,*variants)
    for target,variants in {
      'FC Rouen 1899':['FC ROUEN 1899'], 'Le Havre AC':['HAVRE AC'],
      'Quevilly Rouen Métropole':['QRM'], 'Granville':['US GRANVILLAISE'],
      'ASPTT Caen':['AS PTT CAEN'], 'Vire':['AF VIROIS'],
      'Saint-Lô':['FC ST LO MANCHE'], 'Trouville Deauville':['ASTDV'],
      'Dieppe':['FC DIEPPE'], 'Oissel':['CMS OISSEL'], 'Alençon':['US ALENCONNAISE 61'],
    }.items(): alias(target,*variants)
    for target,variants in {
      'Stade de Reims':['STADE DE REIMS'], 'US Thionville Lusitanos':['THIONVILLE LUSITANOS'],
      'Charleville Prix':['CHARLEVILLE PRIX OAM'], 'FCSR Haguenau':['HAGUENAU FCSR'],
      'SR Colmar':['COLMAR SR FA'], 'SAS Épinal':['EPINAL SAS'], 'ASC Biesheim':['BIESHEIM ASC'],
      'Thaon':['THAON ES'], 'Mulhouse':['MULHOUSE FC'], 'Metz Municipaux':['METZ APM FC'],
      'ST BRICE COURCELLES':['ST BRICE COURCELLES AS'],
      'REMOISE ESPERANCE':['REIMS ESPERANCE'], 'SEZANNAIS SC':['SEZANNE SC'],
      'ES DES COTEAUX SUD':['COTEAUX SUD ES'],
    }.items(): alias(target,*variants)
    corrections=[]
    for g in manifest['groups']:
        for e in g['entries']:
            if e['club']=='LA CHAPELLE HEUL. FCEV':
                corrections.append(dict(name=e['name'],old_dept=e['dept'],dept='44'))
                e.update(name='FC ENTENTE DU VIGNOBLE',club='FC ENTENTE DU VIGNOBLE',dept='44')
    original=[e for g in manifest['groups'] for e in g['entries']]
    byLabel={key(e['published_name']):e for e in original}
    byName={key(e['name']):e for e in original}
    # Current FFF group G contains the newly promoted Entente du Vignoble.
    # Match every other current label explicitly before replacing published groups.
    changes=[]
    for line in (ROOT/'data/official/pdl-r3-live-2627.txt').read_text(encoding='utf-8').splitlines():
        group,names=line.split('|'); out=[]
        old=next(g for g in manifest['groups'] if g['region']==6 and g['tier']==7 and g['group']==group)
        for label in names.split(';'):
            m=re.search(r' ([2-8])$',label);squad=int(m[1]) if m else 1;base=label[:m.start()] if m else label
            canonical=aliases.get(key(base),base); full=canonical+(' '+str(squad) if squad>1 else '')
            e=byLabel.get(key(label)) or byName.get(key(full))
            if not e:
                # Remaining differences are listed for explicit review, never fuzzy matched.
                raise ValueError('Unmapped live PDL label: '+label)
            out.append(dict(e,published_name=label))
        oldSet={e['name'] for e in old['entries']};newSet={e['name'] for e in out}
        if oldSet!=newSet:changes.append(dict(group=group,removed=sorted(oldSet-newSet),added=sorted(newSet-oldSet)))
        old.update(entries=out,source='pdl_r3_live',page=None)
    manifest['sources']['pdl_r3_live']='https://epreuves.fff.fr/competition/engagement/452589-regional-3/phase/1'
    for variant,target in json.loads((ROOT/'data/official/grandest-aliases.json').read_text(encoding='utf-8')).items():alias(target,variant)
    additions=[]
    for filename,region,district,source in [('bretagne-2627.txt',7,'','bretagne'),('loire-atlantique-2627.txt',6,'Loire-Atlantique','loire_atlantique'),('finistere-2627.txt',7,'Finistère','finistere'),('normandie-2627.txt',3,'','normandie'),('grandest-2627.txt',5,'','grandest'),('marne-2627.txt',5,'Marne','marne'),('alsace-2627.txt',5,'Alsace','alsace'),('moselle-2627.txt',5,'Moselle','moselle'),('hautemarne-2627.txt',5,'Haute-Marne','hautemarne'),('ardennes-2627.txt',5,'Ardennes','ardennes'),('aube-2627.txt',5,'Aube','aube'),('meurthemoselle-2627.txt',5,'Meurthe-et-Moselle','meurthemoselle'),('meuse-2627.txt',5,'Meuse','meuse'),('vosges-2627.txt',5,'Vosges','vosges')]:
        for line in (ROOT/'data/official'/filename).read_text(encoding='utf-8').splitlines():
            code,dept,labels=line.split('|');tier=int(code[:-1]);group=code[-1];entries=[]
            if district and source not in ['loire_atlantique','finistere','marne']:tier+=7
            for label in labels.split(';'):
                pieces=label.rsplit('@',1);label=pieces[0];department=pieces[1] if len(pieces)>1 else dept
                m=re.search(r' ([2-8])$',label);squad=int(m[1]) if m else 1;base=label[:m.start()] if m else label
                canonical=aliases.get(key(base),base)
                entries.append(dict(published_name=label,dept=department,club=canonical,squad=squad,parent=canonical if squad>1 else '',name=canonical+(' '+str(squad) if squad>1 else '')))
            additions.append(dict(tier=tier,region=region,district=district,group=group,source=source,page=None,entries=entries))
    manifest['sources'].update(bretagne='https://footbretagne.fff.fr/simple/les-groupes-pour-la-saison-2026-2027/',loire_atlantique='https://foot44.fff.fr/simple/seniors-les-groupes-seniors-masculins-feminines/')
    manifest['sources']['finistere']='https://foot29.fff.fr/simple/groupes-d1-d2-saison-2026-2027/'
    for t,id in [(1,453390),(2,453391),(3,453392)]:
        manifest['sources']['normandie_r'+str(t)]=f'https://epreuves.fff.fr/competition/engagement/{id}/phase/1'
    manifest['sources']['normandie']='https://normandie.fff.fr/'
    manifest['sources']['grandest']='https://lgef.fff.fr/simple/championnats-regionaux-les-calendriers-detailles-26-27/'
    manifest['sources']['marne']='https://epreuves.fff.fr/ligue-et-district/84'
    manifest['sources'].update(alsace='https://alsace.fff.fr/simple/seniors-la-composition-des-groupes-saison-2026-2027/',moselle='https://moselle.fff.fr/simple/groupes-2026-2027-definitifs/',hautemarne='https://epreuves.fff.fr/ligue-et-district/68',ardennes='https://epreuves.fff.fr/ligue-et-district/47',aube='https://epreuves.fff.fr/ligue-et-district/13',meurthemoselle='https://epreuves.fff.fr/ligue-et-district/85',meuse='https://epreuves.fff.fr/ligue-et-district/86',vosges='https://vosges.fff.fr/simple/groupes-seniors-saison-2026-2027/')
    known={key(e['name']):e['name'] for g in manifest['groups']+additions for e in g['entries'] if e['squad']==1}
    for line in (ROOT/'src/data_france_clubs.cpp').read_text(encoding='utf-8').splitlines():
        if re.match(r'\s*[FR]\(',line):
            strings=re.findall(r'"([^"]*)"',line)
            if strings[-1]=='':known[key(strings[0])]=strings[0]
    issues=[]
    for g in additions:
        for e in g['entries']:
            if e['parent']:
                parent=known.get(key(e['parent']))
                if not parent:issues.append(e['parent'])
                else:e.update(parent=parent,club=parent,name=parent+' '+str(e['squad']))
    if issues:raise ValueError('Unresolved parents: '+json.dumps(sorted(set(issues)),ensure_ascii=False))
    manifest['groups']+=additions;manifest['corrections']=changes+corrections
    names=[e['name'] for g in manifest['groups'] for e in g['entries']]
    if len(names)!=len(set(names)):raise ValueError('Duplicate imported teams')
