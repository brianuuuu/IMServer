/*
 *	FlowControl.c Implementation of flow control.
 *  
 */

#include "FlowControl.h"
#ifdef WIN32
#include "Mmsystem.h"
/*
 *	�ش��㷨��
 *      GOP�Ĳ��棺	ǰ���֡�����ش��������֡��û�б�ҪҪ���ش���
 *		Frame���棺 1. Frame�����յ�һ����Ƭ�������ش�����Ϣ��
 *					2. ���Frame���ڴ�������У��������ش���
 *					3. �ش��������ٵ�1.2��Rtt���ٽ����ش���
 *					4. ���ش������պ�50�����ڲ������ش�
 *		Fragment��	ÿ��Frame��û�н���������Fragment�������ش�
 */
const int FLOWCTRL = 1;

void MyShowDebugInfo( int nLevel, char* szMessage, ... )
{
	
//	char szFullMessage[MAX_PATH];
//	char szFormatMessage[MAX_PATH];
//	
//	// format message
//	va_list ap;
//	va_start(ap, szMessage);
//	_vsnprintf( szFormatMessage, MAX_PATH, szMessage, ap);
//	va_end(ap);
//	strncat( szFormatMessage, "\n", MAX_PATH);
//	strcpy( szFullMessage, szFormatMessage );
//	OutputDebugStringA( szFullMessage );
}

static BOOL UShortNotLessThan(USHORT val1, USHORT val2)

{
	ULONG temp = (((ULONG)val1+65536)-(ULONG)val2)%65536;
	if (temp < 2000)
		return TRUE;
	else
		return FALSE;
}

/*
 *	CMediaFragment 
 *	��Ƭ����ÿ������ĩβƬ�⣬ÿһƬ�ĳ���Ϊ1280�ֽ�
 *  �ڷ��������ر�͵�����£�ÿһƬ���п��ܱ���ϸ��
 */
CMediaFragment::CMediaFragment()
{
	m_usSeq = 0;
	m_usLen = 0;
	m_bIsComplete = 0;
	for(int i = 0; i < MAX_SUB_FRAG; i ++)
	{
		m_arrayRequireFrag[i][0] = m_arrayRequireFrag[i][1] = INVALID_FRAG_ITEM;
	}
	memset(m_strBuf, 0, sizeof(m_strBuf));
}

CMediaFragment::~CMediaFragment()
{

}
/*
 *	Ƭ�Ƿ�����
 */
BOOL CMediaFragment::IsComplete()
{
	return m_bIsComplete;
}
/*
 *	����һ��Frame��ֻҪ���յ��κ�һƬ�Ϳ���֪������Ƭ��Sequence
 */
void CMediaFragment::SetSeq(USHORT usSeq)
{
	m_usSeq = usSeq;
}

/*
 * ��������֮��Ĳ���ó���֤����Ч�ļ��ϣ�Ҳ����se* >= ss*	
 * ����ֵ:
 *	0	��ʾ����2��������1����Ľ��Ϊ�գ�ss1 �� se1����Ϊ��
 *  1	��ʾ���Ϊһ�����ϣ��������ss1��se1��
 *	2	��ʾ���Ϊ�������ϣ��������ss1,se1, ss2, se2��
 */
int CMediaFragment::TwoSetMinus(int *ss1, int *se1, int *ss2, int *se2)
{
	int number = 0, rs[2], re[2];

	if (*ss2 > *ss1)
	{
		rs[number] = *ss1;
		re[number] = (*ss2 < *se1)?*ss2:*se1;
		number ++;
	}
	if (*se2 < *se1)
	{
		re[number] = *se1;
		rs[number] = (*se2 > *ss1)?*se2:*ss1;
		number++;
	}
	if (number == 0)
	{
		*ss1 = INVALID_FRAG_ITEM;
		*se1 = INVALID_FRAG_ITEM;
	}
	if (number >= 1)
	{
		*ss1 = rs[0];
		*se1 = re[0];
	}
	if (number >= 2)
	{
		*ss2 = rs[1];
		*se2 = re[1];
	}
	return number;
}
/*
 *	���²����ļ��ϼ���Fragement�ṹ�е�δ���ռ����б���
 */
void CMediaFragment::InsertSet(int s, int e)
{
	for (int i = 0; i < MAX_SUB_FRAG; i++)
	{
		if (m_arrayRequireFrag[i][0] == INVALID_FRAG_ITEM)
		{
			m_arrayRequireFrag[i][0] = s;
			m_arrayRequireFrag[i][1] = e;
			return;
		}
	}
	MyShowDebugInfo(FLOWCTRL, "CMediaFragment::InsertSet: Error! Set Full.\n");
	
}

