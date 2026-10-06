.segment "CODE2"
; =============================================================================
;  intro.asm - presentation des equipes avant le match
;  Deux fiches (comme le choix des equipes), nom du stade, "VS" qui clignote.
;  Trois joueurs par equipe entrent en courant depuis les bords, puis respirent et
;  celebrent. START / A / B pour passer (apres 1 s), fin automatique apres 7 s.
; =============================================================================

IN_SPEED = 3                    ; px par frame a l'entree

match_intro:
    .a16
    .i16
    jsr safe_screen_off
    jsr bg3_clear
    jsr oam_clear
    jsr ui_fill
    jsr load_kits
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
    lda #$FF
    sta ts_side                 ; aucune fiche active (pas de fleches)
    lda #0
    jsr ts_draw_card
    lda #1
    jsr ts_draw_card
    ; titre : stade
    lda #UI_HI
    sta t0
    lda stadium_id
    jsr stad_name
    ldx #TPOS(2, 1)
    jsr print
    lda #UI_ATTR
    sta t0
    ldx #TPOS(10, 25)
    ldy #.loword(str_press_start)
    jsr print
    ; joueurs hors de l'ecran
    ldx #0
@px:
    lda in_start,x
    sta in_x,x
    inx
    inx
    cpx #12
    bne @px
    stz in_t
    stz in_cheer
    jsr screen_on
@loop:
    jsr oam_begin
    jsr in_sprites
    jsr oam_finish
    jsr wait_frame
    lda bg1_mode
    beq :+
    jsr crowd_update            ; etoiles qui scintillent
:
    ; "VS" qui clignote (orange / cyan)
    lda #('V' - 32 + UI_HI)
    ldx #('S' - 32 + UI_HI)
    lda frame
    and #$0010
    beq :+
    lda #('V' - 32 + UI_A)
    ldx #('S' - 32 + UI_A)
    bra :++
:   lda #('V' - 32 + UI_HI)
:   sta bg3_map + TPOS(15, 12)
    txa
    sta bg3_map + TPOS(16, 12)
    lda #1
    sta bg3_dirty
    ; temps
    lda in_t
    clc
    adc rc+RC_TDEC
    sta in_t
    cmp #TU_SEC * 7
    bcs @end
    cmp #TU_SEC
    bcc @loop
    lda joy_new
    ora joy_new+2
    and #(JOY_START | JOY_A | JOY_B)
    beq @loop
@end:
    jsr oam_clear
    rts

; in_sprites : 6 joueurs (3 par equipe) : entree en courant, puis attente / celebration
in_sprites:
    .a16
    .i16
    stz ts_k
    stz in_arr
@l: lda ts_k
    asl a
    tax
    lda ts_k
    tay
    lda ts_spx,y
    and #$00FF
    sta ts_x                    ; position finale
    lda in_x,x
    cmp ts_x
    beq @here
    bmi @right
    sec
    sbc #IN_SPEED
    cmp ts_x
    bpl @mv
    lda ts_x
    bra @mv
@right:
    clc
    adc #IN_SPEED
    cmp ts_x
    bmi @mv
    lda ts_x
@mv:
    sta in_x,x
    ; course
    lda frame
    lsr a
    lsr a
    clc
    adc ts_k
    and #$0007
    tay
    lda run_cycle,y
    and #$00FF
    bra @spr
@here:
    inc in_arr
    lda in_t
    cmp #TU_SEC * 3
    bcc @wait
    ; celebration (bras qui s'agitent, decale d'un joueur a l'autre)
    lda ts_k
    asl a
    asl a
    asl a
    adc frame
    and #$0008
    beq :+
    lda #SPR_CELEB2
    bra @spr
:   lda #SPR_CELEB
    bra @spr
@wait:
    lda ts_k
    asl a
    asl a
    asl a
    adc frame
    and #$0020
    beq :+
    lda #SPR_STAND2
    bra @spr
:   lda #SPR_STAND
@spr:
    sta ts_t
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
    ora #$4000                  ; equipe de droite : tournee vers la gauche
:   sta ts_attr
    sta t1
    lda #1
    sta t2
    lda #90
    sta t0
    lda ts_k
    asl a
    tax
    lda in_x,x
    jsr oam_add
    lda ts_attr
    clc
    adc #32
    sta t1
    lda #1
    sta t2
    lda #106
    sta t0
    lda ts_k
    asl a
    tax
    lda in_x,x
    jsr oam_add
    inc ts_k
    lda ts_k
    cmp #6
    jne @l
    ; tout le monde est arrive : clameur (une fois)
    lda in_arr
    cmp #6
    bne :+
    lda in_cheer
    bne :+
    inc in_cheer
    lda #SFX_CHEER
    jsr sfx_play
:   rts

.segment "RODATA"
in_start:   .word .loword(-24), .loword(-56), .loword(-88), 280, 312, 344
.segment "CODE2"
