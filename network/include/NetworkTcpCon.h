#ifndef _NETWORK_TCP_CON_H_
#define _NETWORK_TCP_CON_H_
#include "NetworkImpl.h"
#include "transcontcp.h"

#define NET_TCP_DATA_TYPE_DATA			1
#define NET_TCP_DATA_TYPE_CMD			2
#define NET_TCP_DATA_TYPE_RESP			3
#define MAX_WAIT_CMD				1000

class CNetRawTcpConnection:public INetConnection, public ITransportSink, public IAcceptorConnectorSink
{
public:
	CNetRawTcpConnection(INetConnectionSink *pSink);
	CNetRawTcpConnection(INetConnectionSink *pSink, ITransport *pTrans);
	~CNetRawTcpConnection();
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
 *	ITransportSink
 */
	
	int OnDisconnect(int aReason);
	int OnReceive(CDataBlock &aData);
	int OnSend();
	int OnConnectIndication(int aReason, ITransport *aTrans);
		
	void SetSink(INetConnectionSink *pSink){m_pSink = pSink;}
	int GetType(){return NETWORK_CLASS_TYPE_TCP_CONN_RAW;}
	
private:
	INetConnectionSink *m_pSink;
	IConnector		*m_pConnector;
	ITransport		*m_pTransport;
};

class CNetTcpConnection:public INetConnection, public ITransConSink, public INetTimerSink
{
public:
	CNetTcpConnection(INetConnectionSink *pSink);
	CNetTcpConnection(INetConnectionSink *pSink, ITransCon *pTransCon);
	~CNetTcpConnection();
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
	int SendCommand(unsigned char *pData, int nLen);
	//TODO: xialj 08-04-21
	int SendCommand(unsigned char* pData, 
				    int nLen, 
					DWORD dwCallBackType, 
					DWORD dwCallBackData);

	int SendMedia(unsigned char *pHeader, int nHeaderLen, 
		char *pData, int nLen, unsigned char nMediaType, unsigned char nSubSeq, 
		unsigned short usIFrameSeq);

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
	
	int GetType(){return NETWORK_CLASS_TYPE_TCP_CONN;}
	/*
	 *	Frank For Reliable TCP
	 */
	void OnTimer(void *pArg, INetTimer *pTimer);
	
private:
	CTcpTransCon *m_pTrans;
	int  m_bConnected;
	INetConnectionSink *m_pSink;
	/*
	* Frank For Reliable TCP
	*/
private:
	unsigned long	m_nCommandPutSeq;
	unsigned long	m_nCommandRecvSeq;
	int			m_nPutIndex;
	int			m_nSendIndex;
	CDataBlock  *m_pCommands[MAX_WAIT_CMD];
	DWORD		m_dwCBTypes[MAX_WAIT_CMD];
	DWORD		m_dwCBDatas[MAX_WAIT_CMD];
	int			m_bCommandSend;
	CNetTimer	*m_pTimer;
};

class CNetRawTcpAcceptor:public INetAcceptor, public IAcceptorConnectorSink
{
public:
	CNetRawTcpAcceptor(INetAcceptorSink *pSink);
	~CNetRawTcpAcceptor();
	/*
	 *	INetAcceptor
	 */
	int StartListen(unsigned short wPort, unsigned long dwLocalAddr=0/*INADDR_ANY*/);
	int StopListen();
	int GetType(){return NETWORK_CLASS_TYPE_TCP_APT_RAW;}
	
	/*
	 *	
	 */
	int OnConnectIndication (int aReason, ITransport *aTrans);

private:
	INetAcceptorSink *m_pSink;
	IAcceptor *m_pApt;
	
};
class CNetTcpAcceptor : public ITransConAcceptorSink, public INetAcceptor
{
public :
	CNetTcpAcceptor (INetAcceptorSink *pSink);
	~CNetTcpAcceptor();
	/*
	 *	INetAcceptor
	 */
	int StartListen(unsigned short wPort, unsigned long dwLocalAddr=0/*INADDR_ANY*/);
	int StopListen();
	int GetType(){return NETWORK_CLASS_TYPE_TCP_APT;}
	
	/*
	 *	ITransConAcceptorSink
	 */
	int OnConnectIndication (ITransCon *pTransCon);
private:
	INetAcceptorSink *m_pSink;
	CTransConTcpAcceptor *m_pApt;
};

#endif

