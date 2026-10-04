#!/usr/bin/env python3
# Convertit la partition LilyPond de La Marseillaise (tools/marseillaise/marseillaise.ly, CC0,
# https://github.com/jeandeaual/lilypond-piano-la-marseillaise) en notes pour le jeu : src/marseillaise_data.inc
# Sous-ensemble de LilyPond géré : \relative, notes, accords < >, silences, durées pointées, liaisons ~,
# \tuplet, \repeat unfold / tremolo, polyphonie << { } \\ { } >>. Le reste (nuances, doigtés...) est ignoré.
import re, os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, 'marseillaise', 'marseillaise.ly')
OUT = os.path.join(HERE, '..', 'src', 'marseillaise_data.inc')
TRANSPOSE = 3          # sol majeur -> si bémol majeur (tonalité habituelle des fanfares)
text = open(SRC, encoding='utf-8').read()
text = re.sub(r'%.*', '', text)
def block(name):
    i = text.index(name + ' = \\relative')
    j = text.index('{', i)
    depth, k = 0, j
    while True:
        if text[k] == '{': depth += 1
        elif text[k] == '}':
            depth -= 1
            if depth == 0: break
        k += 1
    ref = re.match(r"\\relative\s+([a-g](?:is|es)*[',]*)", text[i + len(name) + 3:]).group(1)
    return ref, text[j + 1:k]
STEP = { 'c': 0, 'd': 1, 'e': 2, 'f': 3, 'g': 4, 'a': 5, 'b': 6 }
SEMI = [0, 2, 4, 5, 7, 9, 11]
def pitch_of(name, octs, ref):
    # notation relative : la note la plus proche (à moins d'une quarte) de la note de référence
    step = STEP[name[0]]
    alt = name.count('is') - name.count('es')
    rstep, roct = ref
    best = None
    for o in range(roct - 1, roct + 2):
        d = (o * 7 + step) - (roct * 7 + rstep)
        if best is None or abs(d) < abs(best[0]): best = (d, o)
    o = best[1] + octs.count("'") - octs.count(',')
    return (step, o), 12 * (o + 1) + SEMI[step] + alt
def absolute(name_octs):
    m = re.match(r"([a-g](?:is|es)*)([',]*)", name_octs)
    return (STEP[m.group(1)[0]], 3 + m.group(2).count("'") - m.group(2).count(','))   # c = do3 (MIDI 48), c' = do4 (MIDI 60)
def run(name):
    ref, body = block(name)
    body = re.sub(r'\\clef\s+\w+|\\key\s+\w+\s+\\\w+|\\time\s+\d+/\d+', ' ', body)   # mots qui ne sont pas des notes
    body = re.sub(r'\\[<>!]', ' ', body)                                   # nuances \< \> \! (pas des accords)
    body = body.replace('>>', ' ⟫ ').replace('<<', ' ⟪ ')
    body = re.sub(r'>(\d+)(\.*)', lambda m: '> ⏱' + m.group(1) + m.group(2) + ' ', body)   # durée collée à un accord
    body = body.replace('⟫', '>>').replace('⟪', '<<')
    return ref, body
def parse_voice(name):
    ref, body = run(name)
    toks = []
    for m in re.finditer(r"""\\tuplet\s+(\d+)/(\d+)|\\repeat\s+(unfold|tremolo)\s+(\d+)|\\tweakWholeNoteTremolo\s+\#'\([^)]*\)|\\[a-zA-Z]+|<<|>>|\\\\|⏱(\d+)(\.*)|[{}<>~]|\^"[^"]*"|"[^"]*"|([a-g](?:is|es)*|[rs])([',]*)!?(\d*)(\.*)""", body):
        g = m.group(0)
        if m.group(1): toks.append(('tuplet', int(m.group(1)), int(m.group(2))))
        elif m.group(3): toks.append(('repeat', m.group(3), int(m.group(4))))
        elif m.group(5): toks.append(('cdur', m.group(5), m.group(6)))
        elif m.group(7) is not None: toks.append(('note', m.group(7), m.group(8), m.group(9), m.group(10)))
        elif g in ('<<', '>>', '\\\\', '{', '}', '<', '>', '~'): toks.append((g,))
    pos = [0]
    st = { 'ref': absolute(ref), 'dur': 1.0, 'scale': 1.0, 'tie': False }
    ev = []
    def peek(): return toks[pos[0]] if pos[0] < len(toks) else None
    def nxt(): pos[0] += 1; return toks[pos[0] - 1]
    def setdur(d, dots):
        if d:
            v = 4.0 / int(d); add = v
            for _ in dots: add /= 2; v += add
            st['dur'] = v
        return st['dur'] * st['scale']
    def emit(t, ms, d):
        tie_in = st['tie']; st['tie'] = False
        if peek() is not None and peek()[0] == '~': nxt(); tie_out = True
        else: tie_out = False
        if tie_in and ev and sorted(ev[-1][1]) == sorted(ms) and abs(ev[-1][0] + ev[-1][2] - t) < 1e-6: ev[-1][2] += d
        else: ev.append([t, ms, d])
        st['tie'] = tie_out
    def seq(t):
        while peek() is not None and peek()[0] != '}': t = elem(t)
        if peek() is not None: nxt()
        return t
    def elem(t):
        tok = nxt(); k = tok[0]
        if k == '{': return seq(t)
        if k == 'tuplet':
            old = st['scale']; st['scale'] = old * tok[2] / tok[1]
            if peek()[0] == '{': nxt(); t = seq(t)
            st['scale'] = old; return t
        if k == 'repeat':
            # LilyPond lit le corps répété une seule fois (hauteurs relatives fixées), puis le recopie :
            # on duplique donc les événements au lieu de les relire (sinon chaque reprise monte d'une octave)
            n = tok[2]
            if peek()[0] != '{': return t
            nxt(); e0 = len(ev); t0 = t; t = seq(t); span = t - t0
            body_ev = [list(e) for e in ev[e0:]]
            for r in range(1, n):
                for e in body_ev: ev.append([e[0] + span * r, list(e[1]), e[2]])
            return t0 + span * n
        if k == '<<':
            ref0 = st['ref']; ends = []; firstRef = None
            while peek() is not None and peek()[0] != '>>':
                if peek()[0] == '\\\\': nxt(); continue
                st['ref'] = ref0; ends.append(elem(t))
                if firstRef is None: firstRef = st['ref']
            nxt(); st['ref'] = firstRef or ref0
            return max(ends) if ends else t
        if k == '<':
            notes = []; r = st['ref']; first = None
            while peek()[0] != '>':
                tk = nxt()
                if tk[0] == 'note' and tk[1] not in ('r', 's'):
                    r, mm = pitch_of(tk[1], tk[2], r); notes.append(mm)
                    if first is None: first = r
            nxt()
            dd, dots = ('', '')
            if peek() is not None and peek()[0] == 'cdur': c = nxt(); dd, dots = c[1], c[2]
            d = setdur(dd, dots)
            if first is not None: st['ref'] = first
            emit(t, notes, d); return t + d
        if k == 'note':
            d = setdur(tok[3], tok[4])
            if tok[1] in ('r', 's'): return t + d
            st['ref'], mm = pitch_of(tok[1], tok[2], st['ref'])
            emit(t, [mm], d); return t + d
        return t
    end = seq(0.0)
    return ev, end
