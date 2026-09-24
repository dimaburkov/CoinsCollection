//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <memory>
#include "u_Database.h"
#include "u_SeedData.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------

TDatabase::TDatabase(TFDConnection *AConnection)
	: FConnection(AConnection)
{
}
//---------------------------------------------------------------------------
TDatabase::~TDatabase()
{
}
//---------------------------------------------------------------------------
void TDatabase::Open(const String &AFileName)
{
	const String dir = ExtractFileDir(AFileName);
	if (!dir.IsEmpty() && !ForceDirectories(dir))
		throw Exception(L"Cannot create database folder: " + dir);

	FConnection->Connected = false;
	FConnection->Params->Values[L"Database"] = AFileName;
	FConnection->Connected = true;	// OpenMode по умолчанию CreateUTF8 — файл создаётся
}
//---------------------------------------------------------------------------
void TDatabase::Close()
{
	FConnection->Connected = false;
}
//---------------------------------------------------------------------------
bool TDatabase::IsOpen() const
{
	return FConnection->Connected;
}
//---------------------------------------------------------------------------
int TDatabase::SchemaVersion() const
{
	return FConnection->ExecSQLScalar(L"PRAGMA user_version");
}
//---------------------------------------------------------------------------
void TDatabase::EnsureSchema()
{
	int version = SchemaVersion();
	if (version == CurrentSchemaVersion)
		return;
	if (version > CurrentSchemaVersion)
		throw Exception(L"Database schema version %d is newer than supported (%d).",
			ARRAYOFCONST((version, CurrentSchemaVersion)));

	// Новая пустая БД (версия 0) — копировать нечего.
	if (version > 0)
		BackupBeforeMigration(version);

	FConnection->StartTransaction();
	try
	{
		// Лестница миграций: каждый шаг поднимает схему на одну версию.
		if (version < 1) { MigrateTo1(); version = 1; }
		// if (version < 2) { MigrateTo2(); version = 2; }

		// PRAGMA не принимает параметры; version — целое из кода.
		FConnection->ExecSQL(L"PRAGMA user_version = " + IntToStr(version));
		FConnection->Commit();
	}
	catch (...)
	{
		FConnection->Rollback();
		throw;
	}
}
//---------------------------------------------------------------------------
// Копия файла БД перед миграцией: <имя>.v<версия>.bak рядом с БД.
// Соединение закрывается на время копирования, чтобы файл был согласованным.
void TDatabase::BackupBeforeMigration(int AVersion)
{
	const String dbFile = FConnection->Params->Values[L"Database"];
	const String bakFile = ChangeFileExt(dbFile, L".v" + IntToStr(AVersion) + L".bak");

	FConnection->Connected = false;
	const bool copied = CopyFileW(dbFile.c_str(), bakFile.c_str(), FALSE);
	const DWORD error = copied ? 0 : GetLastError();
	FConnection->Connected = true;

	if (!copied)
		throw Exception(L"Cannot back up database before upgrade to " + bakFile
			+ L": " + SysErrorMessage(error));
}
//---------------------------------------------------------------------------
// v1: нормализованная схема (3НФ) + справочники continents, countries, conditions.
void TDatabase::MigrateTo1()
{
	FConnection->ExecSQL(
		L"CREATE TABLE continents ("
		L"  id   INTEGER PRIMARY KEY,"
		L"  name TEXT NOT NULL UNIQUE"
		L")");

	FConnection->ExecSQL(
		L"CREATE TABLE countries ("
		L"  id           INTEGER PRIMARY KEY,"
		L"  name         TEXT NOT NULL UNIQUE COLLATE NOCASE,"
		L"  id_continent INTEGER REFERENCES continents(id),"
		L"  is_exist     INTEGER NOT NULL DEFAULT 1"
		L")");

	FConnection->ExecSQL(
		L"CREATE TABLE periods ("
		L"  id         INTEGER PRIMARY KEY,"
		L"  id_country INTEGER NOT NULL REFERENCES countries(id),"
		L"  name       TEXT NOT NULL,"
		L"  UNIQUE (id_country, name)"
		L")");

	FConnection->ExecSQL(
		L"CREATE TABLE currencies ("
		L"  id   INTEGER PRIMARY KEY,"
		L"  name TEXT NOT NULL UNIQUE COLLATE NOCASE"
		L")");

	FConnection->ExecSQL(
		L"CREATE TABLE conditions ("
		L"  id         INTEGER PRIMARY KEY,"
		L"  code       TEXT NOT NULL UNIQUE,"
		L"  name       TEXT NOT NULL,"
		L"  sort_order INTEGER NOT NULL"
		L")");

	FConnection->ExecSQL(
		L"CREATE TABLE coins ("
		L"  id             INTEGER PRIMARY KEY,"
		L"  id_period      INTEGER NOT NULL REFERENCES periods(id),"
		L"  id_currency    INTEGER REFERENCES currencies(id),"
		L"  denomination   TEXT NOT NULL,"
		L"  year           INTEGER,"
		L"  variety        TEXT,"
		L"  subject        TEXT,"
		L"  diameter_mm    REAL,"
		L"  catalog_number TEXT"
		L")");
	FConnection->ExecSQL(L"CREATE INDEX ix_coins_period ON coins (id_period)");

	FConnection->ExecSQL(
		L"CREATE TABLE coin_items ("
		L"  id                INTEGER PRIMARY KEY,"
		L"  id_coin           INTEGER NOT NULL REFERENCES coins(id) ON DELETE CASCADE,"
		L"  id_condition      INTEGER REFERENCES conditions(id),"
		L"  quantity          INTEGER NOT NULL DEFAULT 1,"
		L"  need_to_replace   INTEGER NOT NULL DEFAULT 0,"
		L"  catalog_value_uah REAL,"
		L"  my_value_uah      REAL,"
		L"  purchase_date     TEXT,"
		L"  published_date    TEXT,"
		L"  swap_info         TEXT,"
		L"  color             TEXT,"
		L"  grading_company   TEXT,"
		L"  grading_number    TEXT,"
		L"  grade             TEXT,"
		L"  label_text        TEXT,"
		L"  comment           TEXT"
		L")");
	FConnection->ExecSQL(L"CREATE INDEX ix_coin_items_coin ON coin_items (id_coin)");

	SeedReferenceData();
}
//---------------------------------------------------------------------------
// Справочники с начальным составом (u_SeedData). «Extinct states» — не континент:
// у исчезнувшей страны свой континент и is_exist = 0.
void TDatabase::SeedReferenceData()
{
	std::unique_ptr<TFDQuery> q(new TFDQuery(nullptr));
	q->Connection = FConnection;

	q->SQL->Text = L"INSERT INTO continents (id, name) VALUES (:id, :name)";
	for (int i = 0; i < SeedContinentCount; ++i)
	{
		q->ParamByName(L"id")->AsInteger     = SeedContinents[i].Id;
		q->ParamByName(L"name")->AsWideString = SeedContinents[i].Name;
		q->ExecSQL();
	}

	q->SQL->Text = L"INSERT INTO countries (name, id_continent, is_exist) VALUES (:name, :id_continent, :is_exist)";
	for (int i = 0; i < SeedCountryCount; ++i)
	{
		q->ParamByName(L"name")->AsWideString      = SeedCountries[i].Name;
		q->ParamByName(L"id_continent")->AsInteger = SeedCountries[i].ContinentId;
		q->ParamByName(L"is_exist")->AsInteger     = SeedCountries[i].IsExist ? 1 : 0;
		q->ExecSQL();
	}

	// Порядок совпадает с enum TCoinCondition (u_CoinTypes.h).
	FConnection->ExecSQL(
		L"INSERT INTO conditions (code, name, sort_order) VALUES "
		L"('P',   'Poor',               1),"
		L"('FR',  'Fair',               2),"
		L"('G',   'Good',               3),"
		L"('VG',  'Very Good',          4),"
		L"('F',   'Fine',               5),"
		L"('VF',  'Very Fine',          6),"
		L"('XF',  'Extremely Fine',     7),"
		L"('AU',  'About Uncirculated', 8),"
		L"('UNC', 'Uncirculated',       9),"
		L"('PRF', 'Proof',             10)");
}
//---------------------------------------------------------------------------
