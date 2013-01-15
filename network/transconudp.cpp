/*------------------------------------------------------------------------*/
/*                                                                        */
/*  UDP transport connection                                              */
/*  transconudp.cpp                                                       */
/*                                                                        */
/*  Implementation of UDP transport connection                            */
/*                                                                        */
/*  All rights reserved                                                   */
/*                                                                        */
/*------------------------------------------------------------------------*/
#include "CmBase.h"
#include "transconudp.h"
#include "transconpdu.h"
#include "Addr.h"
#include "NetworkImpl.h"
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

/*########################################################################*/
/* CTransConUdpAcceptor*/
CTransConUdpAcceptor::CTransConUdpAcceptor(ITransConAcceptorSink *pSink,
										   DWORD dwType)
	: m_pSink(pSink)
	, m_cUdpSocket(this)
	, m_pTimer(NULL)
	, m_nLastTimerStart(0)
	, m_nTimerCount(TRANS_CON_UDP_HASH_SIZE/100 + 1)
	, m_nConnectionCount(0)
{
	IM_ASSERT(pSink);
	IM_ASSERT(dwType == TYPE_UDP);
	//m_pSink = pSink;
	//m_nTimerCount = TRANS_CON_UDP_HASH_SIZE/100 + 1;
	//m_pTimer = NULL;
	//m_nConnectionCount = 0;
}

CTransConUdpAcceptor::~CTransConUdpAcceptor()
{
	Clean();
	if (m_pTimer != NULL) {
		m_pTimer->Cancel();
		delete m_pTimer;
		m_pTimer = NULL;
	}
}

int CTransConUdpAcceptor::Init()
{
	return 0;
}

void CTransConUdpAcceptor::OnTimer(void *pArg, INetTimer *pTimer)
{
	//important: need two iterators.
	list<CUdpReactiveTransCon*>::iterator pIt, pIt1;
	CUdpReactiveTransCon* pUdpCon = NULL;

    //TODO:FIELD
	//static int printcount;
	//printcount++;
	//if (printcount %100 == 0) {
	//	printf("Total UDP connection count %d\n", m_nConnectionCount);
	//}

	for (int i = 0; i < m_nTimerCount; i++) {
		if (m_pTranConList[m_nLastTimerStart].empty()) {
			++m_nLastTimerStart;
			m_nLastTimerStart = m_nLastTimerStart%TRANS_CON_UDP_HASH_SIZE;
			continue;
		}

		for (pIt=(m_pTranConList[m_nLastTimerStart]).begin(); pIt!=(m_pTranConList[m_nLastTimerStart]).end();){
			pIt1 = pIt;
			pIt1++;
			pUdpCon = (CUdpReactiveTransCon*)(*pIt);
			if (pUdpCon) {
				pUdpCon->OnTick();
			} else {
				VP_TRACE_ERROR("CTransConUdpAcceptor::OnTimer()"
					" pUdpCon==NULL");
			}
			pIt = pIt1;
		}
		++m_nLastTimerStart;
		m_nLastTimerStart = m_nLastTimerStart%TRANS_CON_UDP_HASH_SIZE;
	}
}

int CTransConUdpAcceptor::Clean()
{
	list<CUdpReactiveTransCon*>::iterator pIt;
	CUdpReactiveTransCon* pUdpCon = NULL;

	m_nConnectionCount = 0;
	for (int i = 0; i < TRANS_CON_UDP_HASH_SIZE; i++) {
		for (pIt=(m_pTranConList[i]).begin(); pIt!=(m_pTranConList[i]).end(); ++pIt) {
			pUdpCon = (CUdpReactiveTransCon *)(*pIt);
			if (pUdpCon) {
				pUdpCon->DisconnectByApt(REASON_SOCKET_ERROR);
			} else {
				VP_TRACE_ERROR("CTransConUdpAcceptor::Clean()"
					" pUdpCon==NULL");
			}
		}
		(m_pTranConList[i]).clear();
	}
	return 0;
}

int CTransConUdpAcceptor::StartListen(const char *pAddr,
									  unsigned short wPort,
									  unsigned short bPortAutoSearch)
{
	Clean();
	m_cUdpSocket.Close();

	m_pTimer = new CNetTimer(this);
	m_pTimer->Schedule(100);
	m_nLastTimerStart = 0;

	if (wPort == 0) {
		VP_TRACE_ERROR("CTransConUdpAcceptor::StartListen()"
			" Network CTransConUdpAcceptor::StartListen: port invalid");
		return -1;
	}

	if (!bPortAutoSearch) {
		CInetAddr cAddr(pAddr, wPort);
		if (m_cUdpSocket.Listen(cAddr, 65536) == 0) {
			memcpy (&m_cAddr, cAddr.GetPtr(), sizeof(sockaddr_in));
			return wPort;
		} else {
			VP_TRACE_ERROR("CTransConUdpAcceptor::StartListen()"
				" Network CTransConUdpAcceptor::StartListen: bind failed1");

			return -1;
		}
	}

	for(int i = 0; i <5; i++) {
		CInetAddr cAddr(pAddr, wPort+i);
		if (m_cUdpSocket.Listen(cAddr, 65536) == 0) {
			m_wPort = wPort+i;
			memcpy (&m_cAddr, cAddr.GetPtr(), sizeof(sockaddr_in));
			return wPort+i;
		}
	}
	m_cUdpSocket.SetBuffer(0x200000);
	VP_TRACE_ERROR("CTransConUdpAcceptor::StartListen()"
		" Network CTransConUdpAcceptor::StartListen: bind failed2");
	return -1;
}

