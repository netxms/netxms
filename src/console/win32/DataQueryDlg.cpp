// DataQueryDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "DataQueryDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDataQueryDlg dialog


CDataQueryDlg::CDataQueryDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDataQueryDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDataQueryDlg)
	m_strNode = _T("");
	m_strParameter = _T("");
	//}}AFX_DATA_INIT
}


void CDataQueryDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDataQueryDlg)
	DDX_Control(pDX, IDC_EDIT_VALUE, m_wndEditValue);
	DDX_Text(pDX, IDC_STATIC_NODE, m_strNode);
	DDX_Text(pDX, IDC_STATIC_PARAMETER, m_strParameter);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDataQueryDlg, CDialog)
	//{{AFX_MSG_MAP(CDataQueryDlg)
	ON_BN_CLICKED(IDC_BUTTON_RESTART, OnButtonRestart)
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDataQueryDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CDataQueryDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
   PostMessage(WM_COMMAND, IDC_BUTTON_RESTART);
	return TRUE;
}


//
// Handler for "Restart" button
//

void CDataQueryDlg::OnButtonRestart() 
{
   DWORD dwResult;
   TCHAR szBuffer[1024];

   dwResult = DoRequestArg5(NXCQueryParameter, (void *)m_dwObjectId, (void *)m_iOrigin,
                            (void *)((LPCTSTR)m_strParameter), szBuffer, 
                            (void *)1024, _T("Querying agent..."));
   if (dwResult == RCC_SUCCESS)
   {
      m_wndEditValue.SetWindowText(szBuffer);
   }
   else
   {
      _sntprintf(szBuffer, 1024, _T("Cannot get parameter: %s"), NXCGetErrorText(dwResult));
      MessageBox(szBuffer, _T("Error"), MB_OK | MB_ICONSTOP);
      m_wndEditValue.SetWindowText(szBuffer);
   }
}


//
// WM_CTLCOLOR message handler
//

HBRUSH CDataQueryDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr;
	
   if (pWnd->GetDlgCtrlID() == IDC_EDIT_VALUE)
   {
      hbr = GetSysColorBrush(COLOR_WINDOW);
      pDC->SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
      pDC->SetBkColor(GetSysColor(COLOR_WINDOW));
   }
   else
   {
      hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
   }

	return hbr;
}
