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
#include <nxsnmp.h>
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
#include "ServerCfgEditor.h"
#include "ObjectToolsEditor.h"
#include "SyslogBrowser.h"
#include "LPPList.h"


//
// Constants
//

#define MAX_DC_EDITORS     1024
#define LAST_APP_MENU      4


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
   friend DWORD WINAPI LoginThread(void *pArg);
   friend DWORD LoadObjectTools(void);

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
	void ExecuteCmdTool(NXC_OBJECT *pObject, TCHAR *pszCmd);
	void ExecuteWebTool(NXC_OBJECT *pObject, TCHAR *pszURL);
	void ExecuteTableTool(NXC_OBJECT *pNode, DWORD dwToolId);
	void CreateChildFrameWithSubtitle(CMDIChildWnd *pWnd, UINT nId, TCHAR *pszSubTitle, HMENU hMenu, HACCEL hAccel);
	void CreateObject(NXC_OBJECT_CREATE_INFO *pInfo);
	HMENU LoadAppMenu(HMENU hViewMenu);
	BOOL SetupWorkDir(void);
	CWnd * FindOpenDCEditor(DWORD dwNodeId);
	CMenu m_ctxMenu;
	DWORD m_dwClientState;
   HWND m_hwndEventBrowser;
   HWND m_hwndSyslogBrowser;
   CActionEditor *m_pwndActionEditor;
   CTrapEditor *m_pwndTrapEditor;
	CAlarmBrowser *m_pwndAlarmBrowser;
	CEventBrowser *m_pwndEventBrowser;
	CSyslogBrowser *m_pwndSyslogBrowser;
   CEventEditor *m_pwndEventEditor;
   CUserEditor *m_pwndUserEditor;
	CObjectBrowser *m_pwndObjectBrowser;
	CWnd *m_pwndCtrlPanel;
   CDebugFrame *m_pwndDebugWindow;
   CNetSummaryFrame *m_pwndNetSummary;
   CEventPolicyEditor *m_pwndEventPolicyEditor;
   CPackageMgr *m_pwndPackageMgr;
   CServerCfgEditor *m_pwndServerCfgEditor;
   CObjectToolsEditor *m_pwndObjToolsEditor;
   CLPPList *m_pwndLPPEditor;

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
	HMENU m_hLPPEditorMenu;       // Menu for log processing policy editor
	HACCEL m_hLPPEditorAccel;     // Accelerator for log processing policy editor
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
	HMENU m_hServerCfgEditorMenu; // Menu for server configuration editor
	HACCEL m_hServerCfgEditorAccel; // Accelerator for server configuration editor
	HMENU m_hAgentCfgEditorMenu;  // Menu for agent configuration editor
	HACCEL m_hAgentCfgEditorAccel;// Accelerator for agent configuration editor
	HMENU m_hObjToolsEditorMenu;  // Menu for object tools editor
	HACCEL m_hObjToolsEditorAccel;// Accelerator for object tools editor
	
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
	afx_msg void OnControlpanelServercfg();
	afx_msg void OnViewSyslog();
	afx_msg void OnControlpanelLogprocessing();
	afx_msg void OnControlpanelObjecttools();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	BOOL m_bActionEditorActive;
	BOOL m_bTrapEditorActive;
	BOOL m_bAlarmBrowserActive;
	BOOL m_bEventBrowserActive;
	BOOL m_bSyslogBrowserActive;
	BOOL m_bEventEditorActive;
	BOOL m_bUserEditorActive;
	BOOL m_bObjectBrowserActive;
	BOOL m_bCtrlPanelActive;
   BOOL m_bDebugWindowActive;
   BOOL m_bNetSummaryActive;
   BOOL m_bEventPolicyEditorActive;
   BOOL m_bLPPEditorActive;
   BOOL m_bPackageMgrActive;
   BOOL m_bServerCfgEditorActive;
   BOOL m_bObjToolsEditorActive;

   DC_EDITOR m_openDCEditors[MAX_DC_EDITORS];

public:
	void MoveObject(DWORD dwObjectId, DWORD dwParentId);
	void StartWebBrowser(TCHAR *pszURL);
	void ExecuteObjectTool(NXC_OBJECT *pObject, DWORD dwIndex);
	void CreateVPNConnector(DWORD dwParent);
	void ExportDCIData(DWORD dwNodeId, DWORD dwItemId, DWORD dwTimeFrom, DWORD dwTimeTo, int iSeparator, int iTimeStampFormat, const TCHAR *pszFile);
	CMDIChildWnd *ShowObjectBrowser(TCHAR *pszParams = NULL);
	void ChangeNodeAddress(DWORD dwNodeId);
	void UnbindObject(NXC_OBJECT *pObject);
	void BindObject(NXC_OBJECT *pObject);
	CMDIChildWnd *ShowEventBrowser(void);
	CMDIChildWnd *ShowSyslogBrowser(void);
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
	void EditAgentConfig(NXC_OBJECT *pNode);
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
