; =============================================================================
;  NEXUS BALL - Super Nintendo / Super Famicom
;  OFFGAME - direction Jean Monos
;  Assembleur : ca65 (cc65), CPU 65C816, HiROM FastROM, 256 KiB, SRAM 32 KiB
;
;  Unite d'assemblage unique : ce fichier inclut tous les modules.
;  Conventions : sauf mention contraire, les routines sont appelees et
;  rendent la main en A16/I16, avec DB = $80 et D = $0000.
; =============================================================================

.p816
.ifndef DEBUG
    DEBUG = 0
.endif
.smart +
.macpack longbranch

.include "snes_regs.inc"
.include "constants.inc"
.include "macros.inc"
.include "memory.inc"
.include "data/gen/bigfont.inc"

.segment "CODE"
.include "boot.asm"
.include "nmi.asm"
.include "lz.asm"
.include "video.asm"
.include "layers.asm"
.include "input.asm"
.include "math.asm"
.include "region.asm"
.include "text.asm"
.include "save.asm"
.include "menu.asm"
.include "match.asm"
.include "formation.asm"
.include "player.asm"
.include "ball.asm"
.include "goalkeeper.asm"
.include "ai.asm"
.include "rules.asm"
.include "camera.asm"
.include "hud.asm"
.include "sprites.asm"
.include "teams.asm"
.include "ui.asm"
.include "shootout.asm"
.include "audio.asm"
.include "comp.asm"
.include "editor.asm"
.include "password.asm"
.include "intro.asm"
.segment "CODE"
.include "tsel.asm"
.include "result.asm"
.include "lang.asm"

; -----------------------------------------------------------------------------
;  donnees graphiques (banque $C1)
; -----------------------------------------------------------------------------
.segment "GFX"
gfx_obj_chr:    .incbin "data/gen/obj.chr.lz"
gfx_font_chr:   .incbin "data/gen/font.chr.lz"
gfx_pal:        .incbin "data/gen/pal.bin"
gfx_logo_chr:   .incbin "data/gen/logo.chr.lz"
gfx_logo_map:   .incbin "data/gen/logo.map.lz"
gfx_menubg_chr: .incbin "data/gen/menubg.chr.lz"
gfx_menubg_map: .incbin "data/gen/menubg.map.lz"
gfx_menubg_pal: .incbin "data/gen/menubg.pal"
gfx_crowd_chr:  .incbin "data/gen/crowd.chr.lz"
gfx_crowd_map:  .incbin "data/gen/crowd.map.lz"
gfx_ad0_chr:    .incbin "data/gen/ad0.chr.lz"
gfx_ad0_map:    .incbin "data/gen/ad0.map.lz"
gfx_ad0_pal:    .incbin "data/gen/ad0.pal"
gfx_ad1_chr:    .incbin "data/gen/ad1.chr.lz"
gfx_ad1_map:    .incbin "data/gen/ad1.map.lz"
gfx_ad1_pal:    .incbin "data/gen/ad1.pal"
gfx_ad2_chr:    .incbin "data/gen/ad2.chr.lz"
gfx_ad2_map:    .incbin "data/gen/ad2.map.lz"
gfx_ad2_pal:    .incbin "data/gen/ad2.pal"

.segment "RODATA"
; publicites : chr, banque, map, palette (banque $C1)
ad_tab:
    .word .loword(gfx_ad0_chr), ^gfx_ad0_chr, .loword(gfx_ad0_map), .loword(gfx_ad0_pal)
    .word .loword(gfx_ad1_chr), ^gfx_ad1_chr, .loword(gfx_ad1_map), .loword(gfx_ad1_pal)
    .word .loword(gfx_ad2_chr), ^gfx_ad2_chr, .loword(gfx_ad2_map), .loword(gfx_ad2_pal)
.segment "GFX"
; stades : banques $C2 / $C3 (tiles, tilemap) et palettes en banque $80
.include "data/gen/stadiums.inc"

; -----------------------------------------------------------------------------
;  en-tete cartouche ($FFB0) et vecteurs ($FFE0)
; -----------------------------------------------------------------------------
.segment "HEADER"
    .byte "OG"                      ; $FFB0 maker
    .byte "NXBE"                    ; $FFB2 game code
    .res 6, 0                       ; $FFB6
    .byte 0                         ; $FFBC flash
    .byte 0                         ; $FFBD expansion RAM
    .byte 0                         ; $FFBE special version
    .byte 0                         ; $FFBF chipset subtype
    .byte "NEXUS BALL           "   ; $FFC0 titre (21 octets)
    .byte $31                       ; $FFD5 HiROM + FastROM
    .byte $02                       ; $FFD6 ROM + RAM + batterie
    .byte $08                       ; $FFD7 256 KiB
    .byte $05                       ; $FFD8 SRAM 32 KiB
    .byte $02                       ; $FFD9 pays (Europe ; build.sh produit aussi une ROM NTSC)
    .byte $33                       ; $FFDA en-tete etendu
    .byte $00                       ; $FFDB version
    .word $FFFF                     ; $FFDC complement (corrige par tools/checksum.py)
    .word $0000                     ; $FFDE checksum

.segment "VECTORS"
    ; mode natif ($FFE0)
    .word 0, 0
    .word .loword(VecEmpty)         ; COP
    .word .loword(VecEmpty)         ; BRK
    .word .loword(VecEmpty)         ; ABORT
    .word .loword(VecNmi)           ; NMI
    .word 0
    .word .loword(VecEmpty)         ; IRQ
    ; mode emulation ($FFF0)
    .word 0, 0
    .word .loword(VecEmpty)         ; COP
    .word 0
    .word .loword(VecEmpty)         ; ABORT
    .word .loword(VecEmpty)         ; NMI
    .word .loword(VecReset)         ; RESET
    .word .loword(VecEmpty)         ; IRQ/BRK
