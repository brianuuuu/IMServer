/*------------------------------------------------------*/
/* wrapper UDP socket                                   */
/*                                                      */
/* ClientSocketUDP.h                                    */
/*                                                      */
/*                                                      */
/* History                                              */
/*                                                      */
/*	12/02/2003	Create                                  */
/*                                                      */
/*------------------------------------------------------*/

#ifndef CLIENTSOCKETUDP_H
#define CLIENTSOCKETUDP_H

#include "Reactor.h"
#include "SocketBase.h"
#include "UtilityT.h"

class CInetAddr;
class CDataBlock;

class  IClientSocketUDPSink
{
public:
	virtual int OnReceiveUdp(CDataBlock &aData, const CInetAddr &aAddr) = 0;
	virtual int OnCloseUdp(int aErr) = 0;

	virtual ~IClientSocketUDPSink();
};

class  CClientSocketUDP : public CEventHandlerBase
{
public:
	CClientSocketUDP(IClientSocketUDPSink *aSink);
	~CClientSocketUDP();

	int SetBuffer(int nSize);
	// succeeds return 0, fails return -1. only for server
	int Listen(const CInetAddr &aAddr, DWORD aMaxRecvLen = 4096);

	// succeeds return 0, fails return -1. only for client
	int Connect(const CInetAddr &aAddr, DWORD aMaxRecvLen = 4096);

	// succeeds return 0, fails return -1. Connect() must be invoked prior to it.
	int Send(CDataBlock &aData);

	// succeeds return 0, fails return -1.
	int SendTo(CDataBlock &aData, const CInetAddr &aAddr);

	// succeeds return 0, fails return -1.
	int Close();
	virtual CM_HANDLE GetHandle() const ;

protected:
	virtual int OnInput(CM_HANDLE aFd = CM_INVALID_HANDLE);
	virtual int OnClose(CM_HANDLE aFd, MASK aMask);

private:
	enum { CONNECT = 1 << 0, LISTEN = 1 << 1};
	static CCmBufferWapperChar s_bwRecvMax;

	CSocketUdp m_Socket;
	IClientSocketUDPSink *m_pSink;
	DWORD m_dwMaxRecvLen;
	DWORD m_dwFlag;
};

#endif // !CLIENTSOCKETUDP_H

