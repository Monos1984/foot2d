; =============================================================================
;  audio.asm - chargement du pilote SPC700 (tools/spc.py) et file des sons
;
;  Une commande est envoyee par frame au plus (la file absorbe les rafales) :
;  $2142 = parametre, $2141 = commande, $2140 = compteur.
; =============================================================================

.include "data/gen/spc.inc"

APUIO1 = $2141
APUIO2 = $2142
SND_QLEN = 8

; -----------------------------------------------------------------------------
;  spc_upload : (ecran eteint, NMI coupe) transfert par le protocole de l'IPL
; -----------------------------------------------------------------------------
spc_upload:
    php
    sep #$20
    .a8
    rep #$10
    .i16
@ready:
    ldx APUIO0
    cpx #$BBAA
    bne @ready
    ldx #SPC_ORG
    stx APUIO2
    lda #$01
    sta APUIO1
    lda #$CC
    sta APUIO0
@ack:
    cmp APUIO0
    bne @ack
    ldy #0
@byte:
    tyx
    lda f:spc_bin,x
    sta APUIO1
    tya
    sta APUIO0
@echo:
    cmp APUIO0
    bne @echo
    iny
    cpy #SPC_SIZE
    bne @byte
    ; execution a SPC_ORG
    ldx #SPC_ORG
    stx APUIO2
    stz APUIO1
    tya
    inc a
    inc a
    bne :+
    inc a
:   sta APUIO0
@go:
    cmp APUIO0
    bne @go
    stz snd_cnt
    plp
    rts

; -----------------------------------------------------------------------------
;  sfx_play : A = commande (SFX_x, MUS_x) ; snd_param = parametre
; -----------------------------------------------------------------------------
sfx_play:
    .a16
    .i16
    and #$00FF
    phx
    sta sq_new
    ; priorite : 2 haute (score, buzzers, sifflet, musique...), 1 moyenne, 0 basse
    tax
    lda f:sfx_prio,x
    and #$00FF
    sta sq_pri
    ldx snd_qn
    cpx #SND_QLEN * 2
    bcc @room
    ; file pleine : un son important remplace la derniere commande, sinon il est perdu
    lda sq_pri
    cmp #2
    bne @full
    ldx #(SND_QLEN - 1) * 2
    lda sq_new
    sta snd_q,x
    bra @full
@room:
    ; sons secondaires : pas de retard accumule (file deja chargee -> ignore)
    lda sq_pri
    bne @add
    cpx #3 * 2
    bcs @full
@add:
    lda sq_new
    sta snd_q,x
    inx
    inx
    stx snd_qn
@full:
    plx
    rts

; crowd_level : A = volume de l'ambiance du public (0..$7F)
crowd_level:
    .a16
    xba
    and #$FF00
    ora #SND_CROWD
    phx
    ldx snd_qn
    cpx #SND_QLEN * 2
    bcs :+
    sta snd_q,x
    inx
    inx
    stx snd_qn
:   plx
    rts

; snd_flush : envoie la premiere commande de la file (appele une fois par frame)
snd_flush:
    .a16
    .i16
    lda snd_qn
    beq @d
    lda snd_q
    sep #$20
    .a8
    xba
    sta APUIO2
    xba
    sta APUIO1
    inc snd_cnt
    lda snd_cnt
    sta APUIO0
    rep #$20
    .a16
    ; decaler la file
    ldx #0
@sh:
    lda snd_q+2,x
    sta snd_q,x
    inx
    inx
    cpx #(SND_QLEN - 1) * 2
    bne @sh
    lda snd_qn
    dec a
    dec a
    sta snd_qn
@d: rts

.segment "DATA0"
; priorite de chaque commande son (index = numero de commande)
sfx_prio:
    .repeat 256, i
        .if i = SFX_GOAL || i = SFX_WHISTLE || i = SFX_BUZZER || i = SFX_BUZZLONG || i = SFX_HORN || i = SFX_OK || i >= $F0
            .byte 2
        .elseif i = SFX_BOUNCE || i = SFX_WALL || i = SFX_CATCH || i = SFX_MENU || i = SFX_OOH || i = SFX_CHEER || i = SFX_BOO
            .byte 0
        .else
            .byte 1
        .endif
    .endrepeat

.segment "GFX"
spc_bin: .incbin "data/gen/spc.bin"
.segment "CODE"
