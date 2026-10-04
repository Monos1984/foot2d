; =============================================================================
;  sprites.asm - construction de l'OAM : radar, curseurs, joueurs et ballon
;  tries par profondeur (y), ombres.
;  Ordre OAM = ordre d'affichage (le premier est devant).
; =============================================================================

OBJ_PRIO   = $3000              ; priorite 3
PAL_BALL   = $0800              ; palette OBJ 4
RADAR_X    = 186
RADAR_Y    = 182
ENT_BALL   = NUM_PLAYERS

build_sprites:
    .a16
    .i16
    jsr oam_begin
    lda m_state
    cmp #MS_END
    beq :+
    lda opt_radar
    beq :+
    jsr draw_radar
:   jsr draw_cursors
    jsr sort_entities
    ; entites, de la plus proche (y grand) a la plus lointaine
    ldy #0
@l: phy
    lda sort_buf,y
    and #$00FF
    cmp #ENT_BALL
    bne :+
    jsr draw_ball
    bra @n
:   asl a
    tax
    jsr draw_player
@n: ply
    iny
    cpy #NUM_PLAYERS + 1
    bne @l
    jsr draw_ball_shadow
    jsr oam_finish
    rts

; -----------------------------------------------------------------------------
;  sort_entities : tri par insertion des 13 entites selon y decroissant
; -----------------------------------------------------------------------------
sort_entities:
    .a16
    .i16
    ; cles : sort_key[i]
    ldx #0
@k: lda p_y,x
    sta sort_key,x
    inx
    inx
    cpx #NUM_PLAYERS*2
    bne @k
    lda b_y
    ldy b_owner
    cpy #NO_OWNER
    beq :+
    clc
    adc #FP / 2                 ; le ballon tenu passe devant le porteur
:   sta sort_key,x
    sep #$20
    .a8
    ldx #0
@i: txa
    sta sort_buf,x
    inx
    cpx #NUM_PLAYERS + 1
    bne @i
    rep #$20
    .a16
    ; insertion
    ldx #1
@outer:
    lda sort_buf,x
    and #$00FF
    sta t0                      ; element
    asl a
    tay
    lda sort_key,y
    sta t1                      ; cle
    txy
@inner:
    cpy #0
    beq @place
    lda sort_buf-1,y
    and #$00FF
    asl a
    phx
    tax
    lda sort_key,x
    plx
    cmp t1
    bcs @place                  ; deja plus grand ou egal : stop
    sep #$20
    .a8
    lda sort_buf-1,y
    sta sort_buf,y
    rep #$20
    .a16
    dey
    bra @inner
@place:
    sep #$20
    .a8
    lda t0
    sta sort_buf,y
    rep #$20
    .a16
    inx
    cpx #NUM_PLAYERS + 1
    bne @outer
    rts

; -----------------------------------------------------------------------------
;  draw_player : X = joueur*2
; -----------------------------------------------------------------------------
draw_player:
    .a16
    .i16
    lda p_state,x
    cmp #PS_OUT
    bne :+
    rts
:   ; palette : equipe (0/1) ou gardien (2/3)
    lda p_team,x
    ldy p_role,x
    bne :+
    ora #2
:   asl a
    xba                         ; palette << 9
    ora #OBJ_PRIO
    ora p_flip,x
    ora p_spr,x
    sta t1
    lda #1
    sta t2
    lda p_y,x
    sec
    sbc p_z,x
    ASR_A 4
    sec
    sbc scroll_y
    sec
    sbc #16
    sta t0
    lda p_x,x
    ASR_A 4
    sec
    sbc scroll_x
    sec
    sbc #8
    jsr oam_add
    rts

; -----------------------------------------------------------------------------
;  draw_ball / draw_ball_shadow
; -----------------------------------------------------------------------------
draw_ball:
    .a16
    .i16
    lda #SPR_BALL | PAL_BALL | OBJ_PRIO
    ldx b_z
    cpx #12 * FP
    bcc :+
    lda #SPR_BALLHI | PAL_BALL | OBJ_PRIO
:   sta t1
    stz t2
    lda b_y
    sec
    sbc b_z
    ASR_A 4
    sec
    sbc scroll_y
    sec
    sbc #8
    sta t0
    lda b_x
    ASR_A 4
    sec
    sbc scroll_x
    sec
    sbc #4
    jsr oam_add
    rts

