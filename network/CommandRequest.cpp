#include <unistd.h>
#include <string.h>
#include "CommandRequest.h"
#include "NetworkInterface.h"

CCRSendCommand::Allocator CCRSendCommand::m_allocator;
CCRDisconnect::Allocator  CCRDisconnect::m_allocator;

CCRSendCommand::CCRSendCommand( INetConnection* pConnection,
                                unsigned char* pCommand,
                                int nCmdLen
                              )
                              : m_pConnection( pConnection )
{
    m_nBufferSize = Alignment( nCmdLen );
    m_pCommand = new unsigned char[m_nBufferSize];
    if ( m_pCommand )
    {
        memcpy( m_pCommand, pCommand, m_nCmdLen = nCmdLen );
    }
    else
    {
        m_nBufferSize = 0;
        m_nCmdLen = 0;
    }
}

CCRSendCommand::~CCRSendCommand()
{
    if ( m_pCommand )
        delete [] m_pCommand;
}

void CCRSendCommand::ReAssign( INetConnection* pCon,
                               unsigned char* pCommand, 
                               int nCmdLen
                             )
{
    m_pConnection = pCon;
    memcpy( m_pCommand, pCommand, m_nCmdLen = nCmdLen );
}

void CCRSendCommand::Action()
{
    if ( m_pConnection && m_pCommand )
    {
        m_pConnection->SendCommand( m_pCommand, m_nCmdLen );
    }

    m_nCmdLen = 0;
    m_allocator.Free( this );
}


CCRDisconnect::CCRDisconnect( INetConnection* pConnection, 
                              unsigned char* pData /*=NULL*/,
                              int nDataLen/* = 0*/
                            )
                            : m_pConnection( pConnection )
{
}

CCRDisconnect::~CCRDisconnect()
{
}

void CCRDisconnect::Action()
{
    if ( m_pConnection )
        NetworkDestroyConnection( m_pConnection );

    m_allocator.Free( this );
}

