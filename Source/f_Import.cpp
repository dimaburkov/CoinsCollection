//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "f_Import.h"
#include "u_Translator.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TfmImport *fmImport;
//---------------------------------------------------------------------------
__fastcall TfmImport::TfmImport(TComponent* Owner)
	: TForm(Owner)
{
	Translator().TranslateForm(this);
}
//---------------------------------------------------------------------------
