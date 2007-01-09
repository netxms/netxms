// ClusterPropsResources.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ClusterPropsResources.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CClusterPropsResources property page

IMPLEMENT_DYNCREATE(CClusterPropsResources, CPropertyPage)

CClusterPropsResources::CClusterPropsResources() : CPropertyPage(CClusterPropsResources::IDD)
{
	//{{AFX_DATA_INIT(CClusterPropsResources)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CClusterPropsResources::~CClusterPropsResources()
{
}

void CClusterPropsResources::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CClusterPropsResources)
	DDX_Control(pDX, IDC_LIST_RESOURCES, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CClusterPropsResources, CPropertyPage)
	//{{AFX_MSG_MAP(CClusterPropsResources)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CClusterPropsResources message handlers
