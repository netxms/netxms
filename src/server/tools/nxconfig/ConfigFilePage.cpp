// ConfigFilePage.cpp : implementation file
//

#include "stdafx.h"
#include "nxconfig.h"
#include "ConfigFilePage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConfigFilePage property page

IMPLEMENT_DYNCREATE(CConfigFilePage, CPropertyPage)

CConfigFilePage::CConfigFilePage() : CPropertyPage(CConfigFilePage::IDD)
{
	//{{AFX_DATA_INIT(CConfigFilePage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CConfigFilePage::~CConfigFilePage()
{
}

void CConfigFilePage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConfigFilePage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConfigFilePage, CPropertyPage)
	//{{AFX_MSG_MAP(CConfigFilePage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigFilePage message handlers
