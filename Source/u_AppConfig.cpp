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
}
//---------------------------------------------------------------------------
TAppConfig::TAppConfig()
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
void TAppConfig::Load()
{
	std::unique_ptr<TIniFile> ini(new TIniFile(IniFileName()));
	DbPath = ini->ReadString(cDatabaseSection, cPathKey, L"").Trim();

	if (DbPath.IsEmpty())
		DbPath = ExeDir() + cDefaultDbName;
	else
		DbPath = TPath::GetFullPath(TPath::Combine(ExeDir(), DbPath));	// относительный путь — от каталога exe
}
//---------------------------------------------------------------------------
void TAppConfig::Save()
{
	std::unique_ptr<TIniFile> ini(new TIniFile(IniFileName()));
	ini->WriteString(cDatabaseSection, cPathKey, DbPath);
}
//---------------------------------------------------------------------------
