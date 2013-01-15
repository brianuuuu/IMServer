/*------------------------------------------------------------------------*/
/*                                                                        */
/*  transport connection pdu			     											  */
/*  transconpdu.cpp  													  */
/*                                                                        */
/*  Implementation of IM transport connection pdu										  */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
#include "CmBase.h"
#include "transconpdu.h"

/*########################################################################*/
/* CTransConPduBase*/
CTransConPduBase::CTransConPduBase(BYTE byType, BYTE byVersion /* = 1 */)
{
	m_byType = byType;
	m_byVersion = byVersion;
}

CTransConPduBase::~CTransConPduBase()
{
}

BOOL CTransConPduBase::Decode (CByteStream& is)
{
	is>>m_byVersion;
	is>>m_byType;
	
	return TRUE;
}

BOOL CTransConPduBase::Encode (CByteStream& os)
{
	os<<m_byVersion;
	os<<m_byType;

	return TRUE;
}

BYTE CTransConPduBase::GetType(void)
{
	return m_byType;
}

BYTE CTransConPduBase::GetVersion(void)
{
	return m_byVersion;
}

DWORD CTransConPduBase::GetLen(void)
{
	return 2*sizeof(BYTE);
}

BYTE CTransConPduBase::PeekType (BYTE *pData)
{
	return pData[1];
}

/*########################################################################*/
/* CTransConPduTcpKeepAlive*/
CTransConPduTcpKeepAlive::CTransConPduTcpKeepAlive(WORD wLen, BYTE byType, 
				BYTE byVersion ):CTransConPduBase(byType, byVersion)
{
}

CTransConPduTcpKeepAlive::~CTransConPduTcpKeepAlive()
{
}

BOOL CTransConPduTcpKeepAlive::Decode(CByteStream& is)
{
	return CTransConPduBase::Decode(is);
}

BOOL CTransConPduTcpKeepAlive::Encode(CByteStream& os)
{
	return CTransConPduBase::Encode(os);
}

DWORD CTransConPduTcpKeepAlive::GetLen(void)
{
	return CTransConPduBase::GetLen();
}

/*########################################################################*/
/* CTransConPduTcpData*/
CTransConPduTcpData::CTransConPduTcpData(WORD wLen /* = 0 */, 
										 BYTE *pData /* = NULL */, 
										 BYTE byType /* = TransCon_Pdu_Type_TCP_Data */, 
										 BYTE byVersion /* = 1 */)
										 :CTransConPduBase(byType, byVersion)
{
	m_wLength = wLen;
	m_pData = pData;
}

CTransConPduTcpData::~CTransConPduTcpData()
{
}

BOOL CTransConPduTcpData::Decode(CByteStream& is)
{
	CTransConPduBase::Decode(is);

	is>>m_wLength;
	
	return TRUE;
}

BOOL CTransConPduTcpData::Encode(CByteStream& os)
{
	CTransConPduBase::Encode(os);

	os<<m_wLength;
	if (m_pData)
		os.write(m_pData, (DWORD)m_wLength);
	
	return TRUE;
}

DWORD CTransConPduTcpData::GetLen()
{
	return sizeof(WORD)+CTransConPduBase::GetLen();
}

WORD CTransConPduTcpData::GetContLen()
{
	return m_wLength;
}

/*########################################################################*/
/* CTransConPduUdpBase*/
CTransConPduUdpBase::CTransConPduUdpBase(DWORD nConId, WORD nSubConId, WORD wSeq, BYTE byType, 
										 BYTE byVersion):CTransConPduBase(byType, byVersion)
{
	m_nSequence = wSeq;
	m_nConnectionId = nConId;
	m_nSubConId = nSubConId;
}

CTransConPduUdpBase::~CTransConPduUdpBase()
{
}

BOOL CTransConPduUdpBase::Decode(CByteStream& is)
{
	CTransConPduBase::Decode(is);
	
	is>>m_nSequence;
	is>>m_nConnectionId;
	is>>m_nSubConId;
	return TRUE;
}

BOOL CTransConPduUdpBase::Encode(CByteStream& os)
{
	CTransConPduBase::Encode(os);
	
	os<<m_nSequence;
	os<<m_nConnectionId;
	os<<m_nSubConId;
	return TRUE;
}

DWORD CTransConPduUdpBase::GetLen(void)
{
	return sizeof(DWORD)+2*sizeof(WORD)+CTransConPduBase::GetLen();
}

WORD CTransConPduUdpBase::GetSequence()
{
	return m_nSequence;
}

DWORD CTransConPduUdpBase::GetConnectionId()
{
	return m_nConnectionId;
}

WORD CTransConPduUdpBase::GetSubConId()
{
	return m_nSubConId;
}

/*########################################################################*/
/* CTransConPduUdpData*/
CTransConPduUdpData::CTransConPduUdpData(
					DWORD nConId, 
					WORD nSubConId, 
					WORD wSeq, 
					BYTE *pData /* = NULL */, 
					DWORD dwDataLen,
					BYTE byType /* = TransCon_Pdu_Type_UDP_Data */, 
					BYTE byVersion /* = 1 */):CTransConPduUdpBase(nConId, nSubConId,wSeq, byType, byVersion)
{
	m_pData = pData;
	m_dwDataLen = dwDataLen;
}

CTransConPduUdpData::~CTransConPduUdpData()
{
}

BOOL CTransConPduUdpData::Encode(CByteStream& os)
{
	CTransConPduUdpBase::Encode(os);
	
	if (m_pData != NULL)
		os.write(m_pData, m_dwDataLen);
	
	return TRUE;
}

