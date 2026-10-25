//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <memory>
#include <shellapi.h>
#include "f_NumistaSettings.h"
#include "u_AppConfig.h"
#include "u_NumistaClient.h"
#include "u_Translator.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TfmNumistaSettings *fmNumistaSettings;
//---------------------------------------------------------------------------
__fastcall TfmNumistaSettings::TfmNumistaSettings(TComponent* Owner)
	: TForm(Owner)
{
	Translator().TranslateForm(this);
}
//---------------------------------------------------------------------------
void TfmNumistaSettings::Execute(TComponent *AOwner)
{
	std::unique_ptr<TfmNumistaSettings> form(new TfmNumistaSettings(AOwner));
	form->edApiKey->Text = AppConfig().NumistaApiKey;
	form->UpdateInfo();
	if (form->ShowModal() == mrOk)
		form->ApplyKey();
}
//---------------------------------------------------------------------------
void TfmNumistaSettings::ApplyKey()
{
	TAppConfig &config = AppConfig();
	const String key = edApiKey->Text.Trim();
	if (key == config.NumistaApiKey)
		return;
	config.NumistaApiKey       = key;
	config.NumistaToken        = L"";	// токен выдан для старого ключа
	config.NumistaTokenExpires = 0.0;
	config.NumistaUserId       = 0;
	config.Save();
}
//---------------------------------------------------------------------------
void TfmNumistaSettings::UpdateInfo()
{
	const TAppConfig &config = AppConfig();
	lblStatus->Font->Color = clWindowText;
	lblStatus->Caption = TNumistaClient::HasValidToken()
		? Format(Tr(L"NumistaSettings.Connected"), ARRAYOFCONST((config.NumistaUserId,
			FormatDateTime(L"dd.mm.yyyy", config.NumistaTokenExpires))))
		: Tr(L"NumistaSettings.NotConnected");

	lblRequests->Caption = Format(Tr(L"NumistaSettings.Requests"),
		ARRAYOFCONST((TNumistaClient::RequestsThisMonth(), config.NumistaMonthlyQuota)));

	const int issuers = TNumistaClient::CachedCount(L"issuers");
	const int catalogues = TNumistaClient::CachedCount(L"catalogues");
	lblLists->Caption = issuers < 0 || catalogues < 0
		? Tr(L"NumistaSettings.ListsNone")
		: Format(Tr(L"NumistaSettings.Lists"), ARRAYOFCONST((issuers, catalogues)));
}
//---------------------------------------------------------------------------
void TfmNumistaSettings::ShowError(const String &AText)
{
	UpdateInfo();
	lblStatus->Font->Color = clRed;
	lblStatus->Caption = AText;
}
//---------------------------------------------------------------------------
void __fastcall TfmNumistaSettings::chkShowKeyClick(TObject *Sender)
{
	edApiKey->PasswordChar = chkShowKey->Checked ? L'\0' : L'*';
}
//---------------------------------------------------------------------------
// Проверка подключения: токен (если действующего нет) и, при первом
// подключении, списки эмитентов и каталогов — всего 1–3 запроса.
void __fastcall TfmNumistaSettings::btnCheckClick(TObject *Sender)
{
	if (edApiKey->Text.Trim().IsEmpty())
		return ShowError(Tr(L"Numista.Error.NoKey"));
	ApplyKey();

	const TCursor old = Screen->Cursor;
	Screen->Cursor = crHourGlass;
	try
	{
		TNumistaClient client;
		client.Authorize();
		client.LoadIssuers(false);
		client.LoadCatalogues(false);
		Screen->Cursor = old;
		UpdateInfo();
	}
	catch (Exception &e)
	{
		Screen->Cursor = old;
		ShowError(e.Message);
	}
}
//---------------------------------------------------------------------------
void __fastcall TfmNumistaSettings::btnRefreshListsClick(TObject *Sender)
{
	if (edApiKey->Text.Trim().IsEmpty())
		return ShowError(Tr(L"Numista.Error.NoKey"));
	ApplyKey();

	const TCursor old = Screen->Cursor;
	Screen->Cursor = crHourGlass;
	try
	{
		TNumistaClient client;
		client.LoadIssuers(true);
		client.LoadCatalogues(true);
		Screen->Cursor = old;
		UpdateInfo();
	}
	catch (Exception &e)
	{
		Screen->Cursor = old;
		ShowError(e.Message);
	}
}
//---------------------------------------------------------------------------
void __fastcall TfmNumistaSettings::lblDocsClick(TObject *Sender)
{
	ShellExecuteW(Handle, L"open", L"https://en.numista.com/api/doc/index.php", nullptr, nullptr, SW_SHOWNORMAL);
}
//---------------------------------------------------------------------------
