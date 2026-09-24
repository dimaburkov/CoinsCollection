//---------------------------------------------------------------------------
// Модуль данных: единая точка подключения к SQLite через FireDAC.
// Файл БД задаётся в коде (TDatabase::Open), не в .dfm.
//---------------------------------------------------------------------------
#ifndef dm_DataH
#define dm_DataH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Data.DB.hpp>
#include <FireDAC.Stan.Intf.hpp>
#include <FireDAC.Stan.Option.hpp>
#include <FireDAC.Stan.Error.hpp>
#include <FireDAC.UI.Intf.hpp>
#include <FireDAC.Phys.Intf.hpp>
#include <FireDAC.Stan.Def.hpp>
#include <FireDAC.Stan.Pool.hpp>
#include <FireDAC.Stan.Async.hpp>
#include <FireDAC.Phys.hpp>
#include <FireDAC.Stan.Param.hpp>
#include <FireDAC.DatS.hpp>
#include <FireDAC.DApt.Intf.hpp>
#include <FireDAC.DApt.hpp>
#include <FireDAC.Comp.DataSet.hpp>
#include <FireDAC.Comp.Client.hpp>
#include <FireDAC.Stan.ExprFuncs.hpp>
#include <FireDAC.Phys.SQLiteDef.hpp>
#include <FireDAC.Phys.SQLite.hpp>
#include <FireDAC.Phys.SQLiteWrapper.Stat.hpp>	// SQLite вшит в драйвер, sqlite3.dll не нужен
#include <FireDAC.Comp.UI.hpp>
#include <FireDAC.VCLUI.Wait.hpp>
//---------------------------------------------------------------------------
class TdmData : public TDataModule
{
__published:	// IDE-managed Components
	TFDConnection *FDConnection;
	TFDTransaction *FDTransaction;
	TFDQuery *FDQuery;
	TFDPhysSQLiteDriverLink *FDPhysSQLiteDriverLink;
	TFDGUIxWaitCursor *FDGUIxWaitCursor;
private:	// User declarations
public:		// User declarations
	__fastcall TdmData(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TdmData *dmData;
//---------------------------------------------------------------------------
#endif
