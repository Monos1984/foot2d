; =============================================================================
;  ui.asm - ecrans de configuration : match (equipes, difficulte, regles),
;  equipe (formation + 6 tactiques), remplacements / composition
; =============================================================================

UI_ATTR  = TXT_ATTR
UI_HI    = UI_ATTR + $1000          ; texte accentue (palette 4)
UI_A     = UI_ATTR + $1400          ; equipe humaine (palette 5)
NUM_TACT = 6

; ui_fill : ecran entier sur panneau opaque
ui_fill:
    .a16
    .i16
    ; ecran eteint : fond des menus sur BG1 (sinon le jeu reste visible, assombri)
    lda inidisp
    and #$0080
    beq :+
    jsr load_menubg
:   jsr layers_menu
    jsr bg2_clear
    stz pn_x
    stz pn_y
    lda #32
    sta pn_w
    lda #28
    sta pn_h
    jsr bg2_panel
    ; bandeau de titre (ligne 1)
    ldx #(1 * 32 + 1) * 2
    ldy #30
@hb:
    lda #P_BAR
    sta f:BG2_SHADOW,x
    inx
    inx
    dey
    bne @hb
    lda #1
    sta bar_x0
    lda #30
    sta bar_w
    rts

; print_w : comme print (t0 = attributs) puis complete par des espaces jusqu'a t1 cases
print_w:
    .a16
    .i16
    stx t2
    jsr print
    txa
    sec
    sbc t2
    lsr a                       ; cases ecrites
    eor #$FFFF
    sec
    adc t1
    beq @d
    bmi @d
    tay
    lda t0
    and #$FF00
    jmp fill_tiles
@d: rts

; ui_cursor : A = ligne selectionnee (row), t3 = liste des lignes (adresse), t4 = nombre
; dessine '>' en colonne 1 devant la ligne choisie, efface les autres
ui_cursor:
    .a16
    .i16
    sta t5
    ldy t3
@l: lda a:0,y
    and #$00FF
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc #2                      ; colonne 1
    tax
    lda #UI_ATTR
    phy
    tya
    sec
    sbc t3
    cmp t5
    bne :+
    lda #('>' - 32 + UI_HI)
    bra :++
:   lda #UI_ATTR
:   sta bg3_map,x
    ply
    iny
    dec t4
    bne @l
    lda #1
    sta bg3_dirty
    ; barre de selection sur la ligne choisie
    lda t5
    clc
    adc t3
    tay
    lda a:0,y
    and #$00FF
    jmp bg2_bar

; ui_wait : une frame d'interface ; t7 = boutons appuyes (manettes 1 et 2)
ui_wait:
    .a16
    jsr oam_begin
    jsr oam_finish
    jsr wait_frame
    lda bg1_mode
    beq :+
    jsr crowd_update            ; etoiles qui scintillent
:
    lda joy_new
    ora joy_new+2
    sta t7
    ; repetition automatique des directions maintenues
    lda joy_cur
    ora joy_cur+2
    and #(JOY_UP | JOY_DOWN | JOY_LEFT | JOY_RIGHT)
    beq @none
    cmp rep_bits
    bne @new
    inc rep_t
    lda rep_t
    cmp #20
    bcc @d
    and #$0003
    bne @d
    lda rep_bits
    ora t7
    sta t7
@d: rts
@new:
    sta rep_bits
    stz rep_t
    rts
@none:
    stz rep_bits
    rts

; ui_updown : t7 = boutons, A = selection, Y = nombre -> A = nouvelle selection
ui_updown:
    .a16
    .i16
    sty t6
    pha
    lda t7
    bit #JOY_DOWN
    beq :+
    pla
    jsr ui_click
    inc a
    cmp t6
    bcc @r
    lda #0
    rts
:   bit #JOY_UP
    beq :+
    pla
    jsr ui_click
    dec a
    bpl @r
    lda t6
    dec a
    rts
:   pla
@r: rts

; ui_click : petit son de menu (preserve A, X, Y)
ui_click:
    .a16
    pha
    lda #SFX_MENU
    jsr sfx_play
    pla
    rts

; ui_lr : t7 = boutons, A = valeur, Y = nombre de valeurs -> A modifiee (gauche/droite, A/B)
ui_lr:
    .a16
    .i16
    sty t6
    pha
    lda t7
    bit #JOY_LEFT
    beq :+
    pla
    jsr ui_click
    dec a
    bpl @r
    lda t6
    dec a
    rts
