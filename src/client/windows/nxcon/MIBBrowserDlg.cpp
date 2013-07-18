// MIBBrowserDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "MIBBrowserDlg.h"
#include "DataQueryDlg.h"
#include "ObjectSelDlg.h"
#include "SNMPWalkDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


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
	m_dwInstance = 0;
	//}}AFX_DATA_INIT
   m_bIsExpanded = FALSE;
   m_bDisableOIDUpdate = FALSE;
   m_bDisableSelUpdate = FALSE;
   m_pNode = NULL;
   m_bUseInstance = TRUE;
   m_iOrigin = DS_SNMP_AGENT;
}


void CMIBBrowserDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMIBBrowserDlg)
	DDX_Control(pDX, IDC_EDIT_INSTANCE, m_wndEditInstance);
	DDX_Control(pDX, IDC_EDIT_TYPE, m_wndEditType);
	DDX_Control(pDX, IDC_EDIT_STATUS, m_wndEditStatus);
	DDX_Control(pDX, IDC_EDIT_OID_TEXT, m_wndEditOIDText);
	DDX_Control(pDX, IDC_EDIT_ACCESS, m_wndEditAccess);
	DDX_Control(pDX, IDC_BUTTON_DETAILS, m_wndButtonDetails);
	DDX_Control(pDX, IDC_EDIT_DESCRIPTION, m_wndEditDescription);
	DDX_Control(pDX, IDC_TREE_MIB, m_wndTreeCtrl);
	DDX_Control(pDX, IDC_EDIT_OID, m_wndEditOID);
	DDX_Text(pDX, IDC_EDIT_OID, m_strOID);
	DDX_Text(pDX, IDC_EDIT_INSTANCE, m_dwInstance);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMIBBrowserDlg, CDialog)
	//{{AFX_MSG_MAP(CMIBBrowserDlg)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_MIB, OnSelchangedTreeMib)
	ON_BN_CLICKED(IDC_BUTTON_DETAILS, OnButtonDetails)
	ON_WM_CTLCOLOR()
	ON_EN_CHANGE(IDC_EDIT_OID, OnChangeEditOid)
	ON_EN_CHANGE(IDC_EDIT_INSTANCE, OnChangeEditInstance)
	ON_BN_CLICKED(IDC_BUTTON_GET, OnButtonGet)
	ON_BN_CLICKED(IDC_BUTTON_WALK, OnButtonWalk)
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
   UINT32 dwOIDLen, oid[MAX_OID_LEN];

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
	AddTreeNode(TVI_ROOT, g_pMIBRoot);

   // Select node with given OID (or closest match)
   dwOIDLen = SNMPParseOID(m_strOID, oid, MAX_OID_LEN);
   if (dwOIDLen > 0)
   {
      SelectNode(m_wndTreeCtrl.GetChildItem(TVI_ROOT), oid, dwOIDLen);
   }

   // Change dialog title
   if (m_pNode != NULL)
   {
      CString strTitle;

      GetWindowText(strTitle);
      strTitle += " (";
      strTitle += m_pNode->szName;
      strTitle += ")";
      SetWindowText(strTitle);
   }

   // Disable "instance" field if it's not needed
   // If instance is disabled, hide "Get..." button as well
   if (!m_bUseInstance)
   {
      CWnd *pWnd;

      m_wndEditInstance.EnableWindow(FALSE);
      m_wndEditInstance.SetWindowText(_T(""));
      pWnd = GetDlgItem(IDC_STATIC_INSTANCE);
      if (pWnd != NULL)
         pWnd->EnableWindow(FALSE);

      pWnd = GetDlgItem(IDC_BUTTON_GET);
      if (pWnd != NULL)
         pWnd->ShowWindow(SW_HIDE);
   }

	return TRUE;
}


//
// Add node to tree
//

void CMIBBrowserDlg::AddTreeNode(HTREEITEM hRoot, SNMP_MIBObject *pCurrNode)
{
   SNMP_MIBObject *pNode;
   HTREEITEM hItem;

   hItem = m_wndTreeCtrl.InsertItem(CHECK_NULL(pCurrNode->getName()), 0, 0, hRoot, TVI_LAST);
   m_wndTreeCtrl.SetItemData(hItem, (DWORD)pCurrNode);
   for(pNode = pCurrNode->getFirstChild(); pNode != NULL; pNode = pNode->getNext())
      AddTreeNode(hItem, pNode);
   m_wndTreeCtrl.SortChildren(hItem);
}


