; =============================================================================
;  formation.asm - 6 formations, composition, mise en place des equipes
; =============================================================================

NUM_FORMS = 6

.segment "RODATA"
; par formation, 5 slots de champ : x, y (pixels, equipe attaquant vers la droite), poste
; le slot 5 (dernier) est le plus avance : il donne le coup d'envoi
form_tab:
    ; 2-2-1
    .word 118,140,ROLE_DF, 118,228,ROLE_DF, 186,112,ROLE_MF, 186,256,ROLE_MF, 230,184,ROLE_FW
    ; 2-1-2
    .word 118,140,ROLE_DF, 118,228,ROLE_DF, 176,184,ROLE_MF, 230,248,ROLE_FW, 232,120,ROLE_FW
    ; 1-3-1
    .word 110,184,ROLE_DF, 176,110,ROLE_MF, 170,184,ROLE_MF, 176,258,ROLE_MF, 232,184,ROLE_FW
    ; 1-2-2
    .word 110,184,ROLE_DF, 170,130,ROLE_MF, 170,238,ROLE_MF, 232,248,ROLE_FW, 234,120,ROLE_FW
    ; 3-1-1
    .word 110,116,ROLE_DF, 104,184,ROLE_DF, 110,252,ROLE_DF, 176,184,ROLE_MF, 226,184,ROLE_FW
    ; 3-2-0
    .word 110,116,ROLE_DF, 104,184,ROLE_DF, 110,252,ROLE_DF, 180,238,ROLE_MF, 184,130,ROLE_MF
.segment "CODE"

; form_slot_ptr : A = formation, t2 = slot (1..5) -> Y = adresse de l'entree (x, y, poste)
; (5 entrees de 6 octets = 30 octets par formation). Preserve X.
form_slot_ptr:
    .a16
    .i16
    asl a
    sta t7                      ; f*2
    asl a
    asl a
    asl a
    asl a                       ; f*32
    sec
    sbc t7                      ; f*30
    sta t7
    lda t2
    dec a
    asl a
    sta t6                      ; (slot-1)*2
    asl a
    clc
    adc t6                      ; (slot-1)*6
    adc t7
    adc #.loword(form_tab)
    tay
    rts

; -----------------------------------------------------------------------------
;  setup_teams : roles, positions de formation et caracteristiques des 12 joueurs
;  sur le terrain (d'apres lineup), fatigue de l'effectif
; -----------------------------------------------------------------------------
setup_teams:
    .a16
    .i16
    ldx #0
@pl:
    stz p_human,x
    txa
    lsr a
    ldy #0
    cmp #TEAM_SIZE
    bcc :+
    iny
    sbc #TEAM_SIZE
:   tya
    sta p_team,x
    stx cp
    jsr load_slot_stats
    ldx cp
    inx
    inx
    cpx #NUM_PLAYERS*2
    bne @pl
    lda #0
    jsr apply_formation
    lda #1
    jmp apply_formation

; slot_of : X = joueur*2 -> A = slot 0..5
slot_of:
    .a16
    txa
    lsr a
    cmp #TEAM_SIZE
    bcc :+
    sbc #TEAM_SIZE
:   rts

; lineup_index : X = joueur*2 -> Y = offset dans lineup (equipe*12 + slot*2). Preserve X.
lineup_index:
    .a16
    .i16
    jsr slot_of
    asl a
    sta t6
    lda p_team,x
    beq :+
    lda #12
:   clc
    adc t6
    tay
    rts

; load_slot_stats : X = joueur*2 -> caracteristiques depuis l'effectif, fatigue
load_slot_stats:
    .a16
    .i16
    jsr lineup_index
    lda lineup,y
    sta t5                      ; index effectif
    ; fatigue sauvegardee de ce joueur
    asl a
    ldy p_team,x
    beq :+
    clc
    adc #24
:   tay
    lda r_fat,y
    sta p_fatigue,x
    phx
    lda p_team,x
    ldx t5
    jsr roster_rec
    plx
    ; teinte de peau (masque de l'equipe)
    phy
    lda t5
    asl a
    tay
    lda bit_tab,y
    and trec_buf+T_SKIN
    beq :+
    lda #1
:   sta p_skin,x
    ply
    lda a:PL_STATS+0,y
    and #$00FF
    sta p_speed,x
    lda a:PL_STATS+1,y
    and #$00FF
    sta p_power,x
    lda a:PL_STATS+2,y
    and #$00FF
    sta p_passst,x
    lda a:PL_STATS+3,y
    and #$00FF
    sta p_kickst,x
    lda a:PL_STATS+4,y
    and #$00FF
    sta p_ctrlst,x
    lda a:PL_STATS+5,y
    and #$00FF
    sta p_defst,x
    lda a:PL_STATS+6,y
    and #$00FF
    sta p_stam,x
    rts

; save_slot_fatigue : X = joueur*2 -> recopie p_fatigue dans r_fat
save_slot_fatigue:
    .a16
    .i16
    jsr lineup_index
    lda lineup,y
    asl a
    ldy p_team,x
    beq :+
    clc
    adc #24
:   tay
    lda p_fatigue,x
    sta r_fat,y
    rts

; -----------------------------------------------------------------------------
;  apply_formation : A = cote -> postes et positions de base des 6 joueurs
; -----------------------------------------------------------------------------
apply_formation:
    .a16
    .i16
    sta t5
    asl a
    tay
    lda form_id,y
    sta t4
    lda t5
    beq :+
    lda #TEAM_SIZE
:   asl a
    tax
    ; gardien
    lda #ROLE_GK
    sta p_role,x
    lda #36
    sta p_homex,x
    lda #FIELD_CY
    sta p_homey,x
    lda #1
    sta t2
@s: inx
    inx
    lda t4
    jsr form_slot_ptr
    lda a:0,y
    sta p_homex,x
    lda a:2,y
    sta p_homey,x
    lda a:4,y
    sta p_role,x
    inc t2
    lda t2
    cmp #6
    bne @s
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
:   sta t1                      ; slot
    lda p_homex,x
    cmp #200
    bcc :+
    lda #200
:   sta t0
    lda p_team,x
    cmp kick_team
    bne @nk
    lda t1
    cmp #5                      ; joueur le plus avance de l'equipe qui engage : au centre
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
