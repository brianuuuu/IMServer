#ifndef __NETWORK_IMPL_H_
#define __NETWORK_IMPL_H_

#ifdef WIN32
#	ifdef LIBNETWORK_DLL_EXPORTS
#		define LIBNETWORK_EXPORT __declspec(dllexport)
#	else
#		define LIBNETWORK_EXPORT __declspec(dllimport)
#	endif 
#elif UNIX
#	define LIBNETWORK_EXPORT 
#endif

#include "NetworkInterface.h"
#include "Reactor.h"

class CNetTimer:public INetTimer, public CEventHandlerBase
{
public:
	CNetTimer(INetTimerSink *pSink);
	~CNetTimer();

	void OnTimer(void *pArg, INetTimer *pTimer);
	
	void Schedule(long msec, void *pArg = NULL);
	void Cancel();
	int OnTimeout(const CTimeValue &aCurTime, LPVOID aArg);
private:
	BOOL			m_bScheduled;
	INetTimerSink	*m_pSink;
};
#endif

