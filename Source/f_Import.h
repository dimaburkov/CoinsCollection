//---------------------------------------------------------------------------
// Мастер импорта коллекции из .xlsx: выбор файла -> предпросмотр -> запуск ->
// отчёт. Пустая заготовка.
//---------------------------------------------------------------------------
#ifndef f_ImportH
#define f_ImportH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
//---------------------------------------------------------------------------
class TfmImport : public TForm
{
__published:	// IDE-managed Components
private:	// User declarations
public:		// User declarations
	__fastcall TfmImport(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TfmImport *fmImport;
//---------------------------------------------------------------------------
#endif
