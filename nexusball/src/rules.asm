; =============================================================================
;  rules.asm - regles : port 4 s, charges, fautes, avantage, exclusion,
;              collisions entre joueurs
; =============================================================================

rules_update:
    .a16
    .i16
    ; ---- port du ballon
    lda b_owner
    cmp #NO_OWNER
    beq @tackles
    asl a
    tax
    lda b_carry
    clc
    adc rc+RC_TDEC
    sta b_carry
    lda p_role,x
    beq @tackles                ; le gardien relance seul (gk_ai)
    lda b_carry
    cmp #T_CARRY_MAX
    bcc @tackles
    jsr carry_turnover
    rts
@tackles:
    ; ---- charges
    ldx #0
@l: lda p_state,x
    cmp #PS_TACKLE
    bne @n
    stx cp
    jsr tackle_check
    lda m_state
    cmp #MS_PLAY
    bne @d                      ; faute sifflee : on arrete
    ldx cp
@n: inx
    inx
    cpx #NUM_PLAYERS*2
    bne @l
@d: rts

; -----------------------------------------------------------------------------
;  carry_turnover : port trop long -> ballon a l'adversaire le plus proche
; -----------------------------------------------------------------------------
carry_turnover:
    .a16
    .i16
    stz tk_foul
    lda b_owner
    asl a
    tax
    lda p_team,x
    eor #1
    asl a
    tay
    lda near_team,y
    cmp #NO_OWNER
    bne :+
    lda p_team,x                ; personne : l'attaquant adverse
    eor #1
    beq :+
    lda #TEAM_SIZE + 5
    bra :++
:   lda near_team,y
:   jsr take_ball
    ldy #.loword(str_toolong)
    jsr show_msg
    lda #MS_FOUL
    sta m_state
    lda #TU_SEC / 2
    sta m_timer
    rts

; -----------------------------------------------------------------------------
;  tackle_check : X = cp = joueur en charge
; -----------------------------------------------------------------------------
tackle_check:
    .a16
    .i16
    lda b_owner
    cmp #NO_OWNER
    beq @late
    asl a
    tay
    lda p_team,y
    cmp p_team,x
    beq @late
    jsr contact_xy
    bcc @late
    sty t7                      ; victime*2
    jmp resolve_tackle
@late:
    ; contact tardif sur le passeur
    lda b_passer
    cmp #NO_OWNER
    beq @no
    lda b_passt
    cmp #T_LATE_HIT
    bcs @no
    lda b_passer
    asl a
    tay
    lda p_team,y
    cmp p_team,x
    beq @no
    lda p_state,y
    cmp #PS_DOWN
    beq @no
    ; faute seulement si la charge a commence apres la passe
    lda #T_TACKLE
    sec
    sbc p_timer,x
    cmp b_passt
    bcs @no
    jsr contact_xy
    bcc @no
    sty t7
    inc dbg_late
    lda #NO_OWNER
    sta b_passer
    lda #0
    jmp call_foul
@no:
    rts

; contact_xy : X, Y = joueurs*2 -> carry = 1 s'ils se touchent
contact_xy:
    .a16
    lda p_x,y
    sec
    sbc p_x,x
    ABS_A
    cmp #11 * FP
    bcs @no
    lda p_y,y
    sec
    sbc p_y,x
    ABS_A
    cmp #8 * FP
    bcs @no
    sec
    rts
@no:
    clc
    rts

; -----------------------------------------------------------------------------
;  resolve_tackle : X = cp = chargeur, t7 = porteur*2
; -----------------------------------------------------------------------------
resolve_tackle:
    .a16
    .i16
    inc dbg_turnovers
    stz tk_foul
    ; fin de la charge du chargeur
    stz p_state,x
    lda #T_TACKLE_CD
    sta p_cd,x
    ldy t7
    ; gardien dans sa zone : protege
    lda p_role,y
    bne @behind
    tyx
    jsr own_goal_x
    ldx cp
    sec
    sbc #0
    sta t0
    lda p_x,y
    ASR_A 4
    sec
    sbc t0
    sta t0
    lda p_y,y
    ASR_A 4
    sec
    sbc #FIELD_CY
    sta t1
    phy
    jsr dist_approx
    ply
    ldx cp
    cmp #ZONE_R + 4
    bcs @behind
    inc dbg_gkzone
    lda #1
    sta tk_foul
    bra @duel
@behind:
    ; charge par derriere : le porteur tourne le dos au chargeur
    lda p_dir,y
    sec
    sbc p_dir,x
    and #$0007
    cmp #2
    bcc @foulb
    cmp #7
    bcc @duel
@foulb:
    inc dbg_late
    lda #1
    sta tk_foul
    ; faute grave si le porteur etait en position de marquer
    phx
    tyx
    jsr opp_goal_x
    plx
    sta t0
    lda p_x,y
    ASR_A 4
    sec
    sbc t0
    ABS_A
    cmp #LONG_R
    bcs @duel
    lda #2
    sta tk_foul
@duel:
    ; porteur = POWER + CONTROL + alea ; chargeur = POWER + DEFENSE + alea
    jsr rand
    and #$0007
    sta t0
    ldy t7
    lda p_power,y
    clc
    adc p_ctrlst,y
    adc t0
    sta t1                      ; porteur
    jsr rand
    and #$0007
    ldx cp
    clc
    adc p_power,x
    adc p_defst,x
    sta t2                      ; chargeur
    ; fatigue du porteur
    ldy t7
    lda p_fatigue,y
    cmp #1600
    bcc :+
    inc t2
    inc t2
:
    lda t1
    clc
    adc #3
    cmp t2
    bcs @notsteal
    ; --- ballon arrache
    lda tk_foul
    beq :+
    jmp call_foul_t
