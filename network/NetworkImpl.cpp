
#include "CmBase.h"
#include "NetworkImpl.h"
#include "NetworkInterface.h"
#include "TimeValue.h"
#include "transconmanager.h"
#include "CommandRequest.h"
#include <stdio.h>

static INetConnection	*g_pLastCon;
static INetAcceptor	*g_pLastApt;
static INetTimer	*g_pLastTimer;
extern void NetworkHttpFini();

extern "C"
{
	int NetworkInit()
	{
		CreateTransConManager();
                return CReactor::GetInstance()->Open();
	}
	void NetworkFini()
	{
		NetworkHttpFini();
		if (g_pLastCon)
		{
			delete g_pLastCon;
			g_pLastCon = NULL;
		}
		if (g_pLastApt)
		{
			delete g_pLastApt;
			g_pLastApt = NULL;
		}
		if (g_pLastTimer != NULL)
		{
			delete g_pLastTimer;
			g_pLastTimer = NULL;
		}
	
		CReactor::GetInstance()->StopEventLoop();

	}
	void NetworkRunEventLoop()
	{
		CReactor::GetInstance()->RunEventLoop();
		CReactor::GetInstance()->Close();
		DestoryTransConManager();
                CReactor::DestroyInstance();
	}

	void NetworkDestroyConnection(INetConnection *pCon)
	{
		printf("DestroyConnection %x\n", pCon);
		assert ((UINT) pCon != 0x5000000);

		pCon->Disconnect();
		if (g_pLastCon != NULL)
                {
			delete g_pLastCon;
                }
		g_pLastCon = pCon;
	}

	void NetworkDestroyTimer(INetTimer *pTimer)
	{
		if (pTimer != NULL)
			pTimer->Cancel();
		if (g_pLastTimer != NULL)
			delete g_pLastTimer;
		g_pLastTimer = pTimer;
	}
	
	void NetworkDestroyAcceptor(INetAcceptor *pApt)
	{
		if (pApt != NULL)
		    pApt->StopListen();
		if (g_pLastApt != NULL)
			delete g_pLastApt;
		g_pLastApt = pApt;
	}

	INetTimer *CreateNetTimer(INetTimerSink *pSink)
	{
		if (pSink == NULL)
			return NULL;
		else 
			return new CNetTimer(pSink);
	}

        void ANetworkDestroyConnection(INetConnection* pCon)
        {
            CCRDisconnect::Allocator* pAlloc = CCRDisconnect::GetAllocator();
            CCRDisconnect *pCR = pAlloc->Allocate( pCon );
            if ( pCR )
                CReactor::GetInstance()->EnqueueCommandRequest( pCR );
        }	
}

/*
 *	CNetTimer
 */
CNetTimer::CNetTimer(INetTimerSink *pSink)
{
	m_pSink = pSink;
	m_bScheduled = FALSE;
}

CNetTimer::~CNetTimer()
{
	if (m_bScheduled)
		Cancel();
}

void CNetTimer::Schedule(long msec, void *pArg)
{
	CTimeValue timeval(msec/1000, (msec%1000)*1000);
	if (m_bScheduled)
		Cancel();
	CReactor::GetInstance()->ScheduleTimer(this, pArg, timeval, 0);
	m_bScheduled = TRUE;
	
}

 void CNetTimer::Cancel()
 {
	 if (!m_bScheduled)
		 return;
	 CReactor::GetInstance()->CancelTimer(this);
	 m_bScheduled = FALSE;
	 
 }

 int CNetTimer::OnTimeout(const CTimeValue &aCurTime, LPVOID aArg)
 {
	 m_pSink->OnTimer(aArg, this);
	 return 0;
 }

int INetConnection::ASendCommand(unsigned char* pData, int nLen)
{
    CCRSendCommand::Allocator* pAlloc = CCRSendCommand::GetAllocator();
    CCRSendCommand* pCR = pAlloc->Allocate( this, pData, nLen);
    if ( pCR )
    {
        CReactor::GetInstance()->EnqueueCommandRequest( pCR );
        return nLen;
    }

    return 0;
}
