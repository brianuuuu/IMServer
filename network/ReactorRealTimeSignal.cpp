
#include "CmBase.h"
#include "ReactorRealTimeSignal.h"
#include "TimerQueueOrderedList.h"
#include "Addr.h"
#include <unistd.h>
#include <asm/unistd.h>
#include <linux/sysctl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/epoll.h>
#include <signal.h>
#include "TraceLog.h"
#include <linux/unistd.h>
#include <linux/types.h>

#define LOCAL_HOST "127.0.0.1"
#define IPC_SYNC_PORT 61122
#define MAX_CLIENT 50000

//_syscall1(int, _sysctl, struct __sysctl_args *, args)

CReactorRealTimeSignal::CReactorRealTimeSignal()
        : m_SigNum(SIGRTMAX)
		, m_pTimerQueue(NULL)
		, m_bQuit(FALSE)
        , m_sockSyncReceive( -1 )
        , m_sockSyncSend( -1 )
        , m_sSyncPort(IPC_SYNC_PORT)
{
}

CReactorRealTimeSignal::~CReactorRealTimeSignal()
{
	Close();
}

int CReactorRealTimeSignal::Open()
{
	int nRet;

	m_epfd = epoll_create(MAX_CLIENT);
	printf("CReactorRealTimeSignal::Open epoll_create\n");
/*
	nRet = ::sigemptyset(&m_Sigset);
	if (nRet == -1) {
		CM_ERROR_LOG(("CReactorRealTimeSignal::Open, sigemptyset() failed!"));
		return -1;
	}
	nRet = ::sigaddset(&m_Sigset, m_SigNum);
	if (nRet == -1) {
		CM_ERROR_LOG(("CReactorRealTimeSignal::Open, sigaddset(m_SigNum) failed!"));
		return -1;
	}
	// Put SIGIO in the same set, since we need to listen for that, too.
	nRet = ::sigaddset(&m_Sigset, SIGIO);
	if (nRet == -1) {
		CM_ERROR_LOG(("CReactorRealTimeSignal::Open, sigaddset(SIGIO) failed!"));
		return -1;
	}
	// Finally, block delivery of those signals.
	nRet = ::sigprocmask(SIG_BLOCK, &m_Sigset, NULL);
	if (nRet == -1) {
		CM_ERROR_LOG(("CReactorRealTimeSignal::Open, sigprocmask() failed!"));
		return -1;
	}
*/	
#ifdef EMBEDED_LINUX
	int nMaxSig = SetProcRtsigMax(64);

	if (SetRlimit(RLIMIT_NOFILE, 64) == -1)
		goto fail;

#else
	// don't care whether SetProcRtsigMax() failed or not.
	/*int nMaxSig = SetProcRtsigMax(16383);
	printf("Set Proc Signal Max to %d\n", nMaxSig);*/

	if (SetRlimit(RLIMIT_NOFILE, 8192) == -1)
        {
                printf("Failed to set RLIMIT_NOFILE\n");
		goto fail;
        }
#endif
	CM_ASSERTE(!m_pTimerQueue);
	m_pTimerQueue = new CTimerQueueOrderedList();


	m_bQuit = FALSE;

        m_sockSyncReceive = BindSocket();
        if ( m_sockSyncReceive == -1 )
            goto fail;
        m_sockSyncSend =  socket( AF_INET, SOCK_DGRAM, 0 );
        if ( m_sockSyncSend == -1 )
            goto fail;

        SetHandleSignal( m_sockSyncReceive );

	return 0;
	
fail:
	Close();
	return -1;
}

int CReactorRealTimeSignal::Close()
{
	m_bQuit = TRUE;
	
        if ( m_sockSyncReceive != -1 )
            close( m_sockSyncReceive );
        if ( m_sockSyncSend != -1 )
            close( m_sockSyncSend );
		if ( m_epfd != -1 )
			close( m_epfd );
	if (m_pTimerQueue) {
		delete m_pTimerQueue;
		m_pTimerQueue = NULL;
	}
	return 0;
}

