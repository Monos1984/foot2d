; NEXUS BALL - carte memoire WRAM ($0000-$1FFF, accessible avec DB=$80)

.segment "ZEROPAGE"
t0:         .res 2          ; temporaires (non preserves entre routines)
t1:         .res 2
t2:         .res 2
t3:         .res 2
t4:         .res 2
t5:         .res 2
t6:         .res 2
t7:         .res 2
ptr:        .res 3          ; pointeur long
nmi_ready:  .res 1          ; 1 = la frame est prete a etre transferee
nmi_count:  .res 2
frame:      .res 2
is_pal:     .res 2          ; 0 NTSC, 1 PAL
rng:        .res 2
joy_cur:    .res 4          ; manettes 1 et 2
joy_new:    .res 4          ; boutons appuyes cette frame
joy_old:    .res 4
inidisp:    .res 1          ; valeur a ecrire dans INIDISP au NMI
nmi_on:     .res 2          ; 1 = NMI actif
scroll_x:   .res 2          ; scroll BG1 (pixels)
scroll_y:   .res 2
cp:         .res 2          ; joueur courant (index*2)
lz_src:     .res 4          ; decompression LZSS
lz_size:    .res 2
lz_d:       .res 2
lz_flags:   .res 1
lz_cnt:     .res 1
lz_len:     .res 1
l_dx:       .res 2          ; parametres de lancer du ballon
l_dy:       .res 2
l_spd:      .res 2
l_zt:       .res 2
l_err:      .res 2
l_foot:     .res 2
l_t:        .res 2
tr_zp:      .res 3          ; pointeur long (traductions, regles)
tr_zp2:     .res 2

.segment "BSS"
oam_buf:    .res 544
oam_ptr:    .res 2

; ---- constantes de region (copiees depuis la table NTSC ou PAL)
rc:         .res 128

; ---- copie de la tilemap BG3 (HUD, messages, menus)
bg3_map:    .res 32*32*2
bg3_dirty:  .res 2
msg_time:   .res 2
hud_cache:  .res 8
pal_dirty:  .res 2
ring_pal:   .res 4          ; couleurs des anneaux (CGRAM 42-43), suivies du public
crowd_pal:  .res 6
ring_base:  .res 4
crowd_t:    .res 2
crowd_fast: .res 2

; ---- options
opt_radar:  .res 2
game_mode:  .res 2
menu_sel:   .res 2
menu_len:   .res 2          ; 0..3 -> 2..5 minutes par periode
pause_sel:  .res 2
sort_buf:   .res 14
sort_key:   .res (NUM_PLAYERS + 1) * 2
b_gkdone:   .res 2          ; le gardien a deja tente l'arret sur ce tir
near_tmp:   .res 6
gk_best:    .res 2
ai_gx:      .res 2
tk_foul:    .res 2
dbg_fouls:  .res 2          ; compteurs de test (tools/lrtest.py)
dbg_turnovers: .res 2
dbg_late:   .res 2
dbg_gkzone: .res 2
near_team:  .res 4          ; joueur de champ le plus proche du ballon, par equipe
near_dist:  .res 4
title_scroll: .res 2

; ---- joueurs (tableaux de mots, index = numero*2)
p_x:        .res NUM_PLAYERS*2
p_y:        .res NUM_PLAYERS*2
p_vx:       .res NUM_PLAYERS*2
p_vy:       .res NUM_PLAYERS*2
p_z:        .res NUM_PLAYERS*2   ; saut (12.4)
p_vz:       .res NUM_PLAYERS*2
p_state:    .res NUM_PLAYERS*2
p_timer:    .res NUM_PLAYERS*2
p_cd:       .res NUM_PLAYERS*2
p_dir:      .res NUM_PLAYERS*2   ; direction 0..7 (0 = droite, sens horaire)
p_flip:     .res NUM_PLAYERS*2   ; $4000 si tourne vers la gauche
p_anim:     .res NUM_PLAYERS*2
p_spr:      .res NUM_PLAYERS*2
p_team:     .res NUM_PLAYERS*2
p_role:     .res NUM_PLAYERS*2
p_homex:    .res NUM_PLAYERS*2   ; position de formation (pixels, equipe attaquant vers la droite)
p_homey:    .res NUM_PLAYERS*2
p_tx:       .res NUM_PLAYERS*2   ; cible IA (pixels)
p_ty:       .res NUM_PLAYERS*2
p_think:    .res NUM_PLAYERS*2
p_want:     .res NUM_PLAYERS*2   ; directions voulues (bits manette) pour ce frame
p_act:      .res NUM_PLAYERS*2   ; actions voulues (bits manette) pour ce frame
p_speed:    .res NUM_PLAYERS*2
p_power:    .res NUM_PLAYERS*2
p_passst:   .res NUM_PLAYERS*2
p_kickst:   .res NUM_PLAYERS*2
p_ctrlst:   .res NUM_PLAYERS*2
p_defst:    .res NUM_PLAYERS*2
p_stam:     .res NUM_PLAYERS*2
p_fatigue:  .res NUM_PLAYERS*2   ; 0..255
p_human:    .res NUM_PLAYERS*2   ; 0 IA, 1 manette 1, 2 manette 2

