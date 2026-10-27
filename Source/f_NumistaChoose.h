//---------------------------------------------------------------------------
// Выбор типа Numista из результатов поиска (searchTypes), когда найдено
// несколько вариантов. Документация API: https://en.numista.com/api/doc/index.php
//---------------------------------------------------------------------------
#ifndef f_NumistaChooseH
#define f_NumistaChooseH
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
class TfmNumistaChoose : public TForm
{
__published:	// IDE-managed Components
	TLabel *lblCoin;
	TPanel *pnlQuery;
	TLabel *lblQuery;
	TEdit *edQuery;
	TButton *btnSearch;
	TDrawGrid *dgTypes;
	TPanel *pnlButtons;
	TButton *btnLink;
	TButton *btnOpen;
	TButton *btnCancel;
	void __fastcall dgTypesDrawCell(TObject *Sender, int ACol, int ARow, TRect &Rect, TGridDrawState State);
	void __fastcall dgTypesDblClick(TObject *Sender);
	void __fastcall btnOpenClick(TObject *Sender);
	void __fastcall FormResize(TObject *Sender);
	void __fastcall btnSearchClick(TObject *Sender);
	void __fastcall edQueryEnter(TObject *Sender);
	void __fastcall edQueryExit(TObject *Sender);
	void __fastcall dgTypesSelectCell(TObject *Sender, int ACol, int ARow, bool &CanSelect);
private:	// User declarations
	std::vector<TNumistaType> FTypes;
	TNumistaMatcher          *FMatcher;	// не владеет; nullptr — без повторного поиска
	int  Selected() const;	// индекс в FTypes, -1 — нет
	void ShowTypes(const std::vector<TNumistaType> &ATypes);
public:		// User declarations
	__fastcall TfmNumistaChoose(TComponent* Owner);
	// Возвращает N# выбранного типа, 0 — отмена. AQuery — фраза поиска: на входе
	// показывается, пользователь может изменить её и искать снова (задача 15b, через
	// AMatcher); на выходе — фраза последнего поиска.
	static int Execute(TComponent *AOwner, const String &ACoinText, const std::vector<TNumistaType> &ATypes,
		TNumistaMatcher &AMatcher, String &AQuery);
};
//---------------------------------------------------------------------------
#endif
