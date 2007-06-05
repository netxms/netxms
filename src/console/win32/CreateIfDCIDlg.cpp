// CreateIfDCIDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "CreateIfDCIDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCreateIfDCIDlg dialog


CCreateIfDCIDlg::CCreateIfDCIDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCreateIfDCIDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCreateIfDCIDlg)
	m_bInErrors = FALSE;
	m_bOutErrors = FALSE;
	m_bInBytes = FALSE;
	m_bInPackets = FALSE;
	m_bOutBytes = FALSE;
	m_bOutPackets = FALSE;
	m_strInBytes = _T("");
	m_strInErrors = _T("");
	m_strInPackets = _T("");
	m_strOutBytes = _T("");
	m_strOutErrors = _T("");
	m_strOutPackets = _T("");
	m_bDeltaInBytes = FALSE;
	m_bDeltaInErrors = FALSE;
	m_bDeltaInPackets = FALSE;
	m_bDeltaOutBytes = FALSE;
	m_bDeltaOutErrors = FALSE;
	m_bDeltaOutPackets = FALSE;
	m_dwInterval = 0;
	m_dwRetention = 0;
	//}}AFX_DATA_INIT
}


void CCreateIfDCIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCreateIfDCIDlg)
	DDX_Check(pDX, IDC_CHECK_ERRORS_IN, m_bInErrors);
	DDX_Check(pDX, IDC_CHECK_ERRORS_OUT, m_bOutErrors);
	DDX_Check(pDX, IDC_CHECK_IN, m_bInBytes);
	DDX_Check(pDX, IDC_CHECK_INP, m_bInPackets);
	DDX_Check(pDX, IDC_CHECK_OUT, m_bOutBytes);
	DDX_Check(pDX, IDC_CHECK_OUTP, m_bOutPackets);
	DDX_Text(pDX, IDC_EDIT_IN, m_strInBytes);
	DDV_MaxChars(pDX, m_strInBytes, 255);
	DDX_Text(pDX, IDC_EDIT_IN_ERRORS, m_strInErrors);
	DDV_MaxChars(pDX, m_strInErrors, 255);
	DDX_Text(pDX, IDC_EDIT_INP, m_strInPackets);
	DDV_MaxChars(pDX, m_strInPackets, 255);
	DDX_Text(pDX, IDC_EDIT_OUT, m_strOutBytes);
	DDV_MaxChars(pDX, m_strOutBytes, 255);
	DDX_Text(pDX, IDC_EDIT_OUT_ERRORS, m_strOutErrors);
	DDV_MaxChars(pDX, m_strOutErrors, 255);
	DDX_Text(pDX, IDC_EDIT_OUTP, m_strOutPackets);
	DDV_MaxChars(pDX, m_strOutPackets, 255);
	DDX_Check(pDX, IDC_CHECK_DELTA_IN, m_bDeltaInBytes);
	DDX_Check(pDX, IDC_CHECK_DELTA_INE, m_bDeltaInErrors);
	DDX_Check(pDX, IDC_CHECK_DELTA_INP, m_bDeltaInPackets);
	DDX_Check(pDX, IDC_CHECK_DELTA_OUT, m_bDeltaOutBytes);
	DDX_Check(pDX, IDC_CHECK_DELTA_OUTE, m_bDeltaOutErrors);
	DDX_Check(pDX, IDC_CHECK_DELTA_OUTP, m_bDeltaOutPackets);
	DDX_Text(pDX, IDC_EDIT_INTERVAL, m_dwInterval);
	DDV_MinMaxDWord(pDX, m_dwInterval, 2, 10000000);
	DDX_Text(pDX, IDC_EDIT_RETENTION, m_dwRetention);
	DDV_MinMaxDWord(pDX, m_dwRetention, 1, 65535);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCreateIfDCIDlg, CDialog)
	//{{AFX_MSG_MAP(CCreateIfDCIDlg)
	ON_BN_CLICKED(IDC_CHECK_OUTP, OnCheckOutp)
	ON_BN_CLICKED(IDC_CHECK_OUT, OnCheckOut)
	ON_BN_CLICKED(IDC_CHECK_INP, OnCheckInp)
	ON_BN_CLICKED(IDC_CHECK_IN, OnCheckIn)
	ON_BN_CLICKED(IDC_CHECK_ERRORS_OUT, OnCheckErrorsOut)
	ON_BN_CLICKED(IDC_CHECK_ERRORS_IN, OnCheckErrorsIn)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCreateIfDCIDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CCreateIfDCIDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	EnableControls();
	
	return TRUE;
}


//
// Enable/disable controls based on current state of checkboxes
//

void CCreateIfDCIDlg::EnableControls()
{
	EnableControl(IDC_CHECK_IN, IDC_CHECK_DELTA_IN, IDC_EDIT_IN);
	EnableControl(IDC_CHECK_OUT, IDC_CHECK_DELTA_OUT, IDC_EDIT_OUT);
	EnableControl(IDC_CHECK_INP, IDC_CHECK_DELTA_INP, IDC_EDIT_INP);
	EnableControl(IDC_CHECK_OUTP, IDC_CHECK_DELTA_OUTP, IDC_EDIT_OUTP);
	EnableControl(IDC_CHECK_ERRORS_IN, IDC_CHECK_DELTA_INE, IDC_EDIT_IN_ERRORS);
	EnableControl(IDC_CHECK_ERRORS_OUT, IDC_CHECK_DELTA_OUTE, IDC_EDIT_OUT_ERRORS);
}


//
// Enable/disable single control pair
//

void CCreateIfDCIDlg::EnableControl(int nCtrlBase, int nCtrl1, int nCtrl2)
{
	BOOL bEnable;

	bEnable = (SendDlgItemMessage(nCtrlBase, BM_GETCHECK, 0, 0) == BST_CHECKED);
	EnableDlgItem(this, nCtrl1, bEnable);
	EnableDlgItem(this, nCtrl2, bEnable);
}


//
// Checkbox handlers
//

void CCreateIfDCIDlg::OnCheckOutp() 
{
	EnableControl(IDC_CHECK_OUTP, IDC_CHECK_DELTA_OUTP, IDC_EDIT_OUTP);
}

void CCreateIfDCIDlg::OnCheckOut() 
{
	EnableControl(IDC_CHECK_OUT, IDC_CHECK_DELTA_OUT, IDC_EDIT_OUT);
}

void CCreateIfDCIDlg::OnCheckInp() 
{
	EnableControl(IDC_CHECK_INP, IDC_CHECK_DELTA_INP, IDC_EDIT_INP);
}

void CCreateIfDCIDlg::OnCheckIn() 
{
	EnableControl(IDC_CHECK_IN, IDC_CHECK_DELTA_IN, IDC_EDIT_IN);
}

void CCreateIfDCIDlg::OnCheckErrorsOut() 
{
	EnableControl(IDC_CHECK_ERRORS_OUT, IDC_CHECK_DELTA_OUTE, IDC_EDIT_OUT_ERRORS);
}

void CCreateIfDCIDlg::OnCheckErrorsIn() 
{
	EnableControl(IDC_CHECK_ERRORS_IN, IDC_CHECK_DELTA_INE, IDC_EDIT_IN_ERRORS);
}
