//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <map>
#include <memory>
#include <FireDAC.Comp.Client.hpp>
#include "u_CoinRepository.h"
#include "u_Database.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------

namespace
{
	const wchar_t *const cSelectAll =
		L"SELECT i.id, i.id_coin, co.name AS country, p.name AS period, cu.name AS currency,"
		L"       c.denomination, c.year, c.variety, c.subject, c.diameter_mm, c.catalog_number,"
		L"       cd.code AS condition, i.quantity, i.need_to_replace, i.catalog_value_uah,"
		L"       i.my_value_uah, i.purchase_date, i.published_date, i.swap_info, i.color,"
		L"       i.grading_company, i.grading_number, i.grade, i.label_text, i.comment"
		L"  FROM coin_items i"
		L"  JOIN coins c       ON c.id  = i.id_coin"
		L"  JOIN periods p     ON p.id  = c.id_period"
		L"  JOIN countries co  ON co.id = p.id_country"
		L"  LEFT JOIN currencies cu ON cu.id = c.id_currency"
		L"  LEFT JOIN conditions cd ON cd.id = i.id_condition"
		L" ORDER BY co.name, c.denomination, c.year, i.id";
	//-----------------------------------------------------------------------
	std::unique_ptr<TFDQuery> NewQuery(TDatabase *ADatabase, const String &ASql)
	{
		std::unique_ptr<TFDQuery> q(new TFDQuery(nullptr));
		q->Connection = ADatabase->Connection();
		q->SQL->Text = ASql;
		return q;
	}
	//-----------------------------------------------------------------------
	// TDateTime <-> TEXT 'yyyy-mm-dd'; 0 <-> NULL.
	String DateToDb(TDateTime AValue)
	{
		return (double)AValue == 0.0 ? String() : FormatDateTime(L"yyyy-mm-dd", AValue);
	}
	//-----------------------------------------------------------------------
	TDateTime DateFromDb(const String &AText)
	{
		const String s = AText.Trim();
		TDateTime result = 0.0;
		if (s.Length() >= 10)
		{
			const int y = StrToIntDef(s.SubString(1, 4), 0);
			const int m = StrToIntDef(s.SubString(6, 2), 0);
			const int d = StrToIntDef(s.SubString(9, 2), 0);
			if (!TryEncodeDate(y, m, d, result))
				result = 0.0;
		}
		return result;
	}
	//-----------------------------------------------------------------------
	// Пустая строка -> NULL.
	void SetText(TFDParam *AParam, const String &AValue)
	{
		AParam->DataType = ftWideString;
		const String v = AValue.Trim();
		if (v.IsEmpty())
			AParam->Clear();
		else
			AParam->AsWideString = v;
	}
	//-----------------------------------------------------------------------
	// 0 означает «нет данных» / «нет ссылки» -> NULL.
	void SetIntOrNull(TFDParam *AParam, int AValue)
	{
		AParam->DataType = ftInteger;
		if (AValue == 0)
			AParam->Clear();
		else
			AParam->AsInteger = AValue;
	}
	//-----------------------------------------------------------------------
	void SetFloatOrNull(TFDParam *AParam, double AValue)
	{
		AParam->DataType = ftFloat;
		if (AValue == 0.0)
			AParam->Clear();
		else
			AParam->AsFloat = AValue;
	}
	//-----------------------------------------------------------------------
	String FieldText(TDataSet *D, const String &AName)
	{
		TField *f = D->FieldByName(AName);
		return f->IsNull ? String() : f->AsString;
	}
	//-----------------------------------------------------------------------
	int FieldInt(TDataSet *D, const String &AName, int ADefault = 0)
	{
		TField *f = D->FieldByName(AName);
		return f->IsNull ? ADefault : f->AsInteger;
	}
	//-----------------------------------------------------------------------
	double FieldFloat(TDataSet *D, const String &AName)
	{
		TField *f = D->FieldByName(AName);
		return f->IsNull ? 0.0 : f->AsFloat;
	}
	//-----------------------------------------------------------------------
	TCoinRecord ReadRecord(TDataSet *D)
	{
		TCoinRecord r;
		r.Id              = FieldInt  (D, L"id");
		r.CoinId          = FieldInt  (D, L"id_coin");
		r.Country         = FieldText (D, L"country");
		r.Period          = FieldText (D, L"period");
		r.Currency        = FieldText (D, L"currency");
		r.Denomination    = FieldText (D, L"denomination");
		r.Year            = FieldInt  (D, L"year");
		r.Variety         = FieldText (D, L"variety");
		r.Subject         = FieldText (D, L"subject");
		r.DiameterMm      = FieldFloat(D, L"diameter_mm");
		r.CatalogNumber   = FieldText (D, L"catalog_number");
		r.Condition       = FieldText (D, L"condition");
		r.Quantity        = FieldInt  (D, L"quantity", 1);
		r.NeedToReplace   = FieldInt  (D, L"need_to_replace") != 0;
		r.CatalogValueUah = FieldFloat(D, L"catalog_value_uah");
		r.MyValueUah      = FieldFloat(D, L"my_value_uah");
		r.PurchaseDate    = DateFromDb(FieldText(D, L"purchase_date"));
		r.PublishedDate   = DateFromDb(FieldText(D, L"published_date"));
		r.SwapInfo        = FieldText (D, L"swap_info");
		r.Color           = FieldText (D, L"color");
		r.GradingCompany  = FieldText (D, L"grading_company");
		r.GradingNumber   = FieldText (D, L"grading_number");
		r.Grade           = FieldText (D, L"grade");
		r.LabelText       = FieldText (D, L"label_text");
		r.Comment         = FieldText (D, L"comment");
		return r;
	}
}
//---------------------------------------------------------------------------
TCoinRepository::TCoinRepository(TDatabase *ADatabase)
	: FDatabase(ADatabase),
	  FLookups(ADatabase)
{
}
//---------------------------------------------------------------------------
TCoinRepository::~TCoinRepository()
{
}
//---------------------------------------------------------------------------
std::vector<TCoinRecord> TCoinRepository::LoadAll()
{
	std::vector<TCoinRecord> result;
	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase, cSelectAll);
	for (q->Open(); !q->Eof; q->Next())
		result.push_back(ReadRecord(q.get()));
	return result;
}
//---------------------------------------------------------------------------
int TCoinRepository::Save(TCoinRecord &ARecord)
{
	if (ARecord.Denomination.Trim().IsEmpty())
		throw Exception(L"Denomination is required.");

	TDbTransaction tx(FDatabase->Connection());

	const int countryId   = FLookups.FindOrCreateCountry(ARecord.Country);
	const int periodId    = FLookups.FindOrCreatePeriod(countryId, ARecord.Period);
	const int currencyId  = FLookups.FindOrCreateCurrency(ARecord.Currency);
	const int conditionId = FLookups.FindCondition(ARecord.Condition);

	TCoinRecord r = ARecord;	// ARecord меняем только после успешного Commit
	if (r.CoinId == 0)
		r.CoinId = FindCoin(periodId, currencyId, r);
	WriteCoin(periodId, currencyId, r);
	WriteItem(conditionId, r);

	tx.Commit();
	ARecord.CoinId = r.CoinId;
	ARecord.Id     = r.Id;
	return ARecord.Id;
}
//---------------------------------------------------------------------------
// Такая же монета: тот же период, валюта, номинал, год, разновидность, сюжет
// и номер по каталогу (разный KM# — разные монеты).
// Сравнение NULL-безопасное (IS), пустые строки хранятся как NULL.
int TCoinRepository::FindCoin(int APeriodId, int ACurrencyId, const TCoinRecord &ARecord)
{
	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase,
		L"SELECT id FROM coins"
		L" WHERE id_period = :id_period AND id_currency IS :id_currency"
		L"   AND denomination = :denomination AND year IS :year"
		L"   AND variety IS :variety AND subject IS :subject"
		L"   AND catalog_number IS :catalog_number"
		L" ORDER BY id LIMIT 1");
	q->ParamByName(L"id_period")->AsInteger = APeriodId;
	SetIntOrNull(q->ParamByName(L"id_currency"),  ACurrencyId);
	SetText     (q->ParamByName(L"denomination"), ARecord.Denomination);
	SetIntOrNull(q->ParamByName(L"year"),         ARecord.Year);
	SetText     (q->ParamByName(L"variety"),      ARecord.Variety);
	SetText     (q->ParamByName(L"subject"),      ARecord.Subject);
	SetText     (q->ParamByName(L"catalog_number"), ARecord.CatalogNumber);
	q->Open();
	return q->Eof ? 0 : q->Fields->Fields[0]->AsInteger;
}
//---------------------------------------------------------------------------
namespace
{
	const wchar_t *const cInsertCoin =
		L"INSERT INTO coins (id_period, id_currency, denomination, year, variety, subject,"
		L"                   diameter_mm, catalog_number)"
		L" VALUES (:id_period, :id_currency, :denomination, :year, :variety, :subject,"
		L"         :diameter_mm, :catalog_number)";
	const wchar_t *const cUpdateCoin =
		L"UPDATE coins SET id_period = :id_period, id_currency = :id_currency,"
		L"       denomination = :denomination, year = :year, variety = :variety,"
		L"       subject = :subject, diameter_mm = :diameter_mm, catalog_number = :catalog_number"
		L" WHERE id = :id";
	const wchar_t *const cInsertItem =
		L"INSERT INTO coin_items (id_coin, id_condition, quantity, need_to_replace,"
		L"       catalog_value_uah, my_value_uah, purchase_date, published_date, swap_info,"
		L"       color, grading_company, grading_number, grade, label_text, comment)"
		L" VALUES (:id_coin, :id_condition, :quantity, :need_to_replace,"
		L"       :catalog_value_uah, :my_value_uah, :purchase_date, :published_date, :swap_info,"
		L"       :color, :grading_company, :grading_number, :grade, :label_text, :comment)";
	const wchar_t *const cUpdateItem =
		L"UPDATE coin_items SET id_coin = :id_coin, id_condition = :id_condition,"
		L"       quantity = :quantity, need_to_replace = :need_to_replace,"
		L"       catalog_value_uah = :catalog_value_uah, my_value_uah = :my_value_uah,"
		L"       purchase_date = :purchase_date, published_date = :published_date,"
		L"       swap_info = :swap_info, color = :color, grading_company = :grading_company,"
		L"       grading_number = :grading_number, grade = :grade, label_text = :label_text,"
		L"       comment = :comment"
		L" WHERE id = :id";
	//-----------------------------------------------------------------------
	void BindCoin(TFDQuery *Q, int APeriodId, int ACurrencyId, const TCoinRecord &R)
	{
		Q->ParamByName(L"id_period")->AsInteger = APeriodId;
		SetIntOrNull  (Q->ParamByName(L"id_currency"),    ACurrencyId);
		SetText       (Q->ParamByName(L"denomination"),   R.Denomination);
		SetIntOrNull  (Q->ParamByName(L"year"),           R.Year);
		SetText       (Q->ParamByName(L"variety"),        R.Variety);
		SetText       (Q->ParamByName(L"subject"),        R.Subject);
		SetFloatOrNull(Q->ParamByName(L"diameter_mm"),    R.DiameterMm);
		SetText       (Q->ParamByName(L"catalog_number"), R.CatalogNumber);
	}
	//-----------------------------------------------------------------------
	void BindItem(TFDQuery *Q, int ACoinId, int AConditionId, const TCoinRecord &R)
	{
		Q->ParamByName(L"id_coin")->AsInteger         = ACoinId;
		SetIntOrNull  (Q->ParamByName(L"id_condition"),      AConditionId);
		Q->ParamByName(L"quantity")->AsInteger        = R.Quantity;
		Q->ParamByName(L"need_to_replace")->AsInteger = R.NeedToReplace ? 1 : 0;
		SetFloatOrNull(Q->ParamByName(L"catalog_value_uah"), R.CatalogValueUah);
		SetFloatOrNull(Q->ParamByName(L"my_value_uah"),      R.MyValueUah);
		SetText       (Q->ParamByName(L"purchase_date"),     DateToDb(R.PurchaseDate));
		SetText       (Q->ParamByName(L"published_date"),    DateToDb(R.PublishedDate));
		SetText       (Q->ParamByName(L"swap_info"),         R.SwapInfo);
		SetText       (Q->ParamByName(L"color"),             R.Color);
		SetText       (Q->ParamByName(L"grading_company"),   R.GradingCompany);
		SetText       (Q->ParamByName(L"grading_number"),    R.GradingNumber);
		SetText       (Q->ParamByName(L"grade"),             R.Grade);
		SetText       (Q->ParamByName(L"label_text"),        R.LabelText);
		SetText       (Q->ParamByName(L"comment"),           R.Comment);
	}
	//-----------------------------------------------------------------------
	int LastInsertId(TDatabase *ADatabase)
	{
		return ADatabase->Connection()->ExecSQLScalar(L"SELECT last_insert_rowid()");
	}
}
//---------------------------------------------------------------------------
void TCoinRepository::WriteCoin(int APeriodId, int ACurrencyId, TCoinRecord &ARecord)
{
	const bool isNew = ARecord.CoinId == 0;
	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase, isNew ? cInsertCoin : cUpdateCoin);
	BindCoin(q.get(), APeriodId, ACurrencyId, ARecord);
	if (!isNew)
		q->ParamByName(L"id")->AsInteger = ARecord.CoinId;
	q->ExecSQL();

	if (isNew)
		ARecord.CoinId = LastInsertId(FDatabase);
	else if (q->RowsAffected == 0)
		throw Exception(L"Coin not found: id = " + IntToStr(ARecord.CoinId));
}
//---------------------------------------------------------------------------
void TCoinRepository::WriteItem(int AConditionId, TCoinRecord &ARecord)
{
	const bool isNew = ARecord.Id == 0;
	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase, isNew ? cInsertItem : cUpdateItem);
	BindItem(q.get(), ARecord.CoinId, AConditionId, ARecord);
	if (!isNew)
		q->ParamByName(L"id")->AsInteger = ARecord.Id;
	q->ExecSQL();

	if (isNew)
		ARecord.Id = LastInsertId(FDatabase);
	else if (q->RowsAffected == 0)
		throw Exception(L"Coin item not found: id = " + IntToStr(ARecord.Id));
}
//---------------------------------------------------------------------------
bool TCoinRepository::Delete(int AId)
{
	TDbTransaction tx(FDatabase->Connection());

	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase, L"SELECT id_coin FROM coin_items WHERE id = :id");
	q->ParamByName(L"id")->AsInteger = AId;
	q->Open();
	if (q->Eof)
		return false;
	const int coinId = q->Fields->Fields[0]->AsInteger;
	q->Close();

	q->SQL->Text = L"DELETE FROM coin_items WHERE id = :id";
	q->ParamByName(L"id")->AsInteger = AId;
	q->ExecSQL();

	q->SQL->Text = L"DELETE FROM coins WHERE id = :id"
	               L" AND NOT EXISTS (SELECT 1 FROM coin_items WHERE id_coin = :id)";
	q->ParamByName(L"id")->AsInteger = coinId;
	q->ExecSQL();

	tx.Commit();
	return true;
}
//---------------------------------------------------------------------------
int TCoinRepository::ReplaceAll(const std::vector<TCoinRecord> &ARecords)
{
	// Пакетная запись: справочники и монеты этого импорта кешируются в памяти,
	// два INSERT создаются один раз (FireDAC готовит их при первом выполнении).
	// Правила те же, что у Save: пустые страна / номинал и неизвестное состояние —
	// исключение и откат всего импорта.
	TDbTransaction tx(FDatabase->Connection());

	FDatabase->Connection()->ExecSQL(L"DELETE FROM coin_items");
	FDatabase->Connection()->ExecSQL(L"DELETE FROM coins");

	std::map<String, int> countries, periods, currencies, conditions, coins;
	const std::vector<TCondition> scale = FLookups.Conditions();
	for (size_t i = 0; i < scale.size(); ++i)
		conditions[scale[i].Code.UpperCase()] = scale[i].Id;

	std::unique_ptr<TFDQuery> insertCoin = NewQuery(FDatabase, cInsertCoin);
	std::unique_ptr<TFDQuery> insertItem = NewQuery(FDatabase, cInsertItem);

	const String sep = String(L'\x1F');	// разделитель частей ключа
	for (size_t i = 0; i < ARecords.size(); ++i)
	{
		const TCoinRecord &r = ARecords[i];
		if (r.Denomination.Trim().IsEmpty())
			throw Exception(L"Denomination is required.");

		const String countryKey = r.Country.Trim().UpperCase();
		std::map<String, int>::iterator country = countries.find(countryKey);
		if (country == countries.end())
			country = countries.insert(std::make_pair(countryKey, FLookups.FindOrCreateCountry(r.Country))).first;

		const String periodKey = IntToStr(country->second) + sep + r.Period.Trim();
		std::map<String, int>::iterator period = periods.find(periodKey);
		if (period == periods.end())
			period = periods.insert(std::make_pair(periodKey, FLookups.FindOrCreatePeriod(country->second, r.Period))).first;

		const String currencyKey = r.Currency.Trim().UpperCase();
		std::map<String, int>::iterator currency = currencies.find(currencyKey);
		if (currency == currencies.end())
			currency = currencies.insert(std::make_pair(currencyKey, FLookups.FindOrCreateCurrency(r.Currency))).first;

		int conditionId = 0;
		const String conditionKey = r.Condition.Trim().UpperCase();
		if (!conditionKey.IsEmpty())
		{
			std::map<String, int>::const_iterator condition = conditions.find(conditionKey);
			if (condition == conditions.end())
				throw Exception(L"Unknown condition code: " + r.Condition);
			conditionId = condition->second;
		}

		// Та же монета, что и в FindCoin: период, валюта, номинал, год, разновидность,
		// сюжет, номер по каталогу.
		const String coinKey = IntToStr(period->second) + sep + IntToStr(currency->second) + sep
			+ r.Denomination.Trim() + sep + IntToStr(r.Year) + sep + r.Variety.Trim() + sep + r.Subject.Trim()
			+ sep + r.CatalogNumber.Trim();
		std::map<String, int>::iterator coin = coins.find(coinKey);
		if (coin == coins.end())
		{
			BindCoin(insertCoin.get(), period->second, currency->second, r);
			insertCoin->ExecSQL();
			coin = coins.insert(std::make_pair(coinKey, LastInsertId(FDatabase))).first;
		}

		BindItem(insertItem.get(), coin->second, conditionId, r);
		insertItem->ExecSQL();
	}

	tx.Commit();
	return (int)ARecords.size();
}
//---------------------------------------------------------------------------
int TCoinRepository::ItemCount()
{
	return FDatabase->Connection()->ExecSQLScalar(L"SELECT COUNT(*) FROM coin_items");
}
//---------------------------------------------------------------------------
