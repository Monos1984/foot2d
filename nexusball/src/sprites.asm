; =============================================================================
;  sprites.asm - construction de l'OAM : curseurs, joueurs et ballon
;  tries par profondeur (y), ombres.
;  Ordre OAM = ordre d'affichage (le premier est devant) et priorite en cas de
;  surcharge : curseurs, ballon, joueurs (par profondeur), ombre.
; =============================================================================

OBJ_PRIO   = $3000              ; priorite 3
PAL_BALL   = $0800              ; palette OBJ 4
ENT_BALL   = NUM_PLAYERS

build_sprites:
    .a16
    .i16
    ; ordre OAM = priorite : en cas de surcharge d'une ligne (32 sprites / 34 tiles),
    ; la PPU abandonne les derniers : curseurs et ballon d'abord.
    jsr oam_begin
    jsr draw_cursors
.if DEBUG
    ldx #0 * 2
    jsr prof_mark
.endif
    jsr draw_ball
    jsr draw_sparks
.if DEBUG
    ldx #1 * 2
    jsr prof_mark
.endif
    jsr sort_entities
.if DEBUG
    ldx #2 * 2
    jsr prof_mark
.endif
    ; joueurs, du plus proche (y grand) au plus lointain
    ldy #0
@l: phy
    lda sort_buf,y
    and #$00FF
    cmp #ENT_BALL
    beq @n
    asl a
    tax
    jsr draw_player
@n: ply
    iny
    cpy #NUM_PLAYERS + 1
    bne @l
    jsr draw_ball_shadow
.if DEBUG
    ldx #4 * 2
    jsr prof_mark
.endif
    jsr oam_finish
.if DEBUG
    ldx #5 * 2
    jsr prof_mark
.endif
    rts

; -----------------------------------------------------------------------------
;  sort_entities : tri par insertion des 13 entites selon y decroissant
; -----------------------------------------------------------------------------
sort_entities:
    .a16
    .i16
    ; cles : sort_key[i]
    ldx #0
@k: lda p_y,x
    sta sort_key,x
    inx
    inx
    cpx #NUM_PLAYERS*2
    bne @k
    lda b_y
    ldy b_owner
    cpy #NO_OWNER
    beq :+
    clc
    adc #FP / 2                 ; le ballon tenu passe devant le porteur
:   sta sort_key,x
    ; ordre de la frame precedente conserve : presque trie -> insertion quasi lineaire
    lda sort_ok
    bne @ins
    inc sort_ok
    sep #$20
    .a8
    ldx #0
@i: txa
    sta sort_buf,x
    inx
    cpx #NUM_PLAYERS + 1
    bne @i
    rep #$20
    .a16
@ins:
    ; insertion
    ldx #1
@outer:
    lda sort_buf,x
    and #$00FF
    sta t0                      ; element
    asl a
    tay
    lda sort_key,y
    sta t1                      ; cle
    txy
@inner:
    cpy #0
    beq @place
    lda sort_buf-1,y
    and #$00FF
    asl a
    phx
    tax
    lda sort_key,x
    plx
    cmp t1
    bcs @place                  ; deja plus grand ou egal : stop
    sep #$20
    .a8
    lda sort_buf-1,y
    sta sort_buf,y
    rep #$20
    .a16
    dey
    bra @inner
@place:
    sep #$20
    .a8
    lda t0
    sta sort_buf,y
    rep #$20
    .a16
    inx
    cpx #NUM_PLAYERS + 1
    bne @outer
    rts

; -----------------------------------------------------------------------------
;  draw_player : X = joueur*2
; -----------------------------------------------------------------------------
draw_player:
    .a16
    .i16
    lda p_state,x
    cmp #PS_OUT
    bne :+
    rts
:   ; palette : equipe (0/1), gardien (2/3), peau foncee (5/6)
    lda p_team,x
    ldy p_role,x
    bne :+
    ora #2
    bra :++
:   ldy p_skin,x
    beq :+
    clc
    adc #5
