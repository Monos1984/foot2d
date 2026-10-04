; =============================================================================
;  teams.asm - donnees des equipes (prototype : 2 equipes officielles)
;
;  Par joueur, 7 octets : SPEED POWER PASS KICK CONTROL DEFENSE STAMINA (1..9)
;  Gardien             : REFLEX POWER POSITION THROW CATCH POSITION STAMINA
;  Ordre sur le terrain : GK, DF, DF, MF, MF, FW
; =============================================================================

.segment "RODATA"

TEAM_STATS_SIZE = 6 * 7

team_short:
    .byte "EUR", 0
    .byte "ORI", 0

team_name_0: .byte "EUROPA ICE", 0
team_name_1: .byte "ORION STARS", 0
team_names:
    .word .loword(team_name_0), .loword(team_name_1)

; Europa Ice : equipe technique et rapide
team_stats_0:
    .byte 6, 5, 5, 5, 6, 6, 6       ; GK
    .byte 6, 5, 6, 4, 6, 7, 6       ; DF
    .byte 6, 5, 5, 4, 5, 7, 7       ; DF
    .byte 7, 4, 8, 6, 7, 5, 6       ; MF
    .byte 8, 4, 7, 5, 7, 4, 7       ; MF
    .byte 8, 5, 6, 7, 7, 3, 6       ; FW
; Orion Stars : equipe puissante
team_stats_1:
    .byte 5, 7, 6, 6, 6, 6, 6       ; GK
    .byte 5, 8, 5, 5, 5, 7, 7       ; DF
    .byte 5, 8, 4, 6, 5, 7, 7       ; DF
    .byte 6, 7, 6, 6, 6, 5, 7       ; MF
    .byte 6, 7, 5, 7, 6, 5, 6       ; MF
    .byte 6, 8, 5, 8, 6, 3, 6       ; FW

team_stats:
    .word .loword(team_stats_0), .loword(team_stats_1)

.segment "CODE"
