// ObjectToolsEditor.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectToolsEditor.h"
#include "ObjToolPropGeneral.h"
#include "ObjToolPropOptions.h"
#include "ObjToolPropColumns.h"
#include "NewObjectToolDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Compare two list view items
//

static int CALLBACK ToolCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
   int iResult;
   NXC_OBJECT_TOOL *pItem1, *pItem2;

   pItem1 = ((CObjectToolsEditor *)lParamSort)->GetToolById((DWORD)lParam1);
   pItem2 = ((CObjectToolsEditor *)lParamSort)->GetToolById((DWORD)lParam2);
   if ((pItem1 == NULL) || (pItem2 == NULL))
      return 0;   // Just a paranoid check, shouldn't happen

   switch(((CObjectToolsEditor *)lParamSort)->SortMode())
   {
      case 0:  // ID
         iResult = COMPARE_NUMBERS(lParam1, lParam2);
         break;
      case 1:  // Name
         iResult = _tcsicmp(pItem1->szName, pItem2->szName);
         break;
      case 2:  // Type
         iResult = _tcsicmp(g_szToolType[pItem1->wType], g_szToolType[pItem2->wType]);
         break;
      case 3:  // Description
         iResult = _tcsicmp(pItem1->szDescription, pItem2->szDescription);
         break;
      default:
         iResult = 0;
         break;
   }
   return (((CObjectToolsEditor *)lParamSort)->SortDir() == 0) ? iResult : -iResult;
}


/////////////////////////////////////////////////////////////////////////////
// CObjectToolsEditor

IMPLEMENT_DYNCREATE(CObjectToolsEditor, CMDIChildWnd)

CObjectToolsEditor::CObjectToolsEditor()
{
   m_dwNumTools = 0;
   m_pToolList = NULL;

   m_iSortMode = theApp.GetProfileInt(_T("ObjToolEditor"), _T("SortMode"), 1);
   m_iSortDir = theApp.GetProfileInt(_T("ObjToolEditor"), _T("SortDir"), 0);
}

CObjectToolsEditor::~CObjectToolsEditor()
{
   NXCDestroyObjectToolList(m_dwNumTools, m_pToolList);
   theApp.WriteProfileInt(_T("ObjToolEditor"), _T("SortMode"), m_iSortMode);
   theApp.WriteProfileInt(_T("ObjToolEditor"), _T("SortDir"), m_iSortDir);
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
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_VIEW, OnListViewColumnClick)
	ON_MESSAGE(NXMC_UPDATE_OBJTOOL_LIST, OnUpdateObjectToolsList)
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
   LVCOLUMN lvCol;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   theApp.OnViewCreate(VIEW_OBJECT_TOOLS, this);
	
   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT |
                        LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS,
                        rect, this, IDC_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);

   // Create image list
   m_imageList.Create(16, 16, ILC_COLOR8 | ILC_MASK, 8, 8);
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_UNKNOWN));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_EXEC));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_DOCUMENT));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_DOCUMENT));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_IEXPLORER));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_COMMAND));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_COMMAND));
   m_iSortImageBase = m_imageList.GetImageCount();
   m_imageList.Add(theApp.LoadIcon(IDI_SORT_UP));
   m_imageList.Add(theApp.LoadIcon(IDI_SORT_DOWN));
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, _T("ID"), LVCFMT_LEFT, 70);
   m_wndListCtrl.InsertColumn(1, _T("Name"), LVCFMT_LEFT, 250);
   m_wndListCtrl.InsertColumn(2, _T("Type"), LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(3, _T("Description"), LVCFMT_LEFT, 300);

   // Mark sorting column
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = (m_iSortDir == 0)  ? m_iSortImageBase : (m_iSortImageBase + 1);
   m_wndListCtrl.SetColumn(m_iSortMode, &lvCol);

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);

   return 0;
}


//
// WM_DESTROY message handler
//

