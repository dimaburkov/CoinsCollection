//---------------------------------------------------------------------------
// Карточка добавления / редактирования одного экземпляра коллекции.
// Блок «Coin» — поля монеты (общие для всех её экземпляров),
// блок «My item» — поля этого экземпляра, блок «Photo» — фото аверса и реверса (задача 15).
//---------------------------------------------------------------------------
#ifndef f_CoinEditH
#define f_CoinEditH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ComCtrls.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <memory>
#include <vector>
#include "u_CoinTypes.h"
#include "u_PhotoRepository.h"
//---------------------------------------------------------------------------
class TCoinRepository;
//---------------------------------------------------------------------------
// Изменение фото одной стороны у одного владельца (экземпляр или монета-тип).
struct TPhotoEdit
{
	enum TAction { paNone, paSet, paRemove };
	TAction Action;
	std::shared_ptr<TGraphic> Graphic;	// paSet — новое изображение (в памяти до OK)
	TPhotoEdit() : Action(paNone) {}
};
//---------------------------------------------------------------------------
// Изменения фото из карточки (задача 15). Применяются после сохранения записи,
// когда известны Id экземпляра и монеты (новая запись получает их при сохранении).
struct TPhotoEdits
{
	TPhotoEdit Item[2];	// индекс — TPhotoSide
	TPhotoEdit Coin[2];
	bool Empty() const;
	void Apply(TPhotoRepository &ARepository, int AItemId, int ACoinId) const;
};
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
	TLabel *lblYearTo;
	TEdit *edYearTo;
	TEdit *edVariety;
	TEdit *edDiameter;
	TEdit *edCatalogNumber;
	TEdit *edSubject;
	TLabel *lblNumistaQuery;
	TEdit *edNumistaQuery;
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
	TGroupBox *gbPhotos;
	TLabel *lblObverse;
	TPanel *pnlObverse;
	TImage *imgObverse;
	TLabel *lblObverseNote;
	TButton *btnObverseLoad;
	TButton *btnObversePaste;
	TButton *btnObverseRemove;
	TLabel *lblReverse;
	TPanel *pnlReverse;
	TImage *imgReverse;
	TLabel *lblReverseNote;
	TButton *btnReverseLoad;
	TButton *btnReversePaste;
	TButton *btnReverseRemove;
	TCheckBox *chkTypePhoto;
	void __fastcall cbCountryChange(TObject *Sender);
	void __fastcall QueryPartChange(TObject *Sender);
	void __fastcall btnOKClick(TObject *Sender);
	void __fastcall PhotoLoadClick(TObject *Sender);
	void __fastcall PhotoPasteClick(TObject *Sender);
	void __fastcall PhotoRemoveClick(TObject *Sender);
	void __fastcall chkTypePhotoClick(TObject *Sender);
private:	// User declarations
	TCoinRepository        *FRepository;   // не владеет
	TPhotoRepository       *FPhotos;       // не владеет
	String                  FItemFile[2];  // сохранённое фото экземпляра (полный путь; "" — нет), индекс — TPhotoSide
	String                  FCoinFile[2];  // сохранённое фото типа
	TPhotoEdits             FEdits;
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

	// Фото: загрузка сохранённых, показ стороны с учётом FEdits, изменение выбранного
	// владельца (флажок «как фото типа»).
	void LoadPhotos(const TCoinRecord &ARecord);
	void ShowPhoto(TPhotoSide ASide);
	TPhotoEdit &TargetEdit(TPhotoSide ASide);
	void SetPhoto(TPhotoSide ASide, TGraphic *AGraphic);	// забирает AGraphic
	bool LoadPhotoFile(TPhotoSide ASide, const String &AFileName);
	// Перетаскивание файлов из Проводника на рамку.
	void __fastcall WMDropFiles(TWMDropFiles &AMessage);
protected:
	virtual void __fastcall CreateWnd();
public:		// User declarations
	__fastcall TfmCoinEdit(TComponent* Owner);

	// Показать карточку для ARecord (Id == 0 — новый экземпляр).
	// true — пользователь нажал OK, поля ARecord обновлены, APhotoEdits — изменения фото
	// (в БД не пишет: запись — вызывающий код, затем APhotoEdits.Apply).
	static bool Execute(TComponent *AOwner, TCoinRepository &ARepository, TPhotoRepository &APhotos,
		TCoinRecord &ARecord, TPhotoEdits &APhotoEdits);

	BEGIN_MESSAGE_MAP
		VCL_MESSAGE_HANDLER(WM_DROPFILES, TWMDropFiles, WMDropFiles)
	END_MESSAGE_MAP(TForm)
};
//---------------------------------------------------------------------------
extern PACKAGE TfmCoinEdit *fmCoinEdit;
//---------------------------------------------------------------------------
#endif
