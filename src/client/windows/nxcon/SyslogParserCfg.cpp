// SyslogParserCfg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "SyslogParserCfg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Constants
//

#define MAX_LINE_SIZE      4096

#define PANE_MESSAGE       0
#define PANE_LINE          1
#define PANE_COLUMN        2
#define PANE_MODIFY        3


/////////////////////////////////////////////////////////////////////////////
// CSyslogParserCfg

IMPLEMENT_DYNCREATE(CSyslogParserCfg, CMDIChildWnd)

CSyslogParserCfg::CSyslogParserCfg()
{
}

CSyslogParserCfg::~CSyslogParserCfg()
{
}


BEGIN_MESSAGE_MAP(CSyslogParserCfg, CMDIChildWnd)
	//{{AFX_MSG_MAP(CSyslogParserCfg)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_WM_TIMER()
	ON_COMMAND(ID_EDIT_SELECT_ALL, OnEditSelectAll)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
	ON_COMMAND(ID_EDIT_DELETE, OnEditDelete)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateEditDelete)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
	ON_COMMAND(ID_PARSER_SAVE, OnParserSave)
	ON_UPDATE_COMMAND_UI(ID_PARSER_SAVE, OnUpdateParserSave)
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
	ON_EN_CHANGE(ID_EDIT_CTRL, OnEditCtrlChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSyslogParserCfg message handlers


//
// WM_CREATE message handler
//

int CSyslogParserCfg::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	RECT rect;
   static int widths[4] = { 60, 120, 220, -1 };

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   m_pCtxMenu = new CMenu;
   m_pCtxMenu->CreatePopupMenu();
   CopyMenuItems(m_pCtxMenu, theApp.GetContextMenu(16));
   m_pCtxMenu->AppendMenu(MF_SEPARATOR);
   CopyMenuItems(m_pCtxMenu, theApp.GetContextMenu(17));

   GetClientRect(&rect);

   // Create status bar
	m_wndStatusBar.Create(WS_CHILD | WS_VISIBLE, rect, this, IDC_STATUS_BAR);
   m_wndStatusBar.SetParts(4, widths);
   m_iStatusBarHeight = GetWindowSize(&m_wndStatusBar).cy;
   rect.bottom -= m_iStatusBarHeight;

   // Create edit control
   m_wndEditor.Create(_T("Edit"), WS_CHILD | WS_VISIBLE, rect, this, ID_EDIT_CTRL);
   //m_wndEditor.LoadLexer("nxlexer.dll");
   m_wndEditor.SetLexer("xml");
   m_wndEditor.SetKeywords(0, g_szLogParserKeywords);

   m_dwTimer = SetTimer(1, 1000, NULL);

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
	
   theApp.OnViewCreate(VIEW_SYSLOG_PARSER_CFG, this);
	return 0;
}


//
// WM_DESTROY message handler
//

