// ObjToolPropGeneral.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjToolPropGeneral.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjToolPropGeneral property page

IMPLEMENT_DYNCREATE(CObjToolPropGeneral, CPropertyPage)

CObjToolPropGeneral::CObjToolPropGeneral() : CPropertyPage(CObjToolPropGeneral::IDD)
{
	//{{AFX_DATA_INIT(CObjToolPropGeneral)
	m_strDescription = _T("");
	m_strName = _T("");
	m_strRegEx = _T("");
	m_strEnum = _T("");
	m_strData = _T("");
	//}}AFX_DATA_INIT
}

CObjToolPropGeneral::~CObjToolPropGeneral()
{
}

void CObjToolPropGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CObjToolPropGeneral)
	DDX_Control(pDX, IDC_LIST_USERS, m_wndListCtrl);
	DDX_Text(pDX, IDC_EDIT_DESCRIPTION, m_strDescription);
	DDV_MaxChars(pDX, m_strDescription, 255);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDV_MaxChars(pDX, m_strName, 255);
	DDX_Text(pDX, IDC_EDIT_REGEX, m_strRegEx);
	DDV_MaxChars(pDX, m_strRegEx, 255);
	DDX_Text(pDX, IDC_EDIT_ENUM, m_strEnum);
	DDV_MaxChars(pDX, m_strEnum, 255);
	DDX_Text(pDX, IDC_EDIT_DATA, m_strData);
	DDV_MaxChars(pDX, m_strData, 255);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CObjToolPropGeneral, CPropertyPage)
	//{{AFX_MSG_MAP(CObjToolPropGeneral)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjToolPropGeneral message handlers
