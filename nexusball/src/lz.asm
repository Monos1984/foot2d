; =============================================================================
;  lz.asm - decompression LZSS (format : tools/lz.py) vers la WRAM $7F:0000,
;  puis transfert DMA vers la VRAM. A utiliser ecran eteint (ou en VBlank).
; =============================================================================

LZ_BUF = $7F0000

; LZ_SRC label : prepare la source compressee (A16)
.macro LZ_SRC src
    lda #.loword(src)
    sta lz_src
    lda #^src
    sta lz_src+2
.endmacro

; -----------------------------------------------------------------------------
;  lz_decompress : lz_src -> $7F:0000. Renvoie A (16 bits) = taille.
; -----------------------------------------------------------------------------
lz_decompress:
    php
    phb
    rep #$30
    .a16
    .i16
    lda [lz_src]
    sta lz_size
    lda lz_src
    clc
    adc #2
    sta lz_src
    sep #$20
    .a8
    lda #^LZ_BUF
    pha
    plb
    ldx #0
@group:
    cpx lz_size
    bcs @done
    lda [lz_src]
    sta lz_flags
    jsr @inc
    lda #8
    sta lz_cnt
@bit:
    cpx lz_size
    bcs @done
    lsr lz_flags
    bcc @match
    lda [lz_src]
    jsr @inc
    sta a:0,x
    inx
    bra @next
@match:
    lda [lz_src]
    sta lz_d
    jsr @inc
    lda [lz_src]
    jsr @inc
    pha
    and #$0F
    clc
    adc #3
    sta lz_len
    pla
    lsr a
    lsr a
    lsr a
    lsr a
    sta lz_d+1
    rep #$20
    .a16
    txa
    sec
    sbc lz_d
    tay
    sep #$20
    .a8
@cp:
    lda a:0,y
    sta a:0,x
    iny
    inx
    dec lz_len
    bne @cp
@next:
    dec lz_cnt
    bne @bit
    bra @group
@done:
    plb
    plp
    lda lz_size
    rts

@inc:
    inc lz_src
    bne :+
    inc lz_src+1
:   rts

; -----------------------------------------------------------------------------
;  lz_vram : lz_src = donnees compressees, X = adresse VRAM (mots). A16/I16.
; -----------------------------------------------------------------------------
lz_vram:
    .a16
    .i16
    phx
    jsr lz_decompress
    sta t0
    plx
    sep #$20
    .a8
    lda #$80
    sta VMAIN
    stx VMADDL
    lda #$01
    sta DMAP0
    lda #$18
    sta BBAD0
    ldx #.loword(LZ_BUF)
    stx A1T0L
    lda #^LZ_BUF
    sta A1B0
    ldx t0
    stx DAS0L
    lda #$01
    sta MDMAEN
    rep #$20
    .a16
    rts
