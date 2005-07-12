// ServerCfgEditor.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ServerCfgEditor.h"
#include "EditVariableDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServerCfgEditor

IMPLEMENT_DYNCREATE(CServerCfgEditor, CMDIChildWnd)

CServerCfgEditor::CServerCfgEditor()
{
}

CServerCfgEditor::~CServerCfgEditor()
{
}


BEGIN_MESSAGE_MAP(CServerCfgEditor, CMDIChildWnd)
	//{{AFX_MSG_MAP(CServerCfgEditor)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_COMMAND(ID_VARIABLE_EDIT, OnVariableEdit)
	ON_UPDATE_COMMAND_UI(ID_VARIABLE_DELETE, OnUpdateVariableDelete)
	ON_UPDATE_COMMAND_UI(ID_VARIABLE_EDIT, OnUpdateVariableEdit)
	ON_COMMAND(ID_VARIABLE_NEW, OnVariableNew)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_VARIABLE_DELETE, OnVariableDelete)
	//}}AFX_MSG_MAP
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_VIEW, OnListViewDblClk)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServerCfgEditor message handlers

BOOL CServerCfgEditor::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         NULL, 
                                         GetSysColorBrush(COLOR_WINDOW), 
                                         AfxGetApp()->LoadIcon(IDI_SETUP));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CServerCfgEditor::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   theApp.OnViewCreate(IDR_SERVER_CFG_EDITOR, this);
	
   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
                        rect, this, IDC_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT |
                                  LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 200);
   m_wndListCtrl.InsertColumn(1, _T("Value"), LVCFMT_LEFT, 300);
   m_wndListCtrl.InsertColumn(2, _T("Restart"), LVCFMT_LEFT, 50);

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);

	return 0;
}


//
// WM_DESTROY message handler
//

void CServerCfgEditor::OnDestroy() 
{
   theApp.OnViewDestroy(IDR_SERVER_CFG_EDITOR, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_SIZE message handler
//

void CServerCfgEditor::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_SETFOCUS message handler
//

void CServerCfgEditor::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
   m_wndListCtrl.SetFocus();	
}


//
// WM_CONTEXTMENU message handler
//

void CServerCfgEditor::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CMenu *pMenu;

   pMenu = theApp.GetContextMenu(15);
   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this, NULL);
}


//
// Item comparision function
//

static int CALLBACK CompareItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
   TCHAR szText1[MAX_OBJECT_NAME], szText2[MAX_OBJECT_NAME];

   ((CListCtrl *)lParamSort)->GetItemText(lParam1, 0, szText1, MAX_OBJECT_NAME);
   ((CListCtrl *)lParamSort)->GetItemText(lParam2, 0, szText2, MAX_OBJECT_NAME);
   return _tcsicmp(szText1, szText2);
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CServerCfgEditor::OnViewRefresh() 
{
   DWORD i, dwResult, dwNumVars;
   NXC_SERVER_VARIABLE *pVarList;

   m_wndListCtrl.DeleteAllItems();
   dwResult = DoRequestArg3(NXCGetServerVariables, g_hSession, &pVarList,
                            &dwNumVars, _T("Loading server's configuration variables..."));
   if (dwResult == RCC_SUCCESS)
   {
      for(i = 0; i < dwNumVars; i++)
         AddItem(&pVarList[i]);
      safe_free(pVarList);
      m_wndListCtrl.SortItems(CompareItems, (LPARAM)&m_wndListCtrl);
   }
   else
   {
      theApp.ErrorBox(dwResult, _T("Error loading server variables: %s"));
   }
}


//
// Add new item to list
//

void CServerCfgEditor::AddItem(NXC_SERVER_VARIABLE *pVar)
{
   int iItem;

   iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, pVar->szName);
   if (iItem != -1)
   {
      m_wndListCtrl.SetItemText(iItem, 1, pVar->szValue);
      m_wndListCtrl.SetItemText(iItem, 2, pVar->bNeedRestart ? _T("Yes") : _T("No"));
      m_wndListCtrl.SetItemData(iItem, iItem);
   }
}


//
// UI update handlers
//

void CServerCfgEditor::OnUpdateVariableDelete(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() > 0);	
}

void CServerCfgEditor::OnUpdateVariableEdit(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() == 1);	
}


//
// WM_COMMAND::ID_VARIABLE_EDIT message handler
//

