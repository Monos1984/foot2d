#!/usr/bin/env python3
# Génère src/data_players.cpp et src/data_clubs.cpp depuis la base EA FC 26 (players.csv)
import csv, collections, re, sys, os, hashlib
sys.path.insert(0, os.path.dirname(__file__))
from club_meta import M as META
from euro_pool import POOL

SRC = os.path.join(os.path.dirname(__file__), '..', 'src')
CSV = sys.argv[1] if len(sys.argv) > 1 else '/tmp/EAFC26-DataHub/data/players.csv'
rows = list(csv.DictReader(open(CSV, encoding='utf-8-sig')))

# ordre des nations (data_nations.cpp)
codes = re.findall(r'N\("([A-Z]{3})"', open(os.path.join(SRC, 'data_nations.cpp'), encoding='utf-8').read())
NIDX = {c: i for i, c in enumerate(codes)}
EN = {
 'Afghanistan':'AFG','Albania':'ALB','Algeria':'ALG','Andorra':'AND','Angola':'ANG','Antigua and Barbuda':'ATG','Argentina':'ARG',
 'Armenia':'ARM','Australia':'AUS','Austria':'AUT','Azerbaijan':'AZE','Bangladesh':'BAN','Barbados':'BRB','Belarus':'BLR','Belgium':'BEL',
 'Benin':'BEN','Bermuda':'BER','Bolivia':'BOL','Bosnia and Herzegovina':'BIH','Brazil':'BRA','Bulgaria':'BUL','Burkina Faso':'BFA',
 'Burundi':'BDI','Cabo Verde':'CPV','Cameroon':'CMR','Canada':'CAN','Central African Republic':'CTA','Chad':'CHA','Chile':'CHI',
 'China PR':'CHN','Chinese Taipei':'TPE','Colombia':'COL','Comoros':'COM','Congo':'CGO','Congo DR':'COD','Costa Rica':'CRC',
 'Croatia':'CRO','Cuba':'CUB','Curacao':'CUW','Cyprus':'CYP','Czechia':'CZE',"Côte d'Ivoire":'CIV','Denmark':'DEN',
 'Dominican Republic':'DOM','Ecuador':'ECU','Egypt':'EGY','El Salvador':'SLV','England':'ENG','Equatorial Guinea':'EQG','Estonia':'EST',
 'Faroe Islands':'FRO','Finland':'FIN','France':'FRA','Gabon':'GAB','Gambia':'GAM','Georgia':'GEO','Germany':'GER','Ghana':'GHA',
 'Gibraltar':'GIB','Greece':'GRE','Grenada':'GRN','Guatemala':'GUA','Guinea':'GUI','Guinea-Bissau':'GNB','Guyana':'GUY','Haiti':'HAI',
 'Honduras':'HON','Hong Kong':'HKG','Hungary':'HUN','Iceland':'ISL','India':'IND','Indonesia':'IDN','Iran':'IRN','Iraq':'IRQ',
 'Israel':'ISR','Italy':'ITA','Jamaica':'JAM','Japan':'JPN','Jordan':'JOR','Kenya':'KEN','Korea Republic':'KOR','Kosovo':'KVX',
 'Latvia':'LVA','Lebanon':'LBN','Liberia':'LBR','Libya':'LBY','Liechtenstein':'LIE','Lithuania':'LTU','Luxembourg':'LUX',
 'Madagascar':'MAD','Malawi':'MWI','Malaysia':'MAS','Mali':'MLI','Malta':'MLT','Mauritania':'MTN','Mexico':'MEX','Moldova':'MDA',
 'Montenegro':'MNE','Montserrat':'MSR','Morocco':'MAR','Mozambique':'MOZ','Namibia':'NAM','Netherlands':'NED','New Caledonia':'NCL',
 'New Zealand':'NZL','Niger':'NIG','Nigeria':'NGA','North Macedonia':'MKD','Northern Ireland':'NIR','Norway':'NOR','Pakistan':'PAK',
 'Palestine':'PLE','Panama':'PAN','Paraguay':'PAR','Peru':'PER','Philippines':'PHI','Poland':'POL','Portugal':'POR','Puerto Rico':'PUR',
 'Qatar':'QAT','Republic of Ireland':'IRL','Romania':'ROU','Russia':'RUS','Rwanda':'RWA','Saint Kitts and Nevis':'SKN','Saint Lucia':'LCA',
 'Saudi Arabia':'KSA','Scotland':'SCO','Senegal':'SEN','Serbia':'SRB','Sierra Leone':'SLE','Slovakia':'SVK','Slovenia':'SVN','Somalia':'SOM',
 'South Africa':'RSA','Spain':'ESP','Sri Lanka':'SRI','Suriname':'SUR','Sweden':'SWE','Switzerland':'SUI','Syria':'SYR','Tajikistan':'TJK',
 'Tanzania':'TAN','Thailand':'THA','Togo':'TOG','Trinidad and Tobago':'TRI','Tunisia':'TUN','Türkiye':'TUR','Uganda':'UGA','Ukraine':'UKR',
 'United Arab Emirates':'UAE','United States':'USA','Uruguay':'URU','Uzbekistan':'UZB','Vanuatu':'VAN','Venezuela':'VEN','Wales':'WAL',
 'Yemen':'YEM','Zambia':'ZAM','Zimbabwe':'ZIM',
}
missing = set(r['nationality_name'] for r in rows) - set(EN) - {''}
if missing: print('nations inconnues :', missing)

