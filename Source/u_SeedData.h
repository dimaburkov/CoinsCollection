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

// Континент по эмитенту Numista (задача 54): разделы верхнего уровня (level 1) и заморские
// территории внутри них (Territory — своя страна, а не часть страны раздела). Ищется по цепочке
// эмитент -> родители: первый найденный. Вымышленные государства, микронации, океаны — не указаны.
struct TSeedIssuerContinentRow
{
	const wchar_t *Issuer;
	int            ContinentId;
	bool           IsExist;	// false — раздел исторических государств (Rome, Aksum...)
	bool           Territory;
};

extern const TSeedIssuerContinentRow SeedIssuerContinents[];
extern const int                     SeedIssuerContinentCount;

// Правители эмитента Numista (задача 56): у Roman Empire (rome) нет подразделов по императорам, а в API
// нет списка правителей — ID собраны из /types?issuer=rome&ruler=N и getType (25.11.2026). Порядок —
// хронологический; Group — группа Numista (династия). Каталог фильтрует типы по ruler=Id.
struct TSeedRulerRow
{
	const wchar_t *Issuer;
	int            RulerId;
	const wchar_t *Name;
	const wchar_t *Group;
};

extern const TSeedRulerRow SeedRulers[];
extern const int           SeedRulerCount;
//---------------------------------------------------------------------------
#endif
