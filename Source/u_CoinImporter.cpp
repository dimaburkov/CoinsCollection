//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <math.h>
#include "u_CoinImporter.h"
#include "u_ImportSource.h"
#include "u_Translator.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------

namespace
{
	// Заголовки колонок экспорта uCoin.net.
	const wchar_t *const cCountry        = L"Country";
	const wchar_t *const cPeriod         = L"Period";
	const wchar_t *const cCurrency       = L"Currency";
	const wchar_t *const cDenomination   = L"Denomination";
	const wchar_t *const cYear           = L"Year";
	const wchar_t *const cVariety        = L"Var.";
	const wchar_t *const cSubject        = L"Subject";
	const wchar_t *const cDiameter       = L"Diameter, mm";
	const wchar_t *const cCondition      = L"Condition";
	const wchar_t *const cCatalogValue   = L"Value, UAH [uCoin]";
	const wchar_t *const cNumber         = L"Number";
	const wchar_t *const cColor          = L"Color";
	const wchar_t *const cQuantity       = L"Quantity";
	const wchar_t *const cNeedToReplace  = L"Need to replace";
	const wchar_t *const cPublished      = L"Published";
	const wchar_t *const cSwap           = L"Swap";
	const wchar_t *const cPurchase       = L"Purchase";
	const wchar_t *const cMyValue        = L"Value, UAH [My]";
	const wchar_t *const cGradingCompany = L"Grading company";
	const wchar_t *const cGradingNumber  = L"Grading number";
	const wchar_t *const cGrade          = L"Grade";
	const wchar_t *const cLabel          = L"Label";
	const wchar_t *const cComment        = L"Comment";

	const wchar_t *const cRequired[] = { cCountry, cDenomination };

