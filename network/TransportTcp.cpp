#include "CmBase.h"
#include "TimeValue.h"
#include "TransportTcp.h"
#include "Addr.h"
#include "UtilityT.h"
#include "datablk.h"
#include "TraceLog.h"

#ifdef UNIX
#include <netinet/tcp.h>
#endif // UNIX

#ifdef WIN32
CCmBufferWapperChar CTransportTcp::s_bwRecvMax(65536);
#else 
CCmBufferWapperChar CTransportTcp::s_bwRecvMax(1024*4);
#endif

CTransportTcp::CTransportTcp(CReactor *pReactor)
	: CTransportBase(pReactor)
	, m_bCanRecv(true)
{
}

CTransportTcp::~CTransportTcp()
{
	Close_t(REASON_SUCCESSFUL);
}

int CTransportTcp::Open_t()
{
	int nSize = 65535;
	if (m_SockTcp.SetOption(SOL_SOCKET, SO_SNDBUF, &nSize, sizeof(nSize)) == -1) {
		CM_ERROR_LOG(("CTransportTcp::Open_t, SetOption(SO_SNDBUF) failed!"));
		return -1;
	}
	if (m_SockTcp.SetOption(SOL_SOCKET, SO_RCVBUF, &nSize, sizeof(nSize)) == -1) {
		CM_ERROR_LOG(("CTransportTcp::Open_t, SetOption(SO_SNDBUF) failed!"));
		return -1;
	}

	int nNoDelay = 1;
	if (m_SockTcp.SetOption(IPPROTO_TCP, TCP_NODELAY, &nNoDelay, sizeof(nNoDelay)) == -1) {
		CM_ERROR_LOG(("CTransportTcp::Open_t, SetOption(TCP_NODELAY) failed!"));
		return -1;
	}

	// it works with Win32AsyncSelect and RealTimeSignal 
	// if we regiests READ_MASK & WRITE_MASK together at first
	if (m_pReactor->RegisterHandler(this, CEventHandlerBase::READ_MASK | CEventHandlerBase::WRITE_MASK) == -1) {
		CM_ERROR_LOG(("CTransportTcp::Open_t, RegisterHandler(READ_MASK|WRITE_MASK) failed!"));
		return -1;
	}
#ifdef EMBEDED_LINUX
	m_pReactor->ScheduleTimer(this, (LPVOID)1, CTimeValue(), 1);
#endif
	return 0;
}

#ifdef EMBEDED_LINUX
int CTransportTcp::OnTimeout(const CTimeValue &aCurTime, LPVOID aArg)
{
	int arg = (int)aArg;
	if (arg == 1)
	{
		OnInput();
	}
	else
	{
		CTransportBase::OnTimeout(aCurTime, aArg);
	}
}
#endif
CM_HANDLE CTransportTcp::GetHandle() const
{
	return m_SockTcp.GetHandle();
}

int CTransportTcp::OnInput(CM_HANDLE )
{
	if (!m_bCanRecv)
	{
		return 0;
	}
	
	if (m_pSink == NULL)
		return 0;
	CM_ASSERTE(m_pSink);

	int nRecv = Recv_t(s_bwRecvMax.GetRawPtr(), s_bwRecvMax.GetSize());
	if (nRecv <= 0)
		return nRecv;

	CDataBlock *pdbIn = new CDataBlock((DWORD)nRecv, 0);
	::memcpy(pdbIn->GetBuf(), s_bwRecvMax.GetRawPtr(), nRecv);
	pdbIn->Expand(nRecv);

	m_pSink->OnReceive(*pdbIn);
	pdbIn->Release();
	return 0;
}

int CTransportTcp::OnOutput(CM_HANDLE )
{
	if (m_pSink == NULL)
	{
		VP_TRACE_ERROR("CTransportTcp::OnOutput m_pSink==NULL");
		return 0;
	}
		
	CM_ASSERTE(m_pSink);
//	CM_INFO_LOG(("CTransportTcp::OnOutput"));
	if (m_mbBufferOne.GetLength() == 0)
	{
		m_pReactor->ModifyHandleSignal(this,false);
		return 0;
	}
		
	int nRet = Send_t(m_mbBufferOne.GetReadPtr(), m_mbBufferOne.GetLength());
	if (nRet <= 0)
	{
		VP_TRACE_ERROR("CTransportTcp::OnOutput Send_t nRet=%d GetLength=%d",nRet,m_mbBufferOne.GetLength());
		return nRet;
	}

	if ((DWORD)nRet < m_mbBufferOne.GetLength()) {
		m_pReactor->ModifyHandleSignal(this, true);
		m_mbBufferOne.AdvanceReadPtr((DWORD)nRet);
	}
	else {
		m_pReactor->ModifyHandleSignal(this,false);
		m_mbBufferOne.Resize(0);
		m_pSink->OnSend();
	}
	return 0;
}

