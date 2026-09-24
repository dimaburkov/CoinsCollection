//---------------------------------------------------------------------------
// Главное окно приложения: меню и панель действий, список экземпляров
// коллекции с сортировкой, строка состояния.
//---------------------------------------------------------------------------
#ifndef f_MainH
#define f_MainH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <System.Actions.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.Menus.hpp>
#include <Vcl.ActnList.hpp>
#include <Vcl.ComCtrls.hpp>
#include <Vcl.ToolWin.hpp>
#include <map>
#include <memory>
#include <vector>
#include "u_Database.h"
#include "u_CoinRepository.h"
//---------------------------------------------------------------------------
class TfmMain : public TForm
{
__published:	// IDE-managed Components
	TActionList *ActionList;
	TAction *actImport;
	TAction *actBackup;
	TAction *actExit;
	TAction *actAdd;
	TAction *actEdit;
	TAction *actDelete;
	TAction *actNeedToReplace;
	TAction *actRefresh;
	TAction *actCountries;
	TAction *actPeriods;
	TAction *actCurrencies;
	TAction *actNumistaSettings;
	TAction *actNumistaUploadSelected;
	TAction *actNumistaUploadAll;
	TAction *actSettings;
	TAction *actAbout;
	TMainMenu *MainMenu;
	TMenuItem *miFile;
	TMenuItem *miImport;
	TMenuItem *miBackup;
	TMenuItem *miFileSep1;
	TMenuItem *miExit;
	TMenuItem *miCollection;
	TMenuItem *miAdd;
	TMenuItem *miEdit;
	TMenuItem *miDelete;
	TMenuItem *miCollectionSep1;
	TMenuItem *miNeedToReplace;
	TMenuItem *miCollectionSep2;
	TMenuItem *miRefresh;
	TMenuItem *miReference;
	TMenuItem *miCountries;
	TMenuItem *miPeriods;
	TMenuItem *miCurrencies;
	TMenuItem *miNumista;
	TMenuItem *miNumistaSettings;
	TMenuItem *miNumistaSep1;
	TMenuItem *miNumistaUploadSelected;
	TMenuItem *miNumistaUploadAll;
	TMenuItem *miSettings;
	TMenuItem *miLanguage;
	TMenuItem *miSettingsDialog;
	TMenuItem *miHelp;
	TMenuItem *miAbout;
	TToolBar *ToolBar;
	TToolButton *tbAdd;
	TToolButton *tbEdit;
	TToolButton *tbDelete;
	TToolButton *tbSep1;
	TToolButton *tbImport;
	TListView *lvItems;
	TStatusBar *StatusBar;
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall FormDestroy(TObject *Sender);
	void __fastcall actExitExecute(TObject *Sender);
	void __fastcall actRefreshExecute(TObject *Sender);
	void __fastcall actAboutExecute(TObject *Sender);
	void __fastcall lvItemsData(TObject *Sender, TListItem *Item);
	void __fastcall lvItemsColumnClick(TObject *Sender, TListColumn *Column);
	void __fastcall lvItemsDblClick(TObject *Sender);
	void __fastcall lvItemsKeyDown(TObject *Sender, WORD &Key, TShiftState Shift);
private:	// User declarations
	// Колонки списка (порядок = индекс колонки lvItems).
	enum TItemColumn { icPeriod, icCurrency, icDenomination, icYear, icCondition, icNumber, icValue, icCount };

	std::unique_ptr<TDatabase>       FDatabase;
	std::unique_ptr<TCoinRepository> FRepository;
	std::vector<TCoinRecord>         FItems;           // строки списка в порядке показа
	std::map<String, int>            FConditionOrder;  // код состояния -> sort_order
	int                              FSortColumn;
	bool                             FSortAscending;
	int                              FLanguageToken;

	void BuildLanguageMenu();
	void __fastcall LanguageClick(TObject *Sender);
	void ApplyTexts();

	void LoadItems();
	void SortItems();
	int  CompareItems(const TCoinRecord &A, const TCoinRecord &B, int AColumn) const;
	String CellText(const TCoinRecord &ARecord, int AColumn) const;
	void UpdateSortArrows();
	void UpdateStatus();

	std::vector<int> SelectedIds() const;
	void SelectIds(const std::vector<int> &AIds, int AFocusedId);
	int  FocusedId() const;
public:		// User declarations
	__fastcall TfmMain(TComponent* Owner);
	__fastcall ~TfmMain();
};
//---------------------------------------------------------------------------
extern PACKAGE TfmMain *fmMain;
//---------------------------------------------------------------------------
#endif
