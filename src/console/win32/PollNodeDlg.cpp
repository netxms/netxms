// PollNodeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "PollNodeDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPollNodeDlg dialog


CPollNodeDlg::CPollNodeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPollNodeDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPollNodeDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   m_hThread = NULL;
   m_dwResult = RCC_SYSTEM_FAILURE;
}


void CPollNodeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPollNodeDlg)
	DDX_Control(pDX, IDCANCEL, m_wndCloseButton);
	DDX_Control(pDX, IDC_EDIT_MSG, m_wndMsgBox);
	DDX_Control(pDX, IDC_POLL_PROGRESS, m_wndProgressBar);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPollNodeDlg, CDialog)
	//{{AFX_MSG_MAP(CPollNodeDlg)
	//}}AFX_MSG_MAP
   ON_MESSAGE(WM_REQUEST_COMPLETED, OnRequestCompleted)
   ON_MESSAGE(WM_POLLER_MESSAGE, OnPollerMessage)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPollNodeDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CPollNodeDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
   // Setup message area
   m_font.CreateFont(-MulDiv(8, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                     0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                     OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                     FIXED_PITCH | FF_DONTCARE, "Courier");
   m_wndMsgBox.SetFont(&m_font);

   *m_phWnd = m_hWnd;
   m_wndCloseButton.EnableWindow(FALSE);
   ResumeThread(m_hThread);
	return TRUE;
}


//
// WM_REQUEST_COMPLETED message handler
//

void CPollNodeDlg::OnRequestCompleted(WPARAM wParam, LPARAM lParam)
{
   m_dwResult = lParam;
   m_wndCloseButton.EnableWindow(TRUE);
}


//
// WM_POLLER_MESSAGE message handler
//

void CPollNodeDlg::OnPollerMessage(WPARAM wParam, LPARAM lParam)
{
   m_wndMsgBox.SetSel(0, -1);
   m_wndMsgBox.SetSel(-1, -1);
   m_wndMsgBox.ReplaceSel((LPCTSTR)lParam);
   free((void *)lParam);
}


//
// Handler for "Close" button
//

void CPollNodeDlg::OnCancel() 
{
   EndDialog(m_dwResult);
}
