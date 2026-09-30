#pragma once
// Journal de débogage : fil d'Ariane des dernières actions, écrit dans debug.log en cas de plantage
void crashLogInit();
void crashMark(const char* fmt, ...);        // ajoute une étape au fil d'Ariane (ex. « tirage au sort : Coupe de France, 7e tour »)
void crashLogWrite(const char* reason);      // écrit debug.log immédiatement
