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
	//{{AFX_DATA_INIT(CEventSelDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
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
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEventSelDlg message handlers

//
// WM_INITDIALOG message handler
//

BOOL CEventSelDlg::OnInitDialog() 
{
   NXC_EVENT_NAME *pList;
   DWORD i, dwListSize;
   char szBuffer[32];
   RECT rect;
   int iItem;

	CDialog::OnInitDialog();
	
   // Setup list control
   //m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);
   m_wndListCtrl.GetClientRect(&rect);
   m_wndListCtrl.InsertColumn(0, "ID", LVCFMT_LEFT, 70);
   m_wndListCtrl.InsertColumn(1, "Name", LVCFMT_LEFT, 
                              rect.right - 70 - GetSystemMetrics(SM_CXVSCROLL));
   m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	
   // Fill in event list
   pList = NXCGetEventNamesList(&dwListSize);
   if (pList != NULL)
      for(i = 0; i < dwListSize; i++)
      {
         sprintf(szBuffer, "%u", pList[i].dwEventId);
         iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, szBuffer);
         m_wndListCtrl.SetItemText(iItem, 1, pList[i].szName);
         m_wndListCtrl.SetItemData(iItem, pList[i].dwEventId);
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
      MessageBox("You should select at least one object", "Warning", MB_OK | MB_ICONEXCLAMATION);
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
