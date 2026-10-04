.segment "CODE2"
; =============================================================================
;  editor.asm - CREATE PLAYER / CREATE TEAM, clavier virtuel, bloc SRAM d'edition
;
;  Bloc SRAM a $A1:6000 (independant du bloc options et des competitions) :
;    +0 "NXED"  +4 version  +6 longueur
;    +8    32 joueurs crees x 24 octets :
;          nom (8, espaces), poste, 7 caracteristiques, numero, 2e poste ($FF aucun),
;          peau, coiffure, couleur des cheveux, valide, reserve x2
;    +776  8 equipes creees x 264 octets : enregistrement d'equipe (262) + valide + reserve
;    +2888 checksum (somme des mots)
;  Equipes creees : numeros 16..23 dans tout le jeu.
; =============================================================================

SRAM_ED      = $A16000
ED_VERSION   = 2
ED_PL_N      = 32
ED_PL_SIZE   = 24
ED_TM_N      = 8
ED_TM_SIZE   = 264
ED_PL_OFF    = 8
ED_TM_OFF    = ED_PL_OFF + ED_PL_N * ED_PL_SIZE
ED_CK_OFF    = ED_TM_OFF + ED_TM_N * ED_TM_SIZE
ED_LEN       = ED_CK_OFF - 8
PL_NUM       = 16
PL_POS2      = 17
PL_SKIN      = 18
PL_HAIR      = 19
PL_HCOL      = 20
PL_VALID     = 21
CT_VALID     = TEAM_REC
STAT_BUDGET  = 40
MAX_TEAMS    = NUM_TEAMS + ED_TM_N

; cteam_offset : A = emplacement d'equipe creee (0..7) -> A = adresse dans la banque $A1
cteam_offset:
    .a16
    pha
    asl a
    asl a
    asl a                       ; *8
    sta ct_tmp
    pla
    xba                         ; *256
    clc
    adc ct_tmp                  ; *264
    clc
    adc #.loword(SRAM_ED) + ED_TM_OFF
    rts

; cplayer_offset : A = joueur cree (0..31) -> X = adresse dans la banque $A1
cplayer_offset:
    .a16
    asl a
    asl a
    asl a
    sta ct_tmp                  ; *8
    asl a
    clc
    adc ct_tmp                  ; *24
    clc
    adc #.loword(SRAM_ED) + ED_PL_OFF
    tax
    rts

; team_valid : A = numero d'equipe -> carry = 1 si l'equipe existe. Preserve A, X, Y.
team_valid:
    .a16
    .i16
    cmp #NUM_TEAMS
    bcc @yes
    cmp #MAX_TEAMS
    bcs @no
    pha
    phx
    sec
    sbc #NUM_TEAMS
    jsr cteam_offset
    tax
    lda f:SRAM_ED & $FF0000 + CT_VALID,x
    and #$00FF
    plx
    cmp #1
    pla
    bcs @yes2
@no:
    clc
    rts
@yes2:
@yes:
    sec
    rts

; team_step : A = equipe, t7 = boutons -> A = equipe suivante / precedente valide
team_step:
    .a16
    .i16
    sta ts_cur
    lda t7
    bit #JOY_LEFT
    bne @prev
    and #(JOY_RIGHT | JOY_A)
    beq @same
    jsr ui_click
@n: lda ts_cur
    inc a
    cmp #MAX_TEAMS
    bcc :+
    lda #0
:   sta ts_cur
    jsr team_valid
    bcc @n
    rts
@prev:
    jsr ui_click
@p: lda ts_cur
    dec a
    bpl :+
    lda #MAX_TEAMS - 1
:   sta ts_cur
    jsr team_valid
    bcc @p
    rts
@same:
    lda ts_cur
    rts

; -----------------------------------------------------------------------------
;  bloc SRAM : verification au demarrage, checksum
; -----------------------------------------------------------------------------
ed_checksum:
    .a16
    .i16
    lda #$5AA5
    sta t4
    ldx #.loword(SRAM_ED) + 8
@l: lda f:SRAM_ED & $FF0000,x
    clc
    adc t4
    sta t4
    inx
    inx
    cpx #.loword(SRAM_ED) + ED_CK_OFF
    bcc @l
    lda t4
    rts

ed_check:
    .a16
    .i16
    lda f:SRAM_ED
    cmp #('N' | ('X' << 8))
    bne @reset
    lda f:SRAM_ED+2
    cmp #('E' | ('D' << 8))
    bne @reset
    lda f:SRAM_ED+4
    cmp #ED_VERSION
    bne @reset
    lda f:SRAM_ED+6
    cmp #ED_LEN
    bne @reset
    jsr ed_checksum
    cmp f:SRAM_ED + ED_CK_OFF
    bne @reset
    rts
@reset:
    ; bloc corrompu ou absent : on vide uniquement ce bloc
    ldx #.loword(SRAM_ED)
    lda #0
@z: sta f:SRAM_ED & $FF0000,x
    inx
    inx
    cpx #.loword(SRAM_ED) + ED_CK_OFF + 2
    bcc @z
    lda #('N' | ('X' << 8))
    sta f:SRAM_ED
    lda #('E' | ('D' << 8))
    sta f:SRAM_ED+2
    lda #ED_VERSION
    sta f:SRAM_ED+4
    lda #ED_LEN
    sta f:SRAM_ED+6
ed_commit:
    jsr ed_checksum
    sta f:SRAM_ED + ED_CK_OFF
    lda #$FFFF
    sta trec_id                 ; le cache d'equipe peut etre perime
    rts

; =============================================================================
;  clavier virtuel : kb_buf (termine par 0), kb_max caracteres
;  A ajoute, B efface, START termine.
; =============================================================================
KB_COLS = 13