:   and #(JOY_RIGHT | JOY_A)
    beq :+
    pla
    jsr ui_click
    inc a
    cmp t6
    bcc @r
    lda #0
    rts
:   pla
@r: rts

; =============================================================================
;  match_setup : choix des equipes, tactiques, difficulte, regles
; =============================================================================
SETUP_ITEMS = 9

match_setup:
    .a16
    .i16
    stz ui_sel
    lda #0
@ts:
    jsr team_select             ; d'abord le choix visuel des equipes
    bcc @redraw_all
    jsr menu_confirm
    bcs :+
    lda ts_side
    bra @ts
:   jmp title_screen
@redraw_all:
    jsr safe_screen_off
    jsr bg3_clear
    jsr oam_clear
    jsr ui_fill
    lda #UI_HI
    sta t0
    ldx #TPOS(10, 1)
    ldy #.loword(str_setup)
    jsr print
    jsr setup_draw
    jsr screen_on
@loop:
    jsr ui_wait
    lda t7
    beq @loop
    lda ui_sel
    ldy #SETUP_ITEMS
    jsr ui_updown
    cmp ui_sel
    beq :+
    sta ui_sel
    jsr setup_draw
    bra @loop
:   lda t7
    bit #JOY_B
    beq @nob
    jsr menu_confirm
    bcc @loop
    jmp title_screen
@nob:
    lda t7
    bit #JOY_START
    beq :+
    jmp start_match             ; START : on joue tout de suite
:   lda ui_sel
    bne :+
    jmp @mode
:   dec a
    cmp #2
    bcs @n01
    ; equipe domicile / exterieur (A : ecran de choix visuel)
    pha
    lda t7
    and #JOY_A
    beq :+
    pla
    jsr team_select
    jmp @redraw_all
:   pla
    asl a
    tax
    lda team_id,x
    phx
    jsr team_step
    plx
    cmp team_id,x
    beq @loop
    sta team_id,x
    txa
    lsr a
    jsr team_defaults
    jsr setup_draw
    bra @loop
@n01:
    cmp #4
    bcs @n23
    ; tactiques d'une equipe
    lda t7
    and #(JOY_A | JOY_START)
    beq @loop
    lda ui_sel
    sec
    sbc #3
    sta ui_team
    stz ui_back
    lda ui_sel                  ; l'ecran des tactiques utilise aussi ui_sel
    pha
    jsr team_screen
    pla
    sta ui_sel
    jmp @redraw_all
@n23:
    cmp #4
    bne @n3s
    lda stadium_id
    ldy #NUM_STADIUMS
    jsr ui_lr
    sta stadium_id
    jsr setup_draw
    jmp @loop
@n3s:
    cmp #5
    bne @n4
    lda difficulty
    ldy #4
    jsr ui_lr
    sta difficulty
    jsr setup_draw
    jmp @loop
@n4:
    cmp #6
    bne @n5
    lda draw_rule
    ldy #2
    jsr ui_lr
    sta draw_rule
    jsr setup_draw
    jmp @loop
@n5:
    lda t7
    and #(JOY_A | JOY_START)
    jeq @loop
    jmp start_match
@mode:
    lda game_mode
    ldy #3
    jsr ui_lr
    sta game_mode
    jsr setup_draw
    jmp @loop

setup_rows:
    .byte 3, 5, 8, 11, 12, 14, 15, 16, 18

setup_draw:
    .a16
    .i16
    lda #UI_ATTR
    sta t0
    ldx #TPOS(2, 3)
    ldy #.loword(str_mode)
    jsr print
    lda game_mode
    asl a
    tay
    lda mode_names,y
    tay
    lda #UI_HI
    sta t0
    lda #12
    sta t1
    ldx #TPOS(8, 3)
    jsr print_w
    ; equipes
    ldx #0
@side:
    phx
    lda #UI_ATTR
    sta t0
    ldy #.loword(str_home)
    lda setup_rowpos,x
    tax
    cpx #TPOS(2, 8)
    bne :+
    ldy #.loword(str_away)
:   jsr print
    plx
    phx
    txa
    lsr a
    sta t5
    jsr team_rec
    lda #UI_A
    ldx t5
    beq :+
    lda #UI_HI
