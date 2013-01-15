#include "CmBase.h"
#include "transconudp.h"
#include "NetworkUdpCon.h"
#include "transconmanager.h"
#include "transconpdu.h"
#include "TraceLog.h"
//TODO:FIELD
static void ShowDebugInfo( int nLevel, char* szMessage, ... )
{
/*
	
	char szFullMessage[MAX_PATH];
	char szFormatMessage[MAX_PATH];
	
	// format message
	va_list ap;
	va_start(ap, szMessage);
	_vsnprintf( szFormatMessage, MAX_PATH, szMessage, ap);
	va_end(ap);
	strncat( szFormatMessage, "\n", MAX_PATH);
	strcpy( szFullMessage, szFormatMessage );
	OutputDebugStringA( szFullMessage );
*/
}
extern "C"
{
	INetAcceptor *CreateUdpAcceptor(INetAcceptorSink *pSink)
	{
		if (pSink == NULL)
			return NULL;

		return new CNetUdpAcceptor(pSink);
	}
	INetConnection *CreateRawUdpCon(INetConnectionSink *pSink, 
		unsigned long dwLocalAddr, unsigned short wLocalPort)
	{
		if (pSink == NULL)
			return NULL;
		return new CNetRawUdpConnection(pSink, dwLocalAddr, wLocalPort);
	}

	INetConnection *CreateUdpCon(INetConnectionSink *pSink)
	{
		if (pSink == NULL)
			return NULL;
		return new CNetUdpConnection(pSink);
	}
	
}
/*
 *	CNetRawUdpConnection
 */
CNetRawUdpConnection::CNetRawUdpConnection(INetConnectionSink *pSink, 
										   unsigned long dwLocalAddr, 
										   unsigned short wLocalPort):m_cUdpSocket(this)
{
	CInetAddr cAddr(dwLocalAddr, wLocalPort);
	
	if (m_cUdpSocket.Listen(cAddr, 65536) != 0)
	{
		m_bBindSuccess = FALSE;
	}
	else
		m_bBindSuccess = TRUE;

	m_dwLocalAddr = dwLocalAddr;
	m_wLocalPort = wLocalPort;
	m_pSink = pSink;
	m_dwRemoteAddr = 0;
	m_wRemotePort = 0;

}

CNetRawUdpConnection::~CNetRawUdpConnection()
{
}

int CNetRawUdpConnection::Connect(
			unsigned long	dwAddr, 
			unsigned short	wPort, 
			int				nType,
			void			*pProxySetting
			)
{
	if (!m_bBindSuccess)
		return NETWORK_REASONBIND_ERROR;
	m_dwRemoteAddr = dwAddr;
	m_wRemotePort = wPort;
	return 0;
}

void CNetRawUdpConnection::Disconnect(int iReason)
{
	m_dwRemoteAddr = 0;
	m_wRemotePort = 0;
}

int CNetRawUdpConnection::SendData(unsigned char *pData, int nLen)
{
	if (m_dwRemoteAddr == 0)
		return -1;
	return SendTo(pData, nLen, m_dwRemoteAddr, m_wRemotePort);
}

int CNetRawUdpConnection::SendTo(unsigned char *pData, int nLen, 
		unsigned long dwAddr, unsigned short wPort)
{
	CInetAddr addr(dwAddr, wPort);
	CDataBlock *blk = new CDataBlock(nLen, 128);
	memcpy(blk->GetBuf(), pData, nLen);
	blk->Expand(nLen);
	int rt = m_cUdpSocket.SendTo(*blk, addr);
	blk->Release();
	return rt;

}

int CNetRawUdpConnection::SetOpt(unsigned long OptType, void *pParam)
{
	return -1;
}

int CNetRawUdpConnection::GetOpt(unsigned long OptType, void *pParam)
{
	unsigned long*pVal;
	switch (OptType)
	{
	case NETWORK_NET_OPT_TYPE_GET_TRANS_TYPE:
		pVal = (unsigned long*)pParam;
		*pVal = NETWORK_CONNECT_TYPE_UDP;
		return 0;
	case NETWORK_TRANSPORT_OPT_GET_FD:
		pVal = (unsigned long*)pParam;
		*pVal = (unsigned long)(m_cUdpSocket.GetHandle());
		return 0;
	case NETWORK_TRANSPORT_OPT_GET_PEER_ADDR:
		*((struct sockaddr_in**)pParam) = (struct sockaddr_in*)m_cInetAddr.GetPtr();
		return 0;
	default:
		return -1;
	}
}

int CNetRawUdpConnection::OnReceiveUdp(CDataBlock &aData, const CInetAddr &aAddr)
{
	m_cInetAddr = aAddr;
	return m_pSink->OnReceive(aData.GetBuf(), aData.GetLen(), this);
}

