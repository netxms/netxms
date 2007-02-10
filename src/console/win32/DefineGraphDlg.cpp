// DefineGraphDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "DefineGraphDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDefineGraphDlg dialog


CDefineGraphDlg::CDefineGraphDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDefineGraphDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDefineGraphDlg)
	m_strName = _T("");
	//}}AFX_DATA_INIT
	m_dwACLSize = 0;
	m_pACL = NULL;
}

CDefineGraphDlg::~CDefineGraphDlg()
{
	safe_free(m_pACL);
}

void CDefineGraphDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDefineGraphDlg)
	DDX_Control(pDX, IDC_LIST_USERS, m_wndListCtrl);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDV_MaxChars(pDX, m_strName, 255);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDefineGraphDlg, CDialog)
	//{{AFX_MSG_MAP(CDefineGraphDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDefineGraphDlg message handlers

//
// WM_INITDIALOG message handler
//

BOOL CDefineGraphDlg::OnInitDialog() 
{
	RECT rect;
	DWORD i;
	NXC_USER *pUser;
	int nItem;
	TCHAR szBuffer[16];

	CDialog::OnInitDialog();

	// Setup list control
	m_wndListCtrl.GetClientRect(&rect);
	m_wndListCtrl.InsertColumn(0, _T("User"), LVCFMT_LEFT, rect.right - 60 - GetSystemMetrics(SM_CXVSCROLL));
	m_wndListCtrl.InsertColumn(1, _T("Access"), LVCFMT_LEFT, 60);
	m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);

   // Create image list
   m_imageList.Create(16, 16, ILC_COLOR8 | ILC_MASK, 4, 1);
   m_imageList.Add(theApp.LoadIcon(IDI_USER));
   m_imageList.Add(theApp.LoadIcon(IDI_USER_GROUP));
   m_imageList.Add(theApp.LoadIcon(IDI_EVERYONE));
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);

	// Fill ACL
	for(i = 0; i < m_dwACLSize; i++)
	{
		pUser = NXCFindUserById(g_hSession, m_pACL[i].dwUserId);
		if (pUser != NULL)
		{
			nItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, pUser->szName,
			                                 (m_pACL[i].dwUserId == GROUP_EVERYONE) ? 2 :
					                              ((m_pACL[i].dwUserId & GROUP_FLAG) ? 1 : 0));
			if (nItem != -1)
			{
				m_wndListCtrl.SetItemData(nItem, m_pACL[i].dwUserId);
				szBuffer[0] = (m_pACL[i].dwAccess & NXGRAPH_ACCESS_READ) ? _T('R') : _T('-');
				szBuffer[1] = (m_pACL[i].dwAccess & NXGRAPH_ACCESS_WRITE) ? _T('M') : _T('-');
				szBuffer[2] = 0;
				m_wndListCtrl.SetItemText(nItem, 1, szBuffer);
			}
		}
	}

	EnableDlgItem(this, IDC_CHECK_READ, FALSE);
	EnableDlgItem(this, IDC_CHECK_MODIFY, FALSE);
	EnableDlgItem(this, IDC_BUTTON_DELETE, FALSE);
	
	return TRUE;
}
