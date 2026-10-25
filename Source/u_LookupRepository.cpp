//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <memory>
#include <FireDAC.Comp.Client.hpp>
#include "u_LookupRepository.h"
#include "u_Database.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------

namespace
{
	std::unique_ptr<TFDQuery> NewQuery(TDatabase *ADatabase, const String &ASql)
	{
		std::unique_ptr<TFDQuery> q(new TFDQuery(nullptr));
		q->Connection = ADatabase->Connection();
		q->SQL->Text = ASql;
		return q;
	}
	//-----------------------------------------------------------------------
	// id первой строки результата или 0, если строк нет.
	int FirstId(TFDQuery *AQuery)
	{
		AQuery->Open();
		const int id = AQuery->Eof ? 0 : AQuery->Fields->Fields[0]->AsInteger;
		AQuery->Close();
		return id;
	}
	//-----------------------------------------------------------------------
	int LastInsertId(TDatabase *ADatabase)
	{
		return ADatabase->Connection()->ExecSQLScalar(L"SELECT last_insert_rowid()");
	}
}
//---------------------------------------------------------------------------
TLookupRepository::TLookupRepository(TDatabase *ADatabase)
	: FDatabase(ADatabase)
{
}
//---------------------------------------------------------------------------
TLookupRepository::~TLookupRepository()
{
}
//---------------------------------------------------------------------------
std::vector<TContinent> TLookupRepository::Continents()
{
	std::vector<TContinent> result;
	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase, L"SELECT id, name FROM continents ORDER BY name");
	for (q->Open(); !q->Eof; q->Next())
	{
		TContinent r;
		r.Id   = q->FieldByName(L"id")->AsInteger;
		r.Name = q->FieldByName(L"name")->AsString;
		result.push_back(r);
	}
	return result;
}
//---------------------------------------------------------------------------
std::vector<TCountry> TLookupRepository::Countries()
{
	std::vector<TCountry> result;
	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase,
		L"SELECT id, name, id_continent, is_exist, numista_issuer FROM countries ORDER BY name");
	for (q->Open(); !q->Eof; q->Next())
	{
		TCountry r;
		r.Id          = q->FieldByName(L"id")->AsInteger;
		r.Name        = q->FieldByName(L"name")->AsString;
		r.ContinentId = q->FieldByName(L"id_continent")->IsNull ? 0 : q->FieldByName(L"id_continent")->AsInteger;
		r.IsExist     = q->FieldByName(L"is_exist")->AsInteger != 0;
		r.NumistaIssuer = q->FieldByName(L"numista_issuer")->IsNull ? String() : q->FieldByName(L"numista_issuer")->AsString;
		result.push_back(r);
	}
	return result;
}
//---------------------------------------------------------------------------
std::vector<TPeriod> TLookupRepository::Periods(int ACountryId)
{
	std::vector<TPeriod> result;
	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase,
		L"SELECT id, id_country, name FROM periods WHERE id_country = :id_country ORDER BY name");
	q->ParamByName(L"id_country")->AsInteger = ACountryId;
	for (q->Open(); !q->Eof; q->Next())
	{
		TPeriod r;
		r.Id        = q->FieldByName(L"id")->AsInteger;
		r.CountryId = q->FieldByName(L"id_country")->AsInteger;
		r.Name      = q->FieldByName(L"name")->AsString;
		result.push_back(r);
	}
	return result;
}
//---------------------------------------------------------------------------
std::vector<TCurrency> TLookupRepository::Currencies()
{
	std::vector<TCurrency> result;
	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase, L"SELECT id, name FROM currencies ORDER BY name");
	for (q->Open(); !q->Eof; q->Next())
	{
		TCurrency r;
		r.Id   = q->FieldByName(L"id")->AsInteger;
		r.Name = q->FieldByName(L"name")->AsString;
		result.push_back(r);
	}
	return result;
}
//---------------------------------------------------------------------------
std::vector<TCondition> TLookupRepository::Conditions()
{
	std::vector<TCondition> result;
	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase,
		L"SELECT id, code, name, sort_order FROM conditions ORDER BY sort_order");
	for (q->Open(); !q->Eof; q->Next())
	{
		TCondition r;
		r.Id        = q->FieldByName(L"id")->AsInteger;
		r.Code      = q->FieldByName(L"code")->AsString;
		r.Name      = q->FieldByName(L"name")->AsString;
		r.SortOrder = q->FieldByName(L"sort_order")->AsInteger;
		result.push_back(r);
	}
	return result;
}
//---------------------------------------------------------------------------
int TLookupRepository::FindOrCreateCountry(const String &AName)
{
	const String name = AName.Trim();
	if (name.IsEmpty())
		throw Exception(L"Country is required.");

	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase, L"SELECT id FROM countries WHERE name = :name");
	q->ParamByName(L"name")->AsWideString = name;	// колонка COLLATE NOCASE
	int id = FirstId(q.get());
	if (id == 0)
	{
		q->SQL->Text = L"INSERT INTO countries (name) VALUES (:name)";	// id_continent NULL, is_exist 1
		q->ParamByName(L"name")->AsWideString = name;
		q->ExecSQL();
		id = LastInsertId(FDatabase);
	}
	return id;
}
//---------------------------------------------------------------------------
int TLookupRepository::FindOrCreatePeriod(int ACountryId, const String &AName)
{
	const String name = AName.Trim();

	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase,
		L"SELECT id FROM periods WHERE id_country = :id_country AND name = :name");
	q->ParamByName(L"id_country")->AsInteger = ACountryId;
	q->ParamByName(L"name")->AsWideString    = name;
	int id = FirstId(q.get());
	if (id == 0)
	{
		q->SQL->Text = L"INSERT INTO periods (id_country, name) VALUES (:id_country, :name)";
		q->ParamByName(L"id_country")->AsInteger = ACountryId;
		q->ParamByName(L"name")->AsWideString    = name;
		q->ExecSQL();
		id = LastInsertId(FDatabase);
	}
	return id;
}
//---------------------------------------------------------------------------
int TLookupRepository::FindOrCreateCurrency(const String &AName)
{
	const String name = AName.Trim();
	if (name.IsEmpty())
		return 0;

	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase, L"SELECT id FROM currencies WHERE name = :name");
	q->ParamByName(L"name")->AsWideString = name;
	int id = FirstId(q.get());
	if (id == 0)
	{
		q->SQL->Text = L"INSERT INTO currencies (name) VALUES (:name)";
		q->ParamByName(L"name")->AsWideString = name;
		q->ExecSQL();
		id = LastInsertId(FDatabase);
	}
	return id;
}
//---------------------------------------------------------------------------
int TLookupRepository::FindCondition(const String &ACode)
{
	const String code = ACode.Trim();
	if (code.IsEmpty())
		return 0;

	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase, L"SELECT id FROM conditions WHERE code = :code");
	q->ParamByName(L"code")->AsWideString = code.UpperCase();
	const int id = FirstId(q.get());
	if (id == 0)
		throw Exception(L"Unknown condition code: " + code);
	return id;
}
//---------------------------------------------------------------------------
void TLookupRepository::UpdateCountry(const TCountry &ACountry)
{
	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase,
		L"UPDATE countries SET id_continent = :id_continent, is_exist = :is_exist WHERE id = :id");
	TFDParam *continent = q->ParamByName(L"id_continent");
	continent->DataType = ftInteger;
	if (ACountry.ContinentId == 0)
		continent->Clear();
	else
		continent->AsInteger = ACountry.ContinentId;
	q->ParamByName(L"is_exist")->AsInteger = ACountry.IsExist ? 1 : 0;
	q->ParamByName(L"id")->AsInteger       = ACountry.Id;
	q->ExecSQL();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Правка справочников (форма Reference)
