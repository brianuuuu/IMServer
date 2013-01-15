/*------------------------------------------------------------------------*/
/*                                                                        */
/*  T120 implementation                                                   */
/*  T120BASE.H                                                            */
/*                                                                        */
/*  t120 base definitions                                                 */
/*                                                                        */
/*  All rights reserved                                                   */
/*                                                                        */
/*------------------------------------------------------------------------*/

#ifndef __T120BS_H__
#define __T120BS_H__

#include "platform.h"

/////////////////////////////////////////////////////////////////////////////
//  T120_Byte_Stream
//
#ifdef MACOS
class T120_Byte_Stream
#else
class  T120_Byte_Stream
#endif
{
public :
	T120_Byte_Stream(uint8* buf = NULL, uint32 offset = 0, uint32 buf_size = 0);
	virtual ~T120_Byte_Stream();

	void attach(uint8* buf, uint32 offset, uint32 buf_size);
	uint32 seek(uint32 pos);
	uint32 skip(int32 dis);
	uint32 tell();
	uint8* get_data();
	void write(void* data, uint32 length);
	void read(void* data, uint32 length);
	T120_Byte_Stream& operator << (uint8 ch);
	T120_Byte_Stream& operator << (char ch);
	T120_Byte_Stream& operator << (int16 s);
	T120_Byte_Stream& operator << (uint16 s);
	T120_Byte_Stream& operator << (int32 l);
	T120_Byte_Stream& operator << (int l);
	T120_Byte_Stream& operator << (uint32 l);
	T120_Byte_Stream& operator << (float32 f);
	T120_Byte_Stream& operator << (float64 d);
//	T120_Byte_Stream& operator << (char* str);
//	T120_Byte_Stream& operator << (const char str[]);
	T120_Byte_Stream& operator << (const char *str);
	T120_Byte_Stream& operator >> (uint8& ch);
	T120_Byte_Stream& operator >> (char& ch);
	T120_Byte_Stream& operator >> (int16& s);
	T120_Byte_Stream& operator >> (uint16& s);
	T120_Byte_Stream& operator >> (int32& l);
	T120_Byte_Stream& operator >> (int& l);
	T120_Byte_Stream& operator >> (uint32& l);
	T120_Byte_Stream& operator >> (float32& f);
	T120_Byte_Stream& operator >> (float64& d);
	T120_Byte_Stream& operator >> (char*& str);
	char* read_string(uint16 max_len);
	void read_string(char* str, uint16 max_len);

	static void swap(void* data, int length);
	static void init();

	boolean IsGood() { return m_nState == 0 ? TRUE : FALSE; }

protected :
	uint8* buf;
	uint32 cur_pos;
	uint32 buf_size;
	int m_nState;
	static boolean s_is_little_endian;
};

#endif // __T120BS_H__

