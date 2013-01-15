#ifndef REACTORREALTIMESIGNAL_H
#define REACTORREALTIMESIGNAL_H

#ifndef LINUX
 #error ERROR: only LINUX supports RealTimeSignal!
#endif // LINUX

#include "Reactor.h"
#include "MTPtrMap.h"
#include "CommandQueue.h"

class CTimerQueueBase;

class CReactorRealTimeSignal 
	: public IReactorImpl  
{
public:
	CReactorRealTimeSignal();
	virtual ~CReactorRealTimeSignal();

	virtual int Open();

	virtual int Close();

	virtual int RegisterHandler(CEventHandlerBase *aEh, 
						CEventHandlerBase::MASK aMask);

	virtual int RemoveHandler(CEventHandlerBase *aEh, 
					  CEventHandlerBase::MASK aMask);

	virtual int NotifyHandler(CEventHandlerBase *aEh, 
					  CEventHandlerBase::MASK aMask);

	virtual int ScheduleTimer(CEventHandlerBase *aEh, 
					  LPVOID aArg,
					  const CTimeValue &aInterval,
					  DWORD aCount);

	virtual int CancelTimer(CEventHandlerBase *aEh);

	virtual int RunEventLoop();

	virtual int StopEventLoop();

    virtual int EnqueueCommandRequest( CCommandRequest* pCommandRequest );
    virtual CTimerQueueBase* GetTimerQueue();
	virtual int ModifyHandleSignal( CEventHandlerBase *aEh, bool bEpollout );
	
private:
    typedef std::pair<CEventHandlerBase*, CEventHandlerBase::MASK> EHPair;
    typedef CMTPtrMap<CNullMutex, CM_HANDLE, EHPair*> HANDLERS;

	int HandleIoEvents(const CTimeValue &aTimeout);
	int RemoveSocket_i(CM_HANDLE aFd,
		EHPair* pEHFind,
		CEventHandlerBase::MASK aMask);
	int CheckPollIn(int aFd, CEventHandlerBase *aEh);
	int SetProcRtsigMax(int aMaxNum);
	int SetRlimit(int aResource, int aMaxNum);
	int SetHandleSignal( CM_HANDLE fdNew );
	
    int BindSocket();

private:
	sigset_t m_Sigset;
	int m_SigNum;
	int m_epfd;
	
	CTimerQueueBase *m_pTimerQueue;
	BOOL m_bQuit;
    int  m_pipe[2];
    CCommandQueue m_cmdQueue;
    HANDLERS m_handlers;
    int  m_sockSyncReceive;
    int  m_sockSyncSend;
    unsigned short m_sSyncPort;
};

#endif // !REACTORREALTIMESIGNAL_H
