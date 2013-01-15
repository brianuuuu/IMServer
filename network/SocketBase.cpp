
#include "CmBase.h"
#include "SocketBase.h"
#include "Addr.h"


//////////////////////////////////////////////////////////////////////
// class CSocketBase
//////////////////////////////////////////////////////////////////////

CM_HANDLE CIPCBase::GetHandle() const 
{
	return m_Handle;
}

void CIPCBase::SetHandle(CM_HANDLE aNew)
{
	CM_ASSERTE(m_Handle == CM_INVALID_HANDLE || aNew == CM_INVALID_HANDLE);
	m_Handle = aNew;
}

int CIPCBase::Enable(int aValue) const 
{
	CM_ASSERTE(m_Handle != CM_INVALID_HANDLE);
	switch(aValue) {
	case NON_BLOCK: 
		{
#ifdef WIN32
		u_long nonblock = 1;
		int nRet = ::ioctlsocket((CM_SOCKET)m_Handle, FIONBIO, &nonblock);
		if (nRet == SOCKET_ERROR) {
			errno = ::WSAGetLastError();
			nRet = -1;
		}
		return nRet;

#else // !WIN32
		int nVal = ::fcntl(m_Handle, F_GETFL, 0);
		if (nVal == -1)
			return -1;
		nVal |= O_NONBLOCK;
		if (::fcntl(m_Handle, F_SETFL, nVal) == -1)
			return -1;
		return 0;
#endif // WIN32
		}

	default:
		CM_ERROR_LOG(("CIPCBase::Enable, aValue=%d.", aValue));
		return -1;
	}
}

int CIPCBase::Disable(int aValue) const 
{
	CM_ASSERTE(m_Handle != CM_INVALID_HANDLE);
	switch(aValue) {
	case NON_BLOCK:
		{
#ifdef WIN32
		u_long nonblock = 0;
		int nRet = ::ioctlsocket((CM_SOCKET)m_Handle, FIONBIO, &nonblock);
		if (nRet == SOCKET_ERROR) {
			errno = ::WSAGetLastError();
			nRet = -1;
		}
		return nRet;

#else // !WIN32
		int nVal = ::fcntl(m_Handle, F_GETFL, 0);
		if (nVal == -1)
			return -1;
		nVal &= ~O_NONBLOCK;
		if (::fcntl(m_Handle, F_SETFL, nVal) == -1)
			return -1;
		return 0;
#endif // WIN32
		}

	default:
		CM_ERROR_LOG(("CIPCBase::Disable, aValue=%d.", aValue));
		return -1;
	}
}

int CIPCBase::Control(int aCmd, void *aArg) const
{
	int nRet;
#ifdef WIN32
	nRet = ::ioctlsocket((CM_SOCKET)m_Handle, aCmd, static_cast<unsigned long *>(aArg));
	if (nRet == SOCKET_ERROR) {
		errno = ::WSAGetLastError();
		nRet = -1;
	}
#else // !WIN32
	nRet = ::ioctl(m_Handle, aCmd, aArg);
#endif // WIN32
	return nRet;
}


//////////////////////////////////////////////////////////////////////
// class CSocketBase
//////////////////////////////////////////////////////////////////////

CSocketBase::CSocketBase()
{
}

CSocketBase::~CSocketBase()
{
	Close();
}

int CSocketBase::Open(int aFamily, int aType, int aProtocol, BOOL aReuseAddr)
{
	int nRet = -1;
	Close();
	
	m_Handle = (CM_HANDLE)::socket(aFamily, aType, aProtocol);
	if (m_Handle != CM_INVALID_HANDLE) {
		nRet = 0;
		if (aFamily != PF_UNIX && aReuseAddr) {
			int nReuse = 1;
			nRet = SetOption(SOL_SOCKET, SO_REUSEADDR, &nReuse, sizeof(nReuse));
		}
	}

	if (nRet == -1)
		Close();
	return nRet;
}

int CSocketBase::SetOption(int aLevel, int aOption, const void *aOptval, int aOptlen) const 
{
	CM_ASSERTE(m_Handle != CM_INVALID_HANDLE);
	int nRet = ::setsockopt((CM_SOCKET)m_Handle, aLevel, aOption, 
#ifdef WIN32
		static_cast<const char*>(aOptval), 
#else // !WIN32
		aOptval,
#endif // WIN32
		aOptlen);

#ifdef WIN32
	if (nRet == SOCKET_ERROR) {
		errno = ::WSAGetLastError();
		nRet = -1;
	}
#endif // WIN32
	return nRet;
}

