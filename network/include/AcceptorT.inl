
#include "CmBase.h"
#include "Addr.h"
#include "SocketBase.h"

template <class TranTpye, class SockType>
CAcceptorT<TranTpye, SockType>::CAcceptorT(CReactor *aReactor, IAcceptorConnectorSink *aSink)
	: m_pReactor(aReactor)
	, m_pSink(aSink)
{
	CM_ASSERTE(m_pReactor);
	CM_ASSERTE(m_pSink);
}

template <class TranTpye, class SockType>
CAcceptorT<TranTpye, SockType>::~CAcceptorT()
{
}

template <class TranTpye, class SockType>
int CAcceptorT<TranTpye, SockType>::OnInput(CM_HANDLE aFd)
{
	CM_ASSERTE(aFd == GetHandle());

	TranTpye *pTrpt = NULL;
	int nRet = MakeTransport(pTrpt);
	if (nRet == -1)
		return 0;

	CInetAddr addrPeer;
	nRet = AcceptTransport(pTrpt, addrPeer);
	if (nRet == -1)
		goto fail;

	nRet = ActivateTransport(pTrpt);
	if (nRet == -1) {
		// don't call pTrpt->CloseAndDestory() because pTrpt 
		pTrpt = NULL;
		goto fail;
	}
	else {
		pTrpt = NULL;
	}

	return 0;
	
fail:
	if (pTrpt) {
		pTrpt->CloseAndDestory();
		pTrpt = NULL;
	}
	return 0;
}

template <class TranTpye, class SockType>
int CAcceptorT<TranTpye, SockType>::MakeTransport(TranTpye *&aTrpt)
{
	CM_ASSERTE(!aTrpt);
	aTrpt = new TranTpye(m_pReactor);
	return aTrpt ? 0 : -1;
}

template <class TranTpye, class SockType>
int CAcceptorT<TranTpye, SockType>::AcceptTransport(TranTpye *aTrpt, CInetAddr &aAddr)
{
	int nAddrLen = aAddr.GetSize();
	CM_HANDLE sockNew = (CM_HANDLE)::accept((CM_SOCKET)GetHandle(), 
					  reinterpret_cast<struct sockaddr *>(const_cast<struct sockaddr_in *>(aAddr.GetPtr())), 
#ifdef WIN32
					  &nAddrLen
#else // !WIN32
					  reinterpret_cast<socklen_t*>(&nAddrLen)
#endif // WIN32
	);

	if (sockNew == CM_INVALID_HANDLE) {
#ifdef WIN32
		errno = ::WSAGetLastError();
#endif //WIN32
		CM_ERROR_LOG(("CAcceptorT::AcceptTransport, accept() failed!"));
		return -1;
	}

	aTrpt->GetPeer().SetHandle(sockNew);
	if (aTrpt->GetPeer().Enable(CIPCBase::NON_BLOCK) == -1) {
		CM_ERROR_LOG(("CAcceptorT::AcceptTransport, Enable(NON_BLOCK) failed!"));
		return -1;
	}

#if 0
	CInetAddr addrRemote;
	aTrpt->GetPeer().GetRemoteAddr(addrRemote);
	CmTraceLog2File("CAcceptorT::AcceptTransport, fd=%d addr=%s port=%d", 
		sockNew, addrRemote.GetHostAddr().c_str(), (int)addrRemote.GetPort());
#endif // CM_DEBUG

	return 0;
}

template <class TranTpye, class SockType>
int CAcceptorT<TranTpye, SockType>::ActivateTransport(TranTpye *aTrpt)
{
	return m_pSink->OnConnectIndication(REASON_SUCCESSFUL, aTrpt);
}
