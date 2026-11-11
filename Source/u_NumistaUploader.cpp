//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <memory>
#include <set>
#include "u_NumistaUploader.h"
#include "u_NumistaComment.h"
#include "u_AppConfig.h"
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
			const TForeignItem foreign = { numistaId, JsonInt(item, L"type.id"), JsonInt(item, L"issue.id"),
				SameText(JsonString(item, L"for_swap"), L"true"), false };
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
int TNumistaUploader::TakeForeign(int ATypeId, int AIssueId, bool AForSwap)
{
	// Проход 0 — тот же выпуск; проход 1 — выпуск не указан у предмета или у экземпляра.
	for (int pass = 0; pass < 2; ++pass)
		for (size_t i = 0; i < FForeign.size(); ++i)
		{
			TForeignItem &f = FForeign[i];
			if (f.Taken || f.Id == 0 || f.TypeId != ATypeId || f.ForSwap != AForSwap)
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
void TNumistaUploader::SplitSwapComment(const String &AComment, String &APublic, String &APrivate)
{
	APublic = APrivate = String();
	std::unique_ptr<TStringList> parts(new TStringList());
	parts->StrictDelimiter = true;
	parts->Delimiter = L';';
	parts->DelimitedText = AComment;
	const wchar_t *const prefixes[] = { L"\u043F\u0440\u0438\u0432\u0430\u0442\u043D\u043E:", L"private:" };	// «приватно:»
	for (int i = 0; i < parts->Count; ++i)
	{
		String part = parts->Strings[i].Trim();
		if (part.IsEmpty())
			continue;
		bool hidden = false;
		for (const wchar_t *prefix : prefixes)
			if (part.Length() >= (int)wcslen(prefix) && SameText(part.SubString(1, (int)wcslen(prefix)), prefix))
			{
				part = part.SubString((int)wcslen(prefix) + 1, part.Length()).Trim();
				hidden = true;
			}
		String &target = hidden ? APrivate : APublic;
		target += (target.IsEmpty() ? String() : String(L"; ")) + part;
	}
}
//---------------------------------------------------------------------------
String TNumistaUploader::ItemJson(const TCoinRecord &AItem)
{
	std::unique_ptr<TJSONObject> json(new TJSONObject());
	json->AddPair(L"type", new TJSONNumber(AItem.NumistaTypeId));
	if (AItem.NumistaIssueId != 0)
		json->AddPair(L"issue", new TJSONNumber(AItem.NumistaIssueId));
	json->AddPair(L"quantity", new TJSONNumber(AItem.Quantity > 0 ? AItem.Quantity : 1));

	// Какие поля отправлять — настройки Numista, вкладка «Що вивантажувати» (задача 33).
	const TAppConfig &config = AppConfig();
	const String grade = config.NumistaUploadGrade ? Grade(AItem.Condition) : String();
	if (!grade.IsEmpty())
		json->AddPair(L"grade", grade);

	// Цена — только своя (My value); Catalog value — оценка uCoin, а не цена покупки.
	if (config.NumistaUploadPrice && AItem.MyValueUah > 0.0)
	{
		TJSONObject *price = new TJSONObject();
		price->AddPair(L"value", new TJSONNumber(AItem.MyValueUah));
		price->AddPair(L"currency", L"UAH");
		json->AddPair(L"price", price);
	}
	if (config.NumistaUploadDate && (double)AItem.PurchaseDate != 0.0)
		json->AddPair(L"acquisition_date", FormatDateTime(L"yyyy-mm-dd", AItem.PurchaseDate));

	// Атрибуты: S — обмен, C — комментарий (R — индекс редкости — пишет пользователь на сайте).
	// Экземпляр обмена (задача 47a): отметка for_swap, заметка для партнёров — публичный комментарий.
	const bool swap = AItem.List == L"swap";
	String text = AItem.Comment, publicText;
	if (swap)
	{
		json->AddPair(L"for_swap", new TJSONBool(true));
		SplitSwapComment(AItem.Comment, publicText, text);
		if (config.NumistaUploadComment && !publicText.IsEmpty())
			json->AddPair(L"public_comment", publicText);
	}
	const String comment = config.NumistaUploadComment ? TNumistaComment::Build(AItem.SwapInfo, text) : String();
	if (!comment.IsEmpty())
		json->AddPair(L"private_comment", comment);

	json->AddPair(L"internal_id", IntToStr(AItem.Id));
	return json->ToJSON();
}
//---------------------------------------------------------------------------
