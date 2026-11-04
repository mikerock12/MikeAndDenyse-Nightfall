param(
    [string]$Version = "1.1.1",
    [ValidateSet('arm64-v8a','x86_64')][string]$Abi = 'arm64-v8a',
    [string]$RaylibSource = $env:RAYLIB_SOURCE_DIR,
    [switch]$UnsignedOnly,
    [switch]$QaDebug
)
$ErrorActionPreference = 'Stop'
$NativeRoot = Split-Path -Parent $PSScriptRoot
$RepoRoot = Split-Path -Parent $NativeRoot
$SdkRoot = if ($env:ANDROID_SDK_ROOT) { $env:ANDROID_SDK_ROOT } elseif ($env:ANDROID_HOME) { $env:ANDROID_HOME } else { Join-Path $env:LOCALAPPDATA 'Android/Sdk' }
$NdkRoot = if ($env:ANDROID_NDK_ROOT) { $env:ANDROID_NDK_ROOT } else { Join-Path $SdkRoot 'ndk/27.0.12077973' }
$BuildTools = Join-Path $SdkRoot 'build-tools/36.0.0'
$PlatformJar = Join-Path $SdkRoot 'platforms/android-36/android.jar'
$JdkBin = if ($env:JAVA_HOME) { Join-Path $env:JAVA_HOME 'bin' } else { 'C:/Program Files/Microsoft/jdk-17.0.21.21-hotspot/bin' }
$Cmake = Join-Path $SdkRoot 'cmake/3.22.1/bin/cmake.exe'
$Ninja = Join-Path $SdkRoot 'cmake/3.22.1/bin/ninja.exe'
$BuildDir = Join-Path $NativeRoot "build/android-$Abi"
$PackDir = Join-Path $NativeRoot "Saved/AndroidNative-$Abi"
$OutDir = Join-Path $NativeRoot "Saved/Packages/Release-$Version"
if (-not $RaylibSource) {
    $Dependencies = Join-Path $NativeRoot 'build/dependencies'
    $RaylibSource = Join-Path $Dependencies 'raylib-5.5'
    if (-not (Test-Path (Join-Path $RaylibSource 'src/rcore.c'))) {
        New-Item -ItemType Directory -Force -Path $Dependencies | Out-Null
        $Archive = Join-Path $Dependencies 'raylib-5.5.zip'
        Invoke-WebRequest -Uri 'https://github.com/raysan5/raylib/archive/refs/tags/5.5.zip' -OutFile $Archive
        if ((Get-FileHash -LiteralPath $Archive -Algorithm SHA256).Hash -ne '00E8A7C85F7A5221E56EE8E4E1C6642B773EAE73D29379A91C8AC5149E803F42') { throw 'raylib source SHA-256 mismatch' }
        Expand-Archive -LiteralPath $Archive -DestinationPath $Dependencies -Force
    }
}
if (-not (Test-Path (Join-Path $RaylibSource 'src/rcore.c'))) { throw 'Set RAYLIB_SOURCE_DIR to official raylib 5.5 source' }
function Check([string]$step) { if ($LASTEXITCODE -ne 0) { throw "Failed: $step ($LASTEXITCODE)" } }
New-Item -ItemType Directory -Force -Path $BuildDir,$PackDir,$OutDir | Out-Null
& $Cmake -S $NativeRoot -B $BuildDir -G Ninja "-DCMAKE_MAKE_PROGRAM=$Ninja" "-DCMAKE_TOOLCHAIN_FILE=$NdkRoot/build/cmake/android.toolchain.cmake" "-DANDROID_ABI=$Abi" '-DANDROID_PLATFORM=android-26' '-DANDROID_STL=c++_static' '-DCMAKE_BUILD_TYPE=Release' "-DRAYLIB_SOURCE_DIR=$RaylibSource"
Check 'CMake configure'
& $Cmake --build $BuildDir --target nightfall -j 4
Check 'C++ frontend build'

