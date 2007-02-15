// GraphManagerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "GraphManagerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGraphManagerDlg dialog


CGraphManagerDlg::CGraphManagerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGraphManagerDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGraphManagerDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CGraphManagerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGraphManagerDlg)
	DDX_Control(pDX, IDC_TREE_GRAPHS, m_wndTreeCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGraphManagerDlg, CDialog)
	//{{AFX_MSG_MAP(CGraphManagerDlg)
	ON_NOTIFY(TVN_ITEMEXPANDED, IDC_TREE_GRAPHS, OnItemexpandedTreeGraphs)
	ON_WM_CONTEXTMENU()
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
	ON_COMMAND(ID_GRAPHMANAGER_DELETE, OnGraphmanagerDelete)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGraphManagerDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CGraphManagerDlg::OnInitDialog() 
{
	DWORD dwTemp;

	CDialog::OnInitDialog();
	
	m_imageList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 3, 1);
	m_imageList.Add(theApp.LoadIcon(IDI_GRAPH));
	m_imageList.Add(theApp.LoadIcon(IDI_CLOSED_FOLDER));
	m_imageList.Add(theApp.LoadIcon(IDI_OPEN_FOLDER));
	m_wndTreeCtrl.SetImageList(&m_imageList, TVSIL_NORMAL);

	dwTemp = 0;
	CreateGraphTree(TVI_ROOT, _T(""), &dwTemp);
	
	return TRUE;
}


//
// Create one level of graphs tree
//

void CGraphManagerDlg::CreateGraphTree(HTREEITEM hRoot, TCHAR *pszCurrPath, DWORD *pdwStart)
{
	DWORD i, j;
   TCHAR szName[MAX_DB_STRING], szPath[MAX_DB_STRING];

	for(i = *pdwStart; i < g_dwNumGraphs; i++)
	{
      // Separate item name and path
      memset(szPath, 0, sizeof(TCHAR) * MAX_DB_STRING);
      if (_tcsstr(g_pGraphList[i].pszName, _T("->")) != NULL)
      {
         for(j = _tcslen(g_pGraphList[i].pszName) - 1; j > 0; j--)
            if ((g_pGraphList[i].pszName[j] == _T('>')) &&
                (g_pGraphList[i].pszName[j - 1] == _T('-')))
            {
               _tcscpy(szName, &g_pGraphList[i].pszName[j + 1]);
               j--;
               break;
            }
         memcpy(szPath, g_pGraphList[i].pszName, j * sizeof(TCHAR));
      }
      else
      {
         _tcscpy(szName, g_pGraphList[i].pszName);
      }

		// Add menu item if we are on right level, otherwise create subtree or go one level up
      if (!_tcsicmp(szPath, pszCurrPath))
      {
			HTREEITEM hItem;

         StrStrip(szName);
			hItem = m_wndTreeCtrl.InsertItem(szName, 0, 0, hRoot);
			m_wndTreeCtrl.SetItemData(hItem, g_pGraphList[i].dwId);
      }
      else
      {
         for(j = _tcslen(pszCurrPath); (szPath[j] == _T(' ')) || (szPath[j] == _T('\t')); j++);
         if ((*pszCurrPath == 0) ||
             ((!memicmp(szPath, pszCurrPath, _tcslen(pszCurrPath) * sizeof(TCHAR))) &&
              (szPath[j] == _T('-')) && (szPath[j + 1] == _T('>'))))
         {
            HTREEITEM hSubTree;
            TCHAR *pszName;

            // Subtree of current tree
            if (*pszCurrPath != 0)
               j += 2;
            pszName = &szPath[j];
            for(; (szPath[j] != 0) && (memcmp(&szPath[j], _T("->"), sizeof(TCHAR) * 2)); j++);
            szPath[j] = 0;
            StrStrip(pszName);
				hSubTree = m_wndTreeCtrl.InsertItem(pszName, 1, 1, hRoot);
				m_wndTreeCtrl.SetItemData(hSubTree, 0);
				CreateGraphTree(hSubTree, szPath, &i);
         }
         else
         {
            i--;
            break;
         }
		}
	}
   *pdwStart = i;
}


