
#include "CmBase.h"
#include "TransportBase.h"
#include "SocketBase.h"
#include "TimeValue.h"

CTransportBase::CTransportBase(CReactor *pReactor)
	: m_pSink(NULL)
	, m_pReactor(pReactor)
{
	CM_ASSERTE(m_pReactor);
}

CTransportBase::~CTransportBase()
{
}

int CTransportBase::Open(ITransportSink *aSink)
{
	CM_ASSERTE_RETURN(!m_pSink, -1);
	CM_ASSERTE_RETURN(aSink, -1);
	m_pSink = aSink;

	int nRet = Open_t();
	if (nRet == -1) {
		Close_t(REASON_SUCCESSFUL);
		m_pSink = NULL;
	}
	return nRet;
}

void CTransportBase::Destroy(int aReason)
{
//	Close_t(aReason);
	m_pSink = NULL;
	m_pReactor->ScheduleTimer(this, NULL, CTimeValue(), 1);
}

void CTransportBase::CloseAndDestory(int aReason)
{
	// it is invoked by Connector, we needn't ScheduleTimer to delete this.
	Close_t(aReason);
	m_pSink = NULL;
	delete this;
}

int CTransportBase::OnClose(CM_HANDLE aFd, MASK aMask)
{
	Close_t(REASON_SUCCESSFUL);
	ITransportSink* pTmp = m_pSink;
	m_pSink = NULL;
	if (pTmp)
		pTmp->OnDisconnect(REASON_SOCKET_ERROR);
	else
		delete this;
	return 0;
}

int CTransportBase::OnTimeout(const CTimeValue &aCurTime, LPVOID aArg)
{
	Close_t(REASON_SUCCESSFUL);
	delete this;
	return 0;
}
