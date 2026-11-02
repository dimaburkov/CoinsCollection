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
