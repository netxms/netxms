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
	safe_free(m_pdwEventList);
	safe_free(m_ppszNames);

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
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_TRAPS, OnColumnclickListTraps)
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
	DWORD i, dwIndex;
	
	m_dwNumTraps = m_wndListCtrl.GetSelectedCount();
	if (m_dwNumTraps > 0)
	{
		m_pdwTrapList = (DWORD *)malloc(sizeof(DWORD) * m_dwNumTraps);
		m_pdwEventList = (DWORD *)malloc(sizeof(DWORD) * m_dwNumTraps);
		m_ppszNames = (TCHAR **)malloc(sizeof(TCHAR *) * m_dwNumTraps);
		for(i = 0, nItem = -1; i < m_dwNumTraps; i++)
		{
			nItem = m_wndListCtrl.GetNextItem(nItem, LVIS_SELECTED);
			if (nItem != -1)
			{
				dwIndex = (DWORD)m_wndListCtrl.GetItemData(nItem);
				m_pdwTrapList[i] = m_pTrapCfg[dwIndex].dwId;
				m_pdwEventList[i] = m_pTrapCfg[dwIndex].dwEventCode;
				m_ppszNames[i] = m_pTrapCfg[dwIndex].szDescription;
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

   m_wndListCtrl.SortItems(CompareListItems, (LPARAM)this);

   // Mark sorting column
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = (m_iSortDir == 1) ? m_iLastEventImage : (m_iLastEventImage + 1);
   m_wndListCtrl.SetColumn(m_iSortMode, &lvCol);
}


//
// Compare two OIDs
//

static int CompareOIDs(UINT32 dwLen1, UINT32 *pdwOid1, UINT32 dwLen2, UINT32 *pdwOid2)
{
	DWORD i, dwLen;

	dwLen = min(dwLen1, dwLen2);
	for(i = 0; i < dwLen; i++)
	{
		if (pdwOid1[i] < pdwOid2[i])
			return -1;
		if (pdwOid1[i] > pdwOid2[i])
			return 1;
	}
	return COMPARE_NUMBERS(dwLen1, dwLen2);
}


//
// Compare two list items
//

int CTrapSelDlg::CompareItems(LPARAM nItem1, LPARAM nItem2)
{
	int nRet;

	switch(m_iSortMode)
	{
		case 0:	// OID
			nRet = CompareOIDs(m_pTrapCfg[nItem1].dwOidLen, m_pTrapCfg[nItem1].pdwObjectId,
			                   m_pTrapCfg[nItem2].dwOidLen, m_pTrapCfg[nItem2].pdwObjectId);
			break;
		case 1:	// Event
			nRet = _tcsicmp(NXCGetEventName(g_hSession, m_pTrapCfg[nItem1].dwEventCode),
			                NXCGetEventName(g_hSession, m_pTrapCfg[nItem2].dwEventCode));
			break;
		case 2:	// Description
			nRet = _tcsicmp(m_pTrapCfg[nItem1].szDescription, m_pTrapCfg[nItem2].szDescription);
			break;
	}
	return nRet * m_iSortDir;
}


//
// Handler for list control column click
//

void CTrapSelDlg::OnColumnclickListTraps(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW *pNMListView = (NM_LISTVIEW *)pNMHDR;
   LVCOLUMN lvCol;

   // Unmark current sorting column
   lvCol.mask = LVCF_FMT;
   lvCol.fmt = LVCFMT_LEFT;
   m_wndListCtrl.SetColumn(m_iSortMode, &lvCol);

   if (pNMListView->iSubItem == m_iSortMode)
   {
      // Same column, change sort direction
      m_iSortDir = -m_iSortDir;
   }
   else
   {
      // Another sorting column
      m_iSortMode = pNMListView->iSubItem;
   }

   SortList();
	*pResult = 0;
}
