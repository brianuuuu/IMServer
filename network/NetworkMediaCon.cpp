#ifdef EMBEDED_LINUX
#include <sys/sysinfo.h>
#include <sys/time.h>
#endif

#include "CmBase.h"
#include "NetworkImpl.h"
#include "NetworkUdpCon.h"
#include "NetworkTcpCon.h"
#include "NetworkMediaCon.h"
#ifdef WIN32
#include "Mmsystem.h"
#endif
extern "C"
{
	INetAcceptor *CreateUdpMediaAcceptor(INetAcceptorSink *pSink)
	{
		if (pSink == NULL)
			return NULL;
		return new CNetUdpFCApt(pSink);
	}
	INetAcceptor *CreateTcpMediaAcceptor(INetAcceptorSink *pSink)
	{
		if (pSink == NULL)
			return NULL;
		return new CNetTcpMeidaApt(pSink);
	}
	INetConnection *CreateUdpMediaCon(INetConnectionSink *pSink, 
		INetAcceptor *pApt)
	{
		if (pSink == NULL)
			return NULL;
		if (pApt == NULL)
		{
			CNetUdpFCCon *pFcCon =  new CNetUdpFCCon(NULL);
			CNetJitterBufferCon *pJitterCon = new CNetJitterBufferCon(pSink,pFcCon);
			pFcCon->SetSink(pJitterCon);
			return pJitterCon;
		}
		else if (pApt->GetType() == NETWORK_CLASS_TYPE_UDP_APT_MEDIA)
		{
			CNetUdpFCApt *pFCApt = (CNetUdpFCApt *)pApt;
			CNetUdpFCCon *pFcCon = new CNetUdpFCCon(NULL, pFCApt->GetLowApt());
			CNetJitterBufferCon *pJitterCon = new CNetJitterBufferCon(pSink,pFcCon);
			pFcCon->SetSink(pJitterCon);
			return pJitterCon;
		}
		else
			return NULL;
	}
	INetConnection *CreateTcpMediaCon(INetConnectionSink *pSink)
	{
		INetConnection *pTcpCon =  CreateTcpCon(pSink);
		CNetJitterBufferCon *pJitterCon = new CNetJitterBufferCon(pSink,pTcpCon);
		pTcpCon->SetSink(pJitterCon);
		return pJitterCon;
	}
}

CNetUdpFCCon::CNetUdpFCCon(INetConnectionSink *pSink)
{
	m_pSink = pSink;
	m_pUdpCon = new CNetUdpConnection(this);
	m_nSendBps = NETWORK_SEND_BITRATE_INFINITE;
#ifdef EMBEDED_LINUX
	m_pSendItem = NULL;
#endif
#ifdef WIN32
	m_pFCItem = new CFlowControlConnection(0,0);
#endif
}

CNetUdpFCCon::CNetUdpFCCon(INetConnectionSink *pSink, INetConnection *pCon)
{
	m_pSink = pSink;
	m_pUdpCon = pCon;
	m_nSendBps = NETWORK_SEND_BITRATE_INFINITE;
#ifdef EMBEDED_LINUX
	m_pSendItem = NULL;
#endif
#ifdef WIN32
	m_pFCItem = new CFlowControlConnection(0,0);
#endif
}

CNetUdpFCCon::CNetUdpFCCon(INetConnectionSink *pSink, CNetUdpAcceptor *pApt)
{
	m_pSink = pSink;
	m_pUdpCon = new CNetUdpConnection(this, pApt);
	m_nSendBps = NETWORK_SEND_BITRATE_INFINITE;
#ifdef EMBEDED_LINUX
	m_pSendItem = NULL;
#endif
#ifdef WIN32
	m_pFCItem = new CFlowControlConnection(0,0);
#endif
		
}

CNetUdpFCCon::~CNetUdpFCCon()
{
	if (m_pUdpCon != NULL)
	{
		delete m_pUdpCon;
		m_pUdpCon = NULL;
	}
#ifdef EMBEDED_LINUX
	if(m_pSendItem != NULL)
	{
		delete m_pSendItem;
		m_pSendItem = NULL;
	}
#endif	
#ifdef WIN32
	if (m_pFCItem != NULL)
	{
		delete m_pFCItem;
		m_pFCItem = NULL;
	}
#endif
}

int CNetUdpFCCon::Connect(
	unsigned long	dwAddr, 
	unsigned short	wPort, 
	int				nType,
	void			*pProxySetting
	)
{
	if (m_pUdpCon)
		return m_pUdpCon->Connect(dwAddr, wPort, nType, pProxySetting);
	else
		return -1;
}

