// CreateTGDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "CreateTGDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCreateTGDlg dialog


CCreateTGDlg::CCreateTGDlg(CWnd* pParent /*=NULL*/)
	: CCreateObjectDlg(CCreateTGDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCreateTGDlg)
	m_strDescription = _T("");
	//}}AFX_DATA_INIT
}


void CCreateTGDlg::DoDataExchange(CDataExchange* pDX)
{
	CCreateObjectDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCreateTGDlg)
	DDX_Text(pDX, IDC_EDIT_DESCRIPTION, m_strDescription);
	DDV_MaxChars(pDX, m_strDescription, 255);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCreateTGDlg, CCreateObjectDlg)
	//{{AFX_MSG_MAP(CCreateTGDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCreateTGDlg message handlers
