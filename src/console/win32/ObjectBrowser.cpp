// ObjectBrowser.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectBrowser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjectBrowser

IMPLEMENT_DYNCREATE(CObjectBrowser, CMDIChildWnd)

CObjectBrowser::CObjectBrowser()
{
   m_pImageList = NULL;
}

CObjectBrowser::~CObjectBrowser()
{
   delete m_pImageList;
}


BEGIN_MESSAGE_MAP(CObjectBrowser, CMDIChildWnd)
	//{{AFX_MSG_MAP(CObjectBrowser)
	ON_WM_DESTROY()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_NOTIFY(NM_RCLICK, IDC_TREE_VIEW, OnRclickTreeView)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjectBrowser message handlers

void CObjectBrowser::OnDestroy() 
{
   ((CConsoleApp *)AfxGetApp())->OnViewDestroy(IDR_OBJECTS, this);
	CMDIChildWnd::OnDestroy();
}

int CObjectBrowser::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create list view control
   GetClientRect(&rect);
   m_wndTreeCtrl.Create(WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT,
                        rect, this, IDC_TREE_VIEW);

   // Create image list
   m_pImageList = new CImageList;
   m_pImageList->Create(16, 16, ILC_COLOR8 | ILC_MASK, 8, 8);
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_NETMAP));
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_INTERFACE));
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_NODE));
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_SUBNET));
   m_wndTreeCtrl.SetImageList(m_pImageList, TVSIL_NORMAL);

   ((CConsoleApp *)AfxGetApp())->OnViewCreate(IDR_OBJECTS, this);
   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
	return 0;
}


//
// WM_SIZE message handler
//

void CObjectBrowser::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
	
   m_wndTreeCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_SETFOCUS message handler
//

void CObjectBrowser::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
	
   m_wndTreeCtrl.SetFocus();	
}

void CObjectBrowser::OnViewRefresh() 
{
   NXC_OBJECT *pObject;
   
   m_wndTreeCtrl.DeleteAllItems();
   pObject = NXCGetRootObject();
   if (pObject != NULL)
      AddObjectToTree(pObject, TVI_ROOT);
}

void CObjectBrowser::AddObjectToTree(NXC_OBJECT *pObject, HTREEITEM hParent)
{
   DWORD i;
   HTREEITEM hItem;
   char szBuffer[512], szIpBuffer[32];
   static int iImageCode[] = { -1, 3, 2, 1, 0 };

   // Add object record with class-dependent text
   switch(pObject->iClass)
   {
      case OBJECT_SUBNET:
         sprintf(szBuffer, "%s [Status: %s]", pObject->szName, g_szStatusText[pObject->iStatus]);
         break;
      case OBJECT_INTERFACE:
         if (pObject->dwIpAddr != 0)
            sprintf(szBuffer, "%s [IP: %s/%d Status: %s]", pObject->szName, 
                    IpToStr(pObject->dwIpAddr, szIpBuffer), 
                    BitsInMask(pObject->iface.dwIpNetMask), g_szStatusText[pObject->iStatus]);
         else
            sprintf(szBuffer, "%s [Status: %s]", pObject->szName, g_szStatusText[pObject->iStatus]);
         break;
      default:
         strcpy(szBuffer, pObject->szName);
         break;
   }
   hItem = m_wndTreeCtrl.InsertItem(szBuffer, iImageCode[pObject->iClass], iImageCode[pObject->iClass], hParent);

   // Add object's childs
   for(i = 0; i < pObject->dwNumChilds; i++)
   {
      NXC_OBJECT *pChildObject = NXCFindObjectById(pObject->pdwChildList[i]);
      if (pChildObject != NULL)
         AddObjectToTree(pChildObject, hItem);
   }
}

BOOL CObjectBrowser::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, NULL, NULL, 
                                         AfxGetApp()->LoadIcon(IDI_TREE));
	return CMDIChildWnd::PreCreateWindow(cs);
}

void CObjectBrowser::OnRclickTreeView(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
}
