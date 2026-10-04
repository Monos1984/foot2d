; =============================================================================
;  layers.asm - calques des menus : fond d'ecran (BG1), panneaux translucides
;  (BG2 + color math : moyenne avec BG1 en sous-ecran), barre de selection.
;  La tilemap BG2 a une copie en WRAM $7E:2000, transferee au NMI si modifiee.
;  Les registres TM / TS / CGWSEL / CGADSUB sont ecrits au NMI depuis des ombres.
; =============================================================================

BG2_SHADOW = $7E2000
P_FILL  = $3001 | (4 << 10)     ; tile 1, palette 4, priorite
P_BAR   = $3002 | (4 << 10)
P_TL    = $3003 | (4 << 10)
P_T     = $3004 | (4 << 10)
P_TR    = $3005 | (4 << 10)
P_L     = $3006 | (4 << 10)
P_R     = $3007 | (4 << 10)
P_BL    = $3008 | (4 << 10)
P_B     = $3009 | (4 << 10)
P_BR    = $300A | (4 << 10)

; layers_match : calques du match (pas de BG2, pas de color math)
layers_match:
    .a16
    stz hdma_sh
    sep #$20
    .a8
    lda #$17                    ; BG1 + BG2 (tribunes) + BG3 + OBJ
    sta tm_sh
    stz ts_sh
    stz cgw_sh
    stz cga_sh
    rep #$20
    .a16
    rts

; layers_menu : BG1 + BG2 + BG3 + OBJ, BG2 moyenne avec BG1 (panneaux translucides)
layers_menu:
    .a16
    stz hdma_sh
    stz bg2_hofs
    stz bg2_vofs
    sep #$20
    .a8
    lda #$17
    sta tm_sh
    lda #$01
    sta ts_sh
    lda #$02
    sta cgw_sh
    lda #$42                    ; addition / 2, sur BG2
    sta cga_sh
    rep #$20
    .a16
    rts

; -----------------------------------------------------------------------------
;  load_menubg : (ecran eteint) fond des menus sur BG1, sa palette, etoiles
; -----------------------------------------------------------------------------
load_menubg:
    .a16
    .i16
    lda bg1_mode
    cmp #1
    beq @d
    LZ_SRC gfx_menubg_chr
    ldx #VRAM_BG1_CHR
    jsr lz_vram
    LZ_SRC gfx_menubg_map
    ldx #VRAM_BG1_MAP
    jsr lz_vram
    lda #.loword(gfx_menubg_pal)
    sta tr_zp
    lda #^gfx_menubg_pal
    sta tr_zp+2
    lda #48 * 2
    jsr bg1_pal_load
    lda f:gfx_menubg_pal + 20
    sta ring_base
    sta ring_pal
    lda f:gfx_menubg_pal + 22
    sta ring_base+2
    sta ring_pal+2
    lda f:gfx_menubg_pal + 24
    sta crowd_pal
    lda f:gfx_menubg_pal + 26
    sta crowd_pal+2
    lda f:gfx_menubg_pal + 28
    sta crowd_pal+4
    lda f:gfx_menubg_pal
    ldx #0
    jsr cg_write
    lda #1
    sta bg1_mode
@d: stz scroll_x
    stz scroll_y
    rts

; bg1_pal_load : [tr_zp] = palettes des groupes 1..4 (16 couleurs chacun), A = taille (octets)
;   groupe 1 -> CGRAM 32 (palette 2), 2 -> 80 (5), 3 -> 96 (6), 4 -> 112 (7) ; couleur 0 -> fond
bg1_pal_load:
    .a16
    .i16
    sta t2
    ldy #0
@l: tya
    lsr a
    pha
    lsr a
    lsr a
    lsr a
    lsr a
    tax
    lda f:grp_slot,x
    and #$00FF
    sta t1
    pla
    and #$000F
    clc
    adc t1
    tax
    lda [tr_zp],y
    jsr cg_write
    iny
    iny
    cpy t2
    bne @l
    lda [tr_zp]
    ldx #0
    jmp cg_write

grp_slot:
    .byte 32, 80, 96, 112

; -----------------------------------------------------------------------------
;  ad_show : A = numero de l'ecran de publicite. Appele ecran eteint, rend la main
;  ecran eteint (fond BG1 = publicite : ensure_stadium_bg remet le stade).
;  Environ 3 secondes, A / B / START pour passer.
; -----------------------------------------------------------------------------
ad_show:
    .a16
    .i16
    asl a
    asl a
    asl a
    tay
    lda ad_tab,y
    sta lz_src
    lda ad_tab+2,y
    sta lz_src+2
    phy
    ldx #VRAM_BG1_CHR
    jsr lz_vram
    ply
    lda ad_tab+4,y
    sta lz_src
    lda ad_tab+2,y
    sta lz_src+2
    phy
    ldx #VRAM_BG1_MAP
    jsr lz_vram
    ply
    lda ad_tab+6,y
    sta tr_zp
    lda #^gfx_ad0_pal
    sta tr_zp+2
    lda #64 * 2
    jsr bg1_pal_load
    lda #2
    sta bg1_mode
    lda scroll_x
    pha
    lda scroll_y
    pha
    stz scroll_x
    stz scroll_y
    jsr oam_clear
    stz hdma_sh
    sep #$20
    .a8
    lda #$11                    ; BG1 + OBJ
    sta tm_sh
    stz ts_sh
    stz cgw_sh
    stz cga_sh
    rep #$20
    .a16
    jsr screen_on
    ldy #0
@w: phy
    jsr wait_frame
    ply
    iny
    cpy #20
    bcc @w
    cpy #200
    bcs @end
    lda joy_new
    ora joy_new+2
    and #(JOY_A | JOY_B | JOY_START)
    beq @w
