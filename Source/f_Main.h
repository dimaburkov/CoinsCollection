//---------------------------------------------------------------------------
// Главное окно приложения: список коллекции. Пока — открытие БД и self-test.
//---------------------------------------------------------------------------
#ifndef f_MainH
#define f_MainH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <memory>
#include "u_AppConfig.h"
#include "u_Database.h"
#include "u_CoinRepository.h"
//---------------------------------------------------------------------------
class TfmMain : public TForm
{
__published:	// IDE-managed Components
	TButton *btnSelfTest;
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall FormDestroy(TObject *Sender);
	void __fastcall btnSelfTestClick(TObject *Sender);
private:	// User declarations
	TAppConfig                       FConfig;
	std::unique_ptr<TDatabase>       FDatabase;
	std::unique_ptr<TCoinRepository> FRepository;

	String SchemaReport();
	String CrudReport();
public:		// User declarations
	__fastcall TfmMain(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TfmMain *fmMain;
//---------------------------------------------------------------------------
#endif
