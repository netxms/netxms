// SubmapBkgndDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "SubmapBkgndDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSubmapBkgndDlg dialog


CSubmapBkgndDlg::CSubmapBkgndDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSubmapBkgndDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSubmapBkgndDlg)
	m_strFileName = _T("");
	m_nScaleMethod = -1;
	m_nBkType = -1;
	//}}AFX_DATA_INIT
}


void CSubmapBkgndDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSubmapBkgndDlg)
	DDX_Text(pDX, IDC_EDIT_FILE, m_strFileName);
	DDV_MaxChars(pDX, m_strFileName, 255);
	DDX_Radio(pDX, IDC_RADIO_NO_SCALE, m_nScaleMethod);
	DDX_Radio(pDX, IDC_RADIO_NBK, m_nBkType);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSubmapBkgndDlg, CDialog)
	//{{AFX_MSG_MAP(CSubmapBkgndDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSubmapBkgndDlg message handlers
