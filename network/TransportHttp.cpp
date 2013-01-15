/*
 *	Handle Http connections
 *
 *	Created by Frank Song 2005-8-19
 */
#include "CmBase.h"
#include "NetworkInterface.h"
#include "datablk.h"
#include "ConnectorSelect.h"
#include "AcceptorTcpSocket.h"
#include "TransportHttp.h"
#include "TimeValue.h"
#include "HttpRequest.h"
#include "Reactor.h"

static CTransportHttp* spTransport;

void NetworkHttpFini()
{
	if (spTransport != NULL) {
		delete spTransport;
	}

	spTransport = NULL;
}
/*
 *	CTransportStub
 */
CTransportStub::CTransportStub(ITransportHttpSink *pSink, ITransport *pTrans)
{
	m_pTrans = pTrans;
	m_pSink  = pSink;
}

CTransportStub::~CTransportStub()
{

}

int CTransportStub::OnDisconnect(int aReason)
{
	return m_pSink->OnDisconnect(aReason, this);
}

int CTransportStub::OnReceive(CDataBlock &aData)
{
	return m_pSink->OnReceive(aData, this);
}

int CTransportStub::OnSend()
{
	return m_pSink->OnSend(this);
}

CTransportHttp::CTransportHttp(IHttpEventSink *pEventSink, int bServer)
	: m_pInstub(new CTransportStub(this, NULL))
	, m_pOutStub(new CTransportStub(this, NULL))
	, m_pInTrans(NULL)
	, m_pOutTrans(NULL)
	, m_pNext(NULL)
	, m_pEventSink(pEventSink)
	, m_pSink(NULL)
	, m_nHttpId(0)
	, m_cAddTime(CTimeValue::GetTimeOfDay())
	, m_bServer(bServer)
	, m_bConnected(FALSE)
	, m_pBuf(NULL)
	, m_pRemainBuf(NULL)
	, m_nBufLen(0)
	, m_bNeedProxy(0)
{
	memset(&m_proxySetting, 0, sizeof(m_proxySetting));
}

/*
 *	CTransportHttp
 */
CTransportHttp::~CTransportHttp()
{
	if (m_pInstub) {
		delete m_pInstub;
		m_pInstub = NULL;
	}

	if (m_pOutStub) {
		delete m_pOutStub;
		m_pOutStub = NULL;
	}
	
	if (m_pInTrans) {
		m_pInTrans->Destroy();
		m_pInTrans = NULL;
	}
	
	if (m_pOutTrans) {
		m_pOutTrans->Destroy();
		m_pOutTrans = NULL;
	}
}

int CTransportHttp::Open(ITransportSink *aSink)
{
	m_pSink = aSink;
	return 0;
}

void CTransportHttp::Destroy(int aReason)
{
	if (m_pInTrans)
	{
		m_pInTrans->Destroy(aReason);
		m_pInTrans = NULL;
	}
	
	if (m_pOutTrans)
	{
		m_pOutTrans->Destroy(aReason);
		m_pOutTrans = NULL;
	}

	if (m_pBuf != NULL)
	{
		delete []m_pBuf;
		m_pBuf = NULL;
	}
	if (spTransport != NULL)
		delete spTransport;
	spTransport = this;
}

int CTransportHttp::SendData(CDataBlock &aData)
{
	if (!m_bConnected)
		return -1;
	return m_pOutTrans->SendData(aData);
}

int CTransportHttp::IOCtl(int aCmd, LPVOID aArg)
{
	m_pInTrans->IOCtl(aCmd, aArg);
	return m_pOutTrans->IOCtl(aCmd, aArg);
}

