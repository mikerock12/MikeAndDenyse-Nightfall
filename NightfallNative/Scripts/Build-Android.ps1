# Compatibility entrypoint: build the complete shared C++ frontend, never the old simulation-only JNI bridge.
param(
    [string]$Version = "1.1.1",
    [ValidateSet('arm64-v8a','x86_64')][string]$Abi = 'arm64-v8a',
    [string]$RaylibSource = $env:RAYLIB_SOURCE_DIR,
    [switch]$UnsignedOnly,
    [switch]$QaDebug
)
& (Join-Path $PSScriptRoot 'Build-NativeAndroid.ps1') @PSBoundParameters
