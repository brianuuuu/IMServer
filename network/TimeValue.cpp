
#include "CmBase.h"
#include "TimeValue.h"

#ifdef UNIX
 #include <sys/time.h>
#endif

const CTimeValue CTimeValue::s_tvZero;
const CTimeValue CTimeValue::s_tvMax(LONG_MAX, CM_ONE_SECOND_IN_USECS-1);

CTimeValue::CTimeValue(long aSec, long aUsec)
{
	Set(aSec, aUsec);
}

CTimeValue::CTimeValue(const timeval &aTv)
{
	Set(aTv);
}

CTimeValue::CTimeValue(double aSec)
{
	Set(aSec);
}

void CTimeValue::Set(long aSec, long aUsec)
{
	m_lSec = aSec;
	m_lUsec = aUsec;
	Normalize();
}

void CTimeValue::Set(const timeval &aTv)
{
	m_lSec = aTv.tv_sec;
	m_lUsec = aTv.tv_usec;
	Normalize();
}

void CTimeValue::Set(double aSec)
{
	long l = (long)aSec;
	m_lSec = l;
	m_lUsec = (long)((aSec - (double)l) * CM_ONE_SECOND_IN_USECS);
	Normalize();
}

long CTimeValue::GetSec() const 
{
	return m_lSec;
}

long CTimeValue::GetUsec() const 
{
	return m_lUsec;
}

void CTimeValue::Normalize()
{
	if (m_lUsec >= CM_ONE_SECOND_IN_USECS) {
		do {
			m_lSec++;
			m_lUsec -= CM_ONE_SECOND_IN_USECS;
		}
		while (m_lUsec >= CM_ONE_SECOND_IN_USECS);
	}
	else if (m_lUsec <= -CM_ONE_SECOND_IN_USECS) {
		do {
			m_lSec--;
			m_lUsec += CM_ONE_SECOND_IN_USECS;
		}
		while (m_lUsec <= -CM_ONE_SECOND_IN_USECS);
	}

	if (m_lSec >= 1 && m_lUsec < 0) {
		m_lSec--;
		m_lUsec += CM_ONE_SECOND_IN_USECS;
	}
	else if (m_lSec < 0 && m_lUsec > 0) {
		m_lSec++;
		m_lUsec -= CM_ONE_SECOND_IN_USECS;
	}
}

CTimeValue CTimeValue::GetTimeOfDay()
{
#ifdef WIN32
	FILETIME tfile;
	::GetSystemTimeAsFileTime(&tfile);

	ULARGE_INTEGER _100ns;
	_100ns.LowPart = tfile.dwLowDateTime;
	_100ns.HighPart = tfile.dwHighDateTime;
	_100ns.QuadPart -= (DWORDLONG)0;
	return CTimeValue((long)(_100ns.QuadPart / (10000 * 1000)), 
					  (long)((_100ns.QuadPart % (10000 * 1000)) / 10));
#endif // !WIN32

#ifdef UNIX
	timeval tvCur;
	int nRet = ::GetTimeOfDay(&tvCur, NULL);
	CM_ASSERTE(nRet == 0);
	return CTimeValue(tvCur);
#endif
}


int operator > (const CTimeValue &aLeft, const CTimeValue &aRight)
{
	if (aLeft.GetSec() > aRight.GetSec())
		return 1;
	else if (aLeft.GetSec() == aRight.GetSec() && aLeft.GetUsec() > aRight.GetUsec())
		return 1;
	else
		return 0;
}

int operator >= (const CTimeValue &aLeft, const CTimeValue &aRight)
{
	if (aLeft.GetSec() > aRight.GetSec())
		return 1;
	else if (aLeft.GetSec() == aRight.GetSec() && aLeft.GetUsec() >= aRight.GetUsec())
		return 1;
	else
		return 0;
}

int operator < (const CTimeValue &aLeft, const CTimeValue &aRight)
{
	return aRight > aLeft;
}

int operator <= (const CTimeValue &aLeft, const CTimeValue &aRight)
{
	return aRight >= aLeft;
}

int operator == (const CTimeValue &aLeft, const CTimeValue &aRight)
{
	return aLeft.GetSec() == aRight.GetSec() && 
		   aLeft.GetUsec() == aRight.GetUsec();
}

int operator != (const CTimeValue &aLeft, const CTimeValue &aRight)
{
	return !(aLeft == aRight);
}

void CTimeValue::operator+= (const CTimeValue &aRight)
{
	m_lSec = GetSec() + aRight.GetSec();
	m_lUsec = GetUsec() + aRight.GetUsec();
	Normalize();
}

void CTimeValue::operator-= (const CTimeValue &aRight)
{
	m_lSec = GetSec() - aRight.GetSec();
	m_lUsec = GetUsec() - aRight.GetUsec();
	Normalize();
}

CTimeValue operator + (const CTimeValue &aLeft, const CTimeValue &aRight)
{
	return CTimeValue(aLeft.GetSec() + aRight.GetSec(), 
					  aLeft.GetUsec() + aRight.GetUsec());
}

CTimeValue operator - (const CTimeValue &aLeft, const CTimeValue &aRight)
{
	return CTimeValue(aLeft.GetSec() - aRight.GetSec(), 
					  aLeft.GetUsec() - aRight.GetUsec());
}
