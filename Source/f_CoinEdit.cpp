//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <algorithm>
#include <math.h>
#include <memory>
#include <shellapi.h>
#include <Vcl.Clipbrd.hpp>
#include <Vcl.ExtDlgs.hpp>
#include <Vcl.Imaging.jpeg.hpp>
#include "f_CoinEdit.h"
#include "u_CoinRepository.h"
#include "u_NumistaMatcher.h"
#include "u_Help.h"
#include "u_Translator.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TfmCoinEdit *fmCoinEdit;
//---------------------------------------------------------------------------
__fastcall TfmCoinEdit::TfmCoinEdit(TComponent* Owner)
	: TForm(Owner),
	  FRepository(nullptr),
	  FPhotos(nullptr)
{
	Translator().TranslateForm(this);
	InstallHelpKey(this, L"coin-card");
	CreateItemPhotoColumn();
	// Автофраза поиска Numista — подсказкой в пустом поле, обновляется при правке её частей.
	edDenomination->OnChange = QueryPartChange;
	edYear->OnChange         = QueryPartChange;
	cbPeriod->OnChange       = QueryPartChange;
	edSubject->OnChange      = QueryPartChange;
}
//---------------------------------------------------------------------------
bool TfmCoinEdit::Execute(TComponent *AOwner, TCoinRepository &ARepository, TPhotoRepository &APhotos,
	TCoinRecord &ARecord, TPhotoEdits &APhotoEdits, bool ASample)
{
	std::unique_ptr<TfmCoinEdit> form(new TfmCoinEdit(AOwner));
	form->FRepository = &ARepository;
	form->FPhotos     = &APhotos;
	form->Caption = Tr(ASample ? L"CoinEdit.TitleSample" : ARecord.Id == 0 ? L"CoinEdit.TitleNew" : L"CoinEdit.TitleEdit");

	// Поля монеты общие для всех её экземпляров — предупредить, если их несколько.
	const int items = ARecord.CoinId == 0 ? 0 : ARepository.ItemCount(ARecord.CoinId);
	form->lblShared->Caption = Format(Tr(L"CoinEdit.Shared"), ARRAYOFCONST((items)));
	form->lblShared->Visible = items > 1;

	form->LoadLookups();
	form->LoadFrom(ARecord);
	form->LoadPhotos(ARecord);
	if (ASample)
		form->ActiveControl = form->edYear;	// обычно меняется только год (текст выделяется)
	if (form->ShowModal() != mrOk)
		return false;

	form->SaveTo(ARecord);
	APhotoEdits = form->FEdits;
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
	QueryPartChange(nullptr);
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
	edYearTo->Text        = R.YearTo == 0 || R.YearTo == R.Year ? String() : IntToStr(R.YearTo);
	edVariety->Text       = R.Variety;
	edDiameter->Text      = NumberText(R.DiameterMm);
	edCatalogNumber->Text = R.CatalogNumber;
	edSubject->Text       = R.Subject;
	edNumistaQuery->Text  = R.NumistaQuery;
	QueryPartChange(nullptr);

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

	// Год: до н. э. — отрицательный (задача 20); «по» — последний год диапазона.
	const int maxYear = CurrentYear() + 1;
	int year = 0;
	if (!edYear->Text.Trim().IsEmpty())
	{
		year = StrToIntDef(edYear->Text.Trim(), 0);
		if (year == 0 || year < -3000 || year > maxYear)
			return Fail(edYear, Format(Tr(L"CoinEdit.Error.Year"), ARRAYOFCONST((maxYear))));
	}
	if (!edYearTo->Text.Trim().IsEmpty())
	{
		const int yearTo = StrToIntDef(edYearTo->Text.Trim(), 0);
		if (year == 0 || yearTo == 0 || yearTo < year || yearTo > maxYear)
			return Fail(edYearTo, Format(Tr(L"CoinEdit.Error.YearTo"), ARRAYOFCONST((maxYear))));
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
	R.YearTo        = StrToIntDef(edYearTo->Text.Trim(), 0);
	if (R.YearTo == R.Year)
		R.YearTo = 0;
	R.Variety       = edVariety->Text.Trim();
	R.DiameterMm    = TryParseNumber(edDiameter->Text, value) ? value : 0.0;
	R.CatalogNumber = edCatalogNumber->Text.Trim();
	R.Subject       = edSubject->Text.Trim();
	R.NumistaQuery  = edNumistaQuery->Text.Trim();

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
//---------------------------------------------------------------------------
// Фото (задача 15)
//---------------------------------------------------------------------------
bool TPhotoEdits::Empty() const
{
	for (int s = 0; s < 2; ++s)
		if (Item[s].Action != TPhotoEdit::paNone || Coin[s].Action != TPhotoEdit::paNone)
			return false;
	return true;
}
//---------------------------------------------------------------------------
// Сначала фото типа, затем экземпляра (порядок не важен — владельцы разные).
void TPhotoEdits::Apply(TPhotoRepository &ARepository, int AItemId, int ACoinId) const
{
	for (int s = 0; s < 2; ++s)
	{
		const TPhotoSide side = (TPhotoSide)s;
		if (Coin[s].Action == TPhotoEdit::paSet)
			ARepository.SetCoinPhoto(ACoinId, side, Coin[s].Graphic.get());
		else if (Coin[s].Action == TPhotoEdit::paRemove)
			ARepository.RemoveCoinPhoto(ACoinId, side);
		if (Item[s].Action == TPhotoEdit::paSet)
			ARepository.SetItemPhoto(AItemId, side, Item[s].Graphic.get());
		else if (Item[s].Action == TPhotoEdit::paRemove)
			ARepository.RemoveItemPhoto(AItemId, side);
	}
}
//---------------------------------------------------------------------------
void __fastcall TfmCoinEdit::CreateWnd()
{
	TForm::CreateWnd();
	DragAcceptFiles(Handle, TRUE);
}
//---------------------------------------------------------------------------
// Колонка своих фото экземпляра — копия колонки из .dfm правее; кнопки — с Tag = колонка * 2 + сторона.
void TfmCoinEdit::CreateItemPhotoColumn()
{
	FPhotoControls[pcType][psObverse] = { lblObverse, pnlObverse, imgObverse, lblObverseNote,
		btnObverseLoad, btnObversePaste, btnObverseRemove };
	FPhotoControls[pcType][psReverse] = { lblReverse, pnlReverse, imgReverse, lblReverseNote,
		btnReverseLoad, btnReversePaste, btnReverseRemove };
	const int shift = pnlObverse->Width + 12;
	auto place = [shift](TControl *AControl, TControl *ASample)
	{
		AControl->SetBounds(ASample->Left + shift, ASample->Top, ASample->Width, ASample->Height);
	};
	for (int s = 0; s < 2; ++s)
	{
		TPhotoControls &type = FPhotoControls[pcType][s];
		TPhotoControls &item = FPhotoControls[pcItem][s];
		item.Caption = new TLabel(this);
		item.Caption->Parent   = gbPhotos;
		item.Caption->AutoSize = false;
		place(item.Caption, type.Caption);
		item.Frame = new TPanel(this);
		item.Frame->Parent           = gbPhotos;
		item.Frame->BevelOuter       = type.Frame->BevelOuter;
		item.Frame->Color            = type.Frame->Color;
		item.Frame->ParentBackground = false;
		place(item.Frame, type.Frame);
		item.Image = new TImage(this);
		item.Image->Parent       = item.Frame;
		item.Image->Align        = alClient;
		item.Image->Center       = true;
		item.Image->Proportional = true;
		item.Image->Stretch      = true;
		item.Note = new TLabel(this);
		item.Note->Parent   = gbPhotos;
		item.Note->AutoSize = false;
		item.Note->Font->Assign(type.Note->Font);
		place(item.Note, type.Note);
		auto button = [&](TButton *ASample, TNotifyEvent AOnClick)
		{
			TButton *b = new TButton(this);
			b->Parent  = gbPhotos;
			b->Caption = ASample->Caption;
			b->OnClick = AOnClick;
			place(b, ASample);
			return b;
		};
		item.Load   = button(type.Load, PhotoLoadClick);
		item.Paste  = button(type.Paste, PhotoPasteClick);
		item.Remove = button(type.Remove, PhotoRemoveClick);
	}
	for (int c = 0; c < 2; ++c)
		for (int s = 0; s < 2; ++s)
		{
			TPhotoControls &p = FPhotoControls[c][s];
			p.Load->Tag   = c * 2 + s;
			p.Paste->Tag  = c * 2 + s;
			p.Remove->Tag = c * 2 + s;
			p.Caption->ShowAccelChar = false;
			p.Caption->Caption = Tr(s == psObverse ? L"Main.Details.Obverse" : L"Main.Details.Reverse") + L" — "
				+ Tr(c == pcType ? L"CoinEdit.ColumnType" : L"CoinEdit.ColumnItem");
			p.Note->ShowAccelChar = false;
			p.Note->EllipsisPosition = epEndEllipsis;	// длинное имя автора фото
		}
}
//---------------------------------------------------------------------------
void TfmCoinEdit::LoadPhotos(const TCoinRecord &ARecord)
{
	for (int s = 0; s < 2; ++s)
	{
		const TPhotoSide side = (TPhotoSide)s;
		FItemFile[s] = ARecord.Id == 0 ? String() : FPhotos->PhotoFile(ARecord.Id, 0, side);
		FCoinFile[s] = ARecord.CoinId == 0 ? String() : FPhotos->PhotoFile(0, ARecord.CoinId, side, nullptr, &FCoinCopyright[s]);
		ShowPhoto(pcType, side);
		ShowPhoto(pcItem, side);
	}
}
//---------------------------------------------------------------------------
TPhotoEdit &TfmCoinEdit::TargetEdit(int AColumn, TPhotoSide ASide)
{
	return AColumn == pcType ? FEdits.Coin[ASide] : FEdits.Item[ASide];
}
//---------------------------------------------------------------------------
// Колонка показывает фото своего владельца: новое (ещё не сохранённое) или сохранённое.
void TfmCoinEdit::ShowPhoto(int AColumn, TPhotoSide ASide)
{
	const TPhotoControls &p = FPhotoControls[AColumn][ASide];
	const TPhotoEdit &edit = TargetEdit(AColumn, ASide);
	const String stored = AColumn == pcType ? FCoinFile[ASide] : FItemFile[ASide];

	TGraphic *graphic = nullptr;
	String file;
	if (edit.Action == TPhotoEdit::paSet)
		graphic = edit.Graphic.get();
	else if (edit.Action != TPhotoEdit::paRemove)
		file = stored;

	try
	{
		if (graphic)
			p.Image->Picture->Assign(graphic);
		else if (!file.IsEmpty())
			p.Image->Picture->LoadFromFile(file);
		else
			p.Image->Picture->Assign(nullptr);
	}
	catch (Exception &)
	{
		p.Image->Picture->Assign(nullptr);	// повреждённый файл — как «нет фото»
	}
	// Под фото типа из Numista — автор (как в просмотре).
	p.Note->Caption = AColumn == pcType && !file.IsEmpty() && !FCoinCopyright[ASide].IsEmpty()
		? Format(Tr(L"PhotoView.Copyright"), ARRAYOFCONST((FCoinCopyright[ASide]))) : String();
	p.Remove->Enabled = edit.Action == TPhotoEdit::paSet || (edit.Action == TPhotoEdit::paNone && !stored.IsEmpty());
}
//---------------------------------------------------------------------------
void TfmCoinEdit::SetPhoto(int AColumn, TPhotoSide ASide, TGraphic *AGraphic)
{
	TPhotoEdit &target = TargetEdit(AColumn, ASide);
	target.Action = TPhotoEdit::paSet;
	target.Graphic.reset(AGraphic);
	ShowPhoto(AColumn, ASide);
}
//---------------------------------------------------------------------------
// Формат определяет WIC (JPEG, PNG, BMP, TIFF, GIF ...); не картинка — сообщение.
bool TfmCoinEdit::LoadPhotoFile(int AColumn, TPhotoSide ASide, const String &AFileName)
{
	std::unique_ptr<TWICImage> image(new TWICImage());
	try
	{
		image->LoadFromFile(AFileName);
	}
	catch (Exception &)
	{
		Application->MessageBox(Format(Tr(L"Photo.Error.NotImage"), ARRAYOFCONST((ExtractFileName(AFileName)))).c_str(),
			Caption.c_str(), MB_OK | MB_ICONWARNING);
		return false;
	}
	SetPhoto(AColumn, ASide, image.release());
	return true;
}
//---------------------------------------------------------------------------
void __fastcall TfmCoinEdit::PhotoLoadClick(TObject *Sender)
{
	const int tag = (int)static_cast<TComponent *>(Sender)->Tag;
	std::unique_ptr<TOpenPictureDialog> dialog(new TOpenPictureDialog(nullptr));
	dialog->Filter = Tr(L"CoinEdit.PhotoFilter");
	dialog->Options = dialog->Options << ofFileMustExist;
	if (dialog->Execute(Handle))
		LoadPhotoFile(tag / 2, (TPhotoSide)(tag % 2), dialog->FileName);
}
//---------------------------------------------------------------------------
void __fastcall TfmCoinEdit::PhotoPasteClick(TObject *Sender)
{
	const int tag = (int)static_cast<TComponent *>(Sender)->Tag;
	if (!Clipboard()->HasFormat(CF_BITMAP))
	{
		Application->MessageBox(Tr(L"CoinEdit.NoClipboardImage").c_str(), Caption.c_str(), MB_OK | MB_ICONINFORMATION);
		return;
	}
	std::unique_ptr<TBitmap> bitmap(new TBitmap());
	bitmap->Assign(Clipboard());
	SetPhoto(tag / 2, (TPhotoSide)(tag % 2), bitmap.release());
}
//---------------------------------------------------------------------------
// Новое фото колонки — отменить; сохранённое — удалить по OK.
void __fastcall TfmCoinEdit::PhotoRemoveClick(TObject *Sender)
{
	const int tag = (int)static_cast<TComponent *>(Sender)->Tag;
	const int column = tag / 2;
	const TPhotoSide side = (TPhotoSide)(tag % 2);
	TPhotoEdit &target = TargetEdit(column, side);
	const String stored = column == pcType ? FCoinFile[side] : FItemFile[side];
	target.Action = stored.IsEmpty() ? TPhotoEdit::paNone : TPhotoEdit::paRemove;
	target.Graphic.reset();
	ShowPhoto(column, side);
}
//---------------------------------------------------------------------------
// Файл, брошенный на рамку аверса или реверса (по положению курсора); первый из нескольких.
void __fastcall TfmCoinEdit::WMDropFiles(TWMDropFiles &AMessage)
{
	HDROP drop = reinterpret_cast<HDROP>(AMessage.Drop);
	try
	{
		const TPoint cursor = Mouse->CursorPos;
		int column = -1, side = -1;
		for (int c = 0; c < 2; ++c)
			for (int s = 0; s < 2; ++s)
			{
				TPanel *frame = FPhotoControls[c][s].Frame;
				if (PtInRect(frame->BoundsRect, frame->Parent->ScreenToClient(cursor)))
				{
					column = c;
					side = s;
				}
			}
		if (side >= 0 && DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0) > 0)
		{
			wchar_t name[MAX_PATH];
			if (DragQueryFileW(drop, 0, name, MAX_PATH) > 0)
				LoadPhotoFile(column, (TPhotoSide)side, name);
		}
	}
	__finally
	{
		DragFinish(drop);
	}
	AMessage.Result = 0;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Фраза поиска Numista (задача 15b): пустое поле — автофраза (страна, номинал / сюжет, год;
// задача 25) серой подсказкой.
void __fastcall TfmCoinEdit::QueryPartChange(TObject *Sender)
{
	TCoinRecord r;
	r.Country      = cbCountry->Text;
	r.Denomination = edDenomination->Text;
	r.Period       = cbPeriod->Text;
	r.Year         = StrToIntDef(edYear->Text.Trim(), 0);
	r.Subject      = edSubject->Text;
	edNumistaQuery->TextHint = TNumistaMatcher::AutoQuery(r);
}
//---------------------------------------------------------------------------
