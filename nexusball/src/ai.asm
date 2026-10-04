; =============================================================================
;  ai.asm - intelligence artificielle des joueurs de champ
;
;  Chaque joueur IA "reflechit" a frequence reduite (p_think, en tu) et choisit
;  une cible (p_tx, p_ty) ou la poursuite du ballon (p_tx = $FFFF) ainsi
;  qu'une action eventuelle. A chaque frame, ai_steer convertit la cible en
;  direction manette (p_want), comme pour un humain : l'IA ne triche pas.
; =============================================================================

AI_CHASE = $FFFF

; -----------------------------------------------------------------------------
;  compute_nearest : joueur de champ le plus proche du ballon pour chaque equipe
; -----------------------------------------------------------------------------
compute_nearest:
    .a16
    .i16
    lda #NO_OWNER
    sta near_team
    sta near_team+2
    lda #$7FFF
    sta near_dist
    sta near_dist+2
    sta near2_dist
    sta near2_dist+2
    lda #NO_OWNER
    sta near2_team
    sta near2_team+2
    ldx #0
@l: lda p_role,x
    beq @n
    lda p_state,x
    cmp #PS_OUT
    beq @n
    cmp #PS_DOWN
    beq @n
    jsr dist_to_ball
    sta t3
    lda p_team,x
    asl a
    tay
    lda t3
    cmp near_dist,y
    bcs @second
    ; l'ancien premier devient deuxieme
    lda near_dist,y
    sta near2_dist,y
    lda near_team,y
    sta near2_team,y
    lda t3
    sta near_dist,y
    txa
    lsr a
    sta near_team,y
    bra @n
@second:
    cmp near2_dist,y
    bcs @n
    sta near2_dist,y
    txa
    lsr a
    sta near2_team,y
@n: inx
    inx
    cpx #NUM_PLAYERS*2
    bne @l
    rts

; -----------------------------------------------------------------------------
;  ai_update : tous les joueurs non controles par une manette
; -----------------------------------------------------------------------------
ai_update:
    .a16
    .i16
    ldx #0
@l: stx cp
    lda p_human,x
    bne @n
    lda p_state,x
    cmp #PS_OUT
    beq @n
    lda p_role,x
    bne @field
    jsr gk_ai
    bra @n
@field:
    lda p_think,x
    sec
    sbc rc+RC_TDEC
    sta p_think,x
    bpl :+
    jsr ai_think
    ldx cp
    jsr think_period
    sta p_think,x
    lda p_act,x
    and #JOY_A
    bne @n                      ; charge : direction deja fixee
:   ; deplacement vers la cible
    lda p_tx,x
    cmp #AI_CHASE
    bne @pt
    jsr chase_point
    bra @st
@pt:
    sta t0
    lda p_ty,x
    sta t1
@st:
    jsr ai_steer
@n: ldx cp
    inx
    inx
    cpx #NUM_PLAYERS*2
    bne @l
    rts

; chase_point : t0/t1 = position du ballon anticipee (pixels)
chase_point:
    .a16
    lda b_vx
    asl a
    asl a
    asl a                       ; 8 frames d'avance (12.4 -> x8)
    clc
    adc b_x
    ASR_A 4
    sta t0
    lda b_vy
    asl a
    asl a
    asl a
    clc
    adc b_y
    ASR_A 4
    sta t1
    rts

; -----------------------------------------------------------------------------
;  ai_steer : X = cp, t0/t1 = cible (pixels) -> p_want
; -----------------------------------------------------------------------------
ai_steer:
    .a16
    .i16
    stz t5                      ; bits
    lda p_x,x
    ASR_A 4
    eor #$FFFF
    sec
    adc t0
    sta t0
    lda p_y,x
    ASR_A 4
    eor #$FFFF
    sec
    adc t1
    sta t1
    lda t0
    ABS_A
    sta t2
    lda t1
    ABS_A
    sta t3
    ; horizontal (sauf si presque vertical)
    lda t2
    cmp #4
    bcc @v
    asl a
    asl a
    cmp t3                      ; 4|dx| < |dy| : pas d'horizontal
    bcc @v
    lda t0
    bmi :+
    lda #JOY_RIGHT
    bra :++
