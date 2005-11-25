// ObjToolPropOptions.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjToolPropOptions.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjToolPropOptions property page

IMPLEMENT_DYNCREATE(CObjToolPropOptions, CPropertyPage)

CObjToolPropOptions::CObjToolPropOptions() : CPropertyPage(CObjToolPropOptions::IDD)
{
	//{{AFX_DATA_INIT(CObjToolPropOptions)
	m_strMatchingOID = _T("");
	m_bNeedAgent = FALSE;
	m_bMatchOID = FALSE;
	m_bNeedSNMP = FALSE;
	m_nIndexType = -1;
	//}}AFX_DATA_INIT
}

CObjToolPropOptions::~CObjToolPropOptions()
{
}

void CObjToolPropOptions::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CObjToolPropOptions)
	DDX_Text(pDX, IDC_EDIT_TEMPLATE, m_strMatchingOID);
	DDX_Check(pDX, IDC_CHECK_AGENT, m_bNeedAgent);
	DDX_Check(pDX, IDC_CHECK_MATCH_OID, m_bMatchOID);
	DDX_Check(pDX, IDC_CHECK_SNMP, m_bNeedSNMP);
	DDX_Radio(pDX, IDC_RADIO_SUFFIX, m_nIndexType);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CObjToolPropOptions, CPropertyPage)
	//{{AFX_MSG_MAP(CObjToolPropOptions)
	ON_BN_CLICKED(IDC_CHECK_MATCH_OID, OnCheckMatchOid)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjToolPropOptions message handlers


//
// WM_INITDIALOG message handler
//

BOOL CObjToolPropOptions::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
   if (m_iToolType != TOOL_TYPE_TABLE_SNMP)
   {
      EnableDlgItem(this, IDC_RADIO_SUFFIX, FALSE);
      EnableDlgItem(this, IDC_RADIO_VALUE, FALSE);
      EnableDlgItem(this, IDC_STATIC_SNMP_INDEX, FALSE);
      EnableDlgItem(this, IDC_STATIC_SNMP_OPTIONS, FALSE);
   }

   if (SendDlgItemMessage(IDC_CHECK_MATCH_OID, BM_GETCHECK) != BST_CHECKED)
   {
      EnableDlgItem(this, IDC_EDIT_TEMPLATE, FALSE);
      EnableDlgItem(this, IDC_STATIC_TEMPLATE, FALSE);
   }
	
	return TRUE;
}


//
// Handler for checking/unchecking "match OID"
//

void CObjToolPropOptions::OnCheckMatchOid() 
{
   BOOL bEnable;

   bEnable = (SendDlgItemMessage(IDC_CHECK_MATCH_OID, BM_GETCHECK) == BST_CHECKED);
   EnableDlgItem(this, IDC_EDIT_TEMPLATE, bEnable);
   EnableDlgItem(this, IDC_STATIC_TEMPLATE, bEnable);
}
