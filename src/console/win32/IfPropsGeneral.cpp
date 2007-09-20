// IfPropsGeneral.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "IfPropsGeneral.h"
#include "ObjectPropSheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CIfPropsGeneral property page

IMPLEMENT_DYNCREATE(CIfPropsGeneral, CPropertyPage)

CIfPropsGeneral::CIfPropsGeneral() : CPropertyPage(CIfPropsGeneral::IDD)
{
	//{{AFX_DATA_INIT(CIfPropsGeneral)
	m_strName = _T("");
	m_nRequiredPolls = 0;
	m_dwObjectId = 0;
	//}}AFX_DATA_INIT
}

CIfPropsGeneral::~CIfPropsGeneral()
{
}

void CIfPropsGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CIfPropsGeneral)
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDX_Text(pDX, IDC_EDIT_POLLS, m_nRequiredPolls);
	DDV_MinMaxInt(pDX, m_nRequiredPolls, 0, 100);
	DDX_Text(pDX, IDC_EDIT_ID, m_dwObjectId);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CIfPropsGeneral, CPropertyPage)
	//{{AFX_MSG_MAP(CIfPropsGeneral)
	ON_EN_CHANGE(IDC_EDIT_NAME, OnChangeEditName)
	ON_EN_CHANGE(IDC_EDIT_POLLS, OnChangeEditPolls)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIfPropsGeneral message handlers

//
// WM_INITDIALOG message handler
//

BOOL CIfPropsGeneral::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
   m_pUpdate = ((CObjectPropSheet *)GetParent())->GetUpdateStruct();
	
	return TRUE;
}


//
// Handler for OK button
//

void CIfPropsGeneral::OnOK() 
{
	CPropertyPage::OnOK();

   // Set fields in update structure
   m_pUpdate->pszName = (TCHAR *)((LPCTSTR)m_strName);
	m_pUpdate->wRequiredPollCount = m_nRequiredPolls;
}


//
// Handlers for changed controls
//

void CIfPropsGeneral::OnChangeEditName() 
{
   m_pUpdate->dwFlags |= OBJ_UPDATE_NAME;
   SetModified();
}

void CIfPropsGeneral::OnChangeEditPolls() 
{
   m_pUpdate->dwFlags |= OBJ_UPDATE_REQUIRED_POLLS;
   SetModified();
}
