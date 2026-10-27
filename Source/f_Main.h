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
#include <Vcl.Grids.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include "u_Database.h"
#include "u_CoinRepository.h"
#include "u_PhotoRepository.h"
struct TPhotoEdits;
class TNumistaClient;
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
	TAction *actNumistaFind;
	TAction *actNumistaIssue;
	TAction *actNumistaCheck;
	TAction *actCheckAll;
	TAction *actUncheckAll;
	TAction *actUncheckEverywhere;
	TAction *actShowChecked;
	TAction *actNumistaUpload;
	TAction *actNumistaRarity;
	TAction *actNumistaCatalog;
	TAction *actHelpContents;
	TMenuItem *miHelpContents;
	TMenuItem *miHelpSep1;
	TAction *actNumistaUploadOne;
	TAction *actNumistaRarityOne;
	TAction *actNumistaRefreshOne;
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
	TMenuItem *miNumistaFind;
	TMenuItem *miNumistaIssue;
	TMenuItem *miNumistaCheck;
	TMenuItem *miNumistaSep2;
	TMenuItem *miNumistaUpload;
	TMenuItem *miNumistaRarity;
	TMenuItem *miNumistaCatalog;
	TMenuItem *miNumistaCoinTitle;
	TMenuItem *miNumistaUploadOne;
	TMenuItem *miNumistaRarityOne;
	TMenuItem *miNumistaRefreshOne;
	TMenuItem *miNumistaCheckedTitle;
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
	TToolButton *tbSep2;
	TEdit *edSearch;
	TToolButton *tbClearSearch;
	TTreeView *tvNav;
	TSplitter *Splitter;
	TPanel *pnlRight;
	TDrawGrid *dgItems;
	TSplitter *splDetails;
	TPanel *pnlDetails;
	TPanel *pnlPhotos;
	TPanel *pnlObverse;
	TImage *imgObverse;
	TLabel *lblObverseNone;
	TLabel *lblObverse;
	TPanel *pnlReverse;
	TImage *imgReverse;
	TLabel *lblReverseNone;
	TLabel *lblReverse;
	TScrollBox *sbInfo;
	TLabel *lblDetailsTitle;
	TStatusBar *StatusBar;
	TPopupMenu *pmItems;
	TMenuItem *miPopNumistaCheck;
	TMenuItem *miPopNumistaFind;
	TMenuItem *miPopNumistaIssue;
	TMenuItem *miPopNumistaUpload;
	TMenuItem *miPopNumistaRarity;
	TMenuItem *miPopNumistaCatalog;
	TMenuItem *miPopCoinTitle;
	TMenuItem *miPopNumistaUploadOne;
	TMenuItem *miPopNumistaRarityOne;
	TMenuItem *miPopNumistaRefreshOne;
	TMenuItem *miPopCheckedTitle;
	TPopupMenu *pmHeader;
	TMenuItem *miHdrCheckAll;
	TMenuItem *miHdrUncheckAll;
	TMenuItem *miHdrUncheckEverywhere;
	TMenuItem *miHdrSep1;
	TMenuItem *miHdrShowChecked;
	TMenuItem *miPopSep1;
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall FormDestroy(TObject *Sender);
	void __fastcall actImportExecute(TObject *Sender);
	void __fastcall actAddExecute(TObject *Sender);
	void __fastcall actEditExecute(TObject *Sender);
	void __fastcall actEditUpdate(TObject *Sender);
	void __fastcall actDeleteExecute(TObject *Sender);
	void __fastcall actDeleteUpdate(TObject *Sender);
	void __fastcall actExitExecute(TObject *Sender);
	void __fastcall actRefreshExecute(TObject *Sender);
	void __fastcall actAboutExecute(TObject *Sender);
	void __fastcall dgItemsDrawCell(TObject *Sender, int ACol, int ARow, TRect &Rect, TGridDrawState State);
	void __fastcall dgItemsFixedCellClick(TObject *Sender, int ACol, int ARow);
	void __fastcall dgItemsDblClick(TObject *Sender);
	void __fastcall dgItemsMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift, int X, int Y);
	void __fastcall dgItemsKeyDown(TObject *Sender, WORD &Key, TShiftState Shift);
	void __fastcall dgItemsSelectCell(TObject *Sender, int ACol, int ARow, bool &CanSelect);
	void __fastcall sbInfoResize(TObject *Sender);
	void __fastcall tvNavChange(TObject *Sender, TTreeNode *Node);
	void __fastcall edSearchChange(TObject *Sender);
	void __fastcall edSearchKeyDown(TObject *Sender, WORD &Key, TShiftState Shift);
	void __fastcall tbClearSearchClick(TObject *Sender);
	void __fastcall actNeedToReplaceExecute(TObject *Sender);
	void __fastcall actReferenceExecute(TObject *Sender);
	void __fastcall actNumistaSettingsExecute(TObject *Sender);
	void __fastcall actNumistaFindExecute(TObject *Sender);
	void __fastcall actNumistaIssueExecute(TObject *Sender);
	void __fastcall actNumistaCheckExecute(TObject *Sender);
	void __fastcall actNumistaUploadExecute(TObject *Sender);
	void __fastcall actNumistaUploadOneExecute(TObject *Sender);
	void __fastcall actNumistaRarityOneExecute(TObject *Sender);
	void __fastcall actNumistaRefreshOneExecute(TObject *Sender);
	void __fastcall actNumistaUpdate(TObject *Sender);
	void __fastcall pmItemsPopup(TObject *Sender);
	void __fastcall miNumistaClick(TObject *Sender);
	void __fastcall dgItemsContextPopup(TObject *Sender, const TPoint &MousePos, bool &Handled);
	void __fastcall actNumistaRarityExecute(TObject *Sender);
	void __fastcall actNumistaCatalogExecute(TObject *Sender);
	void __fastcall actHelpContentsExecute(TObject *Sender);
	void __fastcall actCheckAllExecute(TObject *Sender);
	void __fastcall actCheckAllUpdate(TObject *Sender);
	void __fastcall actUncheckEverywhereExecute(TObject *Sender);
	void __fastcall actShowCheckedExecute(TObject *Sender);
