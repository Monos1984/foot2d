; =============================================================================
;  teams.asm - 16 equipes officielles (donnees generees par tools/teams.py),
;  effectifs de 12, maillots, composition automatique
; =============================================================================

.segment "RODATA"
.include "data/gen/teams.inc"

T_NAME   = 0
T_SHORT  = 16
T_WORLD  = 20
T_KIT    = 36
T_AWAY   = 46
T_FAM    = 56
T_FORM   = 58
T_TACT   = 59
T_STYLE  = 65
T_LEVEL  = 66
T_PLAYERS = 68
T_SKIN   = 260
PL_REC   = 16
PL_ROLE  = 8
PL_STATS = 9
.segment "CODE"

; team_rec : A = cote (0/1) -> Y = adresse de l'enregistrement de l'equipe. Preserve X.
team_rec:
    .a16
    .i16
    asl a
    tay
    lda team_id,y
    ; continue dans team_rec_id

; team_rec_id : A = numero d'equipe -> Y = trec_buf (copie de l'enregistrement).
;  0..15 : equipes officielles (ROM banque $C0), 16..23 : equipes creees (SRAM).
;  Le pointeur reste valable jusqu'au prochain appel avec une autre equipe. Preserve X.
team_rec_id:
    .a16
    .i16
    cmp trec_id
    beq @hit
    sta trec_id
    phx
    phb
    cmp #NUM_TEAMS
    bcs @custom
    asl a
    tax
    lda team_ptr,x
    tax
    ldy #.loword(trec_buf)
    lda #TEAM_REC - 1
    mvn #$C0, #$80
    bra @done
@custom:
    sec
    sbc #NUM_TEAMS
    jsr cteam_offset            ; A = adresse SRAM (banque $A1) de l'enregistrement
    tax
    ldy #.loword(trec_buf)
    lda #TEAM_REC - 1
    mvn #$A1, #$80
@done:
    plb
    plx
@hit:
    ldy #.loword(trec_buf)
    rts

; roster_rec : A = cote, X = index effectif (0..11) -> Y = adresse du joueur. Preserve X.
roster_rec:
    .a16
    .i16
    jsr team_rec
    sty t6
    txa
    asl a
    asl a
    asl a
    asl a                       ; *16
    clc
    adc t6
    adc #T_PLAYERS
    tay
    rts

; -----------------------------------------------------------------------------
;  team_defaults : A = cote -> formation et tactiques par defaut de l'equipe
; -----------------------------------------------------------------------------
team_defaults:
    .a16
    .i16
    sta t5
    jsr team_rec
    lda t5
    asl a
    tax
    lda a:T_FORM,y
    and #$00FF
    sta form_id,x
    ; tactiques
    lda t5
    asl a
    asl a
    sta t4
    asl a
    clc
    adc t4                      ; cote * 12
    tax
    lda #6
    sta t4
@t: lda a:T_TACT,y
    and #$00FF
    sta tact,x
    iny
    inx
    inx
    dec t4
    bne @t
    lda t5
    jmp build_lineup

