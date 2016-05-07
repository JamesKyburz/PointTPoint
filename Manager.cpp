// Manager.cpp : Implementation of CManager
#include "stdafx.h"
#include "PointTPoint.h"
#include "Manager.h"

/////////////////////////////////////////////////////////////////////////////
// CManager


STDMETHODIMP CManager::Connect(long *rval)
{
    PTP_Connect( rval, m_Host, &m_Port, m_FailoverHost, &m_FailoverPort, &m_SocketFd, &m_Timeout, &m_Failed );
    
	return S_OK;
}


STDMETHODIMP CManager::ReceiveRequest(BSTR* buffer, long *rval)
{
	USES_CONVERSION;

	char ack[2] = { PTP_ACK, 0x00 };	
	long	rcvMaxSize;
	long	rcvSize;
	long    sendSize = 0x01;

    if ( m_TemporaryPointer )
	{
	  free( m_TemporaryPointer );
  	  m_TemporaryPointer = NULL;
	}

	PTP_ReceiveRequest( rval, &m_SocketFd, NULL, &rcvSize, &rcvMaxSize, &m_Timeout, &m_Language );

    if ( !m_Script )
	{
	  if ( m_TemporaryPointer )
	  {
 	    SysFreeString( *buffer );
        *buffer = A2BSTR( m_TemporaryPointer );
	    free( m_TemporaryPointer );
		m_TemporaryPointer = NULL;
	  }
	}

	/* ACK sending is the last thing so to sending processing will
	   get this notification after we have used the ATL BSTR conversions */

	if ( *rval == PTP_INTERFACE_SUCCESS )
	{
      /* send ack to requester */
      ( void ) zPutGetStream( rval, NULL, &m_SocketFd, ack, &sendSize, &m_Timeout, 
	           0x01, PTP_SEND, "CManager::PTP_ReceiveRequest" );
	}

	return S_OK;
}


STDMETHODIMP CManager::ReceiveReply(BSTR *buffer, long *rval)
{

	USES_CONVERSION;

	char ack[2] = { PTP_ACK, 0x00 };	
	long	rcvMaxSize;
	long	rcvSize;
	long    sendSize = 0x01;

    if ( m_TemporaryPointer )
	{
	  free( m_TemporaryPointer );
  	  m_TemporaryPointer = NULL;
	}

	PTP_ReceiveReply( rval, &m_SocketFd, NULL, &rcvSize, &rcvMaxSize, &m_Timeout, &m_Language );

	if ( !m_Script )
	{
	  if ( m_TemporaryPointer )
	  {
 	    SysFreeString( *buffer );
        *buffer = A2BSTR( m_TemporaryPointer );
	    free( m_TemporaryPointer );
		m_TemporaryPointer = NULL;
	  }
	}

	/* ACK sending is the last thing so to sending processing will
	   get this notification after we have used the ATL BSTR conversions */

	if ( *rval == PTP_INTERFACE_SUCCESS )
	{
	  /* send ack to requester */
      ( void ) zPutGetStream( rval, NULL, &m_SocketFd, ack, &sendSize, &m_Timeout, 
	  0x01, PTP_SEND, "CManager::PTP_ReceiveReply" );
	}

	return S_OK;
}


STDMETHODIMP CManager::SendReply(BSTR *buffer, long *rval)
{

    SOMETIMES_USES_CONVERSION;

	long	buffSize = SysStringLen( *buffer );	

	PTP_SendReply( rval, &m_SocketFd, CW2A( *buffer ), &buffSize, &m_Timeout, &m_Language, &m_Encrypt );

	return S_OK;

}


STDMETHODIMP CManager::SendRequest(BSTR *buffer, long *rval)
{

    SOMETIMES_USES_CONVERSION;

	long	buffSize = SysStringLen( *buffer );
	
	PTP_SendRequest( rval, &m_SocketFd, CW2A( *buffer ), &buffSize, &m_Timeout, &m_Language, &m_Encrypt );
	
	return S_OK;
	
}

STDMETHODIMP CManager::SendSafeRequest(BSTR *buffer, BSTR *path, long *rval)
{

    SOMETIMES_USES_CONVERSION;

  	long	buffSize = SysStringLen( *buffer );

    PTP_SendSafeRequest( rval, &m_SocketFd, CW2A( *path ), CW2A( *buffer ), &buffSize, &m_Timeout, &m_Language, &m_Encrypt );
	
	return S_OK;
}


STDMETHODIMP CManager::ParseFileHeader(BSTR *filePath, long *rval)

{
    SOMETIMES_USES_CONVERSION;
	long headerSize;
	long function;

	m_BufferSize = 0x00;
	m_Language = 0x00;
	m_Encrypt = false;

	PTP_ParseFileHeader( rval, CW2A( *filePath ), &headerSize, &function, &m_BufferSize, &m_Language, &m_Encrypt );

	return S_OK;
}


