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
#include <nxwinui.h>
#include "resource.h"      // Main symbols
#include "globals.h"       // Global symbols
#include "DataCollectionEditor.h"
#include "DebugFrame.h"
#include "AlarmBrowser.h"


//
// Forward class definition
//

class CServerCfgEditor;
class CScriptManager;
class CTrapEditor;


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
// View ID
//

enum
{
   VIEW_ALARMS = 0,
   VIEW_EVENT_LOG,
   VIEW_SYSLOG,
   VIEW_CTRLPANEL,
   VIEW_TRAP_LOG,
   VIEW_OBJECTS,
   VIEW_EVENT_EDITOR,
   VIEW_ACTION_EDITOR,
   VIEW_TRAP_EDITOR,
   VIEW_USER_MANAGER,
   VIEW_DEBUG,
   VIEW_NETWORK_SUMMARY,
   VIEW_EPP_EDITOR,
   VIEW_PACKAGE_MANAGER,
   VIEW_SERVER_CONFIG,
   VIEW_OBJECT_TOOLS,
   VIEW_LPP_EDITOR,
   VIEW_SCRIPT_MANAGER,
   VIEW_BUILDER,
   VIEW_MODULE_MANAGER,
   VIEW_DESKTOP_MANAGER,
   VIEW_AGENT_CONFIG_MANAGER,
   MAX_VIEW_ID
};


//
// View activity information
//

struct CONSOLE_VIEW
{
   BOOL bActive;
   CMDIChildWnd *pWnd;
   HWND hWnd;
};


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
	BOOL m_bIgnoreErrors;
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
	HMENU m_hScriptManagerMenu;   // Menu for script manager
	HACCEL m_hScriptManagerAccel; // Accelerator for script manager
	HMENU m_hViewBuilderMenu;     // Menu for view builder
	HACCEL m_hViewBuilderAccel;   // Accelerator for view builder
	HMENU m_hDataViewMenu;        // Menu for DCI data viewer
	HACCEL m_hDataViewAccel;      // Accelerator for DCI data viewer
	HMENU m_hAgentCfgMgrMenu;     // Menu for agent configuration manager
	HACCEL m_hAgentCfgMgrAccel;   // Accelerator for agent configuration manager
	
public:
	void EventHandler(DWORD dwEvent, DWORD dwCode, void *pArg);
	void OnViewDestroy(DWORD dwView, CMDIChildWnd *pWnd, DWORD dwArg = 0);
	void OnViewCreate(DWORD dwView, CMDIChildWnd *pWnd, DWORD dwArg = 0);
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
	afx_msg void OnControlpanelScriptlibrary();
	afx_msg void OnViewSnmptraplog();
	afx_msg void OnControlpanelViewbuilder();
	afx_msg void OnControlpanelModules();
	afx_msg void OnDesktopManage();
	afx_msg void OnToolsChangepassword();
	afx_msg void OnControlpanelAgentconfigs();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
   CONSOLE_VIEW m_viewState[MAX_VIEW_ID];
   DC_EDITOR m_openDCEditors[MAX_DC_EDITORS];

public:
	void CreateCondition(DWORD dwParent);
	void MoveObject(DWORD dwObjectId, DWORD dwParentId);
	void StartWebBrowser(TCHAR *pszURL);
	void ExecuteObjectTool(NXC_OBJECT *pObject, DWORD dwIndex);
	void CreateVPNConnector(DWORD dwParent);
	void ExportDCIData(DWORD dwNodeId, DWORD dwItemId, DWORD dwTimeFrom, DWORD dwTimeTo, int iSeparator, int iTimeStampFormat, const TCHAR *pszFile);
	CMDIChildWnd *ShowObjectBrowser(TCHAR *pszParams = NULL);
	void ChangeNodeAddress(DWORD dwNodeId);
	void UnbindObject(NXC_OBJECT *pObject);
	void BindObject(NXC_OBJECT *pObject);
	CMDIChildWnd *ShowControlPanel(void);
	CMDIChildWnd *ShowEventBrowser(void);
	CMDIChildWnd *ShowSyslogBrowser(void);
	CMDIChildWnd *ShowTrapLogBrowser(void);
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
	CMDIChildWnd *ShowDCIData(DWORD dwNodeId, DWORD dwItemId,
                             TCHAR *pszItemName, TCHAR *pszParams = NULL);
	void ErrorBox(DWORD dwError, TCHAR *pszMessage = NULL, TCHAR *pszTitle = NULL);
	void SetObjectMgmtStatus(NXC_OBJECT *pObject, BOOL bIsManaged);
	void StartObjectDCEditor(NXC_OBJECT *pObject);
	CMenu * GetContextMenu(int iIndex);
	void ObjectProperties(DWORD dwObjectId);
	void DebugPrintf(TCHAR *szFormat, ...);
   void DebugCallback(TCHAR *pszMsg)
   {
      if (m_viewState[VIEW_DEBUG].bActive)
         ((CDebugFrame *)m_viewState[VIEW_DEBUG].pWnd)->AddMessage(pszMsg);
   }
   CAlarmBrowser *GetAlarmBrowser(void) { return m_viewState[VIEW_ALARMS].bActive ? (CAlarmBrowser *)m_viewState[VIEW_ALARMS].pWnd : NULL; }
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NXCON_H__A5C08A87_42D8_47F3_8F69_9566830EF29E__INCLUDED_)