void CSyslogParserCfg::OnDestroy() 
{
   KillTimer(m_dwTimer);
   theApp.OnViewDestroy(VIEW_SYSLOG_PARSER_CFG, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_SIZE message handler
//

void CSyslogParserCfg::OnSize(UINT nType, int cx, int cy) 
{
   RECT rect;
   int nShift;

	CMDIChildWnd::OnSize(nType, cx, cy);

   m_wndStatusBar.SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOZORDER);
   m_wndEditor.SetWindowPos(NULL, 0, 0, cx, cy - m_iStatusBarHeight, SWP_NOZORDER);
   
   nShift = GetSystemMetrics(SM_CXVSCROLL);
   m_wndStatusBar.GetClientRect(&rect);
   int widths[4] = { rect.right - 170 - nShift, rect.right - 120 - nShift, 
                     rect.right - 70 - nShift, rect.right -  nShift };
   m_wndStatusBar.SetParts(4, widths);
}


//
// WM_SETFOCUS message handler
//

void CSyslogParserCfg::OnSetFocus(CWnd* pOldWnd) 
{
	m_wndEditor.SetFocus();
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CSyslogParserCfg::OnViewRefresh() 
{
   DWORD dwResult;
   TCHAR *value;

   if (m_wndEditor.GetModify())
   {
      if (MessageBox(_T("All changes made since last save will be discarded. Are you sure?"),
                     _T("Warning"), MB_YESNO | MB_ICONEXCLAMATION) != IDYES)
      {
         return;
      }
   }

   dwResult = DoRequestArg3(NXCGetServerConfigCLOB, g_hSession, _T("SyslogParser"),
                            &value, _T("Loading syslog parser configuration..."));
	if (dwResult == RCC_UNKNOWN_VARIABLE)
	{
		value = _tcsdup(_T("<parser>\n</parser>\n"));
		dwResult = RCC_SUCCESS;
	}
   if (dwResult == RCC_SUCCESS)
   {
      m_wndEditor.SetText(value);
      m_wndEditor.EmptyUndoBuffer();
      free(value);
      m_wndStatusBar.SetText(_T(""), PANE_MODIFY, 0);
      WriteStatusMsg(_T("Loaded successfully"));
   }
   else
   {
      m_wndEditor.SetWindowText(_T(""));
      theApp.ErrorBox(dwResult, _T("Error loading syslog parser configuration: %s"));
      WriteStatusMsg(_T("Error loading syslog parser configuration"));
   }
}


//
// Write message to the status bar and set timer
//

void CSyslogParserCfg::WriteStatusMsg(const TCHAR *pszMsg)
{
   m_wndStatusBar.SetText(pszMsg, PANE_MESSAGE, 0);
   m_iMsgTimer = 0;
}


//
// WM_TIMER message handler
//

void CSyslogParserCfg::OnTimer(UINT_PTR nIDEvent) 
{
   m_iMsgTimer++;
   if (m_iMsgTimer == 60)
      m_wndStatusBar.SetText(_T(""), PANE_MESSAGE, 0);
}


//
// Editor commands
//

void CSyslogParserCfg::OnEditSelectAll() 
{
   m_wndEditor.SelectAll();
}

void CSyslogParserCfg::OnEditCopy() 
{
   m_wndEditor.Copy();
}

void CSyslogParserCfg::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
   //pCmdUI->Enable((m_wndEditor.GetSelectionType() & (SEL_TEXT | SEL_MULTICHAR)) ? TRUE : FALSE);
}

void CSyslogParserCfg::OnEditCut() 
{
   m_wndEditor.Cut();
}

void CSyslogParserCfg::OnUpdateEditCut(CCmdUI* pCmdUI) 
{
   //pCmdUI->Enable((m_wndEditor.GetSelectionType() & (SEL_TEXT | SEL_MULTICHAR)) ? TRUE : FALSE);
}

void CSyslogParserCfg::OnEditDelete() 
{
   m_wndEditor.Clear();
}

void CSyslogParserCfg::OnUpdateEditDelete(CCmdUI* pCmdUI) 
{
}

void CSyslogParserCfg::OnEditPaste() 
{
   m_wndEditor.Paste();
}

void CSyslogParserCfg::OnUpdateEditPaste(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndEditor.CanPaste());
}

void CSyslogParserCfg::OnEditRedo() 
{
   m_wndEditor.Redo();
}

void CSyslogParserCfg::OnUpdateEditRedo(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndEditor.CanRedo());
}

void CSyslogParserCfg::OnEditUndo() 
{
   m_wndEditor.Undo();
}

void CSyslogParserCfg::OnUpdateEditUndo(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndEditor.CanUndo());
}


//
// Handle edit control changes
//

void CSyslogParserCfg::OnEditCtrlChange()
{
   m_wndStatusBar.SetText(_T("Modified"), PANE_MODIFY, 0);
}


//
// Save parser configuration
//

BOOL CSyslogParserCfg::SaveConfig()
{
   DWORD dwResult;
   CString strText;
   BOOL bResult = FALSE;

   m_wndEditor.GetText(strText);
   dwResult = DoRequestArg3(NXCSetServerConfigCLOB, g_hSession, _T("SyslogParser"),
                            (void *)((LPCTSTR)strText), _T("Updating syslog parser configuration..."));
   if (dwResult == RCC_SUCCESS)
   {
      m_wndEditor.SetSavePoint();
      m_wndStatusBar.SetText(_T(""), PANE_MODIFY, 0);
      bResult = TRUE;
      WriteStatusMsg(_T("Saved successfully"));
   }
   else
   {
      theApp.ErrorBox(dwResult, _T("Error updating syslog parser configuration: %s"));
      WriteStatusMsg(_T("Error updating syslog parser configuration"));
   }
   return bResult;
}


//
// Handler for Parser -> Save menu
//

void CSyslogParserCfg::OnParserSave() 
{
	SaveConfig();
}

void CSyslogParserCfg::OnUpdateParserSave(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndEditor.GetModify());
}


//
// WM_CLOSE message handler
//

void CSyslogParserCfg::OnClose() 
{
   BOOL bAllowClose = TRUE;

   if (m_wndEditor.GetModify())
   {
      int action;

      action = MessageBox(_T("Configuration not saved. Do you wish to save it before exit?"),
		                    _T("Warning"), MB_YESNOCANCEL | MB_ICONEXCLAMATION);
      switch(action)
      {
         case IDYES:
            bAllowClose = SaveConfig();
            break;
         case IDCANCEL:
            bAllowClose = FALSE;
            break;
         default:
            break;
      }
   }
	
   if (bAllowClose)
	   CMDIChildWnd::OnClose();
}