void CServerCfgEditor::OnVariableEdit() 
{
   CEditVariableDlg dlg;
   int iItem;
   TCHAR szName[MAX_OBJECT_NAME], szValue[MAX_DB_STRING];
   DWORD dwResult;

   iItem = m_wndListCtrl.GetSelectionMark();
   if (iItem != -1)
   {
      m_wndListCtrl.GetItemText(iItem, 0, szName, MAX_OBJECT_NAME);
      m_wndListCtrl.GetItemText(iItem, 1, szValue, MAX_DB_STRING);
      dlg.m_strName = szName;
      dlg.m_strValue = szValue;
      if (dlg.DoModal() == IDOK)
      {
         _tcsncpy(szValue, (LPCTSTR)dlg.m_strValue, MAX_DB_STRING);
         dwResult = DoRequestArg3(NXCSetServerVariable, g_hSession,
                                  szName, szValue, _T("Updating configuration variable..."));
         if (dwResult == RCC_SUCCESS)
         {
            m_wndListCtrl.SetItemText(iItem, 1, szValue);
         }
         else
         {
            theApp.ErrorBox(dwResult, _T("Cannot update variable: %s"));
         }
      }
   }
}


//
// WM_COMMAND::ID_VARIABLE_NEW message handler
//

void CServerCfgEditor::OnVariableNew() 
{
   CEditVariableDlg dlg;
   LVFINDINFO lvfi;
   TCHAR szName[MAX_OBJECT_NAME], szValue[MAX_DB_STRING];
   DWORD dwResult;

   dlg.m_bNewVariable = TRUE;
   if (dlg.DoModal() == IDOK)
   {
      _tcsncpy(szName, (LPCTSTR)dlg.m_strName, MAX_OBJECT_NAME);
      _tcsncpy(szValue, (LPCTSTR)dlg.m_strValue, MAX_DB_STRING);

      lvfi.flags = LVFI_STRING;
      lvfi.psz = szName;
      if (m_wndListCtrl.FindItem(&lvfi) == -1)
      {
         dwResult = DoRequestArg3(NXCSetServerVariable, g_hSession,
                                  szName, szValue, _T("Creating configuration variable..."));
         if (dwResult == RCC_SUCCESS)
         {
            PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
         }
         else
         {
            theApp.ErrorBox(dwResult, _T("Cannot create variable: %s"));
         }
      }
      else
      {
         MessageBox(_T("Variable with given name already exist"),
                    _T("Error"), MB_OK | MB_ICONSTOP);
      }
   }
}


//
// Variable deletion worker function
//

static DWORD DeleteVariables(CListCtrl *pListCtrl, int iNumItems, int *piItemList)
{
   int i;
   DWORD dwResult = RCC_SUCCESS;
   TCHAR szName[MAX_OBJECT_NAME];

   // Delete variables on server
   for(i = 0; (i < iNumItems) && (dwResult == RCC_SUCCESS); i++)
   {
      pListCtrl->GetItemText(piItemList[i], 0, szName, MAX_OBJECT_NAME);
      dwResult = NXCDeleteServerVariable(g_hSession, szName);
   }

   // Delete list view items
   for(i--; i >= 0; i--)
      pListCtrl->DeleteItem(piItemList[i]);

   return dwResult;
}


//
// WM_COMMAND::ID_VARIABLE_DELETE message handler
//

void CServerCfgEditor::OnVariableDelete() 
{
   int i, iItem, iNumItems, *piItemList;
   DWORD dwResult;

   iNumItems = m_wndListCtrl.GetSelectedCount();
   if (iNumItems > 0)
   {
      piItemList = (int *)malloc(sizeof(int) * iNumItems);
      iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
      for(i = 0; (iItem != -1) && (i < iNumItems); i++)
      {
         piItemList[i] = iItem;
         iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
      }
      dwResult = DoRequestArg3(DeleteVariables, &m_wndListCtrl, (void *)iNumItems,
                               piItemList, _T("Deleting variable(s)..."));
      if (dwResult != RCC_SUCCESS)
         theApp.ErrorBox(dwResult, _T("Cannot delete variable: %s"));
      free(piItemList);
   }
}


//
// Handler for WM_NOTIFY::NM_DBLCLK from IDC_LIST_VIEW
//

void CServerCfgEditor::OnListViewDblClk(LPNMITEMACTIVATE pNMHDR, LRESULT *pResult)
{
   PostMessage(WM_COMMAND, ID_VARIABLE_EDIT, 0);
   *pResult = 0;
}
