// ProcessingPage.cpp : implementation file
//

#include "stdafx.h"
#include "nxconfig.h"
#include "ProcessingPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProcessingPage property page

IMPLEMENT_DYNCREATE(CProcessingPage, CPropertyPage)

CProcessingPage::CProcessingPage() : CPropertyPage(CProcessingPage::IDD)
{
	//{{AFX_DATA_INIT(CProcessingPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   
   m_iLastItem = -1;
}

CProcessingPage::~CProcessingPage()
{
}

void CProcessingPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProcessingPage)
	DDX_Control(pDX, IDC_LIST_STATUS, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CProcessingPage, CPropertyPage)
	//{{AFX_MSG_MAP(CProcessingPage)
	//}}AFX_MSG_MAP
   ON_MESSAGE(WM_START_STAGE, OnStartStage)
   ON_MESSAGE(WM_STAGE_COMPLETED, OnStageCompleted)
   ON_MESSAGE(WM_JOB_FINISHED, OnJobFinished)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProcessingPage message handlers


//
// WM_INITDIALOG message handler
//

BOOL CProcessingPage::OnInitDialog() 
{
   RECT rect;

	CPropertyPage::OnInitDialog();
	
   // Setup list control
   m_wndListCtrl.GetClientRect(&rect);
   m_wndListCtrl.InsertColumn(0, _T(""), LVCFMT_LEFT, 20);
   m_wndListCtrl.InsertColumn(1, _T("Message"), LVCFMT_LEFT,
                              rect.right - 20 - GetSystemMetrics(SM_CXVSCROLL));

   // Create image list
   m_imageList.Create(16, 16, ILC_MASK | ILC_COLOR24, 4, 1);
   m_imageList.Add(appNxConfig.LoadIcon(IDI_RUNNING));
   m_imageList.Add(appNxConfig.LoadIcon(IDI_OK));
   m_imageList.Add(appNxConfig.LoadIcon(IDI_FAILED));
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);

	return TRUE;
}


//
// WM_START_STAGE message handler
//

LRESULT CProcessingPage::OnStartStage(WPARAM wParam, LPARAM lParam)
{
   m_iLastItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, _T(""), 0);
   if (m_iLastItem != -1)
      m_wndListCtrl.SetItemText(m_iLastItem, 1, (TCHAR *)lParam);
	return 0;
}


//
// WM_STAGE_COMPLETED message handler
//

LRESULT CProcessingPage::OnStageCompleted(WPARAM wParam, LPARAM lParam)
{
   LVITEM item;

   item.iItem = m_iLastItem;
   item.iSubItem = 0;
   item.mask = LVIF_IMAGE;
   item.iImage = wParam ? 1 : 2;
   m_wndListCtrl.SetItem(&item);
	return 0;
}


//
// WM_JOB_FINISHED message handler
//

LRESULT CProcessingPage::OnJobFinished(WPARAM wParam, LPARAM lParam)
{
   SetDlgItemText(IDC_STATIC_STATUS, wParam ? _T("Completed") : _T("Failed"));
   if (wParam)
      MessageBox(_T("Server configuration was completed successfully"), 
                 _T("Information"), MB_OK | MB_ICONINFORMATION);
   else
      MessageBox(g_szWizardErrorText, _T("Error"), MB_OK | MB_ICONSTOP);
   ((CPropertySheet *)GetParent())->SetWizardButtons(wParam ? PSWIZB_NEXT : PSWIZB_BACK);
	return 0;
}


//
// Page activation handler
//

BOOL CProcessingPage::OnSetActive() 
{
   ((CPropertySheet *)GetParent())->SetWizardButtons(0);
   m_wndListCtrl.DeleteAllItems();
   StartWizardWorkerThread(this->m_hWnd, &((CConfigWizard *)GetParent())->m_cfg);
	return CPropertyPage::OnSetActive();
}
