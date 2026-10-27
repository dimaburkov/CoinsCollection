//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <memory>
#include <System.IOUtils.hpp>
#include <System.JSON.hpp>
#include <System.NetEncoding.hpp>
#include "u_NumistaMatcher.h"
#include "u_NumistaClient.h"
#include "u_LookupRepository.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------

namespace
{
	// Страны, чьё название в uCoin не совпадает с эмитентом Numista
	// (или одноимённых эмитентов несколько, и правило «дочерний» выбирает не тот).
	struct TIssuerAlias { const wchar_t *Country; const wchar_t *Issuer; };
	const TIssuerAlias cIssuerAliases[] =
	{
		{ L"USA",                        L"united-states" },
		{ L"Congo - DRC",                L"drc" },
		{ L"Germany - GDR",              L"ddr" },
		{ L"Germany",                    L"allemagne" },	// ФРГ; "germany" — корень со всеми германскими государствами
		{ L"Germany - Third Reich",      L"allemagne-pre1945" },
		{ L"Eswatini (Swaziland)",       L"swaziland" },
		{ L"Gambia",                     L"gambie" },
		{ L"Bahamas",                    L"bahamas" },
		{ L"East Timor",                 L"timor_oriental" },
		{ L"Eastern Caribbean",          L"oeco" },
		{ L"Rhodesia",                   L"rhodesie" },
		{ L"Russian Empire",             L"russia-empire" },
		{ L"Russia",                     L"russie" },
		{ L"USSR",                       L"ancienne_urss" },
		{ L"Prussia",                    L"prussia_kingdom" },
		{ L"Western Africa (BCEAO)",     L"bceao" },
		{ L"Ceylon",                     L"ceylon_period" },
		{ L"Zaire",                      L"zaire_period" },
		{ L"Lombardy-Venetia",           L"lombardy_venetia" },
		{ L"British East Africa",        L"afrique_de_l_est" },
	};

