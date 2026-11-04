//---------------------------------------------------------------------------
// Продажи на аукционах для монеты (задача 38): getSalesRecords — список продаж выпуска
// или всего типа, отбор по состоянию монеты, сводка цен, ссылка на лот.
// Документация API: https://en.numista.com/api/doc/index.php
//---------------------------------------------------------------------------
#ifndef f_NumistaSalesH
#define f_NumistaSalesH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.Grids.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <vector>
#include "u_CoinTypes.h"
#include "u_NumistaMatcher.h"
//---------------------------------------------------------------------------
class TfmNumistaSales : public TForm
{
__published:	// IDE-managed Components
	TLabel *lblCoin;
	TPanel *pnlFilter;
	TLabel *lblSource;
	TRadioButton *rbIssue;
	TRadioButton *rbType;
	TCheckBox *chkMyGrade;
	TLabel *lblSummary;
	TDrawGrid *dgSales;
	TPanel *pnlButtons;
	TLabel *lblLoaded;
	TButton *btnOpenLot;
	TButton *btnRefresh;
	TButton *btnClose;
	void __fastcall FormShow(TObject *Sender);
	void __fastcall FormResize(TObject *Sender);
	void __fastcall FilterClick(TObject *Sender);
	void __fastcall dgSalesDrawCell(TObject *Sender, int ACol, int ARow, TRect &Rect, TGridDrawState State);
	void __fastcall dgSalesDblClick(TObject *Sender);
	void __fastcall dgSalesFixedCellClick(TObject *Sender, int ACol, int ARow);
	void __fastcall btnOpenLotClick(TObject *Sender);
	void __fastcall btnRefreshClick(TObject *Sender);
	void __fastcall lblSourceClick(TObject *Sender);
private:	// User declarations
	TCoinRecord               FCoin;
	TNumistaMatcher          *FMatcher;	// не владеет
	String                    FCurrency;
	TNumistaSales             FSales;	// последняя загрузка (выпуск или тип)
	std::vector<TNumistaSale> FShown;	// после отбора, в порядке показа
	int                       FSortColumn;
	bool                      FSortAscending;
	void Load(bool ARefresh);
	void ApplyFilter();
	int  Selected() const;	// индекс в FShown, -1 — нет
	String CellText(const TNumistaSale &ASale, int AColumn) const;
public:		// User declarations
	__fastcall TfmNumistaSales(TComponent* Owner);
	// Окно продаж монеты ACoin (связана с типом Numista); ACurrency — валюта пересчёта цен.
	static void Execute(TComponent *AOwner, const TCoinRecord &ACoin, const String &ACoinText,
		TNumistaMatcher &AMatcher, const String &ACurrency);
};
//---------------------------------------------------------------------------
#endif
