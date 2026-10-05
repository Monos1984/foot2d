; =============================================================================
;  menu.asm - ecran titre, options, credits
; =============================================================================

MENU_ITEMS = 9
MENU_ROW   = 12
MENU_COL   = 8

title_screen:
    .a16
    .i16
    stz kit_pick                ; tenues automatiques (choix refait a chaque match amical)
    stz kit_pick+2
    jsr safe_screen_off
    jsr bg3_clear
    jsr oam_clear
    jsr load_menubg
    ; logo (BG2) et panneau translucide du menu
    jsr layers_menu
    jsr title_hdma
    jsr load_logo_chr
    jsr bg2_logo
    lda title_mode
    beq @nopanel
    lda #MENU_COL - 3
    sta pn_x
    sta bar_x0
    lda #MENU_ROW - 1
    sta pn_y
    lda #24
    sta pn_w
    lda #MENU_ITEMS + 2
    sta pn_h
    jsr bg2_panel
    lda #22
    sta bar_w
    inc bar_x0
@nopanel:
    ldx #TPOS(3, 10)
    lda #TXT_ATTR
    sta t0
    ldy #.loword(str_sub)
    jsr print
    ; pied de page
    lda #TXT_ATTR
    sta t0
    ldx #TPOS(7, 22)
    ldy #.loword(str_project)
    jsr print
    ldx #TPOS(12, 25)
    ldy #.loword(str_ntsc)
    lda is_pal
    beq :+
    ldy #.loword(str_pal)
:   jsr print
    lda title_mode
    beq :+
    jsr menu_draw
:   jsr screen_on
    lda title_music
    bne :+
    lda #1
    sta title_music
    lda #MUS_TITLE
    jsr sfx_play
    lda #0
    jsr crowd_level
:

@loop:
    jsr wait_frame
    jsr oam_begin
    jsr oam_finish
    jsr crowd_update
    lda title_mode
    bne @menu
    ; ecran titre : PRESS START clignotant
    lda frame
    and #$0020
    beq :+
    lda #TXT_ATTR + $1000
    sta t0
    ldx #TPOS(10, 16)
    ldy #.loword(str_press_start)
    jsr print
    bra :++
:   ldx #TPOS(8, 16)
    ldy #16
    lda #0
    jsr fill_tiles
:   lda joy_new
    ora joy_new+2
    and #(JOY_START | JOY_A)
    beq @loop
    lda #SFX_OK
    jsr sfx_play
    lda #1
    sta title_mode
    jmp title_screen
@menu:
    lda joy_new
    ora joy_new+2
    sta t7
    beq @loop
    lda menu_sel
    ldy #MENU_ITEMS
    jsr ui_updown
    cmp menu_sel
    beq :+
    sta menu_sel
    jsr menu_draw
    bra @loop
:   lda t7
    and #(JOY_START | JOY_A)
    beq @loop
    lda #SFX_OK
    jsr sfx_play
    lda menu_sel
    asl a
    tax
    jmp (.loword(title_jump),x)

title_jump:
    .word .loword(go_exhibition), .loword(go_champ), .loword(go_cup)
    .word .loword(go_custom), .loword(create_team), .loword(create_player)
    .word .loword(rules_screen), .loword(options_screen), .loword(credits_screen)

go_exhibition:
    stz comp_active
    jmp match_setup
go_champ:
    lda #0
    jmp comp_menu
go_cup:
    lda #1
    jmp comp_menu
go_custom:
    lda #2
    jmp comp_menu

; menu_draw : textes et curseur du menu
menu_draw:
    .a16
    .i16
    lda #TXT_ATTR
    sta t0
    stz t5
@l: lda t5
    asl a
    tay
    lda title_items,y
    tay
    lda t5
    clc
    adc #MENU_ROW
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc #MENU_COL * 2
    tax
    jsr print
    inc t5
    lda t5
    cmp #MENU_ITEMS
    bne @l
    lda #.loword(title_rows)
    sta t3
    lda #MENU_ITEMS
    sta t4
    lda menu_sel
    jsr ui_cursor_col
    rts

