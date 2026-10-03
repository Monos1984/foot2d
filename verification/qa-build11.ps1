$ErrorActionPreference='Stop'
$exe=Join-Path $PSScriptRoot 'FranceFoot2D_v00.03.00_build10_Source\FranceFoot2D.exe'
$cases=@(
 @{name='player-navigation';mode='personality-navigation'},@{name='player-postes';mode='role-player'},@{name='player-personality';mode='personality'},@{name='player-history';mode='personality-history'},@{name='player-scout';mode='personality-scout-high'},@{name='player-simple';mode='personality-simple'},
 @{name='result-summary';mode='pstats';post='0'},@{name='result-attack';mode='pstats';post='1'},@{name='result-discipline';mode='pstats';post='2'},@{name='pause-stats';mode='pstats'},@{name='tv-entry';mode='anthem';tv='1'},@{name='tv-match';mode='hl';tv='1'},@{name='match-career';mode='sporting-match'},@{name='match-simple';mode='supporters-match'}
)
try{foreach($case in $cases){
 $dir=Join-Path $PSScriptRoot ('qa\build11\'+$case.name);New-Item -ItemType Directory $dir -Force | Out-Null
 $env:FOOT_TEST=$case.mode;$env:FOOT_FRAMES='30';$env:FOOT_EVERY='30';if($case.ContainsKey('post')){$env:FOOT_POST=$case.post}else{Remove-Item Env:FOOT_POST -ErrorAction SilentlyContinue};if($case.tv){$env:FOOT_TV_HUD='1'}else{Remove-Item Env:FOOT_TV_HUD -ErrorAction SilentlyContinue}
 $p=Start-Process -FilePath $exe -WorkingDirectory $dir -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $dir 'stdout.txt') -RedirectStandardError (Join-Path $dir 'stderr.txt');if(-not $p.WaitForExit(60000)){$p.Kill();throw $case.name};if($p.ExitCode-ne 0 -or -not(Test-Path (Join-Path $dir 'shot_00030.png'))){throw $case.name};Write-Output ('PASS screen '+$case.name)
}}finally{Remove-Item Env:FOOT_TEST,Env:FOOT_FRAMES,Env:FOOT_EVERY,Env:FOOT_POST,Env:FOOT_TV_HUD -ErrorAction SilentlyContinue}
