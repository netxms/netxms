// EditActionDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "EditActionDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditActionDlg dialog


CEditActionDlg::CEditActionDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEditActionDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEditActionDlg)
	m_strData = _T("");
	m_strName = _T("");
	m_strRcpt = _T("");
	m_strSubject = _T("");
	m_iType = -1;
	//}}AFX_DATA_INIT
}


void CEditActionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditActionDlg)
	DDX_Text(pDX, IDC_EDIT_DATA, m_strData);
	DDV_MaxChars(pDX, m_strData, 4095);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDV_MaxChars(pDX, m_strName, 63);
	DDX_Text(pDX, IDC_EDIT_RCPT, m_strRcpt);
	DDV_MaxChars(pDX, m_strRcpt, 255);
	DDX_Text(pDX, IDC_EDIT_SUBJECT, m_strSubject);
	DDV_MaxChars(pDX, m_strSubject, 255);
	DDX_Radio(pDX, IDC_RADIO_EXEC, m_iType);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEditActionDlg, CDialog)
	//{{AFX_MSG_MAP(CEditActionDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditActionDlg message handlers