title_rows:
    .byte MENU_ROW, MENU_ROW + 1, MENU_ROW + 2, MENU_ROW + 3, MENU_ROW + 4, MENU_ROW + 5
    .byte MENU_ROW + 6, MENU_ROW + 7, MENU_ROW + 8

; ui_cursor_col : comme ui_cursor mais en colonne MENU_COL - 2
ui_cursor_col:
    .a16
    .i16
    sta t5
    ldy t3
@l: lda a:0,y
    and #$00FF
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc #(MENU_COL - 2) * 2
    tax
    phy
    tya
    sec
    sbc t3
    cmp t5
    bne :+
    lda #('>' - 32 + TXT_ATTR + $1000)
    bra :++
:   lda #TXT_ATTR
:   sta bg3_map,x
    ply
    iny
    dec t4
    bne @l
    lda #1
    sta bg3_dirty
    ; barre de selection
    lda menu_sel
    clc
    adc #MENU_ROW
    jmp bg2_bar

; -----------------------------------------------------------------------------
;  options_screen : MATCH LENGTH, CONTROLS, LANGUAGE (sauvegardes en SRAM)
; -----------------------------------------------------------------------------
options_screen:
    .a16
    .i16
    stz ui_sel
options_redraw:
    jsr safe_screen_off
    jsr bg3_clear
    jsr ui_fill
    lda #UI_HI
    sta t0
    ldx #TPOS(12, 3)
    ldy #.loword(str_options)
    jsr print
    jsr opt_draw
    jsr screen_on
@loop:
    jsr ui_wait
    lda t7
    beq @loop
    and #JOY_B
    bne @back
    lda ui_sel
    ldy #4
    jsr ui_updown
    cmp ui_sel
    beq :+
    sta ui_sel
    jsr opt_draw
    bra @loop
:   lda ui_sel
    bne :+
    lda menu_len
    ldy #4
    jsr ui_lr
    sta menu_len
    bra @chg
:   cmp #1
    bne :+
    lda opt_ctrl
    ldy #3
    jsr ui_lr
    sta opt_ctrl
    bra @chg
:   cmp #2
    bne @done
    lda lang
    ldy #2
    jsr ui_lr
    cmp lang
    beq @loop
    sta lang
    jsr save_options
    jmp options_redraw           ; tous les textes changent de longueur
@chg:
    jsr save_options
    jsr opt_draw
    bra @loop
@done:
    lda t7
    and #(JOY_A | JOY_START)
    jeq @loop
@back:
    jmp title_screen

opt_rows:
    .byte 6, 7, 10, 12

opt_draw:
    .a16
    .i16
    lda #UI_ATTR
    sta t0
    ldx #TPOS(2, 6)
    ldy #.loword(str_o_len)
    jsr print
    lda #UI_HI
    sta t0
    lda menu_len
    clc
    adc #2
    ldx #TPOS(20, 6)
    jsr print_digit
    lda #UI_ATTR
    sta t0
    ldx #TPOS(2, 7)
    ldy #.loword(str_o_ctrl)
    jsr print
    lda #UI_HI
    sta t0
    lda opt_ctrl
    clc
    adc #'A' - 32 + UI_HI
    sta bg3_map + TPOS(23, 7)
    ; rappel des boutons
    lda opt_ctrl
    asl a
    tay
    lda ctrl_help,y
    tay
    lda #UI_ATTR
    sta t0
    ldx #TPOS(2, 8)
    jsr print
    ldx #TPOS(2, 10)
    ldy #.loword(str_lang)
    jsr print
    ldy #.loword(str_english)
    lda lang
    beq :+
    ldy #.loword(str_french)
:   lda #UI_HI
    sta t0
    ldx #TPOS(18, 10)
    jsr print
    lda #UI_ATTR
    sta t0
    ldx #TPOS(2, 12)
    ldy #.loword(str_back)
    jsr print
    lda #.loword(opt_rows)
    sta t3
    lda #4
    sta t4
    lda ui_sel
    jmp ui_cursor

