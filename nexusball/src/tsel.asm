; =============================================================================
;  tsel.asm - choix des equipes (ecran visuel) : deux fiches DOMICILE / EXTERIEUR
;  avec les joueurs au maillot de l'equipe, niveau, style et moyennes de
;  l'effectif. Gauche / droite : equipe, haut / bas : cote, A / START / B : retour.
; =============================================================================

TS_CARD_W = 15

; team_select : A = cote actif au depart -> C = 1 si B (retour)
team_select:
    .a16
    .i16
    sta ts_init
    jsr safe_screen_off
    jsr bg3_clear
    jsr oam_clear
    jsr ui_fill
    ; fiches
    stz ts_side
@cards:
    lda ts_side
    beq :+
    lda #16
:   inc a
    sta pn_x
    lda #3
    sta pn_y
    lda #TS_CARD_W - 1
    sta pn_w
    lda #20
    sta pn_h
    jsr bg2_panel
    inc ts_side
    lda ts_side
    cmp #2
    bne @cards
    lda ts_init
    sta ts_side
    lda #UI_HI
    sta t0
    ldx #TPOS(2, 1)
    ldy #.loword(str_tsel)
    jsr print
    lda #UI_HI
    sta t0
    ldx #TPOS(15, 12)
    ldy #.loword(str_c_vs)
    jsr print
    lda #UI_ATTR
    sta t0
    ldx #TPOS(1, 25)
    ldy #.loword(str_tsel_help)
    jsr print
    jsr load_kits
    lda #0
    jsr ts_draw_card
    lda #1
    jsr ts_draw_card
    jsr ts_bar
    jsr screen_on
@loop:
    jsr ts_sprites
    jsr wait_frame
    jsr crowd_update
    lda joy_new
    ora joy_new+2
    sta t7
    beq @loop
    bit #JOY_B
    jne @back
    bit #(JOY_A | JOY_START)
    jne @done
    bit #(JOY_X | JOY_Y)
    beq @nokit
    ; X / Y : tenue domicile <-> exterieur pour la fiche active
    jsr ui_click
    lda ts_side
    asl a
    tax
    lda kit_away,x              ; tenue actuelle (0 domicile, 1 exterieur)
    eor #1
    inc a                       ; -> 1 domicile, 2 exterieur
    sta kit_pick,x
    bra @reload
@nokit:
    bit #(JOY_UP | JOY_DOWN)
    beq @lr
    jsr ui_click
    lda ts_side
    eor #1
    sta ts_side
    jsr ts_bar
    bra @loop
@lr:
    and #(JOY_LEFT | JOY_RIGHT)
    beq @loop
    sta t7
    lda ts_side
    asl a
    tax
    lda team_id,x
    jsr team_step
    ldx ts_side
    cpx #0
    beq :+
    ldx #2
:   cmp team_id,x
    beq @loop
    sta team_id,x
    stz kit_pick,x              ; nouvelle equipe : tenue automatique
    lda ts_side
    jsr team_defaults
@reload:
    ; maillots (CGRAM) : ecran force eteint le temps de l'ecriture, juste apres le VBlank
    jsr wait_frame
    sep #$20
    .a8
    lda #$80
    sta INIDISP
    rep #$20
    .a16
    jsr load_kits
    sep #$20
    .a8
    lda inidisp
    sta INIDISP
    rep #$20
    .a16
    lda #0
    jsr ts_draw_card
    lda #1
    jsr ts_draw_card
    jmp @loop
@done:
    lda #SFX_OK
    jsr sfx_play
    jsr oam_clear
    clc
    rts
@back:
    jsr oam_clear
    sec
    rts

; ts_bar : barre de selection sur l'en-tete de la fiche active
ts_bar:
    .a16
    .i16
    lda ts_side
    beq :+
    lda #16
:   clc
    adc #2
    sta bar_x0
    lda #TS_CARD_W - 3
    sta bar_w
    lda #4
    jmp bg2_bar

; ts_col : A = cote -> ts_x = colonne interieure de la fiche (2 ou 18)
ts_col:
    .a16
    beq :+
    lda #16
