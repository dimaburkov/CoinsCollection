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
	// CoinId == 0 — найти такую же монету или создать; иначе обновить монету
	// (изменения видны у всех её экземпляров). Id == 0 — новый экземпляр.
	// Заполняет CoinId и Id; возвращает Id.
	int  Save(TCoinRecord &ARecord);

	// Удалить экземпляр; монету без экземпляров — тоже. Справочники не трогает.
	// true, если экземпляр существовал.
	bool Delete(int AId);

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
