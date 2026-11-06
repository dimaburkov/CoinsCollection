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
#include <functional>
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

	// Выбор монеты программы для строки ARow (задача 40): true — выбор сделан, AInProgram — новый текст колонки.
	typedef std::function<bool(int ARow, String &AInProgram)> TChooseCoin;

	// Показать ARows; true — Import, ASelected — индексы отмеченных строк в ARows.
	// AChooseCoin — кнопка «Монета в програмі…» (пусто — кнопки нет).
	static bool Execute(TComponent *AOwner, const std::vector<TNumistaImportRow> &ARows, std::vector<int> &ASelected,
		TChooseCoin AChooseCoin = TChooseCoin());
	// Список для выбора (задача 40): ALines[0] — «нова монета», остальные — монеты программы;
	// индекс выбранной строки, -1 — отмена.
	static int ChooseLine(TComponent *AOwner, const String &ACaption, const std::vector<String> &ALines, int ACurrent);

private:
	TComboBox *FIssuer;
	TListView *FList;
	TLabel    *FCount;
	TButton   *FImport;
	TButton   *FChoose;
	TChooseCoin FChooseCoin;

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
	void __fastcall ChooseClick(TObject *Sender);
};
//---------------------------------------------------------------------------
#endif
