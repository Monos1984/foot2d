; =============================================================================
;  hud.asm - bandeau de score, horloge, messages, animation du public
; =============================================================================

HUD_ATTR   = TXT_ATTR + TXT_PANEL
HUD_TEAMA  = HUD_ATTR + $0400       ; palette BG3 1
HUD_TEAMB  = HUD_ATTR + $0C00       ; palette BG3 3
HUD_PLA    = TXT_ATTR + $0400       ; plaques d'equipe (tiles HT_PL_*)
HUD_PLB    = TXT_ATTR + $0C00
HUD_SB     = TXT_ATTR + $1800       ; cadre du score (palette 6)
HUD_HALF   = HUD_ATTR + $1400       ; periode en cyan (palette 5)
MSG_ROW    = 12

hud_init:
    .a16
    .i16
    jsr hud_draw_static
    rts

hud_draw_static:
    .a16
    .i16
    ldx #TPOS(0, 0)
    ldy #96
    lda #0
    jsr fill_tiles
    ; plaques d'equipe
    lda #HUD_PLA
    sta t2
    ldx #TPOS(3, 0)
    lda #0
    jsr hud_plate
    lda #HUD_PLB
    sta t2
    ldx #TPOS(22, 0)
    lda #1
    jsr hud_plate
    ; cadre du score
    ldx #TPOS(11, 0)
    lda #HT_SB_CAPL + HUD_SB
    jsr hud_put2
    ldx #TPOS(20, 0)
    lda #HT_SB_CAPR + HUD_SB
    jsr hud_put2
    ldx #TPOS(12, 0)
@sb:
    lda #HT_SB_BLANK + HUD_SB
    jsr hud_put2
    inx
    inx
    cpx #TPOS(20, 0)
    bne @sb
    ldx #TPOS(15, 0)
    lda #HT_SB_DASHL + HUD_SB
    jsr hud_put2
    ldx #TPOS(16, 0)
    lda #HT_SB_DASHR + HUD_SB
    jsr hud_put2
    ; ligne du temps
    lda #HT_TM_L + HUD_SB
    sta bg3_map + TPOS(11, 2)
    lda #HT_TM_R + HUD_SB
    sta bg3_map + TPOS(20, 2)
    ldx #TPOS(12, 2)
    ldy #8
    lda #HUD_ATTR
    jsr fill_tiles
    lda #HUD_HALF
    sta t0
    ldx #TPOS(17, 2)
    lda m_half
    dec a
    and #$0003
    asl a
    tay
    lda half_names,y
    tay
    jsr print
    lda #$FFFF
    sta hud_cache
    sta hud_cache+2
    sta hud_cache+4
    rts

; hud_put2 : A = tile du haut + attributs, X = position (ligne du haut). Preserve X.
hud_put2:
    .a16
    .i16
    sta bg3_map,x
    inc a
    sta bg3_map+64,x
    rts

; hud_plate : A = cote, X = position de l'extremite gauche, t2 = attributs
hud_plate:
    .a16
    .i16
    phx
    jsr team_rec
    tya
    clc
    adc #T_SHORT
    sta t3                      ; nom court
    plx
    lda #HT_PL_CAPL
    clc
    adc t2
    jsr hud_put2
    inx
    inx
    lda #HT_PL_BLANK
    clc
    adc t2
    jsr hud_put2
    inx
    inx
    stz t4
@ch:
    lda #HT_PL_BLANK
    ldy t3
    beq @put
    lda a:0,y
    and #$00FF
    bne :+
    stz t3                      ; fin de chaine : plaque vide ensuite
    lda #HT_PL_BLANK
    bra @put
:   inc t3
    cmp #'A'
    bcc @blank
    cmp #'Z' + 1
    bcs @blank
    sec
    sbc #'A'
    asl a
    adc #HT_PL_A
    bra @put
@blank:
    lda #HT_PL_BLANK
@put:
    clc
    adc t2
    jsr hud_put2
    inx
    inx
    inc t4
    lda t4
    cmp #3
    bne @ch
    lda #HT_PL_BLANK
    clc
    adc t2
    jsr hud_put2
    inx
    inx
    lda #HT_PL_CAPR
    clc
    adc t2
    jsr hud_put2
    lda #1
    sta bg3_dirty
    rts