void CNetUdpFCCon::Disconnect(int iReason)
{
	if (m_pUdpCon)
		m_pUdpCon->Disconnect(iReason);
#ifdef EMBEDED_LINUX
	if(m_pSendItem != NULL)
	{
		delete m_pSendItem;
		m_pSendItem = NULL;
	}
#endif	

}

void CNetUdpFCCon::SetSink(INetConnectionSink *pSink)
{
	m_pSink = pSink;
}

int CNetUdpFCCon::SendData(unsigned char *pData, int nLen)
{
	if (m_pUdpCon)
	{
		unsigned char *pNewData = new unsigned char[nLen+8/*NETWORK_HEADER_SIZE*/];
		int rt;
		if (pNewData == NULL)
			return -1;
		memset(pNewData, 0, 8); /*FF_NOFRAGMENT*/
		memcpy (pNewData+8, pData, nLen);
		rt = m_pUdpCon->SendData(pNewData, nLen+8);
		delete []pNewData;
		return rt;
		
	}
	else
		return -1;
}
int CNetUdpFCCon::SendMedia(unsigned char *pHeader, int nHeaderLen, 
			  char *pData, int nLen, unsigned char nMediaType, unsigned char nSubSeq, 
			  unsigned short usIFrameSeq)
{
	if (!pHeader || !pData || !m_pUdpCon)
		return -1;
	
	if (nMediaType != MEDIA_TYPE_VIDEO)
	{
		if (m_pUdpCon)
		{
			unsigned char *pNewData = new unsigned char[nHeaderLen+nLen+8/*NETWORK_HEADER_SIZE*/];
			int rt;
			if (pNewData == NULL)
				return -1;
			memset(pNewData, 0, 8); /*FF_NOFRAGMENT*/
			memcpy(pNewData+8, pHeader, nHeaderLen);
			memcpy (pNewData+8+nHeaderLen, pData, nLen);
			rt = m_pUdpCon->SendData(pNewData, nLen+8+nHeaderLen);
			delete []pNewData;
			return rt;
		}
		return -1;
	}
#ifdef EMBEDED_LINUX
	{
		static unsigned char szSend[MEDIA_FRAGMENT_SIZE];
		
		if (!m_pUdpCon)
			return -1;
		if (nLen <= 0)
			return -1;
		if (m_pSendItem == NULL)
			m_pSendItem = new CFlowControlSend;

		int nRemainLen = nHeaderLen + nLen;
		int nCurrentLen, nOffset, nFrag;
		int nTotalFrag = (nRemainLen+MEDIA_FRAGMENT_SIZE-1)/MEDIA_FRAGMENT_SIZE;
		struct timeval current;
			
		GetTimeOfDay(&current, NULL);
		nCurrentLen = nRemainLen < MEDIA_FRAGMENT_SIZE?nRemainLen:MEDIA_FRAGMENT_SIZE;

		memcpy(szSend, pHeader, nHeaderLen);
		memcpy(szSend+nHeaderLen, pData, nCurrentLen-nHeaderLen);
		m_pSendItem->FlowControlPutBuf(m_pUdpCon, szSend, nCurrentLen, 
			&current, nSubSeq, nTotalFrag, 0, usIFrameSeq);

		nOffset = nCurrentLen-nHeaderLen;
		nRemainLen = nRemainLen-nCurrentLen;
		nFrag = 1;
		while (nRemainLen > 0)
		{
			nCurrentLen = nRemainLen < MEDIA_FRAGMENT_SIZE?nRemainLen:MEDIA_FRAGMENT_SIZE;
			memcpy(szSend, pData+nOffset, nCurrentLen);
			m_pSendItem->FlowControlPutBuf(m_pUdpCon, szSend, nCurrentLen, 
				&current, nSubSeq, nTotalFrag, nFrag, usIFrameSeq);
				
			nFrag++;
			nRemainLen = nRemainLen-nCurrentLen;
			nOffset = nOffset+nCurrentLen;
		}
		m_pSendItem->FlowControlSend();
		return 0;
	}
#else
	return -1;
#endif
}

int CNetUdpFCCon::SendTo(unsigned char *pData, int nLen, 
	unsigned long dwAddr, unsigned short wPort)
{
	if (m_pUdpCon)
		return m_pUdpCon->SendTo(pData, nLen, dwAddr, wPort);
	else
		return -1;
}

int CNetUdpFCCon::SetOpt(unsigned long OptType, void *pParam)
{
	switch(OptType) {
	case NETWORK_MEDIA_OPTION_SET_SEND_BITRATE:
		{
			m_nSendBps = *((unsigned long*)pParam);
#ifdef EMBEDED_LINUX
			if (m_pSendItem == NULL)
				m_pSendItem = new CFlowControlSend;
			m_pSendItem->FlowControlSetSendBPS(m_nSendBps);
#endif		
			return 0;
		}
		break;
#ifdef WIN32
	case NETWORK_MEDIA_OPTION_REST_BUFFER:
		{
			if (m_pFCItem)
				delete m_pFCItem;
			m_pFCItem = new CFlowControlConnection(0,0);
			return 0;
		}
		break;
#endif
	default:
		break;
	}
	if (m_pUdpCon)
		return m_pUdpCon->SetOpt(OptType, pParam);
	else
		return -1;
}

int CNetUdpFCCon::GetOpt(unsigned long OptType, void *pParam)
{
	if (m_pUdpCon)
		return m_pUdpCon->GetOpt(OptType, pParam);
	else
		return -1;
}

int CNetUdpFCCon::OnConnect(int nReason, INetConnection *pCon)
{
	return m_pSink->OnConnect(nReason, this);
}

int CNetUdpFCCon::OnDisconnect(int nReason, INetConnection *pCon)
{
#ifdef EMBEDED_LINUX
	if(m_pSendItem != NULL)
	{
		delete m_pSendItem;
		m_pSendItem = NULL;
	}
#endif	
	return m_pSink->OnDisconnect(nReason, this);
}

int CNetUdpFCCon::OnReceive(unsigned char *pData, int nLen, INetConnection *pCon)
{
#ifdef WIN32
	unsigned char packettype = pData[0];
	int rt;
	switch(packettype) {
	case FF_FC_MEDIA:
		rt = m_pFCItem->InsertPacket(m_pUdpCon, pData, nLen, (char*)m_aIndicateBuffer, 65536);
		if (rt > 0)
			return m_pSink->OnReceive(m_aIndicateBuffer, rt, this);
		else
			return m_pSink->OnReceive(NULL, 0, this);
		return 0;
		break;
	case FF_FC_EVALRTT:
		m_pFCItem->OnReceiveRTTEval(pData, nLen);
		return 0;
		break;
	default:
		return m_pSink->OnReceive(pData+NETWORK_HEADER_SIZE, nLen-NETWORK_HEADER_SIZE, this);
		break;
	}
#endif
#ifdef EMBEDED_LINUX
	unsigned char packettype = pData[0];
	int rt;
	switch(packettype)
    {
    case FF_FC_RESEND:
		if (m_pSendItem)
			m_pSendItem->OnReceiveResend((char*)pData);
        return 0;
    case FF_FC_EVALRTT:
        rt = m_pUdpCon->SendData(pData, nLen);
        return rt;
    default:
		return m_pSink->OnReceive(pData+NETWORK_HEADER_SIZE, nLen-NETWORK_HEADER_SIZE, this);
    }
#endif
	return m_pSink->OnReceive(pData+NETWORK_HEADER_SIZE, nLen-NETWORK_HEADER_SIZE, this);
}

int CNetUdpFCCon::OnSend(INetConnection *pCon)
{
	return m_pSink->OnSend(this);
}

/*
 *	CNetUdpFCApt
 */
CNetUdpFCApt::CNetUdpFCApt (INetAcceptorSink *pSink)
{
	m_pSink = pSink;
	m_pApt = new CNetUdpAcceptor(this);
}
CNetUdpFCApt::~CNetUdpFCApt()
{
	if (m_pApt)
	{
		delete m_pApt;
		m_pApt = NULL;
	}
}

int CNetUdpFCApt::StartListen(unsigned short wPort, unsigned long dwLocalAddr)
{
	return m_pApt->StartListen(wPort, dwLocalAddr);
}

int CNetUdpFCApt::StopListen()
{
	return m_pApt->StopListen();
}

int CNetUdpFCApt::OnConnectIndication(INetConnection *pCon, INetAcceptor *pApt)
{
	CNetUdpFCCon *pFCCon = new CNetUdpFCCon(NULL, pCon);
	pCon->SetSink(pFCCon);
	CNetJitterBufferCon *pJitterCon = new CNetJitterBufferCon(NULL, pFCCon);
	pFCCon->SetSink(pJitterCon);
	return m_pSink->OnConnectIndication(pJitterCon, this);
}

/*
 *	CNetJitterBufferCon
 */
CNetJitterBufferCon::CNetJitterBufferCon(INetConnectionSink *pSink, INetConnection *pCon)
{
	m_pCon = pCon;
	m_pSink = pSink;
#ifdef WIN32
	m_pTimer = new CNetTimer(this);
	m_pTimer->Schedule(20, NULL);
	m_pAudioJitterList = NULL;
	m_pVideoJitterList = NULL;
	m_nIndicateDVIPTime = 0;
	m_nIndicateDVIPMsec = 0;
	m_nLastIndicateTime = 0;
	m_nReceiveDVIPTime = 0;
	m_nReceiveDVIPMsec = 0;
	m_nVidoePacketCount = 0;
	m_nCoeff = JITTER_COEFF_NORMAL;
#endif
}
CNetJitterBufferCon::~CNetJitterBufferCon()
{
	if (m_pCon != NULL)
	{
		delete m_pCon;
		m_pCon = NULL;
	}
#ifdef WIN32
	if (m_pTimer != NULL)
	{
		m_pTimer->Cancel();
		delete m_pTimer;
		m_pTimer = NULL;
	}
	JitterBufferList *pList;
	while (m_pVideoJitterList != NULL)
	{
		pList = m_pVideoJitterList;
		m_pVideoJitterList = m_pVideoJitterList->pNext;
		delete []pList->pBuffer;
		delete pList;
	}
	while (m_pAudioJitterList != NULL)
	{
		pList = m_pAudioJitterList;
		m_pAudioJitterList = m_pAudioJitterList->pNext;
		delete []pList->pBuffer;
		delete pList;
	}
#endif		
}

#ifdef EMBEDED_LINUX
void CNetJitterBufferCon::LinuxGetTickCount(unsigned char *pData)
{
	static unsigned long lst_sys, lst_time, sec;
	struct sysinfo info;
	struct timeval tm;
	unsigned short msec;

	sysinfo(&info);
	gettimeofday(&tm, NULL);
	msec = tm.tv_usec/1000;
	sec = lst_sys;
	if (info.uptime != lst_sys)
	{
		if (tm.tv_sec != lst_time)
		{
			lst_sys = info.uptime;
			lst_time = tm.tv_sec;
		}
		sec = lst_sys;
	}else if (tm.tv_sec != lst_time)
	{
		sec = lst_sys +1;
	}
	sec = htonl(sec);
	msec = htons(msec);
	memcpy(pData, &sec, 4);
	memcpy(pData+4, &msec, 2);
	
}
#endif

int CNetJitterBufferCon::SendMedia(unsigned char *pHeader, int nHeaderLen, 
			  char *pData, int nLen, unsigned char nMediaType, unsigned char nSubSeq, 
			  unsigned short usIFrameSeq)
{
#ifdef EMBEDED_LINUX
	if (nMediaType == MEDIA_TYPE_AUDIO || nMediaType == MEDIA_TYPE_VIDEO)
	{
		unsigned char *pNewData = new unsigned char [nHeaderLen+7];
		pNewData[0] = nMediaType;
		LinuxGetTickCount(pNewData+1);
		memcpy(pNewData+7, pHeader, nHeaderLen);
		int rt = m_pCon->SendMedia(pNewData, nHeaderLen+7, pData, nLen, 
			nMediaType, nSubSeq, usIFrameSeq);
		delete []pNewData;
		return rt;
	}
	else
		return -1;
#else
	unsigned char *pNewData = new unsigned char [nHeaderLen+1];
	pNewData[0] = JITTER_PACKET_TYPE_NORMAL;
	memcpy(pNewData+1, pHeader, nHeaderLen);
	int rt = m_pCon->SendMedia(pNewData, nHeaderLen+1, pData, nLen, 
		nMediaType, nSubSeq, usIFrameSeq);
	delete []pNewData;
	return rt;
#endif
}

int CNetJitterBufferCon::OnReceive(unsigned char *pData, int nLen, INetConnection *pCon)
{
#ifdef WIN32
	unsigned char nJitterType;
	if (nLen == 0 || pData == NULL)
	{
		CheckJitterBuffer();
		return 0;
	}
	
	nJitterType = pData[0];
	switch(nJitterType) {
	case JITTER_PACKET_TYPE_NORMAL:
		return m_pSink->OnReceive(pData+1, nLen-1, this);
		break;
	case JITTER_PACKET_TYPE_AUDIO:
	case JITTER_PACKET_TYPE_VIDEO:
		InsertPacketToBuffer(pData+1, nLen - 1, nJitterType);
		break;
	default:
		break;
	}
	CheckJitterBuffer();
	return 0;
#else
	return m_pSink->OnReceive(pData+1, nLen-1, this);
#endif
}
#ifdef WIN32
void CNetJitterBufferCon::ClearJitterList()
{
	JitterBufferList *pList;
	while (m_pVideoJitterList != NULL)
	{
		pList = m_pVideoJitterList;
		m_pVideoJitterList = m_pVideoJitterList->pNext;
		delete []pList->pBuffer;
		delete pList;
	}
	while (m_pAudioJitterList != NULL)
	{
		pList = m_pAudioJitterList;
		m_pAudioJitterList = m_pAudioJitterList->pNext;
		delete []pList->pBuffer;
		delete pList;
	}	
	m_pAudioJitterList = NULL;
	m_pVideoJitterList = NULL;
	m_nIndicateDVIPTime = 0;
	m_nIndicateDVIPMsec = 0;
	m_nLastIndicateTime = 0;
	m_nReceiveDVIPTime = 0;
	m_nReceiveDVIPMsec = 0;
	m_nVidoePacketCount = 0;
	m_nCoeff = JITTER_COEFF_NORMAL;
}
void CNetJitterBufferCon::OnTimer(void *pArg, INetTimer *pTimer)
{
	CheckJitterBuffer();
}
void CNetJitterBufferCon::CalcCoeff()
{
	unsigned long buff_time_len = (m_nReceiveDVIPTime - m_nIndicateDVIPTime)*1000 +
		m_nReceiveDVIPMsec - m_nIndicateDVIPMsec;
	
	if (buff_time_len > JITTER_DELAY_HIGH && m_nVidoePacketCount >= 5)
	{
		m_nCoeff = JITTER_COEFF_HIGH;
	}
	else if (buff_time_len < JITTER_DELAY_LOW )
	{
		m_nCoeff = JITTER_COEFF_LOW;
	}
	else
	{
		if (m_nCoeff == JITTER_COEFF_HIGH && 
			buff_time_len <= JITTER_DELAY_NORMAL)
		{
			m_nCoeff = JITTER_COEFF_NORMAL;
		}
		if (m_nCoeff == JITTER_COEFF_LOW &&
			buff_time_len >= JITTER_DELAY_NORMAL)
		{
			m_nCoeff = JITTER_COEFF_NORMAL;
		}
	}
}

BOOL CNetJitterBufferCon::IsNeedIndicate(JitterBufferList *pList, unsigned long ticket)
{
	long localdiff, dvipdiff;

	if (ticket < m_nLastIndicateTime)
		localdiff = 0;
	else
		localdiff = ((ticket-m_nLastIndicateTime)*m_nCoeff)/10;
	dvipdiff = (pList->dviptime-m_nIndicateDVIPTime)*1000 + pList->dvipmsec - m_nIndicateDVIPMsec;

	if (dvipdiff <= localdiff)
	{
//		char buf[512];
//		sprintf(buf, "ticket %d m_nLst %d, dviptime %d dvipmsec %d intime %d indmsec %d coeff %d %x\n",
//			ticket, m_nLastIndicateTime, pList->dviptime, pList->dvipmsec, 
//			m_nIndicateDVIPTime, m_nIndicateDVIPMsec, m_nCoeff, this);
//		OutputDebugString(buf);
		return TRUE;
	}
	else
		return FALSE;
}

