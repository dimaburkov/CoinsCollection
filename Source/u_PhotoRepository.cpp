//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <algorithm>
#include <memory>
#include <FireDAC.Comp.Client.hpp>
#include <Vcl.Imaging.jpeg.hpp>
#include "u_PhotoRepository.h"
#include "u_AppConfig.h"
#include "u_Database.h"
#include "u_Translator.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------

namespace
{
	const int cJpegQuality = 85;

	std::unique_ptr<TFDQuery> NewQuery(TDatabase *ADatabase, const String &ASql)
	{
		std::unique_ptr<TFDQuery> q(new TFDQuery(nullptr));
		q->Connection = ADatabase->Connection();
		q->SQL->Text = ASql;
		return q;
	}
	//-----------------------------------------------------------------------
	// Колонка владельца: экземпляр или монета-тип (имя колонки — из кода, не от пользователя).
	String OwnerColumn(bool AItem)
	{
		return AItem ? L"id_item" : L"id_coin";
	}
}
//---------------------------------------------------------------------------
TPhotoRepository::TPhotoRepository(TDatabase *ADatabase)
	: FDatabase(ADatabase)
{
}
//---------------------------------------------------------------------------
String TPhotoRepository::PhotosDir()
{
	return IncludeTrailingPathDelimiter(AppConfig().PhotosPath);
}
//---------------------------------------------------------------------------
String TPhotoRepository::SideName(TPhotoSide ASide)
{
	return ASide == psObverse ? L"obverse" : L"reverse";
}
//---------------------------------------------------------------------------
void TPhotoRepository::SetItemPhoto(int AItemId, TPhotoSide ASide, const String &ASourceFile)
{
	SetPhotoFromFile(true, AItemId, ASide, ASourceFile);
}
//---------------------------------------------------------------------------
void TPhotoRepository::SetCoinPhoto(int ACoinId, TPhotoSide ASide, const String &ASourceFile)
{
	SetPhotoFromFile(false, ACoinId, ASide, ASourceFile);
}
//---------------------------------------------------------------------------
void TPhotoRepository::SetItemPhoto(int AItemId, TPhotoSide ASide, TGraphic *AGraphic)
{
	SetPhoto(true, AItemId, ASide, AGraphic);
}
//---------------------------------------------------------------------------
void TPhotoRepository::SetCoinPhoto(int ACoinId, TPhotoSide ASide, TGraphic *AGraphic)
{
	SetPhoto(false, ACoinId, ASide, AGraphic);
}
//---------------------------------------------------------------------------
void TPhotoRepository::RemoveItemPhoto(int AItemId, TPhotoSide ASide)
{
	RemovePhoto(true, AItemId, ASide);
}
//---------------------------------------------------------------------------
void TPhotoRepository::RemoveCoinPhoto(int ACoinId, TPhotoSide ASide)
{
	RemovePhoto(false, ACoinId, ASide);
}
//---------------------------------------------------------------------------
// Формат определяет WIC (JPEG, PNG, BMP, TIFF, GIF ...); не картинка — понятная ошибка.
void TPhotoRepository::SetPhotoFromFile(bool AItem, int AOwnerId, TPhotoSide ASide, const String &ASourceFile)
{
	std::unique_ptr<TWICImage> image(new TWICImage());
	try
	{
		image->LoadFromFile(ASourceFile);
	}
	catch (Exception &)
	{
		throw Exception(Format(Tr(L"Photo.Error.NotImage"), ARRAYOFCONST((ExtractFileName(ASourceFile)))));
	}
	SetPhoto(AItem, AOwnerId, ASide, image.get());
}
//---------------------------------------------------------------------------
// Новый файл — уникальное имя (<item|coin>_<id>_<side>_<время>.jpg): после импорта Id
// меняются, перенесённые фото сохраняют прежние имена — совпадений не будет.
// Ошибка записи в БД — новый файл удаляется; после фиксации — удаляется старый.
void TPhotoRepository::SetPhoto(bool AItem, int AOwnerId, TPhotoSide ASide, TGraphic *AGraphic,
	const String &ASource, const String &ACopyright)
{
	if (!AGraphic || AGraphic->Empty || AGraphic->Width <= 0 || AGraphic->Height <= 0)
		throw Exception(Tr(L"Photo.Error.Empty"));

	const String dir = PhotosDir();
	if (!ForceDirectories(dir))
		throw Exception(Format(Tr(L"Photo.Error.Folder"), ARRAYOFCONST((dir))));
	const String name = String(AItem ? L"item_" : L"coin_") + IntToStr(AOwnerId) + L"_" + SideName(ASide)
		+ L"_" + FormatDateTime(L"yyyymmddhhnnsszzz", Now()) + L".jpg";
	SaveScaledJpeg(AGraphic, dir + name);

	String old;
	try
	{
		TDbTransaction tx(FDatabase->Connection());
		old = FileOf(AItem, AOwnerId, ASide);

		const String owner = OwnerColumn(AItem);
		std::unique_ptr<TFDQuery> q = NewQuery(FDatabase,
			L"DELETE FROM coin_photos WHERE " + owner + L" = :owner AND side = :side");
		q->ParamByName(L"owner")->AsInteger   = AOwnerId;
		q->ParamByName(L"side")->AsWideString = SideName(ASide);
		q->ExecSQL();

		q->SQL->Text = L"INSERT INTO coin_photos (" + owner + L", side, file_name, source, added_at, copyright)"
		               L" VALUES (:owner, :side, :file_name, :source, :added_at, :copyright)";
		q->ParamByName(L"owner")->AsInteger        = AOwnerId;
		q->ParamByName(L"side")->AsWideString      = SideName(ASide);
		q->ParamByName(L"file_name")->AsWideString = name;
		q->ParamByName(L"source")->AsWideString    = ASource;
		q->ParamByName(L"added_at")->AsWideString  = FormatDateTime(L"yyyy-mm-dd hh:nn:ss", Now());
		TFDParam *copyright = q->ParamByName(L"copyright");
		copyright->DataType = ftWideString;
		if (ACopyright.Trim().IsEmpty())
			copyright->Clear();
		else
			copyright->AsWideString = ACopyright.Trim();
		q->ExecSQL();
		tx.Commit();
	}
	catch (...)
	{
		DeleteFile(dir + name);
		throw;
	}
	if (!old.IsEmpty() && !SameText(old, name))
		DeleteFile(dir + old);
}
//---------------------------------------------------------------------------
void TPhotoRepository::RemovePhoto(bool AItem, int AOwnerId, TPhotoSide ASide)
{
	TDbTransaction tx(FDatabase->Connection());
	const String old = FileOf(AItem, AOwnerId, ASide);
	if (old.IsEmpty())
		return;
	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase,
		L"DELETE FROM coin_photos WHERE " + OwnerColumn(AItem) + L" = :owner AND side = :side");
	q->ParamByName(L"owner")->AsInteger   = AOwnerId;
	q->ParamByName(L"side")->AsWideString = SideName(ASide);
	q->ExecSQL();
	tx.Commit();
	DeleteFile(PhotosDir() + old);
}
//---------------------------------------------------------------------------
String TPhotoRepository::FileOf(bool AItem, int AOwnerId, TPhotoSide ASide, String *ACopyright)
{
	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase,
		L"SELECT file_name, copyright FROM coin_photos WHERE " + OwnerColumn(AItem) + L" = :owner AND side = :side");
	q->ParamByName(L"owner")->AsInteger   = AOwnerId;
	q->ParamByName(L"side")->AsWideString = SideName(ASide);
	q->Open();
	if (ACopyright)
		*ACopyright = q->Eof ? String() : q->Fields->Fields[1]->AsString;
	return q->Eof ? String() : q->Fields->Fields[0]->AsString;
}
//---------------------------------------------------------------------------
String TPhotoRepository::PhotoFile(int AItemId, int ACoinId, TPhotoSide ASide, bool *AIsTypePhoto,
	String *ACopyright)
{
	if (AIsTypePhoto)
		*AIsTypePhoto = false;
	if (ACopyright)
		*ACopyright = L"";
	const String dir = PhotosDir();
	String copyright;
	if (AItemId != 0)
	{
		const String name = FileOf(true, AItemId, ASide, &copyright);
		if (!name.IsEmpty() && FileExists(dir + name))
		{
			if (ACopyright)
				*ACopyright = copyright;
			return dir + name;
		}
	}
	if (ACoinId != 0)
	{
		const String name = FileOf(false, ACoinId, ASide, &copyright);
		if (!name.IsEmpty() && FileExists(dir + name))
		{
			if (AIsTypePhoto)
				*AIsTypePhoto = true;
			if (ACopyright)
				*ACopyright = copyright;
			return dir + name;
		}
	}
	return String();
}
//---------------------------------------------------------------------------
int TPhotoRepository::PhotoCount()
{
	return FDatabase->Connection()->ExecSQLScalar(L"SELECT COUNT(*) FROM coin_photos");
}
//---------------------------------------------------------------------------
// Уменьшение до MaxSize по длинной стороне (WIC — высококачественная кубическая
// интерполяция, прочие картинки — HALFTONE), белый фон под прозрачностью, JPEG 85.
void TPhotoRepository::SaveScaledJpeg(TGraphic *AGraphic, const String &AFileName)
{
	const int w = AGraphic->Width, h = AGraphic->Height;
	const double k = std::min(1.0, (double)MaxSize / std::max(w, h));
	const int nw = std::max(1, (int)(w * k + 0.5)), nh = std::max(1, (int)(h * k + 0.5));

	TGraphic *source = AGraphic;
	std::unique_ptr<TWICImage> scaled;
	TWICImage *wic = dynamic_cast<TWICImage *>(AGraphic);
	if (wic && (nw != w || nh != h))
	{
		scaled.reset(wic->CreateScaledCopy(nw, nh, wipmHighQualityCubic));
		source = scaled.get();
	}

	std::unique_ptr<TBitmap> bitmap(new TBitmap());
	bitmap->PixelFormat = pf24bit;
	bitmap->SetSize(nw, nh);
	bitmap->Canvas->Brush->Color = clWhite;
	bitmap->Canvas->FillRect(TRect(0, 0, nw, nh));
	if (source->Width == nw && source->Height == nh)
		bitmap->Canvas->Draw(0, 0, source);
	else
	{
		SetStretchBltMode(bitmap->Canvas->Handle, HALFTONE);
		SetBrushOrgEx(bitmap->Canvas->Handle, 0, 0, nullptr);
		bitmap->Canvas->StretchDraw(TRect(0, 0, nw, nh), source);
	}

	std::unique_ptr<TJPEGImage> jpeg(new TJPEGImage());
	jpeg->CompressionQuality = cJpegQuality;
	jpeg->Assign(bitmap.get());
	jpeg->Compress();
	jpeg->SaveToFile(AFileName);
}
//---------------------------------------------------------------------------
void TPhotoRepository::DeleteFiles(const std::vector<String> &AFileNames)
{
	const String dir = PhotosDir();
	for (size_t i = 0; i < AFileNames.size(); ++i)
		DeleteFile(dir + AFileNames[i]);
}
//---------------------------------------------------------------------------
void TPhotoRepository::MoveFiles(const std::vector<String> &AFileNames, const String &ASubDir)
{
	if (AFileNames.empty())
		return;
	const String dir = PhotosDir();
	const String target = IncludeTrailingPathDelimiter(dir + ASubDir);
	if (!ForceDirectories(target))
		return;
	for (size_t i = 0; i < AFileNames.size(); ++i)
		RenameFile(dir + AFileNames[i], target + AFileNames[i]);
}
//---------------------------------------------------------------------------
