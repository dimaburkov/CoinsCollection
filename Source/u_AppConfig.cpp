//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <memory>
#include <System.IniFiles.hpp>
#include <System.IOUtils.hpp>
#include "u_AppConfig.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------

namespace
{
	const String cDatabaseSection = L"Database";
	const String cPathKey         = L"Path";
	const String cDefaultDbName   = L"Db\\coins.db";	// как в PawnShop: папка Db рядом с exe

	const String cUiSection       = L"UI";
	const String cLanguageKey     = L"Language";
	const String cDefaultLanguage = L"en";
}
//---------------------------------------------------------------------------
TAppConfig &AppConfig()
{
	static TAppConfig config;
	return config;
}
//---------------------------------------------------------------------------
TAppConfig::TAppConfig()
	: Language(cDefaultLanguage)
{
}
//---------------------------------------------------------------------------
TAppConfig::~TAppConfig()
{
}
//---------------------------------------------------------------------------
String TAppConfig::ExeDir()
{
	return IncludeTrailingPathDelimiter(ExtractFilePath(ParamStr(0)));
}
//---------------------------------------------------------------------------
String TAppConfig::IniFileName()
{
	return ExeDir() + L"CoinsCollection.ini";
}
//---------------------------------------------------------------------------
String TAppConfig::DefaultDbPath()
{
	return ExeDir() + cDefaultDbName;
}
//---------------------------------------------------------------------------
// Относительный путь считается от каталога exe.
String TAppConfig::ResolveDbPath(const String &APath)
{
	return TPath::GetFullPath(TPath::Combine(ExeDir(), APath));
}
//---------------------------------------------------------------------------
void TAppConfig::Load()
{
	std::unique_ptr<TIniFile> ini(new TIniFile(IniFileName()));

	FDbPathInIni = ini->ReadString(cDatabaseSection, cPathKey, L"").Trim();
	DbPath = FDbPathInIni.IsEmpty() ? DefaultDbPath() : ResolveDbPath(FDbPathInIni);

	Language = ini->ReadString(cUiSection, cLanguageKey, L"").Trim().LowerCase();
	if (Language.IsEmpty())
		Language = cDefaultLanguage;
}
//---------------------------------------------------------------------------
void TAppConfig::Save()
{
	std::unique_ptr<TIniFile> ini(new TIniFile(IniFileName()));

	// Путь по умолчанию в ini не пишется; не изменившийся путь пишется в прежнем
	// виде (относительный остаётся относительным).
	if (SameFileName(DbPath, DefaultDbPath()))
		FDbPathInIni = L"";
	else if (FDbPathInIni.IsEmpty() || !SameFileName(ResolveDbPath(FDbPathInIni), DbPath))
		FDbPathInIni = DbPath;

	if (FDbPathInIni.IsEmpty())
		ini->DeleteKey(cDatabaseSection, cPathKey);
	else
		ini->WriteString(cDatabaseSection, cPathKey, FDbPathInIni);

	ini->WriteString(cUiSection, cLanguageKey, Language);
}
//---------------------------------------------------------------------------