CSocketTcp& CTransportTcp::GetPeer()
{
	return m_SockTcp;
}

int CTransportTcp::Close_t(int aReason)
{
	if (m_SockTcp.GetHandle() != CM_INVALID_HANDLE) {
#if 0
		CInetAddr addrRemote;
		m_SockTcp.GetRemoteAddr(addrRemote);
		CmTraceLog2File("CTransportTcp::Close_t, fd=%d addr=%s port=%d", 
			m_SockTcp.GetHandle(), addrRemote.GetHostAddr().c_str(), (int)addrRemote.GetPort());
#endif // CM_DEBUG

		m_pReactor->RemoveHandler(this);
		return m_SockTcp.Close(aReason);
	}
	else
		return 0;
}

int CTransportTcp::SendData(CDataBlock &aData)
{
	if (m_mbBufferOne.GetLength() > 0)
	{
		m_pReactor->ModifyHandleSignal(this,true);
		return -1;
	}
		
	//int nRet = CTransportTcp::Send_t(reinterpret_cast<LPSTR>(aData.GetBuf()), aData.GetLen());

	//if (nRet < 0) 
	//{
	//	VP_TRACE_ERROR("CTransportTcp::SendData nRet=%d errno=%d",nRet,errno);
	//	return nRet;
	//}
		
	//if (s_nIndex%100==0)
	//{
		//VP_TRACE_INFO("CTransportTcp::SendData GetLen=%d nRet=%d",aData.GetLen(),nRet);
	//}

	//if (static_cast<DWORD>(nRet) < aData.GetLen()) {
		//VP_TRACE_ERROR("CTransportTcp::SendData, send=%d ret=%d err=%d",aData.GetLen(), nRet, errno);
		//m_pReactor->ModifyHandleSignal(this, true);
		m_mbBufferOne.ResizeFromDataBlock(aData);
		m_mbBufferOne.AdvanceReadPtr(0);
	//}
	return 0;
}

