// DiscoveryPropAddrList.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "DiscoveryPropAddrList.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDiscoveryPropAddrList property page

IMPLEMENT_DYNCREATE(CDiscoveryPropAddrList, CPropertyPage)

CDiscoveryPropAddrList::CDiscoveryPropAddrList() : CPropertyPage(CDiscoveryPropAddrList::IDD)
{
	//{{AFX_DATA_INIT(CDiscoveryPropAddrList)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CDiscoveryPropAddrList::~CDiscoveryPropAddrList()
{
}

void CDiscoveryPropAddrList::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDiscoveryPropAddrList)
	DDX_Control(pDX, IDC_LIST_SUBNETS, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDiscoveryPropAddrList, CPropertyPage)
	//{{AFX_MSG_MAP(CDiscoveryPropAddrList)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDiscoveryPropAddrList message handlers
