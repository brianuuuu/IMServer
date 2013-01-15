/*-------------------------------------------------------------------------*/
/*                                                                         */
/* t120bs.CPP                                                              */
/*                                                                         */
/* Copyright (C) ActiveTouch Inc.                                          */
/* All rights reserved                                                     */
/*                                                                         */
/*-------------------------------------------------------------------------*/
#include "t120base.h"
#include "t120bs.h"

/////////////////////////////////////////////////////////////////////////////
//  T120_Byte_Stream
//
boolean T120_Byte_Stream::s_is_little_endian = FALSE;


#define BS_OVERFLOW_CHECK \
if(buf_size>0) \
{ \
		T120_ASSERTX(cur_pos<=buf_size); \
}

/// budingc modify it to adapt IsGood()
#if 0
#define BS_PRE_OVERFLOW_CHECK(x) \
if(buf_size>0) \
{ \
		T120_ASSERTX(cur_pos+x<=buf_size); \
}
#else

#define BS_PRE_OVERFLOW_CHECK_RETURE(x, r) \
	if (m_nState != 0 || (buf_size > 0 && cur_pos+x > buf_size)) { \
		WARNINGTRACE("T120_Byte_Stream::BS_PRE_OVERFLOW_CHECK_RETURE, state=" << m_nState \
			<< " cur_pos=" << cur_pos << " x=" << x << " buf_size=" << buf_size); \
		m_nState = -1; \
		return r; \
	}

#define BS_PRE_OVERFLOW_CHECK_RETURE_VOID(x) \
	if (m_nState != 0 || (buf_size > 0 && cur_pos+x > buf_size)) { \
		WARNINGTRACE("T120_Byte_Stream::BS_PRE_OVERFLOW_CHECK_RETURE_VOID, state=" << m_nState \
			<< " cur_pos=" << cur_pos << " x=" << x << " buf_size=" << buf_size); \
		m_nState = -1; \
		return ; \
	}

#define BS_CHECK_STATE_RETURE(r) \
	if (m_nState != 0) \
		return r;

#define BS_CHECK_STATE_RETURN_VOID() \
	if (m_nState != 0) \
		return ;

#endif


T120_Byte_Stream::T120_Byte_Stream(uint8* buf, uint32 offset, uint32 buf_size)
	: m_nState(0)
{
	this->buf = buf;
	this->cur_pos = offset;
	this->buf_size = buf_size;
}

T120_Byte_Stream::~T120_Byte_Stream()
{
}

void T120_Byte_Stream::attach(uint8* buf, uint32 offset, uint32 buf_size)
{
	this->buf = buf;
	this->cur_pos = offset;
	this->buf_size = buf_size;
}

uint32 T120_Byte_Stream::seek(uint32 pos)
{
	if (m_nState != 0 || pos > buf_size) {
		WARNINGTRACE("T120_Byte_Stream::seek, state=" << m_nState
			<< " pos=" << pos << " buf_size=" << buf_size);
		m_nState = -1;
		return cur_pos;
	}

	cur_pos = pos;
	BS_OVERFLOW_CHECK
	return pos;
}

uint32 T120_Byte_Stream::skip(int32 dis)
{
	BS_PRE_OVERFLOW_CHECK_RETURE(dis, cur_pos)

	cur_pos += dis;
	BS_OVERFLOW_CHECK
	return cur_pos;
}

uint32 T120_Byte_Stream::tell()
{
	return this->cur_pos;
}

uint8* T120_Byte_Stream::get_data()
{
	return this->buf;
}

void T120_Byte_Stream::write(void* data, uint32 length)
{
	if(length == 0) return;
	BS_PRE_OVERFLOW_CHECK_RETURE_VOID(length)

	memmove((uint8*)this->buf + this->cur_pos, (uint8*)data, length);
	cur_pos += length;
}

void T120_Byte_Stream::read(void* data, uint32 length)
{
	if(length == 0) return;
	BS_PRE_OVERFLOW_CHECK_RETURE_VOID(length)

	memcpy((uint8*)data, this->buf + this->cur_pos, length);
	this->cur_pos += length;
}

