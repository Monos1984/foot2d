#!/usr/bin/env python3
"""NEXUS BALL - traduction francaise et texte des regles.

Produit data/gen/lang.inc (segment DATA0, banque $C0) :
  tr_tab   : TR_COUNT entrees (hash 16 bits, adresse de la chaine anglaise, adresse francaise)
             -> print remplace une chaine anglaise par sa traduction quand lang = 1.
  rules_en / rules_fr : pages des regles (RULES_PAGES pages de RULES_LINES lignes).

Accents : la police 2bpp n'a que les codes 32..95 ; quelques signes inutilises portent
les lettres accentuees : @ = E aigu, [ = E grave, ^ = E circonflexe, ] = A grave,
\\ = C cedille, # = O circonflexe.
"""
import os
import sys

GEN = os.path.join(os.path.dirname(__file__), "..", "data", "gen")
ACC = {"É": "@", "È": "[", "Ê": "^", "À": "]", "Ç": "\\", "Ô": "#", "Û": "U", "Ù": "U", "Î": "I", "Ï": "I"}

# (anglais, francais, longueur maximale si differente de celle de l'anglais)
TR = [
    # ecran titre
    ("INTERPLANETARY SPORT LEAGUE", "LIGUE INTERPLANÉTAIRE", 0),
    ("EXHIBITION", "MATCH AMICAL", 20),
    ("CHAMPIONSHIP", "CHAMPIONNAT", 0),
    ("CUP", "COUPE", 6),
    ("CUSTOM COMPETITION", "COMPÉTITION PERSO", 0),
    ("CREATE TEAM", "CRÉER UNE ÉQUIPE", 20),
    ("CREATE PLAYER", "CRÉER UN JOUEUR", 20),
    ("CREDITS", "CRÉDITS", 0),
    ("RULES", "RÈGLES", 8),
    (" AN OFFGAME PROJECT ", " UN PROJET OFFGAME ", 0),
    # options
    ("MATCH LENGTH    2X  MIN", "DURÉE DU MATCH  2X  MIN", 0),
    ("ON ", "OUI", 0),
    ("OFF", "NON", 0),
    ("BACK", "RETOUR", 8),
    ("CONTROLS        TYPE", "COMMANDES       TYPE", 0),
    ("B:PASS X:KICK Y:SHOOT A:CHARGE", "B:PASSE X:PIED Y:TIR A:CHARGE", 0),
    ("A:PASS B:KICK Y:SHOOT X:CHARGE", "A:PASSE B:PIED Y:TIR X:CHARGE", 0),
    ("Y:PASS B:KICK A:SHOOT X:CHARGE", "Y:PASSE B:PIED A:TIR X:CHARGE", 0),
    ("LANGUAGE", "LANGUE", 0),
    ("TEAM SELECT", "CHOIX DES ÉQUIPES", 22),
    ("<> TEAM   UP/DOWN SIDE   A OK", "<> ÉQUIPE  HAUT/BAS CAMP  A OK", 30),
    ("SPD", "VIT", 0),
    ("POW", "PUI", 0),
    ("<> PAGE    B BACK", "<> PAGE    B RETOUR", 20),
    # match
    ("READY", "PRÊTS", 0),
    ("GO!", "PARTEZ!", 10),
    ("HALF TIME", "MI-TEMPS", 0),
    ("OVERTIME - GOLDEN SCORE", "PROLONGATION - BUT EN OR", 28),
    ("SCORE! +1", "POINT! +1", 0),
    ("SCORE! +2", "POINTS! +2", 12),
    ("RESUME", "REPRENDRE", 10),
    ("TEAM SETUP", "EFFECTIF", 0),
    ("QUIT MATCH", "QUITTER", 0),
    ("HELD TOO LONG!", "PORT TROP LONG!", 28),
    ("FOUL!", "FAUTE!", 28),
    ("MAJOR FOUL! 20 SEC OUT", "FAUTE GRAVE! EXCLU 20 S", 28),
    ("ADVANTAGE", "AVANTAGE", 0),
    ("WINNER", "VAINQUEUR", 10),
    ("DRAW", "MATCH NUL", 10),
    ("4 SEC IN THE ZONE!", "4 S DANS LA RAQUETTE !", 28),
    ("NO SCORE FROM OWN HALF", "PAS DE POINT DE SA MOITIÉ", 28),
    ("SHOOTOUT", "TIRS AU BUT", 28),
    ("SHOOTOUT ", "T.A.B.", 0),
    ("GOAL!  ", "BUT!  ", 0),
    ("SAVED!  ", "ARRÊT!  ", 0),
    ("MISSED!  ", "RATÉ!  ", 0),
    ("1ST", "MT1", 0),
    ("2ND", "MT2", 0),
    ("OT ", "PRO", 0),
    ("SO ", "TAB", 0),
    ("SHOTS", "TIRS", 0),
    ("FOULS", "FAUTES", 6),
    ("FULL TIME", "FIN DU MATCH", 16),
    ("PRESS START", "APPUIE START", 13),
    # avant-match
    ("MATCH SETUP", "RÉGLAGES DU MATCH", 20),
    ("1P VS CPU", "1J VS CPU", 0),
    ("1P VS 2P", "1J VS 2J", 0),
    ("HOME", "DOM.", 0),
    ("AWAY", "EXT.", 0),
    ("LEVEL ", "NIV. ", 0),
    ("P1 ", "J1 ", 0),
    ("P2 ", "J2 ", 0),
    ("P1", "J1", 0),
    ("P2", "J2", 0),
    ("HOME TACTICS", "TACT. DOM.", 0),
    ("AWAY TACTICS", "TACT. EXT.", 0),
    ("DIFFICULTY", "DIFFICULTÉ", 0),
    ("STADIUM", "STADE", 0),
    ("TIE RULE", "ÉGALITÉ", 0),
    ("DRAW ALLOWED", "NUL PERMIS", 0),
    ("OVERTIME+SHOOTOUT", "PROLONG.+T.A.B.", 0),
    ("START MATCH", "JOUER LE MATCH", 16),
    ("<> CHANGE  A OK  B BACK  START", "<> CHOIX  A OK  B RETOUR START", 0),
    ("LINEUP / SUBS", "COMPO / REMPLAÇANTS", 22),
    ("DONE", "TERMINÉ", 8),
    ("<> CHANGE  A SELECT  B DONE", "<> CHOIX  A CHOISIR  B FINI", 0),
    ("   PO NAME     S P A K C D T F", "   PO NOM      V P A T C D E F", 0),
    ("A: PICK A PLAYER, A: SWAP", "A: CHOISIR PUIS A: ÉCHANGER", 30),
    ("COLOR = ON FIELD   F = FATIGUE", "COULEUR = EN JEU  F = FATIGUE", 0),
    ("EASY", "FACILE", 8),
    ("HARD", "DIFFICILE", 10),
    ("MENTALITY", "MENTALITÉ", 0),
    ("PASSING", "PASSES", 0),
    ("PRESSURE", "PRESSING", 0),
    ("DEF LINE", "LIGNE DÉF", 9),
    ("ATTACK", "ATTAQUE", 8),
    ("DEFENSIVE", "DÉFENSIVE", 0),
    ("BALANCED", "ÉQUILIBRÉE", 10),
    ("SHORT", "COURTES", 8),
    ("MIXED", "MIXTE", 0),
    ("LONG", "LONGUES", 8),
    ("LOW", "BAS", 0),
    ("HIGH", "HAUT", 0),
    ("DEEP", "BASSE", 6),
    ("CENTER", "CENTRE", 0),
    ("SIDES", "AILES", 0),
    ("SLOW", "LENT", 0),
    ("FAST", "RAPIDE", 8),
    ("GK", "GB", 0),
    ("MF", "MI", 0),
    ("FW", "AT", 0),
    # editeurs
    ("ENTER NAME", "ENTRE LE NOM", 16),
    ("A ADD  B DELETE  START DONE", "A AJOUTE B EFFACE START FINI", 30),
    ("SAVE", "SAUVEGARDER", 11),
    ("DELETE", "EFFACER", 8),
    ("NAME", "NOM", 0),
    ("NUMBER", "NUMÉRO", 0),
    ("POSITION", "POSTE", 0),
    ("SECOND POS.", "POSTE 2", 0),
    ("NONE", "AUCUN", 5),
    ("SKIN TONE", "PEAU", 0),
    ("HAIR", "CHEVEUX", 8),
    ("HAIR COLOR", "COUL. CHEV.", 11),
    ("POINTS LEFT", "RESTE", 0),
    ("TEAM NAME", "NOM", 0),
    ("SHORT NAME", "NOM COURT", 0),
    ("HOME WORLD", "PLANÈTE", 0),
    ("PRIMARY", "COULEUR 1", 9),
    ("SECONDARY", "COULEUR 2", 0),
    ("SPEED", "VITESSE", 9),
    ("POWER", "PUISSANCE", 9),
    ("PASS", "PASSE", 9),
    ("KICK", "TIR", 0),
    ("CONTROL", "CONTRÔLE", 9),
    ("DEFENSE", "DÉFENSE", 0),
    ("STAMINA", "ENDURANCE", 9),
    ("REFLEX", "RÉFLEXES", 9),
    ("POSITION ", "PLACEMENT", 0),
    ("THROW", "RELANCE", 9),
    ("CATCH", "PRISE", 0),
    # couleurs et styles (largeur fixe)
    ("BLUE    ", "BLEU    ", 0),
    ("NAVY    ", "MARINE  ", 0),
    ("SKY     ", "CIEL    ", 0),
    ("GREEN   ", "VERT    ", 0),
    ("LIME    ", "CITRON  ", 0),
    ("YELLOW  ", "JAUNE   ", 0),
    ("RED     ", "ROUGE   ", 0),
    ("PINK    ", "ROSE    ", 0),
    ("PURPLE  ", "VIOLET  ", 0),
    ("WHITE   ", "BLANC   ", 0),
    ("GREY    ", "GRIS    ", 0),
    ("BLACK   ", "NOIR    ", 0),
    ("SPEED    ", "VITESSE  ", 0),
    ("POWER    ", "PUISSANCE", 0),
    ("PASSING  ", "PASSES   ", 0),
    ("COUNTER  ", "CONTRE   ", 0),
    ("BALANCED ", "ÉQUILIBRÉ", 0),
    ("TECHNICAL", "TECHNIQUE", 0),
    # competitions
    ("CONTINUE", "CONTINUER", 12),
    ("NEW COMPETITION", "NOUVELLE COMPÉTITION", 22),
    ("LEAGUE", "LIGUE", 0),
    ("TEAMS", "ÉQUIPES", 8),
    ("LEAGUE: 3 TO 16 TEAMS", "LIGUE: 3 À 16 ÉQUIPES", 0),
    ("CUP: 4, 8 OR 16 TEAMS", "COUPE: 4, 8 OU 16 ÉQUIPES", 28),
    ("ROUND", "JOURNÉE", 8),
    ("FINAL", "FINALE", 8),
    ("SEMI-FINALS", "DEMI-FINALES", 14),
    ("QUARTER-FINALS", "QUARTS DE FINALE", 18),
    ("ROUND OF 16", "HUITIÈMES", 0),
    ("PLAY", "JOUER", 6),
    ("COMPETITION OVER", "COMPÉTITION FINIE", 18),
    ("TABLE", "CLASSEMENT", 12),
    ("BRACKET", "TABLEAU", 0),
    ("SAVE & EXIT", "SAUVER & QUITTER", 17),
    ("WATCH CPU MATCHES", "VOIR MATCHS CPU", 0),
    ("PLAY AS", "CAMP", 0),
    ("   TEAM             P W D L PTS", "   ÉQUIPE           J G N P PTS", 0),
    ("OUT", "ABS", 0),
    # credits
    ("AN OFFGAME PROJECT", "UN PROJET OFFGAME", 0),
    ("GAME DESIGN", "CONCEPTION DU JEU", 28),
    ("PROJECT DIRECTION", "DIRECTION DU PROJET", 28),
    ("DESIGN & DEVELOPMENT ASSISTANCE", "AIDE CONCEPTION & DÉVELOPPEMENT", 0),
    ("SUPER NINTENDO VERSION", "VERSION SUPER NINTENDO", 0),
    ("SPECIAL THANKS", "REMERCIEMENTS", 0),
    ("  TO BE COMPLETED", "  À COMPLÉTER", 0),
]

