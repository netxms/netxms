// MIBBrowserDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "MIBBrowserDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Externals
//

void BuildOIDString(struct tree *pNode, char *pszBuffer);
char *BuildSymbolicOIDString(struct tree *pNode);


//
// Control id's in expandable area
//

static int m_iExtCtrlId[] = { IDC_STATIC_DESCRIPTION, IDC_EDIT_DESCRIPTION,
                              IDC_STATIC_TYPE, IDC_EDIT_TYPE, IDC_STATIC_ACCESS,
                              IDC_EDIT_ACCESS, IDC_STATIC_STATUS, IDC_EDIT_STATUS, -1 };


/////////////////////////////////////////////////////////////////////////////
// CMIBBrowserDlg dialog


CMIBBrowserDlg::CMIBBrowserDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMIBBrowserDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMIBBrowserDlg)
	m_strOID = _T("");
	//}}AFX_DATA_INIT
   m_bIsExpanded = FALSE;
}


void CMIBBrowserDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMIBBrowserDlg)
	DDX_Control(pDX, IDC_EDIT_TYPE, m_wndEditType);
	DDX_Control(pDX, IDC_EDIT_STATUS, m_wndEditStatus);
	DDX_Control(pDX, IDC_EDIT_OID_TEXT, m_wndEditOIDText);
	DDX_Control(pDX, IDC_EDIT_ACCESS, m_wndEditAccess);
	DDX_Control(pDX, IDC_BUTTON_DETAILS, m_wndButtonDetails);
	DDX_Control(pDX, IDC_EDIT_DESCRIPTION, m_wndEditDescription);
	DDX_Control(pDX, IDC_TREE_MIB, m_wndTreeCtrl);
	DDX_Control(pDX, IDC_EDIT_OID, m_wndEditOID);
	DDX_Text(pDX, IDC_EDIT_OID, m_strOID);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMIBBrowserDlg, CDialog)
	//{{AFX_MSG_MAP(CMIBBrowserDlg)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_MIB, OnSelchangedTreeMib)
	ON_BN_CLICKED(IDC_BUTTON_DETAILS, OnButtonDetails)
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMIBBrowserDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CMIBBrowserDlg::OnInitDialog() 
{
   RECT rect1, rect2;
   oid oidName[MAX_OID_LEN];
   unsigned int uNameLen = MAX_OID_LEN;

	CDialog::OnInitDialog();

   // Get expanded dialog box size
   m_sizeExpanded = GetWindowSize(this);

   // Calculate collapsed dialog box size
   m_wndButtonDetails.GetWindowRect(&rect1);
   ScreenToClient(&rect1);
   m_wndEditDescription.GetWindowRect(&rect2);
   ScreenToClient(&rect2);
   m_sizeCollapsed = m_sizeExpanded;
   m_sizeCollapsed.cy -= rect2.bottom - rect1.bottom;

   // By default, dialog is collapsed
   Collapse();

   // Fill MIB tree
   memset(&m_mibTreeRoot, 0, sizeof(struct tree));
   m_mibTreeRoot.label = "[root]";
   m_mibTreeRoot.child_list = get_tree_head();
	AddTreeNode(TVI_ROOT, &m_mibTreeRoot);

   // Select node with given OID (or closest match)
   if (read_objid((LPCTSTR)m_strOID, oidName, &uNameLen) != 0)
      SelectNode(m_wndTreeCtrl.GetChildItem(TVI_ROOT), oidName, uNameLen);
	return TRUE;
}


//
// Add node to tree
//

void CMIBBrowserDlg::AddTreeNode(HTREEITEM hRoot, tree *pCurrNode)
{
   struct tree *pNode;
   HTREEITEM hItem;

   hItem = m_wndTreeCtrl.InsertItem(pCurrNode->label, 0, 0, hRoot, TVI_LAST);
   m_wndTreeCtrl.SetItemData(hItem, (DWORD)pCurrNode);
   for(pNode = pCurrNode->child_list; pNode != NULL; pNode = pNode->next_peer)
      AddTreeNode(hItem, pNode);
   m_wndTreeCtrl.SortChildren(hItem);
}


//
// Handler for selection change in MIB tree
//

