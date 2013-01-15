/*------------------------------------------------------------------------*/
/*                                                                        */
/*  transport connection manager                                          */
/*  transconmanager.h                                                     */
/*                                                                        */
/*  transport connection manager                                          */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/

#ifndef __TRANS_CON_MANAGER_H
#define __TRANS_CON_MANAGER_H
#include "transconapi.h"
#include "networkbase.h"
#include "utilbase.h"
#include "Reactor.h"

#ifndef TRANS_CON_SUPPORT_UDP
#define TRANS_CON_SUPPORT_UDP
#endif

// budingc
//#define TRANS_CON_KEEP_ALIVE_TIME	10000 //10 seconds
#define TRANS_CON_KEEP_ALIVE_TIME 10000
#define TRANS_CON_KEEP_ALIVE_MAX_NUMBER 8

class CTimeValue;

class CKeepAliveTimer:public CEventHandlerBase
{
private:
	ITransCon	*m_pMngPnt;
	BOOL			bScheduled;
public:
	CKeepAliveTimer(ITransCon *pMngPnt);
	~CKeepAliveTimer();
	int OnTimeout(const CTimeValue &aCurTime, LPVOID aArg);
	void Schedule(long msec);
	void Cancel(void);
};

class CTransConManager:public ITransConManager
{
public:
	CTransConManager();
	virtual ~CTransConManager();
	
	void AddRef(void);
	void Release(void);

	ITransConAcceptor* CreateTransConAcceptor(ITransConAcceptorSink *pSink, 
		unsigned long dwType = TYPE_TCP);

	ITransCon* CreateTransCon(ITransConSink *pSink, 
		unsigned long dwType = TYPE_TCP);

	void DestroyTransConAcceptor(ITransConAcceptor *pApt);
	void DestroyTransCon(ITransCon *pTransCon);

	void SetTransConSink(ITransCon *pTransCon, 
		ITransConSink *pSink);

	void DestroyManager(void);
private:
	ITransCon	*m_pDeadTransCon;
	DWORD		m_dwRef;
};

extern "C"
{
	CTransConManager *GetTransConManager(void);
	void DestoryTransConManager();
	CTransConManager *GetTransConManager(void);
}

#endif

