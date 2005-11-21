// ObjectToolsEditor.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectToolsEditor.h"
#include "ObjToolPropGeneral.h"
#include "ObjToolPropColumns.h"
#include "NewObjectToolDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjectToolsEditor

IMPLEMENT_DYNCREATE(CObjectToolsEditor, CMDIChildWnd)

CObjectToolsEditor::CObjectToolsEditor()
{
   m_dwNumTools = 0;
   m_pToolList = NULL;
}

CObjectToolsEditor::~CObjectToolsEditor()
{
   NXCDestroyObjectToolList(m_dwNumTools, m_pToolList);
}


BEGIN_MESSAGE_MAP(CObjectToolsEditor, CMDIChildWnd)
	//{{AFX_MSG_MAP(CObjectToolsEditor)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_UPDATE_COMMAND_UI(ID_OBJECTTOOLS_DELETE, OnUpdateObjecttoolsDelete)
	ON_UPDATE_COMMAND_UI(ID_OBJECTTOOLS_EDIT, OnUpdateObjecttoolsEdit)
	ON_COMMAND(ID_OBJECTTOOLS_EDIT, OnObjecttoolsEdit)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_OBJECTTOOLS_NEW, OnObjecttoolsNew)
	ON_COMMAND(ID_OBJECTTOOLS_DELETE, OnObjecttoolsDelete)
	//}}AFX_MSG_MAP
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_VIEW, OnListViewDblClk)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjectToolsEditor message handlers

BOOL CObjectToolsEditor::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         NULL, 
                                         GetSysColorBrush(COLOR_WINDOW), 
                                         AfxGetApp()->LoadIcon(IDI_OBJTOOLS));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CObjectToolsEditor::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   theApp.OnViewCreate(IDR_OBJECT_TOOLS_EDITOR, this);
	
   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT |
                        LVS_SHOWSELALWAYS, rect, this, IDC_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);

   // Create image list
   m_imageList.Create(16, 16, ILC_COLOR8 | ILC_MASK, 8, 8);
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_UNKNOWN));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_EXEC));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_DOCUMENT));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_DOCUMENT));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_IEXPLORER));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_UNKNOWN));
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, "ID", LVCFMT_LEFT, 70);
   m_wndListCtrl.InsertColumn(1, "Name", LVCFMT_LEFT, 250);
   m_wndListCtrl.InsertColumn(2, "Type", LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(3, "Description", LVCFMT_LEFT, 300);

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);

   return 0;
}


//
// WM_DESTROY message handler
//

void CObjectToolsEditor::OnDestroy() 
{
   theApp.OnViewDestroy(IDR_OBJECT_TOOLS_EDITOR, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_SETFOCUS message handler
//

void CObjectToolsEditor::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
   m_wndListCtrl.SetFocus();
}


//
// WM_SIZE message handler
//

void CObjectToolsEditor::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_CLOSE message handler
//

void CObjectToolsEditor::OnClose() 
{
   DoRequestArg1(NXCUnlockObjectTools, g_hSession, _T("Unlocking object tools..."));
	CMDIChildWnd::OnClose();
}


//
// Handler for WM_NOTIFY::NM_DBLCLK from IDC_LIST_VIEW
//

void CObjectToolsEditor::OnListViewDblClk(LPNMITEMACTIVATE pNMHDR, LRESULT *pResult)
{
   PostMessage(WM_COMMAND, ID_OBJECTTOOLS_EDIT, 0);
   *pResult = 0;
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CObjectToolsEditor::OnViewRefresh() 
{
   DWORD i, dwResult;
   int iItem;
   TCHAR szBuffer[32];

   m_wndListCtrl.DeleteAllItems();
   NXCDestroyObjectToolList(m_dwNumTools, m_pToolList);

   dwResult = DoRequestArg3(NXCGetObjectTools, g_hSession, &m_dwNumTools,
                            &m_pToolList, _T("Loading list of object tools..."));
   if (dwResult == RCC_SUCCESS)
   {
      for(i = 0; i < m_dwNumTools; i++)
      {
         _stprintf(szBuffer, _T("%d"), m_pToolList[i].dwId);
         iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, szBuffer,
                                          m_pToolList[i].wType);
         if (iItem != -1)
         {
            m_wndListCtrl.SetItemData(iItem, m_pToolList[i].dwId);
            m_wndListCtrl.SetItemText(iItem, 1, m_pToolList[i].szName);
            m_wndListCtrl.SetItemText(iItem, 2, g_szToolType[m_pToolList[i].wType]);
            m_wndListCtrl.SetItemText(iItem, 3, m_pToolList[i].szDescription);
         }
      }
   }
   else
   {
      theApp.ErrorBox(dwResult, _T("Error loading object tools: %s"));
   }
}


//
// UI update handlers
//

void CObjectToolsEditor::OnUpdateObjecttoolsDelete(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() > 0);
}