kb_edit:
    .a16
    .i16
    stz kb_x
    stz kb_y
    jsr safe_screen_off
    jsr bg3_clear
    jsr ui_fill
    lda #UI_HI
    sta t0
    ldx #TPOS(9, 3)
    ldy #.loword(str_kb_title)
    jsr print
    lda #UI_ATTR
    sta t0
    ldx #TPOS(1, 22)
    ldy #.loword(str_kb_help)
    jsr print
    ; grille
    stz t5
@g: lda t5
    tay
    lda kb_chars,y
    and #$00FF
    sec
    sbc #32
    clc
    adc #UI_ATTR
    pha
    jsr kb_pos
    pla
    sta bg3_map,x
    inc t5
    lda t5
    cmp #KB_COLS * 3
    bne @g
    jsr kb_draw
    jsr screen_on
@loop:
    jsr ui_wait
    lda t7
    beq @loop
    bit #JOY_START
    jne @end
    bit #JOY_B
    beq @nb
    ; effacer
    jsr kb_len
    beq @loop
    dec a
    tax
    sep #$20
    .a8
    stz kb_buf,x
    rep #$20
    .a16
    jsr ui_click
    jsr kb_draw
    bra @loop
@nb:
    bit #JOY_A
    beq @move
    jsr kb_len
    cmp kb_max
    bcs @loop
    tax
    lda kb_y
    asl a
    asl a
    sta t6
    asl a
    clc
    adc t6                      ; *12
    adc kb_y                    ; *13
    adc kb_x
    tay
    lda kb_chars,y
    sep #$20
    .a8
    sta kb_buf,x
    stz kb_buf+1,x
    rep #$20
    .a16
    jsr ui_click
    jsr kb_draw
    bra @loop
@move:
    lda t7
    bit #JOY_LEFT
    beq :+
    lda kb_x
    dec a
    bpl @sx
    lda #KB_COLS - 1
    bra @sx
:   bit #JOY_RIGHT
    beq :+
    lda kb_x
    inc a
    cmp #KB_COLS
    bcc @sx
    lda #0
    bra @sx
:   bit #JOY_UP
    beq :+
    lda kb_y
    dec a
    bpl @sy
    lda #2
    bra @sy
:   bit #JOY_DOWN
    jeq @loop
    lda kb_y
    inc a
    cmp #3
    bcc @sy
    lda #0
@sy:
    sta kb_y
    bra @mv
@sx:
    sta kb_x
@mv:
    jsr ui_click
    jsr kb_draw
    jmp @loop
@end:
    rts

; kb_pos : t5 = index de touche -> X = position dans bg3_map
kb_pos:
    .a16
    lda t5
    ldx #KB_COLS
    jsr divu
    sta t6                      ; ligne
    lda RDMPYL                  ; colonne
    asl a
    asl a                       ; 2 cases par touche
    clc
    adc #3 * 2
    sta t4
    lda t6
    asl a
    clc
    adc #11
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc t4
    tax
    rts

; kb_len : A = longueur de kb_buf (Z positionne)
kb_len:
    .a16
    .i16
    ldx #0
@l: lda kb_buf,x
    and #$00FF
    beq @d
    inx
    bra @l
@d: txa
    rts

kb_draw:
    .a16
    .i16
    ; texte en cours
    lda #UI_HI
    sta t0
    lda #17
    sta t1
    ldx #TPOS(8, 7)
    ldy #.loword(kb_buf)
    jsr print_w
    ; curseur : '>' devant la touche choisie, effacer les autres
    stz t5
@c: jsr kb_pos
    dex
    dex
    lda kb_y
    asl a
    asl a
    sta t6
    asl a
    clc
    adc t6
    adc kb_y
    adc kb_x
    cmp t5
    bne :+
    lda #('>' - 32 + UI_HI)
    bra :++
:   lda #UI_ATTR
:   sta bg3_map,x
    inc t5
    lda t5
    cmp #KB_COLS * 3
    bne @c
    lda #1
    sta bg3_dirty
    rts

; =============================================================================
;  CREATE PLAYER : liste des 32 emplacements puis editeur
; =============================================================================
create_player:
    .a16
    .i16
    stz ed_slot
@list:
    jsr safe_screen_off
    jsr bg3_clear
    jsr ui_fill
    lda #UI_HI
    sta t0
    ldx #TPOS(9, 1)
    ldy #.loword(str_cp_title)
    jsr print
    jsr cpl_draw
    jsr screen_on
@loop:
    jsr ui_wait
    lda t7
    beq @loop
    bit #JOY_B
    beq :+
    jmp title_screen
:   lda ed_slot
    ldy #ED_PL_N
    jsr ui_updown
    cmp ed_slot
    beq :+
    sta ed_slot
    jsr cpl_draw
    bra @loop
:   lda t7
    and #(JOY_A | JOY_START)
    beq @loop
    jsr pe_edit
    bra @list

; cpl_draw : 32 joueurs en deux colonnes de 16
cpl_draw:
    .a16
    .i16
    stz t5
@l: lda t5
    jsr cpl_rowpos              ; X = position
    phx
    ; curseur
    lda #UI_ATTR
    ldy t5
    cpy ed_slot
    bne :+
    lda #('>' - 32 + UI_HI)
:   sta bg3_map,x
    ; numero d'emplacement
    lda #UI_ATTR
    sta t0
    lda t5
    inc a
    plx
    phx
    inx
    inx
    jsr print_num2
    ; nom ou "---"
    lda t5
    jsr cplayer_offset
    lda f:SRAM_ED & $FF0000 + PL_VALID,x
    and #$00FF
    beq @empty
    stx t4
    plx
    txa
    clc
    adc #4 * 2
    tax
    ldy #8
