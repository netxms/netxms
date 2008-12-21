// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__4E3D467C_EC59_4C08_A66F_BBAE089BCCD7__INCLUDED_)
#define AFX_STDAFX_H__4E3D467C_EC59_4C08_A66F_BBAE089BCCD7__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#if (_WIN32_WCE <= 200)
#error This project does not support MFCCE 2.00 or earlier, because it requires CControlBar, available only in MFCCE 2.01 or later
#endif

#if (_WIN32_WCE <= 211)
#error This project can not be built for H/PC Pro 2.11 or earlier platforms.
#endif

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#define _WINSOCK2API_    /* Prevent inclusion of winsock2.h */

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions

#if defined(_AFXDLL)
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#endif

#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxsock.h>		// MFC socket extensions


// Redefine message handlers undefined in afxmsg_.h
#ifndef ON_WM_CONTEXTMENU
#define ON_WM_CONTEXTMENU() \
	{ WM_CONTEXTMENU, 0, 0, 0, AfxSig_vWp, \
		(AFX_PMSG)(AFX_PMSGW)(void (AFX_MSG_CALL CWnd::*)(CWnd*, CPoint))&OnContextMenu },
#endif


// Missing function defines
#ifndef LoadCursor
#ifdef UNICODE
#define LoadCursor LoadCursorW
#else
#define LoadCursor LoadCursorA
#endif
#endif


//{{AFX_INSERT_LOCATION}}
// Microsoft eMbedded Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__4E3D467C_EC59_4C08_A66F_BBAE089BCCD7__INCLUDED_)
