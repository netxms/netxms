// ScriptView.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ScriptView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Operation modes
//

#define MODE_INITIAL    -1
#define MODE_LIST       0
#define MODE_VIEW       1
#define MODE_EDIT       2


/////////////////////////////////////////////////////////////////////////////
// CScriptView

CScriptView::CScriptView()
{
   m_nMode = MODE_INITIAL;
}

CScriptView::~CScriptView()
{
}


BEGIN_MESSAGE_MAP(CScriptView, CWnd)
	//{{AFX_MSG_MAP(CScriptView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND(ID_SCRIPT_EDIT, OnScriptEdit)
	ON_UPDATE_COMMAND_UI(ID_SCRIPT_EDIT, OnUpdateScriptEdit)
	ON_WM_CONTEXTMENU()
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
	ON_COMMAND(ID_EDIT_SELECT_ALL, OnEditSelectAll)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SELECT_ALL, OnUpdateEditSelectAll)
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
	//}}AFX_MSG_MAP
   ON_EN_CHANGE(ID_EDIT_BOX, OnEditorChange)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CScriptView message handlers

BOOL CScriptView::PreCreateWindow(CREATESTRUCT& cs) 
{
   cs.lpszClass = AfxRegisterWndClass(0, LoadCursor(NULL, IDC_ARROW),
                                      CreateSolidBrush(g_rgbFlatButtonBackground));
	
	return CWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CScriptView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   static int widths[4] = { 60, 120, 220, -1 };

	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   GetClientRect(&rect);

   // Create image list
   m_imageList.Create(32, 32, ILC_COLOR8 | ILC_MASK, 4, 4);
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_CLOSED_FOLDER));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_SCRIPT));

   // Create list control
   m_wndListCtrl.Create(WS_CHILD, rect, this, ID_LIST_VIEW);
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_NORMAL);
   m_wndListCtrl.InsertColumn(0, _T("Name"));

   // Create status bar
	m_wndStatusBar.Create(WS_CHILD | SBARS_SIZEGRIP, rect, this, IDC_STATUS_BAR);
   m_wndStatusBar.SetParts(4, widths);
   m_iStatusBarHeight = GetWindowSize(&m_wndStatusBar).cy;
   rect.bottom -= m_iStatusBarHeight;

   // Create editor
   m_wndEditor.Create(_T("Edit"), WS_CHILD, rect, this, ID_EDIT_BOX);
   //m_wndEditor.SetFont(&g_fontCode);
   m_wndEditor.LoadLexer("nxlexer.dll");
   m_wndEditor.SetLexer("nxsl");

   // Create status bar

   // Create "Edit" button
   rect.left += 20;
   rect.top += 20;
   rect.right = rect.left + 80;
   rect.bottom = rect.top + 20;
   m_wndButton.Create(NULL, _T("Edit Script"), WS_CHILD, rect, this, ID_SCRIPT_EDIT);
	
	return 0;
}


//
// WM_SIZE message handler
//

void CScriptView::OnSize(UINT nType, int cx, int cy) 
{
   RECT rect;
   int nShift;

	CWnd::OnSize(nType, cx, cy);

   m_wndStatusBar.SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOZORDER);
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
   m_wndEditor.SetWindowPos(NULL, 0, 0, cx, cy - m_iStatusBarHeight, SWP_NOZORDER);

   nShift = GetSystemMetrics(SM_CXVSCROLL);
   m_wndStatusBar.GetClientRect(&rect);
   int widths[4] = { rect.right - 170 - nShift, rect.right - 120 - nShift, 
                     rect.right - 70 - nShift, rect.right -  nShift };
   m_wndStatusBar.SetParts(4, widths);
}


//
// Switch script view to list mode
//

void CScriptView::SetListMode(CTreeCtrl &wndTreeCtrl, HTREEITEM hRoot)
{
   HTREEITEM hItem;
   DWORD dwId;

   m_nMode = MODE_LIST;

   m_wndListCtrl.DeleteAllItems();
   hItem = wndTreeCtrl.GetChildItem(hRoot);
   while(hItem != NULL)
   {
      dwId = wndTreeCtrl.GetItemData(hItem);
      m_wndListCtrl.InsertItem(0x7FFFFFFF, (LPCTSTR)wndTreeCtrl.GetItemText(hItem),
                               (dwId == 0) ? 0 : 1);
      hItem = wndTreeCtrl.GetNextItem(hItem, TVGN_NEXT);
   }

   m_wndListCtrl.ShowWindow(SW_SHOW);
   m_wndEditor.ShowWindow(SW_HIDE);
   m_wndStatusBar.ShowWindow(SW_HIDE);
   m_wndButton.ShowWindow(SW_HIDE);
}


//
// Switch script view to edit mode
//

