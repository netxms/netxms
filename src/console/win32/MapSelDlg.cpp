// MapSelDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "MapSelDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMapSelDlg dialog


CMapSelDlg::CMapSelDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMapSelDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMapSelDlg)
	//}}AFX_DATA_INIT
}


void CMapSelDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMapSelDlg)
	DDX_Control(pDX, IDC_LIST_MAPS, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMapSelDlg, CDialog)
	//{{AFX_MSG_MAP(CMapSelDlg)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_MAPS, OnDblclkListMaps)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMapSelDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CMapSelDlg::OnInitDialog() 
{
	NXC_MAP_INFO *pMapList;
	DWORD i, dwResult, dwNumMaps;
	int item;
	RECT rect;

	CDialog::OnInitDialog();

	m_wndListCtrl.GetClientRect(&rect);
	m_wndListCtrl.InsertColumn(0, _T("Name"), LVCFMT_LEFT, rect.right - GetSystemMetrics(SM_CXVSCROLL));
	m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);

	dwResult = DoRequestArg3(NXCGetMapList, g_hSession, &dwNumMaps, &pMapList, _T("Retrieving map list..."));
	if (dwResult == RCC_SUCCESS)
	{
		for(i = 0; i < dwNumMaps; i++)
		{
			item = m_wndListCtrl.InsertItem(0x7FFFFFFF, pMapList[i].szName);
			m_wndListCtrl.SetItemData(item, pMapList[i].dwMapId);
		}
		free(pMapList);
	}
	else
	{
		theApp.ErrorBox(dwResult, _T("Cannot retrieve map list from server: %s"));
	}

	return TRUE;
}


//
// OK button handler
//

void CMapSelDlg::OnOK() 
{
	if (m_wndListCtrl.GetSelectedCount() == 0)
	{
		MessageBox(_T("You should select map from the list!"), _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	
	m_dwMapId = m_wndListCtrl.GetItemData(m_wndListCtrl.GetSelectionMark());
	CDialog::OnOK();
}


//
// Double click in list
//

void CMapSelDlg::OnDblclkListMaps(NMHDR* pNMHDR, LRESULT* pResult) 
{
	if (m_wndListCtrl.GetSelectedCount() != 0)
		PostMessage(WM_COMMAND, IDOK, 0);
	*pResult = 0;
}
