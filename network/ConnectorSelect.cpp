
#include "CmBase.h"
#include "ConnectorSelect.h"
#include "TimeValue.h"
#include "TraceLog.h"

CConnectorSelect::CConnectorSelect(CReactor *aReactor, IAcceptorConnectorSink *aSink)
	: m_pReactor(aReactor)
	, m_pSink(aSink)
	, m_TcpConnector(aReactor, *this)
#ifdef CM_SUPPORT_SSL
	, m_SslConnector(aReactor, *this)
	, m_SslConnector2(aReactor, *this)
#endif // CM_SUPPORT_SSL
	, m_nType(CTYPE_NONE)
{
	CM_ASSERTE(m_pReactor);
	CM_ASSERTE(m_pSink);
}

CConnectorSelect::~CConnectorSelect()
{
	Close();
}

int CConnectorSelect::
Connect(const CInetAddr &aAddr, DWORD aType, DWORD aTmOut, LPVOID aSetting)
{
	CM_ASSERTE(m_nType == CTYPE_NONE);

	switch (aType) {
	case TYPE_TCP:
		m_nType = CTYPE_TCP;
		break;
	case TYPE_SSL:
		m_nType = CTYPE_SSL;
		break;
	case TYPE_AUTO:
		m_nType = CTYPE_TCP;
		break;
	case 999:
		// for test only
		m_nType = CTYPE_SSL_PROXY;
		break;
	default:
		CM_ERROR_LOG(("CConnectorSelect::Connect, wrong1 type=%d!", aType));
		return -1;
	}

	if (CM_BIT_ENABLED(m_nType, CTYPE_TCP)) {
		int nRet = m_TcpConnector.Connect(aAddr, NULL, aSetting);
		if (nRet == -1)
			CM_CLR_BITS(m_nType, CTYPE_TCP);
	}
#ifdef CM_SUPPORT_SSL
	if (CM_BIT_ENABLED(m_nType, CTYPE_SSL_NO_PROXY)) {
		int nRet = m_SslConnector.Connect(aAddr, NULL, aSetting);
		if (nRet == -1)
			CM_CLR_BITS(m_nType, CTYPE_SSL_NO_PROXY);
	}
	if (CM_BIT_ENABLED(m_nType, CTYPE_SSL_PROXY)) {
		int nRet = m_SslConnector2.Connect(aAddr, NULL, aSetting);
		if (nRet == -1)
			CM_CLR_BITS(m_nType, CTYPE_SSL_PROXY);
	}
#endif // CM_SUPPORT_SSL
	
	if (m_nType == CTYPE_NONE) {
		VP_TRACE_ERROR("CConnectorSelect::Connect, connect failed!");
		CTimeValue tvTime(0, 0);
		m_pReactor->ScheduleTimer(this, 
			reinterpret_cast<LPVOID>(REASON_UNKNOWN_ERROR), tvTime, 1);
		return 0;
	}

	if (aTmOut != 0) {
		CTimeValue tvTime(0, aTmOut * 1000);
		m_pReactor->ScheduleTimer(this, 
			reinterpret_cast<LPVOID>(REASON_CONNECT_TIMEOUT), tvTime, 1);
	}
	return 0;
}

