; =============================================================================
;  menu.asm - ecran titre, options, credits
; =============================================================================

MENU_ITEMS = 6
MENU_ROW   = 12
MENU_COL   = 9

title_screen:
    .a16
    .i16
    jsr safe_screen_off
    jsr bg3_clear
    jsr oam_clear
    lda #64
    sta scroll_x
    lda #40
    sta scroll_y
    stz title_scroll
    ; titre en gros caracteres
    ldx #TPOS(6, 4)
    ldy #.loword(big_title)
    jsr big_print
    ldx #TPOS(10, 7)
    lda #TXT_ATTR
    sta t0
    ldy #.loword(str_sub)
    jsr print
    ; panneau du menu
    lda #MENU_ROW - 1
@pan:
    pha
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc #(MENU_COL - 2) * 2
    tax
    ldy #18
    lda #TXT_ATTR + TXT_PANEL
    jsr fill_tiles
    pla
    inc a
    cmp #MENU_ROW + MENU_ITEMS + 1
    bne @pan
    ; pied de page
    lda #TXT_ATTR + TXT_PANEL
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
    jsr menu_draw
    jsr screen_on
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
    ; fond qui defile doucement
    inc title_scroll
    lda title_scroll
    lsr a
    lsr a
    and #$01FF
    cmp #$0100
    bcc :+
    eor #$01FF
:   sta scroll_x
    jsr oam_begin
    jsr oam_finish
    jsr crowd_update
    lda joy_new
    ora joy_new+2
    sta t7
    beq @loop
    bit #JOY_DOWN
    beq :+
    lda menu_sel
    inc a
    cmp #MENU_ITEMS
    bcc @set
    lda #0
    bra @set
:   bit #JOY_UP
    beq :+
    lda menu_sel
    dec a
    bpl @set
    lda #MENU_ITEMS - 1
    bra @set
:   lda menu_sel
    cmp #3
    beq @radar
    cmp #4
    beq @len
    lda t7
    and #(JOY_START | JOY_A | JOY_B)
    beq @loop
    lda menu_sel
    cmp #5
    beq @credits
    sta game_mode
    jmp match_setup
@credits:
    jmp credits_screen
@radar:
    lda t7
    and #(JOY_LEFT | JOY_RIGHT | JOY_A | JOY_B | JOY_START)
    beq @loop
    lda opt_radar
    eor #1
    sta opt_radar
    jsr save_options
    bra @redraw
@len:
    lda t7
    bit #JOY_LEFT
    beq :+
    lda menu_len
    dec a
    and #$0003
    bra @slen
:   and #(JOY_RIGHT | JOY_A | JOY_B | JOY_START)
    jeq @loop
    lda menu_len
    inc a
    and #$0003
@slen:
    sta menu_len
    jsr save_options
    bra @redraw
@set:
    sta menu_sel
@redraw:
    jsr menu_draw
    jmp @loop

; menu_draw : textes et curseur du menu
menu_draw:
    .a16
    .i16
    lda #TXT_ATTR + TXT_PANEL
    sta t0
    ldx #TPOS(MENU_COL, MENU_ROW)
    ldy #.loword(str_m1p)
    jsr print
    ldx #TPOS(MENU_COL, MENU_ROW + 1)
    ldy #.loword(str_m2p)
    jsr print
    ldx #TPOS(MENU_COL, MENU_ROW + 2)
    ldy #.loword(str_mcpu)
    jsr print
    ldx #TPOS(MENU_COL, MENU_ROW + 3)
    ldy #.loword(str_mradar_on)
    lda opt_radar
    bne :+
    ldy #.loword(str_mradar_off)
:   jsr print
    ldx #TPOS(MENU_COL, MENU_ROW + 4)
    ldy #.loword(str_mlen)
    jsr print
    lda menu_len
    clc
    adc #2
    ldx #TPOS(MENU_COL + 10, MENU_ROW + 4)
    jsr print_digit
    ldx #TPOS(MENU_COL, MENU_ROW + 5)
    ldy #.loword(str_mcredits)
    jsr print
    ; curseur
    ldx #0
@c: txa
    clc
    adc #MENU_ROW
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc #(MENU_COL - 2) * 2
    tay
    lda #TXT_ATTR + TXT_PANEL
    cpx menu_sel
    bne :+
    lda #('>' - 32 + TXT_ATTR + TXT_PANEL + $0C00)
:   sta bg3_map,y
    inx
    cpx #MENU_ITEMS
    bne @c
    lda #1
    sta bg3_dirty
    rts

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
big_title:
    .byte BIG_N, BIG_E, BIG_X, BIG_U, BIG_S, 1, BIG_B, BIG_A, BIG_L, BIG_L, 0
str_sub:        .byte "FUTURE SPORT LEAGUE", 0
str_m1p:        .byte "1P VS CPU  ", 0
str_m2p:        .byte "1P VS 2P   ", 0
str_mcpu:       .byte "CPU VS CPU ", 0
str_mradar_on:  .byte "RADAR    ON ", 0
str_mradar_off: .byte "RADAR    OFF", 0
str_mlen:       .byte "LENGTH  2X  MIN", 0
str_mcredits:   .byte "CREDITS", 0
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
