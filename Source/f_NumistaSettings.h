//---------------------------------------------------------------------------
// Numista → Connect / Settings: ключ API, проверка подключения (токен своей
// коллекции), счётчик запросов, кэш списков эмитентов и каталогов.
// Документация API: https://en.numista.com/api/doc/index.php
//---------------------------------------------------------------------------
#ifndef f_NumistaSettingsH
#define f_NumistaSettingsH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
//---------------------------------------------------------------------------
class TfmNumistaSettings : public TForm
{
__published:	// IDE-managed Components
	TLabel *lblApiKey;
	TLabel *lblStatus;
	TLabel *lblRequests;
	TLabel *lblLists;
	TLabel *lblDocs;
	TEdit *edApiKey;
	TCheckBox *chkShowKey;
	TButton *btnCheck;
	TButton *btnRefreshLists;
	TButton *btnOK;
	TButton *btnCancel;
	void __fastcall chkShowKeyClick(TObject *Sender);
	void __fastcall btnCheckClick(TObject *Sender);
	void __fastcall btnRefreshListsClick(TObject *Sender);
	void __fastcall lblDocsClick(TObject *Sender);
private:	// User declarations
	void ApplyKey();	// ключ из поля -> настройки; новый ключ — старый токен сбрасывается
	void UpdateInfo();
	void ShowError(const String &AText);
public:		// User declarations
	__fastcall TfmNumistaSettings(TComponent* Owner);

	static void Execute(TComponent *AOwner);
};
//---------------------------------------------------------------------------
extern PACKAGE TfmNumistaSettings *fmNumistaSettings;
//---------------------------------------------------------------------------
#endif
