//---------------------------------------------------------------------------
// Окно просмотра фото монеты (задача 16): изображение по размеру окна, колесо мыши —
// масштаб (относительно курсора), перетаскивание — сдвиг, аверс / реверс, Esc — закрыть.
// Форма без .dfm — элементы создаются в конструкторе.
//---------------------------------------------------------------------------
#ifndef f_PhotoViewH
#define f_PhotoViewH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.Buttons.hpp>
#include <memory>
#include "u_PhotoRepository.h"
//---------------------------------------------------------------------------
class TfmPhotoView : public TForm
{
public:
	__fastcall TfmPhotoView(TComponent *AOwner);

	// Показать фото (AObverseFile / AReverseFile — полные пути, "" — нет), начиная с ASide.
	// ACopyright* — автор фото Numista ("" — свой снимок): строка под фото.
	static void Execute(TComponent *AOwner, const String &ATitle, const String &AObverseFile,
		const String &AReverseFile, TPhotoSide ASide, const String &AObverseCopyright = String(),
		const String &AReverseCopyright = String());

private:
	TPanel       *FToolbar;
	TSpeedButton *FObverse;
	TSpeedButton *FReverse;
	TSpeedButton *FFit;
	TLabel       *FHint;
	TPaintBox    *FView;
	TLabel       *FCopyright;

	String                   FFiles[2];	// индекс — TPhotoSide
	String                   FCopyrights[2];
	TPhotoSide               FSide;
	std::unique_ptr<TBitmap> FBitmap;	// текущее фото, декодировано один раз
	double                   FZoom;		// 0 — по размеру окна
	double                   FPanX, FPanY;	// сдвиг от центра, px
	bool                     FDragging;
	TPoint                   FDragStart;
	double                   FDragPanX, FDragPanY;

	void   ShowSide(TPhotoSide ASide);
	void   Fit();
	double FitScale() const;
	double Scale() const;
	TRect  ImageRect() const;

	void __fastcall SideClick(TObject *Sender);
	void __fastcall FitClick(TObject *Sender);
	void __fastcall ViewPaint(TObject *Sender);
	void __fastcall ViewMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift, int X, int Y);
	void __fastcall ViewMouseMove(TObject *Sender, TShiftState Shift, int X, int Y);
	void __fastcall ViewMouseUp(TObject *Sender, TMouseButton Button, TShiftState Shift, int X, int Y);
	void __fastcall FormMouseWheel(TObject *Sender, TShiftState Shift, int WheelDelta, const TPoint &MousePos, bool &Handled);
	void __fastcall FormKeyDown(TObject *Sender, WORD &Key, TShiftState Shift);
	void __fastcall FormResize(TObject *Sender);
};
//---------------------------------------------------------------------------
#endif
