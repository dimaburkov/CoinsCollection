//---------------------------------------------------------------------------
// Сопоставление монет коллекции с типами и выпусками Numista (задача 12).
// Документация: https://en.numista.com/api/doc/index.php
//   searchTypes — GET /types (issuer, catalogue, number, q, year, lang, count)
//   getIssues   — GET /types/{type_id}/issues
// Коды эмитентов и id каталогов — из кэша getIssuers / getCatalogues (задача 11),
// без запросов. Ответы searchTypes и getIssues кэшируются (TNumistaClient::GetCached).
//---------------------------------------------------------------------------
#ifndef u_NumistaMatcherH
#define u_NumistaMatcherH
//---------------------------------------------------------------------------
#include <System.hpp>
#include <map>
#include <vector>
#include "u_CoinTypes.h"
//---------------------------------------------------------------------------
class TNumistaClient;
class TLookupRepository;
//---------------------------------------------------------------------------
struct TNumistaType
{
	int    Id;              // N#
	String Title;
	String IssuerCode;
	String IssuerName;
	int    MinYear;         // 0 — нет данных
	int    MaxYear;
	bool   OtherIssuer;     // эмитент вне ветки страны / периода (задача 15a) — только с подтверждением
	bool   ByAutoQuery;     // найден по автофразе (задача 15b) — только с подтверждением
	TNumistaType() : Id(0), MinYear(0), MaxYear(0), OtherIssuer(false), ByAutoQuery(false) {}
};
//---------------------------------------------------------------------------
class TNumistaMatcher
{
public:
	explicit TNumistaMatcher(TNumistaClient &AClient);

	// Справочники из кэша (issuers.json, catalogues.json); false — кэша нет
	// (сначала Numista → Connect / Settings → Check connection).
	bool LoadReferences();

	// Код эмитента по названию страны: таблица соответствий, затем точное
	// совпадение названия (из одноимённых — дочерний эмитент). "" — не найден.
	String ResolveIssuer(const String &ACountryName) const;
	// Заполнить countries.numista_issuer там, где он пуст. Возвращает число заполненных.
	int EnsureCountryIssuers(TLookupRepository &ALookups);

	// "KM# 308" / "Y# 141a" / "C# 12.1" → id каталога Numista и номер.
	// false — каталог не KM / Y / C (например, UC#) или его нет в кэше.
	bool ParseCatalogNumber(const String &AText, int &ACatalogueId, String &ANumber) const;

	// Поиск типов для монеты. AIssuer — эмитент периода (если запомнен) или страны,
	// ACountryIssuer — эмитент страны ("" — не известен).
	// С номером KM / Y / C — issuer + catalogue + number; при пустом ответе — без issuer,
	// но с date = год монеты (задача 15a): из ветки эмитента страны / периода — как есть,
	// таких нет — все с OtherIssuer. Без номера — issuer + q=номинал + year.
	// Затем отбор по году монеты (годы типа min_year–max_year), если под год подходит хотя бы один тип.
	// Своя фраза монеты (NumistaQuery, задача 15b) — поиск только по ней. AAutoQuery — ничего не
	// найдено обычным путём — ещё поиск по автофразе (результаты — с ByAutoQuery).
	std::vector<TNumistaType> Search(const TCoinRecord &ACoin, const String &AIssuer,
		const String &ACountryIssuer = String(), bool AAutoQuery = false);
	// Поиск по фразе (searchTypes q=…, без эмитента; с кэшем).
	std::vector<TNumistaType> SearchPhrase(const String &AQuery);
	// Автосвязь монеты без номера KM / Y / C (UC#) по автофразе (задача 25, 1 запрос с кэшем):
	// из найденных — типы, в годы которых входит год монеты и эмитент которых в ветке
	// эмитента периода / страны (коды не известны — без этой проверки). Связывать можно,
	// только если остался ровно один. AFound — сколько нашлось по фразе всего.
	std::vector<TNumistaType> SearchAutoQuery(const TCoinRecord &ACoin, const String &AIssuer,
		const String &ACountryIssuer, int &AFound);
	// Автофраза поиска (задача 25): есть сюжет — страна, год, сюжет
	// («Ukraine 2020 Remembrance Day of the Fallen Defenders of Ukraine»), иначе — страна,
	// номинал, год («Ukraine 10 hryvnias 2020»).
	static String AutoQuery(const TCoinRecord &ACoin);
	// Оценка запросов автосвязи (задача 28). Адрес первого поиска для монеты — как в Search /
	// SearchAutoQuery (AAutoQuery — монета ищется по автофразе); ключ кэша.
	String FirstSearchPath(const TCoinRecord &ACoin, const String &AIssuer, bool AAutoQuery) const;
	// Типы из кэшированного ответа поиска, без запроса; false — ответа в кэше нет.
	bool CachedTypes(const String &APath, std::vector<TNumistaType> &ATypes);
	// Эмитенты в одной ветке иерархии Numista (общий корень).
	bool SameBranch(const String &AIssuer1, const String &AIssuer2) const;

	// Выпуск типа для монеты: год совпадает и (если указана разновидность)
	// совпадает знак монетного двора (без точек и пробелов, кириллица — латиницей:
	// "Е.М." = "EM"; задача 28a); из нескольких — единственный без комментария, иначе
	// единственный обычный (не Proof / BU / набор …). 0 — не найден или не однозначен.
	int FindIssue(int ATypeId, const TCoinRecord &ACoin);
	// Все выпуски типа (getIssues, с кэшем; ARefresh — мимо кэша, кэш перезаписывается).
	std::vector<TNumistaIssue> LoadIssues(int ATypeId, bool ARefresh = false);
	// Адрес getIssues типа (ключ кэша — TNumistaClient::IsCached).
	static String IssuesPath(int ATypeId);
	// Характеристики типа (getType, с кэшем; ARefresh — мимо кэша). Задача 13d.
	TNumistaTypeInfo LoadType(int ATypeId, bool ARefresh = false);
	static String TypePath(int ATypeId);

	static String TypeUrl(int ATypeId);
	// Знак двора для сравнения: заглавные, без точек / пробелов / дефисов, кириллица латиницей.
	static String MintKey(const String &AMint);
	// Примечание выпуска говорит о Proof, BU, наборе, пробе, рестрайке … (не обычный тираж).
	static bool IsSpecialIssue(const String &AComment);

private:
	TNumistaClient &FClient;
	std::map<String, String> FIssuerParent;                 // код → код родителя
	std::map<String, std::vector<String> > FIssuersByName;  // название (lower) → коды
	std::map<String, int> FLevel;                           // код → уровень
	std::map<String, String> FParentName;                   // код → название родителя (lower)
	std::map<String, int> FCatalogues;                      // код каталога (upper) → id

	String RootOf(const String &AIssuer) const;
	std::vector<TNumistaType> SearchAll(const TCoinRecord &ACoin, const String &AIssuer, const String &ACountryIssuer);
	std::vector<TNumistaType> QueryTypes(const String &APath);
	// Адреса searchTypes: по фразе, по номеру каталога (без issuer / date), по номиналу.
	static String PhrasePath(const String &AQuery);
	static String NumberPath(int ACatalogueId, const String &ANumber);
	static String DenominationPath(const TCoinRecord &ACoin, const String &AIssuer);
};
//---------------------------------------------------------------------------
#endif
