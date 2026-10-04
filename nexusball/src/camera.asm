; =============================================================================
;  camera.asm - camera qui suit le ballon avec anticipation et amortissement
; =============================================================================

; camera_target : t0 / t1 = point vise (12.4)
camera_target:
    .a16
    lda b_vx
    asl a
    asl a
    asl a                       ; ~8 frames d'avance
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
    clc
    adc t1
    sta t1
    rts

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
    lda t0
    sec
    sbc cam_x
    ASR_A 4
    clc
    adc cam_x
    sta cam_x
    lda t1
    sec
    sbc cam_y
    ASR_A 4
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
    ; tribunes (BG2) : parallaxe horizontale (moitie de la vitesse)
    sta bg2_vofs
    lda scroll_x
    lsr a
    sta bg2_hofs
    rts