int CConnectorSelect::
OnConnectIndication(int aReason, ITransport *aTrpt, CConnectorID *aId)
{
	CM_ASSERTE(m_nType != CTYPE_NONE);

	// workaround for Norton personal firewall
#if 1
#if WIN32
	if (aReason == REASON_SUCCESSFUL) {
		CM_ASSERTE(aTrpt);
		CInetAddr addrClient, addrServer;
		DWORD dwTypeCon;
		int nRet = aTrpt->IOCtl(WUO_TRANSPORT_OPT_GET_LOCAL_ADDR, &addrClient);
		CM_ASSERTE(nRet == 0);
		nRet = aTrpt->IOCtl(WUO_TRANSPORT_OPT_GET_PEER_ADDR, &addrServer);
		CM_ASSERTE(nRet == 0);
		nRet = aTrpt->IOCtl(WUO_TRANSPORT_OPT_GET_TRAN_TYPE, &dwTypeCon);
		CM_ASSERTE(nRet == 0);
		if (dwTypeCon == TYPE_TCP && 
			 strcmp(addrClient.GetHostAddr(), "127.0.0.1") == 0
                        && addrServer.GetPort() == 80) 
		{
			VP_TRACE_ERROR("CConnectorSelect::OnConnectIndication, detect personal firewall."
				" cli_addr=%s cli_port=%d srv_addr=%s srv_port=%d", 
				addrClient.GetHostAddr(), (int)addrClient.GetPort(),
				addrServer.GetHostAddr(), (int)addrServer.GetPort());
			::Sleep(500);

			BOOL bAlive = FALSE;
			nRet = aTrpt->IOCtl(WUO_TRANSPORT_OPT_GET_SOCK_ALIVE, &bAlive);
			if (!bAlive) {
				VP_TRACE_ERROR("CConnectorSelect::OnConnectIndication, transport isn't alive.");
				aTrpt->Destroy();
				aTrpt = NULL;
				aReason = REASON_SOCKET_ERROR;
			}
		}
	}
#endif // WIN32
#endif

	if (aReason != REASON_SUCCESSFUL) {
		if (&m_TcpConnector == aId) {
			CM_ASSERTE(CM_BIT_ENABLED(m_nType, CTYPE_TCP));
			CM_INFO_LOG(("CConnectorSelect::OnConnectIndication, CTYPE_TCP failed."));
			m_TcpConnector.Close();
			CM_CLR_BITS(m_nType, CTYPE_TCP);
		}
#ifdef CM_SUPPORT_SSL
		else if (&m_SslConnector == aId) {
			CM_ASSERTE(CM_BIT_ENABLED(m_nType, CTYPE_SSL_NO_PROXY));
			CM_INFO_LOG(("CConnectorSelect::OnConnectIndication, CTYPE_SSL_NO_PROXY failed."));
			m_SslConnector.Close();
			CM_CLR_BITS(m_nType, CTYPE_SSL_NO_PROXY);
		}
		else if (&m_SslConnector2 == aId) {
			CM_ASSERTE(CM_BIT_ENABLED(m_nType, CTYPE_SSL_PROXY));
			CM_INFO_LOG(("CConnectorSelect::OnConnectIndication, CTYPE_SSL_PROXY failed."));
			m_SslConnector2.Close();
			CM_CLR_BITS(m_nType, CTYPE_SSL_PROXY);
		}
#endif // CM_SUPPORT_SSL
		else {
			CM_ERROR_LOG(("CConnectorSelect::OnConnectIndication,"
				" wrong1 reason=%d id=%x type=%d!", aReason, aId, m_nType));
			CM_ASSERTE(FALSE);
			return -1;
		}
		
		if (m_nType == CTYPE_NONE) {
			Close();
			m_pSink->OnConnectIndication(aReason, NULL);
		}
		return 0;
	}

	if (&m_TcpConnector != aId && CM_BIT_ENABLED(m_nType, CTYPE_TCP)) {
		m_TcpConnector.Close();
		CM_CLR_BITS(m_nType, CTYPE_TCP);
	}
#ifdef CM_SUPPORT_SSL
	if (&m_SslConnector != aId && CM_BIT_ENABLED(m_nType, CTYPE_SSL_NO_PROXY)) {
		m_SslConnector.Close();
		CM_CLR_BITS(m_nType, CTYPE_SSL_NO_PROXY);
	}
	if (&m_SslConnector2 != aId && CM_BIT_ENABLED(m_nType, CTYPE_SSL_PROXY)) {
		m_SslConnector2.Close();
		CM_CLR_BITS(m_nType, CTYPE_SSL_PROXY);
	}
#endif // CM_SUPPORT_SSL
	
	CM_ASSERTE(aTrpt);
	switch (m_nType) {
	case CTYPE_TCP:
#ifdef CM_SUPPORT_SSL
	case CTYPE_SSL_NO_PROXY:
	case CTYPE_SSL_PROXY:
#endif // CM_SUPPORT_SSL
		{
			CInetAddr addrClient, addrServer;
			ITransport *pTrans = aTrpt;
			int nRet = pTrans->IOCtl(WUO_TRANSPORT_OPT_GET_LOCAL_ADDR, &addrClient);
			CM_ASSERTE(nRet == 0);
			nRet = pTrans->IOCtl(WUO_TRANSPORT_OPT_GET_PEER_ADDR, &addrServer);
			CM_ASSERTE(nRet == 0);
			CM_INFO_LOG(("CConnectorSelect::OnConnectIndication, successful, cli_addr=%s cli_port=%d srv_addr=%s srv_port=%d", 
				addrClient.GetHostAddr(), (int)addrClient.GetPort(),
				addrServer.GetHostAddr(), (int)addrServer.GetPort()));
		}

		m_pReactor->CancelTimer(this);
		m_pSink->OnConnectIndication(aReason, aTrpt);
		break;

	default:
		CM_ERROR_LOG(("CConnectorSelect::OnConnectIndication,"
			" wrong2 aId=%x type=%d!", aId, m_nType));
		CM_ASSERTE(FALSE);
		return -1;
	}
	return 0;
}

int CConnectorSelect::Close()
{
	m_pReactor->CancelTimer(this);

	int nRet = 0;
	if (CM_BIT_ENABLED(m_nType, CTYPE_TCP)) {
		nRet |= m_TcpConnector.Close();
		CM_CLR_BITS(m_nType, CTYPE_TCP);
	}
#ifdef CM_SUPPORT_SSL
	if (CM_BIT_ENABLED(m_nType, CTYPE_SSL_NO_PROXY)) {
		nRet |= m_SslConnector.Close();
		CM_CLR_BITS(m_nType, CTYPE_SSL_NO_PROXY);
	}
	if (CM_BIT_ENABLED(m_nType, CTYPE_SSL_PROXY)) {
		nRet |= m_SslConnector2.Close();
		CM_CLR_BITS(m_nType, CTYPE_SSL_PROXY);
	}
#endif // CM_SUPPORT_SSL
	CM_ASSERTE(m_nType == CTYPE_NONE);
	return nRet;
}

int CConnectorSelect::OnTimeout(const CTimeValue &aCurTime, LPVOID aArg)
{
	int nReason = reinterpret_cast<int>(aArg);
	if (nReason == REASON_CONNECT_TIMEOUT) {
		CM_INFO_LOG(("CConnectorSelect::OnTimer, connect timeout."));
	}
	else if (nReason == REASON_UNKNOWN_ERROR) {
		CM_INFO_LOG(("CConnectorSelect::OnTimer, connect failed."));
	}
	else {
		CM_ERROR_LOG(("CConnectorSelect::OnTimer, unkown nReason=%d", nReason));
		CM_ASSERTE(FALSE);
		return -1;
	}

	Close();
	m_pSink->OnConnectIndication(nReason, NULL);
	return 0;
}