:   sta t0
    plx
    phx
    lda setup_rowpos,x
    clc
    adc #(8 - 2) * 2
    tax
    lda #16
    sta t1
    jsr print_w
    ; controle : PLAYER 1 / PLAYER 2 / CPU
    plx
    phx
    lda #UI_ATTR
    sta t0
    lda setup_rowpos,x
    clc
    adc #64 + (8 - 2) * 2
    pha
    txa
    lsr a
    sta t5
    pla
    tax
    phx
    ; style
    lda t5
    jsr team_rec
    lda a:T_STYLE,y
    and #$00FF
    sta t2
    asl a
    asl a
    clc
    adc t2
    asl a                       ; *10
    clc
    adc #.loword(style_names)
    tay
    jsr print
    plx
    txa
    clc
    adc #11 * 2
    tax
    ldy #.loword(str_lv)
    jsr print
    phx
    lda t5
    jsr team_rec
    lda a:T_LEVEL,y
    and #$00FF
    plx
    jsr print_digit
    ; qui joue
    plx
    phx
    lda setup_rowpos,x
    clc
    adc #(26 - 2) * 2
    tax
    lda t5
    asl a
    tay
    ldy #.loword(str_cpu)
    lda game_mode
    cmp #MODE_CPU
    beq @who
    lda t5
    bne :+
    ldy #.loword(str_p1)
    bra @who
:   lda game_mode
    cmp #MODE_2P
    bne @who
    ldy #.loword(str_p2)
@who:
    lda #UI_HI
    sta t0
    jsr print
    plx
    inx
    inx
    cpx #4
    jne @side

    lda #UI_ATTR
    sta t0
    ldx #TPOS(2, 11)
    ldy #.loword(str_home_tac)
    jsr print
    ldx #TPOS(2, 12)
    ldy #.loword(str_away_tac)
    jsr print
    ldx #TPOS(2, 14)
    ldy #.loword(str_stadium)
    jsr print
    lda stadium_id
    asl a
    sta t5
    asl a
    clc
    adc t5
    asl a
    tay
    lda stadium_tab+10,y
    tay
    ldx #TPOS(14, 14)
    lda #UI_HI
    sta t0
    lda #17
    sta t1
    jsr print_w
    lda #UI_ATTR
    sta t0
    ldx #TPOS(2, 15)
    ldy #.loword(str_difficulty)
    jsr print
    lda difficulty
    asl a
    tay
    lda diff_names,y
    tay
    ldx #TPOS(14, 15)
    lda #UI_HI
    sta t0
    lda #10
    sta t1
    jsr print_w
    lda #UI_ATTR
    sta t0
    ldx #TPOS(2, 16)
    ldy #.loword(str_rules)
    jsr print
    ldy #.loword(str_rule0)
    lda draw_rule
    beq :+
    ldy #.loword(str_rule1)
:   ldx #TPOS(14, 16)
    lda #UI_HI
    sta t0
    lda #17
    sta t1
    jsr print_w
    lda #UI_ATTR
    sta t0
    ldx #TPOS(2, 18)
    ldy #.loword(str_start)
    jsr print
    ldx #TPOS(1, 25)
    ldy #.loword(str_setup_help)
    jsr print
    ; curseur
    lda #.loword(setup_rows)
    sta t3
    lda #SETUP_ITEMS
    sta t4
    lda ui_sel
    jmp ui_cursor

setup_rowpos:
    .word TPOS(2, 5), TPOS(2, 8)

; =============================================================================
;  team_screen : formation + tactiques de l'equipe ui_team. Retour par B / DONE.
; =============================================================================
TEAM_ITEMS = 9

team_screen:
    .a16
    .i16
    stz ui_sel
@redraw:
    jsr bg3_clear
    jsr ui_fill
    jsr team_draw
@loop:
    jsr ui_wait
    lda t7
    beq @loop
    and #(JOY_B | JOY_START)
    bne @done
    lda ui_sel
    ldy #TEAM_ITEMS
    jsr ui_updown
    cmp ui_sel
    beq :+
    sta ui_sel
    jsr team_draw
    bra @loop
:   lda ui_sel
    bne @tac
    ; formation
    lda ui_team
    asl a
    tax
    lda form_id,x
    ldy #NUM_FORMS
    jsr ui_lr
    cmp form_id,x
    beq @loop
    sta form_id,x
    jsr team_form_changed
    jsr team_draw
    bra @loop
@tac:
    cmp #7
    bcs @other
    ; tactique (ui_sel - 1)
    dec a
    asl a
    sta t5
    lda ui_team
    beq :+
    lda #12
