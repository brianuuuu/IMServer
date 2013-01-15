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

#ifndef __T120BASE_H__
#define __T120BASE_H__


#include "platform.h"
#include "t120defs.h"

#include <errno.h>

#ifndef __cplusplus
#error This basic t120 facility can only be used in C++
#endif




//////////////////////////////////////////////////////////////////////////////
// T120_Object
//
class  T120_Object
{
public :
    T120_Object();
    virtual ~T120_Object();

    virtual void add_reference();
    virtual void release_reference();
	virtual void assert_valid();
	void set_obj_key(char* obj_key);
	char* get_obj_key();
	char obj_key[64];

	uint16 magic;
protected :
    uint16 ref_cnt;
};


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

#ifdef UNIX
int GetTimeOfDay(struct timeval *tv, struct timezone *tz);
#endif

#endif // __T120BASE_H__

