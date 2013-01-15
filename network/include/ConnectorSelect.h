/*------------------------------------------------------*/
/* select TCP/SSL/Proxy connector                       */
/*                                                      */
/* ConnectorSelect.h                                    */
/*                                                      */
/*                                                      */
/* Author                                               */
/*                                                      */
/* History                                              */
/*                                                      */
/*	12/14/2003	Create                                  */
/*                                                      */
/*------------------------------------------------------*/

#ifndef CONNECTORSELECT_H
#define CONNECTORSELECT_H

#ifdef WIN32
// #define CM_SUPPORT_SSL 1
#endif // WIN32

#include "TransportInterface.h"
#include "Reactor.h"
#include "ConnectorTcpT.h"
#include "TransportTcp.h"
#include "Reactor.h"

#ifdef CM_SUPPORT_SSL
#include "ConnectorOpenSslT.h"
#endif // CM_SUPPORT_SSL

class  CConnectorSelect 
	: public IConnector
	, public CConnectorID
	, public CEventHandlerBase
{
public:
	CConnectorSelect(CReactor *aReactor, IAcceptorConnectorSink *aSink);
	virtual ~CConnectorSelect();

	// aTvOut is ms
	virtual int Connect(const CInetAddr &aAddr, DWORD aType, DWORD aTvOut = 0, LPVOID aSetting = NULL);
	int Close();
	
	int OnConnectIndication(int aReason, ITransport *aTrpt, CConnectorID *aId);

	virtual int OnTimeout(const CTimeValue &aCurTime, LPVOID aArg);

private:
	CReactor *m_pReactor;
	IAcceptorConnectorSink *m_pSink;
	CConnectorTcpT<CConnectorSelect, CTransportTcp, CSocketTcp> m_TcpConnector;
#ifdef CM_SUPPORT_SSL
	CConnectorOpenSslT<CConnectorSelect, CTYPE_SSL_NO_PROXY> m_SslConnector;
	CConnectorOpenSslT<CConnectorSelect, CTYPE_SSL_PROXY> m_SslConnector2;
#endif // CM_SUPPORT_SSL
	CTYPE m_nType;
};

#endif // !CONNECTORSELECT_H

