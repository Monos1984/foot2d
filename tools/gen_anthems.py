#!/usr/bin/env python3
# Génère src/anthems_data.inc à partir des partitions du paquet npm « anthem-scores » (tools/anthems/*.json).
# Les mélodies sont dans le domaine public ; les données extraites gardent la licence de leur source (voir CREDITS.md).
import json, os
HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, 'anthems')
OUT = os.path.join(HERE, '..', 'src', 'anthems_data.inc')
# ISO 3166-1 alpha-2 -> codes des sélections du jeu
MAP = { 'AD': 'AND', 'AL': 'ALB', 'AT': 'AUT', 'AU': 'AUS', 'BE': 'BEL', 'CA': 'CAN', 'CH': 'SUI', 'CN': 'CHN', 'CZ': 'CZE', 'DE': 'GER',
        'DK': 'DEN', 'ES': 'ESP', 'FI': 'FIN', 'FR': 'FRA', 'GB': 'ENG NIR LIE', 'GR': 'GRE', 'HR': 'CRO', 'HU': 'HUN', 'ID': 'IDN', 'IL': 'ISR',
        'IN': 'IND', 'IT': 'ITA', 'JP': 'JPN', 'MX': 'MEX', 'NL': 'NED', 'PK': 'PAK', 'PL': 'POL', 'RU': 'RUS', 'SE': 'SWE', 'SK': 'SVK',
        'SM': 'SMR', 'UA': 'UKR', 'US': 'USA', 'ZA': 'RSA TAN ZAM' }
def F(x):
    t = '%g' % x
    return t + ('' if ('.' in t or 'e' in t) else '.') + 'f'
def esc(s): return s.replace('\\', '\\\\').replace('"', '\\"')
out = ['// Généré par tools/gen_anthems.py — ne pas modifier à la main.',
       '// Début des hymnes nationaux : notes converties de partitions libres (Wikipédia, Wikimedia Commons, Wikisource)',
       '// par le paquet « anthem-scores » ; source, auteur et licence de chaque partition dans le tableau ci-dessous.', '']
defs = []
for cc in sorted(MAP):
    d = json.load(open(os.path.join(SRC, cc + '.json'), encoding='utf-8'))
    n = cc.lower()
    out.append('static const RNote RA_%s_M[] = { %s };' % (n, ', '.join('{ %s, %d, %s }' % (F(b), p, F(l)) for b, p, l in d['melody'])))
    if d['bass']: out.append('static const RNote RA_%s_B[] = { %s };' % (n, ', '.join('{ %s, %d, %s }' % (F(b), p, F(l)) for b, p, l in d['bass'])))
    if d['inner']:
        items = []
        for b, ps, l in d['inner']:
            ps = (ps + [0, 0])[:2] if len(ps) else [0, 0]
            items.append('{ %s, { %d, %d }, %s }' % (F(b), ps[0], ps[1], F(l)))
        out.append('static const RChord RA_%s_I[] = { %s };' % (n, ', '.join(items)))
    title = d['title']
    if cc == 'CN': title = 'March of the Volunteers'
    if cc == 'GR': title = 'Hymne à la Liberté'
    if cc == 'IL': title = 'Hatikvah'
    if cc == 'RU': title = 'Hymne de la Fédération de Russie'
    if cc == 'UA': title = 'Chtche ne vmerla Oukraïny'
    bpm = d['bpm'] if not d['tempoGuessed'] else 0
    defs.append('    { "%s", "%s", "%s", "%s", "%s", %g, %d, %d, %g, %g, RA_%s_M, %d, %s, %d, %s, %d },' % (
        MAP[cc], esc(title), esc(d['composer']), esc(d['license']), esc(d['source']), bpm, d['key']['tonic'], 1 if d['key']['mode'] == 'minor' else 0,
        d['lengthBeats'], d['beatsPerBar'], n, len(d['melody']),
        'RA_%s_B' % n if d['bass'] else 'nullptr', len(d['bass']), 'RA_%s_I' % n if d['inner'] else 'nullptr', len(d['inner'])))
out.append('')
out.append('static const RealAnthem REAL_ANTHEMS[] = {')
out += defs
out.append('};')
out.append('static const int NUM_REAL_ANTHEMS = (int)(sizeof REAL_ANTHEMS / sizeof REAL_ANTHEMS[0]);')
open(OUT, 'w', encoding='utf-8').write('\n'.join(out) + '\n')
print('écrit', OUT, len(defs), 'hymnes')
