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
	//}}AFX_DATA_INIT
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
	DDX_Radio(pDX, IDC_RADIO_MOST_CRITICAL, m_iCalcAlg);
	DDX_Radio(pDX, IDC_RADIO_UNCHANGED, m_iPropAlg);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CObjectPropsStatus, CPropertyPage)
	//{{AFX_MSG_MAP(CObjectPropsStatus)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjectPropsStatus message handlers