BOOL CMediaFragment::InsertPacket(FlowControlHeader *pHeader, BYTE* lpData, USHORT usSegLen, DWORD ticket)
{
	int p1, p2, ret, i;

	/*
	 *	�쳣�ж�
	 */
	if (pHeader->usOffset+usSegLen > MEDIA_FRAGMENT_SIZE)
	{
		pHeader->usOffset = htons(pHeader->usOffset);
		if (pHeader->usOffset+usSegLen > MEDIA_FRAGMENT_SIZE)
		{
			MyShowDebugInfo(FLOWCTRL, "CMediaFragment::InsertPacket: Receive invalid packet\n");
			return m_bIsComplete;
		}
	}

	if (m_usSeq != 0 && m_usSeq != pHeader->usSeq)
	{
		MyShowDebugInfo(FLOWCTRL, "CMediaFragment::InsertPacket: Receive sequence invalid packet\n");
		return m_bIsComplete;
	}
	/*
	 *	Ƭ�ĵ�һ��
	 */
	if (m_usLen == 0)
	{
		m_usLen = pHeader->usLen;
		m_usSeq = pHeader->usSeq;
		memcpy(m_strBuf+pHeader->usOffset, lpData, usSegLen);
		if (usSegLen == m_usLen && pHeader->usOffset == 0)
		{
			m_bIsComplete = TRUE;
		}
		else
		{
			i = 0;
			/*
			 *	����δ���ռ���
			 */
			MyShowDebugInfo(FLOWCTRL, "CMediaFragment::InsertPacket seq %d off %d len %d\n", 
				m_usSeq, pHeader->usOffset, usSegLen);
			
			if(pHeader->usOffset != 0)
			{
				m_arrayRequireFrag[0][0] = 0;
				m_arrayRequireFrag[0][1] = pHeader->usOffset;
				i = 1;
			}
			if (pHeader->usOffset+usSegLen < m_usLen)
			{
				m_arrayRequireFrag[i][0] = pHeader->usOffset+usSegLen;
				m_arrayRequireFrag[i][1] = m_usLen;
			}
		}
		return m_bIsComplete;
	}

	/*
	 *	�ǵ�һ����
	 */
	if (m_bIsComplete)
	{
		MyShowDebugInfo(FLOWCTRL, "CMediaFragment::InsertPacket: Receive duplicate packet\n");
		return m_bIsComplete;
	}
	MyShowDebugInfo(FLOWCTRL, "CMediaFragment::InsertPacket seq %d off %d len %d\n", 
		m_usSeq, pHeader->usOffset, usSegLen);
	memcpy(m_strBuf+pHeader->usOffset, lpData, usSegLen);
	/*
	 *	�޸�δ���ռ���
	 */
	for (i = 0; i < MAX_SUB_FRAG; i++)
	{
		if (m_arrayRequireFrag[i][0] != INVALID_FRAG_ITEM)
		{
			p1 = pHeader->usOffset;
			p2 = pHeader->usOffset+usSegLen;
			
			ret = TwoSetMinus(&(m_arrayRequireFrag[i][0]), &(m_arrayRequireFrag[i][1]), &p1, &p2);
			if (ret >= 2)
				InsertSet(p1, p2);
		}
	}
	/*
	 *	�ж��Ƿ�������
	 */
	m_bIsComplete = TRUE;
	for (i = 0; i < MAX_SUB_FRAG; i++)
	{
		if (m_arrayRequireFrag[i][0] != INVALID_FRAG_ITEM)
		{
			m_bIsComplete = FALSE;
			return m_bIsComplete;
		}
	}
	return m_bIsComplete;
}
/*
 *	�����ط�����
 */
int CMediaFragment::SendResendPacket(INetConnection *pSocket, DWORD dwIP, USHORT usPort)
{
	ResendInfo info;
	int i, len = 0;

	if (m_bIsComplete)
		return 0;


	memset(&info, 0, sizeof(info));
	info.bFragFlag = FF_FC_RESEND;
	info.usResendSeq = htons(m_usSeq);
	/*
	 *	û�н��յ��κΰ�
	 */
	if (m_usLen == 0)
	{
		info.usResendLen = htons(MEDIA_FRAGMENT_SIZE);
		info.usResendOff = 0;
		MyShowDebugInfo(FLOWCTRL, "Resend %d\n", m_usSeq);
		pSocket->SendData((unsigned char*)&info, sizeof(ResendInfo));
		return MEDIA_FRAGMENT_SIZE;
	}
	/*
	 *	����δ���ռ�����Ϣ
	 */
	for (i = 0; i < MAX_SUB_FRAG; i++)
	{
		if (m_arrayRequireFrag[i][0] != INVALID_FRAG_ITEM)
		{
			info.usResendOff = htons(m_arrayRequireFrag[i][0]);
			info.usResendLen = htons(m_arrayRequireFrag[i][1] - m_arrayRequireFrag[i][0]);
			len = len + info.usResendLen;
			MyShowDebugInfo(FLOWCTRL, "Resend 1 %d, off %d, len %d\n", 
				m_usSeq, m_arrayRequireFrag[i][0], m_arrayRequireFrag[i][1] - m_arrayRequireFrag[i][0]);
			pSocket->SendData((unsigned char*)&info, sizeof(ResendInfo));
		}
	}
	return len;
	
}
/*
 *	�����ݸ��Ƶ�������
 */
int CMediaFragment::GetPacket(BYTE *buffer, int len)
{
	int cplen = (m_usLen < len)?m_usLen:len;

	memcpy(buffer, m_strBuf, cplen);

	return cplen;
}

/*
 *	CMediaPacket
 */
CMediaPacket::CMediaPacket()
{
	m_bIsComplete = 0;
	m_dwRecvTicket = 0;
    m_bTotalFrags = 0;
	m_bCompleteFrags = 0;
	m_usGroupSeq = 0;
    m_bMediaSeq = 0;
	m_usStartSeq = 0;
	m_dwFirstRecvTicket = 0;
	m_pMediaFragments = NULL;
	m_dwPrevResendTicket = 0;
	m_bRecvFrags = 0;
}

