//---------------------------------------------------------------------------
// Выбор предметов своей коллекции Numista для импорта в программу (задача 20):
// список предметов, которых нет в программе, фильтр по эмитенту, отметки.
// Форма без .dfm — элементы создаются в конструкторе.
// Документация: https://en.numista.com/api/doc/index.php (User → getCollectedItems).
//---------------------------------------------------------------------------
#ifndef f_NumistaImportH
#define f_NumistaImportH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.ComCtrls.hpp>
#include <vector>
//---------------------------------------------------------------------------
// Строка списка: предмет коллекции Numista.
struct TNumistaImportRow
{
	int    TypeId;
	String Issuer;
	String Title;
	String Year;
	String Grade;
	String InProgram;	// "" — новая монета; иначе — монета программы, с которой будет связан
	TNumistaImportRow() : TypeId(0) {}
};
//---------------------------------------------------------------------------
class TfmNumistaImport : public TForm
{
public:
	__fastcall TfmNumistaImport(TComponent *AOwner);

	// Показать ARows; true — Import, ASelected — индексы отмеченных строк в ARows.
	static bool Execute(TComponent *AOwner, const std::vector<TNumistaImportRow> &ARows, std::vector<int> &ASelected);

private:
	TComboBox *FIssuer;
	TListView *FList;
	TLabel    *FCount;
	TButton   *FImport;

	std::vector<TNumistaImportRow> FRows;
	std::vector<bool>              FChecked;	// по индексу в FRows (отметки сохраняются при смене фильтра)
	bool                           FFilling;

	void Fill();
	void UpdateCount();
	void __fastcall IssuerChange(TObject *Sender);
	void __fastcall CheckAllClick(TObject *Sender);
	void __fastcall UncheckAllClick(TObject *Sender);
	void __fastcall ListItemChecked(TObject *Sender, TListItem *Item);
	void __fastcall ListDblClick(TObject *Sender);
};
//---------------------------------------------------------------------------
#endif
