//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include <algorithm>
#include <set>
#include <shlwapi.h>
#include <CommCtrl.h>
#include "f_Main.h"
#include "dm_Data.h"
#include "u_AppConfig.h"
#include "u_Translator.h"
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
		L"Main.Column.Period", L"Main.Column.Currency", L"Main.Column.Denomination",
		L"Main.Column.Year", L"Main.Column.Condition", L"Main.Column.Number", L"Main.Column.Value"
	};
	//-----------------------------------------------------------------------
	// Строки — «как в Проводнике»: без учёта регистра, числа внутри по значению
	// ("2 hryvni" < "10 hryven", "KM# 9" < "KM# 10").
	int CompareStrings(const String &A, const String &B)
	{
		return StrCmpLogicalW(A.c_str(), B.c_str());
	}
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
	  FLanguageToken(0)
{
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
	lvItems->Items->Count = 0;
	FItems.clear();
	FRepository.reset();
	if (FDatabase)
		FDatabase->Close();
	FDatabase.reset();
}
//---------------------------------------------------------------------------
// Тексты, которые не берутся из .dfm напрямую: заголовки колонок, короткая
// подпись кнопки импорта, строка состояния. Вызывается и при смене языка.
void TfmMain::ApplyTexts()
{
	for (int i = 0; i < icCount && i < lvItems->Columns->Count; ++i)
		lvItems->Columns->Items[i]->Caption = Tr(cColumnKeys[i]);
	// Кнопка связана с actImport, но подпись у неё короче, чем пункт меню.
	tbImport->Caption = Tr(L"Main.Toolbar.Import");
	UpdateSortArrows();
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
// Перечитать коллекцию из БД; выделение и фокус сохраняются по Id.
void TfmMain::LoadItems()
{
	const std::vector<int> selected = SelectedIds();
	const int focused = FocusedId();

	FConditionOrder.clear();
	const std::vector<TCondition> conditions = FRepository->Lookups().Conditions();
	for (size_t i = 0; i < conditions.size(); ++i)
		FConditionOrder[conditions[i].Code] = conditions[i].SortOrder;

	FItems = FRepository->LoadAll();
	SortItems();

	lvItems->HandleNeeded();	// виртуальному списку число строк задаётся через окно
	lvItems->Items->BeginUpdate();
	try
	{
		lvItems->Items->Count = (int)FItems.size();
		SelectIds(selected, focused);
	}
	__finally
	{
		lvItems->Items->EndUpdate();
	}
	lvItems->Invalidate();
	UpdateSortArrows();
	UpdateStatus();
}
//---------------------------------------------------------------------------
// Сравнение по колонке; пустые значения — в начале (при сортировке по возрастанию).
int TfmMain::CompareItems(const TCoinRecord &A, const TCoinRecord &B, int AColumn) const
{
	switch (AColumn)
	{
		case icPeriod:       return CompareStrings(A.Period, B.Period);
		case icCurrency:     return CompareStrings(A.Currency, B.Currency);
		case icDenomination: return CompareStrings(A.Denomination, B.Denomination);
		case icYear:         return CompareValues(A.Year, B.Year);
		case icNumber:       return CompareStrings(A.CatalogNumber, B.CatalogNumber);
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
		case icPeriod:       return R.Period;
		case icCurrency:     return R.Currency;
		case icDenomination: return R.Denomination;
		case icYear:         return R.Year == 0 ? String() : IntToStr(R.Year);
		case icCondition:    return R.Condition;
		case icNumber:       return R.CatalogNumber;
		case icValue:        return R.CatalogValueUah == 0.0 ? String() : FormatFloat(L"#,##0.00", R.CatalogValueUah);
	}
	return String();
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::lvItemsData(TObject *Sender, TListItem *Item)
{
	if (Item->Index < 0 || Item->Index >= (int)FItems.size())
		return;
	const TCoinRecord &r = FItems[Item->Index];
	Item->Caption = CellText(r, 0);
	for (int c = 1; c < icCount; ++c)
		Item->SubItems->Add(CellText(r, c));
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::lvItemsColumnClick(TObject *Sender, TListColumn *Column)
{
	if (Column->Index == FSortColumn)
		FSortAscending = !FSortAscending;
	else
	{
		FSortColumn = Column->Index;
		FSortAscending = true;
	}

	const std::vector<int> selected = SelectedIds();
	const int focused = FocusedId();
	SortItems();
	SelectIds(selected, focused);
	lvItems->Invalidate();
	UpdateSortArrows();
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::lvItemsDblClick(TObject *Sender)
{
	if (actEdit->Enabled)
		actEdit->Execute();
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::lvItemsKeyDown(TObject *Sender, WORD &Key, TShiftState Shift)
{
	if (Key == VK_RETURN && Shift.Empty())
	{
		if (actEdit->Enabled)
			actEdit->Execute();
		Key = 0;
	}
}
//---------------------------------------------------------------------------
// Стрелка сортировки в заголовке (HDF_SORTUP / HDF_SORTDOWN, comctl32 v6).
void TfmMain::UpdateSortArrows()
{
	if (!lvItems->HandleAllocated())
		return;
	const HWND header = ListView_GetHeader(lvItems->Handle);
	for (int i = 0; i < lvItems->Columns->Count; ++i)
	{
		HDITEMW item = {};
		item.mask = HDI_FORMAT;
		Header_GetItem(header, i, &item);
		item.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
		if (i == FSortColumn)
			item.fmt |= FSortAscending ? HDF_SORTUP : HDF_SORTDOWN;
		Header_SetItem(header, i, &item);
	}
}
//---------------------------------------------------------------------------
void TfmMain::UpdateStatus()
{
	if (StatusBar->Panels->Count > 0)
		StatusBar->Panels->Items[0]->Text = Format(Tr(L"Main.Status.Items"), ARRAYOFCONST(((int)FItems.size())));
}
//---------------------------------------------------------------------------
std::vector<int> TfmMain::SelectedIds() const
{
	std::vector<int> ids;
	if (!lvItems->HandleAllocated())
		return ids;
	for (int i = ListView_GetNextItem(lvItems->Handle, -1, LVNI_SELECTED); i >= 0;
	     i = ListView_GetNextItem(lvItems->Handle, i, LVNI_SELECTED))
	{
		if (i < (int)FItems.size())
			ids.push_back(FItems[i].Id);
	}
	return ids;
}
//---------------------------------------------------------------------------
int TfmMain::FocusedId() const
{
	if (!lvItems->HandleAllocated())
		return 0;
	const int i = ListView_GetNextItem(lvItems->Handle, -1, LVNI_FOCUSED);
	return i >= 0 && i < (int)FItems.size() ? FItems[i].Id : 0;
}
//---------------------------------------------------------------------------
void TfmMain::SelectIds(const std::vector<int> &AIds, int AFocusedId)
{
	if (!lvItems->HandleAllocated())
		return;
	const HWND list = lvItems->Handle;
	ListView_SetItemState(list, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);

	const std::set<int> ids(AIds.begin(), AIds.end());
	for (int i = 0; i < (int)FItems.size(); ++i)
	{
		UINT state = 0;
		if (ids.count(FItems[i].Id))
			state |= LVIS_SELECTED;
		if (AFocusedId != 0 && FItems[i].Id == AFocusedId)
			state |= LVIS_FOCUSED;
		if (state)
		{
			ListView_SetItemState(list, i, state, LVIS_SELECTED | LVIS_FOCUSED);
			if (state & LVIS_FOCUSED)
				ListView_EnsureVisible(list, i, FALSE);
		}
	}
}
//---------------------------------------------------------------------------
