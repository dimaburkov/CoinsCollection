//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <algorithm>
#include <memory>
#include <shellapi.h>
#include "f_NumistaIssue.h"
#include "u_Translator.h"
#include "u_Help.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
//---------------------------------------------------------------------------
namespace
{
	enum { icYear, icMint, icMintage, icComment, icCount };
	const wchar_t *const cColumnKeys[icCount] =
	{
		L"NumistaIssue.Column.Year", L"NumistaIssue.Column.Mint",
		L"NumistaIssue.Column.Mintage", L"NumistaIssue.Column.Comment"
	};
	const int cTypeOnly = mrNo;	// ModalResult кнопки Type only
}
//---------------------------------------------------------------------------
__fastcall TfmNumistaIssue::TfmNumistaIssue(TComponent* Owner)
	: TForm(Owner),
	  FYear(0),
	  FTypeId(0)
{
	Translator().TranslateForm(this);
	InstallHelpKey(this, L"numista-link");
	btnTypeOnly->ModalResult = cTypeOnly;
}
//---------------------------------------------------------------------------
int TfmNumistaIssue::Execute(TComponent *AOwner, const String &ACoinText, int ATypeId, int AYear,
	const std::vector<TNumistaIssue> &AIssues, int ACurrentIssueId)
{
	std::unique_ptr<TfmNumistaIssue> form(new TfmNumistaIssue(AOwner));
	form->FIssues = AIssues;
	form->FYear   = AYear;
	form->FTypeId = ATypeId;
	form->lblCoin->Caption = ACoinText + L" \u2014 N# " + IntToStr(ATypeId);

	// Выпусков года монеты нет (или текущий выпуск другого года) — сразу все годы.
	bool hasYear = false, currentInYear = ACurrentIssueId == 0;
	for (const TNumistaIssue &issue : AIssues)
	{
		if (issue.EffectiveYear() == AYear)
		{
			hasYear = true;
			if (issue.Id == ACurrentIssueId)
				currentInYear = true;
		}
	}
	form->chkAllYears->OnClick = nullptr;
	form->chkAllYears->Checked = !hasYear || !currentInYear;
	form->chkAllYears->OnClick = form->chkAllYearsClick;
	form->ShowIssues(ACurrentIssueId);
	form->FormResize(nullptr);

	const int result = form->ShowModal();
	if (result == cTypeOnly)
		return 0;
	if (result != mrOk)
		return -1;
	const int index = form->Selected();
	return index >= 0 ? form->FIssues[index].Id : -1;
}
//---------------------------------------------------------------------------
void TfmNumistaIssue::ShowIssues(int ASelectId)
{
	FShown.clear();
	for (size_t i = 0; i < FIssues.size(); ++i)
		if (chkAllYears->Checked || FIssues[i].EffectiveYear() == FYear)
			FShown.push_back((int)i);
	dgIssues->RowCount = std::max<int>(2, (int)FShown.size() + 1);
	int row = 1;
	for (size_t i = 0; i < FShown.size(); ++i)
		if (FIssues[FShown[i]].Id == ASelectId)
			row = (int)i + 1;
	dgIssues->Row = row;
	btnLink->Enabled = !FShown.empty();
	dgIssues->Invalidate();
}
//---------------------------------------------------------------------------
int TfmNumistaIssue::Selected() const
{
	const int index = dgIssues->Row - 1;
	return index >= 0 && index < (int)FShown.size() ? FShown[index] : -1;
}
//---------------------------------------------------------------------------
void __fastcall TfmNumistaIssue::chkAllYearsClick(TObject *Sender)
{
	const int index = Selected();
	ShowIssues(index >= 0 ? FIssues[index].Id : 0);
}
//---------------------------------------------------------------------------
void __fastcall TfmNumistaIssue::FormResize(TObject *Sender)
{
	const int fixed = 80 + 70 + 100;
	dgIssues->ColWidths[icYear]    = 80;
	dgIssues->ColWidths[icMint]    = 70;
	dgIssues->ColWidths[icMintage] = 100;
	dgIssues->ColWidths[icComment] = std::max(150, dgIssues->ClientWidth - fixed - 4 * dgIssues->GridLineWidth);
}
//---------------------------------------------------------------------------
void __fastcall TfmNumistaIssue::dgIssuesDrawCell(TObject *Sender, int ACol, int ARow, TRect &Rect, TGridDrawState State)
{
	String text;
	UINT flags = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX;
	if (ARow == 0)
		text = Tr(cColumnKeys[ACol]);
	else if (ARow - 1 < (int)FShown.size())
	{
		const TNumistaIssue &issue = FIssues[FShown[ARow - 1]];
		switch (ACol)
		{
			case icYear:
				text = issue.Year != 0 ? IntToStr(issue.Year) : String(L"ND");
				if (issue.GregorianYear != 0 && issue.GregorianYear != issue.Year)
					text += L" (" + IntToStr(issue.GregorianYear) + L")";
				break;
			case icMint:
				text = issue.MintLetter;
				break;
			case icMintage:
			{
				const __int64 mintage = StrToInt64Def(issue.Mintage, -1);
				text = mintage >= 0 ? FormatFloat(L"#,##0", (double)mintage) : issue.Mintage;
				flags |= DT_RIGHT;
				break;
			}
			case icComment:
				text = issue.Comment;
				break;
		}
	}
	TCanvas *canvas = dgIssues->Canvas;
	RECT r = { Rect.Left + 4, Rect.Top, Rect.Right - 4, Rect.Bottom };
	canvas->Brush->Style = bsClear;
	DrawTextW(canvas->Handle, text.c_str(), -1, &r, flags);
}
//---------------------------------------------------------------------------
void __fastcall TfmNumistaIssue::dgIssuesDblClick(TObject *Sender)
{
	if (Selected() >= 0)
		ModalResult = mrOk;
}
//---------------------------------------------------------------------------
void __fastcall TfmNumistaIssue::btnOpenClick(TObject *Sender)
{
	ShellExecuteW(Handle, L"open", TNumistaMatcher::TypeUrl(FTypeId).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}
//---------------------------------------------------------------------------