void CMIBBrowserDlg::OnSelchangedTreeMib(NMHDR *pNMHDR, LRESULT *pResult) 
{
	NMTREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
   struct tree *pNode;
   char *pszTemp, szBuffer[MAX_OID_LEN * 8];

   pNode = (struct tree *)m_wndTreeCtrl.GetItemData(pNMTreeView->itemNew.hItem);
   BuildOIDString(pNode, szBuffer);
   m_wndEditOID.SetWindowText(szBuffer);
   pszTemp = TranslateUNIXText(pNode->description != NULL ? pNode->description : "");
   m_wndEditDescription.SetWindowText(pszTemp);
   free(pszTemp);
   m_wndEditStatus.SetWindowText(CodeToText(pNode->status, g_ctSnmpMibStatus, ""));
   m_wndEditAccess.SetWindowText(CodeToText(pNode->access, g_ctSnmpMibAccess, ""));
   m_wndEditType.SetWindowText(CodeToText(pNode->type, g_ctSnmpMibType, ""));
   pszTemp = BuildSymbolicOIDString(pNode);
   m_wndEditOIDText.SetWindowText(pszTemp);
   free(pszTemp);
	*pResult = 0;
}


//
// Expand dialog box
//

void CMIBBrowserDlg::Expand()
{
   SetWindowPos(NULL, 0, 0, m_sizeExpanded.cx, m_sizeExpanded.cy, SWP_NOZORDER | SWP_NOMOVE);
   ShowExtControls(TRUE);
   m_wndButtonDetails.SetWindowText("&Details <<<");
   m_bIsExpanded = TRUE;
}


//
// Collapse dialog box
//

void CMIBBrowserDlg::Collapse()
{
   ShowExtControls(FALSE);
   SetWindowPos(NULL, 0, 0, m_sizeCollapsed.cx, m_sizeCollapsed.cy, SWP_NOZORDER | SWP_NOMOVE);
   m_wndButtonDetails.SetWindowText("&Details >>>");
   m_bIsExpanded = FALSE;
}


//
// Show or hide controls in expandable area
//

void CMIBBrowserDlg::ShowExtControls(BOOL bShow)
{
   int i;

   for(i = 0; m_iExtCtrlId[i] != -1; i++)
      ::ShowWindow(::GetDlgItem(m_hWnd, m_iExtCtrlId[i]), bShow ? SW_SHOW : SW_HIDE);
}


//
// WM_COMMAND::IDC_BUTTON_DETAILS message handler
//

void CMIBBrowserDlg::OnButtonDetails() 
{
   if (m_bIsExpanded)
      Collapse();
   else
      Expand();
}


//
// WM_CTLCOLOR message handler
//

HBRUSH CMIBBrowserDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
   int nId;
   HBRUSH hbr;

   nId = pWnd->GetDlgCtrlID();
   if ((nId == IDC_EDIT_DESCRIPTION) || (nId == IDC_EDIT_OID) ||
       (nId == IDC_EDIT_STATUS) || (nId == IDC_EDIT_OID_TEXT) ||
       (nId == IDC_EDIT_ACCESS) || (nId == IDC_EDIT_TYPE))
   {
      hbr = GetSysColorBrush(COLOR_INFOBK);
      pDC->SetTextColor(GetSysColor(COLOR_INFOTEXT));
      pDC->SetBkColor(GetSysColor(COLOR_INFOBK));
   }
   else
   {
   	hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
   }
	return hbr;
}


//
// Select node with given OID
//

void CMIBBrowserDlg::SelectNode(HTREEITEM hRoot, oid *oidName, unsigned int uNameLen)
{
   HTREEITEM hItem;
   struct tree *pNode;
   BOOL bMatch = FALSE;

   for(hItem = m_wndTreeCtrl.GetChildItem(hRoot); hItem != NULL; 
       hItem = m_wndTreeCtrl.GetNextItem(hItem, TVGN_NEXT))
   {
      pNode = (struct tree *)m_wndTreeCtrl.GetItemData(hItem);
      if (pNode->subid == oidName[0])
      {
         bMatch = TRUE;
         if (uNameLen == 1)
            m_wndTreeCtrl.SelectItem(hItem);
         else
            SelectNode(hItem, &oidName[1], uNameLen - 1);
      }
   }
   if ((!bMatch) && (hRoot != TVI_ROOT))
      m_wndTreeCtrl.SelectItem(hRoot);
}
