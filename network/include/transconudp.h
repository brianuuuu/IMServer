/*------------------------------------------------------------------------*/
/*                                                                        */
/*  UDP transport connection                                              */
/*  transconudp.h                                                         */
/*                                                                        */
/*  definition of UDP transport connection                                */
/*                                                                        */
/*  All rights reserved                                                   */
/*                                                                        */
/*------------------------------------------------------------------------*/
#ifndef __TRANS_CON_UDP_H
#define __TRANS_CON_UDP_H
#include "utilbase.h"
#include "networkbase.h"
#include "datablk.h"
#include "transconapi.h"
#include "ClientSocketUDP.h"
#include "transconmanager.h"
#include "Addr.h"
#include "NetworkImpl.h"

#define TRANS_CON_UDP_RECEIVE_BUF_SIZE		65536
#define TRANS_CON_UDP_HASH_SIZE             32768

#define TRANS_CON_UDP_RCT_STATE_STARTED		0x1
#define TRANS_CON_UDP_RCT_STATE_WAITACK		0x2
#define TRANS_CON_UDP_CON_STATE_STARTED     0x3
#define TRANS_CON_UDP_CON_STATE_WAITACK		0x4
#define TRANS_CON_UDP_STATE_CONNECTED		0x5
#define TRANS_CON_UDP_STATE_DISCONNECTED	0x6


class CUdpReactiveTransCon;

class CTransConUdpAcceptor
	: public ITransConAcceptor
	, public IClientSocketUDPSink
	, public INetTimerSink
{
public:
	CTransConUdpAcceptor(ITransConAcceptorSink *pSink, DWORD dwType = TYPE_UDP);
	virtual ~CTransConUdpAcceptor();

	int Init(void);
	int Clean (void);
	
	int StartListen(const char	*pAddr, unsigned short wPort, 
		unsigned short bPortAutoSearch = 0);
	int StopListen(int iReason = 0);

	int OnReceiveUdp(CDataBlock &aData, const CInetAddr &aAddr);
	int OnCloseUdp(int aErr);
	
	CClientSocketUDP *GetUdpSocket(void);
	CUdpReactiveTransCon *GetTransCon(const sockaddr_in &pAddr);
	void RemoveTransCon(CUdpReactiveTransCon *pTransCon);
	void RemoveTransCon(CInetAddr &cAddr);
	void RawSentTo(CDataBlock	&aData, CInetAddr &cAddr);	
	ITransConAcceptorSink *GetSink();
	void OnTimer(void *pArg, INetTimer *pTimer);

private:
	DWORD GetHashCode(const sockaddr_in &pAddr);
	
private:
	ITransConAcceptorSink*  m_pSink;
	CClientSocketUDP		m_cUdpSocket;
	list<CUdpReactiveTransCon*> m_pTranConList[TRANS_CON_UDP_HASH_SIZE];

	sockaddr_in		m_cAddr;
	WORD			m_wPort;
	
	CNetTimer*      m_pTimer;
	int				m_nLastTimerStart;
	int				m_nTimerCount;
	int 			m_nConnectionCount;
};

class CUdpReactiveTransCon
	: public ITransCon
{
public:
	CUdpReactiveTransCon(CTransConUdpAcceptor *pApt, 
		ITransConSink *pSink, 
		const CInetAddr &aAddr);
	virtual ~CUdpReactiveTransCon();
	
	int  Init(void);
	int  Clean (void);
	void OnTick(void);
	
	int Connect(const char	*pAddr, 
		unsigned short		wPort, 
		void				*pProxySetting,
		int nType = TYPE_PREV);
	void Disconnect(int iReason = 0);
	int  SendData(CDataBlock	*pData);
	void SetSink(ITransConSink *pSink);

	void OnReceive(CDataBlock &aData, const CInetAddr &aAddr);
	int OnCloseUdp(int aErr);

	void DisconnectByApt(int iReason = 0);
	BOOL CompareAddr(const sockaddr_in &pAddr);
	sockaddr_in *GetPeerAddr(void);

	CDataBlock *BuildDataPdu(CDataBlock *pInBlk);
	CDataBlock *BuildAckPdu(void);
	CDataBlock *BuildAck1Pdu(void);
	CDataBlock *BuildSynPdu(void);
	CDataBlock *BuildFinPdu(void);
	CDataBlock *BuildKeepAlivePdu(void);

	int SetOpt(unsigned long OptType, void *pParam);
	int GetOpt(unsigned long OptType, void *pParam);

private:
	CInetAddr				m_cFirstAddr;
	CInetAddr				m_cInetAddr;
	CInetAddr				m_cAddrTmp;
	DWORD					m_dwConnectionId;
	WORD					m_wConSubId;
	ITransConSink			*m_pSink;
	CTransConUdpAcceptor	*m_pTransApt;
	WORD					m_wState;
	WORD					m_wNextSeqExpect;
	WORD					m_wCurrentSndSeq;
	BOOL					m_bDataReceived;
	BOOL					m_bDataSended;
	WORD					m_wTickCount;

#ifdef TRANS_CON_UDP_SORT
	CDataBlock				*m_pReorderArry[2];
	WORD					m_aReoderySeq[2]
#endif
};

class CUdpConTransCon
	: public IClientSocketUDPSink
	, public ITransCon
{
public:
	CUdpConTransCon(ITransConSink *pSink);
	virtual ~CUdpConTransCon();
	
	int Init(void);
	int Clean (void);
	void OnTick(void);
	
	int Connect(const char	*pAddr, 
		unsigned short		wPort, 
		void				*pProxySetting,
		int nType = TYPE_PREV);
	void Disconnect(int iReason = 0);
	int SendData(CDataBlock	*pData);
	void SetSink(ITransConSink *pSink);

	int OnReceiveUdp(CDataBlock &aData, const CInetAddr &aAddr);
	int OnCloseUdp(int aErr);

	CDataBlock *BuildDataPdu(CDataBlock *pInBlk);
	CDataBlock *BuildSynPdu(void);
	CDataBlock *BuildAckPdu(void);
	CDataBlock *BuildKeepAlivePdu(void);

	int SetOpt(unsigned long OptType, void *pParam);
	int GetOpt(unsigned long OptType, void *pParam);
	
private:
	WORD		m_wCurrentSndSeq;
	WORD		m_wNextSeqExpect;
	WORD		m_wPort;
	BOOL		m_bDataReceived;
	BOOL		m_bDataSended;
	WORD		m_wTickCount;
	WORD		m_wState;
	DWORD		m_dwConnectionId;
	WORD		m_wConSubId;
	CInetAddr	m_cInetAddr;
	ITransConSink		*m_pSink;
	CKeepAliveTimer		*m_pTimer;
	CClientSocketUDP	m_cUdpSocket;
};
#endif //__TRANS_CON_UDP_H
