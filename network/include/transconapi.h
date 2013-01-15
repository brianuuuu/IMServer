#ifndef __TRANS_CON_API_H
#define __TRANS_CON_API_H

#include "networkbase.h"

class ITransCon;
class ITransConSink;
class ITransConAcceptor;
class ITransConAcceptorSink;
class CDataBlock;

class  ITransConManager
{
public:

	virtual ITransConAcceptor* CreateTransConAcceptor(
		ITransConAcceptorSink *pSink, 
		unsigned long dwType = TYPE_TCP
		) = 0;

	virtual ITransCon* CreateTransCon(
		ITransConSink *pSink, 
		unsigned long dwType = TYPE_TCP 
		) = 0;

	virtual void DestroyTransConAcceptor(ITransConAcceptor *pApt) = 0;
	virtual void DestroyTransCon(ITransCon *pTransCon) = 0;

	virtual void SetTransConSink(
		ITransCon *pTransCon, 
		ITransConSink *pSink 
		) = 0;

	virtual void DestroyManager(void) = 0;
};

class  ITransConAcceptorSink
{
public:
	virtual int OnConnectIndication (ITransCon *pTransCon) = 0;
};

class  ITransConAcceptor
{
public:
	//pAddr: The address to listen. If it is NULL, 
	//		 listen on INADDR_ANY.
	//nPort: The port to listen. 
	//return value: The port bind. -1 failed.
	virtual int StartListen(const char	*pAddr, unsigned short wPort,
		unsigned short bPortAutoSearch = 0) = 0;

	virtual int StopListen(int iReason = 0) = 0;

	virtual ~ITransConAcceptor(){};
};

class  ITransConSink
{
public:
	// nReason is like ITransConManager::REASON_SUCCESSFUL, etc.
	virtual int OnConnect(int iReason) = 0;

	// nReason is like ITransConManager::REASON_SERVER_UNAVAILABLE, etc.
	// and it will never be ITransConManager::REASON_SUCCESSFUL
	virtual int OnDisconnect(int iReason) = 0;

	//Recevie data
	virtual int OnReceive(
		CDataBlock	*pData
		) = 0;

	virtual int OnSend()=0;
	virtual void OnTransTimer(){return;}
};

class  ITransCon
{
public:
	// if succeeds return 0, otherwise return -1.
	//pProxySetting: May be used in future.
	virtual int Connect(
		const char		*pAddr, 
		unsigned short	wPort, 
		void			*pProxySetting, 
		int				nType = TYPE_PREV
		) = 0;

	// always succeeds
	virtual void Disconnect(int iReason = 0) = 0;

	// if succeeds return 0, otherwise return -1.
	virtual int SendData(
		CDataBlock	*pData
		) = 0;

	virtual void SetSink(ITransConSink *pSink) = 0;
	virtual void OnTick(void) = 0;
	virtual int	 Clean(void) = 0;

	virtual int SetOpt(unsigned long OptType, void *pParam) = 0;
	virtual int GetOpt(unsigned long OptType, void *pParam) = 0;
	
	virtual ~ITransCon(){};
};

extern "C"
{
	 ITransConManager* CreateTransConManager(void);
};

#endif // !__TRANS_CON_INTERFACE_H

