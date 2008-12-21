// EditEventDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "EditEventDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditEventDlg dialog


CEditEventDlg::CEditEventDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEditEventDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEditEventDlg)
	m_bWriteLog = FALSE;
	m_strName = _T("");
	m_strMessage = _T("");
	m_dwEventId = 0;
	m_strDescription = _T("");
	//}}AFX_DATA_INIT
}


void CEditEventDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditEventDlg)
	DDX_Control(pDX, IDC_COMBO_SEVERITY, m_wndComboBox);
	DDX_Check(pDX, IDC_CHECK_LOG, m_bWriteLog);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDX_Text(pDX, IDC_EDIT_MESSAGE, m_strMessage);
	DDX_Text(pDX, IDC_EDIT_ID, m_dwEventId);
	DDX_Text(pDX, IDC_EDIT_DESCRIPTION, m_strDescription);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEditEventDlg, CDialog)
	//{{AFX_MSG_MAP(CEditEventDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditEventDlg message handlers

BOOL CEditEventDlg::OnInitDialog() 
{
   int i;

	CDialog::OnInitDialog();

   for(i = 0; i < 5; i++)
      m_wndComboBox.AddString(g_szStatusTextSmall[i]);
   m_wndComboBox.SelectString(-1, g_szStatusTextSmall[m_dwSeverity]);
   
	return TRUE;
}


//
// Handle "OK" button
//

void CEditEventDlg::OnOK() 
{
   m_dwSeverity = m_wndComboBox.GetCurSel();
	
	CDialog::OnOK();
}

