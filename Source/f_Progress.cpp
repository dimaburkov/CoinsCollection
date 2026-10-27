//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "f_Progress.h"
#include "u_AppConfig.h"
#include "u_NumistaClient.h"
#include "u_Translator.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
namespace
{
	const int cMinItemsToShow = 4;	// до 3 элементов окно не показывается
}
//---------------------------------------------------------------------------
// TForm(AOwner, 0) — CreateNew: у формы нет .dfm.
__fastcall TfmProgress::TfmProgress(TComponent *AOwner)
	: TForm(AOwner, 0),
	  Step(nullptr),
	  Item(nullptr),
	  Bar(nullptr),
	  Requests(nullptr),
	  Stop(nullptr),
	  Stopped(false)
{
	BorderStyle  = bsDialog;
	BorderIcons  = TBorderIcons();	// без крестика: остановка — кнопкой Stop
	Position     = poOwnerFormCenter;
	ClientWidth  = 460;
	ClientHeight = 132;
	OnCloseQuery = FormCloseQuery;

	auto label = [this](int ATop) {
		TLabel *l = new TLabel(this);
		l->Parent        = this;
		l->AutoSize      = false;
		l->ShowAccelChar = false;
		l->EllipsisPosition = epEndEllipsis;
		l->SetBounds(12, ATop, ClientWidth - 24, 16);
		return l;
	};
	Step = label(10);
	Step->Font->Style = TFontStyles() << fsBold;
	Item = label(30);

	Bar = new TProgressBar(this);
	Bar->Parent = this;
	Bar->SetBounds(12, 52, ClientWidth - 24, 18);

	Requests = label(76);
	Requests->Font->Color = clGrayText;

	Stop = new TButton(this);
	Stop->Parent  = this;
	Stop->SetBounds(ClientWidth - 12 - 96, 98, 96, 25);
	Stop->Caption = Tr(L"Progress.Stop");
	Stop->Cancel  = true;	// Esc — Stop
	Stop->OnClick = StopClick;
}
//---------------------------------------------------------------------------
void __fastcall TfmProgress::StopClick(TObject *Sender)
{
	Stopped = true;
	Stop->Enabled = false;
	Stop->Caption = Tr(L"Progress.Stopping");
}
//---------------------------------------------------------------------------
void __fastcall TfmProgress::FormCloseQuery(TObject *Sender, bool &CanClose)
{
	CanClose = false;	// закрывает только TProgress (Alt+F4 — как Stop)
	StopClick(Sender);
}
//---------------------------------------------------------------------------
TProgress::TProgress(TForm *AOwner, const String &ACaption, int ATotal, const TNumistaClient *AClient)
	: FOwner(AOwner),
	  FForm(nullptr),
	  FCaption(ACaption),
	  FTotal(ATotal),
	  FClient(AClient)
{
	if (ATotal > 0 && ATotal < cMinItemsToShow)
		return;

	FForm = new TfmProgress(AOwner);
	FForm->Caption = ACaption;
	if (ATotal > 0)
	{
		FForm->Bar->Max = ATotal;
		FForm->Bar->Position = 0;
	}
	else
		FForm->Bar->Style = pbstMarquee;
	FForm->Step->Caption = ACaption;
	Refresh();
	// Главное окно недоступно, пока идёт операция: повторный запуск команды невозможен.
	FOwner->Enabled = false;
	FForm->Show();
	FForm->Update();
}
//---------------------------------------------------------------------------
TProgress::~TProgress()
{
	if (!FForm)
		return;
	FOwner->Enabled = true;
	FForm->Hide();
	delete FForm;
	FOwner->SetFocus();
}
//---------------------------------------------------------------------------
bool TProgress::Step(int AIndex, const String &AItemText)
{
	if (!FForm)
		return true;
	FForm->Step->Caption = Format(Tr(L"Progress.Step"), ARRAYOFCONST((FCaption, AIndex + 1, FTotal)));
	FForm->Item->Caption = AItemText;
	FForm->Bar->Position = AIndex;
	Refresh();
	FForm->Update();
	Application->ProcessMessages();
	return !FForm->Stopped;
}
//---------------------------------------------------------------------------
void TProgress::SetText(const String &AText)
{
	if (!FForm)
		return;
	FForm->Item->Caption = AText;
	Refresh();
	FForm->Update();
	Application->ProcessMessages();
}
//---------------------------------------------------------------------------
bool TProgress::Stopped() const
{
	return FForm && FForm->Stopped;
}
//---------------------------------------------------------------------------
void TProgress::Refresh()
{
	if (FClient)
		FForm->Requests->Caption = Format(Tr(L"Progress.Requests"), ARRAYOFCONST((FClient->RequestsMade,
			TNumistaClient::RequestsThisMonth(), AppConfig().NumistaMonthlyQuota)));
	else
		FForm->Requests->Caption = L"";
}
//---------------------------------------------------------------------------