int CNetRawUdpConnection::OnCloseUdp(int aErr)
{
	return m_pSink->OnDisconnect(aErr, this);
}
	
/*
 *	CNetUdpConnection
 */	

CNetUdpConnection::CNetUdpConnection(INetConnectionSink *pSink)
{
	m_pSink = pSink;
	memset(m_dwCBTypes, 0, sizeof(DWORD)*MAX_WAIT_CMD);
	memset(m_dwCBDatas, 0, sizeof(DWORD)*MAX_WAIT_CMD);

	CUdpConTransCon *pTrans = new CUdpConTransCon(this);
	if (pTrans->Init() != 0)
	{
		delete pTrans;
		pTrans = NULL;
	}
	m_pTrans = pTrans;
	m_pApt = NULL;
	m_bConnected = 0;

	m_nCommandRecvSeq = 0;
	m_nCommandPutSeq = 0;
	m_nPutIndex = 0;
	m_nSendIndex = 0;
	memset(m_pCommands, 0, sizeof(m_pCommands));
	
}

CNetUdpConnection::CNetUdpConnection(INetConnectionSink *pSink, ITransCon *pTransCon)
{
	memset(m_dwCBTypes, 0, sizeof(DWORD)*MAX_WAIT_CMD);
	memset(m_dwCBDatas, 0, sizeof(DWORD)*MAX_WAIT_CMD);
	m_pSink = pSink;
	m_pTrans = pTransCon;
	m_bConnected = 1;
	m_pApt = NULL;

	m_nCommandRecvSeq = 0;
	m_nCommandPutSeq = 0;
	m_nPutIndex = 0;
	m_nSendIndex = 0;
	memset(m_pCommands, 0, sizeof(m_pCommands));

}

CNetUdpConnection::CNetUdpConnection(INetConnectionSink *pSink, CNetUdpAcceptor *pApt)
{
	memset(m_dwCBTypes, 0, sizeof(DWORD)*MAX_WAIT_CMD);
	memset(m_dwCBDatas, 0, sizeof(DWORD)*MAX_WAIT_CMD);

	m_pTrans = NULL;
	m_pSink = pSink;
	m_pApt = pApt;
	m_bConnected = 0;

	m_nCommandRecvSeq = 0;
	m_nCommandPutSeq = 0;
	m_nPutIndex = 0;
	m_nSendIndex = 0;
	memset(m_pCommands, 0, sizeof(m_pCommands));
	
}

CNetUdpConnection::~CNetUdpConnection()
{
	if (m_pTrans)
	{
		delete m_pTrans;
		m_pTrans = NULL;
	}
	if (m_pApt)
	{
		m_pApt->UnRegisterConnection(m_cRemoteAddr);
		m_pApt = NULL;
	}
	for (int i = 0; i < MAX_WAIT_CMD; i++)
	{
		if (m_pCommands[i] != NULL)
		{
			(m_pCommands[i])->Release();
		}
	}
	m_bConnected = 0;
	
}

void CNetUdpConnection::SetLowTranConByApt(ITransCon *pTransCon)
{
	m_pTrans = pTransCon;
	if (pTransCon != NULL)
	{
		pTransCon->SetSink(this);
		m_bConnected = 1;		
		m_pSink->OnConnect(0, this);
	}
	else
	{
		m_bConnected = 0;		
		m_pSink->OnConnect(NETWORK_REASONCONNECT_TIMEOUT, this);
	}
	m_pApt = NULL;
}

void CNetUdpConnection::OnTransTimer()
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

int CNetUdpConnection::Connect(
	unsigned long	dwAddr, 
	unsigned short	wPort, 
	int				nType,
	void			*pProxySetting
	)
{
	struct in_addr inaddr;
	inaddr.s_addr = htonl(dwAddr);
	if (m_pApt != NULL)
	{
		CInetAddr addr(dwAddr, wPort);
		m_cRemoteAddr = addr;
		m_pApt->RegisterConnection(this, addr);
		return 0;
	}
	if (m_pTrans)
		return m_pTrans->Connect(inet_ntoa(inaddr), wPort, pProxySetting);
	else
		return -1;
	
}

void CNetUdpConnection::Disconnect(int iReason)
{
	m_bConnected = 0;
	
	if (m_pTrans)
		m_pTrans->Disconnect(iReason);

	if (m_pApt)
	{
		m_pApt->UnRegisterConnection(m_cRemoteAddr);
		m_pApt = NULL;
	}
}

