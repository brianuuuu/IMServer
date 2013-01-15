
#include "CmBase.h"
#include "MessageBlock.h"
#include "datablk.h"

CMessageBlock::CMessageBlock(DWORD aSize)
	: m_pReadPtr(NULL)
	, m_pWritePtr(NULL)
	, m_pBeginPtr(NULL)
	, m_pEndPrt(NULL)
{
	m_strData = NULL;
	if (aSize == 0)
		aSize = 256;
	Resize(aSize);
}

CMessageBlock::~CMessageBlock()
{
	if (m_strData) {
		delete [] m_strData;
	}

}

void CMessageBlock::Resize(DWORD aSize)
{
	if (m_strData)
		delete [] m_strData;
	m_strData = new char[aSize];

	m_pBeginPtr = m_strData;
	m_pReadPtr = m_pBeginPtr;
	m_pWritePtr = const_cast<LPSTR>(m_pBeginPtr);
	m_pEndPrt = m_pBeginPtr + aSize;
}

LPCSTR CMessageBlock::GetReadPtr() const 
{
	return m_pReadPtr;
}

int CMessageBlock::AdvanceReadPtr(DWORD aStep)
{
	CM_ASSERTE_RETURN(m_pWritePtr >= m_pReadPtr + aStep, -1);
	m_pReadPtr += aStep;
	return 0;
}

LPSTR CMessageBlock::GetWritePtr() const 
{
	return m_pWritePtr;
}

int CMessageBlock::AdvanceWritePtr(DWORD aStep)
{
	CM_ASSERTE_RETURN(m_pWritePtr + aStep <= m_pEndPrt, -1);
	m_pWritePtr += aStep;
	return 0;
}

DWORD CMessageBlock::GetLength() const
{
	CM_ASSERTE(m_pWritePtr >= m_pReadPtr);
	return m_pWritePtr - m_pReadPtr;
}

DWORD CMessageBlock::GetSpace() const 
{
	CM_ASSERTE(m_pEndPrt >= m_pWritePtr);
	return m_pEndPrt - m_pWritePtr;
}

void CMessageBlock::ResizeFromDataBlock(const CDataBlock &aDb)
{
	Resize(aDb.GetLen());
	::memcpy(m_pWritePtr, aDb.GetBuf(), aDb.GetLen());
	AdvanceWritePtr(aDb.GetLen());
}
