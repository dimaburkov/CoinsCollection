//---------------------------------------------------------------------------
// Преобразование строк экспорта uCoin.net (.xlsx) в TCoinRecord:
// колонки ищутся по тексту заголовка, числа — с точкой, даты — числа Excel.
// Строка с ошибкой пропускается целиком и попадает в Errors().
//---------------------------------------------------------------------------
#ifndef u_CoinImporterH
#define u_CoinImporterH
//---------------------------------------------------------------------------
#include <System.hpp>
#include <map>
#include <set>
#include <vector>
#include "u_CoinTypes.h"
//---------------------------------------------------------------------------
class TXlsxSource;
//---------------------------------------------------------------------------
struct TImportError
{
	int    Row;       // номер строки в листе Excel
	String Message;
};
//---------------------------------------------------------------------------
class TCoinImporter
{
public:
	// AConditions — шкала состояний из БД (допустимые коды).
	explicit TCoinImporter(const std::vector<TCondition> &AConditions);
	~TCoinImporter();

	// Разобрать все строки источника (первая — заголовки).
	// Нет обязательной колонки — исключение (файл отклоняется).
	std::vector<TCoinRecord> Parse(TXlsxSource &ASource);

	// Ошибки последнего вызова Parse() (строки, которые пропущены).
	const std::vector<TImportError> &Errors() const { return FErrors; }
	// Число строк данных (без заголовка и полностью пустых) последнего Parse().
	int DataRowCount() const { return FDataRows; }

private:
	std::set<String>          FConditionCodes;
	std::map<String, int>     FColumns;      // заголовок (в верхнем регистре) -> индекс колонки
	std::vector<TImportError> FErrors;
	int                       FDataRows;

	int    Column(const String &AHeader) const;
	String Cell(const std::vector<String> &ARow, const String &AHeader) const;
	void   ParseRow(const std::vector<String> &ARow, TCoinRecord &ARecord) const;
};
//---------------------------------------------------------------------------
#endif