void CObjectToolsEditor::OnDestroy() 
{
   theApp.OnViewDestroy(VIEW_OBJECT_TOOLS, this);
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
	CMDIChildWnd::OnClose();
}


//
// Handler for WM_NOTIFY::NM_DBLCLK from IDC_LIST_VIEW
//

void CObjectToolsEditor::OnListViewDblClk(NMHDR *pNMHDR, LRESULT *pResult)
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
         _sntprintf_s(szBuffer, 32, _TRUNCATE, _T("%d"), m_pToolList[i].dwId);
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
      m_wndListCtrl.SortItems(ToolCompareProc, (LPARAM)this);
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
   static WORD m_wTransTbl[7] = { TOOL_TYPE_ACTION, TOOL_TYPE_COMMAND, TOOL_TYPE_SERVER_COMMAND,
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
                               CListCtrl *pwndListCtrl, CObjectToolsEditor *pWnd)
{
   DWORD i, dwResult;

   for(i = 0; i < dwNumTools; i++)
   {
      dwResult = NXCDeleteObjectTool(g_hSession, pdwToolList[i]);
      if (dwResult != RCC_SUCCESS)
         break;
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
   if ((dwNumItems > 0) &&
		 (MessageBox(_T("Do you really want to delete selected tool(s)?"), _T("Confirmation"), MB_YESNO | MB_ICONQUESTION) == IDYES))
   {
      pdwList = (DWORD *)malloc(sizeof(DWORD) * dwNumItems);
      iItem = m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED);
      for(i = 0; (i < dwNumItems) && (iItem != -1); i++)
      {
         pdwList[i] = m_wndListCtrl.GetItemData(iItem);
         iItem = m_wndListCtrl.GetNextItem(iItem, LVIS_SELECTED);
      }
      dwResult = DoRequestArg4(DeleteObjectTools, (void *)dwNumItems,
                               pdwList, &m_wndListCtrl, this,
                               _T("Deleting object tools..."));
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
   CObjToolPropOptions pgOptions;
   CObjToolPropColumns pgColumns;
   TCHAR szBuffer[1024];
   DWORD dwResult;

   _sntprintf_s(szBuffer, 1024, _TRUNCATE, _T("Object Tool Properties (%s)"), g_szToolType[pData->wType]);
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

   // Setup "Options" page
   pgOptions.m_bNeedAgent = (pData->dwFlags & TF_REQUIRES_AGENT) ? TRUE : FALSE;
   pgOptions.m_bNeedSNMP = (pData->dwFlags & TF_REQUIRES_SNMP) ? TRUE : FALSE;
   pgOptions.m_bMatchOID = (pData->dwFlags & TF_REQUIRES_OID_MATCH) ? TRUE : FALSE;
   pgOptions.m_strMatchingOID = CHECK_NULL_EX(pData->pszMatchingOID);
   pgOptions.m_nIndexType = (pData->dwFlags & TF_SNMP_INDEXED_BY_VALUE) ? 1 : 0;
   pgOptions.m_iToolType = (int)pData->wType;
   pgOptions.m_bConfirmation = (pData->dwFlags & TF_ASK_CONFIRMATION) ? TRUE : FALSE;
   pgOptions.m_strConfirmationText = CHECK_NULL_EX(pData->pszConfirmationText);
   psh.AddPage(&pgOptions);

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
      pData->pdwACL = (UINT32 *)nx_memdup(pgGeneral.m_pdwACL, sizeof(UINT32) * pData->dwACLSize);
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

      pData->dwFlags = 0;
      if (pgOptions.m_bNeedAgent)
         pData->dwFlags |= TF_REQUIRES_AGENT;
      if (pgOptions.m_bNeedSNMP)
         pData->dwFlags |= TF_REQUIRES_SNMP;
      safe_free(pData->pszMatchingOID);
      if (pgOptions.m_bMatchOID)
      {
         pData->dwFlags |= TF_REQUIRES_OID_MATCH;
         pData->pszMatchingOID = _tcsdup((LPCTSTR)pgOptions.m_strMatchingOID);
      }
      else
      {
         pData->pszMatchingOID = NULL;
      }
      safe_free(pData->pszConfirmationText)
      if (pgOptions.m_bConfirmation)
      {
         pData->dwFlags |= TF_ASK_CONFIRMATION;
         pData->pszConfirmationText = _tcsdup((LPCTSTR)pgOptions.m_strConfirmationText);
      }
      else
      {
         pData->pszConfirmationText = NULL;
      }
      if (pgOptions.m_nIndexType == 1)
         pData->dwFlags |= TF_SNMP_INDEXED_BY_VALUE;

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
      if (dwResult == RCC_SUCCESS)
      {
         NXC_OBJECT_TOOL *pTool;

         // Update internal tool list
         pTool = GetToolById(pData->dwId);
         if (pTool == NULL)
         {
            // New tool, create new record in internal list
            m_pToolList = (NXC_OBJECT_TOOL *)realloc(m_pToolList, sizeof(NXC_OBJECT_TOOL) * (m_dwNumTools + 1));
            pTool = &m_pToolList[m_dwNumTools];
            m_dwNumTools++;

            memset(pTool, 0, sizeof(NXC_OBJECT_TOOL));
            pTool->dwId = pData->dwId;
            pTool->wType = pData->wType;
         }
         else
         {
            safe_free(pTool->pszData);
            safe_free(pTool->pszMatchingOID);
         }
         pTool->dwFlags = pData->dwFlags;
         _tcscpy_s(pTool->szName, MAX_DB_STRING, pData->szName);
         _tcscpy_s(pTool->szDescription, MAX_DB_STRING, pData->szDescription);
         pTool->pszData = _tcsdup(pData->pszData);
         pTool->pszMatchingOID = (pData->pszMatchingOID == NULL) ? NULL : _tcsdup(pData->pszMatchingOID);

			UpdateListItem(pData);
      }
      else
      {
         theApp.ErrorBox(dwResult, _T("Cannot update object tool configuration: %s"));
      }
   }
   NXCDestroyObjectToolDetails(pData);
}


