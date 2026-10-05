; =============================================================================
;  result.asm - ecran de score (mi-temps et fin de match) sur le fond des menus :
;  plaques des equipes et gros chiffres (tiles du HUD), noms complets,
;  statistiques avec barres aux couleurs des equipes, tirs au but.
; =============================================================================

.segment "CODE2"

RS_BAR = 6                      ; cases par barre

; result_screen : A = 0 mi-temps (continue seul apres 5 s), 1 fin de match
; Rend la main ecran allume (fin de match) ou eteint (mi-temps).
result_screen:
    .a16
    .i16
    sta rs_mode
    jsr screen_off
    jsr oam_clear
    jsr bg3_clear
    ; plaques et score : le HUD est dessine en haut puis recopie lignes 5 a 7
    jsr hud_draw_static
    jsr hud_update
    ldx #0
@cp:
    lda bg3_map,x
    sta bg3_map + 6 * 64,x
    stz bg3_map,x
    inx
    inx
    cpx #3 * 64
    bne @cp
    ; pas d'horloge : seulement la periode
    ldx #TPOS(12, 8)
    ldy #4
    lda #HUD_ATTR
    jsr fill_tiles
    jsr ui_fill
    ; titre : bandeau neon (dessine sur la ligne des messages puis deplace en haut)
    ldy #.loword(str_half)
    lda rs_mode
    beq :+
    ldy #.loword(str_fulltime)
:   jsr show_msg
    stz msg_time
    ldx #0
@tt:
    lda bg3_map + (MSG_ROW - 1) * 64,x
    sta bg3_map + 2 * 64,x
    stz bg3_map + (MSG_ROW - 1) * 64,x
    inx
    inx
    cpx #3 * 64
    bne @tt
    ; noms complets
    lda #TXT_ATTR + $0400
    sta t0
    lda #0
    jsr team_rec
    ldx #TPOS(2, 11)
    jsr print
    lda #TXT_ATTR + $0C00
    sta t0
    lda #1
    jsr team_rec
    sty rs_t
    ldx #0
:   lda a:0,y
    and #$00FF
    beq :+
    inx
    iny
    bra :-
:   txa
    eor #$FFFF
    sec
    adc #30                     ; colonne = 30 - longueur
    asl a
    clc
    adc #TPOS(0, 11)
    tax
    ldy rs_t
    jsr print
    ; statistiques
    lda #14
    sta rs_row
    lda st_shots
    sta rs_a
    lda st_shots+2
    sta rs_b
    ldy #.loword(str_st_shots)
    jsr rs_stat
    jsr poss_pct
    lda t6
    sta rs_a
    lda t7
    sta rs_b
    ldy #.loword(str_st_poss)
    jsr rs_stat
    lda st_fouls
    sta rs_a
    lda st_fouls+2
    sta rs_b
    ldy #.loword(str_st_fouls)
    jsr rs_stat
    ; tirs au but
    lda so_active
    beq @noso
    lda #UI_HI
    sta t0
    ldx #TPOS(10, 21)
    ldy #.loword(str_so_res)
    jsr print
    lda so_goals
    ldx #TPOS(20, 21)
    jsr print_digit
    lda #('-' - 32 + UI_HI)
    sta bg3_map + TPOS(21, 21)
    lda so_goals+2
    ldx #TPOS(22, 21)
    jsr print_digit
@noso:
    lda rs_mode
    jeq @nowin
    ; vainqueur (score, puis tirs au but) ou match nul
    lda score
    cmp score+2
    bne @cmp
    lda so_active
    beq @draw
    lda so_goals
    cmp so_goals+2
    beq @draw
@cmp:
    lda #0
    bcs :+
    lda #1
:   sta rs_s
    lda #UI_HI
    sta t0
    ldx #TPOS(4, 23)
    ldy #.loword(str_winner)
    jsr print
    lda #TXT_ATTR + $0400
    ldy rs_s
    beq :+
    lda #TXT_ATTR + $0C00