void CNetJitterBufferCon::CheckJitterBuffer()
{
	unsigned long ticket = ::timeGetTime();
//	char buf[256];
	JitterBufferList *pList;

	if (m_nLastIndicateTime == 0)
	{
		if (m_pVideoJitterList != NULL)
		{
			pList = m_pVideoJitterList;
			m_pVideoJitterList = m_pVideoJitterList->pNext;
			m_pSink->OnReceive(pList->pBuffer, pList->nLen, this);

			m_nIndicateDVIPTime = pList->dviptime;
			m_nIndicateDVIPMsec = pList->dvipmsec;
			m_nLastIndicateTime = ticket;
			m_nSyncTime = 0;
			delete []pList->pBuffer;
			delete pList;
		}
		return;
	}

	CalcCoeff();
	/*
	 *	Get Video
	 */
	if (m_pVideoJitterList == NULL)
	{
//		sprintf(buf, "Buffer Empty %x\n", this);
//		OutputDebugString(buf);
		m_nLastIndicateTime = ticket;
		m_nSyncTime = 0;

	}
	while (m_pVideoJitterList)
	{
		if (IsNeedIndicate(m_pVideoJitterList, ticket))
		{

//			sprintf(buf, "Indicate %x\n", this);
//			OutputDebugString(buf);

			pList = m_pVideoJitterList;
			m_pVideoJitterList = m_pVideoJitterList->pNext;
			m_pSink->OnReceive(pList->pBuffer, pList->nLen, this);
			m_nVidoePacketCount --;			

			long diff = (pList->dviptime - m_nIndicateDVIPTime)*1000 +
				pList->dvipmsec - m_nIndicateDVIPMsec;
			if (diff < 0)
				diff = 0;
			m_nIndicateDVIPTime = pList->dviptime;
			m_nIndicateDVIPMsec = pList->dvipmsec;

			if (m_nSyncTime < JITTER_SYNC_TIME)
			{
				m_nLastIndicateTime += (diff*10)/m_nCoeff;
				m_nSyncTime += diff;
			}
			else
			{
				m_nLastIndicateTime = ticket;
				m_nSyncTime = 0;
			}
			delete []pList->pBuffer;
			delete pList;
			
		}
		else
		{
			break;
		}
	}
	/*
	 *	Get Audio
	 */
	while (m_pAudioJitterList)
	{
		if (IsNeedIndicate(m_pAudioJitterList, ticket))
		{
			pList = m_pAudioJitterList;
			m_pAudioJitterList = m_pAudioJitterList->pNext;
			m_pSink->OnReceive(pList->pBuffer, pList->nLen, this);
			delete []pList->pBuffer;
			delete pList;
			
		}
		else
		{
			break;
		}
	}
	
}
void CNetJitterBufferCon::InsertPacketToBuffer(unsigned char *pData, int nLen, unsigned char nType)
{
	JitterBufferList *pList,  *pNew;
	unsigned long sec, ticket;
	unsigned short msec;
//	char	buf[256];
	
	memcpy(&sec, pData, 4);
	memcpy(&msec, pData+4, 2);
	sec = ntohl(sec);
	msec = ntohs(msec);
	ticket = ::timeGetTime();

	pNew = new JitterBufferList;
	pNew->dvipmsec = msec;
	pNew->dviptime = sec;
	pNew->hosttime = ticket;
	pNew->nLen = nLen-6;
	pNew->pNext=NULL;
	pNew->pBuffer = new unsigned char[nLen-6];
	memcpy(pNew->pBuffer, pData+6, nLen - 6);

	if (nType == JITTER_PACKET_TYPE_AUDIO)
	{
		if (m_pAudioJitterList == NULL)
		{
			m_pAudioJitterList = pNew;
			return;
		}
		pList = m_pAudioJitterList;
	}
	else
	{
		m_nVidoePacketCount ++;
		if (m_pVideoJitterList == NULL)
		{
			m_pVideoJitterList = pNew;
			m_nReceiveDVIPTime = pNew->dviptime;
			m_nReceiveDVIPMsec = pNew->dvipmsec;
//			sprintf(buf, "Receive1 sec %d msec %d %x\n", m_nReceiveDVIPTime, m_nReceiveDVIPMsec, this);
//			OutputDebugString(buf);
			return;
		}
		pList = m_pVideoJitterList;
		m_nReceiveDVIPTime = pNew->dviptime;
		m_nReceiveDVIPMsec = pNew->dvipmsec;
//		sprintf(buf, "Receive sec %d msec %d %x\n", m_nReceiveDVIPTime, m_nReceiveDVIPMsec, this);
//		OutputDebugString(buf);
	}

	for (;pList->pNext != NULL;pList = pList->pNext)
		;
	pList->pNext = pNew;
}
#endif
/*
 *	CNetTcpMeidaApt
 */
CNetTcpMeidaApt::CNetTcpMeidaApt (INetAcceptorSink *pSink)
{
	m_pSink = pSink;
	m_pApt = new CNetTcpAcceptor(this);
}
int CNetTcpMeidaApt::OnConnectIndication(INetConnection *pCon, INetAcceptor *pApt)
{
	CNetJitterBufferCon *pJitterCon = new CNetJitterBufferCon(NULL, pCon);
	pCon->SetSink(pJitterCon);
	return m_pSink->OnConnectIndication(pJitterCon, this);
}