int CTransportHttp::OnDisconnect(int aReason, CTransportStub *pStub)
{
	if (m_pInTrans)
	{
		m_pInTrans->Destroy(aReason);
		m_pInTrans = NULL;
	}
	
	if (m_pOutTrans)
	{
		m_pOutTrans->Destroy(aReason);
		m_pOutTrans = NULL;
	}

	if (m_pSink)
		m_pSink->OnDisconnect(aReason);
	else if (m_pEventSink)
		m_pEventSink->OnEvent(HTTP_CONN_SECOND_CON_FAIL, this);
	return 0;
}
void CTransportHttp::ReceiveRemainBuf()
{
	if (m_pSink)
	{
		if (m_pRemainBuf != NULL)
		{
			CDataBlock *blk = new CDataBlock(m_nBufLen, 128);
			memcpy(blk->GetBuf(), m_pRemainBuf, m_nBufLen);
			blk->Expand(m_nBufLen);
			m_pSink->OnReceive(*blk);
			blk->Release();
			m_pRemainBuf = NULL;
			delete []m_pBuf;
			m_pBuf = NULL;
			m_nBufLen = 0;
		}
	}
}
int CTransportHttp::OnReceive(CDataBlock &aData, CTransportStub *pStub)
{
	if (m_bConnected)
	{
		if (m_pSink)
		{
			if (m_pRemainBuf != NULL)
			{
				CDataBlock *blk = new CDataBlock(m_nBufLen, 128);
				memcpy(blk->GetBuf(), m_pRemainBuf, m_nBufLen);
				blk->Expand(m_nBufLen);
				m_pSink->OnReceive(*blk);
				blk->Release();
				m_pRemainBuf = NULL;
				delete []m_pBuf;
				m_pBuf = NULL;
				m_nBufLen = 0;
			}
			return m_pSink->OnReceive(aData);
		}
		return 0;
	}
	if (m_pBuf == NULL)
	{
		m_pBuf = new char[1024];
		memset(m_pBuf, 0, sizeof(m_pBuf));
	}

	if (!m_bServer)
	{
		int parselen;
		if (pStub == m_pOutStub)
		{
			WARNINGTRACE("Invalid data\n");
			return 0;
		}
		if (aData.GetLen() + m_nBufLen < 1024)
		{
			memcpy(m_pBuf+m_nBufLen, aData.GetBuf(), aData.GetLen());
			m_nBufLen += aData.GetLen();
		}
		else
		{
			m_pEventSink->OnEvent(HTTP_CONN_FIRST_CON_FAIL, this);
			return 0;
		}
		
		if ((parselen = CHTTPRequest::ParseHttpResponse(m_pBuf, m_nBufLen)) == 0)
		{
			/*
			 *	Wait for remain packet
			 */
			return 0;
		}
		else if (parselen < 0)
		{
			/*
			 *	Invalid
			 */
			if (parselen == -407)
				m_pEventSink->OnEvent(HTTP_CONN_FIRST_CON_NEED_AUTH, this);
			else
				m_pEventSink->OnEvent(HTTP_CONN_FIRST_CON_FAIL, this);
		}
		else if (parselen > 0 )
		{
			if (m_nBufLen - parselen >= 4)
			{
				m_nBufLen = 0;
				memcpy(&m_nHttpId,m_pBuf+parselen, 4);
				if ((m_nBufLen = m_nBufLen - parselen - 4) > 0)
				{
					m_pRemainBuf = m_pBuf + parselen + 4;
				}
				else
				{
					m_pRemainBuf = NULL;
					delete []m_pBuf;
					m_pBuf = NULL;
					m_nBufLen = 0;
				}
				m_pEventSink->OnEvent(HTTP_CONN_FIRST_CON_COMPLETE, this);
			}
			else
				return 0;
		}
	}
	else
	{
		int parselen, httpmethod;
		
		if (m_nBufLen+aData.GetLen() >= 1024)
		{
			m_pEventSink->OnEvent(HTTP_CONN_SECOND_CON_FAIL, this);
			return 0;
		}
		memcpy(m_pBuf+m_nBufLen, aData.GetBuf(), aData.GetLen());	
		m_nBufLen += aData.GetLen();
		if (m_nBufLen > 5 && strncmp(m_pBuf, "POST", 4) != 0 &&
			strncmp(m_pBuf, "GET", 3) != 0)
		{
			m_pEventSink->OnEvent(HTTP_CONN_SECOND_CON_FAIL, this);
			return 0;
		}
		if ((parselen = CHTTPRequest::ParseHttpRequest(m_pBuf, m_nBufLen, &httpmethod)) == 0)
			return 0;
		if (parselen < 0)
		{
			m_pEventSink->OnEvent(HTTP_CONN_SECOND_CON_FAIL, this);
			return 0;
		}
		if (parselen > 0)
		{
			if (httpmethod == HTTP_GET)
			{
				char buf[1024];
				int nLen = 1024;
				/*
				 *	Send Get Command
				 */
				nLen = CHTTPRequest::BuildHttpResponse(buf, &nLen, 0xFFFFFFFF);
				CDataBlock *blk = new CDataBlock(nLen+4, 128);
				memcpy(blk->GetBuf(), buf, nLen);
				memcpy(blk->GetBuf()+nLen, &m_nHttpId, 4);
				blk->Expand(nLen+4);
				pStub->m_pTrans->SendData(*blk);
				blk->Release();
				m_pEventSink->OnEvent(HTTP_CONN_FIRST_CON_COMPLETE, this);
			}
			else if (httpmethod == HTTP_POST || httpmethod == HTTP_PUT)
			{
				if (parselen + 4 > m_nBufLen)
					return 0;
				memcpy(&m_nHttpId,m_pBuf+parselen, 4);
				if (m_nBufLen > parselen + 4)
				{
					m_pRemainBuf = m_pBuf + parselen + 4;
					m_nBufLen = m_nBufLen - parselen - 4;
				}
				else
				{
					m_nBufLen = 0;
					delete m_pBuf;
					m_pBuf = NULL;
				}
				m_pEventSink->OnEvent(HTTP_CONN_SECOND_CON_COMPLETE, this);
			}
		}
	}
	return 0;
}