:   sta t0
    lda rs_s
    jsr team_rec
    ldx #TPOS(14, 23)
    jsr print
    bra @ps
@draw:
    lda #UI_HI
    sta t0
    ldx #TPOS(12, 23)
    ldy #.loword(str_draw)
    jsr print
@ps:
    lda #UI_ATTR
    sta t0
    ldx #TPOS(10, 25)
    ldy #.loword(str_press_start)
    jsr print
@nowin:
:   lda #1
    sta bg3_dirty
    jsr screen_on
    stz rs_t
@w: jsr wait_frame
    jsr oam_begin
    jsr oam_finish
    jsr crowd_update
    lda rs_t
    clc
    adc rc+RC_TDEC              ; temps logique (identique en PAL / NTSC)
    sta rs_t
    cmp #TU_SEC / 2
    bcc @w                      ; pas de saut accidentel
    lda joy_new
    ora joy_new+2
    and #(JOY_START | JOY_A | JOY_B)
    bne @out
    lda rs_mode
    bne :+
    lda rs_t
    cmp #TU_SEC * 5             ; mi-temps : 5 s
    bcs @out
    bra @w
:   lda game_mode
    cmp #MODE_CPU
    bne @w
    lda rs_t
    cmp #TU_SEC * 10            ; demo CPU : 10 s
    bcc @w
@out:
    lda rs_mode
    bne :+
    jsr screen_off
:   rts

; rs_stat : Y = libelle, rs_a / rs_b = valeurs, ligne rs_row (puis +2)
;   9  [barre gauche]  A  LIBELLE  B  [barre droite]
rs_stat:
    .a16
    .i16
    lda rs_row
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    sta rs_pos                  ; debut de ligne
    clc
    adc #13 * 2
    tax
    lda #UI_ATTR
    sta t0
    jsr print
    lda #UI_HI
    sta t0
    lda rs_a
    cmp #100
    bcc :+
    lda #99
:   pha
    lda rs_pos
    clc
    adc #10 * 2
    tax
    pla
    jsr print_num2
    lda rs_b
    cmp #100
    bcc :+
    lda #99
:   pha
    lda rs_pos
    clc
    adc #20 * 2
    tax
    pla
    jsr print_num2
    ; part de A sur RS_BAR cases (arrondie), B = le reste
    lda rs_a
    clc
    adc rs_b
    bne :+
    lda #RS_BAR / 2
    sta rs_la
    bra @bars
:   sta rs_s
    lda rs_a
    sta rs_aa
@red:
    lda rs_s
    cmp #128
    bcc :+
    lsr rs_s                    ; grands totaux : precision reduite (diviseur sur 8 bits)
    lsr rs_aa
    bra @red
:   lda rs_aa
    ldy #RS_BAR * 2
    jsr mulu8
    clc
    adc rs_s
    pha
    lda rs_s
    asl a
    tax
    pla
    jsr divu
    sta rs_la
@bars:
    ; barre gauche : colonnes 3..8, remplie depuis la droite
    lda rs_pos
    clc
    adc #3 * 2
    tax
    ldy #RS_BAR
@lb:
    lda #HUD_ATTR
    cpy rs_la
    beq :+
    bcs :++
:   lda #HT_PL_BLANK + TXT_ATTR + $0400
:   sta bg3_map,x
    inx
    inx
    dey
    bne @lb
    ; barre droite : colonnes 23..28, remplie depuis la gauche
    lda #RS_BAR
    sec
    sbc rs_la
    sta rs_s
    lda rs_pos
    clc
    adc #23 * 2
    tax
    ldy #1
@rb:
    lda #HT_PL_BLANK + TXT_ATTR + $0C00
    cpy rs_s
    beq :+
    bcc :+
    lda #HUD_ATTR
:   sta bg3_map,x
    inx
    inx
    iny
    cpy #RS_BAR + 1
    bne @rb
    inc rs_row
    inc rs_row
    rts

.segment "RODATA"
str_winner:     .byte "WINNER", 0
str_draw:       .byte "DRAW", 0
.segment "CODE"
