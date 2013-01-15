#include "CmBase.h"
#include "datablk.h"
#include "transcontcp.h"
#include "Reactor.h"
#include "NetworkTcpCon.h"
#include "transconmanager.h"
#include "ConnectorSelect.h"
#include "AcceptorTcpSocket.h"
#include "TraceLog.h"

extern "C"
{
	INetAcceptor *CreateRawTcpAcceptor(INetAcceptorSink *pSink)
	{
		if (pSink == NULL)
			return NULL;
		return new CNetRawTcpAcceptor(pSink);
	}
	INetAcceptor *CreateTcpAcceptor(INetAcceptorSink *pSink)
	{
		if (pSink == NULL)
			return NULL;
		return new CNetTcpAcceptor(pSink);
	}
	INetConnection *CreateRawTcpCon(INetConnectionSink *pSink)
	{
		if (pSink == NULL)
			return NULL;
		return new CNetRawTcpConnection(pSink);
	}
	INetConnection *CreateTcpCon(INetConnectionSink *pSink)
	{
		if (pSink == NULL)
			return NULL;
		return new CNetTcpConnection(pSink);
	}
	
}
CNetRawTcpConnection::CNetRawTcpConnection(INetConnectionSink *pSink)
{
	m_pConnector = NULL;
	m_pTransport = NULL;
	m_pSink = pSink;
}

CNetRawTcpConnection::CNetRawTcpConnection(INetConnectionSink *pSink, ITransport *pTrans)
{
	m_pConnector = NULL;
	m_pTransport = pTrans;
	
	if (pTrans->Open(this) != 0)
	{
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
	}
	m_pSink = pSink;	
}

CNetRawTcpConnection::~CNetRawTcpConnection()
{
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
	
}

int CNetRawTcpConnection::Connect(
	unsigned long	dwAddr, 
	unsigned short	wPort, 
	int				nType,
	void			*pProxySetting
	)
{
	if (m_pTransport != NULL)
	{
		m_pTransport->Destroy();
	}
	if (m_pConnector != NULL)
	{
		delete m_pConnector;
	}
	if (m_pConnector == NULL)
		m_pConnector = new CConnectorSelect(CReactor::GetInstance(), this);
	
	
	CInetAddr cAddr(dwAddr, wPort);
	return m_pConnector->Connect(cAddr, nType, TRAN_CON_TCP_CONNECT_TIME_OUT, pProxySetting);
	
}

int CNetRawTcpConnection::OnConnectIndication(int aReason, ITransport *aTrans)
{
	if (aReason == REASON_SUCCESSFUL)
	{
		if (aTrans->Open(this) != 0)
		{
			m_pTransport = aTrans;
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
			return m_pSink->OnConnect(REASON_SOCKET_ERROR, this);
		}
		
		m_pTransport = aTrans;
	}
	else
	{
		m_pTransport = aTrans;
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
	}
	return m_pSink->OnConnect(aReason, this);
}

void CNetRawTcpConnection::Disconnect(int iReason)
{
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
}

int CNetRawTcpConnection::SendData(unsigned char *pData, int nLen)
{
	if (m_pTransport)
	{
		CDataBlock *blk = new CDataBlock(nLen, 128);
		memcpy(blk->GetBuf(), pData, nLen);
		blk->Expand(nLen);
		int rt = m_pTransport->SendData(*blk);
		blk->Release();
		return rt;
	}
	else
		return -1;
}

int CNetRawTcpConnection::SendTo(unsigned char *pData, int nLen, 
	unsigned long dwAddr, unsigned short wPort)
{
	return -1;
}

int CNetRawTcpConnection::SetOpt(unsigned long OptType, void *pParam)
{
	if (m_pTransport)
	{
		return m_pTransport->IOCtl(OptType, pParam);
	}
	else
		return -1;
}

int CNetRawTcpConnection::GetOpt(unsigned long OptType, void *pParam)
{
	if (m_pTransport)
		return m_pTransport->IOCtl(OptType, pParam);
	else
		return -1;
}

int CNetRawTcpConnection::OnDisconnect(int aReason)
{
	return m_pSink->OnDisconnect(aReason, this);
}

int CNetRawTcpConnection::OnReceive(CDataBlock &aData)
{
	return m_pSink->OnReceive(aData.GetBuf(), aData.GetLen(), this);
}

int CNetRawTcpConnection::OnSend()
{
	return m_pSink->OnSend(this);
}
		
/*
 *	CNetTcpConnection
 */