int CTransConUdpAcceptor::StopListen(int iReason)
{
	Clean();

	if (m_pTimer != NULL) {
		m_pTimer->Cancel();
		delete m_pTimer;
		m_pTimer = NULL;
	}
	m_cUdpSocket.Close();
	return 0;
}

void CTransConUdpAcceptor::RawSentTo(CDataBlock	&aData, CInetAddr &cAddr)
{
	m_cUdpSocket.SendTo(aData, cAddr);
}

int CTransConUdpAcceptor::OnReceiveUdp(CDataBlock &aData, const CInetAddr &aAddr)
{
	CTransConPduUdpBase pUdpPdu;
	CUdpReactiveTransCon* pTransCon = NULL;
	if (aData.GetLen() < pUdpPdu.GetLen()) {
		VP_TRACE_ERROR("CTransConUdpAcceptor::OnReceiveUdp()"
			" Network CTransConUdpAcceptor::OnReceiveUdp receive invalid udp packet aData.GetLen() < pUdpPdu.GetLen()");
		return 0;
	}
	CByteStream stream(aData.GetBuf(), 0, aData.GetLen());
	pUdpPdu.Decode(stream);

	if (pUdpPdu.GetType() == TransCon_Pdu_Type_UDP_Syn || 
		pUdpPdu.GetType() == TransCon_Pdu_Type_UDP_Syn_0) {
		CInetAddr addr(pUdpPdu.GetConnectionId(), pUdpPdu.GetSubConId());
		if (pUdpPdu.GetConnectionId() == 0) {
			pTransCon = GetTransCon(*(aAddr.GetPtr()));
		} else {
			pTransCon = GetTransCon(*(addr.GetPtr()));
		}
			
		if (pTransCon == NULL) {
			if (pUdpPdu.GetConnectionId() == 0) {
				pTransCon = new CUdpReactiveTransCon(this, NULL,aAddr);
			} else {
				pTransCon = new CUdpReactiveTransCon(this, NULL,addr);
			}

			IM_ASSERT (pTransCon);
			if (pTransCon->Init() != 0) {
				VP_TRACE_ERROR("CTransConUdpAcceptor::OnReceiveUdp()"
					" Network CTransConUdpAcceptor::OnReceiveUdp: create trancon failed pTransCon->Init() != 0");
				delete pTransCon;
				return 0;
			}
			
			pTransCon->OnReceive(aData, aAddr);
			DWORD dwHash = GetHashCode(*(aAddr.GetPtr()));
			m_nConnectionCount++;
			(m_pTranConList[dwHash]).push_front(pTransCon);
			return 0;
		} else {
			pTransCon->OnReceive(aData, aAddr);
			return 0;
		}
	} else {
		if (pUdpPdu.GetConnectionId() == 0) {
			//todo qinjf 频繁出现
			//VP_TRACE_WARNING("CTransConUdpAcceptor::OnReceiveUdp()"
			//    " Network CTransConUdpAcceptor::OnReceiveUdp: Invalid packet pUdpPdu.GetConnectionId() == 0");
			return 0;
		}
		CInetAddr addr(pUdpPdu.GetConnectionId(), pUdpPdu.GetSubConId());
		pTransCon = GetTransCon(*(addr.GetPtr()));
		if (pTransCon != NULL) {
			pTransCon->OnReceive(aData, aAddr);
		} else {
			//todo qinjf 频繁出现
			//VP_TRACE_STATE("CTransConUdpAcceptor::OnReceiveUdp()"
			//	" Network CTransConUdpAcceptor::OnReceiveUdp: Invalid packet pTransCon == NULL");
		}
		return 0;
	}
}

int CTransConUdpAcceptor::OnCloseUdp(int aErr)
{
	Clean();
	return 0;
}

CClientSocketUDP *CTransConUdpAcceptor::GetUdpSocket(void)
{
	return &m_cUdpSocket;
}