; hud_big : A = chiffre (0..9, $FF = vide), X = position (ligne du haut)
hud_big:
    .a16
    .i16
    cmp #$00FF
    bne :+
    lda #HT_SB_BLANK + HUD_SB
    jmp hud_put2
:   asl a
    clc
    adc #HT_SB_0 + HUD_SB
    jmp hud_put2

; hud_score : A = score, X = position des dizaines, t5 = 0 aligne a droite, 1 a gauche
hud_score:
    .a16
    .i16
    cmp #100
    bcc :+
    lda #99
:   phx
    ldx #10
    jsr divu
    plx
    sta t3                      ; dizaines
    lda RDMPYL
    sta t4                      ; unites
    lda t3
    bne @two
    lda t5
    bne @left
    lda #$FF
    jsr hud_big
    inx
    inx
    lda t4
    jmp hud_big
@left:
    lda t4
    jsr hud_big
    inx
    inx
    lda #$FF
    jmp hud_big
@two:
    jsr hud_big
    inx
    inx
    lda t4
    jmp hud_big

hud_update:
    .a16
    .i16
    ; scores (gros chiffres)
    lda score
    cmp hud_cache
    beq :+
    sta hud_cache
    stz t5
    ldx #TPOS(12, 0)
    jsr hud_score
    lda #1
    sta bg3_dirty
:   lda score+2
    cmp hud_cache+2
    beq :+
    sta hud_cache+2
    lda #1
    sta t5
    lda score+2
    ldx #TPOS(18, 0)
    jsr hud_score
    lda #1
    sta bg3_dirty
:   ; horloge m:ss
    lda #HUD_ATTR
    sta t0
    lda m_sec
    cmp hud_cache+4
    beq @msg
    sta hud_cache+4
    ldx #60
    jsr divu
    sta t3                      ; minutes
    lda RDMPYL
    sta t4                      ; secondes
    lda t3
    ldx #TPOS(12, 2)
    jsr print_digit
    ldx #TPOS(13, 2)
    lda #(':' - 32 + HUD_ATTR)
    sta bg3_map,x
    lda t4
    ldx #10
    jsr divu
    sta t3
    lda RDMPYL
    sta t4
    lda t3
    ldx #TPOS(14, 2)
    jsr print_digit
    lda t4
    ldx #TPOS(15, 2)
    jsr print_digit
@msg:
.if DEBUG
    ; DEBUG=1 : nombre d'OBJ de la frame precedente, porteur, etat du match (ligne 27)
    lda #HUD_ATTR
    sta t0
    lda oam_used
    ldx #TPOS(22, 27)
    jsr print_num2
    lda b_owner
    and #$00FF
    cmp #100
    bcc :+
    lda #99
:   ldx #TPOS(25, 27)
    jsr print_num2
    lda m_state
    ldx #TPOS(28, 27)
    jsr print_digit
    lda ko_active
    ldx #TPOS(30, 27)
    jsr print_digit
.endif
    ; effacement du message
    lda msg_time
    beq @d
    sec
    sbc rc+RC_TDEC
    bpl :+
    lda #0
:   sta msg_time
    bne @d
    jsr clear_msg
@d: rts

; clear_msg : efface le bandeau des messages (3 lignes)
clear_msg:
    .a16
    .i16
    ldx #TPOS(0, MSG_ROW - 1)
    ldy #96
    lda #0
    jsr fill_tiles
    rts

; -----------------------------------------------------------------------------
;  Bandeau des messages du match (3 lignes centrees sur MSG_ROW) :
;    show_msg  : Y = message (cadre cyan)
;    show_tmsg : A = equipe, X = joueur ($FF aucun), Y = message
;                -> cadre a la couleur de l'equipe, ">> EUR  MESSAGE  NOM"
; -----------------------------------------------------------------------------
show_msg:
    .a16
    .i16
    lda #$FF
    sta bn_team
    sta bn_pl
    bra show_banner
show_tmsg:
    .a16
    .i16
    sta bn_team
    stx bn_pl
show_banner:
    jsr tr_str
    sty bn_msg
    jsr clear_msg
    stz bn_n
    lda #TXT_ATTR + $1400       ; palette 5 (cyan)
    ldx bn_team
    cpx #$FF
    beq :+
    lda #TXT_ATTR + $0400       ; palette 1 / 3 : couleur de l'equipe
    cpx #0
    beq :+
    lda #TXT_ATTR + $0C00
