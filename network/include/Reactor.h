/*------------------------------------------------------*/
/* Reacotr wrapper                                      */
/*                                                      */
/* Reactor.h                                            */
/*                                                      */
/*                                                      */
/* History                                              */
/*                                                      */
/*	11/24/2003	Create                                  */
/*                                                      */
/*------------------------------------------------------*/

#ifndef REACTOR_H
#define REACTOR_H

#include "Mutex.h"

class CTimeValue;
class CTimerQueueBase;

/**
 * @class CEventHandlerBase
 *
 * @brief Provides an abstract interface for handling various types of
 * I/O, timer events.
 *
 * Subclasses read/write input/output on an I/O descriptor,
 * handle an exception raised on an I/O descriptor, handle a
 * timer's expiration.
 */
class  CEventHandlerBase
{
public:
	typedef long MASK;
	enum 
	{
		NULL_MASK = 0,
		ACCEPT_MASK = (1 << 0),
		CONNECT_MASK = (1 << 1),
		READ_MASK = (1 << 2),
		WRITE_MASK = (1 << 3),
		EXCEPT_MASK = (1 << 4),
		TIMER_MASK = (1 << 5),
		ALL_EVENTS_MASK = READ_MASK |
                      WRITE_MASK |
                      EXCEPT_MASK |
                      ACCEPT_MASK |
                      CONNECT_MASK |
                      TIMER_MASK,
		SHOULD_CALL = (1 << 6)
	};

	virtual CM_HANDLE GetHandle() const ;
	
	/// Called when input events occur (e.g., data is ready).
	/// OnClose() will be callbacked if return -1.
	virtual int OnInput(CM_HANDLE aFd = CM_INVALID_HANDLE);

	/// Called when output events are possible (e.g., when flow control
	/// abates or non-blocking connection completes).
	/// OnClose() will be callbacked if return -1.
	virtual int OnOutput(CM_HANDLE aFd = CM_INVALID_HANDLE);

	/// Called when an exceptional events occur (e.g., OOB data).
	/// OnClose() will be callbacked if return -1.
	virtual int OnException(CM_HANDLE aFd = CM_INVALID_HANDLE);

	/**
	 * Called when timer expires.  <aCurTime> represents the current
	 * time that the <CEventHandlerBase> was selected for timeout
	 * dispatching and <aArg> is the asynchronous completion token that
	 * was passed in when <ScheduleTimer> was invoked.
	 * the return value is ignored.
	 */
	virtual int OnTimeout(const CTimeValue &aCurTime, LPVOID aArg);

	/**
	 * Called when a <On*()> method returns -1 or when the
	 * <RemoveHandler> method is called on an <CReactor>.  The
	 * <aMask> indicates which event has triggered the
	 * <HandleClose> method callback on a particular <aFd>.
	 */
	virtual int OnClose(CM_HANDLE aFd, MASK aMask);
	
	virtual ~CEventHandlerBase() { };
};

class CCommandRequest;
/**
 * @class IReactorImpl
 *
 * @brief An abstract class for implementing the Reactor Pattern.
 */
class IReactorImpl
{
public:
	virtual int Open() = 0;

	virtual int Close() = 0;

	virtual int RegisterHandler(CEventHandlerBase *aEh, 
						CEventHandlerBase::MASK aMask) = 0;

	virtual int RemoveHandler(CEventHandlerBase *aEh, 
					  CEventHandlerBase::MASK aMask) = 0;

	virtual int NotifyHandler(CEventHandlerBase *aEh, 
					  CEventHandlerBase::MASK aMask) = 0;

	virtual int ScheduleTimer(CEventHandlerBase *aEh, 
					  LPVOID aArg,
					  const CTimeValue &aInterval,
					  DWORD aCount) = 0;

	virtual int CancelTimer(CEventHandlerBase *aEh) = 0;

	virtual int RunEventLoop() = 0;

	virtual int StopEventLoop() = 0;

        virtual int EnqueueCommandRequest(CCommandRequest* pCommandRequest) = 0;

	virtual CTimerQueueBase* GetTimerQueue() = 0;

	virtual ~IReactorImpl() { } ;

	virtual int ModifyHandleSignal( CEventHandlerBase *aEh, bool bEpollout ) = 0;
};


/**
 * @class CReactor
 *
 * @brief The responsibility of this class is to forward all methods to
 * its delegation/implementation class, e.g.,
 * <CReactorSelect> or <CReactorRtSignal>.
 */


class  CReactor  
{
public:
	CReactor(IReactorImpl *aImpl = NULL);
	~CReactor();

	/// Set and Get pointer to a process-wide <CReactor>.
	static CReactor* GetInstance();
        static void DestroyInstance();
	/// Initialize the <CReactor>
	int Open();

	/// Close down and release all resources.
	int Close();

	/// Register <aEh> with <aMask>.  The I/O handle will always
	/// come from <GetHandle> on the <aEh>.
	int RegisterHandler(CEventHandlerBase *aEh, 
						CEventHandlerBase::MASK aMask);

	int ModifyHandleSignal(CEventHandlerBase *aEh, 
						bool bEpollOut);

	/**
	 * Removes <CEventHandlerBase> associated with <aFd>.  If
	 * <aMask> includes <CEventHandlerBase::SHOULD_CALL> then the
	 * <OnClose> method of the <aEh> is invoked.
	 */
	int RemoveHandler(CEventHandlerBase *aEh, 
		CEventHandlerBase::MASK aMask = CEventHandlerBase::ALL_EVENTS_MASK);

	/// Notify <aEh> of <aMask> event.
	int NotifyHandler(CEventHandlerBase *aEh, 
					  CEventHandlerBase::MASK aMask);

	/// Schedule an <aEh> that will expire after an amount of time <aInterval>
	/// for <aCount> times. if <aCount> is 0, schedule infinite times.
	int ScheduleTimer(CEventHandlerBase *aEh, 
					  LPVOID aArg,
					  const CTimeValue &aInterval,
					  DWORD aCount);

	/// Cancel the timer <aEh>.
	int CancelTimer(CEventHandlerBase *aEh);

	/// Run the event loop until be stopped by invoke Stop()
	int RunEventLoop();

        int EnqueueCommandRequest( CCommandRequest* pCommandRequest );
	/// Instruct the Reactor to terminate its event loop and notifies the
	/// Reactor so that it can wake up and close down gracefully.
	int StopEventLoop();

	CTimerQueueBase* GetTimerQueue();

private:
        typedef Mutex MUTEX;
        //to guard the instance access
        static MUTEX    m_mutexInstance;
	static CReactor *s_pInstance;

	IReactorImpl *m_pImplementation;
	BOOL m_bRunning;
};

#endif // !REACTOR_H

