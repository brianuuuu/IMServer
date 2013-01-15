/*------------------------------------------------------*/
/* Delete TP layer's points later                       */
/*                                                      */
/* TransportDestoryEvent.h                              */
/*                                                      */
/*                                                      */
/* History                                              */
/*                                                      */
/*	01/12/2004	Create                                  */
/*                                                      */
/*------------------------------------------------------*/

#ifndef TRANSPORTDESTORYEVENT_H
#define TRANSPORTDESTORYEVENT_H

#include "Reactor.h"

class ITransCon;

class CTransportDestoryEvent : public CEventHandlerBase 
{
public:
	static int PostEvent(ITransCon *aTransCon);

protected:
	virtual int OnTimeout(const CTimeValue &aCurTime, LPVOID aArg);

private:
	CTransportDestoryEvent();
	~CTransportDestoryEvent();

	ITransCon *m_pTransCon;
};

#endif // !TRANSPORTDESTORYEVENT_H

