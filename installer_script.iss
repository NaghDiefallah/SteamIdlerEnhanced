; Define basic app info
#define MyAppName "Steam Idler Enhanced"
#define MyAppVersion "1.1"
#define MyAppPublisher "NewDawn Corp"
#define MyAppExeName "SteamIdlerEnhanced.exe"
#define DistFolder "..\dist"

[Setup]
AppId={C6E2A34B-4D1E-4F2A-8B6C-7D9E0F1A2B3C}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
; Customize the installer look
OutputDir=..\installer_build
OutputBaseFilename=SteamIdlerEnhanced_Setup
SetupIconFile=..\resources\app_icon.ico
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; The main executable
Source: "{#DistFolder}\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
; The background worker (CRITICAL)
Source: "{#DistFolder}\ghost_launcher.exe"; DestDir: "{app}"; Flags: ignoreversion
; All Qt DLLs and dependencies from windeployqt
Source: "{#DistFolder}\*.dll"; DestDir: "{app}"; Flags: ignoreversion
; Subdirectories (plugins, data, themes)
Source: "{#DistFolder}\data\*"; DestDir: "{app}\data"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#DistFolder}\themes\*"; DestDir: "{app}\themes"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#DistFolder}\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#DistFolder}\styles\*"; DestDir: "{app}\styles"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent