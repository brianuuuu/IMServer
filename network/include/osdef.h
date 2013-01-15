
#ifndef OSDEF_H
#define OSDEF_H


//////////////////////////////////////////////////////////////////////
// OS API definition
//////////////////////////////////////////////////////////////////////

#ifdef WIN32
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif // NOMINMAX

 // supports Windows NT 4.0 and later, not Windows 95.
 // mainly for using winsock2 functions
 #ifndef _WIN32_WINNT
  #define _WIN32_WINNT 0x0400
 #endif // _WIN32_WINNT
 #include <windows.h>
 #include <winsock2.h>

 // The ordering of the fields in this struct is important. 
 // It has to match those in WSABUF.
 struct iovec
 {
  u_long iov_len; // byte count to read/write
  char *iov_base; // data to be read/written
 };

 #define EWOULDBLOCK             WSAEWOULDBLOCK
 #define EINPROGRESS             WSAEINPROGRESS
 #define EALREADY                WSAEALREADY
 #define ENOTSOCK                WSAENOTSOCK
 #define EDESTADDRREQ            WSAEDESTADDRREQ
 #define EMSGSIZE                WSAEMSGSIZE
 #define EPROTOTYPE              WSAEPROTOTYPE
 #define ENOPROTOOPT             WSAENOPROTOOPT
 #define EPROTONOSUPPORT         WSAEPROTONOSUPPORT
 #define ESOCKTNOSUPPORT         WSAESOCKTNOSUPPORT
 #define EOPNOTSUPP              WSAEOPNOTSUPP
 #define EPFNOSUPPORT            WSAEPFNOSUPPORT
 #define EAFNOSUPPORT            WSAEAFNOSUPPORT
 #define EADDRINUSE              WSAEADDRINUSE
 #define EADDRNOTAVAIL           WSAEADDRNOTAVAIL
 #define ENETDOWN                WSAENETDOWN
 #define ENETUNREACH             WSAENETUNREACH
 #define ENETRESET               WSAENETRESET
 #define ECONNABORTED            WSAECONNABORTED
 #define ECONNRESET              WSAECONNRESET
 #define ENOBUFS                 WSAENOBUFS
 #define EISCONN                 WSAEISCONN
 #define ENOTCONN                WSAENOTCONN
 #define ESHUTDOWN               WSAESHUTDOWN
 #define ETOOMANYREFS            WSAETOOMANYREFS
 #define ETIMEDOUT               WSAETIMEDOUT
 #define ECONNREFUSED            WSAECONNREFUSED
 #define ELOOP                   WSAELOOP
 #define EHOSTDOWN               WSAEHOSTDOWN
 #define EHOSTUNREACH            WSAEHOSTUNREACH
 #define EPROCLIM                WSAEPROCLIM
 #define EUSERS                  WSAEUSERS
 #define EDQUOT                  WSAEDQUOT
 #define ESTALE                  WSAESTALE
 #define EREMOTE                 WSAEREMOTE
#endif // WIN32

#ifdef WIN32
 typedef HANDLE CM_HANDLE;
 typedef SOCKET CM_SOCKET;
 #define CM_INVALID_HANDLE INVALID_HANDLE_VALUE
 #define CM_SD_RECEIVE SD_RECEIVE
 #define CM_SD_SEND SD_SEND
 #define CM_SD_BOTH SD_BOTH
#else // !WIN32
 typedef int CM_HANDLE;
 typedef CM_HANDLE CM_SOCKET;
 #define CM_INVALID_HANDLE -1
 #define CM_SD_RECEIVE 0
 #define CM_SD_SEND 1
 #define CM_SD_BOTH 2
#endif // WIN32

#ifndef WIN32
 typedef long long           LONGLONG;
 #ifndef DWORD
 typedef unsigned long       DWORD;
 #endif
 typedef int                 BOOL;
 typedef unsigned char       BYTE;
 typedef unsigned short      WORD;
 typedef float               FLOAT;
 typedef int                 INT;
 typedef unsigned int        UINT;
 typedef FLOAT               *PFLOAT;
 typedef BOOL                *LPBOOL;
 typedef BYTE                *LPBYTE;
 typedef int                 *LPINT;
 typedef WORD                *LPWORD;
 typedef long                *LPLONG;
 typedef DWORD               *LPDWORD;
 typedef unsigned int        *LPUINT;
 typedef void                *LPVOID;
 typedef const void          *LPCVOID;

 typedef char                 CHAR;
 typedef char                 TCHAR;
 typedef unsigned short       WCHAR;
 typedef const char           *LPCSTR;
 typedef char                 *LPSTR;
 typedef const unsigned short *LPCWSTR;
 typedef unsigned short       *LPWSTR;

 #ifndef FALSE
  #define FALSE 0
 #endif // FALSE
 #ifndef TRUE
  #define TRUE 1
 #endif // TRUE
#endif // !WIN32

#ifdef _MSC_VER
 #pragma warning(disable: 4786) // identifier was truncated to '255' characters in the browser information(mainly brought by stl)
 #pragma warning(disable: 4355) // disable 'this' used in base member initializer list
 #pragma warning(disable: 4275) // deriving exported class from non-exported
 #pragma warning(disable: 4251) // using non-exported as public in exported
#endif // _MSC_VER


//////////////////////////////////////////////////////////////////////
// C definition
//////////////////////////////////////////////////////////////////////

#ifdef WIN32
 #include <string.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <time.h>
 #include <limits.h>
 #include <stddef.h>
 #include <stdarg.h>
 #include <signal.h>

 #include <crtdbg.h>
 #include <process.h>
 #define getpid _getpid
 #define snprintf _snprintf
 #define strcasecmp _stricmp
 #define strncasecmp _strnicmp
#endif // WIN32

#ifdef UNIX
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <errno.h>
 #include <limits.h>
 #include <stdarg.h>
 #include <time.h>
 #include <signal.h>
 #include <sys/stat.h>
 #include <sys/fcntl.h>
 #include <pthread.h>
 #include <fcntl.h>
 #include <sys/types.h>
 #include <sys/ioctl.h>
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>

 #define EWOULDBLOCK EAGAIN
 
 #include <assert.h>
#endif // UNIX

#endif // !OSDEF_H

