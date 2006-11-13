// DiscoveryPropTargets.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "DiscoveryPropTargets.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDiscoveryPropTargets property page

IMPLEMENT_DYNCREATE(CDiscoveryPropTargets, CPropertyPage)

CDiscoveryPropTargets::CDiscoveryPropTargets() : CPropertyPage(CDiscoveryPropTargets::IDD)
{
	//{{AFX_DATA_INIT(CDiscoveryPropTargets)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CDiscoveryPropTargets::~CDiscoveryPropTargets()
{
}

void CDiscoveryPropTargets::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDiscoveryPropTargets)
	DDX_Control(pDX, IDC_LIST_TARGETS, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDiscoveryPropTargets, CPropertyPage)
	//{{AFX_MSG_MAP(CDiscoveryPropTargets)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDiscoveryPropTargets message handlers