; ---- ballon
b_x:        .res 2
b_y:        .res 2
b_z:        .res 2
b_vx:       .res 2
b_vy:       .res 2
b_vz:       .res 2
b_owner:    .res 2          ; numero du porteur ou NO_OWNER
b_last:     .res 2          ; dernier joueur a avoir touche
b_nograb:   .res 2          ; joueur qui ne peut pas reprendre
b_nograb_t: .res 2
b_shot:     .res 2          ; SHOT_*
b_points:   .res 2          ; points accordes si le tir entre
b_carry:    .res 2          ; temps de port (tu)
b_passt:    .res 2          ; temps depuis la derniere passe (fautes tardives)
b_passer:   .res 2
b_prevx:    .res 2

; ---- match
m_state:    .res 2
m_timer:    .res 2
m_sec:      .res 2          ; secondes restantes de la periode
m_acc:      .res 2          ; accumulateur tu -> secondes
m_half:     .res 2
m_len:      .res 2          ; duree d'une periode (s)
score:      .res 4
kick_team:  .res 2
first_kick: .res 2
team_dir:   .res 4          ; sens d'attaque par equipe : 0 = vers la droite, 1 = vers la gauche
ctrl:       .res 4          ; joueur controle par manette 1 / 2 ($FF aucun)
pad_team:   .res 4          ; equipe de la manette 1 / 2 ($FF aucune)
poss_team:  .res 2          ; equipe en possession ($FF aucune)
switch_t:   .res 4
foul_team:  .res 2          ; equipe qui recoit le ballon apres faute
foul_x:     .res 2
foul_y:     .res 2
foul_kind:  .res 2
sent_off:   .res 2          ; joueur exclu temporairement ($FF aucun)
sent_t:     .res 2
cam_x:      .res 2          ; camera (12.4)
cam_y:      .res 2
ai_phase:   .res 2

; ---- equipes, effectifs, tactiques
team_id:    .res 4          ; equipe choisie (0..15) par cote
kit_away:   .res 4          ; 1 = maillot exterieur
form_id:    .res 4          ; formation (0..5)
tact:       .res 2*6*2      ; tactiques : equipe*12 + parametre*2 (0..2)
lineup:     .res 2*6*2      ; index dans l'effectif (0..11) : equipe*12 + slot*2
r_fat:      .res 2*12*2     ; fatigue des 12 joueurs de chaque effectif
difficulty: .res 2          ; 0 EASY .. 3 EXPERT
draw_rule:  .res 2          ; 0 egalite possible, 1 prolongation + tirs au but
near2_team: .res 4          ; deuxieme joueur le plus proche du ballon
near2_dist: .res 4
ui_sel:     .res 2
ui_team:    .res 2
ui_pick:    .res 2
ui_back:    .res 2

; ---- prolongation / tirs au but
so_active:  .res 2
so_team:    .res 2
so_goals:   .res 4
so_taken:   .res 4
so_phase:   .res 2
so_shooter: .res 2
str_buf:    .res 34
stadium_id: .res 2

; ---- son
snd_q:      .res 16
snd_qn:     .res 2
snd_cnt:    .res 2
title_music: .res 2

