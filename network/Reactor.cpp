
#include "CmBase.h"
#include "Reactor.h"

#ifdef WIN32
#include "ReactorWin32AsyncSelect.h"
typedef CReactorWin32AsyncSelect CReactorOS;
#endif

#ifdef LINUX
#include "ReactorRealTimeSignal.h"
typedef CReactorRealTimeSignal CReactorOS;
#endif

//////////////////////////////////////////////////////////////////////
// class CEventHandlerBase
//////////////////////////////////////////////////////////////////////

CM_HANDLE CEventHandlerBase::GetHandle() const 
{
	CM_ASSERTE(!"CEventHandlerBase::GetHandle()");
	return CM_INVALID_HANDLE;
}

int CEventHandlerBase::OnInput(CM_HANDLE )
{
	CM_ASSERTE(!"CEventHandlerBase::OnInput()");
	return -1;
}

int CEventHandlerBase::OnOutput(CM_HANDLE )
{
	CM_ASSERTE(!"CEventHandlerBase::OnOutput()");
	return -1;
}

int CEventHandlerBase::OnException(CM_HANDLE )
{
	CM_ASSERTE(!"CEventHandlerBase::OnException()");
	return -1;
}

int CEventHandlerBase::OnTimeout(const CTimeValue &, LPVOID )
{
	CM_ASSERTE(!"CEventHandlerBase::OnTimeout()");
	return -1;
}

int CEventHandlerBase::OnClose(CM_HANDLE , MASK )
{
	CM_ASSERTE(!"CEventHandlerBase::OnClose()");
	return -1;
}


//////////////////////////////////////////////////////////////////////
// class CReactor
//////////////////////////////////////////////////////////////////////


CReactor*         CReactor::s_pInstance = NULL;
CReactor::MUTEX   CReactor::m_mutexInstance;

CReactor::CReactor(IReactorImpl *aImpl)
	: m_pImplementation(aImpl)
	, m_bRunning(FALSE)
{
}

CReactor::~CReactor()
{
	if (m_pImplementation) {
		m_pImplementation->Close();
		delete m_pImplementation;
		m_pImplementation = NULL;
	}
}


CReactor* CReactor::GetInstance()
{
	if (!s_pInstance) {
                Lock<MUTEX> guard(m_mutexInstance);
                if ( !s_pInstance )
                {
		    s_pInstance = new CReactor;
                }
	}
	return s_pInstance;
}

void CReactor::DestroyInstance()
{
    Lock<MUTEX> guard(m_mutexInstance);
    if ( s_pInstance )
    {
        delete s_pInstance;
        s_pInstance = NULL;
    }
}

int CReactor::Open()
{
	if (!m_pImplementation) {
		m_pImplementation = new CReactorOS();
	}
	CM_ASSERTE_RETURN(m_pImplementation, -1);
	return m_pImplementation->Open();
}

int CReactor::Close()
{
	CM_ASSERTE_RETURN(m_pImplementation, -1);
	return m_pImplementation->Close();
}

int CReactor::RegisterHandler(CEventHandlerBase *aEh, 
							  CEventHandlerBase::MASK aMask)
{
	CM_ASSERTE_RETURN(m_pImplementation, -1);
	return m_pImplementation->RegisterHandler(aEh, aMask);
}

int CReactor::ModifyHandleSignal(CEventHandlerBase *aEh, 
							  bool bEpollOut)
{
	CM_ASSERTE_RETURN(m_pImplementation, -1);
	return m_pImplementation->ModifyHandleSignal(aEh, bEpollOut);
}

int CReactor::RemoveHandler(CEventHandlerBase *aEh, 
							CEventHandlerBase::MASK aMask)
{
	CM_ASSERTE_RETURN(m_pImplementation, -1);
	return m_pImplementation->RemoveHandler(aEh, aMask);
}

int CReactor::NotifyHandler(CEventHandlerBase *aEh, 
							CEventHandlerBase::MASK aMask)
{
	CM_ASSERTE_RETURN(m_pImplementation, -1);
	return m_pImplementation->NotifyHandler(aEh, aMask);
}

int CReactor::ScheduleTimer(CEventHandlerBase *aEh, 
							LPVOID aArg, 
							const CTimeValue &aInterval,
							DWORD aCount)
{
	CM_ASSERTE_RETURN(m_pImplementation, -1);
	return m_pImplementation->ScheduleTimer(aEh, aArg, aInterval, aCount);
}

int CReactor::CancelTimer(CEventHandlerBase *aEh)
{
	CM_ASSERTE_RETURN(m_pImplementation, -1);
	return m_pImplementation->CancelTimer(aEh);
}

int CReactor::RunEventLoop()
{
	CM_ASSERTE_RETURN(m_pImplementation, -1);
	return m_pImplementation->RunEventLoop();
}

int CReactor::StopEventLoop()
{
	CM_ASSERTE_RETURN(m_pImplementation, -1);
	return m_pImplementation->StopEventLoop();
}

CTimerQueueBase* CReactor::GetTimerQueue()
{
	CM_ASSERTE_RETURN(m_pImplementation, NULL);
	return m_pImplementation->GetTimerQueue();
}

int CReactor::EnqueueCommandRequest(CCommandRequest* pCommandRequest)
{
        CM_ASSERTE_RETURN(m_pImplementation, NULL);
        return m_pImplementation->EnqueueCommandRequest( pCommandRequest );
}