CNetTcpConnection::CNetTcpConnection(INetConnectionSink *pSink)
{
	m_pSink = pSink;
	m_pTrans = new CTcpTransCon(this, NULL);
	if (m_pTrans->Init() != 0)
	{
		delete m_pTrans;
		m_pTrans = NULL;
	}
	m_bConnected = 0;

	/*
	 *	Frank For Reliable TCP
	 */
	m_nCommandRecvSeq = 0;
	m_nCommandPutSeq = 0;
	m_nPutIndex = 0;
	m_nSendIndex = 0;
	memset(m_pCommands, 0, sizeof(m_pCommands));
	memset(m_dwCBTypes, 0, sizeof(DWORD)*MAX_WAIT_CMD);
	memset(m_dwCBDatas, 0, sizeof(DWORD)*MAX_WAIT_CMD);
	
	m_pTimer = new CNetTimer(this);
	m_pTimer->Schedule(1500);
}

CNetTcpConnection::CNetTcpConnection(INetConnectionSink *pSink, ITransCon *pTransCon)
{
	m_pSink = pSink;
	m_pTrans = (CTcpTransCon *)pTransCon;
	m_bConnected = 1;

	/*
	 *	Frank For Reliable TCP
	 */
	m_nCommandRecvSeq = 0;
	m_nCommandPutSeq = 0;
	m_nPutIndex = 0;
	m_nSendIndex = 0;
	memset(m_pCommands, 0, sizeof(m_pCommands));
	memset(m_dwCBTypes, 0, sizeof(DWORD)*MAX_WAIT_CMD);
	memset(m_dwCBDatas, 0, sizeof(DWORD)*MAX_WAIT_CMD);
	
	m_pTimer = new CNetTimer(this);
	m_pTimer->Schedule(1500);
}

CNetTcpConnection::~CNetTcpConnection()
{
	if (m_pTrans != NULL)
	{
		delete m_pTrans;
		m_pTrans = NULL;
	}
	/*
	 *	Frank For Reliable TCP
	 */
	for (int i = 0; i < MAX_WAIT_CMD; i++)
	{
		if (m_pCommands[i] != NULL)
		{
			(m_pCommands[i])->Release();
		}
	}
	m_bConnected = 0;
	
	if (m_pTimer)
	{
		m_pTimer->Cancel();
		delete m_pTimer;
		m_pTimer = NULL;
	}
}
/*
 *	Frank For Reliable TCP
 */
void CNetTcpConnection::OnTimer(void *pArg, INetTimer *pTimer)
{
	if (!m_bConnected || m_nPutIndex == m_nSendIndex )
		return;
	
	if (m_bCommandSend)
	{
		m_bCommandSend = FALSE;
		return;
	}
	
	m_pCommands[m_nSendIndex]->SetCurToOrg();
	m_pTrans->SendData(m_pCommands[m_nSendIndex]);
}

int CNetTcpConnection::Connect(
		unsigned long	dwAddr, 
		unsigned short	wPort, 
		int				nType,
		void			*pProxySetting
		)
{
	struct in_addr addr;
	addr.s_addr = htonl(dwAddr);
	
	if (m_pTrans)
		return m_pTrans->Connect(inet_ntoa(addr), wPort, pProxySetting, nType);
	else
		return -1;
}
	
void CNetTcpConnection::Disconnect(int iReason)
{
	m_bConnected = 0;
	m_pTrans->Disconnect();
}
	
int CNetTcpConnection::SendData(unsigned char *pData, int nLen)
{
	if (!m_bConnected)
		return -1;
	
	if (m_pTrans)
	{
		CDataBlock *blk = new CDataBlock(nLen+1, 128);
		(blk->GetBuf())[0] = NET_TCP_DATA_TYPE_DATA;
		memcpy(blk->GetBuf()+1, pData, nLen);
		blk->Expand(nLen+1);
		int rt = m_pTrans->SendData(blk);
		blk->Release();
		return rt;
	}
	else
		return -1;
}

int CNetTcpConnection::SendCommand(unsigned char *pData, int nLen)
{

	return SendCommand(pData, nLen, 0, 0);
	
//	return 0;
}

