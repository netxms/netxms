// TrapEditDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "TrapEditDlg.h"
#include "MIBBrowserDlg.h"
#include "TrapParamDlg.h"

#define NXSNMP_WITH_NET_SNMP
#include <nxsnmp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTrapEditDlg dialog


CTrapEditDlg::CTrapEditDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTrapEditDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTrapEditDlg)
	//}}AFX_DATA_INIT

   memset(&m_trap, 0, sizeof(NXC_TRAP_CFG_ENTRY));
}

CTrapEditDlg::~CTrapEditDlg()
{
   DWORD i;

   safe_free(m_trap.pdwObjectId);
   for(i = 0; i < m_trap.dwNumMaps; i++)
      safe_free(m_trap.pMaps[i].pdwObjectId);
   safe_free(m_trap.pMaps);
}

void CTrapEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTrapEditDlg)
	DDX_Control(pDX, IDC_EVENT_ICON, m_wndEventIcon);
	DDX_Control(pDX, IDC_EDIT_DESCRIPTION, m_wndEditDescr);
	DDX_Control(pDX, IDC_EDIT_TRAP, m_wndEditOID);
	DDX_Control(pDX, IDC_LIST_ARGS, m_wndArgList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTrapEditDlg, CDialog)
	//{{AFX_MSG_MAP(CTrapEditDlg)
	ON_BN_CLICKED(IDC_SELECT_TRAP, OnSelectTrap)
	ON_BN_CLICKED(IDC_SELECT_EVENT, OnSelectEvent)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
	ON_BN_CLICKED(IDC_BUTTON_EDIT, OnButtonEdit)
	ON_BN_CLICKED(IDC_BUTTON_UP, OnButtonUp)
	ON_BN_CLICKED(IDC_BUTTON_DOWN, OnButtonDown)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_ARGS, OnItemchangedListArgs)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_ARGS, OnDblclkListArgs)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTrapEditDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CTrapEditDlg::OnInitDialog() 
{
   TCHAR szBuffer[1024];
   RECT rect;
   DWORD i;

	CDialog::OnInitDialog();

   m_wndEditDescr.SetWindowText(m_trap.szDescription);
   SNMPConvertOIDToText(m_trap.dwOidLen, m_trap.pdwObjectId, szBuffer, 1024);
   m_wndEditOID.SetWindowText(szBuffer);

   UpdateEventInfo();

   // Setup parameter list
   m_wndArgList.SetExtendedStyle(LVS_EX_FULLROWSELECT);
   m_wndArgList.GetClientRect(&rect);
   m_wndArgList.InsertColumn(0, _T("No."), LVCFMT_LEFT, 50);
   m_wndArgList.InsertColumn(1, _T("Variable"), LVCFMT_LEFT, 
                             rect.right - 50 - GetSystemMetrics(SM_CXVSCROLL));
   for(i = 0; i < m_trap.dwNumMaps; i++)
      AddParameterEntry(i);

   // Initially, there are no selection in list control, so all buttons
   // except "Add" should be disabled
   ::EnableWindow(::GetDlgItem(m_hWnd, IDC_BUTTON_EDIT), FALSE);
   ::EnableWindow(::GetDlgItem(m_hWnd, IDC_BUTTON_DELETE), FALSE);
   ::EnableWindow(::GetDlgItem(m_hWnd, IDC_BUTTON_UP), FALSE);
   ::EnableWindow(::GetDlgItem(m_hWnd, IDC_BUTTON_DOWN), FALSE);

	return TRUE;
}


//
// Handler for "Select" button for trap OID
//

void CTrapEditDlg::OnSelectTrap() 
{
   CMIBBrowserDlg dlg;
   TCHAR szBuffer[1024];

   dlg.m_pNode = NULL;
   m_wndEditOID.GetWindowText(szBuffer, 1024);
   dlg.m_bUseInstance = FALSE;
   dlg.m_strOID = szBuffer;
   if (dlg.DoModal() == IDOK)
   {
      m_wndEditOID.SetWindowText(dlg.m_strOID);
   }
}


//
// Handler for OK button
//

