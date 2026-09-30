//---------------------------------------------------------------------------
// Табличный источник данных для импорта: первый лист файла .xlsx.
// .xlsx — zip с XML внутри; читается стоковыми System.Zip и TXMLDocument (MSXML).
//---------------------------------------------------------------------------
#ifndef u_ImportSourceH
#define u_ImportSourceH
//---------------------------------------------------------------------------
#include <System.hpp>
#include <System.Zip.hpp>
#include <vector>
//---------------------------------------------------------------------------
class TXlsxSource
{
public:
	TXlsxSource();
	~TXlsxSource();

	// Открыть файл AFileName и прочитать первый лист в память.
	// Не .xlsx / нет листов — исключение с понятным текстом.
	void Open(const String &AFileName);
	void Close();

	// Число прочитанных строк (включая строку заголовка).
	int RowCount() const;

	// Ячейки строки AIndex (0-based), по колонкам A, B, C...
	// Пустой вектор при выходе за диапазон.
	std::vector<String> Row(int AIndex) const;

	// Номер строки AIndex в листе Excel (1-based) — для сообщений об ошибках.
	int RowNumber(int AIndex) const;

private:
	std::vector<std::vector<String> > FRows;
	std::vector<int>                  FRowNumbers;

	void Load(TZipFile *AZip);
};
//---------------------------------------------------------------------------
#endif
