#ifndef  __NETWORK_MEDIA_CON_H_
#define  __NETWORK_MEDIA_CON_H_

#include "FlowControl.h"
#include "NetworkUdpCon.h"

#define	JITTER_PACKET_TYPE_NORMAL	0
#define	JITTER_PACKET_TYPE_AUDIO	1
#define	JITTER_PACKET_TYPE_VIDEO	2

#define  NETWORK_HEADER_SIZE 8
#ifdef WIN32
static unsigned char	m_aIndicateBuffer[65536];
#endif

class CNetUdpFCCon: public INetConnection, public INetConnectionSink
{
public:
	CNetUdpFCCon(INetConnectionSink *pSink);
	CNetUdpFCCon(INetConnectionSink *pSink, INetConnection *pCon);
	CNetUdpFCCon(INetConnectionSink *pSink, CNetUdpAcceptor *pApt);
	~CNetUdpFCCon();
	
	/*
	 *	INetConnection
	 */
	int Connect(
		unsigned long	dwAddr, 
		unsigned short	wPort, 
		int				nType,
		void			*pProxySetting = NULL
		);
	
	int OnCommand(unsigned char *pData, int nLen, INetConnection *pCon)
	{
		if (m_pSink)
			return m_pSink->OnCommand(pData, nLen, this);
		return 0;
	}
	int SendCommand(unsigned char *pData, int nLen)
	{
		if (m_pUdpCon)
			return m_pUdpCon->SendCommand(pData, nLen);
		return -1;
	}
	int SendCommand(unsigned char *pData, int nLen, DWORD dwCallBackType, DWORD dwCallBackData)
	{
		if (m_pUdpCon)
			return m_pUdpCon->SendCommand(pData, nLen, dwCallBackType, dwCallBackData);
		return -1;
	}
	//TODO:xialj 2008-04-17
	virtual int OnSend(int nReason, unsigned char* pBuf, int nLen, DWORD dwCallBackType,
		DWORD dwCallBackData, INetConnection* pCon)
	{
		if (m_pSink != NULL)
		{
			m_pSink->OnSend(nReason, pBuf, nLen, dwCallBackType, dwCallBackData, pCon);
		}
		
		return 0;
	}
	
	void Disconnect(int iReason = 0);
	void SetSink(INetConnectionSink *pSink);
	int SendData(unsigned char *pData, int nLen);
	int SendMedia(unsigned char *pHeader, int nHeaderLen, 
		char *pData, int nLen, unsigned char nMediaType, unsigned char nSubSeq, 
		unsigned short usIFrameSeq);	
	int SendTo(unsigned char *pData, int nLen, 
		unsigned long dwAddr, unsigned short wPort);
	int SetOpt(unsigned long OptType, void *pParam);
	int GetOpt(unsigned long OptType, void *pParam);
	int GetType(){return NETWORK_CLASS_TYPE_UDP_CONN_MEDIA;}
	
	/*
	 *	INetConnectionSink
	 */
	int OnConnect(int nReason, INetConnection *pCon);
	int OnDisconnect(int nReason, INetConnection *pCon);
	int OnReceive(unsigned char *pData, int nLen, INetConnection *pCon);
	int OnSend(INetConnection *pCon);
private:
	INetConnectionSink		*m_pSink;
#ifdef WIN32
	CFlowControlConnection	*m_pFCItem;
#endif
	INetConnection			*m_pUdpCon;
	int						m_nSendBps;
#ifdef EMBEDED_LINUX
	CFlowControlSend		*m_pSendItem;
#endif
};

class CNetUdpFCApt:public INetAcceptorSink, public INetAcceptor
{
public:
	CNetUdpFCApt (INetAcceptorSink *pSink);
	~CNetUdpFCApt();
	/*
	 *	INetAcceptor
	 */
	int StartListen(unsigned short wPort, unsigned long dwLocalAddr=0/*INADDR_ANY*/);
	
	int StopListen();
	int GetType(){return NETWORK_CLASS_TYPE_UDP_APT_MEDIA;}
	/*
	 *	INetAcceptorSink
	 */
	int OnConnectIndication(INetConnection *pCon, INetAcceptor *pApt);

	CNetUdpAcceptor *GetLowApt(){return m_pApt;}
private:
	INetAcceptorSink *m_pSink;
	CNetUdpAcceptor *m_pApt;
};
#ifdef WIN32
#define JITTER_DELAY_HIGH		3000 /*3s*/
#define JITTER_DELAY_LOW		500 /*1s*/
#define JITTER_DELAY_NORMAL		1500 /*2s*/
#define JITTER_SYNC_TIME		10000 /*10s*/
#define JITTER_COEFF_HIGH		11
#define JITTER_COEFF_LOW		10
#define JITTER_COEFF_NORMAL		10

struct JitterBufferList
{
	unsigned char	*pBuffer;
	int		nLen;
	unsigned long dviptime;
	unsigned short dvipmsec;
	unsigned long hosttime;
	JitterBufferList *pNext;
};
#endif
class CNetJitterBufferCon :public INetConnection, public INetConnectionSink
#ifdef WIN32
						,public INetTimerSink
