//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include <algorithm>
#include <set>
#include <shlwapi.h>
#include <shellapi.h>
#include <Vcl.Dialogs.hpp>
#include <Vcl.Themes.hpp>
#include "f_Main.h"
#include "dm_Data.h"
#include "u_AppConfig.h"
#include "u_Translator.h"
#include "u_ImportSource.h"
#include "u_CoinImporter.h"
#include "f_CoinEdit.h"
#include "f_Reference.h"
#include "f_NumistaSettings.h"
#include "f_NumistaChoose.h"
#include "f_NumistaIssue.h"
#include "u_NumistaClient.h"
#include "u_NumistaMatcher.h"
#include "u_NumistaUploader.h"
#include "u_NumistaComment.h"
#include "versionConfig.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
#pragma comment(lib, "shlwapi")
TfmMain *fmMain;
//---------------------------------------------------------------------------
namespace
{
	// Ключи заголовков колонок (порядок = TItemColumn).
	const wchar_t *const cColumnKeys[] =
	{
		L"", L"Main.Column.Country", L"Main.Column.Period", L"Main.Column.Currency", L"Main.Column.Denomination",
		L"Main.Column.Numista", L"Main.Column.Uploaded", L"Main.Column.Rarity", L"Main.Column.Mintage",
		L"Main.Column.Composition",
		L"Main.Column.Year", L"Main.Column.Condition", L"Main.Column.Value"
	};
	//-----------------------------------------------------------------------
	// Строки — «как в Проводнике»: без учёта регистра, числа внутри по значению
	// ("2 hryvni" < "10 hryven", "KM# 9" < "KM# 10").
	int CompareStrings(const String &A, const String &B)
	{
		return StrCmpLogicalW(A.c_str(), B.c_str());
	}
	// Ширина колонок по умолчанию (порядок = TItemColumn).
	// Auto-link checked: больше стольких монет — подтверждение.
	const int cConfirmCheckCoins = 10;
	const int cDefaultWidths[] = { 22, 130, 160, 120, 130, 70, 24, 70, 85, 120, 50, 65, 85 };
	// Подписи строк панели подробностей (порядок = TDetailRowIndex).
	const wchar_t *const cDetailKeys[] =
	{
		L"Main.Details.Period", L"Main.Details.Currency", L"Main.Details.Ruler", L"Main.Details.Subject",
		L"Main.Details.Number", L"Main.Details.Numista", L"Main.Details.NumistaItem", L"Main.Details.IssueComment",
		L"Main.Details.Weight", L"Main.Details.Size", L"Main.Details.Shape", L"Main.Details.Technique",
		L"Main.Details.Series", L"Main.Details.Commemorated", L"Main.Details.TypeYears", L"Main.Details.Mints",
		L"Main.Details.Category", L"Main.Details.Demonetized"
	};
	//-----------------------------------------------------------------------
	template <typename T>
	int CompareValues(T A, T B)
	{
		return A < B ? -1 : (B < A ? 1 : 0);
	}
	//-----------------------------------------------------------------------
	// Тираж с разделителями тысяч; "" — неизвестен.
	String MintageText(__int64 AMintage)
	{
		return AMintage < 0 ? String() : FormatFloat(L"#,##0", (double)AMintage);
	}
	//-----------------------------------------------------------------------
	// Ключ сортировки тиража: выпуск не загружен (-2) < у Numista тиража нет (-1) < тираж.
	__int64 MintageKey(const TCoinRecord &R)
	{
		return R.NumistaMintage >= 0 ? R.NumistaMintage : (R.NumistaIssueLoaded ? -1 : -2);
	}
	//-----------------------------------------------------------------------
	// Число без лишних нулей: 6.5, 21.25.
	String NumberText(double AValue)
	{
		return FormatFloat(L"0.##", AValue);
	}
	//-----------------------------------------------------------------------
	// Выпуски типа — в numista_issues (задача 13c). Вызывается, когда выпуск уже
	// определён: ответ getIssues в кэше, запроса нет.
	void StoreIssues(TCoinRepository &ARepository, TNumistaMatcher &AMatcher, int ATypeId)
	{
		ARepository.SaveNumistaIssues(ATypeId, AMatcher.LoadIssues(ATypeId));
	}
}
//---------------------------------------------------------------------------
__fastcall TfmMain::TfmMain(TComponent* Owner)
	: TForm(Owner),
	  FSortColumn(icYear),
	  FSortAscending(true),
	  FLanguageToken(0),
	  FMouseDownRow(-1),
	  FDetailsTypeId(0),
	  FBuildingTree(false),
	  FCheckedNode(nullptr)
{
	for (int c = 0; c < icCount; ++c)
		FColWidths[c] = cDefaultWidths[c];
	UpdateColumns();
	CreateDetailRows();

	Translator().TranslateForm(this);
	BuildLanguageMenu();
	ApplyTexts();
	FLanguageToken = Translator().Subscribe([this]() { ApplyTexts(); });
}
//---------------------------------------------------------------------------
__fastcall TfmMain::~TfmMain()
{
	Translator().Unsubscribe(FLanguageToken);
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::FormCreate(TObject *Sender)
{
	FDatabase.reset(new TDatabase(dmData->FDConnection));
	FDatabase->Open(AppConfig().DbPath);
	FDatabase->EnsureSchema();

	FRepository.reset(new TCoinRepository(FDatabase.get()));
	LoadItems();
#ifdef _DEBUG
	const String selfTest = TNumistaComment::SelfTest();
	if (!selfTest.IsEmpty())
		Application->MessageBox((L"TNumistaComment::SelfTest\r\n\r\n" + selfTest).c_str(), L"Debug", MB_OK | MB_ICONERROR);
#endif
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::FormDestroy(TObject *Sender)
{
	FItems.clear();
	FAll.clear();
	FRepository.reset();
	if (FDatabase)
		FDatabase->Close();
	FDatabase.reset();
}
//---------------------------------------------------------------------------
// Тексты, которые не берутся из .dfm напрямую: короткая подпись кнопки
// импорта, строка состояния; заголовки колонок рисуются в dgItemsDrawCell.
// Вызывается и при смене языка.
void TfmMain::ApplyTexts()
{
	// Кнопка связана с actImport, но подпись у неё короче, чем пункт меню.
	tbImport->Caption = Tr(L"Main.Toolbar.Import");
	edSearch->TextHint = Tr(L"Main.Search.Hint");
	lblObverse->Caption     = Tr(L"Main.Details.Obverse");
	lblReverse->Caption     = Tr(L"Main.Details.Reverse");
	lblObverseNone->Caption = Tr(L"Main.Details.NoPhoto");
	lblReverseNone->Caption = Tr(L"Main.Details.NoPhoto");
	for (size_t i = 0; i < FDetailRows.size(); ++i)
		FDetailRows[i].Caption->Caption = Tr(FDetailRows[i].Key);
	UpdateDetails();
	if (FRepository)
		BuildTree();	// подписи узлов All / No continent / Extinct states
	dgItems->Invalidate();
	UpdateStatus();
}
//---------------------------------------------------------------------------
// Подпункты Settings -> Language: по одному на найденный Lang\*.ini.
// Названия языков пишутся на самом языке и не переводятся.
void TfmMain::BuildLanguageMenu()
{
	miLanguage->Clear();
	const std::vector<TLanguageInfo> &languages = Translator().Languages();
	for (size_t i = 0; i < languages.size(); ++i)
	{
		TMenuItem *item = new TMenuItem(miLanguage);
		item->Caption    = languages[i].Name;
		item->Tag        = (NativeInt)i;
		item->RadioItem  = true;
		item->GroupIndex = 1;
		item->Checked    = SameText(languages[i].Code, Translator().Language());
		item->OnClick    = LanguageClick;
		miLanguage->Add(item);
	}
	miLanguage->Enabled = !languages.empty();
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::LanguageClick(TObject *Sender)
{
	TMenuItem *item = static_cast<TMenuItem *>(Sender);
	const std::vector<TLanguageInfo> &languages = Translator().Languages();
	const size_t index = (size_t)item->Tag;
	if (index >= languages.size())
		return;

	Translator().SetLanguage(languages[index].Code);
	Translator().TranslateAll();
	item->Checked = true;

	AppConfig().Language = Translator().Language();
	AppConfig().Save();
}
//---------------------------------------------------------------------------
// Импорт экспорта uCoin (.xlsx): разбор -> подтверждение -> копия БД -> замена коллекции.
void __fastcall TfmMain::actImportExecute(TObject *Sender)
{
	const String title = Tr(L"Import.Title");
	const String nl = L"\r\n";

	std::unique_ptr<TOpenDialog> dialog(new TOpenDialog(this));
	dialog->Title      = title;
	dialog->Filter     = Tr(L"Import.FileFilter");
	dialog->DefaultExt = L"xlsx";
	dialog->InitialDir = TAppConfig::ExeDir();
	dialog->Options    = dialog->Options << ofFileMustExist << ofPathMustExist;
	if (!dialog->Execute())
		return;

	// 1. Разбор файла.
	std::vector<TCoinRecord> records;
	std::vector<TImportError> errors;
	int dataRows = 0;
	const TCursor oldCursor = Screen->Cursor;
	Screen->Cursor = crHourGlass;
	try
	{
		TXlsxSource source;
		source.Open(dialog->FileName);
		TCoinImporter importer(FRepository->Lookups().Conditions());
		records  = importer.Parse(source);
		errors   = importer.Errors();
		dataRows = importer.DataRowCount();
		Screen->Cursor = oldCursor;
	}
	catch (Exception &e)
	{
		Screen->Cursor = oldCursor;
		Application->MessageBox((Tr(L"Import.Error.Read") + nl + nl + e.Message).c_str(), title.c_str(), MB_OK | MB_ICONERROR);
		return;
	}

	// 2. Итог разбора и подтверждение.
	String text = ExtractFileName(dialog->FileName) + nl + nl
		+ Format(Tr(L"Import.Summary.Rows"),   ARRAYOFCONST((dataRows))) + nl
		+ Format(Tr(L"Import.Summary.Items"),  ARRAYOFCONST(((int)records.size()))) + nl
		+ Format(Tr(L"Import.Summary.Errors"), ARRAYOFCONST(((int)errors.size())));
	if (!errors.empty())
	{
		text += nl + nl + Tr(L"Import.Summary.FirstErrors");
		for (size_t i = 0; i < errors.size() && i < 10; ++i)
			text += nl + Format(Tr(L"Import.ErrorLine"), ARRAYOFCONST((errors[i].Row, errors[i].Message)));
	}
	if (records.empty())
	{
		Application->MessageBox((text + nl + nl + Tr(L"Import.Nothing")).c_str(), title.c_str(), MB_OK | MB_ICONWARNING);
		return;
	}

	const int current = FRepository->ItemCount();
	if (current > 0)
		text += nl + nl + Format(Tr(L"Import.Confirm.Replace"), ARRAYOFCONST((current)));
	text += nl + nl + Tr(L"Import.Confirm.Question");
	if (!Confirm(text, title))
		return;

	// 3. Копия БД перед заменой непустой коллекции, затем замена в одной транзакции.
	Screen->Cursor = crHourGlass;
	int saved = 0;
	TNumistaKept kept;
	try
	{
		if (current > 0)
			FDatabase->Backup(L"before-import-" + FormatDateTime(L"yyyymmdd-hhnnss", Now()));
		saved = FRepository->ReplaceAll(records, &kept);
		Screen->Cursor = oldCursor;
	}
	catch (Exception &e)
	{
		Screen->Cursor = oldCursor;
		Application->MessageBox((Tr(L"Import.Error.Failed") + nl + nl + e.Message).c_str(), title.c_str(), MB_OK | MB_ICONERROR);
		return;
	}

	LoadItems();
	std::set<int> coins;
	for (size_t i = 0; i < FItems.size(); ++i)
		coins.insert(FItems[i].CoinId);
	String done = Format(Tr(L"Import.Done"), ARRAYOFCONST((saved, (int)coins.size(), (int)errors.size())));
	// Связи Numista, перенесённые на новые монеты (задача 13b).
	if (kept.Coins + kept.CoinsLost > 0)
		done += nl + nl + Format(Tr(L"Import.Numista.Kept"), ARRAYOFCONST((kept.Coins, kept.Items))) + nl
			+ Format(Tr(L"Import.Numista.Lost"), ARRAYOFCONST((kept.CoinsLost, kept.ItemsLost)));
	Application->MessageBox(done.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
}
//---------------------------------------------------------------------------
bool TfmMain::Confirm(const String &AText, const String &ACaption)
{
	return Application->MessageBox(AText.c_str(), ACaption.c_str(),
		MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES;
}
//---------------------------------------------------------------------------
// Сохранить запись из карточки; ошибка БД — сообщение, false.
bool TfmMain::SaveRecord(TCoinRecord &ARecord)
{
	try
	{
		FRepository->Save(ARecord);
		return true;
	}
	catch (Exception &e)
	{
		Application->MessageBox((Tr(L"Main.Error.Save") + L"\r\n\r\n" + e.Message).c_str(),
			StripHotkey(actEdit->Caption).c_str(), MB_OK | MB_ICONERROR);
		return false;
	}
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::actAddExecute(TObject *Sender)
{
	TCoinRecord record;
	if (!TfmCoinEdit::Execute(this, *FRepository, record) || !SaveRecord(record))
		return;

	LoadItems();
	SelectId(record.Id);
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::actEditExecute(TObject *Sender)
{
	const int index = CurrentIndex();
	if (index < 0)
		return;

	TCoinRecord record = FItems[index];
	if (TfmCoinEdit::Execute(this, *FRepository, record) && SaveRecord(record))
	{
		LoadItems();
		SelectId(record.Id);
	}
}
//---------------------------------------------------------------------------
// Пока фокус в поле поиска, Enter и Del нужны самому полю — действия с этими
// клавишами выключены (выключенное действие не перехватывает свою клавишу).
void __fastcall TfmMain::actEditUpdate(TObject *Sender)
{
	actEdit->Enabled = CurrentIndex() >= 0 && ActiveControl != edSearch;
}
//---------------------------------------------------------------------------
// Удаляется одна текущая запись; курсор остаётся на той же позиции списка.
void __fastcall TfmMain::actDeleteExecute(TObject *Sender)
{
	const int index = CurrentIndex();
	if (index < 0)
		return;

	const TCoinRecord &r = FItems[index];
	String what = r.Denomination;
	if (r.Year != 0)
		what += L", " + IntToStr(r.Year);
	if (!r.CatalogNumber.IsEmpty())
		what += L", " + r.CatalogNumber;
	if (!Confirm(Format(Tr(L"Main.Delete.Confirm"), ARRAYOFCONST((what))), StripHotkey(actDelete->Caption)))
		return;

	try
	{
		FRepository->Delete(r.Id);
	}
	catch (Exception &e)
	{
		Application->MessageBox((Tr(L"Main.Error.Delete") + L"\r\n\r\n" + e.Message).c_str(),
			StripHotkey(actDelete->Caption).c_str(), MB_OK | MB_ICONERROR);
	}
	LoadItems();
	SelectIndex(index);
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::actDeleteUpdate(TObject *Sender)
{
	actDelete->Enabled = CurrentIndex() >= 0 && ActiveControl != edSearch;
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::actNeedToReplaceExecute(TObject *Sender)
{
	ApplyFilter();	// флажок переключён AutoCheck до вызова
}
//---------------------------------------------------------------------------
// Reference -> Countries / Periods / Currencies: одна форма, своя вкладка.
// После изменений перечитать — страна могла переехать в другой узел дерева.
void __fastcall TfmMain::actReferenceExecute(TObject *Sender)
{
	const int tab = Sender == actPeriods ? 1 : (Sender == actCurrencies ? 2 : 0);
	if (TfmReference::Execute(this, FRepository->Lookups(), tab))
		LoadItems();
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::actNumistaSettingsExecute(TObject *Sender)
{
	TfmNumistaSettings::Execute(this);
}
//---------------------------------------------------------------------------
// Коды эмитентов Numista по странам (после EnsureCountryIssuers).
std::map<String, String> TfmMain::NumistaIssuers()
{
	std::map<String, String> result;
	const std::vector<TCountry> countries = FRepository->Lookups().Countries();
	for (size_t i = 0; i < countries.size(); ++i)
		result[AnsiUpperCase(countries[i].Name)] = countries[i].NumistaIssuer;
	return result;
}
//---------------------------------------------------------------------------
String TfmMain::CoinText(const TCoinRecord &ARecord)
{
	String text = ARecord.Country + L", " + ARecord.Denomination;
	if (ARecord.Year != 0)
		text += L", " + IntToStr(ARecord.Year);
	if (!ARecord.Variety.IsEmpty())
		text += L" " + ARecord.Variety;
	if (!ARecord.CatalogNumber.IsEmpty())
		text += L", " + ARecord.CatalogNumber;
	return text;
}
//---------------------------------------------------------------------------
String TfmMain::NumistaRequestsText(int ASent) const
{
	return Format(Tr(L"Numista.Requests"), ARRAYOFCONST((ASent,
		TNumistaClient::RequestsThisMonth(), AppConfig().NumistaMonthlyQuota)));
}
//---------------------------------------------------------------------------
// Numista → Link manually: тип для текущей монеты (один результат — сразу,
// несколько — выбор), затем выпуск. Документация: searchTypes, getIssues
// (https://en.numista.com/api/doc/index.php).
void __fastcall TfmMain::actNumistaFindExecute(TObject *Sender)
{
	const int index = CurrentIndex();
	if (index < 0)
		return;
	const TCoinRecord coin = FItems[index];
	const String caption = StripHotkey(actNumistaFind->Caption);

	TNumistaClient client;
	TNumistaMatcher matcher(client);
	if (!matcher.LoadReferences())
	{
		Application->MessageBox(Tr(L"Numista.Error.NoLists").c_str(), caption.c_str(), MB_OK | MB_ICONWARNING);
		return;
	}

	String message;
	UINT icon = MB_ICONINFORMATION;
	try
	{
		std::vector<TNumistaType> types;
		const TCursor saved = Screen->Cursor;
		Screen->Cursor = crHourGlass;
		try
		{
			matcher.EnsureCountryIssuers(FRepository->Lookups());
			types = matcher.Search(coin, NumistaIssuers()[AnsiUpperCase(coin.Country)]);
		}
		__finally
		{
			Screen->Cursor = saved;
		}

		int typeId = 0;
		if (types.size() == 1)
			typeId = types[0].Id;
		else if (types.size() > 1)
			typeId = TfmNumistaChoose::Execute(this,
				Format(Tr(L"NumistaChoose.Found"), ARRAYOFCONST(((int)types.size(), CoinText(coin)))), types);

		if (types.empty())
		{
			message = Format(Tr(L"Numista.Find.NotFound"), ARRAYOFCONST((CoinText(coin))));
			icon = MB_ICONWARNING;
		}
		else if (typeId == 0)
			message = Tr(L"Numista.Find.Cancelled");
		else
		{
			// Выпуск не определился однозначно — выбор вручную (Cancel — только тип).
			int issueId = matcher.FindIssue(typeId, coin);
			if (issueId == 0)
			{
				const std::vector<TNumistaIssue> issues = matcher.LoadIssues(typeId);
				if (!issues.empty())
					issueId = std::max(0, TfmNumistaIssue::Execute(this, CoinText(coin), typeId, coin.Year, issues, 0));
			}
			FRepository->SetCoinNumista(coin.CoinId, typeId, issueId);
			if (issueId != 0)
				StoreIssues(*FRepository, matcher, typeId);
			String title;
			for (size_t i = 0; i < types.size(); ++i)
				if (types[i].Id == typeId)
					title = types[i].Title;
			message = Format(Tr(L"Numista.Find.Linked"), ARRAYOFCONST((typeId, title))) + L"\r\n"
				+ (issueId != 0 ? Tr(L"Numista.Find.IssueFound") : Tr(L"Numista.Find.IssueNotFound"));
			// Характеристики типа сразу (getType, задача 13d): связь уже сохранена,
			// ошибка здесь — только примечание в итоге.
			try
			{
				FRepository->SaveNumistaType(matcher.LoadType(typeId));
				message += L"\r\n" + Tr(L"Numista.Find.TypeLoaded");
			}
			catch (Exception &e)
			{
				message += L"\r\n" + Tr(L"Numista.Find.TypeFailed") + L" " + e.Message;
			}
		}
	}
	catch (Exception &e)
	{
		message = e.Message;
		icon = MB_ICONERROR;
	}

	LoadItems();
	Application->MessageBox((message + L"\r\n\r\n" + NumistaRequestsText(client.RequestsMade)).c_str(),
		caption.c_str(), MB_OK | icon);
}
//---------------------------------------------------------------------------
// Choose issue: выпуск для монеты, уже связанной с типом (getIssues; из кэша —
// без запросов). Type only — снять выпуск, оставить тип.
void __fastcall TfmMain::actNumistaIssueExecute(TObject *Sender)
{
	const int index = CurrentIndex();
	if (index < 0 || FItems[index].NumistaTypeId == 0)
		return;
	const TCoinRecord coin = FItems[index];
	const String caption = StripHotkey(actNumistaIssue->Caption);

	TNumistaClient client;
	TNumistaMatcher matcher(client);
	try
	{
		std::vector<TNumistaIssue> issues;
		const TCursor saved = Screen->Cursor;
		Screen->Cursor = crHourGlass;
		try
		{
			issues = matcher.LoadIssues(coin.NumistaTypeId);
		}
		__finally
		{
			Screen->Cursor = saved;
		}
		if (issues.empty())
		{
			Application->MessageBox((Tr(L"Numista.Issue.None") + L"\r\n\r\n" + NumistaRequestsText(client.RequestsMade)).c_str(),
				caption.c_str(), MB_OK | MB_ICONINFORMATION);
			return;
		}
		const int issueId = TfmNumistaIssue::Execute(this, CoinText(coin), coin.NumistaTypeId, coin.Year,
			issues, coin.NumistaIssueId);
		if (issueId < 0)
			return;
		FRepository->SetCoinNumista(coin.CoinId, coin.NumistaTypeId, issueId);
		FRepository->SaveNumistaIssues(coin.NumistaTypeId, issues);
	}
	catch (Exception &e)
	{
		Application->MessageBox((e.Message + L"\r\n\r\n" + NumistaRequestsText(client.RequestsMade)).c_str(),
			caption.c_str(), MB_OK | MB_ICONERROR);
		return;
	}
	LoadItems();
}
//---------------------------------------------------------------------------
// Auto-link checked: отмеченные монеты без N# и с номером KM / Y / C; связывается
// только однозначный результат. Больше 10 монет — подтверждение с оценкой запросов;
// остановка на ошибке (429). Со связанных монет флажок снимается.
void __fastcall TfmMain::actNumistaCheckExecute(TObject *Sender)
{
	const String caption = StripHotkey(Tr(L"Main.Numista.CheckTitle"));
	TNumistaClient client;
	TNumistaMatcher matcher(client);
	if (!matcher.LoadReferences())
	{
		Application->MessageBox(Tr(L"Numista.Error.NoLists").c_str(), caption.c_str(), MB_OK | MB_ICONWARNING);
		return;
	}

	// Кандидаты — по одной записи на монету (экземпляры одной монеты — один тип).
	// Уже связанные и без номера KM / Y / C — пропускаются без запросов.
	std::vector<TCoinRecord> coins;
	std::set<int> seen, linkedBefore, noNumber;
	for (size_t i = 0; i < FAll.size(); ++i)
	{
		const TCoinRecord &r = FAll[i];
		if (!FChecked.count(r.Id))
			continue;
		int catalogueId = 0;
		String number;
		if (r.NumistaTypeId != 0)
			linkedBefore.insert(r.CoinId);
		else if (!matcher.ParseCatalogNumber(r.CatalogNumber, catalogueId, number))
			noNumber.insert(r.CoinId);
		else if (seen.insert(r.CoinId).second)
			coins.push_back(r);
	}
	const String skipped = Format(Tr(L"Numista.Check.AlreadyLinked"), ARRAYOFCONST(((int)linkedBefore.size()))) + L"\r\n"
		+ Format(Tr(L"Numista.Check.NoNumber"), ARRAYOFCONST(((int)noNumber.size())));
	if (coins.empty())
	{
		Application->MessageBox((Tr(L"Numista.Check.Nothing") + L"\r\n\r\n" + skipped).c_str(), caption.c_str(),
			MB_OK | MB_ICONINFORMATION);
		return;
	}

	// Монеты выбраны флажками — лимит не спрашивается. Много отмеченных (например, флажок
	// в заголовке на узле All) — подтверждение с оценкой: обычно 2 запроса на монету.
	const int count = (int)coins.size();
	if (count > cConfirmCheckCoins && !Confirm(Format(Tr(L"Numista.Check.Confirm"), ARRAYOFCONST((count, count * 2,
		TNumistaClient::RequestsThisMonth(), AppConfig().NumistaMonthlyQuota))), caption))
		return;
	client.RequestBudget = count * 3;	// не больше 3 запросов на монету: поиск, повтор без issuer, выпуски

	int linked = 0, withIssue = 0, ambiguous = 0, notFound = 0, processed = 0;
	std::set<int> linkedCoins;
	String stopReason;
	const TCursor saved = Screen->Cursor;
	Screen->Cursor = crHourGlass;
	try
	{
		matcher.EnsureCountryIssuers(FRepository->Lookups());
		std::map<String, String> issuers = NumistaIssuers();
		for (size_t i = 0; i < coins.size() && stopReason.IsEmpty(); ++i)
		{
			const TCoinRecord &coin = coins[i];
			StatusBar->Panels->Items[0]->Text = Format(Tr(L"Numista.Match.Progress"),
				ARRAYOFCONST(((int)i + 1, (int)coins.size())));
			StatusBar->Update();
			try
			{
				const std::vector<TNumistaType> types = matcher.Search(coin, issuers[AnsiUpperCase(coin.Country)]);
				if (types.size() == 1)
				{
					int issueId = 0;
					try
					{
						issueId = matcher.FindIssue(types[0].Id, coin);
					}
					catch (ENumistaError &e)
					{
						stopReason = e.Message;	// тип найден — сохраняем без выпуска и останавливаемся
					}
					FRepository->SetCoinNumista(coin.CoinId, types[0].Id, issueId);
					if (issueId != 0)
						StoreIssues(*FRepository, matcher, types[0].Id);
					linkedCoins.insert(coin.CoinId);
					++linked;
					if (issueId != 0)
						++withIssue;
				}
				else if (types.size() > 1)
					++ambiguous;
				else
					++notFound;
				++processed;
			}
			catch (ENumistaError &e)
			{
				stopReason = e.Message;
			}
		}
	}
	catch (Exception &e)
	{
		stopReason = e.Message;
	}
	Screen->Cursor = saved;

	// Со связанных монет флажок снимается (со всех её экземпляров).
	for (size_t i = 0; i < FAll.size(); ++i)
		if (linkedCoins.count(FAll[i].CoinId))
			FChecked.erase(FAll[i].Id);
	LoadItems();	// и строка состояния
	String report = Format(Tr(L"Numista.Match.Linked"), ARRAYOFCONST((linked, withIssue))) + L"\r\n"
		+ Format(Tr(L"Numista.Match.Ambiguous"), ARRAYOFCONST((ambiguous))) + L"\r\n"
		+ Format(Tr(L"Numista.Match.NotFound"), ARRAYOFCONST((notFound))) + L"\r\n"
		+ Format(Tr(L"Numista.Match.NotProcessed"), ARRAYOFCONST(((int)coins.size() - processed))) + L"\r\n"
		+ skipped;
	if (!stopReason.IsEmpty())
		report += L"\r\n\r\n" + Tr(L"Numista.Match.Stopped") + L" " + stopReason;
	Application->MessageBox((report + L"\r\n\r\n" + NumistaRequestsText(client.RequestsMade)).c_str(),
		caption.c_str(), MB_OK | (stopReason.IsEmpty() ? MB_ICONINFORMATION : MB_ICONWARNING));
}
//---------------------------------------------------------------------------
// Отмеченные экземпляры (в порядке коллекции).
std::vector<TCoinRecord> TfmMain::CheckedRecords() const
{
	std::vector<TCoinRecord> result;
	for (size_t i = 0; i < FAll.size(); ++i)
		if (FChecked.count(FAll[i].Id))
			result.push_back(FAll[i]);
	return result;
}
//---------------------------------------------------------------------------
// Активность команд Numista (задача 13e): часть «эта монета» — по выделенной строке,
// часть «отмеченные» — по отмеченным экземплярам. Один обработчик на все команды.
void __fastcall TfmMain::actNumistaUpdate(TObject *Sender)
{
	const int index = CurrentIndex();
	const TCoinRecord *coin = index >= 0 ? &FItems[index] : nullptr;
	bool coinUploaded = false;	// у монеты выгружен хотя бы один экземпляр
	bool noType = false, toUpload = false, uploaded = false, noCatalog = false;
	for (size_t i = 0; i < FAll.size(); ++i)
	{
		const TCoinRecord &r = FAll[i];
		if (coin && r.CoinId == coin->CoinId && r.NumistaItemId != 0)
			coinUploaded = true;
		if (!FChecked.count(r.Id))
			continue;
		noType    = noType    || r.NumistaTypeId == 0;
		toUpload  = toUpload  || (r.NumistaTypeId != 0 && r.NumistaItemId == 0);
		uploaded  = uploaded  || r.NumistaItemId != 0;
		noCatalog = noCatalog || (r.NumistaIssueId != 0 && !r.NumistaIssueLoaded)
			|| (r.NumistaTypeId != 0 && !r.NumistaTypeLoaded);
	}

	actNumistaFind->Enabled       = coin != nullptr;
	actNumistaIssue->Enabled      = coin && coin->NumistaTypeId != 0;
	actNumistaUploadOne->Enabled  = coin && coin->NumistaTypeId != 0 && coin->NumistaItemId == 0;
	actNumistaRarityOne->Enabled  = coinUploaded;
	actNumistaRefreshOne->Enabled = coin && coin->NumistaTypeId != 0;

	actNumistaCheck->Enabled   = noType;
	actNumistaUpload->Enabled  = toUpload;
	actNumistaRarity->Enabled  = uploaded;
	actNumistaCatalog->Enabled = noCatalog;
}
//---------------------------------------------------------------------------
// Подписи частей меню Numista (контекстного и главного): выделенная монета и
// число отмеченных. Обновляются при открытии меню.
void TfmMain::UpdateNumistaMenuTitles()
{
	const int index = CurrentIndex();
	const String coin = index >= 0 ? CoinText(FItems[index]) : Tr(L"Main.Menu.NoCoin");
	const int outside = CheckedOutside();
	const String checked = outside > 0
		? Format(Tr(L"Main.Menu.CheckedOutside"), ARRAYOFCONST(((int)FChecked.size(), outside)))
		: Format(Tr(L"Main.Menu.Checked"), ARRAYOFCONST(((int)FChecked.size())));
	// "&" в названии монеты — не горячая клавиша.
	const String coinCaption = StringReplace(coin, L"&", L"&&", TReplaceFlags() << rfReplaceAll);
	miNumistaCoinTitle->Caption    = coinCaption;
	miPopCoinTitle->Caption        = coinCaption;
	miNumistaCheckedTitle->Caption = checked;
	miPopCheckedTitle->Caption     = checked;
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::pmItemsPopup(TObject *Sender)
{
	UpdateNumistaMenuTitles();
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::miNumistaClick(TObject *Sender)
{
	UpdateNumistaMenuTitles();
}
//---------------------------------------------------------------------------
// Правый щелчок по заголовку: над колонкой флажков — меню «Отметить все / Снять все»,
// по остальным ячейкам заголовка — без меню; по строкам — основное меню (pmItems).
void __fastcall TfmMain::dgItemsContextPopup(TObject *Sender, const TPoint &MousePos, bool &Handled)
{
	if (MousePos.X < 0 && MousePos.Y < 0)
		return;	// с клавиатуры — основное меню
	int col = -1, row = -1;
	dgItems->MouseToCell(MousePos.X, MousePos.Y, col, row);
	if (row != 0)
		return;
	Handled = true;
	if (Column(col) == icCheck)
	{
		const TPoint pt = dgItems->ClientToScreen(MousePos);
		pmHeader->Popup(pt.X, pt.Y);
	}
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::actNumistaUploadExecute(TObject *Sender)
{
	UploadItems(CheckedRecords());
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::actNumistaUploadOneExecute(TObject *Sender)
{
	const int index = CurrentIndex();
	if (index >= 0)
		UploadItems(std::vector<TCoinRecord>(1, FItems[index]));
}
//---------------------------------------------------------------------------
// Выгрузка экземпляров ACandidates с N# в свою коллекцию Numista (addCollectedItems),
// по запросу на экземпляр. Перед отправкой — коллекция одним запросом
// (getCollectedItems): выгруженные раньше (internal_id = наш Id) получают
// numista_item_id без отправки; монета того же типа, добавленная на сайте (не программой), —
// связывается с найденным предметом без отправки. Со выгруженных экземпляров флажок снимается.
void TfmMain::UploadItems(const std::vector<TCoinRecord> &ACandidates)
{
	const String caption = Tr(L"Main.Numista.UploadTitle");

	std::vector<TCoinRecord> items;
	int noType = 0, uploadedBefore = 0;
	for (size_t i = 0; i < ACandidates.size(); ++i)
	{
		const TCoinRecord &r = ACandidates[i];
		if (r.NumistaTypeId == 0)
			++noType;
		else if (r.NumistaItemId != 0)
			++uploadedBefore;
		else
			items.push_back(r);
	}
	const String skipped = Format(Tr(L"Numista.Upload.NoType"), ARRAYOFCONST((noType))) + L"\r\n"
		+ Format(Tr(L"Numista.Upload.Before"), ARRAYOFCONST((uploadedBefore)));
	if (items.empty())
	{
		Application->MessageBox((Tr(L"Numista.Upload.Nothing") + L"\r\n\r\n" + skipped).c_str(), caption.c_str(),
			MB_OK | MB_ICONINFORMATION);
		return;
	}

	// Квота: экземпляр — запрос, плюс токен и коллекция.
	const int count = (int)items.size();
	const int needed = count + 2;
	const int left = AppConfig().NumistaMonthlyQuota - TNumistaClient::RequestsThisMonth();
	if (needed > left)
	{
		Application->MessageBox(Format(Tr(L"Numista.Upload.Quota"), ARRAYOFCONST((count, needed, left))).c_str(),
			caption.c_str(), MB_OK | MB_ICONWARNING);
		return;
	}
	if (count > cConfirmCheckCoins && !Confirm(Format(Tr(L"Numista.Upload.Confirm"), ARRAYOFCONST((count, needed,
		TNumistaClient::RequestsThisMonth(), AppConfig().NumistaMonthlyQuota))), caption))
		return;

	TNumistaClient client;
	TNumistaUploader uploader;
	int uploaded = 0, found = 0, foreign = 0, errors = 0, processed = 0;
	String firstError, stopReason;
	const TCursor saved = Screen->Cursor;
	Screen->Cursor = crHourGlass;
	try
	{
		client.Authorize();
		std::unique_ptr<TJSONValue> collection = client.GetCollectedItems();
		uploader.ReadCollection(collection.get(), FAll);

		for (size_t i = 0; i < items.size(); ++i)
		{
			const TCoinRecord &item = items[i];
			StatusBar->Panels->Items[0]->Text = Format(Tr(L"Numista.Upload.Progress"),
				ARRAYOFCONST(((int)i + 1, count)));
			StatusBar->Update();

			// Добавлена на сайте — связать с найденным предметом (✓), не отправлять повторно.
			int numistaId = uploader.UploadedItem(item.Id);
			if (numistaId != 0)
				++found;
			else if ((numistaId = uploader.TakeForeign(item.NumistaTypeId, item.NumistaIssueId)) != 0)
				++foreign;
			else
			{
				try
				{
					numistaId = client.AddCollectedItem(TNumistaUploader::ItemJson(item));
					++uploaded;
				}
				catch (ENumistaError &e)
				{
					// Ошибка данных одного экземпляра — в итог, дальше; сеть, 401, 429 — стоп.
					if (e.StatusCode != 400 && e.StatusCode != 404 && e.StatusCode != 422)
						throw;
					if (firstError.IsEmpty())
						firstError = CoinText(item) + L": " + e.Message;
					++errors;
					++processed;
					continue;
				}
			}
			FRepository->SetItemNumista(item.Id, numistaId);
			FChecked.erase(item.Id);
			++processed;
		}
	}
	catch (Exception &e)
	{
		stopReason = e.Message;
	}
	Screen->Cursor = saved;

	LoadItems();
	String report = Format(Tr(L"Numista.Upload.Uploaded"), ARRAYOFCONST((uploaded))) + L"\r\n"
		+ Format(Tr(L"Numista.Upload.Found"), ARRAYOFCONST((found))) + L"\r\n"
		+ Format(Tr(L"Numista.Upload.Foreign"), ARRAYOFCONST((foreign))) + L"\r\n"
		+ Format(Tr(L"Numista.Upload.Errors"), ARRAYOFCONST((errors))) + L"\r\n"
		+ Format(Tr(L"Numista.Match.NotProcessed"), ARRAYOFCONST((count - processed))) + L"\r\n"
		+ skipped;
	if (!firstError.IsEmpty())
		report += L"\r\n\r\n" + firstError;
	if (!stopReason.IsEmpty())
		report += L"\r\n\r\n" + Tr(L"Numista.Match.Stopped") + L" " + stopReason;
	Application->MessageBox((report + L"\r\n\r\n" + NumistaRequestsText(client.RequestsMade)).c_str(), caption.c_str(),
		MB_OK | (stopReason.IsEmpty() && errors == 0 ? MB_ICONINFORMATION : MB_ICONWARNING));
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::actNumistaRarityExecute(TObject *Sender)
{
	std::set<int> coins;
	for (size_t i = 0; i < FAll.size(); ++i)
		if (FChecked.count(FAll[i].Id))
			coins.insert(FAll[i].CoinId);
	ReadRarity(coins, StripHotkey(actNumistaRarity->Caption));
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::actNumistaRarityOneExecute(TObject *Sender)
{
	const int index = CurrentIndex();
	if (index >= 0)
		ReadRarity(std::set<int>{ FItems[index].CoinId }, StripHotkey(actNumistaRarityOne->Caption));
}
//---------------------------------------------------------------------------
// Индекс редкости монет ACoinIds: коллекция одним запросом (getCollectedItems), атрибут R
// комментария предмета → индекс редкости монеты (наибольший по её экземплярам).
// Монета найдена в коллекции, но R нет ни у одного экземпляра — индекс очищается.
void TfmMain::ReadRarity(const std::set<int> &ACoinIds, const String &ACaption)
{
	const String caption = ACaption;

	// Экземпляр -> монета: по id предмета Numista и по нашему Id (internal_id) —
	// только того же типа Numista (после импорта Id могут указывать на другую монету).
	std::map<int, int> coinByNumistaItem, coinByItem, typeByItem;
	std::map<int, int> oldRarity;	// монета -> индекс сейчас
	for (size_t i = 0; i < FAll.size(); ++i)
	{
		const TCoinRecord &r = FAll[i];
		if (r.NumistaItemId != 0)
			coinByNumistaItem[r.NumistaItemId] = r.CoinId;
		coinByItem[r.Id] = r.CoinId;
		typeByItem[r.Id] = r.NumistaTypeId;
		oldRarity[r.CoinId] = r.NumistaRarity;
	}

	TNumistaClient client;
	int total = 0, matched = 0, withRarity = 0, changed = 0, cleared = 0;
	String message;
	UINT icon = MB_ICONINFORMATION;
	const TCursor saved = Screen->Cursor;
	Screen->Cursor = crHourGlass;
	try
	{
		client.Authorize();
		std::unique_ptr<TJSONValue> json = client.GetCollectedItems();
		TJSONArray *items = dynamic_cast<TJSONArray *>(json->FindValue(L"items"));

		std::map<int, int> newRarity;	// монета -> наибольший R (-1 — нет)
		for (int i = 0; items && i < items->Count; ++i)
		{
			TJSONValue *item = items->Items[i];
			++total;
			TJSONValue *idValue = item->FindValue(L"id");
			TJSONValue *internalValue = item->FindValue(L"internal_id");
			const int numistaId = idValue ? StrToIntDef(idValue->Value(), 0) : 0;
			const int internalId = internalValue ? StrToIntDef(internalValue->Value().Trim(), 0) : 0;

			int coinId = 0;
			std::map<int, int>::const_iterator it = coinByNumistaItem.find(numistaId);
			if (it != coinByNumistaItem.end())
				coinId = it->second;
			else if ((it = coinByItem.find(internalId)) != coinByItem.end())
			{
				TJSONValue *typeValue = item->FindValue(L"type.id");
				if (typeValue && StrToIntDef(typeValue->Value(), 0) == typeByItem[internalId])
					coinId = it->second;
			}
			if (coinId == 0 || !ACoinIds.count(coinId))
				continue;
			++matched;

			TJSONValue *commentValue = item->FindValue(L"private_comment");
			const int rarity = TNumistaComment::Parse(commentValue ? commentValue->Value() : String()).Rarity;
			std::map<int, int>::iterator n = newRarity.find(coinId);
			if (n == newRarity.end())
				newRarity[coinId] = rarity;
			else if (rarity > n->second)
				n->second = rarity;
		}

		for (std::map<int, int>::const_iterator n = newRarity.begin(); n != newRarity.end(); ++n)
		{
			if (n->second >= 0)
				++withRarity;
			if (n->second == oldRarity[n->first])
				continue;
			FRepository->SetCoinRarity(n->first, n->second);
			if (n->second < 0)
				++cleared;
			else
				++changed;
		}
		message = Format(Tr(L"Numista.Rarity.Total"), ARRAYOFCONST((total))) + L"\r\n"
			+ Format(Tr(L"Numista.Rarity.Matched"), ARRAYOFCONST((matched, (int)newRarity.size()))) + L"\r\n"
			+ Format(Tr(L"Numista.Rarity.With"), ARRAYOFCONST((withRarity))) + L"\r\n"
			+ Format(Tr(L"Numista.Rarity.Changed"), ARRAYOFCONST((changed))) + L"\r\n"
			+ Format(Tr(L"Numista.Rarity.Cleared"), ARRAYOFCONST((cleared)));
	}
	catch (Exception &e)
	{
		message = e.Message;
		icon = MB_ICONERROR;
	}
	Screen->Cursor = saved;

	LoadItems();
	Application->MessageBox((message + L"\r\n\r\n" + NumistaRequestsText(client.RequestsMade)).c_str(), caption.c_str(),
		MB_OK | icon);
}
//---------------------------------------------------------------------------
// Update catalogue data (задачи 13c, 13d, 13e) для отмеченных монет: выпуски типа
// (getIssues), если выбран выпуск, а его нет в numista_issues, и характеристики типа
// (getType), если их нет в numista_types. С кэшем: из кэша — без запросов.
// Документация: https://en.numista.com/api/doc/index.php (Catalogue → getIssues, getType).
void __fastcall TfmMain::actNumistaCatalogExecute(TObject *Sender)
{
	const String caption = StripHotkey(actNumistaCatalog->Caption);

	std::set<int> typeSet, issueTypes, detailTypes;
	const std::vector<TCoinRecord> checked = CheckedRecords();
	for (size_t i = 0; i < checked.size(); ++i)
	{
		const TCoinRecord &r = checked[i];
		if (r.NumistaIssueId != 0 && !r.NumistaIssueLoaded)
			issueTypes.insert(r.NumistaTypeId);
		if (r.NumistaTypeId != 0 && !r.NumistaTypeLoaded)
			detailTypes.insert(r.NumistaTypeId);
	}
	typeSet.insert(issueTypes.begin(), issueTypes.end());
	typeSet.insert(detailTypes.begin(), detailTypes.end());
	if (typeSet.empty())
	{
		Application->MessageBox(Tr(L"Numista.Catalog.Nothing").c_str(), caption.c_str(), MB_OK | MB_ICONINFORMATION);
		return;
	}
	const std::vector<int> types(typeSet.begin(), typeSet.end());

	// Квота: запрос — только то, чего нет в кэше (новый тип — до 2 запросов).
	int needed = 0;
	for (size_t i = 0; i < types.size(); ++i)
	{
		if (issueTypes.count(types[i]) && !TNumistaClient::IsCached(TNumistaMatcher::IssuesPath(types[i])))
			++needed;
		if (detailTypes.count(types[i]) && !TNumistaClient::IsCached(TNumistaMatcher::TypePath(types[i])))
			++needed;
	}
	const int left = AppConfig().NumistaMonthlyQuota - TNumistaClient::RequestsThisMonth();
	if (needed > left)
	{
		Application->MessageBox(Format(Tr(L"Numista.Catalog.Quota"), ARRAYOFCONST(((int)types.size(), needed, left))).c_str(),
			caption.c_str(), MB_OK | MB_ICONWARNING);
		return;
	}
	if (needed > cConfirmCheckCoins && !Confirm(Format(Tr(L"Numista.Catalog.Confirm"), ARRAYOFCONST(((int)types.size(),
		needed, TNumistaClient::RequestsThisMonth(), AppConfig().NumistaMonthlyQuota))), caption))
		return;

	TNumistaClient client;
	TNumistaMatcher matcher(client);
	int processed = 0, fromCache = 0, issues = 0, details = 0;
	String stopReason;
	const TCursor saved = Screen->Cursor;
	Screen->Cursor = crHourGlass;
	try
	{
		for (size_t i = 0; i < types.size(); ++i)
		{
			StatusBar->Panels->Items[0]->Text = Format(Tr(L"Numista.Catalog.Progress"),
				ARRAYOFCONST(((int)i + 1, (int)types.size())));
			StatusBar->Update();
			const int type = types[i];
			const bool needIssues = issueTypes.count(type) > 0, needType = detailTypes.count(type) > 0;
			if ((!needIssues || TNumistaClient::IsCached(TNumistaMatcher::IssuesPath(type)))
				&& (!needType || TNumistaClient::IsCached(TNumistaMatcher::TypePath(type))))
				++fromCache;
			if (needIssues)
				issues += FRepository->SaveNumistaIssues(type, matcher.LoadIssues(type));
			if (needType)
			{
				FRepository->SaveNumistaType(matcher.LoadType(type));
				++details;
			}
			++processed;
		}
	}
	catch (Exception &e)
	{
		stopReason = e.Message;
	}
	Screen->Cursor = saved;

	LoadItems();	// и строка состояния
	std::set<int> withMintage, noMintage;	// отмеченные монеты: тираж известен / выпуск загружен, тиража у Numista нет
	for (size_t i = 0; i < FAll.size(); ++i)
		if (!FChecked.count(FAll[i].Id))
			continue;
		else if (FAll[i].NumistaMintage >= 0)
			withMintage.insert(FAll[i].CoinId);
		else if (FAll[i].NumistaIssueLoaded)
			noMintage.insert(FAll[i].CoinId);
	String report = Format(Tr(L"Numista.Catalog.Types"), ARRAYOFCONST((processed, (int)types.size(), fromCache))) + L"\r\n"
		+ Format(Tr(L"Numista.Catalog.Issues"), ARRAYOFCONST((issues))) + L"\r\n"
		+ Format(Tr(L"Numista.Catalog.Details"), ARRAYOFCONST((details))) + L"\r\n"
		+ Format(Tr(L"Numista.Catalog.Mintage"), ARRAYOFCONST(((int)withMintage.size()))) + L"\r\n"
		+ Format(Tr(L"Numista.Catalog.NoMintage"), ARRAYOFCONST(((int)noMintage.size())));
	if (!stopReason.IsEmpty())
		report += L"\r\n\r\n" + Tr(L"Numista.Match.Stopped") + L" " + stopReason;
	Application->MessageBox((report + L"\r\n\r\n" + NumistaRequestsText(client.RequestsMade)).c_str(),
		caption.c_str(), MB_OK | (stopReason.IsEmpty() ? MB_ICONINFORMATION : MB_ICONWARNING));
}
//---------------------------------------------------------------------------
// Обновить данные каталога выделенной монеты мимо кэша (задачи 13e, 13d): getIssues и
// getType типа — 2 запроса; ответы перезаписывают кэш, numista_issues и numista_types.
// Для случая, когда данные на Numista изменились (например, добавили тираж).
void __fastcall TfmMain::actNumistaRefreshOneExecute(TObject *Sender)
{
	const int index = CurrentIndex();
	if (index < 0 || FItems[index].NumistaTypeId == 0)
		return;
	const TCoinRecord coin = FItems[index];
	const String caption = StripHotkey(actNumistaRefreshOne->Caption);
	const String nl = L"\r\n";

	TNumistaClient client;
	TNumistaMatcher matcher(client);
	String message;
	UINT icon = MB_ICONINFORMATION;
	const TCursor saved = Screen->Cursor;
	Screen->Cursor = crHourGlass;
	try
	{
		const int issues = FRepository->SaveNumistaIssues(coin.NumistaTypeId, matcher.LoadIssues(coin.NumistaTypeId, true));
		message = Format(Tr(L"Numista.Refresh.Issues"), ARRAYOFCONST((issues)));
		FRepository->SaveNumistaType(matcher.LoadType(coin.NumistaTypeId, true));
		message += L"\r\n" + Tr(L"Numista.Refresh.Type");
	}
	catch (Exception &e)
	{
		message = e.Message;
		icon = MB_ICONERROR;
	}
	Screen->Cursor = saved;

	LoadItems();
	if (icon != MB_ICONERROR)
	{
		if (coin.NumistaIssueId == 0)
			message += nl + Tr(L"Numista.Refresh.NoIssue");
		else
		{
			// Тираж до и после — как в колонке («—» — пусто).
			String after;
			for (size_t i = 0; i < FAll.size(); ++i)
				if (FAll[i].Id == coin.Id)
					after = CellText(FAll[i], icMintage);
			const String before = CellText(coin, icMintage);
			message += nl + Format(Tr(L"Numista.Refresh.Mintage"), ARRAYOFCONST((
				before.IsEmpty() ? String(L"—") : before, after.IsEmpty() ? String(L"—") : after)));
		}
	}
	Application->MessageBox((CoinText(coin) + nl + nl + message + nl + nl + NumistaRequestsText(client.RequestsMade)).c_str(),
		caption.c_str(), MB_OK | icon);
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::NumistaLinkClick(TObject *Sender)
{
	if (FDetailsTypeId != 0)
		ShellExecuteW(Handle, L"open", TNumistaMatcher::TypeUrl(FDetailsTypeId).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::actExitExecute(TObject *Sender)
{
	Close();
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::actRefreshExecute(TObject *Sender)
{
	LoadItems();
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::actAboutExecute(TObject *Sender)
{
	// Заголовок — подпись действия About на текущем языке (без "&").
	const String text    = Format(Tr(L"Main.About"), ARRAYOFCONST((AppInfo::Name, AppInfo::Version)));
	const String caption = StripHotkey(actAbout->Caption);
	Application->MessageBox(text.c_str(), caption.c_str(), MB_OK | MB_ICONINFORMATION);
}
//---------------------------------------------------------------------------
// Перечитать коллекцию из БД; текущая запись сохраняется по Id.
void TfmMain::LoadItems()
{
	const int current = CurrentId();

	FConditionOrder.clear();
	const std::vector<TCondition> conditions = FRepository->Lookups().Conditions();
	for (size_t i = 0; i < conditions.size(); ++i)
		FConditionOrder[conditions[i].Code] = conditions[i].SortOrder;

	FAll = FRepository->LoadAll();

	// Отметки — только у существующих экземпляров (удаление, импорт).
	std::set<int> ids;
	for (size_t i = 0; i < FAll.size(); ++i)
		ids.insert(FAll[i].Id);
	for (std::set<int>::iterator it = FChecked.begin(); it != FChecked.end(); )
		it = ids.count(*it) ? std::next(it) : FChecked.erase(it);

	// Текст для поиска: поля через разделитель, в верхнем регистре.
	const String sep = String(L'\x1F');
	FSearchTexts.clear();
	FSearchTexts.reserve(FAll.size());
	for (size_t i = 0; i < FAll.size(); ++i)
	{
		const TCoinRecord &r = FAll[i];
		FSearchTexts.push_back(AnsiUpperCase(r.Country + sep + r.Period + sep + r.Currency + sep
			+ r.Denomination + sep + (r.Year == 0 ? String() : IntToStr(r.Year)) + sep + r.Subject + sep
			+ r.Variety + sep + r.CatalogNumber + sep + r.Comment));
	}

	FCountryByName.clear();
	const std::vector<TCountry> countries = FRepository->Lookups().Countries();
	for (size_t i = 0; i < countries.size(); ++i)
		FCountryByName[AnsiUpperCase(countries[i].Name)] = countries[i];

	BuildTree();
	ApplyFilter();
	SelectId(current);
}
//---------------------------------------------------------------------------
TTreeNode *TfmMain::AddNavNode(TTreeNode *AParent, int AKind, int AId, const String &AName, int ACount)
{
	TNavNode node;
	node.Kind = AKind;
	node.Id   = AId;
	node.Key  = IntToStr(AKind) + L":" + IntToStr(AId);
	FNav.push_back(node);
	return tvNav->Items->AddChildObject(AParent, AName + L" (" + IntToStr(ACount) + L")",
		reinterpret_cast<void *>((NativeInt)(FNav.size() - 1)));
}
//---------------------------------------------------------------------------
// Дерево: All -> континенты -> страны, плюс No continent и Extinct states.
// В скобках — число экземпляров (поиск и Need to replace на счётчики не влияют).
// Выбранный узел и раскрытые узлы сохраняются по ключу.
void TfmMain::BuildTree()
{
	const bool firstBuild = tvNav->Items->Count == 0;
	String selectedKey;
	std::set<String> expanded;
	for (int i = 0; i < tvNav->Items->Count; ++i)
	{
		TTreeNode *n = tvNav->Items->Item[i];
		const String key = FNav[(NativeInt)n->Data].Key;
		if (n->Expanded)
			expanded.insert(key);
		if (n == tvNav->Selected)
			selectedKey = key;
	}

	// Экземпляры по странам.
	std::map<int, int> byCountry;
	int unknown = 0;	// страна записи не найдена в справочнике (не должно быть)
	for (size_t i = 0; i < FAll.size(); ++i)
	{
		std::map<String, TCountry>::const_iterator c = FCountryByName.find(AnsiUpperCase(FAll[i].Country));
		if (c == FCountryByName.end())
			++unknown;
		else
			++byCountry[c->second.Id];
	}
	std::vector<TCountry> countries;
	for (std::map<String, TCountry>::const_iterator it = FCountryByName.begin(); it != FCountryByName.end(); ++it)
		countries.push_back(it->second);
	std::sort(countries.begin(), countries.end(),
		[](const TCountry &A, const TCountry &B) { return AnsiCompareText(A.Name, B.Name) < 0; });
	std::vector<TContinent> continents = FRepository->Lookups().Continents();
	std::sort(continents.begin(), continents.end(),
		[](const TContinent &A, const TContinent &B) { return A.Id < B.Id; });
	auto count = [&byCountry](int AId) { std::map<int, int>::const_iterator it = byCountry.find(AId); return it == byCountry.end() ? 0 : it->second; };

	FBuildingTree = true;
	tvNav->Items->BeginUpdate();
	try
	{
		tvNav->Items->Clear();
		FNav.clear();

		TTreeNode *root = AddNavNode(nullptr, nkAll, 0, Tr(L"Main.Tree.All"), (int)FAll.size());

		// Континенты (всегда, даже с 0) и их существующие страны с монетами.
		for (size_t k = 0; k < continents.size(); ++k)
		{
			int sum = 0;
			for (size_t i = 0; i < countries.size(); ++i)
				if (countries[i].IsExist && countries[i].ContinentId == continents[k].Id)
					sum += count(countries[i].Id);
			TTreeNode *node = AddNavNode(root, nkContinent, continents[k].Id, continents[k].Name, sum);
			for (size_t i = 0; i < countries.size(); ++i)
				if (countries[i].IsExist && countries[i].ContinentId == continents[k].Id && count(countries[i].Id) > 0)
					AddNavNode(node, nkCountry, countries[i].Id, countries[i].Name, count(countries[i].Id));
		}

		// Страны без континента — только если у них есть монеты.
		int noContinent = unknown;
		for (size_t i = 0; i < countries.size(); ++i)
			if (countries[i].IsExist && countries[i].ContinentId == 0)
				noContinent += count(countries[i].Id);
		if (noContinent > 0)
		{
			TTreeNode *node = AddNavNode(root, nkNoContinent, 0, Tr(L"Main.Tree.NoContinent"), noContinent);
			for (size_t i = 0; i < countries.size(); ++i)
				if (countries[i].IsExist && countries[i].ContinentId == 0 && count(countries[i].Id) > 0)
					AddNavNode(node, nkCountry, countries[i].Id, countries[i].Name, count(countries[i].Id));
		}

		// Исчезнувшие государства — только здесь, не под своим континентом.
		int extinct = 0;
		for (size_t i = 0; i < countries.size(); ++i)
			if (!countries[i].IsExist)
				extinct += count(countries[i].Id);
		TTreeNode *extinctNode = AddNavNode(root, nkExtinct, 0, Tr(L"Main.Tree.Extinct"), extinct);
		for (size_t i = 0; i < countries.size(); ++i)
			if (!countries[i].IsExist && count(countries[i].Id) > 0)
				AddNavNode(extinctNode, nkCountry, countries[i].Id, countries[i].Name, count(countries[i].Id));

		// Отмеченные из любых стран — отдельный узел после All (задача 13e).
		FCheckedNode = AddNavNode(nullptr, nkChecked, 0, Tr(L"Main.Tree.Checked"), (int)FChecked.size());

		// Восстановить раскрытие и выбор (при первом построении — раскрыт и выбран All).
		TTreeNode *select = root;
		for (int i = 0; i < tvNav->Items->Count; ++i)
		{
			TTreeNode *n = tvNav->Items->Item[i];
			const String key = FNav[(NativeInt)n->Data].Key;
			if (firstBuild ? n == root : expanded.count(key) > 0)
				n->Expand(false);
			if (!selectedKey.IsEmpty() && key == selectedKey)
				select = n;
		}
		tvNav->Selected = select;
	}
	__finally
	{
		tvNav->Items->EndUpdate();
		FBuildingTree = false;
	}
}
//---------------------------------------------------------------------------
const TfmMain::TNavNode *TfmMain::CurrentNode() const
{
	if (!tvNav->Selected)
		return nullptr;
	const NativeInt index = (NativeInt)tvNav->Selected->Data;
	return index >= 0 && index < (NativeInt)FNav.size() ? &FNav[index] : nullptr;
}
//---------------------------------------------------------------------------
bool TfmMain::MatchNode(const TCoinRecord &ARecord, const TNavNode &ANode) const
{
	if (ANode.Kind == nkAll)
		return true;
	if (ANode.Kind == nkChecked)
		return FChecked.count(ARecord.Id) > 0;
	std::map<String, TCountry>::const_iterator it = FCountryByName.find(AnsiUpperCase(ARecord.Country));
	if (it == FCountryByName.end())
		return ANode.Kind == nkNoContinent;
	const TCountry &c = it->second;
	switch (ANode.Kind)
	{
		case nkCountry:     return c.Id == ANode.Id;
		case nkExtinct:     return !c.IsExist;
		case nkNoContinent: return c.IsExist && c.ContinentId == 0;
		case nkContinent:   return c.IsExist && c.ContinentId == ANode.Id;
	}
	return false;
}
//---------------------------------------------------------------------------
// Показанные записи = узел дерева И поиск И Need to replace.
void TfmMain::ApplyFilter()
{
	const int current = CurrentId();
	const TNavNode *node = CurrentNode();
	const String search = AnsiUpperCase(edSearch->Text.Trim());
	const bool onlyReplace = actNeedToReplace->Checked;

	FItems.clear();
	for (size_t i = 0; i < FAll.size(); ++i)
	{
		const TCoinRecord &r = FAll[i];
		if (node && !MatchNode(r, *node))
			continue;
		if (onlyReplace && !r.NeedToReplace)
			continue;
		if (!search.IsEmpty() && FSearchTexts[i].Pos(search) == 0)
			continue;
		FItems.push_back(r);
	}

	UpdateColumns();
	SortItems();
	UpdateRowCount();
	SelectId(current);
	dgItems->Invalidate();
	UpdateStatus();
	UpdateDetails();
}
//---------------------------------------------------------------------------
// Колонка Country не нужна, когда выбрана одна страна. Ширины, заданные
// пользователем мышью, запоминаются по логическим колонкам.
void TfmMain::UpdateColumns()
{
	const TNavNode *node = CurrentNode();
	const bool showCountry = !node || node->Kind != nkCountry;
	std::vector<int> columns;
	for (int c = 0; c < icCount; ++c)
		if (c != icCountry || showCountry)
			columns.push_back(c);
	if (columns == FColumns && dgItems->ColCount == (int)columns.size())
		return;

	for (size_t i = 0; i < FColumns.size() && (int)i < dgItems->ColCount; ++i)
		FColWidths[FColumns[i]] = dgItems->ColWidths[(int)i];
	FColumns = columns;
	dgItems->ColCount = (int)FColumns.size();
	for (size_t i = 0; i < FColumns.size(); ++i)
		dgItems->ColWidths[(int)i] = FColWidths[FColumns[i]];

	// Сортировка по скрытой колонке теряет смысл — вернуть порядок по году.
	if (std::find(FColumns.begin(), FColumns.end(), FSortColumn) == FColumns.end())
	{
		FSortColumn = icYear;
		FSortAscending = true;
	}
}
//---------------------------------------------------------------------------
int TfmMain::Column(int AGridCol) const
{
	return AGridCol >= 0 && AGridCol < (int)FColumns.size() ? FColumns[AGridCol] : -1;
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::tvNavChange(TObject *Sender, TTreeNode *Node)
{
	if (!FBuildingTree)
		ApplyFilter();
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::edSearchChange(TObject *Sender)
{
	ApplyFilter();
}
//---------------------------------------------------------------------------
// Esc — очистить поиск; Enter / стрелка вниз — к списку.
void __fastcall TfmMain::edSearchKeyDown(TObject *Sender, WORD &Key, TShiftState Shift)
{
	if (Key == VK_ESCAPE)
	{
		edSearch->Text = L"";
		Key = 0;
	}
	else if (Key == VK_RETURN || Key == VK_DOWN)
	{
		dgItems->SetFocus();
		Key = 0;
	}
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::tbClearSearchClick(TObject *Sender)
{
	edSearch->Text = L"";
	dgItems->SetFocus();
}
//---------------------------------------------------------------------------
// Сравнение по колонке; пустые значения — в начале (при сортировке по возрастанию).
int TfmMain::CompareItems(const TCoinRecord &A, const TCoinRecord &B, int AColumn) const
{
	switch (AColumn)
	{
		case icNumista:      return CompareValues(A.NumistaTypeId, B.NumistaTypeId);
		case icUploaded:     return CompareValues(A.NumistaItemId != 0, B.NumistaItemId != 0);
		case icRarity:       return CompareValues(A.NumistaRarity, B.NumistaRarity);
		case icMintage:      return CompareValues(MintageKey(A), MintageKey(B));
		case icComposition:  return CompareStrings(A.NumistaComposition, B.NumistaComposition);
		case icCountry:      return CompareStrings(A.Country, B.Country);
		case icPeriod:       return CompareStrings(A.Period, B.Period);
		case icCurrency:     return CompareStrings(A.Currency, B.Currency);
		case icDenomination: return CompareStrings(A.Denomination, B.Denomination);
		case icYear:         return CompareValues(A.Year, B.Year);
		case icValue:        return CompareValues(A.CatalogValueUah, B.CatalogValueUah);
		case icCondition:
		{
			std::map<String, int>::const_iterator a = FConditionOrder.find(A.Condition);
			std::map<String, int>::const_iterator b = FConditionOrder.find(B.Condition);
			return CompareValues(a == FConditionOrder.end() ? 0 : a->second,
			                     b == FConditionOrder.end() ? 0 : b->second);
		}
	}
	return 0;
}
//---------------------------------------------------------------------------
void TfmMain::SortItems()
{
	const int column = FSortColumn;
	const bool ascending = FSortAscending;
	std::stable_sort(FItems.begin(), FItems.end(),
		[this, column, ascending](const TCoinRecord &A, const TCoinRecord &B)
		{
			int r = CompareItems(A, B, column);
			if (!ascending)
				r = -r;
			// Равные — всегда в порядке год, номинал, Id.
			if (r == 0) r = CompareItems(A, B, icYear);
			if (r == 0) r = CompareItems(A, B, icDenomination);
			if (r == 0) r = CompareValues(A.Id, B.Id);
			return r < 0;
		});
}
//---------------------------------------------------------------------------
String TfmMain::CellText(const TCoinRecord &R, int AColumn) const
{
	switch (AColumn)
	{
		case icNumista:      return R.NumistaTypeId == 0 ? String() : IntToStr(R.NumistaTypeId);
		case icUploaded:     return R.NumistaItemId == 0 ? String() : String(L"\u2713");
		case icRarity:       return R.NumistaRarity < 0 ? String() : IntToStr(R.NumistaRarity);
		case icMintage:      return R.NumistaMintage >= 0 || !R.NumistaIssueLoaded ? MintageText(R.NumistaMintage)
			: Tr(L"Main.Mintage.Unknown");
		case icComposition:  return R.NumistaComposition;
		case icCountry:      return R.Country;
		case icPeriod:       return R.Period;
		case icCurrency:     return R.Currency;
		case icDenomination: return R.Denomination;
		case icYear:         return R.Year == 0 ? String() : IntToStr(R.Year);
		case icCondition:    return R.Condition;
		case icValue:        return R.CatalogValueUah == 0.0 ? String() : FormatFloat(L"#,##0.00", R.CatalogValueUah);
	}
	return String();
}
//---------------------------------------------------------------------------
// Строка 0 — заголовок (FixedRows = 1), строка N — FItems[N - 1].
// У сетки всегда есть хотя бы одна строка данных; при пустом списке она пустая.
void TfmMain::UpdateRowCount()
{
	dgItems->RowCount = std::max<int>(1, (int)FItems.size()) + 1;
}
//---------------------------------------------------------------------------
int TfmMain::CurrentIndex() const
{
	const int index = dgItems->Row - 1;
	return index >= 0 && index < (int)FItems.size() ? index : -1;
}
//---------------------------------------------------------------------------
int TfmMain::CurrentId() const
{
	const int index = CurrentIndex();
	return index >= 0 ? FItems[index].Id : 0;
}
//---------------------------------------------------------------------------
// Сделать текущей запись AId; если её нет — оставить строку на месте.
void TfmMain::SelectId(int AId)
{
	for (size_t i = 0; i < FItems.size(); ++i)
		if (FItems[i].Id == AId)
		{
			SelectIndex((int)i);
			return;
		}
	SelectIndex(dgItems->Row - 1);
}
//---------------------------------------------------------------------------
// Сделать текущей строку FItems[AIndex] (с поправкой на границы списка).
void TfmMain::SelectIndex(int AIndex)
{
	const int last = std::max<int>(0, (int)FItems.size() - 1);
	dgItems->Row = std::min(std::max(AIndex, 0), last) + 1;
	UpdateDetails();	// Row мог не измениться — тогда OnSelectCell не вызывается
}
//---------------------------------------------------------------------------
// Ячейку рисуем сами: фон и выделение строки сетка уже нарисовала
// (DefaultDrawing), здесь — только текст и стрелка сортировки в заголовке.
void __fastcall TfmMain::dgItemsDrawCell(TObject *Sender, int ACol, int ARow,
	TRect &Rect, TGridDrawState State)
{
	const int column = Column(ACol);
	if (column < 0)
		return;

	if (column == icCheck)
	{
		if (ARow == 0)
			DrawCheckBox(Rect, CheckedState());
		else if (ARow - 1 < (int)FItems.size())
			DrawCheckBox(Rect, FChecked.count(FItems[ARow - 1].Id) ? 2 : 0);
		return;
	}

	String text;
	if (ARow == 0)
		text = Tr(cColumnKeys[column]);
	else if (ARow - 1 < (int)FItems.size())
		text = CellText(FItems[ARow - 1], column);

	TCanvas *canvas = dgItems->Canvas;
	RECT r = { Rect.Left + 4, Rect.Top, Rect.Right - 4, Rect.Bottom };

	// Стрелка сортировки справа в заголовке текущей колонки.
	if (ARow == 0 && column == FSortColumn && r.right - r.left > 16)
	{
		const int cx = r.right - 5, cy = (Rect.Top + Rect.Bottom) / 2;
		TPoint arrow[3];
		if (FSortAscending)
		{
			arrow[0] = TPoint(cx - 4, cy + 2); arrow[1] = TPoint(cx + 4, cy + 2); arrow[2] = TPoint(cx, cy - 2);
		}
		else
		{
			arrow[0] = TPoint(cx - 4, cy - 2); arrow[1] = TPoint(cx + 4, cy - 2); arrow[2] = TPoint(cx, cy + 2);
		}
		canvas->Pen->Color   = clGrayText;
		canvas->Brush->Style = bsSolid;
		canvas->Brush->Color = clGrayText;
		canvas->Polygon(arrow, 2);
		r.right -= 14;
	}

	UINT flags = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX;
	if (ARow > 0 && (column == icYear || column == icValue || column == icNumista || column == icRarity
		|| column == icMintage))
		flags |= DT_RIGHT;
	if (column == icUploaded)
		flags = (flags & ~DT_END_ELLIPSIS) | DT_CENTER;
	// Тип связан, выпуск не определён — N# серым.
	if (ARow > 0 && column == icNumista && ARow - 1 < (int)FItems.size()
		&& FItems[ARow - 1].NumistaIssueId == 0 && !State.Contains(gdSelected))
		canvas->Font->Color = clGrayText;
	canvas->Brush->Style = bsClear;
	DrawTextW(canvas->Handle, text.c_str(), -1, &r, flags);
}
//---------------------------------------------------------------------------
// Клик по заголовку: та же колонка — обратный порядок, другая — по возрастанию.
void __fastcall TfmMain::dgItemsFixedCellClick(TObject *Sender, int ACol, int ARow)
{
	const int column = Column(ACol);
	if (ARow != 0 || column < 0)
		return;
	if (column == icCheck)
	{
		SetAllChecked(CheckedState() != 2);
		return;
	}
	if (column == FSortColumn)
		FSortAscending = !FSortAscending;
	else
	{
		FSortColumn = column;
		FSortAscending = true;
	}

	const int current = CurrentId();
	SortItems();
	SelectId(current);
	dgItems->Invalidate();
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::dgItemsMouseDown(TObject *Sender, TMouseButton Button,
	TShiftState Shift, int X, int Y)
{
	int col = -1;
	dgItems->MouseToCell(X, Y, col, FMouseDownRow);
	const int index = FMouseDownRow - 1;
	if (index < 0 || index >= (int)FItems.size())
		return;
	if (Button == mbLeft && Column(col) == icCheck)
		ToggleChecked(index);
	else if (Button == mbRight)
		SelectIndex(index);	// контекстное меню — для строки под курсором
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::dgItemsDblClick(TObject *Sender)
{
	// Двойной клик по заголовку (например, по границе колонки) — не правка.
	const TPoint pt = dgItems->ScreenToClient(Mouse->CursorPos);
	int col = -1, row = -1;
	dgItems->MouseToCell(pt.X, pt.Y, col, row);
	if (FMouseDownRow > 0 && Column(col) != icCheck && actEdit->Enabled)
		actEdit->Execute();
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::dgItemsKeyDown(TObject *Sender, WORD &Key, TShiftState Shift)
{
	if (Key == VK_RETURN && Shift.Empty())
	{
		if (actEdit->Enabled)
			actEdit->Execute();
		Key = 0;
	}
	else if (Key == VK_SPACE && Shift.Empty())
	{
		if (CurrentIndex() >= 0)
			ToggleChecked(CurrentIndex());
		Key = 0;
	}
}
//---------------------------------------------------------------------------
// Итоги по показанным записям: число, сумма Quantity, сумма Value × Quantity.
void TfmMain::UpdateStatus()
{
	if (StatusBar->Panels->Count < 3)
		return;
	int quantity = 0;
	double value = 0.0;
	for (size_t i = 0; i < FItems.size(); ++i)
	{
		quantity += FItems[i].Quantity;
		value    += FItems[i].CatalogValueUah * FItems[i].Quantity;
	}
	StatusBar->Panels->Items[0]->Text = Format(Tr(L"Main.Status.Items"), ARRAYOFCONST(((int)FItems.size(), (int)FAll.size())));
	StatusBar->Panels->Items[1]->Text = Format(Tr(L"Main.Status.Quantity"), ARRAYOFCONST((quantity)));
	StatusBar->Panels->Items[2]->Text = Format(Tr(L"Main.Status.Value"), ARRAYOFCONST((FormatFloat(L"#,##0.00", value))));
	if (StatusBar->Panels->Count > 3)
	{
		const int outside = CheckedOutside();
		StatusBar->Panels->Items[3]->Text = outside > 0
			? Format(Tr(L"Main.Status.CheckedOutside"), ARRAYOFCONST(((int)FChecked.size(), outside)))
			: Format(Tr(L"Main.Status.Checked"), ARRAYOFCONST(((int)FChecked.size())));
	}
	UpdateCheckedNode();
}
//---------------------------------------------------------------------------
// Отметки строк (флажки)
//---------------------------------------------------------------------------
void TfmMain::ToggleChecked(int AIndex)
{
	const int id = FItems[AIndex].Id;
	if (!FChecked.erase(id))
		FChecked.insert(id);
	dgItems->Invalidate();	// и флажок в заголовке
	UpdateStatus();
}
//---------------------------------------------------------------------------
void TfmMain::SetAllChecked(bool AChecked)
{
	for (size_t i = 0; i < FItems.size(); ++i)
		if (AChecked)
			FChecked.insert(FItems[i].Id);
		else
			FChecked.erase(FItems[i].Id);
	dgItems->Invalidate();
	UpdateStatus();
}
//---------------------------------------------------------------------------
// Отмеченные экземпляры, которых нет в текущем списке (другие страны, поиск).
int TfmMain::CheckedOutside() const
{
	size_t shown = 0;
	for (size_t i = 0; i < FItems.size(); ++i)
		shown += FChecked.count(FItems[i].Id);
	return (int)(FChecked.size() - shown);
}
//---------------------------------------------------------------------------
void TfmMain::UpdateCheckedNode()
{
	if (!FCheckedNode)
		return;
	const String text = Tr(L"Main.Tree.Checked") + L" (" + IntToStr((int)FChecked.size()) + L")";
	if (FCheckedNode->Text != text)
		FCheckedNode->Text = text;
}
//---------------------------------------------------------------------------
int TfmMain::CheckedState() const
{
	size_t checked = 0;
	for (size_t i = 0; i < FItems.size(); ++i)
		checked += FChecked.count(FItems[i].Id);
	return checked == 0 ? 0 : (checked == FItems.size() ? 2 : 1);
}
//---------------------------------------------------------------------------
// Флажок по центру ячейки в стиле системы; AState: 0 — снят, 1 — частично, 2 — отмечен.
void TfmMain::DrawCheckBox(const TRect &ARect, int AState)
{
	const int size = 13;
	TRect box(0, 0, size, size);
	OffsetRect(&box, ARect.Left + (ARect.Width() - size) / 2, ARect.Top + (ARect.Height() - size) / 2);
	if (StyleServices()->Enabled)
	{
		const TThemedButton part = AState == 2 ? tbCheckBoxCheckedNormal
			: (AState == 1 ? tbCheckBoxMixedNormal : tbCheckBoxUncheckedNormal);
		StyleServices()->DrawElement(dgItems->Canvas->Handle, StyleServices()->GetElementDetails(part), box);
	}
	else
	{
		RECT r = box;
		DrawFrameControl(dgItems->Canvas->Handle, &r, DFC_BUTTON,
			DFCS_BUTTONCHECK | DFCS_FLAT | (AState == 2 ? DFCS_CHECKED : 0) | (AState == 1 ? DFCS_INACTIVE | DFCS_CHECKED : 0));
	}
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::actCheckAllExecute(TObject *Sender)
{
	SetAllChecked(Sender == actCheckAll);
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::actCheckAllUpdate(TObject *Sender)
{
	actCheckAll->Enabled   = !FItems.empty();
	actUncheckAll->Enabled = !FItems.empty();
	actUncheckEverywhere->Enabled = !FChecked.empty();
	actShowChecked->Enabled       = !FChecked.empty();
	// Число отмеченных — в подписи (во всей коллекции).
	const String clear = Format(Tr(L"Main.Check.Everywhere"), ARRAYOFCONST(((int)FChecked.size())));
	if (actUncheckEverywhere->Caption != clear)
		actUncheckEverywhere->Caption = clear;
	const String show = Format(Tr(L"Main.Check.Show"), ARRAYOFCONST(((int)FChecked.size())));
	if (actShowChecked->Caption != show)
		actShowChecked->Caption = show;
}
//---------------------------------------------------------------------------
// Снять отметки во всей коллекции, а не только в текущем списке.
void __fastcall TfmMain::actUncheckEverywhereExecute(TObject *Sender)
{
	FChecked.clear();
	const TNavNode *node = CurrentNode();
	if (node && node->Kind == nkChecked)
		ApplyFilter();	// узел «Отмеченные» становится пустым
	dgItems->Invalidate();
	UpdateStatus();
}
//---------------------------------------------------------------------------
// Показать отмеченные из любых стран — узел «Отмеченные» в дереве.
void __fastcall TfmMain::actShowCheckedExecute(TObject *Sender)
{
	if (!FCheckedNode)
		return;
	if (tvNav->Selected == FCheckedNode)
		ApplyFilter();	// список отмеченных мог измениться
	else
		tvNav->Selected = FCheckedNode;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Панель подробностей
//---------------------------------------------------------------------------
// Строки «подпись — значение» в sbInfo; новая строка — новый ключ в cDetailKeys
// и элемент TDetailRowIndex.
void TfmMain::CreateDetailRows()
{
	for (int i = 0; i < drCount; ++i)
	{
		TDetailRow row;
		row.Key = cDetailKeys[i];
		row.Caption = new TLabel(this);
		row.Caption->Parent        = sbInfo;
		row.Caption->ShowAccelChar = false;
		row.Caption->Font->Color   = clGrayText;
		row.Value = new TLabel(this);
		row.Value->Parent        = sbInfo;
		row.Value->ShowAccelChar = false;	// "&" в данных — обычный символ
		row.Value->AutoSize      = false;
		row.Value->WordWrap      = true;
		FDetailRows.push_back(row);
	}
	// N# — ссылка на страницу типа на Numista.
	FDetailRows[drNumista].Value->OnClick = NumistaLinkClick;
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::dgItemsSelectCell(TObject *Sender, int ACol, int ARow, bool &CanSelect)
{
	ShowDetails(ARow - 1);
}
//---------------------------------------------------------------------------
void TfmMain::UpdateDetails()
{
	ShowDetails(CurrentIndex());
}
//---------------------------------------------------------------------------
// Пустые значения — «—»; нет записи — панель пустая.
void TfmMain::ShowDetails(int AIndex)
{
	const bool has = AIndex >= 0 && AIndex < (int)FItems.size();
	auto value = [](const String &AText) { return AText.Trim().IsEmpty() ? String(L"\u2014") : AText; };

	if (has)
	{
		const TCoinRecord &r = FItems[AIndex];
		String title = r.Denomination;
		if (r.Year != 0)
			title += L", " + IntToStr(r.Year);
		lblDetailsTitle->Caption = title + L" \u2014 " + r.Country;
		FDetailRows[drPeriod].Value->Caption   = value(r.Period);
		FDetailRows[drCurrency].Value->Caption = value(r.Currency);
		FDetailRows[drSubject].Value->Caption  = value(r.Subject);
		FDetailRows[drNumber].Value->Caption   = value(r.CatalogNumber);
		FDetailRows[drNumista].Value->Caption  = r.NumistaTypeId == 0 ? String(L"\u2014")
			: L"N# " + IntToStr(r.NumistaTypeId);
		FDetailsTypeId = r.NumistaTypeId;
		FDetailRows[drNumistaItem].Value->Caption = r.NumistaItemId == 0 ? String(L"\u2014")
			: L"#" + IntToStr(r.NumistaItemId);
		FDetailRows[drIssueComment].Value->Caption = value(r.NumistaIssueComment);
		ShowTypeDetails(r.NumistaTypeLoaded ? r.NumistaTypeId : 0);
	}
	else
	{
		lblDetailsTitle->Caption = L"";
		FDetailsTypeId = 0;
	}
	TLabel *link = FDetailRows[drNumista].Value;
	link->Cursor      = FDetailsTypeId != 0 ? crHandPoint : crDefault;
	link->Font->Color = FDetailsTypeId != 0 ? clHotLight : clWindowText;
	link->Font->Style = FDetailsTypeId != 0 ? TFontStyles() << fsUnderline : TFontStyles();

	for (size_t i = 0; i < FDetailRows.size(); ++i)
	{
		// Характеристики типа — только непустые.
		const bool visible = has && ((int)i < drWeight || !FDetailRows[i].Value->Caption.IsEmpty());
		FDetailRows[i].Caption->Visible = visible;
		FDetailRows[i].Value->Visible   = visible;
	}
	LayoutDetails();
}
//---------------------------------------------------------------------------
// Строки характеристик типа (задача 13d); ATypeId = 0 или тип не загружен — все пустые.
void TfmMain::ShowTypeDetails(int ATypeId)
{
	TNumistaTypeInfo t;
	if (ATypeId == 0 || !FRepository->LoadNumistaType(ATypeId, t))
		t = TNumistaTypeInfo();

	String size;
	if (t.Size != 0.0)
		size = NumberText(t.Size) + (t.Size2 != 0.0 ? L" × " + NumberText(t.Size2) : String());
	if (t.Thickness != 0.0)
		size += (size.IsEmpty() ? String() : String(L"; ")) + Format(Tr(L"Main.Details.ThicknessValue"),
			ARRAYOFCONST((NumberText(t.Thickness))));
	String years;
	if (t.MinYear != 0)
		years = t.MaxYear != 0 && t.MaxYear != t.MinYear ? IntToStr(t.MinYear) + L"–" + IntToStr(t.MaxYear)
			: IntToStr(t.MinYear);
	String demonetized;
	if (t.Demonetized == 0)
		demonetized = Tr(L"Main.Details.DemonetizedNo");
	else if (t.Demonetized == 1)
		demonetized = t.DemonetizationDate.IsEmpty() ? Tr(L"Main.Details.DemonetizedYes")
			: Format(Tr(L"Main.Details.DemonetizedSince"), ARRAYOFCONST((t.DemonetizationDate)));

	FDetailRows[drWeight].Value->Caption       = t.Weight != 0.0 ? NumberText(t.Weight) + L" " + Tr(L"Main.Details.Grams") : String();
	FDetailRows[drSize].Value->Caption         = size.IsEmpty() ? String() : size + L" " + Tr(L"Main.Details.Mm");
	FDetailRows[drShape].Value->Caption        = t.Shape;
	FDetailRows[drTechnique].Value->Caption    = t.Technique;
	FDetailRows[drRuler].Value->Caption        = t.Rulers.IsEmpty() ? String(L"\u2014") : t.Rulers;
	FDetailRows[drSeries].Value->Caption       = t.Series;
	FDetailRows[drCommemorated].Value->Caption = t.CommemoratedTopic;
	FDetailRows[drTypeYears].Value->Caption    = years;
	FDetailRows[drMints].Value->Caption        = t.Mints;
	FDetailRows[drCategory].Value->Caption     = t.ObjectType;
	FDetailRows[drDemonetized].Value->Caption  = demonetized;
}
//---------------------------------------------------------------------------
// Высота текста метки при переносе по ширине AWidth.
int TfmMain::TextHeight(TLabel *ALabel, int AWidth)
{
	Canvas->Font->Assign(ALabel->Font);
	RECT rc = { 0, 0, AWidth, 0 };
	DrawTextW(Canvas->Handle, ALabel->Caption.c_str(), -1, &rc, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
	return std::max<int>(rc.bottom, Canvas->TextHeight(L"Wg"));
}
//---------------------------------------------------------------------------
// Раскладка sbInfo: заголовок, затем строки в две колонки (вторая — с drWeight);
// узкая панель — одна колонка. Длинный текст переносится, не помещается по высоте — прокрутка.
void TfmMain::LayoutDetails()
{
	if (!HandleAllocated())
		return;
	const int left = 8, captionWidth = 110, gap = 6, right = 8, columnGap = 16, minValueWidth = 120;
	const int width = std::max(80, sbInfo->ClientWidth - left - right);
	const bool twoColumns = width >= 2 * (captionWidth + gap + minValueWidth) + columnGap;
	const int columnWidth = twoColumns ? (width - columnGap) / 2 : width;
	const int valueWidth = std::max(60, columnWidth - captionWidth - gap);
	int y = 8 - sbInfo->VertScrollBar->Position;

	const int titleHeight = TextHeight(lblDetailsTitle, width);
	lblDetailsTitle->SetBounds(left, y, width, titleHeight);
	y += titleHeight + 8;
	int y1 = y, y2 = y;	// низ первой и второй колонки
	for (size_t i = 0; i < FDetailRows.size(); ++i)
	{
		if (!FDetailRows[i].Value->Visible)
			continue;
		const bool second = twoColumns && (int)i >= drWeight;
		const int x = left + (second ? columnWidth + columnGap : 0);
		int &top = second ? y2 : y1;
		const int h = TextHeight(FDetailRows[i].Value, valueWidth);
		FDetailRows[i].Caption->SetBounds(x, top, captionWidth, TextHeight(FDetailRows[i].Caption, captionWidth));
		FDetailRows[i].Value->SetBounds(x + captionWidth + gap, top, valueWidth, h);
		top += h + 4;
	}
	sbInfo->VertScrollBar->Range = std::max(y1, y2) + sbInfo->VertScrollBar->Position + 4;
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::sbInfoResize(TObject *Sender)
{
	LayoutDetails();
}
//---------------------------------------------------------------------------