int CReactorRealTimeSignal::SetHandleSignal( CM_HANDLE fdNew )
{
	struct epoll_event ev;
	ev.data.fd=fdNew;
	ev.events=EPOLLIN;
	int nRet = epoll_ctl(m_epfd,EPOLL_CTL_ADD,fdNew,&ev);
	printf("CReactorRealTimeSignal::SetHandleSignal fdNew=%d nRet=%d\n",fdNew,nRet);
/*
    int nflags, nFcntl;
    // Set this fd to emit signals.
    nflags = ::fcntl(fdNew, F_GETFL);
		
    // Set socket flags to non-blocking and asynchronous 
    nflags |= O_RDWR | O_NONBLOCK | O_ASYNC;
    nFcntl = ::fcntl(fdNew, F_SETFL, nflags);
    if (nFcntl < 0) {
	CM_ERROR_LOG(("CReactorRealTimeSignal::RegisterHandler, fcntl(F_SETFL) failed! nFcntl=%d", nFcntl));
        return -1;
    }

    // Set signal number >= SIGRTMIN to send a RealTime signal 
    nFcntl = ::fcntl(fdNew, F_SETSIG, m_SigNum);
    if (nFcntl < 0) {
		CM_ERROR_LOG(("CReactorRealTimeSignal::RegisterHandler, fcntl(F_SETSIG) failed! nFcntl=%d", nFcntl));
		return -1;
    }

    // Set process id to send signal to 
	printf("set signal pid %d\n", getpid());
    nFcntl = ::fcntl(fdNew, F_SETOWN, getpid());
    if (nFcntl < 0) {
 		CM_ERROR_LOG(("CReactorRealTimeSignal::RegisterHandler, fcntl(F_SETOWN) failed! nFcntl=%d", nFcntl));
		return -1;
    }

#ifdef F_SETAUXFL
    // Allow only one signal per socket fd 
    nFcntl = ::fcntl(fdNew, F_SETAUXFL, O_ONESIGFD);
    if (nFcntl < 0) {
		CM_ERROR_LOG(("CReactorRealTimeSignal::RegisterHandler, fcntl(F_SETAUXFL) failed! nFcntl=%d", nFcntl));
		return -1;
    }
#endif // !F_SETAUXFL
*/
    return 0;
}

int CReactorRealTimeSignal::
RegisterHandler(CEventHandlerBase *aEh, CEventHandlerBase::MASK aMask)
{
	CM_ASSERTE_RETURN(aEh, -1);
	CM_HANDLE fdNew = aEh->GetHandle();
	CM_ASSERTE_RETURN(fdNew != CM_INVALID_HANDLE, -1);

	CEventHandlerBase::MASK maskNew = aMask & CEventHandlerBase::ALL_EVENTS_MASK;
	if (maskNew == CEventHandlerBase::NULL_MASK) {
		VP_TRACE_ERROR("CReactorRealTimeSignal::RegisterHandler, NULL_MASK.");
		return -1;
	}

        EHPair *pEHFind = m_handlers.Find( fdNew );
	if (pEHFind &&  pEHFind->first == aEh && maskNew == pEHFind->second ) {
		VP_TRACE_ERROR("CReactorRealTimeSignal::RegisterHandler, mask is equal. fdNew=%d", fdNew);
		return 0;
	}

	if ( !pEHFind ) {
		SetHandleSignal( fdNew );

		// connector should register this before call ::connect, 
		// so check if there is data to read except connect mask
		if (CM_BIT_DISABLED(maskNew, CEventHandlerBase::CONNECT_MASK) && CheckPollIn(fdNew, aEh) == -1)
			return -1;

		pEHFind = new EHPair( aEh, maskNew );
		if ( !pEHFind )
			return -1;

		m_handlers.Add( fdNew, pEHFind );
	} else {
		if (pEHFind->first == aEh) {
			pEHFind->second |= maskNew;
		} else {
			pEHFind->first = aEh;
			pEHFind->second = maskNew;
		}
	}
        
	return 0;
}