@end:
    jsr screen_off
    pla
    sta scroll_y
    pla
    sta scroll_x
    rts

; ensure_stadium_bg : (ecran eteint) remet le stade sur BG1 si le fond des menus y est
ensure_stadium_bg:
    .a16
    lda bg1_mode
    beq :+
    stz bg1_mode
    lda stadium_id
    jmp load_stadium
:   rts

; -----------------------------------------------------------------------------
;  copie BG2 en WRAM
; -----------------------------------------------------------------------------
bg2_clear:
    .a16
    .i16
    ldx #0
    lda #0
@l: sta f:BG2_SHADOW,x
    inx
    inx
    cpx #2048
    bne @l
    lda #$FF
    sta bar_row
    lda #1
    sta bg2_dirty
    rts

; bg2_logo : la copie BG2 recoit la tilemap du logo (decompressee)
bg2_logo:
    .a16
    .i16
    LZ_SRC gfx_logo_map
    jsr lz_decompress
    ldx #0
@l: lda f:LZ_BUF,x
    sta f:BG2_SHADOW,x
    inx
    inx
    cpx #2048
    bne @l
    lda #$FF
    sta bar_row
    lda #1
    sta bg2_dirty
    rts

; bg2_put : A = tile, X = position (colonne, ligne -> offset) dans la copie
bg2_put:
    .a16
    sta f:BG2_SHADOW,x
    rts

; -----------------------------------------------------------------------------
;  bg2_panel : panneau encadre. pn_x, pn_y, pn_w, pn_h (cases). Preserve t0..t7.
; -----------------------------------------------------------------------------
bg2_panel:
    .a16
    .i16
    lda pn_y
    sta pn_r
@row:
    ; offset de debut de ligne
    lda pn_r
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc pn_x
    asl a
    tax
    ; choix des tuiles gauche / milieu / droite selon la ligne
    lda pn_r
    cmp pn_y
    bne :+
    lda #P_TL
    ldy #P_T
    sta pn_l
    sty pn_m
    lda #P_TR
    sta pn_rt
    bra @draw
:   lda pn_y
    clc
    adc pn_h
    dec a
    cmp pn_r
    bne :+
    lda #P_BL
    ldy #P_B
    sta pn_l
    sty pn_m
    lda #P_BR
    sta pn_rt
    bra @draw
:   lda #P_L
    ldy #P_FILL
    sta pn_l
    sty pn_m
    lda #P_R
    sta pn_rt
@draw:
    lda pn_l
    sta f:BG2_SHADOW,x
    inx
    inx
    ldy pn_w
    dey
    dey
@m: lda pn_m
    sta f:BG2_SHADOW,x
    inx
    inx
    dey
    bne @m
    lda pn_rt
    sta f:BG2_SHADOW,x
    inc pn_r
    lda pn_y
    clc
    adc pn_h
    cmp pn_r
    jne @row
    lda #1
    sta bg2_dirty
    rts

; -----------------------------------------------------------------------------
;  bg2_bar : A = ligne a surligner (barre de selection entre bar_x0 et bar_x1)
;  La ligne precedente redevient fond de panneau.
; -----------------------------------------------------------------------------
bg2_bar:
    .a16
    .i16
    pha
    lda bar_row
    cmp #$FF
    beq :+
    ldy #P_FILL
    jsr @line
:   pla
    sta bar_row
    ldy #P_BAR
    jsr @line
    lda #1
    sta bg2_dirty
    rts
@line:
    ; A = ligne, Y = tile
    sty pn_m
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc bar_x0
    asl a
    tax
    ldy bar_w
@l: lda pn_m
    sta f:BG2_SHADOW,x
    inx
    inx
    dey
    bne @l
    lda bar_row
    rts

; -----------------------------------------------------------------------------
;  title_hdma : ecran titre - pas de color math sur les lignes du logo (opaque),
;  panneaux translucides en dessous (HDMA canal 7 sur CGADSUB)
; -----------------------------------------------------------------------------
title_hdma:
    .a16
    sep #$20
    .a8
    stz $4370                   ; 1 registre, ecriture simple
    lda #<CGADSUB
    sta $4371
    ldx #.loword(hdma_cga_tab)
    stx $4372
    lda #^hdma_cga_tab
    sta $4374
    lda #$80
    sta hdma_sh
    rep #$20
    .a16
    rts

hdma_cga_tab:
    .byte 84, $00               ; logo : pas de color math
    .byte 1, $42                ; ensuite : panneaux translucides
    .byte 0

; -----------------------------------------------------------------------------
;  tiles BG2 : logo (titre) ou tribunes (match). Les deux jeux contiennent les
;  tuiles de panneaux 1..10 (menus et pause).
; -----------------------------------------------------------------------------
load_logo_chr:
    .a16
    lda bg2_mode
    cmp #1
    beq :+
    LZ_SRC gfx_logo_chr
    ldx #VRAM_BG2_CHR
    jsr lz_vram
    lda #1
    sta bg2_mode
:   rts

; load_crowd : (ecran eteint) tiles des tribunes + tilemap dans la copie BG2
load_crowd:
    .a16
    lda bg2_mode
    cmp #2
    beq bg2_crowd_map
    LZ_SRC gfx_crowd_chr
    ldx #VRAM_BG2_CHR
    jsr lz_vram
    lda #2
    sta bg2_mode
; bg2_crowd_map : tilemap des tribunes dans la copie BG2 (transferee au NMI)
bg2_crowd_map:
    .a16
    .i16
    LZ_SRC gfx_crowd_map
    jsr lz_decompress
    ldx #0
@l: lda f:LZ_BUF,x
    sta f:BG2_SHADOW,x
    inx
    inx
    cpx #2048
    bne @l
    lda #1
    sta bg2_dirty
    rts