int CNetTcpConnection::SendCommand(unsigned char *pData, 
								   int nLen, 
								   DWORD dwCallBackType,
								   DWORD dwCallBackData)
{
	if (!m_bConnected || !m_pTrans || nLen > 1400)
	{
		VP_TRACE_WARNING("CNetTcpConnection::SendCommand() len>1400");
		return -1;
	}
		/*
	 *	Frank For Reliable TCP
	 */
	if ((m_nPutIndex + 1)%MAX_WAIT_CMD ==  m_nSendIndex) {
		VP_TRACE_WARNING("CNetTcpConnection::SendCommand fail putIndex=%d sendIndex=%d",m_nPutIndex,m_nSendIndex);
		return -1;
	}
	
	unsigned long index = htonl(m_nCommandPutSeq++);
	
	CDataBlock *blk = new CDataBlock(nLen+5, 128);
	m_pCommands[m_nPutIndex] = blk;
	(blk->GetBuf())[0] = NET_TCP_DATA_TYPE_CMD;
	memcpy(blk->GetBuf()+1, &index, 4);
	memcpy(blk->GetBuf()+5, pData, nLen);
	blk->Expand(nLen+5);
	blk->SetOrgToCur();
	m_dwCBDatas[m_nPutIndex] = dwCallBackData;
	m_dwCBTypes[m_nPutIndex] = dwCallBackType;
	/*
	*	Buffer Empty
	*/
	if (m_nSendIndex == m_nPutIndex)
	{
		blk->SetCurToOrg();
		m_pTrans->SendData(blk);
		m_bCommandSend = TRUE;
	}
	m_nPutIndex = (m_nPutIndex + 1) % MAX_WAIT_CMD;
	return 0;
}

int CNetTcpConnection::SendMedia(unsigned char *pHeader, int nHeaderLen, 
			  char *pData, int nLen, unsigned char nMediaType, unsigned char nSubSeq, 
			  unsigned short usIFrameSeq)
{
	if (!m_bConnected)
		return -1;
	if (m_pTrans)
	{
		
		CDataBlock *blk = new CDataBlock(nLen+nHeaderLen+1, 128);
		(blk->GetBuf())[0] = NET_TCP_DATA_TYPE_DATA;
		memcpy(blk->GetBuf()+1, pHeader, nHeaderLen);
		memcpy(blk->GetBuf()+nHeaderLen+1, pData, nLen);
		blk->Expand(nLen+nHeaderLen+1);
		int rt = m_pTrans->SendData(blk);
		blk->Release();
		return rt;
	}
	else
		return -1;
	
}
int CNetTcpConnection::SendTo(unsigned char *pData, int nLen, 
	unsigned long dwAddr, unsigned short wPort)
{
	return -1;
}

int CNetTcpConnection::SetOpt(unsigned long OptType, void *pParam)
{
	if (m_pTrans != NULL)
	{
		return m_pTrans->SetOpt(OptType, pParam);
	}
	else
		return -1;
}

int CNetTcpConnection::GetOpt(unsigned long OptType, void *pParam)
{
	if (m_pTrans != NULL)
		return m_pTrans->GetOpt(OptType, pParam);
	else
		return -1;
}

int CNetTcpConnection::OnConnect(int iReason)
{
	if (iReason == 0)
		m_bConnected = 1;

	return m_pSink->OnConnect(iReason, this);
}

int CNetTcpConnection::OnDisconnect(int iReason)
{
	m_bConnected = 0;
	/*
	 *	Frank for Relaibe TCP
	 */
	if (m_pSink != NULL)
	{
		for (int nSendIndex = m_nSendIndex; nSendIndex != m_nPutIndex; nSendIndex = (++nSendIndex)%MAX_WAIT_CMD)
		{

			if (!(m_dwCBTypes[nSendIndex] & DATA_NEED_CALLBACK))
			{
				//此数据不需要回调
				continue;
			}

			if (nSendIndex == m_nSendIndex)
			{
				m_pCommands[m_nSendIndex]->SetCurToOrg();
			}
			m_pSink->OnSend(REASON_SENDCOMMADN_TIMEOUT, 
				m_pCommands[nSendIndex]->GetBuf() + 5,
				m_pCommands[nSendIndex]->GetLen()-5,
				m_dwCBTypes[nSendIndex],
				m_dwCBDatas[nSendIndex],
				this);

		}
		return m_pSink->OnDisconnect(iReason, this);
	}
	return m_pSink->OnDisconnect(iReason,this);
}