@n: phx
    ldx t4
    lda f:SRAM_ED & $FF0000,x
    plx
    and #$00FF
    sec
    sbc #32
    clc
    adc #UI_A
    sta bg3_map,x
    inx
    inx
    inc t4
    dey
    bne @n
    bra @next
@empty:
    plx
    txa
    clc
    adc #4 * 2
    tax
    ldy #.loword(str_ed_empty)
    lda #UI_ATTR
    sta t0
    lda #8
    sta t1
    jsr print_w
@next:
    inc t5
    lda t5
    cmp #ED_PL_N
    jne @l
    lda #1
    sta bg3_dirty
    lda ed_slot
    and #$000F
    clc
    adc #4
    jmp bg2_bar

; cpl_rowpos : A = emplacement -> X = position (colonne 1 ou 16, ligne 4 + n mod 16)
cpl_rowpos:
    .a16
    ldx #1 * 2
    cmp #16
    bcc :+
    ldx #16 * 2
    sbc #16
:   clc
    adc #4
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    stx t6
    clc
    adc t6
    tax
    rts

; -----------------------------------------------------------------------------
;  pe_edit : edition du joueur ed_slot (copie de travail ed_pl)
; -----------------------------------------------------------------------------
PE_ITEMS = 17

pe_edit:
    .a16
    .i16
    ; charger (ou valeurs par defaut)
    lda ed_slot
    jsr cplayer_offset
    ldy #0
@ld:
    lda f:SRAM_ED & $FF0000,x
    sta ed_pl,y
    inx
    inx
    iny
    iny
    cpy #ED_PL_SIZE
    bne @ld
    lda ed_pl + PL_VALID
    and #$00FF
    bne @ok
    ; nouveau joueur
    ldx #0
    lda #$2020
@sp:
    sta ed_pl,x
    inx
    inx
    cpx #8
    bne @sp
    lda #('N' | ('E' << 8))
    sta ed_pl
    lda #('W' | (' ' << 8))
    sta ed_pl+2
    sep #$20
    .a8
    lda #ROLE_MF
    sta ed_pl+PL_ROLE
    ldx #0
    lda #5
@st:
    sta ed_pl+PL_STATS,x
    inx
    cpx #7
    bne @st
    lda #10
    sta ed_pl+PL_NUM
    lda #$FF
    sta ed_pl+PL_POS2
    stz ed_pl+PL_SKIN
    stz ed_pl+PL_HAIR
    stz ed_pl+PL_HCOL
    rep #$20
    .a16
@ok:
    stz ui_sel
@redraw:
    jsr safe_screen_off
    jsr bg3_clear
    jsr ui_fill
    lda #UI_HI
    sta t0
    ldx #TPOS(9, 1)
    ldy #.loword(str_cp_title)
    jsr print
    jsr pe_draw
    jsr screen_on
@loop:
    jsr ui_wait
    lda t7
    beq @loop
    bit #JOY_B
    beq :+
    rts
:   lda ui_sel
    ldy #PE_ITEMS
    jsr ui_updown
    cmp ui_sel
    beq :+
    sta ui_sel
    jsr pe_draw
    bra @loop
:   lda ui_sel
    bne @n0
    ; nom
    lda t7
    and #(JOY_A | JOY_START)
    beq @loop
    jsr pe_name
    jmp @redraw
@n0:
    cmp #7
    bcs @stat
    asl a
    tax
    jsr (.loword(pe_field_tab),x)
    jsr pe_draw
    bra @loop
@stat:
    cmp #14
    bcs @act
    sec
    sbc #7
    jsr pe_stat
    jsr pe_draw
    bra @loop
@act:
    lda t7
    and #(JOY_A | JOY_START)
    beq @loop
    lda ui_sel
    cmp #14
    beq @save
    cmp #15
    beq @del
    rts
@save:
    sep #$20
    .a8
    lda #1
    sta ed_pl+PL_VALID
    rep #$20
    .a16
    bra @write
@del:
    sep #$20
    .a8
    stz ed_pl+PL_VALID
    rep #$20
    .a16
@write:
    lda ed_slot
    jsr cplayer_offset
    ldy #0
@wr:
    lda ed_pl,y
    sta f:SRAM_ED & $FF0000,x
    inx
    inx
    iny
    iny
    cpy #ED_PL_SIZE
    bne @wr
    jsr ed_commit
    lda #SFX_OK
    jsr sfx_play
    rts

pe_field_tab:
    .word 0, .loword(pe_number), .loword(pe_pos), .loword(pe_pos2)
    .word .loword(pe_skin), .loword(pe_hair), .loword(pe_hcol)

; pe_byte : X = offset dans ed_pl, Y = nombre de valeurs -> modifie l'octet par gauche / droite
pe_byte:
    .a16
    .i16
    phx
    lda ed_pl,x
    and #$00FF
    jsr ui_lr
    plx
    sep #$20
    .a8
    sta ed_pl,x
    rep #$20
    .a16
    rts

pe_number:
    ldx #PL_NUM
    ldy #100
    jsr pe_byte
    lda ed_pl+PL_NUM
    and #$00FF
    bne :+
    lda t7
    ldx #PL_NUM
    sep #$20
    .a8
    lda #1
    sta ed_pl+PL_NUM
    rep #$20
    .a16
:   rts
pe_pos:
    ldx #PL_ROLE
    ldy #4
    jmp pe_byte
