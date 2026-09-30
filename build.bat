@echo off
rem France Foot 2D — compilation Windows (w64devkit / MinGW + raylib)
if "%RAYLIB%"=="" set RAYLIB=C:\raylib\raylib
if not exist build mkdir build
windres -I res res\foot.rc -O coff -o build\foot.res.o || goto :err
g++ -std=c++17 -O2 -I"%RAYLIB%\src" src\*.cpp build\foot.res.o -o foot2d.exe -L"%RAYLIB%\src" -lraylib -lopengl32 -lgdi32 -lwinmm -mwindows -static || goto :err
echo OK : foot2d.exe
exit /b 0
:err
echo ECHEC de la compilation
exit /b 1
