// ObjectPropsPresentation.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectPropsPresentation.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjectPropsPresentation property page

IMPLEMENT_DYNCREATE(CObjectPropsPresentation, CPropertyPage)

CObjectPropsPresentation::CObjectPropsPresentation() : CPropertyPage(CObjectPropsPresentation::IDD)
{
	//{{AFX_DATA_INIT(CObjectPropsPresentation)
	m_bUseDefaultImage = FALSE;
	//}}AFX_DATA_INIT
}

CObjectPropsPresentation::~CObjectPropsPresentation()
{
}

void CObjectPropsPresentation::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CObjectPropsPresentation)
	DDX_Control(pDX, IDC_STATIC_ICON, m_wndIcon);
	DDX_Control(pDX, ID_SELECT_IMAGE, m_wndSelButton);
	DDX_Check(pDX, IDC_CHECK_DEFAULT_IMAGE, m_bUseDefaultImage);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CObjectPropsPresentation, CPropertyPage)
	//{{AFX_MSG_MAP(CObjectPropsPresentation)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjectPropsPresentation message handlers


//
// WM_INITDIALOG message handler
//

BOOL CObjectPropsPresentation::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
   m_pUpdate = ((CObjectPropSheet *)GetParent())->GetUpdateStruct();
	
	return TRUE;
}
