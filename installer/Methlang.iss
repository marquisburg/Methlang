[Setup]
AppId={{D9F4D8EF-0B2C-4B37-A9B0-5A6E30C3D2A1}
AppName=Methlang
AppVersion=1.0.0
AppPublisher=GhostDragonWasTaken
AppPublisherURL=https://github.com/GhostDragonWasTaken/methlang
AppSupportURL=https://github.com/GhostDragonWasTaken/methlang/issues
AppUpdatesURL=https://github.com/GhostDragonWasTaken/methlang/releases
DefaultDirName={autopf}\Methlang
DefaultGroupName=Methlang
UninstallDisplayIcon={app}\methicon.ico
ChangesEnvironment=yes
ChangesAssociations=yes
WizardStyle=modern
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
OutputDir=out
OutputBaseFilename=Methlang-Setup
SetupIconFile=..\methicon.ico
LicenseFile=..\LICENSE

[Tasks]
Name: "addtopath"; Description: "Add Methlang bin to system PATH (Recommended)"; GroupDescription: "Additional tasks:"; Flags: checkedonce
Name: "associate"; Description: "Associate .meth files with Methlang"; GroupDescription: "Additional tasks:"

[Files]
Source: "..\bin\methlang.exe"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "meth-build.bat"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\methicon.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\stdlib\*"; DestDir: "{app}\stdlib"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\src\runtime\*"; DestDir: "{app}\runtime"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\docs\*"; DestDir: "{app}\docs"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Methlang Documentation"; Filename: "{app}\README.md"
Name: "{group}\Uninstall Methlang"; Filename: "{uninstallexe}"

[Registry]
Root: HKCR; Subkey: ".meth"; ValueType: string; ValueName: ""; ValueData: "MethlangFile"; Tasks: associate; Flags: uninsdeletevalue
Root: HKCR; Subkey: "MethlangFile"; ValueType: string; ValueName: ""; ValueData: "Methlang Source File"; Tasks: associate; Flags: uninsdeletekey
Root: HKCR; Subkey: "MethlangFile\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\methicon.ico"; Tasks: associate
Root: HKCR; Subkey: "MethlangFile\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\methlang.exe"" ""%1"""; Tasks: associate

[Run]
Filename: "{cmd}"; Parameters: "/K echo Methlang installed. Use meth-build your_file.meth or methlang --help"; Description: "Open terminal with usage hint"; Flags: postinstall nowait skipifsilent

[Code]
const
  UserEnvKey = 'Environment';
  MachineEnvKey = 'SYSTEM\CurrentControlSet\Control\Session Manager\Environment';

function ContainsPathEntry(const PathList, Entry: string): Boolean;
begin
  Result := Pos(';' + Uppercase(Entry) + ';', ';' + Uppercase(PathList) + ';') > 0;
end;

function AddPathEntry(const PathList, Entry: string): string;
begin
  if PathList = '' then
    Result := Entry
  else if PathList[Length(PathList)] = ';' then
    Result := PathList + Entry
  else
    Result := PathList + ';' + Entry;
end;

procedure AddToPath(const RootKey: Integer; const KeyName, Entry: string);
var
  Paths: string;
begin
  if not RegQueryStringValue(RootKey, KeyName, 'Path', Paths) then
    Paths := '';

  if not ContainsPathEntry(Paths, Entry) then
  begin
    Paths := AddPathEntry(Paths, Entry);
    RegWriteStringValue(RootKey, KeyName, 'Path', Paths);
  end;
end;

procedure RemoveFromPath(const RootKey: Integer; const KeyName, Entry: string);
var
  Paths: string;
  NeedleUpper: string;
  PathsUpper: string;
  PosStart: Integer;
begin
  if not RegQueryStringValue(RootKey, KeyName, 'Path', Paths) then
    Exit;

  NeedleUpper := ';' + Uppercase(Entry) + ';';
  Paths := ';' + Paths + ';';
  PathsUpper := Uppercase(Paths);
  PosStart := Pos(NeedleUpper, PathsUpper);

  while PosStart > 0 do
  begin
    Delete(Paths, PosStart, Length(Entry) + 1);
    PathsUpper := Uppercase(Paths);
    PosStart := Pos(NeedleUpper, PathsUpper);
  end;

  while (Length(Paths) > 0) and (Paths[1] = ';') do
    Delete(Paths, 1, 1);
  while (Length(Paths) > 0) and (Paths[Length(Paths)] = ';') do
    Delete(Paths, Length(Paths), 1);

  RegWriteStringValue(RootKey, KeyName, 'Path', Paths);
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  BinDir: string;
begin
  if CurStep <> ssPostInstall then
    Exit;

  BinDir := ExpandConstant('{app}\bin');
  if WizardIsTaskSelected('addtopath') then
    AddToPath(HKEY_LOCAL_MACHINE, MachineEnvKey, BinDir);
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  BinDir: string;
begin
  if CurUninstallStep <> usPostUninstall then
    Exit;

  BinDir := ExpandConstant('{app}\bin');
  RemoveFromPath(HKEY_LOCAL_MACHINE, MachineEnvKey, BinDir);
  RemoveFromPath(HKEY_CURRENT_USER, UserEnvKey, BinDir);
end;
