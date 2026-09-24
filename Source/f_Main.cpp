//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "f_Main.h"
#include "dm_Data.h"
#include "u_AppConfig.h"
#include "u_Translator.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TfmMain *fmMain;
//---------------------------------------------------------------------------
namespace
{
	int RowCount(TFDConnection *AConnection, const String &ATable)
	{
		// Имя таблицы — константа из кода или из схемы БД, не ввод пользователя.
		return AConnection->ExecSQLScalar(L"SELECT COUNT(*) FROM \"" + ATable + L"\"");
	}
	//-----------------------------------------------------------------------
	const TCoinRecord *FindById(const std::vector<TCoinRecord> &ARecords, int AId)
	{
		for (size_t i = 0; i < ARecords.size(); ++i)
			if (ARecords[i].Id == AId)
				return &ARecords[i];
		return nullptr;
	}
	//-----------------------------------------------------------------------
	// Накопитель результатов проверок self-test.
	struct TChecks
	{
		String Text;
		int    Failed;
		TChecks() : Failed(0) {}
		void Check(bool AOk, const String &AWhat)
		{
			Text += (AOk ? L"[OK]   " : L"[FAIL] ") + AWhat + L"\n";
			if (!AOk)
				++Failed;
		}
	};
}
//---------------------------------------------------------------------------
__fastcall TfmMain::TfmMain(TComponent* Owner)
	: TForm(Owner)
{
	Translator().TranslateForm(this);
	BuildLanguageMenu();
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::FormCreate(TObject *Sender)
{
	FDatabase.reset(new TDatabase(dmData->FDConnection));
	FDatabase->Open(AppConfig().DbPath);
	FDatabase->EnsureSchema();

	FRepository.reset(new TCoinRepository(FDatabase.get()));
}
//---------------------------------------------------------------------------
// Подпункты Settings -> Language: по одному на найденный Lang\*.ini.
// Названия языков пишутся на самом языке и не переводятся.
void TfmMain::BuildLanguageMenu()
{
	miLanguage->Clear();
	const std::vector<TLanguageInfo> &languages = Translator().Languages();
	for (size_t i = 0; i < languages.size(); ++i)
	{
		TMenuItem *item = new TMenuItem(miLanguage);
		item->Caption    = languages[i].Name;
		item->Tag        = (NativeInt)i;
		item->RadioItem  = true;
		item->GroupIndex = 1;
		item->Checked    = SameText(languages[i].Code, Translator().Language());
		item->OnClick    = LanguageClick;
		miLanguage->Add(item);
	}
	miLanguage->Enabled = !languages.empty();
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::LanguageClick(TObject *Sender)
{
	TMenuItem *item = static_cast<TMenuItem *>(Sender);
	const std::vector<TLanguageInfo> &languages = Translator().Languages();
	const size_t index = (size_t)item->Tag;
	if (index >= languages.size())
		return;

	Translator().SetLanguage(languages[index].Code);
	Translator().TranslateAll();
	item->Checked = true;

	AppConfig().Language = Translator().Language();
	AppConfig().Save();
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::FormDestroy(TObject *Sender)
{
	FRepository.reset();
	if (FDatabase)
		FDatabase->Close();
	FDatabase.reset();
}
//---------------------------------------------------------------------------
// Временная проверка БД (убрать в задаче с реальным UI).
void __fastcall TfmMain::btnSelfTestClick(TObject *Sender)
{
	if (!FDatabase || !FDatabase->IsOpen() || !FRepository)
	{
		ShowMessage(Tr(L"Main.DbNotOpen"));
		return;
	}
	ShowMessage(SchemaReport() + L"\n" + CrudReport());
}
//---------------------------------------------------------------------------
// Отчёт self-test: заголовки переводятся, описания проверок (временные) — нет.
String TfmMain::SchemaReport()
{
	TFDConnection *conn = FDatabase->Connection();
	String report = Tr(L"Main.SelfTest.Database") + L": " + AppConfig().DbPath
		+ L"\n" + Tr(L"Main.SelfTest.SchemaVersion") + L": " + IntToStr(FDatabase->SchemaVersion())
		+ L"\n" + Tr(L"Main.SelfTest.ForeignKeys") + L": " + VarToStr(conn->ExecSQLScalar(L"PRAGMA foreign_keys"))
		+ L"\n\n" + Tr(L"Main.SelfTest.Tables") + L":";

	std::unique_ptr<TStringList> tables(new TStringList());
	conn->GetTableNames(L"", L"", L"", tables.get(), TFDPhysObjectScopes() << osMy,
		TFDPhysTableKinds() << tkTable, false);
	for (int i = 0; i < tables->Count; ++i)
		report += L"\n  " + tables->Strings[i] + L": " + IntToStr(RowCount(conn, tables->Strings[i]));
	return report + L"\n";
}
//---------------------------------------------------------------------------
// CRUD через TCoinRepository на тестовых записях; после проверки всё удаляется.
String TfmMain::CrudReport()
{
	TFDConnection *conn = FDatabase->Connection();
	const wchar_t *const tables[] = { L"coins", L"coin_items", L"countries", L"periods", L"currencies" };
	const int tableCount = sizeof(tables) / sizeof(tables[0]);
	int before[tableCount];
	for (int i = 0; i < tableCount; ++i)
		before[i] = RowCount(conn, tables[i]);
	// Прирост строк в таблице относительно начала теста.
	auto delta = [&](int AIndex) { return RowCount(conn, tables[AIndex]) - before[AIndex]; };

	const String testPeriod   = L"(self-test period)";
	const String testCurrency = L"(self-test currency)";
	TChecks checks;
	std::vector<int> created;

	try
	{
		// A и B — два экземпляра одной монеты, C — другая монета той же страны.
		TCoinRecord a;
		a.Country       = L"Ukraine";		// есть в справочнике стран
		a.Period        = testPeriod;
		a.Currency      = testCurrency;
		a.Denomination  = L"1 hryvnia";
		a.Year          = 2001;
		a.DiameterMm    = 18.9;
		a.CatalogNumber = L"KM# 999";
		a.Condition     = L"VF";
		a.CatalogValueUah = 12.5;
		a.PurchaseDate  = EncodeDate(2024, 5, 17);
		a.Comment       = L"Тест";	// «Тест» — проверка Unicode

		TCoinRecord b = a;
		b.Condition = L"unc";				// регистр кода не важен
		b.Quantity  = 2;
		b.PurchaseDate = 0;
		b.Comment   = L"";

		TCoinRecord c = a;
		c.Denomination  = L"2 hryvni";
		c.Year          = 0;				// год не указан -> NULL
		c.DiameterMm    = 0;
		c.NeedToReplace = true;
		c.PublishedDate = EncodeDate(2023, 1, 2);

		FRepository->Save(a); created.push_back(a.Id);
		FRepository->Save(b); created.push_back(b.Id);
		FRepository->Save(c); created.push_back(c.Id);

		checks.Check(a.Id > 0 && b.Id > 0 && c.Id > 0, L"Save returns ids");
		checks.Check(a.CoinId == b.CoinId, L"two items of one coin share a coins row");
		checks.Check(c.CoinId != a.CoinId, L"another coin gets its own coins row");
		checks.Check(delta(0) == 2 && delta(1) == 3, L"coins +2, coin_items +3");
		checks.Check(delta(2) == 0, L"country taken from reference (countries +0)");
		checks.Check(delta(3) == 1 && delta(4) == 1, L"periods +1, currencies +1 (no duplicates)");

		const std::vector<TCoinRecord> all = FRepository->LoadAll();
		const TCoinRecord *la = FindById(all, a.Id);
		const TCoinRecord *lb = FindById(all, b.Id);
		const TCoinRecord *lc = FindById(all, c.Id);
		checks.Check(la && lb && lc, L"LoadAll returns all 3 items");
		if (la && lb && lc)
		{
			checks.Check(la->Country == L"Ukraine" && la->Period == testPeriod && la->Currency == testCurrency,
				L"lookups joined back (country, period, currency)");
			checks.Check(la->Year == 2001 && la->DiameterMm == 18.9 && la->CatalogNumber == L"KM# 999"
				&& la->CatalogValueUah == 12.5, L"coin fields and values read back");
			checks.Check(la->Condition == L"VF" && lb->Condition == L"UNC" && lb->Quantity == 2,
				L"condition codes and quantity read back");
			checks.Check(la->PurchaseDate == EncodeDate(2024, 5, 17) && (double)lb->PurchaseDate == 0.0
				&& lc->PublishedDate == EncodeDate(2023, 1, 2), L"dates and empty dates read back");
			checks.Check(lc->Year == 0 && lc->DiameterMm == 0.0 && lc->NeedToReplace,
				L"NULL year / diameter and need-to-replace read back");
			checks.Check(la->Comment == a.Comment && lb->Comment.IsEmpty(), L"Unicode and empty text read back");
		}

		// Неизвестный код состояния — транзакция откатывается целиком:
		// новая страна и её период создаются до проверки кода и должны исчезнуть.
		TCoinRecord bad = a;
		bad.Id = 0; bad.CoinId = 0;
		bad.Country = L"(self-test country)";
		bad.Denomination = L"999 test";
		bad.Condition = L"XYZ";
		bool rejected = false;
		try { FRepository->Save(bad); } catch (Exception &) { rejected = true; }
		checks.Check(rejected && bad.Id == 0 && delta(0) == 2 && delta(1) == 3
			&& delta(2) == 0 && delta(3) == 1, L"unknown condition is rejected, whole transaction rolled back");

		// Изменение экземпляра.
		b.Quantity = 3;
		FRepository->Save(b);
		const std::vector<TCoinRecord> all2 = FRepository->LoadAll();
		const TCoinRecord *lb2 = FindById(all2, b.Id);
		checks.Check(lb2 && lb2->Quantity == 3 && delta(1) == 3, L"update of an item");

		// Удаление: монета удаляется вместе с последним экземпляром.
		checks.Check(FRepository->Delete(a.Id) && delta(0) == 2, L"delete one of two items keeps the coin");
		checks.Check(FRepository->Delete(b.Id) && delta(0) == 1, L"delete last item removes the coin");
		checks.Check(FRepository->Delete(c.Id) && delta(0) == 0 && delta(1) == 0, L"coins and coin_items back to start");
		checks.Check(!FRepository->Delete(a.Id), L"delete of a missing item returns false");
		checks.Check(delta(3) == 1 && delta(4) == 1, L"reference rows are kept after delete");
		created.clear();
	}
	catch (Exception &e)
	{
		checks.Check(false, L"exception: " + e.Message);
	}

	// Уборка: удалить оставшиеся тестовые экземпляры и тестовые записи справочников.
	for (size_t i = 0; i < created.size(); ++i)
	{
		try { FRepository->Delete(created[i]); } catch (Exception &) {}
	}
	try
	{
		std::unique_ptr<TFDQuery> q(new TFDQuery(nullptr));
		q->Connection = conn;
		q->SQL->Text = L"DELETE FROM periods WHERE name = :name"
		               L" AND NOT EXISTS (SELECT 1 FROM coins WHERE id_period = periods.id)";
		q->ParamByName(L"name")->AsWideString = testPeriod;
		q->ExecSQL();
		q->SQL->Text = L"DELETE FROM currencies WHERE name = :name"
		               L" AND NOT EXISTS (SELECT 1 FROM coins WHERE id_currency = currencies.id)";
		q->ParamByName(L"name")->AsWideString = testCurrency;
		q->ExecSQL();
	}
	catch (Exception &) {}

	bool clean = true;
	for (int i = 0; i < tableCount; ++i)
		clean = clean && delta(i) == 0;
	checks.Check(clean, L"test data cleaned up");

	return Format(Tr(L"Main.SelfTest.Crud"),
		ARRAYOFCONST((Tr(checks.Failed == 0 ? L"Main.SelfTest.Ok" : L"Main.SelfTest.Failed"))))
		+ L"\n\n" + checks.Text;
}
//---------------------------------------------------------------------------
