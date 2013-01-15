/*************************************************************
 *	NetworkInterface.h Network Interface to up layer
 *
 *	Created by Frank Song 2005/6/16
 *
 *	All the data used is in host order
 *
 ************************************************************/

#ifndef __NETWORK_INTERFACE_H_
#define __NETWORK_INTERFACE_H_

#ifndef LIBNETWORK_EXPORT
#define LIBNETWORK_EXPORT 
#endif

#define DATA_NEED_CALLBACK					0x1
#define REASON_SENDCOMMADN_TIMEOUT			0x2

/*
 *	Class Type
 */
#define NETWORK_CLASS_TYPE_UDP_CONN_RAW		0x1
#define NETWORK_CLASS_TYPE_UDP_CONN			0x2
#define NETWORK_CLASS_TYPE_UDP_CONN_MEDIA	0x3
#define NETWORK_CLASS_TYPE_UDP_APT_RAW		0x4
#define NETWORK_CLASS_TYPE_UDP_APT			0x5
#define NETWORK_CLASS_TYPE_UDP_APT_MEDIA	0x6

#define NETWORK_CLASS_TYPE_TCP_CONN_RAW		0x81
#define NETWORK_CLASS_TYPE_TCP_CONN			0x82
#define NETWORK_CLASS_TYPE_TCP_CONN_MEDIA	0x83
#define NETWORK_CLASS_TYPE_TCP_APT_RAW		0x84
#define NETWORK_CLASS_TYPE_TCP_APT			0x85
#define NETWORK_CLASS_TYPE_TCP_APT_MEDIA	0x86

/*
 *	Connection type
 */
#define NETWORK_CONNECT_TYPE_AUTO		0
#define NETWORK_CONNECT_TYPE_UDP		1
#define NETWORK_CONNECT_TYPE_TCP		2
#define NETWORK_CONNECT_TYPE_SSL		3
#define NETWORK_CONNECT_TYPE_HTTP		4
/*
 *	Reason
 */
#define NETWORK_REASONSUCCESSFUL				0
#define NETWORK_REASONSERVER_UNAVAILABLE		1 
#define NETWORK_REASONSERVER_DISCONNECT			2
#define NETWORK_REASONPEER_DISCONNECT			3
#define NETWORK_REASONSERVER_ERROR				4
#define NETWORK_REASONSOCKET_ERROR				5
#define NETWORK_REASONBIND_ERROR				6
#define NETWORK_REASONKEEPALIVE_TIMEOUT			7
#define NETWORK_REASONSERVER_DELETE				8
#define NETWORK_REASONCONNECT_TIMEOUT			9
#define NETWORK_REASONDNS_ERROR					10
#define NETWORK_REASONPEER_REJECT				11
#define NETWORK_REASONSERVER_USER_DEL_PENDING	12
#define NETWORK_REASONSERVER_LOAD_OVERFLOW		13
#define NETWORK_REASONUNKNOWN_ERROR				14
#define NETWORK_REASONPROXY_NEED_AUTH			15
/*
 *	Option
 */
#define NETWORK_MEDIA_OPTION_BASE					0x28000
/*
 *	Pointer to ulong
 */
#define NETWORK_MEDIA_OPTION_SET_SEND_BITRATE		(NETWORK_MEDIA_OPTION_BASE+1)
#define NETWORK_MEDIA_OPTION_REST_BUFFER			(NETWORK_MEDIA_OPTION_BASE+2)
/*
 *	Network
 */
#define NETWORK_NETWORK_OPTION_GET_BASE				0x30000

//Parameter is a pointer to ulong
#define NETWORK_NET_OPT_TYPE_GET_CON_TYPE			(NETWORK_NETWORK_OPTION_GET_BASE+1)

//Parameter is a pointer to ulong
//TYPE_TCP, TYPE_UDP ...
#define NETWORK_NET_OPT_TYPE_GET_TRANS_TYPE			(NETWORK_NETWORK_OPTION_GET_BASE+2)

