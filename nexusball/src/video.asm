; =============================================================================
;  video.asm - configuration PPU, chargements DMA, tampon OAM
; =============================================================================

; -----------------------------------------------------------------------------
;  load_graphics : (ecran eteint) mode 1, tiles, tilemaps, palette
; -----------------------------------------------------------------------------
load_graphics:
    php
    sep #$20
    .a8
    rep #$10
    .i16
    lda #$09                    ; mode 1, BG3 prioritaire
    sta BGMODE
    lda #((VRAM_BG1_MAP >> 8) & $FC) | $03   ; 64x64
    sta BG1SC
    lda #((VRAM_BG3_MAP >> 8) & $FC)         ; 32x32
    sta BG3SC
    lda #((VRAM_BG2_MAP >> 8) & $FC)         ; logo du titre
    sta BG2SC
    lda #(VRAM_BG1_CHR >> 12) | ((VRAM_BG2_CHR >> 12) << 4)
    sta BG12NBA
    stz BG2HOFS
    stz BG2HOFS
    lda #$FF
    sta BG2VOFS
    stz BG2VOFS
    lda #(VRAM_BG3_CHR >> 12)
    sta BG34NBA
    lda #(VRAM_OBJ_CHR >> 13)                ; OBJ 8x8 / 16x16
    sta OBJSEL
    lda #$15                    ; BG1 + BG3 + OBJ
    sta TM
    sta tm_sh
    stz TS
    lda #$80
    sta VMAIN

    rep #$20
    .a16
    LZ_SRC gfx_font_chr
    ldx #VRAM_BG3_CHR
    jsr lz_vram
    LZ_SRC gfx_obj_chr
    ldx #VRAM_OBJ_CHR
    jsr lz_vram
    LZ_SRC gfx_logo_chr
    ldx #VRAM_BG2_CHR
    jsr lz_vram
    jsr load_logo_map
    sep #$20
    .a8

    ; palette complete
    stz CGADD
    stz DMAP0
    lda #<CGDATA
    sta BBAD0
    ldx #.loword(gfx_pal)
    stx A1T0L
    lda #^gfx_pal
    sta A1B0
    ldx #512
    stx DAS0L
    lda #$01
    sta MDMAEN

    rep #$20
    .a16
    lda stadium_id
    jsr load_stadium
    jsr oam_clear
    jsr bg3_clear
    plp
    rts

; -----------------------------------------------------------------------------
;  oam_clear : tous les sprites hors ecran, table haute a zero
; -----------------------------------------------------------------------------
oam_clear:
    .a16
    .i16
    ldx #0
    lda #$F000                  ; x = 0, y = 240
@l: sta oam_buf,x
    stz oam_buf+2,x
    inx
    inx
    inx
    inx
    cpx #512
    bne @l
@h: stz oam_buf,x
    inx
    inx
    cpx #544
    bne @h
    stz oam_ptr
    rts

; -----------------------------------------------------------------------------
;  oam_add : ajoute un sprite
;    A  = x ecran (signe), t0 = y ecran (signe), t1 = tile | attributs<<8,
;    t2 = 0 petit (8x8) / 1 grand (16x16)
;  Les sprites entierement hors ecran sont ignores.
; -----------------------------------------------------------------------------
oam_add:
    .a16
    .i16
    cmp #256
    bmi :+
    rts                         ; x >= 256
:   cmp #.loword(-16)
    bpl :+
    rts                         ; x < -16
:   sta t3
    lda t0
    cmp #224
    bmi :+
    rts
:   cmp #.loword(-16)
    bpl :+
    rts
:   ldx oam_ptr
    cpx #512
    bcc :+
    rts                         ; OAM pleine
:   sep #$20
    .a8
    lda t3
    sta oam_buf,x
    lda t0
    sta oam_buf+1,x
    rep #$20
    .a16
    lda t1
    sta oam_buf+2,x
    ; table haute : bit x9 et taille
    lda t3+1
    and #$0001                  ; bit 8 de x
    sta t4
    lda t2
    asl a
    ora t4                      ; 0..3
    beq @done
    sta t4
    txa
    lsr a
    lsr a                       ; numero de sprite
    pha
    and #$0003
    asl a
    tay                         ; decalage = (n & 3) * 2
    lda t4
@sh:
    dey
    bmi @shd
    asl a
    bra @sh
@shd:
    sta t4
    pla
    lsr a
    lsr a
    tay
    sep #$20
    .a8
    lda oam_buf+512,y
    ora t4
    sta oam_buf+512,y
    rep #$20
    .a16
@done:
    lda oam_ptr
    clc
    adc #4
    sta oam_ptr
    rts

; -----------------------------------------------------------------------------
;  oam_finish : cache les sprites inutilises de cette frame
; -----------------------------------------------------------------------------
oam_finish:
    .a16
    .i16
    ldx oam_ptr
    lda #$F000
@l: cpx #512
    bcs @d
    sta oam_buf,x
    stz oam_buf+2,x
    inx
    inx
    inx
    inx
    bra @l
@d: rts

; oam_begin : nouvelle frame de sprites
oam_begin:
    .a16
    .i16
    ldx #0
@h: stz oam_buf+512,x
    inx
    inx
    cpx #32
    bne @h
    stz oam_ptr
    rts

; -----------------------------------------------------------------------------
;  load_stadium : A = stade (ecran eteint) -> tiles, tilemap, palette BG1, public
; -----------------------------------------------------------------------------
STAD_ENTRY = 12

load_stadium:
    .a16
    .i16
    sta t0
    asl a
    sta t1
    asl a
    clc
    adc t1
    asl a                       ; *12
    tay
    lda stadium_tab+0,y
    sta lz_src
    lda stadium_tab+2,y
    sta lz_src+2
    phy
    ldx #VRAM_BG1_CHR
    jsr lz_vram
    ply
    lda stadium_tab+5,y
    sta lz_src
    lda stadium_tab+7,y
    and #$00FF
    sta lz_src+2
    phy
    ldx #VRAM_BG1_MAP
    jsr lz_vram
    ply
    rep #$20
    .a16
    ; palette (CGRAM 32..47), fond (CGRAM 0) et couleurs du public
    lda stadium_tab+8,y
    sta t2
    ldy t2
    lda a:0,y
    ldx #0
    jsr cg_write
    ldx #32
@p: lda a:0,y
    jsr cg_write
    iny
    iny
    inx
    cpx #48
    bne @p
    ldy t2
    lda a:24,y
    sta crowd_pal
    lda a:26,y
    sta crowd_pal+2
    lda a:28,y
    sta crowd_pal+4
    lda a:20,y
    sta ring_pal
    sta ring_base
    lda a:22,y
    sta ring_pal+2
    sta ring_base+2
    rts

; load_logo_map : (ecran eteint) tilemap du logo sur BG2
load_logo_map:
    .a16
    LZ_SRC gfx_logo_map
    ldx #VRAM_BG2_MAP
    jmp lz_vram
