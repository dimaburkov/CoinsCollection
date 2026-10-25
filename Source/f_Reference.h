//---------------------------------------------------------------------------
// Справочники: страны (континент, «существует»), периоды, валюты.
// Добавление, правка, удаление неиспользуемых записей.
//---------------------------------------------------------------------------
#ifndef f_ReferenceH
#define f_ReferenceH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ComCtrls.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.Grids.hpp>
#include <map>
#include <vector>
#include "u_CoinTypes.h"
//---------------------------------------------------------------------------
class TLookupRepository;
//---------------------------------------------------------------------------
class TfmReference : public TForm
{
__published:	// IDE-managed Components
	TPageControl *PageControl;
	TTabSheet *tabCountries;
	TPanel *pnlCountriesTop;
	TCheckBox *chkOnlyNoContinent;
	TDrawGrid *dgCountries;
	TPanel *pnlCountryEdit;
	TLabel *lblCountryName;
	TLabel *lblContinent;
	TLabel *lblRenameWarning;
	TEdit *edCountryName;
	TComboBox *cbContinent;
	TCheckBox *chkExists;
	TLabel *lblNumistaIssuer;
	TEdit *edNumistaIssuer;
	TButton *btnCountryAdd;
	TButton *btnCountrySave;
	TButton *btnCountryDelete;
	TTabSheet *tabPeriods;
	TPanel *pnlPeriodsTop;
	TLabel *lblPeriodCountry;
	TComboBox *cbPeriodCountry;
	TDrawGrid *dgPeriods;
	TPanel *pnlPeriodEdit;
	TLabel *lblPeriodName;
	TEdit *edPeriodName;
	TButton *btnPeriodAdd;
	TButton *btnPeriodSave;
	TButton *btnPeriodDelete;
	TTabSheet *tabCurrencies;
	TDrawGrid *dgCurrencies;
	TPanel *pnlCurrencyEdit;
	TLabel *lblCurrencyName;
	TEdit *edCurrencyName;
	TButton *btnCurrencyAdd;
	TButton *btnCurrencySave;
	TButton *btnCurrencyDelete;
	TPanel *pnlBottom;
	TButton *btnClose;
	void __fastcall GridDrawCell(TObject *Sender, int ACol, int ARow, TRect &Rect, TGridDrawState State);
	void __fastcall GridSelectCell(TObject *Sender, int ACol, int ARow, bool &CanSelect);
	void __fastcall chkOnlyNoContinentClick(TObject *Sender);
	void __fastcall edCountryNameChange(TObject *Sender);
	void __fastcall btnCountryAddClick(TObject *Sender);
	void __fastcall btnCountrySaveClick(TObject *Sender);
	void __fastcall btnCountryDeleteClick(TObject *Sender);
	void __fastcall cbPeriodCountryChange(TObject *Sender);
	void __fastcall btnPeriodAddClick(TObject *Sender);
	void __fastcall btnPeriodSaveClick(TObject *Sender);
	void __fastcall btnPeriodDeleteClick(TObject *Sender);
	void __fastcall btnCurrencyAddClick(TObject *Sender);
	void __fastcall btnCurrencySaveClick(TObject *Sender);
	void __fastcall btnCurrencyDeleteClick(TObject *Sender);
private:	// User declarations
	TLookupRepository       *FLookups;        // не владеет
	bool                     FChanged;        // что-то сохранено / удалено — главному окну перечитать
	std::vector<TContinent>  FContinents;     // по id; cbContinent: 0 — «—», i + 1 — FContinents[i]
	std::vector<TCountry>    FAllCountries;   // для выбора страны на вкладке Periods
	std::vector<TCountry>    FCountries;      // показанные на вкладке Countries (строка сетки = индекс + 1)
	std::map<int, int>       FCountryUsage;
	int                      FCountryId;      // последняя показанная / сохранённая страна (выбирается снова, когда видна)
	std::vector<TPeriod>     FPeriods;
	std::map<int, int>       FPeriodUsage;
	std::vector<TCurrency>   FCurrencies;
	std::map<int, int>       FCurrencyUsage;

	void LoadCountries(int ASelectId = 0);
	void LoadPeriods(int ASelectId = 0);
	void LoadCurrencies(int ASelectId = 0);
	void ShowCountry(int AIndex);
	void ShowPeriod(int AIndex);
	void ShowCurrency(int AIndex);

	int    CountryIndex() const;
	int    PeriodIndex() const;
	int    CurrencyIndex() const;
	int    PeriodCountryId() const;
	int    ContinentIdFromCombo() const;
	String ContinentName(int AId) const;
	String CellText(TObject *AGrid, int ACol, int ARow) const;

	void   Warn(const String &AText);
	bool   Confirm(const String &AText);
	static int  Usage(const std::map<int, int> &AUsage, int AId);
	static void SetRows(TDrawGrid *AGrid, size_t ACount, int ASelectIndex);
public:		// User declarations
	__fastcall TfmReference(TComponent* Owner);

	// Вкладки ATab: 0 — Countries, 1 — Periods, 2 — Currencies.
	// true — справочники изменены (главному окну перечитать дерево и список).
	static bool Execute(TComponent *AOwner, TLookupRepository &ALookups, int ATab);
};
//---------------------------------------------------------------------------
extern PACKAGE TfmReference *fmReference;
//---------------------------------------------------------------------------
#endif
