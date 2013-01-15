/*------------------------------------------------------------------------*/
/*                                                                        */
/*  data block			     											  */
/*  datablk.cpp  														  */
/*                                                                        */
/*  Implementation of data block										  */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
#include "CmBase.h"
#include "datablk.h"
//#include "utilbase.h"

/*########################################################################*/
/* CMsgPool*/
CMsgPool::CMsgPool(int n128BufNum, int n512BufNum, int n1600BufNum, int n64KbufNum)
{
	return;
}

CMsgPool::~CMsgPool()
{
	return;
}

BYTE *CMsgPool::MsgAllocate(DWORD dwLen)
{
	return new BYTE[dwLen];
}

void CMsgPool::MsgFree(BYTE *pBuf)
{
	delete pBuf;
}

/*########################################################################*/
/* CDataBlock*/

list<CDataBlock*> CDataBlock::m_pDataList;

CDataBlock::CDataBlock(DWORD dwDataLen, DWORD dwHeadLen)
	: m_dwBufLen(0)
	, m_pBuf(NULL)
{
	Init(dwDataLen, dwHeadLen);
}

CDataBlock::~CDataBlock()
{
	m_pDataList.remove(this);
	if (m_pBuf != NULL)
	{
//		delete m_pBuf;
		delete [] m_pBuf;
		m_pBuf = NULL;
	}
}

void CDataBlock::Init(DWORD dwDataLen, DWORD dwHeadLen)
{
	if (m_dwBufLen != dwDataLen+dwHeadLen)
	{
		IM_ASSERT(dwDataLen + dwHeadLen > 0);
		if (m_pBuf != NULL)
		{
//			delete m_pBuf;
			delete [] m_pBuf;
		}
		m_pBuf = new BYTE[dwDataLen+dwHeadLen];
		IM_ASSERT(m_pBuf);
		m_dwBufLen = dwDataLen+dwHeadLen;
	}
	m_dwOrg = dwHeadLen;
	m_dwCur = dwHeadLen;
	m_dwOrgLen = m_dwCurLen = 0;
	m_dwRef = 1;
	m_pNext = NULL;
	m_pDataList.push_back(this);
}

void CDataBlock::AddRef(void)
{
	m_dwRef++;
}

void CDataBlock::Release(void)
{
	m_dwRef--;
	if(m_dwRef <= 0)
	{
		delete this;
	}
}

BYTE *CDataBlock::GetBuf(void) const 
{
	if(m_pBuf == NULL)
	{
		return NULL;
	}
	else
	{
		return m_pBuf + m_dwCur;
	}
}

DWORD CDataBlock::GetLen(void) const 
{
	return m_dwCurLen;
}

int CDataBlock::Advance(DWORD dwLen)
{
	if (dwLen > m_dwCurLen)
	{
		IM_ASSERT(0);
		return -1;
	}
	m_dwCurLen -= dwLen;
	m_dwCur += dwLen;
	return 0;
}

int CDataBlock::Expand(DWORD dwLen)
{
	if (dwLen+m_dwCurLen+m_dwCur > m_dwBufLen)
	{
		IM_ASSERT(0);
		return -1;
	}
	m_dwCurLen += dwLen;
	return 0;
}

int CDataBlock::Back(DWORD dwLen)
{
	if (dwLen > m_dwCur)
	{
		IM_ASSERT(0);
		return -1;
	}
	m_dwCur -= dwLen;
	m_dwCurLen += dwLen;
	return 0;
}

void CDataBlock::SetOrgToCur(void)
{
	m_dwOrg = m_dwCur;
	m_dwOrgLen = m_dwCurLen;
}

void CDataBlock::SetCurToOrg(void)
{
	m_dwCur = m_dwOrg;
	m_dwCurLen = m_dwOrgLen;
}

int CDataBlock::GetDataBlkNum()
{
	return m_pDataList.size();
}
