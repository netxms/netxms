// GraphSettingsPage.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "GraphSettingsPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Static data
//

static TCHAR *m_pszTimeUnits[] = { _T("Minutes"), _T("Hours"), _T("Days"), NULL };


/////////////////////////////////////////////////////////////////////////////
// CGraphSettingsPage property page

IMPLEMENT_DYNCREATE(CGraphSettingsPage, CPropertyPage)

CGraphSettingsPage::CGraphSettingsPage() : CPropertyPage(CGraphSettingsPage::IDD)
{
	//{{AFX_DATA_INIT(CGraphSettingsPage)
	m_bAutoscale = FALSE;
	m_bShowGrid = FALSE;
	m_bAutoUpdate = FALSE;
	m_dwRefreshInterval = 0;
	m_dwNumUnits = 0;
	m_iTimeFrame = -1;
	m_dateFrom = 0;
	m_dateTo = 0;
	m_timeFrom = 0;
	m_timeTo = 0;
	m_bRuler = FALSE;
	m_bShowLegend = FALSE;
	m_bEnableZoom = FALSE;
	m_bShowHostNames = FALSE;
	m_strTitle = _T("");
	m_bLogarithmicScale = FALSE;
	//}}AFX_DATA_INIT

   m_iTimeUnit = 0;
}

CGraphSettingsPage::~CGraphSettingsPage()
{
}

void CGraphSettingsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGraphSettingsPage)
	DDX_Control(pDX, IDC_COMBO_UNITS, m_wndTimeUnits);
	DDX_Check(pDX, IDC_CHECK_AUTOSCALE, m_bAutoscale);
	DDX_Check(pDX, IDC_CHECK_GRID, m_bShowGrid);
	DDX_Check(pDX, IDC_CHECK_REFRESH, m_bAutoUpdate);
	DDX_Text(pDX, IDC_EDIT_REFRESH, m_dwRefreshInterval);
	DDV_MinMaxDWord(pDX, m_dwRefreshInterval, 5, 600);
	DDX_Text(pDX, IDC_EDIT_COUNT, m_dwNumUnits);
	DDV_MinMaxDWord(pDX, m_dwNumUnits, 1, 100000);
	DDX_Radio(pDX, IDC_RADIO_FIXED, m_iTimeFrame);
	DDX_DateTimeCtrl(pDX, IDC_DATE_FROM, m_dateFrom);
	DDX_DateTimeCtrl(pDX, IDC_DATE_TO, m_dateTo);
	DDX_DateTimeCtrl(pDX, IDC_TIME_FROM, m_timeFrom);
	DDX_DateTimeCtrl(pDX, IDC_TIME_TO, m_timeTo);
	DDX_Check(pDX, IDC_CHECK_RULER, m_bRuler);
	DDX_Check(pDX, IDC_CHECK_LEGEND, m_bShowLegend);
	DDX_Check(pDX, IDC_CHECK_ZOOM, m_bEnableZoom);
	DDX_Check(pDX, IDC_CHECK_HOSTNAMES, m_bShowHostNames);
	DDX_Text(pDX, IDC_EDIT_TITLE, m_strTitle);
	DDV_MaxChars(pDX, m_strTitle, 127);
	DDX_Check(pDX, IDC_CHECK_LOGSCALE, m_bLogarithmicScale);
	//}}AFX_DATA_MAP

	DDX_Control(pDX, IDC_CB_BACKGROUND, m_wndCSBackground);
	DDX_ColourPickerXP(pDX, IDC_CB_BACKGROUND, m_rgbBackground);

	DDX_Control(pDX, IDC_CB_TEXT, m_wndCSText);
	DDX_ColourPickerXP(pDX, IDC_CB_TEXT, m_rgbText);

	DDX_Control(pDX, IDC_CB_AXIS, m_wndCSAxisLines);
	DDX_ColourPickerXP(pDX, IDC_CB_AXIS, m_rgbAxisLines);

	DDX_Control(pDX, IDC_CB_GRID, m_wndCSGridLines);
	DDX_ColourPickerXP(pDX, IDC_CB_GRID, m_rgbGridLines);

	DDX_Control(pDX, IDC_CB_SELECTION, m_wndCSSelection);
	DDX_ColourPickerXP(pDX, IDC_CB_SELECTION, m_rgbSelection);

	DDX_Control(pDX, IDC_CB_RULER, m_wndCSRuler);
	DDX_ColourPickerXP(pDX, IDC_CB_RULER, m_rgbRuler);
}


