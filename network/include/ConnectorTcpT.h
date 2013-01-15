/*------------------------------------------------------*/
/* TCP Conector Template                                */
/*                                                      */
/* ConnectorTcpT.h                                      */
/*                                                      */
/*                                                      */
/* History                                              */
/*                                                      */
/*	12/10/2003	Create                                  */
/*                                                      */
/*------------------------------------------------------*/

#ifndef CONNECTORTCPT_H
#define CONNECTORTCPT_H

#include "Reactor.h"
#include "SocketBase.h"

class CTimeValue;

class  CConnectorID
{
public:
	typedef int CTYPE;

	enum { 
		CTYPE_NONE = 0,
		CTYPE_TCP = 1 << 1, 
		CTYPE_SSL_NO_PROXY = 1 << 2, 
		CTYPE_SSL_HTTP_PROXY = 1 << 3,
		CTYPE_SSL_SOCK_PROXY = 1 << 4,
		CTYPE_SSL_PROXY = CTYPE_SSL_HTTP_PROXY | CTYPE_SSL_SOCK_PROXY,
		CTYPE_SSL = CTYPE_SSL_NO_PROXY | CTYPE_SSL_PROXY,
		CTYPE_AUTO = CTYPE_TCP | CTYPE_SSL
	};
};

template <class UpperType, class UpTrptType, class UpSockType>
class  CConnectorTcpT : public CEventHandlerBase, public CConnectorID
{
public:
	CConnectorTcpT(CReactor *aReactor, UpperType &aUpper);
	~CConnectorTcpT();

	int Connect(const CInetAddr &aAddr, CTimeValue *aTvOut = NULL, LPVOID aSetting = NULL);
	UpTrptType* MakeTransport();
	int Close();

	virtual CM_HANDLE GetHandle() const ;
	virtual int OnInput(CM_HANDLE aFd = CM_INVALID_HANDLE);
	virtual int OnOutput(CM_HANDLE aFd = CM_INVALID_HANDLE);
	virtual int OnClose(CM_HANDLE aFd, MASK aMask);
	
private:
	int DoConnect(UpTrptType *aTrpt, const CInetAddr &aAddr);

	CReactor *m_pReactor;
	UpperType &m_Upper;
	UpTrptType *m_pTransport;
};

#include "ConnectorTcpT.inl"

#endif // !CONNECTORTCPT_H

