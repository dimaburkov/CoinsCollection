//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <memory>
#include <shellapi.h>
#include "f_NumistaSettings.h"
#include "u_AppConfig.h"
#include "u_NumistaClient.h"
#include "u_Translator.h"
#include "u_Help.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TfmNumistaSettings *fmNumistaSettings;
//---------------------------------------------------------------------------
__fastcall TfmNumistaSettings::TfmNumistaSettings(TComponent* Owner)
	: TForm(Owner), FCounterShown(-1)
{
	Translator().TranslateForm(this);
	InstallHelpKey(this, L"numista-connect");
}
//---------------------------------------------------------------------------
void TfmNumistaSettings::Execute(TComponent *AOwner)
{
	std::unique_ptr<TfmNumistaSettings> form(new TfmNumistaSettings(AOwner));
	form->edApiKey->Text = AppConfig().NumistaApiKey;
	form->LoadOptions();
	form->UpdateInfo();
	form->ShowModal();	// OK сохраняет сам (btnOKClick)
}
//---------------------------------------------------------------------------
void TfmNumistaSettings::LoadOptions()
{
	const TAppConfig &config = AppConfig();
	chkLoadDetails->Checked   = config.NumistaAutoLinkDetails;
	chkLoadPhotos->Checked    = config.NumistaLoadTypePhotos;
	chkUploadGrade->Checked   = config.NumistaUploadGrade;
	chkUploadPrice->Checked   = config.NumistaUploadPrice;
	chkUploadDate->Checked    = config.NumistaUploadDate;
	chkUploadComment->Checked = config.NumistaUploadComment;
	edQuota->Text = IntToStr(config.NumistaMonthlyQuota);
}
//---------------------------------------------------------------------------
bool TfmNumistaSettings::ApplyOptions()
{
	const int quota = StrToIntDef(edQuota->Text.Trim(), -1);
	if (quota < 100 || quota > 100000)
	{
		pcSettings->ActivePage = tsRequests;
		edQuota->SetFocus();
		Application->MessageBox(Format(Tr(L"NumistaSettings.BadNumber"), ARRAYOFCONST((100, 100000))).c_str(),
			Caption.c_str(), MB_OK | MB_ICONWARNING);
		return false;
	}
	const int counter = StrToIntDef(edCounter->Text.Trim(), -1);
	if (counter < 0 || counter > quota)
	{
		pcSettings->ActivePage = tsRequests;
		edCounter->SetFocus();
		Application->MessageBox(Format(Tr(L"NumistaSettings.BadNumber"), ARRAYOFCONST((0, quota))).c_str(),
			Caption.c_str(), MB_OK | MB_ICONWARNING);
		return false;
	}

	ApplyKey();
	TAppConfig &config = AppConfig();
	config.NumistaAutoLinkDetails = chkLoadDetails->Checked;
	config.NumistaLoadTypePhotos  = chkLoadPhotos->Checked;
	config.NumistaUploadGrade     = chkUploadGrade->Checked;
	config.NumistaUploadPrice     = chkUploadPrice->Checked;
	config.NumistaUploadDate      = chkUploadDate->Checked;
	config.NumistaUploadComment   = chkUploadComment->Checked;
	config.NumistaMonthlyQuota    = quota;
	if (counter != FCounterShown)	// исправлен вручную
		TNumistaClient::SetRequestsThisMonth(counter);
	config.Save();
	return true;
}
//---------------------------------------------------------------------------
void __fastcall TfmNumistaSettings::btnOKClick(TObject *Sender)
{
	if (ApplyOptions())
		ModalResult = mrOk;
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

	const int requests = TNumistaClient::RequestsThisMonth();
	lblRequests->Caption = Format(Tr(L"NumistaSettings.Requests"),
		ARRAYOFCONST((requests, config.NumistaMonthlyQuota)));
	lblUsed->Caption = Format(Tr(L"NumistaSettings.Used"), ARRAYOFCONST((requests, config.NumistaMonthlyQuota)));
	// Поле счётчика — новое значение после «Перевірити підключення», если пользователь его не правил.
	if (FCounterShown < 0 || StrToIntDef(edCounter->Text.Trim(), -1) == FCounterShown)
	{
		edCounter->Text = IntToStr(requests);
		FCounterShown = requests;
	}

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
// Ссылка — страница получения ключа API (нужна пользователю; документация API — разработчику).
void __fastcall TfmNumistaSettings::lblDocsClick(TObject *Sender)
{
	ShellExecuteW(Handle, L"open", L"https://en.numista.com/api/api_key.php", nullptr, nullptr, SW_SHOWNORMAL);
}
//---------------------------------------------------------------------------
