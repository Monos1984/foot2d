; =============================================================================
;  match.asm - deroulement d'un match : etats, horloge, buts, pause
; =============================================================================

; -----------------------------------------------------------------------------
;  start_match : game_mode deja choisi
; -----------------------------------------------------------------------------
start_match:
    .a16
    .i16
    stz sort_ok                 ; ordre d'affichage a reinitialiser
    jsr screen_off
    stz wx_init                 ; meteo a reinitialiser
    jsr match_intro             ; presentation des equipes
    lda #0
    jsr coin_toss               ; tirage au sort : engagement et cotes
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
    lda ct_side                 ; cotes tires au sort
    sta team_dir
    eor #1
    sta team_dir+2
    stz score
    stz score+2
    stz so_active
    ldx #0
:   stz st_shots,x
    stz st_poss,x
    stz st_fouls,x
    stz st_saves,x
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
    ldx #0
:   stz p_pen,x
    stz p_pts,x
    inx
    inx
    cpx #NUM_PLAYERS * 2
    bne :-
    ; equipe qui engage : tirage au sort (coin_toss)
    lda ct_win
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
    lda #CROWD_MATCH
    jsr crowd_level

match_loop:
    jsr wait_frame
    jsr match_frame
    bcc match_loop
    lda pz_restart              ; pause : rejouer le match
    beq :+
    stz pz_restart
    jmp start_match
:   lda comp_active
    beq :+
    jmp comp_after_match
:   jmp title_screen

; -----------------------------------------------------------------------------
;  new_kickoff : place les joueurs, etat KICKOFF
; -----------------------------------------------------------------------------
new_kickoff:
    .a16
    stz rp_cnt                  ; ralenti : rien avant l'engagement
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
    bcc :+
    rts                         ; quitter
:   stz joy_new                 ; reprise : le bouton de sortie n'est pas une action
    stz joy_new+2
@nopause:
    lda m_timer
    sec
    sbc rc+RC_TDEC
    bpl :+
    lda #0
:   sta m_timer

.if DEBUG
    ; test de stress : SELECT maintenu = tous les joueurs alignes sur la ligne du ballon
    lda joy_cur
    and #JOY_SELECT
    beq :+
    jsr dbg_regroup
:
.endif
    lda m_state
    asl a
    tax
    jsr (.loword(state_tab),x)
    bcs @quit

    jsr camera_update
    jsr crowd_update
    jsr hud_update
.if DEBUG = 2
    ldx #10 * 2
    jsr prof_mark
.endif
    jsr build_sprites
.if DEBUG = 2
    ldx #11 * 2
    jsr prof_mark
.endif
    clc
@quit:
    rts

state_tab:
    .word .loword(st_kickoff), .loword(st_play), .loword(st_goal)
    .word .loword(st_half), .loword(st_end), .loword(st_foul)
    .word .loword(st_shoot), .loword(st_replay)

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
.macro PROF n
.if DEBUG = 2
    ldx #n * 2
    jsr prof_mark
.endif
.endmacro

