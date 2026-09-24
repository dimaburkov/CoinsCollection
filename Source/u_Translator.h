//---------------------------------------------------------------------------
// Локализация интерфейса: Lang\<code>.ini (UTF-8) рядом с exe.
// [Strings] — строки из кода (Tr), [<имя формы>] — Caption формы и
// <компонент>.Caption / <компонент>.Hint.
// Цепочка подстановки: текущий язык -> en.ini -> исходный текст (.dfm / ключ).
//---------------------------------------------------------------------------
#ifndef u_TranslatorH
#define u_TranslatorH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <System.IniFiles.hpp>
#include <Vcl.Forms.hpp>
#include <functional>
#include <map>
#include <memory>
#include <vector>
//---------------------------------------------------------------------------
struct TLanguageInfo
{
	String Code;       // en / ru / uk
	String Name;       // на самом языке: English / Русский / Українська
	String FileName;
};
//---------------------------------------------------------------------------
class TTranslator
{
public:
	static const wchar_t *const BaseLanguage;	// L"en"

	TTranslator();
	~TTranslator();

	// Найти языки в каталоге и загрузить базовый (en).
	void Load(const String &ALangDir);

	// Переключить язык; неизвестный код -> en. false, если пришлось откатиться на en.
	bool SetLanguage(const String &ACode);
	String Language() const { return FLanguage; }
	const std::vector<TLanguageInfo> &Languages() const { return FLanguages; }

	// Строка из [Strings]; параметры — на месте вызова через Format().
	String Tr(const String &AKey) const;

	// Перевести форму (Caption формы, Caption / Hint компонентов).
	void TranslateForm(TCustomForm *AForm);
	// Перевести все открытые формы и уведомить подписчиков.
	void TranslateAll();

	// Подписка на смену языка — для текстов, выставленных из кода.
	int  Subscribe(const std::function<void()> &AHandler);
	void Unsubscribe(int AToken);

private:
	typedef std::map<String, String> TTextMap;	// "<компонент>.<свойство>" -> исходный текст

	std::vector<TLanguageInfo>             FLanguages;
	std::unique_ptr<TMemIniFile>           FBase;       // en.ini
	std::unique_ptr<TMemIniFile>           FCurrent;    // текущий язык (nullptr для en)
	String                                 FLanguage;
	std::map<String, TTextMap>             FOriginals;  // имя формы -> исходные тексты .dfm
	std::map<int, std::function<void()> >  FHandlers;
	int                                    FNextToken;

	const TLanguageInfo *FindLanguage(const String &ACode) const;
	String Lookup(const String &ASection, const String &AKey) const;
	void   ReportMissingKeys() const;
};
//---------------------------------------------------------------------------
// Единственный экземпляр на приложение.
TTranslator &Translator();

// Короткая запись для строк из кода: Tr(L"Main.DbNotOpen").
inline String Tr(const String &AKey) { return Translator().Tr(AKey); }
//---------------------------------------------------------------------------
#endif
