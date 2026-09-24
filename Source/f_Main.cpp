//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "f_Main.h"
#include "dm_Data.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TfmMain *fmMain;
//---------------------------------------------------------------------------
__fastcall TfmMain::TfmMain(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::FormCreate(TObject *Sender)
{
	FConfig.Load();

	FDatabase.reset(new TDatabase(dmData->FDConnection));
	FDatabase->Open(FConfig.DbPath);
	FDatabase->EnsureSchema();
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::FormDestroy(TObject *Sender)
{
	if (FDatabase)
		FDatabase->Close();
	FDatabase.reset();
}
//---------------------------------------------------------------------------
// Временная проверка БД (убрать в задаче с реальным UI).
void __fastcall TfmMain::btnSelfTestClick(TObject *Sender)
{
	if (!FDatabase || !FDatabase->IsOpen())
	{
		ShowMessage(L"Database is not open.");
		return;
	}

	TFDConnection *conn = FDatabase->Connection();
	String report = L"Database: " + FConfig.DbPath
		+ L"\nSchema version: " + IntToStr(FDatabase->SchemaVersion())
		+ L"\nForeign keys: " + VarToStr(conn->ExecSQLScalar(L"PRAGMA foreign_keys"))
		+ L"\n\nTables:";

	std::unique_ptr<TStringList> tables(new TStringList());
	conn->GetTableNames(L"", L"", L"", tables.get(), TFDPhysObjectScopes() << osMy,
		TFDPhysTableKinds() << tkTable, false);
	for (int i = 0; i < tables->Count; ++i)
	{
		// Имя таблицы берётся из схемы БД, не из ввода пользователя.
		const Variant rows = conn->ExecSQLScalar(L"SELECT COUNT(*) FROM \"" + tables->Strings[i] + L"\"");
		report += L"\n  " + tables->Strings[i] + L": " + VarToStr(rows);
	}
	ShowMessage(report);
}
//---------------------------------------------------------------------------
