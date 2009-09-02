// ObjToolColumnEditDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjToolColumnEditDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjToolColumnEditDlg dialog


CObjToolColumnEditDlg::CObjToolColumnEditDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CObjToolColumnEditDlg::IDD, pParent)
{
	m_isCreate = FALSE;
	m_isAgentTable = FALSE;
	//{{AFX_DATA_INIT(CObjToolColumnEditDlg)
	m_type = -1;
	m_name = _T("");
	m_oid = _T("");
	m_oidLabel = _T("");
	//}}AFX_DATA_INIT
}


void CObjToolColumnEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CObjToolColumnEditDlg)
	DDX_Control(pDX, IDC_COMBO_TYPE, m_wndComboType);
	DDX_CBIndex(pDX, IDC_COMBO_TYPE, m_type);
	DDX_Text(pDX, IDC_EDIT_NAME, m_name);
	DDV_MaxChars(pDX, m_name, 255);
	DDX_Text(pDX, IDC_EDIT_OID, m_oid);
	DDV_MaxChars(pDX, m_oid, 255);
	DDX_Text(pDX, IDC_STATIC_OID, m_oidLabel);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CObjToolColumnEditDlg, CDialog)
	//{{AFX_MSG_MAP(CObjToolColumnEditDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjToolColumnEditDlg message handlers

BOOL CObjToolColumnEditDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	if (m_isCreate)
		SetWindowText(_T("Create Column"));

	return TRUE;
}

void CObjToolColumnEditDlg::OnOK() 
{
	if (m_isAgentTable)
	{
		TCHAR text[256], *eptr;

		GetDlgItemText(IDC_EDIT_OID, text, 256);
		int value = _tcstol(text, &eptr, 0);
		if ((value < 1) || (value > 1024) || (*eptr != 0))
		{
			MessageBox(_T("Invalid substring number entered!\r\nYou should enter a number in range 1..1024"), _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
			return;
		}
	}
	CDialog::OnOK();
}