RULES_LINES = 19
RULES_WIDTH = 28

RULES = {
    "en": [
        ("THE GAME", [
            "NEXUS BALL IS A FUTURISTIC SPORT, HEIR OF FOOTBALL AND RUGBY.",
            "",
            "TWO TEAMS OF 6 PLAYERS (5 + 1 KEEPER) FACE OFF IN A CLOSED ARENA.",
            "",
            "EACH TEAM DEFENDS A GLOWING VERTICAL RING. THE BALL MUST GO FULLY THROUGH THE RING TO SCORE.",
            "",
            "THE WALLS ARE IN PLAY: THE BALL BOUNCES OFF THEM AND STAYS LIVE. USE THEM TO GET PAST A DEFENDER!",
        ]),
        ("SCORING", [
            "1 POINT: A THROW OR A SHOT FROM CLOSE RANGE.",
            "",
            "2 POINTS: A KICK FROM BEYOND THE LONG LINE (2-POINT LINE).",
            "",
            "BUILD A SAFE 1-POINT ATTACK OR RISK A HARD 2-POINT KICK!",
            "",
            "NO POINT CAN BE SCORED FROM YOUR OWN HALF.",
            "",
                        "KICK-OFF: TWO PLAYERS IN THE CENTRE CIRCLE, THE FIRST MUST PASS TO THE SECOND.",
        ]),
        ("CARRYING AND PASSING", [
            "YOU CAN CARRY THE BALL FOR 4 SECONDS. THE CURSOR BLINKS AFTER 3 SECONDS. HELD TOO LONG: THE BALL GOES TO THE OPPONENTS.",
            "",
            "HAND PASS: SIDEWAYS OR BACKWARDS ONLY. NEVER FORWARD!",
            "",
            "FOOT PASS: A LOBBED PASS THAT CAN GO FORWARD.",
            "",
            "A LOOSE BALL CAN BE VOLLEYED, EVEN IN THE AIR.",
        ]),
        ("CHARGES AND FOULS", [
            "CHARGE THE CARRIER TO WIN THE BALL: POWER AND CONTROL AGAINST POWER AND DEFENSE.",
            "",
            "FOULS:",
            "- CHARGE FROM BEHIND",
            "- CONTACT ON THE KEEPER IN HIS ZONE",
            "- LATE CONTACT AFTER A PASS",
            "FREE KICK: OPPONENTS STAY AWAY.",
            "",
            "ADVANTAGE MAY BE PLAYED. MAJOR FOUL: 20 S OUT.",
            "",
            "ZONES: KEEPERS ONLY, BALL 4 S MAX.",
        ]),
        ("MATCH AND CONTROLS", [
            "TWO HALVES. A DRAW CAN BE ALLOWED, OR OVERTIME WITH GOLDEN SCORE, THEN A SHOOTOUT (3 SHOTS EACH, THEN SUDDEN DEATH).",
            "",
            "CONTROLS (TYPE A):",
            "D-PAD  MOVE",
            "B      HAND PASS",
            "X      FOOT PASS / KICK",
            "Y      SHOOT / JUMP",
            "A      DODGE / CHARGE",
            "L      SWITCH PLAYER",
            "R      SPRINT",
            "START  PAUSE",
        ]),
    ],
    "fr": [
        ("LE JEU", [
            "NEXUS BALL EST UN SPORT FUTURISTE, HÉRITIER DU FOOTBALL ET DU RUGBY.",
            "",
            "DEUX ÉQUIPES DE 6 JOUEURS (5 + 1 GARDIEN) S'AFFRONTENT DANS UNE ARÈNE FERMÉE.",
            "",
            "CHAQUE ÉQUIPE DÉFEND UN GRAND ANNEAU LUMINEUX VERTICAL. LE BALLON DOIT TRAVERSER ENTIÈREMENT L'ANNEAU POUR MARQUER.",
            "",
            "LES MURS SONT EN JEU : LE BALLON REBONDIT DESSUS ET RESTE VIVANT. SERS-T'EN POUR PASSER UN DÉFENSEUR !",
        ]),
        ("LE SCORE", [
            "1 POINT : UN LANCER OU UN TIR DE PRÈS.",
            "",
            "2 POINTS : UNE FRAPPE AU PIED DEPUIS LA LIGNE LONGUE (LIGNE DES 2 POINTS).",
            "",
            "CONSTRUIS UNE ACTION SÛRE À 1 POINT OU TENTE UNE FRAPPE DIFFICILE À 2 POINTS !",
            "",
            "IMPOSSIBLE DE MARQUER DEPUIS SA PROPRE MOITIÉ DE TERRAIN.",
            "",
                        "ENGAGEMENT : DEUX JOUEURS DANS LE ROND CENTRAL, LE PREMIER PASSE AU SECOND.",
        ]),
        ("PORT ET PASSES", [
            "TU PEUX GARDER LE BALLON 4 SECONDES. LE CURSEUR CLIGNOTE APRÈS 3 SECONDES. TROP LONG : LE BALLON VA À L'ADVERSAIRE.",
            "",
            "PASSE À LA MAIN : LATÉRALE OU EN RETRAIT SEULEMENT. JAMAIS VERS L'AVANT !",
            "",
            "PASSE AU PIED : UNE PASSE LOBÉE QUI PEUT ALLER VERS L'AVANT.",
            "",
            "UN BALLON LIBRE PEUT ÊTRE REPRIS DE VOLÉE, MÊME EN L'AIR.",
        ]),
        ("CHARGES ET FAUTES", [
            "CHARGE LE PORTEUR POUR PRENDRE LE BALLON (PUISSANCE, CONTRÔLE, DÉFENSE).",
            "",
            "FAUTES :",
            "- CHARGE PAR DERRIÈRE",
            "- CONTACT SUR LE GARDIEN DANS SA ZONE",
            "- CONTACT TARDIF APRÈS UNE PASSE",
            "COUP FRANC : ADVERSAIRES À DISTANCE.",
            "",
            "AVANTAGE POSSIBLE. FAUTE GRAVE : EXCLUSION 20 S.",
            "",
            "RAQUETTES : GARDIENS, BALLON 4 S MAX.",
        ]),
        ("MATCH ET COMMANDES", [
            "DEUX MI-TEMPS. LE NUL PEUT ÊTRE AUTORISÉ, SINON PROLONGATION AVEC BUT EN OR, PUIS TIRS AU BUT (3 CHACUN, PUIS MORT SUBITE).",
            "",
            "COMMANDES (TYPE A) :",
            "CROIX  DÉPLACEMENT",
            "B      PASSE À LA MAIN",
            "X      PASSE PIED / FRAPPE",
            "Y      TIR / SAUT",
            "A      ESQUIVE / CHARGE",
            "L      CHANGER DE JOUEUR",
            "R      SPRINT",
            "START  PAUSE",
        ]),
    ],
}


