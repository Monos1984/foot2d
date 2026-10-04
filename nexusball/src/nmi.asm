; =============================================================================
;  nmi.asm - VBlank : transferts courts et previsibles
;    OAM (544 octets), tilemap BG3 si modifiee, couleurs du public, scroll.
; =============================================================================

NmiHandler:
    rep #$30
    .a16
    .i16
    pha
    phx
    phy
    phb
    phd
    lda #$0000
    tcd
    sep #$20
    .a8
    lda #$80
    pha
    plb
    lda RDNMI                   ; acquittement

    lda nmi_ready
    jeq @late                   ; frame pas prete : on ne touche a rien

    ; --- OAM
    stz OAMADDL
    stz OAMADDH
    lda #$00
    sta DMAP0
    lda #<OAMDATA
    sta BBAD0
    ldx #.loword(oam_buf)
    stx A1T0L
    lda #$80
    sta A1B0
    ldx #544
    stx DAS0L
    lda #$01
    sta MDMAEN

    ; --- tilemap BG3
    lda bg3_dirty
    beq @nobg3
    stz bg3_dirty
    lda #$80
    sta VMAIN
    ldx #VRAM_BG3_MAP
    stx VMADDL
    lda #$01
    sta DMAP0
    lda #$18
    sta BBAD0
    ldx #.loword(bg3_map)
    stx A1T0L
    lda #$80
    sta A1B0
    ldx #2048
    stx DAS0L
    lda #$01
    sta MDMAEN
@nobg3:

    ; --- couleurs du public (CGRAM 44-46)
    lda pal_dirty
    beq @nopal
    stz pal_dirty
    lda #44
    sta CGADD
    ldx #0
@pl:
    lda crowd_pal,x
    sta CGDATA
    inx
    cpx #6
    bne @pl
@nopal:

    ; --- scroll BG1
    lda scroll_x
    sta BG1HOFS
    lda scroll_x+1
    sta BG1HOFS
    rep #$20
    .a16
    lda scroll_y
    dec a                       ; la PPU affiche la ligne VOFS+1 en haut
    sep #$20
    .a8
    sta BG1VOFS
    xba
    sta BG1VOFS
    stz BG3HOFS
    stz BG3HOFS
    lda #$FF
    sta BG3VOFS
    stz BG3VOFS

    lda inidisp
    sta INIDISP
    stz nmi_ready

@late:
    rep #$30
    .a16
    .i16
    inc nmi_count
    pld
    plb
    ply
    plx
    pla
    rti
