; =============================================================================
;  formation.asm - formation 2-2-1, mise en place des equipes
; =============================================================================

.segment "RODATA"
; positions de formation (pixels) pour une equipe attaquant vers la droite
form_x:     .word  36, 118, 118, 186, 186, 230
form_y:     .word 184, 140, 228, 112, 256, 184
form_role:  .word ROLE_GK, ROLE_DF, ROLE_DF, ROLE_MF, ROLE_MF, ROLE_FW
; positions au coup d'envoi
kick_x:     .word  36, 112, 112, 176, 176, 204
.segment "CODE"

; -----------------------------------------------------------------------------
;  setup_teams : stats, roles, positions de formation, equipe/manette
; -----------------------------------------------------------------------------
setup_teams:
    .a16
    .i16
    ldx #0                      ; index joueur*2
@pl:
    txa
    lsr a
    ldy #0
    cmp #TEAM_SIZE
    bcc :+
    iny
    sec
    sbc #TEAM_SIZE
:   sty t1                      ; equipe
    asl a
    sta t2                      ; slot*2
    tya
    sta p_team,x
    ldy t2
    lda form_x,y
    sta p_homex,x
    lda form_y,y
    sta p_homey,x
    lda form_role,y
    sta p_role,x
    ; stats : team_stats[t] + slot*7
    lda t1
    asl a
    tay
    lda team_stats,y
    sta t3
    lda t2
    lsr a
    sta t4
    asl a
    asl a
    asl a
    sec
    sbc t4                      ; slot*7
    clc
    adc t3
    tay
    lda a:0,y
    and #$00FF
    sta p_speed,x
    lda a:1,y
    and #$00FF
    sta p_power,x
    lda a:2,y
    and #$00FF
    sta p_passst,x
    lda a:3,y
    and #$00FF
    sta p_kickst,x
    lda a:4,y
    and #$00FF
    sta p_ctrlst,x
    lda a:5,y
    and #$00FF
    sta p_defst,x
    lda a:6,y
    and #$00FF
    sta p_stam,x
    stz p_fatigue,x
    stz p_human,x
    inx
    inx
    cpx #NUM_PLAYERS*2
    jne @pl
    rts

; -----------------------------------------------------------------------------
;  home_x : X = joueur*2 -> A = x de formation (pixels) selon le sens d'attaque
; -----------------------------------------------------------------------------
home_x:
    .a16
    .i16
    phy
    lda p_team,x
    asl a
    tay
    lda team_dir,y
    bne @mir
    lda p_homex,x
    ply
    rts
@mir:
    lda #(FIELD_L + FIELD_R)
    sec
    sbc p_homex,x
    ply
    rts

; attack_sign : X = joueur*2 -> A = +1 (attaque a droite) ou -1
attack_sign:
    .a16
    phy
    lda p_team,x
    asl a
    tay
    lda team_dir,y
    ply
    cmp #0
    bne :+
    lda #1
    rts
:   lda #.loword(-1)
    rts

; opp_goal_x : X = joueur*2 -> A = x de l'anneau adverse
opp_goal_x:
    .a16
    phy
    lda p_team,x
    asl a
    tay
    lda team_dir,y
    ply
    cmp #0
    bne :+
    lda #FIELD_R
    rts
:   lda #FIELD_L
    rts

; own_goal_x : X = joueur*2 -> A = x de l'anneau defendu
own_goal_x:
    .a16
    phy
    lda p_team,x
    asl a
    tay
    lda team_dir,y
    ply
    cmp #0
    bne :+
    lda #FIELD_L
    rts
:   lda #FIELD_R
    rts

; -----------------------------------------------------------------------------
;  place_kickoff : joueurs dans leur moitie, ballon au centre a l'equipe kick_team
; -----------------------------------------------------------------------------
place_kickoff:
    .a16
    .i16
    ldx #0
@pl:
    ; slot = index modulo 6
    txa
    lsr a
    cmp #TEAM_SIZE
    bcc :+
    sbc #TEAM_SIZE
:   asl a
    tay
    lda kick_x,y
    sta t0
    lda p_team,x
    cmp kick_team
    bne @nk
    cpy #(5*2)                  ; attaquant de l'equipe qui engage : au centre
    bne @nk
    lda #(FIELD_CX - 6)
    sta t0
@nk:
    lda p_team,x
    asl a
    tay
    lda team_dir,y
    beq :+
    lda #(FIELD_L + FIELD_R)
    sec
    sbc t0
    sta t0
:   lda t0
    asl a
    asl a
    asl a
    asl a
    sta p_x,x
    lda p_homey,x
    asl a
    asl a
    asl a
    asl a
    sta p_y,x
    stz p_vx,x
    stz p_vy,x
    stz p_z,x
    stz p_vz,x
    stz p_state,x
    stz p_timer,x
    stz p_cd,x
    stz p_think,x
    stz p_want,x
    stz p_act,x
    ; regard vers l'adversaire
    lda p_team,x
    asl a
    tay
    lda team_dir,y
    beq :+
    lda #4
    sta p_dir,x
    lda #$4000
    sta p_flip,x
    bra :++
:   stz p_dir,x
    stz p_flip,x
:   inx
    inx
    cpx #NUM_PLAYERS*2
    jne @pl

    ; joueur exclu : reste hors du terrain
    lda sent_off
    cmp #NO_OWNER
    beq :+
    asl a
    tax
    lda #PS_OUT
    sta p_state,x
:
    ; ballon a l'attaquant de l'equipe qui engage
    lda kick_team
    beq :+
    lda #TEAM_SIZE
:   clc
    adc #5
    sta t0
    jsr ball_reset
    lda t0
    jsr take_ball
    ; manettes : joueur controle = attaquant (ou milieu) de chaque equipe humaine
    jsr init_control
    rts
