//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <shellapi.h>
#include "u_Help.h"
#include "u_AppConfig.h"
#include "u_Translator.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
namespace
{
	String TopicFile(const String &ALanguage, const String &ATopic)
	{
		return TAppConfig::ExeDir() + L"Help\\" + ALanguage + L"\\" + ATopic + L".html";
	}
	//-----------------------------------------------------------------------
	// Перехват F1 формы; остальные клавиши — прежнему обработчику OnKeyDown.
	class THelpKey : public TComponent
	{
	public:
		__fastcall THelpKey(TForm *AForm, const String &ATopic)
			: TComponent(AForm),
			  FTopic(ATopic),
			  FPrevious(AForm->OnKeyDown)
		{
			AForm->KeyPreview = true;
			AForm->OnKeyDown  = KeyDown;
		}

	private:
		String     FTopic;
		TKeyEvent  FPrevious;

		void __fastcall KeyDown(TObject *Sender, WORD &Key, TShiftState Shift)
		{
			if (Key == VK_F1 && Shift.Empty())
			{
				Key = 0;
				ShowHelp(FTopic);
				return;
			}
			if (FPrevious)
				FPrevious(Sender, Key, Shift);
		}
	};
}
//---------------------------------------------------------------------------
void ShowHelp(const String &ATopic)
{
	String file = TopicFile(Translator().Language(), ATopic);
	if (!FileExists(file))
		file = TopicFile(TTranslator::BaseLanguage, ATopic);
	if (!FileExists(file))
	{
		Application->MessageBox((Tr(L"Help.NotFound") + L"\r\n" + file).c_str(), Application->Title.c_str(),
			MB_OK | MB_ICONWARNING);
		return;
	}
	ShellExecuteW(Application->Handle, L"open", file.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}
//---------------------------------------------------------------------------
void InstallHelpKey(TForm *AForm, const String &ATopic)
{
	new THelpKey(AForm, ATopic);	// владелец — форма
}
//---------------------------------------------------------------------------