//
// Handler for selection change in MIB tree
//

void CMIBBrowserDlg::OnSelchangedTreeMib(NMHDR *pNMHDR, LRESULT *pResult) 
{
	NMTREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
   SNMP_MIBObject *pNode;
   TCHAR *pszTemp, szBuffer[MAX_OID_LEN * 8];

   pNode = (SNMP_MIBObject *)m_wndTreeCtrl.GetItemData(pNMTreeView->itemNew.hItem);

   if (!m_bDisableOIDUpdate)
   {
      BuildOIDString(pNode, szBuffer);
      m_bDisableSelUpdate = TRUE;
      m_wndEditOID.SetWindowText(szBuffer);
      m_bDisableSelUpdate = FALSE;
   }

   pszTemp = TranslateUNIXText(CHECK_NULL_EX(pNode->getDescription()));
   m_wndEditDescription.SetWindowText(pszTemp);
   free(pszTemp);

   m_wndEditStatus.SetWindowText(CodeToText(pNode->getStatus(), g_ctSnmpMibStatus, _T("")));
   m_wndEditAccess.SetWindowText(CodeToText(pNode->getAccess(), g_ctSnmpMibAccess, _T("")));
   m_wndEditType.SetWindowText(CodeToText(pNode->getType(), g_ctSnmpMibType, _T("")));
   m_iSnmpDataType = pNode->getType();   // Store data type of current node

   pszTemp = BuildSymbolicOIDString(pNode, m_dwInstance);
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
   m_wndButtonDetails.SetWindowText(_T("&Details <<<"));
   m_bIsExpanded = TRUE;
}


//
// Collapse dialog box
//