#endif
{
public:
	CNetJitterBufferCon(INetConnectionSink *pSink, INetConnection *pCon);
	~CNetJitterBufferCon();	

	virtual int Connect(
		unsigned long	dwAddr, 
		unsigned short	wPort, 
		int				nType,
		void			*pProxySetting = NULL
		)
	{
		return m_pCon->Connect(dwAddr, wPort, nType, pProxySetting);
	}
	virtual void Disconnect(int iReason = 0)
	{
		m_pCon->Disconnect(iReason);
	}
	virtual void SetSink(INetConnectionSink *pSink) 
	{
		m_pSink = pSink;
	}
	virtual int SendData(unsigned char *pData, int nLen)
	{
		unsigned char *pNewData = new unsigned char [nLen+1];
		pNewData[0] = JITTER_PACKET_TYPE_NORMAL;
		memcpy(pNewData+1, pData, nLen);
		int rt = m_pCon->SendData(pNewData, nLen+1);
		delete []pNewData;
		return rt;
	}

	virtual int OnCommand(unsigned char *pData, int nLen, INetConnection *pCon)
	{
		if (m_pSink)
			return m_pSink->OnCommand(pData, nLen, this);
		return 0;
	}
	//TODO:xialj 2008-04-17
	virtual int OnSend(int nReason, 
					  unsigned char* pBuf, 
					   int nLen, 
					   DWORD dwCallBackType, 
					   DWORD dwCallBackData, 
					   INetConnection* pCon)
	{
		if (m_pSink != NULL)
		{
			m_pSink->OnSend(nReason, pBuf, nLen,dwCallBackType,dwCallBackData, pCon);
		}
		
		return 0;
	}
	virtual int SendCommand(unsigned char *pData, int nLen)
	{
		if (m_pCon)
			return m_pCon->SendCommand(pData, nLen);
		return -1;
	}
	virtual int SendCommand(unsigned char *pData, int nLen, DWORD dwCallBackType, DWORD dwCallBackData)
	{
		if (m_pCon)
			return m_pCon->SendCommand(pData, nLen, dwCallBackType, dwCallBackData);
		return -1;
	}
	
	virtual int SendMedia(unsigned char *pHeader, int nHeaderLen, 
		char *pData, int nLen, unsigned char nMediaType, unsigned char nSubSeq, 
		unsigned short usIFrameSeq);
	
	virtual int SendTo(unsigned char *pData, int nLen, 
		unsigned long dwAddr, unsigned short wPort)
	{
		return m_pCon->SendTo(pData, nLen, dwAddr, wPort);
	}
	virtual int SetOpt(unsigned long OptType, void *pParam)
	{
#ifdef WIN32
		
		if (OptType == NETWORK_MEDIA_OPTION_REST_BUFFER)
		{
			ClearJitterList();
		}
#endif
		return m_pCon->SetOpt(OptType, pParam);
	}
	virtual int GetOpt(unsigned long OptType, void *pParam)
	{
		return m_pCon->GetOpt(OptType, pParam);
	}
	virtual int GetType()
	{
		return m_pCon->GetType();
	}

	virtual int OnConnect(int nReason, INetConnection *pCon)
	{
		return m_pSink->OnConnect(nReason, this);
	}
	virtual int OnDisconnect(int nReason, INetConnection *pCon)
	{
		return m_pSink->OnDisconnect(nReason, this);
	}
	virtual int OnReceive(unsigned char *pData, int nLen, INetConnection *pCon);
	virtual int OnSend(INetConnection *pCon)
	{
		return m_pSink->OnSend(this);
	}
#ifdef WIN32
	void OnTimer(void *pArg, INetTimer *pTimer);
	void CheckJitterBuffer();
	void CalcCoeff();
	BOOL IsNeedIndicate(JitterBufferList *pList, unsigned long ticket);		
	void InsertPacketToBuffer(unsigned char *pData, int nLen, unsigned char nType);
	void ClearJitterList();
#endif	
#ifdef EMBEDED_LINUX
	void LinuxGetTickCount(unsigned char *pData);
#endif	
	
private:
	INetConnectionSink *m_pSink;
	INetConnection	   *m_pCon;
#ifdef WIN32
	INetTimer		   *m_pTimer;
	JitterBufferList	*m_pAudioJitterList;
	JitterBufferList	*m_pVideoJitterList;
	int					m_nVidoePacketCount;
	unsigned long		m_nIndicateDVIPTime;
	unsigned short		m_nIndicateDVIPMsec;
	unsigned long		m_nLastIndicateTime;
	unsigned long		m_nReceiveDVIPTime;
	unsigned long		m_nReceiveDVIPMsec;
	unsigned long		m_nSyncTime;
	int					m_nCoeff;
#endif
};

class CNetTcpMeidaApt: public INetAcceptor,public INetAcceptorSink
{
public:
	CNetTcpMeidaApt (INetAcceptorSink *pSink);
	~CNetTcpMeidaApt()
	{
		if (m_pApt)
		{
			delete m_pApt;
			m_pApt = NULL;
		}
	}
	/*
	 *	INetAcceptor
	 */
	int StartListen(unsigned short wPort, unsigned long dwLocalAddr=0/*INADDR_ANY*/)
	{
		return m_pApt->StartListen(wPort, dwLocalAddr);
	}
	
	int StopListen()
	{
		return m_pApt->StopListen();
	}
	int GetType(){return NETWORK_CLASS_TYPE_TCP_APT_MEDIA;}
	int OnConnectIndication(INetConnection *pCon, INetAcceptor *pApt);

private:
	INetAcceptorSink *m_pSink;
	INetAcceptor *m_pApt;
};

#endif

