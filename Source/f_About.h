//---------------------------------------------------------------------------
// Окно «О программе» (задача 22): название, версия и дата сборки, контакт автора,
// источники данных (Numista, uCoin), папка данных и база. Форма без .dfm.
//---------------------------------------------------------------------------
#ifndef f_AboutH
#define f_AboutH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ExtCtrls.hpp>
//---------------------------------------------------------------------------
class TfmAbout : public TForm
{
public:
	__fastcall TfmAbout(TComponent *AOwner);

	// ADbFile — путь к базе, ASchema — версия её схемы.
	static void Execute(TComponent *AOwner, const String &ADbFile, int ASchema);

private:
	int FTop;	// следующая строка

	TLabel *AddLabel(const String &ACaption, int ALeft = 0, int AWidth = 0);
	TLabel *AddLink(const String &ACaption, const String &ATarget, int ALeft);
	void __fastcall LinkClick(TObject *Sender);
	void __fastcall CopyEmailClick(TObject *Sender);
};
//---------------------------------------------------------------------------
#endif
