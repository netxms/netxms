// EventSelDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "EventSelDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEventSelDlg dialog


CEventSelDlg::CEventSelDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEventSelDlg::IDD, pParent)
{
   m_bSingleSelection = FALSE;
   m_pImageList = NULL;
   m_pdwEventList = NULL;
   m_iSortMode = theApp.GetProfileInt(_T("EventSelDlg"), _T("SortMode"), 0);
   m_iSortDir = theApp.GetProfileInt(_T("EventSelDlg"), _T("SortDir"), 1);

	//{{AFX_DATA_INIT(CEventSelDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CEventSelDlg::~CEventSelDlg()
{
   delete m_pImageList;
   safe_free(m_pdwEventList);
   theApp.WriteProfileInt(_T("EventSelDlg"), _T("SortMode"), m_iSortMode);
   theApp.WriteProfileInt(_T("EventSelDlg"), _T("SortDir"), m_iSortDir);
}

void CEventSelDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEventSelDlg)
	DDX_Control(pDX, IDC_LIST_EVENTS, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEventSelDlg, CDialog)
	//{{AFX_MSG_MAP(CEventSelDlg)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_EVENTS, OnDblclkListEvents)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_EVENTS, OnColumnclickListEvents)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEventSelDlg message handlers

//
// WM_INITDIALOG message handler
//

BOOL CEventSelDlg::OnInitDialog() 
{
   NXC_EVENT_TEMPLATE **pList;
   UINT32 i, dwListSize;
   TCHAR szBuffer[32];
   RECT rect;
   int iItem;

	CDialog::OnInitDialog();
	
   // Create image list
   m_pImageList = CreateEventImageList();
   m_iLastEventImage = m_pImageList->GetImageCount();
   m_pImageList->Add(theApp.LoadIcon(IDI_SORT_UP));
   m_pImageList->Add(theApp.LoadIcon(IDI_SORT_DOWN));
   m_wndListCtrl.SetImageList(m_pImageList, LVSIL_SMALL);

   // Setup list control
   m_wndListCtrl.GetClientRect(&rect);
   m_wndListCtrl.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 
                              rect.right - 70 - GetSystemMetrics(SM_CXVSCROLL));
   m_wndListCtrl.InsertColumn(1, _T("ID"), LVCFMT_LEFT, 70);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
   if (m_bSingleSelection)
   {
      ::SetWindowLong(m_wndListCtrl.m_hWnd, GWL_STYLE, 
         ::GetWindowLong(m_wndListCtrl.m_hWnd, GWL_STYLE) | LVS_SINGLESEL);
   }
	
   // Fill in event list
   NXCGetEventDB(g_hSession, &pList, &dwListSize);
   if (pList != NULL)
   {
      for(i = 0; i < dwListSize; i++)
      {
         _sntprintf_s(szBuffer, 32, _TRUNCATE, _T("%u"), pList[i]->dwCode);
         iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, pList[i]->szName, pList[i]->dwSeverity);
         m_wndListCtrl.SetItemText(iItem, 1, szBuffer);
         m_wndListCtrl.SetItemData(iItem, pList[i]->dwCode);
      }
      SortList();
   }

	return TRUE;
}


//
// "OK" button handler
//

void CEventSelDlg::OnOK() 
{
   int iItem;
   DWORD i;

   m_dwNumEvents = m_wndListCtrl.GetSelectedCount();
   if (m_dwNumEvents > 0)
   {
      // Build list of selected objects
      m_pdwEventList = (DWORD *)malloc(sizeof(DWORD) * m_dwNumEvents);
      iItem = m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED);
      for(i = 0; iItem != -1; i++)
      {
         m_pdwEventList[i] = m_wndListCtrl.GetItemData(iItem);
         iItem = m_wndListCtrl.GetNextItem(iItem, LVIS_SELECTED);
      }
	   CDialog::OnOK();
   }
   else
   {
      MessageBox(_T("You should select at least one event"), _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
   }
}


//
// Handler for list control double click
//

void CEventSelDlg::OnDblclkListEvents(NMHDR* pNMHDR, LRESULT* pResult) 
{
   if (m_wndListCtrl.GetSelectedCount() > 0)
      PostMessage(WM_COMMAND, IDOK, 0);
	*pResult = 0;
}


//
// Compare list control items
//

static int CALLBACK CompareListItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
   int iSortDir = (((DWORD)lParamSort & 0xFFFF0000) == 0xABCD0000) ? 1 : -1;

   if ((lParamSort & 0xFFFF) == 1)
      return (lParam1 < lParam2) ? -iSortDir : ((lParam1 == lParam2) ? 0 : iSortDir);
   return _tcsicmp(NXCGetEventName(g_hSession, lParam1), 
                   NXCGetEventName(g_hSession, lParam2)) * iSortDir;
}


//
// Sort event list
//

void CEventSelDlg::SortList()
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
// Handler for list control column click
//

void CEventSelDlg::OnColumnclickListEvents(NMHDR* pNMHDR, LRESULT* pResult) 
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
