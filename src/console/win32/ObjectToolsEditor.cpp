// ObjectToolsEditor.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectToolsEditor.h"
#include "ObjToolPropGeneral.h"

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
	//}}AFX_MSG_MAP
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
	
   theApp.OnViewCreate(IDR_USER_EDITOR, this);
	
   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL, rect, this, IDC_LIST_VIEW);
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
   m_wndListCtrl.InsertColumn(0, "Name", LVCFMT_LEFT, 250);
   m_wndListCtrl.InsertColumn(1, "Type", LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(2, "Description", LVCFMT_LEFT, 300);

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
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CObjectToolsEditor::OnViewRefresh() 
{
   DWORD i, dwResult;
   int iItem;

   m_wndListCtrl.DeleteAllItems();
   NXCDestroyObjectToolList(m_dwNumTools, m_pToolList);

   dwResult = DoRequestArg3(NXCGetObjectTools, g_hSession, &m_dwNumTools,
                            &m_pToolList, _T("Loading list of object tools..."));
   if (dwResult == RCC_SUCCESS)
   {
      for(i = 0; i < m_dwNumTools; i++)
      {
         iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, m_pToolList[i].szName,
                                          m_pToolList[i].wType);
         if (iItem != -1)
         {
            m_wndListCtrl.SetItemData(iItem, i);
            m_wndListCtrl.SetItemText(iItem, 1, g_szToolType[m_pToolList[i].wType]);
            m_wndListCtrl.SetItemText(iItem, 2, m_pToolList[i].szDescription);
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
   CObjToolPropGeneral pgGeneral;

   iItem = m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED);
   if (iItem != -1)
   {
      dwToolId = m_pToolList[m_wndListCtrl.GetItemData(iItem)].dwId;
      dwResult = DoRequestArg3(NXCGetObjectToolDetails, g_hSession, (void *)dwToolId,
                               &pData, _T("Loading tool data..."));
      if (dwResult == RCC_SUCCESS)
      {
         TCHAR szBuffer[1024];

         _stprintf(szBuffer, _T("Object Tool Properties (%s)"), g_szToolType[pData->wType]);
         CPropertySheet psh(szBuffer, this, 0);

         pgGeneral.m_iToolType = pData->wType;
         pgGeneral.m_strName = pData->szName;
         pgGeneral.m_strDescription = pData->szDescription;
         psh.AddPage(&pgGeneral);
         psh.m_psh.dwFlags |= PSH_NOAPPLYNOW;
         if (psh.DoModal() == IDOK)
         {
         }
         NXCDestroyObjectToolDetails(pData);
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
