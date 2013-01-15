
#include "CmBase.h"
#include "TimerQueueBase.h"
#include "Reactor.h"


CTimerQueueBase::CTimerQueueBase()
{
}

CTimerQueueBase::~CTimerQueueBase()
{
}

int CTimerQueueBase::CheckExpire()
{
	int nCout = 0;
	CTimeValue tvCur = CTimeValue::GetTimeOfDay();

	for ( ; ; ) {
		CEventHandlerBase *pEh = NULL;
		LPVOID pToken = NULL;
		{
			CTimeValue tvEarliest;
			int nRet = GetEarliestTime_l(tvEarliest);
			if (nRet == -1 || tvEarliest > tvCur)
				break;

			CNode ndFirst;
			nRet = PopFirstNode_l(ndFirst);
			CM_ASSERTE(nRet == 0);

			pEh = ndFirst.m_pEh;
			pToken = ndFirst.m_pToken;
			if (ndFirst.m_dwCount != (DWORD)-1)
				ndFirst.m_dwCount--;

			if (ndFirst.m_dwCount > 0 && ndFirst.m_tvInterval > CTimeValue::s_tvZero) {
				ndFirst.m_tvExpired = tvCur + ndFirst.m_tvInterval;
				RePushNode_l(ndFirst);
			}
		}

		CM_ASSERTE(pEh);
		pEh->OnTimeout(tvCur, pToken);
		nCout++;
	}

	return nCout;
}

int CTimerQueueBase::
ScheduleTimer(CEventHandlerBase *aEh, LPVOID aToken, const CTimeValue &aInterval, DWORD aCount)
{
	CM_ASSERTE_RETURN(aEh, -1);
	CM_ASSERTE_RETURN(aInterval > CTimeValue::s_tvZero || aCount == 1, -1);
	
	CTimeValue tvMin;
	int nGet = GetEarliestTime_l(tvMin);

//	int nErase = EraseNode_l(aEh);
//	CM_ASSERTE(nErase == 0 || nErase == 1);
	
	CNode ndNew(aEh, aToken);
	ndNew.m_tvInterval = aInterval;
	ndNew.m_tvExpired = CTimeValue::GetTimeOfDay() + aInterval;
	if (aCount > 0)
		ndNew.m_dwCount = aCount;
	else
		ndNew.m_dwCount = (DWORD)-1;
	
/*	if (RePushNode_l(ndNew) == 0) {
		return nErase ^ 1;
	}
	else
		return -1;
*/
	int nRet = PushNode_l(ndNew);

	if (nRet != -1 &&  !(nGet == 0 && ndNew.m_tvExpired >= tvMin)) 
	{
		CReactor::GetInstance()->NotifyHandler(NULL, CEventHandlerBase::NULL_MASK);
	}
	return nRet;
}

int CTimerQueueBase::CancelTimer(CEventHandlerBase *aEh)
{
	CM_ASSERTE_RETURN(aEh, -1);

	return EraseNode_l(aEh);
}

CTimeValue CTimerQueueBase::GetEarliestTime() 
{
	CTimeValue tvRet;

	int nRet = GetEarliestTime_l(tvRet);
	if (nRet == 0)
		return tvRet;
	else
		return CTimeValue::s_tvMax;
}