pe_pos2:
    ; $FF (aucun) .. 3 : on decale de 1
    lda ed_pl+PL_POS2
    inc a
    and #$00FF
    ldy #5
    jsr ui_lr
    dec a
    sep #$20
    .a8
    sta ed_pl+PL_POS2
    rep #$20
    .a16
    rts
pe_skin:
    ldx #PL_SKIN
    ldy #3
    jmp pe_byte
pe_hair:
    ldx #PL_HAIR
    ldy #4
    jmp pe_byte
pe_hcol:
    ldx #PL_HCOL
    ldy #4
    jmp pe_byte

; pe_stat : A = caracteristique 0..6 -> +/- 1 dans le budget (1..9)
pe_stat:
    .a16
    .i16
    tax
    lda t7
    bit #JOY_LEFT
    beq @inc
    lda ed_pl+PL_STATS,x
    and #$00FF
    cmp #2
    bcc @no
    sep #$20
    .a8
    dec ed_pl+PL_STATS,x
    rep #$20
    .a16
    jmp ui_click
@inc:
    and #(JOY_RIGHT | JOY_A)
    beq @no
    lda ed_pl+PL_STATS,x
    and #$00FF
    cmp #9
    bcs @no
    sep #$20
    .a8
    inc ed_pl+PL_STATS,x
    rep #$20
    .a16
    phx
    jsr pe_cost
    plx
    cmp #STAT_BUDGET + 1
    bcc @ok
    sep #$20
    .a8
    dec ed_pl+PL_STATS,x        ; budget depasse : annule
    rep #$20
    .a16
@no:
    rts
@ok:
    jmp ui_click

; pe_cost : A = cout total des caracteristiques (8 coute 9, 9 coute 11)
pe_cost:
    .a16
    .i16
    stz t6
    ldx #0
@l: lda ed_pl+PL_STATS,x
    and #$00FF
    cmp #8
    bcc :+
    inc a
:   cmp #10
    bcc :+
    inc a
:   clc
    adc t6
    sta t6
    inx
    cpx #7
    bne @l
    lda t6
    rts

; pe_name : saisie du nom (8 caracteres)
pe_name:
    .a16
    .i16
    ldx #0
@c: lda ed_pl,x
    and #$00FF
    sep #$20
    .a8
    sta kb_buf,x
    rep #$20
    .a16
    inx
    cpx #8
    bne @c
    sep #$20
    .a8
    stz kb_buf+8
    rep #$20
    .a16
    ; retirer les espaces de fin
    ldx #8
@t: dex
    bmi @e
    lda kb_buf,x
    and #$00FF
    cmp #' '
    bne @e
    sep #$20
    .a8
    stz kb_buf,x
    rep #$20
    .a16
    bra @t
@e: lda #8
    sta kb_max
    jsr kb_edit
    ; recopier, completer par des espaces
    ldx #0
    ldy #0
@r: lda kb_buf,y
    and #$00FF
    beq :+
    iny
    bra :++
:   lda #' '
:   sep #$20
    .a8
    sta ed_pl,x
    rep #$20
    .a16
    inx
    cpx #8
    bne @r
    rts

pe_rows:
    .byte 3, 4, 5, 6, 7, 8, 9, 11, 12, 13, 14, 15, 16, 17, 21, 22, 23

pe_draw:
    .a16
    .i16
    lda #UI_ATTR
    sta t0
    ldx #TPOS(2, 3)
    ldy #.loword(str_pe_name)
    jsr print
    ; nom (8 caracteres)
    ldx #TPOS(14, 3)
    ldy #0
@n: lda ed_pl,y
    and #$00FF
    sec
    sbc #32
    clc
    adc #UI_HI
    sta bg3_map,x
    inx
    inx
    iny
    cpy #8
    bne @n
    lda #UI_ATTR
    sta t0
    ldx #TPOS(2, 4)
    ldy #.loword(str_pe_num)
    jsr print
    lda #UI_HI
    sta t0
    lda ed_pl+PL_NUM
    and #$00FF
    ldx #TPOS(14, 4)
    jsr print_num2
    lda #UI_ATTR
    sta t0
    ldx #TPOS(2, 5)
    ldy #.loword(str_pe_pos)
    jsr print
    lda ed_pl+PL_ROLE
    and #$00FF
    asl a
    tay
    lda role_names,y
    tay
    lda #UI_HI
    sta t0
    ldx #TPOS(14, 5)
    jsr print
    lda #UI_ATTR
    sta t0
    ldx #TPOS(2, 6)
    ldy #.loword(str_pe_pos2)
    jsr print
    ldy #.loword(str_pe_none)
    lda ed_pl+PL_POS2
    and #$00FF
    cmp #4
    bcs :+
    asl a
    tay
    lda role_names,y
    tay
:   lda #UI_HI
    sta t0
    lda #4
    sta t1
    ldx #TPOS(14, 6)
    jsr print_w
    ; apparence
    lda #UI_ATTR
    sta t0
    ldx #TPOS(2, 7)
    ldy #.loword(str_pe_skin)
    jsr print
    ldx #TPOS(2, 8)
    ldy #.loword(str_pe_hair)
    jsr print
    ldx #TPOS(2, 9)
    ldy #.loword(str_pe_hcol)
    jsr print
    lda #UI_HI
    sta t0
    lda ed_pl+PL_SKIN
    and #$00FF
    inc a
    ldx #TPOS(14, 7)
    jsr print_digit
    lda ed_pl+PL_HAIR
    and #$00FF
    inc a
    ldx #TPOS(14, 8)
    jsr print_digit
    lda ed_pl+PL_HCOL
    and #$00FF
    inc a
    ldx #TPOS(14, 9)
    jsr print_digit
    ; caracteristiques (libelles gardien si poste GK)
    stz t5
