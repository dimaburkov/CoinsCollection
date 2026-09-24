//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include <tchar.h>
//---------------------------------------------------------------------------
USEFORM("dm_Data.cpp", dmData); /* TDataModule: File Type */
USEFORM("f_Main.cpp", fmMain);
USEFORM("f_CoinEdit.cpp", fmCoinEdit);
USEFORM("f_Import.cpp", fmImport);
USEUNIT("u_CoinCollection.cpp");
USEUNIT("u_Database.cpp");
USEUNIT("u_CoinRepository.cpp");
USEUNIT("u_LookupRepository.cpp");
USEUNIT("u_ImportSource.cpp");
USEUNIT("u_CoinImporter.cpp");
USEUNIT("u_NumistaClient.cpp");
USEUNIT("u_AppConfig.cpp");
USEUNIT("versionConfig.cpp");
USEUNIT("u_SeedData.cpp");
USEUNIT("u_Translator.cpp");
//---------------------------------------------------------------------------
#include "u_AppConfig.h"
#include "u_Translator.h"
//---------------------------------------------------------------------------
int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
	try
	{
		Application->Initialize();
		Application->MainFormOnTaskBar = true;

		// Настройки и язык — до создания форм: формы переводятся в конструкторах.
		AppConfig().Load();
		Translator().Load(TAppConfig::ExeDir() + L"Lang");
		Translator().SetLanguage(AppConfig().Language);

		Application->CreateForm(__classid(TdmData), &dmData);
		Application->CreateForm(__classid(TfmMain), &fmMain);
		Application->Run();
	}
	catch (Exception &exception)
	{
		Application->ShowException(&exception);
	}
	catch (...)
	{
		try
		{
			throw Exception("");
		}
		catch (Exception &exception)
		{
			Application->ShowException(&exception);
		}
	}
	return 0;
}
//---------------------------------------------------------------------------
