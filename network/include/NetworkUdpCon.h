#ifndef _NETWORK_UDP_CON_H_
#define _NETWORK_UDP_CON_H_

#include "NetworkImpl.h"
#include "transconudp.h"
#include "MTPtrMap.h"

#define NET_UDP_DATA_TYPE_DATA			1
#define NET_UDP_DATA_TYPE_CMD			2
#define NET_UDP_DATA_TYPE_RESP			3
#define MAX_WAIT_CMD					1000

class CNetUdpAcceptor;
class CNetRawUdpConnection:public INetConnection, public IClientSocketUDPSink
{
public:
	CNetRawUdpConnection(INetConnectionSink *pSink,
		unsigned long dwLocalAddr, unsigned short wLocalPort);
	~CNetRawUdpConnection();
/*
 *	INetConnection
 */
	int Connect(
		unsigned long	dwAddr, 
		unsigned short	wPort, 
		int				nType,
		void			*pProxySetting = NULL
		);
	
	void Disconnect(int iReason = 0);
	
	int SendData(unsigned char *pData, int nLen);
	int SendTo(unsigned char *pData, int nLen, 
		unsigned long dwAddr, unsigned short wPort);
	int SetOpt(unsigned long OptType, void *pParam);
	int GetOpt(unsigned long OptType, void *pParam);
/*
 *	IClientSocketUDPSink
 */
	int OnReceiveUdp(CDataBlock &aData, const CInetAddr &aAddr);
	int OnCloseUdp(int aErr);
	void SetSink(INetConnectionSink *pSink){m_pSink = pSink;}
	
	int GetType(){return NETWORK_CLASS_TYPE_UDP_CONN_RAW;}
	
private:
	INetConnectionSink *m_pSink;
	unsigned long m_dwLocalAddr;
	unsigned short m_wLocalPort;
	CClientSocketUDP m_cUdpSocket;
	unsigned long m_dwRemoteAddr;
	unsigned short m_wRemotePort;
	BOOL			m_bBindSuccess;
	CInetAddr		m_cInetAddr;
};

class CNetUdpConnection:public INetConnection, public ITransConSink
{
public:
	CNetUdpConnection(INetConnectionSink *pSink);
	CNetUdpConnection(INetConnectionSink *pSink, ITransCon *pTransCon);
	CNetUdpConnection(INetConnectionSink *pSink, CNetUdpAcceptor *pApt);
	~CNetUdpConnection();
	/*
	 *	INetConnection
	 */
	void SetLowTranConByApt(ITransCon *pTransCon);
	int Connect(
		unsigned long	dwAddr, 
		unsigned short	wPort, 
		int				nType,
		void			*pProxySetting = NULL
		);

	int SendCommand(unsigned char *pData, int nLen);
	int SendCommand(unsigned char *pData, int nLen, DWORD dwCallBackType, DWORD dwCallBackData = 0);
	
	void Disconnect(int iReason = 0);
	
	int SendData(unsigned char *pData, int nLen);
	int SendTo(unsigned char *pData, int nLen, 
		unsigned long dwAddr, unsigned short wPort);
	int SetOpt(unsigned long OptType, void *pParam);
	int GetOpt(unsigned long OptType, void *pParam);
	/*
	 *	ITransConSink
	 */
	int OnConnect(int iReason);
	int OnDisconnect(int iReason);
	int OnReceive(CDataBlock *pData);
	int OnSend();
	void SetSink(INetConnectionSink *pSink){m_pSink = pSink;}
	
	int GetType(){return NETWORK_CLASS_TYPE_UDP_CONN;}
	void OnTransTimer();
	
private:
	ITransCon *m_pTrans;
	INetConnectionSink *m_pSink;
	CNetUdpAcceptor *m_pApt;
	CInetAddr	m_cRemoteAddr;
	int			m_bConnected; 
private:
	unsigned long	m_nCommandPutSeq;
	unsigned long	m_nCommandRecvSeq;
	int			m_nPutIndex;
	int			m_nSendIndex;
	CDataBlock  *m_pCommands[MAX_WAIT_CMD];
	//TODO: xialj 08-04-21
	DWORD m_dwCBTypes[MAX_WAIT_CMD];
	DWORD m_dwCBDatas[MAX_WAIT_CMD];

	int			m_bCommandSend;
};

class CNetUdpAcceptor : public INetAcceptor, public ITransConAcceptorSink, public INetTimerSink
{
public :
        typedef struct _tagRegisterConnection
        {
            CInetAddr         addr;
            int               retries;
            CNetUdpConnection *pCon;
        } CONNECTION;
 
	CNetUdpAcceptor (INetAcceptorSink *pSink);
	~CNetUdpAcceptor();
	/*
	 *	INetAcceptor
	 */
	int StartListen(unsigned short wPort, unsigned long dwLocalAddr=0/*INADDR_ANY*/);
	int StopListen();
	int GetType(){return NETWORK_CLASS_TYPE_UDP_APT;}
	void RegisterConnection(CNetUdpConnection *pCon, CInetAddr &aAddr);
	void UnRegisterConnection(CInetAddr &aAddr);
	void OnTimer(void *pArg, INetTimer *pTimer);
	/*
	 *	
	 */
	int OnConnectIndication (ITransCon *pTransCon);
private:
       //first is retries and second is connection pointer
        typedef CMTPtrMap<CNullMutex, CInetAddr, CONNECTION*> REGISTERCONNECTIONS;

	INetAcceptorSink *m_pSink;
	CTransConUdpAcceptor *m_pUdpApt;
	CNetTimer *m_pTimer;

        REGISTERCONNECTIONS m_connections;

};

class CFOCheckRetry
{
public:
    CFOCheckRetry(CTransConUdpAcceptor* pApt) : m_pApt( pApt ) {}
    ~CFOCheckRetry(){}

    bool operator()(CNetUdpAcceptor::CONNECTION* pConnection);

private:
    CTransConUdpAcceptor* m_pApt;
};


#endif

