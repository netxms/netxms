// RequestProcessingDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "RequestProcessingDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRequestProcessingDlg dialog


CRequestProcessingDlg::CRequestProcessingDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRequestProcessingDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRequestProcessingDlg)
	m_strInfoText = _T("");
	//}}AFX_DATA_INIT
}


void CRequestProcessingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRequestProcessingDlg)
	DDX_Text(pDX, IDC_INFO_TEXT, m_strInfoText);
	DDV_MaxChars(pDX, m_strInfoText, 255);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRequestProcessingDlg, CDialog)
	//{{AFX_MSG_MAP(CRequestProcessingDlg)
	//}}AFX_MSG_MAP
   ON_MESSAGE(WM_REQUEST_COMPLETED, OnRequestCompleted)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRequestProcessingDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CRequestProcessingDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
   theApp.RegisterRequest(m_hRequest, this);

	return TRUE;
}


//
// WM_REQUEST_COMPLETED message handler
//

void CRequestProcessingDlg::OnRequestCompleted(WPARAM wParam, LPARAM lParam)
{
   if (wParam == m_hRequest)
      EndDialog(lParam);
}
