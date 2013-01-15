/*------------------------------------------------------------------------*/
/*                                                                        */
/*  TCP transport connection                                              */
/*  transcontcp.cpp                                                         */
/*                                                                        */
/*  Implementation of TCP transport connection                            */
/*                                                                        */
/*  History                                                               */
/*  12/3/2003 created Flan Song                                           */
/*------------------------------------------------------------------------*/
#include "CmBase.h"
#include "utilbase.h"
#include "networkbase.h"
#include "ConnectorSelect.h"
#include "transcontcp.h"
#include "TransportHttp.h"
#include "TimeValue.h"
#include "datablk.h"
#include "AcceptorTcpSocket.h"
#include "Addr.h"
#include "transconpdu.h"
#include "TraceLog.h"

#define HTTP_PORT_NUMBER	80

/*########################################################################*/
/* CTransConTcpAcceptor*/
CTransConTcpAcceptor::CTransConTcpAcceptor(ITransConAcceptorSink *pSink, 
										   DWORD dwType /* = TYPE_TCP */)
{
	IM_ASSERT(pSink);
	m_pSink = pSink;
	m_pLowApt = NULL;
	m_nType = dwType;
}

CTransConTcpAcceptor::~CTransConTcpAcceptor()
{
	Clean();
}

int CTransConTcpAcceptor::Init()
{
	return 0;
}

int CTransConTcpAcceptor::Clean()
{
	if (m_pLowApt != NULL)
	{
		delete m_pLowApt;
		m_pLowApt = NULL;
	}
	return 0;
}

int CTransConTcpAcceptor::StartListen(const char *pAddr, unsigned short wPort, 
									  unsigned short bPortAutoSearch)
{
	if (wPort == 0)
		return -1;
	
	if (m_pLowApt == NULL)
	{
		if (wPort == HTTP_PORT_NUMBER)
		{
			m_pLowApt = new CAcceptorHttp(this);
		}
		else
		{
			m_pLowApt = new CAcceptorTcpSocket(CReactor::GetInstance(), this);
		}
		if (m_pLowApt == NULL)
			return -1;
	}
	
	if (!bPortAutoSearch)
	{
		CInetAddr cAddr(pAddr, wPort);
		if (m_pLowApt->StartListen(cAddr, 65536) == 0)
		{
			return wPort;
		}
		else
			return -1;
	}

	for (int i = 0; i < 5; i++)
	{
		CInetAddr cAddr(pAddr, wPort+i);
		if (m_pLowApt->StartListen(cAddr, 1024) == 0)
		{
			return wPort+i;
		}
	}
	return -1;
}

int CTransConTcpAcceptor::StopListen(int iReason)
{
	if (m_pLowApt == NULL)
		return 0;
	m_pLowApt->StopListen(iReason);	
	delete m_pLowApt;
	m_pLowApt = NULL;
	return 0;
}

int CTransConTcpAcceptor::OnConnectIndication(int aReason, ITransport *aTrans)
{
	CTcpTransCon *pCon = new CTcpTransCon(NULL, aTrans, TYPE_TCP, TRUE);
	if (pCon->Init() != 0)
	{
		ERRTRACE("Network CTransConTcpAcceptor::OnConnectIndication: Con init failed");
		delete pCon;
		return -1;
	}
	int nRet = aTrans->Open(pCon);
	
	if (-1 == nRet)
	{
		WARNINGTRACE("TP CTransConTcpAcceptor::OnConnectIndication: open failed");
		delete pCon;
		return -1;
	}

	INFOTRACE("CTransConTcpAcceptor::OnConnectIndication,"
		<< " nRet=" << nRet
		<< " pCon=" << pCon 
		<< " m_pSink=" << m_pSink
		<< " this=" << this);
	m_pSink->OnConnectIndication(pCon);
	return 0;
}

/*########################################################################*/
/* CTcpTransCon*/
CTcpTransCon::CTcpTransCon(ITransConSink *pSink /* = NULL */, 
						   ITransport *pTrans /* = NULL */, 
						   DWORD dwTransType,
						   BOOL bConnected)
{
	m_pSink = pSink;
	m_pTransport = pTrans;
	m_pTimer = NULL;
	m_pConnector = NULL;
	m_dwTranType = dwTransType;
	m_bDataSended = FALSE;
	m_bDataReceived = FALSE;
	m_pBlkLastTime = NULL;

	m_bConnected = bConnected;
}

CTcpTransCon::~CTcpTransCon()
{
	Clean();
}

void CTcpTransCon::SetSink(ITransConSink *pSink)
{
	m_pSink = pSink;
}