private:	// User declarations
	// Колонки списка (логические; видимые и их порядок — FColumns).
	// icCheck — флажок отметки, icNumista — N# (задача 12a), icUploaded — выгружен в коллекцию Numista (задача 13),
	// icRarity — индекс редкости (задача 13a), icMintage — тираж выпуска (задача 13c),
	// icComposition — металл типа (задача 13d).
	enum TItemColumn { icCheck, icCountry, icPeriod, icCurrency, icDenomination, icNumista, icUploaded, icRarity, icMintage,
		icComposition, icYear, icCondition, icValue, icCount };

	// Строка панели подробностей: подпись и значение (создаются в коде, см. CreateDetailRows).
	struct TDetailRow
	{
		const wchar_t *Key;     // ключ подписи для Tr()
		TLabel        *Caption;
		TLabel        *Value;
	};
	// Две колонки: drPeriod … drIssueComment — всегда («—», если пусто); со drWeight — характеристики
	// типа Numista (задача 13d), пустые не показываются. Металл — только в таблице.
	enum TDetailRowIndex { drPeriod, drCurrency, drRuler, drSubject, drNumber, drNumista, drNumistaItem, drIssueComment,
		drWeight, drSize, drShape, drTechnique, drSeries, drCommemorated, drTypeYears, drMints, drCategory,
		drDemonetized, drCount };

	// Узел дерева навигации: что он отбирает.
	// nkChecked — отмеченные экземпляры из любых стран (задача 13e).
	enum TNavKind { nkAll, nkContinent, nkNoContinent, nkExtinct, nkCountry, nkChecked };
	struct TNavNode
	{
		int    Kind;   // TNavKind
		int    Id;     // id континента / страны
		String Key;    // для сохранения выбора и раскрытия при перестройке
	};

	std::unique_ptr<TDatabase>       FDatabase;
	std::unique_ptr<TCoinRepository> FRepository;
	std::unique_ptr<TPhotoRepository> FPhotos;
	std::vector<TCoinRecord>         FAll;             // вся коллекция из БД
	std::vector<String>              FSearchTexts;     // текст для поиска по FAll[i] (в верхнем регистре)
	std::vector<TCoinRecord>         FItems;           // показанные (после фильтров), в порядке показа (строка сетки = индекс + 1)
	std::map<String, TCountry>       FCountryByName;   // имя страны (AnsiUpperCase) -> страна
	std::vector<TNavNode>            FNav;             // узлы дерева; TTreeNode::Data = индекс
	bool                             FBuildingTree;    // перестройка дерева — не фильтровать на каждом OnChange
	TTreeNode                       *FCheckedNode;     // узел «Отмеченные» (счётчик меняется без перестройки дерева)
	std::vector<int>                 FColumns;         // видимые колонки сетки (TItemColumn)
	int                              FColWidths[icCount];
	std::vector<TDetailRow>          FDetailRows;      // порядок = TDetailRowIndex
	std::map<String, int>            FConditionOrder;  // код состояния -> sort_order
	int                              FSortColumn;
	bool                             FSortAscending;
	int                              FLanguageToken;
	int                              FMouseDownRow;    // строка сетки последнего нажатия мыши (для двойного клика)
	int                              FDetailsTypeId;   // N# монеты в панели подробностей (0 — нет)
	String                           FDetailPhotos[2]; // фото в панели (полный путь; "" — нет), индекс — TPhotoSide
	String                           FDetailCopyrights[2]; // автор фото Numista ("" — свой снимок)
	String                           FDetailPhotoTitle; // заголовок окна просмотра
	std::set<int>                    FChecked;         // отмеченные экземпляры (coin_items.id), только в памяти

	void BuildLanguageMenu();
	void __fastcall LanguageClick(TObject *Sender);
	void ApplyTexts();

	void LoadItems();
	void BuildTree();
	TTreeNode *AddNavNode(TTreeNode *AParent, int AKind, int AId, const String &AName, int ACount);
	const TNavNode *CurrentNode() const;
	bool MatchNode(const TCoinRecord &ARecord, const TNavNode &ANode) const;
	void ApplyFilter();
	void UpdateColumns();
	int  Column(int AGridCol) const;	// колонка сетки -> TItemColumn, -1 — нет
	bool Confirm(const String &AText, const String &ACaption);
	void ApplyPhotoEdits(const TPhotoEdits &AEdits, const TCoinRecord &ARecord);
	bool SaveRecord(TCoinRecord &ARecord);
	void SortItems();
	int  CompareItems(const TCoinRecord &A, const TCoinRecord &B, int AColumn) const;
	String CellText(const TCoinRecord &ARecord, int AColumn) const;
	void UpdateStatus();

	// Отметки (флажки) строк
	void ToggleChecked(int AIndex);	// AIndex в FItems
	void SetAllChecked(bool AChecked);	// для строк текущего списка
	int  CheckedOutside() const;		// отмеченные вне текущего списка
	void UpdateCheckedNode();			// счётчик узла «Отмеченные»
	int  CheckedState() const;	// строки текущего списка: 0 — ни одной, 1 — часть, 2 — все
	void DrawCheckBox(const TRect &ARect, int AState);

	void CreateDetailRows();
	void ShowDetails(int AIndex);	// AIndex в FItems, -1 — пусто
	void UpdateDetails();
	void LayoutDetails();
	int  TextHeight(TLabel *ALabel, int AWidth);
	void __fastcall NumistaLinkClick(TObject *Sender);
	void UpdateRowCount();
	int  CurrentIndex() const;	// индекс в FItems текущей строки сетки, -1 — нет
	int  CurrentId() const;	// Id текущей записи, 0 — нет
	void SelectId(int AId);
	void SelectIndex(int AIndex);

	// Numista (задача 12)
	std::map<String, String> NumistaIssuers();	// страна (AnsiUpperCase) -> код эмитента
	static String CoinText(const TCoinRecord &ARecord);
	// Меню Numista (задача 13e): одна монета / отмеченные.
	std::vector<TCoinRecord> CheckedRecords() const;
	void UpdateNumistaMenuTitles();
	void ShowTypeDetails(int ATypeId);
	void ShowDetailPhotos(const TCoinRecord *ARecord);
	int  DownloadTypePhotos(TNumistaClient &AClient, const TNumistaTypeInfo &AType, int ACoinId,
		bool AReplaceNumista, int &AFailed);
	void __fastcall DetailPhotoDblClick(TObject *Sender);
	void UploadItems(const std::vector<TCoinRecord> &ACandidates);
	void ReadRarity(const std::set<int> &ACoinIds, const String &ACaption);
	String NumistaRequestsText(int ASent) const;
public:		// User declarations
	__fastcall TfmMain(TComponent* Owner);
	__fastcall ~TfmMain();
};
//---------------------------------------------------------------------------
extern PACKAGE TfmMain *fmMain;
//---------------------------------------------------------------------------
#endif