CMediaPacket::~CMediaPacket()
{
	if (m_pMediaFragments != NULL)
	{
		delete []m_pMediaFragments;
		m_pMediaFragments = NULL;
	}
}

void CMediaPacket::ReInit()
{
	m_bIsComplete = 0;
	m_dwRecvTicket = 0;
	m_dwFirstRecvTicket = 0;
	m_bTotalFrags = 0;
	m_bCompleteFrags = 0;
	m_usGroupSeq = 0;
    m_bMediaSeq = 0;
	m_usStartSeq = 0;
	m_dwPrevResendTicket = 0;
	m_bRecvFrags = 0;
	/*
	 *	ɾ���ѷ����Fragmentsָ��
	 */
	if (m_pMediaFragments != NULL)
	{
		delete []m_pMediaFragments;
		m_pMediaFragments = NULL;
	}
}

BOOL CMediaPacket::IsComplete()
{
	return m_bIsComplete;
}

/*
 *����ֵ��
 *	0		����û���ش�������İ��������ش�	
 * -1		���������ش��������ڴ�������У����Һ���İ�Ҳ����Ҫ�ش�
 * ������	�ش����ֽ���
 */
int CMediaPacket::SendResendPacket(DWORD ticket, INetConnection *pSocket, 
									 DWORD dwIP, USHORT usPort, USHORT usLastestSeq, 
									 DWORD rtt)
{
	int i, len = 0;

	/*
	 *	��Frameû�н��յ��κ���Ϣ���޷������ش���
	 */
	if (m_pMediaFragments == NULL || m_bTotalFrags == 0)
		return -1;
	/*
	 *	���յ�����SeqΪ��Frame�е�һ����Ƭ��Seq��˵���������ڴ��������
	 */
	if (UShortNotLessThan(m_usStartSeq+m_bTotalFrags, usLastestSeq))
	{
		/*
		 *	���ڴ��䱾Frame�İ�
		 */
		m_dwRecvTicket = ticket;
		return -1;
	}
	/*
	 *	���䱾����ʱ����С��50����
	 */
	if (ticket - m_dwRecvTicket < 80)
	{
		return -1;
	}

	if (m_bIsComplete)
		return 0;
	
	/*
	 *	���ش������ȴ�1.2��rttʱ�����ش�������Frame���ش�����
	 */
	if (ticket - m_dwPrevResendTicket <= 23*rtt/10)
		return 0;

	m_dwPrevResendTicket = ticket;
		
	/*
	 *	ÿһ����Ƭ�����ش���Ϣ
	 */
	for (i = 0; i < m_bTotalFrags; i ++)
	{
		len += (m_pMediaFragments[i]).SendResendPacket(pSocket, dwIP, usPort);
	}
	return len;
}

BOOL CMediaPacket::InsertPacket(FlowControlHeader *pHeader, BYTE* lpData, USHORT usSegLen, DWORD ticket)
{
	if (m_bIsComplete)
	{
		return TRUE;
	}
	/*
	 *	�쳣��������
	 */
	if (pHeader->bFragOff > pHeader->bTotalFrag)
	{
		MyShowDebugInfo(FLOWCTRL, "CMediaPacket::InsertPacket: Invalid packet Frag info\n");
		return m_bIsComplete;
	}

	m_dwRecvTicket = ticket;
	/*
	 *	Frame�յ��ĵ�һ����
	 */
	if (m_bTotalFrags == 0 && m_pMediaFragments == NULL)
	{
		/*
		 *	��ʼ��Frame��Ϣ
		 */
		m_bTotalFrags = pHeader->bTotalFrag;
		m_pMediaFragments = new CMediaFragment[m_bTotalFrags]; 
		m_usGroupSeq = pHeader->usGroupSeq;
		m_bMediaSeq = pHeader->bMediaSeq;
		m_dwFirstRecvTicket = ::timeGetTime();
		if (pHeader->usSeq >= pHeader->bFragOff)
			m_usStartSeq = pHeader->usSeq - pHeader->bFragOff;
		else
			m_usStartSeq = 65536-pHeader->bFragOff+pHeader->usSeq;
		
		/*
		 *	����ÿһƬ��Seq
		 */
		for (int i = 0; i < m_bTotalFrags; i++)
		{
			(m_pMediaFragments[i]).SetSeq(i+m_usStartSeq);
		}
	}

	/*
	 *	����Fragment
	 */
	m_pMediaFragments[pHeader->bFragOff].InsertPacket(pHeader, lpData,usSegLen, ticket);
	/*
	 *	�ж��Ƿ������
	 */
	for (int i = 0; i < m_bTotalFrags; i++)
	{
		if (!(m_pMediaFragments[i]).IsComplete())
		{
			m_bIsComplete = FALSE;
			return FALSE;
		}
	}
	m_bIsComplete = TRUE;
	return m_bIsComplete;
}

int CMediaPacket::GetPacket(BYTE *buffer, int len)
{
	int totallen = 0, i, remainlen;
	if (!m_bIsComplete)
		return 0;
	remainlen = len;
	/*
	 *	��ÿ����Ƭ������Buffer
	 */
	for (i = 0; i < m_bTotalFrags; i++)
	{
		if (remainlen <= 0)
		{
		/*
		 *	�쳣�����Buffer������
		 */
			MyShowDebugInfo(FLOWCTRL, "CMediaPacket::GetPacket: Buffer may too small to hold packet\n");
			return totallen;
		}
		totallen += m_pMediaFragments[i].GetPacket(buffer+totallen, remainlen);
		remainlen = len - totallen;
	}
	return totallen;
}