st_play:
    .a16
    jsr rp_record               ; positions pour le ralenti des buts
    PROF 0
    ; chronometre arrete pendant un engagement (coup d'envoi, coup franc) : il repart
    ; quand la passe / le tir d'engagement est joue
    lda ko_active
    bne :+
    jsr clock_update
    bcs @end
:
    jsr compute_nearest
    PROF 1
    jsr human_input
    PROF 2
    jsr ai_update
    PROF 3
    jsr ko_update
    PROF 4
    jsr players_update
    PROF 5
    jsr ko_push
    PROF 6
    jsr ball_update
    PROF 7
    jsr rules_update
    PROF 8
    jsr zone_clock
    PROF 9
    bcs @zr                     ; 4 s dans une raquette : remise en jeu au centre
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
@zr:
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
    jsr rp_start                ; ralenti du but
    bcs @w
    jmp goal_resume
@w: clc
    rts

; goal_resume : apres le but (et son ralenti) : engagement, ou fin du match (but en or)
goal_resume:
    .a16
    lda m_half
    cmp #3
    beq @golden
    jsr new_kickoff
    clc
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
    jne @w
    ; publicites de la mi-temps (pas avant la prolongation)
    lda m_half
    cmp #1
    bne @noad
    lda #0
    jsr result_screen           ; score de la mi-temps (rend l'ecran eteint)
    lda #1
    jsr ad_show
    jsr ensure_stadium_bg
    jsr layers_match
    jsr bg2_crowd_map
    jsr bg3_clear
    lda #1
    sta ad_flag
    bra :+
@noad:
    stz ad_flag
    lda m_half
    cmp #2
    bne :+
    ; prolongation : nouveau tirage au sort, un joueur de moins par equipe
    lda #1
    jsr coin_toss
    jsr screen_off
    jsr ensure_stadium_bg
    jsr layers_match
    jsr bg2_crowd_map
    jsr bg3_clear
    jsr ot_remove
    lda #1
    sta ad_flag
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
    lda m_half
    cmp #3
    bne :+
    lda ct_win                  ; prolongation : le gagnant du tirage engage
    sta kick_team
:
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

; ot_remove : prolongation a 5 contre 5 (le joueur 1 de chaque equipe sort jusqu'a la fin)
ot_remove:
    .a16
    .i16
    ldx #1 * 2
    jsr @one
    ldx #(TEAM_SIZE + 1) * 2
@one:
    stx cp
    lda #$7FFF
    sta p_pen,x
    lda #PS_OUT
    sta p_state,x
    ; la manette qui le controlait passe a un autre joueur
    ldy #0
@pad:
    lda ctrl,y
    and #$00FF
    asl a
    cmp cp
    bne @np
    ldx cp
    lda p_team,x
    jsr first_active
    jsr set_ctrl
@np:
    iny
    iny
    cpy #4
    bne @pad
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
    lda #1
    jsr result_screen           ; statistiques de fin de match
    sec
    rts
@w: clc
    rts

; --- faute : courte interruption puis remise en jeu
st_foul:
    .a16
    ; arret de jeu apres une faute : chronometre arrete (il repart apres le coup franc)
    jsr ball_update
    lda m_timer
    bne @w
    lda #MS_PLAY
    sta m_state
    lda ko_active
    beq @w
    lda ko_mode
    beq @w
    lda #SFX_WHISTLE            ; reprise : coup franc
    jsr sfx_play
    ldy #.loword(str_fk)
    jsr show_msg
    lda #(T_MSG / 3)
    sta msg_time
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
    ; exclusions temporaires (secondes de jeu, un minuteur par joueur)
    ldx #0
@pen:
    lda p_pen,x
    beq :+
    dec a
    sta p_pen,x
    bne :+
    phx
    jsr return_player
    plx
:   inx
    inx
    cpx #NUM_PLAYERS * 2
    bne @pen
    dec m_sec
    beq @po
    lda m_sec                   ; 5 dernieres secondes : bip
    cmp #6
    bcs @ok
    lda #SFX_MENU
    jsr sfx_play
    bra @ok
@po:
    jsr period_over
    sec
    rts
@ok:
    clc
    rts

period_over:
    .a16
    lda #SFX_BUZZLONG
    jsr sfx_play
    lda #MUS_STOP
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
    sta gs_team
    ; but contre son camp : toujours 1 point
    lda b_last
    cmp #NO_OWNER
    beq @gs_own
    asl a
    tax
    lda p_team,x
    cmp t0
    beq @gs_own
    inc dbg_og
    lda p_role,x
    bne :+
    inc dbg_gkog
:   lda #1
    sta b_points
@gs_own:
    lda t0
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
    lda #SFX_HORN               ; corne de but
    jsr sfx_play
    lda #24                     ; l'ecran tremble
    sta shake_t
    lda b_x                     ; gerbe d'etincelles a l'anneau
    ASR_A 4
    sta spk_x
    lda b_y
    sec
    sbc b_z
    ASR_A 4
    sta spk_y
    lda #SPK_T
    sta spk_t
    ldy #.loword(str_goal1)
    lda b_points
    cmp #2
    bne :+
    ldy #.loword(str_goal2)
:   ; marqueur : dernier joueur de l'equipe qui marque a avoir touche le ballon
    ldx #$FF
    lda b_last
    cmp #NO_OWNER
    beq :+
    asl a
    tax
    lda p_team,x
    cmp gs_team
    php
    txa
    lsr a
    tax
    plp
    beq :+
    ldx #$FF
:   cpx #$FF                    ; points du marqueur (etoile du match)
    beq :+
    phx
    txa
    asl a
    tax
    lda p_pts,x
    clc
    adc b_points
    sta p_pts,x
    plx
:   lda gs_team
    jsr show_tmsg
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
;  pause_menu : RESUME / TEAM SETUP / QUIT MATCH. Carry = 1 pour quitter.
; -----------------------------------------------------------------------------
PAUSE_ITEMS = 4
PZ_ROW      = 8                 ; premiere ligne du cadre de pause
PZ_ROWS     = 8

PAUSE_SAVE  = $7E3000           ; lignes BG3 sauvegardees (WRAM haute)
CROWD_MATCH = $0C               ; volume de la foule en match

pause_menu:
    .a16
    .i16
    stz pause_sel
    lda #0                      ; pause : la foule se tait
    jsr crowd_level
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
    lda bg3_map + PZ_ROW*64,x
    sta f:PAUSE_SAVE,x
    inx
    inx
    cpx #(PZ_ROWS*64)
    bne @sv
@draw:
    jsr pz_frame
    ; titre en cyan et filet dore
    lda #TXT_ATTR + TXT_PANEL + $1400
    sta t0
    ldx #TPOS(13, PZ_ROW + 1)
    ldy #.loword(str_pause)
    jsr print
    ldx #TPOS(10, PZ_ROW + 2)
    ldy #12
    lda #('_' - 32) + TXT_ATTR + TXT_PANEL + $1400
    jsr fill_tiles
    ; options : la ligne choisie en orange, encadree de chevrons
    stz t5
@it:
    lda t5
    asl a
    tay
    lda pause_items,y
    tay
    lda #TXT_ATTR + TXT_PANEL
    ldx t5
    cpx pause_sel
    bne :+
    lda #TXT_ATTR + TXT_PANEL + $1000
:   sta t0
    lda t5
    clc
    adc #PZ_ROW + 3
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc #12 * 2
    tax
    jsr print
    inc t5
    lda t5
    cmp #PAUSE_ITEMS
    bne @it
    ; chevrons de la ligne choisie
    lda pause_sel
    clc
    adc #PZ_ROW + 3
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    tax
    lda #('>' - 32 + TXT_ATTR + TXT_PANEL + $1000)
    sta bg3_map + 10*2,x
    lda #('<' - 32 + TXT_ATTR + TXT_PANEL + $1000)
    sta bg3_map + 22*2,x
    lda #1
    sta bg3_dirty
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
    bit #(JOY_START | JOY_B)
    bne @resume
    and #JOY_A
    beq @wait
    lda pause_sel
    beq @resume
    cmp #1
    beq @team
    cmp #2
    bne @quit
    ; rejouer le match : confirmation
    lda #.loword(str_restart_q)
    sta cf_title
    stz cf_menu
    jsr cf_run
    bcs :+
    jmp @draw
:   lda #1
    sta pz_restart
    jsr pause_restore
    sec
    rts
@quit:
    ; quitter : confirmation
    jsr pause_confirm
    bcs :+
    jmp @draw
:   jsr pause_restore
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
@resume:
    jsr pause_restore
    lda #CROWD_MATCH
    jsr crowd_level
    clc
    rts

; pause_confirm : "QUIT MATCH?" NO / YES dans le cadre de pause -> C = 1 si YES
pause_confirm:
    .a16
    .i16
    lda #.loword(str_quit_q)
    sta cf_title
    stz cf_menu
cf_run:
    stz pc_sel                  ; NO par defaut
@draw:
    ; interieur du cadre (lignes PZ_ROW+1 .. PZ_ROW+5) efface
    lda #PZ_ROW + 1
@cl:
    pha
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc #9 * 2
    tax
    ldy #14
    lda #TXT_ATTR + TXT_PANEL
    jsr fill_tiles
    pla
    inc a
    cmp #PZ_ROW + 7
    bne @cl
    lda #TXT_ATTR + TXT_PANEL + $1400
    sta t0
    ldx #TPOS(10, PZ_ROW + 1)
    ldy cf_title
    jsr print
    ldx #TPOS(10, PZ_ROW + 2)
    ldy #12
    lda #('_' - 32) + TXT_ATTR + TXT_PANEL + $1400
    jsr fill_tiles
    stz t5
@it:
    lda t5
    asl a
    tay
    lda confirm_items,y
    tay
    lda #TXT_ATTR + TXT_PANEL
    ldx t5
    cpx pc_sel
    bne :+
    lda #TXT_ATTR + TXT_PANEL + $1000
:   sta t0
    lda t5
    clc
    adc #PZ_ROW + 3
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc #14 * 2
    tax
    jsr print
    inc t5
    lda t5
    cmp #2
    bne @it
    lda pc_sel
    clc
    adc #PZ_ROW + 3
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    tax
    lda #('>' - 32 + TXT_ATTR + TXT_PANEL + $1000)
    sta bg3_map + 12*2,x
    lda #('<' - 32 + TXT_ATTR + TXT_PANEL + $1000)
    sta bg3_map + 20*2,x
    lda #1
    sta bg3_dirty
@wait:
    lda cf_menu
    beq :+
    jsr ui_wait
    bra :++
:   jsr build_sprites
    jsr wait_frame
    lda joy_new
    ora joy_new+2
    sta t7
:   lda t7
    beq @wait
    lda pc_sel
    ldy #2
    jsr ui_updown
    cmp pc_sel
    beq :+
    sta pc_sel
    jmp @draw
:   lda t7
    bit #JOY_B
    bne @no
    and #(JOY_A | JOY_START)
    beq @wait
    lda pc_sel
    beq @no
    sec
    rts
@no:
    clc
    rts


; pz_frame : cadre neon (tiles du bandeau, palette 5), interieur vide
pz_frame:
    .a16
    .i16
    ; cadre neon (tiles du bandeau, palette 5) : lignes PZ_ROW .. PZ_ROW+7, colonnes 8..23
    lda #TXT_ATTR + $1400
    sta bn_attr
    lda #16
    sta bn_w
    ldx #TPOS(8, PZ_ROW)
    lda #HT_BN_TL
    ldy #HT_BN_T
    jsr bn_row
    ldx #TPOS(8, PZ_ROW + 7)
    lda #HT_BN_BL
    ldy #HT_BN_B
    jsr bn_row
    lda #PZ_ROW + 1
@pf_side:
    pha
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc #8 * 2
    tax
    lda #HT_BN_L + TXT_ATTR + $1400
    sta bg3_map,x
    lda #HT_BN_R + TXT_ATTR + $1400
    sta bg3_map + 15*2,x
    inx
    inx
    ldy #14
    lda #TXT_ATTR + TXT_PANEL
    jsr fill_tiles
    pla
    inc a
    cmp #PZ_ROW + 7
    bne @pf_side
    rts

; menu_confirm : dans un menu, "MAIN MENU?" NO / YES -> C = 1 si YES (lignes BG3 restaurees si NO)
menu_confirm:
    .a16
    .i16
    lda #.loword(str_menu_q)
; menu_confirm_t : A = question -> C = 1 si YES
menu_confirm_t:
    sta cf_title
    jsr ui_click
    ldx #0
@sv:
    lda bg3_map + PZ_ROW*64,x
    sta f:PAUSE_SAVE,x
    inx
    inx
    cpx #(PZ_ROWS*64)
    bne @sv
    jsr pz_frame
    lda #1
    sta cf_menu
    jsr cf_run
    bcs :+
    jsr pause_restore
    clc
:   rts

; pause_restore_clear : apres un ecran plein, la zone sauvegardee devient vide
pause_restore_clear:
    .a16
    ldx #0
    lda #0
@l: sta f:PAUSE_SAVE,x
    inx
    inx
    cpx #(PZ_ROWS*64)
    bne @l
    rts

pause_restore:
    .a16
    ldx #0
@l: lda f:PAUSE_SAVE,x
    sta bg3_map + PZ_ROW*64,x
    inx
    inx
    cpx #(PZ_ROWS*64)
    bne @l
    lda #1
    sta bg3_dirty
    rts

.segment "RODATA"
str_ready:  .byte "READY", 0
str_go:     .byte "GO! PASS!", 0
str_fk:     .byte "FREE KICK!", 0
str_close:  .byte "SO CLOSE!", 0
str_save:   .byte "SAVE!", 0
str_half:   .byte "HALF TIME", 0
str_overtime: .byte "OVERTIME - GOLDEN SCORE", 0
str_goal1:  .byte "SCORE! +1", 0
str_goal2:  .byte "SCORE! +2", 0
str_pause:  .byte "PAUSE", 0
str_quit_q:  .byte "QUIT MATCH?", 0
str_restart_q: .byte "RESTART?", 0
str_restart: .byte "RESTART", 0
str_menu_q:  .byte "MAIN MENU?", 0
str_over_q:  .byte "REPLACE SAVE?", 0
str_no:      .byte "NO", 0
str_yes:     .byte "YES", 0
confirm_items: .word .loword(str_no), .loword(str_yes)
pause_items: .word .loword(str_resume), .loword(str_teamset), .loword(str_restart), .loword(str_quit)
str_resume: .byte "RESUME", 0
str_quit:   .byte "QUIT MATCH", 0
str_teamset: .byte "TEAM SETUP", 0
.segment "CODE"

; -----------------------------------------------------------------------------
;  Coup d'envoi : le porteur (ko_p1) doit passer a son partenaire (ko_p2). Le match
;  reprend quand le partenaire a le ballon. D'ici la, personne d'autre n'entre dans
;  le rond central et le porteur ne bouge pas.
; -----------------------------------------------------------------------------
ko_update:
    .a16
    .i16
    lda ko_active
    bne :+
    rts
:   lda ko_mode
    beq :+
    jmp fk_update
:   lda ko_lim
    sec
    sbc rc+RC_TDEC
    bpl :+
    lda #0
:   sta ko_lim
    beq @end
    ; le partenaire attend le ballon sur place
    lda ko_p2
    asl a
    tax
    stz p_want,x
    stz p_act,x
    lda b_owner
    cmp ko_p1
    beq @carrier
    cmp #NO_OWNER
    beq @rts                    ; ballon en l'air
@end:
    stz ko_active               ; recu (ou intercepte) : le match reprend
@rts:
    rts
@carrier:
    asl a
    tax
    stx cp
    stz p_want,x
    stz b_carry                 ; pas de limite de port avant la passe
    lda p_act,x
    and #.loword(~JOY_R)
    stz p_act,x
    ldy p_human,x
    beq @ai
    cmp #0
    bne @pass                   ; humain : n'importe quel bouton d'action
    rts
@ai:
    lda ko_t
    sec
    sbc rc+RC_TDEC
    bpl :+
    lda #0
:   sta ko_t
    beq @pass
    rts
@pass:
    ldx cp
    lda ko_p2
    asl a
    tay
    lda rc+RC_PASS_H
    sta l_spd
    lda #9
    sta t0
    lda #12
    sta l_zt
    stz l_foot
    jsr pass_to_player
    ldx cp
    lda #SPR_THROW
    sta p_spr,x
    lda #SFX_PASS
    jmp sfx_play

; ko_push : pendant le coup d'envoi, les autres joueurs restent hors du rond central
ko_push:
    .a16
    .i16
    lda ko_active
    bne :+
    rts
:   lda ko_mode
    beq :+
    jmp fk_push
:   lda #FIELD_CX
    sta kz_cx
    lda #FIELD_CY
    sta kz_cy
    lda #44
    sta kz_r
    ldx #0
@l: stx cp
    txa
    lsr a
    cmp ko_p1
    beq @n
    cmp ko_p2
    beq @n
    lda p_state,x
    cmp #PS_OUT
    beq @n
    jsr push_out
@n: ldx cp
    inx
    inx
    cpx #NUM_PLAYERS * 2
    bne @l
    rts

; -----------------------------------------------------------------------------
;  Coup franc (apres une faute) : le joueur qui recoit le ballon ne bouge pas,
;  les adversaires restent a FK_DIST pixels jusqu'a sa passe ou son tir.
; -----------------------------------------------------------------------------
FK_DIST = 40

; fk_start : A = joueur qui tire le coup franc
fk_start:
    .a16
    .i16
    sta ko_p1
    lda #$FF
    sta ko_p2
    lda #1
    sta ko_active
    sta ko_mode
    lda #TU_SEC * 6
    sta ko_lim
    rts

fk_update:
    .a16
    .i16
    lda ko_lim
    sec
    sbc rc+RC_TDEC
    bpl :+
    lda #0
:   sta ko_lim
    beq @end
    lda b_owner
    cmp ko_p1
    bne @end                    ; ballon joue : fin du coup franc
    asl a
    tax
    stz p_want,x                ; le tireur ne bouge pas (passe / tir autorises)
    rts
@end:
    stz ko_active
    rts

fk_push:
    .a16
    .i16
    lda ko_p1
    asl a
    tax
    lda p_x,x
    ASR_A 4
    sta fk_x
    lda p_y,x
    ASR_A 4
    sta fk_y
    lda p_team,x
    sta ko_t                    ; equipe du tireur
    ldx #0
@l: stx cp
    lda p_team,x
    cmp ko_t
    beq @n
    lda p_state,x
    cmp #PS_OUT
    beq @n
    lda fk_x                    ; (keep_out_zone change kz_*)
    sta kz_cx
    lda fk_y
    sta kz_cy
    lda #FK_DIST
    sta kz_r
    jsr push_out
    ldx cp
    jsr keep_out_zone           ; repousse hors du cercle sans entrer dans une raquette
@n: ldx cp
    inx
    inx
    cpx #NUM_PLAYERS * 2
    bne @l
    rts


; -----------------------------------------------------------------------------
;  zone_clock : le ballon ne peut pas rester 4 s dans une raquette (porte par le
;  gardien ou libre). Sinon l'adversaire de l'equipe qui defend cette raquette
;  recoit le ballon au centre (coup d'envoi). Carry = 1 si la regle s'applique.
; -----------------------------------------------------------------------------
zone_clock:
    .a16
    .i16
    lda m_state
    cmp #MS_PLAY
    jne @out
    lda ko_active
    jne @out
    ; rejet rapide : ballon loin des deux fonds
    lda b_x
    cmp #(FIELD_L + ZONE_R) * FP
    bcc :+
    cmp #(FIELD_R - ZONE_R) * FP
    bcc @out
:   ; raquette gauche ?
    lda b_x
    ASR_A 4
    sec
    sbc #FIELD_L
    sta t0
    lda b_y
    ASR_A 4
    sec
    sbc #FIELD_CY
    sta t1
    jsr dist_approx
    ldy #0
    cmp #ZONE_R
    bcc @in
    lda b_x
    ASR_A 4
    sec
    sbc #FIELD_R
    sta t0
    lda b_y
    ASR_A 4
    sec
    sbc #FIELD_CY
    sta t1
    jsr dist_approx
    ldy #1
    cmp #ZONE_R
    bcc @in
@out:
    stz zc_t
    clc
    rts
@in:
    cpy zc_zone
    beq :+
    sty zc_zone
    stz zc_t
:   lda zc_t
    clc
    adc rc+RC_TDEC
    sta zc_t
    cmp #4 * TU_SEC
    bcs :+
    clc
    rts
:   stz zc_t
    ; equipe qui defend cette raquette : gauche = celle qui attaque vers la droite
    lda team_dir
    beq :+
    lda #1                      ; equipe 0 attaque vers la gauche : elle defend a droite
:   ldy zc_zone
    beq :+
    eor #1
:   sta zc_team
    eor #1
    sta kick_team
    lda #SFX_BUZZER
    jsr sfx_play
    jsr new_kickoff
    lda zc_team
    ldx #$FF
    ldy #.loword(str_zone4)
    jsr show_tmsg
    sec
    rts

.segment "RODATA"
str_zone4:  .byte "4 SEC IN THE ZONE!", 0
.segment "CODE"

.if DEBUG
; prof_mark : X = case -> prof_max,x = max(ligne video courante)
prof_mark:
    .a16
    .i16
    sep #$20
    .a8
    lda SLHV
    lda OPVCT_
    xba
    lda OPVCT_
    and #$01
    xba
    rep #$20
    .a16
    pha
    sec
    sbc prof_last               ; duree depuis la marque precedente (lignes)
    bpl :+
    clc
    adc #262
:   cmp prof_max,x
    bcc :+
    sta prof_max,x
:   pla
    sta prof_last
    rts
.endif

.if DEBUG
; dbg_regroup : les 12 joueurs sur la meme ligne que le ballon, espaces de 12 px
; (surcharge maximale des lignes de sprites, collisions, separation)
dbg_regroup:
    .a16
    .i16
    lda b_x
    sec
    sbc #66 * FP
    sta t0
    ldx #0
@l: lda t0
    sta p_x,x
    clc
    adc #12 * FP
    sta t0
    lda b_y
    sta p_y,x
    inx
    inx
    cpx #NUM_PLAYERS * 2
    bne @l
    rts
.endif
