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
		L"SELECT id, name, id_continent, is_exist FROM countries ORDER BY name");
	for (q->Open(); !q->Eof; q->Next())
	{
		TCountry r;
		r.Id          = q->FieldByName(L"id")->AsInteger;
		r.Name        = q->FieldByName(L"name")->AsString;
		r.ContinentId = q->FieldByName(L"id_continent")->IsNull ? 0 : q->FieldByName(L"id_continent")->AsInteger;
		r.IsExist     = q->FieldByName(L"is_exist")->AsInteger != 0;
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