DWORD CMediaPacket::GetPacketTicket()
{
	return m_dwFirstRecvTicket;
}

BYTE CMediaPacket::GetMediaSeq()
{
	return m_bMediaSeq;
}

/*
 *	CGroupOfPicture
 */

CGroupOfPicture::CGroupOfPicture()
{
	m_dwRecvLen = 0;
	m_dwResendLen = 0;
	m_usGroupSeq = 0;
	m_usCompletePackets = 0;
	m_usUsefulPackets = 0;
	m_bIsFinish = 0;
	m_usUsedPackets = 0;
	m_nIndex = 0;
	m_dwFirstTicket = 0;
	m_dwTotalResendLen = 0;
}

CGroupOfPicture::~CGroupOfPicture()
{
}

void CGroupOfPicture::ReInit()
{
	int i;

	m_dwRecvLen = 0;
	m_dwResendLen = 0;
	m_usGroupSeq = 0;
	m_usCompletePackets = 0;
	m_usUsefulPackets = 0;
	m_bIsFinish = 0;
	m_usUsedPackets = 0;
	m_nIndex = 0;
	m_dwFirstTicket = 0;
	m_dwTotalResendLen = 0;

	for (i = 0; i < DEFAULT_GOP_SIZE; i++)
	{
		(m_aMediaPackets[i]).ReInit();
	}
}

void CGroupOfPicture::InsertPacket(FlowControlHeader *pHeader, BYTE* lpData, USHORT usSegLen, DWORD ticket)
{
	/*
	 *	��һ�����յİ�
	 */
	if (m_dwRecvLen == 0)
	{
		m_dwFirstTicket = ::timeGetTime();
		m_usGroupSeq = pHeader->usGroupSeq;
	}

	if (m_usCompletePackets == DEFAULT_GOP_SIZE)
	{
		return;
	}
		
	/*
	 *	�쳣�ж�
	 */
	if (pHeader->usGroupSeq != m_usGroupSeq)
	{
		MyShowDebugInfo(FLOWCTRL, "CGroupOfPicture::InsertPacket: Receive invalid packet\n");
		return;
	}

	/*
	 *	Sub seq ����
	 */
	if (pHeader->bMediaSeq >= DEFAULT_GOP_SIZE)
	{
		MyShowDebugInfo(FLOWCTRL, "CGroupOfPicture::InsertPacket: Invalid packet or gop size changed %d\n", pHeader->bMediaSeq);
		return;
	}
	m_dwRecvLen += usSegLen;

	/*
	 *	�ҵ���Ӧ��Frame������
	 */
	if (!(m_aMediaPackets[pHeader->bMediaSeq]).IsComplete())
	{
		if ((m_aMediaPackets[pHeader->bMediaSeq]).InsertPacket(pHeader, lpData, usSegLen, ticket))
		{
			m_usCompletePackets ++;
		}
	}

	/*
	 *	���еĶ��յ����������
	 */
	if (m_usCompletePackets == DEFAULT_GOP_SIZE)
	{
		m_bIsFinish = TRUE;
	}

}

void CGroupOfPicture::SendResendPacket(DWORD ticket, INetConnection *pSocket, DWORD dwIP, 
									   USHORT usPort, USHORT usLastestSeq, DWORD rtt)
{
	int rt;
	if (m_dwRecvLen == 0)
	{
		return;
	}
	/*
	 *	������I Frame
	 */
	if (!m_aMediaPackets[0].IsComplete())
	{
		rt =m_aMediaPackets[0].SendResendPacket(ticket, pSocket, dwIP, usPort,usLastestSeq, rtt);
		/*
		 *	I Frame �����ش������������治��Ҫ�ٷ���
		 */
		if (rt == -1)
		{
			return;
		}
		m_dwTotalResendLen += rt;
	}

	/*
	 *	�ж��ش����Ƿ��ѳ�������İٷ���
	 */
	if (m_dwResendLen * 100 /m_dwRecvLen > MAX_RESEND_PERCENT)
	{
		return;
	}

	/*
	 *	������Frame �����ش�
	 */
	for (int i = 1;  i < DEFAULT_GOP_SIZE-1; i++)
	{
		if (!m_aMediaPackets[i].IsComplete())
		{
			rt = m_aMediaPackets[i].SendResendPacket(ticket, pSocket, dwIP, usPort, usLastestSeq, rtt);

			/*
			 *	��ǰFrame �����ش�ʧ�ܣ������Frameû�б�Ҫ�ٷ����ش�
			 */
			if (rt = -1)
			{
				return;
			}
			m_dwResendLen += rt;
			m_dwTotalResendLen += rt;
			/*
			 *	�ش�̫��
			 */
			if (m_dwResendLen * 100 /m_dwRecvLen > MAX_RESEND_PERCENT)
			{
				return;
			} 
		}
	}
}

BOOL CGroupOfPicture::IsFinish()
{
	return m_bIsFinish;
}

void CGroupOfPicture::Finish()
{
	m_bIsFinish = 1;

}
/*
 *	��ȡ��ЧFrame����Ŀ
 */
