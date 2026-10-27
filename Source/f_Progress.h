//---------------------------------------------------------------------------
// Окно хода выполнения массовых операций (задача 19): «Auto-link — 5 of 20», текущая
// монета, полоса, запросы к API, кнопка Stop. Работа идёт в том же потоке: окно
// перерисовывается между элементами (Step), главное окно на это время недоступно.
// Использование — объект на время операции:
//   TProgress progress(this, caption, (int)coins.size(), &client);
//   for (...) { if (!progress.Step(i, CoinText(coin))) break; ... }
//---------------------------------------------------------------------------
#ifndef f_ProgressH
#define f_ProgressH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ComCtrls.hpp>
//---------------------------------------------------------------------------
class TNumistaClient;
//---------------------------------------------------------------------------
class TfmProgress : public TForm
{
public:
	__fastcall TfmProgress(TComponent *AOwner);

	TLabel       *Step;
	TLabel       *Item;
	TProgressBar *Bar;
	TLabel       *Requests;
	TButton      *Stop;
	bool          Stopped;

private:
	void __fastcall StopClick(TObject *Sender);
	void __fastcall FormCloseQuery(TObject *Sender, bool &CanClose);
};
//---------------------------------------------------------------------------
class TProgress
{
public:
	// ATotal — число элементов; 0 — без счёта (бегущая полоса, текст — SetText).
	// До 3 элементов окно не показывается (без мелькания). AClient — для строки запросов.
	TProgress(TForm *AOwner, const String &ACaption, int ATotal, const TNumistaClient *AClient = nullptr);
	~TProgress();

	// Перед элементом AIndex (с 0): обновить окно и обработать сообщения.
	// false — пользователь нажал Stop (элемент не обрабатывать).
	bool Step(int AIndex, const String &AItemText);
	// Текст операции без счёта («Reading the collection…»).
	void SetText(const String &AText);
	bool Stopped() const;

private:
	TForm                 *FOwner;
	TfmProgress           *FForm;	// nullptr — окно не показывается
	String                 FCaption;
	int                    FTotal;
	const TNumistaClient  *FClient;

	void Refresh();

	TProgress(const TProgress &);
	TProgress &operator=(const TProgress &);
};
//---------------------------------------------------------------------------
#endif
