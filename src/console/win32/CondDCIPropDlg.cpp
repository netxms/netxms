// CondDCIPropDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "CondDCIPropDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static CODE_TO_TEXT m_ctFunctions[] =
{
   { F_LAST, _T("Last") },
   { F_AVERAGE, _T("Average") },
   { F_DIFF, _T("Diff") },
   { F_ERROR, _T("Error") },
   { F_DEVIATION, _T("Mean Deviation") },
   { 0, NULL }
};

/////////////////////////////////////////////////////////////////////////////
// CCondDCIPropDlg dialog


CCondDCIPropDlg::CCondDCIPropDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCondDCIPropDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCondDCIPropDlg)
	m_nPolls = 0;
	m_strItem = _T("");
	m_strNode = _T("");
	//}}AFX_DATA_INIT
}


void CCondDCIPropDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCondDCIPropDlg)
	DDX_Control(pDX, IDC_COMBO_FUNCTION, m_wndComboFunc);
	DDX_Text(pDX, IDC_EDIT_POLLS, m_nPolls);
	DDV_MinMaxInt(pDX, m_nPolls, 1, 100);
	DDX_Text(pDX, IDC_EDIT_ITEM, m_strItem);
	DDX_Text(pDX, IDC_EDIT_NODE, m_strNode);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCondDCIPropDlg, CDialog)
	//{{AFX_MSG_MAP(CCondDCIPropDlg)
	ON_CBN_SELCHANGE(IDC_COMBO_FUNCTION, OnSelchangeComboFunction)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//
// WM_INITDIALOG message handler
//

BOOL CCondDCIPropDlg::OnInitDialog() 
{
   int i;

	CDialog::OnInitDialog();
	
   for(i = 0; m_ctFunctions[i].pszText != NULL; i++)
      m_wndComboFunc.AddString(m_ctFunctions[i].pszText);
   m_wndComboFunc.SelectString(-1, CodeToText(m_nFunction, m_ctFunctions, _T("error")));
   EnableDlgItem(this, IDC_EDIT_POLLS, m_nFunction == F_AVERAGE);
	
	return TRUE;
}


//
// "OK" button handler
//

void CCondDCIPropDlg::OnOK() 
{
   TCHAR szText[256];
   int i;

   m_wndComboFunc.GetWindowText(szText, 256);
   for(i = 0; m_ctFunctions[i].pszText != NULL; i++)
      if (!_tcscmp(szText, m_ctFunctions[i].pszText))
      {
         m_nFunction = m_ctFunctions[i].iCode;
         break;
      }
	CDialog::OnOK();
}


//
// Handler for function selection change
//

void CCondDCIPropDlg::OnSelchangeComboFunction() 
{
   TCHAR szText[256];

   m_wndComboFunc.GetWindowText(szText, 256);
   EnableDlgItem(this, IDC_EDIT_POLLS, 
                 (!_tcscmp(szText, CodeToText(F_AVERAGE, m_ctFunctions, _T("")))) ||
                 (!_tcscmp(szText, CodeToText(F_DEVIATION, m_ctFunctions, _T("")))) ||
                 (!_tcscmp(szText, CodeToText(F_ERROR, m_ctFunctions, _T("")))));
}
