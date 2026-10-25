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
	  FLanguageToken(0),
	  FMouseDownRow(-1)
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
	FItems.clear();
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
void __fastcall TfmMain::actEditUpdate(TObject *Sender)
{
	actEdit->Enabled = CurrentIndex() >= 0;
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
	actDelete->Enabled = CurrentIndex() >= 0;
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

	FItems = FRepository->LoadAll();
	SortItems();

	UpdateRowCount();
	SelectId(current);
	dgItems->Invalidate();
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
}
//---------------------------------------------------------------------------
// Ячейку рисуем сами: фон и выделение строки сетка уже нарисовала
// (DefaultDrawing), здесь — только текст и стрелка сортировки в заголовке.
void __fastcall TfmMain::dgItemsDrawCell(TObject *Sender, int ACol, int ARow,
	TRect &Rect, TGridDrawState State)
{
	if (ACol < 0 || ACol >= icCount)
		return;

	String text;
	if (ARow == 0)
		text = Tr(cColumnKeys[ACol]);
	else if (ARow - 1 < (int)FItems.size())
		text = CellText(FItems[ARow - 1], ACol);

	TCanvas *canvas = dgItems->Canvas;
	RECT r = { Rect.Left + 4, Rect.Top, Rect.Right - 4, Rect.Bottom };

	// Стрелка сортировки справа в заголовке текущей колонки.
	if (ARow == 0 && ACol == FSortColumn && r.right - r.left > 16)
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
	if (ARow > 0 && (ACol == icYear || ACol == icValue))
		flags |= DT_RIGHT;
	canvas->Brush->Style = bsClear;
	DrawTextW(canvas->Handle, text.c_str(), -1, &r, flags);
}
//---------------------------------------------------------------------------
// Клик по заголовку: та же колонка — обратный порядок, другая — по возрастанию.
void __fastcall TfmMain::dgItemsFixedCellClick(TObject *Sender, int ACol, int ARow)
{
	if (ARow != 0 || ACol < 0 || ACol >= icCount)
		return;
	if (ACol == FSortColumn)
		FSortAscending = !FSortAscending;
	else
	{
		FSortColumn = ACol;
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
void TfmMain::UpdateStatus()
{
	if (StatusBar->Panels->Count > 0)
		StatusBar->Panels->Items[0]->Text = Format(Tr(L"Main.Status.Items"), ARRAYOFCONST(((int)FItems.size())));
}
//---------------------------------------------------------------------------