void CTrapEditDlg::OnOK() 
{
   DWORD pdwOid[MAX_OID_LEN];
   TCHAR szBuffer[1024];

   m_wndEditOID.GetWindowText(szBuffer, 1024);
	m_trap.dwOidLen = SNMPParseOID(szBuffer, pdwOid, MAX_OID_LEN);
   if (m_trap.dwOidLen == 0)
   {
      MessageBox(_T("Invalid trap OID entered"), _T("Error"), MB_OK | MB_ICONSTOP);
   }
   else
   {
      m_trap.pdwObjectId = (DWORD *)realloc(m_trap.pdwObjectId, sizeof(DWORD) * m_trap.dwOidLen);
      memcpy(m_trap.pdwObjectId, pdwOid, sizeof(DWORD) * m_trap.dwOidLen);

      m_wndEditDescr.GetWindowText(m_trap.szDescription, MAX_DB_STRING);

   	CDialog::OnOK();
   }
}


//
// Update selected event information in appropriate controls
//

void CTrapEditDlg::UpdateEventInfo()
{
   TCHAR szBuffer[MAX_DB_STRING];
   int iSeverity;
   static UINT nIconID[] = { IDI_SEVERITY_NORMAL, IDI_SEVERITY_WARNING, IDI_SEVERITY_MINOR,
                             IDI_SEVERITY_MAJOR, IDI_SEVERITY_CRITICAL };

   NXCGetEventText(m_trap.dwEventCode, szBuffer, MAX_DB_STRING);
   SetDlgItemText(IDC_EDIT_MESSAGE, szBuffer);

   NXCGetEventNameEx(m_trap.dwEventCode, szBuffer, MAX_DB_STRING);
   SetDlgItemText(IDC_EDIT_EVENT, szBuffer);

   iSeverity = NXCGetEventSeverity(m_trap.dwEventCode);
   if (iSeverity != -1)
      m_wndEventIcon.SetIcon(theApp.LoadIcon(nIconID[iSeverity]));
   else
      m_wndEventIcon.SetIcon(NULL);
}


//
// Handler for "Select" button for event
//

void CTrapEditDlg::OnSelectEvent() 
{
   CEventSelDlg dlg;

   dlg.m_bSingleSelection = TRUE;
   if (dlg.DoModal() == IDOK)
   {
      m_trap.dwEventCode = dlg.m_pdwEventList[0];
      UpdateEventInfo();
   }
}


//
// Add new parameter mapping entry to list
//

void CTrapEditDlg::AddParameterEntry(DWORD dwIndex)
{
   TCHAR szBuffer[32];
   int iItem;

   _stprintf(szBuffer, _T("%d"), dwIndex + 2);
   iItem = m_wndArgList.InsertItem(dwIndex, szBuffer);
   if ((DWORD)iItem != dwIndex)
      MessageBox(_T("Internal error: iItem != dwIndex"), _T("Error"), MB_OK | MB_ICONSTOP);
   else
      UpdateParameterEntry(dwIndex);
}


//
// Update parameter mapping entry in list
//

void CTrapEditDlg::UpdateParameterEntry(DWORD dwIndex)
{
   TCHAR szBuffer[1024];

   SNMPConvertOIDToText(m_trap.pMaps[dwIndex].dwOidLen, m_trap.pMaps[dwIndex].pdwObjectId,
                        szBuffer, 1024);
   m_wndArgList.SetItemText(dwIndex, 1, szBuffer);
}


//
// Handler for "Add" button
//

void CTrapEditDlg::OnButtonAdd() 
{
   CTrapParamDlg dlg;
   DWORD pdwOid[MAX_OID_LEN];

   if (dlg.DoModal() == IDOK)
   {
      m_trap.pMaps = 
         (NXC_OID_MAP *)realloc(m_trap.pMaps, sizeof(NXC_OID_MAP) * (m_trap.dwNumMaps + 1));
      _tcsncpy(m_trap.pMaps[m_trap.dwNumMaps].szDescription, 
               dlg.m_strDescription, MAX_DB_STRING);
      m_trap.pMaps[m_trap.dwNumMaps].dwOidLen = 
         SNMPParseOID(dlg.m_strOID, pdwOid, MAX_OID_LEN);
      m_trap.pMaps[m_trap.dwNumMaps].pdwObjectId = 
         (DWORD *)nx_memdup(pdwOid, sizeof(DWORD) * m_trap.pMaps[m_trap.dwNumMaps].dwOidLen);
      AddParameterEntry(m_trap.dwNumMaps);
      m_trap.dwNumMaps++;
   }
}


//
// Handler for "Delete" button
//

