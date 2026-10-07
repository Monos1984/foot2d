.segment "CODE2"
; =============================================================================
;  comp.asm - competitions : CHAMPIONSHIP (ligue), CUP (4/8/16), CUSTOM
;  Affectation des equipes CPU / P1 / P2, calendrier, simulation des matchs
;  CPU contre CPU, classement, tableau, sauvegarde / reprise en SRAM.
; =============================================================================

MODE_PRESET  = 3            ; start_match : manettes deja reglees
SRAM_COMP    = $A06100      ; 3 emplacements de $400 octets
COMP_SIZE    = comp_data_end - comp_data
COMP_VERSION = 4            ; 4 : + graine (c_seed) ; la version 3 reste lisible
NUM_CITEMS   = 21           ; 16 equipes + TYPE, LENGTH, DIFFICULTY, STADIUM, START

.segment "RODATA"
.include "data/gen/sched.inc"
.segment "CODE2"

; -----------------------------------------------------------------------------
;  comp_menu : A = emplacement (0 CHAMPIONSHIP, 1 CUP, 2 CUSTOM)
;  Reprise si une competition est sauvegardee, sinon nouvelle competition.
; -----------------------------------------------------------------------------
comp_menu:
    .a16
    .i16
    sta c_slot_sel
    jsr comp_load
    lda #0
    rol a
    sta cm_has                  ; 1 : competition sauvegardee (CONTINUE)
    ; [CONTINUE] / NEW / PASSWORD / BACK
    stz ui_sel
    jsr safe_screen_off
    jsr bg3_clear
    jsr oam_clear
    jsr ui_fill
    jsr comp_title
    lda #UI_ATTR
    sta t0
    ldx #TPOS(1, 25)
    ldy #.loword(str_cm_help)
    jsr print
    jsr screen_on
@draw:
    lda #UI_ATTR
    sta t0
    ldx #TPOS(4, 8)
    lda cm_has
    beq :+
    ldy #.loword(str_c_cont)
    jsr print
    ldx #TPOS(4, 9)
:   phx
    ldy #.loword(str_c_new)
    jsr print
    pla
    clc
    adc #64
    pha
    tax
    ldy #.loword(str_c_pw)
    jsr print
    pla
    clc
    adc #64
    tax
    ldy #.loword(str_back)
    jsr print
    lda #.loword(cm_rows)
    sta t3
    lda cm_has
    clc
    adc #3
    sta t4
    lda ui_sel
    jsr ui_cursor
@loop:
    jsr ui_wait
    lda t7
    beq @loop
    and #JOY_B
    bne @back
    lda cm_has
    clc
    adc #3
    tay
    lda ui_sel
    jsr ui_updown
    cmp ui_sel
    beq :+
    sta ui_sel
    bra @draw
:   lda t7
    and #(JOY_A | JOY_START)
    beq @loop
    lda ui_sel
    clc
    adc #1
    sec
    sbc cm_has                  ; 0 CONTINUE, 1 NEW, 2 PASSWORD, 3 BACK
    beq @cont
    cmp #1
    beq @new
    cmp #2
    beq @pw
@back:
    jsr menu_confirm
    jcc @loop
    jmp title_screen
@cont:
    jmp comp_hub
@new:
    jmp comp_new
@pw:
    jsr pw_entry
    bcc :+
    lda cm_has                  ; une competition sauvegardee serait remplacee
    beq @pws
    lda #.loword(str_over_q)
    jsr menu_confirm_t
    bcc :+
@pws:
    jsr comp_save
    jmp comp_hub
:   lda c_slot_sel
    jmp comp_menu

.segment "RODATA"
cm_rows: .byte 8, 9, 10, 11
.segment "CODE2"

; comp_title : titre de la competition (ligne 1)
comp_title:
    .a16
    .i16
    lda #UI_HI
    sta t0
    lda c_slot_sel
    asl a
    tay
    lda comp_names,y
    tay
    ldx #TPOS(4, 1)
    jmp print

; -----------------------------------------------------------------------------
;  comp_new : reglages par defaut puis ecran de configuration
; -----------------------------------------------------------------------------
comp_new:
    .a16
    .i16
    lda c_slot_sel
    sta c_slot
    ; 8 premieres equipes en CPU, l'equipe favorite du joueur en P1
    ldx #0
@t: lda #0
    cpx #8
    bcs :+
    lda #1
:   sep #$20
    .a8
    sta c_ctrl,x
    rep #$20
    .a16
    inx
    cpx #MAX_TEAMS
    bne @t
    ldx team_id
    sep #$20
    .a8
    lda #2
    sta c_ctrl,x
    rep #$20
    .a16
    cpx #8
    bcc :+
    sep #$20
    .a8
    stz c_ctrl+7                ; garder 8 equipes
    rep #$20
    .a16
:   lda c_slot
    cmp #1
    beq :+
    lda #0
:   sta c_type
    lda menu_len
    sta c_len
    lda difficulty
    sta c_diff
    lda #6
    sta c_stad
    stz c_watch
    jmp comp_setup

; -----------------------------------------------------------------------------
;  comp_setup : participants (OUT / CPU / P1 / P2) et reglages
; -----------------------------------------------------------------------------
comp_setup:
    .a16
    .i16
    stz ui_sel
    stz cs_top
    jsr cs_build
    jsr safe_screen_off
    jsr bg3_clear
    jsr ui_fill
    jsr comp_title
    jsr cs_draw
    jsr screen_on
cs_loop:
    jsr ui_wait
    lda t7
    beq cs_loop
    and #JOY_B
    beq :+
    jsr menu_confirm
    bcc cs_loop
    jmp title_screen
:   lda vt_n
    clc
    adc #5
    tay
    lda ui_sel
    jsr ui_updown
    cmp ui_sel
    beq :+
    sta ui_sel
    jsr cs_scroll
    jsr cs_draw
    bra cs_loop
:   lda ui_sel
    cmp vt_n
    bcs @opt
    ; statut d'une equipe
    tax
    lda vt_list,x
    and #$00FF
    tax
    lda c_ctrl,x
    and #$00FF
    ldy #4
    jsr ui_lr
    sep #$20
    .a8
    sta c_ctrl,x
    rep #$20
    .a16
    jsr cs_draw
    bra cs_loop
@opt:
    sec
    sbc vt_n
    asl a
    tax
    jmp (.loword(cs_opt_tab),x)

.segment "RODATA"
cs_opt_tab:
    .word .loword(cs_type), .loword(cs_len), .loword(cs_diff), .loword(cs_stad), .loword(cs_start)
.segment "CODE2"

cs_type:
    lda c_slot
    cmp #2
    bne cs_ret                  ; seule la competition personnalisee change de type
    lda c_type
    ldy #2
    jsr ui_lr
    sta c_type
    bra cs_redraw