:   clc
    adc #2
    sta ts_x
    rts

; ts_pos : A = ligne -> X = position (colonne ts_x)
ts_pos:
    .a16
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc ts_x
    asl a
    tax
    rts

; ts_draw_card : A = cote
ts_draw_card:
    .a16
    .i16
    sta ts_cur_side
    jsr ts_col
    ; efface l'interieur (lignes 4..21)
    lda #4
    sta ts_row
@clr:
    lda ts_row
    jsr ts_pos
    ldy #TS_CARD_W - 3
    lda #0
    jsr fill_tiles
    inc ts_row
    lda ts_row
    cmp #22
    bne @clr
    ; en-tete : DOM. / EXT. et qui joue
    lda #UI_HI
    sta t0
    lda #4
    jsr ts_pos
    ldy #.loword(str_home)
    lda ts_cur_side
    beq :+
    ldy #.loword(str_away)
:   jsr print
    lda #4
    jsr ts_pos
    txa
    clc
    adc #9 * 2
    tax
    ldy #.loword(str_cpu)
    lda game_mode
    cmp #MODE_CPU
    beq @who
    lda ts_cur_side
    bne :+
    ldy #.loword(str_p1)
    bra @who
:   lda game_mode
    cmp #MODE_2P
    bne @who
    ldy #.loword(str_p2)
@who:
    jsr print
    ; nom sur deux lignes (coupe au premier espace), couleur de l'equipe
    lda #TXT_ATTR + $0400
    ldx ts_cur_side
    beq :+
    lda #TXT_ATTR + $0C00
:   sta t0
    lda ts_cur_side
    jsr team_rec
    sty ts_ptr
    lda #6
    jsr ts_pos
    ldy ts_ptr
    jsr ts_word
    lda #7
    jsr ts_pos
    jsr ts_word
    ; fleches
    lda #('<' - 32 + UI_HI)
    pha
    lda #6
    jsr ts_pos
    pla
    sta bg3_map-2,x
    lda #('>' - 32 + UI_HI)
    sta bg3_map+(TS_CARD_W - 3) * 2,x
    ; monde d'origine
    lda #UI_ATTR
    sta t0
    lda ts_cur_side
    jsr team_rec
    tya
    clc
    adc #T_WORLD
    tay
    lda #9
    jsr ts_pos
    jsr print
    ; tenue (X pour changer)
    lda #UI_A
    sta t0
    lda #10
    jsr ts_pos
    lda ts_cur_side
    asl a
    tay
    lda kit_away,y
    ldy #.loword(str_kit_h)
    cmp #0
    beq :+
    ldy #.loword(str_kit_a)
:   jsr print
    ; style et niveau
    lda #UI_HI
    sta t0
    lda ts_cur_side
    jsr team_rec
    lda a:T_STYLE,y
    and #$00FF
    sta t2
    asl a
    asl a
    clc
    adc t2
    asl a
    adc #.loword(style_names)
    tay
    lda #16
    jsr ts_pos
    jsr print
    lda ts_cur_side
    jsr team_rec
    lda a:T_LEVEL,y
    and #$00FF
    sta ts_n
    lda #17
    jsr ts_pos
    lda #('*' - 32 + UI_HI)
@lv:
    sta bg3_map,x
    inx
    inx
    dec ts_n
    bne @lv
    ; moyennes de l'effectif (joueurs de champ titulaires)
    stz ts_k
@stat:
    jsr ts_avg
    sta ts_n
    lda ts_k
    clc
    adc #18
    jsr ts_pos
    phx
    lda ts_k
    asl a
    tay
    lda ts_stat_names,y
    tay
    lda #UI_ATTR
    sta t0
    jsr print
    plx
    txa
    clc
    adc #3 * 2
    tax
    ldy #1
@bar:
    lda #('*' - 32 + UI_A)
    cpy ts_n
    beq :+
    bcc :+
    lda #('-' - 32 + UI_ATTR)