:   asl a
    xba                         ; palette << 9
    ora #OBJ_PRIO
    ora p_flip,x
    ora p_spr,x
    sta t1
    lda #1
    sta t2
    lda p_y,x
    sec
    sbc p_z,x
    ASR_A 4
    sec
    sbc scroll_y
    sec
    sbc #FEET_ROW + 1
    sta t0
    lda p_x,x
    ASR_A 4
    sec
    sbc scroll_x
    sec
    sbc #8
    sta dp_x
    ; moitie basse (jambes, ombre) puis moitie haute
    lda t0
    pha
    clc
    adc #16
    sta t0
    lda t1
    pha
    clc
    adc #32
    sta t1
    lda dp_x
    jsr oam_add
    pla
    sta t1
    pla
    sta t0
    lda dp_x
    jmp oam_add

; -----------------------------------------------------------------------------
;  draw_ball / draw_ball_shadow
; -----------------------------------------------------------------------------
draw_ball:
    .a16
    .i16
    ; orientation : direction du porteur, sinon vitesse du ballon
    lda b_owner
    cmp #NO_OWNER
    beq @free
    asl a
    tax
    lda p_dir,x
    and #$0003
    sta b_orient
    stz t5                      ; phase
    bra @spr
@free:
    lda b_vx
    ABS_A
    sta t6
    lda b_vy
    ABS_A
    sta t7
    ora t6
    cmp #4
    bcc @still                  ; immobile : orientation conservee
    lda t7
    asl a
    cmp t6
    bcs :+
    stz b_orient                ; horizontal
    bra @ph
:   lda t6
    asl a
    cmp t7
    bcs :+
    lda #2                      ; vertical
    sta b_orient
    bra @ph
:   lda b_vx
    eor b_vy
    bmi :+
    lda #1                      ; diagonale descendante
    sta b_orient
    bra @ph
:   lda #3
    sta b_orient
@ph:
    lda frame
    lsr a
    lsr a
    and #$0001
    sta t5
    bra @spr
@still:
    stz t5
@spr:
    lda b_orient
    asl a
    clc
    adc t5
    asl a
    clc
    adc #SPR_OVAL | PAL_BALL | OBJ_PRIO
    sta t1
    lda #1
    sta t2
    lda b_y
    sec
    sbc b_z
    ASR_A 4
    sec
    sbc scroll_y
    sec
    sbc #12
    sta t0
    lda b_x
    ASR_A 4
    sec
    sbc scroll_x
    sec
    sbc #8
    jsr oam_add
    ; trainee neon derriere un ballon libre et rapide
    lda b_owner
    cmp #NO_OWNER
    bne @nt
    lda b_vx
    ABS_A
    sta t6
    lda b_vy
    ABS_A
    clc
    adc t6
    cmp #TRAIL_SPEED
    bcc @nt
    lda #SPR_TRAIL1 | PAL_BALL | OBJ_PRIO
    ldy #1                      ; 1 puis 2 frames de trajectoire en arriere
    jsr ball_trail
    lda #SPR_TRAIL2 | PAL_BALL | OBJ_PRIO
    ldy #2
    jsr ball_trail
@nt:
    rts

; ball_trail : A = tile + attributs, Y = recul (en frames de vitesse)
ball_trail:
    .a16
    .i16
    sta t1
    stz t2
    sty t6
    ; y = b_y - b_z - vy * recul
    lda #0
    ldx t6
:   clc
    adc b_vy
    dex
    bne :-
    sta t7
    lda b_y
    sec
    sbc b_z
    sec
    sbc t7
    ASR_A 4
    sec
    sbc scroll_y
    sec
    sbc #8
    sta t0
    lda #0
    ldx t6
:   clc
    adc b_vx
    dex
    bne :-
    sta t7
    lda b_x
    sec
    sbc t7
    ASR_A 4
    sec
    sbc scroll_x
    sec
    sbc #4
    jmp oam_add

; draw_sparks : gerbe d'etincelles qui s'ecarte de l'anneau apres un but (8 x 8x8)
SPK_T = 28

draw_sparks:
    .a16
    .i16
    lda spk_t
    beq @d
    dec spk_t
    lda #SPK_T
    sec
    sbc spk_t
    asl a
    asl a                       ; rayon x2 (pour smul)
    sta spk_r
    stz spk_i
