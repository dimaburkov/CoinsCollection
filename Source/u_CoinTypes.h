//---------------------------------------------------------------------------
// Базовые типы предметной области CoinsCollection.
//---------------------------------------------------------------------------
#ifndef u_CoinTypesH
#define u_CoinTypesH
//---------------------------------------------------------------------------
#include <System.hpp>
//---------------------------------------------------------------------------

// Состояние сохранности монеты. Пока в модели хранится строкой (TCoinRecord::Condition);
// enum оставлен для будущей нормализации справочника состояний.
enum TCoinCondition
{
	ccUnknown,
	ccPoor,
	ccFair,
	ccGood,
	ccVeryGood,
	ccFine,
	ccVeryFine,
	ccExtremelyFine,
	ccAboutUnc,
	ccUncirculated,
	ccProof
};

//---------------------------------------------------------------------------
// Один экземпляр коллекции в «плоском» виде — как строка xlsx-экспорта uCoin.net.
// В БД раскладывается по справочникам, coins (монета-тип) и coin_items (экземпляр).
// Числовые "нет данных" кодируются нулём, даты — нулевым TDateTime.
//---------------------------------------------------------------------------
struct TCoinRecord
{
	int       Id;              // coin_items.id (0 = новый экземпляр)
	int       CoinId;          // coins.id (0 = монета ещё не сохранена)

	String    Country;
	String    Period;
	String    Currency;
	String    Denomination;
	int       Year;            // 0 = не указан
	String    Variety;         // колонка "Var."
	String    Subject;
	double    DiameterMm;      // 0 = не указан
	String    Condition;       // "Condition" (VF / XF / UNC ...)

	double    CatalogValueUah; // "Value, UAH [uCoin]"
	String    CatalogNumber;   // "Number", напр. "KM# 954"
	String    Color;
	int       Quantity;
	bool      NeedToReplace;

	TDateTime PublishedDate;   // "Published"; 0 = нет
	String    SwapInfo;        // "Swap", напр. "Adler /uid86829"
	TDateTime PurchaseDate;    // "Purchase"; 0 = нет ("00/01/1900" в файле)
	double    MyValueUah;      // "Value, UAH [My]"

	String    GradingCompany;
	String    GradingNumber;
	String    Grade;
	String    LabelText;       // "Label"
	String    Comment;

	TCoinRecord()
		: Id(0),
		  CoinId(0),
		  Year(0),
		  DiameterMm(0.0),
		  CatalogValueUah(0.0),
		  Quantity(1),
		  NeedToReplace(false),
		  MyValueUah(0.0)
	{
	}
};

//---------------------------------------------------------------------------
// Записи справочников (таблицы continents, countries, periods, currencies, conditions).
//---------------------------------------------------------------------------
struct TContinent
{
	int    Id;
	String Name;
	TContinent() : Id(0) {}
};

struct TCountry
{
	int    Id;
	String Name;
	int    ContinentId;        // 0 = не указан
	bool   IsExist;            // false — исчезнувшее государство
	TCountry() : Id(0), ContinentId(0), IsExist(true) {}
};

struct TPeriod
{
	int    Id;
	int    CountryId;
	String Name;               // пустая строка — период не указан
	TPeriod() : Id(0), CountryId(0) {}
};

struct TCurrency
{
	int    Id;
	String Name;
	TCurrency() : Id(0) {}
};

struct TCondition
{
	int    Id;
	String Code;               // VF, XF, UNC ...
	String Name;
	int    SortOrder;
	TCondition() : Id(0), SortOrder(0) {}
};
//---------------------------------------------------------------------------
#endif
