; =============================================================================
;  match.asm - deroulement d'un match : etats, horloge, buts, pause
; =============================================================================

; -----------------------------------------------------------------------------
;  start_match : game_mode deja choisi
; -----------------------------------------------------------------------------
start_match:
    .a16
    .i16
    jsr screen_off
    lda #0
    jsr ad_show                 ; publicites avant le match
    jsr layers_match
    jsr ensure_stadium_bg
    jsr load_crowd
    ; manettes -> equipes (deja reglees par une competition)
    lda game_mode
    cmp #MODE_PRESET
    beq @pads_ok
    lda #0
    sta pad_team
    lda #NO_OWNER
    sta pad_team+2
    lda game_mode
    cmp #MODE_2P
    bne :+
    lda #1
    sta pad_team+2
:   lda game_mode
    cmp #MODE_CPU
    bne :+
    lda #NO_OWNER
    sta pad_team
:
@pads_ok:
    stz team_dir
    lda #1
    sta team_dir+2
    stz score
    stz score+2
    stz so_active
    ldx #0
:   stz st_shots,x
    stz st_poss,x
    stz st_fouls,x
    inx
    inx
    cpx #4
    bne :-
    lda #1
    sta m_half
    ; duree : (menu_len + 2) minutes
    lda menu_len
    clc
    adc #2
    tay
    lda #60
    jsr mulu8
    sta m_len
    sta m_sec
    stz m_acc
    lda #NO_OWNER
    sta sent_off
    ; equipe qui engage : tirage au sort (alea melange au temps passe sur le titre)
    lda rng
    eor nmi_count
    ora #$0100
    sta rng
    jsr rand
    and #$0001
    sta kick_team
    sta first_kick

    ; fatigue de tous les effectifs a zero
    ldx #0
:   stz r_fat,x
    inx
    inx
    cpx #48
    bne :-
    jsr load_kits
    lda stadium_id
    jsr load_stadium
    jsr setup_teams
    jsr bg3_clear
    jsr hud_init
    jsr new_kickoff
    jsr camera_snap
    jsr oam_clear
    jsr screen_on
    stz title_music
    lda #MUS_STOP
    jsr sfx_play
    lda #$18
    jsr crowd_level

match_loop:
    jsr wait_frame
    jsr match_frame
    bcc match_loop
    lda comp_active
    beq :+
    jmp comp_after_match
:   jmp title_screen

; -----------------------------------------------------------------------------
;  new_kickoff : place les joueurs, etat KICKOFF
; -----------------------------------------------------------------------------
new_kickoff:
    .a16
    jsr place_kickoff
    lda #MS_KICKOFF
    sta m_state
    lda #T_KICKOFF
    sta m_timer
    ldy #.loword(str_ready)
    jsr show_msg
    rts

; -----------------------------------------------------------------------------
;  match_frame : une frame de match. Carry = 1 pour quitter vers le titre.
; -----------------------------------------------------------------------------
match_frame:
    .a16
    .i16
    ; pause
    lda m_state
    cmp #MS_END
    beq @nopause
    lda joy_new
    ora joy_new+2
    and #JOY_START
    beq @nopause
    jsr pause_menu
    bcc @nopause
    rts                         ; quitter
@nopause:
    lda m_timer
    sec
    sbc rc+RC_TDEC
    bpl :+
    lda #0
:   sta m_timer

    lda m_state
    asl a
    tax
    jsr (.loword(state_tab),x)
    bcs @quit

    jsr camera_update
    jsr crowd_update
    jsr hud_update
    jsr build_sprites
    clc
@quit:
    rts

state_tab:
    .word .loword(st_kickoff), .loword(st_play), .loword(st_goal)
    .word .loword(st_half), .loword(st_end), .loword(st_foul)
    .word .loword(st_shoot)

