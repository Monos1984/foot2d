; =============================================================================
;  input.asm - lecture des manettes (lecture automatique materielle)
; =============================================================================

read_pads:
    .a16
    .i16
    sep #$20
    .a8
@w: lda HVBJOY
    and #$01
    bne @w                      ; lecture auto en cours
    rep #$20
    .a16
    lda joy_cur
    sta joy_old
    lda joy_cur+2
    sta joy_old+2
    lda JOY1L
    bit #$000F                  ; pas une manette standard
    beq :+
    lda #0
:   sta joy_cur
    lda JOY2L
    bit #$000F
    beq :+
    lda #0
:   sta joy_cur+2
    lda joy_old
    eor #$FFFF
    and joy_cur
    sta joy_new
    lda joy_old+2
    eor #$FFFF
    and joy_cur+2
    sta joy_new+2
    rts