//---------------------------------------------------------------------------
namespace
{
	std::map<int, int> UsageMap(TDatabase *ADatabase, const String &ASql, int AParam = 0, bool AUseParam = false)
	{
		std::map<int, int> result;
		std::unique_ptr<TFDQuery> q = NewQuery(ADatabase, ASql);
		if (AUseParam)
			q->Params->Items[0]->AsInteger = AParam;
		for (q->Open(); !q->Eof; q->Next())
			result[q->Fields->Fields[0]->AsInteger] = q->Fields->Fields[1]->AsInteger;
		return result;
	}
	//-----------------------------------------------------------------------
	String RequireName(const String &AName)
	{
		const String name = AName.Trim();
		if (name.IsEmpty())
			throw Exception(L"Name is required.");
		return name;
	}
}
//---------------------------------------------------------------------------
std::map<int, int> TLookupRepository::CountryUsage()
{
	return UsageMap(FDatabase,
		L"SELECT p.id_country, COUNT(*) FROM coin_items i"
		L"  JOIN coins c ON c.id = i.id_coin JOIN periods p ON p.id = c.id_period"
		L" GROUP BY p.id_country");
}
//---------------------------------------------------------------------------
std::map<int, int> TLookupRepository::PeriodUsage(int ACountryId)
{
	return UsageMap(FDatabase,
		L"SELECT c.id_period, COUNT(*) FROM coin_items i"
		L"  JOIN coins c ON c.id = i.id_coin JOIN periods p ON p.id = c.id_period"
		L" WHERE p.id_country = :id_country GROUP BY c.id_period", ACountryId, true);
}
//---------------------------------------------------------------------------
std::map<int, int> TLookupRepository::CurrencyUsage()
{
	return UsageMap(FDatabase,
		L"SELECT c.id_currency, COUNT(*) FROM coin_items i JOIN coins c ON c.id = i.id_coin"
		L" WHERE c.id_currency IS NOT NULL GROUP BY c.id_currency");
}
//---------------------------------------------------------------------------
bool TLookupRepository::CountryExists(const String &AName, int AExceptId)
{
	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase, L"SELECT id FROM countries WHERE name = :name AND id <> :id");
	q->ParamByName(L"name")->AsWideString = AName.Trim();	// колонка COLLATE NOCASE
	q->ParamByName(L"id")->AsInteger      = AExceptId;
	return FirstId(q.get()) != 0;
}
//---------------------------------------------------------------------------
bool TLookupRepository::PeriodExists(int ACountryId, const String &AName, int AExceptId)
{
	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase,
		L"SELECT id FROM periods WHERE id_country = :id_country AND name = :name AND id <> :id");
	q->ParamByName(L"id_country")->AsInteger = ACountryId;
	q->ParamByName(L"name")->AsWideString    = AName.Trim();
	q->ParamByName(L"id")->AsInteger         = AExceptId;
	return FirstId(q.get()) != 0;
}
//---------------------------------------------------------------------------
bool TLookupRepository::CurrencyExists(const String &AName, int AExceptId)
{
	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase, L"SELECT id FROM currencies WHERE name = :name AND id <> :id");
	q->ParamByName(L"name")->AsWideString = AName.Trim();	// колонка COLLATE NOCASE
	q->ParamByName(L"id")->AsInteger      = AExceptId;
	return FirstId(q.get()) != 0;
}
//---------------------------------------------------------------------------
int TLookupRepository::AddCountry(const TCountry &ACountry)
{
	const String name = RequireName(ACountry.Name);
	if (CountryExists(name))
		throw Exception(L"Country already exists: " + name);

	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase,
		L"INSERT INTO countries (name, id_continent, is_exist) VALUES (:name, :id_continent, :is_exist)");
	q->ParamByName(L"name")->AsWideString = name;
	TFDParam *continent = q->ParamByName(L"id_continent");
	continent->DataType = ftInteger;
	if (ACountry.ContinentId == 0)
		continent->Clear();
	else
		continent->AsInteger = ACountry.ContinentId;
	q->ParamByName(L"is_exist")->AsInteger = ACountry.IsExist ? 1 : 0;
	q->ExecSQL();
	return LastInsertId(FDatabase);
}
//---------------------------------------------------------------------------
int TLookupRepository::AddPeriod(int ACountryId, const String &AName)
{
	const String name = RequireName(AName);
	if (PeriodExists(ACountryId, name))
		throw Exception(L"Period already exists: " + name);

	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase, L"INSERT INTO periods (id_country, name) VALUES (:id_country, :name)");
	q->ParamByName(L"id_country")->AsInteger = ACountryId;
	q->ParamByName(L"name")->AsWideString    = name;
	q->ExecSQL();
	return LastInsertId(FDatabase);
}
//---------------------------------------------------------------------------
int TLookupRepository::AddCurrency(const String &AName)
{
	const String name = RequireName(AName);
	if (CurrencyExists(name))
		throw Exception(L"Currency already exists: " + name);

	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase, L"INSERT INTO currencies (name) VALUES (:name)");
	q->ParamByName(L"name")->AsWideString = name;
	q->ExecSQL();
	return LastInsertId(FDatabase);
}
//---------------------------------------------------------------------------
void TLookupRepository::SaveCountry(const TCountry &ACountry)
{
	const String name = RequireName(ACountry.Name);
	if (CountryExists(name, ACountry.Id))
		throw Exception(L"Country already exists: " + name);

	TDbTransaction tx(FDatabase->Connection());
	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase, L"UPDATE countries SET name = :name WHERE id = :id");
	q->ParamByName(L"name")->AsWideString = name;
	q->ParamByName(L"id")->AsInteger      = ACountry.Id;
	q->ExecSQL();
	UpdateCountry(ACountry);
	SetCountryIssuer(ACountry.Id, ACountry.NumistaIssuer);
	tx.Commit();
}
//---------------------------------------------------------------------------
void TLookupRepository::RenamePeriod(int AId, const String &AName)
{
	const String name = RequireName(AName);
	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase, L"SELECT id_country FROM periods WHERE id = :id");
	q->ParamByName(L"id")->AsInteger = AId;
	const int countryId = FirstId(q.get());
	if (PeriodExists(countryId, name, AId))
		throw Exception(L"Period already exists: " + name);

	q->SQL->Text = L"UPDATE periods SET name = :name WHERE id = :id";
	q->ParamByName(L"name")->AsWideString = name;
	q->ParamByName(L"id")->AsInteger      = AId;
	q->ExecSQL();
}
//---------------------------------------------------------------------------
void TLookupRepository::RenameCurrency(int AId, const String &AName)
{
	const String name = RequireName(AName);
	if (CurrencyExists(name, AId))
		throw Exception(L"Currency already exists: " + name);

	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase, L"UPDATE currencies SET name = :name WHERE id = :id");
	q->ParamByName(L"name")->AsWideString = name;
	q->ParamByName(L"id")->AsInteger      = AId;
	q->ExecSQL();
}
//---------------------------------------------------------------------------
bool TLookupRepository::DeleteCountry(int AId)
{
	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase,
		L"SELECT c.id FROM coins c JOIN periods p ON p.id = c.id_period WHERE p.id_country = :id LIMIT 1");
	q->ParamByName(L"id")->AsInteger = AId;
	if (FirstId(q.get()) != 0)
		return false;

	TDbTransaction tx(FDatabase->Connection());
	q->SQL->Text = L"DELETE FROM periods WHERE id_country = :id";
	q->ParamByName(L"id")->AsInteger = AId;
	q->ExecSQL();
	q->SQL->Text = L"DELETE FROM countries WHERE id = :id";
	q->ParamByName(L"id")->AsInteger = AId;
	q->ExecSQL();
	tx.Commit();
	return true;
}
//---------------------------------------------------------------------------
bool TLookupRepository::DeletePeriod(int AId)
{
	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase, L"SELECT id FROM coins WHERE id_period = :id LIMIT 1");
	q->ParamByName(L"id")->AsInteger = AId;
	if (FirstId(q.get()) != 0)
		return false;

	q->SQL->Text = L"DELETE FROM periods WHERE id = :id";
	q->ParamByName(L"id")->AsInteger = AId;
	q->ExecSQL();
	return true;
}
//---------------------------------------------------------------------------
bool TLookupRepository::DeleteCurrency(int AId)
{
	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase, L"SELECT id FROM coins WHERE id_currency = :id LIMIT 1");
	q->ParamByName(L"id")->AsInteger = AId;
	if (FirstId(q.get()) != 0)
		return false;

	q->SQL->Text = L"DELETE FROM currencies WHERE id = :id";
	q->ParamByName(L"id")->AsInteger = AId;
	q->ExecSQL();
	return true;
}
//---------------------------------------------------------------------------
// Код эмитента Numista у страны ("" — не указан).
void TLookupRepository::SetCountryIssuer(int ACountryId, const String &AIssuer)
{
	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase, L"UPDATE countries SET numista_issuer = :issuer WHERE id = :id");
	TFDParam *issuer = q->ParamByName(L"issuer");
	issuer->DataType = ftWideString;
	if (AIssuer.Trim().IsEmpty())
		issuer->Clear();
	else
		issuer->AsWideString = AIssuer.Trim();
	q->ParamByName(L"id")->AsInteger = ACountryId;
	q->ExecSQL();
}
//---------------------------------------------------------------------------
