.segment "CODE2"
; =============================================================================
;  password.asm - mot de passe des competitions (alternative a la sauvegarde)
;
;  Les matchs CPU contre CPU sont simules a partir d'une graine (c_seed, 12 bits) et
;  du numero global du match : ils se recalculent a l'identique. Le mot de passe ne
;  contient donc que les reglages, les equipes, la graine, la position dans le
;  calendrier, les scores des matchs humains et les exceptions (matchs CPU regardes).
;
;  Flux de bits (poids faible d'abord) :
;    emplacement 2, type 1, duree 2, difficulte 2, stade 3, regarder 1
;    equipes 0-15 : 16 x 1 ; equipes creees ? 1 (+ 8 x 1)
;    par participant : humain 1 (+ manette 1 : 0 P1, 1 P2)
;    graine 12 ; termine 1 (sinon journee 4, match 4)
;    exceptions ? 1 (+ nombre 7, puis par exception : match 7, score, score, [tab 1])
;    scores des matchs humains dans l'ordre : score, score, [tab 1 si coupe et egalite]
;    controle 10
;  Score : 0-7 -> 0 + 3 bits ; 8-23 -> 10 + 4 bits ; 24-151 -> 11 + 7 bits.
;  5 bits par caractere (32 symboles sans I, O, 0, 1), brouilles par pw_xor.
; =============================================================================

PW_MAXC   = 100                 ; caracteres au plus (5 lignes de 20)
PW_BYTES  = 64
PW_HINIT  = $5A3C

; sim_seed : A = match global -> rng propre a ce match (simulation reproductible)
sim_seed:
    .a16
    xba
    asl a                       ; g << 9
    eor c_seed
    eor #$6D2B
    bne :+
    lda #1
:   sta rng
    jsr rand
    jsr rand
    rts

; -----------------------------------------------------------------------------
;  bits
; -----------------------------------------------------------------------------
pw_clear:
    .a16
    .i16
    ldx #0
@l: stz pw_buf,x
    inx
    inx
    cpx #PW_BYTES
    bne @l
    stz pw_pos
    stz pw_err
    lda #PW_HINIT
    sta pw_h
    rts

; pw_put : A = valeur, Y = nombre de bits. Preserve t0-t7.
pw_put:
    .a16
    .i16
    sta pw_v
    sty pw_n
@l: lda pw_pos
    cmp #PW_BYTES * 8
    bcc :+
    inc pw_err
    rts
:   lsr a
    lsr a
    lsr a
    tax
    lda pw_pos
    and #7
    tay
    stz pw_bitv
    lsr pw_v
    bcc @z
    inc pw_bitv
    sep #$20
    .a8
    lda pw_buf,x
    ora pw_mask,y
    sta pw_buf,x
    rep #$20
    .a16
@z: jsr pw_hbit
    inc pw_pos
    dec pw_n
    bne @l
    rts

; pw_hbit : pw_bitv (0/1) -> CRC-16 (polynome $1021) dans pw_h
pw_hbit:
    .a16
    lda pw_h
    asl a
    sta pw_h
    lda #0
    rol a
    eor pw_bitv
    beq :+
    lda pw_h
    eor #$1021
    sta pw_h
:   rts

; pw_get : Y = nombre de bits -> A = valeur (pw_err si on depasse pw_max)
pw_get:
    .a16
    .i16
    sty pw_n
    stz pw_v
    lda #1
    sta pw_b
@l: lda pw_pos
    cmp pw_max
    bcc :+
    inc pw_err
    stz pw_bitv
    bra @z
:   lsr a
    lsr a
    lsr a
    tax
    lda pw_pos
    and #7
    tay
    sep #$20
    .a8
    lda pw_buf,x
    and pw_mask,y
    rep #$20
    .a16
    stz pw_bitv
    beq @z
    inc pw_bitv
    lda pw_v
    ora pw_b
    sta pw_v
@z: jsr pw_hbit
    asl pw_b
    inc pw_pos
    dec pw_n
    bne @l
    lda pw_v
    rts

; pw_put_score : A = score
pw_put_score:
    .a16
    cmp #8
    bcs :+
    ldy #4                      ; 0 + 3 bits
    asl a
    jmp pw_put
:   cmp #24
    bcs :+
    sec
    sbc #8
    asl a
    asl a
    ora #1                      ; 1, 0, 4 bits
    ldy #6
    jmp pw_put
:   sec
    sbc #24
    cmp #128
    bcc :+
    lda #127
:   asl a
    asl a
    ora #3                      ; 1, 1, 7 bits
    ldy #9
    jmp pw_put

pw_get_score:
    .a16
    ldy #1
    jsr pw_get
    bne :+
    ldy #3
    jmp pw_get
:   ldy #1
    jsr pw_get
    bne :+
    ldy #4
    jsr pw_get
    clc
    adc #8
    rts
:   ldy #7
    jsr pw_get
    clc
    adc #24
    rts

; pw_chk : A = valeur de controle (10 bits) de pw_h
pw_chk:
    .a16
    lda pw_h
    xba
    lsr a
    lsr a
    and #$003F
    eor pw_h
    and #$03FF
    rts

; -----------------------------------------------------------------------------
;  parcours des matchs joues : pw_tr / pw_tm / pw_done = position cible
; -----------------------------------------------------------------------------
pw_iter_start:
    .a16
    stz pw_r
    lda #$FFFF
    sta pw_m
    rts

; pw_iter_next : C = 1 -> match suivant (t0, t1, t2, t5-t7 comme comp_match_info)
pw_iter_next:
    .a16
    .i16
@n: inc pw_m
    lda pw_r
    sta c_round
    jsr comp_rounds
    cmp pw_r
    beq @end
    bcc @end
    jsr comp_mpr
    cmp pw_m
    beq @nr
    bcs @in
@nr:
    stz pw_m
    inc pw_r
    lda pw_r
    sta c_round
    jsr comp_rounds
    cmp pw_r
    beq @end
    bcc @end
    jsr comp_mpr
    cmp #0                      ; (comp_mpr ne positionne pas Z sur A)
    beq @nr
@in:
    lda pw_done
    bne :+
    lda pw_r
    cmp pw_tr
    bcc :+
    bne @end
    lda pw_m
    cmp pw_tm
    bcs @end
:   lda pw_m
    sta c_match
    jsr comp_match_info
    bcs @n                      ; exempt
    sec
    rts
@end:
    clc
    rts

; pw_human : t0 / t1 -> C = 1 si un humain joue ce match
pw_human:
    .a16
    ldx t0
    jsr part_ctrl
    cmp #2
    bcs @y
    ldx t1
    jsr part_ctrl
    cmp #2
@y: rts

pw_save_t:
    .a16
    lda t0
    sta pw_s0
    lda t1
    sta pw_s1
    lda t2
    sta pw_s2
    lda t7
    sta pw_s7
    rts

pw_load_t:
    .a16
    lda pw_s0
    sta t0
    lda pw_s1
    sta t1
    lda pw_s2
    sta t2
    lda pw_s7
    sta t7
    rts

; pw_sim : match courant (t0/t1/t2) simule avec sa graine -> cs_hs / cs_as / cs_so
pw_sim:
    .a16
    jsr pw_save_t
    lda t2
    jsr sim_seed
    jsr sim_match
    jmp pw_load_t

; pw_differs : C = 1 si le resultat stocke (c_res / c_so) differe de la simulation
pw_differs:
    .a16
    .i16
    jsr pw_sim
    lda t2
    asl a
    tax
    lda c_res,x
    and #$00FF
    cmp cs_hs
    bne @y
    lda c_res+1,x
    and #$00FF
    cmp cs_as
    bne @y
    lda c_type
    beq @n
    lda cs_hs
    cmp cs_as
    bne @n
    ldx t2
    lda c_so,x
    and #$00FF
    cmp cs_so
    bne @y
@n: clc
    rts
@y: sec
    rts

; pw_put_res : ecrit le resultat stocke du match t2 (avec tab si coupe et egalite)
pw_put_res:
    .a16
    .i16
    lda t2
    asl a
    tax
    lda c_res,x
    and #$00FF
    sta pw_sh
    lda c_res+1,x
    and #$00FF
    sta pw_sa
    lda pw_sh
    jsr pw_put_score
    lda pw_sa
    jsr pw_put_score
    lda c_type
    beq @d
    lda pw_sh
    cmp pw_sa
    bne @d
    ldx t2
    lda c_so,x
    and #$0001
    ldy #1
    jsr pw_put
@d: rts

; pw_get_res : lit un resultat -> cs_hs / cs_as / cs_so
pw_get_res:
    .a16
    jsr pw_get_score
    sta cs_hs
    jsr pw_get_score
    sta cs_as
    stz cs_so
    lda c_type
    beq @d
    lda cs_hs
    cmp cs_as
    bne @d
    ldy #1
    jsr pw_get
    sta cs_so
@d: rts

; pw_present : A = equipe -> C = 1 si elle participe
pw_present:
    .a16
    .i16
    sta pw_v2
    ldx #0
@l: cpx c_n
    bcs @no
    lda c_teams,x
    and #$00FF
    cmp pw_v2
    beq @y
    inx
    bra @l
@no:
    clc
    rts
@y: sec
    rts

; -----------------------------------------------------------------------------
;  pw_encode : etat de la competition -> pw_txt / pw_len (0 si trop long)
; -----------------------------------------------------------------------------
pw_encode:
    .a16
    .i16
    lda c_round
    sta pw_sr
    lda c_match
    sta pw_sm
    sta pw_tm
    lda c_round
    sta pw_tr
    lda c_done
    sta pw_done
    jsr pw_clear
    lda c_slot
    ldy #2
    jsr pw_put
    lda c_type
    ldy #1
    jsr pw_put
    lda c_len
    ldy #2
    jsr pw_put
    lda c_diff
    ldy #2
    jsr pw_put
    lda c_stad
    ldy #3
    jsr pw_put
    lda c_watch
    ldy #1
    jsr pw_put
    ; equipes
    stz pw_k
@t: lda pw_k
    jsr pw_present
    lda #0
    rol a
    ldy #1
    jsr pw_put
    inc pw_k
    lda pw_k
    cmp #NUM_TEAMS
    bne @t
    stz pw_v3                   ; equipes creees engagees ?
@c: lda pw_k
    jsr pw_present
    bcc :+
    inc pw_v3
:   inc pw_k
    lda pw_k
    cmp #MAX_TEAMS
    bne @c
    lda pw_v3
    beq :+
    lda #1
:   ldy #1
    jsr pw_put
    lda pw_v3
    beq @hum
    lda #NUM_TEAMS
    sta pw_k
@t2:
    lda pw_k
    jsr pw_present
    lda #0
    rol a
    ldy #1
    jsr pw_put
    inc pw_k
    lda pw_k
    cmp #MAX_TEAMS
    bne @t2
@hum:
    ; humains (par participant)
    stz pw_k
@h: lda pw_k
    cmp c_n
    bcs @seed
    tax
    jsr part_ctrl
    sta pw_v3
    lda #0
    ldx pw_v3
    cpx #2
    bcc :+
    lda #1
:   ldy #1
    jsr pw_put
    lda pw_v3
    cmp #2
    bcc :+
    sec
    sbc #2
    ldy #1
    jsr pw_put
:   inc pw_k
    bra @h
@seed:
    lda c_seed
    ldy #12
    jsr pw_put
    lda c_done
    ldy #1
    jsr pw_put
    lda c_done
    bne :+
    lda pw_tr
    ldy #4
    jsr pw_put
    lda pw_tm
    ldy #4
    jsr pw_put
:   ; exceptions : compte
    stz pw_cnt
    jsr pw_iter_start
@e1:
    jsr pw_iter_next
    bcc @e1d
    jsr pw_human
    bcs @e1
    jsr pw_differs
    bcc @e1
    inc pw_cnt
    bra @e1
@e1d:
    lda pw_cnt
    beq :+
    lda #1
:   ldy #1
    jsr pw_put
    lda pw_cnt
    beq @res
    ldy #7
    jsr pw_put
    jsr pw_iter_start
@e2:
    jsr pw_iter_next
    bcc @res
    jsr pw_human
    bcs @e2
    jsr pw_differs
    bcc @e2
    lda t2
    ldy #7
    jsr pw_put
    jsr pw_put_res
    bra @e2
@res:
    ; scores des matchs humains
    jsr pw_iter_start
@r: jsr pw_iter_next
    bcc @fin
    jsr pw_human
    bcc @r
    jsr pw_put_res
    bra @r
@fin:
    jsr pw_chk
    ldy #10
    jsr pw_put
    lda pw_sr
    sta c_round
    lda pw_sm
    sta c_match
    ; caracteres
    lda pw_pos
    clc
    adc #4
    ldx #5
    jsr divu
    sta pw_len
    stz pw_len_ok
    lda pw_err
    bne @long
    lda pw_len
    cmp #PW_MAXC + 1
    bcs @long
    inc pw_len_ok
@long:
    lda #PW_BYTES * 8
    sta pw_max
    stz pw_k
    ; cle = dernier caractere (bits du controle) : brouille tous les autres
    lda pw_len
    dec a
    sta pw_k
    asl a
    asl a
    clc
    adc pw_k
    sta pw_pos
    ldy #5
    jsr pw_get
    sta pw_key
    stz pw_k
@ch:
    lda pw_k
    cmp pw_len
    bcs @done
    cmp #PW_MAXC
    bcs @done
    asl a
    asl a
    clc
    adc pw_k
    sta pw_pos
    ldy #5
    jsr pw_get
    sta pw_v3
    jsr pw_kmask
    eor pw_v3
    tax
    lda pw_chars,x
    ldx pw_k
    sep #$20
    .a8
    sta pw_txt,x
    rep #$20
    .a16
    inc pw_k
    bra @ch
@done:
    rts

; pw_kmask : pw_k -> A = masque du caractere (table + cle, sauf pour le dernier)
pw_kmask:
    .a16
    .i16
    lda pw_k
    and #$000F
    tax
    lda pw_xor,x
    and #$001F
    sta pw_v2
    lda pw_k
    inc a
    cmp pw_len
    beq @last
    lda pw_key
    asl a
    asl a
    asl a
    sec
    sbc pw_key                  ; cle * 7
    clc
    adc pw_k
    adc pw_k
    adc pw_k                    ; + 3k
    and #$001F
    eor pw_v2
    rts
@last:
    lda pw_v2
    rts

; pw_cidx : A = caractere -> A = valeur 0-31 (C = 1 si le caractere est inconnu)
pw_cidx:
    .a16
    .i16
    and #$00FF
    sta pw_v2
    ldy #0
@f: lda pw_chars,y
    and #$00FF
    cmp pw_v2
    beq @ok
    iny
    cpy #32
    bne @f
    sec
    rts
@ok:
    tya
    clc
    rts

; -----------------------------------------------------------------------------
;  pw_decode : pw_txt / pw_len -> competition (C = 1 si le mot de passe est valide)
;  c_slot_sel = emplacement attendu.
; -----------------------------------------------------------------------------
pw_decode:
    .a16
    .i16
    jsr pw_clear
    lda pw_len
    jeq @bad
    ; caracteres -> bits (cle = dernier caractere)
    lda pw_len
    cmp #PW_MAXC + 1
    jcs @bad
    dec a
    sta pw_k
    tax
    lda pw_txt,x
    jsr pw_cidx
    jcs @bad
    sta pw_v3
    lda pw_k
    and #$000F
    tax
    lda pw_xor,x
    and #$001F
    eor pw_v3
    sta pw_key
    stz pw_k
@cv:
    ldx pw_k
    lda pw_txt,x
    jsr pw_cidx
    jcs @bad
    sta pw_v3
    jsr pw_kmask
    eor pw_v3
    ldy #5
    jsr pw_put
    inc pw_k
    lda pw_k
    cmp pw_len
    bne @cv
    lda pw_pos
    sta pw_max
    stz pw_pos
    stz pw_err
    lda #PW_HINIT
    sta pw_h
    ; reglages
    ldy #2
    jsr pw_get
    cmp c_slot_sel
    jne @bad
    sta c_slot
    ldy #1
    jsr pw_get
    sta c_type
    lda c_slot
    cmp #2
    beq :+
    cmp c_type                  ; CHAMPIONSHIP = ligue, CUP = coupe
    jne @bad
:   ldy #2
    jsr pw_get
    sta c_len
    ldy #2
    jsr pw_get
    sta c_diff
    ldy #3
    jsr pw_get
    sta c_stad
    cmp #NUM_STADIUMS + 1
    jcs @bad
    ldy #1
    jsr pw_get
    sta c_watch
    ; equipes
    ldx #0
@z: stz c_ctrl,x
    inx
    inx
    cpx #MAX_TEAMS
    bcc @z
    stz pw_k
@t: ldy #1
    jsr pw_get
    beq :+
    ldx pw_k
    sep #$20
    .a8
    lda #1
    sta c_ctrl,x
    rep #$20
    .a16
:   inc pw_k
    lda pw_k
    cmp #NUM_TEAMS
    bne @t
    ldy #1
    jsr pw_get
    beq @cnt
@t2:
    ldy #1
    jsr pw_get
    beq :+
    lda pw_k
    jsr team_valid              ; equipe creee absente de cette cartouche
    jcc @bad
    ldx pw_k
    sep #$20
    .a8
    lda #1
    sta c_ctrl,x
    rep #$20
    .a16
:   inc pw_k
    lda pw_k
    cmp #MAX_TEAMS
    bne @t2
@cnt:
    jsr cs_count
    sta c_n
    lda c_type
    bne @cup
    lda c_n
    cmp #3
    jcc @bad
    cmp #17
    jcs @bad
    bra @hum
@cup:
    lda c_n
    cmp #4
    beq @hum
    cmp #8
    beq @hum
    cmp #16
    jne @bad
@hum:
    stz pw_k
@h: lda pw_k
    cmp #MAX_TEAMS
    bcs @seed
    tax
    lda c_ctrl,x
    and #$00FF
    beq @hn
    ldy #1
    jsr pw_get
    beq @hn
    ldy #1
    jsr pw_get
    clc
    adc #2
    ldx pw_k
    sep #$20
    .a8
    sta c_ctrl,x
    rep #$20
    .a16
@hn:
    inc pw_k
    bra @h
@seed:
    ldy #12
    jsr pw_get
    sta c_seed
    jsr comp_build
    ; position
    ldy #1
    jsr pw_get
    sta pw_done
    stz pw_tr
    stz pw_tm
    lda pw_done
    bne @exc
    ldy #4
    jsr pw_get
    sta pw_tr
    ldy #4
    jsr pw_get
    sta pw_tm
    jsr comp_rounds
    cmp pw_tr
    jcc @bad
    jeq @bad
    lda pw_tr
    sta c_round
    jsr comp_mpr
    cmp pw_tm
    jcc @bad
@exc:
    ldx #0
@ze:
    stz pw_exm,x
    inx
    inx
    cpx #16
    bne @ze
    ldy #1
    jsr pw_get
    beq @replay
    ldy #7
    jsr pw_get
    sta pw_cnt
    jeq @bad
@e: ldy #7
    jsr pw_get
    sta t2
    cmp #120
    jcs @bad
    jsr pw_get_res
    lda t2
    asl a
    tax
    sep #$20
    .a8
    lda cs_hs
    sta c_res,x
    lda cs_as
    sta c_res+1,x
    rep #$20
    .a16
    lda c_type
    beq :+
    lda cs_hs
    cmp cs_as
    bne :+
    ldx t2
    sep #$20
    .a8
    lda cs_so
    sta c_so,x
    rep #$20
    .a16
:
    lda t2
    lsr a
    lsr a
    lsr a
    tax
    lda t2
    and #7
    tay
    sep #$20
    .a8
    lda pw_exm,x
    ora pw_mask,y
    sta pw_exm,x
    rep #$20
    .a16
    lda pw_err
    jne @bad
    dec pw_cnt
    bne @e
@replay:
    jsr pw_iter_start
@rp:
    jsr pw_iter_next
    jcc @rpd
    jsr pw_save_t
    jsr pw_human
    bcc @cpu
    jsr pw_get_res
    bra @store
@cpu:
    lda t2
    lsr a
    lsr a
    lsr a
    tax
    lda t2
    and #7
    tay
    sep #$20
    .a8
    lda pw_exm,x
    and pw_mask,y
    rep #$20
    .a16
    beq @sim
    lda t2
    asl a
    tax
    lda c_res,x
    and #$00FF
    sta cs_hs
    lda c_res+1,x
    and #$00FF
    sta cs_as
    ldx t2
    lda c_so,x
    and #$00FF
    sta cs_so
    bra @store
@sim:
    jsr pw_sim
@store:
    jsr pw_load_t
    jsr comp_store
    lda pw_err
    jne @bad
    bra @rp
@rpd:
    ; controle et fin du flux
    jsr pw_chk
    sta pw_v3
    ldy #10
    jsr pw_get
    cmp pw_v3
    jne @bad
    lda pw_err
    jne @bad
    lda pw_max
    sec
    sbc pw_pos
    cmp #5
    jcs @bad
    tay
    beq :+
    jsr pw_get
    jne @bad
:   ; etat final
    stz c_match
    lda pw_done
    beq @run
    lda #1
    sta c_done
    jsr comp_rounds
    dec a
    sta c_round
    lda c_type
    bne @ok
    jsr compute_table
    lda st_order
    and #$00FF
    tax
    lda c_teams,x
    and #$00FF
    sta c_champ
    bra @ok
@run:
    stz c_done
    lda pw_tr
    sta c_round
    lda pw_tm
    sta c_match
@ok:
    sec
    rts
@bad:
    clc
    rts

; -----------------------------------------------------------------------------
;  pw_show : A = premiere ligne -> mot de passe en groupes de 4 (5 groupes par ligne)
; -----------------------------------------------------------------------------
pw_show:
    .a16
    .i16
    sta pw_row
    lda pw_len_ok
    bne :+
    lda #UI_HI
    sta t0
    lda pw_row
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc #4 * 2
    tax
    ldy #.loword(str_pw_long)
    jmp print
:
; pw_show_txt : pw_txt / pw_len a partir de la ligne pw_row (curseur si pw_cur)
pw_show_txt:
    .a16
    .i16
    stz pw_k
@l: lda pw_k
    cmp #PW_MAXC
    bcs @d
    ; ligne = k / 20, colonne = 4 + (k % 20) + (k % 20) / 4
    ldx #20
    jsr divu
    clc
    adc pw_row
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    sta pw_v3
    lda RDMPYL
    and #$00FF
    sta pw_v2
    lsr a
    lsr a
    clc
    adc pw_v2
    clc
    adc #4
    asl a
    clc
    adc pw_v3
    tax
    lda pw_k
    cmp pw_len
    bcc @ch
    bne @emp
    lda pw_cur                  ; curseur de saisie
    beq @emp
    lda #('_' - 32 + UI_HI)
    bra @put
@emp:
    lda #0
    bra @put
@ch:
    phx
    tax
    lda pw_txt,x
    plx
    and #$00FF
    sec
    sbc #32
    clc
    adc #UI_A
@put:
    sta bg3_map,x
    inc pw_k
    bra @l
@d: lda #1
    sta bg3_dirty
    rts

; -----------------------------------------------------------------------------
;  pw_entry : saisie d'un mot de passe -> C = 1 si une competition a ete chargee
; -----------------------------------------------------------------------------
PW_COLS = 8

pw_entry:
    .a16
    .i16
    stz pw_len
    stz pw_kx
    stz pw_ky
    lda #1
    sta pw_cur
    jsr safe_screen_off
    jsr bg3_clear
    jsr oam_clear
    jsr ui_fill
    jsr comp_title
    lda #UI_HI
    sta t0
    ldx #TPOS(4, 3)
    ldy #.loword(str_pw_enter)
    jsr print
    lda #UI_ATTR
    sta t0
    ldx #TPOS(1, 25)
    ldy #.loword(str_pw_help)
    jsr print
    ; grille
    stz pw_k
@g: lda pw_k
    tay
    lda pw_chars,y
    and #$00FF
    sec
    sbc #32
    clc
    adc #UI_ATTR
    pha
    jsr pw_kpos
    pla
    sta bg3_map,x
    inc pw_k
    lda pw_k
    cmp #32
    bne @g
    jsr pw_edraw
    jsr screen_on
@loop:
    jsr ui_wait
    lda t7
    beq @loop
    bit #JOY_START
    jne @end
    bit #JOY_B
    beq @nb
    lda pw_len
    bne :+
    clc                         ; B sur un mot de passe vide : retour
    rts
:   dec pw_len
    jsr ui_click
    jsr pw_edraw
    bra @loop
@nb:
    bit #JOY_A
    beq @move
    lda pw_len
    cmp #PW_MAXC
    bcs @loop
    lda pw_ky
    asl a
    asl a
    asl a
    adc pw_kx
    tay
    lda pw_chars,y
    ldx pw_len
    sep #$20
    .a8
    sta pw_txt,x
    rep #$20
    .a16
    inc pw_len
    jsr ui_click
    jsr pw_edraw
    bra @loop
@move:
    lda t7
    bit #JOY_LEFT
    beq :+
    lda pw_kx
    dec a
    and #PW_COLS - 1
    sta pw_kx
    bra @mv
:   bit #JOY_RIGHT
    beq :+
    lda pw_kx
    inc a
    and #PW_COLS - 1
    sta pw_kx
    bra @mv
:   bit #JOY_UP
    beq :+
    lda pw_ky
    dec a
    and #3
    sta pw_ky
    bra @mv
:   bit #JOY_DOWN
    jeq @loop
    lda pw_ky
    inc a
    and #3
    sta pw_ky
@mv:
    jsr ui_click
    jsr pw_edraw
    jmp @loop
@end:
    jsr pw_decode
    bcs @good
    lda #SFX_BUZZER
    jsr sfx_play
    lda #UI_HI
    sta t0
    ldx #TPOS(4, 22)
    ldy #.loword(str_pw_bad)
    jsr print
    jmp @loop
@good:
    lda #SFX_OK
    jsr sfx_play
    sec
    rts

; pw_kpos : pw_k = touche -> X = position dans bg3_map
pw_kpos:
    .a16
    lda pw_k
    and #PW_COLS - 1
    sta pw_v2
    asl a
    adc pw_v2                   ; *3
    clc
    adc #5
    asl a
    sta pw_v2
    lda pw_k
    lsr a
    lsr a
    lsr a
    asl a
    clc
    adc #13                     ; lignes 13, 15, 17, 19
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc pw_v2
    tax
    rts

pw_edraw:
    .a16
    .i16
    ; efface le message d'erreur
    ldx #TPOS(4, 22)
    ldy #24
    lda #0
    jsr fill_tiles
    lda #5
    sta pw_row
    jsr pw_show_txt
    ; curseur de la grille
    lda pw_k
    pha
    stz pw_k
@c: jsr pw_kpos
    dex
    dex
    lda pw_ky
    asl a
    asl a
    asl a
    adc pw_kx
    cmp pw_k
    bne :+
    lda #('>' - 32 + UI_HI)
    bra :++
:   lda #UI_ATTR
:   sta bg3_map,x
    inc pw_k
    lda pw_k
    cmp #32
    bne @c
    pla
    sta pw_k
    lda #1
    sta bg3_dirty
    rts

.segment "RODATA"
pw_mask:  .byte $01, $02, $04, $08, $10, $20, $40, $80
pw_chars: .byte "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"
pw_xor:   .byte 19, 7, 28, 2, 14, 25, 9, 30, 4, 17, 11, 22, 1, 27, 13, 6
str_pw:       .byte "PASSWORD", 0
str_pw_enter: .byte "ENTER PASSWORD", 0
str_pw_help:  .byte "A ADD  B DELETE  START OK", 0
str_pw_bad:   .byte "WRONG PASSWORD", 0
str_pw_long:  .byte "TOO LONG - USE SAVE", 0
.segment "CODE2"
