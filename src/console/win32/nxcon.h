// nxcon.h : main header file for the NXCON application
//

#if !defined(AFX_NXCON_H__A5C08A87_42D8_47F3_8F69_9566830EF29E__INCLUDED_)
#define AFX_NXCON_H__A5C08A87_42D8_47F3_8F69_9566830EF29E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif


//
// Common inline functions
//

inline BOOL SafeFreeResource(HGLOBAL hRes)
{
   return (hRes != NULL) ? FreeResource(hRes) : FALSE;
}


#include <nxclapi.h>
#include <nms_util.h>
#include "resource.h"      // Main symbols
#include "globals.h"       // Global symbols
#include "ControlPanel.h"
#include "EventBrowser.h"
#include "ObjectBrowser.h"
#include "MapFrame.h"
#include "ProgressDialog.h"	// Added by ClassView


/////////////////////////////////////////////////////////////////////////////
// CConsoleApp:
// See nxcon.cpp for the implementation of this class
//

class CConsoleApp : public CWinApp
{
public:
	CConsoleApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConsoleApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
protected:
	DWORD m_dwClientState;
	CEventBrowser *m_pwndEventBrowser;
	CWnd* m_pwndObjectBrowser;
	CWnd *m_pwndCtrlPanel;
	HMENU m_hCtrlPanelMenu;
	HACCEL m_hCtrlPanelAccel;
	HMENU m_hMDIMenu;
	HACCEL m_hMDIAccel;
	BOOL m_bAuthFailed;
	CProgressDialog m_dlgProgress;


public:
	void EventHandler(DWORD dwEvent, DWORD dwCode, void *pArg);
	void OnViewDestroy(DWORD dwView, CWnd *pWnd);
	void OnViewCreate(DWORD dwView, CWnd *pWnd);
   DWORD GetClientState(void) { return m_dwClientState; }
	//{{AFX_MSG(CConsoleApp)
	afx_msg void OnAppAbout();
	afx_msg void OnViewControlpanel();
	afx_msg void OnViewEvents();
	afx_msg void OnViewMap();
	afx_msg void OnConnectToServer();
	afx_msg void OnViewObjectbrowser();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	BOOL m_bEventBrowserActive;
	BOOL m_bObjectBrowserActive;
	BOOL m_bCtrlPanelActive;
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NXCON_H__A5C08A87_42D8_47F3_8F69_9566830EF29E__INCLUDED_)