BEGIN_MESSAGE_MAP(CGraphSettingsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGraphSettingsPage)
	ON_BN_CLICKED(IDC_RADIO_FROM_NOW, OnRadioFromNow)
	ON_BN_CLICKED(IDC_RADIO_FIXED, OnRadioFixed)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGraphSettingsPage message handlers


//
// WM_INITDIALOG message handler
//

BOOL CGraphSettingsPage::OnInitDialog() 
{
   int i;

	CPropertyPage::OnInitDialog();
	
   // Setup time units list
   for(i = 0; i < MAX_TIME_UNITS; i++)
      m_wndTimeUnits.AddString(m_pszTimeUnits[i]);
   m_wndTimeUnits.SelectString(-1, m_pszTimeUnits[m_iTimeUnit]);

   if (m_iTimeFrame == 0)
   {
      // Fixed time frame
      EnableDlgItem(this, IDC_EDIT_COUNT, FALSE);
      EnableDlgItem(this, IDC_COMBO_UNITS, FALSE);
   }
   else
   {
      // Back from current time
      EnableDlgItem(this, IDC_DATE_FROM, FALSE);
      EnableDlgItem(this, IDC_DATE_TO, FALSE);
      EnableDlgItem(this, IDC_TIME_FROM, FALSE);
      EnableDlgItem(this, IDC_TIME_TO, FALSE);
      EnableDlgItem(this, IDC_STATIC_FROM, FALSE);
      EnableDlgItem(this, IDC_STATIC_TO, FALSE);
   }
	
	return TRUE;
}


//
// "OK" and "Apply" button handler
//

void CGraphSettingsPage::OnOK() 
{
   int i;
   TCHAR szBuffer[64];

   m_wndTimeUnits.GetWindowText(szBuffer, 64);
   for(i = 0; i < MAX_TIME_UNITS; i++)
      if (!_tcscmp(szBuffer, m_pszTimeUnits[i]))
      {
         m_iTimeUnit = i;
         break;
      }
	
	CPropertyPage::OnOK();
}


//
// Handlers for radio buttons
//

void CGraphSettingsPage::OnRadioFromNow() 
{
   EnableDlgItem(this, IDC_DATE_FROM, FALSE);
   EnableDlgItem(this, IDC_DATE_TO, FALSE);
   EnableDlgItem(this, IDC_TIME_FROM, FALSE);
   EnableDlgItem(this, IDC_TIME_TO, FALSE);
   EnableDlgItem(this, IDC_STATIC_FROM, FALSE);
   EnableDlgItem(this, IDC_STATIC_TO, FALSE);

   EnableDlgItem(this, IDC_EDIT_COUNT, TRUE);
   EnableDlgItem(this, IDC_COMBO_UNITS, TRUE);
}

void CGraphSettingsPage::OnRadioFixed() 
{
   EnableDlgItem(this, IDC_EDIT_COUNT, FALSE);
   EnableDlgItem(this, IDC_COMBO_UNITS, FALSE);

   EnableDlgItem(this, IDC_DATE_FROM, TRUE);
   EnableDlgItem(this, IDC_DATE_TO, TRUE);
   EnableDlgItem(this, IDC_TIME_FROM, TRUE);
   EnableDlgItem(this, IDC_TIME_TO, TRUE);
   EnableDlgItem(this, IDC_STATIC_FROM, TRUE);
   EnableDlgItem(this, IDC_STATIC_TO, TRUE);
}
