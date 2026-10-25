//---------------------------------------------------------------------------
// Выбор выпуска типа Numista вручную (getIssues): когда выпуски одного года
// различаются только комментарием Numista (двор, набор, пруф).
// Документация API: https://en.numista.com/api/doc/index.php
//---------------------------------------------------------------------------
#ifndef f_NumistaIssueH
#define f_NumistaIssueH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.Grids.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <vector>
#include "u_NumistaMatcher.h"
//---------------------------------------------------------------------------
class TfmNumistaIssue : public TForm
{
__published:	// IDE-managed Components
	TLabel *lblCoin;
	TCheckBox *chkAllYears;
	TDrawGrid *dgIssues;
	TPanel *pnlButtons;
	TButton *btnOpen;
	TButton *btnLink;
	TButton *btnTypeOnly;
	TButton *btnCancel;
	void __fastcall dgIssuesDrawCell(TObject *Sender, int ACol, int ARow, TRect &Rect, TGridDrawState State);
	void __fastcall dgIssuesDblClick(TObject *Sender);
	void __fastcall chkAllYearsClick(TObject *Sender);
	void __fastcall btnOpenClick(TObject *Sender);
	void __fastcall FormResize(TObject *Sender);
private:	// User declarations
	std::vector<TNumistaIssue> FIssues;
	std::vector<int>           FShown;	// индексы FIssues в порядке строк сетки
	int                        FYear;
	int                        FTypeId;
	void ShowIssues(int ASelectId);
	int  Selected() const;	// индекс в FIssues, -1 — нет
public:		// User declarations
	__fastcall TfmNumistaIssue(TComponent* Owner);
	// Результат: id выбранного выпуска; 0 — только тип (Type only); -1 — отмена.
	static int Execute(TComponent *AOwner, const String &ACoinText, int ATypeId, int AYear,
		const std::vector<TNumistaIssue> &AIssues, int ACurrentIssueId);
};
//---------------------------------------------------------------------------
#endif