int CSocketBase::GetOption(int aLevel, int aOption, void *aOptval, int *aOptlen) const 
{
	CM_ASSERTE(m_Handle != CM_INVALID_HANDLE);
	int nRet = ::getsockopt((CM_SOCKET)m_Handle, aLevel, aOption, 
#ifdef WIN32
		static_cast<char*>(aOptval), 
		aOptlen
#else // !WIN32
		aOptval,
		reinterpret_cast<socklen_t*>(aOptlen)
#endif // WIN32
		);

#ifdef WIN32
	if (nRet == SOCKET_ERROR) {
		errno = ::WSAGetLastError();
		nRet = -1;
	}
#endif // WIN32
	return nRet;
}

int CSocketBase::GetRemoteAddr(CInetAddr &aAddr) const
{
	CM_ASSERTE(m_Handle != CM_INVALID_HANDLE);

	int nSize = (int)aAddr.GetSize();
	int nGet = ::getpeername((CM_SOCKET)m_Handle,
					reinterpret_cast<sockaddr *>(const_cast<sockaddr_in *>(aAddr.GetPtr())),
#ifdef WIN32
					&nSize
#else // !WIN32
					reinterpret_cast<socklen_t*>(&nSize)
#endif // WIN32
					);

#ifdef WIN32
	if (nGet == SOCKET_ERROR) {
		errno = ::WSAGetLastError();
		nGet = -1;
	}
#endif // WIN32

	return nGet;
}

int CSocketBase::GetLocalAddr(CInetAddr &aAddr) const
{
	CM_ASSERTE(m_Handle != CM_INVALID_HANDLE);

	int nSize = (int)aAddr.GetSize();
	int nGet = ::getsockname((CM_SOCKET)m_Handle,
					reinterpret_cast<sockaddr *>(const_cast<sockaddr_in *>(aAddr.GetPtr())),
#ifdef WIN32
					&nSize
#else // !WIN32
					reinterpret_cast<socklen_t*>(&nSize)
#endif // WIN32
					);

#ifdef WIN32
	if (nGet == SOCKET_ERROR) {
		errno = ::WSAGetLastError();
		nGet = -1;
	}
#endif // WIN32

	return nGet;
}

int CSocketBase::Close()
{
	int nRet = 0;
	if (m_Handle != CM_INVALID_HANDLE) {
#ifdef WIN32
		nRet = ::closesocket((CM_SOCKET)m_Handle);
		if (nRet == SOCKET_ERROR) {
			errno = ::WSAGetLastError();
			nRet = -1;
		}
#else
		nRet = ::close((CM_SOCKET)m_Handle);
#endif
		m_Handle = CM_INVALID_HANDLE;
	}
	return nRet;
}

int CSocketBase::Recv(char *aBuf, DWORD aLen, int aFlag) const
{
	CM_ASSERTE(m_Handle != CM_INVALID_HANDLE);
	CM_ASSERTE(aBuf);
	
	int nRet = ::recv((CM_SOCKET)m_Handle, aBuf, aLen, aFlag);
#ifndef WIN32
	if (nRet == -1 && errno == EAGAIN)
		errno = EWOULDBLOCK;
#else // !WIN32
	if (nRet == SOCKET_ERROR) {
		errno = ::WSAGetLastError();
		nRet = -1;
	}
#endif // WIN32

	return nRet;
}

int CSocketBase::RecvV(iovec aIov[], DWORD aCount) const 
{
	int nRet;
	CM_ASSERTE(m_Handle != CM_INVALID_HANDLE);
	CM_ASSERTE(aIov);
	
#ifdef WIN32
	DWORD dwBytesReceived = 0;
	DWORD dwFlags = 0;
	nRet = ::WSARecv((CM_SOCKET)m_Handle,
                      (WSABUF *)aIov,
                      aCount,
                      &dwBytesReceived,
                      &dwFlags,
                      0,
                      0);
	if (nRet == SOCKET_ERROR) {
		errno = ::WSAGetLastError();
		nRet = -1;
	}
	else {
		nRet = (int)dwBytesReceived;
	}
#else // !WIN32
	nRet = ::readv(m_Handle, aIov, aCount);
#endif // WIN32
	return nRet;
}

int CSocketBase::Send (const char *aBuf, DWORD aLen, int aFlag) const 
{
	CM_ASSERTE(m_Handle != CM_INVALID_HANDLE);
	CM_ASSERTE(aBuf);

	int nRet = ::send((CM_SOCKET)m_Handle, aBuf, aLen, aFlag);
#ifndef WIN32
	if (nRet == -1 && errno == EAGAIN)
		errno = EWOULDBLOCK;
#else // !WIN32
	if (nRet == SOCKET_ERROR) {
		errno = ::WSAGetLastError();
		nRet = -1;
	}
#endif // WIN32
	return nRet;
}