int CTransportHttp::OnSend(CTransportStub *pStub)
{
	if (pStub == m_pOutStub && m_bConnected)
	{
		m_pSink->OnSend();
	}
	return 0;
}

void CTransportHttp::SetFirstConnection(ITransport *pTrans)
{
	//First Connection is In connection for client
	if (!m_bServer) {
		char buf[1024] = {0};
		int nLen = 1024;
		
		//Send Get Command
		pTrans->Open(m_pInstub);
		m_pInstub->m_pTrans = pTrans;
		m_pInTrans = pTrans;
		if (m_bNeedProxy) {
			nLen = CHTTPRequest::BuildHttpGetRequest(buf,
				&nLen,
				m_cAddr.GetHostAddr(), 
				m_cAddr.GetPort(),
				&m_proxySetting);
		} else {
			nLen = CHTTPRequest::BuildHttpGetRequest(buf, 
				&nLen,
				m_cAddr.GetHostAddr(), 
				m_cAddr.GetPort(),
				NULL);
		}

		CDataBlock *blk = new CDataBlock(nLen, 128);
		memcpy(blk->GetBuf(), buf, nLen);
		blk->Expand(nLen);
		m_pInTrans->SendData(*blk);
		blk->Release();
	} else {
		pTrans->Open(m_pOutStub);
		m_pOutStub->m_pTrans = pTrans;
		m_pOutTrans = pTrans;
	}
}

void CTransportHttp::SetSecondConnection(ITransport *pTrans)
{
	if (!m_bServer) {
		char buf[1024] = {0};
		int nLen = 1024;

		pTrans->Open(m_pOutStub);
		m_pOutStub->m_pTrans = pTrans;
		m_pOutTrans = pTrans;

		if (m_bNeedProxy) {
			nLen = CHTTPRequest::BuildHttpPostRequest(buf,
				&nLen, 
				m_cAddr.GetHostAddr(), 
				m_cAddr.GetPort(), 
				1024, 
				&m_proxySetting);
		} else {
			nLen = CHTTPRequest::BuildHttpPostRequest(buf,
				&nLen, 
				m_cAddr.GetHostAddr(), 
				m_cAddr.GetPort(), 
				1024, 
				NULL);
		}

		CDataBlock *blk = new CDataBlock(nLen+4, 128);
		memcpy(blk->GetBuf(), buf, nLen);
		memcpy(blk->GetBuf()+nLen, &m_nHttpId, 4);
		blk->Expand(nLen+4);
		m_pOutTrans->SendData(*blk);
		blk->Release();

		m_bConnected = TRUE;
		m_pEventSink->OnEvent(HTTP_CONN_SECOND_CON_COMPLETE, this);
	} else {
		pTrans->Open(m_pInstub);
		m_pInstub->m_pTrans = pTrans;
		m_pInTrans = pTrans;
	}	
}

/*
 *	CAcceptorHttp
 */
