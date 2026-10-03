@echo off
rem France Foot 2D - compilation Windows 64 bits (MinGW + raylib)
rem RAYLIB peut pointer soit vers raylib precompilee (include/lib), soit vers l'arbre source raylib (src).
if "%RAYLIB%"=="" set RAYLIB=C:\raylib\raylib

if exist "%RAYLIB%\include\raylib.h" goto :prebuilt
if exist "%RAYLIB%\src\raylib.h" goto :source

echo ERREUR : raylib introuvable dans %RAYLIB%
echo Definissez RAYLIB vers le dossier raylib-5.5_win64_mingw-w64 ou vers l'arbre source raylib.
exit /b 1

:prebuilt
set RAYINC=%RAYLIB%\include
set RAYLIBDIR=%RAYLIB%\lib
goto :build

:source
set RAYINC=%RAYLIB%\src
set RAYLIBDIR=%RAYLIB%\src

:build
if not exist build mkdir build
windres -I res res\foot.rc -O coff -o build\foot.res.o || goto :err
g++ -std=c++17 -O2 -Isrc -I"%RAYINC%" src\*.cpp build\foot.res.o -o foot2d.exe -L"%RAYLIBDIR%" -lraylib -lopengl32 -lgdi32 -lwinmm -lshell32 -static -mwindows || goto :err
echo OK : foot2d.exe
exit /b 0

:err
echo ECHEC de la compilation
exit /b 1