int CNetTcpConnection::OnReceive(CDataBlock *pData)
{
	unsigned char type = (pData->GetBuf())[0];
	switch (type)
	{
	case NET_TCP_DATA_TYPE_DATA:
		//TODO:wujb, improve assert to support log.
		assert(m_pSink != NULL);

		return m_pSink->OnReceive(pData->GetBuf()+1, pData->GetLen()-1, this);
		
	case NET_TCP_DATA_TYPE_CMD:
		/*
		 *	Frank for Reliable TCP
		 */
	{
		unsigned int index;
		memcpy((unsigned char*)&index, pData->GetBuf()+1, 4);
		index = ntohl(index);
		if (index > m_nCommandRecvSeq)
		{
			m_nCommandRecvSeq = index;
		}
		/*
		 *	Send Reponse
		 */
		CDataBlock *blk = new CDataBlock(5, 128);
		(blk->GetBuf())[0] = NET_TCP_DATA_TYPE_RESP;
		memcpy(blk->GetBuf()+1, pData->GetBuf()+1, 4);
		blk->Expand(5);
		m_pTrans->SendData(blk);
		blk->Release();

		if (index == m_nCommandRecvSeq)
		{
			/*
			* Indicate
			*/
			m_pSink->OnCommand(pData->GetBuf()+5, pData->GetLen()-5, this);
			m_nCommandRecvSeq ++;
		}
		return 0;
	}
	break;
	/*
	 *	Frank For Relialbe TCP
	 */
	case NET_TCP_DATA_TYPE_RESP:
	{
		if (m_nSendIndex == m_nPutIndex)
		{
			return 0;
		}
		m_pCommands[m_nSendIndex]->SetCurToOrg();
		BYTE *p1 = m_pCommands[m_nSendIndex]->GetBuf();
		BYTE *p2 = pData->GetBuf();
		if (memcmp((p1+1), (p2+1), 4) == 0)
		{
			/*
			 *	Command send succeed, send next command
			 */
			m_pCommands[m_nSendIndex]->Release();
			m_pCommands[m_nSendIndex] = NULL;
			m_dwCBDatas[m_nSendIndex] = 0;
			m_dwCBTypes[m_nSendIndex] = 0;
			m_nSendIndex = (m_nSendIndex + 1) % MAX_WAIT_CMD;
			if (m_nSendIndex == m_nPutIndex)
			{
				return 0;
			}
			m_pCommands[m_nSendIndex]->SetCurToOrg();

			m_pTrans->SendData(m_pCommands[m_nSendIndex]);
			m_bCommandSend = TRUE;
		}
		return 0;
	}
	break;
	default:
		WARNINGTRACE("CNetTcpConnection::OnReceive Invalid data\n");
		return 0;
		break;
	}
}

int CNetTcpConnection::OnSend()
{
	return m_pSink->OnSend(this);
}
	
/*
 *	CNetRawTcpAcceptor
 */
CNetRawTcpAcceptor::CNetRawTcpAcceptor(INetAcceptorSink *pSink)
{
	m_pSink = pSink;
	m_pApt = new CAcceptorTcpSocket(CReactor::GetInstance(), this);
}

CNetRawTcpAcceptor::~CNetRawTcpAcceptor()
{
	if (m_pApt != NULL)
	{
		delete m_pApt;
		m_pApt = NULL;
	}
	
}

int CNetRawTcpAcceptor::StartListen(unsigned short wPort, unsigned long dwLocalAddr)
{
	CInetAddr cAddr(dwLocalAddr, wPort);

	if (m_pApt && m_pApt->StartListen(cAddr, 65536) == 0)
	{
		return wPort;
	}
	else
		return -1;
}

int CNetRawTcpAcceptor::StopListen()
{

	if (m_pApt)
	{
		m_pApt->StopListen();
		return 0;
	}
	else
	{
		return -1;
	}
}

int CNetRawTcpAcceptor::OnConnectIndication (int aReason, ITransport *aTrans)
{
	CNetRawTcpConnection *pCon = new CNetRawTcpConnection(NULL, aTrans);
	return m_pSink->OnConnectIndication(pCon, this);

}

/*
 *	CNetTcpAcceptor
 */	


CNetTcpAcceptor::CNetTcpAcceptor (INetAcceptorSink *pSink)
{
	m_pSink = pSink;
	m_pApt = new CTransConTcpAcceptor(this);
	if (m_pApt->Init() != 0)
	{
		delete m_pApt;
		m_pApt = NULL;
	}
}

CNetTcpAcceptor::~CNetTcpAcceptor()
{
	if (m_pApt)
	{
		m_pApt->Clean();
		delete m_pApt;
		m_pApt = NULL;
	}
}
int CNetTcpAcceptor::StartListen(unsigned short wPort, unsigned long dwLocalAddr)
{
	VP_TRACE_INFO("CNetTcpAcceptor::StartListen port=%d",wPort);
	struct in_addr addr;
	addr.s_addr = htonl(dwLocalAddr);

	if (m_pApt)
		return m_pApt->StartListen(inet_ntoa(addr), wPort);
	else
		return -1;

}

int CNetTcpAcceptor::StopListen()
{
	if (m_pApt)
		return m_pApt->StopListen();
	return -1;
}

int CNetTcpAcceptor::OnConnectIndication (ITransCon *pTransCon)
{
	CNetTcpConnection *pCon = new CNetTcpConnection(NULL, pTransCon);
	pTransCon->SetSink(pCon);
	return m_pSink->OnConnectIndication(pCon, this);
}
