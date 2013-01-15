#ifndef __FLOW_CONTROL_H__
#define __FLOW_CONTROL_H__
#include "CmBase.h"
#include "NetworkImpl.h"

#define ShowDebugInfo(x,y)
#define MEDIA_FRAGMENT_SIZE		1280
#define INVALID_FRAG_ITEM		-1
#define DEFAULT_GOP_SIZE		15
#define MAX_GROUP_NUMBER		20
#define INDICATE_RTT_NUMBER		7
#define INDICATE_MIN_TIME		500
#define INDICATE_MAX_TIME		2000
#define RESEND_RTT_NUMBER		6
#define RESEND_MIN_TIME			400
#define RESEND_MAX_TIME			1600
#define MAX_RESEND_PERCENT		10
#define  MAX_SUB_FRAG			10

#define FF_NOFRAGMENT           0
#define FF_LASTFRAGMENT         1
#define FF_MOREFRAGMENT         2
#define FF_FC_MEDIA             3 /*Flow control medai data*/
#define FF_FC_RESEND            4 /*flow control resend request*/
#define FF_FC_EVALRTT           5 /*flow control evaluate RTT request*/

/*
 *	Algrithom Description:
 *  1. 采用重传提高接收的包的数目
 *  2. 缓冲5个RTT与0.5秒之间的最大值，并且不超过5秒，向上传递数据
 *  3. 4个RTT与0.4秒之间的最大值，并且不超过4秒，称为重发时间,当一个新的GOP到达时间
 *	   超过重发时间，本GOP不再请求重发
 *  4. 每个GOP除了IFrame外的Packet要求重发的字节数不能超过总字节数的10%
 *  
 */
typedef struct _tagFlowControlHeader
#define USHORT unsigned short
{
    BYTE   bFragFlag;
    BYTE   bMediaSeq; /*0 is I frame, n is the nth p frame*/
    USHORT usOffset;    
    USHORT usSeq;
    USHORT usLen;
    BYTE   bTotalFrag;
    BYTE   bFragOff;
    USHORT usGroupSeq;
} FlowControlHeader;

typedef struct _tagResendInfo
{
    BYTE   bFragFlag;
    BYTE   bMediaSeq; /*0 is I frame, n is the nth p frame*/
    USHORT usResendSeq;
    USHORT usResendOff;
    USHORT usResendLen;
} ResendInfo;

typedef struct _tagEvalRtt
{
    BYTE   bFragFlag;
    BYTE   bReserved[3]; /*0 is I frame, n is the nth p frame*/
	DWORD  dwTicket;
} EvalRtt;

#ifdef WIN32
class CMediaFragment
{
public:
	CMediaFragment();
	~CMediaFragment();
	BOOL IsComplete();
	int SendResendPacket(INetConnection *pSocket, DWORD dwIP, USHORT usPort);
	void SetSeq(USHORT usSeq);
	BOOL InsertPacket(FlowControlHeader *pHeader, BYTE* lpData, USHORT usSegLen, DWORD ticket);
	int GetPacket(BYTE *buffer, int len);
private:
	int TwoSetMinus(int *ss1, int *se1, int *ss2, int *se2);
	void InsertSet(int s, int e);
private:
	USHORT m_usSeq;
    USHORT m_usLen;
	BOOL m_bIsComplete;
	int m_arrayRequireFrag[MAX_SUB_FRAG][2];
	BYTE m_strBuf[MEDIA_FRAGMENT_SIZE];
};

class CMediaPacket
{
public:
	CMediaPacket();
	~CMediaPacket();
	BOOL IsComplete();
	void ReInit();
	int SendResendPacket(DWORD ticket, INetConnection *pSocket, 
		DWORD dwIP, USHORT usPort, USHORT usLastestSeq, DWORD rtt);
	BOOL InsertPacket(FlowControlHeader *pHeader, BYTE* lpData, USHORT usSegLen, DWORD ticket);
	int GetPacket(BYTE *buffer, int len);
	DWORD GetPacketTicket();
	BYTE GetMediaSeq();
private:
	BOOL   m_bIsComplete;
	DWORD  m_dwFirstRecvTicket;
	DWORD  m_dwRecvTicket;
	DWORD  m_dwPrevResendTicket;
    BYTE   m_bTotalFrags;
	BYTE   m_bRecvFrags;
	BYTE   m_bCompleteFrags;
	USHORT m_usStartSeq;
	USHORT m_usGroupSeq;
    BYTE   m_bMediaSeq;
	CMediaFragment *m_pMediaFragments;
};
/*
 *	Group of picture
 */
