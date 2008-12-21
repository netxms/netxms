// AgentCfgDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "AgentCfgDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAgentCfgDlg dialog


CAgentCfgDlg::CAgentCfgDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAgentCfgDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAgentCfgDlg)
	m_strName = _T("");
	//}}AFX_DATA_INIT
}

CAgentCfgDlg::~CAgentCfgDlg()
{
   m_wndEditText.Detach();
   m_wndEditFilter.Detach();
}

void CAgentCfgDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAgentCfgDlg)
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDV_MaxChars(pDX, m_strName, 255);
	//}}AFX_DATA_MAP

   if (pDX->m_bSaveAndValidate)
   {
      m_wndEditFilter.GetText(m_strFilter);
      m_wndEditText.GetText(m_strText);
   }
}


BEGIN_MESSAGE_MAP(CAgentCfgDlg, CDialog)
	//{{AFX_MSG_MAP(CAgentCfgDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAgentCfgDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CAgentCfgDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

   m_wndEditFilter.Attach(::GetDlgItem(m_hWnd, IDC_EDIT_FILTER));
   m_wndEditFilter.SetDefaults();
   m_wndEditFilter.SetText((LPCTSTR)m_strFilter);
   m_wndEditFilter.LoadLexer("nxlexer.dll");
   m_wndEditFilter.SetLexer("nxsl");
   m_wndEditFilter.SetKeywords(0, g_szScriptKeywords);

   m_wndEditText.Attach(::GetDlgItem(m_hWnd, IDC_EDIT_CONFIG));
   m_wndEditText.SetDefaults();
   m_wndEditText.SetText((LPCTSTR)m_strText);
   m_wndEditText.LoadLexer("nxlexer.dll");
   m_wndEditText.SetLexer("nxconfig");
   m_wndEditText.SetKeywords(0, g_szConfigKeywords);
	
	return TRUE;
}
