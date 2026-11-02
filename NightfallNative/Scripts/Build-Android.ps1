param(
    [string]$Version = "1.0.0"
)

$ErrorActionPreference = "Stop"
$NativeRoot = Split-Path -Parent $PSScriptRoot
$RepoRoot = Split-Path -Parent $NativeRoot
$AndroidDir = Join-Path $NativeRoot "Platform\Android"
$BuildDir = Join-Path $NativeRoot "Saved\AndroidBuild"
$OutDir = Join-Path $NativeRoot "Saved\Packages\Release-$Version"

$SdkRoot = if ($env:ANDROID_SDK_ROOT) { $env:ANDROID_SDK_ROOT } else { "C:\Users\Maicon\AppData\Local\Android\Sdk" }
$NdkRoot = if ($env:ANDROID_NDK_ROOT) { $env:ANDROID_NDK_ROOT } else { Join-Path $SdkRoot "ndk\27.0.12077973" }
$BuildTools = Join-Path $SdkRoot "build-tools\36.0.0"
$PlatformJar = Join-Path $SdkRoot "platforms\android-36\android.jar"
$JdkBin = "C:\Program Files\Microsoft\jdk-17.0.21.21-hotspot\bin"

$ClangPP = Join-Path $NdkRoot "toolchains\llvm\prebuilt\windows-x86_64\bin\aarch64-linux-android26-clang++.cmd"
$Aapt2 = Join-Path $BuildTools "aapt2.exe"
$D8 = Join-Path $BuildTools "d8.bat"
$ZipAlign = Join-Path $BuildTools "zipalign.exe"
$ApkSigner = Join-Path $BuildTools "apksigner.bat"
$Javac = Join-Path $JdkBin "javac.exe"
$JarTool = Join-Path $JdkBin "jar.exe"
$KeyTool = Join-Path $JdkBin "keytool.exe"

if (Test-Path $BuildDir) { Remove-Item $BuildDir -Recurse -Force }
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

# 1. Compila libnightfall.so (arm64-v8a) com o nucleo C++20 e rede UDP LAN
$LibDir = Join-Path $BuildDir "apk_stage\lib\arm64-v8a"
New-Item -ItemType Directory -Force -Path $LibDir | Out-Null
$SoOut = Join-Path $LibDir "libnightfall.so"

Write-Output "Compilando libnightfall.so (arm64-v8a, C++20, NDK 27)..."
& $ClangPP -std=c++20 -O2 -fPIC -shared `
    -I (Join-Path $NativeRoot "include") `
    (Join-Path $NativeRoot "src\core\NightfallCore.cpp") `
    (Join-Path $NativeRoot "src\network\NightfallNet.cpp") `
    (Join-Path $AndroidDir "jni\AndroidBridge.cpp") `
    -llog -landroid -lGLESv3 -lEGL -lm `
    -o $SoOut
if ($LASTEXITCODE -ne 0) { throw "Falha ao compilar libnightfall.so" }

# 2. Prepara assets/art e compila recursos com aapt2
$AssetsArt = Join-Path $BuildDir "assets\art"
New-Item -ItemType Directory -Force -Path $AssetsArt | Out-Null
$ArtSrc = Join-Path $RepoRoot "NightfallUnity\Assets\StreamingAssets\art"
Get-ChildItem -Path $ArtSrc -Recurse -Filter "*.png" | ForEach-Object {
    $rel = $_.FullName.Substring($ArtSrc.Length).TrimStart('\','/')
    $targetPath = Join-Path $AssetsArt $rel
    $targetParent = Split-Path -Parent $targetPath
    if (-not (Test-Path $targetParent)) { New-Item -ItemType Directory -Force -Path $targetParent | Out-Null }
    Copy-Item $_.FullName $targetPath -Force
}

$CompiledResZip = Join-Path $BuildDir "compiled_res.zip"
& $Aapt2 compile --dir (Join-Path $AndroidDir "res") -o $CompiledResZip
if ($LASTEXITCODE -ne 0) { throw "Falha no aapt2 compile" }

$GenJavaDir = Join-Path $BuildDir "gen_java"
New-Item -ItemType Directory -Force -Path $GenJavaDir | Out-Null
$BaseApk = Join-Path $BuildDir "base.apk"

& $Aapt2 link `
    -I $PlatformJar `
    --manifest (Join-Path $AndroidDir "AndroidManifest.xml") `
    --java $GenJavaDir `
    -A (Join-Path $BuildDir "assets") `
    -o $BaseApk `
    $CompiledResZip
if ($LASTEXITCODE -ne 0) { throw "Falha no aapt2 link" }

# 3. Compila Java + d8
$ClassesDir = Join-Path $BuildDir "classes"
New-Item -ItemType Directory -Force -Path $ClassesDir | Out-Null
$JavaFiles = @(
    (Get-ChildItem $GenJavaDir -Recurse -Filter "*.java" | Select-Object -ExpandProperty FullName),
    (Join-Path $AndroidDir "src\com\mikeanddenyse\nightfall\MainActivity.java")
)
& $Javac -source 11 -target 11 -encoding UTF-8 -classpath $PlatformJar -d $ClassesDir $JavaFiles
if ($LASTEXITCODE -ne 0) { throw "Falha no javac" }

$ClassFiles = Get-ChildItem $ClassesDir -Recurse -Filter "*.class" | Select-Object -ExpandProperty FullName
$DexOutDir = Join-Path $BuildDir "apk_stage"
& $D8 --lib $PlatformJar --min-api 26 --output $DexOutDir $ClassFiles
if ($LASTEXITCODE -ne 0) { throw "Falha no d8" }

# 4. Empacota classes.dex + lib/arm64-v8a/libnightfall.so no APK
Copy-Item $BaseApk (Join-Path $BuildDir "unsigned.apk") -Force
Push-Location $DexOutDir
& $JarTool uf (Join-Path $BuildDir "unsigned.apk") "classes.dex" "lib/arm64-v8a/libnightfall.so"
Pop-Location
if ($LASTEXITCODE -ne 0) { throw "Falha ao adicionar classes.dex e libnightfall.so no APK" }

# 5. Alinha com zipalign e assina com apksigner
$AlignedApk = Join-Path $BuildDir "aligned.apk"
& $ZipAlign -f -p 4 (Join-Path $BuildDir "unsigned.apk") $AlignedApk
if ($LASTEXITCODE -ne 0) { throw "Falha no zipalign" }

$KeystorePath = Join-Path $env:TEMP "slt_games_local_signing.keystore"
if (-not (Test-Path $KeystorePath)) {
    & $KeyTool -genkeypair -v -keystore $KeystorePath -storepass "sltgames2026" -keypass "sltgames2026" `
        -alias "sltgames" -keyalg RSA -keysize 2048 -validity 10000 `
        -dname "CN=Smells Like Tech, OU=Games, O=SLT, L=Brasil, ST=SP, C=BR"
}

$FinalApk = Join-Path $OutDir "MikeAndDenyse-$Version-Android-arm64.apk"
& $ApkSigner sign --ks $KeystorePath --ks-pass pass:sltgames2026 --key-pass pass:sltgames2026 --out $FinalApk $AlignedApk
if ($LASTEXITCODE -ne 0) { throw "Falha no apksigner sign" }

& $ApkSigner verify --verbose $FinalApk
if ($LASTEXITCODE -ne 0) { throw "Falha no apksigner verify" }

Write-Output "APK Android ARM64 assinado e verificado: $FinalApk ($((Get-Item $FinalApk).Length) bytes)"