@l: ldy spk_i
    ; tile : grande trace puis petite, scintillement en fin de vie
    lda #SPR_TRAIL1 | PAL_BALL | OBJ_PRIO
    ldx spk_t
    cpx #SPK_T / 2
    bcs :+
    lda frame
    and #$0001
    bne @n
    lda #SPR_TRAIL2 | PAL_BALL | OBJ_PRIO
:   sta t1
    stz t2
    lda spk_dy,y
    and #$00FF
    sta t7
    lda spk_r
    jsr smul
    clc
    adc spk_y
    sec
    sbc scroll_y
    sec
    sbc #4
    sta t0
    ldy spk_i
    lda spk_dx,y
    and #$00FF
    sta t7
    lda spk_r
    jsr smul
    clc
    adc spk_x
    sec
    sbc scroll_x
    sec
    sbc #4
    jsr oam_add
@n: inc spk_i
    lda spk_i
    cmp #8
    bne @l
@d: rts

.segment "RODATA"
spk_dx: .byte 120, 85, 0, <(-85), <(-120), <(-85), 0, 85
spk_dy: .byte 0, 85, 120, 85, 0, <(-85), <(-120), <(-85)
.segment "CODE"

draw_ball_shadow:
    .a16
    lda #SPR_OSHADOW | PAL_BALL | $2000
    sta t1
    lda #1
    sta t2
    lda b_y
    ASR_A 4
    sec
    sbc scroll_y
    sec
    sbc #10
    sta t0
    lda b_x
    ASR_A 4
    sec
    sbc scroll_x
    sec
    sbc #8
    jsr oam_add
    rts

; -----------------------------------------------------------------------------
;  draw_cursors : fleche au-dessus du joueur controle (clignote apres 3 s de port)
; -----------------------------------------------------------------------------
draw_cursors:
    .a16
    .i16
    ldy #0
@pad:
    lda ctrl,y
    cmp #NO_OWNER
    jeq @n
    lda m_state
    cmp #MS_END
    jeq @n
    lda ctrl,y
    asl a
    tax
    lda p_state,x
    cmp #PS_OUT
    jeq @n
    lda ctrl,y
    cmp b_owner
    bne @show
    lda b_carry
    cmp #T_CARRY_WARN
    bcc @show
    lda frame
    and #$0004
    jeq @n
@show:
    phy
    lda ctrl,y
    asl a
    tax
    lda #SPR_CUR1 | PAL_BALL | OBJ_PRIO
    cpy #0
    beq :+
    lda #SPR_CUR2 | PAL_BALL | OBJ_PRIO
:   sta t1
    stz t2
    lda p_y,x
    sec
    sbc p_z,x
    ASR_A 4
    sec
    sbc scroll_y
    sec
    sbc #FEET_ROW + 9
    sta t0
    lda p_x,x
    ASR_A 4
    sec
    sbc scroll_x
    sec
    sbc #4
    jsr oam_add
    ply
    jsr pass_marker
@n: iny
    iny
    cpy #4
    jne @pad
    rts

; pass_marker : Y = manette. Porteur humain (joueur de champ) en jeu : fleche a la couleur
; de la manette sous les pieds du coequipier qui recevrait une passe a la main. Preserve Y.
pass_marker:
    .a16
    .i16
    lda m_state
    cmp #MS_PLAY
    bne @d
    lda ctrl,y
    cmp b_owner
    bne @d
    asl a
    tax
    lda p_role,x
    beq @d                      ; gardien : relance a part
    lda frame
    and #$0008
    beq @d                      ; clignote
    phy
    stx cp
    jsr find_hand_target
    ply
    cmp #NO_OWNER
    beq @d
    asl a
    tax
    lda #SPR_CUR1 | PAL_BALL | OBJ_PRIO | $8000
    cpy #0
    beq :+
    lda #SPR_CUR2 | PAL_BALL | OBJ_PRIO | $8000
:   sta t1
    stz t2
    lda p_y,x
    ASR_A 4
    sec
    sbc scroll_y
    clc
    adc #2
    sta t0
    lda p_x,x
    ASR_A 4
    sec
    sbc scroll_x
    sec
    sbc #4
    phy
    jsr oam_add
    ply
@d: rts