CUdpReactiveTransCon *CTransConUdpAcceptor::GetTransCon(const sockaddr_in &pAddr)
{
	DWORD dwHash = GetHashCode(pAddr);
	list<CUdpReactiveTransCon*>::iterator pIt;
	CUdpReactiveTransCon* pUdpCon = NULL;
	
	for (pIt=(m_pTranConList[dwHash]).begin(); pIt!=(m_pTranConList[dwHash]).end(); ++pIt) {
		pUdpCon = (CUdpReactiveTransCon*)(*pIt);
		if (pUdpCon && pUdpCon->CompareAddr(pAddr)) {
			return pUdpCon;
		} else {
			//todo qinjf 频繁出现，影响性能测试
			//VP_TRACE_ERROR("CTransConUdpAcceptor::GetTransCon()"
			//	" pUdpCon==NULL");
		}
	}
	return NULL;
}

void CTransConUdpAcceptor::RemoveTransCon(CInetAddr &cAddr)
{
	CUdpReactiveTransCon *pTransCon = GetTransCon(*(cAddr.GetPtr()));
	if (pTransCon != NULL) {
		pTransCon->DisconnectByApt(REASON_UNKNOWN_ERROR);
	}
}

void CTransConUdpAcceptor::RemoveTransCon(CUdpReactiveTransCon *pTransCon)
{
	DWORD dwHash = GetHashCode(*(pTransCon->GetPeerAddr()));
	m_nConnectionCount --;
	
	(m_pTranConList[dwHash]).remove(pTransCon);

}

ITransConAcceptorSink *CTransConUdpAcceptor::GetSink()
{
	return m_pSink;
}

DWORD CTransConUdpAcceptor::GetHashCode(const sockaddr_in &pAddr)
{
	unsigned long dwAddr = pAddr.sin_addr.s_addr;

	return (dwAddr%TRANS_CON_UDP_HASH_SIZE+pAddr.sin_port%TRANS_CON_UDP_HASH_SIZE)%TRANS_CON_UDP_HASH_SIZE;
}

/*########################################################################*/
/* CUdpReactiveTransCon*/
CUdpReactiveTransCon::CUdpReactiveTransCon(CTransConUdpAcceptor *pApt, 
										   ITransConSink *pSink, 
										   const CInetAddr &aAddr)
{
	IM_ASSERT(pApt);
	m_pTransApt = pApt;
	m_pSink = pSink;
	m_wNextSeqExpect = 0;
	//Get a random initial sequence
	m_wCurrentSndSeq = (WORD)((DWORD)&m_wCurrentSndSeq);
	m_bDataReceived = m_bDataSended = FALSE;
	m_wTickCount = 0;
	m_cFirstAddr = aAddr;
	m_cInetAddr = aAddr;
	m_dwConnectionId = 0;
	m_wConSubId = 0;
#ifdef TRANS_CON_UDP_SORT
	m_pReorderArry[0] = m_pReorderArry[1] = NULL;
#endif
}

CUdpReactiveTransCon::~CUdpReactiveTransCon()
{
	Clean();
}

int CUdpReactiveTransCon::Init()
{
	m_wState = TRANS_CON_UDP_RCT_STATE_STARTED;
	return 0;
}

int CUdpReactiveTransCon::Clean()
{
	m_wState = TRANS_CON_UDP_STATE_DISCONNECTED;

	if (m_pTransApt != NULL) {
		m_pTransApt->RemoveTransCon(this);
		m_pTransApt = NULL;
	}

#ifdef TRANS_CON_UDP_SORT
	if (m_pReorderArry[0] != NULL)
	{
		m_pReorderArry[0]->Release();
		m_pReorderArry[0] = NULL;
	}
	if (m_pReorderArry[1] != NULL)
	{
		m_pReorderArry[1]->Release();
		m_pReorderArry[1] = NULL;
	}
#endif
	return 0;
}

void CUdpReactiveTransCon::OnTick()
{
	if(m_pSink) {
		m_pSink->OnTransTimer();
	}
	if (m_wState == TRANS_CON_UDP_RCT_STATE_WAITACK) {
		if (++m_wTickCount> 3) {//hand shake time out
			Clean();
			delete this;
			return;
		}
		//Resend the ACK and wait reply
		CDataBlock *pBlk = BuildAck1Pdu();
		m_pTransApt->GetUdpSocket()->SendTo(*pBlk, m_cInetAddr);
		pBlk->Release();
	} else if (m_wState == TRANS_CON_UDP_STATE_CONNECTED) {
		if (!m_bDataSended) {
			CDataBlock *pBlk = BuildKeepAlivePdu();
			m_pTransApt->GetUdpSocket()->SendTo(*pBlk, m_cInetAddr);
			pBlk->Release();
		} else {
			m_bDataSended = FALSE;
		}

		if (++m_wTickCount >= 4) {
			if (!m_bDataReceived) {
				VP_TRACE_INFO("CUdpReactiveTransCon::OnTick()"
					" Network CUdpReactiveTransCon::OnTick: KEEPALIVE_TIMEOUT,"
					" m_pSink=%d this=%d",
					m_pSink,
					this);
				Clean();
				if (m_pSink) {
					m_pSink->OnDisconnect(REASON_KEEPALIVE_TIMEOUT);
				} else {
					GetTransConManager()->DestroyTransCon(this);
				}
				return;
			}
			m_bDataReceived = FALSE;
			m_wTickCount    = 0;
		}
	} else {
		VP_TRACE_ERROR("CUdpReactiveTransCon::OnTick()"
			" unknown state=%d",
			m_wState);
	}
}

int CUdpReactiveTransCon::Connect(const char *pAddr, 
								  unsigned short wPort,
								  void *pProxySetting,
								  int nType)
{
	VP_TRACE_ERROR("CUdpReactiveTransCon::Connect()"
		" Network CTransConUdpAcceptor::Connect: reactive con,cannot connect");

	return -1;
}

void CUdpReactiveTransCon::Disconnect(int iReason)
{
	if (m_pTransApt) {
		CDataBlock *pBlk = BuildFinPdu();
		m_pTransApt->GetUdpSocket()->SendTo(*pBlk, m_cInetAddr);
		pBlk->Release();
	}
	Clean();
}

int CUdpReactiveTransCon::SendData(CDataBlock *pData)
{
	BuildDataPdu(pData);

	m_bDataSended = TRUE;
	return m_pTransApt->GetUdpSocket()->SendTo(*pData, m_cInetAddr);
}

void CUdpReactiveTransCon::SetSink(ITransConSink *pSink)
{
	m_pSink = pSink;
}

void CUdpReactiveTransCon::OnReceive(CDataBlock &aData, const CInetAddr &aAddr)
{
	BYTE *pData     = aData.GetBuf();
	DWORD dwDataLen = aData.GetLen();

	WORD wType = CTransConPduUdpBase::PeekType(pData);
	CTransConPduUdpBase pdu;

	if (dwDataLen < pdu.GetLen()) {
		VP_TRACE_WARNING("CUdpReactiveTransCon::OnReceive()"
			" Network CUdpReactiveTransCon::OnReceive: Invalid packet");
		return;
	}

	m_cInetAddr = aAddr;
	switch (wType)
	{
	case TransCon_Pdu_Type_UDP_Syn_0:
		{
			if (m_wState == TRANS_CON_UDP_STATE_DISCONNECTED 
				|| m_wState == TRANS_CON_UDP_STATE_CONNECTED)
				return;
/*
 *	Get Connection ID
 */
			aData.Advance(pdu.GetLen());
			CByteStream stream(aData.GetBuf(),0, aData.GetLen());
			stream>>m_dwConnectionId;
			stream>>m_wConSubId;
/*
 *	Send Sync and Ack Pdu
 */
			CDataBlock *pBlk = BuildSynPdu();
			m_pTransApt->GetUdpSocket()->SendTo(*pBlk, m_cInetAddr);
			pBlk->Release();
			pBlk = BuildAck1Pdu();
			m_pTransApt->GetUdpSocket()->SendTo(*pBlk, m_cInetAddr);
			pBlk->Release();

/*
 *	Set State, Schedule timer
 */
			if (m_wState == TRANS_CON_UDP_RCT_STATE_STARTED)
			{
				m_wState = TRANS_CON_UDP_RCT_STATE_WAITACK;
				m_wTickCount = 0;
			}
		}
		return;
	case TransCon_Pdu_Type_UDP_Syn:
		{
			if (m_wState == TRANS_CON_UDP_STATE_DISCONNECTED 
				|| m_wState == TRANS_CON_UDP_STATE_CONNECTED)
				return;
/*
 *	Get Connection Id
 */
			if (m_dwConnectionId == 0)
			{
				aData.Advance(pdu.GetLen());
				CByteStream stream(aData.GetBuf(),0, aData.GetLen());
				stream>>m_dwConnectionId;
				stream>>m_wConSubId;
			}
/*
 *	Send Ack
 */			
			CDataBlock *pBlk = BuildAck1Pdu();
			m_pTransApt->GetUdpSocket()->SendTo(*pBlk, m_cInetAddr);
			pBlk->Release();

/*
 *	Schedule Timer and set state
 */
			if (m_wState == TRANS_CON_UDP_RCT_STATE_STARTED)
			{
				m_wState = TRANS_CON_UDP_RCT_STATE_WAITACK;
				m_wTickCount = 0;
			}
		}
	return;
	case TransCon_Pdu_Type_UDP_Ack1:
		{
/*
 *	Send Ack
 */			
			CDataBlock *pBlk = BuildAckPdu();
			m_pTransApt->GetUdpSocket()->SendTo(*pBlk, m_cInetAddr);
			pBlk->Release();
			
			if (m_wState == TRANS_CON_UDP_RCT_STATE_WAITACK)
			{
				CTransConPduUdpBase ackPdu;
				CByteStream stream(pData,0, dwDataLen);
				ackPdu.Decode(stream);
				m_wNextSeqExpect = ackPdu.GetSequence();
				
				m_wTickCount = 0;
				m_wState = TRANS_CON_UDP_STATE_CONNECTED;
				ShowDebugInfo(0, "Onconnection Indication %s:%d\n", m_cInetAddr.GetHostAddr(), m_cInetAddr.GetPort());
				m_pTransApt->GetSink()->OnConnectIndication(this);
			}
		}
		break;
	case TransCon_Pdu_Type_UDP_Ack:
		{
			if (m_wState == TRANS_CON_UDP_RCT_STATE_WAITACK)
			{
				CTransConPduUdpBase ackPdu;
				CByteStream stream(pData,0, dwDataLen);
				ackPdu.Decode(stream);
				m_wNextSeqExpect = ackPdu.GetSequence();

				m_wTickCount = 0;
				m_wState = TRANS_CON_UDP_STATE_CONNECTED;
				ShowDebugInfo(0, "Onconnection Indication %s:%d\n", m_cInetAddr.GetHostAddr(), m_cInetAddr.GetPort());
				m_pTransApt->GetSink()->OnConnectIndication(this);
			}
			
		}
	break;

	case TransCon_Pdu_Type_UDP_Keepalive:
		{
			m_bDataReceived = TRUE;
		}
		//We receive data or keep alive when wait for ack. It means peer
		//accept the connection, so we accept it too.
		if (m_wState == TRANS_CON_UDP_RCT_STATE_WAITACK)
		{
			CTransConPduUdpBase keepPdu;
			CByteStream stream(pData,0, dwDataLen);
			keepPdu.Decode(stream);
			m_wNextSeqExpect = keepPdu.GetSequence();

			m_wTickCount = 0;
			m_wState = TRANS_CON_UDP_STATE_CONNECTED;
			m_pTransApt->GetSink()->OnConnectIndication(this);
		}
		break;
	case TransCon_Pdu_Type_UDP_Data:
		if (m_wState == TRANS_CON_UDP_RCT_STATE_WAITACK)
		{
			CTransConPduUdpBase dataPdu;
			CByteStream stream(pData,0, dwDataLen);
			dataPdu.Decode(stream);
			m_wNextSeqExpect = dataPdu.GetSequence();

			m_wTickCount = 0;
			m_wState = TRANS_CON_UDP_STATE_CONNECTED;
			m_pTransApt->GetSink()->OnConnectIndication(this);
			//Not return, do next step
		}
		{
			if (m_wState != TRANS_CON_UDP_STATE_CONNECTED)
				return;
			m_bDataReceived = TRUE;
			CTransConPduUdpBase dataPdu;
			CByteStream stream(pData, 0, dwDataLen);
			dataPdu.Decode(stream);
			aData.Advance(dataPdu.GetLen());

			if (aData.GetLen() <= 0)
			{
				WARNINGTRACE("Network CUdpReactiveTransCon::OnReceive: Invalid data");
				return;
			}
#ifdef TRANS_CON_UDP_SORT
#else
			if (m_pSink)
				m_pSink->OnReceive(&aData);	
#endif
		}
		break;
	case TransCon_Pdu_Type_UDP_FIN:
		Clean();
		if (m_pSink)
			m_pSink->OnDisconnect(REASON_PEER_DISCONNECT);
		break;		
	default:
		WARNINGTRACE("Network CUdpReactiveTransCon::OnReceive: invalid packet");
		break;
	}

}

