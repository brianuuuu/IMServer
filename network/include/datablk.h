#ifndef __DATA_BLOCK_H
#define __DATA_BLOCK_H

#include <list>
//#include "utilbase.h"

using namespace std;

class CMsgPool
{
private:
	list<char*> m_lS128Bufs;
	list<char*> m_lS512Bufs;
	list<char*> m_lS1600Bufs;
	list<char*> m_lS65736Bufs;
public:
	CMsgPool(int n128BufNum, int n512BufNum, int n1600BufNum, int n64KbufNum);
	virtual ~CMsgPool();
	BYTE	*MsgAllocate(DWORD dwLen);
	void	MsgFree(BYTE *pBuf);
};

class  CDataBlock
{
public:
	CDataBlock(DWORD dwDataLen, DWORD dwHeadLen);
	~CDataBlock();
	
	void	AddRef(void);
	void	Release(void);

	BYTE	*GetBuf(void) const ;
	DWORD	GetLen(void) const ;

	//Return value: -1 Failed, 0 success
	int		Advance(DWORD dwLen);
	int		Expand(DWORD dwLen);
	int		Back(DWORD dwLen);

	void	SetOrgToCur(void);
	void	SetCurToOrg(void);
public:
	static int	GetDataBlkNum();
private:
	void	Init(DWORD dwDataLen, DWORD dwHeadLen);

	DWORD			m_dwOrg;
	DWORD			m_dwCur;
	DWORD			m_dwOrgLen;
	DWORD			m_dwCurLen;
	DWORD			m_dwRef;
	DWORD			m_dwBufLen;
	unsigned char	*m_pBuf;
	CDataBlock		*m_pNext;
private:
	static list<CDataBlock*> m_pDataList;
};

#endif //__DATA_BLOCK_H