def enc(s):
    out = []
    for ch in s:
        ch = ACC.get(ch, ch)
        o = ord(ch)
        if o == 39:            # apostrophe : presente dans la police
            pass
        if not 32 <= o < 96:
            sys.exit("caractere hors police: %r dans %r" % (ch, s))
        out.append(o)
    return out


def h16(b):
    h = 0
    for c in b:
        h = (((h << 5) + h) & 0xFFFF) ^ c
    return h


def wrap(par, width):
    if len(par) <= width:
        return [par]
    lines, cur = [], ""
    for w in par.split(" "):
        if cur and len(cur) + 1 + len(w) > width:
            lines.append(cur)
            cur = w
        else:
            cur = (cur + " " + w) if cur else w
    if cur:
        lines.append(cur)
    return lines


def bytes_asm(b):
    return ".byte " + ", ".join("$%02X" % c for c in b + [0])


def main():
    os.makedirs(GEN, exist_ok=True)
    keys = {}
    for en, fr, mx in TR:
        e, f = enc(en), enc(fr)
        lim = mx or len(e)
        if len(f) > lim:
            sys.exit("trop long (%d > %d): %s -> %s" % (len(f), lim, en, fr))
        if tuple(e) in keys:
            sys.exit("doublon: %s" % en)
        keys[tuple(e)] = f
    # une traduction ne doit pas etre elle-meme une cle (sinon double traduction)
    for e, f in keys.items():
        if tuple(f) in keys and tuple(f) != e and keys[tuple(f)] != f:
            sys.exit("traduction ambigue: %s" % bytes(f))
    out = ['; genere par tools/lang.py', '.segment "DATA0"',
           "TR_COUNT = %d" % len(keys), "tr_tab:"]
    items = sorted(keys.items(), key=lambda kv: h16(kv[0]))
    for i, (e, f) in enumerate(items):
        out.append("    .word $%04X, .loword(tr_e%d), .loword(tr_f%d)" % (h16(e), i, i))
    for i, (e, f) in enumerate(items):
        out.append("tr_e%d: %s" % (i, bytes_asm(list(e))))
        out.append("tr_f%d: %s" % (i, bytes_asm(f)))
    # regles
    npages = len(RULES["en"])
    out.append("RULES_PAGES = %d" % npages)
    out.append("RULES_LINES = %d" % RULES_LINES)
    for lang, pages in RULES.items():
        assert len(pages) == npages
        out.append("rules_%s:" % lang)
        for p, (title, pars) in enumerate(pages):
            lines = []
            for par in pars:
                lines += wrap(par, RULES_WIDTH)
            if len(lines) > RULES_LINES:
                sys.exit("page %s/%d trop longue (%d lignes)" % (lang, p, len(lines)))
            lines += [""] * (RULES_LINES - len(lines))
            out.append("    " + bytes_asm(enc(title)))
            for l in lines:
                out.append("    " + bytes_asm(enc(l)))
    out.append('.segment "CODE"')
    with open(os.path.join(GEN, "lang.inc"), "w") as fh:
        fh.write("\n".join(out) + "\n")
    print("lang: %d traductions, %d pages de regles" % (len(keys), npages))


if __name__ == "__main__":
    main()
