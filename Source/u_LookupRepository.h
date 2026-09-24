//---------------------------------------------------------------------------
// Справочники: континенты, страны, периоды, валюты, шкала состояний.
// Поиск по имени (после Trim; страны и валюты — без учёта регистра латиницы),
// при необходимости — создание записи. Все запросы параметризованные.
//---------------------------------------------------------------------------
#ifndef u_LookupRepositoryH
#define u_LookupRepositoryH
//---------------------------------------------------------------------------
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

private:
	TDatabase *FDatabase;
};
//---------------------------------------------------------------------------
#endif