	const int cPageSize = 50;	// максимум count у searchTypes

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
	//-----------------------------------------------------------------------
	// Число JSON с точкой как разделителем; 0 — нет данных.
	double JsonFloat(TJSONValue *AJson, const String &AName)
	{
		TFormatSettings fs = TFormatSettings::Invariant();
		return StrToFloatDef(JsonString(AJson, AName), 0.0, fs);
	}
	//-----------------------------------------------------------------------
	// Имена из массива объектов ("ruler", "mints") через "; ".
	String JsonNames(TJSONValue *AJson, const String &AName)
	{
		TJSONArray *items = AJson ? dynamic_cast<TJSONArray *>(AJson->FindValue(AName)) : nullptr;
		String result;
		for (int i = 0; items && i < items->Count; ++i)
		{
			const String name = JsonString(items->Items[i], L"name").Trim();
			if (!name.IsEmpty())
				result += (result.IsEmpty() ? L"" : L"; ") + name;
		}
		return result;
	}
	//-----------------------------------------------------------------------
	std::unique_ptr<TJSONValue> ReadCache(const String &AName)
	{
		const String file = TNumistaClient::CacheDir() + AName + L".json";
		if (!FileExists(file))
			return nullptr;
		return std::unique_ptr<TJSONValue>(TJSONObject::ParseJSONValue(TFile::ReadAllText(file, TEncoding::UTF8)));
	}
	//-----------------------------------------------------------------------
	String UrlParam(const String &AValue)
	{
		return TNetEncoding::URL->EncodeQuery(AValue);
	}
}
//---------------------------------------------------------------------------
TNumistaMatcher::TNumistaMatcher(TNumistaClient &AClient)
	: FClient(AClient)
{
}
//---------------------------------------------------------------------------
bool TNumistaMatcher::LoadReferences()
{
	FIssuerParent.clear();
	FIssuersByName.clear();
	FLevel.clear();
	FParentName.clear();
	FCatalogues.clear();

	std::unique_ptr<TJSONValue> issuers = ReadCache(L"issuers");
	std::unique_ptr<TJSONValue> catalogues = ReadCache(L"catalogues");
	TJSONArray *issuerList = issuers ? dynamic_cast<TJSONArray *>(issuers->FindValue(L"issuers")) : nullptr;
	TJSONArray *catalogueList = catalogues ? dynamic_cast<TJSONArray *>(catalogues->FindValue(L"catalogues")) : nullptr;
	if (!issuerList || !catalogueList)
		return false;

	for (int i = 0; i < issuerList->Count; ++i)
	{
		TJSONValue *item = issuerList->Items[i];
		const String code = JsonString(item, L"code");
		if (code.IsEmpty())
			continue;
		FIssuersByName[JsonString(item, L"name").LowerCase()].push_back(code);
		FLevel[code] = JsonInt(item, L"level");
		FIssuerParent[code] = JsonString(item, L"parent.code");
		FParentName[code] = JsonString(item, L"parent.name").LowerCase();
	}
	for (int i = 0; i < catalogueList->Count; ++i)
	{
		TJSONValue *item = catalogueList->Items[i];
		FCatalogues[JsonString(item, L"code").UpperCase()] = JsonInt(item, L"id");
	}
	return true;
}
//---------------------------------------------------------------------------
String TNumistaMatcher::ResolveIssuer(const String &ACountryName) const
{
	for (const TIssuerAlias &alias : cIssuerAliases)
		if (SameText(ACountryName, alias.Country))
			return alias.Issuer;

	const String name = ACountryName.LowerCase();
	auto found = FIssuersByName.find(name);
	if (found == FIssuersByName.end())
		return String();

	// Одноимённые эмитенты: раздел-группа и дочерний эмитент (Austria → austria / autriche);
	// монеты привязаны к дочернему. Иначе — самый верхний (не римская провинция и т. п.).
	String child, top;
	int childLevel = -1, topLevel = 1000;
	for (const String &code : found->second)
	{
		const int level = FLevel.at(code);
		if (FParentName.at(code) == name && level > childLevel)
		{
			child = code;
			childLevel = level;
		}
		if (level < topLevel)
		{
			top = code;
			topLevel = level;
		}
	}
	return child.IsEmpty() ? top : child;
}
//---------------------------------------------------------------------------
int TNumistaMatcher::EnsureCountryIssuers(TLookupRepository &ALookups)
{
	int filled = 0;
	for (const TCountry &country : ALookups.Countries())
	{
		if (!country.NumistaIssuer.IsEmpty())
			continue;
		const String issuer = ResolveIssuer(country.Name);
		if (issuer.IsEmpty())
			continue;
		ALookups.SetCountryIssuer(country.Id, issuer);
		++filled;
	}
	return filled;
}
//---------------------------------------------------------------------------
bool TNumistaMatcher::ParseCatalogNumber(const String &AText, int &ACatalogueId, String &ANumber) const
{
	// "KM# 2.1, KM# 2.2" — берём первый номер.
	String text = AText.Trim();
	const int comma = text.Pos(L",");
	if (comma > 0)
		text = text.SubString(1, comma - 1).Trim();

	const int hash = text.Pos(L"#");
	if (hash <= 1)
		return false;
	const String code = text.SubString(1, hash - 1).Trim().UpperCase();
	const String number = text.SubString(hash + 1, text.Length()).Trim();
	if (number.IsEmpty() || (code != L"KM" && code != L"Y" && code != L"C"))
		return false;

	auto found = FCatalogues.find(code);
	if (found == FCatalogues.end() || found->second == 0)
		return false;
	ACatalogueId = found->second;
	ANumber = number;
	return true;
}
//---------------------------------------------------------------------------
String TNumistaMatcher::RootOf(const String &AIssuer) const
{
	String code = AIssuer;
	for (int guard = 0; guard < 10; ++guard)
	{
		auto parent = FIssuerParent.find(code);
		if (parent == FIssuerParent.end() || parent->second.IsEmpty())
			break;
		code = parent->second;
	}
	return code;
}
//---------------------------------------------------------------------------
std::vector<TNumistaType> TNumistaMatcher::QueryTypes(const String &APath)
{
	std::vector<TNumistaType> result;
	std::unique_ptr<TJSONValue> json = FClient.GetCached(APath);
	TJSONArray *types = dynamic_cast<TJSONArray *>(json->FindValue(L"types"));
	for (int i = 0; types && i < types->Count; ++i)
	{
		TJSONValue *item = types->Items[i];
		TNumistaType t;
		t.Id         = JsonInt   (item, L"id");
		t.Title      = JsonString(item, L"title");
		t.IssuerCode = JsonString(item, L"issuer.code");
		t.IssuerName = JsonString(item, L"issuer.name");
		t.MinYear    = JsonInt   (item, L"min_year");
		t.MaxYear    = JsonInt   (item, L"max_year");
		if (t.Id != 0)
			result.push_back(t);
	}
	return result;
}
//---------------------------------------------------------------------------
// Отбор по году монеты — по годам типа (min_year–max_year) из уже полученного ответа,
// без запросов. Под год не подошёл ни один тип (годы не указаны / иное летоисчисление) —
// показываются все найденные.
bool TNumistaMatcher::SameBranch(const String &AIssuer1, const String &AIssuer2) const
{
	return !AIssuer1.IsEmpty() && !AIssuer2.IsEmpty() && RootOf(AIssuer1) == RootOf(AIssuer2);
}
//---------------------------------------------------------------------------
std::vector<TNumistaType> TNumistaMatcher::Search(const TCoinRecord &ACoin, const String &AIssuer,
	const String &ACountryIssuer)
{
	const std::vector<TNumistaType> all = SearchAll(ACoin, AIssuer, ACountryIssuer);
	if (ACoin.Year == 0)
		return all;
	std::vector<TNumistaType> result;
	for (const TNumistaType &t : all)
	{
		const int maxYear = t.MaxYear != 0 ? t.MaxYear : t.MinYear;
		if (t.MinYear != 0 && t.MinYear <= ACoin.Year && ACoin.Year <= maxYear)
			result.push_back(t);
	}
	return result.empty() ? all : result;
}
//---------------------------------------------------------------------------
std::vector<TNumistaType> TNumistaMatcher::SearchAll(const TCoinRecord &ACoin, const String &AIssuer,
	const String &ACountryIssuer)
{
	const String common = L"&lang=en&count=" + IntToStr(cPageSize);
	int catalogueId = 0;
	String number;
	if (ParseCatalogNumber(ACoin.CatalogNumber, catalogueId, number))
	{
		const String path = L"/types?catalogue=" + IntToStr(catalogueId) + L"&number=" + UrlParam(number) + common;
		if (!AIssuer.IsEmpty())
		{
			std::vector<TNumistaType> found = QueryTypes(path + L"&issuer=" + UrlParam(AIssuer));
			if (!found.empty())
				return found;
		}

		// Эмитенты Numista иерархичны: монета может числиться у соседнего эмитента
		// той же страны. Без issuer — с годом монеты (без него — сотни монет с этим
		// номером со всего мира, а читается одна страница) и отбор по ветке эмитента.
		// Из ветки нет — монеты других эмитентов (историческое государство) с пометкой
		// OtherIssuer: связываются только с подтверждением (задача 15a).
		std::vector<TNumistaType> all = QueryTypes(path + (ACoin.Year > 0 ? L"&date=" + IntToStr(ACoin.Year) : String()));
		if (AIssuer.IsEmpty())
			return all;
		std::vector<TNumistaType> result;
		for (const TNumistaType &t : all)
			if (SameBranch(t.IssuerCode, AIssuer) || SameBranch(t.IssuerCode, ACountryIssuer))
				result.push_back(t);
		if (!result.empty())
			return result;
		for (TNumistaType &t : all)
			t.OtherIssuer = true;
		return all;
	}

	// UC# / без номера — по номиналу и году.
	String path = L"/types?q=" + UrlParam(AIssuer.IsEmpty() ? ACoin.Country + L" " + ACoin.Denomination : ACoin.Denomination) + common;
	if (!AIssuer.IsEmpty())
		path += L"&issuer=" + UrlParam(AIssuer);
	if (ACoin.Year > 0)
		path += L"&year=" + IntToStr(ACoin.Year);
	return QueryTypes(path);
}
//---------------------------------------------------------------------------
std::vector<TNumistaIssue> TNumistaMatcher::LoadIssues(int ATypeId, bool ARefresh)
{
	std::vector<TNumistaIssue> result;
	std::unique_ptr<TJSONValue> json = FClient.GetCached(IssuesPath(ATypeId), nullptr, ARefresh);
	TJSONArray *issues = dynamic_cast<TJSONArray *>(json.get());
	for (int i = 0; issues && i < issues->Count; ++i)
	{
		TJSONValue *item = issues->Items[i];
		TNumistaIssue issue;
		issue.Id            = JsonInt   (item, L"id");
		issue.Year          = JsonInt   (item, L"year");
		issue.GregorianYear = JsonInt   (item, L"gregorian_year");
		issue.MinYear       = JsonInt   (item, L"min_year");
		issue.MaxYear       = JsonInt   (item, L"max_year");
		issue.MintLetter    = JsonString(item, L"mint_letter");
		issue.Mintage       = JsonString(item, L"mintage");
		issue.Comment       = JsonString(item, L"comment");
		if (issue.Id != 0)
			result.push_back(issue);
	}
	return result;
}
//---------------------------------------------------------------------------
int TNumistaMatcher::FindIssue(int ATypeId, const TCoinRecord &ACoin)
{
	if (ATypeId == 0 || ACoin.Year == 0)
		return 0;

	const String mint = ACoin.Variety.Trim();
	// Для года и двора у Numista бывает несколько выпусков: обычный и с комментарием
	// ("BU set", "Proof"). Несколько совпадений — берётся единственный без комментария.
	int result = 0, matches = 0, plain = 0, plainMatches = 0;
	for (const TNumistaIssue &issue : LoadIssues(ATypeId))
	{
		if (issue.EffectiveYear() != ACoin.Year)
			continue;
		if (!mint.IsEmpty() && !SameText(issue.MintLetter, mint))
			continue;
		result = issue.Id;
		++matches;
		if (issue.Comment.Trim().IsEmpty())
		{
			plain = result;
			++plainMatches;
		}
	}
	if (matches == 1)
		return result;
	return plainMatches == 1 ? plain : 0;
}
//---------------------------------------------------------------------------
String TNumistaMatcher::TypePath(int ATypeId)
{
	return L"/types/" + IntToStr(ATypeId) + L"?lang=en";
}
//---------------------------------------------------------------------------
TNumistaTypeInfo TNumistaMatcher::LoadType(int ATypeId, bool ARefresh)
{
	std::unique_ptr<TJSONValue> json = FClient.GetCached(TypePath(ATypeId), nullptr, ARefresh);
	TJSONValue *j = json.get();
	TNumistaTypeInfo t;
	t.Id                 = ATypeId;
	t.Title              = JsonString(j, L"title");
	t.ObjectType         = JsonString(j, L"object_type.name");
	t.MinYear            = JsonInt   (j, L"min_year");
	t.MaxYear            = JsonInt   (j, L"max_year");
	t.Composition        = JsonString(j, L"composition.text");
	t.Technique          = JsonString(j, L"technique.text");
	t.Weight             = JsonFloat (j, L"weight");
	t.Size               = JsonFloat (j, L"size");
	t.Size2              = JsonFloat (j, L"size2");
	t.Thickness          = JsonFloat (j, L"thickness");
	t.Shape              = JsonString(j, L"shape");
	const String demonetized = JsonString(j, L"demonetization.is_demonetized");
	t.Demonetized        = SameText(demonetized, L"true") ? 1 : (SameText(demonetized, L"false") ? 0 : -1);
	t.DemonetizationDate = JsonString(j, L"demonetization.demonetization_date");
	t.Rulers             = JsonNames (j, L"ruler");
	t.Series             = JsonString(j, L"series");
	t.CommemoratedTopic  = JsonString(j, L"commemorated_topic");
	t.Mints              = JsonNames (j, L"mints");
	t.Picture[0]          = JsonString(j, L"obverse.picture");
	t.Picture[1]          = JsonString(j, L"reverse.picture");
	t.PictureCopyright[0] = JsonString(j, L"obverse.picture_copyright");
	t.PictureCopyright[1] = JsonString(j, L"reverse.picture_copyright");
	t.PicturesKnown       = true;
	return t;
}
//---------------------------------------------------------------------------
String TNumistaMatcher::IssuesPath(int ATypeId)
{
	return L"/types/" + IntToStr(ATypeId) + L"/issues?lang=en";
}
//---------------------------------------------------------------------------
String TNumistaMatcher::TypeUrl(int ATypeId)
{
	return L"https://en.numista.com/catalogue/pieces" + IntToStr(ATypeId) + L".html";
}
//---------------------------------------------------------------------------
