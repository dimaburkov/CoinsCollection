//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "u_CoinCollection.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------

TCoinCollection::TCoinCollection()
{
}
//---------------------------------------------------------------------------
TCoinCollection::~TCoinCollection()
{
}
//---------------------------------------------------------------------------
int TCoinCollection::Count() const
{
	return static_cast<int>(FItems.size());
}
//---------------------------------------------------------------------------
void TCoinCollection::Clear()
{
	FItems.clear();
}
//---------------------------------------------------------------------------
int TCoinCollection::Add(const TCoinRecord &ARecord)
{
	FItems.push_back(ARecord);
	return static_cast<int>(FItems.size()) - 1;
}
//---------------------------------------------------------------------------
bool TCoinCollection::Update(const TCoinRecord &ARecord)
{
	int idx = IndexOfId(ARecord.Id);
	if (idx < 0)
		return false;

	FItems[idx] = ARecord;
	return true;
}
//---------------------------------------------------------------------------
bool TCoinCollection::Remove(int AId)
{
	int idx = IndexOfId(AId);
	if (idx < 0)
		return false;

	FItems.erase(FItems.begin() + idx);
	return true;
}
//---------------------------------------------------------------------------
int TCoinCollection::IndexOfId(int AId) const
{
	for (std::size_t i = 0; i < FItems.size(); ++i)
		if (FItems[i].Id == AId)
			return static_cast<int>(i);

	return -1;
}
//---------------------------------------------------------------------------
const TCoinRecord &TCoinCollection::operator[](int AIndex) const
{
	return FItems[AIndex];
}
//---------------------------------------------------------------------------
TCoinRecord &TCoinCollection::operator[](int AIndex)
{
	return FItems[AIndex];
}
//---------------------------------------------------------------------------
