#ifndef COMMAND_REQUEST_ALLOCATOR
#define COMMAND_REQUEST_ALLOCATOR

#include <stdio.h>
#include <list>
#include "Mutex.h"

class INetConnection;

template<class T>
class CCRAllocator
{
public:
    CCRAllocator(){}
    ~CCRAllocator();

    T* Allocate( INetConnection* pCon, 
                 unsigned char* pData=NULL, 
                 int nDataLen=0
               );
    void Free(T* pFree);

private:
    typedef std::list<T*> CFreeList;
    typedef Mutex CMutex;

    CMutex     m_lckFreeList;
    CFreeList  m_free;
};

template<class T>
CCRAllocator<T>::~CCRAllocator()
{
    T* pCR = NULL;
    Lock<CMutex> guard(m_lckFreeList);
    typename CFreeList::iterator pos = m_free.begin();
    for ( ; pos != m_free.end(); ++pos )
    {
        pCR = *pos;
        delete pCR;
    }
}

template<class T>
T* CCRAllocator<T>::Allocate( INetConnection* pCon, 
                           unsigned char* pData, 
                           int nDataLen
                         )
{
    T* pNew = NULL;
    Lock<Mutex> guard(m_lckFreeList);
    typename CFreeList::iterator posFind = m_free.begin();
    for ( ; posFind != m_free.end(); ++posFind )
    {
        if ( (*posFind)->BufferSize() >= nDataLen )
        {
            pNew = *posFind;
            pNew->ReAssign( pCon, pData, nDataLen );
            m_free.erase( posFind );
            break;
        }
    }

    if ( !pNew )
    {
        pNew = new T(pCon, pData, nDataLen);
    }

    return pNew;
}

template<class T>
void CCRAllocator<T>::Free(T* pFree)
{
    Lock<Mutex> guard(m_lckFreeList);
    typename CFreeList::iterator posInsert = m_free.begin();
    for ( ; posInsert != m_free.end(); ++posInsert )
        if ( (*posInsert)->BufferSize() >= pFree->BufferSize() )
            break;

    m_free.insert( posInsert, pFree );
}

#endif
