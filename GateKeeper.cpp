// GateKeeper.cpp : Implementation of CGateKeeper
#include "stdafx.h"
#include "PointTPoint.h"
#include "GateKeeper.h"

/////////////////////////////////////////////////////////////////////////////
// CGateKeeper


STDMETHODIMP CGateKeeper::Server(BSTR *configLocation, long *rval)
{

	SOMETIMES_USES_CONVERSION;

    char*	cfgLocation = CW2A( *configLocation );

    PTP_Server( rval, cfgLocation );
	
	return S_OK;
}

STDMETHODIMP CGateKeeper::Shutdown(BSTR *configLocation, long *rval)
{

	SOMETIMES_USES_CONVERSION;

    char*	cfgLocation = CW2A( *configLocation );

	WORD        wVersionRequested;
    WSADATA     wsaData;
    // Setup windows sockets 
    wVersionRequested = MAKEWORD( 0x01, 0x01 ); 

	// No error checking here....
	if ( WSAStartup( wVersionRequested, &wsaData ) != 0x00 ) 
    {
		*rval = 0;
	}

	else
	{
		PTP_Shutdown( rval, &m_ShutdownTimeout, cfgLocation );
	}
	
	return S_OK;
}

STDMETHODIMP CGateKeeper::put_ShutdownTimeout(long newVal)
{
	m_ShutdownTimeout = newVal;

	return S_OK;
}


