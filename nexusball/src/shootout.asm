; =============================================================================
;  shootout.asm - tirs au but : 3 tentatives par equipe, puis mort subite.
;  Attaquant contre gardien depuis une marque fixe, 5 secondes pour tirer.
;  Les tireurs tournent (du joueur le plus avance vers l'arriere).
; =============================================================================

SO_MARK_X  = FIELD_R - 110
SO_TIME    = 5 * TU_SEC
SO_FLIGHT  = TU_SEC * 3 / 2
SO_RESULT  = TU_SEC * 3 / 2

start_shootout:
    .a16
    .i16
    stz so_goals
    stz so_goals+2
    stz so_taken
    stz so_taken+2
    lda #1
    sta so_active
    lda first_kick
    sta so_team
    lda #4
    sta m_half
    jsr hud_draw_static
    jmp so_setup

; -----------------------------------------------------------------------------
;  so_setup : prepare la tentative de l'equipe so_team
; -----------------------------------------------------------------------------
so_setup:
    .a16
    .i16
    ldx #0
@hide:
    lda #PS_OUT
    sta p_state,x
    stz p_vx,x
    stz p_vy,x
    stz p_z,x
    stz p_vz,x
    stz p_want,x
    stz p_act,x
    stz p_cd,x
    inx
    inx
    cpx #NUM_PLAYERS*2
    bne @hide
    ; l'equipe qui tire attaque l'anneau de droite
    lda so_team
    asl a
    tax
    stz team_dir,x
    txa
    eor #2
    tax
    lda #1
    sta team_dir,x
    ; tireur : slot 5 - (tentatives mod 5)
    lda so_team
    asl a
    tax
    lda so_taken,x
@mod:
    cmp #5
    bcc :+
    sbc #5
    bra @mod
:   eor #$FFFF
    sec
    adc #5                      ; 5 - n
    sta t0
    lda so_team
    beq :+
    lda #TEAM_SIZE
:   clc
    adc t0
    sta so_shooter
    asl a
    tax
    stz p_state,x
    stz p_dir,x
    stz p_flip,x
    lda #SO_MARK_X * FP
    sta p_x,x
    lda #FIELD_CY * FP
    sta p_y,x
    ; gardien adverse
    lda so_team
    eor #1
    beq :+
    lda #TEAM_SIZE
:   asl a
    tax
    stz p_state,x
    lda #4
    sta p_dir,x
    lda #$4000
    sta p_flip,x
    lda #(FIELD_R - 10) * FP
    sta p_x,x
    lda #FIELD_CY * FP
    sta p_y,x
    ; ballon au tireur
    jsr ball_reset
    lda so_shooter
    jsr take_ball
    jsr init_control
    lda #MS_SHOOT
    sta m_state
    stz so_phase
    lda #SO_TIME
    sta m_timer
    ldy #.loword(str_shootout)
    jsr show_msg
    jsr camera_snap
    rts

; -----------------------------------------------------------------------------
;  st_shoot : une frame de tirs au but
; -----------------------------------------------------------------------------
st_shoot:
    .a16
    .i16
    lda so_phase
    cmp #2
    beq @result
    jsr human_input
    jsr ai_update
    jsr players_update
    jsr ball_update
    lda so_phase
    cmp #2
    beq @w                      ; but marque pendant cette frame
    lda so_phase
    bne @flight
    ; phase 0 : le tireur a encore le ballon ?
    lda b_owner
    cmp so_shooter
    beq @hold
    lda #1
    sta so_phase
    lda #SO_FLIGHT
    sta m_timer
    bra @w
@hold:
    lda m_timer
    bne @w
    ldy #.loword(str_so_miss)   ; temps ecoule
    lda #0
    jmp so_result
@flight:
    ; ballon capte par le gardien ?
    lda b_owner
    cmp #NO_OWNER
    beq :+
    cmp so_shooter
    beq :+
    ldy #.loword(str_so_saved)
    lda #0
    jmp so_result
:   lda m_timer
    bne @w
    ldy #.loword(str_so_miss)
    lda #0
    jmp so_result
@result:
    jsr players_update
    jsr ball_update
    lda m_timer
    bne @w
    jsr so_next
@w: clc
    rts

; so_goal : appele par goal_scored pendant les tirs au but
so_goal:
    .a16
    lda so_phase
    cmp #2
    beq :+
    ldy #.loword(str_so_goal)
    lda #1
    jmp so_result
:   rts

; so_result : A = 1 si but, Y = message
so_result:
    .a16
    .i16
    sta t5
    lda so_team
    asl a
    tax
    lda t5
    beq :+
    inc so_goals,x
    lda #T_GOAL / 2
    sta crowd_fast
:   lda #2
    sta so_phase
    lda #SO_RESULT
    sta m_timer
    ; message "XXXXX  a-b"
    ldx #0
@cp:
    lda a:0,y
    and #$00FF
    beq @num
    sep #$20
    .a8
    sta str_buf,x
    rep #$20
    .a16
    inx
    iny
    bra @cp
@num:
    lda so_goals
    jsr so_digit
    lda #'-'
    jsr so_char
    lda so_goals+2
    jsr so_digit
    lda #0
    jsr so_char
    ldy #.loword(str_buf)
    jsr show_msg
    lda #SO_RESULT
    sta msg_time
    clc
    rts

so_digit:
    .a16
    cmp #10
    bcc :+
    lda #9
:   clc
    adc #'0'
so_char:
    sep #$20
    .a8
    sta str_buf,x
    rep #$20
    .a16
    inx
    rts

; -----------------------------------------------------------------------------
;  so_next : tentative suivante ou fin
; -----------------------------------------------------------------------------
so_next:
    .a16
    .i16
    lda so_team
    asl a
    tax
    inc so_taken,x
    lda so_taken
    cmp so_taken+2
    bne @cont                   ; serie pas terminee
    cmp #3
    bcc @early
    lda so_goals
    cmp so_goals+2
    bne @end
    bra @cont
@early:
    ; impossible de rattraper apres un tour complet ?
    lda #3
    sec
    sbc so_taken
    sta t0                      ; tirs restants par equipe
    lda so_goals
    clc
    adc t0
    cmp so_goals+2
    bcc @end
    lda so_goals+2
    clc
    adc t0
    cmp so_goals
    bcc @end
@cont:
    lda so_team
    eor #1
    sta so_team
    jmp so_setup
@end:
    lda #MS_END
    sta m_state
    lda #T_GOAL
    sta m_timer
    jsr freeze_players
    jmp hud_full_time

.segment "RODATA"
str_shootout:  .byte "SHOOTOUT", 0
str_so_goal:   .byte "GOAL!  ", 0
str_so_saved:  .byte "SAVED!  ", 0
str_so_miss:   .byte "MISSED!  ", 0
.segment "CODE"