int CReactorRealTimeSignal::
RemoveHandler(CEventHandlerBase *aEh, CEventHandlerBase::MASK aMask)
{
	CM_ASSERTE_RETURN(aEh, -1);
	CM_HANDLE fdNew = aEh->GetHandle();
	CM_ASSERTE_RETURN(fdNew != CM_INVALID_HANDLE, -1);

	CEventHandlerBase::MASK maskNew = aMask & CEventHandlerBase::ALL_EVENTS_MASK;
	if (maskNew == CEventHandlerBase::NULL_MASK) {
		VP_TRACE_ERROR("CReactorRealTimeSignal::RemoveHandler, NULL_MASK.");
		return -1;
	}

    EHPair* pEHFind = m_handlers.Find( fdNew );
	if ( !pEHFind ){
//		VP_TRACE_ERROR(("CReactorRealTimeSignal::RemoveHandler, fd not registed. fdNew=%d", fdNew));
		return -1;
	}

	return RemoveSocket_i(fdNew, pEHFind, aMask);
}

int CReactorRealTimeSignal::
NotifyHandler(CEventHandlerBase *aEh, CEventHandlerBase::MASK aMask)
{
    return 0;
}

int CReactorRealTimeSignal::
ScheduleTimer(CEventHandlerBase *aEh, LPVOID aArg, const CTimeValue &aInterval, DWORD aCount)
{
	CM_ASSERTE_RETURN(m_pTimerQueue, -1);
	return m_pTimerQueue->ScheduleTimer(aEh, aArg, aInterval, aCount);
}

int CReactorRealTimeSignal::CancelTimer(CEventHandlerBase *aEh)
{
	CM_ASSERTE_RETURN(m_pTimerQueue, -1);
	return m_pTimerQueue->CancelTimer(aEh);
}

int CReactorRealTimeSignal::RunEventLoop()
{
	CM_ASSERTE_RETURN(m_pTimerQueue, -1);
	for ( ; ; ) {
		if (m_bQuit)
			return 0;

		CTimeValue tvTimeout = CTimeValue::s_tvZero;
		CTimeValue tvCur = CTimeValue::GetTimeOfDay();
		CTimeValue tvEarliest = m_pTimerQueue->GetEarliestTime();
		if (tvCur < tvEarliest) {
			if (tvEarliest != CTimeValue::s_tvMax)
				tvTimeout = tvEarliest - tvCur;
			else
			{
				tvTimeout.Set(0, 1);
			}
		}
		//VP_TRACE_INFO("CReactorRealTimeSignal::RunEventLoop befor HandleIoEvents");
		if (HandleIoEvents(tvTimeout) == -1)
		{
			return -1;
		}
		//VP_TRACE_INFO("CReactorRealTimeSignal::RunEventLoop after HandleIoEvents m_bQuit=%d",m_bQuit);
		if (m_bQuit)
			return 0;
		
//		printf("check expire...\n");
    		m_pTimerQueue->CheckExpire();
//		printf("...check expire\n");
	}
	return 0;
}

int CReactorRealTimeSignal::StopEventLoop()
{
	m_bQuit = TRUE;
	NotifyHandler(NULL, CEventHandlerBase::NULL_MASK);
	return 0;
}

