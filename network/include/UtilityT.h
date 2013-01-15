/*------------------------------------------------------*/
/* Utility template class                               */
/*                                                      */
/* UtilityT.h                                           */
/*                                                      */
/*                                                      */
/* History                                              */
/*                                                      */
/*	11/28/2003	Create                                  */
/*                                                      */
/*------------------------------------------------------*/

#ifndef UTILITYT_H
#define UTILITYT_H


template <class T> class CCmComAutoPtr
{
public:
	CCmComAutoPtr(T *aPtr = NULL) 
		: m_pRawPtr(aPtr)
	{
		if (m_pRawPtr)
			m_pRawPtr->AddRef();
	}

	CCmComAutoPtr(const CCmComAutoPtr& aAutoPtr) 
		: m_pRawPtr(aAutoPtr.m_pRawPtr)
	{
		if (m_pRawPtr)
			m_pRawPtr->AddRef();
	}

	~CCmComAutoPtr() 
	{
		if (m_pRawPtr)
			m_pRawPtr->Release();
	}

	CCmComAutoPtr& operator = (const CCmComAutoPtr& aAutoPtr) 
	{
		return (*this = aAutoPtr.m_pRawPtr);
	}

	CCmComAutoPtr& operator = (T* aPtr) 
	{
		if (m_pRawPtr == aPtr)
			return *this;

		if (aPtr)
			aPtr->AddRef();
		if (m_pRawPtr)
			m_pRawPtr->Release();
		m_pRawPtr = aPtr;
		return *this;
	}

	operator void* () const 
	{
		return m_pRawPtr;
	}

	T* operator -> () const 
	{
		CM_ASSERTE(m_pRawPtr);
		return m_pRawPtr;
	}

	T* Get() const 
	{
		return m_pRawPtr;
	}

	T* ParaIn() const 
	{
		return m_pRawPtr;
	}

	T*& ParaOut() 
	{
		if (m_pRawPtr) {
			m_pRawPtr->Release();
			m_pRawPtr = NULL;
		}
		return static_cast<T*&>(m_pRawPtr);
	}

	T*& ParaInOut() 
	{
		return static_cast<T*&>(m_pRawPtr);
	}

	T& operator * () const 
	{
		CM_ASSERTE(m_pRawPtr);
		return *m_pRawPtr;
	}

private:
	T *m_pRawPtr;
};

template <class T> class CCmBufferWapper
{
public:
	explicit CCmBufferWapper(DWORD aSize = 0) : m_pBuffer(NULL), m_dwSize(0)
		{ Reset(aSize); }

	~CCmBufferWapper()
		{ Clean(); }

	void Clean()
	{
		delete [] m_pBuffer;
		m_pBuffer = NULL;
		m_dwSize = 0;
	}

	void Reset(DWORD aSize)
	{
		Clean();
		if (aSize > 0) {
			m_dwSize = aSize;
			m_pBuffer = new T[aSize];
			CM_ASSERTE(m_pBuffer);
		}
	}

	T* GetRawPtr() const
		{ return m_pBuffer; }

	DWORD GetSize() const
		{ return m_dwSize; }

	DWORD GetBytes() const
		{ return m_dwSize * sizeof(T); }

private:
	T *m_pBuffer;
	DWORD m_dwSize;
};

typedef CCmBufferWapper<char> CCmBufferWapperChar;


template <class Type> class Factory
{
public:
	static Type* CreateOne()
	{
		return new Type();
	}

	static void DestoryOne(Type *aOne)
	{
		delete aOne;
	}
};

#endif // !UTILITYT_H

