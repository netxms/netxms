// TransformTestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "TransformTestDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTransformTestDlg dialog


CTransformTestDlg::CTransformTestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTransformTestDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTransformTestDlg)
	//}}AFX_DATA_INIT
}


void CTransformTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTransformTestDlg)
	DDX_Control(pDX, IDC_STATUS_ICON, m_wndStatusIcon);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTransformTestDlg, CDialog)
	//{{AFX_MSG_MAP(CTransformTestDlg)
	ON_BN_CLICKED(IDC_BUTTON_RUN, OnButtonRun)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTransformTestDlg message handlers

BOOL CTransformTestDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	SetDlgItemText(IDC_STATUS_TEXT, _T(""));

	return TRUE;
}


//
// Handler for "Run" button
//

void CTransformTestDlg::OnButtonRun() 
{
	DWORD dwResult;
	CString value;
	TCHAR result[256];
	BOOL status;

	GetDlgItemText(IDC_EDIT_VALUE, value);
	dwResult = DoRequestArg8(NXCTestDCITransformation, g_hSession,
	                         CAST_TO_POINTER(m_dwNodeId, void *),
	                         CAST_TO_POINTER(m_dwItemId, void *),
									 (void *)((LPCTSTR)m_strScript),
	                         (void *)((LPCTSTR)value), &status,
									 result, CAST_TO_POINTER(sizeof(result) / sizeof(TCHAR), void *),
									 _T("Requesting script execution on server..."));
	if (dwResult == RCC_SUCCESS)
	{
		if (status)
		{
			SetDlgItemText(IDC_STATUS_TEXT, _T("Script execution was successful"));
			SetDlgItemText(IDC_RESULT_TITLE, _T("Resulting value"));
			m_wndStatusIcon.SetIcon(theApp.LoadIcon(IDI_ACK));
		}
		else
		{
			SetDlgItemText(IDC_STATUS_TEXT, _T("Script execution failed"));
			SetDlgItemText(IDC_RESULT_TITLE, _T("Error message"));
			m_wndStatusIcon.SetIcon(theApp.LoadIcon(IDI_NACK));
		}
		SetDlgItemText(IDC_EDIT_RESULT, result);
	}
	else
	{
		SetDlgItemText(IDC_STATUS_TEXT, _T("Request failed"));
		SetDlgItemText(IDC_RESULT_TITLE, _T("Error message"));
		SetDlgItemText(IDC_EDIT_RESULT, NXCGetErrorText(dwResult));
		m_wndStatusIcon.SetIcon(theApp.LoadIcon(IDI_NACK));
	}
}
