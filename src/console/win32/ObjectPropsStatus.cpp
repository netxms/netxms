// ObjectPropsStatus.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectPropsStatus.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjectPropsStatus property page

IMPLEMENT_DYNCREATE(CObjectPropsStatus, CPropertyPage)

CObjectPropsStatus::CObjectPropsStatus() : CPropertyPage(CObjectPropsStatus::IDD)
{
	//{{AFX_DATA_INIT(CObjectPropsStatus)
	m_iRelChange = 0;
	m_dSingleThreshold = 0.0;
	m_dThreshold1 = 0.0;
	m_dThreshold2 = 0.0;
	m_dThreshold3 = 0.0;
	m_dThreshold4 = 0.0;
	m_iCalcAlg = -1;
	m_iPropAlg = -1;
	m_iFixedStatus = -1;
	m_iStatusTranslation1 = -1;
	m_iStatusTranslation2 = -1;
	m_iStatusTranslation3 = -1;
	m_iStatusTranslation4 = -1;
	//}}AFX_DATA_INIT
   m_iFixedStatus = 0;
   m_iStatusTranslation1 = 0;
   m_iStatusTranslation2 = 0;
   m_iStatusTranslation3 = 0;
   m_iStatusTranslation4 = 0;
}

CObjectPropsStatus::~CObjectPropsStatus()
{
}

void CObjectPropsStatus::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CObjectPropsStatus)
	DDX_Control(pDX, IDC_COMBO_S4, m_wndComboS4);
	DDX_Control(pDX, IDC_COMBO_S3, m_wndComboS3);
	DDX_Control(pDX, IDC_COMBO_S2, m_wndComboS2);
	DDX_Control(pDX, IDC_COMBO_S1, m_wndComboS1);
	DDX_Control(pDX, IDC_COMBO_FIXED, m_wndComboFixed);
	DDX_Text(pDX, IDC_EDIT_RELATIVE, m_iRelChange);
	DDV_MinMaxInt(pDX, m_iRelChange, -4, 4);
	DDX_Text(pDX, IDC_EDIT_THRESHOLD, m_dSingleThreshold);
	DDV_MinMaxDouble(pDX, m_dSingleThreshold, 0., 1.);
	DDX_Text(pDX, IDC_EDIT_T1, m_dThreshold1);
	DDV_MinMaxDouble(pDX, m_dThreshold1, 0., 1.);
	DDX_Text(pDX, IDC_EDIT_T2, m_dThreshold2);
	DDV_MinMaxDouble(pDX, m_dThreshold2, 0., 1.);
	DDX_Text(pDX, IDC_EDIT_T3, m_dThreshold3);
	DDV_MinMaxDouble(pDX, m_dThreshold3, 0., 1.);
	DDX_Text(pDX, IDC_EDIT_T4, m_dThreshold4);
	DDV_MinMaxDouble(pDX, m_dThreshold4, 0., 1.);
	DDX_Radio(pDX, IDC_RADIO_CALC_DEFAULT, m_iCalcAlg);
	DDX_Radio(pDX, IDC_RADIO_PROP_DEFAULT, m_iPropAlg);
	DDX_CBIndex(pDX, IDC_COMBO_FIXED, m_iFixedStatus);
	DDX_CBIndex(pDX, IDC_COMBO_S1, m_iStatusTranslation1);
	DDX_CBIndex(pDX, IDC_COMBO_S2, m_iStatusTranslation2);
	DDX_CBIndex(pDX, IDC_COMBO_S3, m_iStatusTranslation3);
	DDX_CBIndex(pDX, IDC_COMBO_S4, m_iStatusTranslation4);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CObjectPropsStatus, CPropertyPage)
	//{{AFX_MSG_MAP(CObjectPropsStatus)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjectPropsStatus message handlers


//
// WM_INITDIALOG message handler
//

BOOL CObjectPropsStatus::OnInitDialog() 
{
   int i;

	CPropertyPage::OnInitDialog();
	
   m_pUpdate = ((CObjectPropSheet *)GetParent())->GetUpdateStruct();
   m_bIsModified = FALSE;

   // Setup combo boxes
   for(i = 0; i < 5; i++)
   {
      m_wndComboFixed.AddString(g_szStatusTextSmall[i]);
      m_wndComboS1.AddString(g_szStatusTextSmall[i]);
      m_wndComboS2.AddString(g_szStatusTextSmall[i]);
      m_wndComboS3.AddString(g_szStatusTextSmall[i]);
      m_wndComboS4.AddString(g_szStatusTextSmall[i]);
   }
   m_wndComboFixed.SelectString(-1, g_szStatusTextSmall[m_iFixedStatus]);
   m_wndComboS1.SelectString(-1, g_szStatusTextSmall[m_iStatusTranslation1]);
   m_wndComboS2.SelectString(-1, g_szStatusTextSmall[m_iStatusTranslation2]);
   m_wndComboS3.SelectString(-1, g_szStatusTextSmall[m_iStatusTranslation3]);
   m_wndComboS4.SelectString(-1, g_szStatusTextSmall[m_iStatusTranslation4]);

   OnPropAlgChange();
   OnCalcAlgChange();
	
	return TRUE;
}


