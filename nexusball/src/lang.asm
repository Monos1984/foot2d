; =============================================================================
;  lang.asm - langue (anglais / francais) et ecran des regles
;
;  Les chaines du jeu restent ecrites en anglais. Quand lang = 1, print cherche
;  la chaine dans tr_tab (banque $C0, triee par hash, genere par tools/lang.py),
;  verifie le texte anglais complet puis affiche la traduction copiee dans tr_buf.
; =============================================================================

; tr_str : Y = chaine anglaise -> Y = tr_buf et C = 1 si une traduction existe
;          (sinon Y inchange, C = 0). tr_y = chaine d'origine. Preserve X.
tr_str:
    .a16
    .i16
    lda lang
    bne :+
    clc
    rts
:   phx
    lda #^tr_tab
    sta tr_zp+2                 ; (ecrit aussi tr_zp2, initialise juste apres)
    sty tr_y
    sty tr_zp2
    ; hash h = (h * 33) xor c
    stz tr_h
@h: lda a:0,y
    and #$00FF
    beq @hd
    sta tr_c
    lda tr_h
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc tr_h
    eor tr_c
    sta tr_h
    iny
    bra @h
@hd:
    ldx #0
@s: lda f:tr_tab,x
    cmp tr_h
    beq @cand
    bcs @no                     ; table triee : hash depasse
@nx:
    txa
    clc
    adc #6
    tax
    cpx #TR_COUNT * 6
    bcc @s
@no:
    ldy tr_y
    plx
    clc
    rts
@cand:
    lda f:tr_tab+2,x
    sta tr_zp
    sep #$20
    .a8
    ldy #0
@c: lda (tr_zp2),y
    cmp [tr_zp],y
    bne @ne
    cmp #0
    beq @eq
    iny
    bra @c
@ne:
    rep #$20
    .a16
    bra @nx
@eq:
    rep #$20
    .a16
    lda f:tr_tab+4,x
    sta tr_zp
    sep #$20
    .a8
    ldy #0
@cp:
    lda [tr_zp],y
    sta tr_buf,y
    beq :+
    iny
    bra @cp
:   rep #$20
    .a16
    ldy #.loword(tr_buf)
    plx
    sec
    rts

; -----------------------------------------------------------------------------
;  rules_screen : 5 pages (gauche / droite), B ou START pour revenir
; -----------------------------------------------------------------------------
rules_screen:
    .a16
    .i16
    stz rl_page
@redraw:
    jsr safe_screen_off
    jsr rules_draw
    jsr screen_on
@loop:
    jsr ui_wait
    lda t7
    beq @loop
    bit #(JOY_B | JOY_START)
    bne @back
    lda rl_page
    ldy #RULES_PAGES
    jsr ui_lr
    cmp rl_page
    beq @loop
    sta rl_page
    jsr rules_draw
    bra @loop
@back:
    jmp title_screen

rules_draw:
    .a16
    .i16
    jsr bg3_clear
    jsr ui_fill
    lda #UI_HI
    sta t0
    ldx #TPOS(2, 1)
    ldy #.loword(str_t_rules)
    jsr print
    ; numero de page "n/5"
    lda rl_page
    inc a
    ldx #TPOS(27, 1)
    jsr print_digit
    lda #('/' - 32 + UI_HI)
    sta bg3_map + TPOS(28, 1)
    lda #RULES_PAGES
    ldx #TPOS(29, 1)
    jsr print_digit
    ; texte de la page : (RULES_LINES + 1) lignes par page, la premiere est le titre
    lda #.loword(rules_en)
    ldx lang
    beq :+
    lda #.loword(rules_fr)
:   sta tr_zp
    lda #^rules_en
    sta tr_zp+2
    lda rl_page
    sta t5
@skip:
    lda t5
    beq @page
    ldx #RULES_LINES + 1
@sl:
    jsr far_skip
    dex
    bne @sl
    dec t5
    bra @skip
@page:
    lda #UI_HI
    sta t0
    ldx #TPOS(2, 3)
    jsr print_far
    lda #UI_ATTR
    sta t0
    lda #TPOS(2, 5)
    sta t5
    ldy #RULES_LINES
@ln:
    phy
    ldx t5
    jsr print_far
    lda t5
    clc
    adc #64
    sta t5
    ply
    dey
    bne @ln
    ; aide
    lda #UI_ATTR
    sta t0
    ldx #TPOS(2, 25)
    ldy #.loword(str_rules_help)
    jsr print
    lda #1
    sta bg3_dirty
    rts

; far_skip : avance tr_zp apres la prochaine chaine. Preserve X.
far_skip:
    .a16
    .i16
    sep #$20
    .a8
    ldy #0
:   lda [tr_zp],y
    beq :+
    iny
    bra :-
:   rep #$20
    .a16
    iny
    tya
    clc
    adc tr_zp
    sta tr_zp
    rts

; print_far : chaine longue en [tr_zp] -> X, t0 = attributs ; tr_zp avance apres la chaine
print_far:
    .a16
    .i16
    ldy #0
@l: lda [tr_zp],y
    and #$00FF
    beq @d
    sec
    sbc #32
    clc
    adc t0
    sta bg3_map,x
    inx
    inx
    iny
    bra @l
@d: iny
    tya
    clc
    adc tr_zp
    sta tr_zp
    rts

.segment "RODATA"
str_t_rules:    .byte "RULES", 0
str_rules_help: .byte "<> PAGE    B BACK", 0
str_lang:       .byte "LANGUAGE", 0
str_english:    .byte "ENGLISH ", 0
str_french:     .byte "FRAN", $5C, "AIS", 0
.segment "CODE"

.include "data/gen/lang.inc"
