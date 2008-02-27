// SituationManager.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "SituationManager.h"
#include "InputBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSituationManager

IMPLEMENT_DYNCREATE(CSituationManager, CMDIChildWnd)

CSituationManager::CSituationManager()
{
}

CSituationManager::~CSituationManager()
{
}


BEGIN_MESSAGE_MAP(CSituationManager, CMDIChildWnd)
	//{{AFX_MSG_MAP(CSituationManager)
	ON_WM_CREATE()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_WM_DESTROY()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_SITUATION_CREATE, OnSituationCreate)
	ON_COMMAND(ID_SITUATION_DELETE, OnSituationDelete)
	ON_UPDATE_COMMAND_UI(ID_SITUATION_DELETE, OnUpdateSituationDelete)
	//}}AFX_MSG_MAP
   ON_MESSAGE(NXCM_SITUATION_CHANGE, OnSituationChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSituationManager message handlers


//
// WM_CREATE message handler
//

int CSituationManager::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	RECT rect;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   theApp.OnViewCreate(VIEW_SITUATION_MANAGER, this);

   // Create image list
   m_imageList.Create(16, 16, ILC_COLOR8 | ILC_MASK, 4, 4);
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_SCRIPT_LIBRARY));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_SITUATION));

   // Create tree view control
	GetClientRect(&rect);
   m_wndTreeCtrl.Create(WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
                        rect, this, ID_TREE_CTRL);
   //m_wndTreeCtrl.SetImageList(&m_imageList, TVSIL_NORMAL);

	PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
	return 0;
}


//
// WM_DESTROY message handler
//

void CSituationManager::OnDestroy() 
{
   theApp.OnViewDestroy(VIEW_SITUATION_MANAGER, this);
	CMDIChildWnd::OnDestroy();
}


//
// Refresh view
//

void CSituationManager::OnViewRefresh() 
{
	NXC_SITUATION_LIST *list;
	HTREEITEM item, subitem;
	int i, j;

	m_wndTreeCtrl.DeleteAllItems();
	m_root = m_wndTreeCtrl.InsertItem(_T("[root]"));
	list = theApp.GetSituationList();
	if (list != NULL)
	{
		for(i = 0; i < list->m_count; i++)
		{
			item = m_wndTreeCtrl.InsertItem(list->m_situations[i].m_name, m_root);
			m_wndTreeCtrl.SetItemData(item, list->m_situations[i].m_id);
			for(j = 0; j < list->m_situations[i].m_instanceCount; j++)
			{
				subitem = m_wndTreeCtrl.InsertItem(list->m_situations[i].m_instanceList[j].m_name, item);
				m_wndTreeCtrl.SetItemData(subitem, 0xFFFFFFFF);
			}
		}
		theApp.UnlockSituationList();
	}
}


//
// WM_CONTEXTMENU message handler
//

void CSituationManager::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CMenu *pMenu;
   HTREEITEM hItem;
   UINT uFlags;
   CPoint pt;

   pt = point;
   m_wndTreeCtrl.ScreenToClient(&pt);

   hItem = m_wndTreeCtrl.HitTest(pt, &uFlags);
   if ((hItem != NULL) && (uFlags & TVHT_ONITEM))
      m_wndTreeCtrl.Select(hItem, TVGN_CARET);
   pMenu = theApp.GetContextMenu(29);
   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this, NULL);
}


//
// Create new situation
//

void CSituationManager::OnSituationCreate() 
{
	CInputBox dlg;
	DWORD rcc, id;
	HTREEITEM item;

	dlg.m_strTitle = _T("Create situation");
	dlg.m_strHeader = _T("Situation name");
	if (dlg.DoModal() == IDOK)
	{
		rcc = DoRequestArg4(NXCCreateSituation, g_hSession, (void *)((LPCTSTR)dlg.m_strText), _T(""), &id, _T("Creating situation..."));
		if (rcc == RCC_SUCCESS)
		{
			item = m_wndTreeCtrl.InsertItem(dlg.m_strText, m_root);
			m_wndTreeCtrl.SetItemData(item, id);
			m_wndTreeCtrl.SelectItem(item);
		}
		else
		{
			theApp.ErrorBox(rcc, _T("Cannot create situation: %s"));
		}
	}
}


//
// Delete situation
//

void CSituationManager::OnSituationDelete() 
{
	HTREEITEM item;
	DWORD rcc;

	item = m_wndTreeCtrl.GetSelectedItem();
	if (item != NULL)
	{
		rcc = DoRequestArg2(NXCDeleteSituation, g_hSession, CAST_TO_POINTER(m_wndTreeCtrl.GetItemData(item), void *), _T("Deleting situation..."));
		if (rcc == RCC_SUCCESS)
		{
			m_wndTreeCtrl.DeleteItem(item);
		}
		else
		{
			theApp.ErrorBox(rcc, _T("Cannot delete situation: %s"));
		}
	}
}

void CSituationManager::OnUpdateSituationDelete(CCmdUI* pCmdUI) 
{
	HTREEITEM item;

	item = m_wndTreeCtrl.GetSelectedItem();
	if (item != NULL)
	{
		DWORD id = m_wndTreeCtrl.GetItemData(item);
		pCmdUI->Enable((id != 0) && (id != 0xFFFFFFFF));
	}
	else
	{
		pCmdUI->Enable(FALSE);
	}
}


//
// NXCM_SITUATION_CHANGE message handler
//

void CSituationManager::OnSituationChange(WPARAM wParam, LPARAM lParam)
{
	// TODO: implement view update
}
