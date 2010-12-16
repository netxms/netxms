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
#include <nxdbapi.h>


#define MAX_ERROR_TEXT        4096


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
#define DB_ENGINE_ORACLE   3
#define DB_ENGINE_SQLITE   4


//
// Server configuration collected by wizard
//

struct WIZARD_CFG_INFO
{
   TCHAR m_szInstallDir[MAX_PATH];
   BOOL m_bConfigFileDetected;
   TCHAR m_szConfigFile[MAX_PATH];
   int m_iDBEngine;
   TCHAR m_szDBDriver[MAX_PATH];
   TCHAR m_szDBDrvParams[MAX_PATH];
   TCHAR m_szDBServer[MAX_DB_STRING];
   TCHAR m_szDBName[MAX_DB_NAME];
   TCHAR m_szDBLogin[MAX_DB_LOGIN];
   TCHAR m_szDBPassword[MAX_DB_PASSWORD];
   TCHAR m_szDBALogin[MAX_DB_LOGIN];
   TCHAR m_szDBAPassword[MAX_DB_PASSWORD];
   BOOL m_bCreateDB;
   BOOL m_bInitDB;
   BOOL m_bLogFailedSQLQueries;
   BOOL m_bRunAutoDiscovery;
   DWORD m_dwDiscoveryPI;        // Discovery polling interval
   DWORD m_dwStatusPI;           // Status polling interval
   DWORD m_dwConfigurationPI;    // Configuration polling interval
   DWORD m_dwNumStatusPollers;
   DWORD m_dwNumConfigPollers;
   BOOL m_bEnableAdminInterface;
   TCHAR m_szSMTPServer[MAX_DB_STRING];
   TCHAR m_szSMTPMailFrom[MAX_DB_STRING];
   BOOL m_bLogToSyslog;
   TCHAR m_szLogFile[MAX_PATH];
   TCHAR m_szSMSDriver[MAX_PATH];
   TCHAR m_szSMSDrvParam[MAX_DB_STRING];
   TCHAR m_szServiceLogin[MAX_DB_STRING];
   TCHAR m_szServicePassword[MAX_DB_STRING];
   DWORD m_dwDependencyListSize;
   TCHAR *m_pszDependencyList;
	DB_DRIVER m_dbDriver;
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
	afx_msg void OnFileCfgWizard();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
protected:
	void CreateWebConfig(const TCHAR *pszServer);
	TCHAR m_szInstallDir[MAX_PATH];
	void CreateAgentConfig(void);
};


/////////////////////////////////////////////////////////////////////////////


//
// Functions
//

void StartWizardWorkerThread(HWND hWnd, WIZARD_CFG_INFO *pc);
BOOL ExecSQLBatch(DB_HANDLE hConn, TCHAR *pszFile);


//
// Global variables
//

extern CNxconfigApp appNxConfig;
extern TCHAR *g_pszDBEngines[];
extern TCHAR g_szWizardErrorText[];


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NXCONFIG_H__7266C5B0_B724_4590_BD60_4B60C86A7561__INCLUDED_)
