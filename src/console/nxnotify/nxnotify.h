// nxnotify.h : main header file for the NXNOTIFY application
//

#if !defined(AFX_NXNOTIFY_H__5C820938_A67E_45E9_81C7_733570B4B436__INCLUDED_)
#define AFX_NXNOTIFY_H__5C820938_A67E_45E9_81C7_733570B4B436__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

// Registry key for saving alarm sound configuration
#define NXNOTIFY_ALARM_SOUND_KEY  _T("Software\\NetXMS\\NetXMS Alarm Notifier\\Sounds")

#include <nxwinui.h>
#include "resource.h"       // main symbols
#include "globals.h"


/////////////////////////////////////////////////////////////////////////////
// CNxnotifyApp:
// See nxnotify.cpp for the implementation of this class
//

class CNxnotifyApp : public CWinApp
{
public:
	ALARM_SOUND_CFG m_soundCfg;

	CNxnotifyApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNxnotifyApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

public:
	void EventHandler(DWORD dwEvent, DWORD dwCode, void *pArg);
	//{{AFX_MSG(CNxnotifyApp)
	afx_msg void OnAppAbout();
	afx_msg void OnFileSettings();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


extern CNxnotifyApp appNotify;

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NXNOTIFY_H__5C820938_A67E_45E9_81C7_733570B4B436__INCLUDED_)
