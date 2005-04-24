// LoggingPage.cpp : implementation file
//

#include "stdafx.h"
#include "nxconfig.h"
#include "LoggingPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLoggingPage property page

IMPLEMENT_DYNCREATE(CLoggingPage, CPropertyPage)

CLoggingPage::CLoggingPage() : CPropertyPage(CLoggingPage::IDD)
{
	//{{AFX_DATA_INIT(CLoggingPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CLoggingPage::~CLoggingPage()
{
}

void CLoggingPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLoggingPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLoggingPage, CPropertyPage)
	//{{AFX_MSG_MAP(CLoggingPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLoggingPage message handlers
