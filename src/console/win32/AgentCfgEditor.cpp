// AgentCfgEditor.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "AgentCfgEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAgentCfgEditor

IMPLEMENT_DYNCREATE(CAgentCfgEditor, CMDIChildWnd)

CAgentCfgEditor::CAgentCfgEditor()
{
   m_dwNodeId = 0;
}

CAgentCfgEditor::CAgentCfgEditor(DWORD dwNodeId)
{
   m_dwNodeId = dwNodeId;
}

CAgentCfgEditor::~CAgentCfgEditor()
{
}


BEGIN_MESSAGE_MAP(CAgentCfgEditor, CMDIChildWnd)
	//{{AFX_MSG_MAP(CAgentCfgEditor)
	ON_WM_CREATE()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAgentCfgEditor message handlers

BOOL CAgentCfgEditor::PreCreateWindow(CREATESTRUCT& cs) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CAgentCfgEditor::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   CHARFORMAT cf;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   GetClientRect(&rect);
   m_wndEdit.Create(WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOHSCROLL | 
                    ES_AUTOVSCROLL | ES_WANTRETURN | WS_HSCROLL | WS_VSCROLL,
                    rect, this, ID_EDIT_CTRL);
   cf.cbSize = sizeof(CHARFORMAT);
   cf.dwMask = CFM_FACE | CFM_BOLD;
   cf.dwEffects = 0;
   cf.bPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
   _tcscpy(cf.szFaceName, _T("Courier"));
   m_wndEdit.SetDefaultCharFormat(cf);
   m_wndEdit.SetSelectionCharFormat(cf);
   m_wndEdit.SetWordCharFormat(cf);

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
	
	return 0;
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CAgentCfgEditor::OnViewRefresh() 
{
   DWORD dwResult;
   TCHAR *pszConfig;

   dwResult = DoRequestArg3(NXCGetAgentConfig, g_hSession, (void *)m_dwNodeId,
                            &pszConfig, _T("Requesting agent's configuration file..."));
   if (dwResult == RCC_SUCCESS)
   {
      m_wndEdit.SetWindowText(pszConfig);
      free(pszConfig);
   }
   else
   {
      m_wndEdit.SetWindowText(_T(""));
      theApp.ErrorBox(dwResult, _T("Error getting agent's configuration file: %s"));
   }
}


//
// WM_SIZE message handler
//

void CAgentCfgEditor::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
   m_wndEdit.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_SETFOCUS message handler
//

void CAgentCfgEditor::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
   m_wndEdit.SetFocus();	
}