; -----------------------------------------------------------------------------
;  build_lineup : A = cote -> gardien + 5 joueurs selon les postes de la formation
;  (les meilleurs de l'effectif au poste en premier)
; -----------------------------------------------------------------------------
build_lineup:
    .a16
    .i16
    sta t5
    stz t3                      ; masque des joueurs deja pris
    ; base lineup : cote*12
    asl a
    asl a
    sta t4
    asl a
    clc
    adc t4
    sta t4
    tax
    stz lineup,x                ; slot 0 : premier gardien
    lda #1
    sta t3
    lda #1
    sta t2                      ; slot 1..5
@slot:
    ; poste voulu pour ce slot
    lda t5
    asl a
    tay
    lda form_id,y
    jsr form_slot_ptr           ; Y = entree du slot t2
    lda a:4,y
    sta t1                      ; poste
    ; premier joueur libre de ce poste
    ldx #0
@find:
    lda bit_tab,x
    and t3
    bne @next
    phx
    txa
    lsr a
    tax
    lda t5
    jsr roster_rec
    plx
    lda a:PL_ROLE,y
    and #$00FF
    cmp t1
    beq @take
@next:
    inx
    inx
    cpx #24
    bne @find
    ; aucun : premier joueur de champ libre
    ldx #2
@any:
    lda bit_tab,x
    and t3
    beq @take
    inx
    inx
    bra @any
@take:
    lda bit_tab,x
    ora t3
    sta t3
    txa
    lsr a
    pha
    lda t2
    asl a
    clc
    adc t4
    tax
    pla
    sta lineup,x
    inc t2
    lda t2
    cmp #6
    bne @slot
    rts

bit_tab:
    .word $0001, $0002, $0004, $0008, $0010, $0020, $0040, $0080, $0100, $0200, $0400, $0800

; -----------------------------------------------------------------------------
;  load_kits : (ecran eteint) maillots des deux equipes dans les palettes OBJ 0/1,
;  couleurs des noms du HUD et des points du radar
; -----------------------------------------------------------------------------
load_kits:
    .a16
    .i16
    stz kit_away
    stz kit_away+2
    ; meme famille de couleur : l'equipe 2 joue en exterieur
    lda #0
    jsr team_rec
    lda a:T_FAM,y
    and #$00FF
    sta t5
    lda #1
    jsr team_rec
    lda a:T_FAM,y
    and #$00FF
    cmp t5
    bne :+
    lda #1
    sta kit_away+2
:   ; choix impose dans l'ecran de choix des equipes (0 auto, 1 domicile, 2 exterieur)
    ldx #0
@pick:
    lda kit_pick,x
    beq :+
    dec a
    sta kit_away,x
:   inx
    inx
    cpx #4
    bne @pick
    ldx #0
@side:
    phx
    txa
    lsr a
    sta t5
    jsr team_rec
    lda t5
    asl a
    tax
    lda kit_away,x              ; tenue exterieure possible pour les deux equipes
    sta t6
    tya
    clc
    adc #T_KIT
    ldx t6
    beq :+
    clc
    adc #10
:   sta t6                      ; 5 couleurs
    ; CGRAM 128 + 16*cote + 5..8 et +10
    lda t5
    asl a
    asl a
    asl a
    asl a
    clc
    adc #128 + 5
    sta t4
    ldy t6
    ldx t4
    jsr kit_write
    ; meme maillot dans la palette peau foncee (5 / 6)
    lda t4
    clc
    adc #80
    tax
    jsr kit_write
    lda t4
    clc
    adc #80 - 5 + 1
    tax
    lda #$1022                  ; contour
    jsr cg_write
    inx
    lda #$1951                  ; peau foncee
    jsr cg_write
    inx
    lda #$0CCB                  ; ombre
    jsr cg_write
    inx
    lda #$0842                  ; cheveux
    jsr cg_write
    ; reflet -> nom du HUD (BG3 couleur 6 / 14) et point du radar (OBJ 4 : 12 / 13)
    lda a:8,y
    ldx #6
    ldy t5
    beq :+
    ldx #14
:   jsr cg_write
    ldx t5
    ldy t6
    lda a:8,y
    pha
    txa
    clc
    adc #128 + 64 + 12
    tax
    pla
    jsr cg_write
    plx
    inx
    inx
    cpx #4
    jne @side
    rts

; kit_write : X = CGRAM (index 5 de la palette), Y = 5 couleurs du maillot. Preserve Y.
kit_write:
    .a16
    .i16
    lda a:0,y
    jsr cg_write
    lda a:2,y
    inx
    jsr cg_write
    lda a:4,y
    inx
    jsr cg_write
    lda a:6,y
    inx
    jsr cg_write
    lda a:8,y
    inx
    inx
    jmp cg_write

; cg_write : A = couleur BGR555, X = index CGRAM (ecran eteint ou VBlank). Preserve X, Y.
cg_write:
    .a16
    .i16
    pha
    sep #$20
    .a8
    txa
    sta CGADD
    pla
    sta CGDATA
    pla
    sta CGDATA
    rep #$20
    .a16
    rts
