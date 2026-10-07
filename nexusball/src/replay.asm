.segment "CODE2"
; =============================================================================
;  replay.asm - ralenti du but : les 64 dernieres positions (une frame sur deux) des
;  12 joueurs et du ballon sont gardees en WRAM haute, puis rejouees au ralenti
;  apres la celebration (A / B pour passer).
; =============================================================================

RP_BUF  = $7E8000               ; 64 echantillons de RP_SZ octets
RP_N    = 64
RP_SZ   = 104                   ; 12 x (x, y, z, sprite|retournement) + ballon (x, y, z) + 2
RP_SLOW = 3                     ; frames par echantillon a la relecture (ralenti x1,5)

; rp_record : en jeu, une frame sur deux
rp_record:
    .a16
    .i16
    lda frame
    and #$0001
    beq :+
    rts
:   lda rp_w
    ldy #RP_SZ
    jsr mulu8
    tax
    ldy #0
@l: lda p_x,y
    sta f:RP_BUF,x
    lda p_y,y
    sta f:RP_BUF + 2,x
    lda p_z,y
    sta f:RP_BUF + 4,x
    lda p_spr,y
    ora p_flip,y
    sta f:RP_BUF + 6,x
    txa
    clc
    adc #8
    tax
    iny
    iny
    cpy #NUM_PLAYERS * 2
    bne @l
    lda b_x
    sta f:RP_BUF,x
    lda b_y
    sta f:RP_BUF + 2,x
    lda b_z
    sta f:RP_BUF + 4,x
    lda rp_w
    inc a
    and #RP_N - 1
    sta rp_w
    lda rp_cnt
    cmp #RP_N
    bcs :+
    inc rp_cnt
:   rts

; rp_start : C = 1 si un ralenti demarre (assez d'images enregistrees)
rp_start:
    .a16
    .i16
    lda rp_cnt
    cmp #16
    bcs :+
    clc
    rts
:   sta rp_left
    lda rp_w
    sec
    sbc rp_cnt
    and #RP_N - 1
    sta rp_r
    lda #1
    sta rp_d
    lda #MS_REPLAY
    sta m_state
    stz msg_time
    jsr clear_msg               ; le bandeau du but cacherait l'action
    sec
    rts

; rp_label : "REPLAY" clignotant en bas a gauche (ligne 26), efface si A = 0
rp_label:
    .a16
    .i16
    beq @clr
    lda #TXT_ATTR + TXT_PANEL + $1000
    sta t0
    ldx #TPOS(1, 26)
    ldy #.loword(str_replay)
    jmp print
@clr:
    ldx #TPOS(1, 26)
    ldy #8
    lda #0
    jmp fill_tiles

; st_replay : etat MS_REPLAY
st_replay:
    .a16
    .i16
    lda frame
    and #$000F
    bne :+
    lda frame
    and #$0010
    jsr rp_label
:   lda joy_new
    ora joy_new+2
    and #(JOY_A | JOY_B)
    jne @end
    dec rp_d
    jne @w
    lda #RP_SLOW
    sta rp_d
    lda rp_left
    jeq @end
    dec rp_left
    lda rp_r
    ldy #RP_SZ
    jsr mulu8
    tax
    ldy #0
@l: lda f:RP_BUF,x
    sta p_x,y
    lda f:RP_BUF + 2,x
    sta p_y,y
    lda f:RP_BUF + 4,x
    sta p_z,y
    lda f:RP_BUF + 6,x
    and #$4000
    sta p_flip,y
    lda f:RP_BUF + 6,x
    and #$00FF
    sta p_spr,y
    txa
    clc
    adc #8
    tax
    iny
    iny
    cpy #NUM_PLAYERS * 2
    bne @l
    ; ballon libre (orientation et trainee selon le deplacement)
    lda #NO_OWNER
    sta b_owner
    lda f:RP_BUF,x
    sec
    sbc b_x
    ASR_A 1
    sta b_vx
    lda f:RP_BUF + 2,x
    sec
    sbc b_y
    ASR_A 1
    sta b_vy
    lda f:RP_BUF,x
    sta b_x
    lda f:RP_BUF + 2,x
    sta b_y
    lda f:RP_BUF + 4,x
    sta b_z
    lda rp_r
    inc a
    and #RP_N - 1
    sta rp_r
@w: clc
    rts
@end:
    stz rp_cnt
    lda #0
    jsr rp_label
    stz b_vx
    stz b_vy
    jmp goal_resume

.segment "RODATA"
str_replay: .byte "REPLAY", 0
.segment "CODE2"
