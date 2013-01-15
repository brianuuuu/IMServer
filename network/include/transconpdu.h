#ifndef __TRANS_CON_PDU_H
#define __TRANS_CON_PDU_H
#include "utilbase.h"

const WORD TransCon_Pdu_Type_Invalid			= 0x0;
const WORD TransCon_Pdu_Type_TCP_Keepalive		= 0x1;
const WORD TransCon_Pdu_Type_TCP_Data			= 0x2;
const WORD TransCon_Pdu_Type_UDP_Syn			= 0x3;
const WORD TransCon_Pdu_Type_UDP_Ack			= 0x4;
const WORD TransCon_Pdu_Type_UDP_FIN			= 0x5;
const WORD TransCon_Pdu_Type_UDP_Keepalive		= 0x6;
const WORD TransCon_Pdu_Type_UDP_Data			= 0x7;
const WORD TransCon_Pdu_Type_UDP_Syn_0			= 0x8;
const WORD TransCon_Pdu_Type_UDP_Ack1			= 0x9;

class CTransConPduBase
{
public:
	CTransConPduBase(BYTE byType = TransCon_Pdu_Type_Invalid, BYTE byVersion = 2);
	virtual ~CTransConPduBase();
	
	virtual BOOL Decode (CByteStream& is);
	virtual BOOL Encode (CByteStream& os);
	virtual DWORD GetLen(void);

	BYTE GetType(void);
	BYTE GetVersion(void);
	
public:
	static BYTE PeekType (BYTE *pData);
private:
	BYTE	m_byType;
	BYTE	m_byVersion;
};

class CTransConPduTcpKeepAlive :public CTransConPduBase
{
public:
	CTransConPduTcpKeepAlive(WORD wLen = 0, 
		BYTE byType = TransCon_Pdu_Type_TCP_Keepalive, 
		BYTE byVersion = 2);
	virtual ~CTransConPduTcpKeepAlive();

	BOOL Decode (CByteStream& is);
	BOOL Encode (CByteStream& os);
	DWORD GetLen(void);

};

class CTransConPduTcpData :public CTransConPduBase
{
public:
	CTransConPduTcpData(WORD wLen = 0, BYTE *pData = NULL,
		BYTE byType = TransCon_Pdu_Type_TCP_Data, 
		BYTE byVersion = 2);
	virtual ~CTransConPduTcpData();

	BOOL Decode (CByteStream& is);
	BOOL Encode (CByteStream& os);
	DWORD GetLen(void);

	WORD GetContLen(void);
private:
	WORD m_wLength;
	BYTE *m_pData;
};

class CTransConPduUdpBase :public CTransConPduBase
{
public:
	CTransConPduUdpBase(DWORD nConId = 0, WORD nSubConId = 0, WORD wSeq = 0, 
		BYTE byType = TransCon_Pdu_Type_Invalid, 
		BYTE byVersion = 2);
	virtual ~CTransConPduUdpBase();

	BOOL Decode (CByteStream& is);
	virtual BOOL Encode (CByteStream& os);
	DWORD GetLen(void);

	WORD GetSequence(void);
	DWORD GetConnectionId();
	WORD GetSubConId(void);
private:
	WORD m_nSequence;
	DWORD m_nConnectionId;
	WORD m_nSubConId;
};

class CTransConPduUdpData:public CTransConPduUdpBase
{
public:
	CTransConPduUdpData(DWORD nConId = 0, WORD nSubConId = 0, WORD wSeq = 0, BYTE *pData = NULL, DWORD dwDataLen = 0, 
		BYTE byType = TransCon_Pdu_Type_UDP_Data, 
		BYTE byVersion = 2);
	virtual ~CTransConPduUdpData();

	BOOL Encode (CByteStream& os);

private:
	BYTE		*m_pData;
	DWORD		m_dwDataLen;
};

#endif //__TRANS_CON_PDU_H

