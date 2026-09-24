//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <algorithm>
#include <System.IOUtils.hpp>
#include <System.TypInfo.hpp>
#include "u_Translator.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------

namespace
{
	const String cLanguageSection = L"Language";
	const String cStringsSection  = L"Strings";
	const wchar_t *const cTranslatedProps[] = { L"Caption", L"Hint" };

	std::unique_ptr<TMemIniFile> OpenIni(const String &AFileName)
	{
		return std::unique_ptr<TMemIniFile>(new TMemIniFile(AFileName, TEncoding::UTF8));
	}
	//-----------------------------------------------------------------------
	// Строковое published-свойство, которое можно переводить.
	bool HasStringProp(TObject *AObject, const String &AProp)
	{
		if (!IsPublishedProp(AObject, AProp))
			return false;
		const TTypeKind kind = PropType(AObject, AProp);
		return kind == tkUString || kind == tkLString || kind == tkWString || kind == tkString;
	}
	//-----------------------------------------------------------------------
	// Caption / Hint компонента с назначенным Action берутся из Action —
	// переводится само действие, а не компонент.
	bool HasAction(TComponent *AComponent)
	{
		return IsPublishedProp(AComponent, L"Action")
			&& PropType(AComponent, L"Action") == tkClass
			&& GetObjectProp(AComponent, L"Action") != nullptr;
	}
}
//---------------------------------------------------------------------------
const wchar_t *const TTranslator::BaseLanguage = L"en";
//---------------------------------------------------------------------------
TTranslator &Translator()
{
	static TTranslator translator;
	return translator;
}
//---------------------------------------------------------------------------
TTranslator::TTranslator()
	: FLanguage(BaseLanguage),
	  FNextToken(1)
{
}
//---------------------------------------------------------------------------
TTranslator::~TTranslator()
{
}
//---------------------------------------------------------------------------
void TTranslator::Load(const String &ALangDir)
{
	FLanguages.clear();
	FBase.reset();
	FCurrent.reset();
	FLanguage = BaseLanguage;

	if (!TDirectory::Exists(ALangDir))
		return;	// без файлов — исходные тексты .dfm и ключи

	const DynamicArray<String> files = TDirectory::GetFiles(ALangDir, L"*.ini");
	for (int i = 0; i < files.Length; ++i)
	{
		std::unique_ptr<TMemIniFile> ini = OpenIni(files[i]);
		TLanguageInfo info;
		info.Code     = ini->ReadString(cLanguageSection, L"Code", L"").Trim().LowerCase();
		info.Name     = ini->ReadString(cLanguageSection, L"Name", L"").Trim();
		info.FileName = files[i];
		if (info.Code.IsEmpty())
			continue;
		if (info.Name.IsEmpty())
			info.Name = info.Code;
		FLanguages.push_back(info);
	}

	// Базовый язык первым, остальные — по коду.
	std::sort(FLanguages.begin(), FLanguages.end(),
		[](const TLanguageInfo &A, const TLanguageInfo &B)
		{
			const bool aBase = A.Code == TTranslator::BaseLanguage;
			const bool bBase = B.Code == TTranslator::BaseLanguage;
			if (aBase != bBase)
				return aBase;
			return A.Code < B.Code;
		});

	if (const TLanguageInfo *base = FindLanguage(BaseLanguage))
		FBase = OpenIni(base->FileName);
}
//---------------------------------------------------------------------------
const TLanguageInfo *TTranslator::FindLanguage(const String &ACode) const
{
	for (size_t i = 0; i < FLanguages.size(); ++i)
		if (SameText(FLanguages[i].Code, ACode))
			return &FLanguages[i];
	return nullptr;
}
//---------------------------------------------------------------------------
bool TTranslator::SetLanguage(const String &ACode)
{
	const String code = ACode.Trim().LowerCase();
	const TLanguageInfo *info = FindLanguage(code);

	FCurrent.reset();
	if (!info || code == BaseLanguage)
	{
		FLanguage = BaseLanguage;
		return code == BaseLanguage;
	}

	FCurrent = OpenIni(info->FileName);
	FLanguage = info->Code;
#ifdef _DEBUG
	ReportMissingKeys();
#endif
	return true;
}
//---------------------------------------------------------------------------
// Пустое значение считается отсутствующим — пустых надписей не бывает.
String TTranslator::Lookup(const String &ASection, const String &AKey) const
{
	String value;
	if (FCurrent)
		value = FCurrent->ReadString(ASection, AKey, L"");
	if (value.IsEmpty() && FBase)
		value = FBase->ReadString(ASection, AKey, L"");
	return value;
}
//---------------------------------------------------------------------------
String TTranslator::Tr(const String &AKey) const
{
	const String value = Lookup(cStringsSection, AKey);
	return value.IsEmpty() ? AKey : value;
}
//---------------------------------------------------------------------------
void TTranslator::TranslateForm(TCustomForm *AForm)
{
	if (!AForm)
		return;
	const String section = AForm->Name;

	// При первом вызове запомнить исходные тексты .dfm.
	const bool first = FOriginals.find(section) == FOriginals.end();
	TTextMap &originals = FOriginals[section];
	if (first)
	{
		originals[L"Caption"] = AForm->Caption;
		for (int i = 0; i < AForm->ComponentCount; ++i)
		{
			TComponent *c = AForm->Components[i];
			if (c->Name.IsEmpty() || HasAction(c))
				continue;
			for (const wchar_t *prop : cTranslatedProps)
				if (HasStringProp(c, prop))
					originals[c->Name + L"." + prop] = GetStrProp(c, prop);
		}
	}

	const String caption = Lookup(section, L"Caption");
	AForm->Caption = caption.IsEmpty() ? originals[L"Caption"] : caption;

	for (int i = 0; i < AForm->ComponentCount; ++i)
	{
		TComponent *c = AForm->Components[i];
		if (c->Name.IsEmpty() || HasAction(c))
			continue;
		for (const wchar_t *prop : cTranslatedProps)
		{
			const String key = c->Name + L"." + prop;
			TTextMap::const_iterator orig = originals.find(key);
			if (orig == originals.end())
				continue;
			const String value = Lookup(section, key);
			SetStrProp(c, prop, value.IsEmpty() ? orig->second : value);
		}
	}
}
//---------------------------------------------------------------------------
void TTranslator::TranslateAll()
{
	for (int i = 0; i < Screen->FormCount; ++i)
		TranslateForm(Screen->Forms[i]);

	// Копия: обработчик может отписаться во время вызова.
	const std::map<int, std::function<void()> > handlers = FHandlers;
	for (std::map<int, std::function<void()> >::const_iterator it = handlers.begin(); it != handlers.end(); ++it)
		it->second();
}
//---------------------------------------------------------------------------
int TTranslator::Subscribe(const std::function<void()> &AHandler)
{
	const int token = FNextToken++;
	FHandlers[token] = AHandler;
	return token;
}
//---------------------------------------------------------------------------
void TTranslator::Unsubscribe(int AToken)
{
	FHandlers.erase(AToken);
}
//---------------------------------------------------------------------------
// Debug: ключи, которые есть в en.ini, но отсутствуют в текущем переводе.
void TTranslator::ReportMissingKeys() const
{
	if (!FBase || !FCurrent)
		return;

	std::unique_ptr<TStringList> sections(new TStringList());
	std::unique_ptr<TStringList> keys(new TStringList());
	FBase->ReadSections(sections.get());
	for (int s = 0; s < sections->Count; ++s)
	{
		if (sections->Strings[s] == cLanguageSection)
			continue;
		FBase->ReadSection(sections->Strings[s], keys.get());
		for (int k = 0; k < keys->Count; ++k)
			if (FCurrent->ReadString(sections->Strings[s], keys->Strings[k], L"").IsEmpty())
			{
				const String msg = L"[Translator] " + FLanguage + L".ini: missing ["
					+ sections->Strings[s] + L"] " + keys->Strings[k];
				OutputDebugStringW(msg.c_str());
			}
	}
}
//---------------------------------------------------------------------------
