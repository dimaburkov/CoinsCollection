//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <memory>
#include <System.Zip.hpp>
#include <Xml.XMLDoc.hpp>
#include <Xml.XMLIntf.hpp>
#include <Xml.Win.msxmldom.hpp>
#include <Xml.xmldom.hpp>
#include "u_ImportSource.h"
#include "u_Translator.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------

namespace
{
	// Прочитать файл из архива; false, если его нет.
	bool ReadEntry(TZipFile *AZip, const String &AName, TBytes &ABytes)
	{
		if (AZip->IndexOf(AName) < 0)
			return false;
		AZip->Read(AName, ABytes);
		return true;
	}
	//-----------------------------------------------------------------------
	_di_IXMLDocument LoadXml(const TBytes &ABytes)
	{
		std::unique_ptr<TBytesStream> stream(new TBytesStream(ABytes));
		_di_IXMLDocument doc = NewXMLDocument();
		doc->LoadFromStream(stream.get(), xetUTF_8);
		return doc;
	}
	//-----------------------------------------------------------------------
	// Локальное имя узла без префикса пространства имён.
	String LocalName(const _di_IXMLNode &ANode)
	{
		const String name = ANode->NodeName;
		const int colon = name.Pos(L":");
		return colon > 0 ? name.SubString(colon + 1, name.Length()) : name;
	}
	//-----------------------------------------------------------------------
	_di_IXMLNode Child(const _di_IXMLNode &ANode, const String &AName)
	{
		if (!ANode)
			return _di_IXMLNode();
		_di_IXMLNodeList children = ANode->ChildNodes;
		for (int i = 0; i < children->Count; ++i)
			if (LocalName(children->Nodes[i]) == AName)
				return children->Nodes[i];
		return _di_IXMLNode();
	}
	//-----------------------------------------------------------------------
	String Attr(const _di_IXMLNode &ANode, const String &AName)
	{
		// Атрибуты с префиксом (r:id) ищутся по полному имени.
		_di_IXMLNodeList attrs = ANode->AttributeNodes;
		for (int i = 0; i < attrs->Count; ++i)
			if (attrs->Nodes[i]->NodeName == AName)
				return attrs->Nodes[i]->Text;
		return String();
	}
	//-----------------------------------------------------------------------
	// Текст строки sharedStrings / inlineStr: все <t>, кроме фонетики <rPh>.
	void CollectText(const _di_IXMLNode &ANode, String &AText)
	{
		_di_IXMLNodeList children = ANode->ChildNodes;
		for (int i = 0; i < children->Count; ++i)
		{
			_di_IXMLNode child = children->Nodes[i];
			const String name = LocalName(child);
			if (name == L"t")
				AText += child->Text;
			else if (name != L"rPh" && child->NodeType == ntElement)
				CollectText(child, AText);
		}
	}
	//-----------------------------------------------------------------------
	// "F12" -> 5 (колонка A = 0).
	int ColumnIndex(const String &ARef)
	{
		int index = 0;
		for (int i = 1; i <= ARef.Length(); ++i)
		{
			const wchar_t ch = ARef[i];
			if (ch < L'A' || ch > L'Z')
				break;
			index = index * 26 + (ch - L'A' + 1);
		}
		return index - 1;
	}
	//-----------------------------------------------------------------------
	// Путь первого листа: workbook.xml (sheets/sheet r:id) + workbook.xml.rels.
	String FirstSheetPath(TZipFile *AZip)
	{
		const String fallback = L"xl/worksheets/sheet1.xml";
		TBytes bytes;
		if (!ReadEntry(AZip, L"xl/workbook.xml", bytes))
			return fallback;
		_di_IXMLNode sheet = Child(Child(LoadXml(bytes)->DocumentElement, L"sheets"), L"sheet");
		if (!sheet)
			throw Exception(Tr(L"Import.Error.NoSheet"));
		const String relId = Attr(sheet, L"r:id");

		if (relId.IsEmpty() || !ReadEntry(AZip, L"xl/_rels/workbook.xml.rels", bytes))
			return fallback;
		_di_IXMLNodeList rels = LoadXml(bytes)->DocumentElement->ChildNodes;
		for (int i = 0; i < rels->Count; ++i)
		{
			_di_IXMLNode rel = rels->Nodes[i];
			if (rel->NodeType != ntElement || Attr(rel, L"Id") != relId)
				continue;
			const String target = Attr(rel, L"Target");
			if (target.IsEmpty())
				break;
			// Target относительно xl/ или абсолютный (/xl/...).
			return target[1] == L'/' ? target.SubString(2, target.Length()) : L"xl/" + target;
		}
		return fallback;
	}
}
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
	Close();

	std::unique_ptr<TZipFile> zip(new TZipFile());
	try
	{
		zip->Open(AFileName, zmRead);
	}
	catch (Exception &)
	{
		throw Exception(Tr(L"Import.Error.NotXlsx"));
	}

	try
	{
		Load(zip.get());
	}
	catch (EZipException &)
	{
		Close();
		throw Exception(Tr(L"Import.Error.NotXlsx"));
	}
	catch (EDOMParseError &)
	{
		Close();
		throw Exception(Tr(L"Import.Error.NotXlsx"));
	}
	catch (...)
	{
		Close();
		throw;
	}
}
//---------------------------------------------------------------------------
void TXlsxSource::Load(TZipFile *AZip)
{
	const String sheetPath = FirstSheetPath(AZip);
	TBytes bytes;
	if (!ReadEntry(AZip, sheetPath, bytes))
		throw Exception(Tr(L"Import.Error.NotXlsx"));
	_di_IXMLDocument sheetDoc = LoadXml(bytes);

	// Общие строки (их может не быть, если в листе только числа).
	std::vector<String> shared;
	if (ReadEntry(AZip, L"xl/sharedStrings.xml", bytes))
	{
		_di_IXMLNodeList items = LoadXml(bytes)->DocumentElement->ChildNodes;
		shared.reserve(items->Count);
		for (int i = 0; i < items->Count; ++i)
		{
			if (LocalName(items->Nodes[i]) != L"si")
				continue;
			String text;
			CollectText(items->Nodes[i], text);
			shared.push_back(text);
		}
	}

	_di_IXMLNode sheetData = Child(sheetDoc->DocumentElement, L"sheetData");
	if (!sheetData)
		throw Exception(Tr(L"Import.Error.NoSheet"));

	_di_IXMLNodeList rows = sheetData->ChildNodes;
	FRows.reserve(rows->Count);
	FRowNumbers.reserve(rows->Count);
	for (int r = 0; r < rows->Count; ++r)
	{
		_di_IXMLNode row = rows->Nodes[r];
		if (LocalName(row) != L"row")
			continue;

		std::vector<String> cells;
		_di_IXMLNodeList cellNodes = row->ChildNodes;
		for (int c = 0; c < cellNodes->Count; ++c)
		{
			_di_IXMLNode cell = cellNodes->Nodes[c];
			if (LocalName(cell) != L"c")
				continue;

			const int col = ColumnIndex(Attr(cell, L"r"));
			if (col < 0)
				continue;
			const String type = Attr(cell, L"t");
			String value;
			if (type == L"inlineStr")
			{
				if (_di_IXMLNode is = Child(cell, L"is"))
					CollectText(is, value);
			}
			else if (_di_IXMLNode v = Child(cell, L"v"))
			{
				value = v->Text;
				if (type == L"s")
				{
					const int index = StrToIntDef(value, -1);
					value = index >= 0 && index < (int)shared.size() ? shared[index] : String();
				}
			}

			if ((int)cells.size() <= col)
				cells.resize(col + 1);
			cells[col] = value;
		}

		FRows.push_back(cells);
		FRowNumbers.push_back(StrToIntDef(Attr(row, L"r"), (int)FRowNumbers.size() + 1));
	}
}
//---------------------------------------------------------------------------
void TXlsxSource::Close()
{
	FRows.clear();
	FRowNumbers.clear();
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
int TXlsxSource::RowNumber(int AIndex) const
{
	if (AIndex < 0 || AIndex >= static_cast<int>(FRowNumbers.size()))
		return 0;
	return FRowNumbers[AIndex];
}
//---------------------------------------------------------------------------