void CUdpReactiveTransCon::DisconnectByApt(int iReason)
{
	//The acceptor will delete the transcon in it's list
	if (m_pTransApt)
	{
		CDataBlock *pBlk = BuildFinPdu();
		m_pTransApt->GetUdpSocket()->SendTo(*pBlk, m_cInetAddr);
		pBlk->Release();
	}
	
	m_pTransApt = NULL;
	Clean();
	if (m_pSink != NULL)
	{
		m_pSink->OnDisconnect(iReason);
	}
	else
	{
		GetTransConManager()->DestroyTransCon(this);
	}
}

//return TRUE if addr equal
BOOL CUdpReactiveTransCon::CompareAddr(const sockaddr_in &pAddr)
{

	return (((m_cFirstAddr.GetPtr())->sin_port == pAddr.sin_port) && 
			((m_cFirstAddr.GetPtr())->sin_addr.s_addr == pAddr.sin_addr.s_addr));
}

sockaddr_in *CUdpReactiveTransCon::GetPeerAddr(void)
{
	return (sockaddr_in *)m_cFirstAddr.GetPtr();
}

CDataBlock *CUdpReactiveTransCon::BuildDataPdu(CDataBlock *pInBlk)
{
	if (pInBlk == NULL)
	{
		ERRTRACE("Network CUdpReactiveTransCon::BuildDataPdu: build failed");
		return NULL;
	}
	CTransConPduUdpData udpData(m_dwConnectionId, m_wConSubId, ++m_wCurrentSndSeq);
	pInBlk->Back(udpData.GetLen());
	CByteStream stream(pInBlk->GetBuf(), 0, udpData.GetLen());
	udpData.Encode(stream);
	return pInBlk;
}