:   clc
    adc t5
    tax
    lda tact,x
    ldy #3
    jsr ui_lr
    sta tact,x
    jsr team_draw
    bra @loop
@other:
    lda t7
    and #JOY_A
    beq @loop
    lda ui_sel
    cmp #8
    beq @done
    pha
    jsr subs_screen
    pla
    sta ui_sel
    brl @redraw
@done:
    rts

; team_form_changed : la formation de ui_team a change
team_form_changed:
    .a16
    lda ui_back                 ; 0 = avant match : nouvelle composition
    bne :+
    lda ui_team
    jmp build_lineup
:   lda ui_team
    jmp apply_formation

team_rows:
    .byte 6, 8, 9, 10, 11, 12, 13, 15, 17

team_draw:
    .a16
    .i16
    lda ui_team
    jsr team_rec
    lda #UI_HI
    sta t0
    ldx #TPOS(8, 3)
    jsr print
    lda #UI_ATTR
    sta t0
    ldx #TPOS(2, 6)
    ldy #.loword(str_formation)
    jsr print
    lda ui_team
    asl a
    tax
    lda form_id,x
    asl a
    asl a
    asl a                       ; 6 octets par nom ("2-2-1",0)
    sec
    sbc form_id,x
    sbc form_id,x
    clc
    adc #.loword(form_names)
    tay
    lda #UI_HI
    sta t0
    ldx #TPOS(14, 6)
    jsr print
    ; 6 tactiques
    stz t5
@t: lda #UI_ATTR
    sta t0
    lda t5
    asl a
    tay
    lda tact_labels,y
    tay
    lda t5
    clc
    adc #8
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc #2 * 2
    tax
    phx
    jsr print
    ; valeur
    lda ui_team
    beq :+
    lda #12
:   sta t4
    lda t5
    asl a
    clc
    adc t4
    tax
    lda tact,x
    sta t4
    lda t5
    asl a
    adc t5                      ; *3
    adc t4
    asl a
    tay
    lda tact_vals,y
    tay
    pla
    clc
    adc #12 * 2
    tax
    lda #UI_HI
    sta t0
    lda #10
    sta t1
    jsr print_w
    inc t5
    lda t5
    cmp #NUM_TACT
    bne @t
    lda #UI_ATTR
    sta t0
    ldx #TPOS(2, 15)
    ldy #.loword(str_lineup)
    jsr print
    ldx #TPOS(2, 17)
    ldy #.loword(str_done)
    jsr print
    ldx #TPOS(2, 25)
    ldy #.loword(str_team_help)
    jsr print
    lda #.loword(team_rows)
    sta t3
    lda #TEAM_ITEMS
    sta t4
    lda ui_sel
    jmp ui_cursor

; =============================================================================
;  subs_screen : composition / remplacements de ui_team.
;  A sur un joueur puis A sur un autre : echange. B : retour.
; =============================================================================
subs_screen:
    .a16
    .i16
    stz ui_sel
    lda #$FFFF
    sta ui_pick
    jsr bg3_clear
    jsr ui_fill
@redraw:
    jsr subs_draw
@loop:
    jsr ui_wait
    lda t7
    beq @loop
    and #(JOY_B | JOY_START)
    bne @done
    lda ui_sel
    ldy #12
    jsr ui_updown
    cmp ui_sel
    beq :+
    sta ui_sel
    bra @redraw
:   lda t7
    and #JOY_A
    beq @loop
    lda ui_pick
    bpl :+
    lda ui_sel
    sta ui_pick
    bra @redraw
:   cmp ui_sel
    beq @cancel
    jsr subs_swap
@cancel:
    lda #$FFFF
    sta ui_pick
    bra @redraw
@done:
    rts

; slot_of_roster : A = index effectif -> A = slot (0..5) ou $FFFF si remplacant
slot_of_roster:
    .a16
    .i16
    sta t6
    lda ui_team
    beq :+
    lda #12
:   tax
    ldy #0
@l: lda lineup,x
    cmp t6
    beq @f
    inx
    inx
    iny
    cpy #6
    bne @l
    lda #$FFFF
    rts
@f: tya
    rts

