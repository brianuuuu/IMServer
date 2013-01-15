
#include "CmBase.h"
#include "TimerQueueOrderedList.h"


CTimerQueueOrderedList::CTimerQueueOrderedList()
{
}

CTimerQueueOrderedList::~CTimerQueueOrderedList()
{
}

int CTimerQueueOrderedList::EraseNode_l(CEventHandlerBase *aEh)
{
	NodesType::iterator iter = m_Nodes.begin();
	while (iter != m_Nodes.end()) {
		if ((*iter).m_pEh == aEh) {
			m_Nodes.erase(iter);
			return 0;
		}
		else
			++iter;
	}
	return 1;
}

int CTimerQueueOrderedList::PopFirstNode_l(CNode &aPopNode)
{
	CM_ASSERTE_RETURN(!m_Nodes.empty(), -1);

	aPopNode = m_Nodes.front();
	m_Nodes.pop_front();
	return 0;
}

int CTimerQueueOrderedList::RePushNode_l(const CNode &aPushNode)
{
	NodesType::iterator iter = m_Nodes.begin();
	for ( ; iter != m_Nodes.end(); ++iter ) {
		if ((*iter).m_tvExpired >= aPushNode.m_tvExpired) {
			m_Nodes.insert(iter, aPushNode);
			break;
		}
	}
	if (iter == m_Nodes.end())
		m_Nodes.insert(iter, aPushNode);

//	EnsureSorted();
	return 0;
}

int CTimerQueueOrderedList::GetEarliestTime_l(CTimeValue &aEarliest) const
{
	if (!m_Nodes.empty()) {
		aEarliest = m_Nodes.front().m_tvExpired;
		return 0;
	}
	else
		return -1;
}

int CTimerQueueOrderedList::EnsureSorted()
{
#ifdef CM_DEBUG
	if (m_Nodes.size() <= 1)
		return 0;
	CTimeValue tvMin = (*m_Nodes.begin()).m_tvExpired;
	NodesType::iterator iter1 = m_Nodes.begin();
	for ( ++iter1; iter1 != m_Nodes.end(); ++iter1 ) {
		CM_ASSERTE_RETURN((*iter1).m_tvExpired >= tvMin, -1);
		tvMin = (*iter1).m_tvExpired;
	}
#endif // CM_DEBUG
	return 0;
}

int CTimerQueueOrderedList::PushNode_l(const CNode &aPushNode)
{
	BOOL bFoundEqual = FALSE;
	BOOL bInserted = FALSE;
	NodesType::iterator iter = m_Nodes.begin();
	while (iter != m_Nodes.end()) {
		if ((*iter).m_pEh == aPushNode.m_pEh) {
			CM_ASSERTE(!bFoundEqual);
			iter = m_Nodes.erase(iter);
			bFoundEqual = TRUE;
			if (bInserted || iter == m_Nodes.end())
				break;
		}

		if (!bInserted && (*iter).m_tvExpired >= aPushNode.m_tvExpired) {
			iter = m_Nodes.insert(iter, aPushNode);
			++iter;
			bInserted = TRUE;
			if (bFoundEqual)
				break;
		}
		++iter;
	}

	if (iter != m_Nodes.end())
		CM_ASSERTE(bInserted && bFoundEqual);
	if (!bInserted)
		m_Nodes.push_back(aPushNode);

	EnsureSorted();
	return bFoundEqual ? 1 : 0;
}
