// MapManager.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "MapManager.h"
#include "CreateNetMapDlg.h"
#include "InputBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMapManager

IMPLEMENT_DYNCREATE(CMapManager, CMDIChildWnd)

CMapManager::CMapManager()
{
}

CMapManager::~CMapManager()
{
}


BEGIN_MESSAGE_MAP(CMapManager, CMDIChildWnd)
	//{{AFX_MSG_MAP(CMapManager)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_WM_DESTROY()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_COMMAND(ID_MAP_CREATE, OnMapCreate)
	ON_COMMAND(ID_MAP_DELETE, OnMapDelete)
	ON_UPDATE_COMMAND_UI(ID_MAP_DELETE, OnUpdateMapDelete)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_MAP_RENAME, OnMapRename)
	ON_UPDATE_COMMAND_UI(ID_MAP_RENAME, OnUpdateMapRename)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CMapManager::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, NULL, NULL, 
                                         AfxGetApp()->LoadIcon(IDI_NETMAP));
	return CMDIChildWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CMapManager message handlers


//
// WM_CREATE message handler
//

int CMapManager::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	RECT rect;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	GetClientRect(&rect);
	m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_EDITLABELS, rect, this, ID_LIST_VIEW);
	m_wndListCtrl.InsertColumn(0, _T("ID"), LVCFMT_LEFT, 60);
	m_wndListCtrl.InsertColumn(1, _T("Name"), LVCFMT_LEFT, 150);
	m_wndListCtrl.InsertColumn(2, _T("Root object"), LVCFMT_LEFT, 150);
	m_wndListCtrl.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
	LoadListCtrlColumns(m_wndListCtrl, _T("MapManager"), _T("ListCtrl"));

   theApp.OnViewCreate(VIEW_MAP_MANAGER, this);

	PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);

	return 0;
}


//
// WM_SIZE message handler
//

void CMapManager::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
	m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_SETFOCUS message handler
//

void CMapManager::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
	m_wndListCtrl.SetFocus();
}


//
// WM_DESTROY message handler
//

void CMapManager::OnDestroy() 
{
	SaveListCtrlColumns(m_wndListCtrl, _T("MapManager"), _T("ListCtrl"));
   theApp.OnViewDestroy(VIEW_MAP_MANAGER, this);
	CMDIChildWnd::OnDestroy();
}


//
// Refresh view
//

void CMapManager::OnViewRefresh() 
{
	DWORD rcc, numMaps, i;
	NXC_MAP_INFO *mapList;
	TCHAR buffer[64];
	int item;
	NXC_OBJECT *object;

	m_wndListCtrl.DeleteAllItems();

	rcc = DoRequestArg3(NXCGetMapList, g_hSession, &numMaps, &mapList, _T("Retrieving map list..."));
	if (rcc == RCC_SUCCESS)
	{
		for(i = 0; i < numMaps; i++)
		{
			_sntprintf_s(buffer, 64, _TRUNCATE, _T("%d"), mapList[i].dwMapId);
			item = m_wndListCtrl.InsertItem(i, buffer);
			if (item != -1)
			{
				m_wndListCtrl.SetItemData(item, mapList[i].dwMapId);
				m_wndListCtrl.SetItemText(item, 1, mapList[i].szName);
				object = NXCFindObjectById(g_hSession, mapList[i].dwObjectId);
				m_wndListCtrl.SetItemText(item, 2, (object != NULL) ? object->szName : _T("<unknown>"));
			}
		}
		safe_free(mapList);
	}
	else
	{
		theApp.ErrorBox(rcc, _T("Cannot retrieve map list from server: %s"));
	}
}


//
// Map -> Create menu handler
//

void CMapManager::OnMapCreate() 
{
	CCreateNetMapDlg dlg;
	DWORD rcc, mapId;
	TCHAR buffer[64];
	NXC_OBJECT *object;
	int item;

	if (dlg.DoModal() == IDOK)
	{
		rcc = DoRequestArg4(NXCCreateMap, g_hSession, (void *)dlg.m_dwRootObj, (void *)((LPCTSTR)dlg.m_strName),
		                    &mapId, _T("Creating new network map..."));
		if (rcc == RCC_SUCCESS)
		{
			_sntprintf_s(buffer, 64, _TRUNCATE, _T("%d"), mapId);
			item = m_wndListCtrl.InsertItem(0x7FFFFFFF, buffer);
			if (item != -1)
			{
				m_wndListCtrl.SetItemData(item, mapId);
				m_wndListCtrl.SetItemText(item, 1, dlg.m_strName);
				object = NXCFindObjectById(g_hSession, dlg.m_dwRootObj);
				m_wndListCtrl.SetItemText(item, 2, (object != NULL) ? object->szName : _T("<unknown>"));
			}
		}
		else
		{
			theApp.ErrorBox(rcc, _T("Cannot create network map: %s"));
		}
	}
}


//
// Map -> Delete menu handler
//

void CMapManager::OnMapDelete() 
{
	int item;
	DWORD rcc;

	if (m_wndListCtrl.GetSelectedCount() == 0)
		return;

	while((item = m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED)) != -1)
	{
		rcc = DoRequestArg2(NXCDeleteMap, g_hSession, (void *)m_wndListCtrl.GetItemData(item), _T("Deleting map..."));
		if (rcc == RCC_SUCCESS)
		{
			m_wndListCtrl.DeleteItem(item);
		}
		else
		{
			theApp.ErrorBox(rcc, _T("Cannot delete map: %s"));
			break;
		}
	}
}

void CMapManager::OnUpdateMapDelete(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() > 0);	
}


//
// WM_CONTEXTMENU message handler
//

void CMapManager::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CMenu *pMenu;

   pMenu = theApp.GetContextMenu(30);
   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this, NULL);
}


//
// Map -> Rename menu handler
//

void CMapManager::OnMapRename() 
{
	CInputBox dlg;
	int item;
	DWORD rcc;

	if (m_wndListCtrl.GetSelectedCount() != 1)
		return;

	item = m_wndListCtrl.GetSelectionMark();
	dlg.m_strTitle = _T("Rename Map");
	dlg.m_strHeader = _T("Enter new map name");
	dlg.m_strText = m_wndListCtrl.GetItemText(item, 1);
	if (dlg.DoModal() == IDOK)
	{
		rcc = DoRequestArg3(NXCRenameMap, g_hSession, (void *)m_wndListCtrl.GetItemData(item),
		                    (void *)((LPCTSTR)dlg.m_strText), _T("Renaming map..."));
		if (rcc == RCC_SUCCESS)
		{
			m_wndListCtrl.SetItemText(item, 1, dlg.m_strText);
		}
		else
		{
			theApp.ErrorBox(rcc, _T("Unable to rename map: %s"));
		}
	}
}

void CMapManager::OnUpdateMapRename(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() == 1);
}