int CGroupOfPicture::GetUsefulPackNum()
{
	for (int i = 0; i < DEFAULT_GOP_SIZE; i++)
	{
		if (!m_aMediaPackets[i].IsComplete())
		{
			break;
		}
	}
	m_usUsefulPackets = i;

	return m_usUsefulPackets;
}

/*
 *	ȡ����ǰ��ЧFrame
 */
int CGroupOfPicture::GetUsefulPack(BYTE *buffer, int len)
{
	GetUsefulPackNum();
	if (m_nIndex >= m_usUsefulPackets)
	{
		MyShowDebugInfo(FLOWCTRL, "CGroupOfPicture::GetUsefulPack: Not ready\n");
		return 0;
	}
	return m_aMediaPackets[m_nIndex++].GetPacket(buffer, len);
}

/*
 *	��ȡGroup�����һ������ʱ��
 */
DWORD CGroupOfPicture::GetGroupTicket()
{
	return m_dwFirstTicket;
}

/*
 *	��ȡ��ЧFrame�Ĵ���ʱ��
 */
DWORD CGroupOfPicture::GetUsefulPackTicket()
{
	GetUsefulPackNum();
	if (m_nIndex >= m_usUsefulPackets)
	{
		return 0;
	}
	return m_aMediaPackets[m_nIndex].GetPacketTicket();
}

BYTE CGroupOfPicture::GetUserfulMediaSeq()
{
	GetUsefulPackNum();
	if (m_nIndex >= m_usUsefulPackets)
	{
		return 0;
	}
	return m_aMediaPackets[m_nIndex].GetMediaSeq();
}

USHORT CGroupOfPicture::GetGroupSeq()
{
	return m_usGroupSeq;
}

/*
 *	CFlowControlConnection
 */

CFlowControlConnection::CFlowControlConnection(DWORD dwIP, USHORT usPort)
{

	if (dwIP == 0 || usPort == 0)
	{
		MyShowDebugInfo(FLOWCTRL, "CFlowControlConnection::CFlowControlConnection: Invalid param\n");
	}
	m_dwIP = dwIP;
	m_usPort = usPort;

	m_dwRttTicket = 0;
	m_dwIndTicket = INDICATE_MIN_TIME;
	m_dwStopTicket = RESEND_MIN_TIME;
	m_usCurPutO = 0;
	m_usCurPutN = 0;
	m_usCurInd = 0;
	m_dwRttSendTick = 0;
	m_dwTimerTick = 0;
	m_usLastestSeq = 0;
	m_dwRttReceived = 0;
	m_dwLastIndTick = 0;
	m_dwLastUserfulTick = 0;
	m_dwLastSeq = 0;
	m_dwInterval = 0;	
	m_dwIFrameTick = 0;
	m_byteParam = 1;
}

CFlowControlConnection::~CFlowControlConnection()
{
}
void CFlowControlConnection::ResetBuffer()
{
	for (int i = 0; i <MAX_GROUP_NUMBER ; i++)
	{
		m_cGroupOfPicture[i].ReInit();
	}
}

/*
 *	�������ݣ����Խ��յ���������ʱ�����������ط������ϴ������ݡ�
 *  ����������İ�����Indicate�������ݷ���lpData, ����ֵΪ���ݳ��ȡ�
 */
ULONG CFlowControlConnection::InsertPacket(INetConnection *pSocket, BYTE* lpData, USHORT usSegLen, 
				   LPTSTR lpszBuffer, ULONG uBufferSize)
{
	FlowControlHeader header;
	DWORD	dwCurrent = ::timeGetTime();
	int i;
	BOOL	bAdded;

	if (lpData == NULL || usSegLen == 0)
	{
		return OnFlowTimer(dwCurrent, pSocket, lpszBuffer, uBufferSize);
	}

	memcpy(&header, lpData, sizeof(header));
	header.usOffset = ntohs(header.usOffset);
	header.usSeq = ntohs(header.usSeq);
	header.usLen = ntohs(header.usLen);
	header.usGroupSeq = ntohs(header.usGroupSeq);
	MyShowDebugInfo(FLOWCTRL, "Get Packet seq %d\n", header.usSeq);
	bAdded = FALSE;
	/*
	 *	��һ��
	 */
	if (m_dwTimerTick == 0)
	{
		m_usLastestSeq = header.usSeq;
	}
	else
	{
		/*
		 *	���������յ���Seq
		 */
		if (UShortNotLessThan(header.usSeq, m_usLastestSeq))
		{
			m_usLastestSeq = header.usSeq;
		}
	}
	for (i = m_usCurInd; i != m_usCurPutN; i = ((i+1)%MAX_GROUP_NUMBER))
	{
		if ((m_cGroupOfPicture[i]).GetGroupSeq() == header.usGroupSeq)
		{
			(m_cGroupOfPicture[i]).InsertPacket(&header, 
				lpData+sizeof(FlowControlHeader), usSegLen-sizeof(FlowControlHeader), dwCurrent);
			bAdded = TRUE;
			break;
		}
	}

	if (!bAdded)
	{
		/*
		 *	���GroupSeq��С����Indicate������
		 */
		if (!UShortNotLessThan(m_cGroupOfPicture[m_usCurInd].GetGroupSeq(), header.usGroupSeq)
			|| m_cGroupOfPicture[m_usCurInd].GetGroupTicket() == 0)
		{
			m_cGroupOfPicture[m_usCurPutN].InsertPacket(&header, 
				lpData+sizeof(header), usSegLen-sizeof(header), dwCurrent);
			m_usCurPutN = (m_usCurPutN+1)%MAX_GROUP_NUMBER;
			if (m_usCurPutN == m_usCurInd)
			{
				MyShowDebugInfo(FLOWCTRL, "CFlowControlConnection::InsertPacket: Panic, ring buffer full\n");
				m_cGroupOfPicture[m_usCurInd].ReInit();
				m_usCurInd = (m_usCurInd+1)%MAX_GROUP_NUMBER;
			}
		}
	}

	/*
	 *	Timer
	 */
//	if (dwCurrent - m_dwTimerTick > 10)
	{
		m_dwTimerTick = dwCurrent;
		return OnFlowTimer(dwCurrent, pSocket, lpszBuffer, uBufferSize);
	}
	return 0;
}
/*
 *	����Indicate buffer�ĳ���
 */
