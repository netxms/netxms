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
#include "EventEditor.h"
#include "ObjectBrowser.h"
#include "MapFrame.h"
#include "DebugFrame.h"
#include "NodePropsGeneral.h"
#include "ObjectPropCaps.h"
#include "ObjectPropSheet.h"
#include "RequestProcessingDlg.h"
#include "ObjectPropsGeneral.h"
#include "ObjectPropsSecurity.h"
#include "ObjectPropsPresentation.h"
#include "UserEditor.h"
#include "NetSummaryFrame.h"
#include "DataCollectionEditor.h"
#include "DCIDataView.h"
#include "GraphFrame.h"
#include "EventPolicyEditor.h"
#include "AlarmBrowser.h"
#include "ConsolePropsGeneral.h"
#include "ActionEditor.h"
#include "TrapEditor.h"
#include "PackageMgr.h"


#define MAX_DC_EDITORS     1024


//
// Key states
//

#define KS_SHIFT_PRESSED   0x0001
#define KS_ALT_PRESSED     0x0002
#define KS_CTRL_PRESSED    0x0004


//
// Open DCI editor structure
//

struct DC_EDITOR
{
   DWORD dwNodeId;
   CWnd *pWnd;
};


/////////////////////////////////////////////////////////////////////////////
// CConsoleApp:
// See nxcon.cpp for the implementation of this class
//

class CConsoleApp : public CWinApp
{
public:
	CConsoleApp();
   virtual ~CConsoleApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConsoleApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
protected:
	void CreateChildFrameWithSubtitle(CMDIChildWnd *pWnd, UINT nId, TCHAR *pszSubTitle, HMENU hMenu, HACCEL hAccel);
	void CreateObject(NXC_OBJECT_CREATE_INFO *pInfo);
	HMENU LoadAppMenu(HMENU hViewMenu);
	BOOL SetupWorkDir(void);
	CWnd * FindOpenDCEditor(DWORD dwNodeId);
	CMenu m_ctxMenu;
	DWORD m_dwClientState;
   CActionEditor *m_pwndActionEditor;
   CTrapEditor *m_pwndTrapEditor;
	CAlarmBrowser *m_pwndAlarmBrowser;
	CEventBrowser *m_pwndEventBrowser;
   CEventEditor *m_pwndEventEditor;
   CUserEditor *m_pwndUserEditor;
	CWnd* m_pwndObjectBrowser;
	CWnd *m_pwndCtrlPanel;
   CDebugFrame *m_pwndDebugWindow;
   CNetSummaryFrame *m_pwndNetSummary;
   CEventPolicyEditor *m_pwndEventPolicyEditor;
   CPackageMgr *m_pwndPackageMgr;

