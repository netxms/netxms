// RequestProcessingDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxav.h"
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
   m_hThread = NULL;
}


void CRequestProcessingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRequestProcessingDlg)
	DDX_Control(pDX, IDC_INFO_TEXT, m_wndInfoText);
	DDX_Text(pDX, IDC_INFO_TEXT, m_strInfoText);
	DDV_MaxChars(pDX, m_strInfoText, 255);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRequestProcessingDlg, CDialog)
	//{{AFX_MSG_MAP(CRequestProcessingDlg)
	//}}AFX_MSG_MAP
   ON_MESSAGE(WM_REQUEST_COMPLETED, OnRequestCompleted)
   ON_MESSAGE(WM_SET_INFO_TEXT, OnSetInfoText)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRequestProcessingDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CRequestProcessingDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

   *m_phWnd = m_hWnd;
   // Check if our worker thread already finished
   if (WaitForSingleObject(m_hThread, 0) == WAIT_OBJECT_0)
   {
      DWORD dwResult = RCC_SYSTEM_FAILURE;

      // Finished, get it's exit code and close status window
      GetExitCodeThread(m_hThread, &dwResult);
      PostMessage(WM_REQUEST_COMPLETED, 0, dwResult);
   }
   else
   {
      ResumeThread(m_hThread);
   }

	return TRUE;
}


//
// WM_REQUEST_COMPLETED message handler
//

LRESULT CRequestProcessingDlg::OnRequestCompleted(WPARAM wParam, LPARAM lParam)
{
   EndDialog((int)lParam);
	return 0;
}


//
// WM_SET_INFO_TEXT message handler
//

LRESULT CRequestProcessingDlg::OnSetInfoText(WPARAM wParam, LPARAM lParam)
{
   m_wndInfoText.SetWindowText((LPCTSTR)lParam);
	return 0;
}
