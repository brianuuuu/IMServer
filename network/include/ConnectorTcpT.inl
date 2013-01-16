
#include "CmBase.h"
#include "Addr.h"

template <class UpperType, class UpTrptType, class UpSockType>
CConnectorTcpT<UpperType, UpTrptType, UpSockType>::CConnectorTcpT(CReactor *aReactor, UpperType &aUpper)
	: m_pReactor(aReactor)
	, m_Upper(aUpper)
	, m_pTransport(NULL)
{
}

template <class UpperType, class UpTrptType, class UpSockType>
CConnectorTcpT<UpperType, UpTrptType, UpSockType>::~CConnectorTcpT()
{
	Close();
}

template <class UpperType, class UpTrptType, class UpSockType>
int CConnectorTcpT<UpperType, UpTrptType, UpSockType>::
Connect(const CInetAddr &aAddr, CTimeValue *aTvOut, LPVOID )
{
	int nRet = 0;
	CM_ASSERTE_RETURN(!m_pTransport, -1);
	CM_ASSERTE(!aTvOut);
	
	m_pTransport = MakeTransport();
	if (!m_pTransport) {
		nRet = -1;
		goto fail;
	}

	nRet = DoConnect(m_pTransport, aAddr);
	if (nRet == 0) {
		CM_WARNING_LOG(("CConnectorTcpT::Connect, connect return 0."));
		nRet = m_pReactor->NotifyHandler(this, CEventHandlerBase::WRITE_MASK);
	}
	else if (nRet == 1)
		nRet = 0;
	
fail:
	/// class <CConnectorTcpT> doesn't handler ::connect failed and timer out
	/// the upper layer such as class <CConnectorSelect> should handler these issues.
#if 0
	if (nRet == -1){
		CM_WARNING_LOG(("CConnectorTcpT::Connect, connect return -1. err=%d", errno));
		nRet = m_pReactor->NotifyHandler(this, CEventHandlerBase::READ_MASK);
	}

	if (nRet == -1) {
		CM_ERROR_LOG(("CConnectorTcpT::Connect, NotifyHandler() failed!"));
		Close();
		return -1;
	}
	return 0;
#endif
	return nRet;
}

template <class UpperType, class UpTrptType, class UpSockType>
CM_HANDLE CConnectorTcpT<UpperType, UpTrptType, UpSockType>::GetHandle() const 
{
	CM_ASSERTE_RETURN(m_pTransport, CM_INVALID_HANDLE);
	return m_pTransport->GetHandle();
}

template <class UpperType, class UpTrptType, class UpSockType>
int CConnectorTcpT<UpperType, UpTrptType, UpSockType>::OnInput(CM_HANDLE aFd)
{
	CM_ASSERTE(m_pTransport);
	CM_ASSERTE(aFd == m_pTransport->GetHandle());

	return -1;
}

template <class UpperType, class UpTrptType, class UpSockType>
int CConnectorTcpT<UpperType, UpTrptType, UpSockType>::OnOutput(CM_HANDLE aFd)
{
	CM_ASSERTE(m_pTransport);
	CM_ASSERTE(aFd == m_pTransport->GetHandle());

#if 0
	CInetAddr addrRemote;
	m_pTransport->GetPeer().GetLocalAddr(addrRemote);
	CmTraceLog2File("CConnectorTcpT::OnOutput, fd=%d addr=%s port=%d", 
		(int)m_pTransport->GetHandle(), addrRemote.GetHostAddr().c_str(), (int)addrRemote.GetPort());
#endif // CM_DEBUG

	UpTrptType* pTrans = m_pTransport;
	m_pTransport = NULL;
	m_Upper.OnConnectIndication(REASON_SUCCESSFUL, pTrans, this);
	return 0;
}

template <class UpperType, class UpTrptType, class UpSockType>
int CConnectorTcpT<UpperType, UpTrptType, UpSockType>::OnClose(CM_HANDLE aFd, CEventHandlerBase::MASK aMask)
{
	CM_ASSERTE(m_pTransport);
	CM_ASSERTE(aFd == m_pTransport->GetHandle());
	CM_ASSERTE(aMask == CEventHandlerBase::CONNECT_MASK);
	
	Close();
	m_Upper.OnConnectIndication(REASON_SOCKET_ERROR, NULL, this);
	return 0;
}

template <class UpperType, class UpTrptType, class UpSockType>
int CConnectorTcpT<UpperType, UpTrptType, UpSockType>::Close()
{
	if (m_pTransport) {
		m_pReactor->RemoveHandler(this, CEventHandlerBase::CONNECT_MASK);
		m_pTransport->CloseAndDestory();
		m_pTransport = NULL;
	}
	return 0;
}

template <class UpperType, class UpTrptType, class UpSockType>
UpTrptType* CConnectorTcpT<UpperType, UpTrptType, UpSockType>::MakeTransport()
{
	return new UpTrptType(m_pReactor);
}

template <class UpperType, class UpTrptType, class UpSockType>
int CConnectorTcpT<UpperType, UpTrptType, UpSockType>::DoConnect(UpTrptType *aTrpt, const CInetAddr &aAddr)
{
	int nRet;
	UpSockType &sockPeer = aTrpt->GetPeer();
	CM_ASSERTE(sockPeer.GetHandle() == CM_INVALID_HANDLE);
	
	if (sockPeer.Open() == -1) {
		CM_ERROR_LOG(("CConnectorTcpT::DoConnect, Open() failed!"));
		return -1;
	}
	if (sockPeer.Enable(CIPCBase::NON_BLOCK) == -1) {
		CM_ERROR_LOG(("CConnectorTcpT::DoConnect, Enable(NON_BLOCK) failed!"));
		return -1;
	}

//	CM_INFO_LOG(("CConnectorTcpT::DoConnect, addr=%s port=%d fd=%d", 
//		aAddr.GetHostAddr(), (int)aAddr.GetPort(), sockPeer.GetHandle()));

	/// we regiester CONNECT_MASK prior to connect() to avoid lossing OnConnect()
	nRet = m_pReactor->RegisterHandler(this, CEventHandlerBase::CONNECT_MASK);
	if (nRet == -1)
		return -1;
	m_pReactor->ModifyHandleSignal(this,true);

	nRet = ::connect((CM_SOCKET)sockPeer.GetHandle(), 
					  reinterpret_cast<const struct sockaddr *>(aAddr.GetPtr()), 
					  aAddr.GetSize());
#ifdef WIN32
	if (nRet == SOCKET_ERROR) {
		errno = ::WSAGetLastError();
		nRet = -1;
	}
#else // ! WIN32
	if (nRet == -1 && errno == EINPROGRESS)
		errno = EWOULDBLOCK;
#endif // WIN32

	if (nRet == -1) {
		if (errno == EWOULDBLOCK)
			return 1;
		else {
//			CM_ERROR_LOG(("CConnectorTcpT::DoConnect, connect() failed! addr=%s port=%d err=%d", 
//				aAddr.GetHostAddr(), (int)aAddr.GetPort(), errno));
			return -1;
		}
	}
	else
		return 0;
}