:   lda #JOY_LEFT
:   sta t5
@v: lda t3
    cmp #4
    bcc @d
    asl a
    asl a
    cmp t2
    bcc @d
    lda t1
    bmi :+
    lda #JOY_DOWN
    bra :++
:   lda #JOY_UP
:   ora t5
    sta t5
@d: lda t5
    sta p_want,x
    rts

; -----------------------------------------------------------------------------
;  ai_think : X = cp, decision tactique
; -----------------------------------------------------------------------------
ai_think:
    .a16
    .i16
    lda p_act,x
    and #.loword(~JOY_R)
    sta p_act,x
    lda b_owner
    cmp #NO_OWNER
    bne :+
    jmp ai_loose
:   txa
    lsr a
    cmp b_owner
    bne :+
    jmp ai_carrier
:   lda b_owner
    asl a
    tay
    lda p_team,y
    cmp p_team,x
    bne :+
    jmp ai_support
:   jmp ai_defend

; --- porteur du ballon
ai_carrier:
    .a16
    .i16
    stz p_act,x
    lda #TACT_TEMPO
    jsr get_tact
    beq :+                      ; tempo lent : pas de sprint
    lda #JOY_R
    sta p_act,x
:   ; port trop long (selon le tempo) : se debarrasser du ballon
    lda #TACT_TEMPO
    jsr get_tact
    asl a
    tay
    lda b_carry
    cmp carry_limit,y
    bcc @nolimit
    lda #TACT_PASS
    jsr get_tact
    cmp #2
    beq @long                   ; jeu long : au pied d'abord
    jsr find_hand_target
    cmp #NO_OWNER
    beq @long
    lda #JOY_B
    brl @act
@long:
    lda #JOY_X
    brl @act
@nolimit:
    jsr opp_goal_x
    sta ai_gx
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
    ldx cp
    sta t3
    cmp #72
    bcs :+
    lda #JOY_Y                  ; lancer a 1 point
    bra @act
:   cmp #LONG_R + 2
    bcc @run
    cmp #LONG_R + 26
    bcs @run
    lda p_kickst,x
    cmp #6
    bcc @run
    jsr rand
    and #$0003
    bne @run
    lda #JOY_Y                  ; tentative a 2 points
    bra @act
@run:
    ; adversaire au contact ?
    jsr nearest_opp_dist
    ldy #22
    pha
    lda #TACT_PASS
    jsr get_tact
    bne :+
    ldy #27                     ; jeu court : on passe plus tot
:   sty t4
    pla
    cmp t4
    bcs @go
    jsr rand
    and #$0003
    beq @dodge
    cmp #1
    bne @go
    jsr find_hand_target
    cmp #NO_OWNER
    beq @go
    lda #JOY_B
    bra @act
@dodge:
    lda #JOY_A
@act:
    ora p_act,x
    sta p_act,x
@go:
    ; course vers l'anneau, en s'ecartant selon son couloir
    jsr attack_sign
    sta t5
    lda ai_gx
    bit t5
    bmi :+
    sec
    sbc #44
    bra :++
:   clc
    adc #44
:   sta p_tx,x
    ; couloir selon ATTACK : centre, cotes, mixte
    lda p_homey,x
    sec
    sbc #FIELD_CY
    sta t4
    lda #TACT_ATT
    jsr get_tact
    beq @center
    cmp #1
    beq @sides
    lda t4
    ASR_A 1
    bra @lane
@center:
    lda t4
    ASR_A 3
    bra @lane
@sides:
    lda t4
    asl a
    cmp #.loword(-90)
    bpl :+
    lda #.loword(-90)
:   cmp #90
    bmi @lane
    lda #90
@lane:
    clc
    adc #FIELD_CY
    sta p_ty,x
    rts

