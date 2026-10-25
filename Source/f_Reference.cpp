//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <algorithm>
#include <memory>
#include "f_Reference.h"
#include "u_LookupRepository.h"
#include "u_Translator.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TfmReference *fmReference;
//---------------------------------------------------------------------------
namespace
{
	const wchar_t *const cCountryColumns[]  = { L"Ref.Col.Name", L"Ref.Col.Continent", L"Ref.Col.Exists", L"Ref.Col.Items" };
	const wchar_t *const cNameItemColumns[] = { L"Ref.Col.Name", L"Ref.Col.Items" };
}
//---------------------------------------------------------------------------
__fastcall TfmReference::TfmReference(TComponent* Owner)
	: TForm(Owner),
	  FLookups(nullptr),
	  FChanged(false),
	  FCountryId(0)
{
	Translator().TranslateForm(this);
}
//---------------------------------------------------------------------------
bool TfmReference::Execute(TComponent *AOwner, TLookupRepository &ALookups, int ATab)
{
	std::unique_ptr<TfmReference> form(new TfmReference(AOwner));
	form->FLookups = &ALookups;

	form->FContinents = ALookups.Continents();
	std::sort(form->FContinents.begin(), form->FContinents.end(),
		[](const TContinent &A, const TContinent &B) { return A.Id < B.Id; });
	form->cbContinent->Items->Add(L"—");
	for (size_t i = 0; i < form->FContinents.size(); ++i)
		form->cbContinent->Items->Add(form->FContinents[i].Name);

	form->LoadCountries();
	form->LoadCurrencies();
	if (form->cbPeriodCountry->Items->Count > 0)
		form->cbPeriodCountry->ItemIndex = 0;
	form->LoadPeriods();

	form->PageControl->ActivePageIndex = std::max(0, std::min(ATab, form->PageControl->PageCount - 1));
	form->ShowModal();
	return form->FChanged;
}
//---------------------------------------------------------------------------
// Общее
//---------------------------------------------------------------------------
int TfmReference::Usage(const std::map<int, int> &AUsage, int AId)
{
	std::map<int, int>::const_iterator it = AUsage.find(AId);
	return it == AUsage.end() ? 0 : it->second;
}
//---------------------------------------------------------------------------
// Строка 0 — заголовок; при пустом списке — одна пустая строка данных.
void TfmReference::SetRows(TDrawGrid *AGrid, size_t ACount, int ASelectIndex)
{
	AGrid->RowCount = (int)std::max<size_t>(1, ACount) + 1;
	AGrid->Row = std::min(std::max(ASelectIndex, 0), std::max(0, (int)ACount - 1)) + 1;
	AGrid->Invalidate();
}
//---------------------------------------------------------------------------
void TfmReference::Warn(const String &AText)
{
	Application->MessageBox(AText.c_str(), Caption.c_str(), MB_OK | MB_ICONWARNING);
}
//---------------------------------------------------------------------------
bool TfmReference::Confirm(const String &AText)
{
	return Application->MessageBox(AText.c_str(), Caption.c_str(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES;
}
//---------------------------------------------------------------------------
String TfmReference::ContinentName(int AId) const
{
	for (size_t i = 0; i < FContinents.size(); ++i)
		if (FContinents[i].Id == AId)
			return FContinents[i].Name;
	return L"—";
}
//---------------------------------------------------------------------------
int TfmReference::ContinentIdFromCombo() const
{
	const int index = cbContinent->ItemIndex;
	return index > 0 && index <= (int)FContinents.size() ? FContinents[index - 1].Id : 0;
}
//---------------------------------------------------------------------------
String TfmReference::CellText(TObject *AGrid, int ACol, int ARow) const
{
	const int index = ARow - 1;
	if (AGrid == dgCountries)
	{
		if (ARow == 0)
			return ACol < 4 ? Tr(cCountryColumns[ACol]) : String();
		if (index >= (int)FCountries.size())
			return String();
		const TCountry &c = FCountries[index];
		switch (ACol)
		{
			case 0: return c.Name;
			case 1: return ContinentName(c.ContinentId);
			case 2: return Tr(c.IsExist ? L"Ref.Yes" : L"Ref.No");
			case 3: return IntToStr(Usage(FCountryUsage, c.Id));
		}
	}
	else if (AGrid == dgPeriods)
	{
		if (ARow == 0)
			return ACol < 2 ? Tr(cNameItemColumns[ACol]) : String();
		if (index >= (int)FPeriods.size())
			return String();
		return ACol == 0 ? FPeriods[index].Name : IntToStr(Usage(FPeriodUsage, FPeriods[index].Id));
	}
	else if (AGrid == dgCurrencies)
	{
		if (ARow == 0)
			return ACol < 2 ? Tr(cNameItemColumns[ACol]) : String();
		if (index >= (int)FCurrencies.size())
			return String();
		return ACol == 0 ? FCurrencies[index].Name : IntToStr(Usage(FCurrencyUsage, FCurrencies[index].Id));
	}
	return String();
}
//---------------------------------------------------------------------------
// Как в главном окне: фон и выделение рисует сетка, здесь — только текст.
void __fastcall TfmReference::GridDrawCell(TObject *Sender, int ACol, int ARow,
	TRect &Rect, TGridDrawState State)
{
	TDrawGrid *grid = static_cast<TDrawGrid *>(Sender);
	const String text = CellText(Sender, ACol, ARow);
	RECT r = { Rect.Left + 4, Rect.Top, Rect.Right - 4, Rect.Bottom };
	UINT flags = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX;
	if (ARow > 0 && ACol == grid->ColCount - 1)	// колонка Items — числа справа
		flags |= DT_RIGHT;
	grid->Canvas->Brush->Style = bsClear;
	DrawTextW(grid->Canvas->Handle, text.c_str(), -1, &r, flags);
}
//---------------------------------------------------------------------------
void __fastcall TfmReference::GridSelectCell(TObject *Sender, int ACol, int ARow, bool &CanSelect)
{
	if (Sender == dgCountries)
		ShowCountry(ARow - 1);
	else if (Sender == dgPeriods)
		ShowPeriod(ARow - 1);
	else if (Sender == dgCurrencies)
		ShowCurrency(ARow - 1);
}
//---------------------------------------------------------------------------
// Страны
//---------------------------------------------------------------------------
void TfmReference::LoadCountries(int ASelectId)
{
	// Выбрать заданную страну, иначе последнюю показанную: после фильтра
	// «только без континента» она могла пропасть из списка и вернуться.
	if (ASelectId == 0)
		ASelectId = FCountryId;
	const int periodCountry = PeriodCountryId();	// до перечитывания FAllCountries

	FAllCountries = FLookups->Countries();
	FCountryUsage = FLookups->CountryUsage();
	FCountries.clear();
	int select = 0;
	for (size_t i = 0; i < FAllCountries.size(); ++i)
	{
		if (chkOnlyNoContinent->Checked && FAllCountries[i].ContinentId != 0)
			continue;
		if (FAllCountries[i].Id == ASelectId)
			select = (int)FCountries.size();
		FCountries.push_back(FAllCountries[i]);
	}
	SetRows(dgCountries, FCountries.size(), select);
	ShowCountry(FCountries.empty() ? -1 : select);

	// Список стран для вкладки Periods (выбор сохраняется).
	cbPeriodCountry->Items->BeginUpdate();
	cbPeriodCountry->Items->Clear();
	for (size_t i = 0; i < FAllCountries.size(); ++i)
	{
		cbPeriodCountry->Items->Add(FAllCountries[i].Name);
		if (FAllCountries[i].Id == periodCountry)
			cbPeriodCountry->ItemIndex = (int)i;
	}
	cbPeriodCountry->Items->EndUpdate();
}
//---------------------------------------------------------------------------
int TfmReference::CountryIndex() const
{
	const int index = dgCountries->Row - 1;
	return index >= 0 && index < (int)FCountries.size() ? index : -1;
}
//---------------------------------------------------------------------------
void TfmReference::ShowCountry(int AIndex)
{
	if (AIndex < 0 || AIndex >= (int)FCountries.size())
	{
		edCountryName->Text = L"";
		cbContinent->ItemIndex = 0;
		chkExists->Checked = true;
	}
	else
	{
		const TCountry &c = FCountries[AIndex];
		FCountryId = c.Id;
		edCountryName->Text = c.Name;
		cbContinent->ItemIndex = 0;
		for (size_t i = 0; i < FContinents.size(); ++i)
			if (FContinents[i].Id == c.ContinentId)
				cbContinent->ItemIndex = (int)i + 1;
		chkExists->Checked = c.IsExist;
	}
	lblRenameWarning->Visible = false;
}
//---------------------------------------------------------------------------
// Предупреждение показывается, когда меняется имя уже существующей страны.
void __fastcall TfmReference::edCountryNameChange(TObject *Sender)
{
	const int index = CountryIndex();
	lblRenameWarning->Visible = index >= 0 && edCountryName->Text.Trim() != FCountries[index].Name;
}
//---------------------------------------------------------------------------
void __fastcall TfmReference::chkOnlyNoContinentClick(TObject *Sender)
{
	LoadCountries();
}
//---------------------------------------------------------------------------
void __fastcall TfmReference::btnCountryAddClick(TObject *Sender)
{
	const String name = edCountryName->Text.Trim();
	if (name.IsEmpty())
		return Warn(Tr(L"Ref.Error.EmptyName"));
	if (FLookups->CountryExists(name))
		return Warn(Format(Tr(L"Ref.Error.Duplicate"), ARRAYOFCONST((name))));

	TCountry c;
	c.Name        = name;
	c.ContinentId = ContinentIdFromCombo();
	c.IsExist     = chkExists->Checked;
	const int id = FLookups->AddCountry(c);
	FChanged = true;
	LoadCountries(id);
}
//---------------------------------------------------------------------------
void __fastcall TfmReference::btnCountrySaveClick(TObject *Sender)
{
	const int index = CountryIndex();
	if (index < 0)
		return;
	const String name = edCountryName->Text.Trim();
	if (name.IsEmpty())
		return Warn(Tr(L"Ref.Error.EmptyName"));
	if (FLookups->CountryExists(name, FCountries[index].Id))
		return Warn(Format(Tr(L"Ref.Error.Duplicate"), ARRAYOFCONST((name))));

	TCountry c = FCountries[index];
	c.Name        = name;
	c.ContinentId = ContinentIdFromCombo();
	c.IsExist     = chkExists->Checked;
	FLookups->SaveCountry(c);
	FChanged = true;
	LoadCountries(c.Id);
}
//---------------------------------------------------------------------------
void __fastcall TfmReference::btnCountryDeleteClick(TObject *Sender)
{
	const int index = CountryIndex();
	if (index < 0)
		return;
	const TCountry c = FCountries[index];
	const int items = Usage(FCountryUsage, c.Id);
	if (items > 0)
		return Warn(Format(Tr(L"Ref.Error.InUse"), ARRAYOFCONST((c.Name, items))));
	if (!Confirm(Format(Tr(L"Ref.Delete.Confirm"), ARRAYOFCONST((c.Name)))))
		return;
	if (!FLookups->DeleteCountry(c.Id))
		return Warn(Format(Tr(L"Ref.Error.InUse"), ARRAYOFCONST((c.Name, items))));
	FChanged = true;
	LoadCountries();
	LoadPeriods();
}
//---------------------------------------------------------------------------
// Периоды
//---------------------------------------------------------------------------
int TfmReference::PeriodCountryId() const
{
	const int index = cbPeriodCountry->ItemIndex;
	return index >= 0 && index < (int)FAllCountries.size() ? FAllCountries[index].Id : 0;
}
//---------------------------------------------------------------------------
void TfmReference::LoadPeriods(int ASelectId)
{
	const int countryId = PeriodCountryId();
	FPeriods = countryId == 0 ? std::vector<TPeriod>() : FLookups->Periods(countryId);
	FPeriodUsage = countryId == 0 ? std::map<int, int>() : FLookups->PeriodUsage(countryId);
	int select = 0;
	for (size_t i = 0; i < FPeriods.size(); ++i)
		if (FPeriods[i].Id == ASelectId)
			select = (int)i;
	SetRows(dgPeriods, FPeriods.size(), select);
	ShowPeriod(FPeriods.empty() ? -1 : select);
}
//---------------------------------------------------------------------------
int TfmReference::PeriodIndex() const
{
	const int index = dgPeriods->Row - 1;
	return index >= 0 && index < (int)FPeriods.size() ? index : -1;
}
//---------------------------------------------------------------------------
void TfmReference::ShowPeriod(int AIndex)
{
	edPeriodName->Text = AIndex >= 0 && AIndex < (int)FPeriods.size() ? FPeriods[AIndex].Name : String();
}
//---------------------------------------------------------------------------
void __fastcall TfmReference::cbPeriodCountryChange(TObject *Sender)
{
	LoadPeriods();
}
//---------------------------------------------------------------------------
void __fastcall TfmReference::btnPeriodAddClick(TObject *Sender)
{
	const int countryId = PeriodCountryId();
	if (countryId == 0)
		return Warn(Tr(L"Ref.Error.NoCountry"));
	const String name = edPeriodName->Text.Trim();
	if (name.IsEmpty())
		return Warn(Tr(L"Ref.Error.EmptyName"));
	if (FLookups->PeriodExists(countryId, name))
		return Warn(Format(Tr(L"Ref.Error.Duplicate"), ARRAYOFCONST((name))));

	const int id = FLookups->AddPeriod(countryId, name);
	FChanged = true;
	LoadPeriods(id);
}
//---------------------------------------------------------------------------
void __fastcall TfmReference::btnPeriodSaveClick(TObject *Sender)
{
	const int index = PeriodIndex();
	if (index < 0)
		return;
	const String name = edPeriodName->Text.Trim();
	if (name.IsEmpty())
		return Warn(Tr(L"Ref.Error.EmptyName"));
	if (FLookups->PeriodExists(PeriodCountryId(), name, FPeriods[index].Id))
		return Warn(Format(Tr(L"Ref.Error.Duplicate"), ARRAYOFCONST((name))));

	const int id = FPeriods[index].Id;
	FLookups->RenamePeriod(id, name);
	FChanged = true;
	LoadPeriods(id);
}
//---------------------------------------------------------------------------
void __fastcall TfmReference::btnPeriodDeleteClick(TObject *Sender)
{
	const int index = PeriodIndex();
	if (index < 0)
		return;
	const TPeriod p = FPeriods[index];
	const int items = Usage(FPeriodUsage, p.Id);
	if (items > 0)
		return Warn(Format(Tr(L"Ref.Error.InUse"), ARRAYOFCONST((p.Name, items))));
	if (!Confirm(Format(Tr(L"Ref.Delete.Confirm"), ARRAYOFCONST((p.Name)))))
		return;
	if (!FLookups->DeletePeriod(p.Id))
		return Warn(Format(Tr(L"Ref.Error.InUse"), ARRAYOFCONST((p.Name, items))));
	FChanged = true;
	LoadPeriods();
}
//---------------------------------------------------------------------------
// Валюты
//---------------------------------------------------------------------------
void TfmReference::LoadCurrencies(int ASelectId)
{
	FCurrencies = FLookups->Currencies();
	FCurrencyUsage = FLookups->CurrencyUsage();
	int select = 0;
	for (size_t i = 0; i < FCurrencies.size(); ++i)
		if (FCurrencies[i].Id == ASelectId)
			select = (int)i;
	SetRows(dgCurrencies, FCurrencies.size(), select);
	ShowCurrency(FCurrencies.empty() ? -1 : select);
}
//---------------------------------------------------------------------------
int TfmReference::CurrencyIndex() const
{
	const int index = dgCurrencies->Row - 1;
	return index >= 0 && index < (int)FCurrencies.size() ? index : -1;
}
//---------------------------------------------------------------------------
void TfmReference::ShowCurrency(int AIndex)
{
	edCurrencyName->Text = AIndex >= 0 && AIndex < (int)FCurrencies.size() ? FCurrencies[AIndex].Name : String();
}
//---------------------------------------------------------------------------
void __fastcall TfmReference::btnCurrencyAddClick(TObject *Sender)
{
	const String name = edCurrencyName->Text.Trim();
	if (name.IsEmpty())
		return Warn(Tr(L"Ref.Error.EmptyName"));
	if (FLookups->CurrencyExists(name))
		return Warn(Format(Tr(L"Ref.Error.Duplicate"), ARRAYOFCONST((name))));

	const int id = FLookups->AddCurrency(name);
	FChanged = true;
	LoadCurrencies(id);
}
//---------------------------------------------------------------------------
void __fastcall TfmReference::btnCurrencySaveClick(TObject *Sender)
{
	const int index = CurrencyIndex();
	if (index < 0)
		return;
	const String name = edCurrencyName->Text.Trim();
	if (name.IsEmpty())
		return Warn(Tr(L"Ref.Error.EmptyName"));
	if (FLookups->CurrencyExists(name, FCurrencies[index].Id))
		return Warn(Format(Tr(L"Ref.Error.Duplicate"), ARRAYOFCONST((name))));

	const int id = FCurrencies[index].Id;
	FLookups->RenameCurrency(id, name);
	FChanged = true;
	LoadCurrencies(id);
}
//---------------------------------------------------------------------------
void __fastcall TfmReference::btnCurrencyDeleteClick(TObject *Sender)
{
	const int index = CurrencyIndex();
	if (index < 0)
		return;
	const TCurrency c = FCurrencies[index];
	const int items = Usage(FCurrencyUsage, c.Id);
	if (items > 0)
		return Warn(Format(Tr(L"Ref.Error.InUse"), ARRAYOFCONST((c.Name, items))));
	if (!Confirm(Format(Tr(L"Ref.Delete.Confirm"), ARRAYOFCONST((c.Name)))))
		return;
	if (!FLookups->DeleteCurrency(c.Id))
		return Warn(Format(Tr(L"Ref.Error.InUse"), ARRAYOFCONST((c.Name, items))));
	FChanged = true;
	LoadCurrencies();
}
//---------------------------------------------------------------------------