; safe_screen_off : eteint l'ecran (meme si le NMI n'est pas encore actif)
safe_screen_off:
    .a16
    sep #$20
    .a8
    lda nmi_on
    rep #$20
    .a16
    beq @direct
    jmp screen_off
@direct:
    sep #$20
    .a8
    lda #$8F
    sta INIDISP
    sta inidisp
    rep #$20
    .a16
    rts

; -----------------------------------------------------------------------------
;  credits_screen
; -----------------------------------------------------------------------------
credits_screen:
    .a16
    .i16
    jsr screen_off
    jsr bg3_clear
    lda #TXT_ATTR
    sta t0
    ldy #.loword(credits_text)
    ldx #TPOS(2, 3)
@line:
    phx
    jsr print
    plx
    iny                         ; saute le 0
    txa
    clc
    adc #64
    tax
    lda a:0,y
    and #$00FF
    cmp #$FF
    bne @line
    jsr screen_on
@w: jsr wait_frame
    jsr oam_begin
    jsr oam_finish
    lda joy_new
    ora joy_new+2
    and #(JOY_START | JOY_A | JOY_B)
    beq @w
    jmp title_screen

.segment "RODATA"
str_sub:        .byte "INTERPLANETARY SPORT LEAGUE", 0
str_t_exh:      .byte "EXHIBITION", 0
str_t_champ:    .byte "CHAMPIONSHIP", 0
str_t_cup:      .byte "CUP", 0
str_t_custom:   .byte "CUSTOM COMPETITION", 0
str_t_opt:      .byte "OPTIONS", 0
str_t_cteam:    .byte "CREATE TEAM", 0
str_t_cplayer:  .byte "CREATE PLAYER", 0
str_t_cred:     .byte "CREDITS", 0
title_items:
    .word .loword(str_t_exh), .loword(str_t_champ), .loword(str_t_cup)
    .word .loword(str_t_custom), .loword(str_t_cteam), .loword(str_t_cplayer)
    .word .loword(str_t_rules), .loword(str_t_opt), .loword(str_t_cred)
str_options:    .byte "OPTIONS", 0
str_o_len:      .byte "MATCH LENGTH    2X  MIN", 0
str_on:         .byte "ON ", 0
str_off:        .byte "OFF", 0
str_back:       .byte "BACK", 0
str_o_ctrl:     .byte "CONTROLS        TYPE", 0
str_ch0:        .byte "B:PASS X:KICK Y:SHOOT A:CHARGE", 0
str_ch1:        .byte "A:PASS B:KICK Y:SHOOT X:CHARGE", 0
str_ch2:        .byte "Y:PASS B:KICK A:SHOOT X:CHARGE", 0
ctrl_help:      .word .loword(str_ch0), .loword(str_ch1), .loword(str_ch2)
str_project:    .byte " AN OFFGAME PROJECT ", 0
str_ntsc:       .byte " NTSC 60HZ ", 0
str_pal:        .byte " PAL 50HZ  ", 0
credits_text:
    .byte "NEXUS BALL", 0
    .byte 0
    .byte "AN OFFGAME PROJECT", 0
    .byte 0
    .byte "GAME DESIGN", 0
    .byte "  JEAN MONOS", 0
    .byte 0
    .byte "PROJECT DIRECTION", 0
    .byte "  JEAN MONOS / OFFGAME", 0
    .byte 0
    .byte "DESIGN & DEVELOPMENT ASSISTANCE", 0
    .byte "  CHATGPT - OPENAI", 0
    .byte "  ELEA - OPENAI", 0
    .byte "  CLAUDE - ANTHROPIC", 0
    .byte 0
    .byte "SUPER NINTENDO VERSION", 0
    .byte "  OFFGAME", 0
    .byte 0
    .byte "SPECIAL THANKS", 0
    .byte "  TO BE COMPLETED", 0
    .byte $FF
.segment "CODE"
