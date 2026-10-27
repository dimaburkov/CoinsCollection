//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <map>
#include <memory>
#include <set>
#include <FireDAC.Comp.Client.hpp>
#include "u_CoinRepository.h"
#include "u_Database.h"
#include "u_PhotoRepository.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------

namespace
{
	const wchar_t *const cSelectAll =
		L"SELECT i.id, i.id_coin, co.name AS country, p.name AS period, cu.name AS currency,"
		L"       c.denomination, c.year, c.year_to, c.variety, c.subject, c.diameter_mm, c.catalog_number,"
		L"       cd.code AS condition, i.quantity, i.need_to_replace, i.catalog_value_uah,"
		L"       i.my_value_uah, i.purchase_date, i.published_date, i.swap_info, i.color,"
		L"       i.grading_company, i.grading_number, i.grade, i.label_text, i.comment, i.origin,"
		L"       c.numista_type_id, c.numista_issue_id, i.numista_item_id, c.numista_rarity, c.numista_query,"
		L"       ni.id AS issue_loaded, ni.mintage, ni.comment AS issue_comment,"
		L"       nt.id AS type_loaded, nt.composition,"
		L"       CASE WHEN c.numista_type_id IS NULL THEN 0"
		L"            WHEN nt.id IS NULL OR nt.pictures_known = 0 THEN 1"
		L"            WHEN nt.obverse_picture IS NOT NULL AND NOT EXISTS (SELECT 1 FROM coin_photos ph"
		L"                 WHERE ph.id_coin = c.id AND ph.side = 'obverse') THEN 1"
		L"            WHEN nt.reverse_picture IS NOT NULL AND NOT EXISTS (SELECT 1 FROM coin_photos ph"
		L"                 WHERE ph.id_coin = c.id AND ph.side = 'reverse') THEN 1"
		L"            ELSE 0 END AS photo_pending"
		L"  FROM coin_items i"
		L"  JOIN coins c       ON c.id  = i.id_coin"
		L"  JOIN periods p     ON p.id  = c.id_period"
		L"  JOIN countries co  ON co.id = p.id_country"
		L"  LEFT JOIN currencies cu ON cu.id = c.id_currency"
		L"  LEFT JOIN conditions cd ON cd.id = i.id_condition"
		L"  LEFT JOIN numista_issues ni ON ni.id = c.numista_issue_id"
		L"  LEFT JOIN numista_types nt  ON nt.id = c.numista_type_id"
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
		r.YearTo          = FieldInt  (D, L"year_to");
		r.Origin          = FieldText (D, L"origin");
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
		r.NumistaQuery    = FieldText (D, L"numista_query");
		r.NumistaIssueLoaded  = !D->FieldByName(L"issue_loaded")->IsNull;
		TField *mintage = D->FieldByName(L"mintage");
		r.NumistaMintage      = mintage->IsNull ? -1 : mintage->AsLargeInt;
		r.NumistaIssueComment = FieldText(D, L"issue_comment");
		r.NumistaTypeLoaded   = !D->FieldByName(L"type_loaded")->IsNull;
		r.NumistaComposition  = FieldText(D, L"composition");
		r.NumistaPhotoPending = FieldInt(D, L"photo_pending") != 0;
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
		L"   AND denomination = :denomination AND year IS :year AND year_to IS :year_to"
		L"   AND variety IS :variety AND subject IS :subject"
		L"   AND catalog_number IS :catalog_number"
		L" ORDER BY id LIMIT 1");
	q->ParamByName(L"id_period")->AsInteger = APeriodId;
	SetIntOrNull(q->ParamByName(L"id_currency"),  ACurrencyId);
	SetText     (q->ParamByName(L"denomination"), ARecord.Denomination);
	SetIntOrNull(q->ParamByName(L"year"),         ARecord.Year);
	SetIntOrNull(q->ParamByName(L"year_to"),      ARecord.YearTo == ARecord.Year ? 0 : ARecord.YearTo);
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
		L"INSERT INTO coins (id_period, id_currency, denomination, year, year_to, variety, subject,"
		L"                   diameter_mm, catalog_number, numista_query)"
		L" VALUES (:id_period, :id_currency, :denomination, :year, :year_to, :variety, :subject,"
		L"         :diameter_mm, :catalog_number, :numista_query)";
	const wchar_t *const cUpdateCoin =
		L"UPDATE coins SET id_period = :id_period, id_currency = :id_currency,"
		L"       denomination = :denomination, year = :year, year_to = :year_to, variety = :variety,"
		L"       subject = :subject, diameter_mm = :diameter_mm, catalog_number = :catalog_number,"
		L"       numista_query = :numista_query"
		L" WHERE id = :id";
	const wchar_t *const cInsertItem =
		L"INSERT INTO coin_items (id_coin, id_condition, quantity, need_to_replace,"
		L"       catalog_value_uah, my_value_uah, purchase_date, published_date, swap_info,"
		L"       color, grading_company, grading_number, grade, label_text, comment, origin)"
		L" VALUES (:id_coin, :id_condition, :quantity, :need_to_replace,"
		L"       :catalog_value_uah, :my_value_uah, :purchase_date, :published_date, :swap_info,"
		L"       :color, :grading_company, :grading_number, :grade, :label_text, :comment, :origin)";
	const wchar_t *const cUpdateItem =
		L"UPDATE coin_items SET id_coin = :id_coin, id_condition = :id_condition,"
		L"       quantity = :quantity, need_to_replace = :need_to_replace,"
		L"       catalog_value_uah = :catalog_value_uah, my_value_uah = :my_value_uah,"
		L"       purchase_date = :purchase_date, published_date = :published_date,"
		L"       swap_info = :swap_info, color = :color, grading_company = :grading_company,"
		L"       grading_number = :grading_number, grade = :grade, label_text = :label_text,"
		L"       comment = :comment, origin = :origin"
		L" WHERE id = :id";
	//-----------------------------------------------------------------------
	void BindCoin(TFDQuery *Q, int APeriodId, int ACurrencyId, const TCoinRecord &R)
	{
		Q->ParamByName(L"id_period")->AsInteger = APeriodId;
		SetIntOrNull  (Q->ParamByName(L"id_currency"),    ACurrencyId);
		SetText       (Q->ParamByName(L"denomination"),   R.Denomination);
		SetIntOrNull  (Q->ParamByName(L"year"),           R.Year);
		SetIntOrNull  (Q->ParamByName(L"year_to"),        R.YearTo == R.Year ? 0 : R.YearTo);
		SetText       (Q->ParamByName(L"variety"),        R.Variety);
		SetText       (Q->ParamByName(L"subject"),        R.Subject);
		SetFloatOrNull(Q->ParamByName(L"diameter_mm"),    R.DiameterMm);
		SetText       (Q->ParamByName(L"catalog_number"), R.CatalogNumber);
		SetText       (Q->ParamByName(L"numista_query"),  R.NumistaQuery);
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
		SetText       (Q->ParamByName(L"origin"),            R.Origin);
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

	// Файлы фото экземпляра и (если монета удаляется вместе с ним) фото типа;
	// строки coin_photos уходят каскадом, файлы удаляются после фиксации.
	std::vector<String> photos, coinPhotos;
	q->SQL->Text = L"SELECT id_item, file_name FROM coin_photos WHERE id_item = :id OR id_coin = :id_coin";
	q->ParamByName(L"id")->AsInteger      = AId;
	q->ParamByName(L"id_coin")->AsInteger = coinId;
	for (q->Open(); !q->Eof; q->Next())
		(q->Fields->Fields[0]->IsNull ? coinPhotos : photos).push_back(q->Fields->Fields[1]->AsString);
	q->Close();

	q->SQL->Text = L"DELETE FROM coin_items WHERE id = :id";
	q->ParamByName(L"id")->AsInteger = AId;
	q->ExecSQL();

	q->SQL->Text = L"DELETE FROM coins WHERE id = :id"
	               L" AND NOT EXISTS (SELECT 1 FROM coin_items WHERE id_coin = :id)";
	q->ParamByName(L"id")->AsInteger = coinId;
	q->ExecSQL();
	if (q->RowsAffected > 0)
		photos.insert(photos.end(), coinPhotos.begin(), coinPhotos.end());

	tx.Commit();
	TPhotoRepository::DeleteFiles(photos);
	return true;
}
//---------------------------------------------------------------------------
namespace
{
	// Связи монеты до импорта (задачи 13b, 14): Numista и фото. Значения — как в БД (NULL — Null()).
	struct TOldPhoto
	{
		Variant Side, FileName, Source, AddedAt, Copyright;
	};
	struct TOldItem
	{
		int     NumistaItemId;	// 0 — не выгружен (только фото)
		Variant Condition, Quantity, MyValue, PurchaseDate, Comment, Swap;
		std::vector<TOldPhoto> Photos;
	};
	struct TOldCoin
	{
		Variant TypeId, IssueId, Rarity, Query;
		std::vector<TOldPhoto> Photos;	// фото типа
		std::vector<TOldItem>  Items;	// выгруженные и / или с фото
		bool HasNumista() const { return !TypeId.IsNull() || !Rarity.IsNull() || !Query.IsNull(); }
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
	TOldPhoto ReadPhoto(TDataSet *D)
	{
		TOldPhoto photo;
		photo.Side     = D->FieldByName(L"side")->Value;
		photo.FileName = D->FieldByName(L"file_name")->Value;
		photo.Source   = D->FieldByName(L"source")->Value;
		photo.AddedAt  = D->FieldByName(L"added_at")->Value;
		photo.Copyright = D->FieldByName(L"copyright")->Value;
		return photo;
	}
	//-----------------------------------------------------------------------
	// Монеты с N#, индексом, выгруженными экземплярами или фото — по ключу монеты
	// импорта (ReplaceAll): период, валюта, номинал, год, разновидность, сюжет, номер.
	// Ключ монеты (как в ReplaceAll): период, валюта, номинал, годы, разновидность, сюжет, номер.
	String CoinKey(TDataSet *D, const String &ASep)
	{
		return IntToStr(FieldInt(D, L"id_period")) + ASep + IntToStr(FieldInt(D, L"id_currency"))
			+ ASep + FieldText(D, L"denomination") + ASep + IntToStr(FieldInt(D, L"year"))
			+ ASep + IntToStr(FieldInt(D, L"year_to")) + ASep + FieldText(D, L"variety")
			+ ASep + FieldText(D, L"subject") + ASep + FieldText(D, L"catalog_number");
	}
	//-----------------------------------------------------------------------
	std::map<String, TOldCoin> ReadLinks(TDatabase *ADatabase, const String &ASep)
	{
		std::map<String, TOldCoin> result;
		std::map<int, String> keys;	// id монеты -> ключ
		std::unique_ptr<TFDQuery> q = NewQuery(ADatabase,
			L"SELECT id, id_period, id_currency, denomination, year, year_to, variety, subject, catalog_number,"
			L"       numista_type_id, numista_issue_id, numista_rarity, numista_query"
			L"  FROM coins c"
			L" WHERE NOT EXISTS (SELECT 1 FROM coin_items k WHERE k.id_coin = c.id AND k.origin = 'numista')"
			L"   AND (numista_type_id IS NOT NULL OR numista_rarity IS NOT NULL OR numista_query IS NOT NULL"
			L"    OR EXISTS (SELECT 1 FROM coin_items i WHERE i.id_coin = c.id AND i.numista_item_id IS NOT NULL)"
			L"    OR EXISTS (SELECT 1 FROM coin_photos ph WHERE ph.id_coin = c.id)"
			L"    OR EXISTS (SELECT 1 FROM coin_items i JOIN coin_photos ph ON ph.id_item = i.id WHERE i.id_coin = c.id))");
		for (q->Open(); !q->Eof; q->Next())
		{
			const String key = CoinKey(q.get(), ASep);
			TOldCoin &coin = result[key];
			coin.TypeId  = q->FieldByName(L"numista_type_id")->Value;
			coin.IssueId = q->FieldByName(L"numista_issue_id")->Value;
			coin.Rarity  = q->FieldByName(L"numista_rarity")->Value;
			coin.Query   = q->FieldByName(L"numista_query")->Value;
			keys[q->FieldByName(L"id")->AsInteger] = key;
		}
		q->Close();

		std::map<int, std::pair<String, size_t> > itemPos;	// id экземпляра -> (ключ монеты, индекс в Items)
		q->SQL->Text = L"SELECT i.id, i.id_coin, i.numista_item_id, i.id_condition, i.quantity, i.my_value_uah,"
		               L"       i.purchase_date, i.comment, i.swap_info"
		               L"  FROM coin_items i"
		               L" WHERE i.origin IS NULL"
		               L"   AND (i.numista_item_id IS NOT NULL"
		               L"    OR EXISTS (SELECT 1 FROM coin_photos ph WHERE ph.id_item = i.id))"
		               L" ORDER BY i.id";
		for (q->Open(); !q->Eof; q->Next())
		{
			TOldItem item;
			item.NumistaItemId = FieldInt(q.get(), L"numista_item_id");
			item.Condition     = q->FieldByName(L"id_condition")->Value;
			item.Quantity      = q->FieldByName(L"quantity")->Value;
			item.MyValue       = q->FieldByName(L"my_value_uah")->Value;
			item.PurchaseDate  = q->FieldByName(L"purchase_date")->Value;
			item.Comment       = q->FieldByName(L"comment")->Value;
			item.Swap          = q->FieldByName(L"swap_info")->Value;
			const String key = keys[q->FieldByName(L"id_coin")->AsInteger];
			std::vector<TOldItem> &items = result[key].Items;
			itemPos[q->FieldByName(L"id")->AsInteger] = std::make_pair(key, items.size());
			items.push_back(item);
		}
		q->Close();

		q->SQL->Text = L"SELECT id_coin, id_item, side, file_name, source, added_at, copyright FROM coin_photos ORDER BY id";
		for (q->Open(); !q->Eof; q->Next())
		{
			if (!q->FieldByName(L"id_coin")->IsNull)
			{
				// Фото типа только тех монет, что импорт удаляет (монеты «из Numista» остаются).
				std::map<int, String>::const_iterator key = keys.find(q->FieldByName(L"id_coin")->AsInteger);
				if (key != keys.end())
					result[key->second].Photos.push_back(ReadPhoto(q.get()));
			}
			else
			{
				std::map<int, std::pair<String, size_t> >::const_iterator pos =
					itemPos.find(q->FieldByName(L"id_item")->AsInteger);
				if (pos != itemPos.end())
					result[pos->second.first].Items[pos->second.second].Photos.push_back(ReadPhoto(q.get()));
			}
		}
		return result;
	}
	//-----------------------------------------------------------------------
	// Фото на нового владельца (APhotoInsert — INSERT с :owner); возвращает число.
	int InsertPhotos(TFDQuery *APhotoInsert, int AOwnerId, const std::vector<TOldPhoto> &APhotos)
	{
		for (size_t i = 0; i < APhotos.size(); ++i)
		{
			APhotoInsert->ParamByName(L"owner")->AsInteger = AOwnerId;
			SetVariant(APhotoInsert->ParamByName(L"side"),      ftWideString, APhotos[i].Side);
			SetVariant(APhotoInsert->ParamByName(L"file_name"), ftWideString, APhotos[i].FileName);
			SetVariant(APhotoInsert->ParamByName(L"source"),    ftWideString, APhotos[i].Source);
			SetVariant(APhotoInsert->ParamByName(L"added_at"),  ftWideString, APhotos[i].AddedAt);
			SetVariant(APhotoInsert->ParamByName(L"copyright"), ftWideString, APhotos[i].Copyright);
			APhotoInsert->ExecSQL();
		}
		return (int)APhotos.size();
	}
	//-----------------------------------------------------------------------
	void AddLostPhotos(const std::vector<TOldPhoto> &APhotos, TImportKept &AKept)
	{
		for (size_t i = 0; i < APhotos.size(); ++i)
			AKept.LostPhotos.push_back(VarToStr(APhotos[i].FileName));
	}
	//-----------------------------------------------------------------------
	// Перенести связи и фото на новые монеты (ANewCoins: ключ -> id). Экземпляр — с теми
	// же полями; нет такого — единственный экземпляр монеты, если и старый был один.
	// Экземпляр получает не больше одного старого (одинаковые — неоднозначно).
	void RestoreLinks(TDatabase *ADatabase, const std::map<String, TOldCoin> &AOldCoins,
		const std::map<String, int> &ANewCoins, TImportKept &AKept)
	{
		const wchar_t *const photoValues = L" side, file_name, source, added_at, copyright)"
			L" VALUES (:owner, :side, :file_name, :source, :added_at, :copyright)";
		std::unique_ptr<TFDQuery> updateCoin = NewQuery(ADatabase,
			L"UPDATE coins SET numista_type_id = :type_id, numista_issue_id = :issue_id,"
			L"       numista_rarity = :rarity, numista_query = :query WHERE id = :id");
		std::unique_ptr<TFDQuery> findItem = NewQuery(ADatabase,
			L"SELECT id FROM coin_items"
			L" WHERE id_coin = :id_coin AND numista_item_id IS NULL"
			L"   AND id_condition IS :id_condition AND quantity IS :quantity AND my_value_uah IS :my_value_uah"
			L"   AND purchase_date IS :purchase_date AND comment IS :comment AND swap_info IS :swap_info");
		std::unique_ptr<TFDQuery> onlyItem = NewQuery(ADatabase,
			L"SELECT MIN(id), COUNT(*) FROM coin_items WHERE id_coin = :id_coin");
		std::unique_ptr<TFDQuery> updateItem = NewQuery(ADatabase,
			L"UPDATE coin_items SET numista_item_id = :numista_item_id WHERE id = :id");
		std::unique_ptr<TFDQuery> coinPhoto = NewQuery(ADatabase, String(L"INSERT INTO coin_photos (id_coin,") + photoValues);
		std::unique_ptr<TFDQuery> itemPhoto = NewQuery(ADatabase, String(L"INSERT INTO coin_photos (id_item,") + photoValues);
		std::set<int> usedItems;	// новые экземпляры, уже получившие старый

		for (std::map<String, TOldCoin>::const_iterator old = AOldCoins.begin(); old != AOldCoins.end(); ++old)
		{
			const TOldCoin &coin = old->second;
			std::map<String, int>::const_iterator found = ANewCoins.find(old->first);
			if (found == ANewCoins.end())
			{
				if (coin.HasNumista())
					++AKept.CoinsLost;
				AddLostPhotos(coin.Photos, AKept);
				for (size_t i = 0; i < coin.Items.size(); ++i)
				{
					if (coin.Items[i].NumistaItemId != 0)
						++AKept.ItemsLost;
					AddLostPhotos(coin.Items[i].Photos, AKept);
				}
				continue;
			}
			const int coinId = found->second;

			if (coin.HasNumista())
			{
				SetVariant(updateCoin->ParamByName(L"type_id"),  ftInteger, coin.TypeId);
				SetVariant(updateCoin->ParamByName(L"issue_id"), ftInteger, coin.IssueId);
				SetVariant(updateCoin->ParamByName(L"rarity"),   ftInteger, coin.Rarity);
				SetVariant(updateCoin->ParamByName(L"query"),    ftWideString, coin.Query);
				updateCoin->ParamByName(L"id")->AsInteger = coinId;
				updateCoin->ExecSQL();
				++AKept.Coins;
			}
			AKept.Photos += InsertPhotos(coinPhoto.get(), coinId, coin.Photos);

			for (size_t i = 0; i < coin.Items.size(); ++i)
			{
				const TOldItem &item = coin.Items[i];
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

				if (itemId == 0 || !usedItems.insert(itemId).second)
				{
					if (item.NumistaItemId != 0)
						++AKept.ItemsLost;
					AddLostPhotos(item.Photos, AKept);
					continue;
				}
				if (item.NumistaItemId != 0)
				{
					updateItem->ParamByName(L"numista_item_id")->AsInteger = item.NumistaItemId;
					updateItem->ParamByName(L"id")->AsInteger = itemId;
					updateItem->ExecSQL();
					++AKept.Items;
				}
				AKept.Photos += InsertPhotos(itemPhoto.get(), itemId, item.Photos);
			}
		}
	}
}
//---------------------------------------------------------------------------
int TCoinRepository::ReplaceAll(const std::vector<TCoinRecord> &ARecords, TImportKept *AKept)
{
	// Пакетная запись: справочники и монеты этого импорта кешируются в памяти,
	// два INSERT создаются один раз (FireDAC готовит их при первом выполнении).
	// Правила те же, что у Save: пустые страна / номинал и неизвестное состояние —
	// исключение и откат всего импорта.
	TDbTransaction tx(FDatabase->Connection());

	const String sep = String(L'\x1F');	// разделитель частей ключа
	std::map<String, TOldCoin> oldCoins = ReadLinks(FDatabase, sep);

	// Экземпляры, импортированные из Numista (задача 20), и их монеты не удаляются.
	FDatabase->Connection()->ExecSQL(L"DELETE FROM coin_items WHERE origin IS NULL");
	FDatabase->Connection()->ExecSQL(L"DELETE FROM coins WHERE NOT EXISTS (SELECT 1 FROM coin_items i WHERE i.id_coin = coins.id)");

	std::map<String, int> countries, periods, currencies, conditions, coins;
	{
		// Оставшиеся монеты: экземпляр из файла с тем же ключом пойдёт к ним, без дубля монеты.
		std::unique_ptr<TFDQuery> kept = NewQuery(FDatabase,
			L"SELECT id, id_period, id_currency, denomination, year, year_to, variety, subject, catalog_number FROM coins");
		for (kept->Open(); !kept->Eof; kept->Next())
			coins[CoinKey(kept.get(), sep)] = kept->FieldByName(L"id")->AsInteger;
	}
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
			+ r.Denomination.Trim() + sep + IntToStr(r.Year) + sep + IntToStr(r.YearTo == r.Year ? 0 : r.YearTo)
			+ sep + r.Variety.Trim() + sep + r.Subject.Trim()
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

	TImportKept kept;
	RestoreLinks(FDatabase, oldCoins, coins, kept);

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
// Характеристики типа (ответ getType): вставка или обновление по id.
void TCoinRepository::SaveNumistaType(const TNumistaTypeInfo &AType)
{
	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase,
		L"INSERT OR REPLACE INTO numista_types (id, title, object_type, min_year, max_year, composition,"
		L"       technique, weight, size, size2, thickness, shape, is_demonetized, demonetization_date,"
		L"       rulers, series, commemorated_topic, mints, updated_at,"
		L"       obverse_picture, reverse_picture, obverse_copyright, reverse_copyright, pictures_known)"
		L" VALUES (:id, :title, :object_type, :min_year, :max_year, :composition,"
		L"       :technique, :weight, :size, :size2, :thickness, :shape, :is_demonetized, :demonetization_date,"
		L"       :rulers, :series, :commemorated_topic, :mints, :updated_at,"
		L"       :obverse_picture, :reverse_picture, :obverse_copyright, :reverse_copyright, :pictures_known)");
	q->ParamByName(L"id")->AsInteger = AType.Id;
	SetText       (q->ParamByName(L"title"),               AType.Title);
	SetText       (q->ParamByName(L"object_type"),         AType.ObjectType);
	SetIntOrNull  (q->ParamByName(L"min_year"),            AType.MinYear);
	SetIntOrNull  (q->ParamByName(L"max_year"),            AType.MaxYear);
	SetText       (q->ParamByName(L"composition"),         AType.Composition);
	SetText       (q->ParamByName(L"technique"),           AType.Technique);
	SetFloatOrNull(q->ParamByName(L"weight"),              AType.Weight);
	SetFloatOrNull(q->ParamByName(L"size"),                AType.Size);
	SetFloatOrNull(q->ParamByName(L"size2"),               AType.Size2);
	SetFloatOrNull(q->ParamByName(L"thickness"),           AType.Thickness);
	SetText       (q->ParamByName(L"shape"),               AType.Shape);
	TFDParam *demonetized = q->ParamByName(L"is_demonetized");
	demonetized->DataType = ftInteger;
	if (AType.Demonetized < 0)
		demonetized->Clear();
	else
		demonetized->AsInteger = AType.Demonetized;
	SetText       (q->ParamByName(L"demonetization_date"), AType.DemonetizationDate);
	SetText       (q->ParamByName(L"rulers"),              AType.Rulers);
	SetText       (q->ParamByName(L"series"),              AType.Series);
	SetText       (q->ParamByName(L"commemorated_topic"),  AType.CommemoratedTopic);
	SetText       (q->ParamByName(L"mints"),               AType.Mints);
	q->ParamByName(L"updated_at")->AsWideString = DateToDb(Date());
	SetText       (q->ParamByName(L"obverse_picture"),     AType.Picture[0]);
	SetText       (q->ParamByName(L"reverse_picture"),     AType.Picture[1]);
	SetText       (q->ParamByName(L"obverse_copyright"),   AType.PictureCopyright[0]);
	SetText       (q->ParamByName(L"reverse_copyright"),   AType.PictureCopyright[1]);
	q->ParamByName(L"pictures_known")->AsInteger = AType.PicturesKnown ? 1 : 0;
	q->ExecSQL();
}
//---------------------------------------------------------------------------
bool TCoinRepository::LoadNumistaType(int ATypeId, TNumistaTypeInfo &AType)
{
	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase, L"SELECT * FROM numista_types WHERE id = :id");
	q->ParamByName(L"id")->AsInteger = ATypeId;
	q->Open();
	if (q->Eof)
		return false;
	TDataSet *D = q.get();
	AType = TNumistaTypeInfo();
	AType.Id                 = ATypeId;
	AType.Title              = FieldText (D, L"title");
	AType.ObjectType         = FieldText (D, L"object_type");
	AType.MinYear            = FieldInt  (D, L"min_year");
	AType.MaxYear            = FieldInt  (D, L"max_year");
	AType.Composition        = FieldText (D, L"composition");
	AType.Technique          = FieldText (D, L"technique");
	AType.Weight             = FieldFloat(D, L"weight");
	AType.Size               = FieldFloat(D, L"size");
	AType.Size2              = FieldFloat(D, L"size2");
	AType.Thickness          = FieldFloat(D, L"thickness");
	AType.Shape              = FieldText (D, L"shape");
	AType.Demonetized        = FieldInt  (D, L"is_demonetized", -1);
	AType.DemonetizationDate = FieldText (D, L"demonetization_date");
	AType.Rulers             = FieldText (D, L"rulers");
	AType.Series             = FieldText (D, L"series");
	AType.CommemoratedTopic  = FieldText (D, L"commemorated_topic");
	AType.Mints              = FieldText (D, L"mints");
	AType.Picture[0]          = FieldText(D, L"obverse_picture");
	AType.Picture[1]          = FieldText(D, L"reverse_picture");
	AType.PictureCopyright[0] = FieldText(D, L"obverse_copyright");
	AType.PictureCopyright[1] = FieldText(D, L"reverse_copyright");
	AType.PicturesKnown       = FieldInt (D, L"pictures_known") != 0;
	return true;
}
//---------------------------------------------------------------------------
// Экземпляры, импортированные из своей коллекции Numista (задача 20).
int TCoinRepository::NumistaOriginCount()
{
	return FDatabase->Connection()->ExecSQLScalar(L"SELECT COUNT(*) FROM coin_items WHERE origin = 'numista'");
}
//---------------------------------------------------------------------------
// Своя фраза поиска Numista ("" — автофраза).
void TCoinRepository::SetCoinQuery(int ACoinId, const String &AQuery)
{
	std::unique_ptr<TFDQuery> q = NewQuery(FDatabase, L"UPDATE coins SET numista_query = :query WHERE id = :id");
	SetText(q->ParamByName(L"query"), AQuery);
	q->ParamByName(L"id")->AsInteger = ACoinId;
	q->ExecSQL();
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
