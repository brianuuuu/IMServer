/*------------------------------------------------------*/
/* CProtocolBase adapter for ITtransportSink            */
/*                                                      */
/* ProtocolAdapterTransportSink.h                       */
/*                                                      */
/*                                                      */
/* History                                              */
/*                                                      */
/*	12/01/2003	Create                                  */
/*                                                      */
/*------------------------------------------------------*/

#ifndef TRANSPORTINTERFACE_H
#define TRANSPORTINTERFACE_H

class IAcceptor;
class IConnector;
class IAcceptorConnectorSink;
class ITransport;
class ITransportSink;
class CDataBlock;
class CInetAddr;

class IAcceptorConnectorSink
{
public:
	virtual int OnConnectIndication(int aReason, ITransport *aTrans) = 0;

	virtual ~IAcceptorConnectorSink() {}
};

class IAcceptor
{
public:
	virtual int StartListen(const CInetAddr &aAddr, DWORD aBacklog) = 0;

	virtual int StopListen(int iReason = 0) = 0;

	virtual ~IAcceptor() {}
};

class IConnector
{
public:
	virtual int Connect(const CInetAddr &aAddr, DWORD aType, DWORD aTmOut = 0, LPVOID aSetting = NULL) = 0;

	virtual ~IConnector() {}
};

class ITransportSink
{
public:
	virtual int OnDisconnect(int aReason) = 0;

	virtual int OnReceive(CDataBlock &aData) = 0;

	virtual int OnSend() = 0;

	virtual ~ITransportSink() {}
};

class ITransport
{
public:
	virtual int Open(ITransportSink *aSink) = 0;

	virtual void Destroy(int aReason = REASON_SUCCESSFUL) = 0;

	/// if succeeds return 0, can't buffered return -1, 
	/// it would never fail.
	virtual int SendData(CDataBlock &aData) = 0;

	/// the <aCmd> is such WUO_TRANSPORT_OPT_GET_FIO_NREAD, etc.
	virtual int IOCtl(int aCmd, LPVOID aArg) = 0;

	virtual ~ITransport() {}
};


#endif // !TRANSPORTINTERFACE_H