draw_ball_shadow:
    .a16
    lda #SPR_SHADOW | PAL_BALL | $2000
    sta t1
    stz t2
    lda b_y
    ASR_A 4
    sec
    sbc scroll_y
    sec
    sbc #6
    sta t0
    lda b_x
    ASR_A 4
    sec
    sbc scroll_x
    sec
    sbc #4
    jsr oam_add
    rts

; -----------------------------------------------------------------------------
;  draw_cursors : fleche au-dessus du joueur controle (clignote apres 3 s de port)
; -----------------------------------------------------------------------------
draw_cursors:
    .a16
    .i16
    ldy #0
@pad:
    lda ctrl,y
    cmp #NO_OWNER
    beq @n
    lda m_state
    cmp #MS_END
    beq @n
    lda ctrl,y
    asl a
    tax
    lda p_state,x
    cmp #PS_OUT
    beq @n
    lda ctrl,y
    cmp b_owner
    bne @show
    lda b_carry
    cmp #T_CARRY_WARN
    bcc @show
    lda frame
    and #$0004
    beq @n
@show:
    phy
    lda ctrl,y
    asl a
    tax
    lda #SPR_CUR1 | PAL_BALL | OBJ_PRIO
    cpy #0
    beq :+
    lda #SPR_CUR2 | PAL_BALL | OBJ_PRIO
:   sta t1
    stz t2
    lda p_y,x
    sec
    sbc p_z,x
    ASR_A 4
    sec
    sbc scroll_y
    sec
    sbc #24
    sta t0
    lda p_x,x
    ASR_A 4
    sec
    sbc scroll_x
    sec
    sbc #4
    jsr oam_add
    ply
@n: iny
    iny
    cpy #4
    jne @pad
    rts

; -----------------------------------------------------------------------------
;  draw_radar : mini-radar en bas a droite (points puis fond)
; -----------------------------------------------------------------------------
draw_radar:
    .a16
    .i16
    stz t2
    ; ballon
    lda #SPR_DOTW | PAL_BALL | OBJ_PRIO
    sta t1
    lda b_y
    jsr radar_y
    sta t0
    lda b_x
    jsr radar_x
    jsr oam_add
    ; joueurs
    ldx #0
@l: lda p_state,x
    cmp #PS_OUT
    beq @n
    lda #SPR_DOTA | PAL_BALL | OBJ_PRIO
    ldy p_team,x
    beq :+
    lda #SPR_DOTB | PAL_BALL | OBJ_PRIO
:   sta t1
    lda p_y,x
    jsr radar_y
    sta t0
    lda p_x,x
    jsr radar_x
    phx
    jsr oam_add
    plx
@n: inx
    inx
    cpx #NUM_PLAYERS*2
    bne @l
    ; fond 64x32 : 4 x 2 sprites 16x16
    lda #1
    sta t2
    lda #RADAR_Y
    sta t0
    lda #SPR_PANEL_TL | PAL_BALL | OBJ_PRIO
    jsr radar_row
    lda #RADAR_Y + 16
    sta t0
    lda #SPR_PANEL_BL | PAL_BALL | OBJ_PRIO
    jsr radar_row
    rts

; radar_row : A = tile du coin gauche (le bord est tile + 2), t0 = y
radar_row:
    .a16
    sta t6
    sta t1
    lda #RADAR_X
    jsr oam_add
    lda t6
    clc
    adc #2
    sta t1
    lda #RADAR_X + 16
    jsr oam_add
    lda #RADAR_X + 32
    jsr oam_add
    lda t6
    ora #$4000                  ; coin droit = coin gauche retourne
    sta t1
    lda #RADAR_X + 48
    jsr oam_add
    rts

; radar_x / radar_y : A = coordonnee monde (12.4) -> position ecran du point
radar_x:
    .a16
    ASR_A 7                     ; / 8 pixels
    sec
    sbc #FIELD_L / 8
    clc
    adc #RADAR_X + 3
    rts
radar_y:
    .a16
    ASR_A 7
    sec
    sbc #FIELD_T / 8
    clc
    adc #RADAR_Y + 2
    rts
