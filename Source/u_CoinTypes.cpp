//---------------------------------------------------------------------------
// u_CoinTypes.h — типы данных; здесь — показ года (YearText), шкала состояний цен Numista.
//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "u_CoinTypes.h"
#include "u_Translator.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
namespace
{
	String OneYear(int AYear)
	{
		return AYear < 0 ? Format(Tr(L"Year.BC"), ARRAYOFCONST((-AYear))) : IntToStr(AYear);
	}
}
//---------------------------------------------------------------------------
String YearText(int AYear, int AYearTo)
{
	if (AYear == 0)
		return String();
	if (AYearTo == 0 || AYearTo == AYear)
		return OneYear(AYear);
	return OneYear(AYear) + L"\u2013" + OneYear(AYearTo);
}
//---------------------------------------------------------------------------
String NumistaTitleExtra(const String &ATitle)
{
	const String title = ATitle.Trim();
	// 1) « - » вне скобок: правитель, сюжет и т. п. — всё после первого такого.
	int depth = 0;
	for (int i = 1; i <= title.Length(); ++i)
	{
		const wchar_t ch = title[i];
		if (ch == L'(')
			++depth;
		else if (ch == L')' && depth > 0)
			--depth;
		else if (depth == 0 && ch == L'-' && i > 1 && i < title.Length() && title[i - 1] == L' ' && title[i + 1] == L' ')
			return title.SubString(i + 2, title.Length()).Trim();
	}
	// 2) Скобки в конце: разновидность — их содержимое.
	if (title.Length() > 2 && title[title.Length()] == L')')
	{
		depth = 0;
		for (int i = title.Length(); i >= 1; --i)
		{
			if (title[i] == L')')
				++depth;
			else if (title[i] == L'(' && --depth == 0)
				return title.SubString(i + 1, title.Length() - i - 1).Trim();
		}
	}
	return String();
}
//---------------------------------------------------------------------------
namespace
{
	const wchar_t *const cGradeCodes[TNumistaPrices::GradeCount] = { L"g", L"vg", L"f", L"vf", L"xf", L"au", L"unc" };
}
//---------------------------------------------------------------------------
String TNumistaPrices::GradeCode(int AIndex)
{
	return AIndex >= 0 && AIndex < GradeCount ? String(cGradeCodes[AIndex]) : String();
}
//---------------------------------------------------------------------------
int TNumistaPrices::GradeIndex(const String &ACondition)
{
	const String c = ACondition.Trim().LowerCase();
	for (int i = 0; i < GradeCount; ++i)
		if (c == cGradeCodes[i])
			return i;
	return -1;
}
//---------------------------------------------------------------------------
double TNumistaPrices::ForCondition(const String &ACondition) const
{
	const int i = GradeIndex(ACondition);
	return i < 0 ? 0.0 : Price[i];
}
//---------------------------------------------------------------------------
bool TNumistaPrices::Empty() const
{
	for (int i = 0; i < GradeCount; ++i)
		if (Price[i] > 0.0)
			return false;
	return true;
}
//---------------------------------------------------------------------------