; --- coup d'envoi : les joueurs attendent le signal
st_kickoff:
    .a16
    jsr ball_update
    lda m_timer
    bne @w
    lda #SFX_WHISTLE
    jsr sfx_play
    lda #MS_PLAY
    sta m_state
    ldy #.loword(str_go)
    jsr show_msg
    lda #(T_MSG / 3)
    sta msg_time
@w: clc
    rts

; --- jeu
st_play:
    .a16
    jsr clock_update
    bcs @end
    jsr compute_nearest
    jsr human_input
    jsr ai_update
    jsr players_update
    jsr ball_update
    jsr rules_update
    jsr separate_players
    ; possession
    lda b_owner
    cmp #NO_OWNER
    beq :+
    asl a
    tax
    lda p_team,x
    asl a
    tax
    inc st_poss,x
:   clc
    rts
@end:
    clc
    rts

; --- but marque : celebration
st_goal:
    .a16
    jsr players_update
    jsr ball_update
    lda m_timer
    bne @w
    lda m_half
    cmp #3
    beq @golden
    jsr new_kickoff
@w: clc
    rts
@golden:
    ; but en or : fin du match
    lda #MS_END
    sta m_state
    jsr freeze_players
    jsr hud_full_time
    clc
    rts

; --- mi-temps
st_half:
    .a16
    lda m_timer
    bne @w
    ; publicites de la mi-temps (pas avant la prolongation)
    lda m_half
    cmp #1
    bne @noad
    jsr screen_off
    lda #1
    jsr ad_show
    jsr ensure_stadium_bg
    jsr layers_match
    jsr bg3_clear
    lda #1
    sta ad_flag
    bra :+
@noad:
    stz ad_flag
:   ; changement de cote, l'autre equipe engage
    lda team_dir
    eor #1
    sta team_dir
    lda team_dir+2
    eor #1
    sta team_dir+2
    lda first_kick
    eor #1
    sta kick_team
    jsr halftime_rest
    inc m_half
    lda m_len
    ldy m_half
    cpy #3
    bne :+
    lda #120                    ; prolongation : 2 minutes, but en or
:   sta m_sec
    stz m_acc
    jsr hud_draw_static
    jsr new_kickoff
    lda ad_flag
    beq @w
    jsr camera_snap
    jsr screen_on
@w: clc
    rts

; halftime_rest : a la pause, toute la fatigue des effectifs est divisee par 2
halftime_rest:
    .a16
    .i16
    ldx #0
@sv:
    stx cp
    jsr save_slot_fatigue
    ldx cp
    inx
    inx
    cpx #NUM_PLAYERS*2
    bne @sv
    ldx #0
@h: lsr r_fat,x
    inx
    inx
    cpx #48
    bne @h
    ldx #0
@ld:
    stx cp
    jsr load_slot_stats
    ldx cp
    inx
    inx
    cpx #NUM_PLAYERS*2
    bne @ld
    rts

; --- fin du match : START pour revenir au titre
st_end:
    .a16
    jsr players_update
    jsr ball_update
    lda m_timer
    bne @w
    lda joy_new
    ora joy_new+2
    and #(JOY_START | JOY_A | JOY_B)
    bne @quit
    lda game_mode
    cmp #MODE_CPU
    bne @w
    lda joy_cur                 ; mode demo : retour automatique
    ora joy_cur+2
    bne @quit
@w: clc
    rts
@quit:
    sec
    rts

; --- faute : courte interruption puis remise en jeu
st_foul:
    .a16
    lda tk_foul
    bne :+                      ; faute grave : horloge arretee
    jsr clock_update
    bcc :+
    clc
    rts
:   jsr ball_update
    lda m_timer
    bne @w
    lda #MS_PLAY
    sta m_state
@w: clc
    rts

; -----------------------------------------------------------------------------
;  clock_update : decompte du temps de jeu. Carry = 1 si la periode est finie.
; -----------------------------------------------------------------------------
clock_update:
    .a16
    lda m_acc
    clc
    adc rc+RC_TDEC
    sta m_acc
    cmp #TU_SEC
    bcc @ok
    sbc #TU_SEC
    sta m_acc
    ; exclusion temporaire (secondes de jeu)
    lda sent_off
    cmp #NO_OWNER
    beq :+
    dec sent_t
    bne :+
    jsr return_sent_off