	// Ошибка разбора ячейки — текст уже переведён.
	class ERowError : public Exception
	{
	public:
		__fastcall ERowError(const String &AMessage) : Exception(AMessage) {}
	};
	//-----------------------------------------------------------------------
	// Число с точкой (как пишет Excel) независимо от настроек Windows.
	double ParseNumber(const String &AText, const String &AHeader)
	{
		if (AText.IsEmpty())
			return 0.0;
		double value = 0.0;
		if (!TryStrToFloat(AText, value, TFormatSettings::Invariant()))
			throw ERowError(Format(Tr(L"Import.Error.BadNumber"), ARRAYOFCONST((AHeader, AText))));
		return value;
	}
	//-----------------------------------------------------------------------
	int ParseInt(const String &AText, const String &AHeader, int AEmpty)
	{
		if (AText.IsEmpty())
			return AEmpty;
		const double value = ParseNumber(AText, AHeader);
		if (value != floor(value) || fabs(value) > 1e9)
			throw ERowError(Format(Tr(L"Import.Error.BadNumber"), ARRAYOFCONST((AHeader, AText))));
		return (int)value;
	}
	//-----------------------------------------------------------------------
	// Дата Excel — число дней от 30.12.1899 (как TDateTime); 0 / пусто -> нет.
	TDateTime ParseDate(const String &AText, const String &AHeader)
	{
		if (AText.IsEmpty())
			return 0.0;
		double value = 0.0;
		if (!TryStrToFloat(AText, value, TFormatSettings::Invariant()) || value < 0 || value > 2958465)
			throw ERowError(Format(Tr(L"Import.Error.BadDate"), ARRAYOFCONST((AHeader, AText))));
		return TDateTime(floor(value));	// время не нужно
	}
	//-----------------------------------------------------------------------
	// Число в текстовом поле (Var., Label): "1.0" -> "1".
	String NumberAsText(const String &AText)
	{
		double value = 0.0;
		if (!AText.IsEmpty() && TryStrToFloat(AText, value, TFormatSettings::Invariant()) && value == floor(value)
			&& fabs(value) < 1e9 && AText.Pos(L".") > 0)
			return IntToStr((int)value);
		return AText;
	}
}
//---------------------------------------------------------------------------
TCoinImporter::TCoinImporter(const std::vector<TCondition> &AConditions)
	: FDataRows(0)
{
	for (size_t i = 0; i < AConditions.size(); ++i)
		FConditionCodes.insert(AConditions[i].Code.UpperCase());
}
//---------------------------------------------------------------------------
TCoinImporter::~TCoinImporter()
{
}
//---------------------------------------------------------------------------
int TCoinImporter::Column(const String &AHeader) const
{
	std::map<String, int>::const_iterator it = FColumns.find(AHeader.UpperCase());
	return it == FColumns.end() ? -1 : it->second;
}
//---------------------------------------------------------------------------
String TCoinImporter::Cell(const std::vector<String> &ARow, const String &AHeader) const
{
	const int col = Column(AHeader);
	return col >= 0 && col < (int)ARow.size() ? ARow[col].Trim() : String();
}
//---------------------------------------------------------------------------
std::vector<TCoinRecord> TCoinImporter::Parse(TXlsxSource &ASource)
{
	FErrors.clear();
	FColumns.clear();
	FDataRows = 0;

	std::vector<TCoinRecord> result;
	if (ASource.RowCount() == 0)
		return result;

	// Первая строка — заголовки.
	const std::vector<String> header = ASource.Row(0);
	for (size_t i = 0; i < header.size(); ++i)
	{
		const String name = header[i].Trim().UpperCase();
		if (!name.IsEmpty() && FColumns.find(name) == FColumns.end())
			FColumns[name] = (int)i;
	}
	for (const wchar_t *required : cRequired)
		if (Column(required) < 0)
			throw Exception(Format(Tr(L"Import.Error.NoColumn"), ARRAYOFCONST((String(required)))));

	result.reserve(ASource.RowCount());
	for (int i = 1; i < ASource.RowCount(); ++i)
	{
		const std::vector<String> row = ASource.Row(i);
		bool empty = true;
		for (size_t c = 0; c < row.size() && empty; ++c)
			empty = row[c].Trim().IsEmpty();
		if (empty)
			continue;

		++FDataRows;
		try
		{
			TCoinRecord record;
			ParseRow(row, record);
			result.push_back(record);
		}
		catch (ERowError &e)
		{
			TImportError error;
			error.Row = ASource.RowNumber(i);
			error.Message = e.Message;
			FErrors.push_back(error);
		}
	}
	return result;
}
//---------------------------------------------------------------------------
void TCoinImporter::ParseRow(const std::vector<String> &ARow, TCoinRecord &R) const
{
	R.Country      = Cell(ARow, cCountry);
	R.Denomination = Cell(ARow, cDenomination);
	for (const wchar_t *required : cRequired)
		if (Cell(ARow, required).IsEmpty())
			throw ERowError(Format(Tr(L"Import.Error.Empty"), ARRAYOFCONST((String(required)))));

	R.Period          = Cell(ARow, cPeriod);
	R.Currency        = Cell(ARow, cCurrency);
	R.Year            = ParseInt(Cell(ARow, cYear), cYear, 0);
	R.Variety         = NumberAsText(Cell(ARow, cVariety));
	R.Subject         = Cell(ARow, cSubject);
	R.DiameterMm      = ParseNumber(Cell(ARow, cDiameter), cDiameter);
	R.CatalogValueUah = ParseNumber(Cell(ARow, cCatalogValue), cCatalogValue);
	R.CatalogNumber   = Cell(ARow, cNumber);
	R.Color           = Cell(ARow, cColor);
	R.Quantity        = ParseInt(Cell(ARow, cQuantity), cQuantity, 1);
	R.NeedToReplace   = !Cell(ARow, cNeedToReplace).IsEmpty();
	R.PublishedDate   = ParseDate(Cell(ARow, cPublished), cPublished);
	R.SwapInfo        = Cell(ARow, cSwap);
	R.PurchaseDate    = ParseDate(Cell(ARow, cPurchase), cPurchase);
	R.MyValueUah      = ParseNumber(Cell(ARow, cMyValue), cMyValue);
	R.GradingCompany  = Cell(ARow, cGradingCompany);
	R.GradingNumber   = Cell(ARow, cGradingNumber);
	R.Grade           = Cell(ARow, cGrade);
	R.LabelText       = NumberAsText(Cell(ARow, cLabel));
	R.Comment         = Cell(ARow, cComment);

	if (R.Quantity < 1)
		throw ERowError(Format(Tr(L"Import.Error.BadNumber"), ARRAYOFCONST((String(cQuantity), IntToStr(R.Quantity)))));

	const String condition = Cell(ARow, cCondition).UpperCase();
	if (!condition.IsEmpty() && FConditionCodes.find(condition) == FConditionCodes.end())
		throw ERowError(Format(Tr(L"Import.Error.UnknownCondition"), ARRAYOFCONST((Cell(ARow, cCondition)))));
	R.Condition = condition;
}
//---------------------------------------------------------------------------
