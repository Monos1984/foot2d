param([string]$Compiler = 'clang++', [string]$Raylib = $env:RAYLIB, [switch]$Tests)
$ErrorActionPreference = 'Stop'
if (-not $Raylib) { throw 'Specify -Raylib with the raylib-5.5_win64_mingw-w64 directory.' }
$arch = & $Compiler -dumpmachine
if ($LASTEXITCODE -or $arch -notmatch 'x86_64') { throw 'A Windows x86_64 compiler is required.' }
$ray = (Resolve-Path -LiteralPath $Raylib).Path
Push-Location $PSScriptRoot
try {
  New-Item -ItemType Directory build -Force | Out-Null
  foreach ($f in Get-ChildItem src -Filter '*.cpp') {
    & $Compiler -std=c++17 -O2 -Isrc "-I$ray\include" -c $f.FullName -o "build\$($f.BaseName).o"
    if ($LASTEXITCODE) { throw ('Compilation failed: '+$f.Name) }
  }
  $windres = Join-Path (Split-Path (Get-Command $Compiler).Source) 'llvm-windres.exe'
  if (-not (Test-Path -LiteralPath $windres)) { throw 'llvm-windres.exe is required for the Windows icon and version resource.' }
  & $windres '-Ires' res\foot.rc -O coff -o build\foot.res.o
  if ($LASTEXITCODE) { throw 'Windows resource compilation failed' }
  $objects = @(Get-ChildItem build -Filter '*.o' | ForEach-Object FullName)
  $libs = @("-L$ray\lib",'-lraylib','-lopengl32','-lgdi32','-lwinmm','-lshell32','-static')
  & $Compiler @objects -o FranceFoot2D.exe @libs -mwindows
  if ($LASTEXITCODE) { throw 'Executable link failed' }
  $core = @($objects | Where-Object { (Split-Path $_ -Leaf) -notin 'main.o','foot.res.o' })
  & $Compiler -std=c++17 -O2 -Isrc "-I$ray\include" tools\validate_france.cpp @core -o Verifier-France.exe @libs
  if ($LASTEXITCODE) { throw 'Validator link failed' }
  & .\Verifier-France.exe
  if ($LASTEXITCODE) { throw 'France database validation failed' }
  if ($Tests) {
    & $Compiler -std=c++17 -O2 -Isrc tools\test_france_2627.cpp src\data_france_clubs.cpp -o build\test_france_2627.exe -static
    if ($LASTEXITCODE) { throw 'National test link failed' }
    & .\build\test_france_2627.exe
    if ($LASTEXITCODE) { throw 'National validation failed' }
    $withoutCareer = @($core | Where-Object { (Split-Path $_ -Leaf) -ne 'career.o' })
    foreach ($legacy in @(@{build=10;version=23},@{build=11;version=24},@{build=1;version=25},@{build=2;version=26})) {
      & $Compiler -std=c++17 -O2 -Isrc -c "tools\fixtures\career_build$($legacy.build)_v$($legacy.version).cpp" -o build\legacy-career.testobj
      if ($LASTEXITCODE) { throw 'Legacy writer compile failed' }
      & $Compiler -std=c++17 -O2 -Isrc tools\write_legacy_fixture.cpp build\legacy-career.testobj @withoutCareer -o build\write-legacy.exe @libs
      if ($LASTEXITCODE) { throw 'Legacy writer link failed' }
      & .\build\write-legacy.exe "build/test-legacy-v$($legacy.version).sav"
      if ($LASTEXITCODE) { throw 'Legacy fixture failed' }
    }
    & $Compiler -std=c++17 -O2 -Isrc tools\test_official_2627.cpp @core -o build\test_official_2627.exe @libs
    if ($LASTEXITCODE) { throw 'Integration test link failed' }
    & .\build\test_official_2627.exe
    if ($LASTEXITCODE) { throw 'Integration test failed' }
    & $Compiler -std=c++17 -O2 -Isrc tools\test_nation_context.cpp @core -o build\test_nation_context.exe @libs
    if ($LASTEXITCODE) { throw 'Nation test link failed' }
    & .\build\test_nation_context.exe
    if ($LASTEXITCODE) { throw 'Nation test failed' }
    & $Compiler -std=c++17 -O2 -Isrc tools\test_ballondor.cpp @core -o build\test_ballondor.exe @libs
    if ($LASTEXITCODE) { throw 'Award test link failed' }
    & .\build\test_ballondor.exe
    if ($LASTEXITCODE) { throw 'Award test failed' }
    & $Compiler -std=c++17 -O2 -Isrc tools\test_build2_fixes.cpp @core -o build\test_build2_fixes.exe @libs
    if ($LASTEXITCODE) { throw 'Build 2 fixes test link failed' }
    & .\build\test_build2_fixes.exe
    if ($LASTEXITCODE) { throw 'Build 2 fixes test failed' }
    & $Compiler -std=c++17 -O2 -Isrc "-I$ray\include" tools\test_build3_features.cpp @core -o build\test_build3_features.exe @libs
    if ($LASTEXITCODE) { throw 'Build 3 features test link failed' }
    & $Compiler -std=c++17 -O2 -Isrc "-I$ray\include" tools\test_build4_roles.cpp @core -o build\test_build4_roles.exe @libs
    if ($LASTEXITCODE) { throw 'Build4 test link failed' }
    & .\build\test_build4_roles.exe
    if ($LASTEXITCODE) { throw 'Build4 role tests failed' }
    & $Compiler -std=c++17 -O2 -Isrc tools\test_build5_museum.cpp @core -o build\test_build5_museum.exe @libs
    if ($LASTEXITCODE) { throw 'Museum test link failed' }
    & .\build\test_build5_museum.exe
    if($LASTEXITCODE){throw "Museum validation failed"}
    & $Compiler -std=c++17 -O2 -Isrc tools\test_build7_director.cpp @core -o build\test_build7_director.exe @libs
    if($LASTEXITCODE){throw "Director test link failed"}
    & .\build\test_build7_director.exe
    if ($LASTEXITCODE) { throw 'Director tests failed' }
    & $Compiler -std=c++17 -O2 -Isrc tools\test_build8_sporting.cpp @core -o build\test_build8_sporting.exe @libs
    if ($LASTEXITCODE) { throw 'Sporting test link failed' }
    & .\build\test_build8_sporting.exe
    if ($LASTEXITCODE) { throw 'Sporting tests failed' }
    & $Compiler -std=c++17 -O2 -Isrc "-I$ray\include" tools\test_build9_supporters.cpp @core -o build\test_build9_supporters.exe @libs
    if ($LASTEXITCODE) { throw 'Supporter test link failed' }
    & .\build\test_build9_supporters.exe
    if ($LASTEXITCODE) { throw 'Supporter tests failed' }
    & $Compiler -std=c++17 -O2 -Isrc "-I$ray\include" tools\test_build10_personality.cpp @core -o build\test_build10_personality.exe @libs
    if ($LASTEXITCODE) { throw 'Personality test link failed' }
    & $Compiler -std=c++17 -O2 -Isrc "-I$ray\include" tools\test_build10_match.cpp @core -o build\test_build10_match.exe @libs
    if ($LASTEXITCODE) { throw 'Live match test link failed' }
    & $Compiler -std=c++17 -O2 -Isrc "-I$ray\include" tools\test_build11_stats.cpp @core -o build\test_build11_stats.exe @libs
    if($LASTEXITCODE){throw 'Stats test link failed'}
    & .\build\test_build11_stats.exe
    if($LASTEXITCODE){throw 'Stats tests failed'}
    & .\build\test_build10_match.exe
    if ($LASTEXITCODE) { throw 'Live match tests failed' }
    & .\build\test_build10_personality.exe
    if ($LASTEXITCODE) { throw 'Personality tests failed' }
    & .\build\test_build3_features.exe
    if ($LASTEXITCODE) { throw 'Build 3 features test failed' }

  }
} finally { Pop-Location }

