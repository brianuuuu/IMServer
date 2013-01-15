/*------------------------------------------------------------------------*/
/*                                                                        */
/*  TCP transport connection                                              */
/*  transcontcp.h                                                         */
/*                                                                        */
/*  definition of TCP transport connection                                */
/*                                                                        */
/*  All rights reserved                                                   */
/*  History                                                               */
/*  12/3/2003 created Flan Song                                           */
/*------------------------------------------------------------------------*/
#ifndef __TRANS_CON_TCP_H
#define __TRANS_CON_TCP_H

#include "networkbase.h"
#include "TransportInterface.h"
#include "transconapi.h"
#include "transconmanager.h"

#define TRAN_CON_TCP_CONNECT_TIME_OUT	30000/*30 seconds*/
class CDataBlk;

class  CTransConTcpAcceptor:public IAcceptorConnectorSink, public ITransConAcceptor
{
public:
	CTransConTcpAcceptor(ITransConAcceptorSink *pSink, DWORD dwType = TYPE_TCP);
	~CTransConTcpAcceptor();

	int Init(void);
	int Clean(void);

	int StartListen(const char	*pAddr, unsigned short wPort, 
		unsigned short bPortAutoSearch = 0);
	int StopListen(int iReason = 0);
	int OnConnectIndication(int aReason, ITransport *aTrans);

private:
	IAcceptor	*m_pLowApt;		
	ITransConAcceptorSink *m_pSink;
	int			m_nType;
};

class  CTcpTransCon:public ITransCon, public IAcceptorConnectorSink, public ITransportSink
{
public:
	CTcpTransCon(ITransConSink *pSink = NULL, ITransport *pTrans = NULL, 
		DWORD dwTransType = TYPE_TCP, BOOL bConnected = FALSE);
	~CTcpTransCon();

	void SetSink(ITransConSink *pSink);
	void OnTick(void);
	int	Clean(int nReason);
	int	Clean();
	int Init(void);

	int Connect(const char *pAddr, unsigned short wPort, 
		void *pProxySetting, int nType = TYPE_PREV);
	void Disconnect(int iReason = 0);
	int SendData(CDataBlock	*pData);

	int OnConnectIndication(int aReason, ITransport *aTrans);
	int OnDisconnect(int iReason);
	int OnReceive(CDataBlock &aData);
	int OnSend();

	CDataBlock *BuildDataPdu(CDataBlock *pInBlk);
	CDataBlock *BuildKeepAlivePdu(void);

	int SetOpt(unsigned long OptType, void *pParam);
	int GetOpt(unsigned long OptType, void *pParam);

private:
	BOOL	m_bDataReceived;
	BOOL	m_bDataSended;
	BOOL	m_bConnected;
	DWORD	m_dwTickCount;
	DWORD	m_dwTranType;
	CDataBlock		*m_pBlkLastTime;
	CKeepAliveTimer *m_pTimer;
	IConnector		*m_pConnector;
	ITransport		*m_pTransport;
	ITransConSink	*m_pSink;
};

#endif//__TRANS_CON_TCP_H

