//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <memory>
#include <set>
#include "u_NumistaUploader.h"
#include "u_NumistaComment.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
namespace
{
	String JsonString(TJSONValue *AJson, const String &AName)
	{
		TJSONValue *value = AJson ? AJson->FindValue(AName) : nullptr;
		return value && !dynamic_cast<TJSONNull *>(value) ? value->Value() : String();
	}
	//-----------------------------------------------------------------------
	int JsonInt(TJSONValue *AJson, const String &AName)
	{
		return StrToIntDef(JsonString(AJson, AName), 0);
	}
}
//---------------------------------------------------------------------------
void TNumistaUploader::ReadCollection(TJSONValue *AJson, const std::vector<TCoinRecord> &AAll)
{
	FUploaded.clear();
	FForeign.clear();
	std::set<int> known;		// id предметов Numista у наших экземпляров
	std::map<int, int> typeOf;	// Id экземпляра -> тип Numista
	for (size_t i = 0; i < AAll.size(); ++i)
	{
		if (AAll[i].NumistaItemId != 0)
			known.insert(AAll[i].NumistaItemId);
		typeOf[AAll[i].Id] = AAll[i].NumistaTypeId;
	}

	TJSONArray *items = AJson ? dynamic_cast<TJSONArray *>(AJson->FindValue(L"items")) : nullptr;
	for (int i = 0; items && i < items->Count; ++i)
	{
		TJSONValue *item = items->Items[i];
		const int numistaId  = JsonInt(item, L"id");
		const int internalId = StrToIntDef(JsonString(item, L"internal_id").Trim(), 0);
		std::map<int, int>::const_iterator our = typeOf.find(internalId);
		if (known.count(numistaId))
			continue;	// уже связан с экземпляром программы
		if (internalId != 0 && our != typeOf.end() && our->second == JsonInt(item, L"type.id"))
			FUploaded[internalId] = numistaId;
		else
		{
			const TForeignItem foreign = { numistaId, JsonInt(item, L"type.id"), JsonInt(item, L"issue.id"), false };
			FForeign.push_back(foreign);
		}
	}
}
//---------------------------------------------------------------------------
int TNumistaUploader::UploadedItem(int AItemId) const
{
	std::map<int, int>::const_iterator it = FUploaded.find(AItemId);
	return it == FUploaded.end() ? 0 : it->second;
}
//---------------------------------------------------------------------------
int TNumistaUploader::TakeForeign(int ATypeId, int AIssueId)
{
	// Проход 0 — тот же выпуск; проход 1 — выпуск не указан у предмета или у экземпляра.
	for (int pass = 0; pass < 2; ++pass)
		for (size_t i = 0; i < FForeign.size(); ++i)
		{
			TForeignItem &f = FForeign[i];
			if (f.Taken || f.Id == 0 || f.TypeId != ATypeId)
				continue;
			const bool sameIssue = AIssueId != 0 && f.IssueId == AIssueId;
			if (pass == 0 ? sameIssue : (f.IssueId == 0 || AIssueId == 0))
			{
				f.Taken = true;
				return f.Id;
			}
		}
	return 0;
}
//---------------------------------------------------------------------------
String TNumistaUploader::Grade(const String &ACondition)
{
	const String c = ACondition.Trim().UpperCase();
	if (c == L"G" || c == L"VG" || c == L"F" || c == L"VF" || c == L"XF" || c == L"AU" || c == L"UNC")
		return c.LowerCase();
	return String();	// P, FR, PRF, пусто — в шкале Numista нет
}
//---------------------------------------------------------------------------
String TNumistaUploader::ItemJson(const TCoinRecord &AItem)
{
	std::unique_ptr<TJSONObject> json(new TJSONObject());
	json->AddPair(L"type", new TJSONNumber(AItem.NumistaTypeId));
	if (AItem.NumistaIssueId != 0)
		json->AddPair(L"issue", new TJSONNumber(AItem.NumistaIssueId));
	json->AddPair(L"quantity", new TJSONNumber(AItem.Quantity > 0 ? AItem.Quantity : 1));

	const String grade = Grade(AItem.Condition);
	if (!grade.IsEmpty())
		json->AddPair(L"grade", grade);

	// Цена — только своя (My value); Catalog value — оценка uCoin, а не цена покупки.
	if (AItem.MyValueUah > 0.0)
	{
		TJSONObject *price = new TJSONObject();
		price->AddPair(L"value", new TJSONNumber(AItem.MyValueUah));
		price->AddPair(L"currency", L"UAH");
		json->AddPair(L"price", price);
	}
	if ((double)AItem.PurchaseDate != 0.0)
		json->AddPair(L"acquisition_date", FormatDateTime(L"yyyy-mm-dd", AItem.PurchaseDate));

	// Атрибуты: S — обмен, C — комментарий (R — индекс редкости — пишет пользователь на сайте).
	const String comment = TNumistaComment::Build(AItem.SwapInfo, AItem.Comment);
	if (!comment.IsEmpty())
		json->AddPair(L"private_comment", comment);

	json->AddPair(L"internal_id", IntToStr(AItem.Id));
	return json->ToJSON();
}
//---------------------------------------------------------------------------