CAcceptorHttp::CAcceptorHttp(IAcceptorConnectorSink *pSink)
{
	m_pSink = pSink;
	m_pDeleteTrans = NULL;
	m_pHalfCompTrans = NULL;
	m_nHttpId = (CTimeValue::GetTimeOfDay()).GetUsec();
	m_pAcceptor = new CAcceptorTcpSocket(CReactor::GetInstance(), this);
}

CAcceptorHttp::~CAcceptorHttp()
{
	CTransportHttp *pTemp;
	
	while(m_pHalfCompTrans != NULL) {
		pTemp = m_pHalfCompTrans;
		m_pHalfCompTrans = m_pHalfCompTrans->m_pNext;
		pTemp->Destroy();
	}
	
	if (m_pDeleteTrans != NULL) {
		delete m_pDeleteTrans;
		m_pDeleteTrans = NULL;
	}
	
	if (m_pAcceptor != NULL) {
		delete m_pAcceptor;
		m_pAcceptor = NULL;
	}
}

CTransportHttp *CAcceptorHttp::FindHttpPair(unsigned long nFindId)
{
	CTransportHttp *pTrans;
	for (pTrans = m_pHalfCompTrans; pTrans != NULL; pTrans = pTrans->m_pNext) {
		if (pTrans->m_nHttpId == nFindId) {
			return pTrans;
		}
	}
	return NULL;
}

void  CAcceptorHttp::RemoveHttpTrans(CTransportHttp *pHttpTrans)
{
	CTransportHttp* pTrans1 = NULL;

	if (m_pHalfCompTrans == NULL) {
		return;
	}

	if (pHttpTrans == m_pHalfCompTrans) {
		m_pHalfCompTrans = m_pHalfCompTrans->m_pNext;
		return;
	}

	pTrans1 = m_pHalfCompTrans;
	while (pTrans1->m_pNext != NULL) {
		if (pTrans1->m_pNext == pHttpTrans) {
			pTrans1->m_pNext = pHttpTrans->m_pNext;
			return;
		}
		pTrans1 = pTrans1->m_pNext;
	}
}

int CAcceptorHttp::OnEvent(int nID, CTransportHttp *pHttpTrans)
{
	CTransportHttp *pHttpPair = NULL;
	switch (nID)
	{
	case HTTP_CONN_FIRST_CON_COMPLETE:
		return 0;
	case HTTP_CONN_FIRST_CON_FAIL:
	case HTTP_CONN_FIRST_CON_NEED_AUTH:
		RemoveHttpTrans(pHttpTrans);
		pHttpTrans->Destroy();
		return 0;
	case HTTP_CONN_SECOND_CON_COMPLETE:
		RemoveHttpTrans(pHttpTrans);
		pHttpPair = FindHttpPair(pHttpTrans->m_nHttpId);
		if (pHttpPair == NULL) {
			WARNINGTRACE("Can not find the first connection\n");
			pHttpTrans->Destroy();
			return 0;
		}
		RemoveHttpTrans(pHttpPair);
		pHttpTrans->SetFirstConnection(pHttpPair->m_pInTrans);
		pHttpPair->m_pInTrans = NULL;
		pHttpPair->Destroy();
		pHttpTrans->m_bConnected = TRUE;
		m_pSink->OnConnectIndication(REASON_SUCCESSFUL, pHttpTrans);
		pHttpTrans->ReceiveRemainBuf();
		return 0;
		break;
	case HTTP_CONN_SECOND_CON_FAIL:
		RemoveHttpTrans(pHttpTrans);
		pHttpTrans->Destroy();
		return 0;
	default:
		break;
	}
	return 0;
}

int CAcceptorHttp::OnConnectIndication(int aReason, ITransport *aTrans)
{
	CTransportHttp *pTrans = new CTransportHttp(this, TRUE);
	pTrans->SetSecondConnection(aTrans);
	pTrans->m_nHttpId = ++m_nHttpId;
	pTrans->m_pNext = m_pHalfCompTrans;
	m_pHalfCompTrans = pTrans;
	return 0;
}

int CAcceptorHttp::StartListen(const CInetAddr &aAddr, DWORD aBacklog)
{
	if (m_pAcceptor == NULL) {
		return -1;
	}

	return m_pAcceptor->StartListen(aAddr, aBacklog);
}

