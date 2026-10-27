//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <algorithm>
#include <memory>
#include <shellapi.h>
#include "f_NumistaChoose.h"
#include "u_Translator.h"
#include "u_Help.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
//---------------------------------------------------------------------------
namespace
{
	enum { ccId, ccTitle, ccIssuer, ccYears, ccCount };
	const wchar_t *const cColumnKeys[ccCount] =
	{
		L"NumistaChoose.Column.Id", L"NumistaChoose.Column.Title",
		L"NumistaChoose.Column.Issuer", L"NumistaChoose.Column.Years"
	};
}
//---------------------------------------------------------------------------
__fastcall TfmNumistaChoose::TfmNumistaChoose(TComponent* Owner)
	: TForm(Owner),
	  FMatcher(nullptr)
{
	Translator().TranslateForm(this);
	InstallHelpKey(this, L"numista-link");
}
//---------------------------------------------------------------------------
int TfmNumistaChoose::Execute(TComponent *AOwner, const String &ACoinText, const std::vector<TNumistaType> &ATypes,
	TNumistaMatcher &AMatcher, String &AQuery)
{
	std::unique_ptr<TfmNumistaChoose> form(new TfmNumistaChoose(AOwner));
	form->FMatcher = &AMatcher;
	form->lblCoin->Caption = ACoinText;
	form->edQuery->Text = AQuery;
	form->ShowTypes(ATypes);
	form->FormResize(nullptr);
	if (ATypes.empty())
		form->ActiveControl = form->edQuery;	// ничего не найдено — сразу править фразу
	const bool ok = form->ShowModal() == mrOk;
	AQuery = form->edQuery->Text.Trim();
	if (!ok)
		return 0;
	const int index = form->Selected();
	return index >= 0 ? form->FTypes[index].Id : 0;
}
//---------------------------------------------------------------------------
void TfmNumistaChoose::ShowTypes(const std::vector<TNumistaType> &ATypes)
{
	FTypes = ATypes;
	dgTypes->RowCount = std::max<int>(2, (int)FTypes.size() + 1);
	dgTypes->Row = 1;
	btnLink->Enabled = !FTypes.empty();
	dgTypes->Invalidate();
}
//---------------------------------------------------------------------------
// Новый поиск по фразе (searchTypes q=…; 1 запрос, ответ кэшируется). Ошибка — сообщение,
// список прежний.
void __fastcall TfmNumistaChoose::btnSearchClick(TObject *Sender)
{
	if (!FMatcher || edQuery->Text.Trim().IsEmpty())
		return;
	const TCursor saved = Screen->Cursor;
	Screen->Cursor = crHourGlass;
	try
	{
		std::vector<TNumistaType> types = FMatcher->SearchPhrase(edQuery->Text);
		Screen->Cursor = saved;
		ShowTypes(types);
		if (!types.empty())
			ActiveControl = dgTypes;
	}
	catch (Exception &e)
	{
		Screen->Cursor = saved;
		Application->MessageBox(e.Message.c_str(), Caption.c_str(), MB_OK | MB_ICONERROR);
	}
}
//---------------------------------------------------------------------------
// В поле фразы Enter — поиск, а не Link (Default-кнопка).
void __fastcall TfmNumistaChoose::edQueryEnter(TObject *Sender)
{
	btnSearch->Default = true;
}
//---------------------------------------------------------------------------
void __fastcall TfmNumistaChoose::edQueryExit(TObject *Sender)
{
	btnLink->Default = true;
}
//---------------------------------------------------------------------------
void __fastcall TfmNumistaChoose::dgTypesSelectCell(TObject *Sender, int ACol, int ARow, bool &CanSelect)
{
	CanSelect = ARow - 1 < (int)FTypes.size();
}
//---------------------------------------------------------------------------
int TfmNumistaChoose::Selected() const
{
	const int index = dgTypes->Row - 1;
	return index >= 0 && index < (int)FTypes.size() ? index : -1;
}
//---------------------------------------------------------------------------
void __fastcall TfmNumistaChoose::FormResize(TObject *Sender)
{
	const int fixed = 70 + 150 + 80;
	dgTypes->ColWidths[ccId]     = 70;
	dgTypes->ColWidths[ccIssuer] = 150;
	dgTypes->ColWidths[ccYears]  = 80;
	dgTypes->ColWidths[ccTitle]  = std::max(150, dgTypes->ClientWidth - fixed - 4 * dgTypes->GridLineWidth);
}
//---------------------------------------------------------------------------
void __fastcall TfmNumistaChoose::dgTypesDrawCell(TObject *Sender, int ACol, int ARow, TRect &Rect, TGridDrawState State)
{
	String text;
	if (ARow == 0)
		text = Tr(cColumnKeys[ACol]);
	else if (FTypes.empty() && ARow == 1 && ACol == ccTitle)
		text = Tr(L"NumistaChoose.Nothing");
	else if (ARow - 1 < (int)FTypes.size())
	{
		const TNumistaType &t = FTypes[ARow - 1];
		switch (ACol)
		{
			case ccId:     text = IntToStr(t.Id); break;
			case ccTitle:  text = t.Title; break;
			case ccIssuer: text = t.IssuerName; break;
			case ccYears:
				if (t.MinYear != 0)
					text = t.MaxYear != 0 && t.MaxYear != t.MinYear
						? IntToStr(t.MinYear) + L"\u2013" + IntToStr(t.MaxYear) : IntToStr(t.MinYear);
				break;
		}
	}
	TCanvas *canvas = dgTypes->Canvas;
	RECT r = { Rect.Left + 4, Rect.Top, Rect.Right - 4, Rect.Bottom };
	canvas->Brush->Style = bsClear;
	DrawTextW(canvas->Handle, text.c_str(), -1, &r, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
}
//---------------------------------------------------------------------------
void __fastcall TfmNumistaChoose::dgTypesDblClick(TObject *Sender)
{
	if (Selected() >= 0)
		ModalResult = mrOk;
}
//---------------------------------------------------------------------------
void __fastcall TfmNumistaChoose::btnOpenClick(TObject *Sender)
{
	const int index = Selected();
	if (index >= 0)
		ShellExecuteW(Handle, L"open", TNumistaMatcher::TypeUrl(FTypes[index].Id).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}
//---------------------------------------------------------------------------
