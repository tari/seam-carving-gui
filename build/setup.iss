#define MyAppName "Seam Carving GUI"
#define MyAppNameAbbrev "SeamCarvingGui"
#define MyAppVer "1.11"
#define MyAppVerName "Seam Carving GUI Win32 1.11"
#define MyAppPublisher "By Gabe Rudy and Brain_Recall"
#define MyAppURL "http://code.google.com/p/seam-carving-gui/"
#define MyAppExeName "SeamCarvingGui.exe"
#define MyLicFile "E:/Build/SeamCarvingGui/StageWin32/GPL.txt"
#define MyPlatform "Win32"
#define MyOutputDir "E:/Build/SeamCarvingGui/InstallerWin32"
#define MyStageDir "E:/Build/SeamCarvingGui/StageWin32/"
#define MyRedistPath "E:/Build/SeamCarvingGui/SourceWin32/build/vcredist_x86.exe"
#define MySetupIconPath "E:/Build/SeamCarvingGui/SourceWin32/build/install.ico"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{BD3C1871-753C-493D-A1D2-2F1A66666260}
AppName={#MyAppName}
AppVerName={#MyAppVerName}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={pf}\{#MyAppName}
DefaultGroupName={#MyAppName}\
LicenseFile={#MyLicFile}
OutputDir={#MyOutputDir}
OutputBaseFilename={#MyAppNameAbbrev}-{#MyPlatform}-{#MyAppVer}
SetupIconFile={#MySetupIconPath}
Compression=lzma
SolidCompression=true
UninstallDisplayIcon={app}\{#MyAppExeName}
UninstallDisplayName={#MyAppVerName}

VersionInfoDescription={#MyAppName}-{#MyPlatform}-{#MyAppVer}
VersionInfoTextVersion={#MyAppName}-{#MyPlatform}-{#MyAppVer}
VersionInfoCompany={#MyAppPublisher}
RestartIfNeededByRun=false

[Languages]
Name: english; MessagesFile: compiler:Default.isl

[Files]
Source: {#MyStageDir}\*; DestDir: {app}; Flags: ignoreversion recursesubdirs createallsubdirs
Source: {#MyRedistPath}; DestDir: {tmp}; DestName: vcredist.exe; Flags: deleteafterinstall
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: {group}\{#MyAppName}; Filename: {app}\{#MyAppExeName}; WorkingDir: {app}
Name: {group}\{cm:ProgramOnTheWeb,{#MyAppName}}; Filename: {#MyAppURL}
Name: {group}\{cm:UninstallProgram,{#MyAppName}}; Filename: {uninstallexe}

[Run]
Filename: {app}\{#MyAppExeName}; Description: {cm:LaunchProgram,{#MyAppName}}; Flags: nowait postinstall skipifsilent
Filename: {tmp}\vcredist.exe; StatusMsg: Installing Visual C++ 2008 Runtime...; Parameters: /q

[UninstallDelete]
Name: {app}\*; Type: filesandordirs; Tasks: ; Languages: 
