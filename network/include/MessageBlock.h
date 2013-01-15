
#ifndef MESSAGEBLOCK_H
#define MESSAGEBLOCK_H

class CDataBlock;

class  CMessageBlock  
{
public:
	CMessageBlock(DWORD aSize = 0);
	~CMessageBlock();

	LPCSTR GetReadPtr() const ;
	int AdvanceReadPtr(DWORD aStep);
	LPSTR GetWritePtr() const ;
	int AdvanceWritePtr(DWORD aStep);
	
	DWORD GetLength() const ;
	DWORD GetSpace() const ;

	void Resize(DWORD aSize);
	void ResizeFromDataBlock(const CDataBlock &aDb);

private:
	char *m_strData;
	LPCSTR m_pReadPtr;
	LPSTR m_pWritePtr;
	LPCSTR m_pBeginPtr;
	LPCSTR m_pEndPrt;
};

#endif // !MESSAGEBLOCK_H

