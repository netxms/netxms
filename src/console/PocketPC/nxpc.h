// nxpc.h : main header file for the NXPC application
//

#if !defined(AFX_NXPC_H__CFE02F40_D486_4559_BCD5_D6A2F0B12067__INCLUDED_)
#define AFX_NXPC_H__CFE02F40_D486_4559_BCD5_D6A2F0B12067__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include <nms_common.h>
#include <nxclapi.h>

#include "resource.h"       // main symbols
#include "globals.h"
#include "MainFrm.h"
#include "RequestProcessingDlg.h"


/////////////////////////////////////////////////////////////////////////////
// CNxpcApp:
// See nxpc.cpp for the implementation of this class
//

class CNxpcApp : public CWinApp
{
public:
	CMenu * GetContextMenu(int nIndex);
	void EventHandler(DWORD dwEvent, DWORD dwCode, void *pArg);
	CNxpcApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNxpcApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
   void ErrorBox(DWORD dwError, TCHAR *pszMessage = NULL, TCHAR *pszTitle = NULL);

	//{{AFX_MSG(CNxpcApp)
	afx_msg void OnAppAbout();
	afx_msg void OnConnectToServer();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
protected:
	CMenu m_ctxMenu;
	BOOL SetupWorkDir(void);
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft eMbedded Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NXPC_H__CFE02F40_D486_4559_BCD5_D6A2F0B12067__INCLUDED_)