void CObjectToolsEditor::OnUpdateObjecttoolsEdit(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() == 1);
}


//
// Edit object tool
//

void CObjectToolsEditor::OnObjecttoolsEdit() 
{
   int iItem;
   DWORD dwToolId, dwResult;
   NXC_OBJECT_TOOL_DETAILS *pData;

   iItem = m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED);
   if (iItem != -1)
   {
      dwToolId = m_wndListCtrl.GetItemData(iItem);
      dwResult = DoRequestArg3(NXCGetObjectToolDetails, g_hSession, (void *)dwToolId,
                               &pData, _T("Loading tool data..."));
      if (dwResult == RCC_SUCCESS)
      {
         EditTool(pData);      
      }
      else
      {
         theApp.ErrorBox(dwResult, _T("Cannot load object tool data: %s"));
      }
   }
}


//
// WM_CONTEXTMENU message handler
//

void CObjectToolsEditor::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CMenu *pMenu;

   pMenu = theApp.GetContextMenu(18);
   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this, NULL);
}


//
// Create new object tool
//

void CObjectToolsEditor::OnObjecttoolsNew() 
{
   DWORD dwResult, dwToolId;
   NXC_OBJECT_TOOL_DETAILS *pData;
   CNewObjectToolDlg dlg;
   static WORD m_wTransTbl[6] = { TOOL_TYPE_ACTION, TOOL_TYPE_COMMAND,
                                  TOOL_TYPE_INTERNAL, TOOL_TYPE_TABLE_AGENT,
                                  TOOL_TYPE_TABLE_SNMP, TOOL_TYPE_URL };

   dlg.m_iToolType = 0;
   if (dlg.DoModal() == IDOK)
   {
      dwResult = DoRequestArg2(NXCGenerateObjectToolId, g_hSession, &dwToolId,
                               _T("Generating unique identified for new tool..."));
      if (dwResult == RCC_SUCCESS)
      {
         pData = (NXC_OBJECT_TOOL_DETAILS *)malloc(sizeof(NXC_OBJECT_TOOL_DETAILS));
         memset(pData, 0, sizeof(NXC_OBJECT_TOOL_DETAILS));
         pData->dwId = dwToolId;
         nx_strncpy(pData->szName, (LPCTSTR)dlg.m_strName, MAX_DB_STRING);
         pData->wType = m_wTransTbl[dlg.m_iToolType];
         EditTool(pData);
      }
      else
      {
         theApp.ErrorBox(dwResult, _T("Cannot generate unique ID: %s"));
      }
   }
}


//
// Do actual object tools deletion
//

static DWORD DeleteObjectTools(DWORD dwNumTools, DWORD *pdwToolList,
                               CListCtrl *pwndListCtrl)
{
   DWORD i, dwResult;
   LVFINDINFO lvfi;
   int iItem;

   for(i = 0; i < dwNumTools; i++)
   {
      dwResult = NXCDeleteObjectTool(g_hSession, pdwToolList[i]);
      if (dwResult != RCC_SUCCESS)
         break;
      lvfi.flags = LVFI_PARAM;
      lvfi.lParam = pdwToolList[i];
      iItem = pwndListCtrl->FindItem(&lvfi, -1);
      if (iItem != -1)
         pwndListCtrl->DeleteItem(iItem);
   }
   return dwResult;
}


//
// Delete object tool
//

void CObjectToolsEditor::OnObjecttoolsDelete() 
{
   int iItem;
   DWORD i, dwNumItems, *pdwList, dwResult;

   dwNumItems = m_wndListCtrl.GetSelectedCount();
   if (dwNumItems > 0)
   {
      pdwList = (DWORD *)malloc(sizeof(DWORD) * dwNumItems);
      iItem = m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED);
      for(i = 0; (i < dwNumItems) && (iItem != -1); i++)
      {
         pdwList[i] = m_wndListCtrl.GetItemData(iItem);
         iItem = m_wndListCtrl.GetNextItem(iItem, LVIS_SELECTED);
      }
      dwResult = DoRequestArg3(DeleteObjectTools, (void *)dwNumItems,
                               pdwList, &m_wndListCtrl, _T("Deleting object tools..."));
      if (dwResult != RCC_SUCCESS)
      {
         theApp.ErrorBox(dwResult, _T("Cannot delete object tool: %s"));
      }
      free(pdwList);
   }
}


