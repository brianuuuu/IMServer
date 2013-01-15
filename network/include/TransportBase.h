
#ifndef TRANSPORTBASE_H
#define TRANSPORTBASE_H

#include "Reactor.h"
#include "TransportInterface.h"

class CSocketTcp;
class CProtocolBase;
class CMessageBlock;

class  CTransportBase : public CEventHandlerBase, public ITransport
{
public:
	CTransportBase(CReactor *pReactor);
	virtual ~CTransportBase();

	virtual int Open(ITransportSink *aSink);
	virtual void Destroy(int aReason = 0);
	
	void CloseAndDestory(int aReason = REASON_SUCCESSFUL);

	virtual int OnClose(CM_HANDLE aFd, MASK aMask);
	virtual int OnTimeout(const CTimeValue &aCurTime, LPVOID aArg);

protected:
	// template method for open() and close()
	virtual int Open_t() = 0;
	virtual int Close_t(int aReason) = 0;

	ITransportSink *m_pSink;
	CReactor *m_pReactor;
};


#endif // !TRANSPORTBASE_H