T120_Byte_Stream& T120_Byte_Stream::operator << (uint8 ch)
{
	write(&ch, sizeof(uint8));
	return *this;
}

T120_Byte_Stream& T120_Byte_Stream::operator << (char ch)
{
	write(&ch, sizeof(char));
	return *this;
}

T120_Byte_Stream& T120_Byte_Stream::operator << (int16 s)
{
//	BS_PRE_OVERFLOW_CHECK_RETURE(sizeof(int16), *this)

	swap(&s, sizeof(int16));
	write(&s, sizeof(int16));
	return *this;
}

T120_Byte_Stream& T120_Byte_Stream::operator << (uint16 s)
{
//	BS_PRE_OVERFLOW_CHECK_RETURE(sizeof(uint16), *this)
		
	swap(&s, sizeof(uint16));
	write(&s, sizeof(uint16));
	return *this;
}

T120_Byte_Stream& T120_Byte_Stream::operator << (int32 l)
{
//	BS_PRE_OVERFLOW_CHECK_RETURE(sizeof(int32), *this)

	swap(&l, sizeof(int32));
	write(&l, sizeof(int32));
	return *this;
}

T120_Byte_Stream& T120_Byte_Stream::operator << (int l)
{
//	BS_PRE_OVERFLOW_CHECK_RETURE(sizeof(int), *this)

	swap(&l, sizeof(int));
	write(&l, sizeof(int));
	return *this;
}

T120_Byte_Stream& T120_Byte_Stream::operator << (uint32 l)
{
//	BS_PRE_OVERFLOW_CHECK_RETURE(sizeof(uint32), *this)

	swap(&l, sizeof(uint32));
	write(&l, sizeof(uint32));
	return *this;
}

T120_Byte_Stream& T120_Byte_Stream::operator << (float32 f)
{
//	BS_PRE_OVERFLOW_CHECK_RETURE(sizeof(float32), *this)

	swap(&f, sizeof(float32));
	write(&f, sizeof(float32));
	return *this;
}

T120_Byte_Stream& T120_Byte_Stream::operator << (float64 d)
{
//	BS_PRE_OVERFLOW_CHECK_RETURE(sizeof(float64), *this)

	swap(&d, sizeof(float64));
	write(&d, sizeof(float64));
	return *this;
}


//T120_Byte_Stream& T120_Byte_Stream::operator << (const char str[])
T120_Byte_Stream& T120_Byte_Stream::operator << (const char *str)
{
	if(str)
	{
		uint16 len = strlen(str);
		(*this) << len;
		
		BS_CHECK_STATE_RETURE(*this)

		if(len > 0)
			write((void*)str, len);
	}
	else
	{
		uint16 len = 0;
		(*this) << len;
	}

	return *this;
}

T120_Byte_Stream& T120_Byte_Stream::operator >> (uint8& ch)
{
	BS_PRE_OVERFLOW_CHECK_RETURE(1, *this)

	ch = this->buf[cur_pos++];
	BS_OVERFLOW_CHECK

	return *this;
}

T120_Byte_Stream& T120_Byte_Stream::operator >> (char& ch)
{
	BS_PRE_OVERFLOW_CHECK_RETURE(1, *this)

	ch = (char)this->buf[cur_pos++];
	BS_OVERFLOW_CHECK

	return *this;
}

T120_Byte_Stream& T120_Byte_Stream::operator >> (int16& s)
{
//	BS_PRE_OVERFLOW_CHECK_RETURE(sizeof(int16), *this)

	read(&s, sizeof(int16));
	swap(&s, sizeof(int16));
	return *this;
}

T120_Byte_Stream& T120_Byte_Stream::operator >> (uint16& s)
{
//	BS_PRE_OVERFLOW_CHECK_RETURE(sizeof(uint16), *this)

	read(&s, sizeof(uint16));
	swap(&s, sizeof(uint16));
	return *this;
}

