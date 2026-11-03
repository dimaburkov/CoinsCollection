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

	const String cPhotosSection   = L"Photos";
	const String cDefaultPhotos   = L"Photos\\";

	const String cNumistaSection  = L"Numista";
	const int    cDefaultQuota    = 2000;
	const String cDateTimeFormat  = L"yyyy-mm-dd hh:nn:ss";

	TDateTime ParseDateTime(const String &AText)
	{
		// 'yyyy-mm-dd hh:nn:ss' независимо от настроек Windows; иначе 0.
		const String s = AText.Trim();
		TDateTime result = 0.0;
		if (s.Length() >= 19)
		{
			TDateTime d, t;
			if (TryEncodeDate(StrToIntDef(s.SubString(1, 4), 0), StrToIntDef(s.SubString(6, 2), 0), StrToIntDef(s.SubString(9, 2), 0), d)
				&& TryEncodeTime(StrToIntDef(s.SubString(12, 2), 0), StrToIntDef(s.SubString(15, 2), 0), StrToIntDef(s.SubString(18, 2), 0), 0, t))
				result = d + t;
		}
		return result;
	}
}
//---------------------------------------------------------------------------
TAppConfig &AppConfig()
{
	static TAppConfig config;
	return config;
}
//---------------------------------------------------------------------------
TAppConfig::TAppConfig()
	: Language(cDefaultLanguage),
	  NumistaTokenExpires(0.0),
	  NumistaUserId(0),
	  NumistaRequestsCount(0),
	  NumistaMonthlyQuota(cDefaultQuota),
	  NumistaAutoLinkDetails(true),
	  NumistaLoadTypePhotos(true),
	  NumistaLoadPrices(false),
	  NumistaPriceCurrency(L"UAH"),
	  NumistaPriceDays(90),
	  NumistaPricesWithDetails(false),
	  NumistaUploadGrade(true),
	  NumistaUploadPrice(true),
	  NumistaUploadDate(true),
	  NumistaUploadComment(true),
	  BackupLast(0.0),
	  BackupAsked(0.0)
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

	FPhotosPathInIni = ini->ReadString(cPhotosSection, cPathKey, L"").Trim();
	PhotosPath = IncludeTrailingPathDelimiter(FPhotosPathInIni.IsEmpty() ? ExeDir() + cDefaultPhotos
		: ResolveDbPath(FPhotosPathInIni));

	Language = ini->ReadString(cUiSection, cLanguageKey, L"").Trim().LowerCase();
	if (Language.IsEmpty())
		Language = cDefaultLanguage;

	NumistaApiKey        = ini->ReadString(cNumistaSection, L"ApiKey", L"").Trim();
	NumistaToken         = ini->ReadString(cNumistaSection, L"AccessToken", L"").Trim();
	NumistaTokenExpires  = ParseDateTime(ini->ReadString(cNumistaSection, L"TokenExpires", L""));
	NumistaUserId        = ini->ReadInteger(cNumistaSection, L"UserId", 0);
	NumistaRequestsMonth = ini->ReadString(cNumistaSection, L"RequestsMonth", L"").Trim();
	NumistaRequestsCount = ini->ReadInteger(cNumistaSection, L"RequestsCount", 0);
	NumistaMonthlyQuota  = ini->ReadInteger(cNumistaSection, L"MonthlyQuota", cDefaultQuota);
	NumistaAutoLinkDetails = ini->ReadBool(cNumistaSection, L"AutoLinkDetails", true);
	NumistaLoadTypePhotos  = ini->ReadBool(cNumistaSection, L"LoadTypePhotos", true);
	NumistaLoadPrices      = ini->ReadBool(cNumistaSection, L"LoadPrices", false);
	NumistaPriceCurrency   = ini->ReadString(cNumistaSection, L"PriceCurrency", L"UAH").Trim().UpperCase();
	if (NumistaPriceCurrency.Length() != 3)
		NumistaPriceCurrency = L"UAH";
	NumistaPriceDays       = ini->ReadInteger(cNumistaSection, L"PriceDays", 90);
	if (NumistaPriceDays < 1 || NumistaPriceDays > 3650)
		NumistaPriceDays = 90;
	NumistaPricesWithDetails = ini->ReadInteger(cNumistaSection, L"PriceMode", 0) == 1;
	NumistaUploadGrade     = ini->ReadBool(cNumistaSection, L"UploadGrade", true);
	NumistaUploadPrice     = ini->ReadBool(cNumistaSection, L"UploadPrice", true);
	NumistaUploadDate      = ini->ReadBool(cNumistaSection, L"UploadDate", true);
	NumistaUploadComment   = ini->ReadBool(cNumistaSection, L"UploadComment", true);

	BackupFolder = ini->ReadString(L"Backup", L"Folder", L"").Trim();
	BackupLast   = ParseDateTime(ini->ReadString(L"Backup", L"Last", L""));
	BackupAsked  = ParseDateTime(ini->ReadString(L"Backup", L"Asked", L""));
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

	if (!BackupFolder.IsEmpty() || (double)BackupLast != 0.0 || (double)BackupAsked != 0.0)
	{
		ini->WriteString(L"Backup", L"Folder", BackupFolder);
		ini->WriteString(L"Backup", L"Last",
			(double)BackupLast == 0.0 ? String() : FormatDateTime(cDateTimeFormat, BackupLast));
		ini->WriteString(L"Backup", L"Asked",
			(double)BackupAsked == 0.0 ? String() : FormatDateTime(cDateTimeFormat, BackupAsked));
	}

	// Папка фото в интерфейсе не меняется: заданная вручную пишется как была.
	if (!FPhotosPathInIni.IsEmpty())
		ini->WriteString(cPhotosSection, cPathKey, FPhotosPathInIni);

	ini->WriteString(cUiSection, cLanguageKey, Language);

	if (!NumistaApiKey.IsEmpty() || ini->SectionExists(cNumistaSection))
	{
		ini->WriteString(cNumistaSection, L"ApiKey", NumistaApiKey);
		ini->WriteString(cNumistaSection, L"AccessToken", NumistaToken);
		ini->WriteString(cNumistaSection, L"TokenExpires",
			(double)NumistaTokenExpires == 0.0 ? String() : FormatDateTime(cDateTimeFormat, NumistaTokenExpires));
		ini->WriteInteger(cNumistaSection, L"UserId", NumistaUserId);
		ini->WriteString(cNumistaSection, L"RequestsMonth", NumistaRequestsMonth);
		ini->WriteInteger(cNumistaSection, L"RequestsCount", NumistaRequestsCount);
		ini->WriteInteger(cNumistaSection, L"MonthlyQuota", NumistaMonthlyQuota);
		ini->WriteBool(cNumistaSection, L"AutoLinkDetails", NumistaAutoLinkDetails);
		ini->WriteBool(cNumistaSection, L"LoadTypePhotos", NumistaLoadTypePhotos);
		ini->WriteBool(cNumistaSection, L"LoadPrices", NumistaLoadPrices);
		ini->WriteString(cNumistaSection, L"PriceCurrency", NumistaPriceCurrency);
		ini->WriteInteger(cNumistaSection, L"PriceDays", NumistaPriceDays);
		ini->WriteInteger(cNumistaSection, L"PriceMode", NumistaPricesWithDetails ? 1 : 0);
		ini->WriteBool(cNumistaSection, L"UploadGrade", NumistaUploadGrade);
		ini->WriteBool(cNumistaSection, L"UploadPrice", NumistaUploadPrice);
		ini->WriteBool(cNumistaSection, L"UploadDate", NumistaUploadDate);
		ini->WriteBool(cNumistaSection, L"UploadComment", NumistaUploadComment);
	}
}
//---------------------------------------------------------------------------