:   dec m_sec
    bne @ok
    jsr period_over
    sec
    rts
@ok:
    clc
    rts

period_over:
    .a16
    lda #SFX_WHISTLE
    jsr sfx_play
    lda #SFX_WHISTLE
    jsr sfx_play
    lda #T_GOAL
    sta m_timer
    lda m_half
    cmp #3
    bne :+
    jmp start_shootout          ; prolongation terminee sans but
:   cmp #2
    bne @half
    ; egalite et regle de coupe : prolongation
    lda draw_rule
    beq @full
    lda score
    cmp score+2
    bne @full
    lda #MS_HALF
    sta m_state
    ldy #.loword(str_overtime)
    jsr show_msg
    lda #T_GOAL
    sta msg_time
    jsr freeze_players
    rts
@half:
    lda #MS_HALF
    sta m_state
    ldy #.loword(str_half)
    jsr show_msg
    lda #T_GOAL
    sta msg_time
    jsr freeze_players
    rts
@full:
    lda #MUS_JINGLE
    jsr sfx_play
    lda #MS_END
    sta m_state
    jsr freeze_players
    jsr hud_full_time
    rts

; freeze_players : arrete tout le monde (fin de periode)
freeze_players:
    .a16
    .i16
    ldx #0
@l: stz p_vx,x
    stz p_vy,x
    stz p_want,x
    stz p_act,x
    lda p_state,x
    cmp #PS_OUT
    beq :+
    stz p_state,x
:   inx
    inx
    cpx #NUM_PLAYERS*2
    bne @l
    rts

; -----------------------------------------------------------------------------
;  goal_scored : A = equipe qui marque, b_points = valeur
; -----------------------------------------------------------------------------
goal_scored:
    .a16
    .i16
    ldx m_state
    cpx #MS_SHOOT
    bne :+
    jmp so_goal
:   sta t0
    ; but contre son camp : toujours 1 point
    lda b_last
    cmp #NO_OWNER
    beq :+
    asl a
    tax
    lda p_team,x
    cmp t0
    beq :+
    lda #1
    sta b_points
:   lda t0
    asl a
    tax
    lda score,x
    clc
    adc b_points
    cmp #100
    bcc :+
    lda #99
:   sta score,x
    ; l'equipe qui encaisse engage
    lda t0
    eor #1
    sta kick_team
    lda #MS_GOAL
    sta m_state
    lda #T_GOAL
    sta m_timer
    lda #NO_OWNER
    sta b_owner
    lda #SFX_GOAL
    jsr sfx_play
    lda #SFX_WHISTLE
    jsr sfx_play
    lda b_points
    cmp #2
    beq @two
    ldy #.loword(str_goal1)
    bra @msg
@two:
    ldy #.loword(str_goal2)
@msg:
    jsr show_msg
    lda #T_GOAL
    sta msg_time
    ; public en folie
    lda #T_GOAL
    sta crowd_fast
    ; celebration des buteurs
    ldx #0
@l: lda p_team,x
    cmp t0
    bne @opp
    lda p_state,x
    cmp #PS_OUT
    beq @nx
    lda #PS_CELEB
    sta p_state,x
    bra @st
@opp:
    lda p_state,x
    cmp #PS_OUT
    beq @nx
    stz p_state,x
@st:
    stz p_want,x
    stz p_act,x
@nx:
    inx
    inx
    cpx #NUM_PLAYERS*2
    bne @l
    rts

; -----------------------------------------------------------------------------
;  pause_menu : RESUME / RADAR / QUIT MATCH. Carry = 1 pour quitter.
; -----------------------------------------------------------------------------
;  pause_menu : RESUME / TEAM SETUP / RADAR / QUIT MATCH. Carry = 1 pour quitter.
; -----------------------------------------------------------------------------
PAUSE_ITEMS = 4