int CFlowControlConnection::OnFlowTimer(DWORD dwCurrent, INetConnection *pSocket,
									 LPTSTR lpszBuffer, ULONG uBufferSize)
{
	if (m_dwRttReceived <= 5 && dwCurrent - m_dwRttSendTick > 1000)
	{
		m_dwRttSendTick = dwCurrent;
		SendRttEvalPacket(pSocket);
	}
	
	/*
	 *	Check GOP that need to stop resend.
	 *  Current PutO GOP has a subsequent GOP
	 */
	if (m_usCurPutN != m_usCurPutO && 
		(m_usCurPutO+1)%MAX_GROUP_NUMBER != m_usCurPutN)
	{
		DWORD dwNextTick = m_cGroupOfPicture[(m_usCurPutO+1)%MAX_GROUP_NUMBER].GetGroupTicket();
		if (dwCurrent - dwNextTick > m_dwStopTicket && dwNextTick != 0)
		{
			m_cGroupOfPicture[m_usCurPutO].Finish();
			m_usCurPutO = (m_usCurPutO+1)%MAX_GROUP_NUMBER;
		}
	}

	/*
	 *	Check resend
	 */
	for (int i = m_usCurPutO; i != m_usCurPutN; i = (i+1)%MAX_GROUP_NUMBER)
	{
		m_cGroupOfPicture[i].SendResendPacket(dwCurrent, pSocket, m_dwIP, m_usPort, 
			m_usLastestSeq, (m_dwRttTicket !=0)?m_dwRttTicket:40);
	}

	/*
	 *	Indicate packet
	 */
	DWORD dwUsefulTick = m_cGroupOfPicture[m_usCurInd].GetUsefulPackTicket();
	int	  ret = 0;
	/*
	 *	�����������GOP
	 */
	while (dwUsefulTick == 0)
	{
		if (m_usCurInd != m_usCurPutO)
		{
			/*
			 *	����ʹ�����GOP���³�ʼ��
			 */
			MyShowDebugInfo(FLOWCTRL, "CFlowControlConnection::OnFlowTimer: useful frame number is %d\n", 
				m_cGroupOfPicture[m_usCurInd].GetUsefulPackNum());
			m_cGroupOfPicture[m_usCurInd].ReInit();
			m_usCurInd = (m_usCurInd+1)%MAX_GROUP_NUMBER;
			dwUsefulTick = m_cGroupOfPicture[m_usCurInd].GetUsefulPackTicket();
		}
		else
		{
			break;
		}
	}
	int currentseq;
	currentseq = m_cGroupOfPicture[m_usCurInd].GetUserfulMediaSeq();
	
	if (dwUsefulTick != 0 ) 
//		&& dwCurrent - dwUsefulTick >= m_dwIndTicket)
//		&& ((dwCurrent - m_dwLastIndTick) >= (m_dwInterval/m_byteParam) || 
//		((dwCurrent - dwUsefulTick >= m_dwIndTicket)&&currentseq == 0)) )
	{

		/*
		 *	������Indicate
		 */
		MyShowDebugInfo(FLOWCTRL, "CFlowControlConnection::OnFlowTimer: Ticket is %d packet %d\n", dwCurrent, dwUsefulTick);
		
		ret = m_cGroupOfPicture[m_usCurInd].GetUsefulPack((BYTE*)lpszBuffer, uBufferSize);
	}
	return ret;
}

void CFlowControlConnection::SendRttEvalPacket(INetConnection *pSocket)
{
	EvalRtt rtt;

	rtt.bFragFlag = FF_FC_EVALRTT;
	rtt.dwTicket = htonl(::timeGetTime());
	pSocket->SendData((unsigned char*)&rtt, sizeof(rtt));
}