mel, mend = parse_voice('altoVoice')
rh, rend = parse_voice('right')
lh, lend = parse_voice('left')
print('durées (temps) : chant %.2f, main droite %.2f, main gauche %.2f' % (mend, rend, lend), file=sys.stderr)
if abs(mend - rend) > 0.01 or abs(mend - lend) > 0.01: print('ATTENTION : voix de longueurs différentes', file=sys.stderr)
NAMES = ['do', 'do#', 'ré', 'mib', 'mi', 'fa', 'fa#', 'sol', 'lab', 'la', 'sib', 'si']
print('chant :', ' '.join('%s%d' % (NAMES[(e[1][0]) % 12], e[1][0] // 12 - 1) for e in mel[:24]), '...', file=sys.stderr)
def F(x):
    t = '%g' % x
    return t + ('' if '.' in t else '.') + 'f'
out = ['// Généré par tools/ly_to_notes.py à partir de tools/marseillaise/marseillaise.ly — ne pas modifier à la main.',
       '// La Marseillaise (Rouget de Lisle, 1792) : premier couplet et refrain « Aux armes, citoyens ! ».',
       '// Partition piano et chant d\'Alexis Jeandeau, CC0 1.0 (https://github.com/jeandeaual/lilypond-piano-la-marseillaise).',
       '// Temps en noires, notes MIDI transposées en si bémol majeur.', '']
out.append('static const RNote MARS_MEL[] = { %s };' % ', '.join('{ %s, %d, %s }' % (F(e[0]), e[1][0] + TRANSPOSE, F(e[2])) for e in mel))
# main droite : jusqu'à 4 notes par accord
rows = []
# garde-fou de registre : quelques constructions LilyPond (<< >> en mode relatif) font dériver l'octave de la main droite ;
# chaque accord est ramené autour du médium (sous le chant), comme les cordes d'une fanfare
for e in rh:
    while e[1] and sum(e[1]) / len(e[1]) > 72: e[1] = [m - 12 for m in e[1]]
    while e[1] and sum(e[1]) / len(e[1]) < 55: e[1] = [m + 12 for m in e[1]]
for e in lh:
    while e[1] and min(e[1]) > 55: e[1] = [m - 12 for m in e[1]]
    while e[1] and min(e[1]) < 31: e[1] = [m + 12 for m in e[1]]
for e in rh:
    ns = sorted(set(m + TRANSPOSE for m in e[1]))[:4] + [0, 0, 0, 0]
    rows.append('{ %s, { %d, %d, %d, %d }, %s }' % (F(e[0]), ns[0], ns[1], ns[2], ns[3], F(e[2])))
out.append('static const RNote MARS_BASS[] = { %s };' % ', '.join('{ %s, %d, %s }' % (F(e[0]), min(e[1]) + TRANSPOSE, F(e[2])) for e in lh))
out.append('static const RChord4 MARS_RH[] = { %s };' % ', '.join(rows))
out.append('static const float MARS_LENGTH = %s;' % F(mend))
open(OUT, 'w', encoding='utf-8').write('\n'.join(out) + '\n')
print('écrit', OUT, len(mel), 'notes de chant,', len(rh), 'accords,', len(lh), 'basses', file=sys.stderr)
