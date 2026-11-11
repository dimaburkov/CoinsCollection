//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <algorithm>
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
	  GridHidden(L"obverse,reverse"),
	  GridRowHeight(18),
	  WindowMaximized(false),
	  WindowTreeWidth(0),
	  WindowDetailsHeight(0),
	  BackupLast(0.0),
	  BackupAsked(0.0)
{
}
//---------------------------------------------------------------------------
TAppConfig::~TAppConfig()
{
}
//---------------------------------------------------------------------------
void TAppConfig::RestoreFormSize(TForm *AForm, const String &AName)
{
	std::map<String, TPoint>::const_iterator it = FormSizes.find(AName);
	if (it == FormSizes.end())
		return;
	AForm->Width  = std::min(std::max((int)it->second.x, (int)AForm->Constraints->MinWidth), Screen->WorkAreaWidth);
	AForm->Height = std::min(std::max((int)it->second.y, (int)AForm->Constraints->MinHeight), Screen->WorkAreaHeight);
}
//---------------------------------------------------------------------------
void TAppConfig::StoreFormSize(TForm *AForm, const String &AName)
{
	const TRect r = NormalBounds(AForm);
	FormSizes[AName] = TPoint(r.right - r.left, r.bottom - r.top);
	try { Save(); } catch (Exception &) {}
}
//---------------------------------------------------------------------------
// GetWindowPlacement: обычные границы и у развёрнутого / свёрнутого окна — в координатах
// рабочей области монитора (без панели задач), переводятся в экранные.
TRect TAppConfig::NormalBounds(TForm *AForm, bool *AMaximized)
{
	TRect result = AForm->BoundsRect;
	bool maximized = AForm->WindowState == wsMaximized;
	WINDOWPLACEMENT placement;
	placement.length = sizeof(placement);
	if (AForm->HandleAllocated() && GetWindowPlacement(AForm->Handle, &placement))
	{
		const RECT &r = placement.rcNormalPosition;
		result = TRect(r.left, r.top, r.right, r.bottom);
		maximized = placement.showCmd == SW_SHOWMAXIMIZED
			|| (placement.showCmd == SW_SHOWMINIMIZED && (placement.flags & WPF_RESTORETOMAXIMIZED) != 0);
		MONITORINFO info;
		info.cbSize = sizeof(info);
		if (GetMonitorInfo(MonitorFromRect(&r, MONITOR_DEFAULTTONEAREST), &info))
			OffsetRect(&result, info.rcWork.left - info.rcMonitor.left, info.rcWork.top - info.rcMonitor.top);
	}
	if (AMaximized)
		*AMaximized = maximized;
	return result;
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

	GridHidden    = ini->ReadString(L"Grid", L"Hidden", L"obverse,reverse").Trim().LowerCase();
	GridRowHeight = ini->ReadInteger(L"Grid", L"RowHeight", 18);
	if (GridRowHeight != 18 && GridRowHeight != 40 && GridRowHeight != 64)
		GridRowHeight = 18;

	GridOrder = ini->ReadString(L"Grid", L"Order", L"").Trim().LowerCase();
	GridAlign = ini->ReadString(L"Grid", L"Align", L"").Trim().LowerCase();

	// Размеры (задача 44) — к текущему масштабу экрана.
	const int ppi = ini->ReadInteger(L"Window", L"PixelsPerInch", Screen->PixelsPerInch);
	auto scaled = [ppi](int AValue) { return ppi > 0 ? MulDiv(AValue, Screen->PixelsPerInch, ppi) : AValue; };
	GridWidths.clear();
	std::unique_ptr<TStringList> list(new TStringList());
	list->CommaText = ini->ReadString(L"Grid", L"Widths", L"");
	for (int i = 0; i < list->Count; ++i)
	{
		const String item = list->Strings[i];
		const int colon = item.Pos(L":");
		const int width = colon > 1 ? StrToIntDef(item.SubString(colon + 1, 10), 0) : 0;
		if (width > 0)
			GridWidths[item.SubString(1, colon - 1).Trim().LowerCase()] = scaled(width);
	}
	WindowBounds = TRect(0, 0, 0, 0);
	const int width = ini->ReadInteger(L"Window", L"Width", 0), height = ini->ReadInteger(L"Window", L"Height", 0);
	if (width > 0 && height > 0)
	{
		const int left = scaled(ini->ReadInteger(L"Window", L"Left", 0)), top = scaled(ini->ReadInteger(L"Window", L"Top", 0));
		WindowBounds = TRect(left, top, left + scaled(width), top + scaled(height));
	}
	WindowMaximized     = ini->ReadBool(L"Window", L"Maximized", false);
	WindowTreeWidth     = scaled(std::max(0, ini->ReadInteger(L"Window", L"TreeWidth", 0)));
	WindowDetailsHeight = scaled(std::max(0, ini->ReadInteger(L"Window", L"DetailsHeight", 0)));
	FormSizes.clear();
	list->Clear();
	ini->ReadSectionValues(L"Windows", list.get());
	for (int i = 0; i < list->Count; ++i)
	{
		const String value = list->ValueFromIndex[i].LowerCase();
		const int x = value.Pos(L"x");
		const int w = x > 1 ? StrToIntDef(value.SubString(1, x - 1).Trim(), 0) : 0;
		const int h = x > 1 ? StrToIntDef(value.SubString(x + 1, 10).Trim(), 0) : 0;
		if (w > 0 && h > 0)
			FormSizes[list->Names[i]] = TPoint(scaled(w), scaled(h));
	}

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
	ini->WriteString(L"Grid", L"Hidden", GridHidden);
	ini->WriteInteger(L"Grid", L"RowHeight", GridRowHeight);
	ini->WriteString(L"Grid", L"Order", GridOrder);
	ini->WriteString(L"Grid", L"Align", GridAlign);
	String widths;
	for (std::map<String, int>::const_iterator it = GridWidths.begin(); it != GridWidths.end(); ++it)
		widths += (widths.IsEmpty() ? String() : String(L",")) + it->first + L":" + IntToStr(it->second);
	ini->WriteString(L"Grid", L"Widths", widths);

	ini->WriteInteger(L"Window", L"PixelsPerInch", Screen->PixelsPerInch);
	if (WindowBounds.right > WindowBounds.left && WindowBounds.bottom > WindowBounds.top)
	{
		ini->WriteInteger(L"Window", L"Left", WindowBounds.left);
		ini->WriteInteger(L"Window", L"Top", WindowBounds.top);
		ini->WriteInteger(L"Window", L"Width", WindowBounds.right - WindowBounds.left);
		ini->WriteInteger(L"Window", L"Height", WindowBounds.bottom - WindowBounds.top);
		ini->WriteBool(L"Window", L"Maximized", WindowMaximized);
		ini->WriteInteger(L"Window", L"TreeWidth", WindowTreeWidth);
		ini->WriteInteger(L"Window", L"DetailsHeight", WindowDetailsHeight);
	}
	for (std::map<String, TPoint>::const_iterator it = FormSizes.begin(); it != FormSizes.end(); ++it)
		ini->WriteString(L"Windows", it->first, IntToStr((int)it->second.x) + L"x" + IntToStr((int)it->second.y));

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