; --- soutien du porteur
ai_support:
    .a16
    .i16
    stz p_act,x
    jsr attack_sign
    sta t5
    lda b_x
    ASR_A 4
    sta t0                      ; x porteur
    lda b_y
    ASR_A 4
    sta t1                      ; y porteur
    lda p_role,x
    cmp #ROLE_FW
    bne @mf
    ; attaquant : devant (selon MENTALITY), dans l'autre couloir
    lda #TACT_MENT
    jsr get_tact
    asl a
    tay
    lda fw_ahead,y
    jsr signed_t5
    clc
    adc t0
    sta t2
    lda t1
    cmp #FIELD_CY
    bcs :+
    adc #50
    bra :++
:   sbc #50
:   sta t3
    bra @clamp
@mf:
    cmp #ROLE_MF
    bne @df
    ; milieu : soutien lateral, legerement en retrait (passe a la main)
    lda #.loword(-18)
    jsr signed_t5
    clc
    adc t0
    sta t2
    lda p_homey,x
    cmp #FIELD_CY
    bcs :+
    lda t1
    sec
    sbc #52
    bra :++
:   lda t1
    clc
    adc #52
:   sta t3
    bra @clamp
@df:
    ; defenseur : couverture (selon DEF LINE)
    lda #TACT_LINE
    jsr get_tact
    asl a
    tay
    lda df_behind,y
    jsr signed_t5
    clc
    adc t0
    sta t2
    lda p_homey,x
    sta t3
@clamp:
    jsr clamp_t2t3
    lda t2
    sta p_tx,x
    lda t3
    sta p_ty,x
    rts

; --- ballon libre
ai_loose:
    .a16
    .i16
    stz p_act,x
    lda p_team,x
    asl a
    tay
    txa
    lsr a
    cmp near_team,y
    bne :+
    lda #AI_CHASE
    sta p_tx,x
    lda #JOY_R
    sta p_act,x
    rts
:   jmp ai_zone

; --- defense
ai_defend:
    .a16
    .i16
    stz p_act,x
    lda p_team,x
    asl a
    tay
    txa
    lsr a
    cmp near_team,y
    beq @presser
    ; pressing fort : le deuxieme plus proche harcele aussi
    cmp near2_team,y
    bne @zone
    lda #TACT_PRESS
    jsr get_tact
    cmp #2
    bne @zone
    bra @presser
@zone:
    jmp ai_zone
@presser:
    ; pressing faible : on attend le porteur dans sa moitie
    lda #TACT_PRESS
    jsr get_tact
    bne :+
    jsr attack_sign
    sta t5
    lda b_x
    ASR_A 4
    sec
    sbc #FIELD_CX
    jsr signed_t5
    cmp #0
    bpl @zone                   ; ballon dans la moitie adverse
:   ; gardien protege dans sa zone : on se replace
    lda b_owner
    asl a
    tay
    lda p_role,y
    bne :+
    jmp ai_zone
:   ; pressing sur le porteur
    lda #AI_CHASE
    sta p_tx,x
    lda #JOY_R
    sta p_act,x
    lda p_cd,x
    jne @no
    jsr dist_to_ball
    cmp #22
    jcs @no
    ; direction vers le porteur
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
    jsr atan64
    ldx cp
    clc
    adc #4
    lsr a
    lsr a
    lsr a
    and #$0007
    sta t3
    ; le porteur nous tourne le dos ? (charge par derriere = faute)
    lda b_owner
    asl a
    tay
    lda p_dir,y
    sec
    sbc t3
    and #$0007
    cmp #2
    bcc @behind
    cmp #7
    bcs @behind
    bra @tackle
@behind:
    jsr rand
    and #$003F
    bne @no                     ; rarement, l'IA commet la faute
@tackle:
    ; IA facile : hesite une fois sur deux
    jsr cpu_level
    bne :+
    jsr rand
    and #$0001
    bne @no
:   ldy t3
    lda dir8_pad,y
    and #$00FF
    xba
    sta p_want,x                ; la charge part vers le porteur
    lda #(JOY_A | JOY_R)
    sta p_act,x
@no:
    rts