void CTcpTransCon::OnTick(void)
{
	if (!m_bDataSended)
	{
		if (m_bConnected && m_pTransport)
		{
			CDataBlock *pBlk = BuildKeepAlivePdu();
			m_pTransport->SendData(*pBlk);
			pBlk->Release();
		}
		else
		{
			ERRTRACE("Network CTcpTransCon::OnTick: send keep alive when not connected");
		}
	}
	else
		m_bDataSended = FALSE;

	// budingc
	if (++m_dwTickCount>= 5)
//	if (++m_dwTickCount >= TRANS_CON_KEEP_ALIVE_MAX_NUMBER*2)
	{
		if (m_bDataReceived)
		{
			m_bDataReceived = FALSE;
			m_dwTickCount = 0;
			return;
		}
		Clean(REASON_KEEPALIVE_TIMEOUT);
		VP_TRACE_INFO("Network CTcpTransCon::OnTick: Keep alive time out,m_pSink=%d this=%d",
			m_pSink,
			this);


		if (m_pSink)
			m_pSink->OnDisconnect(REASON_KEEPALIVE_TIMEOUT);
		else
			GetTransConManager()->DestroyTransCon(this);
		return;
	}
}

int	CTcpTransCon::Clean(int nReason)
{
	m_bConnected = FALSE;

	if (m_pTimer != NULL)
	{
		m_pTimer->Cancel();
		delete m_pTimer;
		m_pTimer = NULL;
	}
	
	if (m_pConnector)
	{
		delete m_pConnector;
		m_pConnector = NULL;
	}

	if (m_pTransport)
	{
		m_pTransport->Destroy(nReason);
		m_pTransport = NULL;
	}

	if (m_pBlkLastTime != NULL)
	{
		m_pBlkLastTime->Release();
		m_pBlkLastTime = NULL;
	}
	return 0;
}

int	CTcpTransCon::Clean()
{
	m_bConnected = FALSE;

	if (m_pTimer != NULL)
	{
		m_pTimer->Cancel();
		delete m_pTimer;
		m_pTimer = NULL;
	}
	
	if (m_pConnector)
	{
		delete m_pConnector;
		m_pConnector = NULL;
	}

	if (m_pTransport)
	{
		m_pTransport->Destroy();
		m_pTransport = NULL;
	}

	if (m_pBlkLastTime != NULL)
	{
		m_pBlkLastTime->Release();
		m_pBlkLastTime = NULL;
	}
	return 0;
}

int CTcpTransCon::Init(void)
{
	//If it is reactor, schedule a timer
	if (m_pTransport != NULL)
	{
		if (m_pTimer == NULL)
			m_pTimer = new CKeepAliveTimer(this);
		if (m_pTimer == NULL)
			return -1;
		m_dwTickCount = 0;
		m_pTimer->Schedule(TRANS_CON_KEEP_ALIVE_TIME);
	}
	return 0;
}

int CTcpTransCon::Connect(const char *pAddr, unsigned short wPort, 
	void *pProxySetting,  int nType)
{
	if (m_pTransport != NULL)
	{
		m_pTransport->Destroy();
		m_pTransport = NULL;
	}
	if (m_pConnector != NULL)
	{
		delete m_pConnector;
	}
	if (m_pConnector == NULL)
	{
		if (wPort == HTTP_PORT_NUMBER)
		{
			m_pConnector = new CConnectorHttp(this);
		}
		else
		{
			m_pConnector = new CConnectorSelect(CReactor::GetInstance(), this);
		}
		if (m_pConnector == NULL)
			return -1;
	}
	
	m_bConnected = FALSE;
	if (nType != TYPE_PREV)
		m_dwTranType = nType;
	CInetAddr cAddr(pAddr, wPort);
	return m_pConnector->Connect(cAddr, m_dwTranType, TRAN_CON_TCP_CONNECT_TIME_OUT, pProxySetting);
}

void CTcpTransCon::Disconnect(int iReason)
{
	Clean();
	return;
}

int CTcpTransCon::SendData(CDataBlock *pData)
{
	if (!m_bConnected)
	{
		ERRTRACE("Network CTcpTransCon::SendData: Send error, not connected");
		return -1;
	}
	IM_ASSERT (m_pTransport != NULL);
	
	m_bDataSended = TRUE;
	BuildDataPdu(pData);
	return m_pTransport->SendData(*pData);

}

int CTcpTransCon::OnConnectIndication(int aReason, ITransport *aTrans)
{
	if (aReason == REASON_SUCCESSFUL)
	{
		if (aTrans->Open(this) != 0)
		{
			m_pTransport = aTrans;
			Clean();
			return m_pSink->OnConnect(REASON_SOCKET_ERROR);
		}

		m_pTransport = aTrans;
		m_bConnected = TRUE;

		if (m_pTimer == NULL)
			m_pTimer = new CKeepAliveTimer(this);
		m_dwTickCount = 0;
		m_pTimer->Schedule(TRANS_CON_KEEP_ALIVE_TIME);
	}
	else
	{
		m_pTransport = aTrans;
		Clean();
	}
	IM_ASSERT(m_pSink);
	return m_pSink->OnConnect(aReason);
}