void CFlowControlConnection::OnReceiveRTTEval(void* lpData, USHORT usSegLen)
{
	EvalRtt rtt;
	DWORD	tmp;
	
	if (m_dwRttReceived <= 10)
	{
		m_dwRttReceived ++;
		memcpy(&rtt, lpData, sizeof(rtt));
		rtt.dwTicket = ntohl(rtt.dwTicket);
		/*
		 *	����RTTʱ��
		 */
		tmp = ::timeGetTime()-rtt.dwTicket;
		if (m_dwRttTicket != 0 && m_dwRttTicket < tmp)
		{
			return;
		}
		MyShowDebugInfo(FLOWCTRL, "RTT is %d\n", tmp);
		m_dwRttTicket = tmp;
		m_dwIndTicket = INDICATE_MIN_TIME;
		m_dwStopTicket = RESEND_MIN_TIME;

		if (m_dwRttTicket * INDICATE_RTT_NUMBER > INDICATE_MIN_TIME)
		{
			m_dwIndTicket = m_dwRttTicket * INDICATE_RTT_NUMBER;
			if (m_dwIndTicket > INDICATE_MAX_TIME)
				m_dwIndTicket = INDICATE_MAX_TIME;
		}
		
		if (m_dwRttTicket * RESEND_RTT_NUMBER > RESEND_MIN_TIME)
		{
			m_dwStopTicket = m_dwRttTicket * RESEND_RTT_NUMBER;
			if (m_dwStopTicket > RESEND_MAX_TIME)
				m_dwStopTicket = RESEND_MAX_TIME;
		}
	}
}

#endif
#ifdef EMBEDED_LINUX
static message_block_t *_g_message;;
void CFlowControlSend::FlowControlInit()
{
    int rt;
	locked = 0;
	current_seq = 0;
	send_remains = 0;
	put_point = send_point = end_point = 0;
	
	g_send_byte_rate = NETWORK_SEND_BITRATE_INFINITE;

	g_send_len =  g_resend_len = g_send_out_len = g_cal_len = 0;
	
    rt = pthread_mutex_init(&g_fc_lock, NULL);

    if (rt != 0)
    {
        printf("Flow Control Init: init lock failed\n");
    }
    GetTimeOfDay(&prev_send_time,NULL);
	prev_cal_time = prev_send_time;
    if (_g_message == NULL)
	_g_message = (message_block_t *)malloc(RING_BUF_SIZE*sizeof(message_block_t));
    ringbuf = _g_message;
    m_pTimer = new CNetTimer(this);
    m_pTimer->Schedule(15);
	
}

void CFlowControlSend::FlowControlFini()
{
    pthread_mutex_destroy(&g_fc_lock);
	if (m_pTimer)
	{
		m_pTimer->Cancel();
		delete m_pTimer;
		m_pTimer = NULL;
	}
    ringbuf = NULL;
}
void CFlowControlSend::FlowControlSetSendBPS(int bps)
{
    g_send_byte_rate = bps/8;
    FlowControlReinitialize();
    printf("bps is %d, byte_rate %d\n", bps, g_send_byte_rate);
}

int CFlowControlSend::TryLock()
{
    if (!locked)
    {
        if (pthread_mutex_trylock(&g_fc_lock) == 0)
        {
            locked = 1;
            return 0;
        }
    }
    return -1;
}
void CFlowControlSend::Lock()
{
    locked = 1;
    pthread_mutex_lock(&g_fc_lock);
}

void CFlowControlSend::UnLock()
{
    locked = 0;
    pthread_mutex_unlock(&g_fc_lock);
}

void CFlowControlSend::FlowControlReinitialize()
{
    printf("reinitialize\n");
    Lock();
    GetTimeOfDay(&prev_send_time,NULL);
    prev_cal_time = prev_send_time;
    send_remains = 0;
    put_point=send_point=end_point = 0;
    UnLock();
}
void CFlowControlSend::DropPacket()
{
	send_point = (send_point+32)%RING_BUF_SIZE;
}

void CFlowControlSend::FlowControlPutBuf(
			INetConnection *pCon, unsigned char *buf, unsigned long len, 
			struct timeval *sendtm,	unsigned char mseq,unsigned int totalfrag, 
			unsigned int currentfrag, unsigned short Iframeseq)
{
    FlowControlHeader *pHeader;

    g_send_len+= len;
    if (len > MESSAGE_BLOCK_SIZE)
    {
    	printf("FlowControlPutBuf: Buffer too big\n");
    }
    Lock();
    if ((put_point+1)%RING_BUF_SIZE == send_point)
    {
    	printf("FlowControlPutBuf: drop packets\n");
    	DropPacket();
    }
    
    if ((put_point+1)%RING_BUF_SIZE == end_point)
    {
    	end_point = (end_point+32)%RING_BUF_SIZE;
    }
    ringbuf[put_point].seq = current_seq++;
    ringbuf[put_point].pCon = pCon;
    ringbuf[put_point].addtime = *sendtm;
    ringbuf[put_point].len = len;
    ringbuf[put_point].remainlen = len;

    memset(ringbuf[put_point].buf, 0, 4+MESSAGE_BLOCK_SIZE);
    memcpy(ringbuf[put_point].buf+4+8/*NETWORK_HEADER_SIZE*/, buf, len);
    pHeader = (FlowControlHeader *)ringbuf[put_point].buf;
    
    pHeader->usSeq = htons(ringbuf[put_point].seq);
    pHeader->usLen = htons(ringbuf[put_point].len);
    if (mseq >= 15)
	printf("Invalid sub sequece %d\n", mseq);
    pHeader->bMediaSeq = mseq;
    pHeader->bTotalFrag = totalfrag;
    pHeader->bFragOff = currentfrag;
    pHeader->usGroupSeq = Iframeseq;
    pHeader->bFragFlag = FF_FC_MEDIA;
    
    put_point = (put_point+1)%RING_BUF_SIZE;
    UnLock();
} 

