; =============================================================================
;  player.asm - controle humain, deplacements, actions (passe, frappe, tir,
;               charge, esquive, saut), animation
; =============================================================================

; -----------------------------------------------------------------------------
;  init_control : choisit le joueur controle par chaque manette
; -----------------------------------------------------------------------------
init_control:
    .a16
    .i16
    ldx #0
@clr:
    stz p_human,x
    inx
    inx
    cpx #NUM_PLAYERS*2
    bne @clr
    ldy #0
@pad:
    lda #NO_OWNER
    sta ctrl,y
    lda pad_team,y
    cmp #NO_OWNER
    beq @next
    ; le porteur s'il est de l'equipe, sinon l'attaquant
    lda b_owner
    cmp #NO_OWNER
    beq @fw
    asl a
    tax
    lda p_team,x
    cmp pad_team,y
    bne @fw
    lda b_owner
    bra @set
@fw:
    lda pad_team,y
    beq :+
    lda #TEAM_SIZE
:   clc
    adc #5
@set:
    jsr set_ctrl
@next:
    iny
    iny
    cpy #4
    bne @pad
    lda #T_SWITCH_AUTO
    sta switch_t
    sta switch_t+2
    rts

; set_ctrl : Y = manette*2, A = joueur -> la manette controle ce joueur
set_ctrl:
    .a16
    .i16
    pha
    lda ctrl,y
    cmp #NO_OWNER
    beq :+
    asl a
    tax
    stz p_human,x
    stz p_want,x
    stz p_act,x
:   pla
    sta ctrl,y
    asl a
    tax
    tya
    lsr a
    inc a
    sta p_human,x
    stz p_think,x
    rts

; -----------------------------------------------------------------------------
;  human_input : manettes -> p_want / p_act, changement de joueur
; -----------------------------------------------------------------------------
human_input:
    .a16
    .i16
    ldy #0
@pad:
    lda pad_team,y
    cmp #NO_OWNER
    bne :+
    jmp @next
:   sta t6                      ; equipe
    ; suivre automatiquement le porteur de l'equipe (sauf gardien)
    lda b_owner
    cmp #NO_OWNER
    beq @def
    cmp ctrl,y
    beq @input
    asl a
    tax
    lda p_team,x
    cmp t6
    bne @def
    lda p_role,x
    beq @input                  ; gardien : on garde le joueur actuel
    lda b_owner
    jsr set_ctrl
    bra @input