#define NETWORK_TRANSPORT_OPT_GET_BASE				0x32000
//Param. is Pointer to DWORD
//IO Read len
#define NETWORK_TRANSPORT_OPT_GET_FIO_NREAD			(NETWORK_TRANSPORT_OPT_GET_BASE+1)
//Param. is Pointer to DWORD
//Transport unread len
#define NETWORK_TRANSPORT_OPT_GET_TRAN_NREAD		(NETWORK_TRANSPORT_OPT_GET_BASE+2)
//Param. is pointer to fd
//Get fd
#define NETWORK_TRANSPORT_OPT_GET_FD				(NETWORK_TRANSPORT_OPT_GET_BASE+3)
//Param. is Pointer to struct sockaddr
//Get address socket binded
#define NETWORK_TRANSPORT_OPT_GET_LOCAL_ADDR		(NETWORK_TRANSPORT_OPT_GET_BASE+4)
//Param. is Pointer to struct sockaddr
//Get peer addr
#define NETWORK_TRANSPORT_OPT_GET_PEER_ADDR			(NETWORK_TRANSPORT_OPT_GET_BASE+5)
//If the socket is still alive
//Param. is Pointer to BOOL
#define NETWORK_TRANSPORT_OPT_GET_SOCK_ALIVE		(NETWORK_TRANSPORT_OPT_GET_BASE+6)
//Param. is Pointer to DWORD
//TYPE_TCP, TYPE_UDP, TYPE_SSL...
#define NETWORK_TRANSPORT_OPT_GET_TRAN_TYPE 		(NETWORK_TRANSPORT_OPT_GET_BASE+7)

// budingc add to support SO_KEEPALIVE function
//Param. is Pointer to DWORD
#define NETWORK_TRANSPORT_OPT_SET_TCP_KEEPALIVE		(NETWORK_TRANSPORT_OPT_GET_BASE+8)

#define NETWORK_SEND_BITRATE_INFINITE				0x1FFFFFFF

#define MEDIA_TYPE_AUDIO							0x1
#define MEDIA_TYPE_VIDEO							0x2

struct ProxySetting 
{
	unsigned long ProxyAddr;
	unsigned short ProxyPort;
	char UserName[128]; /*the UserName[0] should be '\0' if no user name present*/
	char Password[128]; /*the Password[0] should be '\0' if no password present*/
};

class INetConnection;
class INetAcceptor;
class LIBNETWORK_EXPORT INetConnectionSink
{
public:
	virtual int OnConnect(int nReason, INetConnection *pCon) = 0;
       //WUJB,after OnDisconnect is called, sink implementor should call NetworkDestroyConnection(pCon);
	virtual int OnDisconnect(int nReason, INetConnection *pCon) = 0;
	virtual int OnReceive(unsigned char *pData, int nLen, INetConnection *pCon) = 0;
	virtual int OnSend(INetConnection *pCon) = 0;
	/*
	 *	Frank For Reliable TCP
	 */
	virtual int OnSend(int nReason, unsigned char* pBuf,  int nLen, unsigned long dwCallBackType, unsigned long  dwCallBackData, 
					   INetConnection* pCon){return 0;}
	virtual int OnCommand(unsigned char *pData, int nLen, INetConnection *pCon){return 0;}
	virtual ~INetConnectionSink(){}
};

class LIBNETWORK_EXPORT INetConnection
{
public:
	virtual int Connect(
		unsigned long	dwAddr, 
		unsigned short	wPort, 
		int				nType,
		void			*pProxySetting = NULL
		) = 0;
	
	virtual void Disconnect(int iReason = 0) = 0;
	virtual void SetSink(INetConnectionSink *pSink) = 0;
	virtual int SendData(unsigned char *pData, int nLen) = 0;
	
	virtual int SendCommand(unsigned char *pData, int nLen){return -1;}
	/*
	 *	Frank For Reliable TCP
	 */
	virtual int SendCommand(unsigned char *pData, int nLen, unsigned long dwCallBackType, unsigned long dwCallBackData){return -1;}
	virtual int SendMedia(unsigned char *pHeader, int nHeaderLen, 
		char *pData, int nLen, unsigned char nMediaType, unsigned char nSubSeq, 
		unsigned short usIFrameSeq)	{return -1;}

