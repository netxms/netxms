// OPGeneral.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "OPGeneral.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COPGeneral property page

IMPLEMENT_DYNCREATE(COPGeneral, CPropertyPage)

COPGeneral::COPGeneral() : CPropertyPage(COPGeneral::IDD)
{
	//{{AFX_DATA_INIT(COPGeneral)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

COPGeneral::~COPGeneral()
{
}

void COPGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COPGeneral)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COPGeneral, CPropertyPage)
	//{{AFX_MSG_MAP(COPGeneral)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COPGeneral message handlers