; ---- competition en cours (bloc sauvegarde tel quel en SRAM)
comp_active: .res 2         ; 1 = le match en cours appartient a la competition
comp_data:
c_slot:     .res 2          ; 0 CHAMPIONSHIP, 1 CUP, 2 CUSTOM
c_type:     .res 2          ; 0 ligue, 1 coupe
c_n:        .res 2          ; nombre de participants
c_nn:       .res 2          ; ligue : taille de calendrier (paire) ; coupe : = c_n
c_len:      .res 2
c_diff:     .res 2
c_stad:     .res 2          ; 0..5, 6 = rotation
c_round:    .res 2
c_match:    .res 2
c_done:     .res 2
c_champ:    .res 2          ; equipe championne
c_cur:      .res 2          ; match global en cours (humain)
c_watch:    .res 2          ; 1 = regarder les matchs CPU contre CPU
c_teams:    .res 16         ; equipes participantes (index participant -> equipe)
c_ctrl:     .res 24         ; par equipe : 0 absente, 1 CPU, 2 P1, 3 P2
c_res:      .res 240        ; scores (domicile, exterieur), $FF = non joue
c_br:       .res 32         ; coupe : participant par case du tableau
c_so:       .res 16         ; coupe : vainqueur aux tirs au but (0 dom, 1 ext, $FF aucun)
c_seed:     .res 2          ; graine des matchs simules (mot de passe)
comp_data_end:
; classement (calcule)
st_order:   .res 16
st_pts:     .res 32
st_w:       .res 32
st_d:       .res 32
st_l:       .res 32
st_pf:      .res 32
st_pa:      .res 32
c_slot_sel: .res 2
cs_hs:      .res 2
cs_as:      .res 2
cs_so:      .res 2
sim_lh:     .res 2
sim_la:     .res 2
sim_th:     .res 2
sim_ta:     .res 2
sim_t:      .res 2
sim_p:      .res 2
sim_k:      .res 2
pm_h:       .res 2
pm_a:       .res 2
pm_len:     .res 2
pm_diff:    .res 2
pm_rule:    .res 2
hub_i:      .res 2
trec_id:    .res 2          ; equipe actuellement copiee dans trec_buf ($FFFF : aucune)
trec_buf:   .res 262

; ---- editeurs
ct_tmp:     .res 2
ts_cur:     .res 2
ed_slot:    .res 2
ed_pl:      .res 24
ed_team:    .res 262
te_c1:      .res 2
te_c2:      .res 2
te_k:       .res 2
te_src:     .res 24
kb_buf:     .res 18
kb_max:     .res 2
kb_x:       .res 2
kb_y:       .res 2
vt_n:       .res 2
vt_list:    .res 24
cs_top:     .res 2
rep_bits:   .res 2
rep_t:      .res 2
p_skin:     .res NUM_PLAYERS*2   ; 1 = peau foncee (palette OBJ 5/6)
pm_conf:    .res 2
pm_side:    .res 2
pm_cpad:    .res 2
opt_ctrl:   .res 2          ; configuration des boutons (0..2)
; ---- statistiques du match
st_shots:   .res 4
st_poss:    .res 4          ; frames de possession
st_fouls:   .res 4
st_saves:   .res 4
hs_row:     .res 2
dbg_saves:  .res 2
dbg_ringmiss: .res 2
dbg_og:     .res 2          ; buts contre son camp (dont gardien)
dbg_gkog:   .res 2

