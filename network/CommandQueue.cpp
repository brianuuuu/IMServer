#include <unistd.h>
#include "CommandQueue.h"
#include "CommandRequest.h"

CCommandQueue::~CCommandQueue()
{
}

void CCommandQueue::Enqueue(CCommandRequest* pCommandRequest)
{
    Lock<MUTEX> guard( m_mutexQueue );
    m_crQueue.push( pCommandRequest );
}

void CCommandQueue::Execute()
{
    CCommandRequest* pCommandRequest =  NULL;

    Lock<MUTEX> guard( m_mutexQueue );
    while ( !m_crQueue.empty() )
    {
        pCommandRequest = m_crQueue.front();
        pCommandRequest->Action(); //pCommandRequest will be freed in Action
        m_crQueue.pop();       
    }
}
