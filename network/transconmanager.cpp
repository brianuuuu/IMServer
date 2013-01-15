/*------------------------------------------------------------------------*/
/*                                                                        */
/*  Transport connnection manager										  */
/*  transconmanager.cpp													  */
/*                                                                        */
/*  Implementation of transport connnection manager						  */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
#include "CmBase.h"
#include "transconmanager.h"
#include "transconudp.h"
#include "TimeValue.h"
#include "Reactor.h"
#include "transcontcp.h"
#include "TransportDestoryEvent.h"

static CTransConManager *g_pTransConManager;

extern "C"
{
ITransConManager* CreateTransConManager(void)
{
	if (g_pTransConManager == NULL)
	{
		g_pTransConManager = new CTransConManager;
	}
	else
		g_pTransConManager->AddRef();
	return g_pTransConManager;
}

void DestoryTransConManager()
{
	if (g_pTransConManager != NULL)
	{
		delete g_pTransConManager;
		g_pTransConManager = NULL;
	}
}

CTransConManager *GetTransConManager(void)
{
	return g_pTransConManager;
}

}

/*########################################################################*/
/* CKeepAliveTimer*/
CKeepAliveTimer::CKeepAliveTimer(ITransCon *pMngPnt)
{
	IM_ASSERT(pMngPnt);
	m_pMngPnt = pMngPnt;
	bScheduled = FALSE;
}

CKeepAliveTimer::~CKeepAliveTimer()
{
	if (bScheduled)
		Cancel();
}

int CKeepAliveTimer::OnTimeout(const CTimeValue &aCurTime, LPVOID aArg)
{
	m_pMngPnt->OnTick();
	return 0;
}

void CKeepAliveTimer::Schedule(long msec)
{

	CTimeValue timeval(msec/1000, (msec%1000)*1000);
	if (bScheduled)
		Cancel();
	CReactor::GetInstance()->ScheduleTimer(this, NULL, timeval, 0);
	bScheduled = TRUE;
}

void CKeepAliveTimer::Cancel()
{
	if (!bScheduled)
		return;
	CReactor::GetInstance()->CancelTimer(this);
	bScheduled = FALSE;
}

/*########################################################################*/
/* CTransConManager*/
CTransConManager::CTransConManager()
{
	m_pDeadTransCon = NULL;
	m_dwRef = 1;
}

CTransConManager::~CTransConManager()
{
	if (m_pDeadTransCon != NULL)
	{
		delete m_pDeadTransCon;
	}
}

void CTransConManager::AddRef()
{
	m_dwRef ++;
}

void CTransConManager::Release()
{
	if (--m_dwRef == 0)
	{
		g_pTransConManager = NULL;
		delete this;
	}
}

ITransConAcceptor *CTransConManager::CreateTransConAcceptor(ITransConAcceptorSink *pSink, 
															unsigned long dwType /* = TYPE_TCP */)
{
	if (dwType == TYPE_TCP)
	{
		CTransConTcpAcceptor *pApt = new CTransConTcpAcceptor(pSink, dwType);
		if (pApt->Init() != 0)
		{
			delete pApt;
			return NULL;
		}
		return pApt;
	}
#ifdef TRANS_CON_SUPPORT_UDP 
	else if (dwType == TYPE_UDP)
	{
		CTransConUdpAcceptor *pApt = new CTransConUdpAcceptor(pSink, dwType);
		if (pApt->Init() != 0)
		{
			delete pApt;
			return NULL;
		}
		return pApt;
	}
#endif
	else
		return NULL;
}

ITransCon* CTransConManager::CreateTransCon(ITransConSink *pSink, 
											unsigned long dwType /* = TYPE_TCP */)
{
	if (dwType != TYPE_UDP)
	{
		if (dwType == TYPE_TCP || TYPE_AUTO == dwType
			|| TYPE_SSL == dwType)
		{
			CTcpTransCon *pCon;
			pCon = new CTcpTransCon(pSink, NULL, dwType);
			if (pCon->Init() != 0)
			{
				delete pCon;
				return NULL;
			}
			return pCon;
		}
		return NULL;
	}
#ifdef TRANS_CON_SUPPORT_UDP 
	else
	{
		CUdpConTransCon *pCon;
		pCon = new CUdpConTransCon(pSink);
		if (pCon->Init() != 0)
		{
			delete pCon;
			return NULL;
		}
		return pCon;
	}
#endif
	return NULL;
}

void CTransConManager::DestroyTransConAcceptor(ITransConAcceptor *pApt)
{
	if (pApt != NULL)
		delete pApt;
}

void CTransConManager::DestroyTransCon(ITransCon *pTransCon)
{
	pTransCon->Clean();	

	// budingc
	/// crash in OnReceive() especially when AudioSession
	/// when the upper layer destroys two cons in the same OnReceive() 
#if 0
	if (m_pDeadTransCon != NULL && m_pDeadTransCon != pTransCon)
	{
		delete m_pDeadTransCon;
	}
	m_pDeadTransCon = pTransCon;
#else
	CTransportDestoryEvent::PostEvent(pTransCon);
#endif
}

void CTransConManager::SetTransConSink(ITransCon *pTransCon, ITransConSink *pSink)
{
	pTransCon->SetSink(pSink);
}

void CTransConManager::DestroyManager(void)
{
	Release();
}

