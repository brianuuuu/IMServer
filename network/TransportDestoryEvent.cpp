
#include "CmBase.h"
#include "TransportDestoryEvent.h"
#include "TimeValue.h"
#include "transconapi.h"

CTransportDestoryEvent::CTransportDestoryEvent()
	: m_pTransCon(NULL)
{
}

CTransportDestoryEvent::~CTransportDestoryEvent()
{
}

int CTransportDestoryEvent::PostEvent(ITransCon *aTransCon)
{
	CM_ASSERTE_RETURN(aTransCon, -1);

	CTransportDestoryEvent *pEvent = new CTransportDestoryEvent();
	pEvent->m_pTransCon = aTransCon;
	return CReactor::GetInstance()->ScheduleTimer(pEvent, NULL, CTimeValue(), 1);
}


int CTransportDestoryEvent::OnTimeout(const CTimeValue &aCurTime, LPVOID aArg)
{
	if (m_pTransCon) {
		delete m_pTransCon;
		m_pTransCon = NULL;
	}

	delete this;
	return 0;
}
