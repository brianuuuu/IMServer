/*-------------------------------------------------------------------------*/
/*                                                                         */
/* t120base.CPP                                                            */
/*                                                                         */
/*                                                                         */
/*-------------------------------------------------------------------------*/
//#include "stdafx.h"

#include "t120base.h"

#ifdef WIN32
#include <memory.h>
#include <winsock2.h>
#include <stdio.h>
#else
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <strings.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#endif


#ifdef UNIX
int GetTimeOfDay(struct timeval *tv, struct timezone *tz)
{
	static unsigned long lst_sys, lst_time;
	unsigned long sec;
	struct sysinfo info;
	int rt;
	
	sysinfo(&info);
	rt = gettimeofday(tv, tz);
	if (rt != 0)
		return -1;
	sec = lst_sys;
	if (info.uptime != lst_sys)
	{
		if (tv->tv_sec != lst_time)
		{
			lst_sys = info.uptime;
			lst_time = tv->tv_sec;
		}
		sec = lst_sys;
	}else if (tv->tv_sec != lst_time)
	{
		sec = lst_sys +1;
	}
	tv->tv_sec=sec;
	return rt;
}
#endif

/////////////////////////////////////////////////////////////////////////////
// T120_Object
//
T120_Object::T120_Object() 
{
	ref_cnt = 0;
	magic = 18207;
	sprintf(obj_key, "obj_%lu", (uint32)this);
}

T120_Object::~T120_Object()
{
	magic = 0;
	T120_ASSERT(ref_cnt == 0);
}

void T120_Object::add_reference()
{
	ref_cnt++;
}

void T120_Object::release_reference()
{
	ref_cnt--;
	if(ref_cnt == 0)
		delete this;
}

void T120_Object::assert_valid()
{
	T120_ASSERT(magic == 18207);
}

void T120_Object::set_obj_key(char* obj_key)
{
	if(obj_key && strlen(obj_key)<64)
		strcpy(this->obj_key, obj_key);
}

char* T120_Object::get_obj_key()
{
	return obj_key;
}

