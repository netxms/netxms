// SNMPWalkDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "SNMPWalkDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSNMPWalkDlg dialog


CSNMPWalkDlg::CSNMPWalkDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSNMPWalkDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSNMPWalkDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CSNMPWalkDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSNMPWalkDlg)
	DDX_Control(pDX, IDC_LIST_DATA, m_wndListCtrl);
	DDX_Control(pDX, IDC_STATIC_STATUS, m_wndStatus);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSNMPWalkDlg, CDialog)
	//{{AFX_MSG_MAP(CSNMPWalkDlg)
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSNMPWalkDlg message handlers

BOOL CSNMPWalkDlg::OnInitDialog() 
{
   RECT rect;

	CDialog::OnInitDialog();
	
   m_wndListCtrl.GetClientRect(&rect);
   m_wndListCtrl.InsertColumn(0, _T("OID"), LVCFMT_LEFT, 200);
   m_wndListCtrl.InsertColumn(1, _T("Type"), LVCFMT_LEFT, 70);
   m_wndListCtrl.InsertColumn(2, _T("Value"), LVCFMT_LEFT, rect.right - GetSystemMetrics(SM_CXVSCROLL));

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
	return TRUE;
}


//
// Walker callback
//

static void WalkerCallback(TCHAR *pszName, DWORD dwType, TCHAR *pszValue, void *pArg)
{
   int iItem;

   iItem = ((CListCtrl *)pArg)->InsertItem(0x7FFFFFFF, pszName);
   if (iItem != -1)
   {
      ((CListCtrl *)pArg)->SetItemText(iItem, 2, pszValue);
   }
}


//
// Walker thread
//

/*static void WalkerThread(void *pArg)
{
   DWORD dwResult;

   dwResult = NXCSnmpWalk(g_hSession, 
}*/


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CSNMPWalkDlg::OnViewRefresh() 
{
   //EnableDlgItem(this, IDCANCEL, FALSE);
   //m_wndStatus.SetWindowText(_T("Processing..."));
  // _beginthread(WalkerThread, 0, &m_wndListCtrl);
   DoRequestArg5(NXCSnmpWalk, g_hSession, CAST_TO_POINTER(m_dwObjectId, void *),
                 (void *)((LPCTSTR)m_strRootOID), &m_wndListCtrl, WalkerCallback,
                 _T("Retrieving data..."));
}
