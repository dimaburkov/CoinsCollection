//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <algorithm>
#include <memory>
#include <shellapi.h>
#include <System.DateUtils.hpp>
#include "f_NumistaSales.h"
#include "u_AppConfig.h"
#include "u_NumistaClient.h"
#include "u_Translator.h"
#include "u_Help.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
//---------------------------------------------------------------------------
namespace
{
	enum { csDate, csHouse, csTitle, csGrade, csPrice, csConverted, csPremium, csCount };
	const wchar_t *const cColumnKeys[csCount] =
	{
		L"Sales.Column.Date", L"Sales.Column.House", L"Sales.Column.Title", L"Sales.Column.Grade",
		L"Sales.Column.Price", L"Sales.Column.Converted", L"Sales.Column.Premium"
	};
	const int cWidths[csCount] = { 80, 170, 0, 70, 100, 100, 70 };	// 0 — остаток ширины

	template <typename T>
	int CompareValues(T A, T B)
	{
		return A < B ? -1 : (B < A ? 1 : 0);
	}
	//-----------------------------------------------------------------------
	String Money(double AValue)
	{
		return FormatFloat(L"#,##0.##", AValue);
	}
	//-----------------------------------------------------------------------
	// Дата продажи как в списке: dd.mm.yyyy.
	String SaleDate(const String &AIso)
	{
		return AIso.Length() >= 10 ? AIso.SubString(9, 2) + L"." + AIso.SubString(6, 2) + L"." + AIso.SubString(1, 4) : AIso;
	}
}
//---------------------------------------------------------------------------
__fastcall TfmNumistaSales::TfmNumistaSales(TComponent* Owner)
	: TForm(Owner),
	  FMatcher(nullptr),
	  FSortColumn(csDate),
	  FSortAscending(false)
{
	Translator().TranslateForm(this);
	InstallHelpKey(this, L"numista-catalog");
	AppConfig().RestoreFormSize(this, L"NumistaSales");	// задача 44
}
//---------------------------------------------------------------------------
void TfmNumistaSales::Execute(TComponent *AOwner, const TCoinRecord &ACoin, const String &ACoinText,
	TNumistaMatcher &AMatcher, const String &ACurrency)
{
	std::unique_ptr<TfmNumistaSales> form(new TfmNumistaSales(AOwner));
	TfmNumistaSales *f = form.get();
	f->FCoin     = ACoin;
	f->FMatcher  = &AMatcher;
	f->FCurrency = ACurrency;
	f->lblCoin->Caption = ACoinText + L" — N# " + IntToStr(ACoin.NumistaTypeId);

	// Без выпуска — только продажи типа; состояние вне шкалы Numista (P, FR, PRF) — без отбора.
	f->rbIssue->Enabled = ACoin.NumistaIssueId != 0;
	f->rbIssue->OnClick = nullptr;
	f->rbType->OnClick  = nullptr;
	f->rbIssue->Checked = ACoin.NumistaIssueId != 0;
	f->rbType->Checked  = ACoin.NumistaIssueId == 0;
	f->rbIssue->OnClick = f->FilterClick;
	f->rbType->OnClick  = f->FilterClick;
	const int grade = TNumistaPrices::GradeIndex(ACoin.Condition);
	f->chkMyGrade->Enabled = grade >= 0;
	f->chkMyGrade->Caption = grade >= 0
		? Format(Tr(L"Sales.MyGrade"), ARRAYOFCONST((ACoin.Condition.UpperCase()))) : Tr(L"Sales.MyGradeNone");
	f->ShowModal();
	AppConfig().StoreFormSize(f, L"NumistaSales");
}
//---------------------------------------------------------------------------
void __fastcall TfmNumistaSales::FormShow(TObject *Sender)
{
	FormResize(nullptr);
	Update();
	Load(false);
}
//---------------------------------------------------------------------------
// Продажи выпуска или типа: из кэша (моложе 7 дней) или 1 запрос; квоты нет — сообщение.
void TfmNumistaSales::Load(bool ARefresh)
{
	const int issueId = rbIssue->Checked ? FCoin.NumistaIssueId : 0;
	const String path = TNumistaMatcher::SalesPath(FCoin.NumistaTypeId, issueId, FCurrency);
	const TDateTime cached = TNumistaClient::CachedAt(path);
	const bool needRequest = ARefresh || (double)cached == 0.0 || (double)Now() - (double)cached > 7.0;
	FSales = TNumistaSales();
	lblSummary->Font->Color = clWindowText;
	if (needRequest && AppConfig().NumistaMonthlyQuota - TNumistaClient::RequestsThisMonth() <= 0)
	{
		lblSummary->Font->Color = clRed;
		lblSummary->Caption = Tr(L"Numista.Error.Quota");
		ApplyFilter();
		return;
	}
	const TCursor saved = Screen->Cursor;
	Screen->Cursor = crHourGlass;
	try
	{
		FSales = FMatcher->LoadSales(FCoin.NumistaTypeId, issueId, FCurrency, ARefresh);
		Screen->Cursor = saved;
		ApplyFilter();
	}
	catch (Exception &e)
	{
		Screen->Cursor = saved;
		ApplyFilter();
		lblSummary->Font->Color = clRed;
		lblSummary->Caption = e.Message;
	}
}
//---------------------------------------------------------------------------
// Отбор по состоянию, сортировка, сводка по показанным продажам.
void TfmNumistaSales::ApplyFilter()
{
	const int grade = TNumistaPrices::GradeIndex(FCoin.Condition);
	const String code = chkMyGrade->Checked && grade >= 0 ? TNumistaPrices::GradeCode(grade) : String();
	FShown.clear();
	for (const TNumistaSale &s : FSales.Items)
		if (code.IsEmpty() || s.Grade == code)
			FShown.push_back(s);

	const int column = FSortColumn;
	const bool ascending = FSortAscending;
	std::stable_sort(FShown.begin(), FShown.end(), [column, ascending](const TNumistaSale &A, const TNumistaSale &B)
	{
		int r = 0;
		switch (column)
		{
			case csDate:      r = CompareValues(A.Date, B.Date); break;
			case csHouse:     r = CompareText(A.House, B.House); break;
			case csTitle:     r = CompareText(A.Title, B.Title); break;
			case csGrade:     r = CompareValues(TNumistaPrices::GradeIndex(A.Grade), TNumistaPrices::GradeIndex(B.Grade)); break;
			case csPrice:     r = CompareValues(A.Amount, B.Amount); break;
			case csConverted: r = CompareValues(A.Converted, B.Converted); break;
			case csPremium:   r = CompareValues(A.Premium, B.Premium); break;
		}
		if (!ascending)
			r = -r;
		if (r == 0)
			r = -CompareValues(A.Date, B.Date);	// равные — новые сверху
		return r < 0;
	});

	// Сводка: всего, за 3 года, цены в валюте пересчёта (min / max / медиана).
	String summary;
	if (FShown.empty())
		summary = Tr(L"Sales.None");
	else
	{
		const String since = FormatDateTime(L"yyyy-mm-dd", IncYear(Date(), -3));
		int recent = 0;
		std::vector<double> prices;
		for (const TNumistaSale &s : FShown)
		{
			if (s.Date >= since)
				++recent;
			if (s.Converted > 0.0)
				prices.push_back(s.Converted);
		}
		summary = Format(Tr(L"Sales.Summary"), ARRAYOFCONST(((int)FShown.size(), recent)));
		if (!prices.empty())
		{
			std::sort(prices.begin(), prices.end());
			const size_t n = prices.size();
			const double median = n % 2 ? prices[n / 2] : (prices[n / 2 - 1] + prices[n / 2]) / 2.0;
			summary += L" " + Format(Tr(L"Sales.Prices"), ARRAYOFCONST((FCurrency, Money(prices.front()),
				Money(prices.back()), Money(median))));
		}
	}
	if (FSales.Count > (int)FSales.Items.size())
		summary += L" " + Format(Tr(L"Sales.More"), ARRAYOFCONST((FSales.Count, (int)FSales.Items.size())));
	lblSummary->Caption = summary;

	lblLoaded->Caption = (double)FSales.LoadedAt == 0.0 ? String()
		: Format(Tr(FSales.FromCache ? L"Sales.LoadedCache" : L"Sales.Loaded"),
			ARRAYOFCONST((FormatDateTime(L"dd.mm.yyyy hh:nn", FSales.LoadedAt), TNumistaClient::RequestsThisMonth(),
				AppConfig().NumistaMonthlyQuota)));

	dgSales->RowCount = std::max<int>(2, (int)FShown.size() + 1);
	dgSales->Row = 1;
	btnOpenLot->Enabled = !FShown.empty();
	dgSales->Invalidate();
}
//---------------------------------------------------------------------------
void __fastcall TfmNumistaSales::FilterClick(TObject *Sender)
{
	if (!Visible)
		return;
	if (Sender == chkMyGrade)
		ApplyFilter();
	else
		Load(false);	// выпуск / тип — другой запрос
}
//---------------------------------------------------------------------------
int TfmNumistaSales::Selected() const
{
	const int index = dgSales->Row - 1;
	return index >= 0 && index < (int)FShown.size() ? index : -1;
}
//---------------------------------------------------------------------------
void __fastcall TfmNumistaSales::FormResize(TObject *Sender)
{
	int fixed = 0;
	for (int c = 0; c < csCount; ++c)
		fixed += cWidths[c] + dgSales->GridLineWidth;
	for (int c = 0; c < csCount; ++c)
		dgSales->ColWidths[c] = cWidths[c] != 0 ? cWidths[c] : std::max(150, dgSales->ClientWidth - fixed);
}
//---------------------------------------------------------------------------
String TfmNumistaSales::CellText(const TNumistaSale &ASale, int AColumn) const
{
	switch (AColumn)
	{
		case csDate:      return SaleDate(ASale.Date);
		case csHouse:     return ASale.House;
		case csTitle:     return ASale.Title + (ASale.LotNumber.IsEmpty() ? String() : L", " + Tr(L"Sales.Lot") + L" " + ASale.LotNumber);
		case csGrade:     return ASale.Grade.UpperCase() + (ASale.GradeDetails.IsEmpty() ? String() : L" " + ASale.GradeDetails);
		case csPrice:     return ASale.Amount > 0.0 ? Money(ASale.Amount) + L" " + ASale.Currency : String();
		case csConverted: return ASale.Converted > 0.0 ? Money(ASale.Converted) : String(L"—");
		case csPremium:   return Tr(ASale.Premium == 1 ? L"Sales.Yes" : (ASale.Premium == 0 ? L"Sales.No" : L"Sales.Unknown"));
	}
	return String();
}
//---------------------------------------------------------------------------
void __fastcall TfmNumistaSales::dgSalesDrawCell(TObject *Sender, int ACol, int ARow, TRect &Rect, TGridDrawState State)
{
	String text;
	if (ARow == 0)
	{
		text = Tr(cColumnKeys[ACol]);
		if (ACol == csConverted)
			text = Format(text, ARRAYOFCONST((FCurrency)));
		if (ACol == FSortColumn)
			text += FSortAscending ? L" ▲" : L" ▼";
	}
	else if (ARow - 1 < (int)FShown.size())
		text = CellText(FShown[ARow - 1], ACol);
	TCanvas *canvas = dgSales->Canvas;
	RECT r = { Rect.Left + 4, Rect.Top, Rect.Right - 4, Rect.Bottom };
	UINT flags = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX;
	if (ARow > 0 && (ACol == csPrice || ACol == csConverted))
		flags |= DT_RIGHT;
	canvas->Brush->Style = bsClear;
	DrawTextW(canvas->Handle, text.c_str(), -1, &r, flags);
}
//---------------------------------------------------------------------------
// Клик по заголовку: та же колонка — обратный порядок; другая — по возрастанию (дата и цены — по убыванию).
void __fastcall TfmNumistaSales::dgSalesFixedCellClick(TObject *Sender, int ACol, int ARow)
{
	if (ACol == FSortColumn)
		FSortAscending = !FSortAscending;
	else
	{
		FSortColumn = ACol;
		FSortAscending = !(ACol == csDate || ACol == csPrice || ACol == csConverted);
	}
	ApplyFilter();
}
//---------------------------------------------------------------------------
void __fastcall TfmNumistaSales::dgSalesDblClick(TObject *Sender)
{
	btnOpenLotClick(Sender);
}
//---------------------------------------------------------------------------
// Ссылка на лот — только https:// / http:// (адрес пришёл из ответа API).
void __fastcall TfmNumistaSales::btnOpenLotClick(TObject *Sender)
{
	const int index = Selected();
	if (index < 0)
		return;
	const String url = FShown[index].LotUrl.Trim();
	if (url.SubString(1, 8).LowerCase() == L"https://" || url.SubString(1, 7).LowerCase() == L"http://")
		ShellExecuteW(Handle, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
	else
		Application->MessageBox(Tr(L"Sales.NoLot").c_str(), Caption.c_str(), MB_OK | MB_ICONINFORMATION);
}
//---------------------------------------------------------------------------
void __fastcall TfmNumistaSales::btnRefreshClick(TObject *Sender)
{
	Load(true);
}
//---------------------------------------------------------------------------
void __fastcall TfmNumistaSales::lblSourceClick(TObject *Sender)
{
	ShellExecuteW(Handle, L"open", TNumistaMatcher::TypeUrl(FCoin.NumistaTypeId).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}
//---------------------------------------------------------------------------
