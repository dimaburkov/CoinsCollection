//---------------------------------------------------------------------------
// Numista → Connect / Settings: ключ API, проверка подключения (токен своей
// коллекции), счётчик запросов, кэш списков эмитентов и каталогов; вкладки
// «что загружать» / «что выгружать» / «запросы» (задача 33).
// Документация API: https://en.numista.com/api/doc/index.php
//---------------------------------------------------------------------------
#ifndef f_NumistaSettingsH
#define f_NumistaSettingsH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ComCtrls.hpp>
//---------------------------------------------------------------------------
class TfmNumistaSettings : public TForm
{
__published:	// IDE-managed Components
	TPageControl *pcSettings;
	TTabSheet *tsConnect;
	TTabSheet *tsLoad;
	TTabSheet *tsUpload;
	TTabSheet *tsRequests;
	TLabel *lblLoadIntro;
	TLabel *lblPhotosHint;
	TCheckBox *chkLoadDetails;
	TCheckBox *chkLoadPhotos;
	TCheckBox *chkLoadPrices;
	TLabel *lblCurrency;
	TComboBox *cbCurrency;
	TCheckBox *chkPricesWithDetails;
	TLabel *lblPriceDays;
	TEdit *edPriceDays;
	TLabel *lblPricesHint;
	TLabel *lblUploadIntro;
	TLabel *lblUploadAlways;
	TLabel *lblUploadHint;
	TCheckBox *chkUploadGrade;
	TCheckBox *chkUploadPrice;
	TCheckBox *chkUploadDate;
	TCheckBox *chkUploadComment;
	TLabel *lblUsed;
	TLabel *lblUsedNote;
	TLabel *lblQuota;
	TLabel *lblCounter;
	TLabel *lblCounterHint;
	TEdit *edQuota;
	TEdit *edCounter;
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
	void __fastcall btnOKClick(TObject *Sender);
	void __fastcall chkLoadPricesClick(TObject *Sender);
private:	// User declarations
	int  FCounterShown;	// счётчик, показанный в поле: изменён пользователем — записать
	void LoadOptions();	// настройки -> вкладки
	bool ApplyOptions();	// вкладки -> настройки; false — неверное число (фокус на поле)
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
