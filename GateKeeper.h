// GateKeeper.h : Declaration of the CGateKeeper

#ifndef __GATEKEEPER_H_
#define __GATEKEEPER_H_

#include "resource.h"       // main symbols

#ifdef PTP_IMANAGER
  #undef PTP_IMANAGER
  #define PTP_IMANAGER 0x00
#endif

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
// CGateKeeper
class ATL_NO_VTABLE CGateKeeper : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CGateKeeper, &CLSID_GateKeeper>,
	public IDispatchImpl<IGateKeeper, &IID_IGateKeeper, &LIBID_POINTTPOINTLib>
{
public:
	CGateKeeper()
	{
	  m_pUnkMarshaler = NULL;
	  m_ShutdownTimeout = 0;
	}

	~CGateKeeper()
	{
      // Release windows sockets if not already done
	  WSACleanup(); 
	}

DECLARE_REGISTRY_RESOURCEID(IDR_GATEKEEPER)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CGateKeeper)
	COM_INTERFACE_ENTRY(IGateKeeper)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()


	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

// IGateKeeper
public:
	STDMETHOD(Server)(/*[in]*/ BSTR *configLocation, /*[out,retval]*/ long *rval);
	STDMETHOD(Shutdown)(/*[in]*/ BSTR *configLocation, /*[out,retval]*/ long *rval);
	STDMETHOD(put_ShutdownTimeout)(/*[in]*/ long newVal);

private:

CComPtr<IUnknown>          m_pUnkMarshaler;
long					   m_ShutdownTimeout;

#include "ptp.cpp" // point to point API 

};

#endif //__GATEKEEPER_H_
