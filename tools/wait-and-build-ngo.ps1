$ErrorActionPreference = 'Continue'
$log = 'D:\MikeAndDenyse\tools\ngo-build-orchestrator.log'
function Log($m) { $line = "[{0}] {1}" -f (Get-Date -Format 'HH:mm:ss'), $m; Add-Content $log $line; Write-Host $line }

$roots = @(
  'D:\UnityEditors\6000.5.8f1',
  'D:\Unity\6000.5.8f1',
  'C:\Program Files\Unity 6000.5.8f1'
)
function FindRoot {
  foreach ($r in $roots) {
    if ((Test-Path (Join-Path $r 'Editor\Unity.exe')) -and (Test-Path (Join-Path $r 'Editor\Data\Resources\PackageManager\Server\UnityPackageManager.exe'))) { return $r }
  }
  return $null
}
$editor = 'D:\UnityEditors\6000.5.8f1\Editor\Unity.exe'
$upm    = 'D:\UnityEditors\6000.5.8f1\Editor\Data\Resources\PackageManager\Server\UnityPackageManager.exe'
$androidPlayer = 'D:\UnityEditors\6000.5.8f1\Editor\Data\PlaybackEngines\AndroidPlayer'
$androidSetup = 'D:\Unity\Downloads\UnitySetup-Android-Support-for-Editor-6000.5.8f1.exe'
$project = 'D:\MikeAndDenyse\NightfallUnity'
$apk = 'D:\MikeAndDenyse\MikeAndDenyse-Nightfall-NGO.apk'
$hub = 'D:\Unity\Unity Hub\Unity Hub.exe'

Log 'Waiting for Unity 6 editor install...'
$deadline = (Get-Date).AddHours(2)
$goneTicks = 0
while ((Get-Date) -lt $deadline) {
  $setup = Get-Process -Name 'UnitySetup64-6000.5.8f1','UnitySetup64','UnitySetup' -ErrorAction SilentlyContinue
  $root = FindRoot
  $hasEditor = $null -ne $root
  if ($hasEditor) {
    $editor = Join-Path $root 'Editor\Unity.exe'
    $upm = Join-Path $root 'Editor\Data\Resources\PackageManager\Server\UnityPackageManager.exe'
    $androidPlayer = Join-Path $root 'Editor\Data\PlaybackEngines\AndroidPlayer'
  }
  if ($hasEditor -and -not $setup) {
    Log "Editor present at $root and installer exited"
    break
  }
  $destSize = 0
  foreach ($r in $roots) {
    if (Test-Path $r) {
      $destSize += [math]::Round(((Get-ChildItem $r -Recurse -File -ErrorAction SilentlyContinue | Measure-Object Length -Sum).Sum / 1MB), 0)
    }
  }
  $setupIds = ($setup | ForEach-Object { $_.Id }) -join ','
  Log ("waiting editor={0} setup=[{1}] destMB={2} root={3}" -f $hasEditor, $setupIds, $destSize, $root)
  if (-not $setup -and -not $hasEditor) {
    $goneTicks++
    if ($goneTicks -ge 3) { Log 'Installer gone and editor missing — fail'; exit 10 }
  } else { $goneTicks = 0 }
  Start-Sleep -Seconds 20
}

if (-not (Test-Path $editor)) { Log 'Unity.exe missing'; exit 11 }
if (-not (Test-Path $upm)) { Log 'UPM server missing — editor incomplete'; exit 12 }
Log "Using editor $editor"

$root = FindRoot
if ($root) {
  $editor = Join-Path $root 'Editor\Unity.exe'
  $upm = Join-Path $root 'Editor\Data\Resources\PackageManager\Server\UnityPackageManager.exe'
  $androidPlayer = Join-Path $root 'Editor\Data\PlaybackEngines\AndroidPlayer'
}

if (-not (Test-Path $androidPlayer)) {
  if (-not (Test-Path $androidSetup)) { Log 'Android support installer missing'; exit 13 }
  Log "Installing Android Build Support into $root ..."
  $psi = New-Object System.Diagnostics.ProcessStartInfo
  $psi.FileName = $androidSetup
  $psi.Arguments = "--unattended --install-location=`"$root`""
  $psi.UseShellExecute = $false
  $psi.Environment['TEMP'] = 'D:\Unity\Temp'
  $psi.Environment['TMP'] = 'D:\Unity\Temp'
  $p = New-Object System.Diagnostics.Process
  $p.StartInfo = $psi
  [void]$p.Start()
  $p.WaitForExit()
  Log ("Android unattended exit {0}" -f $p.ExitCode)
  if (-not (Test-Path $androidPlayer)) {
    Log 'Retry Android support with NSIS /S'
    $p2 = Start-Process -FilePath $androidSetup -ArgumentList '/S',"/D=$root" -PassThru -Wait
    Log ("Android /S exit {0}" -f $p2.ExitCode)
  }
}

if (-not (Test-Path $androidPlayer)) { Log 'AndroidPlayer still missing'; exit 14 }
Log 'AndroidPlayer present'

try {
  & $hub -- --headless editors --add 'D:\UnityEditors\6000.5.8f1' 2>&1 | Out-File 'D:\MikeAndDenyse\tools\hub-add-editor.log' -Encoding utf8
} catch { Log $_.Exception.Message }

if (Test-Path "$project\Temp\UnityLockfile") { Remove-Item "$project\Temp\UnityLockfile" -Force -ErrorAction SilentlyContinue }

Log 'Starting Unity batchmode Android build...'
$unityLog = 'D:\MikeAndDenyse\unity-build.log'
$args = @(
  '-batchmode','-nographics','-quit',
  '-projectPath', $project,
  '-executeMethod','Nightfall.Editor.AndroidBuilder.Build',
  '-buildTarget','Android',
  '-logFile', $unityLog
)
$u = Start-Process -FilePath $editor -ArgumentList $args -PassThru -Wait
Log ("Unity build exit {0}" -f $u.ExitCode)

if (Test-Path $apk) {
  $item = Get-Item $apk
  Log ("APK OK {0} bytes" -f $item.Length)
  exit 0
} else {
  Log 'APK missing'
  exit 15
}
