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
	TDrawGrid *dgTypes;
	TPanel *pnlButtons;
	TButton *btnLink;
	TButton *btnOpen;
	TButton *btnCancel;
	void __fastcall dgTypesDrawCell(TObject *Sender, int ACol, int ARow, TRect &Rect, TGridDrawState State);
	void __fastcall dgTypesDblClick(TObject *Sender);
	void __fastcall btnOpenClick(TObject *Sender);
	void __fastcall FormResize(TObject *Sender);
private:	// User declarations
	std::vector<TNumistaType> FTypes;
	int Selected() const;	// индекс в FTypes, -1 — нет
public:		// User declarations
	__fastcall TfmNumistaChoose(TComponent* Owner);
	// Возвращает N# выбранного типа, 0 — отмена.
	static int Execute(TComponent *AOwner, const String &ACoinText, const std::vector<TNumistaType> &ATypes);
};
//---------------------------------------------------------------------------
#endif
