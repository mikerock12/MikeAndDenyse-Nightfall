#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif

[Setup]
AppId={{D49123B1-73E2-4C84-9F1A-62A1B9120001}
AppName=Mike & Denyse: Nightfall
AppVersion={#AppVersion}
AppPublisher=Smells Like Tech
AppPublisherURL=https://softwares.smellsliketech.com.br
DefaultDirName={autopf}\SmellsLikeTech\MikeAndDenyseNightfall
DefaultGroupName=Smells Like Tech\Mike & Denyse Nightfall
DisableProgramGroupPage=yes
OutputDir=..\..\Saved\Packages\Release-{#AppVersion}
OutputBaseFilename=MikeAndDenyse-{#AppVersion}-Windows-x64-Setup
Compression=lzma2/fast
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequiredOverridesAllowed=dialog

[Languages]
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "..\..\Saved\Packages\WindowsStage\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Mike & Denyse - Nightfall"; Filename: "{app}\NightfallNative.exe"
Name: "{autodesktop}\Mike & Denyse - Nightfall"; Filename: "{app}\NightfallNative.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\NightfallNative.exe"; Description: "{cm:LaunchProgram,Mike & Denyse: Nightfall}"; Flags: nowait postinstall skipifsilent
