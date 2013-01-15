/*------------------------------------------------------*/
/* Time value                                           */
/*                                                      */
/* TimeValue.h                                          */
/*                                                      */
/*                                                      */
/* History                                              */
/*                                                      */
/*	11/24/2003	Create                                  */
/*                                                      */
/*------------------------------------------------------*/

#ifndef TIMEVALUE_H
#define TIMEVALUE_H

#define CM_ONE_SECOND_IN_MSECS 1000L
#define CM_ONE_SECOND_IN_USECS 1000000L
#define CM_ONE_SECOND_IN_NSECS 1000000000L

#ifdef _MSC_VER
 // -------------------------------------------------------------------
 // These forward declarations are only used to circumvent a bug in
 // MSVC 6.0 compiler.  They shouldn't cause any problem for other
 // compilers.
 class CTimeValue;
  CTimeValue operator + (const CTimeValue &aLeft, const CTimeValue &aRight);
  CTimeValue operator - (const CTimeValue &aLeft, const CTimeValue &aRight);
#endif // _MSC_VER

class  CTimeValue  
{
public:
	CTimeValue(long aSec = 0, long aUsec = 0);
	CTimeValue(const timeval &aTv);
	CTimeValue(double aSec);
	
	void Set(long aSec, long aUsec);
	void Set(const timeval &aTv);
	void Set(double aSec);

	long GetSec() const ;
	long GetUsec() const ;

	void operator += (const CTimeValue &aRight);
	void operator -= (const CTimeValue &aRight);

	friend  CTimeValue operator + (const CTimeValue &aLeft, const CTimeValue &aRight);
	friend  CTimeValue operator - (const CTimeValue &aLeft, const CTimeValue &aRight);
	friend  int operator < (const CTimeValue &aLeft, const CTimeValue &aRight);
	friend  int operator > (const CTimeValue &aLeft, const CTimeValue &aRight);
	friend  int operator <= (const CTimeValue &aLeft, const CTimeValue &aRight);
	friend  int operator >= (const CTimeValue &aLeft, const CTimeValue &aRight);
	friend  int operator == (const CTimeValue &aLeft, const CTimeValue &aRight);
	friend  int operator != (const CTimeValue &aLeft, const CTimeValue &aRight);

	static CTimeValue GetTimeOfDay();
	static const CTimeValue s_tvZero;
	static const CTimeValue s_tvMax;
	
private:
	void Normalize();
	
	long m_lSec;
	long m_lUsec;
};

#endif // !TIMEVALUE_H

