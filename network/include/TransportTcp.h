#ifndef TRANSPORTTCP_H
#define TRANSPORTTCP_H

#include "TransportBase.h"
#include "SocketBase.h"
#include "MessageBlock.h"
#include "UtilityT.h"

#ifdef CM_DEBUG_SEND_RECV
#include <vector>
#endif // CM_DEBUG_SEND_RECV

class  CTransportTcp
	: public CTransportBase 
{
public:
	CTransportTcp(CReactor *pReactor);
	virtual ~CTransportTcp();

	CSocketTcp& GetPeer();

	virtual CM_HANDLE GetHandle() const ;
	virtual int OnInput(CM_HANDLE aFd = CM_INVALID_HANDLE);
	virtual int OnOutput(CM_HANDLE aFd = CM_INVALID_HANDLE);

	virtual int SendData(CDataBlock	&aData);
	virtual int IOCtl(int aCmd, LPVOID aArg);

#ifdef EMBEDED_LINUX
	virtual int OnTimeout(const CTimeValue &aCurTime, LPVOID aArg);
#endif

protected:
	virtual int Open_t();
	virtual int Close_t(int aReason);

	virtual int Recv_t(LPSTR aBuf, DWORD aLen);
	virtual int Send_t(LPCSTR aBuf, DWORD aLen);

	CMessageBlock m_mbBufferOne;
private:
	static CCmBufferWapperChar s_bwRecvMax;

	CSocketTcp m_SockTcp;

	bool m_bCanRecv;	//允许通过控制不接收来控制发送速度

#ifdef CM_DEBUG_SEND_RECV	
	typedef std::pair<char, int> RecEleType;
	typedef std::vector<RecEleType> RecType;
	RecType m_RecordDebug;
#endif // CM_DEBUG_SEND_RECV
};
#endif // !TRANSPORTTCP_H
