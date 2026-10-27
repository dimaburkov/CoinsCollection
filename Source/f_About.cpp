//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <algorithm>
#include <memory>
#include <shellapi.h>
#include <Vcl.Clipbrd.hpp>
#include "f_About.h"
#include "u_AppConfig.h"
#include "u_Help.h"
#include "u_Translator.h"
#include "versionConfig.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
namespace
{
	const wchar_t *const cEmail = L"progua@ukr.net";
	const int cLeft   = 72;		// текст — правее иконки
	const int cWidth  = 440;	// ширина текста
}
//---------------------------------------------------------------------------
// TForm(AOwner, 0) — CreateNew: у формы нет .dfm.
__fastcall TfmAbout::TfmAbout(TComponent *AOwner)
	: TForm(AOwner, 0),
	  FTop(16)
{
	BorderStyle = bsDialog;
	Position    = poOwnerFormCenter;
	Caption     = Tr(L"About.Title");
	ClientWidth = cLeft + cWidth + 16;
	KeyPreview  = true;

	TImage *icon = new TImage(this);
	icon->Parent = this;
	icon->SetBounds(16, 16, 40, 40);
	icon->Center = true;
	icon->Picture->Icon->Assign(Application->Icon);

	InstallHelpKey(this, L"index");
}
//---------------------------------------------------------------------------
TLabel *TfmAbout::AddLabel(const String &ACaption, int ALeft, int AWidth)
{
	TLabel *label = new TLabel(this);
	label->Parent        = this;
	label->ShowAccelChar = false;
	label->AutoSize      = false;
	label->WordWrap      = true;
	label->Caption       = ACaption;
	label->SetBounds(cLeft + ALeft, FTop, AWidth > 0 ? AWidth : cWidth - ALeft, 16);
	// Высота — по тексту (перенос строк).
	Canvas->Font->Assign(label->Font);
	RECT rc = { 0, 0, label->Width, 0 };
	DrawTextW(Canvas->Handle, ACaption.c_str(), -1, &rc, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
	label->Height = std::max<int>(rc.bottom, 16);
	return label;
}
//---------------------------------------------------------------------------
// Ссылка: ATarget — адрес (https://, mailto:) или папка; открывается по клику.
TLabel *TfmAbout::AddLink(const String &ACaption, const String &ATarget, int ALeft)
{
	TLabel *link = new TLabel(this);
	link->Parent        = this;
	link->ShowAccelChar = false;
	link->Caption       = ACaption;
	link->Hint          = ATarget;
	link->Cursor        = crHandPoint;
	link->Font->Color   = clHotLight;
	link->Font->Style   = TFontStyles() << fsUnderline;
	link->Left          = cLeft + ALeft;
	link->Top           = FTop;
	link->OnClick       = LinkClick;
	return link;
}
//---------------------------------------------------------------------------
void TfmAbout::Execute(TComponent *AOwner, const String &ADbFile, int ASchema)
{
	std::unique_ptr<TfmAbout> form(new TfmAbout(AOwner));
	TfmAbout *f = form.get();

	// Название, версия и дата сборки (дата файла exe).
	TLabel *name = f->AddLabel(L"Coins Collection");
	name->Font->Size  = 14;
	name->Font->Style = TFontStyles() << fsBold;
	name->Height = 26;
	f->FTop += 30;
	TDateTime built;
	const String date = FileAge(ParamStr(0), built) ? FormatDateTime(L"yyyy-mm-dd", built) : String();
	f->AddLabel(Format(Tr(L"About.Version"), ARRAYOFCONST((AppInfo::Version, date))));
	f->FTop += 22;
	TLabel *purpose = f->AddLabel(Tr(L"About.Purpose"));
	f->FTop += purpose->Height + 12;

	// Контакт автора.
	TLabel *contact = f->AddLabel(Tr(L"About.Contact"));
	f->FTop += contact->Height + 4;
	TLabel *email = f->AddLink(cEmail, String(L"mailto:") + cEmail, 0);
	TButton *copy = new TButton(f);
	copy->Parent  = f;
	copy->Caption = Tr(L"About.Copy");
	copy->SetBounds(cLeft + email->Width + 12, f->FTop - 4, 90, 23);
	copy->OnClick = f->CopyEmailClick;
	f->FTop += 30;

	// Источники данных.
	TLabel *numista = f->AddLabel(Tr(L"About.Numista"), 0, 300);
	f->AddLink(L"numista.com", L"https://en.numista.com/", 304);
	f->FTop += numista->Height + 6;
	TLabel *ucoin = f->AddLabel(Tr(L"About.UCoin"), 0, 300);
	f->AddLink(L"ucoin.net", L"https://ucoin.net/", 304);
	f->FTop += ucoin->Height + 16;

	// Данные программы.
	// Длинный путь — с многоточием посередине; полный путь открывается по клику.
	TLabel *folder = f->AddLabel(Tr(L"About.DataFolder"), 0, 140);
	TLabel *folderLink = f->AddLink(TAppConfig::ExeDir(), TAppConfig::ExeDir(), 144);
	folderLink->AutoSize = false;
	folderLink->Width = cWidth - 144;
	folderLink->EllipsisPosition = epPathEllipsis;
	f->FTop += folder->Height + 4;
	f->AddLabel(Format(Tr(L"About.Database"), ARRAYOFCONST((ExtractRelativePath(TAppConfig::ExeDir(), ADbFile), ASchema))));
	f->FTop += 30;

	TButton *ok = new TButton(f);
	ok->Parent      = f;
	ok->Caption     = L"OK";
	ok->Default     = true;
	ok->Cancel      = true;	// Esc — закрыть
	ok->ModalResult = mrOk;
	ok->SetBounds(f->ClientWidth - 16 - 90, f->FTop, 90, 25);
	f->ClientHeight = f->FTop + 25 + 14;

	f->ShowModal();
}
//---------------------------------------------------------------------------
void __fastcall TfmAbout::LinkClick(TObject *Sender)
{
	const String target = static_cast<TLabel *>(Sender)->Hint;
	ShellExecuteW(Handle, L"open", target.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}
//---------------------------------------------------------------------------
void __fastcall TfmAbout::CopyEmailClick(TObject *Sender)
{
	Clipboard()->AsText = cEmail;
	static_cast<TButton *>(Sender)->Caption = Tr(L"About.Copied");
}
//---------------------------------------------------------------------------
