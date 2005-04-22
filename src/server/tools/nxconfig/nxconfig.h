// nxconfig.h : main header file for the NXCONFIG application
//

#if !defined(AFX_NXCONFIG_H__7266C5B0_B724_4590_BD60_4B60C86A7561__INCLUDED_)
#define AFX_NXCONFIG_H__7266C5B0_B724_4590_BD60_4B60C86A7561__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include <nms_common.h>
#include <nms_util.h>
#include <nxsrvapi.h>


//
// Custom Windows messages
//

#define WM_START_STAGE        (WM_USER + 1)
#define WM_STAGE_COMPLETED    (WM_USER + 2)
#define WM_JOB_FINISHED       (WM_USER + 3)


//
// Database engine codes
//

#define DB_ENGINE_MYSQL    0
#define DB_ENGINE_PGSQL    1
#define DB_ENGINE_MSSQL    2


//
// Server configuration collected by wizard
//

struct WIZARD_CFG_INFO
{
   int m_iDBEngine;
   TCHAR m_szDBDriver[MAX_DB_STRING];
   TCHAR m_szDBServer[MAX_DB_STRING];
   TCHAR m_szDBName[MAX_DB_NAME];
   TCHAR m_szDBLogin[MAX_DB_LOGIN];
   TCHAR m_szDBPassword[MAX_DB_PASSWORD];
   TCHAR m_szDBALogin[MAX_DB_LOGIN];
   TCHAR m_szDBAPassword[MAX_DB_PASSWORD];
   BOOL m_bCreateDB;
   BOOL m_bInitDB;
   BOOL m_bRunAutoDiscovery;
   DWORD m_dwDiscoveryPI;        // Discovery polling interval
   DWORD m_dwStatusPI;           // Status polling interval
   DWORD m_dwConfigurationPI;    // Configuration polling interval
   DWORD m_dwNumStatusPollers;
   DWORD m_dwNumConfigPollers;
   BOOL m_bEnableAdminInterface;
   TCHAR m_szSMTPServer[MAX_DB_STRING];
   TCHAR m_szSMTPMailFrom[MAX_DB_STRING];
};


//
// Local includes
//

#include "resource.h"       // main symbols
#include "tools.h"
#include "ConfigWizard.h"


/////////////////////////////////////////////////////////////////////////////
// CNxconfigApp:
// See nxconfig.cpp for the implementation of this class
//

class CNxconfigApp : public CWinApp
{
public:
	CNxconfigApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNxconfigApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

public:
	//{{AFX_MSG(CNxconfigApp)
	afx_msg void OnAppAbout();
	afx_msg void OnFileCfgWizard();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////


//
// Wizard worker thread starter
//

void StartWizardWorkerThread(HWND hWnd, WIZARD_CFG_INFO *pc);


//
// Global variables
//

extern CNxconfigApp theApp;
extern TCHAR *g_pszDBEngines[];


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NXCONFIG_H__7266C5B0_B724_4590_BD60_4B60C86A7561__INCLUDED_)
