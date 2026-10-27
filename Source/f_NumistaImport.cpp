//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <algorithm>
#include <memory>
#include <set>
#include <shellapi.h>
#include "f_NumistaImport.h"
#include "u_NumistaMatcher.h"
#include "u_Translator.h"
#include "u_Help.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
// TForm(AOwner, 0) — CreateNew: у формы нет .dfm.
__fastcall TfmNumistaImport::TfmNumistaImport(TComponent *AOwner)
	: TForm(AOwner, 0),
	  FIssuer(nullptr),
	  FList(nullptr),
	  FCount(nullptr),
	  FImport(nullptr),
	  FFilling(false)
{
	Caption     = Tr(L"NumistaImport.Title");
	BorderStyle = bsSizeable;
	BorderIcons = TBorderIcons() << biSystemMenu << biMaximize;
	Position    = poOwnerFormCenter;
	Width       = 920;
	Height      = 520;
	Constraints->MinWidth  = 560;
	Constraints->MinHeight = 300;

	TPanel *top = new TPanel(this);
	top->Parent = this;
	top->Align = alTop;
	top->Height = 36;
	top->BevelOuter = bvNone;

	TLabel *issuerLabel = new TLabel(this);
	issuerLabel->Parent = top;
	issuerLabel->SetBounds(8, 11, 60, 13);
	issuerLabel->Caption = Tr(L"NumistaImport.Issuer");

	FIssuer = new TComboBox(this);
	FIssuer->Parent = top;
	FIssuer->Style = csDropDownList;
	FIssuer->SetBounds(72, 7, 260, 21);
	FIssuer->DropDownCount = 20;
	FIssuer->OnChange = IssuerChange;
	issuerLabel->FocusControl = FIssuer;

	auto button = [this, top](int ALeft, int AWidth, const String &ACaption, TNotifyEvent AClick) {
		TButton *b = new TButton(this);
		b->Parent = top;
		b->SetBounds(ALeft, 6, AWidth, 23);
		b->Caption = ACaption;
		b->OnClick = AClick;
		return b;
	};
	button(344, 140, Tr(L"NumistaImport.CheckAll"), CheckAllClick);
	button(490, 140, Tr(L"NumistaImport.UncheckAll"), UncheckAllClick);

	TPanel *bottom = new TPanel(this);
	bottom->Parent = this;
	bottom->Align = alBottom;
	bottom->Height = 44;
	bottom->BevelOuter = bvNone;

	FCount = new TLabel(this);
	FCount->Parent = bottom;
	FCount->SetBounds(8, 15, 300, 13);

	TButton *cancel = new TButton(this);
	cancel->Parent = bottom;
	cancel->Anchors = TAnchors() << akTop << akRight;
	cancel->SetBounds(ClientWidth - 92, 10, 84, 25);
	cancel->Caption = Tr(L"NumistaImport.Cancel");
	cancel->Cancel = true;
	cancel->ModalResult = mrCancel;

	FImport = new TButton(this);
	FImport->Parent = bottom;
	FImport->Anchors = TAnchors() << akTop << akRight;
	FImport->SetBounds(ClientWidth - 92 - 6 - 110, 10, 110, 25);
	FImport->Caption = Tr(L"NumistaImport.Import");
	FImport->Default = true;
	FImport->ModalResult = mrOk;

	FList = new TListView(this);
	FList->Parent = this;
	FList->Align = alClient;
	FList->AlignWithMargins = true;
	FList->ViewStyle = vsReport;
	FList->Checkboxes = true;
	FList->RowSelect = true;
	FList->ReadOnly = true;
	FList->HideSelection = false;
	FList->OnItemChecked = ListItemChecked;
	FList->OnDblClick = ListDblClick;
	const wchar_t *const columns[] = { L"NumistaImport.Column.Issuer", L"NumistaImport.Column.Title",
		L"NumistaImport.Column.Year", L"NumistaImport.Column.Grade", L"NumistaImport.Column.InProgram" };
	const int widths[] = { 170, 300, 80, 55, 260 };
	for (int i = 0; i < 5; ++i)
	{
		TListColumn *c = FList->Columns->Add();
		c->Caption = Tr(columns[i]);
		c->Width = widths[i];
	}

	InstallHelpKey(this, L"import");
}
//---------------------------------------------------------------------------
bool TfmNumistaImport::Execute(TComponent *AOwner, const std::vector<TNumistaImportRow> &ARows,
	std::vector<int> &ASelected)
{
	std::unique_ptr<TfmNumistaImport> form(new TfmNumistaImport(AOwner));
	form->FRows = ARows;
	form->FChecked.assign(ARows.size(), false);

	// Эмитенты по алфавиту, сверху — «все».
	std::set<String> issuers;
	for (size_t i = 0; i < ARows.size(); ++i)
		issuers.insert(ARows[i].Issuer);
	form->FIssuer->Items->Add(Tr(L"NumistaImport.AllIssuers"));
	for (std::set<String>::const_iterator it = issuers.begin(); it != issuers.end(); ++it)
		form->FIssuer->Items->Add(*it);
	form->FIssuer->ItemIndex = 0;
	form->Fill();

	ASelected.clear();
	if (form->ShowModal() != mrOk)
		return false;
	for (size_t i = 0; i < form->FChecked.size(); ++i)
		if (form->FChecked[i])
			ASelected.push_back((int)i);
	return !ASelected.empty();
}
//---------------------------------------------------------------------------
// Строки выбранного эмитента; отметки — из FChecked.
void TfmNumistaImport::Fill()
{
	const String issuer = FIssuer->ItemIndex > 0 ? FIssuer->Items->Strings[FIssuer->ItemIndex] : String();
	FFilling = true;
	FList->Items->BeginUpdate();
	try
	{
		FList->Items->Clear();
		for (size_t i = 0; i < FRows.size(); ++i)
		{
			const TNumistaImportRow &r = FRows[i];
			if (!issuer.IsEmpty() && r.Issuer != issuer)
				continue;
			TListItem *item = FList->Items->Add();
			item->Caption = r.Issuer;
			item->SubItems->Add(r.Title);
			item->SubItems->Add(r.Year);
			item->SubItems->Add(r.Grade);
			item->SubItems->Add(r.InProgram.IsEmpty() ? Tr(L"NumistaImport.New") : r.InProgram);
			item->Data = reinterpret_cast<void *>((NativeInt)i);
			item->Checked = FChecked[i];
		}
	}
	__finally
	{
		FList->Items->EndUpdate();
		FFilling = false;
	}
	UpdateCount();
}
//---------------------------------------------------------------------------
void TfmNumistaImport::UpdateCount()
{
	const int checked = (int)std::count(FChecked.begin(), FChecked.end(), true);
	FCount->Caption = Format(Tr(L"NumistaImport.Count"), ARRAYOFCONST((checked, (int)FRows.size())));
	FImport->Enabled = checked > 0;
}
//---------------------------------------------------------------------------
void __fastcall TfmNumistaImport::IssuerChange(TObject *Sender)
{
	Fill();
}
//---------------------------------------------------------------------------
// «Все в списке» — только показанные строки (выбранный эмитент).
void __fastcall TfmNumistaImport::CheckAllClick(TObject *Sender)
{
	for (int i = 0; i < FList->Items->Count; ++i)
		FList->Items->Item[i]->Checked = true;
}
//---------------------------------------------------------------------------
void __fastcall TfmNumistaImport::UncheckAllClick(TObject *Sender)
{
	for (int i = 0; i < FList->Items->Count; ++i)
		FList->Items->Item[i]->Checked = false;
}
//---------------------------------------------------------------------------
void __fastcall TfmNumistaImport::ListItemChecked(TObject *Sender, TListItem *Item)
{
	if (FFilling || !Item)
		return;
	FChecked[(NativeInt)Item->Data] = Item->Checked;
	UpdateCount();
}
//---------------------------------------------------------------------------
// Двойной клик — тип на сайте Numista.
void __fastcall TfmNumistaImport::ListDblClick(TObject *Sender)
{
	if (!FList->Selected)
		return;
	const TNumistaImportRow &r = FRows[(NativeInt)FList->Selected->Data];
	ShellExecuteW(Handle, L"open", TNumistaMatcher::TypeUrl(r.TypeId).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}
//---------------------------------------------------------------------------
