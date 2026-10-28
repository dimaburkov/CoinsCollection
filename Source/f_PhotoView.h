//---------------------------------------------------------------------------
// Окно просмотра фото монеты (задача 16): изображение по размеру окна, колесо мыши —
// масштаб (относительно курсора), перетаскивание — сдвиг, аверс / реверс, Esc — закрыть.
// Два комплекта (задача 30): свои фото экземпляра и фото типа (каталог) — переключатель.
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
#include <vector>
#include "u_PhotoRepository.h"
//---------------------------------------------------------------------------
// Комплект фото для просмотра: подпись кнопки, файлы сторон (полные пути, "" — нет),
// автор фото Numista ("" — свой снимок), индекс — TPhotoSide.
struct TPhotoViewSet
{
	String Caption;
	String Files[2];
	String Copyrights[2];
	bool   Empty() const { return Files[0].IsEmpty() && Files[1].IsEmpty(); }
};
//---------------------------------------------------------------------------
class TfmPhotoView : public TForm
{
public:
	__fastcall TfmPhotoView(TComponent *AOwner);

	// Показать фото комплекта ASet стороны ASide (нет — другой стороны). Комплектов больше
	// одного — кнопки комплектов, ↑ / ↓ — другой комплект.
	static void Execute(TComponent *AOwner, const String &ATitle, const std::vector<TPhotoViewSet> &ASets,
		int ASet, TPhotoSide ASide);

private:
	TPanel       *FToolbar;
	TSpeedButton *FSetButtons[2];
	TSpeedButton *FObverse;
	TSpeedButton *FReverse;
	TSpeedButton *FFit;
	TLabel       *FHint;
	TPaintBox    *FView;
	TLabel       *FCopyright;

	std::vector<TPhotoViewSet> FSets;
	int                      FSet;
	TPhotoSide               FSide;
	std::unique_ptr<TBitmap> FBitmap;	// текущее фото, декодировано один раз
	double                   FZoom;		// 0 — по размеру окна
	double                   FPanX, FPanY;	// сдвиг от центра, px
	bool                     FDragging;
	TPoint                   FDragStart;
	double                   FDragPanX, FDragPanY;

	void   ShowPhoto(int ASet, TPhotoSide ASide);
	void   LayoutToolbar();
	void   Fit();
	double FitScale() const;
	double Scale() const;
	TRect  ImageRect() const;

	void __fastcall SideClick(TObject *Sender);
	void __fastcall SetClick(TObject *Sender);
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
