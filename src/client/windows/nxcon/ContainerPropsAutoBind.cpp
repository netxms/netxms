// ContainerPropsAutoBind.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ContainerPropsAutoBind.h"
#include "ObjectPropSheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CContainerPropsAutoBind property page

IMPLEMENT_DYNCREATE(CContainerPropsAutoBind, CPropertyPage)

CContainerPropsAutoBind::CContainerPropsAutoBind() : CPropertyPage(CContainerPropsAutoBind::IDD)
{
	//{{AFX_DATA_INIT(CContainerPropsAutoBind)
	m_bEnableAutoBind = FALSE;
	//}}AFX_DATA_INIT
}

CContainerPropsAutoBind::~CContainerPropsAutoBind()
{
   m_wndEditor.Detach();
}

void CContainerPropsAutoBind::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CContainerPropsAutoBind)
	DDX_Check(pDX, IDC_CHECK_ENABLE, m_bEnableAutoBind);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CContainerPropsAutoBind, CPropertyPage)
	//{{AFX_MSG_MAP(CContainerPropsAutoBind)
	ON_BN_CLICKED(IDC_CHECK_ENABLE, OnCheckEnable)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CContainerPropsAutoBind message handlers

//
// WM_INITDIALOG message handler
//

BOOL CContainerPropsAutoBind::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
   m_pUpdate = ((CObjectPropSheet *)GetParent())->GetUpdateStruct();
	
   m_wndEditor.Attach(::GetDlgItem(m_hWnd, IDC_EDIT_SCRIPT));
   m_wndEditor.SetDefaults();
   m_wndEditor.LoadLexer("nxlexer.dll");
   m_wndEditor.SetLexer("nxsl");
   m_wndEditor.SetKeywords(0, g_szScriptKeywords);
   m_wndEditor.SetText(m_strFilterScript);
	m_wndEditor.EnableWindow(m_bEnableAutoBind);

	m_bInitialEnableFlag = m_bEnableAutoBind;

	return TRUE;
}


//
// OK button handler
//

void CContainerPropsAutoBind::OnOK() 
{
	CPropertyPage::OnOK();

   // Set fields in update structure
   if ((m_wndEditor.GetModify() && m_bEnableAutoBind) ||
		 (!m_bEnableAutoBind && m_bInitialEnableFlag) ||
		 (m_bEnableAutoBind && !m_bInitialEnableFlag))
   {
		m_wndEditor.GetText(m_strFilterScript);

		m_pUpdate->qwFlags |= OBJ_UPDATE_AUTOBIND | OBJ_UPDATE_FLAGS;
		if (m_bEnableAutoBind)
			m_pUpdate->dwObjectFlags |= CF_AUTO_BIND;
		else
			m_pUpdate->dwObjectFlags &= ~CF_AUTO_BIND;
		m_pUpdate->pszAutoBindFilter = m_bEnableAutoBind ? (LPCTSTR)m_strFilterScript : _T("");
	}
}


//
// Handler for clicks on "enable" checkbox
//

void CContainerPropsAutoBind::OnCheckEnable() 
{
	if (SendDlgItemMessage(IDC_CHECK_ENABLE, BM_GETCHECK, 0, 0) == BST_CHECKED)
	{
		m_wndEditor.EnableWindow(TRUE);
		m_wndEditor.SetFocus();
	}
	else
	{
		m_wndEditor.EnableWindow(FALSE);
	}
}