@s: lda #UI_ATTR
    sta t0
    lda t5
    asl a
    tay
    lda stat_labels,y
    ldx ed_pl+PL_ROLE-1         ; octet haut = poste
    cpx #$0100
    bcs :+
    lda gk_labels,y
:   tay
    lda t5
    clc
    adc #11
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc #2 * 2
    pha
    tax
    jsr print
    ; valeur et barre
    pla
    clc
    adc #12 * 2
    tax
    ldy t5
    lda ed_pl+PL_STATS,y
    and #$00FF
    sta t4
    clc
    adc #('0' - 32 + UI_HI)
    sta bg3_map,x
    inx
    inx
    inx
    inx
    ldy #1
@b: lda #('*' - 32 + UI_A)
    cpy t4
    beq :+
    bcc :+
    lda #UI_ATTR
:   sta bg3_map,x
    inx
    inx
    iny
    cpy #10
    bne @b
    inc t5
    lda t5
    cmp #7
    jne @s
    ; budget
    lda #UI_ATTR
    sta t0
    ldx #TPOS(2, 19)
    ldy #.loword(str_pe_pts)
    jsr print
    jsr pe_cost
    eor #$FFFF
    sec
    adc #STAT_BUDGET
    ldx #TPOS(14, 19)
    pha
    lda #UI_HI
    sta t0
    pla
    jsr print_num2
    lda #UI_ATTR
    sta t0
    ldx #TPOS(2, 21)
    ldy #.loword(str_ed_save)
    jsr print
    ldx #TPOS(2, 22)
    ldy #.loword(str_ed_delete)
    jsr print
    ldx #TPOS(2, 23)
    ldy #.loword(str_back)
    jsr print
    lda #.loword(pe_rows)
    sta t3
    lda #PE_ITEMS
    sta t4
    lda ui_sel
    jmp ui_cursor

; =============================================================================
;  CREATE TEAM : 8 emplacements, editeur (noms, couleurs, formation, effectif)
; =============================================================================
create_team:
    .a16
    .i16
    stz ed_slot
@list:
    jsr safe_screen_off
    jsr bg3_clear
    jsr ui_fill
    lda #UI_HI
    sta t0
    ldx #TPOS(10, 1)
    ldy #.loword(str_ct_title)
    jsr print
    jsr ctl_draw
    jsr screen_on
@loop:
    jsr ui_wait
    lda t7
    beq @loop
    bit #JOY_B
    beq :+
    jmp title_screen
:   lda ed_slot
    ldy #ED_TM_N
    jsr ui_updown
    cmp ed_slot
    beq :+
    sta ed_slot
    jsr ctl_draw
    bra @loop
:   lda t7
    and #(JOY_A | JOY_START)
    beq @loop
    jsr te_edit
    bra @list

ctl_draw:
    .a16
    .i16
    stz t5
@l: lda t5
    clc
    adc #4
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    sta near_tmp+2
    tax
    lda #UI_ATTR
    ldy t5
    cpy ed_slot
    bne :+
    lda #('>' - 32 + UI_HI)
:   sta bg3_map+2,x
    lda #UI_ATTR
    sta t0
    lda t5
    inc a
    inx
    inx
    inx
    inx
    jsr print_digit
    lda t5
    clc
    adc #NUM_TEAMS
    jsr team_valid
    bcc @empty
    jsr team_rec_id
    lda #UI_A
    sta t0
    bra @pr
@empty:
    ldy #.loword(str_ed_empty)
    lda #UI_ATTR
    sta t0
@pr:
    lda near_tmp+2
    clc
    adc #5 * 2
    tax
    lda #16
    sta t1
    jsr print_w
    inc t5
    lda t5
    cmp #ED_TM_N
    bne @l
    lda ed_slot
    clc
    adc #4
    jmp bg2_bar

; -----------------------------------------------------------------------------
;  te_edit : edition de l'equipe creee ed_slot (copie de travail ed_team)
; -----------------------------------------------------------------------------
TE_ITEMS = 23

te_edit:
    .a16
    .i16
    lda ed_slot
    clc
    adc #NUM_TEAMS
    jsr team_valid
    bcc @new
    jsr team_rec_id
    ldx #0
@cp:
    lda trec_buf,x
    sta ed_team,x
    inx
    inx
    cpx #TEAM_REC
    bne @cp
    ; couleurs choisies : retrouvees par la couleur principale
    lda ed_team+T_KIT
    jsr kitcol_find
    sta te_c1
    lda ed_team+T_AWAY
    jsr kitcol_find
    sta te_c2
    bra @src
@new:
    ; base : copie de l'equipe officielle 0, renommee
    lda #0
    jsr team_rec_id
    ldx #0
@cn:
    lda trec_buf,x
    sta ed_team,x
    inx
    inx
    cpx #TEAM_REC
    bne @cn
    ldx #0
@cz:
    stz ed_team,x
    inx
    inx
    cpx #T_KIT
    bne @cz
    lda #('N' | ('E' << 8))
    sta ed_team+T_NAME
    lda #('W' | (' ' << 8))
    sta ed_team+T_NAME+2
    lda #('T' | ('E' << 8))
    sta ed_team+T_NAME+4
    lda #('A' | ('M' << 8))
    sta ed_team+T_NAME+6
    lda #('N' | ('E' << 8))
    sta ed_team+T_SHORT
    lda #'W'
    sta ed_team+T_SHORT+2
    lda #('N' | ('E' << 8))
    sta ed_team+T_WORLD
    lda #('X' | ('U' << 8))
    sta ed_team+T_WORLD+2
    lda #'S'
    sta ed_team+T_WORLD+4
    stz te_c1
    lda #7
    sta te_c2
    jsr te_colors
