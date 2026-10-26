//---------------------------------------------------------------------------
// Коллекция в БД: «плоская» запись TCoinRecord <-> справочники + coins
// (монета-тип) + coin_items (экземпляр). Все запросы параметризованные.
//---------------------------------------------------------------------------
#ifndef u_CoinRepositoryH
#define u_CoinRepositoryH
//---------------------------------------------------------------------------
#include <vector>
#include "u_CoinTypes.h"
#include "u_LookupRepository.h"
//---------------------------------------------------------------------------
class TDatabase;
//---------------------------------------------------------------------------
// Итог переноса связей при импорте (ReplaceAll): Numista (задача 13b) и фото (задача 14).
struct TImportKept
{
	int Coins;        // монет с N# / индексом перенесено
	int Items;        // выгруженных экземпляров перенесено
	int CoinsLost;    // монет нет в новом файле
	int ItemsLost;    // экземпляров нет или сопоставление неоднозначно
	int Photos;       // фото перенесено
	std::vector<String> LostPhotos;	// файлы фото без пары (строки удалены, файлы — на месте)
	TImportKept() : Coins(0), Items(0), CoinsLost(0), ItemsLost(0), Photos(0) {}
};
//---------------------------------------------------------------------------
class TCoinRepository
{
public:
	explicit TCoinRepository(TDatabase *ADatabase);
	~TCoinRepository();

	// Все экземпляры коллекции (сортировка: страна, номинал, год).
	std::vector<TCoinRecord> LoadAll();

	// Сохранить экземпляр в одной транзакции: справочники -> coins -> coin_items.
	// CoinId != 0 — правка: поля монеты обновляются (видно у всех её экземпляров).
	// CoinId == 0 — найти такую же монету (период, валюта, номинал, год,
	// разновидность, сюжет, номер по каталогу): найдена — её поля не меняются
	// (пустой диаметр дополняется), не найдена — создать. Id == 0 — новый экземпляр.
	// Заполняет CoinId и Id; возвращает Id.
	int  Save(TCoinRecord &ARecord);

	// Удалить экземпляр; монету без экземпляров — тоже. Справочники не трогает.
	// Фото удалённых экземпляра и монеты удаляются (строки — каскадом, файлы — после фиксации).
	// true, если экземпляр существовал.
	bool Delete(int AId);

	// Заменить всю коллекцию (импорт): одна транзакция — удалить все экземпляры
	// и монеты, затем сохранить ARecords. Ошибка — откат, коллекция не меняется.
	// Справочники не очищаются. Возвращает число сохранённых экземпляров.
	// Связи Numista переносятся на новые монеты с тем же ключом (как в FindCoin):
	// N#, выпуск, индекс редкости; numista_item_id — на экземпляр с теми же полями
	// (состояние, количество, My value, дата покупки, комментарий, Swap) или на
	// единственный экземпляр монеты. Так же переносятся фото типа и экземпляров (задача 14);
	// фото без пары — в AKept->LostPhotos. AKept — итог переноса (может быть nullptr).
	int  ReplaceAll(const std::vector<TCoinRecord> &ARecords, TImportKept *AKept = nullptr);

	// Число экземпляров в коллекции.
	int  ItemCount();
	// Число экземпляров монеты ACoinId.
	int  ItemCount(int ACoinId);

	// Связь монеты с типом и выпуском Numista (0 — нет).
	void SetCoinNumista(int ACoinId, int ATypeId, int AIssueId);
	// id предмета в коллекции Numista у экземпляра (0 — не выгружен).
	void SetItemNumista(int AItemId, int ANumistaItemId);
	// Индекс редкости монеты (-1 — не известен).
	void SetCoinRarity(int ACoinId, int ARarity);
	// Выпуски типа Numista (getIssues) в numista_issues — вставка или обновление
	// по id (задача 13c). Возвращает число выпусков.
	int  SaveNumistaIssues(int ATypeId, const std::vector<TNumistaIssue> &AIssues);
	// Характеристики типа Numista (getType) в numista_types (задача 13d).
	void SaveNumistaType(const TNumistaTypeInfo &AType);
	// false — тип не загружен.
	bool LoadNumistaType(int ATypeId, TNumistaTypeInfo &AType);

	TLookupRepository &Lookups() { return FLookups; }

private:
	TDatabase         *FDatabase;   // не владеет
	TLookupRepository  FLookups;

	int  FindCoin(int APeriodId, int ACurrencyId, const TCoinRecord &ARecord);
	void WriteCoin(int APeriodId, int ACurrencyId, TCoinRecord &ARecord);
	void WriteItem(int AConditionId, TCoinRecord &ARecord);
};
//---------------------------------------------------------------------------
#endif