int CAcceptorHttp::StopListen(int iReason)
{
	if (m_pAcceptor == NULL) {
		return -1;
	}

	return m_pAcceptor->StopListen(iReason);
}

/*
 *	CConnectorHttp
 */
CConnectorHttp::CConnectorHttp(IAcceptorConnectorSink *pSink)
{
	m_pSink = pSink;
	m_pConnector = NULL;
	m_pTrans = NULL;
	m_bNeedProxy = 0;
	memset(&m_proxySetting, 0, sizeof(m_proxySetting));
}

CConnectorHttp::~CConnectorHttp()
{
	if (m_pConnector != NULL) {
		delete m_pConnector;
		m_pConnector = NULL;
	}

	if (m_pTrans != NULL) {
		delete m_pTrans;
		m_pTrans = NULL;
	}
}

int CConnectorHttp::Connect(const CInetAddr &aAddr, 
							DWORD aType,
							DWORD aTmOut, 
							LPVOID aSetting)
{
	if (m_pConnector != NULL) {
		return -1;
	}
	
	m_pConnector = new CConnectorSelect(CReactor::GetInstance(), this);
	m_serverAddr = aAddr;
	if (aSetting != NULL) {
		m_bNeedProxy = 1;
		memcpy(&m_proxySetting, aSetting, sizeof(m_proxySetting));
		CInetAddr proxyAddr(m_proxySetting.ProxyAddr, m_proxySetting.ProxyPort);
		m_aAddr = proxyAddr;
		proxyAddr = aAddr;
		m_proxySetting.ProxyAddr = ntohl(inet_addr(proxyAddr.GetHostAddr()));
		m_proxySetting.ProxyPort = aAddr.GetPort();
	} else {
		m_bNeedProxy = 0;
		m_aAddr = aAddr;
	}
	m_aType = aType;
	m_aTmOut = aTmOut;
	return m_pConnector->Connect(m_aAddr, aType, aTmOut, aSetting);
}

int CConnectorHttp::OnConnectIndication(int aReason, ITransport *aTrans)
{
	if (aReason == REASON_SUCCESSFUL) {
		/*
		 *	First Connection
		 */
		if (m_pTrans == NULL) {
			m_pTrans = new CTransportHttp(this, FALSE);
			m_pTrans->m_cAddr = m_serverAddr;
			m_pTrans->m_bNeedProxy = m_bNeedProxy;
			memcpy(&(m_pTrans->m_proxySetting), &m_proxySetting, sizeof(m_proxySetting));
			m_pTrans->SetFirstConnection(aTrans);
		} else {
			m_pTrans->SetSecondConnection(aTrans);
		}
	} else {
		if (m_pTrans != NULL) {
			m_pTrans->Destroy();
			m_pTrans = NULL;
		}
		m_pSink->OnConnectIndication(aReason, NULL);
	}
	return 0;
}

int CConnectorHttp::OnEvent(int nID, CTransportHttp *pHttpTrans)
{
	switch (nID)
	{
	case HTTP_CONN_FIRST_CON_COMPLETE:
		if (m_pConnector)
			delete m_pConnector;
		m_pConnector = new CConnectorSelect(CReactor::GetInstance(), this);
		return m_pConnector->Connect(m_aAddr, m_aType, m_aTmOut, NULL);
	case HTTP_CONN_FIRST_CON_FAIL:
		m_pTrans->Destroy();
		m_pTrans = NULL;
		return m_pSink->OnConnectIndication(REASON_SOCKET_ERROR, NULL);
		break;
	case HTTP_CONN_FIRST_CON_NEED_AUTH:
		m_pTrans->Destroy();
		m_pTrans = NULL;
		return m_pSink->OnConnectIndication(NETWORK_REASONPROXY_NEED_AUTH, NULL);
	case HTTP_CONN_SECOND_CON_COMPLETE:
	{
		CTransportHttp *pTrans = m_pTrans;
		m_pTrans = NULL;
		m_pSink->OnConnectIndication(REASON_SUCCESSFUL, pTrans);
		pTrans->ReceiveRemainBuf();
		return 0;
	}
	case HTTP_CONN_SECOND_CON_FAIL:
		m_pTrans->Destroy();
		m_pTrans = NULL;
		return m_pSink->OnConnectIndication(REASON_SOCKET_ERROR, NULL);
	default:
		break;
	}
	return 0;
}