@src:
    ; sources d'effectif : joueurs officiels de l'equipe 0 par defaut
    ldx #0
@s: txa
    clc
    adc #ED_PL_N                ; source = 32 + equipe*12 + joueur
    sta te_src,x
    inx
    inx
    cpx #24
    bne @s
    stz ui_sel
@redraw:
    jsr safe_screen_off
    jsr bg3_clear
    jsr ui_fill
    lda #UI_HI
    sta t0
    ldx #TPOS(10, 1)
    ldy #.loword(str_ct_title)
    jsr print
    jsr te_draw
    jsr screen_on
@loop:
    jsr ui_wait
    lda t7
    beq @loop
    bit #JOY_B
    beq :+
    rts
:   lda ui_sel
    ldy #TE_ITEMS
    jsr ui_updown
    cmp ui_sel
    beq :+
    sta ui_sel
    jsr te_draw
    bra @loop
:   lda ui_sel
    cmp #3
    bcs @n3
    ; textes
    lda t7
    and #(JOY_A | JOY_START)
    beq @loop
    jsr te_text
    jmp @redraw
@n3:
    cmp #8
    bcs @n8
    sec
    sbc #3
    asl a
    tax
    jsr (.loword(te_field_tab),x)
    jsr te_draw
    bra @loop
@n8:
    cmp #20
    bcs @act
    sec
    sbc #8
    jsr te_roster
    jsr te_draw
    jmp @loop
@act:
    lda t7
    and #(JOY_A | JOY_START)
    jeq @loop
    lda ui_sel
    cmp #20
    beq @save
    cmp #21
    beq @del
    rts
@save:
    jsr te_finalize
    lda #1
    bra @write
@del:
    lda #0
@write:
    sta t5
    lda ed_slot
    jsr cteam_offset
    tax
    ldy #0
@wr:
    lda ed_team,y
    sta f:SRAM_ED & $FF0000,x
    inx
    inx
    iny
    iny
    cpy #TEAM_REC
    bne @wr
    lda t5
    sta f:SRAM_ED & $FF0000,x   ; octet valide (+ reserve)
    jsr ed_commit
    lda #SFX_OK
    jsr sfx_play
    rts

te_field_tab:
    .word .loword(te_col1), .loword(te_col2), .loword(te_form), .loword(te_ment), .loword(te_press)

te_col1:
    lda te_c1
    ldy #NUM_KITCOLS
    jsr ui_lr
    sta te_c1
    jmp te_colors
te_col2:
    lda te_c2
    ldy #NUM_KITCOLS
    jsr ui_lr
    sta te_c2
    jmp te_colors
te_form:
    ldx #T_FORM
    ldy #NUM_FORMS
    bra te_byte
te_ment:
    ldx #T_TACT
    ldy #3
    bra te_byte
te_press:
    ldx #T_TACT + 2
    ldy #3
te_byte:
    .a16
    phx
    lda ed_team,x
    and #$00FF
    jsr ui_lr
    plx
    sep #$20
    .a8
    sta ed_team,x
    rep #$20
    .a16
    rts

; kitcol_find : A = couleur BGR555 -> A = index dans la palette proposee (0 si inconnue)
kitcol_find:
    .a16
    .i16
    ldx #0
@l: cmp kitcol_main,x
    beq @f
    inx
    inx
    cpx #NUM_KITCOLS * 2
    bne @l
    lda #0
    rts
@f: txa
    lsr a
    rts

; te_colors : maillots domicile (couleur 1, liseré couleur 2) et exterieur (inverse)
te_colors:
    .a16
    .i16
    lda te_c1
    jsr kitcol_ptr
    ldx #0
@h: phx
    tyx
    lda f:kitcol_kit,x
    plx
    sta ed_team+T_KIT,x
    iny
    iny
    inx
    inx
    cpx #10
    bne @h
    lda te_c2
    asl a
    tay
    lda kitcol_main,y
    sta ed_team+T_KIT+4         ; liseré
    lda te_c2
    jsr kitcol_ptr
    ldx #0
@a: phx
    tyx
    lda f:kitcol_kit,x
    plx
    sta ed_team+T_AWAY,x
    iny
    iny
    inx
    inx
    cpx #10
    bne @a
    lda te_c1
    asl a
    tay
    lda kitcol_main,y
    sta ed_team+T_AWAY+4
    ; familles
    ldy te_c1
    lda kitcol_fam,y
    and #$00FF
    sta t4
    ldy te_c2
    lda kitcol_fam,y
    and #$00FF
    xba
    ora t4
    sta ed_team+T_FAM
    rts

; kitcol_ptr : A = couleur -> Y = offset dans kitcol_kit (10 octets par couleur)
kitcol_ptr:
    .a16
    asl a
    sta t6
    asl a
    asl a
    clc
    adc t6
    tay
    rts

; te_text : saisie du nom (15), du nom court (3) ou du monde (15)
te_text:
    .a16
    .i16
    lda ui_sel
    ldx #T_NAME
    ldy #15
    cmp #1
    bne :+
    ldx #T_SHORT
    ldy #3
:   cmp #2
    bne :+
    ldx #T_WORLD
    ldy #15
:   stx t5
    sty kb_max
    ; copie vers kb_buf
    ldy #0
@c: lda ed_team,x
    and #$00FF
    sep #$20
    .a8
    sta kb_buf,y
    rep #$20
    .a16
    beq :+
    inx
    iny
    cpy kb_max
    bne @c
    sep #$20
    .a8
    lda #0
    sta kb_buf,y
    rep #$20
    .a16
:   jsr kb_edit
    ; retour (termine par 0)
    ldx t5
    ldy #0