//
// Handler for tree item expand/collapse
//

void CGraphManagerDlg::OnItemexpandedTreeGraphs(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMTREEVIEW* pNMTreeView = (NMTREEVIEW *)pNMHDR;

	if (pNMTreeView->action == TVE_COLLAPSE)
	{
		m_wndTreeCtrl.SetItemImage(pNMTreeView->itemNew.hItem, 1, 1);
	}
	else if (pNMTreeView->action == TVE_EXPAND)
	{
		m_wndTreeCtrl.SetItemImage(pNMTreeView->itemNew.hItem, 2, 2);
	}
	*pResult = 0;
}


//
// WM_CONTEXTMENU message handler
//

void CGraphManagerDlg::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CPoint pt;
   HTREEITEM hItem;
   UINT uFlags;
	RECT rect;
	CMenu *pMenu;

	if (pWnd->GetDlgCtrlID() != IDC_TREE_GRAPHS)
		return;

	if (point.x >= 0)
	{
		pt = point;
		pWnd->ScreenToClient(&pt);
		hItem = m_wndTreeCtrl.HitTest(pt, &uFlags);
		pt = point;
	}
	else
	{
		hItem = m_wndTreeCtrl.GetSelectedItem();
		if (hItem != NULL)
		{
			m_wndTreeCtrl.GetItemRect(hItem, &rect, TRUE);
			pt.x = rect.left;
			pt.y = rect.bottom;
			m_wndTreeCtrl.ClientToScreen(&pt);
		}
	}
   if ((hItem != NULL) && ((uFlags & TVHT_ONITEM) || (point.x < 0)))
   {
      m_wndTreeCtrl.Select(hItem, TVGN_CARET);
      pMenu = theApp.GetContextMenu(23);
      pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, this, NULL);
	}
}


//
// Delete button handler
//

void CGraphManagerDlg::OnButtonDelete() 
{
	PostMessage(WM_COMMAND, ID_GRAPHMANAGER_DELETE, 0);
}


//
// Delete graph or subtree of graphs
//

BOOL CGraphManagerDlg::DeleteGraph(HTREEITEM hItem)
{
	DWORD dwGraphId, dwResult;
	HTREEITEM hChild;
	BOOL bRet = TRUE;

	dwGraphId = m_wndTreeCtrl.GetItemData(hItem);
	if (dwGraphId == 0)	// Folder
	{
		while(((hChild = m_wndTreeCtrl.GetChildItem(hItem)) != NULL) && bRet)
		{
			bRet = DeleteGraph(hChild);
		}
		if (bRet)
		{
			m_wndTreeCtrl.DeleteItem(hItem);
		}
	}
	else	// Leaf item
	{
		dwResult = DoRequestArg2(NXCDeleteGraph, g_hSession, (void *)dwGraphId, _T("Deleting graph..."));
		if (dwResult == RCC_SUCCESS)
		{
			m_wndTreeCtrl.DeleteItem(hItem);
		}
		else
		{
			theApp.ErrorBox(dwResult, _T("Cannot delete predefined graph: %s"));
			bRet = FALSE;
		}
	}
	return bRet;
}


//
// Handler for "Delete" item in context menu
//

void CGraphManagerDlg::OnGraphmanagerDelete() 
{
	HTREEITEM hItem, hParent;

	hItem = m_wndTreeCtrl.GetSelectedItem();
	if (hItem == NULL)
		return;

	hParent = m_wndTreeCtrl.GetParentItem(hItem);
	if (DeleteGraph(hItem))
	{
		while((hParent != NULL) && (m_wndTreeCtrl.GetChildItem(hParent) == NULL))
		{
			hItem = hParent;
			hParent = m_wndTreeCtrl.GetParentItem(hItem);
			m_wndTreeCtrl.DeleteItem(hItem);
		}
	}
}
