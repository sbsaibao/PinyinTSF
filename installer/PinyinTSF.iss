#define MyAppName "PinyinTSF"
#define MyAppVersion "0.1.0"
#define MyAppPublisher "PinyinTSF"
#ifndef BuildDir
#define BuildDir "..\bin\Release"
#endif

[Setup]
AppId={{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\PinyinTSF
DefaultGroupName=PinyinTSF
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64os
ArchitecturesInstallIn64BitMode=x64os
OutputDir=..\dist
OutputBaseFilename=PinyinTSF-Setup-x64
SetupIconFile=..\icon\profile_icon.ico
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\profile_icon.ico
UsedUserAreasWarning=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "{#BuildDir}\PinyinTSF.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\app_icon.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\tray_icon.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\profile_icon.ico"; DestDir: "{app}"; Flags: ignoreversion

[Run]
Filename: "{sys}\regsvr32.exe"; Parameters: "/s ""{app}\PinyinTSF.dll"""; StatusMsg: "Registering PinyinTSF input method..."; Flags: runhidden waituntilterminated

[UninstallRun]
Filename: "{sys}\regsvr32.exe"; Parameters: "/u /s ""{app}\PinyinTSF.dll"""; Flags: runhidden waituntilterminated; RunOnceId: "UnregisterPinyinTSF"

[Registry]
Root: HKCU; Subkey: "Software\PinyinTSF"; Flags: uninsdeletekey

[Code]
function InitializeSetup(): Boolean;
begin
  Result := IsWin64;
  if not Result then
    MsgBox('PinyinTSF only supports 64-bit Windows.', mbError, MB_OK);
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
  begin
    MsgBox(
      'PinyinTSF has been removed from Windows registration.' + #13#10 + #13#10 +
      'If the tray icon is still visible, an already-running application still has PinyinTSF.dll loaded. Close applications that used the input method, restart Windows Explorer, or sign out/restart Windows to unload the old DLL.',
      mbInformation,
      MB_OK);
  end;
end;