void CFlowControlSend::SendOutPacket(int size)
{
    FlowControlHeader *pHeader;
    
    pHeader = (FlowControlHeader *)ringbuf[send_point].buf;
    
    pHeader->usOffset = htons(ringbuf[send_point].len-ringbuf[send_point].remainlen);

 
    if (ringbuf[send_point].remainlen == ringbuf[send_point].len)
    {
	ringbuf[send_point].pCon->SendData(ringbuf[send_point].buf, 
		size+sizeof(FlowControlHeader)); 
    }
    else
    {
    	memcpy(m_sendbuf, pHeader, sizeof(*pHeader));
    	memcpy(m_sendbuf+sizeof(*pHeader), ringbuf[send_point].buf+sizeof(*pHeader)+
		ringbuf[send_point].len-ringbuf[send_point].remainlen, size);
    	ringbuf[send_point].pCon->SendData( m_sendbuf, size+sizeof(FlowControlHeader));
    }
    
    if (size >= ringbuf[send_point].remainlen)
    {
    	send_point = (send_point+1)%RING_BUF_SIZE;
    }
    else
    {
    	ringbuf[send_point].remainlen = ringbuf[send_point].remainlen - size;
    }
}

void CFlowControlSend::FlowControlSend()
{
    struct timeval current;
    int remains;
    long timediff, udiff, sendcount, addlen = sizeof( struct sockaddr_in );
    
    if (TryLock() != 0)
    	return;
    	
    GetTimeOfDay(&current, NULL);

    if (send_point == put_point)
    {
    	prev_send_time = current;
    	UnLock();
    	return;
    }
    
    udiff = 1000000*(current.tv_sec-prev_send_time.tv_sec)+(current.tv_usec-prev_send_time.tv_usec);

    timediff = udiff/1000;
    udiff = udiff%1000;

    remains = 16*(timediff*g_send_byte_rate/1000
		   +udiff*g_send_byte_rate/1000000)/10+send_remains;

    if (remains < 0)
    {
	g_cal_len += remains - send_remains;
	send_remains = remains;
        prev_send_time = current;
	UnLock();
	return;
    }
    if (remains < ringbuf[send_point].remainlen
	 &&(timediff < 40 
/*
	|| remains < 200 || 
    	ringbuf[send_point].remainlen - remains < 200*/))
    {
    	UnLock();
    	return;
    }
    
    g_cal_len += remains - send_remains;
    send_remains = remains;
    prev_send_time = current;

    
    timediff = (1000000*(current.tv_sec-ringbuf[send_point].addtime.tv_sec)+(current.tv_usec-ringbuf[send_point].addtime.tv_usec))/1000;
    if(timediff > 1000)
    {
	static int printcount;
	if (++printcount % 40 == 0)
		printf("Delay %d ms\n", timediff);
    }
    if (send_remains < ringbuf[send_point].remainlen)
    {
	g_send_out_len += send_remains;
	SendOutPacket(send_remains);
	send_remains = 0;
    }
    
    sendcount = 0;
    while (send_remains > ringbuf[send_point].remainlen)
    {
    	if (send_point == put_point)
    	{
    		send_remains = 0;
    		break;
    	}
	g_send_out_len += ringbuf[send_point].remainlen;
    	send_remains = send_remains-ringbuf[send_point].remainlen;
	SendOutPacket(ringbuf[send_point].remainlen);
	sendcount++;
    }
    if (sendcount > 10)
    {
	printf("send too many packets\n");
    }
    
    UnLock();
}

void CFlowControlSend::TimerFlowControlSend()
{
    FlowControlSend();
}

void CFlowControlSend::OnReceiveResend(char *buf)
{
    ResendInfo Info, *pInfo;
    unsigned short usResendSeq, usResendOff, usResendLen;
    unsigned long offset, index;
    FlowControlHeader *pHeader;
        
    Lock();
    pInfo = &Info;
    memcpy(pInfo, buf, sizeof(Info));
    
    usResendSeq = ntohs(pInfo->usResendSeq);
    usResendOff = ntohs(pInfo->usResendOff);
    usResendLen = ntohs(pInfo->usResendLen);
  
    if (usResendSeq < ringbuf[end_point].seq)
    	offset = SEQ_SIZE - ringbuf[end_point].seq + usResendSeq;
    else
    	offset = usResendSeq - ringbuf[end_point].seq;
    
    index = ( end_point + offset ) % RING_BUF_SIZE;
    if (ringbuf[index].seq != usResendSeq)
    {
    	printf("Pakcet request Resend isn't in buffer\n");
    	UnLock();
    	return;
    }
   
    if (usResendLen > ringbuf[index].len - usResendOff)
    	usResendLen = ringbuf[index].len - usResendOff;
    	
    pHeader = (FlowControlHeader *)ringbuf[index].buf;
    
    pHeader->usOffset = htons(usResendOff);
/*   
    send_remains = send_remains - usResendLen;
*/
    g_resend_len += usResendLen;
    if (usResendOff == 0)
    {
	ringbuf[index].pCon->SendData(ringbuf[index].buf, 
		usResendLen+sizeof(FlowControlHeader)); 
    }
    else
    {
    	memcpy(m_sendbuf, pHeader, sizeof(*pHeader));
    	memcpy(m_sendbuf+sizeof(*pHeader), ringbuf[index].buf+sizeof(*pHeader)+
    			usResendOff, usResendLen);
    	ringbuf[index].pCon->SendData(m_sendbuf,usResendLen+sizeof(FlowControlHeader)); 
    }
    UnLock();
    return;
}

#endif
