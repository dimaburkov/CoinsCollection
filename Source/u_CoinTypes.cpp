//---------------------------------------------------------------------------
// u_CoinTypes.h — типы данных; здесь — показ года (YearText).
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
