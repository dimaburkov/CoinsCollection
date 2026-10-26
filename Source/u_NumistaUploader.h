//---------------------------------------------------------------------------
// Выгрузка экземпляров в свою коллекцию Numista (задача 13).
// Документация: https://en.numista.com/api/doc/index.php
//   addCollectedItems — POST /users/{user_id}/collected_items
//   getCollectedItems — GET  /users/{user_id}/collected_items (проверка дублей)
//---------------------------------------------------------------------------
#ifndef u_NumistaUploaderH
#define u_NumistaUploaderH
//---------------------------------------------------------------------------
#include <System.hpp>
#include <System.JSON.hpp>
#include <map>
#include <vector>
#include "u_CoinTypes.h"
//---------------------------------------------------------------------------
class TNumistaUploader
{
public:
	// Разобрать ответ getCollectedItems: предметы, выгруженные программой, и
	// остальные (добавлены не программой). Свой предмет — id = numista_item_id
	// экземпляра из AAll или internal_id = Id экземпляра того же типа Numista
	// (после импорта Id меняются, internal_id может указывать на другую монету).
	void ReadCollection(TJSONValue *AJson, const std::vector<TCoinRecord> &AAll);

	// id предмета Numista, выгруженного раньше для экземпляра AItemId; 0 — нет.
	int  UploadedItem(int AItemId) const;
	// Такая монета уже есть в коллекции, добавленная не программой: тот же тип и
	// (если у обоих известен выпуск) тот же выпуск.
	bool HasForeign(int ATypeId, int AIssueId) const;

	// Тело POST addCollectedItems для экземпляра.
	static String ItemJson(const TCoinRecord &AItem);
	// Наша шкала состояний → grade Numista (g … unc); "" — без оценки.
	static String Grade(const String &ACondition);

private:
	std::map<int, int> FUploaded;                    // Id экземпляра -> id предмета Numista
	std::multimap<int, int> FForeign;                // тип -> выпуск (0 — не указан)
};
//---------------------------------------------------------------------------
#endif
