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
	// TODO (задача «БД»): SELECT * FROM coin, маппинг строк в TCoinRecord.
	return std::vector<TCoinRecord>();
}
//---------------------------------------------------------------------------
int TCoinRepository::Save(TCoinRecord &ARecord)
{
	// TODO (задача «БД»): INSERT при Id == 0, иначе UPDATE; вернуть Id.
	return ARecord.Id;
}
//---------------------------------------------------------------------------
bool TCoinRepository::Delete(int AId)
{
	// TODO (задача «БД»): DELETE FROM coin WHERE id = :id.
	(void)AId;
	return false;
}
//---------------------------------------------------------------------------
