//---------------------------------------------------------------------------
// Карточка добавления / редактирования одного экземпляра коллекции.
// Блок «Coin» — поля монеты (общие для всех её экземпляров),
// блок «My item» — поля этого экземпляра.
//---------------------------------------------------------------------------
#ifndef f_CoinEditH
#define f_CoinEditH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ComCtrls.hpp>
#include <vector>
#include "u_CoinTypes.h"
//---------------------------------------------------------------------------
class TCoinRepository;
//---------------------------------------------------------------------------
class TfmCoinEdit : public TForm
{
__published:	// IDE-managed Components
	TLabel *lblShared;
	TGroupBox *gbCoin;
	TLabel *lblCountry;
	TLabel *lblPeriod;
	TLabel *lblCurrency;
	TLabel *lblDenomination;
	TLabel *lblYear;
	TLabel *lblVariety;
	TLabel *lblDiameter;
	TLabel *lblCatalogNumber;
	TLabel *lblSubject;
	TComboBox *cbCountry;
	TComboBox *cbPeriod;
	TComboBox *cbCurrency;
	TEdit *edDenomination;
	TEdit *edYear;
	TEdit *edVariety;
	TEdit *edDiameter;
	TEdit *edCatalogNumber;
	TEdit *edSubject;
	TGroupBox *gbItem;
	TLabel *lblCondition;
	TLabel *lblQuantity;
	TLabel *lblCatalogValue;
	TLabel *lblMyValue;
	TLabel *lblPurchaseDate;
	TLabel *lblPublishedDate;
	TLabel *lblColor;
	TLabel *lblGradingCompany;
	TLabel *lblGradingNumber;
	TLabel *lblGrade;
	TLabel *lblLabel;
	TLabel *lblSwap;
	TLabel *lblComment;
	TComboBox *cbCondition;
	TEdit *edQuantity;
	TUpDown *udQuantity;
	TEdit *edCatalogValue;
	TEdit *edMyValue;
	TDateTimePicker *dtPurchaseDate;
	TDateTimePicker *dtPublishedDate;
	TCheckBox *chkNeedToReplace;
	TEdit *edColor;
	TEdit *edGradingCompany;
	TEdit *edGradingNumber;
	TEdit *edGrade;
	TEdit *edLabel;
	TEdit *edSwap;
	TMemo *mmComment;
	TButton *btnOK;
	TButton *btnCancel;
	void __fastcall cbCountryChange(TObject *Sender);
	void __fastcall btnOKClick(TObject *Sender);
private:	// User declarations
	TCoinRepository        *FRepository;   // не владеет
	std::vector<TCountry>   FCountries;
	std::vector<TCondition> FConditions;   // индекс в cbCondition = индекс + 1 (0 — «нет»)
	String                  FLoadedCountry;

	void LoadLookups();
	void LoadPeriods();
	void LoadFrom(const TCoinRecord &ARecord);
	bool Validate();
	void SaveTo(TCoinRecord &ARecord) const;

	bool Fail(TWinControl *AControl, const String &AMessage);
	static String FieldName(TLabel *ALabel);
	static bool   TryParseNumber(const String &AText, double &AValue);
	static String NumberText(double AValue);
public:		// User declarations
	__fastcall TfmCoinEdit(TComponent* Owner);

	// Показать карточку для ARecord (Id == 0 — новый экземпляр).
	// true — пользователь нажал OK, поля ARecord обновлены (в БД не пишет).
	static bool Execute(TComponent *AOwner, TCoinRepository &ARepository, TCoinRecord &ARecord);
};
//---------------------------------------------------------------------------
extern PACKAGE TfmCoinEdit *fmCoinEdit;
//---------------------------------------------------------------------------
#endif
