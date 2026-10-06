; NEXUS BALL - constantes de jeu
;
; Coordonnees monde : fixed-point 12.4 (16 = 1 pixel)
; Hauteur Z         : 12.4 ; vitesse verticale VZ en 1/64 pixel (z += vz/4)
; Temps             : unites de 1/300 s ("tu"). NTSC retire 5 tu par frame,
;                     PAL 6 tu par frame : tous les timers durent le meme temps reel.

FP          = 16
TU_SEC      = 300

; ---- terrain (pixels monde, cf. tools/gfx.py)
WORLD_W     = 512
WORLD_H     = 320
FIELD_L     = 24
FIELD_R     = 487
FIELD_T     = 72
FIELD_B     = 295
FIELD_CX    = 256
FIELD_CY    = 184
RING_Z      = 20
RING_R      = 22
ZONE_R      = 40             ; zone protegee du gardien
LONG_R      = 150            ; ligne des 2 points

CAM_MAX_X   = WORLD_W - 256
CAM_MAX_Y   = WORLD_H - 224

; ---- entites
NUM_PLAYERS = 12
TEAM_SIZE   = 6
NO_OWNER    = $FF

; roles (formation 2-2-1)
ROLE_GK     = 0
ROLE_DF     = 1
ROLE_MF     = 2
ROLE_FW     = 3

; etats joueur
PS_NORMAL   = 0
PS_TACKLE   = 1              ; charge en cours
PS_DOWN     = 2              ; au sol
PS_KICK     = 3              ; animation de frappe / passe
PS_DIVE     = 4              ; plongeon gardien
PS_CELEB    = 5
PS_OUT      = 6              ; exclu temporairement

; ---- timers (tu)
T_CARRY_MAX   = 4 * TU_SEC     ; port du ballon maximum
T_CARRY_WARN  = 3 * TU_SEC
T_NOGRAB      = 60             ; le passeur ne peut pas reprendre tout de suite
T_TACKLE      = 85             ; duree d'une charge
T_TACKLE_CD   = 220            ; recuperation apres une charge
T_DOWN        = 260            ; temps au sol
T_KICKANIM    = 60
T_KICKOFF     = 1 * TU_SEC
T_GOAL        = 3 * TU_SEC
T_GK_HOLD     = 270
T_AI_THINK    = 45             ; reflexion d'un joueur IA
T_MSG         = 2 * TU_SEC
T_LATE_HIT    = 90             ; contact tardif apres une passe = faute
T_SWITCH_AUTO = 150

; ---- etats du match
MS_KICKOFF  = 0
MS_PLAY     = 1
MS_GOAL     = 2
MS_HALF     = 3
MS_END      = 4
MS_FOUL     = 5
MS_SHOOT    = 6

; ---- types de tir (points)
SHOT_NONE   = 0
SHOT_HAND   = 1
SHOT_KICK   = 2

; ---- modes de jeu
MODE_1P     = 0
MODE_2P     = 1
MODE_CPU    = 2

; ---- OBJ (tiles)
FEET_ROW    = 22             ; ligne des pieds dans le cadre 16x32 d'un joueur
SPR_STAND   = 0
SPR_RUN1    = 2
SPR_RUN2    = 4
SPR_RUN3    = 6
SPR_KICK    = 8
SPR_THROW   = 10
SPR_CHARGE  = 12
SPR_FALL    = 14
SPR_CELEB   = 64
SPR_DIVE    = 66
SPR_CATCH   = 68
SPR_STUMBLE = 70
SPR_CARRY1  = 72
SPR_CARRY2  = 74
SPR_CARRYS  = 76
SPR_JUMP    = 78
SPR_RUNAM   = 128            ; cadres 16-23 (0.16.0)
SPR_RUNBM   = 130
SPR_RUN4    = 132
SPR_CARRYP  = 134
SPR_KICK2   = 136
SPR_THROW2  = 138
SPR_STAND2  = 140
SPR_CELEB2  = 142
SPR_OVAL    = 192            ; ballon ovale 16x16 : + 4*orientation + 2*phase
SPR_BALL    = 224
SPR_SHADOW  = 225
SPR_CUR1    = 226
SPR_CUR2    = 227
SPR_TRAIL1  = 228            ; trainee du ballon (8x8)
SPR_TRAIL2  = 229
TRAIL_SPEED = 80             ; |vx| + |vy| (12.4) a partir duquel la trainee apparait
SPR_DOTW    = 230
SPR_WARN    = 231
SPR_BALLHI  = 232
SPR_OSHADOW = 238
SPR_COIN    = 234            ; piece du tirage au sort (face / tranche, palettes 0-1)
SPR_COINE   = 236

; ---- VRAM (adresses en mots)
VRAM_BG1_CHR = $0000
VRAM_BG1_MAP = $4000
VRAM_BG3_CHR = $5000
VRAM_BG3_MAP = $5800
VRAM_OBJ_CHR = $6000
VRAM_BG2_CHR = $7000            ; logo du titre (4bpp)
VRAM_BG2_MAP = $5C00

; ---- texte BG3 : tile = ascii - 32 (+64 sur panneau), palette 0, priorite haute
TXT_ATTR    = $2000
TXT_PANEL   = 64
