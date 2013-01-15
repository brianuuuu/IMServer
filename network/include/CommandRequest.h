#ifndef COMMAND_REQUEST_H
#define COMMAND_REQUEST_H

#include "CRAllocator.h"

class INetConnection;

//interface of the command queue
class CCommandRequest
{
public:
    CCommandRequest(){}
    virtual ~CCommandRequest(){}

    virtual void Action() = 0;
};

//command 
class CCRSendCommand : public CCommandRequest
{
public:
    typedef CCRAllocator<CCRSendCommand> Allocator;

    CCRSendCommand( INetConnection* pConnection, 
                    unsigned char* pCommand,
                    int nCmdLen
                  );
    virtual ~CCRSendCommand();

    virtual void Action();

    int BufferSize() const { return m_nBufferSize; }
    void ReAssign(INetConnection* pCon, unsigned char* pCommand, int nCmdLen);

    static Allocator* GetAllocator() { return &m_allocator; }

private:
    enum{ ALIGNMENT = 16 };

    int Alignment( int nLength )
    {
        return ( nLength + ALIGNMENT - 1 ) & ~( ALIGNMENT - 1 );
    }

    static Allocator m_allocator;

    INetConnection* m_pConnection;
    unsigned char*  m_pCommand;
    int             m_nCmdLen;
    int             m_nBufferSize;
};

class CCRDisconnect : public CCommandRequest
{
public:
    typedef CCRAllocator<CCRDisconnect> Allocator;

    CCRDisconnect( INetConnection* pConnection, 
                   unsigned char* pData=NULL,
                   int nDataLen = 0
                 );
    virtual ~CCRDisconnect();

    virtual void Action();

    int BufferSize() const { return 0; }
    void ReAssign(INetConnection* pCon, unsigned char* pCommand, int nCmdLen)
    {   m_pConnection = pCon; }

    static Allocator* GetAllocator() { return &m_allocator; }

private:
    static Allocator m_allocator;

    INetConnection* m_pConnection;
};


#endif
