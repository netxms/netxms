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
// Constants
//

#define STATUS_LINE_HEIGHT       25


//
// Operation modes
//

#define MODE_LIST    0
#define MODE_VIEW    1
#define MODE_EDIT    2


/////////////////////////////////////////////////////////////////////////////
// CScriptView

CScriptView::CScriptView()
{
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
	ON_WM_PAINT()
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

	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   GetClientRect(&rect);

   // Create font
   m_fontStatus.CreateFont(-MulDiv(8, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                           0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                           OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                           VARIABLE_PITCH | FF_DONTCARE, "Verdana");

   // Create image list
   m_imageList.Create(32, 32, ILC_COLOR8 | ILC_MASK, 4, 4);
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_CLOSED_FOLDER));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_SCRIPT));

   // Create list control
   m_wndListCtrl.Create(WS_CHILD, rect, this, ID_LIST_VIEW);
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_NORMAL);
   m_wndListCtrl.InsertColumn(0, _T("Name"));

   // Create editor
   m_wndEditor.Create(WS_CHILD | ES_MULTILINE, rect, this, ID_EDIT_BOX);
   m_wndEditor.SetFont(&g_fontCode);

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
	CWnd::OnSize(nType, cx, cy);
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
   m_wndEditor.SetWindowPos(NULL, 0, STATUS_LINE_HEIGHT, cx, cy - STATUS_LINE_HEIGHT, SWP_NOZORDER);
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
   m_wndListCtrl.ShowWindow(SW_HIDE);
}


//
// WM_COMMAND::ID_SCRIPT_EDIT
//

void CScriptView::OnScriptEdit() 
{
   DWORD dwResult;
   TCHAR *pszText;
   RECT rect;

   if (m_nMode != MODE_VIEW)
      return;

   dwResult = DoRequestArg3(NXCGetScript, g_hSession, (void *)m_dwScriptId,
                            &pszText, _T("Loading script..."));
   if (dwResult == RCC_SUCCESS)
   {
      m_nMode = MODE_EDIT;
      m_bIsModified = FALSE;
      m_wndEditor.SetWindowText(pszText);
      free(pszText);
      m_wndEditor.ShowWindow(SW_SHOW);
      m_wndButton.ShowWindow(SW_HIDE);
      m_wndEditor.SetFocus();
      GetClientRect(&rect);
      rect.bottom = STATUS_LINE_HEIGHT;
      InvalidateRect(&rect);
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

   if ((m_nMode != MODE_EDIT) || (!m_bIsModified))
      return TRUE;

   nRet = MessageBox(_T("Script is modified. Do you want to save changes?"),
                     _T("Warning"), MB_YESNOCANCEL | MB_ICONEXCLAMATION);
   switch(nRet)
   {
      case IDYES:
         m_wndEditor.GetWindowText(strText);
         dwResult = DoRequestArg4(NXCUpdateScript, g_hSession,
                                  (void *)m_dwScriptId, (void *)((LPCTSTR)m_strScriptName),
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
   RECT rect;

   if (!m_bIsModified)
   {
      m_bIsModified = TRUE;
      GetClientRect(&rect);
      rect.bottom = STATUS_LINE_HEIGHT;
      InvalidateRect(&rect);
   }
}


//
// WM_PAINT message handler
//

void CScriptView::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
   CFont *pOldFont;
   RECT rect;

   if (m_nMode != MODE_EDIT)
      return;

   pOldFont = dc.SelectObject(&m_fontStatus);
   dc.SetTextColor(RGB(0, 0, 128));
   dc.TextOut(4, 4, m_strScriptName);

   if (m_bIsModified)
   {
      GetClientRect(&rect);
      dc.SetTextColor(RGB(200, 0, 0));
      dc.TextOut(rect.right - 100, 4, _T("  Modified"), 10);
   }

   dc.SelectObject(pOldFont);
}