int CTransportTcp::IOCtl(int aCmd, LPVOID aArg)
{
	switch (aCmd) {
	case WUO_TRANSPORT_OPT_GET_FIO_NREAD:
		return m_SockTcp.Control(FIONREAD, aArg);

	case WUO_TRANSPORT_OPT_GET_TRAN_NREAD:
		*(static_cast<DWORD*>(aArg)) = m_mbBufferOne.GetLength();
		return 0;

	case WUO_TRANSPORT_OPT_GET_FD: 
		*(static_cast<CM_HANDLE *>(aArg)) = m_SockTcp.GetHandle();
		return 0;

	case WUO_TRANSPORT_OPT_GET_LOCAL_ADDR:
		m_SockTcp.GetLocalAddr(*(static_cast<CInetAddr*>(aArg)));
		return 0;

	case WUO_TRANSPORT_OPT_GET_PEER_ADDR:
		{
			static CInetAddr Addr;
			m_SockTcp.GetRemoteAddr(Addr);
			*((struct sockaddr_in **)aArg) = (struct sockaddr_in *)Addr.GetPtr();
			return 0;
		}

	case WUO_TRANSPORT_OPT_GET_SOCK_ALIVE: {
		char cTmp;
		int nRet = m_SockTcp.Recv(&cTmp, sizeof(cTmp), MSG_PEEK);
		if (nRet > 0 || (nRet < 0 && errno == EWOULDBLOCK))
			*static_cast<BOOL*>(aArg) = TRUE;
		else
			*static_cast<BOOL*>(aArg) = FALSE;
		return 0;
	}

	case WUO_TRANSPORT_OPT_GET_TRAN_TYPE:
		*(static_cast<DWORD*>(aArg)) = TYPE_TCP;
		return 0;

	case WUO_TRANSPORT_OPT_SET_SND_BUF_LEN:
		if (m_SockTcp.SetOption(SOL_SOCKET, SO_SNDBUF, aArg, sizeof(DWORD)) == -1) {
			CM_ERROR_LOG(("CTransportTcp::IOCtl, SetOption(SO_SNDBUF) failed!"));
			return -1;
		}
		return 0;

	case WUO_TRANSPORT_OPT_SET_RCV_BUF_LEN:
		if (m_SockTcp.SetOption(SOL_SOCKET, SO_RCVBUF, aArg, sizeof(DWORD)) == -1) {
			CM_ERROR_LOG(("CTransportTcp::IOCtl, SetOption(SO_RCVBUF) failed!"));
			return -1;
		}
		return 0;

	case WUO_TRANSPORT_OPT_SET_CAN_RCV_DATA:
		{
			bool bCanRecv = static_cast<bool>(*static_cast<DWORD*>(aArg));
			m_bCanRecv = bCanRecv;
			if (m_bCanRecv)
			{
				OnInput(NULL);
			}
			
		}
		return 0;

#ifdef CM_DEBUG_SEND_RECV
	case 13333:
	{
		char *szBuf = static_cast<char*>(aArg);
		RecType::iterator iter = m_RecordDebug.begin();
		for ( ; iter != m_RecordDebug.end(); ++iter) {
			int nPrint = sprintf(szBuf, "%c%d, ", (*iter).first, (*iter).second); 
			szBuf += nPrint;
		}
		return 0;
	}
#endif // CM_DEBUG_SEND_RECV

	// mainly for TeleSession usage. support SO_KEEPALIVE function
	case WUO_TRANSPORT_OPT_SET_TCP_KEEPALIVE: {
		DWORD dwTime = *static_cast<DWORD*>(aArg);
		int nKeep = dwTime > 0 ? 1 : 0;
		if (m_SockTcp.SetOption(SOL_SOCKET, SO_KEEPALIVE, &nKeep, sizeof(nKeep)) == -1) {
			CM_ERROR_LOG(("CTransportTcp::IOCtl, SetOption(SO_KEEPALIVE) failed! dwTime=%u", dwTime));
			return -1;
		}
#ifdef LINUX
		if (dwTime > 0) {
			if (m_SockTcp.SetOption(SOL_TCP, TCP_KEEPIDLE, &dwTime, sizeof(dwTime)) == -1) {
				CM_ERROR_LOG(("CTransportTcp::IOCtl, SetOption(TCP_KEEPINTVL) failed! dwTime=%u", dwTime));
				return -1;
			}
		}
#endif // LINUX
		return 0;
	}

	default:
		VP_TRACE_ERROR("CTransportTcp::IOCtl, unknow aCmd=%d aArg=%x", aCmd, aArg);
		return -1;
	}
}

int CTransportTcp::Recv_t(LPSTR aBuf, DWORD aLen)
{
	// the recv len must be as large as possible
	// due to avoid lossing real-time signal
	CM_ASSERTE(aLen > 0);
	int nRecv = m_SockTcp.Recv(aBuf, aLen);

#ifdef CM_DEBUG_SEND_RECV
	m_RecordDebug.push_back(RecEleType('R', nRecv < 0 ? -errno : nRecv));
#endif // CM_DEBUG_SEND_RECV

	if (nRecv < 0) {
		if (errno == EWOULDBLOCK)
			return 0;
		else {
			CErrnoGuard egTmp;
			VP_TRACE_ERROR("CTransportTcp::Recv_t, recv() failed! err=%d", errno);
			return -1;
		}
	}
	if (nRecv == 0) {
		// it is a graceful disconnect
		return -1;
	}
	return nRecv;
}

int CTransportTcp::Send_t(LPCSTR aBuf, DWORD aLen)
{
	CM_ASSERTE(aLen > 0);
	int nRet = m_SockTcp.Send(aBuf, aLen);
	
#ifdef CM_DEBUG_SEND_RECV
	m_RecordDebug.push_back(RecEleType('S', nRet < 0 ? -errno : nRet));
#endif // CM_DEBUG_SEND_RECV

	if (nRet < 0) {
		if (errno == EWOULDBLOCK)
			return 0;
		else {
			CErrnoGuard egTmp;
			VP_TRACE_ERROR("CTransportTcp::Send_t, send() failed! err=%d", errno);
			return -1;
		}
	}
	return nRet;
}
