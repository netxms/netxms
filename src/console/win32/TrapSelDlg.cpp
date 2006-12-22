// TrapSelDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "TrapSelDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTrapSelDlg dialog


CTrapSelDlg::CTrapSelDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTrapSelDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTrapSelDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_pdwTrapList = NULL;
	m_dwNumTraps = 0;
	m_dwTrapCfgSize = 0;

   m_iSortMode = theApp.GetProfileInt(_T("TrapSelDlg"), _T("SortMode"), 0);
   m_iSortDir = theApp.GetProfileInt(_T("TrapSelDlg"), _T("SortDir"), 1);
}

CTrapSelDlg::~CTrapSelDlg()
{
	safe_free(m_pdwTrapList);
   theApp.WriteProfileInt(_T("TrapSelDlg"), _T("SortMode"), m_iSortMode);
   theApp.WriteProfileInt(_T("TrapSelDlg"), _T("SortDir"), m_iSortDir);
}

void CTrapSelDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTrapSelDlg)
	DDX_Control(pDX, IDC_LIST_TRAPS, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTrapSelDlg, CDialog)
	//{{AFX_MSG_MAP(CTrapSelDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTrapSelDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CTrapSelDlg::OnInitDialog() 
{
	DWORD i;
	int nItem;
	TCHAR szBuffer[1024];
	RECT rect;

	CDialog::OnInitDialog();

	m_pImageList = CreateEventImageList();
   m_iLastEventImage = m_pImageList->GetImageCount();
   m_pImageList->Add(theApp.LoadIcon(IDI_SORT_UP));
   m_pImageList->Add(theApp.LoadIcon(IDI_SORT_DOWN));
	m_wndListCtrl.SetImageList(m_pImageList, LVSIL_SMALL);
	m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_INFOTIP);
	
	m_wndListCtrl.GetClientRect(&rect);
	m_wndListCtrl.InsertColumn(0, _T("Trap OID"), LVCFMT_LEFT, 150);
	m_wndListCtrl.InsertColumn(1, _T("Event"), LVCFMT_LEFT, 150);
	m_wndListCtrl.InsertColumn(2, _T("Description"), LVCFMT_LEFT, rect.right - 300 - GetSystemMetrics(SM_CXVSCROLL));

	for(i = 0; i < m_dwTrapCfgSize; i++)
	{
		SNMPConvertOIDToText(m_pTrapCfg[i].dwOidLen, m_pTrapCfg[i].pdwObjectId, szBuffer, 1024);
		nItem = m_wndListCtrl.InsertItem(i, szBuffer, NXCGetEventSeverity(g_hSession, m_pTrapCfg[i].dwEventCode));
		if (nItem != -1)
		{
			m_wndListCtrl.SetItemText(nItem, 1, NXCGetEventName(g_hSession, m_pTrapCfg[i].dwEventCode));
			m_wndListCtrl.SetItemText(nItem, 2, m_pTrapCfg[i].szDescription);
			m_wndListCtrl.SetItemData(nItem, i);
		}
	}
	SortList();
	
	return TRUE;
}


//
// "OK" button handler
//

void CTrapSelDlg::OnOK() 
{
	int nItem;
	DWORD i;
	
	m_dwNumTraps = m_wndListCtrl.GetSelectedCount();
	if (m_dwNumTraps > 0)
	{
		m_pdwTrapList = (DWORD *)malloc(sizeof(DWORD) * m_dwNumTraps);
		for(i = 0, nItem = -1; i < m_dwNumTraps; i++)
		{
			nItem = m_wndListCtrl.GetNextItem(nItem, LVIS_SELECTED);
			if (nItem != -1)
			{
				m_pdwTrapList[i] = m_pTrapCfg[m_wndListCtrl.GetItemData(nItem)].dwId;
			}
		}
	}
	
	CDialog::OnOK();
}


//
// Callback for sort function
//

static int CALLBACK CompareListItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	return ((CTrapSelDlg *)lParamSort)->CompareItems(lParam1, lParam2);
}


//
// Sort trap list
//

void CTrapSelDlg::SortList()
{
   LVCOLUMN lvCol;

   m_wndListCtrl.SortItems(CompareListItems, ((m_iSortDir == 1) ? 0xABCD0000 : 0xDCBA0000) | (WORD)m_iSortMode);

   // Mark sorting column
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = (m_iSortDir == 1) ? m_iLastEventImage : (m_iLastEventImage + 1);
   m_wndListCtrl.SetColumn(m_iSortMode, &lvCol);
}


//
// Compare two list items
//

int CTrapSelDlg::CompareItems(LPARAM nItem1, LPARAM nItem2)
{

}