CDataBlock *CUdpReactiveTransCon::BuildAck1Pdu()
{
	CTransConPduUdpBase udpBase(m_dwConnectionId, m_wConSubId,m_wCurrentSndSeq, TransCon_Pdu_Type_UDP_Ack1);
	CDataBlock *pBlk = new CDataBlock(32, 0);
	CByteStream stream(pBlk->GetBuf(), 0, udpBase.GetLen()+6);
	udpBase.Encode(stream);
	stream<<(DWORD)(ntohl(m_cFirstAddr.GetPtr()->sin_addr.s_addr));
	stream<<(WORD)(ntohs(m_cFirstAddr.GetPtr()->sin_port));
	pBlk->Expand(udpBase.GetLen()+6);
	return pBlk;
}

CDataBlock *CUdpReactiveTransCon::BuildAckPdu()
{
	CTransConPduUdpBase udpBase(m_dwConnectionId, m_wConSubId,m_wCurrentSndSeq, TransCon_Pdu_Type_UDP_Ack);
	CDataBlock *pBlk = new CDataBlock(32, 0);
	CByteStream stream(pBlk->GetBuf(), 0, udpBase.GetLen()+6);
	udpBase.Encode(stream);
	stream<<(DWORD)(ntohl(m_cFirstAddr.GetPtr()->sin_addr.s_addr));
	stream<<(WORD)(ntohs(m_cFirstAddr.GetPtr()->sin_port));
	pBlk->Expand(udpBase.GetLen()+6);
	return pBlk;
}

CDataBlock *CUdpReactiveTransCon::BuildSynPdu()
{
	CTransConPduUdpBase udpBase(m_dwConnectionId, m_wConSubId, m_wCurrentSndSeq, TransCon_Pdu_Type_UDP_Syn);
	CDataBlock *pBlk = new CDataBlock(32, 0);
	CByteStream stream(pBlk->GetBuf(), 0, udpBase.GetLen()+6);
	udpBase.Encode(stream);
	stream<<(DWORD)(ntohl(m_cFirstAddr.GetPtr()->sin_addr.s_addr));
	stream<<(WORD)(ntohs(m_cFirstAddr.GetPtr()->sin_port));
	pBlk->Expand(udpBase.GetLen()+6);
	return pBlk;
}

CDataBlock *CUdpReactiveTransCon::BuildFinPdu()
{
	CTransConPduUdpBase udpBase(m_dwConnectionId, m_wConSubId, m_wCurrentSndSeq, TransCon_Pdu_Type_UDP_FIN);
	CDataBlock *pBlk = new CDataBlock(32, 0);
	CByteStream stream(pBlk->GetBuf(), 0, udpBase.GetLen());
	udpBase.Encode(stream);
	pBlk->Expand(udpBase.GetLen());
	return pBlk;
}

CDataBlock *CUdpReactiveTransCon::BuildKeepAlivePdu()
{
	CTransConPduUdpBase udpBase(m_dwConnectionId, m_wConSubId, m_wCurrentSndSeq, TransCon_Pdu_Type_UDP_Keepalive);
	CDataBlock *pBlk = new CDataBlock(32, 0);
	CByteStream stream(pBlk->GetBuf(), 0, udpBase.GetLen());
	udpBase.Encode(stream);
	pBlk->Expand(udpBase.GetLen());
	return pBlk;

}

int CUdpReactiveTransCon::SetOpt(unsigned long OptType, void *pParam)
{
	ERRTRACE("Network CUdpReactiveTransCon::SetOpt: bad option");
	return -1;
}

int CUdpReactiveTransCon::GetOpt(unsigned long OptType, void *pParam)
{
	switch(OptType) {
	case NETWORK_TRANSPORT_OPT_GET_PEER_ADDR:
		m_cAddrTmp = m_cInetAddr;
		*((struct sockaddr_in**)pParam) = (struct sockaddr_in*)m_cAddrTmp.GetPtr();
		return 0;
		break;
	default:
		break;
	}
	ERRTRACE("Network CUdpReactiveTransCon::GetOpt:bad option");
	return -1;
}


/*########################################################################*/
/* CUdpTransCon*/
CUdpConTransCon::CUdpConTransCon(ITransConSink *pSink):m_cUdpSocket(this)
{
	IM_ASSERT (pSink);
	m_pSink = pSink;
	m_pTimer = NULL;
	//Get a random initial sequence;
	m_wCurrentSndSeq = (WORD)(DWORD)&m_wCurrentSndSeq;
	m_wNextSeqExpect = 0;
	m_wPort = 0;
	m_bDataReceived = FALSE;
	m_bDataSended = FALSE;
	m_wTickCount = 0;
	m_dwConnectionId = 0;
	m_wConSubId = 0;
}

CUdpConTransCon::~CUdpConTransCon()
{
	Clean();
}