@def:
    ; L : joueur le plus proche du ballon (autre que l'actuel)
    lda joy_new,y
    and #JOY_L
    beq @auto
    jsr nearest_other
    cmp #NO_OWNER
    beq @input
    jsr set_ctrl
    lda #T_SWITCH_AUTO
    sta switch_t,y
    bra @input
@auto:
    ; changement automatique en defense si un coequipier est bien plus proche
    lda switch_t,y
    sec
    sbc rc+RC_TDEC
    bpl :+
    lda #0
:   sta switch_t,y
    bne @input
    lda t6
    asl a
    tax
    lda near_team,x
    cmp #NO_OWNER
    beq @input
    cmp ctrl,y
    beq @input
    sta sw_cand                 ; (dist_to_ball ecrase t4 / t5)
    lda near_dist,x
    clc
    adc #40
    sta sw_lim
    lda ctrl,y
    asl a
    tax
    jsr dist_to_ball
    cmp sw_lim
    bcc @input
    lda sw_cand
    jsr set_ctrl
    lda #T_SWITCH_AUTO
    sta switch_t,y
@input:
    lda ctrl,y
    asl a
    tax
    lda joy_cur,y
    and #(JOY_UP | JOY_DOWN | JOY_LEFT | JOY_RIGHT)
    sta p_want,x
    lda joy_new,y
    jsr map_buttons
    sta t5
    lda joy_cur,y
    and #JOY_R
    ora t5
    sta p_act,x
@next:
    iny
    iny
    cpy #4
    beq :+
    jmp @pad
:   rts

; nearest_other : Y = manette*2, t6 = equipe -> A = joueur de champ le plus
; proche du ballon autre que ctrl,y (NO_OWNER si aucun). Preserve Y.
nearest_other:
    .a16
    .i16
    phy
    lda ctrl,y
    sta t3
    lda #$7FFF
    sta t2
    lda #NO_OWNER
    sta t1+0
    sta near_tmp
    lda t6
    beq :+
    lda #TEAM_SIZE
:   inc a                       ; on saute le gardien
    asl a
    tax
    clc
    adc #(TEAM_SIZE-1)*2
    sta t7
@l: txa
    lsr a
    cmp t3
    beq @n
    lda p_state,x
    cmp #PS_OUT
    beq @n
    jsr dist_to_ball
    cmp t2
    bcs @n
    sta t2
    txa
    lsr a
    sta near_tmp
@n: inx
    inx
    cpx t7
    bne @l
    lda near_tmp
    ply
    rts

; dist_to_ball : X = joueur*2 -> A = distance approx. au ballon (pixels). Preserve X, Y.
dist_to_ball:
    .a16
    .i16
    lda b_x
    sec
    sbc p_x,x
    ASR_A 4
    sta t0
    lda b_y
    sec
    sbc p_y,x
    ASR_A 4
    sta t1
    phy
    phx
    jsr dist_approx
    plx
    ply
    rts

; -----------------------------------------------------------------------------
;  players_update : tous les joueurs
; -----------------------------------------------------------------------------
players_update:
    .a16
    .i16
    ldx #0
@l: stx cp
    lda p_state,x
    cmp #PS_OUT
    beq :+
    jsr player_step
:   ldx cp
    inx
    inx
    cpx #NUM_PLAYERS*2
    bne @l
    rts

; -----------------------------------------------------------------------------
;  player_step : X = cp = joueur*2
; -----------------------------------------------------------------------------
player_step:
    .a16
    .i16
    ; timers
    lda p_cd,x
    sec
    sbc rc+RC_TDEC
    bpl :+
    lda #0
:   sta p_cd,x
    lda p_timer,x
    sec
    sbc rc+RC_TDEC
    bpl :+
    lda #0
:   sta p_timer,x
    ; saut
    lda p_z,x
    ora p_vz,x
    beq @nojump
    lda p_vz,x
    sec
    sbc rc+RC_GRAV
    sta p_vz,x
    ASR_A 2
    clc
    adc p_z,x
    bpl :+
    stz p_vz,x
    lda #0
:   sta p_z,x
@nojump:
    lda p_state,x
    beq @normal
    cmp #PS_CELEB
    beq @celeb
    ; charge, au sol, frappe, plongeon : glissade puis retour a la normale
    cmp #PS_TACKLE
    beq @slide
    lda #64
    bra @fr
@slide:
    lda #10
@fr:
    sta t7
    lda p_vx,x
    jsr apply_friction
    ldx cp
    sta p_vx,x
    lda p_vy,x
    jsr apply_friction
    ldx cp
    sta p_vy,x
    lda p_timer,x
    bne @move
    lda p_state,x
    cmp #PS_TACKLE
    bne :+
    lda #T_TACKLE_CD
    sta p_cd,x
:   stz p_state,x
    bra @move
@celeb:
    stz p_vx,x
    stz p_vy,x
    lda p_z,x
    bne @move
    lda frame
    and #$001F
    bne @move
    lda rc+RC_JUMP
    sta p_vz,x                  ; petits sauts de joie
    bra @move
@normal:
    jsr player_steer
    ldx cp
    jsr player_actions
    ldx cp
@move:
    ; integration et limites du terrain
    lda p_x,x
    clc
    adc p_vx,x
    cmp #(FIELD_L + 5) * FP
    bcs :+
    lda #(FIELD_L + 5) * FP
:   cmp #(FIELD_R - 5) * FP
    bcc :+
    lda #(FIELD_R - 5) * FP
:   sta p_x,x
    lda p_y,x
    clc
    adc p_vy,x
    cmp #(FIELD_T + 4) * FP
    bcs :+
    lda #(FIELD_T + 4) * FP
:   cmp #FIELD_B * FP
    bcc :+
    lda #FIELD_B * FP
:   sta p_y,x
    jsr keep_out_zone
    ldx cp
    jsr player_anim
    rts

; keep_out_zone : X = cp. Seul le gardien peut entrer dans une raquette (demi-cercle de
; rayon ZONE_R devant chaque anneau) : un joueur de champ est repousse sur le bord.
keep_out_zone:
    .a16
    .i16
    lda p_role,x
    bne :+
    rts
:   lda #ZONE_R + 4
    sta kz_r
    lda #FIELD_CY
    sta kz_cy
    lda #FIELD_L
    sta kz_cx
    jsr push_out
    lda #FIELD_R
    sta kz_cx
    jmp push_out

; push_out : X = cp -> si le joueur est a moins de kz_r pixels de (kz_cx, kz_cy),
; il est replace sur le cercle. Preserve X.
push_out:
    .a16
    .i16
    lda p_x,x
    ASR_A 4
    sec
    sbc kz_cx
    sta t0
    lda p_y,x
    ASR_A 4
    sec
    sbc kz_cy
    sta t1
    jsr dist_approx
    ldx cp
    cmp kz_r
    bcc :+
    rts
:   lda p_x,x
    ASR_A 4
    sec
    sbc kz_cx
    sta t0
    lda p_y,x
    ASR_A 4
    sec
    sbc kz_cy
    sta t1
    ora t0
    bne :+
    inc t1                      ; centre exact : direction arbitraire
:   jsr atan64
    pha
    lda kz_r
    sta t2
    pla
    jsr vel_from_dir
    ldx cp
    lda t0
    clc
    adc kz_cx
    asl a
    asl a
    asl a
    asl a
    sta p_x,x
    lda t1
    clc
    adc kz_cy
    asl a
    asl a
    asl a
    asl a
    sta p_y,x
    rts

; -----------------------------------------------------------------------------
;  player_steer : direction voulue -> vitesse (etat normal)
; -----------------------------------------------------------------------------
player_steer:
    .a16
    .i16
    lda p_want,x
    jsr dpad_dir
    ldx cp
    cmp #$FFFF
    bne @dir
    ; pas de direction : freinage
    lda rc+RC_FRIC_P
    sta t7
    lda p_vx,x
    jsr apply_friction
    ldx cp
    sta p_vx,x
    lda p_vy,x
    jsr apply_friction
    ldx cp
    sta p_vy,x
    ; recuperation
    lda p_fatigue,x
    sec
    sbc rc+RC_TDEC
    sbc rc+RC_TDEC
    bpl :+
    lda #0
:   sta p_fatigue,x
    rts
@dir:
    sta p_dir,x
    sta t3
    ; orientation du sprite
    cmp #3
    bcc @right
    cmp #6
    bcs @r2
    lda #$4000
    sta p_flip,x
    bra @spd
@r2:
    cmp #7
    bcc @spd
@right:
    stz p_flip,x
@spd:
    ; vitesse max
    lda p_role,x
    bne :+
    lda rc+RC_GKSPD
    bra @gotspd
:   lda p_speed,x
    dec a
    asl a
    tay
    lda rc+RC_SPD1,y
@gotspd:
    sta t2
    ; sprint
    lda p_act,x
    and #JOY_R
    beq @nosprint
    lda p_fatigue,x
    cmp #2400
    bcs @nosprint
    lda t2
    clc
    adc rc+RC_SPRINT
    sta t2
    ; fatigue selon STAMINA
    lda p_stam,x
    ldy #3
    cmp #4
    bcc :+
    dey
    cmp #7
    bcc :+
    dey
:   lda p_fatigue,x
:   clc
    adc rc+RC_TDEC
    dey
    bne :-
    sta p_fatigue,x
    bra @fat
@nosprint:
    lda p_fatigue,x
    sec
    sbc rc+RC_TDEC
    bpl :+
    lda #0
:   sta p_fatigue,x
@fat:
    ; malus : fatigue et port du ballon
    lda p_fatigue,x
    xba
    and #$00FF
    lsr a
    lsr a                       ; fatigue / 1024
    eor #$FFFF
    sec
    adc t2
    sta t2
    txa
    lsr a
    cmp b_owner
    bne :+
    lda t2
    sec
    sbc rc+RC_CARRYPEN
    sta t2
:
    ; vitesse cible
    ldy t3
    lda dir8_x,y
    and #$00FF
    sta t7
    lda t2
    asl a
    jsr smul
    sta t0
    ldy t3
    lda dir8_y,y
    and #$00FF
    sta t7
    lda t2
    asl a
    jsr smul
    sta t1
    ldx cp
    ; vx -> t0
    lda p_vx,x
    jsr approach_t0
    sta p_vx,x
    lda t1
    sta t0
    lda p_vy,x
    jsr approach_t0
    sta p_vy,x
    rts

; approach_t0 : A = vitesse actuelle -> A rapprochee de t0 de RC_ACCEL au plus
approach_t0:
    .a16
    cmp t0
    beq @d
    bmi @up
    sec
    sbc rc+RC_ACCEL
    cmp t0
    bpl @d
    lda t0
    rts
@up:
    clc
    adc rc+RC_ACCEL
    cmp t0
    bmi @d
    lda t0
@d: rts

; -----------------------------------------------------------------------------
;  player_actions : X = cp
; -----------------------------------------------------------------------------
player_actions:
    .a16
    .i16
    lda p_act,x
    and #(JOY_A | JOY_B | JOY_X | JOY_Y)
    bne :+
    rts
:   sta t6
    lda p_act,x
    and #JOY_R
    sta p_act,x                 ; actions consommees
    txa
    lsr a
    cmp b_owner
    bne @noball
    lda t6
    bit #JOY_Y
    beq :+
    jmp act_shoot
:   bit #JOY_X
    beq :+
    jmp act_kick
:   bit #JOY_B
    beq :+
    jmp act_hand_pass
:   jmp act_dodge
@noball:
    lda t6
    bit #JOY_A
    beq :+
    jmp act_tackle
:   and #(JOY_X | JOY_Y)
    beq @r
    ; reprise de volee : ballon libre a portee
    jsr volley_reach
    bcc @nov
    lda cp
    lsr a
    jsr take_ball
    ldx cp
    lda t6
    bit #JOY_Y
    beq :+
    jmp act_shoot
:   jmp act_kick
@nov:
    lda t6
    bit #JOY_Y
    beq @r
    jmp act_jump
@r: rts

; volley_reach : X = cp -> carry = 1 si le ballon libre est a portee de reprise
volley_reach:
    .a16
    .i16
    lda b_owner
    cmp #NO_OWNER
    bne @no
    txa
    lsr a
    cmp b_nograb
    bne :+
    lda b_nograb_t
    bne @no
:   lda b_x
    sec
    sbc p_x,x
    ABS_A
    cmp #14 * FP
    bcs @no
    lda b_y
    sec
    sbc p_y,x
    ABS_A
    cmp #10 * FP
    bcs @no
    lda p_z,x
    clc
    adc #32 * FP
    cmp b_z
    bcc @no
    sec
    rts
@no:
    clc
    rts

; --- charge
act_tackle:
    .a16
    ldx cp
    lda p_cd,x
    bne @no
    lda p_z,x
    bne @no
    lda #PS_TACKLE
    sta p_state,x
    lda #T_TACKLE
    sta p_timer,x
    lda rc+RC_TACKLE
    jsr push_dir
@no:
    rts

; --- esquive (avec ballon)
act_dodge:
    .a16
    ldx cp
    lda p_cd,x
    bne @no
    lda #T_TACKLE_CD
    sta p_cd,x
    lda rc+RC_TACKLE
    jsr push_dir
@no:
    rts

; push_dir : A = vitesse -> vitesse du joueur cp dans sa direction p_dir
push_dir:
    .a16
    asl a
    sta t2
    ldx cp
    ldy p_dir,x
    lda dir8_x,y
    and #$00FF
    sta t7
    lda t2
    jsr smul
    ldx cp
    sta p_vx,x
    ldy p_dir,x
    lda dir8_y,y
    and #$00FF
    sta t7
    lda t2
    jsr smul
    ldx cp
    sta p_vy,x
    rts

; --- saut
act_jump:
    .a16
    ldx cp
    lda p_z,x
    bne :+
    lda rc+RC_JUMP
    sta p_vz,x
:   rts

; -----------------------------------------------------------------------------
;  act_hand_pass : passe a la main vers un coequipier lateral ou en retrait
;  (la passe a la main vers l'avant est interdite). Carry = 0 si impossible.
; -----------------------------------------------------------------------------
act_hand_pass:
    .a16
    .i16
    jsr find_hand_target
    cmp #NO_OWNER
    bne :+
    clc
    rts
:   asl a
    tay
    lda rc+RC_PASS_H
    sta l_spd
    lda p_passst,x
    sta t0
    lda #12
    sta l_zt
    stz l_foot
    jsr pass_to_player
    ldx cp
    lda #SPR_THROW
    sta p_spr,x
    sec
    rts

; find_hand_target : X = cp -> A = coequipier (index) ou NO_OWNER. Preserve X.
find_hand_target:
    .a16
    .i16
    jsr attack_sign
    sta t6
    lda #$7FFF
    sta near_tmp+2
    lda #NO_OWNER
    sta near_tmp
    ; premier coequipier
    lda p_team,x
    beq :+
    lda #TEAM_SIZE
:   asl a
    tay
@l: cpy cp
    jeq @n
    lda p_state,y
    cmp #PS_OUT
    jeq @n
    cmp #PS_DOWN
    jeq @n
    lda p_x,y
    sec
    sbc p_x,x
    ASR_A 4
    sta t0
    ; en avant ?
    bit t6
    bpl :+
    eor #$FFFF
    inc a
:   cmp #11
    bmi :+
    jmp @n
:   lda p_y,y
    sec
    sbc p_y,x
    ASR_A 4
    sta t1
    phy
    jsr dist_approx
    ply
    cmp #14
    bcc @n
    cmp #170
    bcs @n
    sta t3
    phy
    jsr atan64
    ply
    ldx cp
    sta t4
    lda p_dir,x
    asl a
    asl a
    asl a
    sec
    sbc t4
    and #$003F
    cmp #33
    bcc :+
    eor #$003F
    inc a
:   asl a
    asl a                       ; ecart angulaire * 4
    clc
    adc t3
    sta t3
    lda p_role,y
    bne :+
    lda t3
    clc
    adc #80                     ; le gardien en dernier recours
    sta t3
:   lda t3
    cmp near_tmp+2
    bcs @n
    sta near_tmp+2
    tya
    lsr a
    sta near_tmp
@n: ldx cp
    iny
    iny
    tya
    lsr a
    cmp #TEAM_SIZE
    beq @end
    cmp #NUM_PLAYERS
    beq @end
    brl @l
@end:
    ldx cp
    lda near_tmp
    rts

; -----------------------------------------------------------------------------
;  pass_to_player : X = cp, Y = receveur*2, l_spd, l_zt, l_foot, t0 = stat precision
;  vise la position future du receveur
; -----------------------------------------------------------------------------
pass_to_player:
    .a16
    .i16
    ; erreur = (10 - stat) / 2 + 1 pixels
    lda #10
    sec
    sbc t0
    lsr a
    inc a
    sta l_err
    sty t3
    lda p_x,y
    sec
    sbc p_x,x
    ASR_A 4
    sta l_dx
    sta t0
    lda p_y,y
    sec
    sbc p_y,x
    ASR_A 4
    sta l_dy
    sta t1
    jsr dist_approx
    ; T = d*16/spd (borne 1..100)
    asl a
    asl a
    asl a
    asl a
    ldx l_spd
    jsr divu
    cmp #100
    bcc :+
    lda #100
:   sta t2
    ; anticipation : + v * T / 16
    ldy t3
    lda t2
    sta t7
    lda p_vx,y
    asl a
    asl a
    asl a
    asl a
    jsr smul
    clc
    adc l_dx
    sta l_dx
    ldy t3
    lda p_vy,y
    asl a
    asl a
    asl a
    asl a
    jsr smul
    clc
    adc l_dy
    sta l_dy
    ldx cp
    jmp launch_ball

; -----------------------------------------------------------------------------
;  act_kick (X) : passe longue au pied vers un coequipier dans l'axe,
;  sinon frappe puissante dans la direction du joueur
; -----------------------------------------------------------------------------
act_kick:
    .a16
    .i16
    jsr find_kick_target
    cmp #NO_OWNER
    beq @free
    asl a
    tay
    lda rc+RC_PASS_K
    sta l_spd
    lda p_kickst,x
    sta t0
    lda #4
    sta l_zt
    lda #1
    sta l_foot
    jsr pass_to_player
    bra @anim
@free:
    ; frappe directionnelle
    lda p_kickst,x
    asl a
    clc
    adc rc+RC_SHOT_K
    sta t2
    lda p_dir,x
    asl a
    asl a
    asl a
    jsr vel_from_dir
    lda rc+RC_VZ_KICK
    sta t2
    lda #1
    sta l_foot
    jsr release_ball
@anim:
    ldx cp
    lda #SPR_KICK
    sta p_spr,x
    rts

; find_kick_target : X = cp -> A = coequipier dans un cone de +-30 degres (le plus proche), ou NO_OWNER
find_kick_target:
    .a16
    .i16
    lda #$7FFF
    sta near_tmp+2
    lda #NO_OWNER
    sta near_tmp
    lda p_team,x
    beq :+
    lda #TEAM_SIZE
:   asl a
    tay
    clc
    adc #TEAM_SIZE*2
    sta t6
@l: cpy cp
    jeq @n
    lda p_state,y
    cmp #PS_OUT
    beq @n
    lda p_role,y
    beq @n
    lda p_x,y
    sec
    sbc p_x,x
    ASR_A 4
    sta t0
    lda p_y,y
    sec
    sbc p_y,x
    ASR_A 4
    sta t1
    phy
    jsr dist_approx
    ply
    cmp #36
    bcc @n
    cmp #280
    bcs @n
    sta t3
    phy
    jsr atan64
    ply
    ldx cp
    sta t4
    lda p_dir,x
    asl a
    asl a
    asl a
    sec
    sbc t4
    and #$003F
    cmp #33
    bcc :+
    eor #$003F
    inc a
:   cmp #6
    bcs @n
    lda t3
    cmp near_tmp+2
    bcs @n
    sta near_tmp+2
    tya
    lsr a
    sta near_tmp
@n: ldx cp
    iny
    iny
    cpy t6
    jne @l
    ldx cp
    lda near_tmp
    rts

; -----------------------------------------------------------------------------
;  act_shoot (Y) : tir vise vers l'anneau adverse.
;  Loin (au-dela de la ligne des 2 points) : frappe au pied. Pres : lancer.
; -----------------------------------------------------------------------------
act_shoot:
    .a16
    .i16
    lda p_team,x
    asl a
    tay
    lda st_shots,y
    inc a
    sta st_shots,y
    jsr opp_goal_x
    sta t5
    asl a
    asl a
    asl a
    asl a
    sec
    sbc p_x,x
    ASR_A 4
    sta l_dx
    sta t0
    jsr shot_side
    clc
    adc #FIELD_CY
    asl a
    asl a
    asl a
    asl a
    sec
    sbc p_y,x
    ASR_A 4
    sta l_dy
    sta t1
    jsr dist_approx
    sta t3
    ldx cp
    cmp #LONG_R
    bcc @hand
    ; frappe
    lda #1
    sta l_foot
    lda p_kickst,x
    sta t4
    ldx rc+RC_SHOT_T
    lda rc+RC_SHOT_K
    bra @calc
@hand:
    stz l_foot
    lda p_passst,x
    sta t4
    ldx rc+RC_THROW_T
    lda rc+RC_THROW
@calc:
    ; vitesse = max(vmin, d*16/T)
    sta t2
    lda t3
    asl a
    asl a
    asl a
    asl a
    jsr divu
    cmp t2
    bcs :+
    lda t2
:   cmp #220
    bcc :+
    lda #220
:   sta l_spd
    ; erreur = (10 - stat) * d / 64 + 1
    lda #10
    sec
    sbc t4
    tay
    lda t3
    lsr a
    lsr a
    lsr a
    lsr a
    jsr mulu8
    lsr a
    lsr a
    inc a
    sta l_err
    ; adversaire au contact : tir gene
    ldx cp
    jsr nearest_opp_dist
    cmp #20
    bcs :+
    lda l_err
    clc
    adc #3
    sta l_err
:   lda #RING_Z
    sta l_zt
    ldx cp
    jsr launch_ball
    ldx cp
    lda #SPR_KICK
    ldy l_foot
    bne :+
    lda #SPR_THROW
:   sta p_spr,x
    lda #SHOT_KICK
    ldy l_foot
    bne :+
    lda #SHOT_HAND
:   sta b_shot
    rts

; shot_side : X = tireur -> A = decalage vertical de la visee dans l'anneau
;  haut/bas au D-pad, sinon le cote oppose au gardien adverse
shot_side:
    .a16
    .i16
    lda p_want,x
    bit #JOY_UP
    bne @up
    bit #JOY_DOWN
    bne @down
    ldy #GK0
    lda p_team,x
    bne :+
    ldy #GK1
:   lda p_y,y
    cmp #FIELD_CY * FP
    bcs @up
@down:
    lda #12
    rts
@up:
    lda #.loword(-12)
    rts

; nearest_opp_dist : X = joueur*2 -> A = distance a l'adversaire le plus proche (pixels)
nearest_opp_dist:
    .a16
    .i16
    stx t6
    lda #$7FFF
    sta near_tmp+2
    lda p_team,x
    eor #1
    beq :+
    lda #TEAM_SIZE
:   asl a
    tay
    clc
    adc #TEAM_SIZE*2
    sta t7+0
    sta near_tmp+4
@l: lda p_state,y
    cmp #PS_OUT
    beq @n
    ldx t6
    lda p_x,y
    sec
    sbc p_x,x
    ASR_A 4
    sta t0
    lda p_y,y
    sec
    sbc p_y,x
    ASR_A 4
    sta t1
    phy
    jsr dist_approx
    ply
    cmp near_tmp+2
    bcs @n
    sta near_tmp+2
@n: iny
    iny
    cpy near_tmp+4
    bne @l
    ldx t6
    lda near_tmp+2
    rts

; -----------------------------------------------------------------------------
;  launch_ball : lance le ballon tenu par cp vers (l_dx, l_dy) pixels relatifs,
;  vitesse horizontale l_spd, hauteur visee l_zt a l'arrivee, erreur l_err.
; -----------------------------------------------------------------------------
launch_ball:
    .a16
    .i16
    ; erreur aleatoire sur la cible
    lda l_err
    asl a
    inc a
    sta t5
    jsr rand
    and #$00FF
    ldy t5
    jsr mulu8
    xba
    and #$00FF
    sec
    sbc l_err
    clc
    adc l_dx
    sta l_dx
    jsr rand
    and #$00FF
    ldy t5
    jsr mulu8
    xba
    and #$00FF
    sec
    sbc l_err
    clc
    adc l_dy
    sta l_dy
    ; distance et duree de vol
    lda l_dx
    sta t0
    lda l_dy
    sta t1
    jsr dist_approx
    cmp #8
    bcs :+
    lda #8
:   asl a
    asl a
    asl a
    asl a
    ldx l_spd
    jsr divu
    cmp #4
    bcs :+
    lda #4
:   cmp #120
    bcc :+
    lda #120
:   sta l_t
    tax
    ; vx = dx*16/T
    lda l_dx
    jsr sdiv16_t
    sta t0
    lda l_dy
    ldx l_t
    jsr sdiv16_t
    sta t1
    ; vz = g*T/2 + 64*(zt - 8)/T
    lda rc+RC_GRAV
    ldy l_t
    jsr mulu8
    lsr a
    sta t2
    lda l_zt
    sec
    sbc #8
    php
    ABS_A
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    ldx l_t
    jsr divu
    plp
    bpl :+
    eor #$FFFF
    inc a
:   clc
    adc t2
    sta t2
    jmp release_ball

; sdiv16_t : A = valeur signee (pixels), X = T -> A = valeur*16/T signee
sdiv16_t:
    .a16
    .i16
    cmp #0
    php
    ABS_A
    asl a
    asl a
    asl a
    asl a
    jsr divu
    plp
    bpl :+
    eor #$FFFF
    inc a
:   rts

; -----------------------------------------------------------------------------
;  release_ball : le joueur cp lache le ballon avec t0 = vx, t1 = vy, t2 = vz
;  l_foot = 1 si joue au pied (valeur 2 points au-dela de la ligne)
; -----------------------------------------------------------------------------
release_ball:
    .a16
    .i16
    ldx cp
    lda t0
    sta b_vx
    lda t1
    sta b_vy
    lda t2
    sta b_vz
    lda #NO_OWNER
    sta b_owner
    txa
    lsr a
    sta b_nograb
    sta b_passer
    sta b_last
    lda p_x,x
    ASR_A 4
    sta b_relx                  ; position du lanceur (regle de la moitie de terrain)
    lda #T_NOGRAB
    sta b_nograb_t
    stz b_passt
    stz b_gkdone
    stz b_carry
    lda #SHOT_NONE
    sta b_shot
    lda #8 * FP
    sta b_z
    lda #PS_KICK
    sta p_state,x
    lda #T_KICKANIM
    sta p_timer,x
    lda #SFX_PASS
    ldy l_foot
    beq :+
    lda #SFX_KICK
:   jsr sfx_play
    ; valeur du point : 2 au pied depuis l'exterieur de la ligne
    lda #1
    sta b_points
    lda l_foot
    beq @d
    jsr opp_goal_x
    asl a
    asl a
    asl a
    asl a
    sec
    sbc p_x,x
    ASR_A 4
    sta t0
    lda #FIELD_CY * FP
    sec
    sbc p_y,x
    ASR_A 4
    sta t1
    jsr dist_approx
    cmp #LONG_R
    bcc @d
    lda #2
    sta b_points
@d: ldx cp
    rts

; -----------------------------------------------------------------------------
;  player_anim : choix du sprite
; -----------------------------------------------------------------------------
player_anim:
    .a16
    .i16
    ldx cp
    lda p_state,x
    cmp #PS_DOWN
    bne :+
    lda #SPR_FALL
    brl @set
:   cmp #PS_TACKLE
    bne :+
    lda #SPR_CHARGE
    brl @set
:   cmp #PS_KICK
    jeq @keep
    cmp #PS_DIVE
    jeq @keep
    cmp #PS_CELEB
    bne :+
    lda #SPR_CELEB
    brl @set
:   lda p_z,x
    beq :+
    lda #SPR_JUMP
    bra @set
:   ; vitesse
    lda p_vx,x
    ABS_A
    sta t0
    lda p_vy,x
    ABS_A
    clc
    adc t0
    sta t0
    lsr a
    lsr a
    clc
    adc p_anim,x
    sta p_anim,x
    txa
    lsr a
    cmp b_owner
    bne @noball
    lda p_role,x
    bne :+
    lda #SPR_CATCH
    bra @set
:   lda t0
    cmp #8
    bcs :+
    lda #SPR_CARRYS
    bra @set
:   lda p_anim,x
    and #$0040
    beq :+
    lda #SPR_CARRY2
    bra @set
:   lda #SPR_CARRY1
    bra @set
@noball:
    lda t0
    cmp #8
    bcs :+
    lda #SPR_STAND
    bra @set
:   lda p_anim,x
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    and #$0003                  ; (anim >> 6) & 3
    tay
    lda run_cycle,y
    and #$00FF
@set:
    sta p_spr,x
@keep:
    rts

run_cycle:
    .byte SPR_RUN1, SPR_RUN2, SPR_RUN3, SPR_RUN2

; -----------------------------------------------------------------------------
;  map_buttons : A = boutons appuyes -> A = actions standard selon opt_ctrl
;  (JOY_B passe, JOY_X frappe, JOY_Y tir, JOY_A charge). Preserve X, Y.
; -----------------------------------------------------------------------------
map_buttons:
    .a16
    .i16
    phx
    phy
    sta t4
    stz t5
    lda opt_ctrl
    asl a
    asl a
    asl a
    tax
    ldy #0
@l: lda ctrl_map,x
    and t4
    beq :+
    lda std_act,y
    ora t5
    sta t5
:   inx
    inx
    iny
    iny
    cpy #8
    bne @l
    lda t5
    ply
    plx
    rts

; boutons de PASS, KICK, SHOOT, CHARGE pour chaque configuration
ctrl_map:
    .word JOY_B, JOY_X, JOY_Y, JOY_A
    .word JOY_A, JOY_B, JOY_Y, JOY_X
    .word JOY_Y, JOY_B, JOY_A, JOY_X
std_act:
    .word JOY_B, JOY_X, JOY_Y, JOY_A
