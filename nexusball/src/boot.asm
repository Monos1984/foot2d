; =============================================================================
;  boot.asm - demarrage, initialisation materiel, boucle principale
; =============================================================================

; Les vecteurs pointent ici (execution en banque $00) : on saute en banque $80
; pour profiter de la FastROM.
VecReset:
    sei
    clc
    xce                         ; mode natif
    jml FastReset

VecNmi:
    jml NmiHandler

VecEmpty:
    rti

zero_byte: .byte 0

FastReset:
    .a8
    .i8
    rep #$10
    .i16
    ldx #$1FFF
    txs
    sep #$20
    .a8
    lda #$80
    pha
    plb                         ; DB = $80
    lda #$01
    sta MEMSEL                  ; FastROM
    rep #$20
    .a16
    lda #$0000
    tcd                         ; D = 0
    sep #$20
    .a8
    lda #$8F
    sta INIDISP                 ; ecran force eteint
    stz NMITIMEN

    ; --- effacement des 128 Kio de WRAM (DMA, source fixe = 0). Pas de pile ici.
    stz WMADDL
    stz WMADDL+1
    stz WMADDL+2
    lda #$08                    ; source fixe, 1 registre
    sta DMAP0
    lda #<WMDATA
    sta BBAD0
    ldx #.loword(zero_byte)
    stx A1T0L
    lda #^zero_byte
    sta A1B0
    ldx #$0000                  ; 65536 octets
    stx DAS0L
    lda #$01
    sta MDMAEN
    ldx #$0000
    stx DAS0L
    sta MDMAEN                  ; deuxieme moitie

    jsr init_ppu_regs
    jsr clear_vram
    jsr detect_region
    jsr init_region
    jsr load_graphics
    jsr spc_upload
    jsr load_options
    rep #$30
    jsr ed_check

    rep #$30
    .a16
    .i16
    lda #$ACE1
    sta rng
    ; equipes et reglages par defaut
    lda #$FFFF
    sta trec_id
    lda #6                      ; EUROPA ICE
    sta team_id
    stz team_id+2               ; ORION STARS
    lda #0
    jsr team_defaults
    lda #1
    jsr team_defaults
    lda #1
    sta difficulty
    stz draw_rule
    jmp title_screen

; -----------------------------------------------------------------------------
;  detect_region : is_pal = 1 si la console est PAL (bit 4 de STAT78)
; -----------------------------------------------------------------------------
detect_region:
    php
    sep #$20
    .a8
    lda STAT78
    and #$10
    beq :+
    lda #1
:   sta is_pal
    stz is_pal+1
    plp
    rts

; -----------------------------------------------------------------------------
;  init_ppu_regs : remet tous les registres PPU/CPU dans un etat connu
; -----------------------------------------------------------------------------
init_ppu_regs:
    php
    sep #$20
    .a8
    rep #$10
    .i16
    lda #$8F
    sta INIDISP
    ldx #$2101
@clr:
    stz $00,x                   ; $2101-$210C
    inx
    cpx #$210D
    bne @clr
@scr:
    stz $00,x                   ; $210D-$2114 : ecriture double
    stz $00,x
    inx
    cpx #$2115
    bne @scr
    lda #$80
    sta VMAIN
    stz VMADDL
    stz VMADDH
    stz M7SEL
    stz M7A
    lda #$01
    sta M7A
    stz M7B
    stz M7B
    stz $211D
    stz $211D
    stz $211E
    lda #$01
    sta $211E
    stz $211F
    stz $211F
    stz $2120
    stz $2120
    stz CGADD
    ldx #$2123
@win:
    stz $00,x                   ; $2123-$2133
    inx
    cpx #$2134
    bne @win
    lda #$30
    sta CGWSEL
    lda #$E0
    sta COLDATA
    lda #$FF
    sta WRIO
    stz NMITIMEN
    stz HDMAEN
    plp
    rts

; -----------------------------------------------------------------------------
;  clear_vram : 64 Kio a zero par DMA
; -----------------------------------------------------------------------------
clear_vram:
    php
    sep #$20
    .a8
    rep #$10
    .i16
    lda #$80
    sta VMAIN
    ldx #$0000
    stx VMADDL
    lda #$09                    ; source fixe, 2 registres
    sta DMAP0
    lda #$18
    sta BBAD0
    ldx #.loword(zero_byte)
    stx A1T0L
    lda #^zero_byte
    sta A1B0
    ldx #$0000
    stx DAS0L
    lda #$01
    sta MDMAEN
    plp
    rts

; -----------------------------------------------------------------------------
;  wait_frame : signale au NMI que la frame est prete, attend le VBlank,
;  puis lit les manettes.
; -----------------------------------------------------------------------------
wait_frame:
    .a16
    .i16
    sep #$20
    .a8
    lda #1
    sta nmi_ready
@w: lda nmi_ready
    bne @w
    rep #$20
    .a16
    inc frame
    jsr read_pads
    jsr snd_flush
    rts

; -----------------------------------------------------------------------------
;  screen_off / screen_on (A16/I16)
; -----------------------------------------------------------------------------
screen_off:
    ; fondu vers le noir puis ecran force eteint
    .a16
    lda #12
@f: sta fade_v
    sep #$20
    .a8
    sta inidisp
    rep #$20
    .a16
    jsr wait_frame
    lda fade_v
    sec
    sbc #3
    bpl @f
    sep #$20
    .a8
    lda #$8F
    sta inidisp
    jsr wait_frame              ; le NMI applique INIDISP
    sep #$20
    .a8
    stz NMITIMEN
    lda #$8F
    sta INIDISP
    stz nmi_on
    rep #$20
    .a16
    rts

screen_on:
    ; ecran rallume avec un fondu depuis le noir
    sep #$20
    .a8
    lda #$00
    sta inidisp
    lda #$81                    ; NMI + lecture auto manettes
    sta NMITIMEN
    sta nmi_on
    rep #$20
    .a16
    lda #3
@f: sta fade_v
    sep #$20
    .a8
    sta inidisp
    rep #$20
    .a16
    jsr wait_frame
    lda fade_v
    clc
    adc #3
    cmp #16
    bcc @f
    sep #$20
    .a8
    lda #$0F
    sta inidisp
    rep #$20
    .a16
    rts
