; =============================================================================
;  math.asm - aleatoire, multiplications materielles, directions, distances
; =============================================================================

; rand : A = nombre pseudo-aleatoire 16 bits (xorshift)
rand:
    .a16
    lda rng
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    eor rng
    sta rng
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    eor rng
    sta rng
    xba
    and #$FF00
    eor rng
    sta rng
    rts

; smul : A (signe 16) * t7 (octet signe) -> A = produit >> 8 (signe)
smul:
    .a16
    sep #$20
    .a8
    sta M7A
    xba
    sta M7A
    lda t7
    sta M7B
    rep #$20
    .a16
    lda MPYM
    rts

; smul_floor0 : comme smul mais arrondi vers zero (utile pour les frottements)
; A = valeur, t7 = facteur positif. Renvoie A = A*t7/256, au moins 1 si A != 0
fric_amount:
    .a16
    pha
    jsr smul
    sta t6
    pla
    beq @z
    bmi @neg
    lda t6
    bne @r
    lda #1
@r: rts
@neg:
    lda t6
    inc a                       ; arrondi vers zero du negatif
    cmp #0
    bne @r2
    lda #.loword(-1)
@r2:
    rts
@z: lda #0
    rts

; apply_friction : A = vitesse, t7 = facteur -> A = vitesse freinee
apply_friction:
    .a16
    sta t5
    jsr fric_amount
    eor #$FFFF
    sec
    adc t5
    rts

; mulu8 : A (octet bas) * Y (octet bas) -> A (16 bits non signe)
mulu8:
    .a16
    .i16
    sep #$20
    .a8
    sta WRMPYA
    tya
    sta WRMPYB
    nop                         ; 8 cycles d'attente
    nop
    nop
    nop
    rep #$20
    .a16
    lda RDMPYL
    rts

; divu : A (16 bits) / X (octet, non nul) -> A quotient
divu:
    .a16
    .i16
    sta WRDIVL
    sep #$20
    .a8
    txa
    sta WRDIVB
    rep #$20                    ; 16 cycles d'attente
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    .a16
    lda RDDIVL
    rts

; -----------------------------------------------------------------------------
;  dist_approx : t0 = dx, t1 = dy (pixels signes) -> A ~ sqrt(dx2+dy2)
;  max + min*3/8 (erreur < 7 %)
; -----------------------------------------------------------------------------
dist_approx:
    .a16
    lda t0
    ABS_A
    sta t4
    lda t1
    ABS_A
    cmp t4
    bcc @a                      ; |dy| < |dx|
    ldx t4                      ; min = |dx|, max = |dy|
    sta t4
    txa
@a: ; A = min, t4 = max
    lsr a
    lsr a
    sta t5
    lsr a
    clc
    adc t5
    adc t4
    rts

; -----------------------------------------------------------------------------
;  atan64 : t0 = dx, t1 = dy (signes) -> A = direction 0..63
;  0 = droite, 16 = bas, 32 = gauche, 48 = haut
; -----------------------------------------------------------------------------
atan64:
    .a16
    .i16
    lda t0
    ABS_A
    sta t4
    lda t1
    ABS_A
    sta t5
@scale:
    lda t4
    ora t5
    and #$FF00
    beq @fit
    lsr t4
    lsr t5
    bra @scale
@fit:
    lda t4
    ora t5
    bne :+
    lda #0
    rts
:   lda t4
    cmp t5
    bcc @steep
    ; |dx| >= |dy| : a = atan(dy/dx)
    lda t5
    asl a
    asl a
    asl a
    asl a
    asl a
    ldx t4
    jsr divu
    tax
    lda atan_tab,x
    and #$00FF
    bra @quad
@steep:
    lda t4
    asl a
    asl a
    asl a
    asl a
    asl a
    ldx t5
    jsr divu
    tax
    lda atan_tab,x
    and #$00FF
    eor #$FFFF
    sec
    adc #16
@quad:
    sta t4                      ; angle 0..16 dans le premier quadrant
    lda t0
    bmi @left
    lda t1
    bmi @q4
    lda t4                      ; dx>=0, dy>=0
    rts
@q4:
    lda #64
    sec
    sbc t4
    and #$003F
    rts
@left:
    lda t1
    bmi @q3
    lda #32
    sec
    sbc t4
    rts
@q3:
    lda #32
    clc
    adc t4
    rts

; -----------------------------------------------------------------------------
;  vel_from_dir : A = direction 0..63, t2 = vitesse (0..255)
;  -> t0 = vx, t1 = vy  (vitesse * cos / 128)
; -----------------------------------------------------------------------------
vel_from_dir:
    .a16
    .i16
    and #$003F
    tax
    sep #$20
    .a8
    lda cos_tab,x
    sta t7
    rep #$20
    .a16
    lda t2
    asl a
    jsr smul
    sta t0
    txa
    sec
    sbc #16
    and #$003F
    tax
    sep #$20
    .a8
    lda cos_tab,x
    sta t7
    rep #$20
    .a16
    lda t2
    asl a
    jsr smul
    sta t1
    rts

; dpad_dir : A = bits manette -> A = direction 0..7 ou $FFFF si aucune
dpad_dir:
    .a16
    xba
    and #$000F
    tax
    lda dpad_tab,x
    and #$00FF
    cmp #$00FF
    bne :+
    lda #$FFFF
:   rts

; -----------------------------------------------------------------------------
.segment "RODATA"
; cos_tab (64 octets signes, x127) et atan_tab (r = 0..32) : generes par tools/gfx.py
.include "data/gen/tables.inc"
; bits (haut,bas,gauche,droite) -> direction 0..7
dpad_tab:
    .byte $FF,0,4,$FF, 2,1,3,2, 6,7,5,6, $FF,0,4,$FF
; vecteur unitaire par direction 0..7 (x 127)
dir8_x:
    .byte 127, 90, 0, <-90, <-127, <-90, 0, 90
dir8_y:
    .byte 0, 90, 127, 90, 0, <-90, <-127, <-90
.segment "CODE"