//
// "OK" handler
//

void CObjectPropsStatus::OnOK() 
{
	CPropertyPage::OnOK();
   
   if (m_bIsModified)
   {
      m_pUpdate->dwFlags |= OBJ_UPDATE_STATUS_ALG;
      m_pUpdate->iStatusCalcAlg = m_iCalcAlg;
      m_pUpdate->iStatusPropAlg = m_iPropAlg;
      m_pUpdate->iFixedStatus = m_iFixedStatus;
      m_pUpdate->iStatusShift = m_iRelChange;
      m_pUpdate->iStatusTrans[0] = m_iStatusTranslation1;
      m_pUpdate->iStatusTrans[1] = m_iStatusTranslation2;
      m_pUpdate->iStatusTrans[2] = m_iStatusTranslation3;
      m_pUpdate->iStatusTrans[3] = m_iStatusTranslation4;
      m_pUpdate->iStatusSingleTh = (int)(m_dSingleThreshold * 100);
      m_pUpdate->iStatusThresholds[0] = (int)(m_dThreshold1 * 100);
      m_pUpdate->iStatusThresholds[1] = (int)(m_dThreshold2 * 100);
      m_pUpdate->iStatusThresholds[2] = (int)(m_dThreshold3 * 100);
      m_pUpdate->iStatusThresholds[3] = (int)(m_dThreshold4 * 100);
   }
}


//
// WM_COMMAND message handler
//

BOOL CObjectPropsStatus::OnCommand(WPARAM wParam, LPARAM lParam) 
{
   WORD wCtrl;

   switch(HIWORD(wParam))
   {
      case BN_CLICKED:
         wCtrl = LOWORD(wParam);
         if ((wCtrl == IDC_RADIO_PROP_DEFAULT) ||
             (wCtrl == IDC_RADIO_UNCHANGED) ||
             (wCtrl == IDC_RADIO_FIXED) ||
             (wCtrl == IDC_RADIO_RELATIVE) ||
             (wCtrl == IDC_RADIO_SEVERITY))
         {
            OnPropAlgChange();
         }
         else if ((wCtrl == IDC_RADIO_CALC_DEFAULT) ||
                  (wCtrl == IDC_RADIO_MOST_CRITICAL) ||
                  (wCtrl == IDC_RADIO_SINGLE) ||
                  (wCtrl == IDC_RADIO_MULTIPLE))
         {
            OnCalcAlgChange();
         }
      case CBN_SELCHANGE:
      case EN_CHANGE:
         m_bIsModified = TRUE;
         break;
      default:
         break;
   }
	return CPropertyPage::OnCommand(wParam, lParam);
}


//
// Handle propagation algorithm change
//

void CObjectPropsStatus::OnPropAlgChange()
{
   BOOL bEnable;

   EnableDlgItem(this, IDC_COMBO_FIXED, IsButtonChecked(this, IDC_RADIO_FIXED));
   EnableDlgItem(this, IDC_EDIT_RELATIVE, IsButtonChecked(this, IDC_RADIO_RELATIVE));
   bEnable = IsButtonChecked(this, IDC_RADIO_SEVERITY);
   EnableDlgItem(this, IDC_COMBO_S1, bEnable);
   EnableDlgItem(this, IDC_COMBO_S2, bEnable);
   EnableDlgItem(this, IDC_COMBO_S3, bEnable);
   EnableDlgItem(this, IDC_COMBO_S4, bEnable);
}


//
// Handle calculation algorithm change
//

void CObjectPropsStatus::OnCalcAlgChange()
{
   BOOL bEnable;

   EnableDlgItem(this, IDC_EDIT_THRESHOLD, IsButtonChecked(this, IDC_RADIO_SINGLE));
   bEnable = IsButtonChecked(this, IDC_RADIO_MULTIPLE);
   EnableDlgItem(this, IDC_EDIT_T1, bEnable);
   EnableDlgItem(this, IDC_EDIT_T2, bEnable);
   EnableDlgItem(this, IDC_EDIT_T3, bEnable);
   EnableDlgItem(this, IDC_EDIT_T4, bEnable);
}