int CReactorRealTimeSignal::
RemoveSocket_i(CM_HANDLE aFd, EHPair* pEHFind, CEventHandlerBase::MASK aMask)
{
	CEventHandlerBase::MASK maskNew = aMask & CEventHandlerBase::ALL_EVENTS_MASK;
	CEventHandlerBase::MASK maskEh = pEHFind->second;
	CEventHandlerBase::MASK maskSelect = (maskEh & maskNew) ^ maskEh;
	if (maskSelect == maskEh) {
		VP_TRACE_ERROR("CReactorRealTimeSignal::RemoveSocket_i, mask is equal. aMask=%d", aMask);
		return -1;
	}

	if (maskSelect == CEventHandlerBase::NULL_MASK) {
		int nflags, nFcntl;
		nFcntl = ::fcntl(aFd, F_GETFL, &nflags);
		if (nFcntl < 0) {
			VP_TRACE_ERROR("CReactorRealTimeSignal::RemoveSocket_i, fcntl(F_GETFL) failed! nFcntl=%d", nFcntl);
		}
		
		nflags &= ~O_ASYNC;
		nFcntl = ::fcntl(aFd, F_SETFL, nflags);
		if (nFcntl < 0) {
//			VP_TRACE_ERROR("CReactorRealTimeSignal::RemoveSocket_i, fcntl(F_SETFL) failed! nFcntl=%d", nFcntl);
		}

		CEventHandlerBase *pEh = pEHFind->first;
		if (aMask & CEventHandlerBase::SHOULD_CALL) {
			int nRet = epoll_ctl(m_epfd,EPOLL_CTL_DEL,aFd,NULL);
			pEh->OnClose(aFd, maskEh);
		}

        m_handlers.Delete( aFd );
	}
	else
		pEHFind->second = maskSelect;
	return 0;
}