void CScriptView::SetEditMode(DWORD dwScriptId, LPCTSTR pszScriptName)
{
   m_nMode = MODE_VIEW;
   m_dwScriptId = dwScriptId;
   m_strScriptName = pszScriptName;

   m_wndButton.ShowWindow(SW_SHOW);
   m_wndEditor.ShowWindow(SW_HIDE);
   m_wndStatusBar.ShowWindow(SW_HIDE);
   m_wndListCtrl.ShowWindow(SW_HIDE);
}


//
// Switch script view to empty mode
//

void CScriptView::SetEmptyMode()
{
   m_nMode = MODE_INITIAL;
   m_wndButton.ShowWindow(SW_HIDE);
   m_wndEditor.ShowWindow(SW_HIDE);
   m_wndStatusBar.ShowWindow(SW_HIDE);
   m_wndListCtrl.ShowWindow(SW_HIDE);
}


//
// WM_COMMAND::ID_SCRIPT_EDIT
//

void CScriptView::OnScriptEdit() 
{
   DWORD dwResult;
   TCHAR *pszText;

   if (m_nMode != MODE_VIEW)
      return;

   dwResult = DoRequestArg3(NXCGetScript, g_hSession, (void *)m_dwScriptId,
                            &pszText, _T("Loading script..."));
   if (dwResult == RCC_SUCCESS)
   {
      m_nMode = MODE_EDIT;
      m_wndEditor.SetText(pszText);
      m_wndEditor.EmptyUndoBuffer();
      free(pszText);
      m_wndEditor.ShowWindow(SW_SHOW);
      m_wndStatusBar.ShowWindow(SW_SHOW);
      m_wndButton.ShowWindow(SW_HIDE);
      m_wndEditor.SetFocus();
   }
   else
   {
      theApp.ErrorBox(dwResult, _T("Cannot load script: %s"));
   }
}

void CScriptView::OnUpdateScriptEdit(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_nMode == MODE_VIEW);
}


//
// Validate if view can be closed
//

BOOL CScriptView::ValidateClose(void)
{
   int nRet;
   BOOL bRet = FALSE;
   CString strText;
   DWORD dwResult;
   TCHAR szMessage[1024];

   if ((m_nMode != MODE_EDIT) || (!m_wndEditor.GetModify()))
      return TRUE;

   _sntprintf(szMessage, 1024, _T("Script %s is modified. Do you want to save changes?"),
              (LPCTSTR)m_strScriptName);
   nRet = MessageBox(szMessage, _T("Warning"), MB_YESNOCANCEL | MB_ICONEXCLAMATION);
   switch(nRet)
   {
      case IDYES:
         m_wndEditor.GetText(strText);
         dwResult = DoRequestArg4(NXCUpdateScript, g_hSession,
                                  &m_dwScriptId, (void *)((LPCTSTR)m_strScriptName),
                                  (void *)((LPCTSTR)strText), _T("Updating script..."));
         if (dwResult == RCC_SUCCESS)
         {
            bRet = TRUE;
         }
         else
         {
            theApp.ErrorBox(dwResult, _T("Cannot update script: %s"));
         }
         break;
      case IDNO:
         bRet = TRUE;
         break;
   }
   return bRet;
}


//
// Handler for editor changes
//

void CScriptView::OnEditorChange()
{
}


//
// WM_CONTEXTMENU message handler
//

void CScriptView::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CMenu *pMenu;

   pMenu = theApp.GetContextMenu(16);
   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this, NULL);
}


//
// Editor commands
//

void CScriptView::OnEditCopy() 
{
   if (m_nMode == MODE_EDIT)
      m_wndEditor.Copy();
}

void CScriptView::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_nMode == MODE_EDIT);
}

void CScriptView::OnEditCut() 
{
   if (m_nMode == MODE_EDIT)
      m_wndEditor.Copy();
}

void CScriptView::OnUpdateEditCut(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_nMode == MODE_EDIT);
}

void CScriptView::OnEditDelete() 
{
   if (m_nMode == MODE_EDIT)
      m_wndEditor.Clear();
}

void CScriptView::OnUpdateEditDelete(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_nMode == MODE_EDIT);
}

void CScriptView::OnEditPaste() 
{
   if (m_nMode == MODE_EDIT)
      m_wndEditor.Paste();
}

void CScriptView::OnUpdateEditPaste(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable((m_nMode == MODE_EDIT) && m_wndEditor.CanPaste());
}

void CScriptView::OnEditRedo() 
{
   if (m_nMode == MODE_EDIT)
      m_wndEditor.Redo();
}

void CScriptView::OnUpdateEditRedo(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable((m_nMode == MODE_EDIT) && m_wndEditor.CanRedo());
}

void CScriptView::OnEditSelectAll() 
{
   if (m_nMode == MODE_EDIT)
      m_wndEditor.SelectAll();
}

void CScriptView::OnUpdateEditSelectAll(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_nMode == MODE_EDIT);
}

void CScriptView::OnEditUndo() 
{
   if (m_nMode == MODE_EDIT)
      m_wndEditor.Undo();
}

void CScriptView::OnUpdateEditUndo(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable((m_nMode == MODE_EDIT) && m_wndEditor.CanUndo());
}
