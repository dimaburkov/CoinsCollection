; Coins Collection — инсталлятор (задача 23), Inno Setup 5.
; Сборка: Setup\build-setup.bat (Release без пакетов -> Setup\Build, затем ISCC -> Setup\Output).
;
; Установка для текущего пользователя, без прав администратора: программа хранит базу, фото
; и настройки рядом с exe, поэтому папка должна быть доступна для записи (не Program Files).
; Данные (Db, Photos, CoinsCollection.ini, кэш Numista) не устанавливаются и не удаляются.

#define AppName      "Coins Collection"
#define AppVersion   "0.2.0"
#define AppExe       "CoinsCollection.exe"
#define BuildDir     "Build"

[Setup]
AppId={{1789D0DF-1962-4E34-AA1E-375475572998}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher=progua@ukr.net
AppSupportURL=mailto:progua@ukr.net
DefaultDirName={localappdata}\Programs\CoinsCollection
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
OutputDir=Output
OutputBaseFilename=CoinsCollection-Setup-{#AppVersion}
Compression=lzma2
SolidCompression=yes
UninstallDisplayIcon={app}\{#AppExe}
UninstallDisplayName={#AppName}
VersionInfoVersion={#AppVersion}.0
VersionInfoProductName={#AppName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "ukrainian"; MessagesFile: "compiler:Languages\Ukrainian.isl"

[CustomMessages]
english.DesktopIcon=Create a &desktop shortcut
ukrainian.DesktopIcon=Створити ярлик на &робочому столі
english.LaunchApp=Start {#AppName}
ukrainian.LaunchApp=Запустити {#AppName}
english.WritableFolder=The program keeps your collection, photos and settings in its own folder, so the folder must be writable.%n%nPlease choose a folder outside Program Files (for example the suggested one).
ukrainian.WritableFolder=Програма зберігає колекцію, фото та налаштування у своїй папці, тому в неї має бути можна записувати.%n%nВиберіть папку поза Program Files (наприклад, запропоновану).
english.DataKept=The program is removed. Your collection, photos and settings are kept in:%n%n%1%n%nDelete this folder yourself if you do not need them.
ukrainian.DataKept=Програму видалено. Вашу колекцію, фото та налаштування збережено в:%n%n%1%n%nВидаліть цю папку самостійно, якщо вони не потрібні.

[Tasks]
Name: "desktopicon"; Description: "{cm:DesktopIcon}"; Flags: unchecked

[Files]
Source: "{#BuildDir}\_CoinsCollection.exe"; DestDir: "{app}"; DestName: "{#AppExe}"; Flags: ignoreversion
Source: "{#BuildDir}\Lang\*.ini"; DestDir: "{app}\Lang"; Flags: ignoreversion
Source: "{#BuildDir}\Help\*"; DestDir: "{app}\Help"; Flags: ignoreversion recursesubdirs

[InstallDelete]
; Русский язык убран из программы (задача 18) — старый файл при обновлении удаляется.
Type: files; Name: "{app}\Lang\ru.ini"

[INI]
; Язык программы — как язык мастера установки, только если он ещё не выбран.
Filename: "{app}\CoinsCollection.ini"; Section: "UI"; Key: "Language"; String: "uk"; Languages: ukrainian; Flags: createkeyifdoesntexist
Filename: "{app}\CoinsCollection.ini"; Section: "UI"; Key: "Language"; String: "en"; Languages: english; Flags: createkeyifdoesntexist

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExe}"
Name: "{group}\{cm:UninstallProgram,{#AppName}}"; Filename: "{uninstallexe}"
Name: "{userdesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExe}"; Description: "{cm:LaunchApp}"; Flags: nowait postinstall skipifsilent

[Code]
// Папка внутри Program Files недоступна для записи обычному пользователю.
function IsUnderProgramFiles(const Dir: String): Boolean;
var
  D: String;
begin
  D := Lowercase(AddBackslash(Dir));
  Result := (Pos(Lowercase(AddBackslash(ExpandConstant('{pf32}'))), D) = 1);
  if IsWin64 then
    Result := Result or (Pos(Lowercase(AddBackslash(ExpandConstant('{pf64}'))), D) = 1);
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
  if (CurPageID = wpSelectDir) and IsUnderProgramFiles(WizardDirValue) then
  begin
    MsgBox(CustomMessage('WritableFolder'), mbError, MB_OK);
    Result := False;
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if (CurUninstallStep = usPostUninstall) and not UninstallSilent and DirExists(ExpandConstant('{app}')) then
    MsgBox(FmtMessage(CustomMessage('DataKept'), [ExpandConstant('{app}')]), mbInformation, MB_OK);
end;
