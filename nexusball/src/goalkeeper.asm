; =============================================================================
;  goalkeeper.asm - gardiens : placement, arrets (capter / repousser), relance
; =============================================================================

GK0 = 0
GK1 = TEAM_SIZE * 2

; -----------------------------------------------------------------------------
;  gk_saves : appele quand le ballon est libre. Chaque gardien tente au plus
;  un arret par tir (b_gkdone).
; -----------------------------------------------------------------------------
gk_saves:
    .a16
    .i16
    ldx #GK0
    jsr gk_try
    lda b_owner
    cmp #NO_OWNER
    bne :+
    ldx #GK1
    jsr gk_try
:   rts

gk_try:
    .a16
    .i16
    stx cp
    lda p_state,x
    cmp #PS_OUT
    beq @no
    cmp #PS_DOWN
    beq @no
    ; bit du gardien dans b_gkdone
    lda p_team,x
    inc a
    sta t6
    and b_gkdone
    bne @no
    ; le ballon doit aller vers le but defendu
    jsr own_goal_x
    cmp #FIELD_L
    bne @r
    lda b_vx
    bpl @no
    bra @dir
@r: lda b_vx
    bmi @dir
@no:
    rts
@dir:
    ; a portee ?
    lda b_x
    sec
    sbc p_x,x
    ASR_A 4
    ABS_A
    cmp #12
    bcs @no
    lda b_z
    cmp #40 * FP
    bcs @no
    lda b_y
    sec
    sbc p_y,x
    ASR_A 4
    sta t5
    ABS_A
    sta t4
    lda p_speed,x               ; REFLEX
    clc
    adc #12
    cmp t4
    bcc @no
    ; une seule tentative pour ce tir
    lda b_gkdone
    ora t6
    sta b_gkdone
    ; reussite sur 16 : REFLEX + 8 - vitesse/2 - ecart/4
    lda b_vx
    ABS_A
    ASR_A 5
    sta t3
    lda t4
    lsr a
    lsr a
    clc
    adc t3
    sta t3
    lda p_speed,x
    clc
    adc #10
    sec
    sbc t3
    sta t3
    jsr rand
    and #$000F
    cmp t3
    jcs @no
    ldx cp
    ; plongeon si le ballon est loin du corps
    lda t4
    cmp #7
    bcc @catch
    lda #PS_DIVE
    sta p_state,x
    lda #90
    sta p_timer,x
    lda #SPR_DIVE
    sta p_spr,x
    lda t5
    and #$8000
    lsr a                       ; vers le haut : retourne
    sta p_flip,x
    lda t5
    asl a
    asl a
    asl a
    asl a
    clc
    adc p_y,x
    sta p_y,x
@catch:
    ; capter si le ballon n'est pas trop rapide (CATCH), sinon repousser
    lda b_vx
    ABS_A
    sta t3
    lda b_vy
    ABS_A
    clc
    adc t3
    sta t3
    lda p_ctrlst,x              ; CATCH
    asl a
    asl a
    asl a
    adc #44
    cmp t3
    bcc @punch
    txa
    lsr a
    jsr take_ball
    ldx cp
    lda p_state,x
    cmp #PS_DIVE
    beq :+
    lda #SPR_CATCH
    sta p_spr,x
:   rts
@punch:
    lda b_vx
    ASR_A 1
    eor #$FFFF
    inc a
    sta b_vx
    jsr rand
    and #$003F
    sec
    sbc #32
    clc
    adc b_vy
    ASR_A 1
    sta b_vy
    lda #180
    sta b_vz
    lda cp
    lsr a
    sta b_nograb
    sta b_last
    lda #T_NOGRAB
    sta b_nograb_t
    stz b_shot
    lda #1
    sta b_points
    lda #T_GOAL / 4
    sta crowd_fast
    rts

; -----------------------------------------------------------------------------
;  gk_ai : X = cp = gardien. Placement devant l'anneau, sortie sur ballon
;  proche, relance apres quelques instants.
; -----------------------------------------------------------------------------
gk_ai:
    .a16
    .i16
    lda p_state,x
    beq :+
    rts
:   txa
    lsr a
    cmp b_owner
    bne @noball
    ; relance
    stz p_want,x
    lda b_carry
    cmp #T_GK_HOLD
    bcs :+
    rts
:   jsr gk_pick_target
    asl a
    tay
    ldx cp
    lda rc+RC_GKTHROW
    sta l_spd
    lda p_kickst,x              ; THROW
    sta t0
    lda #10
    sta l_zt
    stz l_foot
    jsr pass_to_player
    ldx cp
    lda #SPR_THROW
    sta p_spr,x
    rts
@noball:
    jsr own_goal_x
    sta t2                      ; x de l'anneau
    ; ballon libre dans la zone : sortir le chercher
    lda b_owner
    cmp #NO_OWNER
    bne @place
    lda b_z
    cmp #24 * FP
    bcs @place
    lda b_x
    ASR_A 4
    sec
    sbc t2
    sta t0
    lda b_y
    ASR_A 4
    sec
    sbc #FIELD_CY
    sta t1
    jsr dist_approx
    ldx cp
    cmp #ZONE_R
    bcs @place
    lda b_x
    ASR_A 4
    sta t0
    lda b_y
    ASR_A 4
    sta t1
    lda #JOY_R
    sta p_act,x
    jmp ai_steer
@place:
    ; y : suit le ballon (1/3), borne devant l'anneau
    lda b_y
    ASR_A 4
    sec
    sbc #FIELD_CY
    sta t0
    ASR_A 2
    clc
    adc t0
    ASR_A 1                     ; ~ 5/8... puis bornage
    cmp #.loword(-(RING_R - 2))
    bpl :+
    lda #.loword(-(RING_R - 2))
:   cmp #RING_R - 2
    bmi :+
    lda #RING_R - 2
:   clc
    adc #FIELD_CY
    sta t1
    ; x : un peu devant l'anneau, plus loin si le ballon approche
    lda b_x
    ASR_A 4
    sec
    sbc t2
    ABS_A
    ldy #10
    cmp #140
    bcs :+
    ldy #16
:   tya
    sta t0
    lda t2
    cmp #FIELD_L
    beq :+
    lda t2
    sec
    sbc t0
    bra :++
:   lda t2
    clc
    adc t0
:   sta t0
    stz p_act,x
    jmp ai_steer

; gk_pick_target : X = gardien -> A = coequipier le plus demarque (le plus loin d'un adversaire)
gk_pick_target:
    .a16
    .i16
    lda p_team,x
    beq :+
    lda #TEAM_SIZE
:   inc a
    sta t3                      ; premier joueur de champ
    clc
    adc #TEAM_SIZE - 1
    sta t2                      ; fin
    stz near_tmp                ; meilleur ecart
    lda t3
    sta gk_best
@l: lda t3
    asl a
    tax
    lda p_state,x
    cmp #PS_OUT
    beq @n
    lda t3
    pha
    lda t2
    pha
    jsr nearest_opp_dist
    sta t4
    pla
    sta t2
    pla
    sta t3
    lda t4
    cmp near_tmp
    bcc @n
    sta near_tmp
    lda t3
    sta gk_best
@n: inc t3
    lda t3
    cmp t2
    bne @l
    ldx cp
    lda gk_best
    rts