//
// Update list control
//

void CObjectToolsEditor::UpdateListItem(NXC_OBJECT_TOOL_DETAILS *pData)
{
   int iItem;
   LVFINDINFO lvfi;
	TCHAR szBuffer[32];

   lvfi.flags = LVFI_PARAM;
   lvfi.lParam = pData->dwId;
   iItem = m_wndListCtrl.FindItem(&lvfi, -1);
   if (iItem == -1)
   {
      _sntprintf_s(szBuffer, 32, _TRUNCATE, _T("%d"), pData->dwId);
      iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, szBuffer, pData->wType);
      if (iItem != -1)
         m_wndListCtrl.SetItemData(iItem, pData->dwId);
   }
   if (iItem != -1)
   {
      m_wndListCtrl.SetItemText(iItem, 1, pData->szName);
      m_wndListCtrl.SetItemText(iItem, 2, g_szToolType[pData->wType]);
      m_wndListCtrl.SetItemText(iItem, 3, pData->szDescription);
      m_wndListCtrl.SortItems(ToolCompareProc, (LPARAM)this);
   }
}


//
// Get object tool by it's id
//

NXC_OBJECT_TOOL *CObjectToolsEditor::GetToolById(DWORD dwId)
{
   DWORD i;

   for(i = 0; i < m_dwNumTools; i++)
      if (m_pToolList[i].dwId == dwId)
         return &m_pToolList[i];
   return NULL;
}


//
// WM_NOTIFY::LVN_COLUMNCLICK message handler
//

