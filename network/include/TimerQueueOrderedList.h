/*------------------------------------------------------*/
/* Timer queue implemented by ordered list              */
/*                                                      */
/* TimerQueueOrderedList.h                              */
/*                                                      */
/*                                                      */
/* History                                              */
/*                                                      */
/*	11/24/2003	Create                                  */
/*                                                      */
/*------------------------------------------------------*/

#ifndef TIMERQUEUEORDEREDLIST_H
#define TIMERQUEUEORDEREDLIST_H

#include "TimerQueueBase.h"
#include <list>

class CTimerQueueOrderedList : public CTimerQueueBase  
{
public:
	CTimerQueueOrderedList();
	virtual ~CTimerQueueOrderedList();

protected:
	virtual int PushNode_l(const CNode &aPushNode);
	virtual int EraseNode_l(CEventHandlerBase *aEh);
	virtual int RePushNode_l(const CNode &aPushNode);
	virtual int PopFirstNode_l(CNode &aPopNode);
	virtual int GetEarliestTime_l(CTimeValue &aEarliest) const;

private:
	int EnsureSorted();

	typedef std::list<CNode> NodesType;
	NodesType m_Nodes;
};

#endif // !TIMERQUEUEORDEREDLIST_H

