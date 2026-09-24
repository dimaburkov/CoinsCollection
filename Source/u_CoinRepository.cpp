//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "u_CoinRepository.h"
#include "u_Database.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------

TCoinRepository::TCoinRepository(TDatabase *ADatabase)
	: FDatabase(ADatabase)
{
}
//---------------------------------------------------------------------------
TCoinRepository::~TCoinRepository()
{
}
//---------------------------------------------------------------------------
std::vector<TCoinRecord> TCoinRepository::LoadAll()
{
	// TODO задача 3a: coin_items JOIN coins, periods, countries, currencies, conditions.
	return std::vector<TCoinRecord>();
}
//---------------------------------------------------------------------------
int TCoinRepository::Save(TCoinRecord &ARecord)
{
	// TODO задача 3a: справочники -> coins -> coin_items в одной транзакции.
	return ARecord.Id;
}
//---------------------------------------------------------------------------
bool TCoinRepository::Delete(int AId)
{
	// TODO задача 3a: удалить coin_items (и coins без экземпляров).
	(void)AId;
	return false;
}
//---------------------------------------------------------------------------