   HMENU m_hMDIMenu;             // Default menu for MDI
	HACCEL m_hMDIAccel;           // Default accelerator for MDI
	HMENU m_hAlarmBrowserMenu;    // Menu for alarm browser
	HACCEL m_hAlarmBrowserAccel;  // Accelerator for alarm browser
	HMENU m_hEventBrowserMenu;    // Menu for event browser
	HACCEL m_hEventBrowserAccel;  // Accelerator for event browser
	HMENU m_hObjectBrowserMenu;   // Menu for object browser
	HACCEL m_hObjectBrowserAccel; // Accelerator for object browser
	HMENU m_hEventEditorMenu;     // Menu for event editor
	HACCEL m_hEventEditorAccel;   // Accelerator for event editor
	HMENU m_hUserEditorMenu;      // Menu for user editor
	HACCEL m_hUserEditorAccel;    // Accelerator for user editor
	HMENU m_hDCEditorMenu;        // Menu for data collection editor
	HACCEL m_hDCEditorAccel;      // Accelerator for data collection editor
	HMENU m_hPolicyEditorMenu;    // Menu for event policy editor
	HACCEL m_hPolicyEditorAccel;  // Accelerator for event policy editor
	HMENU m_hMapMenu;             // Menu for map view
	HACCEL m_hMapAccel;           // Accelerator for map view
	HMENU m_hTrapEditorMenu;      // Menu for trap editor
	HACCEL m_hTrapEditorAccel;    // Accelerator for trap editor
	HMENU m_hActionEditorMenu;    // Menu for user editor
	HACCEL m_hActionEditorAccel;  // Accelerator for user editor
	HMENU m_hGraphMenu;           // Menu for history and real-time graphs
	HACCEL m_hGraphAccel;         // Accelerator for history and real-time graphs
	HMENU m_hPackageMgrMenu;      // Menu for package manager
	HACCEL m_hPackageMgrAccel;    // Accelerator for package manager
	HMENU m_hLastValuesMenu;      // Menu for last values view
	HACCEL m_hLastValuesAccel;    // Accelerator for last values view
	
public:
	void EventHandler(DWORD dwEvent, DWORD dwCode, void *pArg);
	void OnViewDestroy(DWORD dwView, CWnd *pWnd, DWORD dwArg = 0);
	void OnViewCreate(DWORD dwView, CWnd *pWnd, DWORD dwArg = 0);
   DWORD GetClientState(void) { return m_dwClientState; }
	//{{AFX_MSG(CConsoleApp)
	afx_msg void OnAppAbout();
	afx_msg void OnViewControlpanel();
	afx_msg void OnViewEvents();
	afx_msg void OnViewMap();
	afx_msg void OnConnectToServer();
	afx_msg void OnViewObjectbrowser();
	afx_msg void OnControlpanelEvents();
	afx_msg void OnViewDebug();
	afx_msg void OnControlpanelUsers();
	afx_msg void OnViewNetworksummary();
	afx_msg void OnToolsMibbrowser();
	afx_msg void OnControlpanelEventpolicy();
	afx_msg void OnViewAlarms();
	afx_msg void OnFileSettings();
	afx_msg void OnControlpanelActions();
	afx_msg void OnToolsAddnode();
	afx_msg void OnControlpanelSnmptraps();
	afx_msg void OnControlpanelAgentpkg();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	BOOL m_bActionEditorActive;
	BOOL m_bTrapEditorActive;
	BOOL m_bAlarmBrowserActive;
	BOOL m_bEventBrowserActive;
	BOOL m_bEventEditorActive;
	BOOL m_bUserEditorActive;
	BOOL m_bObjectBrowserActive;
	BOOL m_bCtrlPanelActive;
   BOOL m_bDebugWindowActive;
   BOOL m_bNetSummaryActive;
   BOOL m_bEventPolicyEditorActive;
   BOOL m_bPackageMgrActive;

   DC_EDITOR m_openDCEditors[MAX_DC_EDITORS];

public:
	CMDIChildWnd *ShowEventBrowser(void);
	CMDIChildWnd *ShowAlarmBrowser(TCHAR *pszParams = NULL);
	CMDIChildWnd *ShowNetworkSummary(void);
	void ApplyTemplate(NXC_OBJECT *pObject);
	CMDIChildWnd *ShowLastValues(NXC_OBJECT *pObject, TCHAR *pszParams = NULL);
	void DeployPackage(DWORD dwPkgId, DWORD dwNumObjects, DWORD *pdwObjectList);
	void CreateNetworkService(DWORD dwParent);
	void WakeUpNode(DWORD dwObjectId);
	void CreateTemplateGroup(DWORD dwParent);
	void CreateTemplate(DWORD dwParent);
	void PollNode(DWORD dwObjectId, int iPollType);
	void DeleteNetXMSObject(NXC_OBJECT *pObject);
	void CreateNode(DWORD dwParent);
	void CreateContainer(DWORD dwParent);
   NXC_EPP *m_pEventPolicy;

	CMDIChildWnd *ShowDCIGraph(DWORD dwNodeId, DWORD dwNumItems, NXC_DCI **ppItemList,
                              TCHAR *pszItemName, TCHAR *pszParams = NULL);
	CMDIChildWnd *ShowDCIData(DWORD dwNodeId, DWORD dwItemId, char *pszItemName, TCHAR *pszParams = NULL);
	void ErrorBox(DWORD dwError, TCHAR *pszMessage = NULL, TCHAR *pszTitle = NULL);
	void SetObjectMgmtStatus(NXC_OBJECT *pObject, BOOL bIsManaged);
	void StartObjectDCEditor(NXC_OBJECT *pObject);
	CMenu * GetContextMenu(int iIndex);
	void ObjectProperties(DWORD dwObjectId);
	void DebugPrintf(char *szFormat, ...);
   void DebugCallback(char *pszMsg)
   {
      if (m_bDebugWindowActive)
         m_pwndDebugWindow->AddMessage(pszMsg);
   }
   CAlarmBrowser *GetAlarmBrowser(void) { return m_bAlarmBrowserActive ? m_pwndAlarmBrowser : NULL; }
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NXCON_H__A5C08A87_42D8_47F3_8F69_9566830EF29E__INCLUDED_)
