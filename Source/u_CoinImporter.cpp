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
	// Список обмена (задача 47): своя оценка и заметки для партнёров.
	const wchar_t *const cSwapValue      = L"Value, UAH";
	const wchar_t *const cInformation    = L"Information";
	const wchar_t *const cReserve        = L"Reserve";
	const wchar_t *const cPrivateInfo    = L"Information (Private)";

	const wchar_t *const cRequired[] = { cCountry, cDenomination };

	// Колонка, которую uCoin не переводит, — признак экспорта uCoin на другом языке; есть в экспорте
	// коллекции, обмена и желаемого (задача 47).
	const wchar_t *const cUntranslated[] = { cVariety };
	//-----------------------------------------------------------------------
	// Язык заголовков неанглийского экспорта uCoin: ключ перевода названия языка.
	String HeaderLanguage(const std::vector<String> &AHeader)
	{
		bool cyrillic = false;
		for (size_t i = 0; i < AHeader.size(); ++i)
		{
			const String text = AHeader[i];
			for (int c = 1; c <= text.Length(); ++c)
			{
				const wchar_t ch = text[c];
				// і ї є ґ (и прописные) — только в украинском.
				if (ch == 0x0456 || ch == 0x0457 || ch == 0x0454 || ch == 0x0491
					|| ch == 0x0406 || ch == 0x0407 || ch == 0x0404 || ch == 0x0490)
					return L"Import.Language.uk";
				if (ch >= 0x0400 && ch <= 0x04FF)
					cyrillic = true;
			}
		}
		return cyrillic ? L"Import.Language.ru" : L"Import.Language.Other";
	}

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
	// В списке обмена uCoin (задача 47) — текст «дд.мм.гггг».
	TDateTime ParseDate(const String &AText, const String &AHeader)
	{
		if (AText.IsEmpty())
			return 0.0;
		if (AText.Length() == 10 && AText[3] == L'.' && AText[6] == L'.')
		{
			TDateTime date;
			if (TryEncodeDate((Word)StrToIntDef(AText.SubString(7, 4), 0), (Word)StrToIntDef(AText.SubString(4, 2), 0),
				(Word)StrToIntDef(AText.SubString(1, 2), 0), date))
				return date;
			throw ERowError(Format(Tr(L"Import.Error.BadDate"), ARRAYOFCONST((AHeader, AText))));
		}
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
	: FDataRows(0),
	  FKind(ufCollection)
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
	FKind = ufCollection;

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
	// Вид файла (задача 47): у коллекции есть период; у обмена — заметки и бронь; у желаемого —
	// номер без количества.
	if (Column(cPeriod) < 0 && (Column(cReserve) >= 0 || Column(cInformation) >= 0))
		FKind = ufSwap;
	else if (Column(cPeriod) < 0 && Column(cNumber) >= 0 && Column(cQuantity) < 0)
		FKind = ufWish;

	for (const wchar_t *required : cRequired)
		if (Column(required) < 0)
		{
			// Экспорт uCoin не на английском: заголовки и данные (страны, периоды,
			// валюты) переведены — такой файл не принимаем, объясняем, как получить английский.
			bool ucoin = true;
			for (const wchar_t *untranslated : cUntranslated)
				ucoin = ucoin && Column(untranslated) >= 0;
			if (ucoin)
				throw Exception(Format(Tr(L"Import.Error.Language"), ARRAYOFCONST((Tr(HeaderLanguage(header)))))
					+ L" " + Tr(L"Import.Error.Language.Why") + sLineBreak + sLineBreak
					+ Tr(L"Import.Error.Language.Fix") + sLineBreak
					+ L"1. " + Tr(L"Import.Error.Language.Step1") + sLineBreak
					+ L"2. " + Tr(L"Import.Error.Language.Step2") + sLineBreak
					+ L"3. " + Tr(L"Import.Error.Language.Step3"));
			throw Exception(Format(Tr(L"Import.Error.NoColumn"), ARRAYOFCONST((String(required)))));
		}

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
	if (FKind == ufSwap)
	{
		// Обмен: оценка — «Value, UAH»; заметка, бронь и приватная заметка — в комментарий экземпляра.
		R.List            = L"swap";
		R.CatalogValueUah = ParseNumber(Cell(ARow, cSwapValue), cSwapValue);
		String comment = Cell(ARow, cInformation);
		const String reserve = Cell(ARow, cReserve);
		if (!reserve.IsEmpty())
			comment += (comment.IsEmpty() ? String() : String(L"; ")) + reserve;
		const String privateInfo = Cell(ARow, cPrivateInfo);
		if (!privateInfo.IsEmpty())
			comment += (comment.IsEmpty() ? String() : String(L"; "))
				+ Format(Tr(L"Import.Swap.Private"), ARRAYOFCONST((privateInfo)));
		R.Comment = comment;
	}

	if (R.Quantity < 1)
		throw ERowError(Format(Tr(L"Import.Error.BadNumber"), ARRAYOFCONST((String(cQuantity), IntToStr(R.Quantity)))));

	const String condition = Cell(ARow, cCondition).UpperCase();
	if (!condition.IsEmpty() && FConditionCodes.find(condition) == FConditionCodes.end())
		throw ERowError(Format(Tr(L"Import.Error.UnknownCondition"), ARRAYOFCONST((Cell(ARow, cCondition)))));
	R.Condition = condition;
}
//---------------------------------------------------------------------------