@r: lda kb_buf,y
    and #$00FF
    sep #$20
    .a8
    sta ed_team,x
    rep #$20
    .a16
    beq @d
    inx
    iny
    bra @r
@d: rts

; te_roster : A = place 0..11 de l'effectif -> source suivante / precedente
;  sources : 0..31 joueurs crees (valides), 32..223 joueurs des equipes officielles
te_roster:
    .a16
    .i16
    asl a
    sta te_k
    tax
    lda te_src,x
    sta ts_cur
    lda t7
    bit #JOY_LEFT
    bne @prev
    and #(JOY_RIGHT | JOY_A)
    bne @next
    rts
@next:
    lda ts_cur
    inc a
    cmp #ED_PL_N + NUM_TEAMS * 12
    bcc :+
    lda #0
:   sta ts_cur
    jsr src_valid
    bcc @next
    bra @set
@prev:
    lda ts_cur
    dec a
    bpl :+
    lda #ED_PL_N + NUM_TEAMS * 12 - 1
:   sta ts_cur
    jsr src_valid
    bcc @prev
@set:
    jsr ui_click
    ldx te_k
    lda ts_cur
    sta te_src,x
    ; copie des 16 octets (nom, poste, caracteristiques)
    lda te_k
    asl a
    asl a
    asl a
    clc
    adc #T_PLAYERS              ; ed_team + 68 + k*16
    sta t4
    jsr src_copy
    rts

; src_valid : ts_cur -> carry = 1 si la source existe
src_valid:
    .a16
    lda ts_cur
    cmp #ED_PL_N
    bcs @yes
    jsr cplayer_offset
    lda f:SRAM_ED & $FF0000 + PL_VALID,x
    and #$00FF
    cmp #1
    rts
@yes:
    sec
    rts

; src_copy : ts_cur -> ed_team + t4 (16 octets)
src_copy:
    .a16
    .i16
    lda ts_cur
    cmp #ED_PL_N
    bcs @off
    jsr cplayer_offset
    ldy t4
    lda #8
    sta t6
@c: lda f:SRAM_ED & $FF0000,x
    sta ed_team,y
    inx
    inx
    iny
    iny
    dec t6
    bne @c
    lda f:SRAM_ED & $FF0000 + (PL_SKIN - 16),x
    and #$00FF                  ; teinte 0 : claire, 1-2 : foncee
    jmp set_skin_bit
@off:
    sec
    sbc #ED_PL_N
    ldx #12
    jsr divu
    pha                         ; equipe
    lda RDMPYL
    asl a
    asl a
    asl a
    asl a
    clc
    adc #T_PLAYERS
    sta t6                      ; offset du joueur
    lda RDMPYL
    sta t5                      ; joueur dans son equipe
    pla
    jsr team_rec_id
    tya
    clc
    adc t6
    tax
    ldy t4
    lda #8
    sta t6
@o: lda a:0,x
    sta ed_team,y
    inx
    inx
    iny
    iny
    dec t6
    bne @o
    lda t5
    asl a
    tay
    lda bit_tab,y
    and trec_buf+T_SKIN
    ; continue

; set_skin_bit : A != 0 -> peau foncee pour la place te_k/2 de l'effectif
set_skin_bit:
    .a16
    .i16
    pha
    ldy te_k
    lda bit_tab,y
    eor #$FFFF
    and ed_team+T_SKIN
    sta ed_team+T_SKIN
    pla
    beq :+
    lda bit_tab,y
    ora ed_team+T_SKIN
    sta ed_team+T_SKIN
:   rts

; te_finalize : niveau global = moyenne des caracteristiques, style BALANCED
te_finalize:
    .a16
    .i16
    stz t6
    ldx #0                      ; joueur * 16
@p: ldy #7
    phx
@l: lda ed_team+T_PLAYERS+PL_STATS,x
    and #$00FF
    clc
    adc t6
    sta t6
    inx
    dey
    bne @l
    pla
    clc
    adc #16
    tax
    cpx #12 * 16
    bne @p
    lda t6
    ldx #84                     ; 12 joueurs x 7
    jsr divu
    cmp #1
    bcs :+
    lda #1
:   cmp #10
    bcc :+
    lda #9
:   sep #$20
    .a8
    sta ed_team+T_LEVEL
    lda #6
    sta ed_team+T_STYLE
    lda #1
    sta ed_team+T_TACT+1
    sta ed_team+T_TACT+3
    sta ed_team+T_TACT+4
    sta ed_team+T_TACT+5
    rep #$20
    .a16
    rts

te_rows:
    .byte 3, 4, 5, 6, 7, 8, 9, 10
    .byte 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23
    .byte 25, 26, 27

