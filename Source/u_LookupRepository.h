//---------------------------------------------------------------------------
// Справочники: континенты, страны, периоды, валюты, шкала состояний.
// Поиск по имени (после Trim; страны и валюты — без учёта регистра латиницы),
// при необходимости — создание записи. Все запросы параметризованные.
//---------------------------------------------------------------------------
#ifndef u_LookupRepositoryH
#define u_LookupRepositoryH
//---------------------------------------------------------------------------
#include <map>
#include <vector>
#include "u_CoinTypes.h"
//---------------------------------------------------------------------------
class TDatabase;
//---------------------------------------------------------------------------
class TLookupRepository
{
public:
	explicit TLookupRepository(TDatabase *ADatabase);	// не владеет
	~TLookupRepository();

	// Списки по алфавиту (Conditions — по sort_order).
	std::vector<TContinent> Continents();
	std::vector<TCountry>   Countries();
	std::vector<TPeriod>    Periods(int ACountryId);
	std::vector<TCurrency>  Currencies();
	std::vector<TCondition> Conditions();

	// Найти или создать; вернуть id. Пустое имя страны — исключение.
	// Новая страна: континент не указан, is_exist = 1.
	int FindOrCreateCountry(const String &AName);
	// Пустое имя периода допустимо («период не указан»).
	int FindOrCreatePeriod(int ACountryId, const String &AName);
	// Пустое имя валюты -> 0 (NULL в coins).
	int FindOrCreateCurrency(const String &AName);

	// Только поиск: шкала фиксирована. Пустой код -> 0; неизвестный — исключение.
	int FindCondition(const String &ACode);

	// Сохранить континент и признак is_exist страны.
	void UpdateCountry(const TCountry &ACountry);
	// Код эмитента Numista у страны ("" — не указан).
	void SetCountryIssuer(int ACountryId, const String &AIssuer);

	// --- Правка справочников (форма Reference) ---
	// Число экземпляров коллекции: id записи справочника -> экземпляров.
	std::map<int, int> CountryUsage();
	std::map<int, int> PeriodUsage(int ACountryId);
	std::map<int, int> CurrencyUsage();

	// Есть ли уже запись с таким именем (страны и валюты — без учёта регистра
	// латиницы, периоды — в пределах страны). AExceptId — не считать саму запись.
	bool CountryExists(const String &AName, int AExceptId = 0);
	bool PeriodExists(int ACountryId, const String &AName, int AExceptId = 0);
	bool CurrencyExists(const String &AName, int AExceptId = 0);

	// Добавить — вернуть id; имя должно быть непустым и свободным (иначе исключение).
	int  AddCountry(const TCountry &ACountry);
	int  AddPeriod(int ACountryId, const String &AName);
	int  AddCurrency(const String &AName);

	// Сохранить имя, континент, is_exist и код Numista страны / переименовать период, валюту.
	void SaveCountry(const TCountry &ACountry);
	void RenamePeriod(int AId, const String &AName);
	void RenameCurrency(int AId, const String &AName);

	// Удалить, только если на запись не ссылается ни одна монета; false — используется.
	// Страна удаляется вместе со своими (неиспользуемыми) периодами.
	bool DeleteCountry(int AId);
	bool DeletePeriod(int AId);
	bool DeleteCurrency(int AId);

private:
	TDatabase *FDatabase;
};
//---------------------------------------------------------------------------
#endif
