param([string]$Version='1.0.1')
$ErrorActionPreference='Stop'
$NativeRoot=[IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$Packages=Join-Path $NativeRoot "Saved/Packages/Release-$Version"
$RunRoot=Join-Path $NativeRoot ("Saved/InstallerSmoke/"+$Version+'-'+[Guid]::NewGuid().ToString('N'))
$Target=[IO.Path]::GetFullPath((Join-Path $RunRoot 'Installed'))
$SavedPrefix=[IO.Path]::GetFullPath((Join-Path $NativeRoot 'Saved'))+[IO.Path]::DirectorySeparatorChar
if(-not $Target.StartsWith($SavedPrefix,[StringComparison]::OrdinalIgnoreCase)) { throw 'Test install must remain inside NightfallNative/Saved' }
$AppId='{D49123B1-73E2-4C84-9F1A-62A1B9120001}_is1'
$RegistryRoots=@('HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall','HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall','HKLM:\Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall')
foreach($RegistryRoot in $RegistryRoots) {
    if(Test-Path -LiteralPath (Join-Path $RegistryRoot $AppId)) { throw 'Existing Nightfall installation found; refusing to replace it for a test' }
}
New-Item -ItemType Directory -Force -Path $RunRoot | Out-Null
$Portable=Join-Path $RunRoot 'Portable'
Expand-Archive -LiteralPath (Join-Path $Packages "MikeAndDenyse-$Version-Windows-x64.zip") -DestinationPath $Portable
$Receipt=@{version=$Version;initialInstallationPresent=$false;runDirectory=$RunRoot;uninstalled=$false}
$RegistryKey=Join-Path $RegistryRoots[0] $AppId
try {
    $Installer=Join-Path $Packages "MikeAndDenyse-$Version-Windows-x64-Setup.exe"
    $InstallLog=Join-Path $RunRoot 'install.log'
    $Process=Start-Process -FilePath $Installer -ArgumentList @('/CURRENTUSER','/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/SP-','/NOICONS',('/DIR="'+$Target+'"'),('/LOG="'+$InstallLog+'"')) -PassThru -Wait -WindowStyle Hidden
    $Receipt['installerExitCode']=$Process.ExitCode
    if($Process.ExitCode -ne 0) { throw 'Installer did not finish successfully' }
    $CheckedFiles=0
    Get-ChildItem -LiteralPath $Portable -Recurse -File | ForEach-Object {
        $Relative=$_.FullName.Substring($Portable.Length).TrimStart('\','/')
        $Installed=Join-Path $Target $Relative
        if(-not (Test-Path -LiteralPath $Installed) -or (Get-FileHash -LiteralPath $Installed).Hash -ne (Get-FileHash -LiteralPath $_.FullName).Hash) { throw "Installed file differs from portable ZIP: $Relative" }
        $CheckedFiles++
    }
    $Receipt['matchingPortableFiles']=$CheckedFiles
    $InstalledExe=Join-Path $Target 'NightfallNative.exe'
    $Receipt['installedExeSHA256']=(Get-FileHash -LiteralPath $InstalledExe -Algorithm SHA256).Hash.ToLower()
    $Smoke=Start-Process -FilePath $InstalledExe -ArgumentList '--headless-smoke' -PassThru -Wait -WindowStyle Hidden
    $Receipt['headlessSmokePassed']=$Smoke.ExitCode -eq 0
    if(-not $Receipt['headlessSmokePassed']) { throw 'Installed executable headless smoke failed' }
    $Info=[Diagnostics.ProcessStartInfo]::new()
    $Info.FileName=$InstalledExe
    $Info.WorkingDirectory=$Target
    $Info.UseShellExecute=$false
    $Info.CreateNoWindow=$true
    $Info.WindowStyle=[Diagnostics.ProcessWindowStyle]::Hidden
    $Info.RedirectStandardOutput=$true
    $Info.RedirectStandardError=$true
    # Only this child receives isolated storage. The user's environment and saves are unchanged.
    $Info.Environment['LOCALAPPDATA']=Join-Path $RunRoot 'UserData'
    $Info.ArgumentList.Add('--render-smoke')
    $Info.ArgumentList.Add((Join-Path $RunRoot 'screenshots'))
    $Game=[Diagnostics.Process]::Start($Info)
    $StandardOutput=$Game.StandardOutput.ReadToEndAsync()
    $StandardError=$Game.StandardError.ReadToEndAsync()
    $Game.WaitForExit()
    $Log=$StandardOutput.GetAwaiter().GetResult()+$StandardError.GetAwaiter().GetResult()
    [IO.File]::WriteAllText((Join-Path $RunRoot 'render.log'),$Log)
    $Receipt['renderExitCode']=$Game.ExitCode
    $Receipt['render16WorldsPassed']=$Game.ExitCode -eq 0 -and $Log.Contains('NIGHTFALL_RENDER_SMOKE_OK worlds=16 assets=77')
    if(-not $Receipt['render16WorldsPassed']) { throw 'Installed executable render smoke failed' }
} finally {
    if(Test-Path -LiteralPath $RegistryKey) {
        $Registered=Get-ItemProperty -LiteralPath $RegistryKey
        if(-not [IO.Path]::GetFullPath($Registered.InstallLocation).TrimEnd('\').Equals($Target.TrimEnd('\'),[StringComparison]::OrdinalIgnoreCase)) { throw 'Uninstaller no longer points to the isolated test install' }
        $Uninstaller=[IO.Path]::GetFullPath((Join-Path $Target 'unins000.exe'))
        if(-not $Uninstaller.StartsWith($Target+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Unexpected uninstaller path' }
        $UninstallLog=Join-Path $RunRoot 'uninstall.log'
        $Removal=Start-Process -FilePath $Uninstaller -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',('/LOG="'+$UninstallLog+'"')) -PassThru -Wait -WindowStyle Hidden
        $Receipt['uninstallerExitCode']=$Removal.ExitCode
        $Receipt['uninstalled']=$Removal.ExitCode -eq 0 -and -not (Test-Path -LiteralPath (Join-Path $Target 'NightfallNative.exe')) -and -not (Test-Path -LiteralPath $RegistryKey)
    }
    $Receipt | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $RunRoot 'receipt.json')
}
if(-not $Receipt['uninstalled']) { throw 'Isolated test install was not fully removed' }
$Receipt | ConvertTo-Json -Depth 5
