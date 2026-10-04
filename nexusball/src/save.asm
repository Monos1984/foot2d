; =============================================================================
;  save.asm - sauvegarde SRAM (HiROM : $A0:6000-$7FFF)
;
;  Bloc OPTIONS a $A06000 :
;    +0 "NXBL"  +4 version  +5 longueur des donnees (4)
;    +6 radar   +7 duree de match (index)  +8 reserve x2
;    +10 checksum 16 bits (somme des octets 0..9)
;  Un bloc invalide est remplace par les valeurs par defaut, sans toucher
;  au reste de la SRAM.
; =============================================================================

SRAM_OPT      = $A06000
SAVE_VERSION  = 1

load_options:
    php
    rep #$30
    .a16
    .i16
    jsr opt_checksum
    cmp f:SRAM_OPT+10
    bne @default
    lda f:SRAM_OPT
    cmp #('N' | ('X' << 8))
    bne @default
    lda f:SRAM_OPT+2
    cmp #('B' | ('L' << 8))
    bne @default
    lda f:SRAM_OPT+4
    and #$00FF
    cmp #SAVE_VERSION
    bne @default
    lda f:SRAM_OPT+6
    and #$0001
    sta opt_radar
    lda f:SRAM_OPT+7
    and #$0003
    sta menu_len
    plp
    rts
@default:
    lda #1
    sta opt_radar
    lda #2                      ; 2 x 4 min
    sta menu_len
    jsr save_options
    plp
    rts

save_options:
    php
    rep #$30
    .a16
    .i16
    lda #('N' | ('X' << 8))
    sta f:SRAM_OPT
    lda #('B' | ('L' << 8))
    sta f:SRAM_OPT+2
    lda #SAVE_VERSION | (4 << 8)
    sta f:SRAM_OPT+4
    lda menu_len
    xba
    ora opt_radar
    sta f:SRAM_OPT+6
    lda #0
    sta f:SRAM_OPT+8
    jsr opt_checksum
    sta f:SRAM_OPT+10
    plp
    rts

; opt_checksum : A = somme des octets 0..9 du bloc options
opt_checksum:
    .a16
    .i16
    lda #$5A5A
    sta t6
    ldx #0
@l: lda f:SRAM_OPT,x
    and #$00FF
    clc
    adc t6
    sta t6
    inx
    cpx #10
    bne @l
    lda t6
    rts