int CTcpTransCon::OnDisconnect(int iReason)
{
	if (!m_bConnected)
		return 0;
	Clean();
	if (m_pSink)
		m_pSink->OnDisconnect(iReason);
	else
		GetTransConManager()->DestroyTransCon(this);
	return 0;
}

int CTcpTransCon::OnReceive(CDataBlock &aData)
{
	CDataBlock *pInData;
	CTransConPduTcpData tcpData;
	
//	INFOTRACE("NetworkTest: CTcpTransCon::OnReceive receive data len = "<<aData.GetLen());
	if (!m_bConnected)
	{
		ERRTRACE("Network CTcpTransCon::OnReceive: not connected");
		return 0;
	}
	m_bDataReceived = TRUE;
	if (m_pBlkLastTime != NULL)
	{
		//merge
		pInData = new CDataBlock(m_pBlkLastTime->GetLen()+aData.GetLen(), 0);
		memcpy(pInData->GetBuf(), m_pBlkLastTime->GetBuf(), m_pBlkLastTime->GetLen());
		memcpy(pInData->GetBuf()+m_pBlkLastTime->GetLen(), aData.GetBuf(), aData.GetLen());
		pInData->Expand(m_pBlkLastTime->GetLen()+aData.GetLen());
		m_pBlkLastTime->Release();
		m_pBlkLastTime = NULL;
	}
	else
	{
		pInData = &aData;
		pInData->AddRef();
	}

	//Maybe delete in on receive
	while(pInData->GetLen() >= 0 && m_bConnected)
	{
		
		if (pInData->GetLen() < tcpData.GetLen())
		{
			m_pBlkLastTime = pInData;
			return 0;
		}

		CByteStream	stream(pInData->GetBuf(), 0, pInData->GetLen());
		tcpData.Decode(stream);

		if (tcpData.GetVersion() != 1)
		{
			WARNINGTRACE("Network CTcpTransCon::OnReceive invalid version");
		}

		if (tcpData.GetType() == TransCon_Pdu_Type_TCP_Keepalive)
		{
			CTransConPduTcpKeepAlive keepalive;
//			INFOTRACE("NetworkTest: CTcpTransCon::OnReceive keep alive packet");
			pInData->Advance(keepalive.GetLen());
			if (pInData->GetLen() <= 0)
			{
				pInData->Release();
				return 0;
			}
			continue;
		}

		DWORD	dwContLen = tcpData.GetContLen(); 
		if (dwContLen > 8192)
			WARNINGTRACE("NetworkTest: CTcpTransCon::OnReceive may be error dwContLen = "<<dwContLen);

		pInData->Advance(tcpData.GetLen());


		if (dwContLen == pInData->GetLen())
		{
			if (m_pSink)
				m_pSink->OnReceive(pInData);
			else
				WARNINGTRACE("Network CTcpTransCon::OnReceive: Error");
			pInData->Release();
			return 0;
		}
		else if (dwContLen < pInData->GetLen())
		{
			CDataBlock *pBlk = new CDataBlock(dwContLen,tcpData.GetLen());

//			INFOTRACE("Network CTcpTransCon::OnReceive: packet divided");
			memcpy(pBlk->GetBuf(), pInData->GetBuf(), dwContLen);
			pBlk->Expand(dwContLen);

			if (m_pSink)
				m_pSink->OnReceive(pBlk);
			else
				WARNINGTRACE("Network CTcpTransCon::OnReceive: Error");
			pBlk->Release();
			pInData->Advance(dwContLen);
		}
		else
		{
			pInData->Back(tcpData.GetLen());
			m_pBlkLastTime = pInData;
			return 0;
		}
	}
	return 0;
}

int CTcpTransCon::OnSend()
{
	if (m_pSink)
		m_pSink->OnSend();
	return 0;
}

CDataBlock *CTcpTransCon::BuildDataPdu(CDataBlock *pInBlk)
{
	if (pInBlk == NULL)
	{
		return NULL;
	}
	CTransConPduTcpData tcpData(pInBlk->GetLen());
	pInBlk->Back(tcpData.GetLen());
	CByteStream stream(pInBlk->GetBuf(), 0, tcpData.GetLen());
	tcpData.Encode(stream);	
	return pInBlk;
}

CDataBlock *CTcpTransCon::BuildKeepAlivePdu()
{
	CTransConPduTcpKeepAlive tcpKeep;
	CDataBlock *pBlk = new CDataBlock(8, 0);
	CByteStream stream(pBlk->GetBuf(), 0, tcpKeep.GetLen());
	tcpKeep.Encode(stream);
	pBlk->Expand(tcpKeep.GetLen());
	return pBlk;

}

int CTcpTransCon::SetOpt(unsigned long OptType, void *pParam)
{
	if (m_pTransport)
	{
		return m_pTransport->IOCtl(OptType, pParam);
	}
	else
		return -1;
}

int CTcpTransCon::GetOpt(unsigned long OptType, void *pParam)
{
	if (m_pTransport)
		return m_pTransport->IOCtl(OptType, pParam);
	else
		return -1;
}

