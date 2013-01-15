/*------------------------------------------------------*/
/* Acceptor for TCP Socket                              */
/*                                                      */
/* AcceptorTcpSocket.h                                  */
/*                                                      */
/*                                                      */
/* History                                              */
/*                                                      */
/*	11/27/2003	Create                                  */
/*                                                      */
/*------------------------------------------------------*/

#ifndef ACCEPTORTCPSOCKET_H
#define ACCEPTORTCPSOCKET_H

#include "AcceptorT.h"
#include "TransportTcp.h"
#include "SocketBase.h"

class  CAcceptorTcpSocket : public CAcceptorT<CTransportTcp, CSocketTcp>
{
public:
	CAcceptorTcpSocket(CReactor *aReactor, IAcceptorConnectorSink *aSink);
	virtual ~CAcceptorTcpSocket();

	virtual int StartListen(const CInetAddr &aAddr, DWORD aBacklog);
	virtual int StopListen(int aReason = 0);

	virtual CM_HANDLE GetHandle() const ;
	virtual int OnClose(CM_HANDLE aFd, MASK aMask);

private:
	CSocketTcp m_Socket;
};

#endif // !ACCEPTORTCPSOCKET_H

