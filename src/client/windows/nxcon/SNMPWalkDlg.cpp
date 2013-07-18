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


//
// Walker arguments
//

typedef struct
{
   HWND hWnd;
   CListCtrl *pListCtrl;
   UINT32 dwObjectId;
   TCHAR *pszRootOID;
} WALKER_ARGS;


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
   ON_MESSAGE(NXCM_REQUEST_COMPLETED, OnRequestCompleted)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSNMPWalkDlg message handlers

BOOL CSNMPWalkDlg::OnInitDialog() 
{
   RECT rect;

	CDialog::OnInitDialog();
	
   m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.GetClientRect(&rect);
   m_wndListCtrl.InsertColumn(0, _T("OID"), LVCFMT_LEFT, 200);
   m_wndListCtrl.InsertColumn(1, _T("Type"), LVCFMT_LEFT, 70);
   m_wndListCtrl.InsertColumn(2, _T("Value"), LVCFMT_LEFT, rect.right - 270 - GetSystemMetrics(SM_CXVSCROLL));

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
	return TRUE;
}


//
// Walker callback
//

static void WalkerCallback(TCHAR *pszName, UINT32 dwType, TCHAR *pszValue, void *pArg)
{
   int iItem;
	TCHAR typeName[256];

   iItem = ((CListCtrl *)pArg)->InsertItem(0x7FFFFFFF, pszName);
   if (iItem != -1)
   {
		if (dwType == 0xFFFF)	// Octet string converted to hex
			((CListCtrl *)pArg)->SetItemText(iItem, 1, _T("Hex-STRING"));
		else
			((CListCtrl *)pArg)->SetItemText(iItem, 1, SNMPDataTypeName(dwType, typeName, 256));
      ((CListCtrl *)pArg)->SetItemText(iItem, 2, pszValue);
      ((CListCtrl *)pArg)->EnsureVisible(iItem, FALSE);
   }
}


//
// Walker thread
//

static THREAD_RESULT THREAD_CALL WalkerThread(void *pArg)
{
   UINT32 dwResult;

   dwResult = NXCSnmpWalk(g_hSession, ((WALKER_ARGS *)pArg)->dwObjectId,
                          ((WALKER_ARGS *)pArg)->pszRootOID,
                          ((WALKER_ARGS *)pArg)->pListCtrl, WalkerCallback);
   PostMessage(((WALKER_ARGS *)pArg)->hWnd, NXCM_REQUEST_COMPLETED, 0, dwResult);
   free(((WALKER_ARGS *)pArg)->pszRootOID);
   free(pArg);
	return THREAD_OK;
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CSNMPWalkDlg::OnViewRefresh() 
{
   WALKER_ARGS *pArgs;

   pArgs = (WALKER_ARGS *)malloc(sizeof(WALKER_ARGS));
   pArgs->hWnd = m_hWnd;
   pArgs->dwObjectId = m_dwObjectId;
   pArgs->pListCtrl = &m_wndListCtrl;
   pArgs->pszRootOID = _tcsdup((LPCTSTR)m_strRootOID);
   EnableDlgItem(this, IDCANCEL, FALSE);
   m_wndStatus.SetWindowText(_T("Retrieving data..."));
   ThreadCreate(WalkerThread, 0, pArgs);
   BeginWaitCursor();
}


//
// WM_REQUEST_COMPLETED message handler
//

LRESULT CSNMPWalkDlg::OnRequestCompleted(WPARAM wParam, LPARAM lParam)
{
   EnableDlgItem(this, IDCANCEL, TRUE);
   m_wndStatus.SetWindowText(_T("Completed"));
   EndWaitCursor();
	return 0;
}