; --- placement en zone (formation decalee vers le ballon)
ai_zone:
    .a16
    .i16
    jsr home_x
    sta t2
    lda b_x
    ASR_A 4
    sec
    sbc #FIELD_CX
    ASR_A 1
    clc
    adc t2
    sta t2
    jsr attack_sign
    sta t5
    ; MENTALITY (tous) et DEF LINE (defenseurs)
    lda #TACT_MENT
    jsr get_tact
    asl a
    tay
    lda tact_shift,y
    sta t4
    lda p_role,x
    cmp #ROLE_DF
    bne :+
    lda #TACT_LINE
    jsr get_tact
    asl a
    tay
    lda tact_shift,y
    clc
    adc t4
    sta t4
:   ; repli si l'adversaire a le ballon
    lda b_owner
    cmp #NO_OWNER
    beq :+
    lda t4
    sec
    sbc #20
    sta t4
:   lda t4
    jsr signed_t5
    clc
    adc t2
    sta t2
    lda b_y
    ASR_A 4
    sec
    sbc #FIELD_CY
    ASR_A 2
    clc
    adc p_homey,x
    sta t3
    jsr clamp_t2t3
    lda t2
    sta p_tx,x
    lda t3
    sta p_ty,x
    rts

; -----------------------------------------------------------------------------
;  tactiques et difficulte
; -----------------------------------------------------------------------------
TACT_MENT  = 0
TACT_PASS  = 1
TACT_PRESS = 2
TACT_LINE  = 3
TACT_ATT   = 4
TACT_TEMPO = 5

; get_tact : A = parametre, X = joueur*2 -> A = valeur (0..2). Preserve X, Y. Z positionne.
get_tact:
    .a16
    .i16
    phy
    asl a
    ldy p_team,x
    beq :+
    clc
    adc #12
:   tay
    lda tact,y
    ply
    cmp #0
    rts

; cpu_level : X = joueur*2 -> A = difficulte (0..3) si son equipe est jouee par
; le CPU, sinon 1 (coequipiers d'un humain : NORMAL). Z positionne.
cpu_level:
    .a16
    .i16
    lda p_team,x
    cmp pad_team
    beq @hum
    cmp pad_team+2
    beq @hum
    lda difficulty
    rts
@hum:
    lda #1
    rts

; think_period : X = joueur*2 -> A = delai avant la prochaine decision (tu)
think_period:
    .a16
    .i16
    jsr rand
    and #$000F
    clc
    adc #T_AI_THINK
    sta t4
    lda #TACT_TEMPO
    jsr get_tact
    asl a
    tay
    lda tempo_think,y
    clc
    adc t4
    sta t4
    jsr cpu_level
    asl a
    tay
    lda diff_think,y
    clc
    adc t4
    rts

tempo_think:  .word 20, 0, .loword(-12)
diff_think:   .word 45, 12, 0, .loword(-12)
carry_limit:  .word T_CARRY_WARN - 30, T_CARRY_WARN - 60, T_CARRY_WARN - 200
fw_ahead:     .word 50, 70, 92
df_behind:    .word .loword(-122), .loword(-100), .loword(-78)
tact_shift:   .word .loword(-24), 0, 24

; direction 0..7 -> bits manette (octet haut)
dir8_pad:
    .byte $01, $05, $04, $06, $02, $0A, $08, $09

; signed_t5 : A = valeur -> A * signe(t5)
signed_t5:
    .a16
    bit t5
    bpl :+
    eor #$FFFF
    inc a
:   rts

; clamp_t2t3 : borne la cible (t2, t3) a l'interieur du terrain
clamp_t2t3:
    .a16
    lda t2
    cmp #FIELD_L + 20
    bpl :+
    lda #FIELD_L + 20
:   cmp #FIELD_R - 20
    bmi :+
    lda #FIELD_R - 20
:   sta t2
    lda t3
    cmp #FIELD_T + 12
    bpl :+
    lda #FIELD_T + 12
:   cmp #FIELD_B - 8
    bmi :+
    lda #FIELD_B - 8
:   sta t3
    rts
