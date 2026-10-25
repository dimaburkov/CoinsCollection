//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include <algorithm>
#include <set>
#include <shlwapi.h>
#include <Vcl.Dialogs.hpp>
#include "f_Main.h"
#include "dm_Data.h"
#include "u_AppConfig.h"
#include "u_Translator.h"
#include "u_ImportSource.h"
#include "u_CoinImporter.h"
#include "f_CoinEdit.h"
#include "f_Reference.h"
#include "f_NumistaSettings.h"
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
		L"Main.Column.Country", L"Main.Column.Period", L"Main.Column.Currency", L"Main.Column.Denomination",
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
	const int cDefaultWidths[] = { 130, 160, 120, 130, 50, 65, 85 };
	// Подписи строк панели подробностей (порядок = TDetailRowIndex).
	const wchar_t *const cDetailKeys[] =
	{
		L"Main.Details.Period", L"Main.Details.Currency", L"Main.Details.Subject", L"Main.Details.Number"
	};
	//-----------------------------------------------------------------------
	template <typename T>
	int CompareValues(T A, T B)
	{
		return A < B ? -1 : (B < A ? 1 : 0);
	}
}
//---------------------------------------------------------------------------
__fastcall TfmMain::TfmMain(TComponent* Owner)
	: TForm(Owner),
	  FSortColumn(icDenomination),
	  FSortAscending(true),
	  FLanguageToken(0),
	  FMouseDownRow(-1),
	  FBuildingTree(false)
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
	try
	{
		if (current > 0)
			FDatabase->Backup(L"before-import-" + FormatDateTime(L"yyyymmdd-hhnnss", Now()));
		saved = FRepository->ReplaceAll(records);
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
	Application->MessageBox(Format(Tr(L"Import.Done"),
		ARRAYOFCONST((saved, (int)coins.size(), (int)errors.size()))).c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
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

	// Сортировка по скрытой колонке теряет смысл — вернуть порядок по номиналу.
	if (std::find(FColumns.begin(), FColumns.end(), FSortColumn) == FColumns.end())
	{
		FSortColumn = icDenomination;
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
			// Равные — всегда в порядке номинал, год, Id.
			if (r == 0) r = CompareItems(A, B, icDenomination);
			if (r == 0) r = CompareItems(A, B, icYear);
			if (r == 0) r = CompareValues(A.Id, B.Id);
			return r < 0;
		});
}
//---------------------------------------------------------------------------
String TfmMain::CellText(const TCoinRecord &R, int AColumn) const
{
	switch (AColumn)
	{
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
	if (ARow > 0 && (column == icYear || column == icValue))
		flags |= DT_RIGHT;
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
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::dgItemsDblClick(TObject *Sender)
{
	// Двойной клик по заголовку (например, по границе колонки) — не правка.
	if (FMouseDownRow > 0 && actEdit->Enabled)
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
	}
	else
		lblDetailsTitle->Caption = L"";

	for (size_t i = 0; i < FDetailRows.size(); ++i)
	{
		FDetailRows[i].Caption->Visible = has;
		FDetailRows[i].Value->Visible   = has;
	}
	LayoutDetails();
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
// Раскладка sbInfo: заголовок, затем строки; длинный текст переносится,
// не помещается по высоте — прокрутка.
void TfmMain::LayoutDetails()
{
	if (!HandleAllocated())
		return;
	const int left = 8, captionWidth = 110, gap = 6, right = 8;
	const int width = std::max(80, sbInfo->ClientWidth - left - right);
	const int valueWidth = std::max(60, width - captionWidth - gap);
	int y = 8 - sbInfo->VertScrollBar->Position;

	const int titleHeight = TextHeight(lblDetailsTitle, width);
	lblDetailsTitle->SetBounds(left, y, width, titleHeight);
	y += titleHeight + 8;
	for (size_t i = 0; i < FDetailRows.size(); ++i)
	{
		if (!FDetailRows[i].Value->Visible)
			continue;
		const int h = TextHeight(FDetailRows[i].Value, valueWidth);
		FDetailRows[i].Caption->SetBounds(left, y, captionWidth, TextHeight(FDetailRows[i].Caption, captionWidth));
		FDetailRows[i].Value->SetBounds(left + captionWidth + gap, y, valueWidth, h);
		y += h + 4;
	}
	sbInfo->VertScrollBar->Range = y + sbInfo->VertScrollBar->Position + 4;
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::sbInfoResize(TObject *Sender)
{
	LayoutDetails();
}
//---------------------------------------------------------------------------