	virtual int SendTo(unsigned char *pData, int nLen, 
		unsigned long dwAddr, unsigned short wPort) = 0;
	virtual int SetOpt(unsigned long OptType, void *pParam) = 0;
	virtual int GetOpt(unsigned long OptType, void *pParam) = 0;
	virtual int GetType() = 0;

        //Async call for the caller in different thread
        virtual int ASendCommand(unsigned char* pData, int nLen);

	virtual ~INetConnection(){};
};

class LIBNETWORK_EXPORT INetAcceptorSink
{
public:
	virtual int OnConnectIndication(INetConnection *pCon, INetAcceptor *pApt) = 0;
	
	virtual ~INetAcceptorSink() {}
};

class LIBNETWORK_EXPORT INetAcceptor
{
public:
	virtual int StartListen(unsigned short wPort, unsigned long dwLocalAddr=0/*INADDR_ANY*/) = 0;
	
	virtual int StopListen() = 0;
	virtual int GetType()=0;
	
	virtual ~INetAcceptor() {}
};
class INetTimer;
class LIBNETWORK_EXPORT INetTimerSink
{
public:
       //TODO:WUJB, the reason for pTimer to sink?
	virtual void OnTimer(void *pArg, INetTimer *pTimer) = 0;
	virtual ~INetTimerSink(){}
};

class LIBNETWORK_EXPORT INetTimer 
{
public:
	virtual void Schedule(long msec, void *pArg) = 0;
	virtual void Cancel() = 0;
	virtual ~INetTimer(){}
};


extern "C"
{
	LIBNETWORK_EXPORT int NetworkInit();
	LIBNETWORK_EXPORT void NetworkFini();
	/*
	 *	Application on WIN doesn't need call NetworkRunEventLoop
	 */
	LIBNETWORK_EXPORT void NetworkRunEventLoop();
	LIBNETWORK_EXPORT void NetworkDestroyConnection(INetConnection *pCon);
	LIBNETWORK_EXPORT void NetworkDestroyAcceptor(INetAcceptor *pApt);
	LIBNETWORK_EXPORT void NetworkDestroyTimer(INetTimer *pTimer);

	/*
	 *	Acceptor
	 */
	LIBNETWORK_EXPORT INetAcceptor *CreateUdpAcceptor(INetAcceptorSink *pSink);
	LIBNETWORK_EXPORT INetAcceptor *CreateUdpMediaAcceptor(INetAcceptorSink *pSink);
	
	LIBNETWORK_EXPORT INetAcceptor *CreateRawTcpAcceptor(INetAcceptorSink *pSink);
	LIBNETWORK_EXPORT INetAcceptor *CreateTcpAcceptor(INetAcceptorSink *pSink);
	LIBNETWORK_EXPORT INetAcceptor *CreateTcpMediaAcceptor(INetAcceptorSink *pSink);
	
	/*
	 *	Connection
	 */
	LIBNETWORK_EXPORT INetConnection *CreateRawUdpCon(INetConnectionSink *pSink, 
		unsigned long dwLocalAddr, unsigned short wLocalPort);
	LIBNETWORK_EXPORT INetConnection *CreateUdpCon(INetConnectionSink *pSink);
	LIBNETWORK_EXPORT INetConnection *CreateUdpMediaCon(INetConnectionSink *pSink, 
		 INetAcceptor *pApt);

	LIBNETWORK_EXPORT INetConnection *CreateRawTcpCon(INetConnectionSink *pSink);
	LIBNETWORK_EXPORT INetConnection *CreateTcpCon(INetConnectionSink *pSink);
	LIBNETWORK_EXPORT INetConnection *CreateTcpMediaCon(INetConnectionSink *pSink);
        
	/*
	 *	Timer
	 */
	LIBNETWORK_EXPORT INetTimer *CreateNetTimer(INetTimerSink *pSink);

        /*
         *Async interface: call this if the caller in different thread
         */

        LIBNETWORK_EXPORT void ANetworkDestroyConnection(INetConnection *pCon);

}

#endif //__NETWORK_INTERFACE_H_