void CMIBBrowserDlg::Collapse()
{
   ShowExtControls(FALSE);
   SetWindowPos(NULL, 0, 0, m_sizeCollapsed.cx, m_sizeCollapsed.cy, SWP_NOZORDER | SWP_NOMOVE);
   m_wndButtonDetails.SetWindowText(_T("&Details >>>"));
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
   if ((nId == IDC_EDIT_DESCRIPTION) ||
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

void CMIBBrowserDlg::SelectNode(HTREEITEM hRoot, UINT32 *pdwOID, unsigned int uNameLen)
{
   HTREEITEM hItem;
   SNMP_MIBObject *pNode;
   BOOL bMatch = FALSE;

   for(hItem = m_wndTreeCtrl.GetChildItem(hRoot); hItem != NULL; 
       hItem = m_wndTreeCtrl.GetNextItem(hItem, TVGN_NEXT))
   {
      pNode = (SNMP_MIBObject *)m_wndTreeCtrl.GetItemData(hItem);
      if (pNode->getObjectId() == pdwOID[0])
      {
         bMatch = TRUE;
         if (uNameLen == 1)
            m_wndTreeCtrl.SelectItem(hItem);
         else
            SelectNode(hItem, &pdwOID[1], uNameLen - 1);
      }
   }
   if ((!bMatch) && (hRoot != TVI_ROOT))
      m_wndTreeCtrl.SelectItem(hRoot);
}


//
// Handle manual OID change
//

void CMIBBrowserDlg::OnChangeEditOid() 
{
   TCHAR szBuffer[1024];
   UINT32 dwOIDLen, oid[MAX_OID_LEN];

   // Select node with given OID (or closest match)
   if (!m_bDisableSelUpdate)
   {
      m_wndEditOID.GetWindowText(szBuffer, 1024);
      dwOIDLen = SNMPParseOID(szBuffer, oid, MAX_OID_LEN);
      if (dwOIDLen > 0)
      {
         m_bDisableOIDUpdate = TRUE;   // Prevent edit box text replacement when tree selection changes
         SelectNode(m_wndTreeCtrl.GetChildItem(TVI_ROOT), oid, dwOIDLen);
         m_bDisableOIDUpdate = FALSE;
      }
   }
}


//
// Handle instance change
//

void CMIBBrowserDlg::OnChangeEditInstance() 
{
   TCHAR szBuffer[32];
   HTREEITEM hItem;

   m_wndEditInstance.GetWindowText(szBuffer, 32);
   m_dwInstance = _tcstoul(szBuffer, NULL, 10);

   // Update symbolic OID
   hItem = m_wndTreeCtrl.GetNextItem(NULL, TVGN_CARET);
   if (hItem != NULL)
   {
      SNMP_MIBObject *pNode;
      TCHAR *pszTemp;

      pNode = (SNMP_MIBObject *)m_wndTreeCtrl.GetItemData(hItem);

      pszTemp = BuildSymbolicOIDString(pNode, m_dwInstance);
      m_wndEditOIDText.SetWindowText(pszTemp);
      free(pszTemp);
   }
}


//
// Get requested parameter from host immediately
//

void CMIBBrowserDlg::OnButtonGet() 
{
   // Select node object if none selected
   if (m_pNode == NULL)
   {
      CObjectSelDlg dlg;

      dlg.m_dwAllowedClasses = SCL_NODE;
      if (dlg.DoModal() == IDOK)
      {
         m_pNode = NXCFindObjectById(g_hSession, dlg.m_pdwObjectList[0]);
         if (m_pNode != NULL)
         {
            CString strTitle;

            GetWindowText(strTitle);
            strTitle += " (";
            strTitle += m_pNode->szName;
            strTitle += ")";
            SetWindowText(strTitle);
         }
      }
   }

   // If node is selected, continue
   if (m_pNode != NULL)
   {
      CDataQueryDlg dlg;
      TCHAR szBuffer[1024];

      m_wndEditOID.GetWindowText(szBuffer, 1024);
      dlg.m_dwObjectId = m_pNode->dwId;
      dlg.m_strNode = (LPCTSTR)m_pNode->szName;
      dlg.m_strParameter = (LPCTSTR)szBuffer;
      _sntprintf_s(szBuffer, 1024, _TRUNCATE, _T(".%d"), m_dwInstance);
      dlg.m_strParameter += szBuffer;
      dlg.m_iOrigin = m_iOrigin;
      dlg.DoModal();
   }
}


//
// Handler for "OK" button
//

void CMIBBrowserDlg::OnOK() 
{
   // To avoid errors in data exchange we should
   // set input box text to correct number
   if (!m_bUseInstance)
      m_wndEditInstance.SetWindowText(_T("0"));

   // Convert SNMP data type to NetXMS data type
   switch(m_iSnmpDataType)
   {
      case MIB_TYPE_INTEGER:
      case MIB_TYPE_INTEGER32:
         m_iDataType = DCI_DT_INT;
         break;
      case MIB_TYPE_COUNTER:
      case MIB_TYPE_COUNTER32:
      case MIB_TYPE_GAUGE:
      case MIB_TYPE_GAUGE32:
      case MIB_TYPE_TIMETICKS:
      case MIB_TYPE_UINTEGER:
      case MIB_TYPE_UNSIGNED32:
         m_iDataType = DCI_DT_UINT;
         break;
      case MIB_TYPE_COUNTER64:
         m_iDataType = DCI_DT_UINT64;
         break;
      default:
         m_iDataType = DCI_DT_STRING;
         break;
   }
	
	CDialog::OnOK();
}


//
// Walk target's MIB
//

void CMIBBrowserDlg::OnButtonWalk() 
{
   // Select node object if none selected
   if (m_pNode == NULL)
   {
      CObjectSelDlg dlg;

      dlg.m_dwAllowedClasses = SCL_NODE;
      if (dlg.DoModal() == IDOK)
      {
         m_pNode = NXCFindObjectById(g_hSession, dlg.m_pdwObjectList[0]);
         if (m_pNode != NULL)
         {
            CString strTitle;

            GetWindowText(strTitle);
            strTitle += " (";
            strTitle += m_pNode->szName;
            strTitle += ")";
            SetWindowText(strTitle);
         }
      }
   }

   // If node is selected, continue
   if (m_pNode != NULL)
   {
      CSNMPWalkDlg dlg;
      TCHAR szBuffer[1024];

      m_wndEditOID.GetWindowText(szBuffer, 1024);
      dlg.m_dwObjectId = m_pNode->dwId;
      dlg.m_strNode = (LPCTSTR)m_pNode->szName;
      dlg.m_strRootOID = (LPCTSTR)szBuffer;
      dlg.DoModal();
   }
}