int CSocketBase::SendV(const iovec aIov[], DWORD aCount) const 
{
	int nRet;
	CM_ASSERTE(m_Handle != CM_INVALID_HANDLE);
	CM_ASSERTE(aIov);
	
#ifdef WIN32
	DWORD dwBytesSend = 0;
       //TODO:Field WSASend?
	nRet = ::WSARecv((CM_SOCKET)m_Handle,
                      (WSABUF *)aIov,
                      aCount,
                      &dwBytesSend,
                      0,
                      0,
                      0);
	if (nRet == SOCKET_ERROR) {
		errno = ::WSAGetLastError();
		nRet = -1;
	}
	else {
		nRet = (int)dwBytesSend;
	}
#else // !WIN32
	nRet = ::writev(m_Handle, aIov, aCount);
#endif // WIN32
	return nRet;
}


//////////////////////////////////////////////////////////////////////
// class CSocketTcp
//////////////////////////////////////////////////////////////////////

CSocketTcp::CSocketTcp()
{
}

CSocketTcp::~CSocketTcp()
{
	Close();
}

int CSocketTcp::Open(BOOL aReuseAddr)
{
	return CSocketBase::Open(PF_INET, SOCK_STREAM, 0, aReuseAddr);
}

int CSocketTcp::Close(int aReason)
{
#ifdef WIN32
	// We need the following call to make things work correctly on
	// Win32, which requires use to do a <CloseWriter> before doing the
	// close in order to avoid losing data.  Note that we don't need to
	// do this on UNIX since it doesn't have this "feature".  Moreover,
	// this will cause subtle problems on UNIX due to the way that
	// fork() works.
	if (m_Handle != CM_INVALID_HANDLE && aReason == REASON_SUCCESSFUL)
		CloseWriter();
#endif // WIN32

	return CSocketBase::Close();
}

int CSocketTcp::CloseWriter()
{
	CM_ASSERTE(m_Handle != CM_INVALID_HANDLE);
	int nRet = ::shutdown((CM_SOCKET)m_Handle, CM_SD_SEND);

#ifdef WIN32
	if (nRet == SOCKET_ERROR) {
		errno = ::WSAGetLastError();
		nRet = -1;
	}
#endif // WIN32
	return nRet;
}

int CSocketTcp::CloseReader()
{
	CM_ASSERTE(m_Handle != CM_INVALID_HANDLE);
	int nRet = ::shutdown((CM_SOCKET)m_Handle, CM_SD_RECEIVE);

#ifdef WIN32
	if (nRet == SOCKET_ERROR) {
		errno = ::WSAGetLastError();
		nRet = -1;
	}
#endif // WIN32
	return nRet;
}


//////////////////////////////////////////////////////////////////////
// class CSocketUdp
//////////////////////////////////////////////////////////////////////

CSocketUdp::CSocketUdp()
{
}

CSocketUdp::~CSocketUdp()
{
	Close();
}

int CSocketUdp::Open(const CInetAddr &aLocal)
{
	if (CSocketBase::Open(PF_INET, SOCK_DGRAM, 0, FALSE) == -1)
		return -1;

	if (::bind((CM_SOCKET)m_Handle, 
						  reinterpret_cast<const sockaddr *>(aLocal.GetPtr()),
#ifdef WIN32
						  aLocal.GetSize()
#else // !WIN32
						  static_cast<socklen_t>(aLocal.GetSize())
#endif // WIN32
						  ) == -1)
	{
#ifdef WIN32
		errno = ::WSAGetLastError();
#endif // WIN32
		Close();
		return -1;
	}
	return 0;
}

int CSocketUdp::
RecvFrom(char *aBuf, DWORD aLen, CInetAddr &aAddr, int aFlag) const 
{
	CM_ASSERTE(m_Handle != CM_INVALID_HANDLE);

	int nSize = (int)aAddr.GetSize();
	int nRet = ::recvfrom((CM_SOCKET)m_Handle,
						  aBuf,
						  aLen,
						  aFlag,
						  reinterpret_cast<sockaddr *>(const_cast<sockaddr_in *>(aAddr.GetPtr())),
#ifdef WIN32
						  &nSize
#else // !WIN32
						  reinterpret_cast<socklen_t*>(&nSize)
#endif // WIN32
						   );

#ifdef WIN32
	if (nRet == SOCKET_ERROR) {
		errno = ::WSAGetLastError();
		nRet = -1;
	}
#endif // WIN32

	return nRet;
}

int CSocketUdp::
SendTo(const char *aBuf, DWORD aLen, const CInetAddr &aAddr, int aFlag) const 
{
	CM_ASSERTE(m_Handle != CM_INVALID_HANDLE);

	int nRet = ::sendto((CM_SOCKET)m_Handle,
						  aBuf,
						  aLen,
						  aFlag,
						  reinterpret_cast<const sockaddr *>(aAddr.GetPtr()),
#ifdef WIN32
						  aAddr.GetSize()
#else // !WIN32
						  static_cast<socklen_t>(aAddr.GetSize())
#endif // WIN32
						  );

#ifdef WIN32
	if (nRet == SOCKET_ERROR) {
		errno = ::WSAGetLastError();
		nRet = -1;
	}
#endif // WIN32
	
	return nRet;
}
