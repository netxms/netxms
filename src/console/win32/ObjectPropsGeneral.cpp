// ObjectPropsGeneral.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectPropsGeneral.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjectPropsGeneral property page

IMPLEMENT_DYNCREATE(CObjectPropsGeneral, CPropertyPage)

CObjectPropsGeneral::CObjectPropsGeneral() : CPropertyPage(CObjectPropsGeneral::IDD)
{
	//{{AFX_DATA_INIT(CObjectPropsGeneral)
	m_dwObjectId = 0;
	m_strName = _T("");
	m_strClass = _T("");
	//}}AFX_DATA_INIT
}

CObjectPropsGeneral::~CObjectPropsGeneral()
{
}

void CObjectPropsGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CObjectPropsGeneral)
	DDX_Text(pDX, IDC_EDIT_ID, m_dwObjectId);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDV_MaxChars(pDX, m_strName, 63);
	DDX_Text(pDX, IDC_EDIT_CLASS, m_strClass);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CObjectPropsGeneral, CPropertyPage)
	//{{AFX_MSG_MAP(CObjectPropsGeneral)
	ON_EN_CHANGE(IDC_EDIT_NAME, OnChangeEditName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjectPropsGeneral message handlers


//
// WM_INITDIALOG message handler
//

BOOL CObjectPropsGeneral::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
   m_pUpdate = ((CObjectPropSheet *)GetParent())->GetUpdateStruct();
	
	return TRUE;
}

void CObjectPropsGeneral::OnChangeEditName() 
{
   m_pUpdate->dwFlags |= OBJ_UPDATE_NAME;
   SetModified();
}


//
// PSN_OK handler
//

void CObjectPropsGeneral::OnOK() 
{
	CPropertyPage::OnOK();
   m_pUpdate->pszName = (char *)((LPCTSTR)m_strName);
}
