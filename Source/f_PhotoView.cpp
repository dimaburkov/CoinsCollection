//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <algorithm>
#include <Vcl.Imaging.jpeg.hpp>
#include "f_PhotoView.h"
#include "u_Translator.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
namespace
{
	const double cZoomStep = 1.25;
	const double cMaxZoom  = 8.0;	// 800 %
}
//---------------------------------------------------------------------------
// TForm(AOwner, 0) — CreateNew: у формы нет .dfm.
__fastcall TfmPhotoView::TfmPhotoView(TComponent *AOwner)
	: TForm(AOwner, 0),
	  FToolbar(nullptr),
	  FObverse(nullptr),
	  FReverse(nullptr),
	  FFit(nullptr),
	  FHint(nullptr),
	  FView(nullptr),
	  FSide(psObverse),
	  FZoom(0.0),
	  FPanX(0.0),
	  FPanY(0.0),
	  FDragging(false),
	  FDragPanX(0.0),
	  FDragPanY(0.0)
{
	BorderStyle  = bsSizeable;
	Position     = poOwnerFormCenter;
	Width        = 900;
	Height       = 720;
	KeyPreview   = true;
	Color        = clBtnFace;
	OnMouseWheel = FormMouseWheel;
	OnKeyDown    = FormKeyDown;
	OnResize     = FormResize;

	FToolbar = new TPanel(this);
	FToolbar->Parent     = this;
	FToolbar->Align      = alTop;
	FToolbar->Height     = 32;
	FToolbar->BevelOuter = bvNone;

	auto button = [this](int ALeft, int AWidth, const String &ACaption, int AGroup) {
		TSpeedButton *b = new TSpeedButton(this);
		b->Parent     = FToolbar;
		b->SetBounds(ALeft, 4, AWidth, 24);
		b->Caption    = ACaption;
		b->GroupIndex = AGroup;
		return b;
	};
	FObverse = button(8,   90, Tr(L"PhotoView.Obverse"), 1);
	FReverse = button(100, 90, Tr(L"PhotoView.Reverse"), 1);
	FFit     = button(200, 120, Tr(L"PhotoView.Fit"), 0);
	FObverse->Tag = psObverse;
	FReverse->Tag = psReverse;
	FObverse->OnClick = SideClick;
	FReverse->OnClick = SideClick;
	FFit->OnClick     = FitClick;

	FHint = new TLabel(this);
	FHint->Parent     = FToolbar;
	FHint->Left       = 332;
	FHint->Top        = 9;
	FHint->Caption    = Tr(L"PhotoView.Hint");
	FHint->Font->Color = clGrayText;

	FView = new TPaintBox(this);
	FView->Parent      = this;
	FView->Align       = alClient;
	FView->OnPaint     = ViewPaint;
	FView->OnMouseDown = ViewMouseDown;
	FView->OnMouseMove = ViewMouseMove;
	FView->OnMouseUp   = ViewMouseUp;

	DoubleBuffered = true;	// без мерцания при масштабе и сдвиге
}
//---------------------------------------------------------------------------
void TfmPhotoView::Execute(TComponent *AOwner, const String &ATitle, const String &AObverseFile,
	const String &AReverseFile, TPhotoSide ASide)
{
	std::unique_ptr<TfmPhotoView> form(new TfmPhotoView(AOwner));
	form->Caption  = ATitle;
	form->FFiles[psObverse] = AObverseFile;
	form->FFiles[psReverse] = AReverseFile;
	form->FObverse->Enabled = !AObverseFile.IsEmpty();
	form->FReverse->Enabled = !AReverseFile.IsEmpty();
	form->ShowSide(form->FFiles[ASide].IsEmpty() ? (TPhotoSide)(1 - ASide) : ASide);
	form->ShowModal();
}
//---------------------------------------------------------------------------
// Файл декодируется один раз (в TBitmap): перерисовка при масштабе и сдвиге — быстрая.
// Ошибка чтения — пустое окно, без сообщений.
void TfmPhotoView::ShowSide(TPhotoSide ASide)
{
	FSide = ASide;
	(ASide == psObverse ? FObverse : FReverse)->Down = true;
	FBitmap.reset();
	try
	{
		if (!FFiles[ASide].IsEmpty())
		{
			std::unique_ptr<TPicture> picture(new TPicture());
			picture->LoadFromFile(FFiles[ASide]);
			std::unique_ptr<TBitmap> bitmap(new TBitmap());
			bitmap->PixelFormat = pf24bit;
			bitmap->SetSize(picture->Width, picture->Height);
			bitmap->Canvas->Draw(0, 0, picture->Graphic);
			FBitmap = std::move(bitmap);
		}
	}
	catch (Exception &)
	{
		FBitmap.reset();
	}
	Fit();
}
//---------------------------------------------------------------------------
void TfmPhotoView::Fit()
{
	FZoom = 0.0;
	FPanX = FPanY = 0.0;
	FView->Invalidate();
}
//---------------------------------------------------------------------------
// По размеру окна; маленькое фото не увеличивается.
double TfmPhotoView::FitScale() const
{
	if (!FBitmap || FBitmap->Width == 0 || FBitmap->Height == 0)
		return 1.0;
	return std::min(1.0, std::min((double)FView->Width / FBitmap->Width, (double)FView->Height / FBitmap->Height));
}
//---------------------------------------------------------------------------
double TfmPhotoView::Scale() const
{
	return FZoom > 0.0 ? FZoom : FitScale();
}
//---------------------------------------------------------------------------
TRect TfmPhotoView::ImageRect() const
{
	const double s = Scale();
	const int w = (int)(FBitmap->Width * s + 0.5), h = (int)(FBitmap->Height * s + 0.5);
	const int left = (int)((FView->Width - w) / 2.0 + FPanX), top = (int)((FView->Height - h) / 2.0 + FPanY);
	return TRect(left, top, left + w, top + h);
}
//---------------------------------------------------------------------------
void __fastcall TfmPhotoView::ViewPaint(TObject *Sender)
{
	TCanvas *canvas = FView->Canvas;
	canvas->Brush->Color = clAppWorkSpace;
	canvas->FillRect(FView->ClientRect);
	if (!FBitmap)
	{
		canvas->Font->Color = clWhite;
		const String text = Tr(L"Main.Details.NoPhoto");
		canvas->TextOut((FView->Width - canvas->TextWidth(text)) / 2, (FView->Height - canvas->TextHeight(text)) / 2, text);
		return;
	}
	SetStretchBltMode(canvas->Handle, HALFTONE);
	SetBrushOrgEx(canvas->Handle, 0, 0, nullptr);
	canvas->StretchDraw(ImageRect(), FBitmap.get());
}
//---------------------------------------------------------------------------
void __fastcall TfmPhotoView::SideClick(TObject *Sender)
{
	ShowSide((TPhotoSide)static_cast<TComponent *>(Sender)->Tag);
}
//---------------------------------------------------------------------------
void __fastcall TfmPhotoView::FitClick(TObject *Sender)
{
	Fit();
}
//---------------------------------------------------------------------------
// Колесо: масштаб относительно точки под курсором; меньше «по размеру окна» — не уменьшается.
void __fastcall TfmPhotoView::FormMouseWheel(TObject *Sender, TShiftState Shift, int WheelDelta,
	const TPoint &MousePos, bool &Handled)
{
	Handled = true;
	if (!FBitmap)
		return;
	const TPoint p = FView->ScreenToClient(MousePos);
	const TRect r = ImageRect();
	const double s = Scale();
	const double ns = std::min(cMaxZoom, WheelDelta > 0 ? s * cZoomStep : s / cZoomStep);
	if (ns <= FitScale())
	{
		Fit();
		return;
	}
	// Точка изображения под курсором остаётся под курсором.
	const double ix = (p.X - r.Left) / s, iy = (p.Y - r.Top) / s;
	FZoom = ns;
	FPanX = p.X - ix * ns - (FView->Width - FBitmap->Width * ns) / 2.0;
	FPanY = p.Y - iy * ns - (FView->Height - FBitmap->Height * ns) / 2.0;
	FView->Invalidate();
}
//---------------------------------------------------------------------------
void __fastcall TfmPhotoView::ViewMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift, int X, int Y)
{
	if (Button != mbLeft || FZoom <= 0.0)
		return;
	FDragging  = true;
	FDragStart = TPoint(X, Y);
	FDragPanX  = FPanX;
	FDragPanY  = FPanY;
	FView->Cursor = crSizeAll;
}
//---------------------------------------------------------------------------
void __fastcall TfmPhotoView::ViewMouseMove(TObject *Sender, TShiftState Shift, int X, int Y)
{
	if (!FDragging)
		return;
	FPanX = FDragPanX + (X - FDragStart.X);
	FPanY = FDragPanY + (Y - FDragStart.Y);
	FView->Invalidate();
}
//---------------------------------------------------------------------------
void __fastcall TfmPhotoView::ViewMouseUp(TObject *Sender, TMouseButton Button, TShiftState Shift, int X, int Y)
{
	FDragging = false;
	FView->Cursor = crDefault;
}
//---------------------------------------------------------------------------
// Esc — закрыть; ← / → / пробел — другая сторона; 0 / Home — по размеру окна.
void __fastcall TfmPhotoView::FormKeyDown(TObject *Sender, WORD &Key, TShiftState Shift)
{
	if (Key == VK_ESCAPE)
		Close();
	else if (Key == VK_LEFT || Key == VK_RIGHT || Key == VK_SPACE)
	{
		const TPhotoSide other = (TPhotoSide)(1 - FSide);
		if (!FFiles[other].IsEmpty())
			ShowSide(other);
	}
	else if (Key == '0' || Key == VK_HOME)
		Fit();
	else
		return;
	Key = 0;
}
//---------------------------------------------------------------------------
void __fastcall TfmPhotoView::FormResize(TObject *Sender)
{
	if (FView)
		FView->Invalidate();
}
//---------------------------------------------------------------------------
