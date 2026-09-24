//---------------------------------------------------------------------------
// Главное окно приложения: список коллекции. Пока — открытие БД, self-test
// и выбор языка интерфейса.
//---------------------------------------------------------------------------
#ifndef f_MainH
#define f_MainH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.Menus.hpp>
#include <memory>
#include "u_Database.h"
#include "u_CoinRepository.h"
//---------------------------------------------------------------------------
class TfmMain : public TForm
{
__published:	// IDE-managed Components
	TMainMenu *MainMenu;
	TMenuItem *miSettings;
	TMenuItem *miLanguage;
	TButton *btnSelfTest;
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall FormDestroy(TObject *Sender);
	void __fastcall btnSelfTestClick(TObject *Sender);
private:	// User declarations
	std::unique_ptr<TDatabase>       FDatabase;
	std::unique_ptr<TCoinRepository> FRepository;

	void BuildLanguageMenu();
	void __fastcall LanguageClick(TObject *Sender);

	String SchemaReport();
	String CrudReport();
public:		// User declarations
	__fastcall TfmMain(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TfmMain *fmMain;
//---------------------------------------------------------------------------
#endif
