//---------------------------------------------------------------------------
// Модуль данных: единая точка подключения к SQLite через FireDAC.
// Компоненты пока без настройки — параметры соединения задаются в задаче «БД».
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
//---------------------------------------------------------------------------
class TdmData : public TDataModule
{
__published:	// IDE-managed Components
	TFDConnection *FDConnection;
	TFDTransaction *FDTransaction;
	TFDQuery *FDQuery;
private:	// User declarations
public:		// User declarations
	__fastcall TdmData(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TdmData *dmData;
//---------------------------------------------------------------------------
#endif
