// ThresholdDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ThresholdDlg.h"
#include "EventSelDlg.h"

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
	m_nSeconds = 0;
	m_nRepeat = -1;
	//}}AFX_DATA_INIT
}


void CThresholdDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CThresholdDlg)
	DDX_Control(pDX, IDC_EDIT_REARM_EVENT, m_wndRearmEventName);
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
	DDX_Text(pDX, IDC_EDIT_SECONDS, m_nSeconds);
	DDV_MinMaxLong(pDX, m_nSeconds, 1, 1000000);
	DDX_Radio(pDX, IDC_RADIO_DEFAULT, m_nRepeat);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CThresholdDlg, CDialog)
	//{{AFX_MSG_MAP(CThresholdDlg)
	ON_CBN_SELCHANGE(IDC_COMBO_FUNCTION, OnSelchangeComboFunction)
	ON_BN_CLICKED(IDC_BUTTON_SELECT, OnButtonSelect)
	ON_BN_CLICKED(IDC_BUTTON_SELECT_REARM_EVENT, OnButtonSelectRearmEvent)
	ON_BN_CLICKED(IDC_RADIO_DEFAULT, OnRadioDefault)
	ON_BN_CLICKED(IDC_RADIO_NEVER, OnRadioNever)
	ON_BN_CLICKED(IDC_RADIO_REPEAT, OnRadioRepeat)
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
   int nIndex = -1;

	CDialog::OnInitDialog();

   // Setup lists
   for(i = 0; i < 5; i++)
      m_wndComboFunction.AddString(g_pszThresholdFunctionLong[i]);
   for(i = 0; i < 8; i++)
      m_wndComboOperation.AddString(g_pszThresholdOperationLong[i]);
   m_wndComboFunction.SelectString(-1, g_pszThresholdFunctionLong[m_iFunction]);

   do
   {
      nIndex = m_wndComboOperation.SelectString(nIndex, g_pszThresholdOperationLong[m_iOperation]);
   } while((nIndex != m_iOperation) && (nIndex != CB_ERR));

   switch(m_iFunction)
   {
      case F_LAST:
      case F_DIFF:
         m_wndStaticFor.EnableWindow(FALSE);
         m_wndStaticSamples.EnableWindow(FALSE);
         m_wndEditArg1.EnableWindow(FALSE);
         break;
      case F_ERROR:
         m_wndComboOperation.EnableWindow(FALSE);
         EnableDlgItem(this, IDC_EDIT_VALUE, FALSE);
         EnableDlgItem(this, IDC_STATIC_WILLBE, FALSE);
         EnableDlgItem(this, IDC_STATIC_COMPARE, FALSE);
         break;
   }

   m_wndEventName.SetWindowText(NXCGetEventName(g_hSession, m_dwEventId));
   m_wndRearmEventName.SetWindowText(NXCGetEventName(g_hSession, m_dwRearmEventId));

	EnableDlgItem(this, IDC_EDIT_SECONDS, m_nRepeat == 2);
	
	return TRUE;
}


//
// Handler for selection change in functions
//

void CThresholdDlg::OnSelchangeComboFunction() 
{
   m_iFunction = m_wndComboFunction.GetCurSel();
   m_wndStaticFor.EnableWindow((m_iFunction != F_LAST) && (m_iFunction != F_DIFF));
   m_wndStaticSamples.EnableWindow((m_iFunction != F_LAST) && (m_iFunction != F_DIFF));
   m_wndEditArg1.EnableWindow((m_iFunction != F_LAST) && (m_iFunction != F_DIFF));
   m_wndComboOperation.EnableWindow(m_iFunction != F_ERROR);
   EnableDlgItem(this, IDC_EDIT_VALUE, m_iFunction != F_ERROR);
   EnableDlgItem(this, IDC_STATIC_WILLBE, m_iFunction != F_ERROR);
   EnableDlgItem(this, IDC_STATIC_COMPARE, m_iFunction != F_ERROR);
}


//
// "Select" button handlers
//

void CThresholdDlg::OnButtonSelect() 
{
   CEventSelDlg dlg;

   dlg.m_bSingleSelection = TRUE;
   if (dlg.DoModal() == IDOK)
   {
      m_dwEventId = dlg.m_pdwEventList[0];
      m_wndEventName.SetWindowText(NXCGetEventName(g_hSession, m_dwEventId));
   }
}

void CThresholdDlg::OnButtonSelectRearmEvent()
{
   CEventSelDlg dlg;

   dlg.m_bSingleSelection = TRUE;
   if (dlg.DoModal() == IDOK)
   {
      m_dwRearmEventId = dlg.m_pdwEventList[0];
      m_wndRearmEventName.SetWindowText(NXCGetEventName(g_hSession, m_dwRearmEventId));
   }
}


//
// Handlers for repeat interval radio buttons
//

void CThresholdDlg::OnRadioDefault() 
{
	EnableDlgItem(this, IDC_EDIT_SECONDS, FALSE);
}

void CThresholdDlg::OnRadioNever() 
{
	EnableDlgItem(this, IDC_EDIT_SECONDS, FALSE);
}

void CThresholdDlg::OnRadioRepeat() 
{
	EnableDlgItem(this, IDC_EDIT_SECONDS, TRUE);
}