void CTrapEditDlg::OnButtonDelete() 
{
   int iItem;

   while(1)
   {
      iItem = m_wndArgList.GetNextItem(-1, LVIS_SELECTED);
      if (iItem == -1)
         break;
      m_wndArgList.DeleteItem(iItem);
      safe_free(m_trap.pMaps[iItem].pdwObjectId);
      m_trap.dwNumMaps--;
      memmove(&m_trap.pMaps[iItem], &m_trap.pMaps[iItem + 1],
              sizeof(NXC_OID_MAP) * (m_trap.dwNumMaps - iItem));
   }
}


//
// Handler for "Edit" button
//

void CTrapEditDlg::OnButtonEdit() 
{
   if (m_wndArgList.GetSelectedCount() == 1)
   {
      int iItem;

      iItem = m_wndArgList.GetNextItem(-1, LVIS_SELECTED);
      if (iItem != -1)
      {
         CTrapParamDlg dlg;
         DWORD pdwOid[MAX_OID_LEN];
         TCHAR szBuffer[1024];

         dlg.m_strDescription = m_trap.pMaps[iItem].szDescription;
         SNMPConvertOIDToText(m_trap.pMaps[iItem].dwOidLen, m_trap.pMaps[iItem].pdwObjectId,
                              szBuffer, 1024);
         dlg.m_strOID = szBuffer;
         if (dlg.DoModal() == IDOK)
         {
            _tcsncpy(m_trap.pMaps[iItem].szDescription, 
                     dlg.m_strDescription, MAX_DB_STRING);
            m_trap.pMaps[iItem].dwOidLen = 
               SNMPParseOID(dlg.m_strOID, pdwOid, MAX_OID_LEN);
            m_trap.pMaps[iItem].pdwObjectId = 
               (DWORD *)realloc(m_trap.pMaps[iItem].pdwObjectId, sizeof(DWORD) * m_trap.pMaps[iItem].dwOidLen);
            memcpy(m_trap.pMaps[iItem].pdwObjectId, pdwOid, sizeof(DWORD) * m_trap.pMaps[iItem].dwOidLen);
            UpdateParameterEntry(iItem);
         }
      }
   }
}


//
// Handler for "Up" button
//

void CTrapEditDlg::OnButtonUp() 
{
   if (m_wndArgList.GetSelectedCount() == 1)
   {
      int iItem;

      iItem = m_wndArgList.GetNextItem(-1, LVIS_SELECTED);
      if (iItem > 0)
      {
         nx_memswap(&m_trap.pMaps[iItem - 1], &m_trap.pMaps[iItem], sizeof(NXC_OID_MAP));
         UpdateParameterEntry(iItem - 1);
         UpdateParameterEntry(iItem);
         SelectListViewItem(&m_wndArgList, iItem - 1);
      }
   }
}


//
// Handler for "Down" button
//

void CTrapEditDlg::OnButtonDown() 
{
   if (m_wndArgList.GetSelectedCount() == 1)
   {
      int iItem;

      iItem = m_wndArgList.GetNextItem(-1, LVIS_SELECTED);
      if ((iItem != -1) && (iItem < (int)m_trap.dwNumMaps - 1))
      {
         nx_memswap(&m_trap.pMaps[iItem], &m_trap.pMaps[iItem + 1], sizeof(NXC_OID_MAP));
         UpdateParameterEntry(iItem);
         UpdateParameterEntry(iItem + 1);
         SelectListViewItem(&m_wndArgList, iItem + 1);
      }
   }
}


//
// Handle item changing in argument list
//

void CTrapEditDlg::OnItemchangedListArgs(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

   // Enable or disable buttons depending on number of selected items
   ::EnableWindow(::GetDlgItem(m_hWnd, IDC_BUTTON_EDIT), m_wndArgList.GetSelectedCount() == 1);
   ::EnableWindow(::GetDlgItem(m_hWnd, IDC_BUTTON_DELETE), m_wndArgList.GetSelectedCount() > 0);
   ::EnableWindow(::GetDlgItem(m_hWnd, IDC_BUTTON_UP), m_wndArgList.GetSelectedCount() == 1);
   ::EnableWindow(::GetDlgItem(m_hWnd, IDC_BUTTON_DOWN), m_wndArgList.GetSelectedCount() == 1);

   *pResult = 0;
}


//
// Handler for doble click inside argument list
//

void CTrapEditDlg::OnDblclkListArgs(NMHDR* pNMHDR, LRESULT* pResult) 
{
   PostMessage(WM_COMMAND, IDC_BUTTON_EDIT);
	
	*pResult = 0;
}
