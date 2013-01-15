/*------------------------------------------------------*/
/* Common base                                          */
/*                                                      */
/* CmBase.h                                             */
/*                                                      */
/* History                                              */
/*                                                      */
/*	11/24/2003	Create                                  */
/*                                                      */
/*------------------------------------------------------*/

#ifndef CMBASE_H
#define CMBASE_H

#include "utilbase.h"
#include "networkbase.h"

#ifdef WIN32
 #define CM_OS_SEPARATE '\\'
#elif defined UNIX 
 #define CM_OS_SEPARATE '/'
#endif

# define CM_BIT_ENABLED(dword, bit) (((dword) & (bit)) != 0)
# define CM_BIT_DISABLED(dword, bit) (((dword) & (bit)) == 0)
# define CM_BIT_CMP_MASK(dword, bit, mask) (((dword) & (bit)) == mask)
# define CM_SET_BITS(dword, bits) (dword |= (bits))
# define CM_CLR_BITS(dword, bits) (dword &= ~(bits))


//////////////////////////////////////////////////////////////////////
// C definition
//////////////////////////////////////////////////////////////////////

#ifdef WIN32
 #include <crtdbg.h>
 #ifdef _DEBUG
//  #define CM_DEBUG
 #endif // _DEBUG

/*
 #ifdef CM_DEBUG
  #define CM_LOG CTraceWin32Debug::Output
  #define CM_ASSERTE _ASSERTE
 #else // !CM_DEBUG
  #define CM_LOG CTraceLog2File::Output
 #endif // CM_DEBUG

 #define vsnprintf _vsnprintf
*/
#endif // WIN32

#ifdef UNIX
 #include <assert.h>
/*
 #ifdef CM_DEBUG
  #define CM_LOG CTraceLog2File::Output
  #define CM_ASSERTE assert
 #else // !CM_DEBUG
  #define CM_LOG CTraceLog2File::Output
 #endif // CM_DEBUG
*/
#endif // UNIX
#define CM_ASSERTE(expr) 

#define CM_ASSERTE_RETURN(expr, rv) \

// to use T120 trace


#define CM_ERROR_LOG(X) 
#define CM_WARNING_LOG(X) 
#define CM_INFO_LOG(X) 

#ifdef WIN32
 #define CmTraceDebug CmTraceDebugWin32;
#else // !WIN32
 #define CmTraceDebug /*CmTraceCout;*/
#endif // WIN32

 void CmTraceDebugWin32(LPCSTR aFormat, ...);
 void CmTraceLog2File(LPCSTR aFormat, ...);
 void CmTraceCout(LPCSTR aFormat, ...);

class  CPrintfFormat
{
public:
	CPrintfFormat();
	int BeforeFormat(int aMask);
	int PrintfMessage(LPCSTR aFormat, ...);
	int PrintfMessageVa(LPCSTR aFormat, va_list aArgs);

	const char* GetString() { return m_pszMsg; };

private:
	char m_pszMsg[512];
	int m_nStart;
};

#ifdef WIN32
class  CTraceWin32Debug
{
public:
	static void Output(LPCSTR aMsg)
	{
		::OutputDebugStringA(aMsg);
	}
};
#endif // !WIN32


class  CTraceLog2File
{
public:
	static void Output(LPCSTR aMsg);

	static FILE *s_fp;
};

class  CErrnoGuard
{
public:
	CErrnoGuard() : m_nErr(errno)
	{
	}

	~CErrnoGuard()
	{
		errno = m_nErr;
	}
private:
	int m_nErr;
};

//////////////////////////////////////////////////////////////////////
// C++(standard) definition
//////////////////////////////////////////////////////////////////////

#include <algorithm>
//#include <stdexcept>

#ifdef _MSC_VER
 #define min _cpp_min
 #define max _cpp_max
#endif // _MSC_VER

// we have to not use std::string to avoid loading MSVCP60.dll
#ifdef WIN32
 #include "wuohString.h"
 typedef flex_string<char, CWuoHTraits<char>, std::allocator<char>, 
	 CCowNewStringStorage<char, std::allocator<char> > > CCmString;
#else // !WIN32
// #include <string>
// typedef std::string CCmString;
#endif // WIN32

#endif // !CMBASE_H