int CUdpConTransCon::Init(void)
{
	m_wState = TRANS_CON_UDP_CON_STATE_STARTED;
	if (m_pSink == NULL)
	{
		ERRTRACE("Network CUdpConTransCon::Init:sink NULL");
		return -1;
	}
	return 0;
}

int CUdpConTransCon::Clean (void)
{
	m_wState = TRANS_CON_UDP_STATE_DISCONNECTED;

	m_cUdpSocket.Close();
	if (m_pTimer != NULL)
	{
		m_pTimer->Cancel();
		delete m_pTimer;
		m_pTimer = NULL;
	}
	return 0;
}

void CUdpConTransCon::OnTick(void)
{
	if (m_pSink != NULL)
		m_pSink->OnTransTimer();
	if (m_wState == TRANS_CON_UDP_STATE_CONNECTED)
	{
		if (!m_bDataSended)
		{
			CDataBlock *pBlk = BuildKeepAlivePdu();
			m_cUdpSocket.Send(*pBlk);
			pBlk->Release();
		}
		else
			m_bDataSended = FALSE;

		// budingc
	//	if (++m_wTickCount>=6)
		if (++m_wTickCount >= TRANS_CON_KEEP_ALIVE_MAX_NUMBER)
		{
			if (!m_bDataReceived)
			{
				VP_TRACE_INFO("Network CUdpConTransCon::OnTick: KEEPALIVE_TIMEOUT, m_pSink=%d this=%d",
					m_pSink,
					this);


				m_pTimer->Cancel();
				if (m_pSink)
					m_pSink->OnDisconnect(REASON_KEEPALIVE_TIMEOUT);
				else
					GetTransConManager()->DestroyTransCon(this);
				return;
			}
			m_bDataReceived = FALSE;
			m_wTickCount = 0;
		}
	}
	else if (m_wState == TRANS_CON_UDP_CON_STATE_WAITACK)
	{
		if (++m_wTickCount> 8)
		//hand shake time out
		{
			m_pTimer->Cancel();
			m_pSink->OnConnect(REASON_SERVER_UNAVAILABLE);
			return;
		}
		//Resend the syn and wait reply
		CDataBlock *pBlk = BuildSynPdu();
		m_cUdpSocket.Send(*pBlk);
		pBlk->Release();
		m_pTimer->Cancel();
		m_pTimer->Schedule(m_wTickCount*500);

	}
	else if (m_wState == TRANS_CON_UDP_STATE_DISCONNECTED)
	{
		m_pSink->OnConnect(REASON_UNKNOWN_ERROR);
		return;
	}
}

int CUdpConTransCon::Connect(const char	*pAddr, unsigned short wPort, 
							void *pProxySetting, int nType)
{
	IM_ASSERT(pAddr != NULL);
	CInetAddr cAddr(pAddr, wPort);
	m_cInetAddr = cAddr;
	Clean();
	if (m_cUdpSocket.Connect(cAddr, 65536) == 0)
	{
		CDataBlock *pBlk = BuildSynPdu();
		m_cUdpSocket.Send(*pBlk);
		pBlk->Release();
		m_wState = TRANS_CON_UDP_CON_STATE_WAITACK;
		m_wTickCount = 0;
		if (m_pTimer == NULL)
			m_pTimer = new CKeepAliveTimer(this);
		IM_ASSERT(m_pTimer);
		m_pTimer->Schedule(500);
		return 0;
	}
	else
	{
		if (m_pTimer == NULL)
			m_pTimer = new CKeepAliveTimer(this);
		IM_ASSERT(m_pTimer);
		m_wState = TRANS_CON_UDP_STATE_DISCONNECTED;
		m_pTimer->Schedule(1);
		return 0;
	}
}

void CUdpConTransCon::Disconnect(int iReason)
{
	Clean();
	m_wState = TRANS_CON_UDP_STATE_DISCONNECTED;
}

int CUdpConTransCon::SendData(CDataBlock *pData)
{
	if (pData == NULL)
		return 0;
	BuildDataPdu(pData);
	m_bDataSended = TRUE;
	return m_cUdpSocket.Send(*pData);
}

void CUdpConTransCon::SetSink(ITransConSink *pSink)
{
	m_pSink = pSink;
}