T120_Byte_Stream& T120_Byte_Stream::operator >> (int32& l)
{
//	BS_PRE_OVERFLOW_CHECK_RETURE(sizeof(int32), *this)

	read(&l, sizeof(int32));
	swap(&l, sizeof(int32));
	return *this;
}

T120_Byte_Stream& T120_Byte_Stream::operator >> (int& l)
{
//	BS_PRE_OVERFLOW_CHECK_RETURE(sizeof(int), *this)

	read(&l, sizeof(int));
	swap(&l, sizeof(int));
	return *this;
}

T120_Byte_Stream& T120_Byte_Stream::operator >> (uint32& l)
{
//	BS_PRE_OVERFLOW_CHECK_RETURE(sizeof(uint32), *this)

	read(&l, sizeof(uint32));
	swap(&l, sizeof(uint32));
	return *this;
}

T120_Byte_Stream& T120_Byte_Stream::operator >> (float32& f)
{
//	BS_PRE_OVERFLOW_CHECK_RETURE(sizeof(float32), *this)

	read(&f, sizeof(float32));
	swap(&f, sizeof(float32));
	return *this;
}

T120_Byte_Stream& T120_Byte_Stream::operator >> (float64& d)
{
//	BS_PRE_OVERFLOW_CHECK_RETURE(sizeof(float64), *this)

	read(&d, sizeof(float64));
	swap(&d, sizeof(float64));
	return *this;
}

T120_Byte_Stream& T120_Byte_Stream::operator >> (char*& str)
{
//	T120_ASSERT(FALSE);
	uint16 len = 0;
	(*this) >> len;

	BS_CHECK_STATE_RETURE(*this)

	if(len > 0)
	{
		if(str)
		{
			read((void*)str, len);

			BS_CHECK_STATE_RETURE(*this)

			str[len] = 0;
		}
		else
		{
			str = new char[len + 1];
			T120_ASSERTX(str);
			read((void*)str, len);

			BS_CHECK_STATE_RETURE(*this)

			str[len] = 0;
		}
	}
	else
	{
		if(str)
			str[0] = 0;
	}

	return *this;
}

char* T120_Byte_Stream::read_string(uint16 max_len)
{
	// no need to check it
//	BS_PRE_OVERFLOW_CHECK_RETURE(max_len, NULL)

	char* str = NULL;
	uint16 len = 0;
	(*this) >> len;

	BS_CHECK_STATE_RETURE(NULL)
	T120_ASSERTX(len<max_len);

	if(len > 0)
	{
		str = new char[len + 1];
		T120_ASSERTX(str);
		read((void*)str, len);

		// avoid memery leak
//		BS_CHECK_STATE_RETURE(NULL)
		if (m_nState != 0) {
			delete str;
			return NULL;
		}
		
		str[len] = 0;
	}

	return str;
}

void T120_Byte_Stream::read_string(char* str, uint16 max_len)
{
	// no need to check it
//	BS_PRE_OVERFLOW_CHECK_RETURE_VOID(max_len)

	T120_ASSERTX(str);
	str[0] = 0;

	uint16 len = 0;
	(*this) >> len;

	BS_CHECK_STATE_RETURN_VOID()
	T120_ASSERTX(len<max_len);

	if(len > 0)
	{
		read((void*)str, len);
		BS_CHECK_STATE_RETURN_VOID()
		str[len] = 0;
	}
}

boolean g_need_init = TRUE;
void T120_Byte_Stream::swap(void* data, int length)
{
	if(g_need_init)
	{
		init();
	}

	if(s_is_little_endian)
	{
		uint8* pch = (uint8*)data;

		for(int i = 0; i < length / 2; i++)
		{
			uint8 ch_temp;
			ch_temp = pch[i];
			pch[i] = pch[length-1-i];
			pch[length-1-i] = ch_temp;
		}
	}
}

void T120_Byte_Stream::init()
{
	g_need_init = FALSE;

	uint16 test = 0x55aa;

	if(*(uint8*)&test == 0xaa)
		s_is_little_endian = TRUE;
	else
		s_is_little_endian = FALSE;

}

