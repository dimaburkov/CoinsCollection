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
		L"       i.grading_company, i.grading_number, i.grade, i.label_text, i.comment,"
		L"       c.numista_type_id, c.numista_issue_id, i.numista_item_id, c.numista_rarity,"
		L"       ni.id AS issue_loaded, ni.mintage, ni.comment AS issue_comment"
		L"  FROM coin_items i"
		L"  JOIN coins c       ON c.id  = i.id_coin"
		L"  JOIN periods p     ON p.id  = c.id_period"
		L"  JOIN countries co  ON co.id = p.id_country"
		L"  LEFT JOIN currencies cu ON cu.id = c.id_currency"
		L"  LEFT JOIN conditions cd ON cd.id = i.id_condition"
		L"  LEFT JOIN numista_issues ni ON ni.id = c.numista_issue_id"
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
		r.NumistaTypeId   = FieldInt  (D, L"numista_type_id");
		r.NumistaIssueId  = FieldInt  (D, L"numista_issue_id");
		r.NumistaItemId   = FieldInt  (D, L"numista_item_id");
		r.NumistaRarity   = FieldInt  (D, L"numista_rarity", -1);
		r.NumistaIssueLoaded  = !D->FieldByName(L"issue_loaded")->IsNull;
		TField *mintage = D->FieldByName(L"mintage");
		r.NumistaMintage      = mintage->IsNull ? -1 : mintage->AsLargeInt;
		r.NumistaIssueComment = FieldText(D, L"issue_comment");
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
	if (r.CoinId != 0)
		WriteCoin(periodId, currencyId, r);	// правка монеты — обновить её поля
	else if ((r.CoinId = FindCoin(periodId, currencyId, r)) != 0)
	{
		// Ещё один экземпляр уже известной монеты: её поля не перезаписываются,
		// только пустой диаметр дополняется введённым значением.
		if (r.DiameterMm != 0.0)
		{
			std::unique_ptr<TFDQuery> q = NewQuery(FDatabase,
				L"UPDATE coins SET diameter_mm = :diameter_mm WHERE id = :id AND diameter_mm IS NULL");
			q->ParamByName(L"diameter_mm")->AsFloat = r.DiameterMm;
			q->ParamByName(L"id")->AsInteger        = r.CoinId;
			q->ExecSQL();
		}
	}
	else
		WriteCoin(periodId, currencyId, r);	// новая монета
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
namespace
{
	// Связи Numista монеты до импорта (задача 13b). Значения — как в БД (NULL — Null()).
	struct TOldNumistaItem
	{
		int     NumistaItemId;
		Variant Condition, Quantity, MyValue, PurchaseDate, Comment, Swap;
	};
	struct TOldNumistaCoin
	{
		Variant TypeId, IssueId, Rarity;
		std::vector<TOldNumistaItem> Items;
	};
	//-----------------------------------------------------------------------
	void SetVariant(TFDParam *AParam, TFieldType AType, const Variant &AValue)
	{
		AParam->DataType = AType;
		if (AValue.IsNull())
			AParam->Clear();
		else
			AParam->Value = AValue;
	}
	//-----------------------------------------------------------------------
	// Монеты с N#, индексом или выгруженными экземплярами — по ключу монеты
	// импорта (ReplaceAll): период, валюта, номинал, год, разновидность, сюжет, номер.
	std::map<String, TOldNumistaCoin> ReadNumistaLinks(TDatabase *ADatabase, const String &ASep)
	{
		std::map<String, TOldNumistaCoin> result;
		std::map<int, String> keys;	// id монеты -> ключ
		std::unique_ptr<TFDQuery> q = NewQuery(ADatabase,
			L"SELECT id, id_period, id_currency, denomination, year, variety, subject, catalog_number,"
			L"       numista_type_id, numista_issue_id, numista_rarity"
			L"  FROM coins c"
			L" WHERE numista_type_id IS NOT NULL OR numista_rarity IS NOT NULL"
			L"    OR EXISTS (SELECT 1 FROM coin_items i WHERE i.id_coin = c.id AND i.numista_item_id IS NOT NULL)");
		for (q->Open(); !q->Eof; q->Next())
		{
			const String key = IntToStr(FieldInt(q.get(), L"id_period")) + ASep + IntToStr(FieldInt(q.get(), L"id_currency"))
				+ ASep + FieldText(q.get(), L"denomination") + ASep + IntToStr(FieldInt(q.get(), L"year"))
				+ ASep + FieldText(q.get(), L"variety") + ASep + FieldText(q.get(), L"subject")
				+ ASep + FieldText(q.get(), L"catalog_number");
			TOldNumistaCoin &coin = result[key];
			coin.TypeId  = q->FieldByName(L"numista_type_id")->Value;
			coin.IssueId = q->FieldByName(L"numista_issue_id")->Value;
			coin.Rarity  = q->FieldByName(L"numista_rarity")->Value;
			keys[q->FieldByName(L"id")->AsInteger] = key;
		}
		q->Close();

		q->SQL->Text = L"SELECT id_coin, numista_item_id, id_condition, quantity, my_value_uah, purchase_date,"
		               L"       comment, swap_info"
		               L"  FROM coin_items WHERE numista_item_id IS NOT NULL ORDER BY id";
		for (q->Open(); !q->Eof; q->Next())
		{
			TOldNumistaItem item;
			item.NumistaItemId = q->FieldByName(L"numista_item_id")->AsInteger;
			item.Condition     = q->FieldByName(L"id_condition")->Value;
			item.Quantity      = q->FieldByName(L"quantity")->Value;
			item.MyValue       = q->FieldByName(L"my_value_uah")->Value;
			item.PurchaseDate  = q->FieldByName(L"purchase_date")->Value;
			item.Comment       = q->FieldByName(L"comment")->Value;
			item.Swap          = q->FieldByName(L"swap_info")->Value;
			result[keys[q->FieldByName(L"id_coin")->AsInteger]].Items.push_back(item);
		}
		return result;
	}
	//-----------------------------------------------------------------------
	// Перенести связи на новые монеты (ANewCoins: ключ -> id). Экземпляр — с теми же
	// полями; нет такого — единственный экземпляр монеты, если и выгружен был один.
	void RestoreNumistaLinks(TDatabase *ADatabase, const std::map<String, TOldNumistaCoin> &AOldCoins,
		const std::map<String, int> &ANewCoins, TNumistaKept &AKept)
	{
		std::unique_ptr<TFDQuery> updateCoin = NewQuery(ADatabase,
			L"UPDATE coins SET numista_type_id = :type_id, numista_issue_id = :issue_id,"
			L"       numista_rarity = :rarity WHERE id = :id");
		std::unique_ptr<TFDQuery> findItem = NewQuery(ADatabase,
			L"SELECT id FROM coin_items"
			L" WHERE id_coin = :id_coin AND numista_item_id IS NULL"
			L"   AND id_condition IS :id_condition AND quantity IS :quantity AND my_value_uah IS :my_value_uah"
			L"   AND purchase_date IS :purchase_date AND comment IS :comment AND swap_info IS :swap_info");
		std::unique_ptr<TFDQuery> onlyItem = NewQuery(ADatabase,
			L"SELECT MIN(id), COUNT(*) FROM coin_items WHERE id_coin = :id_coin");
		std::unique_ptr<TFDQuery> updateItem = NewQuery(ADatabase,
			L"UPDATE coin_items SET numista_item_id = :numista_item_id WHERE id = :id");

		for (std::map<String, TOldNumistaCoin>::const_iterator old = AOldCoins.begin(); old != AOldCoins.end(); ++old)
		{
			const TOldNumistaCoin &coin = old->second;
			std::map<String, int>::const_iterator found = ANewCoins.find(old->first);
			if (found == ANewCoins.end())
			{
				++AKept.CoinsLost;
				AKept.ItemsLost += (int)coin.Items.size();
				continue;
			}
			const int coinId = found->second;

			SetVariant(updateCoin->ParamByName(L"type_id"),  ftInteger, coin.TypeId);
			SetVariant(updateCoin->ParamByName(L"issue_id"), ftInteger, coin.IssueId);
			SetVariant(updateCoin->ParamByName(L"rarity"),   ftInteger, coin.Rarity);
			updateCoin->ParamByName(L"id")->AsInteger = coinId;
			updateCoin->ExecSQL();
			++AKept.Coins;

			for (size_t i = 0; i < coin.Items.size(); ++i)
			{
				const TOldNumistaItem &item = coin.Items[i];
				int itemId = 0;

				findItem->ParamByName(L"id_coin")->AsInteger = coinId;
				SetVariant(findItem->ParamByName(L"id_condition"),  ftInteger,    item.Condition);
				SetVariant(findItem->ParamByName(L"quantity"),      ftInteger,    item.Quantity);
				SetVariant(findItem->ParamByName(L"my_value_uah"),  ftFloat,      item.MyValue);
				SetVariant(findItem->ParamByName(L"purchase_date"), ftWideString, item.PurchaseDate);
				SetVariant(findItem->ParamByName(L"comment"),       ftWideString, item.Comment);
				SetVariant(findItem->ParamByName(L"swap_info"),     ftWideString, item.Swap);
				findItem->Open();
				if (!findItem->Eof)
				{
					itemId = findItem->Fields->Fields[0]->AsInteger;
					findItem->Next();
					if (!findItem->Eof)
						itemId = 0;	// несколько одинаковых — неоднозначно
				}
				findItem->Close();

				if (itemId == 0 && coin.Items.size() == 1)
				{
					onlyItem->ParamByName(L"id_coin")->AsInteger = coinId;
					onlyItem->Open();
					if (onlyItem->Fields->Fields[1]->AsInteger == 1)
						itemId = onlyItem->Fields->Fields[0]->AsInteger;
					onlyItem->Close();
				}

				if (itemId == 0)
				{
					++AKept.ItemsLost;
					continue;
				}
				updateItem->ParamByName(L"numista_item_id")->AsInteger = item.NumistaItemId;
				updateItem->ParamByName(L"id")->AsInteger = itemId;
				updateItem->ExecSQL();
				++AKept.Items;
			}
		}
	}
}
//---------------------------------------------------------------------------
int TCoinRepository::ReplaceAll(const std::vector<TCoinRecord> &ARecords, TNumistaKept *AKept)
{
	// Пакетная запись: справочники и монеты этого импорта кешируются в памяти,
	// два INSERT создаются один раз (FireDAC готовит их при первом выполнении).
	// Правила те же, что у Save: пустые страна / номинал и неизвестное состояние —
	// исключение и откат всего импорта.
	TDbTransaction tx(FDatabase->Connection());

	const String sep = String(L'\x1F');	// разделитель частей ключа
	std::map<String, TOldNumistaCoin> oldCoins = ReadNumistaLinks(FDatabase, sep);

	FDatabase->Connection()->ExecSQL(L"DELETE FROM coin_items");
	FDatabase->Connection()->ExecSQL(L"DELETE FROM coins");

	std::map<String, int> countries, periods, currencies, conditions, coins;
	const std::vector<TCondition> scale = FLookups.Conditions();
	for (size_t i = 0; i < scale.size(); ++i)
		conditions[scale[i].Code.UpperCase()] = scale[i].Id;

	std::unique_ptr<TFDQuery> insertCoin = NewQuery(FDatabase, cInsertCoin);
	std::unique_ptr<TFDQuery> insertItem = NewQuery(FDatabase, cInsertItem);

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

	TNumistaKept kept;
	RestoreNumistaLinks(FDatabase, oldCoins, coins, kept);

	tx.Commit();
	if (AKept)
		*AKept = kept;
	return (int)ARecords.size();
}
//---------------------------------------------------------------------------
int TCoinRepository::ItemCount()
{
	return FDatabase->Connection()->ExecSQLScalar(L"SELECT COUNT(*) FROM coin_items");
}
//---------------------------------------------------------------------------
int TCoinRepository::ItemCount(int ACoinId)
{
	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase, L"SELECT COUNT(*) FROM coin_items WHERE id_coin = :id_coin");
	q->ParamByName(L"id_coin")->AsInteger = ACoinId;
	q->Open();
	return q->Fields->Fields[0]->AsInteger;
}
//---------------------------------------------------------------------------
// Связь монеты с типом и выпуском Numista (0 — снять связь).
void TCoinRepository::SetCoinNumista(int ACoinId, int ATypeId, int AIssueId)
{
	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase,
		L"UPDATE coins SET numista_type_id = :type_id, numista_issue_id = :issue_id WHERE id = :id");
	SetIntOrNull(q->ParamByName(L"type_id"),  ATypeId);
	SetIntOrNull(q->ParamByName(L"issue_id"), AIssueId);
	q->ParamByName(L"id")->AsInteger = ACoinId;
	q->ExecSQL();
}
//---------------------------------------------------------------------------
void TCoinRepository::SetItemNumista(int AItemId, int ANumistaItemId)
{
	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase,
		L"UPDATE coin_items SET numista_item_id = :numista_item_id WHERE id = :id");
	SetIntOrNull(q->ParamByName(L"numista_item_id"), ANumistaItemId);
	q->ParamByName(L"id")->AsInteger = AItemId;
	q->ExecSQL();
}
//---------------------------------------------------------------------------
// Выпуски типа (ответ getIssues): вставка или обновление по id выпуска.
// Год — как на монете; недатированный — "min–max". Возвращает число выпусков.
int TCoinRepository::SaveNumistaIssues(int ATypeId, const std::vector<TNumistaIssue> &AIssues)
{
	TDbTransaction tx(FDatabase->Connection());
	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase,
		L"INSERT OR REPLACE INTO numista_issues (id, type_id, year, mint_letter, mintage, comment, updated_at)"
		L" VALUES (:id, :type_id, :year, :mint_letter, :mintage, :comment, :updated_at)");
	const String today = DateToDb(Date());
	for (size_t i = 0; i < AIssues.size(); ++i)
	{
		const TNumistaIssue &issue = AIssues[i];
		String year;
		if (issue.Year != 0)
			year = IntToStr(issue.Year);
		else if (issue.MinYear != 0)
			year = issue.MaxYear != 0 && issue.MaxYear != issue.MinYear
				? IntToStr(issue.MinYear) + L"\u2013" + IntToStr(issue.MaxYear) : IntToStr(issue.MinYear);

		q->ParamByName(L"id")->AsInteger      = issue.Id;
		q->ParamByName(L"type_id")->AsInteger = ATypeId;
		SetText(q->ParamByName(L"year"),        year);
		SetText(q->ParamByName(L"mint_letter"), issue.MintLetter);
		TFDParam *mintage = q->ParamByName(L"mintage");
		mintage->DataType = ftLargeint;
		const __int64 value = StrToInt64Def(issue.Mintage.Trim(), -1);
		if (value < 0)
			mintage->Clear();
		else
			mintage->AsLargeInt = value;
		SetText(q->ParamByName(L"comment"),     issue.Comment);
		q->ParamByName(L"updated_at")->AsWideString = today;
		q->ExecSQL();
	}
	tx.Commit();
	return (int)AIssues.size();
}
//---------------------------------------------------------------------------
// Индекс редкости монеты (-1 — очистить; 0 — допустимое значение).
void TCoinRepository::SetCoinRarity(int ACoinId, int ARarity)
{
	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase, L"UPDATE coins SET numista_rarity = :rarity WHERE id = :id");
	TFDParam *rarity = q->ParamByName(L"rarity");
	rarity->DataType = ftInteger;
	if (ARarity < 0)
		rarity->Clear();
	else
		rarity->AsInteger = ARarity;
	q->ParamByName(L"id")->AsInteger = ACoinId;
	q->ExecSQL();
}
//---------------------------------------------------------------------------
