//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "u_ImportSource.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------

TXlsxSource::TXlsxSource()
{
}
//---------------------------------------------------------------------------
TXlsxSource::~TXlsxSource()
{
}
//---------------------------------------------------------------------------
void TXlsxSource::Open(const String &AFileName)
{
	// TODO (задача «Импорт»): открыть .xlsx как zip, разобрать
	// xl/worksheets/sheet1.xml + xl/sharedStrings.xml, заполнить FRows.
	(void)AFileName;
	FRows.clear();
}
//---------------------------------------------------------------------------
void TXlsxSource::Close()
{
	FRows.clear();
}
//---------------------------------------------------------------------------
int TXlsxSource::RowCount() const
{
	return static_cast<int>(FRows.size());
}
//---------------------------------------------------------------------------
std::vector<String> TXlsxSource::Row(int AIndex) const
{
	if (AIndex < 0 || AIndex >= static_cast<int>(FRows.size()))
		return std::vector<String>();

	return FRows[AIndex];
}
//---------------------------------------------------------------------------
