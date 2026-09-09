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
// Одна монета коллекции. Соответствует строке xlsx-экспорта uCoin.net.
// Числовые "нет данных" кодируются нулём, даты — нулевым TDateTime.
//---------------------------------------------------------------------------
struct TCoinRecord
{
	int       Id;              // внутренний первичный ключ (0 = новая запись)

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
#endif
