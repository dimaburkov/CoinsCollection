//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "u_CoinImporter.h"
#include "u_ImportSource.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------

TCoinImporter::TCoinImporter()
{
}
//---------------------------------------------------------------------------
TCoinImporter::~TCoinImporter()
{
}
//---------------------------------------------------------------------------
std::vector<TCoinRecord> TCoinImporter::Parse(TXlsxSource &ASource)
{
	// TODO (задача «Импорт»): по строке заголовка определить индексы колонок,
	// затем каждую строку данных превратить в TCoinRecord; ошибки -> FErrors.
	(void)ASource;
	FErrors.clear();
	return std::vector<TCoinRecord>();
}
//---------------------------------------------------------------------------
const std::vector<String> &TCoinImporter::Errors() const
{
	return FErrors;
}
//---------------------------------------------------------------------------
