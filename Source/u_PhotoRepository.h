//---------------------------------------------------------------------------
// Фото монет (задача 14): файлы JPEG в папке AppConfig().PhotosPath, в БД —
// только имена (таблица coin_photos). Владелец фото — экземпляр (свои снимки)
// или монета-тип; показывается фото экземпляра, иначе фото типа.
//---------------------------------------------------------------------------
#ifndef u_PhotoRepositoryH
#define u_PhotoRepositoryH
//---------------------------------------------------------------------------
#include <System.hpp>
#include <Vcl.Graphics.hpp>
#include <vector>
//---------------------------------------------------------------------------
class TDatabase;
//---------------------------------------------------------------------------
enum TPhotoSide { psObverse, psReverse };
//---------------------------------------------------------------------------
class TPhotoRepository
{
public:
	static const int MaxSize = 1200;	// длинная сторона сохранённого фото, px

	explicit TPhotoRepository(TDatabase *ADatabase);

	// Добавить / заменить фото: изображение уменьшается до MaxSize по длинной стороне
	// и сохраняется JPEG в папку фото; старый файл заменённого фото удаляется.
	// Не изображение — исключение с понятным текстом.
	void SetItemPhoto(int AItemId, TPhotoSide ASide, const String &ASourceFile);
	void SetCoinPhoto(int ACoinId, TPhotoSide ASide, const String &ASourceFile);
	// То же из картинки в памяти (вставка из буфера обмена — задача 15).
	void SetItemPhoto(int AItemId, TPhotoSide ASide, TGraphic *AGraphic);
	void SetCoinPhoto(int ACoinId, TPhotoSide ASide, TGraphic *AGraphic);
	// Фото типа с источником (задача 17b): ASource — "numista", ACopyright — автор фото.
	void SetCoinPhoto(int ACoinId, TPhotoSide ASide, TGraphic *AGraphic, const String &ASource,
		const String &ACopyright);
	// Фото экземпляра с источником (задача 20: свои фото предмета с Numista).
	void SetItemPhoto(int AItemId, TPhotoSide ASide, TGraphic *AGraphic, const String &ASource,
		const String &ACopyright);
	// Источник фото типа ("file" / "numista"); "" — фото нет.
	String CoinPhotoSource(int ACoinId, TPhotoSide ASide);

	// Удалить фото (строку и файл); фото нет — ничего не делает.
	void RemoveItemPhoto(int AItemId, TPhotoSide ASide);
	void RemoveCoinPhoto(int ACoinId, TPhotoSide ASide);

	// Полный путь показываемого фото: экземпляра, иначе типа; "" — нет.
	// Строка есть, а файла на диске нет — считается «нет фото».
	// AIsTypePhoto — показано фото типа.
	// ACopyright — автор показанного фото ("" — свой снимок).
	String PhotoFile(int AItemId, int ACoinId, TPhotoSide ASide, bool *AIsTypePhoto = nullptr,
		String *ACopyright = nullptr);

	// Число фото в коллекции.
	int PhotoCount();

	static String PhotosDir();
	static String SideName(TPhotoSide ASide);	// "obverse" / "reverse"
	// Файлы в папке фото: удалить / перенести в подпапку (ошибки отдельных файлов пропускаются).
	static void DeleteFiles(const std::vector<String> &AFileNames);
	static void MoveFiles(const std::vector<String> &AFileNames, const String &ASubDir);

private:
	TDatabase *FDatabase;	// не владеет

	void   SetPhoto(bool AItem, int AOwnerId, TPhotoSide ASide, TGraphic *AGraphic,
		const String &ASource = L"file", const String &ACopyright = String());
	void   SetPhotoFromFile(bool AItem, int AOwnerId, TPhotoSide ASide, const String &ASourceFile);
	void   RemovePhoto(bool AItem, int AOwnerId, TPhotoSide ASide);
	String FileOf(bool AItem, int AOwnerId, TPhotoSide ASide, String *ACopyright = nullptr);
	static void SaveScaledJpeg(TGraphic *AGraphic, const String &AFileName);
};
//---------------------------------------------------------------------------
#endif
