// UserEditor.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "UserEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUserEditor

IMPLEMENT_DYNCREATE(CUserEditor, CMDIChildWnd)

CUserEditor::CUserEditor()
{
}

CUserEditor::~CUserEditor()
{
}


BEGIN_MESSAGE_MAP(CUserEditor, CMDIChildWnd)
	//{{AFX_MSG_MAP(CUserEditor)
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_COMMAND(ID_USER_CREATE_GROUP, OnUserCreateGroup)
	ON_COMMAND(ID_USER_CREATE_USER, OnUserCreateUser)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUserEditor message handlers

BOOL CUserEditor::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         NULL, 
                                         GetSysColorBrush(COLOR_WINDOW), 
                                         AfxGetApp()->LoadIcon(IDI_USERS));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CLOSE message handler
//

void CUserEditor::OnClose() 
{
   theApp.WaitForRequest(NXCUnlockUserDB());
	CMDIChildWnd::OnClose();
}


//
// WM_DESTROY message handler
//

void CUserEditor::OnDestroy() 
{
   theApp.OnViewDestroy(IDR_USER_EDITOR, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_CREATE message handler
//

int CUserEditor::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   CImageList *pImageList;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   theApp.OnViewCreate(IDR_USER_EDITOR, this);
	
   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS, rect, this, ID_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT | LVS_EX_FULLROWSELECT);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);

   // Create image list
   pImageList = new CImageList;
   pImageList->Create(16, 16, ILC_COLOR8 | ILC_MASK, 8, 8);
   pImageList->Add(AfxGetApp()->LoadIcon(IDI_USER));
   pImageList->Add(AfxGetApp()->LoadIcon(IDI_USER_GROUP));
   pImageList->Add(AfxGetApp()->LoadIcon(IDI_EVERYONE));
   m_wndListCtrl.SetImageList(pImageList, LVSIL_SMALL);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, "UID", LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(1, "Login", LVCFMT_LEFT, 100);
   m_wndListCtrl.InsertColumn(2, "Full Name", LVCFMT_LEFT, 170);
   m_wndListCtrl.InsertColumn(3, "Description", LVCFMT_LEFT, 300);

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);

   return 0;
}


//
// WM_SIZE message handler
//

void CUserEditor::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
	
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CUserEditor::OnViewRefresh() 
{
   NXC_USER *pUserList;
   DWORD i, dwNumUsers;
   int iItem;
   char szBuffer[32];

   m_wndListCtrl.DeleteAllItems();
   
   // Fill in list view
   if (NXCGetUserDB(&pUserList, &dwNumUsers))
   {
      for(i = 0; i < dwNumUsers; i++)
      {
         sprintf(szBuffer, "%08X", pUserList[i].dwId);
         iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, szBuffer,
            (pUserList[i].dwId == GROUP_EVERYONE) ? 2 :
               ((pUserList[i].dwId & GROUP_FLAG) ? 1 : 0));
         if (iItem != -1)
         {
            m_wndListCtrl.SetItemData(iItem, pUserList[i].dwId);
            m_wndListCtrl.SetItemText(iItem, 1, pUserList[i].szName);
            m_wndListCtrl.SetItemText(iItem, 2, pUserList[i].szFullName);
            m_wndListCtrl.SetItemText(iItem, 3, pUserList[i].szDescription);
         }
      }
   }
}


//
// WM_COMMAND::ID_USER_CREATE_USER message handler
//

void CUserEditor::OnUserCreateUser() 
{
   CNewUserDlg wndDialog;

   wndDialog.m_strTitle = "New User";
   wndDialog.m_strHeader = "Login name";
   wndDialog.m_bDefineProperties = TRUE;
   if (wndDialog.DoModal() == IDOK)
      CreateUserObject((LPCTSTR)wndDialog.m_strName, FALSE, wndDialog.m_bDefineProperties);
}


//
// WM_COMMAND::ID_USER_CREATE_GROUP message handler
//

void CUserEditor::OnUserCreateGroup() 
{
   CNewUserDlg wndDialog;

   wndDialog.m_strTitle = "New Group";
   wndDialog.m_strHeader = "Group name";
   wndDialog.m_bDefineProperties = TRUE;
   if (wndDialog.DoModal() == IDOK)
      CreateUserObject((LPCTSTR)wndDialog.m_strName, TRUE, wndDialog.m_bDefineProperties);
}


//
// Create user or group
//

void CUserEditor::CreateUserObject(const char *pszName, BOOL bIsGroup, BOOL bShowProp)
{
   DWORD dwResult;

   // Send request to server
   if (bIsGroup)
      dwResult = theApp.WaitForRequest(NXCCreateUserGroup((char *)pszName), "Creating new group...");
   else
      dwResult = theApp.WaitForRequest(NXCCreateUser((char *)pszName), "Creating new user...");

   if (dwResult == RCC_SUCCESS)
   {
   }
   else
   {
      char szBuffer[256];

      sprintf(szBuffer, "Error creating %s object: %s", 
              bIsGroup ? "group" : "user", NXCGetErrorText(dwResult));
      MessageBox(szBuffer, "Error", MB_OK | MB_ICONSTOP);
   }
}