int CReactorRealTimeSignal::HandleIoEvents(const CTimeValue &aTimeout)
{
	struct timespec tsBuf;
	struct timespec *pTs = NULL;
	if (aTimeout != CTimeValue::s_tvMax) {
		tsBuf.tv_sec = aTimeout.GetSec();
		tsBuf.tv_nsec = aTimeout.GetUsec() * 1000;
		pTs = &tsBuf;
	}
	
	struct epoll_event events[20];
	int nfds = epoll_wait(m_epfd,events,20,-1);
	VP_TRACE_INFO("CReactorRealTimeSignal::HandleIoEvents nfds=%d",nfds);
	int i = 0;
	for(i=0;i<nfds;++i)
	{
		int fdSig = events[i].data.fd;
	
		//VP_TRACE_INFO("after epoll_wait()fdSig=%d events[i].events=%d",fdSig,events[i].events);
		if ( fdSig == m_sockSyncReceive )
		{
			/*struct sockaddr_in sin;
			sin.sin_family = AF_INET;
			socklen_t nLen = sizeof( sin );
	
			unsigned char bufSync[256];
			recvfrom( m_sockSyncReceive,
				bufSync,
				sizeof( bufSync ),
				0,
				(struct sockaddr*)&sin,
				&nLen
				);
	
			m_cmdQueue.Execute();
			continue;*/
		}
		//else
			//VP_TRACE_INFO("CReactorRealTimeSignal::HandleIoEvents fdSig=%d", fdSig);
		//VP_TRACE_INFO("CReactorRealTimeSignal::HandleIoEvents fdSig == m_sockSyncReceive");
		EHPair* pEHFind = m_handlers.Find( fdSig );
		if ( !pEHFind ) {
//			VP_TRACE_ERROR(("CReactorRealTimeSignal::RunEventLoop, fdSig not regiested, fdSig=%d", fdSig));
			return 0;
		}
		int nOnCall = 0;
		CEventHandlerBase::MASK maskSig = CEventHandlerBase::NULL_MASK;
		long lSigEvent = events[i].events;
		if (lSigEvent & (EPOLLERR|EPOLLHUP)) {
			if (lSigEvent != EPOLLERR)
				VP_TRACE_ERROR("CReactorRealTimeSignal::RunEventLoop, !EPOLLERR, fdSig=%x", fdSig);
			nOnCall = -1;
		}
		else 
		{
			if ((lSigEvent & EPOLLIN) &&
				(pEHFind->second & (CEventHandlerBase::READ_MASK | CEventHandlerBase::ACCEPT_MASK | CEventHandlerBase::CONNECT_MASK)))
			{
				//VP_TRACE_INFO("CReactorRealTimeSignal::HandleIoEvents EPOLLIN MASK=%d",pEHFind->second);
				maskSig |= CEventHandlerBase::READ_MASK;
			}
			if ((lSigEvent & EPOLLOUT) &&
				(pEHFind->second & (CEventHandlerBase::WRITE_MASK | CEventHandlerBase::CONNECT_MASK)))
			{
				//VP_TRACE_INFO("CReactorRealTimeSignal::HandleIoEvents EPOLLOUT");
				maskSig |= CEventHandlerBase::WRITE_MASK;
			}
		}
		//VP_TRACE_INFO("CReactorRealTimeSignal::HandleIoEvents nOnCall=%d", nOnCall);
		CEventHandlerBase *pEh = pEHFind->first;
		if (nOnCall == 0 && (maskSig & CEventHandlerBase::READ_MASK))
			nOnCall = pEh->OnInput(fdSig);
		if (nOnCall == 0 && (maskSig & CEventHandlerBase::WRITE_MASK))
			nOnCall = pEh->OnOutput(fdSig);
	
		if (nOnCall == -1) {
			RemoveSocket_i(fdSig, pEHFind, CEventHandlerBase::ALL_EVENTS_MASK | CEventHandlerBase::SHOULD_CALL);
		}
	}
	/*
	siginfo_t siginfo;
	int sigRet = ::sigtimedwait(&m_Sigset, &siginfo, pTs);
	if (sigRet == -1 || sigRet == SIGIO) {
		if (sigRet == -1 && (errno == EINTR || errno == EAGAIN))
			return 0;
		VP_TRACE_ERROR("CReactorRealTimeSignal::RunEventLoop, "
			"sigwaitinfo() failed! sigRet=%d err=%d", sigRet, errno);
	//	return -1;
		return 0;
	}
//	printf("handle event....\n");
	CM_ASSERTE(sigRet == m_SigNum);
	int fdSig = siginfo.si_fd;

	if ( fdSig == m_sockSyncReceive )
	{
		struct sockaddr_in sin;
		sin.sin_family = AF_INET;
		socklen_t nLen = sizeof( sin );

		unsigned char bufSync[256];
		recvfrom( m_sockSyncReceive,
			bufSync,
			sizeof( bufSync ),
			0,
			(struct sockaddr*)&sin,
			&nLen
			);

		m_cmdQueue.Execute();
		return 0;
	}

	EHPair* pEHFind = m_handlers.Find( fdSig );
	if ( !pEHFind ) {
//		VP_TRACE_ERROR(("CReactorRealTimeSignal::RunEventLoop, fdSig not regiested, fdSig=%d", fdSig));
		return 0;
	}

	int nOnCall = 0;
	CEventHandlerBase::MASK maskSig = CEventHandlerBase::NULL_MASK;
	long lSigEvent = siginfo.si_band;
	if (lSigEvent & (POLLERR|POLLHUP|POLLNVAL)) {
		if (lSigEvent != POLLERR)
			VP_TRACE_ERROR("CReactorRealTimeSignal::RunEventLoop, !POLLERR, lSigEvent=%d", lSigEvent);
		nOnCall = -1;
	}
	else {
		if ((lSigEvent & POLLIN) &&
			(pEHFind->second & (CEventHandlerBase::READ_MASK | CEventHandlerBase::ACCEPT_MASK | CEventHandlerBase::CONNECT_MASK)))
		{
			maskSig |= CEventHandlerBase::READ_MASK;
		}
		if ((lSigEvent & POLLOUT) &&
			(pEHFind->second & (CEventHandlerBase::WRITE_MASK | CEventHandlerBase::CONNECT_MASK)))
		{
			maskSig |= CEventHandlerBase::WRITE_MASK;
		}
	}

	CEventHandlerBase *pEh = pEHFind->first;
	if (nOnCall == 0 && (maskSig & CEventHandlerBase::READ_MASK))
		nOnCall = pEh->OnInput(fdSig);
	if (nOnCall == 0 && (maskSig & CEventHandlerBase::WRITE_MASK))
		nOnCall = pEh->OnOutput(fdSig);

	if (nOnCall == -1) {
		RemoveSocket_i(fdSig, pEHFind, CEventHandlerBase::ALL_EVENTS_MASK | CEventHandlerBase::SHOULD_CALL);
	}*/
//	printf("...handle event\n");
	return 0;
}

