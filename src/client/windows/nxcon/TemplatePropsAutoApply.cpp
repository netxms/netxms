// TemplatePropsAutoApply.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "TemplatePropsAutoApply.h"
#include "ObjectPropSheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTemplatePropsAutoApply property page

IMPLEMENT_DYNCREATE(CTemplatePropsAutoApply, CPropertyPage)

CTemplatePropsAutoApply::CTemplatePropsAutoApply() : CPropertyPage(CTemplatePropsAutoApply::IDD)
{
	//{{AFX_DATA_INIT(CTemplatePropsAutoApply)
	m_bEnableAutoApply = FALSE;
	//}}AFX_DATA_INIT
}

CTemplatePropsAutoApply::~CTemplatePropsAutoApply()
{
   m_wndEditor.Detach();
}

void CTemplatePropsAutoApply::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTemplatePropsAutoApply)
	DDX_Check(pDX, IDC_CHECK_ENABLE, m_bEnableAutoApply);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTemplatePropsAutoApply, CPropertyPage)
	//{{AFX_MSG_MAP(CTemplatePropsAutoApply)
	ON_BN_CLICKED(IDC_CHECK_ENABLE, OnCheckEnable)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTemplatePropsAutoApply message handlers

BOOL CTemplatePropsAutoApply::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
   m_pUpdate = ((CObjectPropSheet *)GetParent())->GetUpdateStruct();
	
   m_wndEditor.Attach(::GetDlgItem(m_hWnd, IDC_EDIT_SCRIPT));
   m_wndEditor.SetDefaults();
   m_wndEditor.LoadLexer("nxlexer.dll");
   m_wndEditor.SetLexer("nxsl");
   m_wndEditor.SetKeywords(0, g_szScriptKeywords);
   m_wndEditor.SetText(m_strFilterScript);
	m_wndEditor.EnableWindow(m_bEnableAutoApply);

	m_bInitialEnableFlag = m_bEnableAutoApply;

	return TRUE;
}


//
// OK button handler
//

void CTemplatePropsAutoApply::OnOK() 
{
	CPropertyPage::OnOK();

   // Set fields in update structure
   if ((m_wndEditor.GetModify() && m_bEnableAutoApply) ||
		 (!m_bEnableAutoApply && m_bInitialEnableFlag) ||
		 (m_bEnableAutoApply && !m_bInitialEnableFlag))
   {
		m_wndEditor.GetText(m_strFilterScript);

		m_pUpdate->qwFlags |= OBJ_UPDATE_AUTOBIND | OBJ_UPDATE_FLAGS;
		if (m_bEnableAutoApply)
			m_pUpdate->dwObjectFlags |= TF_AUTO_APPLY;
		else
			m_pUpdate->dwObjectFlags &= ~TF_AUTO_APPLY;
		m_pUpdate->pszAutoBindFilter = m_bEnableAutoApply ? (LPCTSTR)m_strFilterScript : _T("");
	}
}


//
// Handler for clicks on "enable" checkbox
//

void CTemplatePropsAutoApply::OnCheckEnable() 
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
