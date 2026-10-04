; =============================================================================
;  region.asm - tables de constantes NTSC / PAL et copie en WRAM (rc)
;  Usage : lda rc+RC_GRAV
; =============================================================================

.segment "RODATA"

RC_PASS_BUILD .set 0

.macro RCR sym, vn, vp
    .if RC_PASS_BUILD = 0
        sym = * - rc_table_ntsc
        .word vn
    .else
        .word vp
    .endif
.endmacro
.macro RCV sym, v
    RCR sym, v, ((v) * 6 + 2) / 5
.endmacro
.macro RCA sym, v
    RCR sym, v, ((v) * 36 + 12) / 25
.endmacro

rc_table_ntsc:
.include "region.inc"
rc_table_ntsc_end:
RC_PASS_BUILD .set 1
rc_table_pal:
.include "region.inc"

RC_SIZE = rc_table_ntsc_end - rc_table_ntsc
.assert RC_SIZE <= 128, error, "table de region trop grande"

.segment "CODE"

; init_region : copie la table de la region detectee dans rc
init_region:
    php
    rep #$30
    .a16
    .i16
    ldy #.loword(rc_table_ntsc)
    lda is_pal
    beq :+
    ldy #.loword(rc_table_pal)
:   ldx #0
@l: lda a:0,y
    sta rc,x
    iny
    iny
    inx
    inx
    cpx #RC_SIZE
    bne @l
    plp
    rts
