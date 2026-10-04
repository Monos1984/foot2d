; =============================================================================
;  ball.asm - physique du ballon : vol, rebonds sol/murs, anneaux, prise de balle
; =============================================================================

; ball_reset : ballon au centre, immobile
ball_reset:
    .a16
    lda #FIELD_CX * FP
    sta b_x
    lda #FIELD_CY * FP
    sta b_y
    stz b_z
    stz b_vx
    stz b_vy
    stz b_vz
    lda #NO_OWNER
    sta b_owner
    sta b_last
    sta b_nograb
    sta b_passer
    stz b_nograb_t
    stz b_carry
    stz b_shot
    lda #1
    sta b_points
    lda #$7FFF
    sta b_passt
    lda #$FFFF
    sta b_relx
    rts

; -----------------------------------------------------------------------------
;  take_ball : A = joueur -> il prend le ballon
; -----------------------------------------------------------------------------
take_ball:
    .a16
    .i16
    sta b_owner
    sta b_last
    asl a
    tax
    lda p_team,x
    sta poss_team
    stz b_carry
    stz b_shot
    stz b_vx
    stz b_vy
    stz b_vz
    stz b_gkdone
    lda #NO_OWNER
    sta b_nograb
    lda p_state,x
    cmp #PS_TACKLE
    bne :+
    stz p_state,x
:   ; le receveur devient le joueur controle de sa manette
    ldy #0
@pad:
    lda pad_team,y
    cmp p_team,x
    bne @n
    lda p_role,x
    beq @n
    lda b_owner
    cmp ctrl,y
    beq @n
    phx
    jsr set_ctrl
    plx
@n: iny
    iny
    cpy #4
    bne @pad
    rts

; -----------------------------------------------------------------------------
;  ball_update
; -----------------------------------------------------------------------------
ball_update:
    .a16
    .i16
    lda b_nograb_t
    sec
    sbc rc+RC_TDEC
    bpl :+
    lda #0
:   sta b_nograb_t
    lda b_passt
    cmp #$7000
    bcs :+
    clc
    adc rc+RC_TDEC
    sta b_passt
:
    lda b_owner
    cmp #NO_OWNER
    beq @free
    ; ballon tenu : devant le porteur, a hauteur de poitrine
    asl a
    tax
    lda p_flip,x
    beq :+
    lda #.loword(-5 * FP)
    bra :++
:   lda #5 * FP
:   clc
    adc p_x,x
    sta b_x
    lda p_y,x
    clc
    adc #FP
    sta b_y
    lda p_z,x
    clc
    adc #8 * FP
    sta b_z
    lda p_vx,x
    sta b_vx
    lda p_vy,x
    sta b_vy
    stz b_vz
    rts

@free:
    lda b_x
    sta b_prevx
    clc
    adc b_vx
    sta b_x
    lda b_y
    clc
    adc b_vy
    sta b_y
    ; gravite
    lda b_vz
    sec
    sbc rc+RC_GRAV
    sta b_vz
    ASR_A 2
    clc
    adc b_z
    bpl @air
    ; contact avec le sol
    lda b_vz
    cmp #.loword(-48)
    bpl @stop
    eor #$FFFF
    inc a
    lsr a                       ; rebond : moitie de la vitesse
    sta b_vz
    lda #0
    bra @air
@stop:
    stz b_vz
    lda #0
@air:
    sta b_z
    ; frottements
    lda rc+RC_FRIC_A
    ldx b_z
    bne :+
    lda rc+RC_FRIC_G
:   sta t7
    lda b_vx
    beq :+
    jsr apply_friction
    sta b_vx
:   lda b_vy
    beq :+
    jsr apply_friction
    sta b_vy
:
    ; murs lateraux
    lda b_y
    cmp #(FIELD_T + 2) * FP
    bcs :+
    lda #(FIELD_T + 2) * FP
    sta b_y
    lda b_vy
    jsr wall_bounce
    sta b_vy
    bra @wx
:   cmp #(FIELD_B + 1) * FP
    bcc @wx
    lda #(FIELD_B + 1) * FP
    sta b_y
    lda b_vy
    jsr wall_bounce
    sta b_vy
@wx:
    ; murs de fond / anneaux
    lda m_state
    cmp #MS_PLAY
    beq :+
    cmp #MS_SHOOT
    beq :+
    cmp #MS_FOUL
    jne @nogoal
:   lda b_x
    cmp #FIELD_L * FP
    bcs @rgt
    ; plan du but gauche
    jsr ring_test
    bcc @bl
    ; but pour l'equipe qui attaque vers la gauche
    lda team_dir
    bne :+
    lda #1
    bra :++
:   lda #0
:   jsr goal_valid
    bcc @bl
    jsr goal_scored
    lda #(FIELD_L - 3) * FP
    sta b_x
    stz b_vx
    stz b_vy
    rts
@bl:
    inc dbg_ringmiss
    lda #FIELD_L * FP
    sta b_x
    lda b_vx
    jsr wall_bounce
    sta b_vx
    bra @pick
@rgt:
    cmp #(FIELD_R + 1) * FP
    bcc @pick
    jsr ring_test
    bcc @br
    lda team_dir
    beq :+
    lda #1
    bra :++
:   lda #0
:   jsr goal_valid
    bcc @br
    jsr goal_scored
    lda #(FIELD_R + 3) * FP
    sta b_x
    stz b_vx
    stz b_vy
    rts