pause_menu:
    .a16
    .i16
    stz pause_sel
    ; equipe de la manette qui a mis en pause (equipe 1 par defaut)
    stz ui_team
    lda joy_new+2
    and #JOY_START
    beq :+
    lda pad_team+2
    cmp #NO_OWNER
    beq :+
    sta ui_team
:   ; sauvegarde des lignes utilisees
    ldx #0
@sv:
    lda bg3_map + 10*64,x
    sta pause_save,x
    inx
    inx
    cpx #(5*64)
    bne @sv
@draw:
    lda #10
@fill:
    pha
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc #10 * 2
    tax
    ldy #12
    lda #TXT_ATTR + TXT_PANEL
    jsr fill_tiles
    pla
    inc a
    cmp #15
    bne @fill
    ldx #TPOS(13, 10)
    ldy #.loword(str_pause)
    jsr print_panel
    ldx #TPOS(12, 11)
    ldy #.loword(str_resume)
    jsr print_panel
    ldx #TPOS(12, 12)
    ldy #.loword(str_teamset)
    jsr print_panel
    ldx #TPOS(12, 13)
    ldy #.loword(str_radar_on)
    lda opt_radar
    bne :+
    ldy #.loword(str_radar_off)
:   jsr print_panel
    ldx #TPOS(12, 14)
    ldy #.loword(str_quit)
    jsr print_panel
    ; curseur
    lda pause_sel
    clc
    adc #11
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc #(11*2)
    tax
    lda #('>' - 32 + TXT_ATTR + TXT_PANEL)
    sta bg3_map,x
@wait:
    jsr build_sprites
    jsr wait_frame
    lda joy_new
    ora joy_new+2
    sta t7
    beq @wait
    lda pause_sel
    ldy #PAUSE_ITEMS
    jsr ui_updown
    cmp pause_sel
    beq :+
    sta pause_sel
    jmp @draw
:   lda t7
    bit #JOY_START
    bne @resume
    bit #JOY_SELECT
    bne @radar
    and #(JOY_A | JOY_B)
    beq @wait
    lda pause_sel
    beq @resume
    cmp #1
    beq @team
    cmp #2
    beq @radar
    ; quitter
    jsr pause_restore
    sec
    rts
@team:
    lda #1
    sta ui_back
    jsr team_screen
    jsr layers_match
    jsr bg2_crowd_map
    jsr bg3_clear
    jsr hud_draw_static
    jsr hud_update
    jsr pause_restore_clear
    jmp @draw
@radar:
    lda opt_radar
    eor #1
    sta opt_radar
    jsr save_options
    jmp @draw
@resume:
    jsr pause_restore
    clc
    rts

; pause_restore_clear : apres un ecran plein, la zone sauvegardee devient vide
pause_restore_clear:
    .a16
    ldx #0
@l: stz pause_save,x
    inx
    inx
    cpx #(5*64)
    bne @l
    rts

pause_restore:
    .a16
    ldx #0
@l: lda pause_save,x
    sta bg3_map + 10*64,x
    inx
    inx
    cpx #(5*64)
    bne @l
    lda #1
    sta bg3_dirty
    rts

.segment "RODATA"
str_ready:  .byte "READY", 0
str_go:     .byte "GO!", 0
str_half:   .byte "HALF TIME", 0
str_overtime: .byte "OVERTIME - GOLDEN SCORE", 0
str_goal1:  .byte "SCORE! +1", 0
str_goal2:  .byte "SCORE! +2", 0
str_pause:  .byte "PAUSE", 0
str_resume: .byte "RESUME", 0
str_radar_on:  .byte "RADAR ON ", 0
str_radar_off: .byte "RADAR OFF", 0
str_quit:   .byte "QUIT MATCH", 0
str_teamset: .byte "TEAM SETUP", 0
.segment "CODE"
