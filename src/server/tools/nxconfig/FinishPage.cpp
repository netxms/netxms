// FinishPage.cpp : implementation file
//

#include "stdafx.h"
#include "nxconfig.h"
#include "FinishPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFinishPage property page

IMPLEMENT_DYNCREATE(CFinishPage, CPropertyPage)

CFinishPage::CFinishPage() : CPropertyPage(CFinishPage::IDD)
{
	//{{AFX_DATA_INIT(CFinishPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CFinishPage::~CFinishPage()
{
}

void CFinishPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFinishPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFinishPage, CPropertyPage)
	//{{AFX_MSG_MAP(CFinishPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFinishPage message handlers


//
// Handler for page activation
//

BOOL CFinishPage::OnSetActive() 
{
   ((CPropertySheet *)GetParent())->SetWizardButtons(PSWIZB_FINISH);
	return CPropertyPage::OnSetActive();
}
