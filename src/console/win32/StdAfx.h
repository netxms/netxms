// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__E691F9D5_0307_4FD4_9622_EB9BED9EAB89__INCLUDED_)
#define AFX_STDAFX_H__E691F9D5_0307_4FD4_9622_EB9BED9EAB89__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#define _WINSOCKAPI_    /* Prevent inclusion of winsock.h in windows.h */

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxpriv.h>
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <winsock2.h>
#include <afxsock.h>		// MFC socket extensions
#include <afxhtml.h>

#include <wininet.h>
#include <wincrypt.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__E691F9D5_0307_4FD4_9622_EB9BED9EAB89__INCLUDED_)
