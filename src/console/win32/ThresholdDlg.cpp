// ThresholdDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ThresholdDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CThresholdDlg dialog


CThresholdDlg::CThresholdDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CThresholdDlg::IDD, pParent)
{
   m_dwEventId = EVENT_THRESHOLD_REACHED;
	//{{AFX_DATA_INIT(CThresholdDlg)
	m_iFunction = -1;
	m_strValue = _T("");
	m_dwArg1 = 0;
	m_iOperation = -1;
	//}}AFX_DATA_INIT
}


void CThresholdDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CThresholdDlg)
	DDX_Control(pDX, IDC_EDIT_EVENT, m_wndEventName);
	DDX_Control(pDX, IDC_STATIC_SAMPLES, m_wndStaticSamples);
	DDX_Control(pDX, IDC_STATIC_FOR, m_wndStaticFor);
	DDX_Control(pDX, IDC_EDIT_ARG1, m_wndEditArg1);
	DDX_Control(pDX, IDC_COMBO_OPERATION, m_wndComboOperation);
	DDX_Control(pDX, IDC_COMBO_FUNCTION, m_wndComboFunction);
	DDX_CBIndex(pDX, IDC_COMBO_FUNCTION, m_iFunction);
	DDX_Text(pDX, IDC_EDIT_VALUE, m_strValue);
	DDV_MaxChars(pDX, m_strValue, 255);
	DDX_Text(pDX, IDC_EDIT_ARG1, m_dwArg1);
	DDX_CBIndex(pDX, IDC_COMBO_OPERATION, m_iOperation);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CThresholdDlg, CDialog)
	//{{AFX_MSG_MAP(CThresholdDlg)
	ON_CBN_SELCHANGE(IDC_COMBO_FUNCTION, OnSelchangeComboFunction)
	ON_BN_CLICKED(IDC_BUTTON_SELECT, OnButtonSelect)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CThresholdDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CThresholdDlg::OnInitDialog() 
{
   DWORD i;

	CDialog::OnInitDialog();

   // Setup lists
   for(i = 0; i < 3; i++)
      m_wndComboFunction.AddString(g_pszThresholdFunctionLong[i]);
   for(i = 0; i < 8; i++)
      m_wndComboOperation.AddString(g_pszThresholdOperationLong[i]);
   m_wndComboFunction.SelectString(-1, g_pszThresholdFunctionLong[m_iFunction]);
   m_wndComboOperation.SelectString(-1, g_pszThresholdOperationLong[m_iOperation]);

   if (m_iFunction == F_LAST)
   {
      m_wndStaticFor.EnableWindow(FALSE);
      m_wndStaticSamples.EnableWindow(FALSE);
      m_wndEditArg1.EnableWindow(FALSE);
   }

   m_wndEventName.SetWindowText(NXCGetEventName(m_dwEventId));
	
	return TRUE;
}


//
// Handler for selection change in functions
//

void CThresholdDlg::OnSelchangeComboFunction() 
{
   m_iFunction = m_wndComboFunction.GetCurSel();
   m_wndStaticFor.EnableWindow(m_iFunction != F_LAST);
   m_wndStaticSamples.EnableWindow(m_iFunction != F_LAST);
   m_wndEditArg1.EnableWindow(m_iFunction != F_LAST);
}


//
// "Select" button handler
//

void CThresholdDlg::OnButtonSelect() 
{
   CEventSelDlg dlg;

   dlg.m_bSingleSelection = TRUE;
   if (dlg.DoModal() == IDOK)
   {
      m_dwEventId = dlg.m_pdwEventList[0];
      m_wndEventName.SetWindowText(NXCGetEventName(m_dwEventId));
   }
}
