#ifndef COMMAND_QUEUE_H
#define COMMAND_QUEUE_H

#include <queue>
#include "Mutex.h"

class CCommandRequest;

//thread-safe command queue
class CCommandQueue
{
public:
    CCommandQueue(){}
    virtual ~CCommandQueue();

    void Enqueue(CCommandRequest* pCommandRequest);
    void Execute();


private:
    typedef Mutex MUTEX;
    typedef std::queue<CCommandRequest*> CRQUEUE;

    MUTEX   m_mutexQueue;
    CRQUEUE m_crQueue;
};

#endif
