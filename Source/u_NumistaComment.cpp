//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "u_NumistaComment.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
namespace
{
	bool IsCode(const String &AText)
	{
		if (AText.IsEmpty())
			return false;
		for (int i = 1; i <= AText.Length(); ++i)
		{
			const wchar_t c = AText[i];
			if (!((c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z')))
				return false;
		}
		return true;
	}
}
//---------------------------------------------------------------------------
TNumistaComment TNumistaComment::Parse(const String &AComment)
{
	TNumistaComment result;
	String rest = AComment;

	// Индекс числом в начале (задача 20): "97" или "97;текст" — R = 97; дальше без атрибутов —
	// весь остаток — свободный комментарий, с атрибутами — разбор как обычно (R: — важнее).
	{
		const int semicolon = rest.Pos(L";");
		const String first = (semicolon > 0 ? rest.SubString(1, semicolon - 1) : rest).Trim();
		bool digits = !first.IsEmpty() && first.Length() <= 3;
		for (int i = 1; digits && i <= first.Length(); ++i)
			digits = first[i] >= L'0' && first[i] <= L'9';
		const int rarity = digits ? StrToIntDef(first, -1) : -1;
		if (rarity >= 0 && rarity <= 100)
		{
			result.Rarity = rarity;
			rest = semicolon > 0 ? rest.SubString(semicolon + 1, rest.Length()) : String();
			const int colon = rest.Pos(L";") > 0 ? rest.SubString(1, rest.Pos(L";") - 1).Pos(L":") : rest.Pos(L":");
			const String part = rest.Pos(L";") > 0 ? rest.SubString(1, rest.Pos(L";") - 1) : rest;
			if (colon == 0 || !IsCode(part.SubString(1, colon - 1).Trim()))
			{
				result.Text = rest.Trim();
				return result;
			}
		}
	}

	while (!rest.Trim().IsEmpty())
	{
		const int semicolon = rest.Pos(L";");
		String part = semicolon > 0 ? rest.SubString(1, semicolon - 1) : rest;
		const int colon = part.Pos(L":");
		const String code = colon > 0 ? part.SubString(1, colon - 1).Trim().UpperCase() : String();

		// C — до конца строки, включая ";" и ":".
		if (code == L"C")
		{
			result.Text = rest.SubString(rest.Pos(L":") + 1, rest.Length()).Trim();
			break;
		}
		if (IsCode(code))
		{
			const String value = part.SubString(colon + 1, part.Length()).Trim();
			if (code == L"R")
			{
				const int rarity = StrToIntDef(value, -1);
				result.Rarity = rarity >= 0 && rarity <= 100 ? rarity : -1;
			}
			else if (code == L"S")
				result.Swap = value;
		}
		if (semicolon == 0)
			break;
		rest = rest.SubString(semicolon + 1, rest.Length());
	}
	return result;
}
//---------------------------------------------------------------------------
String TNumistaComment::Build(const String &ASwap, const String &AText)
{
	String result;
	if (!ASwap.Trim().IsEmpty())
		result = L"S:" + ASwap.Trim();
	if (!AText.Trim().IsEmpty())
		result += String(result.IsEmpty() ? L"" : L";") + L"C:" + AText.Trim();
	return result;
}
//---------------------------------------------------------------------------
String TNumistaComment::SelfTest()
{
	struct TCase { const wchar_t *Comment; int Rarity; const wchar_t *Swap; const wchar_t *Text; };
	const TCase cases[] =
	{
		{ L"R:9;S:Am___cos /uid124372",            9, L"Am___cos /uid124372", L"" },
		{ L" r : 12 ; c: a;b:c",                  12, L"",                    L"a;b:c" },
		{ L"R:9;Swap: Am___cos /uid124372",        9, L"",                    L"" },
		{ L"Swap: Am___cos /uid124372",           -1, L"",                    L"" },
		{ L"7;S:x",                                7, L"x",                   L"" },
		{ L"97",                                  97, L"",                    L"" },
		{ L"97;bought in Rome",                   97, L"",                    L"bought in Rome" },
		{ L" 0 ; a: b",                            0, L"",                    L"" },
		{ L"97;R:12",                             12, L"",                    L"" },
		{ L"150;x",                               -1, L"",                    L"" },
		{ L"1996",                                -1, L"",                    L"" },
		// Примеры владельца (задача 20): индекс 97 ...
		{ L"R:97",                                97, L"",                    L"" },
		{ L"R:97;",                               97, L"",                    L"" },
		{ L"R:97;C:Comm here",                    97, L"",                    L"Comm here" },
		{ L"R:97;S:Aru",                          97, L"Aru",                 L"" },
		{ L"97;",                                 97, L"",                    L"" },
		{ L"97;Comment",                          97, L"",                    L"Comment" },
		{ L"97;Swap Lenin",                       97, L"",                    L"Swap Lenin" },
		// ... и индекса нет
		{ L"97 Hello;",                           -1, L"",                    L"" },
		{ L"97 Hello",                            -1, L"",                    L"" },
		{ L"Hello 97",                            -1, L"",                    L"" },
		{ L"102",                                 -1, L"",                    L"" },
		{ L"R:abc",                               -1, L"",                    L"" },
		{ L"R:150",                               -1, L"",                    L"" },
		{ L"2 coins",                             -1, L"",                    L"" },
		{ L"",                                    -1, L"",                    L"" },
		{ L"S:x;C:y",                             -1, L"x",                   L"y" },
		{ L"C:R:5;S:z",                           -1, L"",                    L"R:5;S:z" },
	};
	for (const TCase &c : cases)
	{
		const TNumistaComment r = Parse(c.Comment);
		if (r.Rarity != c.Rarity || r.Swap != c.Swap || r.Text != c.Text)
			return Format(L"Parse(\"%s\") = R %d, S \"%s\", C \"%s\"",
				ARRAYOFCONST((c.Comment, r.Rarity, r.Swap, r.Text)));
	}
	if (Build(L"x", L"y") != L"S:x;C:y" || Build(L"", L"y") != L"C:y" || Build(L"x", L"") != L"S:x" || !Build(L"", L"").IsEmpty())
		return L"Build";
	const TNumistaComment back = Parse(Build(L"Adler /uid86829", L"a;b"));
	if (back.Swap != L"Adler /uid86829" || back.Text != L"a;b")
		return L"Parse(Build())";
	return String();
}
//---------------------------------------------------------------------------
