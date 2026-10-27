//---------------------------------------------------------------------------
// Резервная копия коллекции (задача 21): один zip — база, фото, настройки и backup.txt.
//   Db\<имя базы>.db, Photos\*.jpg, CoinsCollection.ini, backup.txt (сведения о копии).
// Восстановление (замена базы и фото) — в главном окне; здесь — чтение и распаковка копии.
//---------------------------------------------------------------------------
#ifndef u_BackupH
#define u_BackupH
//---------------------------------------------------------------------------
#include <System.hpp>
#include <functional>
//---------------------------------------------------------------------------
class TDatabase;
//---------------------------------------------------------------------------
struct TBackupInfo
{
	String Created;     // 'yyyy-mm-dd hh:nn:ss'
	String AppVersion;
	String DbName;      // имя файла базы в копии
	int    Schema;      // версия схемы базы
	int    Items;
	int    Photos;
	TBackupInfo() : Schema(0), Items(0), Photos(0) {}
};
//---------------------------------------------------------------------------
// Ход: (сделано, всего) — для окна хода.
typedef std::function<void(int, int)> TBackupProgress;

// Создать копию AZipFile (существующий файл заменяется). AItems — число экземпляров (для backup.txt).
// Соединение с базой закрывается на время копирования файла базы. AInfo — что записано.
void CreateBackup(TDatabase &ADatabase, const String &AZipFile, int AItems, TBackupInfo &AInfo,
	TBackupProgress AProgress);

// Сведения о копии; false — это не копия Coins Collection (нет Db\*.db).
bool ReadBackupInfo(const String &AZipFile, TBackupInfo &AInfo);

// Распаковать копию во временную папку; возвращает её путь (с "\"). Удалить — DeleteBackupFolder.
String ExtractBackup(const String &AZipFile);
void DeleteBackupFolder(const String &AFolder);

// Версия схемы файла базы SQLite (PRAGMA user_version — смещение 60 заголовка); -1 — не база.
int SchemaVersionOfFile(const String &ADbFile);
//---------------------------------------------------------------------------
#endif
