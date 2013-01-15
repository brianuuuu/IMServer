/*------------------------------------------------------*/
/* Timer queue base class                               */
/*                                                      */
/* TimerQueueBase.h                                     */
/*                                                      */
/* All rights reserved                                  */
/*                                                      */
/*                                                      */
/* History                                              */
/*                                                      */
/*	11/24/2003	Create                                  */
/*                                                      */
/*------------------------------------------------------*/

#ifndef TIMERQUEUEBASE_H
#define TIMERQUEUEBASE_H

#include "TimeValue.h"

class CEventHandlerBase;

class CTimerQueueBase
{
	/// for being compliant with T120_Timer_Manager.
	friend class T120_Timer_Manager;
	
public:
	struct CNode
	{
		CNode(CEventHandlerBase *aEh = NULL, LPVOID aToken = NULL) 
			: m_pEh(aEh), m_pToken(aToken), m_dwCount(0) 
		{ }

		CEventHandlerBase *m_pEh;
		LPVOID m_pToken;
		CTimeValue m_tvExpired;
		CTimeValue m_tvInterval;
		DWORD m_dwCount;
	};
	
	CTimerQueueBase();
	virtual ~CTimerQueueBase();

	/// if <aInterval> is zero timevalue, the <aCount> must be 1.
	/// If success:
	///    if <aEh> exists in queue, return 1;
	///    else return 0;
	/// else
	///    return -1;
	int ScheduleTimer(CEventHandlerBase *aEh, 
					  LPVOID aToken, 
					  const CTimeValue &aInterval,
					  DWORD aCount);

	/// If success:
	///    if <aEh> exists in queue, return 0;
	///    else return 1;
	/// else
	///    return -1;
	int CancelTimer(CEventHandlerBase *aEh);

	/// if the queue is empty, return CTimeValue::s_tvMax.
	CTimeValue GetEarliestTime();

	int CheckExpire();

protected:
	/// the sub-classes of CTimerQueueBase always use STL contains that 
	/// we just let them manage the memery allocation of CNode
	
	/// the following motheds are all called after locked
	virtual int PushNode_l(const CNode &aPushNode) = 0;
	virtual int EraseNode_l(CEventHandlerBase *aEh) = 0;
	virtual int RePushNode_l(const CNode &aPushNode) = 0;
	virtual int PopFirstNode_l(CNode &aPopNode) = 0;
	virtual int GetEarliestTime_l(CTimeValue &aEarliest) const = 0;

};


#endif // !TIMERQUEUEBASE_H