# APK assets are opened by raylib's AAssetManager callbacks. The index avoids filesystem enumeration of ZIP entries.
$ArtSrc = Join-Path $RepoRoot 'NightfallUnity/Assets/StreamingAssets/art'
$Assets = Join-Path $PackDir 'assets'
$ArtDst = Join-Path $Assets 'art'
New-Item -ItemType Directory -Force -Path $ArtDst | Out-Null
$index = @()
Get-ChildItem -LiteralPath $ArtSrc -Recurse -Filter '*.png' | Sort-Object FullName | ForEach-Object {
    $rel = $_.FullName.Substring($ArtSrc.Length).TrimStart('\','/').Replace('\','/')
    $dest = Join-Path $ArtDst $rel
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $dest) | Out-Null
    Copy-Item -LiteralPath $_.FullName -Destination $dest -Force
    $index += $rel
}
[IO.File]::WriteAllLines((Join-Path $ArtDst 'index.txt'), $index, [Text.UTF8Encoding]::new($false))
$Platform = Join-Path $NativeRoot 'Platform/Android'
$manifest = [xml](Get-Content -LiteralPath (Join-Path $Platform 'AndroidManifest.xml') -Raw)
$null = $manifest.manifest.SetAttribute('versionName','http://schemas.android.com/apk/res/android',$Version)
if ($QaDebug) {
    $null = $manifest.manifest.SetAttribute('package','com.mikeanddenyse.nightfall.qa')
    $null = $manifest.manifest.application.SetAttribute('label','http://schemas.android.com/apk/res/android','Nightfall QA')
    $null = $manifest.manifest.application.SetAttribute('debuggable','http://schemas.android.com/apk/res/android','true')
}
$manifest.Save((Join-Path $PackDir 'AndroidManifest.xml'))
& (Join-Path $BuildTools 'aapt2.exe') compile --dir (Join-Path $Platform 'res') -o (Join-Path $PackDir 'res.zip')
Check 'aapt2 compile'
$Gen = Join-Path $PackDir 'gen'
New-Item -ItemType Directory -Force -Path $Gen | Out-Null
& (Join-Path $BuildTools 'aapt2.exe') link -I $PlatformJar --manifest (Join-Path $PackDir 'AndroidManifest.xml') --java $Gen -A $Assets -o (Join-Path $PackDir 'base.apk') (Join-Path $PackDir 'res.zip')
Check 'aapt2 link'
$Classes = Join-Path $PackDir 'classes'
$Stage = Join-Path $PackDir 'stage'
New-Item -ItemType Directory -Force -Path $Classes,$Stage | Out-Null
$JavaFiles = @((Get-ChildItem -LiteralPath $Gen -Recurse -Filter '*.java').FullName) + @((Join-Path $Platform 'src/com/mikeanddenyse/nightfall/MainActivity.java'))
& (Join-Path $JdkBin 'javac.exe') -source 11 -target 11 -encoding UTF-8 -classpath $PlatformJar -d $Classes $JavaFiles
Check 'javac NativeActivity'
$ClassFiles = (Get-ChildItem -LiteralPath $Classes -Recurse -Filter '*.class').FullName
& (Join-Path $BuildTools 'd8.bat') --lib $PlatformJar --min-api 26 --output $Stage $ClassFiles
Check 'd8'
$LibDir = Join-Path $Stage "lib/$Abi"
New-Item -ItemType Directory -Force -Path $LibDir | Out-Null
Copy-Item -LiteralPath (Join-Path $BuildDir 'libnightfall.so') -Destination $LibDir -Force
$Unsigned = Join-Path $PackDir 'unsigned.apk'
Copy-Item -LiteralPath (Join-Path $PackDir 'base.apk') -Destination $Unsigned -Force
Push-Location $Stage
try { & (Join-Path $JdkBin 'jar.exe') uf $Unsigned 'classes.dex' "lib/$Abi/libnightfall.so"; Check 'jar' } finally { Pop-Location }
# Android treats backslashes from Windows aapt2 ZIP names literally.
# Repair local and central names, validate artwork, then align and sign.
& python (Join-Path $PSScriptRoot 'normalize_apk_zip.py') $Unsigned
Check 'portable APK ZIP entries and 77 original artwork assets'
& (Join-Path $BuildTools 'zipalign.exe') -f -P 16 4 $Unsigned (Join-Path $PackDir 'aligned.apk')
Check '16KB APK alignment'
if ($UnsignedOnly) {
    $UnsignedOutput = Join-Path $OutDir "MikeAndDenyse-$Version-Android-$Abi-UNSIGNED.apk"
    Copy-Item -LiteralPath (Join-Path $PackDir 'aligned.apk') -Destination $UnsignedOutput -Force
    Write-Output "Unsigned candidate (not installable): $UnsignedOutput"
    Get-FileHash -LiteralPath $UnsignedOutput -Algorithm SHA256
    return
}
if ($QaDebug) {
    # A separate application ID and disposable test certificate never replace the published identity.
    $QaKey = Join-Path $NativeRoot 'Saved/qa-signing/debug.keystore'
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $QaKey) | Out-Null
    if (-not (Test-Path -LiteralPath $QaKey)) {
        & (Join-Path $JdkBin 'keytool.exe') -genkeypair -keystore $QaKey -storepass android -keypass android -alias androiddebugkey -keyalg RSA -keysize 2048 -validity 3650 -dname 'CN=Android Debug,O=Android,C=US'
        Check 'create isolated disposable QA certificate'
    }
    $QaOutput = Join-Path $OutDir "MikeAndDenyse-$Version-Android-$Abi-QA.apk"
    & (Join-Path $BuildTools 'apksigner.bat') sign --ks $QaKey --ks-key-alias androiddebugkey --ks-pass pass:android --key-pass pass:android --out $QaOutput (Join-Path $PackDir 'aligned.apk')
    Check 'sign isolated QA app'
    & (Join-Path $BuildTools 'apksigner.bat') verify --verbose $QaOutput
    Check 'verify QA APK'
    Get-FileHash -LiteralPath $QaOutput -Algorithm SHA256
    Write-Output "QA only, separate package: $QaOutput"
    return
}
$Keystore = if ($env:SLT_GAMES_KEYSTORE) { $env:SLT_GAMES_KEYSTORE } else { Join-Path $env:TEMP 'slt_games_local_signing.keystore' }
if (-not (Test-Path -LiteralPath $Keystore) -or -not $env:SLT_GAMES_STORE_PASSWORD) { throw 'Existing release keystore and SLT_GAMES_STORE_PASSWORD required; no replacement key is generated.' }
$Alias = if ($env:SLT_GAMES_KEY_ALIAS) { $env:SLT_GAMES_KEY_ALIAS } else { 'sltgames' }
$Suffix = if ($Abi -eq 'arm64-v8a') { 'arm64' } else { 'x86_64-QA' }
$FinalApk = Join-Path $OutDir "MikeAndDenyse-$Version-Android-$Suffix.apk"
& (Join-Path $BuildTools 'apksigner.bat') sign --ks $Keystore --ks-key-alias $Alias --ks-pass env:SLT_GAMES_STORE_PASSWORD --key-pass env:SLT_GAMES_STORE_PASSWORD --out $FinalApk (Join-Path $PackDir 'aligned.apk')
Check 'sign with existing identity'
& (Join-Path $BuildTools 'apksigner.bat') verify --verbose --print-certs $FinalApk
Check 'verify signed APK'
& (Join-Path $BuildTools 'zipalign.exe') -c -P 16 4 $FinalApk
Check 'verify alignment'
Get-FileHash -LiteralPath $FinalApk -Algorithm SHA256
Write-Output "Candidate APK: $FinalApk ($((Get-Item -LiteralPath $FinalApk).Length) bytes)"
