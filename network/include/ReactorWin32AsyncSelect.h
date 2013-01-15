/*------------------------------------------------------*/
/* Reacotr for WIN32 AsyncSelect                        */
/*                                                      */
/* ReactorWin32AsyncSelect.h                            */
/*                                                      */
/*                                                      */
/* History                                              */
/*                                                      */
/*	11/24/2003	Create                                  */
/*                                                      */
/*------------------------------------------------------*/

#ifndef REACTORWIN32ASYNCSELECT_H
#define REACTORWIN32ASYNCSELECT_H

#ifndef WIN32
 #error ERROR: only WIN32 supports Win32AsyncSelect!
#endif

#include "Reactor.h"
#include <map>

class CTimerQueueBase;

class CReactorWin32AsyncSelect : public IReactorImpl  
{
public:
	CReactorWin32AsyncSelect();
	virtual ~CReactorWin32AsyncSelect();

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

	virtual CTimerQueueBase* GetTimerQueue();

	virtual int ModifyHandleSignal( CEventHandlerBase *aEh, bool bEpollout );

	CEventHandlerBase* GetEventHandle(CM_HANDLE aFd);

private:
	static BOOL Win32SocketStartup(HINSTANCE aInstance);
	static void Win32SocketCleanup(HINSTANCE aInstance);
	static BOOL s_bSocketInited;
	static LRESULT CALLBACK Win32SocketWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	struct CHandlerElement
	{
		CM_HANDLE m_Fd;
		CEventHandlerBase *m_pEh;
		CEventHandlerBase::MASK m_Mask;

		CHandlerElement(CM_HANDLE aFd = CM_INVALID_HANDLE, 
						CEventHandlerBase *aEh = NULL,
						CEventHandlerBase::MASK aMask = CEventHandlerBase::NULL_MASK)
			: m_Fd(aFd), m_pEh(aEh), m_Mask(aMask)
		{
		}

		bool operator == (const CHandlerElement &aRight)
		{
			return m_Fd == aRight.m_Fd;
		}
	};
	typedef std::map<CM_HANDLE, CHandlerElement> HandlersType;
	HandlersType m_Handlers;

	int HandleSocketEvent(CM_HANDLE aFd, CEventHandlerBase::MASK aMask, int aErr);
	int RemoveSocket(const HandlersType::iterator &aIter, CEventHandlerBase::MASK aMask);
	int HandleNotifyEvent(CEventHandlerBase *aEh, CM_HANDLE aFd, CEventHandlerBase::MASK aMask);
	int HandleTimerTick();
	int DoAsyncSelect(CM_HANDLE aFd, CEventHandlerBase::MASK aMask);
	
	HWND m_hwndNotify;
	CTimerQueueBase *m_pTimerQueue;
	UINT m_dwTimerId;
	BOOL m_bQuit;
};

#endif // !REACTORWIN32ASYNCSELECT_H

