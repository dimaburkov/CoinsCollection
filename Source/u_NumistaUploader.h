//---------------------------------------------------------------------------
// Выгрузка экземпляров в свою коллекцию Numista (задача 13).
// Документация: https://en.numista.com/api/doc/index.php
//   addCollectedItems — POST /users/{user_id}/collected_items
//   getCollectedItems — GET  /users/{user_id}/collected_items (проверка дублей)
//   Экземпляр обмена (задача 47a) — for_swap = true, заметка для партнёров — public_comment.
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
	// Такая монета уже есть в коллекции, добавленная не программой (на сайте): тот же
	// тип и (если у обоих известен выпуск) тот же выпуск — сначала с тем же выпуском.
	// Возвращает id предмета Numista и помечает его занятым (не отдаётся второму
	// экземпляру); 0 — нет.
	// Обмен (задача 47a): предмет с той же отметкой for_swap, что у экземпляра.
	int  TakeForeign(int ATypeId, int AIssueId, bool AForSwap);

	// Тело POST addCollectedItems для экземпляра.
	static String ItemJson(const TCoinRecord &AItem);
	// Наша шкала состояний → grade Numista (g … unc); "" — без оценки.
	static String Grade(const String &ACondition);
	// Комментарий экземпляра обмена (задача 47a): части через "; ", помеченные «приватно:» / «private:»
	// (импорт uCoin, Information (Private)) — в APrivate без пометки, остальное — в APublic.
	static void SplitSwapComment(const String &AComment, String &APublic, String &APrivate);

private:
	std::map<int, int> FUploaded;                    // Id экземпляра -> id предмета Numista
	struct TForeignItem
	{
		int  Id;        // id предмета Numista
		int  TypeId;
		int  IssueId;   // 0 — не указан
		bool ForSwap;   // на обмен (задача 47a)
		bool Taken;     // уже связан с экземпляром
	};
	std::vector<TForeignItem> FForeign;              // предметы, добавленные не программой
};
//---------------------------------------------------------------------------
#endif