// Property implementations

STDMETHODIMP CManager::put_Timeout(long newVal)
{
	m_Timeout = newVal;

	return S_OK;
}

STDMETHODIMP CManager::put_Port(long newVal)
{
	if ( m_SocketFd )
	{
	  if ( newVal != m_Port )
	  {
		  PTP_Cleanup( &m_SocketFd );
	  }
	}

    m_Port = newVal;

	return S_OK;
}

STDMETHODIMP CManager::put_FailoverPort(long newVal)
{
	if ( m_SocketFd )
	{
	  if ( newVal != m_FailoverPort )
	  {
		  PTP_Cleanup( &m_SocketFd );
	  }
	}

    m_FailoverPort = newVal;

	return S_OK;
}

STDMETHODIMP CManager::put_Host(BSTR newVal)
{
    
    SOMETIMES_USES_CONVERSION;

  	long	hostSize = SysStringLen( newVal );


	if ( m_Host )
	{
  	  if ( m_SocketFd )
	  {
        if ( strcmp( CW2A( newVal ), m_Host ) )
		{
	      PTP_Cleanup( &m_SocketFd );
		}
	  }

	  free( m_Host );
	  m_Host = NULL;
	}

	m_Host = ( char * ) malloc ( hostSize + 0x01 );

	// malloc failed
	if ( !m_Host )
	{
		zPTPLog( ( FILE * ) NULL, 0x01, "CManager::put_Host", PTP_LOG_LEVEL_ERROR , 0x00, "%s %d", "Error in malloc for host with length ", hostSize );
	}
	else
	{
	  sprintf( m_Host, "%s", CW2A( newVal ) );
	}

	return S_OK;
}


STDMETHODIMP CManager::put_FailoverHost(BSTR newVal)
{
	
    SOMETIMES_USES_CONVERSION;

  	long	hostSize = SysStringLen( newVal );

	if ( m_FailoverHost )
	{
  	  if ( m_SocketFd )
	  {
        if ( strcmp( CW2A( newVal ), m_FailoverHost ) )
		{
	      PTP_Cleanup( &m_SocketFd );
		}
	  }
	
	  free( m_FailoverHost );
	  m_FailoverHost = NULL;
	}

	m_FailoverHost = ( char * ) malloc ( hostSize + 0x01 );

	// malloc failed
	if ( !m_FailoverHost )
	{
		zPTPLog( ( FILE * ) NULL, 0x01, "CManager::put_FailoverHost", PTP_LOG_LEVEL_ERROR , 0x00, "%s %d", "Error in malloc for host with length ", hostSize );
	}
	else
	{
	  sprintf( m_FailoverHost, "%s", CW2A( newVal ) );
	}

	return S_OK;
}


STDMETHODIMP CManager::get_ReceivedBuffer(BSTR *pVal)
{
	USES_CONVERSION;

	SysFreeString( *pVal );
	*pVal = A2BSTR( m_TemporaryPointer );

	return S_OK;
}


STDMETHODIMP CManager::get_Language(long *pVal)
{
    *pVal = m_Language;

	return S_OK;
}

STDMETHODIMP CManager::put_Language(long newVal)
{
    m_Language = newVal;

	return S_OK;
}


STDMETHODIMP CManager::get_Encrypt(VARIANT_BOOL *pVal)
{
    *pVal = m_Encrypt == 0xFFFFFFFF;

	return S_OK;
}


STDMETHODIMP CManager::put_Encrypt(VARIANT_BOOL newVal)
{
    m_Encrypt = newVal == 0xFFFFFFFF;

	return S_OK;
}

STDMETHODIMP CManager::put_Script(VARIANT_BOOL newVal)
{
    m_Script = newVal == 0xFFFFFFFF;

	return S_OK;
}

STDMETHODIMP CManager::get_HeaderSize(long *pVal)
{
	PTP_GetHeaderSize( pVal );
	
	return S_OK;
}



STDMETHODIMP CManager::get_BufferSize(long *pVal)
{
	*pVal = m_BufferSize;

	return S_OK;
}

STDMETHODIMP CManager::put_Application(BSTR newVal)
{
	
    SOMETIMES_USES_CONVERSION;

  	long	applicationSize = SysStringLen( newVal );

	if ( applicationSize )
	{
	  if ( m_Application )
	  {
		free( m_Application );
		m_Application = NULL;
	  }
	  m_Application = ( char * ) malloc ( applicationSize + 0x01 );

	  // malloc failed
	  if ( !m_Application )
	  {
		zPTPLog( ( FILE * ) NULL, 0x01, "CManager::put_FailoverHost", PTP_LOG_LEVEL_ERROR , 0x00, "%s %d", "Error in malloc for application with length ", applicationSize );
	  }
	  else
	  {
	    sprintf( m_Application, "%s", CW2A( newVal ) );
	  }
	}

	return S_OK;
}