:   sta bn_attr
    ; equipe : chevron + nom court
    lda bn_team
    cmp #$FF
    beq @msg
    lda #HT_BN_CHEV
    clc
    adc bn_attr
    jsr bn_put
    lda #' '
    jsr bn_chr_team
    lda bn_team
    jsr team_rec
    tya
    clc
    adc #T_SHORT
    tay
    ldx #4
@tn:
    lda a:0,y
    and #$00FF
    beq @tnd
    phy
    phx
    jsr bn_chr_team
    plx
    ply
    iny
    dex
    bne @tn
@tnd:
    lda #' '
    jsr bn_chr_team
    lda #' '
    jsr bn_chr_team
@msg:
    ldy bn_msg
@ml:
    lda a:0,y
    and #$00FF
    beq @mld
    clc
    adc #TXT_PANEL - 32
    clc
    adc #TXT_ATTR
    phy
    jsr bn_put
    ply
    iny
    bra @ml
@mld:
    ; joueur
    lda bn_pl
    cmp #$FF
    beq @draw
    jsr bn_player_name
@draw:
    ; largeur totale = contenu + 4, colonne de depart centree
    lda bn_n
    clc
    adc #4
    sta bn_w
    lda #32
    sec
    sbc bn_w
    lsr a
    asl a
    sta bn_x                    ; offset en octets dans la ligne
    ; ligne du haut
    clc
    adc #TPOS(0, MSG_ROW - 1)
    tax
    lda #HT_BN_TL
    ldy #HT_BN_T
    jsr bn_row
    lda bn_x
    clc
    adc #TPOS(0, MSG_ROW + 1)
    tax
    lda #HT_BN_BL
    ldy #HT_BN_B
    jsr bn_row
    ; ligne du milieu
    lda bn_x
    clc
    adc #TPOS(0, MSG_ROW)
    tax
    lda #HT_BN_L
    clc
    adc bn_attr
    sta bg3_map,x
    inx
    inx
    lda #TXT_PANEL
    clc
    adc bn_attr
    sta bg3_map,x
    inx
    inx
    ldy #0
@c: cpy bn_n
    beq @cd
    phy
    tya
    asl a
    tay
    lda bn_line,y
    ply
    sta bg3_map,x
    inx
    inx
    iny
    bra @c
@cd:
    lda #TXT_PANEL
    clc
    adc bn_attr
    sta bg3_map,x
    inx
    inx
    lda #HT_BN_R
    clc
    adc bn_attr
    sta bg3_map,x
    lda #1
    sta bg3_dirty
    lda #T_MSG
    sta msg_time
    rts

; bn_row : X = position, A = tile du coin gauche (coin droit = +2), Y = tile du bord
bn_row:
    .a16
    .i16
    sty bn_t
    pha
    clc
    adc bn_attr
    sta bg3_map,x
    inx
    inx
    ldy bn_w
    dey
    dey
@r: lda bn_t
    clc
    adc bn_attr
    sta bg3_map,x
    inx
    inx
    dey
    bne @r
    pla
    clc
    adc #2
    adc bn_attr
    sta bg3_map,x
    rts

; bn_put : A = tile -> contenu du bandeau (28 au plus)
bn_put:
    .a16
    .i16
    ldx bn_n
    cpx #28
    bcs :+
    pha
    txa
    asl a
    tax
    pla
    sta bn_line,x
    inc bn_n
:   rts

; bn_chr_team : A = caractere -> police opaque, couleur de l'equipe
bn_chr_team:
    .a16
    clc
    adc #TXT_PANEL - 32
    clc
    adc bn_attr
    jmp bn_put

; bn_player_name : nom du joueur bn_pl (8 caracteres au plus, sans espaces finaux), en or
bn_player_name:
    .a16
    .i16
    lda bn_pl
    asl a
    tax
    jsr lineup_index
    lda lineup,y
    pha
    lda bn_pl
    asl a
    tax
    lda p_team,x
    plx
    jsr roster_rec              ; Y = enregistrement, nom en +0
    ; longueur utile
    sty bn_t
    ldx #0
    stx bn_w
@len:
    lda a:0,y
    and #$00FF
    beq @ld
    cmp #' '
    beq :+
    inx
    stx bn_w                    ; dernier caractere non blanc
    dex
