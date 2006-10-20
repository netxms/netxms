// RuleScriptDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "RuleScriptDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRuleScriptDlg dialog


CRuleScriptDlg::CRuleScriptDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRuleScriptDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRuleScriptDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CRuleScriptDlg::~CRuleScriptDlg()
{
   m_wndEdit.Detach();
}

void CRuleScriptDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRuleScriptDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

   if (pDX->m_bSaveAndValidate)
   {
      m_wndEdit.GetText(m_strText);
   }
}


BEGIN_MESSAGE_MAP(CRuleScriptDlg, CDialog)
	//{{AFX_MSG_MAP(CRuleScriptDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRuleScriptDlg message handlers

BOOL CRuleScriptDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
   m_wndEdit.Attach(::GetDlgItem(m_hWnd, IDC_SCRIPT));
   m_wndEdit.SetDefaults();
   m_wndEdit.SetText((LPCTSTR)m_strText);
   m_wndEdit.LoadLexer("nxlexer.dll");
   m_wndEdit.SetLexer("nxsl");
   m_wndEdit.SetKeywords(0, g_szScriptKeywords);
	
	return TRUE;
}