:   ldy t7
    lda #PS_DOWN
    sta p_state,y
    lda #T_DOWN / 2
    sta p_timer,y
    lda cp
    lsr a
    jsr take_ball
    lda #T_GOAL / 4
    sta crowd_fast
    rts
@notsteal:
    lda t2
    clc
    adc #3
    cmp t1
    bcc @resist
    ; --- ballon lache
    lda tk_foul
    beq :+
    jmp call_foul_t
:   ldy t7
    lda #PS_DOWN
    sta p_state,y
    lda #T_DOWN
    sta p_timer,y
    lda #NO_OWNER
    sta b_owner
    lda t7
    lsr a
    sta b_nograb
    sta b_last
    lda #T_NOGRAB * 2
    sta b_nograb_t
    stz b_gkdone
    ldx cp
    lda p_vx,x
    ASR_A 1
    sta b_vx
    lda p_vy,x
    ASR_A 1
    sta b_vy
    lda rc+RC_VZ_DROP
    sta b_vz
    lda #T_GOAL / 4
    sta crowd_fast
    rts
@resist:
    ; --- le porteur resiste, le chargeur tombe
    ldx cp
    lda #PS_DOWN
    sta p_state,x
    lda #T_DOWN
    sta p_timer,x
    stz p_vx,x
    stz p_vy,x
    lda tk_foul
    beq :+
    ldy #.loword(str_advantage)  ; avantage : on laisse jouer
    jsr show_msg
:   rts

call_foul_t:
    lda tk_foul
    dec a                       ; 0 faute, 1 faute grave
; -----------------------------------------------------------------------------
;  call_foul : X = cp = fautif, t7 = victime*2, A = 1 si faute grave
; -----------------------------------------------------------------------------
call_foul:
    .a16
    .i16
    sta tk_foul
    inc dbg_fouls
    ldx cp
    lda #PS_DOWN
    sta p_state,x
    lda #T_DOWN
    sta p_timer,x
    stz p_vx,x
    stz p_vy,x
    ; le fautif recule
    jsr attack_sign
    sta t5
    lda #.loword(-24 * FP)
    jsr signed_t5
    clc
    adc p_x,x
    cmp #(FIELD_L + 6) * FP
    bcs :+
    lda #(FIELD_L + 6) * FP
:   cmp #(FIELD_R - 6) * FP
    bcc :+
    lda #(FIELD_R - 6) * FP
:   sta p_x,x
    ; la victime recupere le ballon
    ldy t7
    lda #PS_NORMAL
    sta p_state,y
    lda t7
    lsr a
    jsr take_ball
    lda #MS_FOUL
    sta m_state
    lda #TU_SEC
    sta m_timer
    lda #T_GOAL / 3
    sta crowd_fast
    lda tk_foul
    bne @major
    ldy #.loword(str_foul)
    jmp show_msg
@major:
    ; exclusion temporaire de 20 secondes de jeu (une seule a la fois)
    lda sent_off
    cmp #NO_OWNER
    beq :+
    ldy #.loword(str_foul)
    jmp show_msg
:   lda cp
    lsr a
    sta sent_off
    lda #20
    sta sent_t
    ldx cp
    lda #PS_OUT
    sta p_state,x
    ; la manette qui le controlait passe a un milieu
    ldy #0
@pad:
    lda ctrl,y
    asl a
    cmp cp
    bne @np
    lda p_team,x
    beq :+
    lda #TEAM_SIZE
:   clc
    adc #3
    jsr set_ctrl
    ldx cp
@np:
    iny
    iny
    cpy #4
    bne @pad
    ldy #.loword(str_major)
    jmp show_msg

; return_sent_off : fin d'exclusion, retour pres de son propre but
return_sent_off:
    .a16
    .i16
    lda sent_off
    asl a
    tax
    lda #NO_OWNER
    sta sent_off
    stz p_state,x
    stz p_vx,x
    stz p_vy,x
    jsr home_x
    asl a
    asl a
    asl a
    asl a
    sta p_x,x
    lda #(FIELD_T + 6) * FP
    sta p_y,x
    rts

; -----------------------------------------------------------------------------
;  separate_players : les joueurs ne se traversent pas
; -----------------------------------------------------------------------------
separate_players:
    .a16
    .i16
    ldx #0
@i: lda p_state,x
    cmp #PS_OUT
    beq @ni
    cmp #PS_DOWN
    beq @ni
    txy
    iny
    iny
@j: cpy #NUM_PLAYERS*2
    bcs @ni
    lda p_state,y
    cmp #PS_OUT
    beq @nj
    cmp #PS_DOWN
    beq @nj
    lda p_y,y
    sec
    sbc p_y,x
    ABS_A
    cmp #5 * FP
    bcs @nj
    lda p_x,y
    sec
    sbc p_x,x
    sta t0
    ABS_A
    cmp #9 * FP
    bcs @nj
    ; ecarter de 0.5 pixel chacun
    lda t0
    bmi @neg
    lda p_x,y
    clc
    adc #FP / 2
    sta p_x,y
    lda p_x,x
    sec
    sbc #FP / 2
    sta p_x,x
    bra @nj
@neg:
    lda p_x,y
    sec
    sbc #FP / 2
    sta p_x,y
    lda p_x,x
    clc
    adc #FP / 2
    sta p_x,x
@nj:
    iny
    iny
    bra @j
@ni:
    inx
    inx
    cpx #(NUM_PLAYERS - 1) * 2
    jcc @i
    rts

.segment "RODATA"
str_toolong:   .byte "HELD TOO LONG!", 0
str_foul:      .byte "FOUL!", 0
str_major:     .byte "MAJOR FOUL! 20 SEC OUT", 0
str_advantage: .byte "ADVANTAGE", 0
.segment "CODE"