; subs_swap : echange les joueurs ui_pick et ui_sel de l'effectif
subs_swap:
    .a16
    .i16
    lda ui_pick
    jsr slot_of_roster
    sta near_tmp                ; slot du premier
    lda ui_sel
    jsr slot_of_roster
    sta near_tmp+2              ; slot du second
    and near_tmp
    bpl :+
    lda near_tmp
    and near_tmp+2
    bmi @none                   ; deux remplacants : rien
:   ; en match : sauver la fatigue des joueurs concernes
    jsr subs_save_fatigue
    lda ui_team
    beq :+
    lda #12
:   sta t4
    lda near_tmp
    bmi :+
    asl a
    clc
    adc t4
    tax
    lda ui_sel
    sta lineup,x
:   lda near_tmp+2
    bmi :+
    asl a
    clc
    adc t4
    tax
    lda ui_pick
    sta lineup,x
:   ; en match : recharger les caracteristiques des slots
    lda ui_back
    beq @none
    lda near_tmp
    jsr subs_reload
    lda near_tmp+2
    jsr subs_reload
@none:
    rts

subs_save_fatigue:
    .a16
    lda ui_back
    beq @d
    lda ui_team
    beq :+
    lda #TEAM_SIZE
:   asl a
    tax
    ldy #6
@l: phy
    phx
    jsr save_slot_fatigue
    plx
    ply
    inx
    inx
    dey
    bne @l
@d: rts

; subs_reload : A = slot ($FFFF : rien)
subs_reload:
    .a16
    .i16
    cmp #0
    bmi @d
    sta t5
    lda ui_team
    beq :+
    lda #TEAM_SIZE
:   clc
    adc t5
    asl a
    tax
    stx cp
    jsr load_slot_stats
@d: rts

subs_draw:
    .a16
    .i16
    lda ui_team
    jsr team_rec
    lda #UI_HI
    sta t0
    ldx #TPOS(8, 2)
    jsr print
    lda #UI_ATTR
    sta t0
    ldx #TPOS(0, 4)
    ldy #.loword(str_subs_head)
    jsr print
    stz t5                      ; index effectif
@row:
    ; position ecran : ligne 5 + i (colonne 2)
    lda t5
    clc
    adc #5
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    sta near_tmp+4
    ; marqueurs
    lda t5
    jsr slot_of_roster
    sta t3
    ldx near_tmp+4
    lda #UI_ATTR
    ldy t5
    cpy ui_sel
    bne :+
    lda #('>' - 32 + UI_HI)
:   sta bg3_map+2,x
    lda #UI_ATTR
    ldy t5
    cpy ui_pick
    bne :+
    lda #('*' - 32 + UI_HI)
:   sta bg3_map+4,x
    ; poste
    lda ui_team
    ldx t5
    jsr roster_rec
    sty t2
    lda a:PL_ROLE,y
    and #$00FF
    asl a
    tay
    lda role_names,y
    tay
    lda #UI_ATTR
    ldx t3
    bmi :+
    lda #UI_A                   ; titulaire : en couleur
:   sta t0
    lda near_tmp+4
    clc
    adc #3 * 2
    tax
    jsr print
    ; nom (8 caracteres)
    ldy t2
    lda near_tmp+4
    clc
    adc #6 * 2
    tax
    lda #8
    sta t1
@nm:
    lda a:0,y
    and #$00FF
    sec
    sbc #32
    clc
    adc t0
    sta bg3_map,x
    inx
    inx
    iny
    dec t1
    bne @nm
    ; 7 caracteristiques
    lda #UI_ATTR
    sta t0
    ldy t2
    lda near_tmp+4
    clc
    adc #15 * 2
    tax
    lda #7
    sta t1
@st:
    lda a:PL_STATS,y
    and #$00FF
    clc
    adc #('0' - 32)
    adc t0
    sta bg3_map,x
    inx
    inx
    inx
    inx
    iny
    dec t1
    bne @st
    ; fatigue (0-9)
    ldx t3
    bmi @bench
    lda ui_back
    beq @bench
    lda ui_team
    beq :+
    lda #TEAM_SIZE
:   clc
    adc t3
    asl a
    tax
    lda p_fatigue,x
    bra @fat
@bench:
    lda t5
    asl a
    ldy ui_team
    beq :+
    clc
    adc #24
:   tay
    lda r_fat,y
@fat:
    ldx #300
    jsr divu
    cmp #10
    bcc :+
    lda #9