int CReactorRealTimeSignal::CheckPollIn(int aFd, CEventHandlerBase *aEh)
{
/*
	struct pollfd pfRead[1];
	pfRead[0].fd = aFd;
	pfRead[0].events = POLLIN | POLLERR|POLLHUP|POLLNVAL;
	pfRead[0].revents = 0;
	int nReady = ::poll(pfRead, 1, 0);
	if (nReady < 0) {
		CM_ERROR_LOG(("CReactorRealTimeSignal::CheckPollIn, poll() failed! err=%d", errno));
		return -1;
	}
	if (nReady > 0) {
		if (pfRead[0].revents & (POLLERR|POLLHUP|POLLNVAL)) {
			CM_ERROR_LOG(("CReactorRealTimeSignal::CheckPollIn, poll(POLLERR). revents=%d", (int)pfRead[0].revents));
			return -1;
		}
		else if (pfRead[0].revents & POLLIN) {
#if 1
			CInetAddr addrRemote;
			int nSize = (int)addrRemote.GetSize();
			int nGet1 = ::getpeername(aFd,
					reinterpret_cast<sockaddr *>(const_cast<sockaddr_in *>(addrRemote.GetPtr())),
					reinterpret_cast<socklen_t*>(&nSize)
					);
			CM_ASSERTE(nGet1 == 0);

			DWORD dwIoRead = 0;
			int nGet2 = ::ioctl(aFd, FIONREAD, &dwIoRead);
			CM_ASSERTE(nGet2 == 0);

			VP_TRACE_ERROR("CReactorRealTimeSignal::CheckPollIn, poll(POLLIN)."
				" revents=%d fd=%d io=%u addr=%s port=%d", 
				(int)pfRead[0].revents, aFd, dwIoRead, 
				"", (int)addrRemote.GetPort());
#else
			VP_TRACE_ERROR("CReactorRealTimeSignal::CheckPollIn, poll(POLLIN). revents=%d", (int)pfRead[0].revents);
#endif
			return NotifyHandler(aEh, CEventHandlerBase::READ_MASK);
		}
		else {
			VP_TRACE_ERROR("CReactorRealTimeSignal::CheckPollIn, poll(unknow). revents=%d", (int)pfRead[0].revents);
			return 0;
		}
	}
	*/
	return 0;
}

CTimerQueueBase* CReactorRealTimeSignal::GetTimerQueue()
{
	return m_pTimerQueue;
}

int CReactorRealTimeSignal::SetRlimit(int aResource, int aMaxNum)
{
	rlimit rlCur;
	::memset(&rlCur, 0, sizeof(rlCur));
	int nRet = ::getrlimit(aResource, &rlCur);
	if (nRet == -1 || rlCur.rlim_cur == RLIM_INFINITY) {
		CM_ERROR_LOG(("CReactorRealTimeSignal::SetRlimit, getrlimit() failed! err=%d", errno));
                   printf("getrlimit failed\n");
		return -1;
	}
	
	if (aMaxNum > static_cast<int>(rlCur.rlim_cur)) {
		rlimit rlNew;
		::memset(&rlNew, 0, sizeof(rlNew));
		rlNew.rlim_cur = aMaxNum;
                rlNew.rlim_max = aMaxNum;
		nRet = ::setrlimit(aResource, &rlNew);
		if (nRet == -1) {
			if (errno == EPERM) {
				VP_TRACE_ERROR("CReactorRealTimeSignal::SetRlimit, setrlimit() failed. "
					"you should use superuser to setrlimit(RLIMIT_NOFILE)!");
				nRet = 0;
			}
			else {
				CM_ERROR_LOG(("CReactorRealTimeSignal::SetRlimit, setrlimit() failed! err=%d", errno));
                            perror("setrrlimit failed: ");
			    return -1;
			}
		}
	}
	
	return aMaxNum;
}