:   inx
    iny
    cpx #8
    bne @len
@ld:
    lda bn_w
    beq @d
    lda #' '
    jsr bn_chr_team
    lda #' '
    jsr bn_chr_team
    ldy bn_t
@cp:
    lda a:0,y
    and #$00FF
    clc
    adc #TXT_PANEL - 32
    clc
    adc #TXT_ATTR + $1000       ; palette 4 : or
    phy
    jsr bn_put
    ply
    iny
    dec bn_w
    bne @cp
@d: rts

; hud_full_time : ecran de fin de match
hud_full_time:
    .a16
    .i16
    ldy #.loword(str_fulltime)
    jsr show_msg
    stz msg_time                ; reste affiche jusqu'a l'ecran des statistiques
    lda #T_GOAL
    sta m_timer
    rts

; stat_line : Y = libelle, t6 / t7 = valeurs, ligne hs_row (puis +1)
stat_line:
    .a16
    .i16
    phy
    lda hs_row
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    sta near_tmp+2
    clc
    adc #8 * 2
    tax
    ldy #16
    lda #HUD_ATTR
    jsr fill_tiles
    lda #HUD_ATTR
    sta t0
    lda near_tmp+2
    clc
    adc #13 * 2
    tax
    ply
    jsr print
    lda t6
    cmp #100
    bcc :+
    lda #99
:   pha
    lda near_tmp+2
    clc
    adc #9 * 2
    tax
    pla
    jsr print_num2
    lda t7
    cmp #100
    bcc :+
    lda #99
:   pha
    lda near_tmp+2
    clc
    adc #20 * 2
    tax
    pla
    jsr print_num2
    inc hs_row
    rts

; poss_pct : t6 / t7 = pourcentages de possession
poss_pct:
    .a16
    .i16
    lda st_poss
    sta t6
    lda st_poss+2
    sta t7
@sc:
    lda t6
    clc
    adc t7
    cmp #256
    bcc @ok
    lsr t6
    lsr t7
    bra @sc
@ok:
    sta t5
    beq @zero
    lda t6
    ldy #100
    jsr mulu8
    ldx t5
    jsr divu
    sta t6
    lda #100
    sec
    sbc t6
    sta t7
    rts
@zero:
    lda #50
    sta t6
    sta t7
    rts

; -----------------------------------------------------------------------------
;  crowd_update : rotation des 3 couleurs du public (plus rapide apres un
;  evenement : but, arret, charge)
; -----------------------------------------------------------------------------
crowd_update:
    .a16
    .i16
    ; anneaux : clignotement pendant la celebration d'un but
    lda m_state
    cmp #MS_GOAL
    bne @ring_n
    lda frame
    and #$0004
    beq @ring_n
    lda #$7FFF
    sta ring_pal
    lda ring_base
    sta ring_pal+2
    bra @ring_d
@ring_n:
    lda ring_base
    sta ring_pal
    lda ring_base+2
    sta ring_pal+2
@ring_d:
    lda #1
    sta pal_dirty
    lda crowd_fast
    beq :+
    sec
    sbc rc+RC_TDEC
    bpl :+
    lda #0
:   sta crowd_fast
    lda crowd_t
    clc
    adc rc+RC_TDEC
    sta crowd_t
    ldy #100
    lda crowd_fast
    beq :+
    ldy #18
:   tya
    cmp crowd_t
    bcs @d
    stz crowd_t
    lda crowd_pal
    pha
    lda crowd_pal+2
    sta crowd_pal
    lda crowd_pal+4
    sta crowd_pal+2
    pla
    sta crowd_pal+4
    lda #1
    sta pal_dirty
@d: rts

.segment "RODATA"
str_1st:        .byte "1ST", 0
str_2nd:        .byte "2ND", 0
str_ot:         .byte "OT ", 0
str_so:         .byte "SO ", 0
half_names:     .word .loword(str_1st), .loword(str_2nd), .loword(str_ot), .loword(str_so)
str_so_res:     .byte "SHOOTOUT ", 0
str_st_shots:   .byte "SHOTS", 0
str_st_poss:    .byte "POSS.", 0
str_st_fouls:   .byte "FOULS", 0
str_st_saves:   .byte "SAVES", 0
str_fulltime:   .byte "FULL TIME", 0
str_press_start: .byte "PRESS START", 0
.segment "CODE"