int CUdpConTransCon::OnReceiveUdp(CDataBlock &aData, const CInetAddr &aAddr)
{
	BYTE *pData = aData.GetBuf();
	DWORD dwDataLen = aData.GetLen();
	CTransConPduUdpBase pdu;

	if (dwDataLen <pdu.GetLen())
	{
		WARNINGTRACE("Network CUdpConTransCon::OnReceiveUdp: invalid packet");
		return 0;
	}
	WORD wType = CTransConPduUdpBase::PeekType(pData);

	switch (wType)
	{

	case TransCon_Pdu_Type_UDP_Ack1:
	case TransCon_Pdu_Type_UDP_Ack:
		{
			if (m_wState == TRANS_CON_UDP_CON_STATE_WAITACK)
			{
				CDataBlock *pBlk = BuildAckPdu();
				m_cUdpSocket.Send(*pBlk);
				pBlk->Release();

				CTransConPduUdpBase ackPdu;
				CByteStream stream(pData,0, dwDataLen);
				ackPdu.Decode(stream);
				m_wNextSeqExpect = ackPdu.GetSequence();
				stream>>m_dwConnectionId;
				stream>>m_wConSubId;
				m_wTickCount = 0;
				m_wState = TRANS_CON_UDP_STATE_CONNECTED;
				if (!m_pTimer)
				{
					m_pTimer = new CKeepAliveTimer(this);
				}
				m_pTimer->Schedule(TRANS_CON_KEEP_ALIVE_TIME);
				m_pSink->OnConnect(REASON_SUCCESSFUL);
			}
			
		}
	break;

	case TransCon_Pdu_Type_UDP_Keepalive:
		{
			m_bDataReceived = TRUE;
		}
		break;
	case TransCon_Pdu_Type_UDP_Data:
		{
			if (m_wState != TRANS_CON_UDP_STATE_CONNECTED)
				return 0;
			m_bDataReceived = TRUE;
			CTransConPduUdpBase dataPdu;
#ifdef TRANS_CON_UDP_SORT
			CByteStream stream(pData, 0, dwDataLen);
			dataPdu.Decode(stream);
#endif
			aData.Advance(dataPdu.GetLen());

#ifdef TRANS_CON_UDP_SORT
#else
			m_pSink->OnReceive(&aData);	
#endif
		}
		break;
	default:
		WARNINGTRACE("Network CUdpReactiveTransCon::OnReceive: invalid packet");
		break;
	}
	return 0;

}

int CUdpConTransCon::OnCloseUdp(int aErr)
{
	if (m_pSink)
	{
		m_pSink->OnDisconnect(REASON_SOCKET_ERROR);
	}
	else
		GetTransConManager()->DestroyTransCon(this);
	return 0;
}

CDataBlock *CUdpConTransCon::BuildDataPdu(CDataBlock *pInBlk)
{
	if (pInBlk == NULL)
	{
		return NULL;
	}
	CTransConPduUdpData udpData(m_dwConnectionId, m_wConSubId, ++m_wCurrentSndSeq);
	pInBlk->Back(udpData.GetLen());
	CByteStream stream(pInBlk->GetBuf(), 0, udpData.GetLen());
	udpData.Encode(stream);	
	return pInBlk;
}
CDataBlock *CUdpConTransCon::BuildSynPdu()
{
	CTransConPduUdpBase udpBase(m_dwConnectionId, m_wConSubId, m_wCurrentSndSeq, TransCon_Pdu_Type_UDP_Syn);
	CDataBlock *pBlk = new CDataBlock(32, 0);
	CByteStream stream(pBlk->GetBuf(), 0, udpBase.GetLen()+6);
	udpBase.Encode(stream);
	stream<<(DWORD)(ntohl(m_cInetAddr.GetPtr()->sin_addr.s_addr));
	stream<<(WORD)(ntohs(m_cInetAddr.GetPtr()->sin_port));
	pBlk->Expand(udpBase.GetLen()+6);
	return pBlk;
}

CDataBlock *CUdpConTransCon::BuildAckPdu()
{
	CTransConPduUdpBase udpBase(m_dwConnectionId, m_wConSubId, m_wCurrentSndSeq, TransCon_Pdu_Type_UDP_Ack);
	CDataBlock *pBlk = new CDataBlock(32, 0);
	CByteStream stream(pBlk->GetBuf(), 0, udpBase.GetLen()+6);
	udpBase.Encode(stream);
	stream<<(DWORD)(ntohl(m_cInetAddr.GetPtr()->sin_addr.s_addr));
	stream<<(WORD)(ntohs(m_cInetAddr.GetPtr()->sin_port));
	pBlk->Expand(udpBase.GetLen()+6);
	return pBlk;
}

CDataBlock *CUdpConTransCon::BuildKeepAlivePdu()
{
	CTransConPduUdpBase udpBase(m_dwConnectionId, m_wConSubId,m_wCurrentSndSeq, TransCon_Pdu_Type_UDP_Keepalive);
	CDataBlock *pBlk = new CDataBlock(32, 0);
	CByteStream stream(pBlk->GetBuf(), 0, udpBase.GetLen());
	udpBase.Encode(stream);
	pBlk->Expand(udpBase.GetLen());
	return pBlk;

}

int CUdpConTransCon::SetOpt(unsigned long OptType, void *pParam)
{
	ERRTRACE("Network CUdpConTransCon::SetOpt: bad option");
	return -1;
}

int CUdpConTransCon::GetOpt(unsigned long OptType, void *pParam)
{
	ERRTRACE("Network CUdpConTransCon::GetOpt: bad option");
	return -1;
}