class CGroupOfPicture
{
public:
	CGroupOfPicture();
	~CGroupOfPicture();
	void ReInit();
	BOOL IsFinish();
	void Finish();
	int GetUsefulPackNum();
	int GetUsefulPack(BYTE *buffer, int len);
	void InsertPacket(FlowControlHeader *pHeader, BYTE* lpData, USHORT usSegLen, DWORD ticket);
	void SendResendPacket(DWORD ticket, INetConnection *pSocket, DWORD dwIP, 
		USHORT usPort, USHORT usLastestSeq, DWORD rtt);
	DWORD GetGroupTicket();
	DWORD GetUsefulPackTicket();
	BYTE GetUserfulMediaSeq();
	USHORT GetGroupSeq();
private:
	DWORD m_dwRecvLen;
	DWORD m_dwTotalResendLen;
	DWORD m_dwResendLen;
	DWORD m_dwFirstTicket;
	int   m_nIndex;
	USHORT m_usGroupSeq;
	USHORT m_usCompletePackets;
	USHORT m_usUsefulPackets;
	BOOL   m_bIsFinish;
	CMediaPacket m_aMediaPackets[DEFAULT_GOP_SIZE];
public:
	USHORT m_usUsedPackets;
};

/*
 *	FlowControlConnection Means ((FromAddr, FromPort), (LocalAddr, LocalPort)) pairs
 */
class CFlowControlConnection 
{
public:
	CFlowControlConnection(DWORD dwIP, USHORT usPort);
	~CFlowControlConnection();
	ULONG InsertPacket(INetConnection *pSocket, BYTE* lpData, USHORT usSegLen, 
		LPTSTR lpszBuffer, ULONG uBufferSize);
	void OnReceiveRTTEval(void* lpData, USHORT usSegLen);
	void ResetBuffer();
private:
	void SendRttEvalPacket(INetConnection *pSocket);
	int OnFlowTimer(DWORD dwCurrent,INetConnection *pSocket, 
		LPTSTR lpszBuffer, ULONG uBufferSize);
private:
	DWORD m_dwRttTicket;
	int	  m_dwRttReceived;
	DWORD m_dwRttSendTick;
	DWORD m_dwTimerTick;
	DWORD m_dwIndTicket;
	USHORT m_usLastestSeq;
	DWORD m_dwStopTicket;
	DWORD m_dwIP;
	USHORT m_usPort;
	USHORT m_usCurPutO;
	USHORT m_usCurPutN;
	USHORT m_usCurInd;
	CGroupOfPicture m_cGroupOfPicture[MAX_GROUP_NUMBER];

///For Jitter buffer
	DWORD m_dwLastIndTick;
	DWORD m_dwIFrameTick;
	DWORD m_dwLastUserfulTick;
	DWORD m_dwLastSeq;
	DWORD m_dwInterval;
	BYTE  m_byteParam;
};
#endif

#ifdef EMBEDED_LINUX

#define MESSAGE_BLOCK_SIZE	(8/*NETWORK_HEADER_SIZE*/ + MEDIA_FRAGMENT_SIZE)
#define RING_BUF_SIZE 	512
#define SEQ_SIZE	65536

typedef struct _message_block
{
    INetConnection *pCon;
    struct sockaddr_in addr;
    struct timeval addtime;
    int len;
    int remainlen;
    unsigned short seq;
    unsigned short Reserve;
    unsigned char buf[4+MESSAGE_BLOCK_SIZE];
}message_block_t;


class CFlowControlSend:public INetTimerSink
{
public:
	CFlowControlSend(){FlowControlInit();}
	~CFlowControlSend(){FlowControlFini();}
	void FlowControlInit();
	void FlowControlFini();
	void FlowControlSetSendBPS(int bps);
	void FlowControlPutBuf(INetConnection *pCon, unsigned char *buf, unsigned long len, 
		 struct timeval *sendtm, 
		unsigned char mseq,unsigned int totalfrag, 
		unsigned int currentfrag, unsigned short Iframeseq);
	void FlowControlSend();
	void TimerFlowControlSend();
	void OnReceiveResend(char *buf);
	void FlowControlReinitialize();
	void OnTimer(void *pArg, INetTimer *pTimer){TimerFlowControlSend();}
private:
	int TryLock();
	void Lock();
	void UnLock();
	void DropPacket();
	void SendOutPacket(int size);
		
private:
	CNetTimer *m_pTimer;
	int locked;
	pthread_mutex_t g_fc_lock;
	unsigned short current_seq;
	struct timeval prev_send_time, prev_cal_time;
	int send_remains;
	
	message_block_t *ringbuf;
	int put_point, send_point, end_point;
	
	int g_send_byte_rate;
	int g_send_len, g_resend_len, g_send_out_len,g_cal_len;
    	unsigned char m_sendbuf[4+MESSAGE_BLOCK_SIZE];
	
};
#endif
#endif

