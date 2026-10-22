//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <algorithm>
#include <math.h>
#include <memory>
#include "f_CoinEdit.h"
#include "u_CoinRepository.h"
#include "u_Translator.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TfmCoinEdit *fmCoinEdit;
//---------------------------------------------------------------------------
__fastcall TfmCoinEdit::TfmCoinEdit(TComponent* Owner)
	: TForm(Owner),
	  FRepository(nullptr)
{
	Translator().TranslateForm(this);
}
//---------------------------------------------------------------------------
bool TfmCoinEdit::Execute(TComponent *AOwner, TCoinRepository &ARepository, TCoinRecord &ARecord)
{
	std::unique_ptr<TfmCoinEdit> form(new TfmCoinEdit(AOwner));
	form->FRepository = &ARepository;
	form->Caption = Tr(ARecord.Id == 0 ? L"CoinEdit.TitleNew" : L"CoinEdit.TitleEdit");

	// Поля монеты общие для всех её экземпляров — предупредить, если их несколько.
	const int items = ARecord.CoinId == 0 ? 0 : ARepository.ItemCount(ARecord.CoinId);
	form->lblShared->Caption = Format(Tr(L"CoinEdit.Shared"), ARRAYOFCONST((items)));
	form->lblShared->Visible = items > 1;

	form->LoadLookups();
	form->LoadFrom(ARecord);
	if (form->ShowModal() != mrOk)
		return false;

	form->SaveTo(ARecord);
	return true;
}
//---------------------------------------------------------------------------
void TfmCoinEdit::LoadLookups()
{
	FCountries = FRepository->Lookups().Countries();
	cbCountry->Items->BeginUpdate();
	cbCountry->Items->Clear();
	for (size_t i = 0; i < FCountries.size(); ++i)
		cbCountry->Items->Add(FCountries[i].Name);
	cbCountry->Items->EndUpdate();

	const std::vector<TCurrency> currencies = FRepository->Lookups().Currencies();
	cbCurrency->Items->BeginUpdate();
	cbCurrency->Items->Clear();
	for (size_t i = 0; i < currencies.size(); ++i)
		cbCurrency->Items->Add(currencies[i].Name);
	cbCurrency->Items->EndUpdate();

	FConditions = FRepository->Lookups().Conditions();
	cbCondition->Items->Clear();
	cbCondition->Items->Add(L"");
	for (size_t i = 0; i < FConditions.size(); ++i)
		cbCondition->Items->Add(FConditions[i].Code + L" — " + FConditions[i].Name);
}
//---------------------------------------------------------------------------
// Периоды выбранной страны; для новой (ещё не сохранённой) страны список пуст.
void TfmCoinEdit::LoadPeriods()
{
	const String country = cbCountry->Text.Trim();
	if (SameText(country, FLoadedCountry) && cbPeriod->Items->Count > 0)
		return;
	FLoadedCountry = country;

	const String period = cbPeriod->Text;
	cbPeriod->Items->BeginUpdate();
	cbPeriod->Items->Clear();
	for (size_t i = 0; i < FCountries.size(); ++i)
	{
		if (!SameText(FCountries[i].Name, country))
			continue;
		const std::vector<TPeriod> periods = FRepository->Lookups().Periods(FCountries[i].Id);
		for (size_t p = 0; p < periods.size(); ++p)
			if (!periods[p].Name.IsEmpty())
				cbPeriod->Items->Add(periods[p].Name);
		break;
	}
	cbPeriod->Items->EndUpdate();
	cbPeriod->Text = period;	// введённый текст не теряется
}
//---------------------------------------------------------------------------
void __fastcall TfmCoinEdit::cbCountryChange(TObject *Sender)
{
	LoadPeriods();
}
//---------------------------------------------------------------------------
String TfmCoinEdit::NumberText(double AValue)
{
	return AValue == 0.0 ? String() : FloatToStr(AValue);
}
//---------------------------------------------------------------------------
void TfmCoinEdit::LoadFrom(const TCoinRecord &R)
{
	cbCountry->Text       = R.Country;
	LoadPeriods();
	cbPeriod->Text        = R.Period;
	cbCurrency->Text      = R.Currency;
	edDenomination->Text  = R.Denomination;
	edYear->Text          = R.Year == 0 ? String() : IntToStr(R.Year);
	edVariety->Text       = R.Variety;
	edDiameter->Text      = NumberText(R.DiameterMm);
	edCatalogNumber->Text = R.CatalogNumber;
	edSubject->Text       = R.Subject;

	cbCondition->ItemIndex = 0;
	for (size_t i = 0; i < FConditions.size(); ++i)
		if (SameText(FConditions[i].Code, R.Condition))
			cbCondition->ItemIndex = (int)i + 1;
	udQuantity->Position   = (short)std::max(1, std::min(999, R.Quantity));
	chkNeedToReplace->Checked = R.NeedToReplace;
	edCatalogValue->Text   = NumberText(R.CatalogValueUah);
	edMyValue->Text        = NumberText(R.MyValueUah);

	// Снятый флажок — даты нет; в поле при этом стоит сегодняшняя дата.
	dtPurchaseDate->Date    = (double)R.PurchaseDate == 0.0 ? Date() : R.PurchaseDate;
	dtPurchaseDate->Checked = (double)R.PurchaseDate != 0.0;
	dtPublishedDate->Date    = (double)R.PublishedDate == 0.0 ? Date() : R.PublishedDate;
	dtPublishedDate->Checked = (double)R.PublishedDate != 0.0;

	edColor->Text          = R.Color;
	edGradingCompany->Text = R.GradingCompany;
	edGradingNumber->Text  = R.GradingNumber;
	edGrade->Text          = R.Grade;
	edLabel->Text          = R.LabelText;
	edSwap->Text           = R.SwapInfo;
	mmComment->Text        = R.Comment;
}
//---------------------------------------------------------------------------
// Число с разделителем из настроек Windows; точка тоже принимается.
// Пробелы (разделители разрядов) игнорируются.
bool TfmCoinEdit::TryParseNumber(const String &AText, double &AValue)
{
	String text = StringReplace(AText.Trim(), L" ", L"", TReplaceFlags() << rfReplaceAll);
	text = StringReplace(text, L" ", L"", TReplaceFlags() << rfReplaceAll);
	AValue = 0.0;
	if (text.IsEmpty())
		return true;
	return TryStrToFloat(text, AValue) || TryStrToFloat(text, AValue, TFormatSettings::Invariant());
}
//---------------------------------------------------------------------------
String TfmCoinEdit::FieldName(TLabel *ALabel)
{
	String name = StripHotkey(ALabel->Caption).Trim();
	if (name.Length() > 0 && name[name.Length()] == L':')
		name.SetLength(name.Length() - 1);
	return name;
}
//---------------------------------------------------------------------------
bool TfmCoinEdit::Fail(TWinControl *AControl, const String &AMessage)
{
	Application->MessageBox(AMessage.c_str(), Caption.c_str(), MB_OK | MB_ICONWARNING);
	if (AControl->CanFocus())
		AControl->SetFocus();
	return false;
}
//---------------------------------------------------------------------------
bool TfmCoinEdit::Validate()
{
	if (cbCountry->Text.Trim().IsEmpty())
		return Fail(cbCountry, Format(Tr(L"CoinEdit.Error.Required"), ARRAYOFCONST((FieldName(lblCountry)))));
	if (edDenomination->Text.Trim().IsEmpty())
		return Fail(edDenomination, Format(Tr(L"CoinEdit.Error.Required"), ARRAYOFCONST((FieldName(lblDenomination)))));

	if (!edYear->Text.Trim().IsEmpty())
	{
		const int maxYear = CurrentYear() + 1;
		const int year = StrToIntDef(edYear->Text.Trim(), 0);
		if (year < 1 || year > maxYear)
			return Fail(edYear, Format(Tr(L"CoinEdit.Error.Year"), ARRAYOFCONST((maxYear))));
	}

	double value = 0.0;
	if (!TryParseNumber(edDiameter->Text, value) || value < 0.0 || (!edDiameter->Text.Trim().IsEmpty() && value == 0.0))
		return Fail(edDiameter, Format(Tr(L"CoinEdit.Error.Positive"), ARRAYOFCONST((FieldName(lblDiameter)))));
	if (!TryParseNumber(edCatalogValue->Text, value) || value < 0.0)
		return Fail(edCatalogValue, Format(Tr(L"CoinEdit.Error.NotNegative"), ARRAYOFCONST((FieldName(lblCatalogValue)))));
	if (!TryParseNumber(edMyValue->Text, value) || value < 0.0)
		return Fail(edMyValue, Format(Tr(L"CoinEdit.Error.NotNegative"), ARRAYOFCONST((FieldName(lblMyValue)))));

	const int quantity = StrToIntDef(edQuantity->Text.Trim(), 0);
	if (quantity < 1 || quantity > 999)
		return Fail(edQuantity, Format(Tr(L"CoinEdit.Error.Quantity"), ARRAYOFCONST((FieldName(lblQuantity)))));
	return true;
}
//---------------------------------------------------------------------------
void __fastcall TfmCoinEdit::btnOKClick(TObject *Sender)
{
	if (Validate())
		ModalResult = mrOk;
}
//---------------------------------------------------------------------------
void TfmCoinEdit::SaveTo(TCoinRecord &R) const
{
	double value = 0.0;
	R.Country       = cbCountry->Text.Trim();
	R.Period        = cbPeriod->Text.Trim();
	R.Currency      = cbCurrency->Text.Trim();
	R.Denomination  = edDenomination->Text.Trim();
	R.Year          = StrToIntDef(edYear->Text.Trim(), 0);
	R.Variety       = edVariety->Text.Trim();
	R.DiameterMm    = TryParseNumber(edDiameter->Text, value) ? value : 0.0;
	R.CatalogNumber = edCatalogNumber->Text.Trim();
	R.Subject       = edSubject->Text.Trim();

	const int condition = cbCondition->ItemIndex;
	R.Condition       = condition > 0 && condition <= (int)FConditions.size() ? FConditions[condition - 1].Code : String();
	R.Quantity        = StrToIntDef(edQuantity->Text.Trim(), 1);
	R.NeedToReplace   = chkNeedToReplace->Checked;
	R.CatalogValueUah = TryParseNumber(edCatalogValue->Text, value) ? value : 0.0;
	R.MyValueUah      = TryParseNumber(edMyValue->Text, value) ? value : 0.0;
	R.PurchaseDate    = dtPurchaseDate->Checked ? TDateTime((int)dtPurchaseDate->Date) : TDateTime(0.0);
	R.PublishedDate   = dtPublishedDate->Checked ? TDateTime((int)dtPublishedDate->Date) : TDateTime(0.0);
	R.Color           = edColor->Text.Trim();
	R.GradingCompany  = edGradingCompany->Text.Trim();
	R.GradingNumber   = edGradingNumber->Text.Trim();
	R.Grade           = edGrade->Text.Trim();
	R.LabelText       = edLabel->Text.Trim();
	R.SwapInfo        = edSwap->Text.Trim();
	R.Comment         = mmComment->Text.Trim();
}
//---------------------------------------------------------------------------
