param(
    [string]$Version = "1.0.0"
)

$ErrorActionPreference = "Stop"
$NativeRoot = Split-Path -Parent $PSScriptRoot
$RepoRoot = Split-Path -Parent $NativeRoot

Set-Location $NativeRoot
cmake --preset windows-msvc-release
if ($LASTEXITCODE -ne 0) { throw "Falha no cmake configure" }
cmake --build --preset windows-msvc-release
if ($LASTEXITCODE -ne 0) { throw "Falha no cmake build" }
ctest --preset windows-msvc-release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "Falha no ctest" }

$StageDir = Join-Path $NativeRoot "Saved\Packages\WindowsStage"
$OutDir = Join-Path $NativeRoot "Saved\Packages\Release-$Version"
if (Test-Path $StageDir) { Remove-Item $StageDir -Recurse -Force }
New-Item -ItemType Directory -Force -Path $StageDir | Out-Null
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$ReleaseBin = Join-Path $NativeRoot "build\windows-msvc-release\Release"
Copy-Item (Join-Path $ReleaseBin "NightfallNative.exe") (Join-Path $StageDir "NightfallNative.exe") -Force
Copy-Item (Join-Path $ReleaseBin "raylib.dll") (Join-Path $StageDir "raylib.dll") -Force

$ArtSrc = Join-Path $RepoRoot "NightfallUnity\Assets\StreamingAssets\art"
$ArtDst = Join-Path $StageDir "Content\art"
New-Item -ItemType Directory -Force -Path $ArtDst | Out-Null
Get-ChildItem -Path $ArtSrc -Recurse -Filter "*.png" | ForEach-Object {
    $rel = $_.FullName.Substring($ArtSrc.Length).TrimStart('\','/')
    $targetPath = Join-Path $ArtDst $rel
    $targetParent = Split-Path -Parent $targetPath
    if (-not (Test-Path $targetParent)) { New-Item -ItemType Directory -Force -Path $targetParent | Out-Null }
    Copy-Item $_.FullName $targetPath -Force
}

$ZipPath = Join-Path $OutDir "MikeAndDenyse-$Version-Windows-x64.zip"
if (Test-Path $ZipPath) { Remove-Item $ZipPath -Force }
Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::CreateFromDirectory($StageDir, $ZipPath)

$Iscc = "C:\Users\Maicon\AppData\Local\Programs\Inno Setup 6\ISCC.exe"
if (-not (Test-Path $Iscc)) { throw "ISCC.exe nao encontrado em $Iscc" }
& $Iscc "/DAppVersion=$Version" (Join-Path $NativeRoot "Platform\Windows\Nightfall.iss")
if ($LASTEXITCODE -ne 0) { throw "Falha no Inno Setup" }

$ExePath = Join-Path $OutDir "MikeAndDenyse-$Version-Windows-x64-Setup.exe"
Write-Output "Windows packages gerados:"
Write-Output "  ZIP: $ZipPath ($((Get-Item $ZipPath).Length) bytes)"
Write-Output "  EXE: $ExePath ($((Get-Item $ExePath).Length) bytes)"