; ---- calques des menus
bg1_mode:   .res 2          ; 0 stade, 1 fond des menus
bg2_dirty:  .res 2
tm_sh:      .res 1
ts_sh:      .res 1
cgw_sh:     .res 1
cga_sh:     .res 1
bar_row:    .res 2
bar_x0:     .res 2
bar_w:      .res 2
pn_x:       .res 2
pn_y:       .res 2
pn_w:       .res 2
pn_h:       .res 2
pn_r:       .res 2
pn_l:       .res 2
pn_m:       .res 2
pn_rt:      .res 2
hdma_sh:    .res 2          ; valeur de HDMAEN ecrite au NMI
fade_v:     .res 2
dp_x:       .res 2
bg2_mode:   .res 2          ; tiles BG2 en VRAM : 1 logo, 2 tribunes
bg2_hofs:   .res 2
bg2_vofs:   .res 2
b_orient:   .res 2          ; orientation du ballon ovale (0..3)
lang:       .res 2          ; 0 anglais, 1 francais
tr_y:       .res 2
tr_h:       .res 2
tr_c:       .res 2
tr_buf:     .res 40
rl_page:    .res 2
sw_cand:    .res 2          ; changement automatique de joueur
sw_lim:     .res 2
ad_flag:    .res 2          ; mi-temps : ecran de publicite affiche
b_relx:     .res 2          ; x (pixels) du dernier lanceur, $FFFF : pas un tir
gv_team:    .res 2
bn_team:    .res 2          ; bandeau des messages
bn_pl:      .res 2
bn_msg:     .res 2
bn_attr:    .res 2
bn_n:       .res 2
bn_w:       .res 2
bn_x:       .res 2
bn_t:       .res 2
bn_line:    .res 56
gs_team:    .res 2
title_mode: .res 2          ; 0 : ecran titre PRESS START, 1 : menu
ts_side:    .res 2          ; choix des equipes
ts_init:    .res 2
ts_cur_side: .res 2
ts_x:       .res 2
ts_row:     .res 2
ts_ptr:     .res 2
ts_n:       .res 2
ts_k:       .res 2
ts_ofs:     .res 2
ts_sum:     .res 2
ts_slot:    .res 2
ts_t:       .res 2
ts_attr:    .res 2
fs_buf:     .res 6
kz_cx:      .res 2
kz_cy:      .res 2
kz_r:       .res 2
ko_active:  .res 2          ; coup d'envoi en cours (passe obligatoire)
ko_p1:      .res 2
ko_p2:      .res 2
ko_t:       .res 2
ko_lim:     .res 2
ko_y:       .res 2
rs_mode:    .res 2          ; ecran de score
rs_t:       .res 2
rs_row:     .res 2
rs_pos:     .res 2
rs_a:       .res 2
rs_b:       .res 2
rs_aa:      .res 2
rs_s:       .res 2
rs_la:      .res 2
ko_mode:    .res 2          ; 0 coup d'envoi, 1 coup franc
chant_idx:  .res 2
fk_x:       .res 2
fk_y:       .res 2
gk_sign:    .res 2
gk_x0:      .res 2
zc_t:       .res 2          ; temps du ballon dans une raquette
zc_zone:    .res 2
zc_team:    .res 2
b_prevy:    .res 2
b_prevz:    .res 2
rt_y:       .res 2          ; point de franchissement du plan de l'anneau
rt_z:       .res 2
rc_num:     .res 2
rc_den:     .res 2
p_pen:      .res NUM_PLAYERS*2   ; exclusion temporaire restante (s)
fa_base:    .res 2
sq_new:     .res 2
sq_pri:     .res 2
cs_a:       .res 2          ; sauvegarde A/B des competitions
cs_b:       .res 2
cs_va:      .res 2
cs_vb:      .res 2
cs_ga:      .res 2
cs_gb:      .res 2
cs_gen:     .res 2
cs_old:     .res 2
oam_used:   .res 2
cam_t:      .res 2
ad_t:       .res 2
prof_max:   .res 24
prof_last:  .res 2
sort_ok:    .res 2
oam_prev:   .res 2
oam_lim:    .res 2
kit_pick:   .res 4          ; tenue choisie par cote : 0 auto, 1 domicile, 2 exterieur
pc_sel:     .res 2
; ---- mot de passe des competitions
pw_buf:     .res 64
pw_txt:     .res 100
pw_exm:     .res 16
pw_pos:     .res 2
pw_err:     .res 2
pw_h:       .res 2
pw_v:       .res 2
pw_n:       .res 2
pw_b:       .res 2
pw_bitv:    .res 2
pw_max:     .res 2
pw_k:       .res 2
pw_v2:      .res 2
pw_v3:      .res 2
pw_cnt:     .res 2
pw_r:       .res 2
pw_m:       .res 2
pw_tr:      .res 2
pw_tm:      .res 2
pw_done:    .res 2
pw_sr:      .res 2
pw_sm:      .res 2
pw_s0:      .res 2
pw_s1:      .res 2
pw_s2:      .res 2
pw_s7:      .res 2
pw_sh:      .res 2
pw_sa:      .res 2
pw_len:     .res 2
pw_len_ok:  .res 2
pw_row:     .res 2
pw_cur:     .res 2
pw_kx:      .res 2
pw_ky:      .res 2
cm_has:     .res 2
rs_win:     .res 2          ; ecran de score : equipe gagnante ($FF aucune)
ct_mode:    .res 2          ; tirage au sort
ct_win:     .res 2
ct_side:    .res 2
ct_q:       .res 2
ct_qn:      .res 2
ct_d:       .res 2
ct_y:       .res 2
ct_vy:      .res 2
ct_land:    .res 2
ct_done:    .res 2
ct_t:       .res 2
ct_ptr:     .res 2
ct_i:       .res 2
ct_k:       .res 2
ct_k2:      .res 2
in_x:       .res 12         ; presentation : x des joueurs
in_t:       .res 2
in_arr:     .res 2
in_cheer:   .res 2
spk_t:      .res 2          ; etincelles du but
spk_x:      .res 2
spk_y:      .res 2
spk_r:      .res 2
spk_i:      .res 2
cr_row:     .res 2          ; credits
cr_col:     .res 2
cr_ptr:     .res 2
tf_t:       .res 2          ; ecran titre : animation
tf_x:       .res 2
tf_y:       .res 2
pz_restart: .res 2
wx_init:    .res 2          ; meteo du stade
wx_i:       .res 2
wx_x:       .res 24
wx_y:       .res 24
p_pts:      .res NUM_PLAYERS*2   ; points marques par joueur (etoile du match)
rs_bv:      .res 2
rs_bp:      .res 2
rs_bt:      .res 2
rs_name:    .res 10          ; pause : rejouer le match
shake_t:    .res 2          ; frames de tremblement de l'ecran
pw_key:     .res 2
cs_sz:      .res 2
cf_title:   .res 2          ; titre de la confirmation
cf_menu:    .res 2          ; 1 : confirmation dans un menu (ui_wait)
