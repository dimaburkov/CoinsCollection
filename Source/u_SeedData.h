//---------------------------------------------------------------------------
// Начальные данные справочников для миграции схемы (continents, countries).
// Названия стран — как в экспорте uCoin.net, чтобы импорт находил их по имени.
//---------------------------------------------------------------------------
#ifndef u_SeedDataH
#define u_SeedDataH
//---------------------------------------------------------------------------

// Континенты; Id — значение continents.id (вставляется явно).
enum TSeedContinent
{
	scEurope       = 1,
	scAsia         = 2,
	scAfrica       = 3,
	scNorthAmerica = 4,	// включая Центральную Америку и Карибы
	scSouthAmerica = 5,
	scOceania      = 6	// "Australia and Oceania"
};

struct TSeedContinentRow
{
	int            Id;
	const wchar_t *Name;
};

struct TSeedCountryRow
{
	const wchar_t *Name;
	int            ContinentId;
	bool           IsExist;	// false — исчезнувшее государство (узел Extinct states)
};

extern const TSeedContinentRow SeedContinents[];
extern const int               SeedContinentCount;

extern const TSeedCountryRow   SeedCountries[];
extern const int               SeedCountryCount;
//---------------------------------------------------------------------------
#endif