:   ldx near_tmp+4
    clc
    adc #('0' - 32 + UI_HI)
    sta bg3_map + 29*2,x
    inc t5
    lda t5
    cmp #12
    jne @row
    lda #UI_ATTR
    sta t0
    ldx #TPOS(1, 19)
    ldy #.loword(str_subs_help)
    jsr print
    ldx #TPOS(1, 20)
    ldy #.loword(str_subs_help2)
    jsr print
    lda #1
    sta bg3_dirty
    lda ui_sel
    clc
    adc #5
    jmp bg2_bar

.segment "RODATA"
str_setup:      .byte "MATCH SETUP", 0
str_mode:       .byte "MODE", 0
str_md0:        .byte "1P VS CPU", 0
str_md1:        .byte "1P VS 2P", 0
str_md2:        .byte "CPU VS CPU", 0
mode_names:     .word .loword(str_md0), .loword(str_md1), .loword(str_md2)
str_home:       .byte "HOME", 0
str_away:       .byte "AWAY", 0
str_lv:         .byte "LEVEL ", 0
str_cpu:        .byte "CPU", 0
str_p1:         .byte "P1 ", 0
str_p2:         .byte "P2 ", 0
str_home_tac:   .byte "HOME TACTICS", 0
str_away_tac:   .byte "AWAY TACTICS", 0
str_difficulty: .byte "DIFFICULTY", 0
str_stadium:    .byte "STADIUM", 0
str_rules:      .byte "TIE RULE", 0
str_rule0:      .byte "DRAW ALLOWED", 0
str_rule1:      .byte "OVERTIME+SHOOTOUT", 0
str_start:      .byte "START MATCH", 0
str_setup_help: .byte "<> CHANGE  A OK  B BACK  START", 0
str_formation:  .byte "FORMATION", 0
str_lineup:     .byte "LINEUP / SUBS", 0
str_done:       .byte "DONE", 0
str_team_help:  .byte "<> CHANGE  A SELECT  B DONE", 0
str_subs_head:  .byte "   PO NAME     S P A K C D T F", 0
str_subs_help:  .byte "A: PICK A PLAYER, A: SWAP", 0
str_subs_help2: .byte "COLOR = ON FIELD   F = FATIGUE", 0
str_easy:       .byte "EASY", 0
str_normal:     .byte "NORMAL", 0
str_hard:       .byte "HARD", 0
str_expert:     .byte "EXPERT", 0
diff_names:     .word .loword(str_easy), .loword(str_normal), .loword(str_hard), .loword(str_expert)
str_t_ment:     .byte "MENTALITY", 0
str_t_pass:     .byte "PASSING", 0
str_t_press:    .byte "PRESSURE", 0
str_t_line:     .byte "DEF LINE", 0
str_t_att:      .byte "ATTACK", 0
str_t_tempo:    .byte "TEMPO", 0
tact_labels:
    .word .loword(str_t_ment), .loword(str_t_pass), .loword(str_t_press)
    .word .loword(str_t_line), .loword(str_t_att), .loword(str_t_tempo)
str_v_def:      .byte "DEFENSIVE", 0
str_v_bal:      .byte "BALANCED", 0
str_v_off:      .byte "OFFENSIVE", 0
str_v_short:    .byte "SHORT", 0
str_v_mixed:    .byte "MIXED", 0
str_v_long:     .byte "LONG", 0
str_v_low:      .byte "LOW", 0
str_v_high:     .byte "HIGH", 0
str_v_deep:     .byte "DEEP", 0
str_v_center:   .byte "CENTER", 0
str_v_sides:    .byte "SIDES", 0
str_v_slow:     .byte "SLOW", 0
str_v_fast:     .byte "FAST", 0
tact_vals:
    .word .loword(str_v_def), .loword(str_v_bal), .loword(str_v_off)
    .word .loword(str_v_short), .loword(str_v_mixed), .loword(str_v_long)
    .word .loword(str_v_low), .loword(str_normal), .loword(str_v_high)
    .word .loword(str_v_deep), .loword(str_normal), .loword(str_v_high)
    .word .loword(str_v_center), .loword(str_v_sides), .loword(str_v_mixed)
    .word .loword(str_v_slow), .loword(str_normal), .loword(str_v_fast)
str_gk:         .byte "GK", 0
str_df:         .byte "DF", 0
str_mf:         .byte "MF", 0
str_fw:         .byte "FW", 0
role_names:     .word .loword(str_gk), .loword(str_df), .loword(str_mf), .loword(str_fw)
.segment "CODE"
