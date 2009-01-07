// nxav.h : main header file for the NXAV application
//

#if !defined(AFX_NXAV_H__4741A39F_BBB2_467C_984A_CF9B24661DC3__INCLUDED_)
#define AFX_NXAV_H__4741A39F_BBB2_467C_984A_CF9B24661DC3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#ifndef UNICODE
#error Building NetXMS Alarm Viewer without UNICODE support is deprecated
#endif


// Registry key for saving alarm sound configuration
#define NXAV_ALARM_SOUND_KEY  _T("Software\\NetXMS\\NetXMS Alarm Viewer\\Sounds")

#include "resource.h"       // main symbols

#include <nms_common.h>
#include <nxclapi.h>
#include <nxwinui.h>

#include "globals.h"

/////////////////////////////////////////////////////////////////////////////
// CAlarmViewApp:
// See nxav.cpp for the implementation of this class
//

class CAlarmViewApp : public CWinApp
{
public:
	CAlarmViewApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAlarmViewApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation

public:
	ALARM_SOUND_CFG m_soundCfg;
	void EventHandler(DWORD dwEvent, DWORD dwCode, void *pArg);
	void ErrorBox(DWORD dwError, TCHAR *pszMessage = NULL, TCHAR *pszTitle = NULL);
	//{{AFX_MSG(CAlarmViewApp)
	afx_msg void OnAppAbout();
	afx_msg void OnCmdSettings();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
protected:
	void DeleteWorkDir(void);
	BOOL InitWorkDir(void);
};


extern CAlarmViewApp appAlarmViewer;


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NXAV_H__4741A39F_BBB2_467C_984A_CF9B24661DC3__INCLUDED_)
