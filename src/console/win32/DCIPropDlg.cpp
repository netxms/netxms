// DCIPropDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "DCIPropDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDCIPropDlg dialog


CDCIPropDlg::CDCIPropDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDCIPropDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDCIPropDlg)
	m_iPollingInterval = 0;
	m_iRetentionTime = 0;
	m_strName = _T("");
	m_iDataType = -1;
	m_iOrigin = -1;
	m_iStatus = -1;
	//}}AFX_DATA_INIT
}


void CDCIPropDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDCIPropDlg)
	DDX_Control(pDX, IDC_COMBO_ORIGIN, m_wndOriginList);
	DDX_Control(pDX, IDC_COMBO_DT, m_wndTypeList);
	DDX_Control(pDX, IDC_BUTTON_SELECT, m_wndSelectButton);
	DDX_Text(pDX, IDC_EDIT_INTERVAL, m_iPollingInterval);
	DDV_MinMaxInt(pDX, m_iPollingInterval, 5, 100000);
	DDX_Text(pDX, IDC_EDIT_RETENTION, m_iRetentionTime);
	DDV_MinMaxInt(pDX, m_iRetentionTime, 1, 100000);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDV_MaxChars(pDX, m_strName, 255);
	DDX_CBIndex(pDX, IDC_COMBO_DT, m_iDataType);
	DDX_CBIndex(pDX, IDC_COMBO_ORIGIN, m_iOrigin);
	DDX_Radio(pDX, IDC_RADIO_ACTIVE, m_iStatus);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDCIPropDlg, CDialog)
	//{{AFX_MSG_MAP(CDCIPropDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDCIPropDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CDCIPropDlg::OnInitDialog() 
{
   DWORD i;

	CDialog::OnInitDialog();

   // Add origins
   for(i = 0; i < 3; i++)
      m_wndOriginList.AddString(g_pszItemOriginLong[i]);
   m_wndOriginList.SelectString(-1, g_pszItemOriginLong[m_iOrigin]);
	
   // Add data types
   for(i = 0; i < 4; i++)
      m_wndTypeList.AddString(g_pszItemDataType[i]);
   m_wndTypeList.SelectString(-1, g_pszItemDataType[m_iDataType]);
	
	return TRUE;
}
