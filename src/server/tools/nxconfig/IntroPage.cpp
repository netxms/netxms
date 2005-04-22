// IntroPage.cpp : implementation file
//

#include "stdafx.h"
#include "nxconfig.h"
#include "IntroPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CIntroPage property page

IMPLEMENT_DYNCREATE(CIntroPage, CPropertyPage)

CIntroPage::CIntroPage() : CPropertyPage(CIntroPage::IDD)
{
	//{{AFX_DATA_INIT(CIntroPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CIntroPage::~CIntroPage()
{
}

void CIntroPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CIntroPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CIntroPage, CPropertyPage)
	//{{AFX_MSG_MAP(CIntroPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIntroPage message handlers

LRESULT CIntroPage::OnWizardNext() 
{
	return CPropertyPage::OnWizardNext();
}


//
// Page activation handler
//

BOOL CIntroPage::OnSetActive() 
{
   ((CPropertySheet *)GetParent())->SetWizardButtons(PSWIZB_NEXT);	
	return CPropertyPage::OnSetActive();
}
