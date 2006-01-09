// nxuilib.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include <afxdllx.h>
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Global data
//

TCHAR *g_pszSoundNames[] = { _T("<alarm1>"), _T("<alarm2>"), _T("<beep>"),
                             _T("<misc1>"), _T("<misc2>"), _T("<misc3>"),
                             _T("<misc4>"), _T("<misc5>"), _T("<ring1>"),
                             _T("<ring2>"), _T("<siren1>"), _T("<siren2>"),
                             NULL };
int g_nSoundId[] = { IDR_SND_ALARM1, IDR_SND_ALARM2, IDR_SND_BEEP,
                     IDR_SND_MISC1, IDR_SND_MISC2, IDR_SND_MISC3,
                     IDR_SND_MISC4, IDR_SND_MISC5, IDR_SND_RING1,
                     IDR_SND_RING2, IDR_SND_SIREN1, IDR_SND_SIREN2, 0 };
HINSTANCE g_hInstance;
COLORREF g_rgbInfoLineButtons = RGB(130, 70, 210);
COLORREF g_rgbInfoLineBackground = RGB(255, 255, 255);


//
// MFC DLL code
//

static AFX_EXTENSION_MODULE NxuilibDLL = { NULL, NULL };

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	// Remove this if you use lpReserved
	UNREFERENCED_PARAMETER(lpReserved);

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		TRACE0("NXUILIB.DLL Initializing!\n");
		
		// Extension DLL one-time initialization
		if (!AfxInitExtensionModule(NxuilibDLL, hInstance))
			return 0;

		// Insert this DLL into the resource chain
		// NOTE: If this Extension DLL is being implicitly linked to by
		//  an MFC Regular DLL (such as an ActiveX Control)
		//  instead of an MFC application, then you will want to
		//  remove this line from DllMain and put it in a separate
		//  function exported from this Extension DLL.  The Regular DLL
		//  that uses this Extension DLL should then explicitly call that
		//  function to initialize this Extension DLL.  Otherwise,
		//  the CDynLinkLibrary object will not be attached to the
		//  Regular DLL's resource chain, and serious problems will
		//  result.

		new CDynLinkLibrary(NxuilibDLL);
      g_hInstance = hInstance;
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		TRACE0("NXUILIB.DLL Terminating!\n");
		// Terminate the library before destructors are called
		AfxTermExtensionModule(NxuilibDLL);
	}
	return 1;   // ok
}
