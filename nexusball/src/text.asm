; =============================================================================
;  text.asm - texte sur BG3 (copie bg3_map transferee au NMI)
; =============================================================================

; position dans bg3_map
.define TPOS(col, row) ((row) * 64 + (col) * 2)

; bg3_clear : efface la tilemap BG3 (tiles transparentes)
bg3_clear:
    .a16
    .i16
    ldx #0
@l: stz bg3_map,x
    inx
    inx
    cpx #2048
    bne @l
    lda #1
    sta bg3_dirty
    rts

; print : Y = adresse de la chaine (banque $80, terminee par 0)
;         X = position (TPOS), t0 = valeur ajoutee a chaque tile (attributs)
print:
    .a16
    .i16
    lda lang
    beq print_raw
    jsr tr_str
    bcc print_raw
    jsr print_raw
    ldy tr_y                    ; Y = fin de la chaine d'origine (comme sans traduction)
@e: lda a:0,y
    and #$00FF
    beq :+
    iny
    bra @e
:   lda #1
    rts

; print_raw : comme print, sans traduction
print_raw:
    .a16
    .i16
@l: lda a:0,y
    and #$00FF
    beq @d
    sec
    sbc #32
    clc
    adc t0
    sta bg3_map,x
    inx
    inx
    iny
    bra @l
@d: lda #1
    sta bg3_dirty
    rts

; print_panel : comme print, police sur panneau opaque, palette 0
print_panel:
    .a16
    lda #TXT_ATTR + TXT_PANEL
    sta t0
    bra print

; fill_panel : X = position, Y = nombre de cases, A = attributs (+TXT_PANEL pour espace opaque)
fill_tiles:
    .a16
    .i16
@l: sta bg3_map,x
    inx
    inx
    dey
    bne @l
    pha
    lda #1
    sta bg3_dirty
    pla
    rts

; print_num2 : A = nombre 0..99, X = position, t0 = attributs. Ecrit 2 chiffres (espace devant si < 10)
print_num2:
    .a16
    .i16
    phx
    ldx #10
    jsr divu
    plx
    sta t1                      ; dizaines
    lda RDMPYL                  ; reste
    sta t2
    lda t1
    bne :+
    lda #0                      ; espace
    bra :++
:   clc
    adc #('0' - 32)
:   clc
    adc t0
    sta bg3_map,x
    lda t2
    clc
    adc #('0' - 32)
    adc t0
    sta bg3_map+2,x
    lda #1
    sta bg3_dirty
    rts

; print_digit : A = chiffre, X = position, t0 = attributs
print_digit:
    .a16
    clc
    adc #('0' - 32)
    adc t0
    sta bg3_map,x
    lda #1
    sta bg3_dirty
    rts

; big_print : Y = chaine d'index de gros caracteres (BIG_x, 0 = fin, 1 = espace), X = position
big_print:
    .a16
    .i16
@l: lda a:0,y
    and #$00FF
    beq @d
    cmp #1
    beq @sp
    ora #TXT_ATTR | $0800       ; palette 2
    sta bg3_map,x
    inc a
    sta bg3_map+2,x
    inc a
    sta bg3_map+64,x
    inc a
    sta bg3_map+66,x
@sp:
    inx
    inx
    inx
    inx
    iny
    bra @l
@d: lda #1
    sta bg3_dirty
    rts
