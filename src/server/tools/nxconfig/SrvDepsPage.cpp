// SrvDepsPage.cpp : implementation file
//

#include "stdafx.h"
#include "nxconfig.h"
#include "SrvDepsPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSrvDepsPage property page

IMPLEMENT_DYNCREATE(CSrvDepsPage, CPropertyPage)

CSrvDepsPage::CSrvDepsPage() : CPropertyPage(CSrvDepsPage::IDD)
{
	//{{AFX_DATA_INIT(CSrvDepsPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CSrvDepsPage::~CSrvDepsPage()
{
}

void CSrvDepsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSrvDepsPage)
	DDX_Control(pDX, IDC_LIST_SERVICES, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSrvDepsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSrvDepsPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSrvDepsPage message handlers


//
// WM_INITDIALOG message handler
//

BOOL CSrvDepsPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
   m_wndListCtrl.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
	
	return TRUE;
}