@br:
    inc dbg_ringmiss
    lda #(FIELD_R + 1) * FP
    sta b_x
    lda b_vx
    jsr wall_bounce
    sta b_vx
@pick:
    lda m_state
    cmp #MS_SHOOT
    beq :+
    cmp #MS_PLAY
    bne @nogoal
:
    jsr gk_saves
    lda b_owner
    cmp #NO_OWNER
    bne @nogoal
    jsr ball_pickup
@nogoal:
    rts
@nopl:
    ; hors jeu actif : rester dans l'arene
    rts

; wall_bounce : A = vitesse -> -A * 3/4 (son si le choc est fort)
wall_bounce:
    .a16
    pha
    ABS_A
    cmp #28
    bcc :+
    lda #SFX_BOUNCE
    jsr sfx_play
:   pla
    eor #$FFFF
    inc a
    sta t0
    ASR_A 2
    eor #$FFFF
    sec
    adc t0
    rts

; ring_test : carry = 1 si le ballon passe dans l'anneau (dy^2 + dz^2 < r^2)
ring_test:
    .a16
    .i16
    lda b_y
    ASR_A 4
    sec
    sbc #FIELD_CY
    ABS_A
    cmp #RING_R
    bcs @no
    tay
    jsr mulu8                   ; dy^2
    sta t0
    lda b_z
    ASR_A 4
    sec
    sbc #RING_Z
    ABS_A
    cmp #RING_R
    bcs @no
    tay
    jsr mulu8
    clc
    adc t0
    cmp #(RING_R - 2) * (RING_R - 2)
    bcs @no
    sec
    rts
@no:
    clc
    rts

; -----------------------------------------------------------------------------
;  ball_pickup : un joueur proche recupere le ballon libre
; -----------------------------------------------------------------------------
ball_pickup:
    .a16
    .i16
    ; ordre de test alterne pour ne favoriser aucune equipe
    lda frame
    and #$0001
    beq :+
    lda #(NUM_PLAYERS - 1) * 2
    sta t6
    lda #.loword(-2)
    bra :++
:   stz t6
    lda #2
:   sta near_tmp+4
    lda #NUM_PLAYERS
    sta near_tmp+2
@l: ldx t6
    lda p_state,x
    cmp #PS_OUT
    jeq @n
    cmp #PS_DOWN
    jeq @n
    cmp #PS_CELEB
    jeq @n
    txa
    lsr a
    cmp b_nograb
    bne :+
    lda b_nograb_t
    jne @n
:   ; gardien qui a deja manque son arret sur ce tir
    lda p_role,x
    bne :+
    lda p_team,x
    inc a
    and b_gkdone
    jne @n
:   lda b_x
    sec
    sbc p_x,x
    ABS_A
    cmp #8 * FP
    jcs @n
    lda b_y
    sec
    sbc p_y,x
    ABS_A
    cmp #7 * FP
    jcs @n
    ; hauteur atteignable
    lda p_z,x
    clc
    adc #22 * FP
    cmp b_z
    bcc @n
    ; ballon trop rapide pour CONTROL ?
    lda b_vx
    ABS_A
    sta t0
    lda b_vy
    ABS_A
    clc
    adc t0
    sta t0
    lda p_ctrlst,x
    asl a
    asl a
    asl a
    clc
    adc rc+RC_PICKUP_V
    cmp t0
    bcs @take
    jsr rand
    and #$000F
    cmp p_ctrlst,x
    bcc @take
    ; controle rate : le ballon rebondit sur le joueur
    lda b_vx
    ASR_A 2
    eor #$FFFF
    inc a
    sta b_vx
    lda b_vy
    ASR_A 1
    sta b_vy
    lda #100
    sta b_vz
    txa
    lsr a
    sta b_nograb
    sta b_last
    lda #$FFFF
    sta b_relx
    txa
    lsr a
    lda #T_NOGRAB / 2
    sta b_nograb_t
    rts
@take:
    txa
    lsr a
    jmp take_ball
@n: lda t6
    clc
    adc near_tmp+4
    sta t6
    dec near_tmp+2
    jne @l
    rts

; -----------------------------------------------------------------------------
;  goal_valid : A = equipe qui marquerait -> C = 1 si le point compte (A preserve).
;  Un tir lance depuis sa propre moitie de terrain ne compte pas (sauf contre son
;  camp et pendant les tirs au but) : message "NO SCORE FROM OWN HALF".
; -----------------------------------------------------------------------------
goal_valid:
    .a16
    .i16
    pha
    sta gv_team
    ldx m_state
    cpx #MS_SHOOT
    beq @ok
    lda b_relx
    cmp #$FFFF
    beq @ok
    lda b_last
    cmp #NO_OWNER
    beq @ok
    asl a
    tax
    lda p_team,x
    cmp gv_team
    bne @ok                     ; contre son camp
    lda gv_team
    asl a
    tax
    lda team_dir,x
    bne @left
    lda b_relx                  ; attaque vers la droite : moitie gauche interdite
    cmp #FIELD_CX
    bcc @no
    bra @ok
@left:
    lda b_relx
    cmp #FIELD_CX
    bcs @no
@ok:
    pla
    sec
    rts
@no:
    lda #$FFFF
    sta b_relx
    lda gv_team
    ldx #$FF
    ldy #.loword(str_ownhalf)
    jsr show_tmsg
    pla
    clc
    rts

.segment "RODATA"
str_ownhalf:    .byte "NO SCORE FROM OWN HALF", 0
.segment "CODE"