cs_len:
    lda c_len
    ldy #4
    jsr ui_lr
    sta c_len
    bra cs_redraw
cs_diff:
    lda c_diff
    ldy #4
    jsr ui_lr
    sta c_diff
    bra cs_redraw
cs_stad:
    lda c_stad
    ldy #NUM_STADIUMS + 1
    jsr ui_lr
    sta c_stad
cs_redraw:
    jsr cs_draw
cs_ret:
    jmp cs_loop
cs_start:
    lda t7
    and #(JOY_A | JOY_START)
    beq cs_ret
    jsr cs_count
    sta c_n
    ldy #.loword(str_c_badl)
    lda c_type
    bne @cup
    lda c_n
    cmp #3
    bcc @bad
    bra @ok
@cup:
    ldy #.loword(str_c_badc)
    lda c_n
    cmp #4
    beq @ok
    cmp #8
    beq @ok
    cmp #16
    beq @ok
@bad:
    lda #UI_HI
    sta t0
    ldx #TPOS(2, 27)
    jsr print
    jmp cs_loop
@ok:
    jsr comp_create
    jsr comp_save
    jmp comp_hub

; cs_count : A = nombre d'equipes engagees
cs_count:
    .a16
    .i16
    ldx #0
    ldy #0
@l: lda c_ctrl,x
    and #$00FF
    beq :+
    txa
    jsr team_valid
    bcc :+
    iny
:   inx
    cpx #MAX_TEAMS
    bne @l
    tya
    rts

; cs_build : liste des equipes existantes (vt_list)
cs_build:
    .a16
    .i16
    stz vt_n
    lda #0
@l: jsr team_valid
    bcc :+
    ldx vt_n
    sep #$20
    .a8
    sta vt_list,x
    rep #$20
    .a16
    inc vt_n
:   inc a
    cmp #MAX_TEAMS
    bne @l
    rts

