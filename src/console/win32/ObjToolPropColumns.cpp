// ObjToolPropColumns.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjToolPropColumns.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjToolPropColumns property page

IMPLEMENT_DYNCREATE(CObjToolPropColumns, CPropertyPage)

CObjToolPropColumns::CObjToolPropColumns() : CPropertyPage(CObjToolPropColumns::IDD)
{
	//{{AFX_DATA_INIT(CObjToolPropColumns)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CObjToolPropColumns::~CObjToolPropColumns()
{
}

void CObjToolPropColumns::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CObjToolPropColumns)
	DDX_Control(pDX, IDC_LIST_COLUMNS, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CObjToolPropColumns, CPropertyPage)
	//{{AFX_MSG_MAP(CObjToolPropColumns)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjToolPropColumns message handlers
