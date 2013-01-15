
#ifndef ACCEPTORT_H
#define ACCEPTORT_H

#include "TransportInterface.h"
#include "Reactor.h"

class CInetAddr;

template <class TranTpye, class SockType>
class CAcceptorT : public CEventHandlerBase, public IAcceptor 
{
public:
	CAcceptorT(CReactor *aReactor, IAcceptorConnectorSink *aSink);
	virtual ~CAcceptorT();

	virtual int OnInput(CM_HANDLE aFd = CM_INVALID_HANDLE);

protected:
	virtual int MakeTransport(TranTpye *&aTrpt);
	virtual int AcceptTransport(TranTpye *aTrpt, CInetAddr &aAddr);
	virtual int ActivateTransport(TranTpye *aTrpt);

	CReactor *m_pReactor;
	IAcceptorConnectorSink *m_pSink;
	
};

#include "AcceptorT.inl"

#endif // !ACCEPTORT_H