:   sta bg3_map,x
    inx
    inx
    iny
    cpy #10
    bne @bar
    inc ts_k
    lda ts_k
    cmp #4
    bne @stat
    lda #1
    sta bg3_dirty
    rts

; ts_word : ecrit un mot du nom (Y avance apres l'espace), X = position
ts_word:
    .a16
    .i16
@l: lda a:0,y
    and #$00FF
    beq @d
    iny
    cmp #' '
    beq @d
    sec
    sbc #32
    clc
    adc t0
    sta bg3_map,x
    inx
    inx
    bra @l
@d: rts

; ts_avg : ts_k = caracteristique (0 vitesse, 1 puissance, 2 passe, 3 defense)
;          -> A = moyenne (1..9) des 5 joueurs de champ titulaires
ts_avg:
    .a16
    .i16
    lda ts_k
    tay
    lda ts_stat_ofs,y
    and #$00FF
    sta ts_ofs
    stz ts_sum
    lda #1
    sta ts_slot
@l: lda ts_cur_side
    beq :+
    lda #12
:   sta ts_t
    lda ts_slot
    asl a
    clc
    adc ts_t
    tax
    lda lineup,x
    tax
    lda ts_cur_side
    jsr roster_rec
    tya
    clc
    adc ts_ofs
    tay
    lda a:PL_STATS,y
    and #$00FF
    clc
    adc ts_sum
    sta ts_sum
    inc ts_slot
    lda ts_slot
    cmp #TEAM_SIZE
    bne @l
    lda ts_sum
    clc
    adc #2                      ; arrondi
    ldx #5
    jsr divu
    cmp #1
    bcs :+
    lda #1
:   cmp #10
    bcc :+
    lda #9
:   rts

; ts_sprites : trois joueurs au maillot de chaque equipe (gardien au centre), qui courent sur place
ts_sprites:
    .a16
    .i16
    jsr oam_begin
    stz ts_k
@l: lda ts_k
    tay
    lda ts_spx,y
    and #$00FF
    sta ts_x
    ; cadre d'animation : course
    lda frame
    lsr a
    lsr a
    lsr a
    clc
    adc ts_k
    and #$0003
    tay
    lda ts_frames,y
    and #$00FF
    sta ts_t
    ; palette : equipe 0 / 1, gardien 2 / 3
    lda ts_k
    tay
    lda ts_spp,y
    and #$00FF
    xba
    asl a                       ; palette << 9
    ora #OBJ_PRIO
    ora ts_t
    ldx ts_k
    cpx #3
    bcc :+
    ora #$4000                  ; fiche de droite : joueurs tournes vers la gauche
:   sta ts_attr
    ; haut (16x16) puis bas
    sta t1
    lda #1
    sta t2
    lda #90
    sta t0
    lda ts_x
    jsr oam_add
    lda ts_attr
    clc
    adc #32
    sta t1
    lda #1
    sta t2
    lda #106
    sta t0
    lda ts_x
    jsr oam_add
    inc ts_k
    lda ts_k
    cmp #6
    bne @l
    jmp oam_finish

.segment "RODATA"
ts_spx:     .byte 24, 52, 80, 160, 188, 216
ts_spp:     .byte 0, 2, 0, 1, 3, 1
ts_frames:  .byte SPR_RUN1, SPR_RUN2, SPR_RUN3, SPR_RUN2
ts_stat_ofs: .byte 0, 1, 2, 5
ts_stat_names: .word .loword(str_ts_spd), .loword(str_ts_pow), .loword(str_ts_pas), .loword(str_ts_def)
str_tsel:       .byte "TEAM SELECT", 0
str_tsel_help:  .byte "<>TEAM UP/DOWN SIDE X KIT A OK", 0
str_kit_h:      .byte "HOME KIT", 0
str_kit_a:      .byte "AWAY KIT", 0
str_ts_spd:     .byte "SPD", 0
str_ts_pow:     .byte "POW", 0
str_ts_pas:     .byte "PAS", 0
str_ts_def:     .byte "DEF", 0
.segment "CODE"
