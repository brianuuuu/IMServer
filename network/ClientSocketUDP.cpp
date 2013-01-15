
#include "CmBase.h"
#include "ClientSocketUDP.h"
#include "datablk.h"
#include "Addr.h"
#include "UtilityT.h"


//////////////////////////////////////////////////////////////////////
// class IClientSocketUDPSink
//////////////////////////////////////////////////////////////////////

IClientSocketUDPSink::~IClientSocketUDPSink()
{
}


//////////////////////////////////////////////////////////////////////
// class CClientSocketUDP
//////////////////////////////////////////////////////////////////////
#ifdef WIN32
CCmBufferWapperChar CClientSocketUDP::s_bwRecvMax(65536);
#else
CCmBufferWapperChar CClientSocketUDP::s_bwRecvMax(4096);
#endif

CClientSocketUDP::CClientSocketUDP(IClientSocketUDPSink *aSink)
	: m_pSink(aSink)
	, m_dwFlag(0)
{
	CM_ASSERTE(m_pSink);
}

CClientSocketUDP::~CClientSocketUDP()
{
}

int CClientSocketUDP::SetBuffer(int nSize)
{
	if (m_Socket.SetOption(SOL_SOCKET, SO_SNDBUF, &nSize, sizeof(nSize)) == -1) {
		CM_ERROR_LOG(("CClientSocketUDP::Listen, SetOption(SO_SNDBUF) failed!"));
		return -1;
	}
	if (m_Socket.SetOption(SOL_SOCKET, SO_RCVBUF, &nSize, sizeof(nSize)) == -1) {
		CM_ERROR_LOG(("CClientSocketUDP::Listen, SetOption(SO_SNDBUF) failed!"));
		return -1;
	}
	return 0;
}

int CClientSocketUDP::Listen(const CInetAddr &aAddr, DWORD aMaxRecvLen)
{
	CM_ASSERTE_RETURN(m_Socket.GetHandle() == CM_INVALID_HANDLE, -1);

	if (m_Socket.Open(aAddr) == -1) {
		CM_ASSERTE("CClientSocketUDP::Open, m_Socket.Open() failed!");
		return -1;
	}

	if (CReactor::GetInstance()->
			RegisterHandler(this, CEventHandlerBase::READ_MASK) == -1) 
	{
		Close();
		return -1;
	}

	if (aMaxRecvLen < 1024)
		aMaxRecvLen = 1024;
	else if (aMaxRecvLen > 65536)
		aMaxRecvLen = 65536;
	m_dwMaxRecvLen = aMaxRecvLen;

	m_dwFlag = LISTEN;
	return 0;
}

int CClientSocketUDP::Connect(const CInetAddr &aAddr, DWORD aMaxRecvLen)
{
	CM_ASSERTE_RETURN(m_Socket.GetHandle() == CM_INVALID_HANDLE, -1);

	CInetAddr addNull("", 0);
	if (Listen(addNull, aMaxRecvLen) == -1)
		return -1;
	
	int nRet = ::connect((CM_SOCKET)m_Socket.GetHandle(), 
						  reinterpret_cast<const struct sockaddr *>(aAddr.GetPtr()), 
						  aAddr.GetSize());
	if (nRet == -1) {
		CM_ERROR_LOG(("CClientSocketUDP::Connect, connect() failed! addr=%s port=%d err=%d", 
			aAddr.GetHostAddr().c_str(), (int)aAddr.GetPort(), errno));
		Close();
		return -1;
	}

	m_dwFlag = CONNECT;
	return 0;
}

int CClientSocketUDP::Send(CDataBlock &aData)
{
	CM_ASSERTE_RETURN(m_Socket.GetHandle() != CM_INVALID_HANDLE, -1);
	CM_ASSERTE(m_dwFlag == CONNECT);
	
	int nSend = m_Socket.Send((char*)aData.GetBuf(), aData.GetLen());
	if (nSend < (int)aData.GetLen()) {
		CM_ERROR_LOG(("CClientSocketUDP::Send, send() failed!"
			" nSend=%d len=%u err=%d", nSend, aData.GetLen(), errno));
		return 0;
	}
	return 0;
}

int CClientSocketUDP::SendTo(CDataBlock &aData, const CInetAddr &aAddr)
{
	CM_ASSERTE_RETURN(m_Socket.GetHandle() != CM_INVALID_HANDLE, -1);
	CM_ASSERTE(m_dwFlag == LISTEN);
	
	int nSend = m_Socket.SendTo((char*)aData.GetBuf(), aData.GetLen(), aAddr);
	if (nSend < (int)aData.GetLen()) {
		CM_ERROR_LOG(("CClientSocketUDP::Send, sendto() failed!"
			" nSend=%d len=%u err=%d", nSend, aData.GetLen(), errno));
		return 0;
	}
	return 0;
}

int CClientSocketUDP::Close()
{
	int nRet = 0;
	if (m_Socket.GetHandle() != CM_INVALID_HANDLE) {
		CReactor::GetInstance()->RemoveHandler(this);
		nRet = m_Socket.Close();
		m_dwFlag = 0;
	}
	return nRet;
}

CM_HANDLE CClientSocketUDP::GetHandle() const 
{
	return m_Socket.GetHandle();
}

int CClientSocketUDP::OnInput(CM_HANDLE aFd)
{
	CM_ASSERTE(aFd == m_Socket.GetHandle());

	CInetAddr addrRecv;
	int nRecv = m_Socket.RecvFrom(s_bwRecvMax.GetRawPtr(), s_bwRecvMax.GetSize(), addrRecv);
	if (nRecv < 0) {
		// recvfrom() will get ECONNRESET on WIN32 if the peer doesn't exist
		// even if connect() isn't invoked before
		if (errno == ECONNRESET && (m_dwFlag == LISTEN))
			return 0;

		if (errno == ECONNRESET && (m_dwFlag == CONNECT)) {
			// don't log it.
		}
		else {
			CM_ERROR_LOG(("CClientSocketUDP::OnInput, RecvFrom() failed!"
				" nRecv=%d len=%u flag=%u err=%d", nRecv, m_dwMaxRecvLen, m_dwFlag, errno));
		}
		return 0;
	}
	if (nRecv == 0)
		return 0;

	CDataBlock *pdbRecv = new CDataBlock(nRecv, 0);
	::memcpy(pdbRecv->GetBuf(), s_bwRecvMax.GetRawPtr(), nRecv);
	pdbRecv->Expand(nRecv);
	
	m_pSink->OnReceiveUdp(*pdbRecv, addrRecv);
	pdbRecv->Release();
	return 0;
}

int CClientSocketUDP::OnClose(CM_HANDLE aFd, MASK aMask)
{
	CM_ASSERTE(aFd == m_Socket.GetHandle());
	CM_ASSERTE(aMask == CEventHandlerBase::READ_MASK);

	int nErr = errno;
	Close();
	m_pSink->OnCloseUdp(nErr);
	return 0;
}
