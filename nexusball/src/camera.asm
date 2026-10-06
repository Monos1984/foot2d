; =============================================================================
;  camera.asm - camera qui suit le ballon avec anticipation et amortissement
; =============================================================================

; camera_target : t0 / t1 = point vise (12.4)
camera_target:
    .a16
    ; avance = 133 ms de trajectoire dans les deux regions : 8 frames a 60 Hz,
    ; vitesse * 8 * 5/6 a 50 Hz (les vitesses PAL par frame sont 6/5 plus grandes)
    lda b_vx
    asl a
    asl a
    asl a
    jsr cam_pal
    clc
    adc b_x
    sta t0
    lda b_y
    sec
    sbc b_z
    sta t1
    lda b_vy
    asl a
    asl a
    jsr cam_pal
    clc
    adc t1
    sta t1
    rts

; cam_pal : A (signe) -> A * 5/6 en PAL (a - a/8 - a/32 - a/128), inchange en NTSC
cam_pal:
    .a16
    ldx is_pal
    beq @d
    pha
    ASR_A 3
    sta cam_t
    ASR_A 2
    clc
    adc cam_t
    sta cam_t
    pla
    pha
    ASR_A 7
    clc
    adc cam_t
    sta cam_t
    pla
    sec
    sbc cam_t
@d: rts

camera_snap:
    .a16
    jsr camera_target
    lda t0
    sta cam_x
    lda t1
    sta cam_y
    bra camera_scroll

camera_update:
    .a16
    jsr camera_target
    ; amortissement : 1/16 de l'ecart par frame a 60 Hz, ~1/13 a 50 Hz (meme temps reel)
    lda t0
    sec
    sbc cam_x
    jsr cam_damp
    clc
    adc cam_x
    sta cam_x
    lda t1
    sec
    sbc cam_y
    jsr cam_damp
    clc
    adc cam_y
    sta cam_y
camera_scroll:
    lda cam_x
    ASR_A 4
    sec
    sbc #128
    bpl :+
    lda #0
:   cmp #CAM_MAX_X
    bcc :+
    lda #CAM_MAX_X
:   sta scroll_x
    lda cam_y
    ASR_A 4
    sec
    sbc #120
    bpl :+
    lda #0
:   cmp #CAM_MAX_Y
    bcc :+
    lda #CAM_MAX_Y
:   sta scroll_y
    ; tremblement de l'ecran (but) : 2 px verticaux une frame sur deux, vers l'interieur
    lda shake_t
    beq @ns
    dec shake_t
    lda frame
    and #$0002
    beq @ns
    lda scroll_y
    cmp #2
    bcs :+
    adc #2
    bra :++
:   sbc #2
:   sta scroll_y
@ns:
    lda scroll_y
    ; tribunes (BG2) : parallaxe horizontale (moitie de la vitesse)
    sta bg2_vofs
    lda scroll_x
    lsr a
    sta bg2_hofs
    rts

; cam_damp : A = ecart (signe) -> A/16 (NTSC) ou A/16 + A/64 (PAL)
cam_damp:
    .a16
    ASR_A 4
    ldx is_pal
    beq :+
    pha
    ASR_A 2
    sta cam_t
    pla
    clc
    adc cam_t
:   rts