int CNetUdpConnection::SendCommand(unsigned char *pData, int nLen)
{
	if(nLen > 1400)
	{
		printf("CNetUdpConnection::SendCommand len=%d > max_package_size \n",nLen);
		return -1;
	}
	if (!m_bConnected)
		return -1;
	if ((m_nPutIndex + 1)%MAX_WAIT_CMD ==  m_nSendIndex) {
		VP_TRACE_STATE("CNetUdpConnection::SendCommand fail putIndex=%d sendIndex=%d",m_nPutIndex,m_nSendIndex);
		return -1;
	}

	unsigned long index = htonl(m_nCommandPutSeq++);

	CDataBlock *blk = new CDataBlock(nLen+5, 128);
	m_pCommands[m_nPutIndex] = blk;
	(blk->GetBuf())[0] = NET_UDP_DATA_TYPE_CMD;
	memcpy(blk->GetBuf()+1, &index, 4);
	memcpy(blk->GetBuf()+5, pData, nLen);
	blk->Expand(nLen+5);
	blk->SetOrgToCur();
	/*
	 *	Frank Fix Bug
	 */
	m_dwCBDatas[m_nPutIndex] = 0;
	m_dwCBTypes[m_nPutIndex] = 0;
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
//TODO: xialj 2008-04-17 重载SendCommand,增加数据发送标记
int CNetUdpConnection::SendCommand(unsigned char *pData, int nLen, DWORD dwCallBackType, DWORD dwCallBackData)
{

	if (!m_bConnected)
		return -1;
	if ((m_nPutIndex + 1)%MAX_WAIT_CMD ==  m_nSendIndex) {
		VP_TRACE_STATE("CNetUdpConnection::SendCommand fail putIndex=%d sendIndex=%d",m_nPutIndex,m_nSendIndex);
		return -1;
	}

	//TODO: xialj 设置标记
	m_dwCBDatas[m_nPutIndex] = dwCallBackData;
	m_dwCBTypes[m_nPutIndex] = dwCallBackType;

	unsigned long index = htonl(m_nCommandPutSeq++);

	CDataBlock *blk = new CDataBlock(nLen+5, 128);
	m_pCommands[m_nPutIndex] = blk;
	(blk->GetBuf())[0] = NET_UDP_DATA_TYPE_CMD;
	memcpy(blk->GetBuf()+1, &index, 4);
	memcpy(blk->GetBuf()+5, pData, nLen);
	blk->Expand(nLen+5);
	blk->SetOrgToCur();
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


int CNetUdpConnection::SendData(unsigned char *pData, int nLen)
{
	if (!m_bConnected)
		return -1;
	if (m_pTrans)
	{
		CDataBlock *blk = new CDataBlock(nLen+1, 128);
		(blk->GetBuf())[0] = NET_UDP_DATA_TYPE_DATA;
		memcpy(blk->GetBuf()+1, pData, nLen);
		blk->Expand(nLen+1);
		int rt = m_pTrans->SendData(blk);
		blk->Release();
		return rt;
	}
	else
		return -1;
}

int CNetUdpConnection::SendTo(unsigned char *pData, int nLen, 
	unsigned long dwAddr, unsigned short wPort)
{
	return -1;
}

int CNetUdpConnection::SetOpt(unsigned long OptType, void *pParam)
{
	if (m_pTrans)
		return m_pTrans->SetOpt(OptType, pParam);
	return -1;
}

int CNetUdpConnection::GetOpt(unsigned long OptType, void *pParam)
{
	if (m_pTrans)
		return m_pTrans->GetOpt(OptType, pParam);
	return -1;
}

int CNetUdpConnection::OnConnect(int iReason)
{
	if (iReason == 0)
		m_bConnected = 1;
	return m_pSink->OnConnect(iReason, this);
}

int CNetUdpConnection::OnDisconnect(int iReason)
{
	m_bConnected = 0;
	//TODO:xialj 08-04-18
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
			/*
			 *	Frank, Remove new buffer
			 */
			m_pSink->OnSend(REASON_SENDCOMMADN_TIMEOUT, 
				m_pCommands[nSendIndex]->GetBuf() + 5,
				m_pCommands[nSendIndex]->GetLen()-5,
				m_dwCBTypes[nSendIndex],
				m_dwCBDatas[nSendIndex],
				this);

		}
		return m_pSink->OnDisconnect(iReason, this);
	}

	return 0;	
}