void CObjectToolsEditor::OnListViewColumnClick(NMHDR *pNMHDR, LRESULT *pResult)
{
   LVCOLUMN lvCol;

   // Unmark old sorting column
   lvCol.mask = LVCF_FMT;
   lvCol.fmt = LVCFMT_LEFT;
   m_wndListCtrl.SetColumn(m_iSortMode, &lvCol);

   // Change current sort mode and resort list
   if (m_iSortMode == ((LPNMLISTVIEW)pNMHDR)->iSubItem)
   {
      // Same column, change sort direction
      m_iSortDir = 1 - m_iSortDir;
   }
   else
   {
      // Another sorting column
      m_iSortMode = ((LPNMLISTVIEW)pNMHDR)->iSubItem;
   }
   m_wndListCtrl.SortItems(ToolCompareProc, (LPARAM)this);

   // Mark new sorting column
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = (m_iSortDir == 0)  ? m_iSortImageBase : (m_iSortImageBase + 1);
   m_wndListCtrl.SetColumn(((LPNMLISTVIEW)pNMHDR)->iSubItem, &lvCol);
   
   *pResult = 0;
}


//
// NXMC_UPDATE_OBJTOOL_LIST message handler
//

LPARAM CObjectToolsEditor::OnUpdateObjectToolsList(WPARAM wParam, LPARAM lParam)
{
	NXC_OBJECT_TOOL_DETAILS *data = CAST_TO_POINTER(lParam, NXC_OBJECT_TOOL_DETAILS *);

	NXC_OBJECT_TOOL *tool = GetToolById(data->dwId);
	if (tool == NULL)
	{
      // New tool, create new record in internal list
      m_pToolList = (NXC_OBJECT_TOOL *)realloc(m_pToolList, sizeof(NXC_OBJECT_TOOL) * (m_dwNumTools + 1));
      tool = &m_pToolList[m_dwNumTools];
      m_dwNumTools++;

      memset(tool, 0, sizeof(NXC_OBJECT_TOOL));
      tool->dwId = data->dwId;
	}

	tool->dwFlags = data->dwFlags;
	tool->wType = data->wType;
	_tcscpy(tool->szName, data->szName);
	_tcscpy(tool->szDescription, data->szDescription);
	safe_free(tool->pszMatchingOID);
	safe_free(tool->pszData);
	safe_free(tool->pszConfirmationText);
	tool->pszMatchingOID = data->pszMatchingOID;
	tool->pszData = data->pszData;
	tool->pszConfirmationText = data->pszConfirmationText;

	UpdateListItem(data);

	free(data);
	return 0;
}


//
// Update tool
//

static DWORD UpdateTool(DWORD toolId, CObjectToolsEditor *pWnd)
{
	DWORD rcc;
	NXC_OBJECT_TOOL_DETAILS *data;

	rcc = NXCGetObjectToolDetails(g_hSession, toolId, &data);
	if (rcc == RCC_SUCCESS)
	{
		pWnd->PostMessage(NXMC_UPDATE_OBJTOOL_LIST, 0, (LPARAM)data);
	}
	return rcc;
}


//
// Handler for object tool update or deletion
//

void CObjectToolsEditor::OnObjectToolsUpdate(DWORD toolId, bool isDelete)
{
	if (isDelete)
	{
		LVFINDINFO lvfi;
		int item;

      lvfi.flags = LVFI_PARAM;
      lvfi.lParam = toolId;
      item = m_wndListCtrl.FindItem(&lvfi, -1);
      if (item != -1)
         m_wndListCtrl.DeleteItem(item);

		for(DWORD i = 0; i < m_dwNumTools; i++)
		{
			if (m_pToolList[i].dwId == toolId)
			{
				safe_free(m_pToolList[i].pszData);
				safe_free(m_pToolList[i].pszMatchingOID);
				safe_free(m_pToolList[i].pszConfirmationText);
				m_dwNumTools--;
				memmove(&m_pToolList[i], &m_pToolList[i + 1], sizeof(NXC_OBJECT_TOOL) * (m_dwNumTools - i));
				break;
			}
		}
	}
	else
	{
		DoAsyncRequestArg2(m_hWnd, toolId, UpdateTool, (void *)toolId, this);
	}
}
