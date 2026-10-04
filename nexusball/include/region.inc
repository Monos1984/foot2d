; NEXUS BALL - constantes dependantes de la region (50/60 Hz)
;
; Ce fichier est inclus deux fois par src/region.asm : une fois pour construire
; la table NTSC, une fois pour la table PAL. Les valeurs sont ecrites en unites
; NTSC ; les macros calculent automatiquement la valeur PAL :
;   RCR sym, ntsc, pal  valeur brute differente par region
;   RCV sym, v          vitesse        (x 6/5 en PAL)
;   RCA sym, v          acceleration   (x 36/25 en PAL)

RCR RC_TDEC,    5, 6        ; tu retires par frame
RCR RC_FPS,     60, 50
RCA RC_GRAV,    16          ; gravite (VZ en 1/64 px)
RCR RC_FRIC_G,  6, 7        ; frottement au sol  (v -= v*k/256)
RCR RC_FRIC_A,  1, 1        ; frottement en l'air
RCR RC_FRIC_P,  40, 47      ; freinage d'un joueur sans direction

RCV RC_SPD1,    19          ; vitesse max selon SPEED (1..9)
RCV RC_SPD2,    20
RCV RC_SPD3,    21
RCV RC_SPD4,    22
RCV RC_SPD5,    23
RCV RC_SPD6,    24
RCV RC_SPD7,    25
RCV RC_SPD8,    26
RCV RC_SPD9,    27
RCV RC_SPRINT,  9           ; bonus de sprint
RCV RC_CARRYPEN, 2          ; malus en portant le ballon
RCA RC_ACCEL,   4           ; acceleration des joueurs
RCV RC_JUMP,    190         ; impulsion de saut (VZ)
RCR RC_SHOT_T,  34, 28      ; duree de vol visee d une frappe (frames)
RCR RC_THROW_T, 26, 22      ; duree de vol visee d un lancer (frames)
RCV RC_TACKLE,  42          ; vitesse de charge
RCV RC_GKSPD,   22

RCV RC_PASS_H,  52          ; passe a la main
RCV RC_PASS_K,  74          ; passe au pied
RCV RC_SHOT_K,  90          ; frappe
RCV RC_THROW,   60          ; lancer vers l'anneau
RCV RC_GKTHROW, 58
RCV RC_DROP,    18          ; ballon lache sur une charge
RCV RC_VZ_PASSH, 70         ; VZ passe main
RCV RC_VZ_PASSK, 230        ; VZ passe au pied (lobee)
RCV RC_VZ_KICK, 120         ; VZ frappe directionnelle (X)
RCV RC_VZ_DROP, 120
RCV RC_PICKUP_V, 70         ; au-dela, ballon trop rapide pour etre controle sans CONTROL
