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
	//}}AFX_DATA_MAP
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
   static int piItemList[MAX_GRAPH_ITEMS] = 
      { IDC_CB_ITEM1, IDC_CB_ITEM2, IDC_CB_ITEM3, IDC_CB_ITEM4,
        IDC_CB_ITEM5, IDC_CB_ITEM6, IDC_CB_ITEM7, IDC_CB_ITEM8,
        IDC_CB_ITEM9, IDC_CB_ITEM10, IDC_CB_ITEM11, IDC_CB_ITEM12,
        IDC_CB_ITEM13, IDC_CB_ITEM14, IDC_CB_ITEM15, IDC_CB_ITEM16 };

	CPropertyPage::OnInitDialog();
	
   // Init color selectors
   m_wndCSBackground.m_rgbColor = m_rgbBackground;
   m_wndCSText.m_rgbColor = m_rgbText;
   m_wndCSAxisLines.m_rgbColor = m_rgbAxisLines;
   m_wndCSGridLines.m_rgbColor = m_rgbGridLines;
   m_wndCSLabelBkgnd.m_rgbColor = m_rgbLabelBkgnd;
   m_wndCSLabelText.m_rgbColor = m_rgbLabelText;

   // Subclass color selectors
   m_wndCSBackground.SubclassDlgItem(IDC_CB_BACKGROUND, this);
   m_wndCSText.SubclassDlgItem(IDC_CB_TEXT, this);
   m_wndCSAxisLines.SubclassDlgItem(IDC_CB_AXIS, this);
   m_wndCSGridLines.SubclassDlgItem(IDC_CB_GRID, this);
   m_wndCSLabelBkgnd.SubclassDlgItem(IDC_CB_LABELBK, this);
   m_wndCSLabelText.SubclassDlgItem(IDC_CB_LABEL, this);
   for(i = 0; i < MAX_GRAPH_ITEMS; i++)
   {
      m_pwndCSItem[i].m_rgbColor = m_rgbItems[i];
      m_pwndCSItem[i].SubclassDlgItem(piItemList[i], this);
   }

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

   m_rgbBackground = m_wndCSBackground.m_rgbColor;
   m_rgbText = m_wndCSText.m_rgbColor;
   m_rgbAxisLines = m_wndCSAxisLines.m_rgbColor;
   m_rgbGridLines = m_wndCSGridLines.m_rgbColor;
   m_rgbLabelText = m_wndCSLabelText.m_rgbColor;
   m_rgbLabelBkgnd = m_wndCSLabelBkgnd.m_rgbColor;
   for(i = 0; i < MAX_GRAPH_ITEMS; i++)
      m_rgbItems[i] = m_pwndCSItem[i].m_rgbColor;

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
