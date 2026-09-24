//---------------------------------------------------------------------------
// Управление файлом БД SQLite: открытие/закрытие и миграции схемы.
//---------------------------------------------------------------------------
#ifndef u_DatabaseH
#define u_DatabaseH
//---------------------------------------------------------------------------
#include <System.hpp>
#include <FireDAC.Comp.Client.hpp>
//---------------------------------------------------------------------------
class TDatabase
{
public:
	// Актуальная версия схемы (PRAGMA user_version).
	static const int CurrentSchemaVersion = 1;

	explicit TDatabase(TFDConnection *AConnection);	// не владеет соединением
	~TDatabase();

	// Открыть БД по пути AFileName (файл создаётся, если его нет).
	void Open(const String &AFileName);
	void Close();
	bool IsOpen() const;

	// Привести схему к актуальной версии (PRAGMA user_version).
	void EnsureSchema();
	// Версия схемы в открытой БД.
	int  SchemaVersion() const;

	TFDConnection *Connection() const { return FConnection; }

private:
	TFDConnection *FConnection;

	void MigrateTo1();
	void SeedReferenceData();
	void BackupBeforeMigration(int AVersion);
};
//---------------------------------------------------------------------------
#endif
