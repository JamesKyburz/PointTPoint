// Manager.h : Declaration of the CManager

#ifndef __MANAGER_H_
#define __MANAGER_H_

#include "resource.h"       // main symbols

#define PTP_IMANAGER 0x01

#include "ptp.h"            // point to point definition

// If using ATL 7.0 do not use "USE_CONVERSION" macro and use CW2A otherwise
// use macro with OLE2A ( this is only for the sending routines )
// the receiving routines use ATL 3.0 allways.

#if _ATL_VER < 0x0700 
#define CW2A OLE2A
  #ifndef SOMETIMES_USES_CONVERSION
    #define SOMETIMES_USES_CONVERSION USES_CONVERSION;
  #endif
#else
  #ifndef SOMETIMES_USES_CONVERSION
    #define SOMETIMES_USES_CONVERSION ""
  #endif
#endif

/////////////////////////////////////////////////////////////////////////////
// CManager
class ATL_NO_VTABLE CManager : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CManager, &CLSID_Manager>,
	public IDispatchImpl<IManager, &IID_IManager, &LIBID_POINTTPOINTLib>
{
public:
	CManager()
	{
      WORD        wVersionRequested;
      WSADATA     wsaData;
      // Setup windows sockets 
      wVersionRequested = MAKEWORD( 0x01, 0x01 ); 
      if ( WSAStartup( wVersionRequested, &wsaData ) != 0x00 ) 
      {
        zPTPLog( ( FILE * ) NULL, 0x03, "CManager", PTP_LOG_LEVEL_ERROR , 0x00, "%s", "Error initlializing Windows sockets" );
      }

	  m_Log = 0x00;
	  m_Script = 0x00;
	  m_Language = 0x00;
	  m_Failed = 0x00;
	  m_Encrypt = 0x00;
	  m_BufferSize = 0x00;
	  m_SocketFd = 0x00;
	  m_Host = m_FailoverHost = NULL;
	  m_Port = 0x4D2;
	  m_FailoverPort = 0x00;
	  m_Timeout = 0x00;
	  m_TemporaryPointer = NULL;
	  m_Application = NULL;
    }

	~CManager()
	{

	  // Disconnect if exists
	  PTP_Cleanup( &m_SocketFd );

      // Release windows sockets 
  	  WSACleanup(); 

	  // Release memory allcated previously
	  if ( m_TemporaryPointer )
	  {
	    free ( m_TemporaryPointer );
	  }
	  if ( m_Host )
	  {
	    free ( m_Host );
	  }
  	  if ( m_FailoverHost )
	  {
	    free ( m_FailoverHost );
	  }
  	  if ( m_Application )
	  {
	    free ( m_Application );
	  }

	}

DECLARE_REGISTRY_RESOURCEID(IDR_MANAGER)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CManager)
	COM_INTERFACE_ENTRY(IManager)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IManager
public:
	STDMETHOD(put_Application)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_BufferSize)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_Encrypt)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_Encrypt)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_HeaderSize)(/*[out, retval]*/ long *pVal);
	STDMETHOD(ParseFileHeader)(/*[in]*/ BSTR *filePath, /*[out,retval]*/ long *rval);
	STDMETHOD(put_Script)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_ReceivedBuffer)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Language)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_Language)(/*[in]*/ long newVal);
	STDMETHOD(put_Timeout)(/*[in]*/ long newVal);
	STDMETHOD(put_FailoverPort)(/*[in]*/ long newVal);
	STDMETHOD(put_FailoverHost)(/*[in]*/ BSTR newVal);
	STDMETHOD(put_Port)(/*[in]*/ long newVal);
	STDMETHOD(put_Host)(/*[in]*/ BSTR newVal);
	STDMETHOD(ReceiveReply)(/*[out]*/ BSTR *buffer, /*[out,retval]*/ long *rval);
	STDMETHOD(SendReply)(/*[in]*/ BSTR *buffer, /*[out,retval]*/ long *rval);
	STDMETHOD(ReceiveRequest)(/*[out]*/ BSTR *buffer, /*[out,retval]*/ long *rval);
	STDMETHOD(SendSafeRequest)(/*[in]*/ BSTR *buffer,/*[in]*/ BSTR *path, /*[out,retval]*/ long *rval);
	STDMETHOD(SendRequest)(/*[in]*/ BSTR *buffer,/*[out,retval]*/ long *rval);
	STDMETHOD(Connect)(/*[out,retval]*/ long *rval);

private:

///////////////////////////////////////////
// PTP_Log                      
// Write information to                   
// eventlog
// is not registered.                     
// Information can be errors or data you  
// always want to log.                    
// returns privateVoid                    
////////////////////////////////////////////

privateVoid zPTPLog( FILE *fp, char outLevel, char *funcName, int type, int code, const char *fmt, ... )
{
    va_list ap;
    char message[PTP_MAX_LOG_MESSAGE_SIZE];
	char details[0x64];
 	char machineName[MAX_COMPUTERNAME_LENGTH + 0x01];
    DWORD machineNameLen = MAX_COMPUTERNAME_LENGTH + 0x01;
	char *logLevel[] = { "-E-",
                         "-I-",
                         "-W-",
                         "-D-",
                         "-S-",
                         "" };
                   
    if ( !outLevel || type == PTP_LOG_LEVEL_NOTHING )
    {
      return;
    }

    va_start( ap, fmt );

	int rc = GetLastError();

	// Check if file pointer defined then write to file pointer
	if( fp )
    {
	  if ( type != PTP_LOG_LEVEL_NOTHING )
	  {
        message[(vsprintf( message, fmt, ap ))] = '\x00';
        fprintf( fp, "%s%s%s, system msg:%s, errno:%d\n", funcName, logLevel[type], message,  
          strerror( PTP_ERROR ), PTP_ERROR );
	  } 

      else
	  {   
        vfprintf( fp, fmt, ap );
	  }
	}

    else
	{

     GetComputerName( machineName, &machineNameLen );

     message[(vsprintf( message, fmt, ap ))] = '\x00';
	 sprintf( details, ", system msg:%s, errno:%d\n", strerror( PTP_ERROR ), PTP_ERROR );

     strcat( message, ( const char * ) details );

 	 // Incase the error was in file I/O log the error number here
	 sprintf( details, ", %d ", rc );
     strcat( message, ( const char * ) details );

   LPTSTR lpszStrings[1];
   lpszStrings[0] = message; 
   
   HANDLE hEventSource = RegisterEventSource( machineName, m_Application == NULL ? "PointTPoint" : m_Application );
   if ( hEventSource != 0x00 )
   {
     ReportEvent( hEventSource, 
                type == PTP_LOG_LEVEL_ERROR ? EVENTLOG_ERROR_TYPE : EVENTLOG_INFORMATION_TYPE,
            0x00,
          0x00,
          NULL,
          0x01,
          0x00,
          ( LPCTSTR * ) &lpszStrings[0],
          NULL );
   DeregisterEventSource( hEventSource );
   }
	}


}

IAppLogPtr					m_Log;
char					   *m_TemporaryPointer;
PTP_SOCKET					m_SocketFd;
long						m_Timeout;
long						m_Port;
long						m_FailoverPort;
long						m_Language;
long 						m_Encrypt;
long                        m_Failed;
long                        m_Script;
long                        m_BufferSize;
char                        *m_Host;
char                        *m_FailoverHost;
char                        *m_Application;

#include "ptp.cpp" // point to point API 

};

#endif //__MANAGER_H_
