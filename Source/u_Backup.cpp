//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <memory>
#include <System.Classes.hpp>
#include <System.IOUtils.hpp>
#include <System.Zip.hpp>
#include "u_Backup.h"
#include "u_AppConfig.h"
#include "u_Database.h"
#include "u_PhotoRepository.h"
#include "versionConfig.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
namespace
{
	const String cInfoFile = L"backup.txt";
	const String cIniFile  = L"CoinsCollection.ini";

	String TempFolder()
	{
		return IncludeTrailingPathDelimiter(TPath::Combine(TPath::GetTempPath(),
			L"CoinsCollection-backup-" + FormatDateTime(L"yyyymmdd-hhnnss-zzz", Now())));
	}
	//-----------------------------------------------------------------------
	String InfoText(const TBackupInfo &AInfo)
	{
		return L"Created=" + AInfo.Created + L"\r\nApp=" + AInfo.AppVersion + L"\r\nDb=" + AInfo.DbName
			+ L"\r\nSchema=" + IntToStr(AInfo.Schema) + L"\r\nItems=" + IntToStr(AInfo.Items)
			+ L"\r\nPhotos=" + IntToStr(AInfo.Photos) + L"\r\n";
	}
}
//---------------------------------------------------------------------------
void CreateBackup(TDatabase &ADatabase, const String &AZipFile, int AItems, TBackupInfo &AInfo,
	TBackupProgress AProgress)
{
	const String dbFile = AppConfig().DbPath;
	const String temp = TempFolder();
	ForceDirectories(temp);
	try
	{
		// Копия базы — при закрытом соединении (файл не меняется во время копирования).
		const String dbCopy = temp + ExtractFileName(dbFile);
		ADatabase.CopyTo(dbCopy);

		const String photosDir = TPhotoRepository::PhotosDir();
		DynamicArray<String> photos;
		if (TDirectory::Exists(photosDir))
			photos = TDirectory::GetFiles(photosDir, L"*.jpg", TSearchOption::soTopDirectoryOnly);

		AInfo = TBackupInfo();
		AInfo.Created    = FormatDateTime(L"yyyy-mm-dd hh:nn:ss", Now());
		AInfo.AppVersion = AppInfo::Version;
		AInfo.DbName     = ExtractFileName(dbFile);
		AInfo.Schema     = SchemaVersionOfFile(dbCopy);
		AInfo.Items      = AItems;
		AInfo.Photos     = photos.Length;

		if (FileExists(AZipFile))
			DeleteFile(AZipFile);
		std::unique_ptr<TZipFile> zip(new TZipFile());
		zip->Open(AZipFile, zmWrite);
		zip->Add(dbCopy, L"Db/" + AInfo.DbName);
		const String ini = TAppConfig::ExeDir() + cIniFile;
		if (FileExists(ini))
			zip->Add(ini, cIniFile);
		for (int i = 0; i < photos.Length; ++i)
		{
			if (AProgress)
				AProgress(i, photos.Length);
			zip->Add(photos[i], L"Photos/" + ExtractFileName(photos[i]), zcStored);	// JPEG уже сжат
		}
		zip->Add(TEncoding::UTF8->GetBytes(InfoText(AInfo)), cInfoFile);
		zip->Close();
	}
	__finally
	{
		DeleteBackupFolder(temp);
	}
}
//---------------------------------------------------------------------------
bool ReadBackupInfo(const String &AZipFile, TBackupInfo &AInfo)
{
	AInfo = TBackupInfo();
	if (!TZipFile::IsValid(AZipFile))
		return false;
	std::unique_ptr<TZipFile> zip(new TZipFile());
	zip->Open(AZipFile, zmRead);
	bool hasDb = false;
	const DynamicArray<String> names = zip->FileNames;
	for (int i = 0; i < names.Length; ++i)
	{
		const String name = StringReplace(names[i], L"\\", L"/", TReplaceFlags() << rfReplaceAll);
		if (name.Pos(L"Db/") == 1 && SameText(ExtractFileExt(name), L".db"))
		{
			hasDb = true;
			AInfo.DbName = name.SubString(4, name.Length());
		}
		else if (name.Pos(L"Photos/") == 1)
			++AInfo.Photos;
	}
	if (!hasDb)
		return false;
	if (zip->IndexOf(cInfoFile) >= 0)
	{
		TBytes bytes;
		zip->Read(cInfoFile, bytes);
		std::unique_ptr<TStringList> lines(new TStringList());
		lines->Text = TEncoding::UTF8->GetString(bytes);
		AInfo.Created    = lines->Values[L"Created"];
		AInfo.AppVersion = lines->Values[L"App"];
		AInfo.Schema     = StrToIntDef(lines->Values[L"Schema"], 0);
		AInfo.Items      = StrToIntDef(lines->Values[L"Items"], 0);
	}
	return true;
}
//---------------------------------------------------------------------------
String ExtractBackup(const String &AZipFile)
{
	const String temp = TempFolder();
	ForceDirectories(temp);
	std::unique_ptr<TZipFile> zip(new TZipFile());
	zip->Open(AZipFile, zmRead);
	zip->ExtractAll(temp);
	zip->Close();
	return temp;
}
//---------------------------------------------------------------------------
void DeleteBackupFolder(const String &AFolder)
{
	try
	{
		if (TDirectory::Exists(AFolder))
			TDirectory::Delete(AFolder, true);
	}
	catch (Exception &)
	{
		// временная папка — не критично
	}
}
//---------------------------------------------------------------------------
int SchemaVersionOfFile(const String &ADbFile)
{
	// Заголовок SQLite: "SQLite format 3\0" (16 байт), user_version — big-endian на смещении 60.
	std::unique_ptr<TFileStream> f;
	try
	{
		f.reset(new TFileStream(ADbFile, fmOpenRead | fmShareDenyNone));
	}
	catch (Exception &)
	{
		return -1;
	}
	unsigned char header[64];
	if (f->Read(header, sizeof(header)) != (int)sizeof(header) || memcmp(header, "SQLite format 3", 15) != 0)
		return -1;
	return (header[60] << 24) | (header[61] << 16) | (header[62] << 8) | header[63];
}
//---------------------------------------------------------------------------
