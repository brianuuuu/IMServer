#ifndef _VIGO_NET_TRANSPORT_HTTP_H_
#define _VIGO_NET_TRANSPORT_HTTP_H_

#include "NetworkInterface.h"
#include "TransportInterface.h"
#include "TimeValue.h"

#define HTTP_CONN_FIRST_CON_COMPLETE		1
#define HTTP_CONN_FIRST_CON_FAIL			2
#define HTTP_CONN_SECOND_CON_COMPLETE		3
#define HTTP_CONN_SECOND_CON_FAIL			4
#define HTTP_CONN_FIRST_CON_NEED_AUTH		5

class CTransportHttp;
class CTransportStub;

enum tunnel_request
{
	TUNNEL_SIMPLE     = 0x40,
	TUNNEL_OPEN       = 0x01,
	TUNNEL_DATA       = 0x02,
	TUNNEL_PADDING    = 0x03,
	TUNNEL_ERROR      = 0x04,
	TUNNEL_PAD1       = TUNNEL_SIMPLE | 0x05,
	TUNNEL_CLOSE      = TUNNEL_SIMPLE | 0x06,
	TUNNEL_DISCONNECT = TUNNEL_SIMPLE | 0x07,
	TUNNEL_ID         = 0x08,
};

class IHttpEventSink
{
public:
	virtual int OnEvent(int nID, CTransportHttp *pHttpTrans) = 0;
	virtual ~IHttpEventSink()
	{

	}
};

class ITransportHttpSink
{
public:
	virtual int OnDisconnect(int aReason, CTransportStub *pStub) = 0;
	virtual int OnReceive(CDataBlock &aData, CTransportStub *pStub) = 0;
	virtual int OnSend(CTransportStub *pStub) = 0;

	virtual ~ITransportHttpSink() 
	{

	}
};

class CTransportStub
	: public ITransportSink
{
public:
	CTransportStub(ITransportHttpSink *pSink = NULL, ITransport *pTrans = NULL);
	~CTransportStub();

public:
	int OnDisconnect(int aReason);
	int OnReceive(CDataBlock &aData);
	int OnSend();

public:
	ITransport	*m_pTrans;
	ITransportHttpSink *m_pSink;
};

class CTransportHttp
	: public ITransport
	, public ITransportHttpSink
{
public:
	CTransportHttp(IHttpEventSink *pEventSink, int bServer);
	~CTransportHttp();

public:
	int  Open(ITransportSink *aSink);
	void Destroy(int aReason = REASON_SUCCESSFUL);
	int  SendData(CDataBlock &aData);
	int  IOCtl(int aCmd, LPVOID aArg);

	int  OnDisconnect(int aReason, CTransportStub *pStub);
	int  OnReceive(CDataBlock &aData, CTransportStub *pStub);
	int  OnSend(CTransportStub *pStub);
	void SetFirstConnection(ITransport *pTrans);
	void SetSecondConnection(ITransport *pTrans);

	void ReceiveRemainBuf();

public:
	CTransportStub	*m_pInstub, *m_pOutStub;
	ITransport		*m_pInTrans, *m_pOutTrans;
	CTransportHttp	*m_pNext;
	IHttpEventSink	*m_pEventSink;
	ITransportSink *m_pSink;
	unsigned long	m_nHttpId;
	CTimeValue		m_cAddTime;
	int				m_bServer;
	int				m_bConnected;
	char			*m_pBuf;
	char			*m_pRemainBuf;
	int				m_nBufLen;
	int						m_bNeedProxy;
	struct ProxySetting		m_proxySetting;
	CInetAddr		m_cAddr;
};

class CAcceptorHttp 
	: public IAcceptorConnectorSink
	, public IAcceptor
	, public IHttpEventSink
{
public:
	CAcceptorHttp(IAcceptorConnectorSink *pSink);
	~CAcceptorHttp();

public:
	int OnEvent(int nID, CTransportHttp *pHttpTrans);
	int OnConnectIndication(int aReason, ITransport *aTrans);
	int StartListen(const CInetAddr &aAddr, DWORD aBacklog);
	int StopListen(int iReason = 0);

private:
	void RemoveHttpTrans(CTransportHttp *pHttpTrans);
	CTransportHttp *FindHttpPair(unsigned long nFindId);

private:
	IAcceptor		*m_pAcceptor;
	IAcceptorConnectorSink	*m_pSink;
	CTransportHttp *m_pHalfCompTrans;
	CTransportHttp	*m_pDeleteTrans;
	unsigned long	m_nHttpId;
};

class CConnectorHttp
	: public IAcceptorConnectorSink
	, public IConnector
	, public IHttpEventSink
{
public:
	CConnectorHttp(IAcceptorConnectorSink *pSink);
	~CConnectorHttp();

public:
	int Connect(const CInetAddr &aAddr, DWORD aType, DWORD aTmOut = 0, LPVOID aSetting = NULL);
	int OnEvent(int nID, CTransportHttp *pHttpTrans);
	int OnConnectIndication(int aReason, ITransport *aTrans);

public:
	struct ProxySetting		m_proxySetting;
	int						m_bNeedProxy;

private:
	IAcceptorConnectorSink	*m_pSink;
	IConnector				*m_pConnector;
	CTransportHttp			*m_pTrans;
	CInetAddr				m_aAddr;
	CInetAddr				m_serverAddr;
	DWORD					m_aType;
	DWORD					m_aTmOut;
};
#endif //#ifndef _VIGO_NET_TRANSPORT_HTTP_H_