te_draw:
    .a16
    .i16
    ; libelles et valeurs
    lda #UI_ATTR
    sta t0
    ldx #TPOS(2, 3)
    ldy #.loword(str_te_name)
    jsr print
    ldx #TPOS(2, 4)
    ldy #.loword(str_te_short)
    jsr print
    ldx #TPOS(2, 5)
    ldy #.loword(str_te_world)
    jsr print
    ldx #TPOS(2, 6)
    ldy #.loword(str_te_col1)
    jsr print
    ldx #TPOS(2, 7)
    ldy #.loword(str_te_col2)
    jsr print
    ldx #TPOS(2, 8)
    ldy #.loword(str_formation)
    jsr print
    ldx #TPOS(2, 9)
    ldy #.loword(str_t_ment)
    jsr print
    ldx #TPOS(2, 10)
    ldy #.loword(str_t_press)
    jsr print
    lda #UI_HI
    sta t0
    lda #16
    sta t1
    ldx #TPOS(13, 3)
    ldy #.loword(ed_team + T_NAME)
    jsr print_w
    ldx #TPOS(13, 4)
    ldy #.loword(ed_team + T_SHORT)
    lda #4
    sta t1
    jsr print_w
    ldx #TPOS(13, 5)
    ldy #.loword(ed_team + T_WORLD)
    lda #16
    sta t1
    jsr print_w
    lda te_c1
    jsr kc_name
    ldx #TPOS(13, 6)
    jsr print_w
    lda te_c2
    jsr kc_name
    ldx #TPOS(13, 7)
    jsr print_w
    lda ed_team+T_FORM
    and #$00FF
    sta t6
    asl a
    asl a
    asl a
    sec
    sbc t6
    sbc t6
    clc
    adc #.loword(form_names)
    tay
    ldx #TPOS(13, 8)
    jsr print_w
    lda ed_team+T_TACT
    and #$00FF
    asl a
    tay
    lda tact_vals,y
    tay
    ldx #TPOS(13, 9)
    jsr print_w
    lda ed_team+T_TACT+2
    and #$00FF
    clc
    adc #6
    asl a
    tay
    lda tact_vals,y
    tay
    ldx #TPOS(13, 10)
    jsr print_w
    ; effectif
    stz t5
@r: lda t5
    clc
    adc #12
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    sta near_tmp+2
    lda t5
    asl a
    asl a
    asl a
    asl a
    clc
    adc #T_PLAYERS
    sta t4
    tay
    lda ed_team+PL_ROLE,y
    and #$00FF
    asl a
    tay
    lda role_names,y
    tay
    lda #UI_ATTR
    sta t0
    lda near_tmp+2
    clc
    adc #3 * 2
    tax
    jsr print
    ; nom
    ldy t4
    lda near_tmp+2
    clc
    adc #6 * 2
    tax
    lda #8
    sta t6
@nm:
    lda ed_team,y
    and #$00FF
    sec
    sbc #32
    clc
    adc #UI_HI
    sta bg3_map,x
    inx
    inx
    iny
    dec t6
    bne @nm
    ; caracteristiques compactes
    ldy t4
    lda #7
    sta t6
    inx
    inx
@st:
    lda ed_team+PL_STATS,y
    and #$00FF
    clc
    adc #('0' - 32 + UI_ATTR)
    sta bg3_map,x
    inx
    inx
    iny
    dec t6
    bne @st
    ; createur ?
    lda t5
    asl a
    tay
    lda te_src,y
    cmp #ED_PL_N
    bcs :+
    lda #('*' - 32 + UI_A)
    sta bg3_map,x
:   inc t5
    lda t5
    cmp #12
    jne @r
    lda #UI_ATTR
    sta t0
    ldx #TPOS(2, 25)
    ldy #.loword(str_ed_save)
    jsr print
    ldx #TPOS(2, 26)
    ldy #.loword(str_ed_delete)
    jsr print
    ldx #TPOS(2, 27)
    ldy #.loword(str_back)
    jsr print
    lda #.loword(te_rows)
    sta t3
    lda #TE_ITEMS
    sta t4
    lda ui_sel
    jmp ui_cursor

; kc_name : A = couleur -> Y = nom, t0 = UI_HI, t1 = 9
kc_name:
    .a16
    sta t6
    asl a
    asl a
    asl a
    clc
    adc t6                      ; *9
    clc
    adc #.loword(kitcol_names)
    tay
    lda #UI_HI
    sta t0
    lda #9
    sta t1
    rts

.segment "RODATA"
kb_chars:       .byte "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-. '"
str_kb_title:   .byte "ENTER NAME", 0
str_kb_help:    .byte "A ADD  B DELETE  START DONE", 0
str_cp_title:   .byte "CREATE PLAYER", 0
str_ct_title:   .byte "CREATE TEAM", 0
str_ed_empty:   .byte "---", 0
str_ed_save:    .byte "SAVE", 0
str_ed_delete:  .byte "DELETE", 0
str_pe_name:    .byte "NAME", 0
str_pe_num:     .byte "NUMBER", 0
str_pe_pos:     .byte "POSITION", 0
str_pe_pos2:    .byte "SECOND POS.", 0
str_pe_none:    .byte "NONE", 0
str_pe_skin:    .byte "SKIN TONE", 0
str_pe_hair:    .byte "HAIR", 0
str_pe_hcol:    .byte "HAIR COLOR", 0
str_pe_pts:     .byte "POINTS LEFT", 0
str_te_name:    .byte "TEAM NAME", 0
str_te_short:   .byte "SHORT NAME", 0
str_te_world:   .byte "HOME WORLD", 0
str_te_col1:    .byte "PRIMARY", 0
str_te_col2:    .byte "SECONDARY", 0
str_sp:         .byte "SPEED", 0
str_po:         .byte "POWER", 0
str_pa:         .byte "PASS", 0
str_ki:         .byte "KICK", 0
str_co:         .byte "CONTROL", 0
str_de:         .byte "DEFENSE", 0
str_sta:        .byte "STAMINA", 0
str_re:         .byte "REFLEX", 0
str_pos:        .byte "POSITION ", 0
str_th:         .byte "THROW", 0
str_ca:         .byte "CATCH", 0
stat_labels:
    .word .loword(str_sp), .loword(str_po), .loword(str_pa), .loword(str_ki)
    .word .loword(str_co), .loword(str_de), .loword(str_sta)
gk_labels:
    .word .loword(str_re), .loword(str_po), .loword(str_pos), .loword(str_th)
    .word .loword(str_ca), .loword(str_pos), .loword(str_sta)
.segment "CODE2"