//
// Edit object tool
//

void CObjectToolsEditor::EditTool(NXC_OBJECT_TOOL_DETAILS *pData)
{
   CObjToolPropGeneral pgGeneral;
   CObjToolPropColumns pgColumns;
   TCHAR szBuffer[1024];
   DWORD dwResult;

   _stprintf(szBuffer, _T("Object Tool Properties (%s)"), g_szToolType[pData->wType]);
   CPropertySheet psh(szBuffer, this, 0);

   // Setup "General" page
   pgGeneral.m_iToolType = pData->wType;
   pgGeneral.m_strName = pData->szName;
   pgGeneral.m_strDescription = pData->szDescription;
   pgGeneral.m_dwACLSize = pData->dwACLSize;
   pgGeneral.m_pdwACL = (DWORD *)nx_memdup(pData->pdwACL, sizeof(DWORD) * pData->dwACLSize);
   if (pData->wType == TOOL_TYPE_TABLE_AGENT)
   {
      TCHAR *pszBuffer, *pCurr, *pEnd;

      pszBuffer = _tcsdup(CHECK_NULL_EX(pData->pszData));
      pEnd = _tcschr(pszBuffer, _T('\x7F'));
      if (pEnd != NULL)
         *pEnd = 0;
      pgGeneral.m_strData = pszBuffer;

      if (pEnd != NULL)
      {
         pCurr = pEnd + 1;
         pEnd = _tcschr(pCurr, _T('\x7F'));
         if (pEnd != NULL)
            *pEnd = 0;
         pgGeneral.m_strEnum = pCurr;
      }

      if (pEnd != NULL)
      {
         pCurr = pEnd + 1;
         pEnd = _tcschr(pCurr, _T('\x7F'));
         if (pEnd != NULL)
            *pEnd = 0;
         pgGeneral.m_strRegEx = pCurr;
      }
   }
   else
   {
      pgGeneral.m_strData = pData->pszData;
   }
   psh.AddPage(&pgGeneral);

   // Setup "Columns" page
   if ((pData->wType == TOOL_TYPE_TABLE_SNMP) ||
       (pData->wType == TOOL_TYPE_TABLE_AGENT))
   {
      pgColumns.m_iToolType = pData->wType;
      pgColumns.m_dwNumColumns = pData->wNumColumns;
      pgColumns.m_pColumnList = (NXC_OBJECT_TOOL_COLUMN *)nx_memdup(pData->pColList, sizeof(NXC_OBJECT_TOOL_COLUMN) * pData->wNumColumns);
      psh.AddPage(&pgColumns);
   }

   // Execute property sheet
   psh.m_psh.dwFlags |= PSH_NOAPPLYNOW;
   if (psh.DoModal() == IDOK)
   {
      safe_free(pData->pdwACL);
      pData->dwACLSize = pgGeneral.m_dwACLSize;
      pData->pdwACL = (DWORD *)nx_memdup(pgGeneral.m_pdwACL, sizeof(DWORD) * pData->dwACLSize);
      nx_strncpy(pData->szName, (LPCTSTR)pgGeneral.m_strName, MAX_DB_STRING);
      nx_strncpy(pData->szDescription, (LPCTSTR)pgGeneral.m_strDescription, MAX_DB_STRING);
      safe_free(pData->pszData);
      if (pData->wType == TOOL_TYPE_TABLE_AGENT)
      {
         pgGeneral.m_strData += _T('\x7F');
         pgGeneral.m_strData += pgGeneral.m_strEnum;
         pgGeneral.m_strData += _T('\x7F');
         pgGeneral.m_strData += pgGeneral.m_strRegEx;
      }
      pData->pszData = _tcsdup((LPCTSTR)pgGeneral.m_strData);
      if ((pData->wType == TOOL_TYPE_TABLE_AGENT) ||
          (pData->wType == TOOL_TYPE_TABLE_SNMP))
      {
         safe_free(pData->pColList);
         pData->wNumColumns = (WORD)pgColumns.m_dwNumColumns;
         pData->pColList = (NXC_OBJECT_TOOL_COLUMN *)nx_memdup(pgColumns.m_pColumnList,
                            sizeof(NXC_OBJECT_TOOL_COLUMN) * pgColumns.m_dwNumColumns);
      }

      dwResult = DoRequestArg2(NXCUpdateObjectTool, g_hSession, pData,
                               _T("Updating object tool configuration..."));
      if (dwResult != RCC_SUCCESS)
      {
         theApp.ErrorBox(dwResult, _T("Cannot update object tool configuration: %s"));
      }
   }
   NXCDestroyObjectToolDetails(pData);
}
