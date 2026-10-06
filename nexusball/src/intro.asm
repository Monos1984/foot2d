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

; =============================================================================
;  coin_toss : A = 0 debut du match (engagement + cotes), 1 prolongation (engagement)
;  Piece holographique aux couleurs des deux equipes : elle monte en tournant, ralentit
;  et retombe sur la face du gagnant. -> ct_win (equipe qui engage), ct_side (1 : cotes
;  inverses, debut du match seulement).
; =============================================================================
CT_Y = 92                       ; hauteur de la piece au repos (ecran)

coin_toss:
    .a16
    .i16
    sta ct_mode
    ; alea melange au temps passe dans les menus
    lda rng
    eor nmi_count
    ora #$0100
    sta rng
    jsr rand
    and #$0001
    sta ct_win
    stz ct_side
    lda ct_mode
    bne :+
    jsr rand
    and #$0001
    sta ct_side
:   jsr safe_screen_off
    jsr bg3_clear
    jsr oam_clear
    jsr ui_fill
    jsr load_kits
    lda #UI_HI
    sta t0
    ldx #TPOS(2, 1)
    ldy #.loword(str_toss_title)
    jsr print
    ; equipes : a gauche et a droite, a leurs couleurs
    lda #TXT_ATTR + $0400
    sta t0
    lda #0
    jsr team_rec
    ldx #TPOS(2, 5)
    jsr print
    lda #TXT_ATTR + $0C00
    sta t0
    lda #1
    jsr ct_right_name
    lda #UI_ATTR
    sta t0
    ldx #TPOS(15, 5)
    ldy #.loword(str_c_vs)
    jsr print
    ; etat de la piece
    stz ct_q
    lda ct_win
    asl a
    clc
    adc #16
    sta ct_qn                   ; 16 ou 18 quarts de tour : finit sur la face du gagnant
    lda #2
    sta ct_d
    stz ct_y
    lda #.loword(-72)
    sta ct_vy
    stz ct_land
    stz ct_done
    stz ct_t
    jsr screen_on
    lda #SFX_KICK
    jsr sfx_play
@loop:
    jsr oam_begin
    jsr ct_sprite
    jsr oam_finish
    jsr wait_frame
    lda bg1_mode
    beq :+
    jsr crowd_update
:   ; vol : gravite jusqu'au retour au sol
    lda ct_land
    bne @spin
    lda ct_vy
    clc
    adc #2
    sta ct_vy
    clc
    adc ct_y
    sta ct_y
    bmi @spin
    stz ct_y
    inc ct_land
    lda #SFX_CATCH
    jsr sfx_play
@spin:
    lda ct_q
    cmp ct_qn
    bcs @res
    dec ct_d
    bne @res
    inc ct_q
    lda ct_q
    lsr a
    lsr a
    lsr a
    clc
    adc #2
    sta ct_d                    ; la rotation ralentit
@res:
    lda ct_done
    bne @wait
    lda ct_land
    beq @loop
    lda ct_q
    cmp ct_qn
    bcc @loop
    ; resultat
    inc ct_done
    lda #SFX_CHEER
    jsr sfx_play
    jsr ct_result
    bra @loop
@wait:
    lda ct_t
    clc
    adc rc+RC_TDEC
    sta ct_t
    cmp #TU_SEC * 4
    bcs @end
    cmp #TU_SEC / 2
    jcc @loop
    lda joy_new
    ora joy_new+2
    and #(JOY_START | JOY_A | JOY_B)
    jeq @loop
@end:
    jsr oam_clear
    rts

; ct_right_name : A = equipe, ligne 5, alignee a droite (colonne 30)
ct_right_name:
    .a16
    .i16
    jsr team_rec
    sty ct_ptr
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
    adc #30
    asl a
    clc
    adc #TPOS(0, 5)
    tax
    ldy ct_ptr
    jmp print

; ct_result : texte du resultat (engagement, sens d'attaque)
ct_result:
    .a16
    .i16
    lda #UI_HI
    sta t0
    ldx #TPOS(4, 18)
    ldy #.loword(str_ct_kick)
    jsr print
    lda #TXT_ATTR + $0400
    ldy ct_win
    beq :+
    lda #TXT_ATTR + $0C00
:   sta t0
    lda ct_win
    jsr team_rec
    ldx #TPOS(15, 18)
    jsr print
    lda ct_mode
    bne @d
    ; sens d'attaque sous chaque nom
    lda #TXT_ATTR + $0400
    sta t0
    ldy #.loword(str_ct_r)
    lda ct_side
    beq :+
    ldy #.loword(str_ct_l)
:   ldx #TPOS(2, 7)
    jsr print
    lda #TXT_ATTR + $0C00
    sta t0
    ldy #.loword(str_ct_l)
    lda ct_side
    beq :+
    ldy #.loword(str_ct_r)
:   ldx #TPOS(26, 7)
    jsr print
@d: lda #UI_ATTR
    sta t0
    ldx #TPOS(10, 25)
    ldy #.loword(str_press_start)
    jsr print
    lda #1
    sta bg3_dirty
    rts

; ct_sprite : piece (face equipe 0, tranche, face equipe 1, tranche retournee) + ombre
ct_sprite:
    .a16
    .i16
    lda ct_q
    and #$0003
    tax
    lda ct_tiles,x
    and #$00FF
    ora #OBJ_PRIO
    cpx #2
    bne :+
    ora #$0200                  ; palette 1 : couleurs de l'equipe de droite
:   cpx #3
    bne :+
    ora #$4000
:   sta t1
    lda #1
    sta t2
    lda ct_y
    ASR_A 4
    clc
    adc #CT_Y
    sta t0
    lda #120
    jsr oam_add
    ; quatre etincelles en orbite tant que la piece tourne
    lda ct_done
    bne @sh
    stz ct_i
@o: lda ct_i
    asl a
    adc frame
    lsr a
    and #$0007
    sta ct_k
    lda #SPR_TRAIL2 | PAL_BALL | OBJ_PRIO
    sta t1
    stz t2
    ldy ct_k
    lda spk_dy,y
    and #$00FF
    sta t7
    lda #36
    jsr smul
    sta ct_k2
    lda ct_y
    ASR_A 4
    clc
    adc #CT_Y + 4
    clc
    adc ct_k2
    sta t0
    ldy ct_k
    lda spk_dx,y
    and #$00FF
    sta t7
    lda #36
    jsr smul
    clc
    adc #124
    jsr oam_add
    lda ct_i
    clc
    adc #2
    sta ct_i
    cmp #8
    bne @o
@sh:
    ; ombre au sol
    lda #SPR_OSHADOW | PAL_BALL | $2000
    sta t1
    lda #1
    sta t2
    lda #CT_Y + 6
    sta t0
    lda #120
    jmp oam_add

.segment "RODATA"
ct_tiles:     .byte SPR_COIN, SPR_COINE, SPR_COIN, SPR_COINE
str_toss_title: .byte "COIN TOSS", 0
str_ct_kick:  .byte "KICK OFF", 0
str_ct_r:     .byte ">>>", 0
str_ct_l:     .byte "<<<", 0
.segment "CODE2"
