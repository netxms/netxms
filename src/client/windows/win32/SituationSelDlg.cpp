// SituationSelDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "SituationSelDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSituationSelDlg dialog


CSituationSelDlg::CSituationSelDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSituationSelDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSituationSelDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CSituationSelDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSituationSelDlg)
	DDX_Control(pDX, IDC_LIST_SITUATIONS, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSituationSelDlg, CDialog)
	//{{AFX_MSG_MAP(CSituationSelDlg)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_SITUATIONS, OnDblclkListSituations)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSituationSelDlg message handlers

//
// WM_INITDIALOG message handler
//

BOOL CSituationSelDlg::OnInitDialog() 
{
   RECT rect;
	NXC_SITUATION_LIST *list;
   int i, item;

	CDialog::OnInitDialog();

   // Setup list view
   m_wndListCtrl.GetClientRect(&rect);
   m_wndListCtrl.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 
                              rect.right - GetSystemMetrics(SM_CXVSCROLL) - 4);
	m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);

   // Create image list
   m_imageList.Create(16, 16, ILC_COLOR8 | ILC_MASK, 8, 8);
   m_imageList.Add(theApp.LoadIcon(IDI_SITUATION));
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);

   // Fill in list view
	list = theApp.GetSituationList();
   if (list != NULL)
   {
      for(i = 0; i < list->m_count; i++)
      {
         item = m_wndListCtrl.InsertItem(0x7FFFFFFF, list->m_situations[i].m_name, 0);
         if (item != -1)
            m_wndListCtrl.SetItemData(item, list->m_situations[i].m_id);
      }
		theApp.UnlockSituationList();
   }

	return TRUE;
}


//
// "OK" handler
//

void CSituationSelDlg::OnOK() 
{
   if (m_wndListCtrl.GetSelectedCount() > 0)
   {
      m_dwSituation = m_wndListCtrl.GetItemData(m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED));
	   CDialog::OnOK();
   }
   else
   {
      MessageBox(_T("You should select situation first"),
                 _T("Situation Selection"), MB_OK | MB_ICONSTOP);
   }
}


//
// Handler for doble click in situation list
//

void CSituationSelDlg::OnDblclkListSituations(NMHDR* pNMHDR, LRESULT* pResult) 
{
   PostMessage(WM_COMMAND, IDOK, 0);
	*pResult = 0;
}
