; =============================================================================
;  hud.asm - bandeau de score, horloge, messages, animation du public
; =============================================================================

HUD_ATTR   = TXT_ATTR + TXT_PANEL
HUD_TEAMA  = HUD_ATTR + $0400       ; palette BG3 1
HUD_TEAMB  = HUD_ATTR + $0C00       ; palette BG3 3
MSG_ROW    = 12

hud_init:
    .a16
    .i16
    jsr hud_draw_static
    rts

hud_draw_static:
    .a16
    .i16
    ldx #TPOS(0, 0)
    ldy #64
    lda #HUD_ATTR
    jsr fill_tiles
    lda #HUD_TEAMA
    sta t0
    ldx #TPOS(1, 1)
    ldy #.loword(team_short)
    jsr print
    lda #HUD_TEAMB
    sta t0
    ldx #TPOS(13, 1)
    ldy #.loword(team_short + 4)
    jsr print
    lda #HUD_ATTR
    sta t0
    ldx #TPOS(8, 1)
    lda #('-' - 32 + HUD_ATTR)
    sta bg3_map,x
    ldx #TPOS(27, 1)
    lda m_half
    cmp #2
    beq :+
    ldy #.loword(str_1st)
    bra :++
:   ldy #.loword(str_2nd)
:   jsr print
    lda #$FFFF
    sta hud_cache
    sta hud_cache+2
    sta hud_cache+4
    rts

hud_update:
    .a16
    .i16
    lda #HUD_ATTR
    sta t0
    ; scores
    lda score
    cmp hud_cache
    beq :+
    sta hud_cache
    ldx #TPOS(5, 1)
    jsr print_num2
:   lda score+2
    cmp hud_cache+2
    beq :+
    sta hud_cache+2
    ldx #TPOS(10, 1)
    jsr print_num2
:   ; horloge m:ss
    lda m_sec
    cmp hud_cache+4
    beq @msg
    sta hud_cache+4
    ldx #60
    jsr divu
    sta t3                      ; minutes
    lda RDMPYL
    sta t4                      ; secondes
    lda t3
    ldx #TPOS(21, 1)
    jsr print_digit
    ldx #TPOS(22, 1)
    lda #(':' - 32 + HUD_ATTR)
    sta bg3_map,x
    lda t4
    ldx #10
    jsr divu
    sta t3
    lda RDMPYL
    sta t4
    lda t3
    ldx #TPOS(23, 1)
    jsr print_digit
    lda t4
    ldx #TPOS(24, 1)
    jsr print_digit
@msg:
    ; effacement du message
    lda msg_time
    beq @d
    sec
    sbc rc+RC_TDEC
    bpl :+
    lda #0
:   sta msg_time
    bne @d
    jsr clear_msg
@d: rts

; clear_msg : vide la ligne de message
clear_msg:
    .a16
    .i16
    ldx #TPOS(0, MSG_ROW)
    ldy #32
    lda #0
    jsr fill_tiles
    rts

; -----------------------------------------------------------------------------
;  show_msg : Y = chaine -> centree sur la ligne de message, sur panneau
; -----------------------------------------------------------------------------
show_msg:
    .a16
    .i16
    phy
    jsr clear_msg
    ply
    ; longueur
    ldx #0
    phy
@len:
    lda a:0,y
    and #$00FF
    beq @lend
    inx
    iny
    bra @len
@lend:
    ply
    ; colonne = (32 - (len + 2)) / 2
    stx t1
    txa
    clc
    adc #2
    sta t2
    lda #32
    sec
    sbc t2
    lsr a
    sta t3
    asl a
    clc
    adc #TPOS(0, MSG_ROW)
    tax
    phy
    phx
    ldy t2
    lda #HUD_ATTR
    jsr fill_tiles
    plx
    ply
    inx
    inx
    lda #HUD_ATTR
    sta t0
    jsr print
    lda #T_MSG
    sta msg_time
    rts

; hud_full_time : ecran de fin de match
hud_full_time:
    .a16
    .i16
    ldy #.loword(str_fulltime)
    jsr show_msg
    stz msg_time                ; reste affiche
    ldx #TPOS(8, MSG_ROW + 1)
    ldy #16
    lda #HUD_ATTR
    jsr fill_tiles
    ldx #TPOS(8, MSG_ROW + 2)
    ldy #16
    jsr fill_tiles
    lda #HUD_TEAMA
    sta t0
    ldx #TPOS(9, MSG_ROW + 1)
    ldy #.loword(team_short)
    jsr print
    lda #HUD_TEAMB
    sta t0
    ldx #TPOS(20, MSG_ROW + 1)
    ldy #.loword(team_short + 4)
    jsr print
    lda #HUD_ATTR
    sta t0
    lda score
    ldx #TPOS(13, MSG_ROW + 1)
    jsr print_num2
    lda score+2
    ldx #TPOS(17, MSG_ROW + 1)
    jsr print_num2
    ldx #TPOS(16, MSG_ROW + 1)
    lda #('-' - 32 + HUD_ATTR)
    sta bg3_map,x
    ldx #TPOS(10, MSG_ROW + 2)
    ldy #.loword(str_press_start)
    jsr print
    rts

; -----------------------------------------------------------------------------
;  crowd_update : rotation des 3 couleurs du public (plus rapide apres un
;  evenement : but, arret, charge)
; -----------------------------------------------------------------------------
crowd_update:
    .a16
    .i16
    lda crowd_fast
    beq :+
    sec
    sbc rc+RC_TDEC
    bpl :+
    lda #0
:   sta crowd_fast
    lda crowd_t
    clc
    adc rc+RC_TDEC
    sta crowd_t
    ldy #100
    lda crowd_fast
    beq :+
    ldy #18
:   tya
    cmp crowd_t
    bcs @d
    stz crowd_t
    lda crowd_pal
    pha
    lda crowd_pal+2
    sta crowd_pal
    lda crowd_pal+4
    sta crowd_pal+2
    pla
    sta crowd_pal+4
    lda #1
    sta pal_dirty
@d: rts

.segment "RODATA"
str_1st:        .byte "1ST", 0
str_2nd:        .byte "2ND", 0
str_fulltime:   .byte "FULL TIME", 0
str_press_start: .byte "PRESS START", 0
.segment "CODE"