int CNetUdpConnection::OnReceive(CDataBlock *pData)
{
	unsigned char type = (pData->GetBuf())[0];
	unsigned long index;
	switch (type)
	{
	case NET_UDP_DATA_TYPE_DATA:
	{
		return m_pSink->OnReceive(pData->GetBuf()+1, pData->GetLen()-1, this);
	}
	break;

	case NET_UDP_DATA_TYPE_CMD:
	{
		memcpy(&index, pData->GetBuf()+1, 4);
		index = ntohl(index);
		if (index > m_nCommandRecvSeq)
		{
			m_nCommandRecvSeq = index;
		}
		/*
		 *	Send Reponse
		 */
		CDataBlock *blk = new CDataBlock(5, 128);
		(blk->GetBuf())[0] = NET_UDP_DATA_TYPE_RESP;
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
	case NET_UDP_DATA_TYPE_RESP:
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
	default:
		WARNINGTRACE("Invalid packet type");
		break;
	}
	return m_pSink->OnReceive(pData->GetBuf(), pData->GetLen(), this);
}

int CNetUdpConnection::OnSend()
{
	return m_pSink->OnSend(this);
}

/*
 *	CNetUdpAcceptor
 */

CNetUdpAcceptor::CNetUdpAcceptor (INetAcceptorSink *pSink)
{
	m_pSink = pSink;
	m_pUdpApt = new CTransConUdpAcceptor(this);
	if (m_pUdpApt->Init() != 0)
	{
		delete m_pUdpApt;
		m_pUdpApt = NULL;
	}
	m_pTimer = new CNetTimer(this);
	m_pTimer->Schedule(1000);
}

CNetUdpAcceptor::~CNetUdpAcceptor()
{
	if (m_pUdpApt)
		delete m_pUdpApt;

	if (m_pTimer)
	{
		m_pTimer->Cancel();
		delete m_pTimer;
		m_pTimer = NULL;
	}

}

void CNetUdpAcceptor::OnTimer(void *pArg, INetTimer *pTimer)
{
    m_connections.REMOVE_IF( CFOCheckRetry( m_pUdpApt ) );
}

int CNetUdpAcceptor::StartListen(unsigned short wPort, unsigned long dwLocalAddr)
{
	struct in_addr inaddr;
	inaddr.s_addr = htonl(dwLocalAddr);
	
	if (m_pUdpApt)
		return m_pUdpApt->StartListen(inet_ntoa(inaddr), wPort);
	else
		return -1;
}

int CNetUdpAcceptor::StopListen()
{
	if (m_pUdpApt)
		return m_pUdpApt->StopListen();
	else
		return -1;
}

int CNetUdpAcceptor::OnConnectIndication (ITransCon *pTransCon)
{
	DWORD dwAddr;
	WORD  wPort;
	const sockaddr_in *pAddr = NULL;

	pTransCon->GetOpt(NETWORK_TRANSPORT_OPT_GET_PEER_ADDR, &pAddr);
	if (pAddr != NULL)
	{
		dwAddr = ntohl(pAddr->sin_addr.s_addr);
		wPort = ntohs(pAddr->sin_port);
		CInetAddr aAddr(dwAddr, wPort);

                CONNECTION* pFind = m_connections.Remove( aAddr );
                if ( pFind )
                {
                    pFind->pCon->SetLowTranConByApt( pTransCon );
                    delete pFind;
                    pFind = NULL;
                    return 0;
                }
	}
	CNetUdpConnection *pNetUdpCon = new CNetUdpConnection(NULL, pTransCon);
	pTransCon->SetSink(pNetUdpCon);
	return m_pSink->OnConnectIndication(pNetUdpCon, this);
}

void CNetUdpAcceptor::RegisterConnection(CNetUdpConnection *pCon, CInetAddr &aAddr)
{

	ShowDebugInfo(0, "Connect to  %s:%d\n", aAddr.GetHostAddr(), aAddr.GetPort());
	
	m_pUdpApt->RemoveTransCon(aAddr);
        CONNECTION* pFind = m_connections.Find( aAddr );
        if ( pFind )
            return;

	pFind = new CONNECTION;
	pFind->pCon = pCon;
	pFind->addr = aAddr;
	pFind->retries = 40;

        m_connections.Add( aAddr, pFind );
}

void CNetUdpAcceptor::UnRegisterConnection(CInetAddr &aAddr)
{
    m_connections.Delete( aAddr );
}

bool CFOCheckRetry::operator()(CNetUdpAcceptor::CONNECTION* pConnection)
{
    if ( pConnection->retries > 0 )
    {
	CTransConPduUdpBase udpBase(0, 0, 0, TransCon_Pdu_Type_UDP_Syn_0);
	CDataBlock *aData = new CDataBlock(32, 0);
	CByteStream stream(aData->GetBuf(), 0, udpBase.GetLen()+6);
	udpBase.Encode(stream);
	stream<<(DWORD)(ntohl(pConnection->addr.GetPtr()->sin_addr.s_addr));
	stream<<(WORD)(ntohs(pConnection->addr.GetPtr()->sin_port));
	aData->Expand(udpBase.GetLen()+6);
	m_pApt->RawSentTo(*aData, pConnection->addr);
	aData->Release();
	--(pConnection->retries);
   }

   if ( pConnection->retries <= 0 )
   {
       pConnection->pCon->SetLowTranConByApt(NULL);
       return true; //delete this item
   }

   return false;
}
