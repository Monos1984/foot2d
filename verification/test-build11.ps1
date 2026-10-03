$ErrorActionPreference='Stop'
$cc='C:\Users\monos\Documents\Codex\2026-08-17\referenced-chatgpt-conversation-this-is-an\.tools\llvm-mingw-20260616-ucrt-x86_64\bin\clang++.exe'
$objects=@(Get-ChildItem build -Filter '*.o' | Where-Object {$_.Name -notin 'main.o','foot.res.o'} | ForEach-Object FullName)
foreach($test in @('test_build11_stats','test_build10_match','test_build4_roles','test_build3_features')){& $cc -std=c++17 -O2 -Isrc '-I..\deps\raylib-5.5_win64_mingw-w64\include' "tools\$test.cpp" @objects -o "build\$test.exe" '-L..\deps\raylib-5.5_win64_mingw-w64\lib' -lraylib -lopengl32 -lgdi32 -lwinmm -lshell32 -static;if($LASTEXITCODE){throw $test};& ".\build\$test.exe";if($LASTEXITCODE){throw $test}}
'BUILD 11 ALL TARGETED TESTS PASS'