int CReactorRealTimeSignal::SetProcRtsigMax(int aMaxNum)
{
	int nSigMax;
	size_t nNumLen = sizeof(nSigMax);
	int pName[] = { CTL_KERN, KERN_RTSIGMAX };
	
	struct __sysctl_args args;
	args.name = pName;
	args.nlen = sizeof(pName)/sizeof(pName[0]);
	args.oldval = &nSigMax;
	args.oldlenp = &nNumLen;
	args.newval = NULL;
	args.newlen = 0;
	
	//if (_sysctl(&args) == -1) {
	if(syscall(2, __NR__sysctl, &args) == -1) {
		CM_ERROR_LOG(("CReactorRealTimeSignal::SetProcRtsigMax, _sysctl(get) failed! err=%d", errno));
		return -1;
	}
	
	if (aMaxNum > nSigMax) {
		int nNewSigMax = aMaxNum;
		args.oldval = NULL;
		args.oldlenp = 0;
		args.newval = &nNewSigMax;
		args.newlen = sizeof(nNewSigMax);
		//if (_sysctl(&args) == -1) {
		if(syscall(2, __NR__sysctl, &args) == -1) {
			if (EPERM == errno) {
				VP_TRACE_ERROR("CReactorRealTimeSignal::SetProcRtsigMax, _sysctl(set) failed. "
					"you should use superuser to _sysctl(rtsig-max)!");
				return nSigMax;
			}
			else {
				CM_ERROR_LOG(("CReactorRealTimeSignal::SetProcRtsigMax, _sysctl(set) failed! err=%d", errno));
				return -1;
			}
		}
	}
	else
		aMaxNum = nSigMax;
	return aMaxNum;
}

int CReactorRealTimeSignal::BindSocket()
{
    int sock;
    sock = socket( AF_INET, SOCK_DGRAM, 0 );
    if ( sock == -1 )
    {
        printf("Fail to socket for sync\n");
        return -1;
    }
	
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr( LOCAL_HOST );
    socklen_t nLen = sizeof( sin );

    m_sSyncPort = IPC_SYNC_PORT;
    while ( m_sSyncPort < 65536 ) 
    {
        sin.sin_port = htons( m_sSyncPort );
        if (  bind( sock, (struct sockaddr*)&sin, nLen ) != 0 )
        {
            ++m_sSyncPort;
            continue;
        }
        printf("IPC socket port is %d\n", m_sSyncPort);
        break;
    }
    
    if ( m_sSyncPort >= 65536 )
    {
        printf("Failed to bind sync port\n");
        close( sock );
        sock = -1;
    }

    return sock;                             
}


int CReactorRealTimeSignal::EnqueueCommandRequest( CCommandRequest* 
                                         pCommandRequest )
{
    m_cmdQueue.Enqueue( pCommandRequest );
    char cSync = 0;
    struct sockaddr_in sin;

    sin.sin_family = AF_INET;
    sin.sin_port = htons( m_sSyncPort );
    sin.sin_addr.s_addr = inet_addr( LOCAL_HOST );
    socklen_t nLen = sizeof( sin );
	
	
    sendto( m_sockSyncSend, 
	    &cSync, 
	    sizeof(char),
	    0,
	    (const struct sockaddr*)&sin,
	    nLen
	  );

    return 0;
}

int CReactorRealTimeSignal::ModifyHandleSignal( CEventHandlerBase *aEh, bool bEpollout )
{
	struct epoll_event ev;
	ev.data.fd = aEh->GetHandle();
	if (bEpollout)
	{
		ev.events = EPOLLIN | EPOLLOUT;
	}
	else
	{
		ev.events = EPOLLIN;
	}
	int nRet = epoll_ctl(m_epfd,EPOLL_CTL_MOD,aEh->GetHandle(),&ev);
	VP_TRACE_INFO("CReactorRealTimeSignal::ModifyHandleSignal bEpollout=%d nRet=%d",bEpollout,nRet);
}