; cs_scroll : garde la ligne selectionnee visible (16 lignes d'equipes)
cs_scroll:
    .a16
    lda ui_sel
    cmp vt_n
    bcs @d
    cmp cs_top
    bcs :+
    sta cs_top
    rts
:   sec
    sbc cs_top
    cmp #16
    bcc @d
    lda ui_sel
    sec
    sbc #15
    sta cs_top
@d: rts

.segment "RODATA"
cs_opt_rows:
    .byte 20, 21, 22, 23, 25
.segment "CODE2"

cs_draw:
    .a16
    .i16
    ; efface la colonne du curseur
    ldx #TPOS(1, 3)
@cc:
    lda #UI_ATTR
    sta bg3_map,x
    txa
    clc
    adc #64
    tax
    cpx #TPOS(1, 26)
    bcc @cc
    stz t5
@team:
    lda t5
    clc
    adc cs_top
    sta near_tmp+4              ; index dans vt_list
    lda t5
    clc
    adc #3
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc #3 * 2
    sta near_tmp+2              ; position de ligne
    lda near_tmp+4
    cmp vt_n
    bcc :+
    ; ligne vide
    ldx near_tmp+2
    ldy #24
    lda #UI_ATTR
    jsr fill_tiles
    jmp @nt
:   tax
    lda vt_list,x
    and #$00FF
    sta t6
    tax
    lda c_ctrl,x
    and #$00FF
    sta near_tmp
    lda t6
    jsr team_rec_id
    lda #UI_ATTR
    ldx near_tmp
    cpx #2
    bcc :+
    lda #UI_A
:   sta t0
    lda #17
    sta t1
    ldx near_tmp+2
    jsr print_w
    ; statut
    lda near_tmp
    asl a
    tay
    lda ctrl_names,y
    tay
    lda #UI_HI
    sta t0
    lda #4
    sta t1
    lda near_tmp+2
    clc
    adc #20 * 2
    tax
    jsr print_w
    ; curseur
    lda near_tmp+4
    cmp ui_sel
    bne @nt
    lda near_tmp+2
    sec
    sbc #2 * 2
    tax
    lda #('>' - 32 + UI_HI)
    sta bg3_map,x
@nt:
    inc t5
    lda t5
    cmp #16
    jne @team
    ; reglages
    lda #UI_ATTR
    sta t0
    ldx #TPOS(3, 20)
    ldy #.loword(str_c_type)
    jsr print
    ldy #.loword(str_c_league)
    lda c_type
    beq :+
    ldy #.loword(str_c_cup)
:   lda #UI_HI
    sta t0
    lda #8
    sta t1
    ldx #TPOS(15, 20)
    jsr print_w
    lda #UI_ATTR
    sta t0
    ldx #TPOS(3, 21)
    ldy #.loword(str_o_len)
    jsr print
    lda #UI_HI
    sta t0
    lda c_len
    clc
    adc #2
    ldx #TPOS(21, 21)
    jsr print_digit
    lda #UI_ATTR
    sta t0
    ldx #TPOS(3, 22)
    ldy #.loword(str_difficulty)
    jsr print
    lda c_diff
    asl a
    tay
    lda diff_names,y
    tay
    lda #UI_HI
    sta t0
    lda #8
    sta t1
    ldx #TPOS(15, 22)
    jsr print_w
    lda #UI_ATTR
    sta t0
    ldx #TPOS(3, 23)
    ldy #.loword(str_stadium)
    jsr print
    lda c_stad
    jsr stad_name
    lda #UI_HI
    sta t0
    lda #17
    sta t1
    ldx #TPOS(12, 23)
    jsr print_w
    lda #UI_ATTR
    sta t0
    ldx #TPOS(3, 25)
    ldy #.loword(str_start)
    jsr print
    jsr cs_count
    ldx #TPOS(17, 25)
    jsr print_num2
    ldx #TPOS(20, 25)
    ldy #.loword(str_c_teams)
    jsr print
    ; ligne d'erreur effacee
    ldx #TPOS(0, 27)
    ldy #32
    lda #UI_ATTR
    jsr fill_tiles
    ; curseur sur les reglages
    lda ui_sel
    sec
    sbc vt_n
    bcc @nc
    tay
    lda cs_opt_rows,y
    and #$00FF
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc #1 * 2
    tax
    lda #('>' - 32 + UI_HI)
    sta bg3_map,x
@nc:
    lda #1
    sta bg3_dirty
    ; barre de selection
    lda ui_sel
    cmp vt_n
    bcs :+
    sec
    sbc cs_top
    clc
    adc #3
    jmp bg2_bar
:   sec
    sbc vt_n
    tay
    lda cs_opt_rows,y
    and #$00FF
    jmp bg2_bar

; stad_name : A = stade (6 = rotation) -> Y = nom
stad_name:
    .a16
    cmp #NUM_STADIUMS
    bcc :+
    ldy #.loword(str_c_rot)
    rts
:   asl a
    sta t6
    asl a
    clc
    adc t6
    asl a
    tay
    lda stadium_tab+10,y
    tay
    rts

; -----------------------------------------------------------------------------
;  comp_create : participants, calendrier ou tirage du tableau
; -----------------------------------------------------------------------------
comp_create:
    .a16
    .i16
    jsr rand
    and #$0FFF
    sta c_seed
; comp_build : participants, calendrier et tirage au sort (graine c_seed)
comp_build:
    .a16
    .i16
    ; participants dans l'ordre des equipes
    ldx #0
    ldy #0
@p: lda c_ctrl,x
    and #$00FF
    beq :+
    txa
    jsr team_valid
    bcc :+
    sep #$20
    .a8
    txa
    sta c_teams,y
    rep #$20
    .a16
    iny
:   inx
    cpx #MAX_TEAMS
    bne @p
    ; resultats vides
    ldx #0
    lda #$FFFF
@r: sta c_res,x
    inx
    inx
    cpx #240
    bne @r
    ldx #0
@b: sta c_br,x
    inx
    inx
    cpx #32
    bne @b
    ldx #0
@s: sta c_so,x
    inx
    inx
    cpx #16
    bne @s
    stz c_round
    stz c_match
    stz c_done
    lda #$FFFF
    sta c_champ
    lda c_n
    sta c_nn
    lda c_type
    bne @cup
    ; ligue : calendrier pair
    lda c_n
    and #1
    beq :+
    inc c_nn
:   rts
@cup:
    ; tirage au sort (melange de Fisher-Yates), reproductible a partir de la graine
    lda c_seed
    eor #$3C5A
    sta rng
    ldx #0
@i: sep #$20
    .a8
    txa
    sta c_br,x
    rep #$20
    .a16
    inx
    cpx c_n
    bne @i
    ldx c_n
@sh:
    dex
    beq @done
    stx t3
    jsr rand
    and #$00FF
    inc t3                      ; j dans [0, i]
    ldy t3
    jsr mulu8
    xba
    and #$00FF
    tay                         ; j
    ldx t3
    dex                         ; i
    sep #$20
    .a8
    lda c_br,x
    pha
    lda c_br,y
    sta c_br,x
    pla
    sta c_br,y
    rep #$20
    .a16
    bra @sh
@done:
    rts

; -----------------------------------------------------------------------------
;  structure du calendrier
; -----------------------------------------------------------------------------
; comp_rounds : A = nombre de journees / tours
comp_rounds:
    .a16
    lda c_type
    bne @cup
    lda c_nn
    dec a
    rts
@cup:
    lda c_n
    ldy #0
@l: lsr a
    beq @d
    iny
    bra @l
@d: tya
    rts

; comp_mpr : A = nombre de matchs de la journee c_round
comp_mpr:
    .a16
    lda c_type
    bne @cup
    lda c_nn
    lsr a
    rts
@cup:
    lda c_n
    ldx c_round
    inx
@l: lsr a
    dex
    bne @l
    rts

; comp_match_info : A = match j de la journee c_round
;   -> t0 = participant domicile, t1 = exterieur, t2 = index global, carry = 1 si exempt
comp_match_info:
    .a16
    .i16
    sta t3
    lda c_type
    bne @cup
    ; ligue : g = round * (nn/2) + j
    lda c_nn
    lsr a
    tay
    lda c_round
    jsr mulu8
    clc
    adc t3
    sta t2
    ldy c_nn
    lda rr_ptr,y
    sta t4
    lda t2
    asl a
    clc
    adc t4
    tax                         ; tables en banque $C0
    lda f:$C00000,x
    and #$00FF
    sta t0
    lda f:$C00001,x
    and #$00FF
    sta t1
    lda t0
    cmp c_n
    bcs @bye
    lda t1
    cmp c_n
    bcs @bye
    clc
    rts
@bye:
    sec
    rts
@cup:
    jsr cup_bases
    lda t5                      ; offset des matchs du tour
    clc
    adc t3
    sta t2
    lda t3
    asl a
    clc
    adc t6                      ; base du tableau du tour
    tax
    lda c_br,x
    and #$00FF
    sta t0
    lda c_br+1,x
    and #$00FF
    sta t1
    clc
    rts

; cup_bases : t5 = N - (N >> k), t6 = 2N - (2N >> k) pour k = c_round ; t7 = 2N - (2N >> (k+1))
cup_bases:
    .a16
    .i16
    lda c_n
    ldx c_round
    beq :++
:   lsr a
    dex
    bne :-
:   sta t4                      ; N >> k
    lda c_n
    sec
    sbc t4
    sta t5
    lda t5
    asl a
    sta t6                      ; 2N - 2(N>>k)
    lda t4
    lsr a
    sta t4                      ; N >> (k+1)
    lda c_n
    sec
    sbc t4
    asl a
    sta t7
    rts

; -----------------------------------------------------------------------------
;  comp_store : resultat du match global t2 (t0/t1 participants) :
;  a0 = score domicile, a1 = exterieur, so_win (0/1) en cas d'egalite (coupe)
; -----------------------------------------------------------------------------
comp_store:
    .a16
    .i16
    lda t2
    asl a
    tax
    sep #$20
    .a8
    lda cs_hs
    sta c_res,x
    lda cs_as
    sta c_res+1,x
    rep #$20
    .a16
    lda c_type
    bne :+
    rts
:   ; coupe : vainqueur
    lda cs_hs
    cmp cs_as
    beq @so
    bcs @home
    bra @away
@so:
    ldx t2
    sep #$20
    .a8
    lda cs_so
    sta c_so,x
    rep #$20
    .a16
    lda cs_so
    bne @away
@home:
    lda t0
    bra @win
@away:
    lda t1
@win:
    sta t3
    jsr comp_rounds
    dec a
    cmp c_round
    bne @next
    ; finale
    ldx t3
    lda c_teams,x
    and #$00FF
    sta c_champ
    rts
@next:
    lda c_match
    clc
    adc t7
    tax
    sep #$20
    .a8
    lda t3
    sta c_br,x
    rep #$20
    .a16
    rts

; -----------------------------------------------------------------------------
;  sim_match : t0 / t1 participants -> cs_hs, cs_as, cs_so (simulation rapide)
;  8 occasions par equipe, conversion selon l'ecart de niveau, 1 ou 2 points
; -----------------------------------------------------------------------------
sim_match:
    .a16
    .i16
    ldx t0
    lda c_teams,x
    and #$00FF
    jsr team_rec_id
    lda a:T_LEVEL,y
    and #$00FF
    sta sim_lh
    ldx t1
    lda c_teams,x
    and #$00FF
    jsr team_rec_id
    lda a:T_LEVEL,y
    and #$00FF
    sta sim_la
    ; seuil domicile = 9 + 2*(lh - la), exterieur = 8 + 2*(la - lh)
    lda sim_lh
    sec
    sbc sim_la
    asl a
    clc
    adc #9
    sta sim_th
    lda sim_la
    sec
    sbc sim_lh
    asl a
    clc
    adc #8
    sta sim_ta
    lda sim_th
    jsr sim_points
    sta cs_hs
    lda sim_ta
    jsr sim_points
    sta cs_as
    ; tirs au but : avantage au meilleur niveau
    jsr rand
    and #$000F
    clc
    adc sim_lh
    sec
    sbc sim_la
    ldy #0
    cmp #8
    bcs :+
    iny
:   sty cs_so
    rts

; sim_points : A = seuil (sur 32) -> A = points
sim_points:
    .a16
    sta sim_t
    stz sim_p
    lda #8
    sta sim_k
@l: jsr rand
    and #$001F
    cmp sim_t
    bpl @n
    inc sim_p
    jsr rand
    and #$0003
    bne @n
    inc sim_p
@n: dec sim_k
    bne @l
    lda sim_p
    rts

; -----------------------------------------------------------------------------
;  comp_hub : journee en cours, resultats, actions
; -----------------------------------------------------------------------------
comp_hub:
    .a16
    .i16
    lda #0
; comp_hub_at : A = ligne du curseur
comp_hub_at:
    sta ui_sel
    stz comp_active
    jsr safe_screen_off
    jsr bg3_clear
    jsr oam_clear
    jsr ui_fill
    jsr comp_title
    jsr hub_draw
    jsr screen_on
    lda title_music
    bne :+
    lda #1
    sta title_music
    lda #MUS_TITLE
    jsr sfx_play
    lda #0
    jsr crowd_level
:
@loop:
    jsr ui_wait
    lda t7
    beq @loop
    lda ui_sel
    ldy #4
    jsr ui_updown
    cmp ui_sel
    beq :+
    sta ui_sel
    jsr hub_cursor
    bra @loop
:   lda ui_sel
    cmp #2
    bne :+
    ; regarder les matchs CPU : oui / non
    lda c_watch
    ldy #2
    jsr ui_lr
    sta c_watch
    jsr hub_draw
    bra @loop
:   lda t7
    and #(JOY_A | JOY_START)
    beq @loop
    lda ui_sel
    beq @play
    cmp #1
    beq @table
    jsr menu_confirm
    jcc @loop
    jsr comp_save
    jmp title_screen
@table:
    lda c_type
    bne :+
    jsr table_screen
    lda #1
    jmp comp_hub_at
:   jsr bracket_screen
    lda #1
    jmp comp_hub_at
@play:
    lda c_done
    bne @loop
    jmp comp_play

.segment "RODATA"
hub_rows: .byte 23, 24, 25, 26
.segment "CODE2"

hub_cursor:
    .a16
    lda #.loword(hub_rows)
    sta t3
    lda #4
    sta t4
    lda ui_sel
    jmp ui_cursor

hub_draw:
    .a16
    .i16
    lda c_done
    beq @round
    ; champion
    lda #UI_HI
    sta t0
    ldx #TPOS(4, 4)
    ldy #.loword(str_c_champ)
    jsr print
    lda c_champ
    jsr team_rec_id
    lda #UI_A
    sta t0
    ldx #TPOS(4, 6)
    jsr print
    brl @menu
@round:
    lda #UI_ATTR
    sta t0
    ldy #.loword(str_c_round)
    lda c_type
    beq :+
    jsr cup_round_name          ; (ecrase X)
:   ldx #TPOS(4, 3)
    jsr print
    lda c_type
    bne :+
    lda c_round
    inc a
    ldx #TPOS(10, 3)
    jsr print_num2
    lda #('/' - 32 + UI_ATTR)
    sta bg3_map + TPOS(12, 3)
    jsr comp_rounds
    ldx #TPOS(13, 3)
    jsr print_num2
:   ; matchs de la journee
    stz hub_i
    stz near_tmp+4              ; ligne
@m: lda hub_i
    jsr comp_mpr
    cmp hub_i
    jeq @menu
    jcc @menu
    lda hub_i
    jsr comp_match_info
    jcs @next                   ; exempt
    lda near_tmp+4
    clc
    adc #5
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    sta near_tmp+2              ; position de ligne
    lda t0
    pha
    lda t1
    pha
    lda t2
    pha
    lda t0
    ldx #6
    jsr hub_team
    pla
    sta t2
    pla
    sta t1
    pla
    sta t0
    lda t1
    pha
    lda t2
    pha
    lda t1
    ldx #20
    jsr hub_team
    pla
    sta t2
    pla
    sta t1
    ; score
    lda t2
    asl a
    tax
    lda c_res,x
    and #$00FF
    cmp #$FF
    beq @vs
    sta t6
    lda c_res+1,x
    and #$00FF
    sta t7
    lda #UI_HI
    sta t0
    lda near_tmp+2
    clc
    adc #11 * 2
    tax
    lda t6
    jsr print_num2
    lda near_tmp+2
    clc
    adc #14 * 2
    tax
    lda #('-' - 32 + UI_HI)
    sta bg3_map,x
    lda near_tmp+2
    clc
    adc #16 * 2
    tax
    lda t7
    jsr print_num2
    bra @nl
@vs:
    lda #UI_ATTR
    sta t0
    lda near_tmp+2
    clc
    adc #13 * 2
    tax
    ldy #.loword(str_c_vs)
    jsr print
@nl:
    inc near_tmp+4
@next:
    inc hub_i
    brl @m
@menu:
    ; mot de passe de la competition (lignes 14-19)
    jsr pw_encode
    lda #UI_HI
    sta t0
    ldx #TPOS(4, 14)
    ldy #.loword(str_pw)
    jsr print
    stz pw_cur
    lda #15
    jsr pw_show
    lda #UI_ATTR
    sta t0
    ldx #TPOS(4, 23)
    ldy #.loword(str_c_play)
    lda c_done
    beq :+
    ldy #.loword(str_c_over)
:   jsr print
    ldx #TPOS(4, 24)
    ldy #.loword(str_c_table)
    lda c_type
    beq :+
    ldy #.loword(str_c_bracket)
:   jsr print
    ldx #TPOS(4, 25)
    ldy #.loword(str_c_watch)
    jsr print
    ldy #.loword(str_off)
    lda c_watch
    beq :+
    ldy #.loword(str_on)
:   lda #UI_HI
    sta t0
    ldx #TPOS(22, 25)
    jsr print
    lda #UI_ATTR
    sta t0
    ldx #TPOS(4, 26)
    ldy #.loword(str_c_save)
    jsr print
    jmp hub_cursor

; hub_team : A = participant, X = colonne, near_tmp+2 = ligne -> nom court (couleur si humain)
hub_team:
    .a16
    .i16
    stx t6
    tax
    lda c_teams,x
    and #$00FF
    sta t7
    tax
    lda c_ctrl,x
    and #$00FF
    ldx #UI_ATTR
    cmp #2
    bcc :+
    ldx #UI_A
:   stx t0
    lda t7
    jsr team_rec_id
    tya
    clc
    adc #T_SHORT
    tay
    lda t6
    asl a
    clc
    adc near_tmp+2
    tax
    jmp print

; cup_round_name : Y = nom du tour (selon le nombre de matchs restants)
cup_round_name:
    .a16
    jsr comp_mpr
    ldy #.loword(str_c_final)
    cmp #1
    beq @d
    ldy #.loword(str_c_semi)
    cmp #2
    beq @d
    ldy #.loword(str_c_quarter)
    cmp #4
    beq @d
    ldy #.loword(str_c_r16)
@d: rts

; -----------------------------------------------------------------------------
;  comp_play : joue la journee : simule les matchs CPU, lance le premier match
;  humain rencontre. Fin de journee : sauvegarde.
; -----------------------------------------------------------------------------
comp_play:
    .a16
    .i16
@next:
    jsr comp_mpr
    cmp c_match
    beq @round_end
    bcc @round_end
    lda c_match
    jsr comp_match_info
    bcs @skip
    ; deja joue ?
    lda t2
    asl a
    tax
    lda c_res,x
    and #$00FF
    cmp #$FF
    bne @skip
    ; humain implique ?
    ldx t0
    jsr part_ctrl
    sta t4
    ldx t1
    jsr part_ctrl
    ora t4
    cmp #2
    bcs @human
    lda c_watch
    bne @human
    lda t2
    jsr sim_seed
    jsr sim_match
    jsr comp_store
@skip:
    inc c_match
    bra @next
@human:
    jmp comp_prematch
@round_end:
    stz c_match
    inc c_round
    jsr comp_rounds
    cmp c_round
    beq @finish
    bcc @finish
    jsr comp_save
    jmp comp_hub
@finish:
    dec c_round
    lda #1
    sta c_done
    lda c_type
    bne :+
    jsr compute_table
    lda st_order
    and #$00FF
    tax
    lda c_teams,x
    and #$00FF
    sta c_champ
:   jsr comp_save
    jmp champion_screen

; part_ctrl : X = participant -> A = controle (1 CPU, 2 P1, 3 P2)
part_ctrl:
    .a16
    lda c_teams,x
    and #$00FF
    tax
    lda c_ctrl,x
    and #$00FF
    rts

; -----------------------------------------------------------------------------
;  comp_prematch : t0/t1/t2 = match humain. Ecran d'avant-match puis match.
; -----------------------------------------------------------------------------
comp_prematch:
    .a16
    .i16
    lda t0
    sta pm_h
    lda t1
    sta pm_a
    lda t2
    sta c_cur
    ldx pm_h
    lda c_teams,x
    and #$00FF
    sta team_id
    ldx pm_a
    lda c_teams,x
    and #$00FF
    sta team_id+2
    lda #0
    jsr team_defaults
    lda #1
    jsr team_defaults
    ; manettes
    lda #NO_OWNER
    sta pad_team
    sta pad_team+2
    ldx pm_h
    jsr part_ctrl
    sta t4
    ldx pm_a
    jsr part_ctrl
    sta t5
    ; une meme manette possede les deux equipes : elle choisit son camp
    stz pm_conf
    stz pm_side
    lda t4
    cmp t5
    bne :+
    cmp #2
    bcc :+
    sbc #2
    asl a
    sta pm_cpad                 ; 0 = manette 1, 2 = manette 2
    lda #1
    sta pm_conf
:
    lda t4
    cmp #2
    bne :+
    stz pad_team                ; P1 joue a domicile
:   lda t5
    cmp #2
    bne :+
    lda pad_team
    cmp #NO_OWNER
    bne :+
    lda #1
    sta pad_team                ; P1 a l'exterieur (si pas deja a domicile)
:   lda t4
    cmp #3
    bne :+
    stz pad_team+2
:   lda t5
    cmp #3
    bne :+
    lda pad_team+2
    cmp #NO_OWNER
    bne :+
    lda #1
    sta pad_team+2
:   ; stade
    lda c_stad
    cmp #NUM_STADIUMS
    bcc :+
    lda c_round
    clc
    adc c_cur
    ldx #NUM_STADIUMS
    jsr divu
    lda RDMPYL
:   sta stadium_id
    stz ui_sel
@redraw:
    jsr safe_screen_off
    jsr bg3_clear
    jsr ui_fill
    jsr comp_title
    lda #UI_A
    sta t0
    lda team_id
    jsr team_rec_id
    ldx #TPOS(6, 6)
    jsr print
    lda #UI_ATTR
    sta t0
    ldx #TPOS(14, 8)
    ldy #.loword(str_c_vs)
    jsr print
    lda #UI_HI
    sta t0
    lda team_id+2
    jsr team_rec_id
    ldx #TPOS(6, 10)
    jsr print
    lda stadium_id
    jsr stad_name
    lda #UI_HI
    sta t0
    ldx #TPOS(6, 13)
    jsr print
    lda #UI_ATTR
    sta t0
    ldx #TPOS(4, 15)
    ldy #.loword(str_c_playas)
    jsr print
    jsr pm_side_draw
    lda #UI_ATTR
    sta t0
    ldx #TPOS(4, 17)
    ldy #.loword(str_home_tac)
    jsr print
    ldx #TPOS(4, 18)
    ldy #.loword(str_away_tac)
    jsr print
    ldx #TPOS(4, 20)
    ldy #.loword(str_start)
    jsr print
    jsr pm_cursor
    jsr screen_on
@loop:
    jsr ui_wait
    lda t7
    beq @loop
    and #JOY_B
    beq :+
    jmp comp_hub
:   lda ui_sel
    ldy #4
    jsr ui_updown
    cmp ui_sel
    beq :+
    sta ui_sel
    jsr pm_cursor
    bra @loop
:   lda ui_sel
    bne :+
    lda pm_conf
    beq @loop
    lda pm_side
    ldy #2
    jsr ui_lr
    sta pm_side
    jsr pm_side_draw
    bra @loop
:   lda t7
    and #(JOY_A | JOY_START)
    beq @loop
    lda ui_sel
    cmp #3
    beq @go
    dec a
    sta ui_team
    stz ui_back
    lda ui_sel
    pha
    jsr team_screen
    pla
    sta ui_sel
    jmp @redraw
@go:
    lda pm_conf
    beq :+
    lda #NO_OWNER
    sta pad_team
    sta pad_team+2
    ldx pm_cpad
    lda pm_side
    sta pad_team,x
:   ; reglages du match de competition (sauvegarde des reglages d'exhibition)
    lda menu_len
    sta pm_len
    lda difficulty
    sta pm_diff
    lda draw_rule
    sta pm_rule
    lda c_len
    sta menu_len
    lda c_diff
    sta difficulty
    lda c_type
    sta draw_rule
    lda #1
    sta comp_active
    lda #MODE_PRESET
    sta game_mode
    jmp start_match

.segment "RODATA"
pm_rows: .byte 15, 17, 18, 20
.segment "CODE2"

; pm_side_draw : camp choisi (ou "-" si pas de conflit)
pm_side_draw:
    .a16
    ldy #.loword(str_c_tbd)
    lda pm_conf
    beq :+
    ldy #.loword(str_c_home)
    lda pm_side
    beq :+
    ldy #.loword(str_c_away)
:   lda #UI_HI
    sta t0
    lda #5
    sta t1
    ldx #TPOS(14, 15)
    jmp print_w

pm_cursor:
    .a16
    lda #.loword(pm_rows)
    sta t3
    lda #4
    sta t4
    lda ui_sel
    jmp ui_cursor

; -----------------------------------------------------------------------------
;  comp_after_match : retour d'un match de competition
;  (m_state = MS_END : match termine, sinon abandon : a rejouer)
; -----------------------------------------------------------------------------
comp_after_match:
    .a16
    .i16
    stz comp_active
    lda pm_len
    sta menu_len
    lda pm_diff
    sta difficulty
    lda pm_rule
    sta draw_rule
    lda #MODE_1P
    sta game_mode
    lda m_state
    cmp #MS_END
    bne @abort
    lda pm_h
    sta t0
    lda pm_a
    sta t1
    lda c_cur
    sta t2
    lda score
    cmp #99
    bcc :+
    lda #99
:   sta cs_hs
    lda score+2
    cmp #99
    bcc :+
    lda #99
:   sta cs_as
    ldy #0
    lda so_goals
    cmp so_goals+2
    bcs :+
    iny
:   sty cs_so
    jsr cup_bases               ; t7 pour comp_store (coupe)
    lda pm_h
    sta t0
    lda pm_a
    sta t1
    lda c_cur
    sta t2
    jsr comp_store
    inc c_match
    jsr comp_save
@abort:
    jmp comp_hub

; -----------------------------------------------------------------------------
;  compute_table : classement de la ligue -> st_order (participants tries)
; -----------------------------------------------------------------------------
compute_table:
    .a16
    .i16
    ldx #0
@z: stz st_pts,x
    stz st_w,x
    stz st_d,x
    stz st_l,x
    stz st_pf,x
    stz st_pa,x
    inx
    inx
    cpx #32
    bne @z
    ; tous les matchs joues
    lda c_round
    pha
    stz c_round
@r: jsr comp_rounds
    cmp c_round
    beq @rd
    bcc @rd
    stz t5
@m: jsr comp_mpr
    cmp t5
    beq @mn
    bcc @mn
    lda t5
    jsr comp_match_info
    bcs @skip
    lda t2
    asl a
    tax
    lda c_res,x
    and #$00FF
    cmp #$FF
    beq @skip
    sta t6
    lda c_res+1,x
    and #$00FF
    sta t7
    jsr table_add
@skip:
    inc t5
    bra @m
@mn:
    inc c_round
    bra @r
@rd:
    pla
    sta c_round
    ; tri par selection : points, difference, marques
    ldx #0
@i: sep #$20
    .a8
    txa
    sta st_order,x
    rep #$20
    .a16
    inx
    cpx c_n
    bne @i
    ldx #0
@outer:
    txy
    iny
@inner:
    cpy c_n
    bcs @on
    phx
    phy
    lda st_order,x
    and #$00FF
    sta t3
    lda st_order,y
    and #$00FF
    sta t4
    jsr table_better            ; carry = 1 si t4 devant t3
    ply
    plx
    bcc @in
    sep #$20
    .a8
    lda st_order,x
    pha
    lda st_order,y
    sta st_order,x
    pla
    sta st_order,y
    rep #$20
    .a16
@in:
    iny
    bra @inner
@on:
    inx
    cpx c_n
    bcc @outer
    rts

; table_add : t0 dom, t1 ext (participants), t6 / t7 scores
table_add:
    .a16
    .i16
    lda t0
    asl a
    tax
    lda t1
    asl a
    tay
    lda st_pf,x
    clc
    adc t6
    sta st_pf,x
    lda st_pa,x
    clc
    adc t7
    sta st_pa,x
    lda st_pf,y
    clc
    adc t7
    sta st_pf,y
    lda st_pa,y
    clc
    adc t6
    sta st_pa,y
    lda t6
    cmp t7
    beq @draw
    bcc @away
    inc st_w,x
    lda st_l,y
    inc a
    sta st_l,y
    lda st_pts,x
    clc
    adc #3
    sta st_pts,x
    rts
@away:
    lda st_w,y
    inc a
    sta st_w,y
    inc st_l,x
    lda st_pts,y
    clc
    adc #3
    sta st_pts,y
    rts
@draw:
    inc st_d,x
    lda st_d,y
    inc a
    sta st_d,y
    inc st_pts,x
    lda st_pts,y
    inc a
    sta st_pts,y
    rts

; table_better : t3, t4 participants -> carry = 1 si t4 est mieux classe que t3
table_better:
    .a16
    .i16
    lda t3
    asl a
    tax
    lda t4
    asl a
    tay
    lda st_pts,y
    cmp st_pts,x
    beq :+
    bcs @yes
    bra @no
:   lda st_pf,y
    sec
    sbc st_pa,y
    sta t6
    lda st_pf,x
    sec
    sbc st_pa,x
    sta t7
    lda t6
    cmp t7
    beq :+
    bmi @no
    bra @yes
:   lda st_pf,y
    cmp st_pf,x
    beq @no
    bcs @yes
@no:
    clc
    rts
@yes:
    sec
    rts

; -----------------------------------------------------------------------------
;  table_screen : classement
; -----------------------------------------------------------------------------
table_screen:
    .a16
    .i16
    jsr compute_table
    jsr safe_screen_off
    jsr bg3_clear
    jsr ui_fill
    jsr comp_title
    lda #UI_ATTR
    sta t0
    ldx #TPOS(1, 3)
    ldy #.loword(str_c_thead)
    jsr print
    stz t5
@row:
    lda t5
    cmp c_n
    bcc :+
    jmp @done
:   clc
    adc #4
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    sta near_tmp+2
    ; position
    lda #UI_ATTR
    sta t0
    lda t5
    inc a
    ldx near_tmp+2
    inx
    inx
    jsr print_num2
    ; equipe
    ldx t5
    lda st_order,x
    and #$00FF
    sta near_tmp+4
    tax
    lda c_teams,x
    and #$00FF
    sta t6
    tax
    lda c_ctrl,x
    and #$00FF
    ldx #UI_ATTR
    cmp #2
    bcc :+
    ldx #UI_A
:   stx t0
    lda t6
    jsr team_rec_id
    lda near_tmp+2
    clc
    adc #4 * 2
    tax
    jsr print
    ; P W D L PTS PF-PA
    lda #UI_ATTR
    sta t0
    lda near_tmp+4
    asl a
    tay
    lda st_w,y
    clc
    adc st_d,y
    adc st_l,y
    ldx #20
    jsr tbl_num
    lda st_w,y
    ldx #22
    jsr tbl_num
    lda st_d,y
    ldx #24
    jsr tbl_num
    lda st_l,y
    ldx #26
    jsr tbl_num
    lda #UI_HI
    sta t0
    lda st_pts,y
    ldx #29
    jsr tbl_num
    inc t5
    jmp @row
@done:
    jsr screen_on
@w: jsr ui_wait
    lda t7
    and #(JOY_A | JOY_B | JOY_START)
    beq @w
    rts

; tbl_num : A = valeur, X = colonne, near_tmp+2 = ligne. Preserve Y.
tbl_num:
    .a16
    .i16
    phy
    pha
    txa
    asl a
    clc
    adc near_tmp+2
    tax
    pla
    cmp #100
    bcc :+
    lda #99
:   jsr print_num2
    ply
    rts

; -----------------------------------------------------------------------------
;  bracket_screen : tous les tours de la coupe
; -----------------------------------------------------------------------------
bracket_screen:
    .a16
    .i16
    jsr safe_screen_off
    jsr bg3_clear
    jsr ui_fill
    jsr comp_title
    lda c_round
    pha
    stz c_round
    lda #3
    sta near_tmp+4              ; ligne
@r: jsr comp_rounds
    cmp c_round
    jeq @rd
    jcc @rd
    ; titre du tour
    jsr cup_round_name
    lda near_tmp+4
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc #2 * 2
    tax
    lda #UI_HI
    sta t0
    jsr print
    inc near_tmp+4
    stz hub_i
@m: jsr comp_mpr
    cmp hub_i
    beq @mn
    bcc @mn
    lda hub_i
    jsr comp_match_info
    lda near_tmp+4
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    sta near_tmp+2
    lda t0
    jsr br_team
    ldx #3
    jsr br_put
    lda t1
    jsr br_team
    ldx #16
    jsr br_put
    ; score
    lda t2
    asl a
    tax
    lda c_res,x
    and #$00FF
    cmp #$FF
    beq @nl
    sta t6
    lda c_res+1,x
    and #$00FF
    sta t7
    lda #UI_HI
    sta t0
    lda t6
    ldx #8
    jsr tbl_num
    lda #('-' - 32 + UI_HI)
    pha
    lda near_tmp+2
    clc
    adc #11 * 2
    tax
    pla
    sta bg3_map,x
    lda t7
    ldx #12
    jsr tbl_num
@nl:
    inc near_tmp+4
    inc hub_i
    bra @m
@mn:
    inc c_round
    brl @r
@rd:
    pla
    sta c_round
    jsr screen_on
@w: jsr ui_wait
    lda t7
    and #(JOY_A | JOY_B | JOY_START)
    beq @w
    rts

; br_team : A = participant ($FF : a determiner) -> Y = nom court, t0 = attributs
br_team:
    .a16
    .i16
    cmp #$FF
    bne :+
    ldy #.loword(str_c_tbd)
    lda #UI_ATTR
    sta t0
    rts
:   tax
    lda c_teams,x
    and #$00FF
    sta t6
    tax
    lda c_ctrl,x
    and #$00FF
    ldx #UI_ATTR
    cmp #2
    bcc :+
    ldx #UI_A
:   stx t0
    lda t6
    jsr team_rec_id
    tya
    clc
    adc #T_SHORT
    tay
    rts

; br_put : X = colonne -> imprime Y a la ligne near_tmp+2
br_put:
    .a16
    txa
    asl a
    clc
    adc near_tmp+2
    tax
    jmp print

; -----------------------------------------------------------------------------
;  champion_screen
; -----------------------------------------------------------------------------
champion_screen:
    .a16
    .i16
    jsr safe_screen_off
    jsr bg3_clear
    jsr ui_fill
    jsr comp_title
    lda #UI_HI
    sta t0
    ldx #TPOS(11, 8)
    ldy #.loword(str_c_champ)
    jsr print
    lda c_champ
    jsr team_rec_id
    lda #UI_A
    sta t0
    ldx #TPOS(8, 11)
    jsr print
    lda c_champ
    jsr team_rec_id
    tya
    clc
    adc #T_WORLD
    tay
    lda #UI_ATTR
    sta t0
    ldx #TPOS(8, 13)
    jsr print
    ; trophee
    lda #UI_HI
    sta t0
    ldx #TPOS(13, 16)
    ldy #.loword(str_tro1)
    jsr print
    ldx #TPOS(13, 17)
    ldy #.loword(str_tro2)
    jsr print
    ldx #TPOS(13, 18)
    ldy #.loword(str_tro3)
    jsr print
    ldx #TPOS(13, 19)
    ldy #.loword(str_tro4)
    jsr print
    jsr screen_on
    lda #MUS_JINGLE
    jsr sfx_play
    lda #SFX_GOAL
    jsr sfx_play
@w: jsr ui_wait
    lda t7
    and #(JOY_A | JOY_B | JOY_START)
    beq @w
    jmp comp_hub

; -----------------------------------------------------------------------------
;  sauvegarde SRAM : en-tete "NXCP", version, longueur, donnees, checksum
; -----------------------------------------------------------------------------
; comp_sram : A = emplacement -> X = decalage dans la SRAM
comp_sram:
    .a16
    xba
    asl a
    asl a                       ; * $400
    and #$0C00
    tax
    rts

; Sauvegarde A/B : chaque emplacement existe en deux copies ($A06100 et $A06D00).
; On ecrit toujours la copie la plus ancienne (ou invalide) avec un numero de generation
; +1, la signature en dernier : une coupure pendant l'ecriture laisse l'autre copie intacte.
; Bloc : +0 "NXCP", +4 version, +6 taille, +8 donnees, +8+N checksum, +10+N generation.
COMP_B = $C00                   ; ecart entre les copies A et B

; comp_check : X = offset de la copie -> C = 1 si valide, A = generation. Preserve X.
comp_check:
    .a16
    .i16
    phx
    lda f:SRAM_COMP,x
    cmp #('N' | ('X' << 8))
    bne @no
    lda f:SRAM_COMP+2,x
    cmp #('C' | ('P' << 8))
    bne @no
    lda f:SRAM_COMP+4,x
    cmp #COMP_VERSION
    bne @v3
    lda f:SRAM_COMP+6,x
    cmp #COMP_SIZE
    bne @no
    bra @sz
@v3:
    cmp #3
    bne @no
    lda f:SRAM_COMP+6,x
    cmp #COMP_SIZE - 2
    bne @no
@sz:
    sta cs_sz
    lda #$1234
    sta t4
    ldy #0
@c: lda f:SRAM_COMP+8,x
    clc
    adc t4
    sta t4
    inx
    inx
    iny
    iny
    cpy cs_sz
    bcc @c
    lda f:SRAM_COMP+8,x
    cmp t4
    bne @no
    lda f:SRAM_COMP+10,x
    plx
    sec
    rts
@no:
    plx
    clc
    rts

; comp_pick : A = emplacement -> X = offset de la copie la plus recente valide (C = 1),
; cs_old = offset de la copie a reecrire, cs_gen = generation suivante
comp_pick:
    .a16
    .i16
    jsr comp_sram
    stx cs_a
    txa
    clc
    adc #COMP_B
    sta cs_b
    ldx cs_a
    jsr comp_check
    stz cs_va
    bcc :+
    sta cs_ga
    inc cs_va
:   ldx cs_b
    jsr comp_check
    stz cs_vb
    bcc :+
    sta cs_gb
    inc cs_vb
:   lda cs_va
    beq @b                      ; A invalide : B si valide
    lda cs_vb
    beq @a                      ; seulement A
    lda cs_gb
    sec
    sbc cs_ga
    bmi @a                      ; A plus recente
@b: lda cs_vb
    beq @none
    lda cs_gb
    inc a
    sta cs_gen
    lda cs_a
    sta cs_old
    ldx cs_b
    sec
    rts
@a: lda cs_ga
    inc a
    sta cs_gen
    lda cs_b
    sta cs_old
    ldx cs_a
    sec
    rts
@none:
    stz cs_gen
    lda cs_a
    sta cs_old
    clc
    rts

comp_save:
    .a16
    .i16
    lda c_slot
    jsr comp_pick
    ldx cs_old
    lda #0
    sta f:SRAM_COMP,x           ; copie invalidee pendant l'ecriture
    lda #COMP_VERSION
    sta f:SRAM_COMP+4,x
    lda #COMP_SIZE
    sta f:SRAM_COMP+6,x
    ldy #0
    lda #$1234
    sta t4
@l: lda comp_data,y
    sta f:SRAM_COMP+8,x
    clc
    adc t4
    sta t4
    inx
    inx
    iny
    iny
    cpy #COMP_SIZE
    bcc @l
    lda t4
    sta f:SRAM_COMP+8,x
    lda cs_gen
    sta f:SRAM_COMP+10,x
    ldx cs_old
    lda #('C' | ('P' << 8))
    sta f:SRAM_COMP+2,x
    lda #('N' | ('X' << 8))     ; signature en dernier : la copie devient valide
    sta f:SRAM_COMP,x
    rts

; comp_load : c_slot_sel -> carry = 1 si une competition valide a ete chargee
comp_load:
    .a16
    .i16
    lda c_slot_sel
    jsr comp_pick
    bcc @no
    stz c_seed                  ; (version 3 : pas de graine)
    lda f:SRAM_COMP+6,x
    sta cs_sz
    ldy #0
@k: lda f:SRAM_COMP+8,x
    sta comp_data,y
    inx
    inx
    iny
    iny
    cpy cs_sz
    bcc @k
    lda c_slot
    cmp c_slot_sel
    bne @no
    sec
    rts
@no:
    clc
    rts

.segment "RODATA"
str_c_champs:   .byte "CHAMPIONSHIP", 0
str_c_cups:     .byte "CUP", 0
str_c_customs:  .byte "CUSTOM COMPETITION", 0
comp_names:     .word .loword(str_c_champs), .loword(str_c_cups), .loword(str_c_customs)
str_c_cont:     .byte "CONTINUE", 0
str_c_new:      .byte "NEW COMPETITION", 0
str_c_type:     .byte "TYPE", 0
str_c_league:   .byte "LEAGUE", 0
str_c_cup:      .byte "CUP", 0
str_c_rot:      .byte "ROTATION", 0
str_c_teams:    .byte "TEAMS", 0
str_c_badl:     .byte "LEAGUE: 3 TO 16 TEAMS", 0
str_c_badc:     .byte "CUP: 4, 8 OR 16 TEAMS", 0
str_c_round:    .byte "ROUND", 0
str_c_final:    .byte "FINAL", 0
str_c_semi:     .byte "SEMI-FINALS", 0
str_c_quarter:  .byte "QUARTER-FINALS", 0
str_c_r16:      .byte "ROUND OF 16", 0
str_c_vs:       .byte "VS", 0
str_c_play:     .byte "PLAY", 0
str_c_over:     .byte "COMPETITION OVER", 0
str_c_table:    .byte "TABLE", 0
str_c_bracket:  .byte "BRACKET", 0
str_c_save:     .byte "SAVE & EXIT", 0
str_c_pw:       .byte "PASSWORD", 0
str_c_watch:    .byte "WATCH CPU MATCHES", 0
str_c_playas:   .byte "PLAY AS", 0
str_c_home:     .byte "HOME", 0
str_c_away:     .byte "AWAY", 0
str_c_champ:    .byte "CHAMPION", 0
str_c_thead:    .byte "   TEAM             P W D L PTS", 0
str_c_tbd:      .byte "---", 0
str_c_out:      .byte "OUT", 0
str_c_cpu:      .byte "CPU", 0
str_c_p1:       .byte "P1", 0
str_c_p2:       .byte "P2", 0
ctrl_names:     .word .loword(str_c_out), .loword(str_c_cpu), .loword(str_c_p1), .loword(str_c_p2)
str_tro1:       .byte "*****", 0
str_tro2:       .byte " ***", 0
str_tro3:       .byte "  *", 0
str_tro4:       .byte " ***", 0
.segment "CODE2"
