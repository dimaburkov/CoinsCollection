//---------------------------------------------------------------------------
// Коллекция монет в памяти. UI работает с этим классом, а не с БД напрямую.
//---------------------------------------------------------------------------
#ifndef u_CoinCollectionH
#define u_CoinCollectionH
//---------------------------------------------------------------------------
#include <vector>
#include "u_CoinTypes.h"
//---------------------------------------------------------------------------
class TCoinCollection
{
public:
	TCoinCollection();
	~TCoinCollection();

	int  Count() const;
	void Clear();

	// Добавляет запись, возвращает её индекс.
	int  Add(const TCoinRecord &ARecord);
	// Обновляет запись с тем же Id. true, если запись найдена.
	bool Update(const TCoinRecord &ARecord);
	// Удаляет запись по Id. true, если запись найдена.
	bool Remove(int AId);

	// Индекс записи по Id или -1.
	int  IndexOfId(int AId) const;

	const TCoinRecord &operator[](int AIndex) const;
	TCoinRecord       &operator[](int AIndex);

private:
	std::vector<TCoinRecord> FItems;
};
//---------------------------------------------------------------------------
#endif