def attr(v):
    try: v = int(float(v))
    except: return 10
    return max(5, min(99, round(v * 1.3 - 25)))

def posOf(p):
    p = p.split(',')[0].strip()
    if p == 'GK': return 0
    if p in ('CB', 'LB', 'RB', 'LWB', 'RWB'): return 1
    if p in ('CDM', 'CM', 'CAM', 'LM', 'RM'): return 2
    return 3

def cstr(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'

ORDER = {'SUB': 1, 'RES': 2, '': 3}
clubs = collections.OrderedDict()
for r in rows:
    cn = r['club_name']
    if not cn: continue
    clubs.setdefault(cn, []).append(r)

players_out = []
dbclubs = []
club_rating = {}
for cn, lst in clubs.items():
    lst.sort(key=lambda r: (ORDER.get(r['club_position'], 0), -int(r['overall'])))
    lst = lst[:30]
    first = len(players_out)
    for r in lst:
        pos = posOf(r['player_positions'])
        gk = [r['goalkeeping_diving'], r['goalkeeping_handling'], r['goalkeeping_reflexes'], r['goalkeeping_positioning']]
        try: keep = attr(sum(float(x) for x in gk) / 4) if pos == 0 else 10
        except: keep = 10
        spd = attr(r['movement_sprint_speed'] or r['pace'] or 50) if pos else attr(r['movement_sprint_speed'] or 45)
        sho = attr(r['shooting'] or r['attacking_finishing'] or 30)
        pas = attr(r['passing'] or r['attacking_short_passing'] or 40)
        tac = attr(r['defending'] or r['defending_standing_tackle'] or 30)
        sta = attr(r['power_stamina'] or 60)
        num = r['club_jersey_number']
        try: num = int(float(num))
        except: num = 0
        nat = NIDX.get(EN.get(r['nationality_name'], ''), -1)
        nt = 1 if r['nation_position'] else 0
        ovr = int(r['overall'])
        try: age = int(float(r['age']))
        except: age = 25
        try: pot = int(float(r['potential']))
        except: pot = ovr
        players_out.append((r['short_name'], pos, num, spd, sho, pas, tac, keep, sta, nat, nt, ovr, age, attr(pot)))
    ov = sorted((int(r['overall']) for r in lst), reverse=True)
    avg = sum(ov[:14]) / max(1, min(14, len(ov)))
    rating = round(90 - (86 - avg) * 1.5)
    club_rating[cn] = rating
    dbclubs.append((cn, first, len(players_out) - first, lst[0]['league_name'], rating))

# notes des sélections
natp = collections.defaultdict(list)
for p in players_out:
    if p[9] >= 0: natp[p[9]].append(p)
nation_rating = [-1] * len(codes)
for n, lst in natp.items():
    lst.sort(key=lambda p: (-p[10], -p[11]))
    top = sorted((p[11] for p in lst[:26]), reverse=True)[:14]
    if len(top) >= 11:
        nation_rating[n] = round(90 - (86 - sum(top) / len(top)) * 1.5)

with open(os.path.join(SRC, 'data_players.cpp'), 'w', encoding='utf-8') as f:
    f.write('// Généré par tools/gen_data.py depuis la base EA SPORTS FC 26 (effectifs réels 2025-2026)\n#include "data.h"\n\n')
    f.write('const PlayerRec PLAYERS[] = {\n')
    for p in players_out:
        f.write('  {%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d},\n' % (cstr(p[0]), *p[1:]))
    f.write('};\nconst int NUM_PLAYERS = %d;\n\n' % len(players_out))
    f.write('const DbClub DBCLUBS[] = {\n')
    for c in dbclubs:
        f.write('  {%s,%d,%d,%d},\n' % (cstr(c[0]), c[1], c[2], c[4]))
    f.write('};\nconst int NUM_DBCLUBS = %d;\n\n' % len(dbclubs))
    f.write('const int NATION_DB_RATING[] = {' + ','.join(str(x) for x in nation_rating) + '};\n')

# ---------------------------------------------------------------- championnats
PAT = {'P': 'KP_PLAIN', 'V': 'KP_VSTRIPES', 'H': 'KP_HOOPS', 'M': 'KP_HALVES', 'S': 'KP_SASH', 'C': 'KP_CHECK', 'L': 'KP_SLEEVES'}
PALETTE = [0xE2001A, 0x0055A4, 0x00843D, 0xFFE500, 0xFFFFFF, 0x000000, 0xF47920, 0x7B1E2B, 0x3FA9F5, 0x5B2C83, 0x1B2A63]
def meta(cn):
    if cn in META: return META[cn]
    h = int(hashlib.md5(cn.encode()).hexdigest(), 16)
    a = PALETTE[h % 11]; b = PALETTE[(h // 11) % 11]
    if b == a: b = 0xFFFFFF if a != 0xFFFFFF else 0x0055A4
    sn = ''.join(ch for ch in cn.upper() if 'A' <= ch <= 'Z')[:3] or 'CLB'
    return (cn, sn, a, b, a, 'P', '')

LEAGUES = [  # id, nom, pays, niveau, nom de la ligue dans la base, (id base), culture, tiebreak
 ('ENG1', 'Premier League', 'ENG', 1, 'Premier League', '13.0', 'CU_EN'),
 ('ENG2', 'Championship', 'ENG', 2, 'Championship', '14.0', 'CU_EN'),
 ('ENG3', 'League One', 'ENG', 3, 'League One', '60.0', 'CU_EN'),
 ('ENG4', 'League Two', 'ENG', 4, 'League Two', '61.0', 'CU_EN'),
 ('ESP1', 'LaLiga', 'ESP', 1, 'La Liga', '53.0', 'CU_ES'),
 ('ESP2', 'LaLiga 2', 'ESP', 2, 'La Liga 2', '54.0', 'CU_ES'),
 ('ITA1', 'Serie A', 'ITA', 1, 'Serie A', '31.0', 'CU_IT'),
 ('ITA2', 'Serie B', 'ITA', 2, 'Serie B', '32.0', 'CU_IT'),
 ('GER1', 'Bundesliga', 'GER', 1, 'Bundesliga', '19.0', 'CU_DE'),
 ('GER2', '2. Bundesliga', 'GER', 2, '2. Bundesliga', '20.0', 'CU_DE'),
 ('GER3', '3. Liga', 'GER', 3, '3. Liga', '2076.0', 'CU_DE'),
 ('POR1', 'Liga Portugal', 'POR', 1, 'Primeira Liga', '308.0', 'CU_PT'),
 ('NED1', 'Eredivisie', 'NED', 1, 'Eredivisie', '10.0', 'CU_NL'),
 ('BEL1', 'Pro League', 'BEL', 1, 'Pro League', '4.0', 'CU_NL'),
 ('SCO1', 'Premiership', 'SCO', 1, 'Premiership', '50.0', 'CU_EN'),
 ('TUR1', 'Süper Lig', 'TUR', 1, 'Süper Lig', '68.0', 'CU_TR'),
 ('AUT1', 'Bundesliga (Autriche)', 'AUT', 1, 'Bundesliga', '80.0', 'CU_DE'),
 ('SUI1', 'Super League', 'SUI', 1, 'Super League', '189.0', 'CU_DE'),
 ('DEN1', 'Superliga', 'DEN', 1, 'Superliga', '1.0', 'CU_NORD'),
 ('NOR1', 'Eliteserien', 'NOR', 1, 'Eliteserien', '41.0', 'CU_NORD'),
 ('SWE1', 'Allsvenskan', 'SWE', 1, 'Allsvenskan', '56.0', 'CU_NORD'),
 ('POL1', 'Ekstraklasa', 'POL', 1, 'Ekstraklasa', '66.0', 'CU_SLAV'),
 ('ROU1', 'Liga I', 'ROU', 1, 'Liga I', '330.0', 'CU_BALK'),
 ('IRL1', 'Premier Division', 'IRL', 1, 'Premier Division', '65.0', 'CU_EN'),
 # reste du monde
 ('USA1', 'Major League Soccer', 'USA', 1, 'Major League Soccer', '39.0', 'CU_EN'),
 ('ARG1', 'Liga Profesional', 'ARG', 1, 'Liga Profesional de Fútbol', '353.0', 'CU_LATAM'),
 ('BRA1', 'Brasileirão', 'BRA', 1, 'Série A', '7.0', 'CU_BR'),
 ('KSA1', 'Saudi Pro League', 'KSA', 1, 'Pro League', '350.0', 'CU_ARAB'),
 ('KOR1', 'K League 1', 'KOR', 1, 'K League 1', '83.0', 'CU_KR'),
 ('CHN1', 'Chinese Super League', 'CHN', 1, 'Super League', '2012.0', 'CU_CN'),
 ('AUS1', 'A-League', 'AUS', 1, 'A-League Men', '351.0', 'CU_EN'),
 ('IND1', 'Indian Super League', 'IND', 1, 'Super League', '2149.0', 'CU_IND'),
]
league_clubs = collections.defaultdict(list)
for r in rows:
    league_clubs[(r['league_name'], r['league_id'])].append(r['club_name'])

def clubdef(cn, dept='""'):
    d = meta(cn)
    return '    { %s, %s, %d, 0x%06X, 0x%06X, 0x%06X, %s, %s, %s, %s },\n' % (
        cstr(d[0]), cstr(d[1]), club_rating.get(cn, 50), d[2], d[3], d[4], PAT[d[5]], dept, cstr(d[6]), cstr(cn))

with open(os.path.join(SRC, 'data_clubs.cpp'), 'w', encoding='utf-8') as f:
    f.write('// Généré par tools/gen_data.py : championnats (compositions 2025-2026, base EA SPORTS FC 26)\n#include "data.h"\n\n')
    for lid, name, country, tier, lname, lidd, cu in LEAGUES:
        cl = []
        for c in league_clubs[(lname, lidd)]:
            if c not in cl: cl.append(c)
        cl.sort(key=lambda c: meta(c)[0])
        f.write('static const ClubDef L_%s[] = {\n' % lid)
        for c in cl: f.write(clubdef(c))
        f.write('};\n')
    f.write('\nconst LeagueDef LEAGUES[] = {\n')
    for lid, name, country, tier, lname, lidd, cu in LEAGUES:
        f.write('    { "%s", %s, "%s", %d, L_%s, sizeof(L_%s) / sizeof(ClubDef), %s },\n' % (lid, cstr(name), country, tier, lid, lid, cu))
    f.write('};\nconst int NUM_LEAGUES = sizeof(LEAGUES) / sizeof(LEAGUES[0]);\n\n')
    # clubs européens hors championnats simulés
    f.write('const ClubDef EURO_POOL[] = {\n')
    for country in sorted(POOL):
        for (n, sn, rt, a, b, sh, pat, st, key) in POOL[country]:
            rating = club_rating.get(key, rt) if key else rt
            f.write('    { %s, %s, %d, 0x%06X, 0x%06X, 0x%06X, %s, "%s", %s, %s },\n' % (cstr(n), cstr(sn), rating, a, b, sh, PAT[pat], country, cstr(st), cstr(key)))
    f.write('    { "FC Vaduz", "VAD", 48, 0xE00000, 0x0033A0, 0xFFFFFF, KP_PLAIN, "LIE", "Rheinpark Stadion", "" },\n')
    f.write('};\nconst int NUM_EURO_POOL = sizeof(EURO_POOL) / sizeof(EURO_POOL[0]);\n\n')
    # autres clubs du monde présents dans la base (championnats incomplets)
    used = set()
    for lid, name, country, tier, lname, lidd, cu in LEAGUES:
        for c in league_clubs[(lname, lidd)]: used.add(c)
    for country in POOL:
        for x in POOL[country]:
            if x[8]: used.add(x[8])
    f.write('const ClubDef WORLD_POOL[] = {\n')
    WORLD_COUNTRY = {'Liga 1': 'PER', 'Primera Division': 'CHI', 'División Profesional': 'PAR', 'Primera División': 'URU',
                     'Categoría Primera A': 'COL', 'Serie A': 'ECU', 'División de Fútbol Profesional': 'BOL', 'Pro League': 'UAE'}
    for (lname, lidd), cl in sorted(league_clubs.items()):
        for c in sorted(set(cl)):
            if c in used or not c: continue
            cc = WORLD_COUNTRY.get(lname, '')
            if lidd == '2019.0': cc = 'VEN'
            if cc == '': continue
            used.add(c)
            f.write(clubdef(c, '"%s"' % cc))
    f.write('};\nconst int NUM_WORLD_POOL = sizeof(WORLD_POOL) / sizeof(WORLD_POOL[0]);\n')
print('joueurs', len(players_out), 'clubs base', len(dbclubs))
