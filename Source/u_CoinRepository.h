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
	// true, если экземпляр существовал.
	bool Delete(int AId);

	// Заменить всю коллекцию (импорт): одна транзакция — удалить все экземпляры
	// и монеты, затем сохранить ARecords. Ошибка — откат, коллекция не меняется.
	// Справочники не очищаются. Возвращает число сохранённых экземпляров.
	int  ReplaceAll(const std::vector<TCoinRecord> &ARecords);

	// Число экземпляров в коллекции.
	int  ItemCount();
	// Число экземпляров монеты ACoinId.
	int  ItemCount(int ACoinId);

	// Связь монеты с типом и выпуском Numista (0 — нет).
	void SetCoinNumista(int ACoinId, int ATypeId, int AIssueId);
	// id предмета в коллекции Numista у экземпляра (0 — не выгружен).
	void SetItemNumista(int AItemId, int ANumistaItemId);

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
