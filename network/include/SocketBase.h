
#ifndef SOCKETBASE_H
#define SOCKETBASE_H

class CInetAddr;

class  CIPCBase
{
public:
	enum { NON_BLOCK };

	CIPCBase() : m_Handle(CM_INVALID_HANDLE) { }

	CM_HANDLE GetHandle() const;
	void SetHandle(CM_HANDLE aNew);

	int Enable(int aValue) const ;
	int Disable(int aValue) const ;
	int Control(int aCmd, void *aArg) const;
	
protected:
	CM_HANDLE m_Handle;
};


class  CSocketBase : public CIPCBase
{
protected:
	CSocketBase();
	~CSocketBase();

public:
	/// Wrapper around the BSD-style <socket> system call (no QoS).
	int Open(int aFamily, int aType, int aProtocol, BOOL aReuseAddr);

	/// Close down the socket handle.
	int Close();

	/// Wrapper around the <setsockopt> system call.
	int SetOption(int aLevel, int aOption, const void *aOptval, int aOptlen) const ;

	/// Wrapper around the <getsockopt> system call.
	int GetOption(int aLevel, int aOption, void *aOptval, int *aOptlen) const ;

	/// Return the address of the remotely connected peer (if there is
	/// one), in the referenced <aAddr>.
	int GetRemoteAddr(CInetAddr &aAddr) const;

	/// Return the local endpoint address in the referenced <aAddr>.
	int GetLocalAddr(CInetAddr &aAddr) const;

	/// Recv an <aLen> byte buffer from the connected socket.
	int Recv(char *aBuf, DWORD aLen, int aFlag = 0) const ;

	/// Recv an <aIov> of size <aCount> from the connected socket.
	int RecvV(iovec aIov[], DWORD aCount) const ;

	/// Send an <aLen> byte buffer to the connected socket.
	int Send(const char *aBuf, DWORD aLen, int aFlag = 0) const ;

	/// Send an <aIov> of size <aCount> from the connected socket.
	int SendV(const iovec aIov[], DWORD aCount) const ;
};


class  CSocketTcp : public CSocketBase
{
public:
	CSocketTcp();
	~CSocketTcp();

	int Open(BOOL aReuseAddr = FALSE);
	int Close(int aReason = REASON_SUCCESSFUL);
	int CloseWriter();
	int CloseReader();
};

class  CSocketUdp : public CSocketBase
{
public:
	CSocketUdp();
	~CSocketUdp();

	int Open(const CInetAddr &aLocal);

	int RecvFrom(char *aBuf, 
				 DWORD aLen, 
				 CInetAddr &aAddr, 
				 int aFlag = 0) const ;

	int SendTo(const char *aBuf, 
			   DWORD aLen, 
			   const CInetAddr &aAddr, 
			   int aFlag = 0) const ;
};

#endif // !SOCKETBASE_H

