//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <memory>
#include "u_Database.h"
#include "u_SeedData.h"
#include "u_Translator.h"
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
		if (version < 2) { MigrateTo2(); version = 2; }
		if (version < 3) { MigrateTo3(); version = 3; }
		if (version < 4) { MigrateTo4(); version = 4; }
		if (version < 5) { MigrateTo5(); version = 5; }
		if (version < 6) { MigrateTo6(); version = 6; }
		// if (version < 7) { MigrateTo7(); version = 7; }

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
// Копия файла БД рядом с ним: <имя>.<ASuffix>.bak (существующая перезаписывается).
// Соединение закрывается на время копирования, чтобы файл был согласованным;
// вызывать вне транзакции. Возвращает путь копии.
String TDatabase::Backup(const String &ASuffix)
{
	if (FConnection->InTransaction)
		throw Exception(L"TDatabase::Backup: called inside a transaction");

	const String dbFile = FConnection->Params->Values[L"Database"];
	const String bakFile = ChangeFileExt(dbFile, L"." + ASuffix + L".bak");

	FConnection->Connected = false;
	const bool copied = CopyFileW(dbFile.c_str(), bakFile.c_str(), FALSE);
	const DWORD error = copied ? 0 : GetLastError();
	FConnection->Connected = true;

	if (!copied)
		throw Exception(Format(Tr(L"Db.Error.Backup"), ARRAYOFCONST((bakFile, SysErrorMessage(error)))));
	return bakFile;
}
//---------------------------------------------------------------------------
// Копия перед миграцией: <имя>.v<версия>.bak.
void TDatabase::BackupBeforeMigration(int AVersion)
{
	Backup(L"v" + IntToStr(AVersion));
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
// v2: связь с Numista (задача 12): код эмитента у страны, тип и выпуск у монеты,
// id предмета коллекции Numista у экземпляра (для выгрузки, задача 13).
void TDatabase::MigrateTo2()
{
	FConnection->ExecSQL(L"ALTER TABLE countries ADD COLUMN numista_issuer TEXT");
	FConnection->ExecSQL(L"ALTER TABLE coins ADD COLUMN numista_type_id INTEGER");
	FConnection->ExecSQL(L"ALTER TABLE coins ADD COLUMN numista_issue_id INTEGER");
	FConnection->ExecSQL(L"ALTER TABLE coin_items ADD COLUMN numista_item_id INTEGER");
}
//---------------------------------------------------------------------------
// v3: индекс редкости Numista (задача 13a) — из атрибута R комментария коллекции.
void TDatabase::MigrateTo3()
{
	FConnection->ExecSQL(L"ALTER TABLE coins ADD COLUMN numista_rarity INTEGER");
}
//---------------------------------------------------------------------------
// v4: выпуски Numista (задача 13c) — тираж и примечание; coins.numista_issue_id
// ссылается на id без внешнего ключа (выпуск может быть ещё не загружен).
void TDatabase::MigrateTo4()
{
	FConnection->ExecSQL(
		L"CREATE TABLE numista_issues ("
		L"  id          INTEGER PRIMARY KEY,"
		L"  type_id     INTEGER NOT NULL,"
		L"  year        TEXT,"
		L"  mint_letter TEXT,"
		L"  mintage     INTEGER,"
		L"  comment     TEXT,"
		L"  updated_at  TEXT NOT NULL"
		L")");
	FConnection->ExecSQL(L"CREATE INDEX ix_numista_issues_type ON numista_issues (type_id)");
}
//---------------------------------------------------------------------------
// v5: характеристики типов Numista (задача 13d, getType); coins.numista_type_id
// ссылается на id без внешнего ключа. Правители и дворы — текстом через "; " (только показ).
void TDatabase::MigrateTo5()
{
	FConnection->ExecSQL(
		L"CREATE TABLE numista_types ("
		L"  id                  INTEGER PRIMARY KEY,"
		L"  title               TEXT,"
		L"  object_type         TEXT,"
		L"  min_year            INTEGER,"
		L"  max_year            INTEGER,"
		L"  composition         TEXT,"
		L"  technique           TEXT,"
		L"  weight              REAL,"
		L"  size                REAL,"
		L"  size2               REAL,"
		L"  thickness           REAL,"
		L"  shape               TEXT,"
		L"  is_demonetized      INTEGER,"
		L"  demonetization_date TEXT,"
		L"  rulers              TEXT,"
		L"  series              TEXT,"
		L"  commemorated_topic  TEXT,"
		L"  mints               TEXT,"
		L"  updated_at          TEXT NOT NULL"
		L")");
}
//---------------------------------------------------------------------------
// v6: фото монет (задача 14). Владелец — ровно один: экземпляр (свои снимки) или
// монета-тип; по одному фото на сторону. Файлы — в папке Photos, здесь только имена.
void TDatabase::MigrateTo6()
{
	FConnection->ExecSQL(
		L"CREATE TABLE coin_photos ("
		L"  id        INTEGER PRIMARY KEY,"
		L"  id_coin   INTEGER REFERENCES coins(id) ON DELETE CASCADE,"
		L"  id_item   INTEGER REFERENCES coin_items(id) ON DELETE CASCADE,"
		L"  side      TEXT NOT NULL CHECK (side IN ('obverse', 'reverse')),"
		L"  file_name TEXT NOT NULL,"
		L"  source    TEXT NOT NULL DEFAULT 'file',"
		L"  added_at  TEXT NOT NULL,"
		L"  CHECK ((id_coin IS NULL) <> (id_item IS NULL))"
		L")");
	FConnection->ExecSQL(L"CREATE UNIQUE INDEX ux_coin_photos_item ON coin_photos (id_item, side) WHERE id_item IS NOT NULL");
	FConnection->ExecSQL(L"CREATE UNIQUE INDEX ux_coin_photos_coin ON coin_photos (id_coin, side) WHERE id_coin IS NOT NULL");
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
