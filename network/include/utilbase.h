#ifndef __UTIL_BASE_H__
#define __UTIL_BASE_H__

#ifdef WIN32
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif // NOMINMAX

 #ifndef _WIN32_WINNT
  #define _WIN32_WINNT 0x0400
 #endif // _WIN32_WINNT
#endif

#include "t120base.h"
#include "t120bs.h"
#include "osdef.h"

typedef T120_Byte_Stream CByteStream;

#define T120TRACE(mask, str)
#define T120_ASSERT(x) 
#define T120_ASSERTX(x) 
#define ERRTRACE(str) /*T120TRACE(0, str)*/
#define WARNINGTRACE(str) /*T120TRACE(1, str)*/
#define INFOTRACE(str) /*T120TRACE(2, str)*/
#define STATETRACE(str) /*T120TRACE(3, str)*/
#define PDUTRACE(str) /*T120TRACE(4, str)*/
#define FUNCTRACE(str) /*func_tracer _$FUNCTRACE$(str);*/
#define TICKTRACE(str) /*T120TRACE(6, str)*/
#define DETAILTRACE(str) /*T120TRACE(7, str)*/

#define ERRXTRACE(str) /*T120TRACE(0, obj_key<<":: "<<str)*/
#define WARNINGXTRACE(str) /*T120TRACE(1, obj_key<<":: "<<str)*/
#define INFOXTRACE(str) /*T120TRACE(2, obj_key<<":: "<<str)*/
#define STATEXTRACE(str)/* T120TRACE(3, obj_key<<":: "<<str)*/
#define PDUXTRACE(str) /*T120TRACE(4, obj_key<<":: "<<str)*/
#define FUNCXTRACE(str) /*funcx_tracer _$FUNCXTRACE$(obj_key, str);*/
#define DETAILXTRACE(str) /*T120TRACE(7, obj_key<<":: "<<str)*/

#define IM_ASSERT(x) 

#define IM_ASSERT(x) 




#endif//!__UTIL_BASE_H__

