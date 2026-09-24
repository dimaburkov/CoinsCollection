//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "f_CoinEdit.h"
#include "u_Translator.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TfmCoinEdit *fmCoinEdit;
//---------------------------------------------------------------------------
__fastcall TfmCoinEdit::TfmCoinEdit(TComponent* Owner)
	: TForm(Owner)
{
	Translator().TranslateForm(this);
}
//---------------------------------------------------------------------------
