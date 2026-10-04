; NEXUS BALL - macros

.macro A8
    sep #$20
.endmacro
.macro A16
    rep #$20
.endmacro
.macro I8
    sep #$10
.endmacro
.macro I16
    rep #$10
.endmacro
.macro AXY8
    sep #$30
.endmacro
.macro AXY16
    rep #$30
.endmacro

; DMA canal 0 vers la VRAM : src (long), adresse VRAM (mots), taille (octets). A8/I16 requis.
.macro VRAM_DMA src, vaddr, size
    ldx #vaddr
    stx VMADDL
    lda #$01
    sta DMAP0
    lda #$18
    sta BBAD0
    ldx #.loword(src)
    stx A1T0L
    lda #^src
    sta A1B0
    ldx #size
    stx DAS0L
    lda #$01
    sta MDMAEN
.endmacro

; valeur absolue de A (16 bits) - ne depend pas des drapeaux precedents
.macro ABS_A
    .local @pos
    cmp #0
    bpl @pos
    eor #$FFFF
    inc a
@pos:
.endmacro

; A = A >> n (arithmetique, 16 bits)
.macro ASR_A n
    .repeat n
    cmp #$8000
    ror a
    .endrepeat
.endmacro
