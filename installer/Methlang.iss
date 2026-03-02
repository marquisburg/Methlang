[Setup]
AppName=Methlang
AppVersion=1.0
AppPublisher=GhostDragonWasTaken
AppPublisherURL=https://github.com/GhostDragonWasTaken/methlang
ChangesEnvironment=yes
DefaultDirName={autopf}\Methlang
DefaultGroupName=Methlang
OutputDir=out
OutputBaseFilename=Methlang-Setup
SetupIconFile=..\methicon.ico
SolidCompression=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

[Files]
Source: "..\bin\methlang.exe"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\methicon.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "meth-build.bat"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\stdlib\*"; DestDir: "{app}\stdlib"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\src\runtime\*"; DestDir: "{app}\src\runtime"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Methlang Documentation"; Filename: "https://github.com/GhostDragonWasTaken/methlang"
Name: "{group}\Uninstall Methlang"; Filename: "{uninstallexe}"

[Registry]
Root: HKCR; Subkey: ".meth"; ValueType: string; ValueName: ""; ValueData: "MethlangFile"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "MethlangFile"; ValueType: string; ValueName: ""; ValueData: "Methlang Source File"; Flags: uninsdeletekey
Root: HKCR; Subkey: "MethlangFile\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\methicon.ico"

[Code]
const
  EnvironmentKey = 'Environment';

procedure AddToPath(const Path: string);
var
  Paths: string;
begin
  if RegQueryStringValue(HKEY_CURRENT_USER, EnvironmentKey, 'Path', Paths) then
  begin
    if Pos(';' + Uppercase(Path) + ';', ';' + Uppercase(Paths) + ';') = 0 then
    begin
      Paths := Paths + ';' + Path;
      RegWriteStringValue(HKEY_CURRENT_USER, EnvironmentKey, 'Path', Paths);
    end;
  end;
end;

procedure RemoveFromPath(const Path: string);
var
  Paths: string;
  P: Integer;
begin
  if RegQueryStringValue(HKEY_CURRENT_USER, EnvironmentKey, 'Path', Paths) then
  begin
    Paths := ';' + Paths + ';';
    P := Pos(';' + Uppercase(Path) + ';', Uppercase(Paths));
    if P > 0 then
    begin
      Delete(Paths, P, Length(Path) + 1);
      Paths := Copy(Paths, 2, Length(Paths) - 2);
      RegWriteStringValue(HKEY_CURRENT_USER, EnvironmentKey, 'Path', Paths);
    end;
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    AddToPath(ExpandConstant('{app}\bin'));
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
  begin
    RemoveFromPath(ExpandConstant('{app}\bin'));
  end;
end;
